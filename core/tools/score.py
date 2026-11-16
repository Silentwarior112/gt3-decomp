"""Matching score: how close the C, built with the retail compiler, is to the
retail machine code, per function and in total.

    python tools/score.py                     # every source file
    python tools/score.py src/imageloader.cpp  # some files (the symbol map of
                                               # the last full run is reused)
    python tools/score.py --diff 0x01000ED8    # side-by-side diff of one function
    python tools/score.py --diff "Buffer::resize"
    python tools/score.py --cc 991111-01 [files]   # the reference compiler, see below

--cc 991111-01 compiles with the secondary, reference compiler (Sony's Linux
ee-gcc 2.9-ee-991111-01 under tools/linuxcc.py; tools/mcc.py) instead of the
default, writes to build/score-991111-01/ and lists every function whose
result differs from build/score/score.json, the default compiler's last full
run.  It confirms that a residual difference comes from the source, not from
the compiler build (MATCHING.md).  --diff works with it too.

(Ported from the bootstrap's tools/score.py: core addresses, core paths, the
core's tools/mcc.py and tools/dataref.py.)

For each source file: compile with tools/mcc.py (ref/compile_flags.json) plus
-g, read the object's stabs to find each function's defining line, and map it
to the nearest `// FUNC 0x...` marker above that line. Then compare the
object's instructions with the retail bytes at that address:

  * instructions are aligned on their opcode/register skeleton (difflib), so
    one inserted or missing instruction does not fail the whole function;
  * an instruction under a relocation is compared with the relocated field
    masked; for jal/j the target is also checked against the retail call
    target (by the symbol map: mangled name -> retail address);
  * trailing alignment nops after the retail function are not counted.

Per function:  bytes%  = matching instructions / max(C length, retail length)
               size%   = min(lengths) / max(lengths)
               exact   = same length and every instruction matches
Totals are weighted by retail length; functions with no compiled counterpart
(failed compile, marker without code, inlined away) count as 0 in the
"whole binary" figures and are listed.

Output: build/score/score.json, build/score/report.md, and a console summary.
"""
import argparse, collections, concurrent.futures as cf, difflib, fnmatch, json, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import mcc
import v2mangle
import dataref
from elfobj import ElfObject, RELOC_FIELD, R_MIPS_26
import paths

BIN = paths.retail_elf()
OUT = os.path.join(ROOT, "build", "score")
SRC_DIRS = ["src"]
MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+0x(00[0-9A-Fa-f]{6})\b\s*(.*?)\s*(?:\*/)?\s*$")

img = open(BIN, "rb").read()
def rword(a):
    return struct.unpack_from("<I", img, 0x1000 + a - 0x00100000)[0]

