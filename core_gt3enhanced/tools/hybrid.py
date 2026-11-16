"""Link the hybrid program: the original code and data at their own addresses,
with the functions of chosen work units taken from the compiled C.

    python tools/hybrid.py                         what hybrid.txt says (the build's choice)
    python tools/hybrid.py --units all             every unit's C (period build objects)
    python tools/hybrid.py --units u001,u010-u020  some units
    python tools/hybrid.py --units none            no C: must give the original image
    python tools/hybrid.py --units all --keep-asm 0x00260D48,...   some functions stay original

Inputs: a build of tools/build.py --compiler 991111b (its obj/ with the
*.funcs.json function lists) and its relink of the split (<build>/link/).

How a unit's C goes in:
  * A unit whose C provides every one of its functions replaces its original
    code: the C goes into the unit's own slot when it fits (else with the
    rest of the C), and the unit's other labels (jump-table targets the
    original data still names, used only by functions that are C now) keep
    their original addresses.  --overlay-only turns this off.
  * Otherwise (the overlay) the unit's original assembly stays linked at its
    addresses.  The symbols
    of the functions its C defines are made local in a copy of the assembly
    object, and the global names point at the C functions instead (bridge.ld):
    every call from other units, every vtable and function table in the data,
    reaches the C.  What the C does not emit (inline copies, typeinfo
    functions, implicit destructors) and calls inside the unit's own
    assembly keep the original code.
  * The C's code and data go after the original .bss: .c_bss first (crt0
    clears it with the original .bss: its loop end is patched to __bss_end),
    then .c_text, .c_rodata, .c_data.  The heap starts after them: the
    original's "end of the program" symbol D_4B2C0C (the heap base word at
    0x003531D0, InitHeap's argument) becomes __heap_start.
  * The eh_tables region of the split (0x0028E760..0x0030729C: frame
    records, and between them the constructor list and ordinary tables) is
    linked as assembly whose code pointers are symbols, so constructors and
    table calls reach the C too (the frame records' pc_begin stay numbers).
  * What the C uses but no linked object defines is given its original
    address (bridge.ld): functions by the function map (tools/funcmap.py) or
    their FUNC marker's name, data by the address in its name (g_0030A898)
    or by ref/data_symbols.csv and ref/symbols.csv.  Data and the code left
    in assembly never move, so original addresses are right.

Output in <build>/hybrid/: hybrid.ld, bridge.ld, core.elf, core.map,
report.txt (what was redirected, what is unresolved, data the C defines that
the original also has), and with --pack a signed core.gt3.
"""
import argparse
import collections
import concurrent.futures as cf
import csv
import glob
import json
import os
import re
import struct
import subprocess
import sys
import time

import funcmap
import gt3image
import layout
import pass2_names
from elfobj import ElfObject
import paths
import relink

ADDR_IN_NAME = re.compile(r"(?:^|[^0-9A-Za-z])(?:g|D|F|func|B|R)_([0-9A-F]{8})(?![0-9A-Fa-f])")
HEAP_SYM = "D_4B2C0C"           # the original's end of the program: heap base, InitHeap, bss end
ORIG_END = 0x004B2C0C
CRT0_CLEAR = ("0010000C", "00100014")     # crt0's bss-clear loop end (%hi / %lo of HEAP_SYM)
TC_BIN = None


def tool(name):
    return os.path.join(TC_BIN, paths.EE_PREFIX + name + paths.EXE)


def run(cmd, cwd=None):
    return subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=cwd)


NM_CACHE = {}


def nm_all(paths_, *flags):
    """nm of many files at once (threads), into the cache nm() reads."""
    todo = [p for p in paths_ if (p, flags) not in NM_CACHE]
    with cf.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        for p, out in zip(todo, ex.map(lambda q: _nm(q, flags), todo)):
            NM_CACHE[(p, flags)] = out


def nm(path, *flags):
    """[(value, type, name)] of nm's output."""
    if (path, flags) not in NM_CACHE:
        NM_CACHE[(path, flags)] = _nm(path, flags)
    return NM_CACHE[(path, flags)]


def _nm(path, flags):
    r = run([tool("nm")] + list(flags) + [path])
    out = []
    for L in r.stdout.splitlines():
        p = L.split()
        if len(p) == 3:
            out.append((int(p[0], 16), p[1], p[2]))
        elif len(p) == 2:
            out.append((None, p[0], p[1]))
    return out


def unit_ranges():
    units = json.load(open(os.path.join(paths.REF, "units.json")))
    return [(u, int(s["ranges"][0][0], 16), int(s["ranges"][0][1], 16)) for u, s in units.items()]


def parse_units(spec, all_units):
    if spec in ("", "none"):
        return []
    if spec == "all":
        return list(all_units)
    out = []
    for part in spec.split(","):
        part = part.strip()
        if "-" in part:
            a, b = part.split("-")
            out += [u for u in all_units if a <= u <= b]
        elif part:
            out.append(part)
    return [u for u in all_units if u in set(out)]


CONFIG = os.path.join(paths.ROOT, "hybrid.txt")


def read_config(path, all_units):
    """(units, keep) of a hybrid.txt: the units linked from C, and the
    functions kept as the original code."""
    units, drop, keep = [], set(), set()
    for L in open(path, encoding="utf-8"):
        L = L.split("#")[0].strip()
        if not L:
            continue
        if L.startswith("keep "):
            keep.add(int(L.split()[1], 16))
        elif L.startswith("not "):
            drop |= set(parse_units(L[4:].strip(), all_units))
        else:
            units += [u for u in parse_units(L, all_units) if u not in units]
    return [u for u in all_units if u in units and u not in drop], keep


def vtables():
    """{egcs vtable symbol: address} from ref/classes.json: _vt$<class> is the
    class's primary vtable, _vt$<class>$<base> its secondary one (when it has
    exactly one).  A template's identical copies: the first."""
    out = {}
    for c in json.load(open(os.path.join(paths.REF, "classes.json"))).values():
        prim = [int(v["addr"], 16) for v in c.get("vtables", []) if v.get("kind") == "primary"]
        sec = [int(v["addr"], 16) for v in c.get("vtables", []) if v.get("kind") == "secondary"]
        if prim:
            out["_vt$" + c["mangled"]] = prim[0]
        if len(sec) == 1:
            out["_vt$%s$*" % c["mangled"]] = sec[0]
    return out


def table_names():
    """{name: address} from ref/symbols.csv and ref/data_symbols.csv."""
    out = {}
    for fn in ("data_symbols.csv", "symbols.csv"):
        p = os.path.join(paths.REF, fn)
        if os.path.exists(p):
            for r in csv.DictReader(open(p, encoding="utf-8")):
                try:
                    out.setdefault(r["name"].split("(")[0].strip(), int(r["addr"], 16))
                except (KeyError, ValueError):
                    pass
    return out


def egcs_static(qual):
    """egcs's symbol for a qualified variable: A::b -> _1A$b, N::A::b -> _Q21N1A$b."""
    parts = qual.split("::")
    scope, member = parts[:-1], parts[-1]
    if len(scope) == 1:
        return "_%d%s$%s" % (len(scope[0]), scope[0], member)
    return "_Q%d%s$%s" % (len(scope), "".join("%d%s" % (len(x), x) for x in scope), member)


def declared():
    """{symbol: address} of every declaration with an address comment under
    include/ and src/ (tools/pass2_names.py's scanner); qualified variables
    under their egcs symbol.  Names declared at two addresses are left out."""
    seen = collections.defaultdict(set)
    for top in ("include", "src"):
        for dp, _, fns in os.walk(os.path.join(paths.ROOT, top)):
            for fn in fns:
                if fn.endswith((".h", ".c", ".cpp")):
                    for addr, kind, name, _, _ in pass2_names.scan(os.path.join(dp, fn)):
                        a = int(addr, 16)
                        if kind == "global" and "::" in name:
                            seen[egcs_static(name)].add(a)
                        elif "::" not in name:
                            seen[name].add(a)
    return {n: next(iter(a)) for n, a in seen.items() if len(a) == 1}


def extern_line(name):
    """The address on `name`'s own extern declaration line, for declarations
    the scanner does not parse (an __attribute__ after the name, a function
    pointer): `extern ... name ...; /* 0x00XXXXXX`.  None when not exactly one."""
    pat = re.compile(r"^\s*extern\b[^;]*\b%s\b[^;]*;\s*/[*/]\s*0x(00[1-4][0-9A-Fa-f]{5})" % re.escape(name))
    found = set()
    for top in ("include", "src"):
        for dp, _, fns in os.walk(os.path.join(paths.ROOT, top)):
            for fn in fns:
                if fn.endswith((".h", ".c", ".cpp")):
                    for L in open(os.path.join(dp, fn), encoding="utf-8", errors="replace"):
                        if name in L:
                            m = pat.match(L)
                            if m:
                                found.add(int(m.group(1), 16))
    return found.pop() if len(found) == 1 else None


# functions egcs calls without -ffast-math that the original program has none of
# (its code never calls them): given to the hybrid link as what -ffast-math inlines
SUPPORT = {
    "sqrtf": ".globl sqrtf\n.ent sqrtf\nsqrtf:\n    jr $31\n    sqrt.s $f0, $f12\n.end sqrtf\n",
}


# what the original's linker defined for crt0 (src/runtime/startup/crt0.S lists them)
CRT0_SYMBOLS = {"_fbss": 0x00353200, "_stack": 0x01F00000, "_stack_size": 0x00100000,
                "_heap_size": 0xFFFFFFFF, "_args": 0x00353580}


EH_ONLY = None                  # (lo, hi): make only these words symbolic (bisecting)
EH_BASE = 0x0028E760            # the split's eh_tables blob: frame records, and between them
FDE_TABLE = 0x0028E780          # the constructor list and ordinary tables