funcs = {}
for _f in json.load(open(os.path.join(ROOT, "ref", "funcs.json"))):
    _s, _e = int(_f["start"], 16), int(_f["end"], 16)
    funcs[_s] = dict(_f, start=_s, end=min(_e, _s + 4 * ((_f["size"] + 3) // 4)) if _f.get("size") else _e)
units = json.load(open(os.path.join(ROOT, "ref", "units.json")))
unit_ranges = [(u, int(lo, 16), int(hi, 16)) for u, s in units.items() for lo, hi in s["ranges"]]

def unit_of(a):
    for u, lo, hi in unit_ranges:
        if lo <= a < hi:
            return u
    return "?"

# ------------------------------------------------------------------ instruction helpers

def has_delay_slot(w):
    op = w >> 26
    if op in (2, 3, 1, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17):
        return True
    if op == 0 and (w & 0x3F) in (8, 9):
        return True
    if op in (0x11, 0x12) and ((w >> 21) & 31) == 8:
        return True
    return False

def retail_words(a):
    f = funcs[a]
    ws = [rword(x) for x in range(a, f["end"], 4)]
    while len(ws) > 1 and ws[-1] == 0 and not has_delay_slot(ws[-2]):
        ws.pop()
    return ws

def skeleton(w):
    """What an instruction *is*, ignoring registers and immediates: the key the
    two streams are aligned on, so a register-allocation difference pairs the
    instructions up (and scores their equal bytes) instead of reading as an
    insertion plus a deletion."""
    op = w >> 26
    if op == 0:                                   # SPECIAL: funct
        return (0, w & 0x3F)
    if op == 1:                                   # REGIMM: rt selects the op
        return (1, (w >> 16) & 31)
    if op in (0x10, 0x11, 0x12):                  # COPn: fmt/rs + funct
        return (op, (w >> 21) & 31, w & 0x3F)
    if op == 0x1C:                                # MMI: funct + sub-op
        return (0x1C, w & 0x3F, (w >> 6) & 31)
    return (op,)

def byte_matches(o, t, field):
    """Equal bytes of two instruction words, bytes under a relocation field
    counted equal when the unrelocated bits of that byte agree."""
    n = 0
    for k in range(4):
        m = 0xFF << (8 * k)
        keep = m & ~field
        if keep == 0:
            n += 1                                 # byte entirely relocated
        elif (o & keep) == (t & keep):
            n += 1
    return n

# ------------------------------------------------------------------ source side

def markers(rel):
    out = []
    for n, line in enumerate(open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace"), 1):
        m = MARK.match(line)
        if m:
            out.append((n, int(m.group(1), 16), m.group(2)))
    return out

def sources(patterns):
    allf = []
    for d in SRC_DIRS:
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in fns:
                if fn.endswith((".c", ".cpp", ".cc", ".S")):
                    allf.append(os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/"))
    allf.sort()
    skip = set(json.load(open(os.path.join(ROOT, "ref", "compile_flags.json"))).get("not_matched", {}).get("files", []))
    allf = [f for f in allf if not f.endswith(".S")]          # hand-written assembly: not compiler output
    allf = [f for f in allf if f not in skip]
    if not patterns:
        return allf
    # a work unit's id (u042) stands for its files (tools/layout.py)
    pats = []
    for p in patterns:
        if re.fullmatch(r"u\d{3}", p):
            import layout
            pats += layout.files_of_unit(p)
        else:
            pats.append(p)
    return [f for f in allf if any(fnmatch.fnmatch(f, p) or f == p.replace("\\", "/") for p in pats)]

def obj_path(rel):
    return os.path.join(OUT, "obj", rel.replace("/", "__") + ".o")

def asm_path(rel):
    return os.path.join(OUT, "obj", rel.replace("/", "__") + ".g.s")

def compile_one(rel):
    """Object without -g (the bytes that are scored) plus -g -S assembly, whose
    .stabs directives give each function's defining line.  (With -g this
    assembler writes ECOFF .mdebug into the object, not .stab.)"""
    out = obj_path(rel)
    ok, err = mcc.compile(rel, out)
    if ok and not rel.endswith(".S"):
        ok2, err2 = mcc.compile(rel, asm_path(rel), asm=True, debug=True)
        if not ok2:
            err += err2
    return rel, ok, err

STAB = re.compile(r'^\s*\.stabs\s+"((?:[^"\\]|\\.)*)",\s*(\d+),\s*\d+,\s*(\d+)')

def stabs_from_asm(rel):
    """[(name, line, file, in_main_file)] for N_FUN stabs of the -g assembly."""
    p = asm_path(rel)
    if not os.path.exists(p):
        return []
    # The driver preprocesses to a temporary .ii, so N_SO names that file; the
    # real source arrives as N_SOL.  A function is "ours" when the current
    # file path ends with this source's path.
    want = rel.lower()
    out, cur = [], None
    for L in open(p, encoding="latin1"):
        m = STAB.match(L)
        if not m:
            continue
        s, typ, desc = m.group(1), int(m.group(2)), int(m.group(3))
        if typ in (100, 132) and s and not s.endswith("/"):   # N_SO / N_SOL
            cur = s.replace("\\\\", "/").replace("\\", "/").lower()
        elif typ == 36 and ":" in s:                          # N_FUN
            out.append((s.split(":", 1)[0], desc, cur, bool(cur) and cur.endswith(want)))
    return out

def _norm(ps):
    return [p.replace(" ", "").lower() for p in ps]

# egcs names an anonymous namespace after the file cc1plus reads plus six
# characters from that file's stat (_GLOBAL_$N$<path>.ii<6 chars>).  The
# object and the -g assembly are compiled from two preprocessed files here
# (mcc.compile), so their symbols in such a namespace differ (u119's pdi*
# RPC classes): the line mapping compares them with the name left out, and
# so does a marker that names a retail symbol (`.._.._pdicdvd.cxxs1Kop7`).
ANON_NS = re.compile(r"\d+_GLOBAL_\$N\$\S*?\.(?:ii|cxx)[A-Za-z0-9]{6}")

def _anon(sym):
    return ANON_NS.sub("<anon>", sym)

def map_functions(rel, obj):
    """-> ([(symbol, section, retail_addr, marker_name)], unmapped markers).

    1. by name: the marker text names the symbol exactly (compiler-generated
       functions are marked with their mangled name), or the marker's
       qualified name equals the symbol's GNU v2 demangled parts;
    2. by line: the symbol's defining line (stabs) -> nearest marker above.
    """
    marks = markers(rel)
    if not marks:
        return [], []
    fsyms = {y.name: (y, s) for y, s in obj.functions()}
    result, used, taken = [], {}, set()
    if rel.endswith(".S"):
        # assembly: functions in address order <-> markers in file order
        byaddr = sorted(fsyms.values(), key=lambda t: (t[1].index, t[0].value))
        for (y, s), (ln, a, nm) in zip(byaddr, marks):
            result.append((y, s, a, nm))
        return result, []

    def take(sym, m):
        y, s = fsyms[sym]
        used[m[1]] = sym
        taken.add(sym)
        result.append((y, s, m[1], m[2]))

    anon = collections.defaultdict(list)            # symbols by name without their anonymous namespace
    for s in fsyms:
        anon[_anon(s)].append(s)
    # 1a. exact symbol name in the marker text
    for m in marks:
        # dotted names first (egcs anonymous-namespace symbols such as
        # __tfQ233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget), then plain
        # ones: any before the first '(', only mangled-looking ones inside it
        # (a comment's "of vsprintf.cxx" once mapped vsprintf to the marker)
        head, _, rest = m[2].partition("(")
        toks = (re.findall(r"[A-Za-z_$][\w$]*(?:\.+[\w$]+)+", m[2]) + re.findall(r"[A-Za-z_$][\w$]*", head) +
                [t for t in re.findall(r"[A-Za-z_$][\w$]*", rest) if "$" in t or "__" in t or t.startswith("_")])
        for tok in toks:
            if tok not in fsyms and len(anon.get(_anon(tok), ())) == 1:
                tok = anon[_anon(tok)][0]
            if tok in fsyms and tok not in taken and m[1] not in used:
                take(tok, m)
                break
    # 1b. demangled qualified name
    sym_parts = {}
    for sym in fsyms:
        if sym not in taken:
            sym_parts.setdefault(tuple(_norm(v2mangle.parts(sym))), []).append(sym)
    for m in marks:
        if m[1] in used:
            continue
        mp = tuple(_norm(v2mangle.marker_parts(m[2])))
        cands = [s for s in sym_parts.get(mp, []) if s not in taken]
        if len(cands) == 1:
            take(cands[0], m)
    # 2. defining line
    lines = [(n, ln) for n, ln, f, main in stabs_from_asm(rel) if main]
    for name, ln in sorted(lines, key=lambda t: t[1]):
        if name not in fsyms and len(anon.get(_anon(name), ())) == 1:
            name = anon[_anon(name)][0]             # anonymous namespace (see _anon)
        if name not in fsyms or name in taken:
            continue
        cand = [m for m in marks if m[0] <= ln]
        if not cand:
            continue
        m = cand[-1]
        if m[1] in used:
            continue
        take(name, m)
    orphans = [m for m in marks if m[1] not in used]
    return result, orphans

# ------------------------------------------------------------------ comparison

def compare(obj, y, sec, raddr, symmap, localmap):
    ow = [struct.unpack_from("<I", sec.data, y.value + 4 * i)[0] for i in range(y.size // 4)]
    # the same trailing-nop trim as retail_words (only matters for padding /
    # assembly symbols that end in nops outside a delay slot)
    while len(ow) > 1 and ow[-1] == 0 and not has_delay_slot(ow[-2]):
        ow.pop()
    rw = retail_words(raddr)
    rel = {}
    for r in obj.relocs.get(sec.index, []):
        if y.value <= r.offset < y.value + y.size:
            rel[(r.offset - y.value) // 4] = r
    sm = difflib.SequenceMatcher(None, [skeleton(w) for w in ow], [skeleton(w) for w in rw], autojunk=False)
    pairs, matched, mbytes, bad_targets = [], 0, 0, 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                i, j = i1 + k, j1 + k
                o, t = ow[i], rw[j]
                r = rel.get(i)
                field = RELOC_FIELD.get(r.type, 0xFFFFFFFF) if r is not None else 0
                ok = (o & ~field) == (t & ~field)
                nb = byte_matches(o, t, field)
                if ok and r is not None and r.type == R_MIPS_26 and r.sym is not None:
                    if r.sym.type == 3:                         # section symbol: local call
                        # keyed by (section, offset): with -ffunction-sections every
                        # function sits at offset 0 of its own section
                        want = localmap.get((r.sym.shndx, (o & 0x03FFFFFF) << 2))
                    elif r.sym.bind == 2:                       # weak (egcs's inline / typeinfo copies)
                        # the linker resolves every call to the first copy linked: the
                        # lowest retail address of that name, not this object's copy
                        want = weak_first().get(r.sym.name)
                        if want is None and _template_sym(r.sym.name):
                            # a template instance: the qualified parts below drop its
                            # arguments (Cell<OperatorHolder> would get Cell<GarageCar>'s
                            # copy); its exact name in the symmap (lowest copy) instead
                            want = symmap.get(r.sym.name)
                        if want is None:                        # markers with a qualified name
                            want = weak_first_parts().get(tuple(_norm(v2mangle.parts(r.sym.name))))
                        if want is None:
                            want = symmap.get(r.sym.name)
                    else:
                        want = symmap.get(r.sym.name)
                    got = ((t & 0x03FFFFFF) << 2) | (raddr & 0xF0000000)
                    if want is not None and want != got:
                        ok = False
                        nb = 1                                  # opcode byte only
                        bad_targets += 1
                matched += ok
                mbytes += nb
                pairs.append((i, j, ok))
        else:
            for i in range(i1, i2):
                pairs.append((i, None, False))
            for j in range(j1, j2):
                pairs.append((None, j, False))
    lo, lr = len(ow), len(rw)
    exact = lo == lr and matched == lr
    return dict(ow=ow, rw=rw, rel=rel, pairs=pairs, matched=matched, mbytes=mbytes, lo=lo, lr=lr, exact=exact,
                insn_pct=100.0 * matched / max(lo, lr, 1), bytes_pct=100.0 * mbytes / (4 * max(lo, lr, 1)),
                size_pct=100.0 * min(lo, lr) / max(lo, lr, 1), bad_targets=bad_targets)

# ------------------------------------------------------------------ driver

def symmap_path():
    return os.path.join(OUT, "symmap.json")

def seed_symmap():
    m = {}
    for rel in sources([]):
        for ln, a, nm in markers(rel):
            if re.match(r"^[A-Za-z_]\w*$", nm):
                m.setdefault(nm, a)
    return m

_WEAK_FIRST = None

def _template_sym(sym):
    """GNU v2 symbol of a member of a template instance (t<len><name>... in its
    class): weak_first_parts drops template arguments, so all instances of
    one template would share an address."""
    if sym.startswith("_$_"):
        i = 3
    elif sym.startswith("__tf"):
        i = 4
    elif sym.startswith("__") and len(sym) > 2 and (sym[2].isdigit() or sym[2] in "Qt"):
        i = 2
    else:
        k = sym.find("__", 1)
        if k <= 0:
            return False
        i = k + 2
        if sym[i:i + 1] == "C":
            i += 1
    if sym[i:i + 1] == "Q":
        if sym[i + 1:i + 2] == "_":
            cnt, j = v2mangle._num(sym, i + 2)
            j += 1
        else:
            cnt, j = int(sym[i + 1]), i + 2
    else:
        cnt, j = 1, i
    for _ in range(cnt or 0):
        if sym[j:j + 1] == "t":
            return True
        n, j2 = v2mangle._num(sym, j)
        if n is None:
            return False
        j = j2 + n
    return False

def weak_first():
    """Marker name -> its lowest address.  For a weak symbol (an inline copy,
    a typeinfo function) every retail call goes to the first copy linked, the
    lowest of the copies' addresses; the symmap holds whichever copy was
    mapped last (u013's notes: T096's __tf9GarageCar calls T036's
    __tfQ29gt2k_menut4Cell1Z9GarageCar 0x00125B00)."""
    global _WEAK_FIRST
    if _WEAK_FIRST is None:
        m = {}
        for rel in sources([]):
            for ln, a, nm in markers(rel):
                if re.match(r"^[A-Za-z_]\w*$", nm):
                    m[nm] = min(m.get(nm, a), a)
        _WEAK_FIRST = m
    return _WEAK_FIRST

_WEAK_PARTS = None

def weak_first_parts():
    """Qualified-name parts of a marker -> the lowest address of those parts:
    weak_first for weak copies whose markers name them as Class::method
    (u049: T146's Color8::setR copy 0x0018BE38 is never called; T146's calls
    reach T188's 0x00174AE0, the copy linked first).  Overloads of one name
    are not told apart (weak overloaded copies: none found so far)."""
    global _WEAK_PARTS
    if _WEAK_PARTS is None:
        m = {}
        for rel in sources([]):
            for ln, a, nm in markers(rel):
                k = tuple(_norm(v2mangle.marker_parts(nm)))
                if k:
                    m[k] = min(m.get(k, a), a)
        _WEAK_PARTS = m
    return _WEAK_PARTS

LAST_LAYOUT = {}                                 # rel -> (ElfObject, {section index: retail base})

def run(patterns, jobs):
    rels = sources(patterns)
    os.makedirs(OUT, exist_ok=True)
    with cf.ThreadPoolExecutor(max_workers=jobs) as ex:
        results = list(ex.map(compile_one, rels))
    symmap = seed_symmap()
    if patterns and os.path.exists(symmap_path()):
        symmap.update(json.load(open(symmap_path())))
    mapped = {}
    mapped_weak = set()
    failed, orphans_all = {}, {}
    for rel, ok, err in results:
        if not ok:
            failed[rel] = err
            continue
        obj = ElfObject(obj_path(rel))
        mp, orph = map_functions(rel, obj)
        mapped[rel] = (obj, mp)
        orphans_all[rel] = orph
        for y, s, a, nm in mp:
            if y.bind == 2 and y.name in mapped_weak:
                # weak copies (inline functions, typeinfo functions) of several
                # objects: calls reach the first one linked, the lowest address
                symmap[y.name] = min(symmap[y.name], a)
            else:
                symmap[y.name] = a
            if y.bind == 2:
                mapped_weak.add(y.name)
    if not patterns:
        json.dump(symmap, open(symmap_path(), "w"), indent=0, sort_keys=True)

    records = []
    layout = {}                                      # rel -> {section: [inferred retail base, size]}
    for rel, (obj, mp) in mapped.items():
        localmap = {}
        for y, s, a, nm in mp:
            localmap[(s.index, y.value)] = a
        # object section -> inferred retail base: its named data's declared
        # addresses first (dataref.named_bases), else the first reference
        bases = dataref.named_bases(obj)
        layout[rel] = bases
        for y, s, a, nm in sorted(mp, key=lambda t: t[2]):
            if a not in funcs:
                continue
            c = compare(obj, y, s, a, symmap, localmap)
            dr = dataref.verify(obj, y, s, c["pairs"], c["ow"], c["rw"], symmap, bases)
            records.append(dict(addr=a, name=nm, symbol=y.name, file=rel, unit=unit_of(a),
                                c_insns=c["lo"], retail_insns=c["lr"], matched=c["matched"], mbytes=c["mbytes"],
                                insn_pct=round(c["insn_pct"], 2), bytes_pct=round(c["bytes_pct"], 2),
                                size_pct=round(c["size_pct"], 2),
                                exact=c["exact"], bad_call_targets=c["bad_targets"],
                                data_refs_ok=dr["verified"], data_refs_bad=dr["mismatch"],
                                data_refs_unresolved=dr["unresolved"], data_notes=dr["notes"][:4],
                                exact_with_data=c["exact"] and dr["mismatch"] == 0))
    # where each object's data sections sat in retail (from its references):
    # build/score/layout.json, for tools/datalayout.py (full runs only);
    # LAST_LAYOUT for tools/datacheck.py (any run)
    global LAST_LAYOUT
    LAST_LAYOUT = {rel: (mapped[rel][0], layout.get(rel, {})) for rel in mapped}
    if not patterns:
        out = {}
        for rel, (obj, mp) in mapped.items():
            b = layout.get(rel, {})
            out[rel] = {obj.sections[k].name: [v, obj.sections[k].size] for k, v in b.items()
                        if 0 <= k < len(obj.sections)}
            for sec in obj.sections:                 # sizes of the data sections never referenced too
                if sec.name in (".rodata", ".data", ".sdata", ".sbss", ".bss", ".gcc_except_table",
                                ".lit4", ".lit8") and sec.name not in out[rel] and sec.size:
                    out[rel][sec.name] = [None, sec.size]
        json.dump(out, open(os.path.join(OUT, "layout.json"), "w"), indent=0, sort_keys=True)
    return rels, records, failed, orphans_all

LIB_UNITS = ("libgcc_eh", "frame", "cxxrt", "libgcc_math")

def library_records(have):
    """Retail runtime functions that are byte-identical to the toolchain's own
    libgcc.a / libstdc++.a (ref/libmatch.json, from tools/match_lib.py): the
    original link took them from those archives, and so does a matching build."""
    p = os.path.join(ROOT, "ref", "toolchain_libmatch.json")
    if not os.path.exists(p):
        return []
    out = []
    for k, (lib, member, sym, n) in json.load(open(p)).items():
        a = int(k, 16)
        if a in have or unit_of(a) not in LIB_UNITS:
            continue
        lr = len(retail_words(a))
        out.append(dict(addr=a, name=sym, symbol=sym, file="toolchain:%s(%s)" % (lib, member), unit=unit_of(a),
                        c_insns=lr, retail_insns=lr, matched=lr, mbytes=4 * lr, insn_pct=100.0, bytes_pct=100.0,
                        size_pct=100.0, exact=True, bad_call_targets=0, data_refs_ok=0, data_refs_bad=0,
                        data_refs_unresolved=0, data_notes=[], exact_with_data=True))
    return out

def summarize(rels, records, failed, orphans_all, scope_all):
    by_addr = {r["addr"]: r for r in records}
    scope = [a for a in funcs if unit_of(a) != "?"] if scope_all else sorted({r["addr"] for r in records} |
            {m[1] for rel in rels for m in markers(rel)})
    tot_r = sum(len(retail_words(a)) for a in scope)
    comp = [by_addr[a] for a in scope if a in by_addr]
    m_ins = sum(r["matched"] for r in comp)
    m_bytes = sum(r["mbytes"] for r in comp)
    mx = sum(max(r["c_insns"], r["retail_insns"]) for r in comp)
    mn = sum(min(r["c_insns"], r["retail_insns"]) for r in comp)
    rsum = sum(r["retail_insns"] for r in comp)
    csum = sum(r["c_insns"] for r in comp)
    whole = tot_r + (mx - rsum)          # every retail insn, plus C's extra length where C is longer
    s = collections.OrderedDict([
        ("functions_in_scope", len(scope)),
        ("functions_compiled", len(comp)),
        ("functions_exact", sum(1 for r in comp if r["exact"])),
        ("functions_exact_with_data", sum(1 for r in comp if r.get("exact_with_data"))),
        ("retail_insns_in_scope", tot_r),
        ("bytes_pct_of_compiled", round(100.0 * m_bytes / max(4 * mx, 1), 2)),
        ("insn_pct_of_compiled", round(100.0 * m_ins / max(mx, 1), 2)),
        ("size_pct_of_compiled", round(100.0 * mn / max(mx, 1), 2)),
        ("total_size_ratio_pct", round(100.0 * csum / max(rsum, 1), 2)),
        ("bytes_pct_of_scope", round(100.0 * m_bytes / max(4 * whole, 1), 2)),
        ("size_pct_of_scope", round(100.0 * mn / max(whole, 1), 2)),
        ("exact_insns_pct_of_scope", round(100.0 * sum(r["retail_insns"] for r in comp if r["exact"]) / max(tot_r, 1), 2)),
        ("files_failed", sorted(failed)),
    ])
    return s

def write_report(summary, records, failed, orphans_all):
    os.makedirs(OUT, exist_ok=True)
    json.dump(dict(summary=summary, functions=sorted(records, key=lambda r: r["addr"])),
              open(os.path.join(OUT, "score.json"), "w"), indent=1)
    per = collections.OrderedDict()
    for u in units:
        per[u] = [r for r in records if r["unit"] == u]
    L = ["# Matching score", "",
         "| | |", "|---|---|"]
    for k, v in summary.items():
        if k != "files_failed":
            L.append("| %s | %s |" % (k, v))
    L += ["", "## Per unit", "",
          "bytes % = equal bytes over aligned instructions (relocated fields masked), "
          "insns % = whole instructions equal, both over max(C, retail) length of the compiled functions.", "",
          "| unit | functions (compiled/total) | exact | bytes % | insns % | size % |", "|---|---|---|---|---|---|"]
    for u, rs in per.items():
        total = sum(1 for a in funcs if unit_of(a) == u)
        mx = sum(max(r["c_insns"], r["retail_insns"]) for r in rs)
        m = sum(r["matched"] for r in rs)
        mb = sum(r["mbytes"] for r in rs)
        mn = sum(min(r["c_insns"], r["retail_insns"]) for r in rs)
        L.append("| %s | %d/%d | %d | %.1f | %.1f | %.1f |" % (u, len(rs), total, sum(1 for r in rs if r["exact"]),
                                                             100.0 * mb / max(4 * mx, 1), 100.0 * m / max(mx, 1),
                                                             100.0 * mn / max(mx, 1)))
    if failed:
        L += ["", "## Files that do not compile with the matching compiler", ""]
        for rel, err in sorted(failed.items()):
            first = "; ".join(l.strip() for l in err.splitlines() if l.strip())[:300]
            L.append("- `%s`: %s" % (rel, first))
    L += ["", "## Least matching compiled functions", "",
          "| addr | function | file | C/retail insns | bytes % |", "|---|---|---|---|---|"]
    for r in sorted(records, key=lambda r: (r["bytes_pct"], -r["retail_insns"]))[:40]:
        L.append("| 0x%08X | %s | %s | %d/%d | %.1f |" % (r["addr"], r["name"][:50], r["file"], r["c_insns"],
                                                         r["retail_insns"], r["bytes_pct"]))
    orph = [(rel, m) for rel, ms in orphans_all.items() for m in ms]
    if orph:
        L += ["", "## Markers with no compiled function (%d)" % len(orph), ""]
        for rel, (ln, a, nm) in orph[:200]:
            L.append("- 0x%08X %s (%s:%d)" % (a, nm, rel, ln))
    open(os.path.join(OUT, "report.md"), "w", encoding="utf-8").write("\n".join(L) + "\n")

def show_diff(key, jobs):
    target = None
    try:
        target = int(key, 16)
    except ValueError:
        pass
    rels = sources([])
    cands = []
    for rel in rels:
        for ln, a, nm in markers(rel):
            if a == target or (target is None and key in nm):
                cands.append(rel)
    if not cands:
        print("no FUNC marker matches %r" % key); return 1
    rel = cands[0]
    _, ok, err = compile_one(rel)
    if not ok:
        print(err); return 1
    obj = ElfObject(obj_path(rel))
    mp, _ = map_functions(rel, obj)
    symmap = seed_symmap()
    if os.path.exists(symmap_path()):
        symmap.update(json.load(open(symmap_path())))
    localmap = {(s.index, y.value): a for y, s, a, nm in mp}
    for y, s, a, nm in mp:
        if a == target or (target is None and key in nm):
            c = compare(obj, y, s, a, symmap, localmap)
            print("%s  0x%08X  %s  (%s)   C %d insns / retail %d   bytes %.1f%%  size %.1f%%%s" % (
                rel, a, nm, y.name, c["lo"], c["lr"], c["bytes_pct"], c["size_pct"], "  EXACT" if c["exact"] else ""))
            try:
                import rabbitizer
                def dis(w, addr):
                    return rabbitizer.Instruction(w, vram=addr, category=rabbitizer.InstrCategory.R5900).disassemble()
            except Exception:
                def dis(w, addr):
                    return "%08X" % w
            for i, j, ok in c["pairs"]:
                left = "%08X  %-34s" % (c["ow"][i], dis(c["ow"][i], a + 4 * i)[:34]) if i is not None else " " * 44
                right = "%08X  %s" % (c["rw"][j], dis(c["rw"][j], a + 4 * j)) if j is not None else ""
                rl = c["rel"].get(i) if i is not None else None
                tag = "  " if ok else ("<>" if i is not None and j is not None else ("- " if j is None else "+ "))
                rs = (" [%s]" % (rl.sym.name if rl and rl.sym else "")) if rl else ""
                print("%s %s | %s%s" % (tag, left, right, rs))
            return 0
    print("function at %r is not mapped from %s" % (key, rel)); return 1

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--diff", metavar="ADDR|NAME")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4)))
    ap.add_argument("--cc", metavar="NAME",
                    help="score with a secondary compiler (%s) into build/score-NAME/" % ", ".join(mcc.SECONDARY))
    a = ap.parse_args()
    global OUT
    base_json = os.path.join(OUT, "score.json")
    if a.cc:
        if a.cc not in mcc.SECONDARY:
            sys.exit("unknown compiler %s (known: %s)" % (a.cc, ", ".join(mcc.SECONDARY)))
        mcc.use_cc = a.cc
        base_symmap = symmap_path()
        OUT = os.path.join(ROOT, "build", "score-" + a.cc)
        os.makedirs(OUT, exist_ok=True)
        if not os.path.exists(symmap_path()) and os.path.exists(base_symmap):
            import shutil
            shutil.copy(base_symmap, symmap_path())
    if a.diff:
        return show_diff(a.diff, a.jobs)
    rels, records, failed, orphans = run(a.files, a.jobs)
    if not a.files:
        # runtime units: the toolchain library is the source of the retail code
        lib = library_records(set())
        libaddrs = {r["addr"] for r in lib}
        records = [r for r in records if r["addr"] not in libaddrs] + lib
    summary = summarize(rels, records, failed, orphans, scope_all=not a.files)
    if not a.files:
        write_report(summary, records, failed, orphans)     # partial runs leave the shared report alone
    else:
        for r in sorted(records, key=lambda r: r["addr"]):
            print("0x%08X %-44s %4d/%-4d bytes %5.1f%%  size %5.1f%%%s%s" % (
                r["addr"], r["name"][:44], r["c_insns"], r["retail_insns"], r["bytes_pct"], r["size_pct"],
                "  EXACT" if r["exact"] else "",
                ("  data-refs: %d differ (%s)" % (r["data_refs_bad"], r["data_notes"][0][:90])) if r["data_refs_bad"] else ""))
        for rel, ms in orphans.items():
            for ln, addr, nm in ms:
                print("0x%08X %-44s  not compiled / not mapped (%s:%d)" % (addr, nm[:44], rel, ln))
        for rel, err in failed.items():
            print("FAIL %s\n%s" % (rel, err[-2000:]))
    for k, v in summary.items():
        if k == "files_failed":
            print("%-28s %d%s" % (k, len(v), (": " + ", ".join(v[:8]) + (" ..." if len(v) > 8 else "")) if v else ""))
        else:
            print("%-28s %s" % (k, v))
    if not a.files:
        print("report: %s" % os.path.join(OUT, "report.md"))
    if a.cc:
        compare_with_default(records, base_json, a.cc)
    return 0

def compare_with_default(records, base_json, cc):
    """List the functions whose result under compiler cc differs from the
    default compiler's last full run."""
    try:
        base = {r["addr"]: r for r in json.load(open(base_json))["functions"]}
    except (OSError, ValueError, KeyError):
        print("no %s: run python tools/score.py first to compare" % base_json)
        return
    diffs = []
    for r in sorted(records, key=lambda r: r["addr"]):
        b = base.get(r["addr"])
        if b is None or b.get("bytes_pct") is None or r.get("bytes_pct") is None:
            continue
        if r["exact"] != b["exact"] or abs(r["bytes_pct"] - b["bytes_pct"]) > 0.05:
            diffs.append((r, b))
    print("%s vs default: %d of %d functions differ" % (cc, len(diffs), len(records)))
    for r, b in diffs:
        print("  0x%08X %-44s default %5.1f%%%s -> %s %5.1f%%%s" % (
            r["addr"], r["name"][:44], b["bytes_pct"], " EXACT" if b["exact"] else "", cc,
            r["bytes_pct"], " EXACT" if r["exact"] else ""))

if __name__ == "__main__":
    sys.exit(main())