def assemble_eh_tables(blob_path, names, starts, dst):
    """The eh_tables blob as assembly: every word that is a function's start
    becomes a reference to the function's symbol (the constructor list
    __CTOR_LIST__ at 0x0028F020 and the tables between the frame blocks call
    and name functions by address), except the frame records' pc_begin
    fields, which describe the original code.  With no unit from C, the
    bytes are the blob's."""
    blob = open(blob_path, "rb").read()

    def word(a):
        return struct.unpack_from("<I", blob, a - EH_BASE)[0]
    keep = set()
    a = FDE_TABLE
    while EH_BASE <= a < EH_BASE + len(blob) and word(a) and EH_BASE <= word(a) < EH_BASE + len(blob):
        p = word(a)
        while EH_BASE <= p < EH_BASE + len(blob) - 8:
            ln = word(p)
            if not ln or ln > 0x10000:
                break
            if word(p + 4) != 0:
                keep.add(p + 8)                         # an FDE's pc_begin
            p += 4 + ln
        a += 4
    lines, n = [".section .data", ".set noreorder"], 0
    for off in range(0, len(blob) - len(blob) % 4, 4):
        va, v = EH_BASE + off, struct.unpack_from("<I", blob, off)[0]
        if v in starts and va not in keep and v in names and (EH_ONLY is None or EH_ONLY[0] <= va < EH_ONLY[1]):
            lines.append(".word %s" % names[v])
            n += 1
        else:
            lines.append(".word 0x%08X" % v)
    for b in blob[len(blob) - len(blob) % 4:]:
        lines.append(".byte 0x%02X" % b)
    src = dst[:-2]
    os.makedirs(os.path.dirname(src), exist_ok=True)
    with open(src, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    r = run([tool("as")] + relink.AS_FLAGS + ["-o", dst, src])
    if r.returncode:
        raise SystemExit("assembling the eh_tables region failed:\n" + r.stderr[-2000:])
    return n


R_32, R_26, R_HI16, R_LO16 = 2, 4, 5, 6


def retarget(src, dst, redirected_data):
    """Copy of a C object whose section-relative references to its own global
    symbols name those symbols instead.  egcs's assembler writes a reference
    to something defined in the same file as "section + offset", which no
    symbol assignment of the link can redirect: a call to a function the
    hybrid keeps original, or an access to a variable it points at the
    original's copy, would still reach the C's own.  Rewritten: R_MIPS_26
    calls that land exactly on a global function, and %hi/%lo pairs and
    32-bit words that point inside a variable of `redirected_data`.  Returns
    the number of relocations rewritten."""
    d = bytearray(open(src, "rb").read())
    shoff, = struct.unpack_from("<I", d, 0x20)
    shentsize, shnum, _ = struct.unpack_from("<HHH", d, 0x2E)
    secs = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    symtab = next((i for i, s in enumerate(secs) if s[1] == 2), None)
    if symtab is None:
        return 0
    so, ss, strsec = secs[symtab][4], secs[symtab][5], secs[symtab][6]
    stro = secs[strsec][4]

    def cstr(o):
        e = d.index(0, o)
        return d[o:e].decode("latin1")
    syms = []                                   # (name, value, size, bind, type, shndx)
    for k in range(ss // 16):
        n, v, sz, info, _, sh = struct.unpack_from("<IIIBBH", d, so + 16 * k)
        syms.append((cstr(stro + n), v, sz, info >> 4, info & 0xF, sh))
    funcs = {}                                  # (section, offset) -> symbol index
    data = collections.defaultdict(list)        # section -> [(value, size, symbol index)]
    # egcs gives a file-static (.lcomm / local label) no size: it reaches to
    # the next symbol of its section, or the section's end (u104's g_seBanks
    # had size 0, so &g_seBanks[1] stayed the C's own unconstructed copy)
    starts = collections.defaultdict(set)
    for n, v, sz, bind, typ, sh in syms:
        if 0 < sh < shnum and typ != 3 and n:
            starts[sh].add(v)
    for k, (n, v, sz, bind, typ, sh) in enumerate(syms):
        if bind in (1, 2) and 0 < sh < shnum:
            if typ == 2 or (secs[sh][2] & 4):   # STT_FUNC, or in an executable section
                funcs.setdefault((sh, v), k)
            if n in redirected_data:
                if sz == 0:
                    after = [s for s in starts[sh] if s > v]
                    sz = (min(after) if after else secs[sh][5]) - v
                data[sh].append((v, max(sz, 1), k))

    def inside(sh, addend):
        # the variable that starts there, else the one it points into, else
        # the one it points just past (an array's end bound in a loop: left
        # unredirected, the loop runs from the original copy to the C's end)
        cands = data.get(sh, ())
        for v, sz, k in cands:
            if v == addend:
                return v, k
        for v, sz, k in cands:
            if v <= addend < v + sz:
                return v, k
        for v, sz, k in cands:
            if addend == v + sz:
                return v, k
        return None

    def sx16(x):
        x &= 0xFFFF
        return x - 0x10000 if x & 0x8000 else x
    changed = 0
    for rs in secs:
        if rs[1] != 9:                           # SHT_REL
            continue
        tgt = secs[rs[7]]
        base, n_rel = rs[4], rs[5] // 8
        ents = [list(struct.unpack_from("<II", d, base + 8 * i)) for i in range(n_rel)]

        def word(off):
            return struct.unpack_from("<I", d, tgt[4] + off)[0]

        def put(off, w):
            struct.pack_into("<I", d, tgt[4] + off, w & 0xFFFFFFFF)
        # %hi/%lo groups as the linker pairs them: the HI16s of a section
        # symbol wait for the next LO16 of the same symbol in table order
        # (not the next entry: calls can sit between); a further LO16 with no
        # HI16 waiting shares the last group's high part (one lui, several
        # addiu: u104's g_seBanks had such an addiu left pointing at the C's
        # copy, half an address, a crash).
        groups, waiting, last = [], collections.defaultdict(list), {}
        for i, (off, info) in enumerate(ents):
            k, typ = info >> 8, info & 0xFF
            if k >= len(syms) or syms[k][4] != 3:           # only section symbols
                continue
            sh = syms[k][5]
            if typ == R_26:
                f = funcs.get((sh, (word(off) & 0x03FFFFFF) << 2))
                if f is not None:
                    put(off, word(off) & 0xFC000000)
                    ents[i][1] = (f << 8) | typ
                    changed += 1
            elif typ == R_32:
                hit = inside(sh, word(off))
                if hit:
                    put(off, word(off) - hit[0])
                    ents[i][1] = (hit[1] << 8) | typ
                    changed += 1
            elif typ == R_HI16:
                waiting[k].append(i)
            elif typ == R_LO16:
                if waiting[k]:
                    g = (k, waiting[k], [i])
                    waiting[k] = []
                    groups.append(g)
                    last[k] = g
                elif k in last:
                    last[k][2].append(i)
        for k, his, los in groups:
            sh = syms[k][5]
            hi_imm = word(ents[his[0]][0]) & 0xFFFF
            addrs = [(hi_imm << 16) + sx16(word(ents[x][0])) for x in los]
            hits = [inside(sh, a) for a in addrs]
            if not (all(hits) and len({h[1] for h in hits}) == 1):
                continue
            v, ksym = hits[0]
            new = [a - v for a in addrs]
            hi_new = {((a + 0x8000) >> 16) & 0xFFFF for a in new}
            if len(hi_new) != 1:
                continue
            hv = next(iter(hi_new))
            for x in his:
                put(ents[x][0], (word(ents[x][0]) & 0xFFFF0000) | hv)
                ents[x][1] = (ksym << 8) | R_HI16
            for x, a in zip(los, new):
                put(ents[x][0], (word(ents[x][0]) & 0xFFFF0000) | (a & 0xFFFF))
                ents[x][1] = (ksym << 8) | R_LO16
            changed += len(his) + len(los)
        for x, (off, info) in enumerate(ents):
            struct.pack_into("<II", d, base + 8 * x, off, info)
    with open(dst, "wb") as f:
        f.write(d)
    return changed


def assemble_patched_crt0(src, dst):
    """u000.s with the bss-clear loop ending at __bss_end instead of HEAP_SYM."""
    lines = open(src, encoding="utf-8").read().split("\n")
    n = 0
    for i, L in enumerate(lines):
        if any(" %s " % a in L for a in CRT0_CLEAR) and HEAP_SYM in L:
            lines[i] = L.replace(HEAP_SYM, "__bss_end")
            n += 1
    if n != 2:
        raise SystemExit("crt0 patch: expected 2 lines in %s, found %d" % (src, n))
    patched = dst[:-2]                                  # .../u000.s
    os.makedirs(os.path.dirname(patched), exist_ok=True)
    with open(patched, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    r = run([tool("as")] + relink.AS_FLAGS + ["-I", os.path.join(relink.SPLAT, "include"), "-I", relink.SPLAT,
                                               "-o", dst, patched], cwd=relink.SPLAT)
    if r.returncode:
        raise SystemExit("assembling the patched crt0 failed:\n" + r.stderr[-2000:])


def link(build, c_units, pack=None, keep_asm=(), replace_complete=True):
    """Link the hybrid in <build>/hybrid/.  Returns (elf path or None, report lines)."""
    link_dir = os.path.join(build, "link")
    out = os.path.join(build, "hybrid")
    os.makedirs(os.path.join(out, "obj"), exist_ok=True)
    base_ld = os.path.join(link_dir, "core.ld")
    base_elf = os.path.join(link_dir, "core.elf")
    if not (os.path.isfile(base_ld) and os.path.isfile(base_elf)):
        return None, ["no relink in %s: run tools/build.py (or tools/relink.py --build-dir %s)" % (link_dir, link_dir)]
    report = []
    units = unit_ranges()
    # each object's work unit (ref/file_units.json, tools/layout.py), the C
    # objects in the original's link order (not the tree's)
    # (every walk over the objects below goes in this order: a name defined
    # in several objects, a header's static helper, maps to the first one)
    rel_of_obj = {}
    for o in glob.glob(os.path.join(build, "obj", "src__*.o")):
        rel = os.path.basename(o)[:-len(".o")].replace("__", "/")
        if layout.unit_of_file(rel) is not None:
            rel_of_obj[rel] = o
    unit_of_obj = {rel_of_obj[r]: layout.unit_of_file(r) for r in layout.link_order(list(rel_of_obj))}
    c_objs = [o for o, u in unit_of_obj.items() if u in set(c_units)]

    # a file-static C variable with an original address (u119's RPC objects,
    # `static RPC g_004A6B00;`) is made global, so it is handled as every other
    # such variable: the original copy is the one.  A local symbol cannot be
    # redirected: the C's code would use its own copy, the rest the original's
    # (two copies of the RPC client state hung the boot at PDI1394.IRX).
    # Only ordinary .data / .bss ones (`d` / `b`): their references are
    # %hi/%lo pairs that retarget() can redirect; a $gp-relative small-data
    # static (`g` / `s`) cannot be, so it keeps its own copy (as before).  The
    # build's objects stay as compiled: the C objects' copies below get it.
    decl_early = declared()
    nm_all(c_objs, "--defined-only")
    globalize = {}
    only = os.environ.get("HYBRID_GLOBALIZE_ONLY")      # debugging: "u119,u104" limits it to these units
    for o in c_objs:
        if only is not None and unit_of_obj.get(o) not in only.split(","):
            continue
        loc = sorted({n for _, t, n in nm(o, "--defined-only")
                      if t in "db" and (ADDR_IN_NAME.search(n) or n in decl_early)})
        if loc:
            globalize[o] = loc
    n_glob = sum(len(v) for v in globalize.values())
    if n_glob:
        report.append("file-static C variables with an original address made global: %d" % n_glob)

    # the function map, over every compiled object (also units not linked from C)
    entries, defined_all = [], set()
    for o in unit_of_obj:
        fj = o[:-2] + ".funcs.json"
        if os.path.exists(fj):
            entries += [tuple(e) for e in json.load(open(fj))]
    nm_all(list(unit_of_obj), "--defined-only")
    nm_all(c_objs, "-u")
    nm_all(glob.glob(os.path.join(link_dir, "obj", "asm", "*.s.o")) +
           glob.glob(os.path.join(link_dir, "obj", "asm", "data", "*.o")), "--defined-only", "-g")
    for o in unit_of_obj:
        defined_all |= {n for _, t, n in nm(o, "--defined-only") if t in "TtWw"}
    fmap, notes = funcmap.build(entries, defined=defined_all)
    report += ["function map: %d symbols" % len(fmap)] + ["  note: " + n for n in notes]

    # file-static C functions with an original address (newlib's, libgcc's and
    # Sony's static helpers, the menus' qsort comparators ...): global in the
    # link's copies of the C objects too, so a unit whose functions are all
    # C is complete (its original code goes) and the original's references
    # to them (data, the constructor list) reach the C.  Only names unique
    # among the C objects and not defined by the split; the rest stay local
    # (their unit keeps its original code, as before).
    asm_names = set()
    for p in (glob.glob(os.path.join(link_dir, "obj", "asm", "*.s.o")) +
              glob.glob(os.path.join(link_dir, "obj", "asm", "data", "*.o"))):
        asm_names |= {n for _, t, n in nm(p, "--defined-only", "-g")}
    counts = collections.Counter(n for o in c_objs for _, t, n in nm(o, "--defined-only"))
    n_fglob = 0
    for o in c_objs:
        if unit_of_obj.get(o) == "u000":                # crt0's unit stays original (as below)
            continue
        fl = {n for _, t, n in nm(o, "--defined-only")
              if t == "t" and n in fmap and counts[n] == 1 and n not in asm_names}
        if fl:
            globalize[o] = sorted(set(globalize.get(o, ())) | fl)
            n_fglob += len(fl)
    if n_fglob:
        report.append("file-static C functions with an original address made global: %d" % n_fglob)

    # the C objects linked: what they define
    c_glob = {}                                         # global symbol -> type
    for o in c_objs:
        made = set(globalize.get(o, ()))
        for _, t, n in nm(o, "--defined-only"):
            if t.isupper() or n in made:
                c_glob.setdefault(n, t.upper())

    # the original's symbols and where the split objects define them
    orig = {n: v for v, t, n in nm(base_elf, "--defined-only") if v is not None}
    asm_dir = os.path.join(link_dir, "obj", "asm")
    replace = {}                                        # base object path -> new path
    bridge = []
    redirected = 0
    # crt0 keeps clearing only the .bss: its loop end gets its own symbol
    crt0 = os.path.join(out, "obj", "asm", "u000.s.o")
    assemble_patched_crt0(os.path.join(relink.SPLAT, "asm", "u000.s"), crt0)
    replace[os.path.join(asm_dir, "u000.s.o")] = crt0
    # the eh_tables region, its code pointers (constructor list, tables) as symbols
    names_by_addr = {}
    for v, t, n in nm(base_elf, "--defined-only"):
        if v is not None and t in "TW" and not n.endswith(".NON_MATCHING"):
            if n == "F_%08X" % v or v not in names_by_addr:
                names_by_addr[v] = n
    starts = {int(f["start"], 16) for f in json.load(open(os.path.join(paths.REF, "funcs.json")))}
    eh_obj = os.path.join(out, "obj", "assets", "eh_tables.o")
    n_eh = assemble_eh_tables(os.path.join(relink.SPLAT, "assets", "eh_tables.bin"), names_by_addr, starts, eh_obj)
    replace[os.path.join(link_dir, "obj", "assets", "eh_tables.bin.o")] = eh_obj
    report.append("code pointers in the eh_tables region (constructor list, tables) as symbols: %d" % n_eh)
    all_funcs = [int(f["start"], 16) for f in json.load(open(os.path.join(paths.REF, "funcs.json")))]
    replaced = {}                                       # unit -> its C objects (no original code left)
    for u, lo, hi in units:
        if u not in c_units:
            continue
        src = replace.get(os.path.join(asm_dir, u + ".s.o"), os.path.join(asm_dir, u + ".s.o"))
        to_local = []
        provided = {}                                   # address -> C symbol
        for n, a in fmap.items():
            if lo <= a < hi and n in c_glob and c_glob[n] in "TW" and a not in keep_asm:
                provided.setdefault(a, n)
        if replace_complete and u != "u000" and all(a in provided for a in all_funcs if lo <= a < hi):
            # every function of the unit comes from its C: the original code goes.
            # Its other labels (jump-table targets the original data still names,
            # used only by functions that are C now) keep their original addresses.
            # (the object's own symbol table: nm hides the .L jump-table labels)
            eo = ElfObject(src)
            for y in eo.symbols:
                if y.bind != 1 or y.shndx == 0 or y.shndx >= len(eo.sections) or not y.name:
                    continue
                n = y.name
                a = orig.get(n)
                if a is None and eo.sections[y.shndx].name == ".text":
                    a = lo + y.value                    # .L labels: the linked ELF drops them
                if a in provided:
                    if n != provided[a]:
                        bridge.append("%s = %s;" % (n, provided[a]))
                elif a is not None and n not in c_glob:
                    bridge.append("%s = 0x%08X;" % (n, a))
            replaced[u] = [o for o in c_objs if unit_of_obj[o] == u]
            redirected += len(provided)
            continue
        for _, t, n in nm(src, "--defined-only", "-g"):
            a = orig.get(n)
            if a in provided:
                to_local.append(n)
                if n != provided[a]:
                    bridge.append("%s = %s;" % (n, provided[a]))
        redirected += len(provided)
        lst = os.path.join(out, "obj", "asm", u + ".localize")
        with open(lst, "w") as f:
            f.write("\n".join(to_local) + "\n")
        dst = os.path.join(out, "obj", "asm", u + ".s.o")
        r = run([tool("objcopy"), "--localize-symbols=" + lst, src, dst + ".tmp"])
        if r.returncode:
            return None, report + ["objcopy failed for %s: %s" % (u, r.stderr.strip())]
        os.replace(dst + ".tmp", dst)
        replace[os.path.join(asm_dir, u + ".s.o")] = dst
    # functions kept as the original code: their C symbols point at the original
    # (a linker-script assignment overrides the C definition, for C callers too)
    for n, a in fmap.items():
        if a in keep_asm and n in c_glob:
            bridge.append("%s = 0x%08X;" % (n, a))
    report.append("units from C: %d; functions redirected to the C: %d%s" % (
        len(c_units), redirected, "; kept in assembly: %d" % len(keep_asm) if keep_asm else ""))
    report.append("units with no original code left: %d (their C replaces it)" % len(replaced))

    # what the C needs that no linked object defines: original addresses
    asm_glob = set()
    for p in glob.glob(os.path.join(asm_dir, "*.s.o")) + glob.glob(os.path.join(asm_dir, "data", "*.o")):
        q = replace.get(p, p)
        asm_glob |= {n for _, t, n in nm(q, "--defined-only", "-g")}
    for p in glob.glob(os.path.join(link_dir, "obj", "asm", "data", "*.o")):
        asm_glob |= {n for _, t, n in nm(p, "--defined-only", "-g")}
    auto = {}
    for fn in ("undefined_syms_auto.txt", "undefined_funcs_auto.txt"):
        for L in open(os.path.join(relink.SPLAT, fn)):
            m = re.match(r"\s*(\S+)\s*=", L)
            if m:
                auto[m.group(1)] = L.strip()
    bridged = {b.split("=")[0].strip() for b in bridge}
    names = table_names()
    decl = declared()
    vts = vtables()
    markers_by_name = funcmap.by_name()
    unresolved, how = [], collections.Counter()
    needed = set()
    for o in c_objs:
        needed |= {n for _, t, n in nm(o, "-u")}
    for n in sorted(needed):
        if n in c_glob or n in asm_glob or n in bridged or n in auto or n in ("_gp", "__bss_end", "__heap_start", "_end"):
            continue
        if n in fmap:
            a, k = fmap[n], "function map"
        elif n in markers_by_name:
            a, k = markers_by_name[n], "marker name"
        elif n in decl:
            a, k = decl[n], "declaration"
        elif n.startswith("_vt$") and (n in vts or re.sub(r"^(_vt\$[^$]+)\$.*$", r"\1$*", n) in vts):
            a, k = vts.get(n, vts.get(re.sub(r"^(_vt\$[^$]+)\$.*$", r"\1$*", n))), "class index (vtable)"
        elif ADDR_IN_NAME.search(n):
            a, k = int(ADDR_IN_NAME.search(n).group(1), 16), "address in the name"
        elif n in names:
            a, k = names[n], "symbol tables"
        elif n in CRT0_SYMBOLS:
            a, k = CRT0_SYMBOLS[n], "crt0 linker symbols"
        elif extern_line(n) is not None:
            a, k = extern_line(n), "declaration line"
        else:
            unresolved.append(n)
            continue
        bridge.append("%s = 0x%08X;" % (n, a))
        how[k] += 1
    report.append("C references given their original address: %d (%s)" % (
        sum(how.values()), ", ".join("%s %d" % kv for kv in how.most_common())))

    # data the C defines that the original also has at a fixed address
    # (its own vtables and exception tables are expected: what the C constructs uses them)
    own = [n for n, t in c_glob.items() if t in "DBRVC" and n.startswith(("_vt$", "__vt_", "_GLOBAL_$F$"))]
    dup = sorted(n for n, t in c_glob.items() if t in "DBRVC" and n not in own
                 and (ADDR_IN_NAME.search(n) or n in names or n in decl))
    # the original copy is the one: the code left original reads and writes it,
    # so every reference of the C goes there too (a script assignment overrides
    # the C's definition; the C's copy stays, unused)
    for n in dup:
        if n in asm_glob:                               # the split defines it at its address (made weak in the C below)
            continue
        a = decl.get(n, names.get(n))
        if a is None:
            a = int(ADDR_IN_NAME.search(n).group(1), 16)
        bridge.append("%s = 0x%08X;" % (n, a))
    report.append("vtables and exception tables the C emits itself: %d" % len(own))
    report.append("variables the C defines that the original also has (they use the original's): %d" % len(dup))
    report += ["  " + n for n in dup]

    # the C objects as linked: their references to their own symbols by name
    cdir = os.path.join(out, "obj", "c")
    os.makedirs(cdir, exist_ok=True)
    dupset, moved, n_rt = set(dup), {}, 0
    for o in c_objs:
        dst = os.path.join(cdir, os.path.basename(o))
        src = o
        if o in globalize:                              # the file statics chosen above, global in the copy
            src = dst + ".glob"
            r = run([tool("objcopy")] + ["--globalize-symbol=" + n for n in globalize[o]] + [o, src])
            if r.returncode:
                return None, report + ["objcopy failed for %s: %s" % (o, r.stderr.strip())]
        n_rt += retarget(src, dst, dupset)
        if src != o:
            os.remove(src)
        moved[o] = dst
    c_objs = [moved[o] for o in c_objs]
    for u in replaced:
        replaced[u] = [moved[o] for o in replaced[u]]
    report.append("same-file references made symbolic in the C objects: %d" % n_rt)
    # a variable the split's data also defines under the same global name
    # (data_symbols.csv names it, as u125's heap_object): the C's copy becomes
    # weak, so the original's definition wins and every reference goes there
    weakened = 0
    for o in c_objs:
        clash = sorted({n for _, t, n in nm(o, "--defined-only") if t in "DBRVC"} & dupset & asm_glob)
        if not clash:
            continue
        r = run([tool("objcopy")] + ["--weaken-symbol=" + n for n in clash] + [o, o + ".tmp"])
        if r.returncode:
            return None, report + ["objcopy failed for %s: %s" % (o, r.stderr.strip())]
        os.replace(o + ".tmp", o)
        weakened += len(clash)
    if weakened:
        report.append("C variables made weak (the split defines the same name): %d" % weakened)

    # the scripts
    text = open(base_ld, encoding="utf-8").read()
    # (a relink made before its objects were ROOT-relative names them absolutely)
    text = text.replace(os.path.join(link_dir, "obj").replace("\\", "/") + "/",
                        paths.ld_path(os.path.join(link_dir, "obj")) + "/")
    text = re.sub(r"(?s)\n\s*elf_header_ROM_START.*?elf_header_VRAM_END = \.;", "", text)
    text = re.sub(r"(?s)\n\s*elf_trailer_ROM_START.*?elf_trailer_VRAM_END = \.;", "", text)
    text = text.replace(" : AT(main_ROM_START)", " :")
    for a, b in replace.items():                       # (the relink's script: ROOT-relative paths)
        text = text.replace(paths.ld_path(a) + "(", paths.ld_path(b) + "(")
    # replaced units: their C's code in the unit's own slot when it fits, else
    # with the rest of the C; the following unit stays at its address either way.
    # A translation unit merged back into one file (in its first work unit's
    # folder) also provides the functions of the work units after it, which are
    # then replaced with no object of their own: their slots, right after its
    # unit's, join that slot (else the merged object would not fit)
    in_slot, joined = 0, set()
    for i, (u, lo, hi) in enumerate(units):
        if u not in replaced:
            continue
        path = paths.ld_path(os.path.join(asm_dir, u + ".s.o"))
        if u in joined:                                 # its slot holds the unit before's C
            body = ""
        else:
            size = 0
            for o in replaced[u]:
                for sec in ElfObject(o).sections:
                    if sec.name == ".text" or sec.name.startswith(".text."):
                        al = max(sec.align, 1)
                        size = (size + al - 1) // al * al + sec.size
            end, span = hi, []
            for v, vlo, vhi in units[i + 1:]:
                if size <= end - lo or vlo != end or replaced.get(v) != []:
                    break
                end, span = vhi, span + [v]
            if size <= end - lo:
                body = ("".join("        %s(.text .text.*);\n" % paths.ld_path(o) for o in replaced[u]) +
                        "        . = 0x%X;\n" % (end - 0x00100000))
                in_slot += 1
                joined.update(span)
            else:
                body = "        . = 0x%X;\n" % (hi - 0x00100000)
        text = text.replace("        %s(.text);\n" % path, body)
        text = re.sub(r"(?m)^\s*%s\([^)]*\);\n" % re.escape(path), "", text)
    report.append("  of those, with their C in the original slot: %d" % in_slot)
    c_sections = """
    .c_bss 0x%08X (NOLOAD) :
    {
        *(.bss .bss.* .sbss .sbss.* .scommon COMMON)
    }
    __bss_end = SIZEOF(.c_bss) ? ALIGN(., 16) : 0x%08X;
    .c_text ALIGN(MAX(., 0x%08X), 64) :
    {
        *(.text .text.*)
    }
    .c_rodata :
    {
        *(.rodata .rodata.* .lit4 .lit8)
    }
    .c_data :
    {
        *(.data .data.* .sdata .sdata.* .gcc_except_table .gcc_except_table.* .ctors .dtors .eh_frame)
    }
    __heap_start = (SIZEOF(.c_bss) + SIZEOF(.c_text) + SIZEOF(.c_rodata) + SIZEOF(.c_data)) ? ALIGN(., 64) : 0x%08X;
    %s = __heap_start;
    PROVIDE(_end = __heap_start);

""" % ((ORIG_END + 15) & ~15, ORIG_END, (ORIG_END + 63) & ~63, ORIG_END, HEAP_SYM)
    text = text.replace("    /DISCARD/ :", c_sections + "    /DISCARD/ :")
    script = os.path.join(out, "hybrid.ld")
    with open(script, "w", encoding="utf-8") as f:
        f.write("ENTRY(F_00100008)\n" + text)
    autos = os.path.join(out, "auto.ld")
    with open(autos, "w", encoding="utf-8") as f:
        f.write("\n".join(v for k, v in auto.items() if k != HEAP_SYM) + "\n")
    bridge_ld = os.path.join(out, "bridge.ld")
    with open(bridge_ld, "w", encoding="utf-8") as f:
        f.write("/* generated by tools/hybrid.py */\n" + "\n".join(bridge) + "\n")

    # calls egcs emits that the original never makes (it has no such function):
    # supplied here, as what -ffast-math would have inlined
    support = [n for n in unresolved if n in SUPPORT]
    unresolved = [n for n in unresolved if n not in SUPPORT]
    extra_objs = []
    if support:
        src = os.path.join(out, "obj", "support.s")
        with open(src, "w", encoding="utf-8") as f:
            f.write(".set noreorder\n.section .text\n" + "".join(SUPPORT[n] for n in support))
        dst = src[:-2] + ".o"
        r = run([tool("as")] + relink.AS_FLAGS + ["-o", dst, src])
        if r.returncode:
            return None, report + ["assembling the support functions failed: " + r.stderr.strip()]
        extra_objs.append(dst)
        report.append("support functions the original does not have, for the C: %s" % ", ".join(support))
    if unresolved:
        report.append("UNRESOLVED (%d): what the C uses that nothing defines" % len(unresolved))
        report += ["  " + n for n in unresolved]
        return None, report

    elf, mapf = os.path.join(out, "core.elf"), os.path.join(out, "core.map")
    if os.path.exists(elf):
        os.remove(elf)
    lst = os.path.join(out, "c_objects.txt")
    with open(lst, "w", encoding="utf-8") as f:
        f.write("\n".join(paths.ld_path(o) for o in c_objs) + "\n")
    r = run([tool("ld"), "-EL", "--no-check-sections", "--no-warn-mismatch", "-nostdlib", "-T", script,
             "-T", autos, "-T", bridge_ld, "-Map", mapf, "-o", elf] + (["@" + lst] if c_objs else []) + extra_objs,
            cwd=paths.ROOT)                             # the scripts' objects are ROOT-relative
    if r.returncode or not os.path.isfile(elf):
        report += ["LINK FAILED:"] + (r.stdout + r.stderr).strip().splitlines()[-60:]
        return None, report
    syms = {n: v for v, t, n in nm(elf) if v is not None}
    heap = syms.get("__heap_start", 0)
    report.append("heap starts at 0x%08X (original 0x%08X): %d KB less heap" % (heap, ORIG_END, (heap - ORIG_END) >> 10))
    if pack:
        img = gt3image.Image.from_elf(open(elf, "rb").read())
        img.sign()
        raw = img.pack()
        with open(pack, "wb") as f:
            f.write(raw)
        report += ["packed %s:" % pack] + ["  " + L for L in gt3image.describe(raw)]
    return elf, report


def main():
    global TC_BIN
    ap = argparse.ArgumentParser(description="Link the hybrid program (original code and data, chosen units from C).")
    ap.add_argument("--units", help="all, none, or a list like u001,u010-u020 (default: hybrid.txt)")
    ap.add_argument("--build-dir", default=os.path.join(paths.ROOT, "build", "991111b"))
    ap.add_argument("--toolchain")
    ap.add_argument("--pack", metavar="OUT", help="also write a signed core.gt3")
    ap.add_argument("--overlay-only", action="store_true",
                    help="keep every C unit's original code linked (no unit replaced by its C)")
    ap.add_argument("--keep-asm", default="", metavar="ADDR,...",
                    help="functions that stay the original code although their unit is from C")
    a = ap.parse_args()
    TC_BIN = paths.ee_bin(a.toolchain)
    relink.TC_BIN = TC_BIN
    t0 = time.time()
    units = [u for u, _, _ in unit_ranges()]
    if a.units is None:
        chosen, keep = read_config(CONFIG, units)
    else:
        chosen, keep = parse_units(a.units, units), set()
    keep |= {int(x, 16) for x in a.keep_asm.split(",") if x.strip()}
    elf, report = link(os.path.abspath(a.build_dir), chosen, a.pack, keep, replace_complete=not a.overlay_only)
    out = os.path.join(os.path.abspath(a.build_dir), "hybrid")
    with open(os.path.join(out, "report.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(report) + "\n")
    print("\n".join(L for L in report if not L.startswith("  ") or len(report) < 60))
    print(("linked %s" % elf) if elf else "error: the hybrid did not link", "in %.1f s" % (time.time() - t0))
    return 0 if elf else 1


if __name__ == "__main__":
    sys.exit(main())
