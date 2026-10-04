"""Translation-unit layout: which retail objects (original translation units)
the functions and data of a source file came from, and where they were linked.

    python tools/layout.py src/gs_driver.cpp [more files]  # a summary per file
    python tools/layout.py -v src/pdistd/printf.cpp         # + items, refs, gaps, constraints
    python tools/layout.py                                  # problem files + project summary
    python tools/layout.py --all                            # every file + project summary
    python tools/layout.py --objects                        # the retail EH objects (frame table)
    python tools/layout.py --retail 0x01039400 0x01039600   # what is known at retail addresses
    python tools/layout.py ... --json out.json              # machine-readable result

Every C/C++ file of the tree (score.py's list) is compiled with tools/mcc.py
into build/layout/ (never the scorer's build/score/; about 2 s in parallel),
because what a file's neighbours own is part of the evidence.  Functions are
mapped to their FUNC markers and aligned with the retail code exactly as
tools/score.py does.  Then, per file:

Text.  The file's functions in retail address order, the runs they form in
  .text, other files' functions interleaved between runs (a run break is an
  object boundary), and the alignment padding (nops) at each run end.  egcs
  aligns every function to 8 bytes and the old assembler gives each object's
  .text the alignment of its contents (8), so padding alone never proves a
  boundary.  Code in `.text.<name>` function sections (libgraph) is placed
  after .text by the linker whatever the link order.
  "text drift": where the object's own .text offsets stop matching the
  retail distances - a function before it is longer or shorter (not exact),
  padded differently (a retail function that is not 8-byte aligned was not
  compiled as a function: top-level asm), or emitted in another order
  (compiler-generated / inline functions, merged objects).  Local code
  addresses (static callbacks) are then off, and so would a link be.

Retail objects with EH frames.  egcs had no .eh_frame section on this target:
  every C++ object compiled with exceptions put its DWARF2 frame block (one
  CIE, one FDE per function, a terminating zero word) at the END of its own
  .data, labelled _GLOBAL_$F$<first global symbol>, and collect2 listed the
  36 labels in the table at 0x01026C00 (src/crtbegin.c).  Each block's FDEs
  give that object's exact .text range; the block's position in .data marks
  where the object's .data ends.  A CIE with augmentation "eh" means the
  object has exception regions (.gcc_except_table entries).  Objects compiled
  without exceptions (or C) have no block; a C++ object with exceptions but
  no function still has one (a CIE and the zero word, 0x18 bytes: "data-only").
  The file's own object is compared with that: frame block or not ("FLAGS":
  -fno-exceptions needed or wrongly given), the "eh" augmentation, and the
  block's size and FDE count.  When a file is exactly one retail EH object
  (or a data-only file fits a data-only EH object between the right
  neighbours) its frame block gets the retail address, which also checks the
  .data in front of it.

Data items.  Every relocation of a mapped function that builds a data address
  (%hi/%lo pair, %gp_rel, literal) is resolved to the retail address the
  aligned retail instructions build (tools/dataref.py's rules, GP0 bias
  included).  The reference is attributed to an ITEM of the object: the
  symbol (static or global) whose extent contains the offset, else an
  anonymous piece (jump table, string literal, literal pool) at that offset.
  That gives each item its retail address and retail section.  Items no
  function of the file uses get theirs from an `@0x...` annotation of their
  definition or from another file's (extern) references.
  * base groups: items of one object section that imply the same retail
    section base (retail address - offset in the object) are laid out as in
    retail.  More than one group = the scorer's "data-refs differ".
  * ownership: every placed item of every file is OWNED by the file that
    defines it (a file's references to externs are not).  Annotated globals
    that no compiled file defines, references by uncompiled code (runtime
    libraries, .S) and the retail EH frame blocks complete the map of
    "landmarks" at retail data addresses.
  * a landmark BETWEEN two items of the file (same retail section) means the
    two items are in different retail objects, the lower one linked first
    (objects contribute their sections in link order).
  * neighbours: for each inferred object and section, the nearest landmarks
    of other owners below and above its items (= the objects linked right
    before and after it, as far as known).  LINK ORDER notes flag a neighbour
    whose own text lies on the wrong side of the object's text.
  * OVERLAP: two files claim the same retail bytes.

Inference.  Assuming every item lives in the object of the functions that use
  it (always true for statics and literals), each split needs an object
  boundary between the last user of the lower item and the first user of the
  higher one.  A boundary can not fall inside the span of any item's users,
  nor inside an EH object's FDE range; a change of EH object, or another
  file's function in between, is a boundary of its own.  The tool reports
  the minimal set of boundaries, each with the range of positions the
  evidence allows ("undetermined" functions between them could belong to
  either side), and for each inferred object its items: the base groups
  inside it, items whose order or gaps differ from the retail order, and
  items that sit in another retail section kind than the object section
  (.rodata in the object but .data in retail = writable in the original).
"""
import argparse, bisect, collections, concurrent.futures as cf, json, os, re, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import score
import dataref
from elfobj import ElfObject, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL

OUT = os.path.join(ROOT, "build", "layout")
score.OUT = OUT                       # score's obj_path/asm_path: our own object directory
GP = dataref.GP
DATA_KINDS = (".data", ".rodata", ".rdata", ".sdata", ".sbss", ".bss", ".lit4", ".lit8", ".gcc_except_table")
SHN_UNDEF, SHN_ABS, SHN_COMMON = 0, 0xFFF1, 0xFFF2
COMMONS = (SHN_COMMON, 0xFF01, 0xFF03)    # + MIPS ACOMMON / SCOMMON (small common, -G)
EH_SYMS = re.compile(r"^(__FRAME_BEGIN__|_GLOBAL_\$F\$.*|__EXCEPTION_TABLE__)$")

# ------------------------------------------------------------------ retail model

RETAIL = ElfObject(score.BIN)
RSECS = [(s.name, s.addr, s.addr + s.size) for s in RETAIL.sections
         if s.addr and s.size and (s.name in DATA_KINDS or s.name.startswith(".text"))]
FSECS = [(lo, hi) for n, lo, hi in RSECS if n.startswith(".text.")]      # function sections

def rsec(a):
    for n, lo, hi in RSECS:
        if lo <= a < hi:
            return n
    return None

def in_fsec(a):
    return any(lo <= a < hi for lo, hi in FSECS)

FUNCS = score.funcs                                   # start -> funcs.json record
FSTARTS = sorted(FUNCS)
FINDEX = {a: k for k, a in enumerate(FSTARTS)}

def code_end(a):
    """End of the retail function's real code (trailing alignment nops excluded)."""
    return a + 4 * len(score.retail_words(a))

SRC_DIRS = ["src", "sdk", "runtime", "link", "include"]

def all_marker_files():
    out = []
    for d in ("src", "sdk", "runtime"):
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in fns:
                if fn.endswith((".c", ".cpp", ".cc", ".S")):
                    out.append(os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/"))
    return sorted(out)

FILE_OF = {}                                          # retail function start -> source file
NAME_OF = {}
for _rel in all_marker_files():
    for _ln, _a, _nm in score.markers(_rel):
        FILE_OF.setdefault(_a, _rel)
        NAME_OF.setdefault(_a, _nm)

def fname(a):
    """Short name of a retail function from its FUNC marker text."""
    nm = NAME_OF.get(a) or ""
    nm = nm.replace("(anonymous namespace)", "{anon}")
    nm = re.sub(r"\s*\(.*$", "", nm).strip()
    return nm.split()[0] if nm else "F_%08X" % a

def owner(a):
    """'file' of a retail function address (FUNC marker), else its unit."""
    return FILE_OF.get(a) or "unit:" + score.unit_of(a)

# --- retail code: data addresses built by lui+lo / gp-relative instructions
_LOADSTORE = {0x09, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2E,
              0x1A, 0x1B, 0x1E, 0x1F, 0x31, 0x35, 0x37, 0x39, 0x3D, 0x3F}
_WRITES_RT = _LOADSTORE - {0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x1F, 0x31, 0x35, 0x39, 0x3D, 0x3F} | {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x18}

def _scan_retail_refs():
    """[(target, function start, insn address)] for every retail function."""
    out = []
    for a in FSTARTS:
        his = {}
        for x in range(a, FUNCS[a]["end"], 4):
            w = score.rword(x)
            op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
            if op == 0x0F:                                  # lui
                his[rt] = (w & 0xFFFF) << 16
                continue
            t = None
            if op in _LOADSTORE or op == 0x0D:
                if rs == 28 and op != 0x0D:
                    t = (GP + dataref.sx16(w)) & 0xFFFFFFFF
                elif rs in his:
                    t = (his[rs] | (w & 0xFFFF)) if op == 0x0D else (his[rs] + dataref.sx16(w)) & 0xFFFFFFFF
            if t is not None and rsec(t) in DATA_KINDS:
                out.append((t, a, x))
            # forget registers this instruction overwrites
            if op == 0 or op == 0x1C:
                his.pop(rd, None)
            elif op in _WRITES_RT:
                his.pop(rt, None)
    return out

RREFS = _scan_retail_refs()

# --- the collect2 frame table: one EH frame block per C++ object with exceptions
def _u32(a):
    return score.rword(a)

def img_cstr(a):
    s = b""
    while True:
        c = score.img[0x1000 + a - 0x01000000]
        if c == 0 or len(s) > 64:
            return s.decode("latin1")
        s += bytes([c])
        a += 1

def fstart_of(a):
    i = bisect.bisect_right(FSTARTS, a) - 1
    return FSTARTS[i] if i >= 0 and a < FUNCS[FSTARTS[i]]["end"] else None

def _scan_frames():
    tab, a = [], 0x01026C00
    while True:
        w = _u32(a)
        if not (0x01026C00 <= w < 0x0103DDD8):
            break
        tab.append(w)
        a += 4
    objs = []
    for i, cie in enumerate(tab):
        clen = _u32(cie)
        aug = img_cstr(cie + 9)
        q, fdes = cie + 4 + clen, []
        while True:
            L = _u32(q)
            if L == 0 or _u32(q + 4) == 0 or q + 4 - _u32(q + 4) != cie:
                break
            fdes.append((_u32(q + 8), _u32(q + 8) + _u32(q + 12)))
            q += 4 + L
        end = q + 4 if _u32(q) == 0 else q              # the terminating zero word
        objs.append(dict(table_index=i, frame=(cie, end), size=end - cie, eh=aug == "eh", fdes=fdes,
                         funcs=sorted({fstart_of(f) for f, _ in fdes} - {None}),
                         text=(min(f[0] for f in fdes), max(f[1] for f in fdes)) if fdes else None))
    objs.sort(key=lambda o: o["frame"][0])
    for k, o in enumerate(objs):
        o["id"] = k
        o["files"] = sorted({owner(f) for f in o["funcs"]})
    return tab, objs

FRAME_TABLE, EHOBJS = _scan_frames()
_EH_OF = {}
for _o in EHOBJS:
    for _f in _o["funcs"]:
        _EH_OF[_f] = _o

def ehobj_of(a):
    return _EH_OF.get(a)

# --- where the linker put the common symbols: after every object's .sbss /
# .bss.  egcs emits each type_info node (__ti<type>, written by its __tf
# function) as a common symbol, so the lowest one bounds that area.
def _common_area():
    lows = {}
    for t, f, x in RREFS:
        if fname(f).startswith("__tf") and rsec(t) in (".sbss", ".bss"):
            lows[rsec(t)] = min(lows.get(rsec(t), t), t)
    return lows

COMMON_LOW = _common_area()

def in_common_area(a):
    s = rsec(a)
    return s in COMMON_LOW and a >= COMMON_LOW[s]

def eh_name(o):
    return "#%d" % o["id"]

# --- annotated globals of the whole tree (name, retail address, file)
def _scan_annotations():
    out = []
    for d in SRC_DIRS:
        for dp, _, fns in os.walk(os.path.join(ROOT, d)):
            for fn in fns:
                if not fn.endswith((".c", ".cpp", ".cc", ".h", ".S")):
                    continue
                p = os.path.join(dp, fn)
                rel = os.path.relpath(p, ROOT).replace("\\", "/")
                for L in open(p, encoding="utf-8", errors="replace"):
                    g = dataref.DEF.match(L)
                    if not g and rel.endswith(".S"):
                        g = re.match(r"^(\w+):\s*\.\w+.*?@\s*(0x01[0-9A-Fa-f]{6})", L)
                    if g:
                        a = int(g.group(2), 16)
                        if rsec(a) in DATA_KINDS:
                            out.append((g.group(1), a, rel))
    return out

ANNOT = _scan_annotations()

def raw_landmarks():
    """Everything known at retail data addresses, unowned: [(addr, end, text)]
    (for --retail and the split test)."""
    lm = []
    for t, f, x in RREFS:
        lm.append((t, t + 1, "ref by %s %s" % (fname(f), owner(f)), owner(f)))
    for nm, a, rel in ANNOT:
        lm.append((a, a + 1, "%s (@ in %s)" % (nm, rel), rel))
    for o in EHOBJS:
        lo, hi = o["frame"]
        lm.append((lo, hi, "EH frame block of object %s (%s)" % (eh_name(o), ", ".join(o["files"]) or "no functions"),
                   None))
    lm.sort()
    return lm

LANDMARKS = raw_landmarks()

# ------------------------------------------------------------------ object side

def compile_all(rels, jobs):
    os.makedirs(OUT, exist_ok=True)
    with cf.ThreadPoolExecutor(max_workers=jobs) as ex:
        return list(ex.map(score.compile_one, rels))

def symmap():
    m = score.seed_symmap()
    p = os.path.join(ROOT, "build", "score", "symmap.json")
    if os.path.exists(p):
        try:
            m.update(json.load(open(p)))
        except ValueError:
            pass
    return m

class Item:
    """A piece of an object section: a symbol, an anonymous offset or the EH frame block."""
    def __init__(self, sec, start, size, name):
        self.sec, self.start, self.size, self.name = sec, start, size, name
        self.retail = None                 # retail address of self.start
        self.conflict = []                 # other retail addresses implied by refs
        self.users = set()                 # retail function starts
        self.ext_users = set()             # functions of other files using it
        self.refs = []
        self.via = None                    # how an item no function of the file uses got its address
        self.sized = False                 # a symbol with an st_size (not an extent guessed here)

    @property
    def rsec(self):
        return rsec(self.retail) if self.retail is not None else None

    @property
    def end(self):
        return self.retail + max(self.size, 1)

    def label(self):
        return self.name

def object_items(obj):
    """section index -> sorted [Item] for the named symbols of the data sections.
    A symbol without a size (egcs writes no .size for some statics) extends to
    the next symbol or the end of its section; a zero-size label at the offset
    of a sized symbol (e.g. printf.cpp's namespace anchor) is not an item, nor
    are the EH frame labels (the frame block is added by FileLayout)."""
    items = collections.defaultdict(list)
    syms = [y for y in obj.symbols
            if y.shndx not in (SHN_UNDEF, SHN_ABS, SHN_COMMON) and y.shndx < len(obj.sections)
            and obj.sections[y.shndx].name in DATA_KINDS and y.type in (0, 1) and y.name
            and not y.name.startswith("$") and not EH_SYMS.match(y.name)]
    sized = {(y.shndx, y.value) for y in syms if y.size}
    for y in syms:
        if y.size == 0 and (y.shndx, y.value) in sized:
            continue
        if any(i.start == y.value and i.name == y.name for i in items[y.shndx]):
            continue
        it = Item(obj.sections[y.shndx].name, y.value, y.size, y.name)
        it.sized = y.size > 0
        items[y.shndx].append(it)
    for k, lst in items.items():
        lst.sort(key=lambda i: i.start)
        size = obj.sections[k].size
        for n, it in enumerate(lst):
            if it.size == 0:
                nxt = next((x.start for x in lst[n + 1:] if x.start > it.start), size)
                it.size = max(nxt - it.start, 1)
    return items

def parse_frame(obj):
    """The object's own EH frame block: dict(shndx, off, size, nfde, eh) or None."""
    fb = next((y for y in obj.symbols if y.name == "__FRAME_BEGIN__" and 0 < y.shndx < len(obj.sections)), None)
    if fb is None:
        return None
    d = obj.sections[fb.shndx].data
    q0 = fb.value

    def u32(o):
        return struct.unpack_from("<I", d, o)[0] if o + 4 <= len(d) else 0
    clen = u32(q0)
    try:
        aug = d[q0 + 9:d.index(b"\0", q0 + 9)].decode("latin1")
    except ValueError:
        aug = ""
    q, n = q0 + 4 + clen, 0
    while q + 16 <= len(d):
        L = u32(q)
        if L == 0 or q + 4 - u32(q + 4) != q0:
            break
        n += 1
        q += 4 + L
    end = q + 4 if u32(q) == 0 and q + 4 <= len(d) else q
    return dict(shndx=fb.shndx, off=q0, size=end - q0, nfde=n, eh=aug == "eh")

def add_anonymous(obj, items, offsets):
    """Anonymous pieces (jump tables, literals, string constants) at the
    referenced offsets that no named symbol covers; each extends to the next
    piece or symbol."""
    for k, offs in offsets.items():
        lst = items[k]
        named = list(lst)
        for off in sorted(offs):
            if any(i.start <= off < i.start + i.size for i in named):
                continue
            if any(i.start == off for i in lst):
                continue
            lst.append(Item(obj.sections[k].name, off, 0, "<%s+0x%X>" % (obj.sections[k].name, off)))
        lst.sort(key=lambda i: i.start)
        size = obj.sections[k].size
        for n, it in enumerate(lst):
            if it.name.startswith("<") and it.size == 0:
                nxt = next((x.start for x in lst[n + 1:] if x.start > it.start), size)
                it.size = max(nxt - it.start, 1)

def data_refs(obj, y, sec, c):
    """[(sym, objsec index or None, offset or None, retail address, insn index)] of one function."""
    o2r = {i: j for i, j, ok in c["pairs"] if i is not None and j is not None}
    ow, rw = c["ow"], c["rw"]
    rels = sorted([r for r in obj.relocs.get(sec.index, []) if y.value <= r.offset < y.value + y.size],
                  key=lambda r: r.offset)
    out, pending = [], []

    def add(sym, addend, retail, i):
        if sym is None:
            return
        if sym.type == 3:
            if obj.sections[sym.shndx].name in DATA_KINDS:
                out.append((sym, sym.shndx, addend, retail, i))
        elif sym.shndx not in (SHN_UNDEF, SHN_ABS, SHN_COMMON) and sym.shndx < len(obj.sections) \
                and obj.sections[sym.shndx].name in DATA_KINDS:
            out.append((sym, sym.shndx, sym.value + addend, retail, i))
        elif sym.shndx in COMMONS:
            out.append((sym, "COMMON", addend, retail, i))       # a C tentative definition
        else:
            out.append((sym, None, addend, retail, i))           # extern

    for r in rels:
        i = (r.offset - y.value) // 4
        if r.type == R_MIPS_HI16:
            pending.append((r, i))
        elif r.type == R_MIPS_LO16:
            his = [(h, hi) for h, hi in pending if h.sym is r.sym or (h.sym and r.sym and h.sym.name == r.sym.name
                                                                     and h.sym.shndx == r.sym.shndx)]
            for h, hi in his:
                pending.remove((h, hi))
                if hi in o2r and i in o2r:
                    addend = ((ow[hi] & 0xFFFF) << 16) + dataref.sx16(ow[i])
                    retail = (((rw[o2r[hi]] & 0xFFFF) << 16) + dataref.sx16(rw[o2r[i]])) & 0xFFFFFFFF
                    add(r.sym, addend, retail, i)
        elif r.type in (R_MIPS_GPREL16, R_MIPS_LITERAL):
            if i not in o2r:
                continue
            addend = dataref.sx16(ow[i])
            if r.sym is not None and r.sym.type == 3:
                addend += dataref.gp0(obj)
            add(r.sym, addend, (GP + dataref.sx16(rw[o2r[i]])) & 0xFFFFFFFF, i)
    return out

class FileLayout:
    def __init__(self, rel, obj, mapped, sm):
        self.rel, self.obj = rel, obj
        self.funcs = sorted(a for y, s, a, nm in mapped)
        self.fnames = {a: nm for y, s, a, nm in mapped}
        self.textpos = {a: (s.index, y.value) for y, s, a, nm in mapped}      # retail -> (section, offset)
        self.textsize = {s.index: s.size for y, s, a, nm in mapped}
        self.items = object_items(obj)
        self.frame = parse_frame(obj)
        self.frame_item = None
        self.frame_match = None             # retail EH object the frame block was matched with
        if self.frame:
            it = Item(obj.sections[self.frame["shndx"]].name, self.frame["off"], self.frame["size"], "<EH frame>")
            self.items[self.frame["shndx"]].append(it)
            self.items[self.frame["shndx"]].sort(key=lambda i: i.start)
            it.sized = True
            self.frame_item = it
        self.externs = collections.defaultdict(set)          # name -> retail addresses
        self.extern_users = collections.defaultdict(set)
        self.exact = {}
        self.notes_global = []
        localmap = {(s.index, y.value): a for y, s, a, nm in mapped}
        allrefs = []
        for y, s, a, nm in sorted(mapped, key=lambda t: t[2]):
            c = score.compare(obj, y, s, a, sm, localmap)
            self.exact[a] = c["exact"]
            for r in data_refs(obj, y, s, c):
                allrefs.append((a,) + r)
        offsets = collections.defaultdict(set)
        for a, sym, shndx, off, retail, i in allrefs:
            if shndx is not None and shndx != "COMMON":
                offsets[shndx].add(off)
        add_anonymous(obj, self.items, offsets)
        for a, sym, shndx, off, retail, i in allrefs:
            if shndx is None:
                self.externs[sym.name].add(retail - off)
                self.extern_users[sym.name].add(a)
                continue
            if shndx == "COMMON":
                # common symbols: allocated by the linker after every object's
                # .bss; an item of a pseudo section with its own retail base
                it = next((x for x in self.items["COMMON"] if x.name == sym.name), None)
                if it is None:
                    it = Item("COMMON", 0, sym.size, sym.name)
                    it.sized = True
                    self.items["COMMON"].append(it)
                start = retail - off
                if it.retail is None:
                    it.retail = start
                elif it.retail != start and start not in it.conflict:
                    it.conflict.append(start)
                it.users.add(a)
                it.refs.append((a, i, off, retail))
                continue
            it = self.item_at(shndx, off)
            start = retail - (off - it.start)
            if it.retail is None:
                it.retail = start
            elif it.retail != start and start not in it.conflict:
                it.conflict.append(start)
            it.users.add(a)
            it.refs.append((a, i, off, retail))
        self.annotate_from_source()

    def annotate_from_source(self):
        """Retail addresses of items no function of this file references, from
        the `@0x...` annotation of their definition (on the line, or in the
        comment block right above it)."""
        unplaced = {it.name: it for lst in self.items.values() for it in lst
                    if it.retail is None and not it.name.startswith("<")}
        if not unplaced:
            return
        lines = open(os.path.join(ROOT, self.rel), encoding="utf-8", errors="replace").read().split("\n")
        at = re.compile(r"@\s*(0x01[0-9A-Fa-f]{6})")
        for i, L in enumerate(lines):
            m = at.search(L)
            if not m:
                continue
            for j in range(i, min(i + 12, len(lines))):
                if j > i and at.search(lines[j]):
                    break
                if j > i and lines[j].lstrip().startswith(("*", "/*", "//")):
                    continue                            # the rest of the comment block
                code = lines[j].split("/*")[0].split("//")[0] if j > i else L.split("@")[0]
                hit = [nm for nm in unplaced if re.search(r"\b%s\b" % re.escape(nm), code)]
                if hit:
                    it = unplaced.pop(hit[0])
                    it.retail, it.via = int(m.group(1), 16), "annotation"
                    break
                if j > i and code.strip():
                    break                               # another declaration owns the annotation
        dm = dataref.datamap()                          # the tree's other annotations (headers)
        for nm, it in unplaced.items():
            if nm in dm and it.retail is None:
                it.retail, it.via = dm[nm], "annotation"

    def resolve_externs(self, layouts):
        """Retail addresses of this file's items that only OTHER files' functions
        use (as externs), from those files' aligned references."""
        for lst in self.items.values():
            for it in lst:
                if it.name.startswith("<") or (it.retail is not None and it.via is None):
                    continue
                for L in layouts:
                    if L is not self and it.name in L.externs:
                        if it.retail is None:
                            it.retail = min(L.externs[it.name])
                            it.via = "used by " + L.rel
                        it.ext_users |= set(L.extern_users[it.name])

    def item_at(self, shndx, off):
        for it in self.items[shndx]:
            if it.start <= off < it.start + max(it.size, 1):
                return it
        raise KeyError((shndx, off))

    def placed(self, commons=False):
        """Items with a retail address; common symbols that the linker put
        with the commons (after all objects) only with commons=True."""
        return [it for lst in self.items.values() for it in lst if it.retail is not None
                and (commons or it.sec != "COMMON" or not in_common_area(it.retail))]

    def sec_index(self, it):
        for k, lst in self.items.items():
            if it in lst:
                return k
        return None

    # ---- EH frame block: ours against retail
    def match_frame_1to1(self):
        """A file that is exactly one retail EH object: its frame block is that
        object's (if the size agrees), at the object's retail address."""
        if not self.frame or not self.funcs:
            return
        objs = {id(ehobj_of(a)): ehobj_of(a) for a in self.funcs}
        if len(objs) != 1 or None in [ehobj_of(a) for a in self.funcs]:
            return
        o = list(objs.values())[0]
        if set(o["funcs"]) != set(self.funcs):
            return
        self.frame_match = o
        if self.frame["size"] == o["size"]:
            self.frame_item.retail, self.frame_item.via = o["frame"][0], "EH frame %s" % eh_name(o)

    def eh_notes(self, objs):
        """Per inferred object: compile-flag (EH) and frame-block differences."""
        notes, flags = [], []
        F = self.funcs
        for k0, k1 in objs:
            fs = F[k0:k1 + 1]
            ret = [ehobj_of(a) for a in fs]
            ids = sorted({o["id"] for o in ret if o})
            where = "%s..%s" % (fname(fs[0]), fname(fs[-1])) if len(fs) > 1 else fname(fs[0])
            if not ids and self.frame:
                flags.append("%s: retail has no EH frame block for these functions, but this file is compiled with "
                             "exceptions (its object has one): -fno-exceptions (or C) in ref/compile_flags.json" % where)
            elif ids and not self.frame:
                flags.append("%s: retail object(s) %s have an EH frame block, but this file is compiled without "
                             "exceptions" % (where, ", ".join("#%d" % i for i in ids)))
            elif ids and None in ret:
                notes.append("%s: some functions have FDEs (%s), some do not: several objects" % (
                    where, ", ".join("#%d" % i for i in ids)))
        if self.frame and self.frame_match:
            o = self.frame_match
            if self.frame["size"] != o["size"] or self.frame["nfde"] != len(o["fdes"]):
                notes.append("EH frame block: 0x%X bytes / %d FDEs here, retail %s 0x%X bytes / %d FDEs" % (
                    self.frame["size"], self.frame["nfde"], eh_name(o), o["size"], len(o["fdes"])))
            if self.frame["eh"] != o["eh"]:
                notes.append("EH frame block: augmentation %r here, %r in retail %s (exception regions %s)" % (
                    "eh" if self.frame["eh"] else "", "eh" if o["eh"] else "", eh_name(o),
                    "missing here" if o["eh"] else "only here"))
        return notes, flags

    # ---- text
    def contiguous(self, a, b):
        """Nothing but this file's own (possibly unmapped) functions between
        retail functions a < b."""
        return all(owner(x) == self.rel for x in FSTARTS[FINDEX[a] + 1:FINDEX[b]])

    def runs(self):
        """[(first, last)] runs of consecutive retail functions all in this file."""
        runs = []
        for a in self.funcs:
            if runs and self.contiguous(runs[-1][1], a):
                runs[-1][1] = a
            else:
                runs.append([a, a])
        return [tuple(r) for r in runs]

    # ---- base groups per object section
    def base_groups(self):
        g = collections.defaultdict(lambda: collections.defaultdict(list))
        for shndx, lst in self.items.items():
            if shndx == "COMMON":
                continue
            for it in lst:
                if it.retail is not None:
                    g[shndx][it.retail - it.start].append(it)
        return g

    # ---- splits: landmarks between two items of the file
    def splits(self):
        by_sec = collections.defaultdict(list)
        for it in self.placed():
            by_sec[it.rsec].append(it)
        out = []
        for rs, lst in by_sec.items():
            lst.sort(key=lambda i: i.retail)
            for a, b in zip(lst, lst[1:]):
                lo, hi = a.end, b.retail
                if hi <= lo:
                    continue
                # (the file's own annotations and its own functions' retail
                # references are its own data - or externs it shares with the
                # neighbours anyway - possibly under another spelling or offset
                # than the object's items: not evidence of a split)
                between = [m for m in LANDMARKS if lo <= m[0] < hi and m[3] != self.rel
                           and not self.inside_own(m[0])]
                gap = hi - lo
                if between:
                    out.append(dict(sec=rs, lower=a, upper=b, gap=gap, between=between, hard=True))
                elif gap >= 16:
                    out.append(dict(sec=rs, lower=a, upper=b, gap=gap, between=[], hard=False))
        return out

    def inside_own(self, addr):
        for it in self.placed():
            if it.retail <= addr < it.end:
                return True
        return False

    # ---- inference
    def infer(self):
        F = self.funcs
        n = len(F)
        pos_of = {a: k for k, a in enumerate(F)}
        # position p = boundary between F[p] and F[p+1]
        forced, why_forced = set(), {}
        for p in range(n - 1):
            if not self.contiguous(F[p], F[p + 1]):
                forced.add(p)
                between = FSTARTS[FINDEX[F[p]] + 1:FINDEX[F[p + 1]]]
                why_forced[p] = "other files' functions between: " + ", ".join(
                    sorted({owner(x) for x in between}))
            oa, ob = ehobj_of(F[p]), ehobj_of(F[p + 1])
            if (oa and oa["id"]) != (ob and ob["id"]) and (oa or ob):
                forced.add(p)
                why_forced.setdefault(p, "EH object changes (#%s -> #%s)" % (oa and oa["id"], ob and ob["id"]))
        invalid = set()
        for o in EHOBJS:
            ks = [pos_of[a] for a in o["funcs"] if a in pos_of]
            if ks:
                invalid.update(range(min(ks), max(ks)))
        for it in self.placed():
            ks = sorted(pos_of[u] for u in it.users if u in pos_of)
            if ks:
                invalid.update(range(ks[0], ks[-1]))
        need = []                                   # (lo position, hi position, reason)
        anomalies = []
        for s in self.splits():
            if not s["hard"]:
                continue
            a, b = s["lower"], s["upper"]
            ua = [pos_of[u] for u in a.users if u in pos_of]
            ub = [pos_of[u] for u in b.users if u in pos_of]
            if not ua or not ub:
                continue
            lo, hi = max(ua), min(ub) - 1
            reason = "%s %s @0x%08X | %s | %s @0x%08X" % (s["sec"], a.label(), a.retail, s["between"][0][2],
                                                          b.label(), b.retail)
            if lo > hi:
                shared = sorted(nm for nm, addrs in self.externs.items()
                                if any(a.end <= x < b.retail for x in addrs))
                anomalies.append("data order contradicts text order: " + reason +
                                 " (users %s / %s)" % (", ".join(fname(F[k]) for k in sorted(set(ua))),
                                                         ", ".join(fname(F[k]) for k in sorted(set(ub)))) +
                                 ("; this file uses %s there as extern(s): defined in this object, they would "
                                  "make it one contiguous object" % ", ".join(shared) if shared else ""))
                continue
            need.append((lo, hi, reason))
        # minimal boundaries (interval stabbing over valid positions; forced ones first)
        chosen = {}                                    # position -> (lo, hi, reasons)
        for p in sorted(forced):
            chosen[p] = (p, p, [why_forced[p]])
        for lo, hi, reason in sorted(need, key=lambda t: t[1]):
            valid = [p for p in range(lo, hi + 1) if p not in invalid or p in forced]
            if not valid:
                anomalies.append("no valid boundary position for: " + reason)
                continue
            hit = [p for p in chosen if lo <= p <= hi]
            if hit:
                chosen[hit[0]][2].append(reason)
                continue
            chosen[valid[-1]] = (valid[0], valid[-1], [reason])
        bounds = [(p,) + chosen[p] for p in sorted(chosen)]
        # objects (each boundary at its latest valid position)
        objs, start = [], 0
        for p, lo, hi, reasons in bounds:
            objs.append((start, p))
            start = p + 1
        if n:
            objs.append((start, n - 1))
        return bounds, objs, anomalies, invalid

    def elsewhere(self, LM):
        """Items no function of this file uses (placed by an annotation or by
        other files' references) that other owners' data separates from all of
        this file's used data in their section: defined in another object."""
        out = []
        used = [it for it in self.placed() if it.users]
        for it in self.placed():
            if it.users or it is self.frame_item:
                continue
            mine = [u for u in used if u.rsec == it.rsec]
            if not mine:
                # no used data of this file there: compare the neighbours'
                # text with this file's (one run of text only)
                runs = self.runs()
                if len(runs) != 1 or in_fsec(runs[0][0]):
                    continue
                t0, t1 = runs[0]
                b = [m for m in LM.below(it.rsec, it.retail, self.rel) if not m[5]]
                a = [m for m in LM.above(it.rsec, it.end, self.rel) if not m[5]]
                late = [m for m in b if m[4] is not None and not in_fsec(m[4]) and m[4] > t1]
                early = [m for m in a if m[4] is not None and not in_fsec(m[4]) and m[4] < t0]
                if (b and len(late) == len(b)) or (a and len(early) == len(a)):
                    out.append("%s @0x%08X (%s) lies between %s and %s, but this file's text (0x%08X..0x%08X) is "
                               "linked %s them: it belongs to another object" % (
                                   it.label(), it.retail, it.via, fmt_lm(LM.below(it.rsec, it.retail, self.rel)),
                                   fmt_lm(LM.above(it.rsec, it.end, self.rel)), t0, FUNCS[t1]["end"],
                                   "before" if late else "after"))
                continue

            def foreign(lo, hi):
                return [m for m in LM.by_sec.get(it.rsec, []) if lo <= m[0] < hi and m[3] != self.rel]
            lower = [u for u in mine if u.end <= it.retail]
            upper = [u for u in mine if u.retail >= it.end]
            sides = []
            if lower:
                sides.append(foreign(max(u.end for u in lower), it.retail))
            if upper:
                sides.append(foreign(it.end, min(u.retail for u in upper)))
            if sides and all(sides):
                cut = sides[-1][0] if upper else sides[0][-1]
                out.append("%s @0x%08X (%s) is cut off from this file's other %s data by %s: it belongs to another "
                           "object, linked between %s and %s" % (
                               it.label(), it.retail, it.via, it.rsec, fmt_lm([cut]),
                               fmt_lm(LM.below(it.rsec, it.retail, self.rel)), fmt_lm(LM.above(it.rsec, it.end, self.rel))))
        return out

    def text_drift(self, k0, k1):
        """Where this object's .text offsets stop matching retail's: a function
        before it takes another number of bytes here (code size or padding).
        Local code addresses (static callbacks) then point elsewhere, and the
        object could not be linked to the retail image."""
        out = []
        fs = self.funcs[k0:k1 + 1]
        by_sec = collections.defaultdict(list)
        for a in fs:
            by_sec[self.textpos[a][0]].append(a)
        for k, v in by_sec.items():
            v.sort()
            offs = sorted((self.textpos[a][1], a) for a in self.funcs if self.textpos[a][0] == k)
            for p, q in zip(v, v[1:]):
                op, oq = self.textpos[p][1], self.textpos[q][1]
                here, there = oq - op, q - p
                if here == there or not self.contiguous(p, q):
                    continue
                between = [fname(a) for o, a in offs if op < o < oq]
                if here < 0 or between:
                    out.append("%s: emitted in another order (%s)" % (
                        fmt_fn(q), ("it comes before %s here" % fname(p)) if here < 0 else
                        "%s here between it and %s, which retail has elsewhere" % (", ".join(between[:4])
                                                                                   + (" ..." if len(between) > 4
                                                                                      else ""), fname(p))))
                else:
                    out.append("%s: %+d bytes from here on (%s takes 0x%X bytes here, 0x%X in retail%s)" % (
                        fmt_fn(q), here - there, fname(p), here, there,
                        "" if self.exact.get(p) else "; not exact"))
        return out

    def object_items_of(self, k0, k1):
        fs = set(self.funcs[k0:k1 + 1])
        its = [it for it in self.placed() if it.users & fs]
        if self.frame_item is not None and self.frame_item.retail is not None and \
                self.frame_match and set(self.frame_match["funcs"]) <= fs:
            its.append(self.frame_item)
        return its

    def object_report(self, its):
        """Checks inside one inferred object's items."""
        notes = []
        for shndx, rs in sorted({(self.sec_index(it), it.rsec) for it in its if it.sec != "COMMON"}, key=str):
            group = sorted([it for it in its if self.sec_index(it) == shndx and it.rsec == rs], key=lambda i: i.start)
            bases = sorted({it.retail - it.start for it in group})
            if len(bases) > 1:
                by_r = sorted(group, key=lambda i: i.retail)
                if [i.start for i in by_r] != sorted(i.start for i in by_r):
                    notes.append("%s: items in another order than in retail %s (retail order: %s)" % (
                        self.obj.sections[shndx].name, rs, ", ".join(i.label() for i in by_r)))
                else:
                    notes.append("%s: gaps differ from retail %s (%s)" % (
                        self.obj.sections[shndx].name, rs,
                        ", ".join("%s +0x%X -> 0x%08X" % (i.label(), i.start, i.retail) for i in group)))
        commons = sorted((it for it in its if it.sec == "COMMON"), key=lambda i: i.retail)
        inside = [i for i in commons if not in_common_area(i.retail)]
        if inside:
            notes.append("common symbol(s) here, but retail has them among the objects' sections, not with the "
                         "commons after them (%s): %s - in the original object's own section (static, initialised, "
                         "or -fno-common)" % (", ".join("%s from 0x%08X" % kv for kv in sorted(COMMON_LOW.items())),
                                              ", ".join("%s @0x%08X %s" % (i.label(), i.retail, i.rsec)
                                                        for i in inside)))
        for it in its:
            if it.sec == "COMMON":
                continue
            if it.rsec and it.rsec != it.sec:
                notes.append("%s is in object %s but retail %s%s" % (
                    it.label(), it.sec, it.rsec, " (writable in the original: not const)"
                    if it.sec == ".rodata" and it.rsec == ".data" else ""))
            if it.conflict:
                notes.append("%s: references imply several retail addresses %s" % (
                    it.label(), ", ".join("0x%08X" % x for x in [it.retail] + it.conflict)))
        return notes

# ------------------------------------------------------------------ the whole tree: owned landmarks

class Landmarks:
    """Retail data addresses with their OWNER (the file whose object holds
    them, None if unknown) and the text position of that object (a function
    start, or None for data-only / unknown).  Sorted per retail section.
    Tuples (addr, end, label, owner, text_pos, weak, sized): `weak` = the
    owner's own code does not reference it (placed by an annotation or by
    other files' references), so the ownership is a claim of the source tree,
    not evidence; `sized` = a symbol with a real size (not a guessed extent)."""

    def __init__(self, layouts):
        self.lm = []
        defined = set()
        owned = []
        for L in layouts:
            single_run = L.funcs[0] if L.funcs and len(L.runs()) == 1 else None
            for it in L.placed():
                if it is L.frame_item:
                    continue                            # the retail frame landmark covers it
                users = sorted(it.users)
                tp = users[0] if users else single_run
                self.lm.append((it.retail, it.end, it.label(), L.rel, tp, not it.users, it.sized))
                owned.append((it.retail, it.end))
                if not it.name.startswith("<"):
                    defined.add(it.name)
        merged = []                                     # union of the owned ranges
        for lo, hi in sorted(owned):
            if merged and lo <= merged[-1][1]:
                merged[-1][1] = max(merged[-1][1], hi)
            else:
                merged.append([lo, hi])
        starts = [m[0] for m in merged]

        def inside_owned(a):
            i = bisect.bisect_right(starts, a) - 1
            return i >= 0 and a < merged[i][1]
        compiled = {L.rel for L in layouts}
        for nm, a, rel in ANNOT:
            if nm in defined or inside_owned(a):
                continue
            own = rel if rel.endswith(".S") or rel.startswith("link/") else None
            self.lm.append((a, a + 1, "%s (@ in %s)" % (nm, rel), own, None, True, False))
        for t, f, x in RREFS:
            o = owner(f)
            if o in compiled or inside_owned(t):
                continue                                # compiled files: their items are above
            self.lm.append((t, t + 1, "ref by %s" % fname(f), o, f, False, False))
        for o in EHOBJS:
            lo, hi = o["frame"]
            own = o["files"][0] if len(o["files"]) == 1 else None
            self.lm.append((lo, hi, "EH frame %s%s" % (eh_name(o), "" if own or not o["files"] else
                                                        " of " + " + ".join(o["files"])),
                            own or "EH %s" % eh_name(o), o["text"][0] if o["text"] else None, False, True))
        self.lm.sort()
        self.by_sec = collections.defaultdict(list)
        for m in self.lm:
            self.by_sec[rsec(m[0])].append(m)
        self.keys = {s: [m[0] for m in v] for s, v in self.by_sec.items()}

    def below(self, sec, addr, me, skip=None):
        """The landmarks at the nearest address below addr that are not `me`'s
        (or, with `skip`, not one of these (addr, label) of `me`): several when
        files disagree about who owns it."""
        lst, ks = self.by_sec.get(sec, []), self.keys.get(sec, [])
        mine = (lambda m: m[3] == me) if skip is None else (lambda m: m[3] == me and (m[0], m[2]) in skip)
        i = bisect.bisect_left(ks, addr) - 1
        while i >= 0 and mine(lst[i]):
            i -= 1
        if i < 0:
            return []
        return [m for m in lst if m[0] == lst[i][0] and not mine(m)]

    def above(self, sec, addr, me, skip=None):
        """The same at or above addr."""
        lst, ks = self.by_sec.get(sec, []), self.keys.get(sec, [])
        mine = (lambda m: m[3] == me) if skip is None else (lambda m: m[3] == me and (m[0], m[2]) in skip)
        i = bisect.bisect_left(ks, addr)
        while i < len(lst) and mine(lst[i]):
            i += 1
        if i >= len(lst):
            return []
        return [m for m in lst if m[0] == lst[i][0] and not mine(m)]

    def overlaps(self):
        """Retail bytes claimed by two compiled files."""
        out = []
        files = sorted(m for m in self.lm if m[6] and m[3] and not m[3].startswith("EH ")
                       and not m[2].startswith("EH frame"))
        for i, a in enumerate(files):
            for b in files[i + 1:]:
                if b[0] >= a[1]:
                    break
                if a[3] != b[3]:
                    out.append((a, b))
        return out

def match_data_only_frames(layouts, LM):
    """A data-only file with a frame block <-> a retail data-only EH object
    whose neighbours in .data order are the objects of the files around the
    file's items."""
    for L in layouts:
        if L.funcs or not L.frame or L.frame_item.retail is not None:
            continue
        cands = []
        for k, o in enumerate(EHOBJS):
            if o["fdes"] or o["size"] != L.frame["size"]:
                continue
            prev_files = set(EHOBJS[k - 1]["files"]) if k > 0 else set()
            next_files = set(EHOBJS[k + 1]["files"]) if k + 1 < len(EHOBJS) else set()
            ok = False
            for it in L.placed():
                b, a = LM.below(it.rsec, it.retail, L.rel), LM.above(it.rsec, it.end, L.rel)
                if any(m[3] in prev_files for m in b) and any(m[3] in next_files for m in a):
                    ok = True
            if ok:
                cands.append(o)
        if len(cands) == 1:
            o = cands[0]
            L.frame_match = o
            L.frame_item.retail, L.frame_item.via = o["frame"][0], "EH frame %s (data-only)" % eh_name(o)
            o["files"] = [L.rel]

# ------------------------------------------------------------------ reporting

def fmt_fn(a):
    return "%s 0x%08X" % (fname(a), a)

def pad_after(a):
    return FUNCS[a]["end"] - code_end(a)

def align_of(a):
    n = 1
    while n < 128 and a % (n * 2) == 0:
        n *= 2
    return n

def fmt_lm(ms):
    """Landmarks at one address (LM.below / LM.above)."""
    if not ms:
        return "(nothing known)"
    labels = list(collections.OrderedDict.fromkeys(m[2] for m in ms))
    owners = list(collections.OrderedDict.fromkeys(m[3] or "owner unknown" for m in ms))
    return "%s @0x%08X (%s)" % (" / ".join(labels), ms[0][0], " | ".join(owners))

def neighbours(L, LM, its, t0, t1, whole_file=True):
    """[(section, lo, hi, below, above, link-order notes, items)] of one
    object's items.  With several objects in the file, the file's other
    objects are neighbours too."""
    out = []
    secs = collections.defaultdict(list)
    for it in its:
        secs[it.rsec].append(it)
    skip = None if whole_file else {(it.retail, it.label()) for it in its}
    for rs, v in sorted(secs.items(), key=lambda t: min(i.retail for i in t[1])):
        lo, hi = min(i.retail for i in v), max(i.end for i in v)
        b, a = LM.below(rs, lo, L.rel, skip), LM.above(rs, hi, L.rel, skip)
        notes = []
        if t0 is not None and not in_fsec(t0):
            # only from evidence (not weak claims), and when every claimant disagrees
            strong = [m for m in b if not m[5]]
            bad = [m for m in strong if m[4] is not None and not in_fsec(m[4]) and m[4] > t1]
            if strong and len(bad) == len(strong):
                notes.append("%s 0x%08X follows %s, whose text (%s) comes after this object's (0x%08X..0x%08X): "
                             "linked later than that" % (rs, lo, fmt_lm(b), fmt_fn(bad[0][4]), t0, FUNCS[t1]["end"]))
            strong = [m for m in a if not m[5]]
            bad = [m for m in strong if m[4] is not None and not in_fsec(m[4]) and m[4] < t0]
            if strong and len(bad) == len(strong):
                notes.append("%s 0x%08X precedes %s, whose text (%s) comes before this object's (0x%08X..0x%08X): "
                             "linked earlier than that" % (rs, hi, fmt_lm(a), fmt_fn(bad[0][4]), t0, FUNCS[t1]["end"]))
        out.append((rs, lo, hi, b, a, notes, v))
    return out

def gaps(its):
    """Unattributed stretches (>= 16 bytes) between an object's items, per section."""
    out = []
    secs = collections.defaultdict(list)
    for it in its:
        secs[it.rsec].append(it)
    for rs, v in secs.items():
        v = sorted(v, key=lambda i: i.retail)
        for x, y in zip(v, v[1:]):
            if y.retail - x.end >= 16:
                between = [m for m in LANDMARKS if x.end <= m[0] < y.retail]
                out.append("%s 0x%08X..0x%08X (0x%X bytes) between %s and %s%s" % (
                    rs, x.end, y.retail, y.retail - x.end, x.label(), y.label(),
                    (": " + "; ".join(m[2] for m in between[:3])) if between else ": nothing known there"))
    return out

def report_file(L, LM, verbose):
    out = []
    F = L.funcs
    problems = collections.Counter()
    info = dict(file=L.rel, functions=len(F))
    if not F:
        its = L.placed()
        unplaced = [it for lst in L.items.values() for it in lst if it.retail is None]
        out.append("%s: data only (%d item(s) placed, %d not)" % (L.rel, len(its), len(unplaced)))
        if L.frame:
            if L.frame_match:
                o = L.frame_match
                k = o["id"]
                out.append("  EH frame block 0x%X bytes = retail data-only object %s @0x%08X (between %s and %s)" % (
                    L.frame["size"], eh_name(o), o["frame"][0],
                    ", ".join(EHOBJS[k - 1]["files"]) if k > 0 else "-",
                    ", ".join(EHOBJS[k + 1]["files"]) if k + 1 < len(EHOBJS) else "-"))
            else:
                out.append("  EH frame block 0x%X bytes: no retail data-only EH object fits (compiled with "
                           "exceptions; retail candidates: %s)" % (
                               L.frame["size"], ", ".join(eh_name(o) for o in EHOBJS if not o["fdes"]) or "none"))
                problems["flags"] += 1
        groups = L.base_groups()
        for k, g in sorted(groups.items()):
            if len(g) > 1:
                problems["bases"] += 1
                out.append("  object %s: %d base groups: %s" % (L.obj.sections[k].name, len(g), "; ".join(
                    "0x%08X (%s)" % (b, ", ".join(i.label() for i in v)) for b, v in sorted(g.items()))))
        for rs, lo, hi, b, a, notes, v in neighbours(L, LM, its, None, None):
            out.append("  %-8s 0x%08X..0x%08X %s" % (rs, lo, hi, ", ".join(i.label() for i in
                                                                           sorted(v, key=lambda i: i.retail))[:100]))
            out.append("           after %s; before %s" % (fmt_lm(b), fmt_lm(a)))
        for nt in L.object_report(its):
            out.append("  note: " + nt)
        if verbose and unplaced:
            out.append("  not placed: " + ", ".join("%s(%s+0x%X)" % (i.label(), i.sec, i.start) for i in unplaced))
        info.update(problems=sum(problems.values()), kinds=dict(problems), objects=[])
        return out, info
    runs = L.runs()
    nonexact = [a for a in F if not L.exact.get(a)]
    out.append("%s: %d functions, %d run(s) in .text%s" % (
        L.rel, len(F), len(runs), "" if not nonexact else "  (%d not exact: alignment of their refs may be off)"
        % len(nonexact)))
    for k, (a, b) in enumerate(runs):
        end = FUNCS[b]["end"]
        out.append("  text 0x%08X..0x%08X  (%s .. %s; start %d-aligned, %d pad byte(s) at the end)%s" % (
            a, end, fname(a), fname(b), align_of(a), pad_after(b),
            "  [function sections]" if in_fsec(a) else ""))
        if k + 1 < len(runs):
            others = collections.OrderedDict()
            for x in FSTARTS[FINDEX[b] + 1:FINDEX[runs[k + 1][0]]]:
                others.setdefault(owner(x), []).append(x)
            for f, xs in others.items():
                out.append("    then %s: 0x%08X..0x%08X (%d functions)" % (f, xs[0], FUNCS[xs[-1]]["end"], len(xs)))
    eh = collections.OrderedDict()
    for a in F:
        o = ehobj_of(a)
        eh.setdefault(o["id"] if o else None, []).append(a)
    for oid, xs in eh.items():
        if oid is None:
            out.append("  no EH frame: %d function(s) (C, or C++ without exceptions)" % len(xs))
        else:
            o = EHOBJS[oid]
            out.append("  EH object #%d: frame 0x%08X..0x%08X%s, FDEs 0x%08X..0x%08X: %d of its %d functions here%s" % (
                oid, o["frame"][0], o["frame"][1], " (eh)" if o["eh"] else "", o["text"][0], o["text"][1], len(xs),
                len(o["fdes"]), "" if o["files"] == [L.rel] else " (files: %s)" % ", ".join(o["files"])))
    out.append("  this object: %s" % (
        "EH frame block 0x%X bytes, %d FDEs%s%s" % (L.frame["size"], L.frame["nfde"], " (eh)" if L.frame["eh"] else "",
                                                     (" = retail %s" % eh_name(L.frame_match)
                                                      if L.frame_item.retail is not None else ""))
        if L.frame else "no EH frame block (compiled without exceptions, or C)"))
    groups = L.base_groups()
    for k, g in sorted(groups.items()):
        sname = L.obj.sections[k].name
        if len(g) > 1:
            problems["bases"] += 1
        if len(g) > 1 or verbose:
            out.append("  object %s: %d base group(s)" % (sname, len(g)))
            for base, its in sorted(g.items(), key=lambda t: min(i.retail for i in t[1])):
                out.append("    base 0x%08X: %s" % (base, ", ".join(
                    "%s(+0x%X->0x%08X %s)" % (i.label(), i.start, i.retail, i.rsec) for i in its)))
    bounds, objs, anomalies, invalid = L.infer()
    if verbose:
        for s in L.splits():
            out.append("  %s %s: %s @0x%08X .. %s @0x%08X, gap 0x%X%s" % (
                "SPLIT" if s["hard"] else "gap  ", s["sec"], s["lower"].label(), s["lower"].retail,
                s["upper"].label(), s["upper"].retail, s["gap"],
                (": " + "; ".join(m[2] for m in s["between"][:3]) + (" ..." if len(s["between"]) > 3 else ""))
                if s["between"] else " (no landmark: unattributed bytes)"))
    if len(objs) > 1 or anomalies:
        out.append("  inferred: %d object(s)" % len(objs))
        for p, lo, hi, reasons in bounds:
            if lo == hi:
                where = "between %s and %s" % (fmt_fn(F[p]), fmt_fn(F[p + 1]))
            else:
                where = "after %s and before %s (undetermined: %s)" % (
                    fmt_fn(F[lo]), fmt_fn(F[hi + 1]), ", ".join(fname(x) for x in F[lo + 1:hi + 1]))
            out.append("    boundary %s" % where)
            for r in reasons:
                out.append("      because %s" % r)
    if len(objs) > 1:
        problems["objects"] += len(objs) - 1
    order = []
    obj_info = []
    for k0, k1 in objs:
        its = L.object_items_of(k0, k1)
        notes = L.object_report(its)
        nb = neighbours(L, LM, its, F[k0], F[k1], whole_file=len(objs) == 1)
        out.append("  object 0x%08X..0x%08X (%d function%s)%s" % (
            F[k0], FUNCS[F[k1]]["end"], k1 - k0 + 1, "" if k1 == k0 else "s", "" if its else ": no data refs"))
        for rs, lo, hi, b, a, onotes, v in nb:
            out.append("    %-8s 0x%08X..0x%08X %d item(s)%s; after %s; before %s" % (
                rs, lo, hi, len(v), (": " + ", ".join(i.label() for i in sorted(v, key=lambda i: i.retail)[:3])
                                      + (" ..." if len(v) > 3 else "")) if verbose else "", fmt_lm(b), fmt_lm(a)))
            order += onotes
        drift = L.text_drift(k0, k1)
        for d in drift:
            out.append("    text drift: " + d)
        problems["text"] += len(drift)
        if verbose:
            for g in gaps(its):
                out.append("    gap " + g)
        for nt in notes:
            out.append("    note: " + nt)
        obj_info.append(dict(first=F[k0], last=F[k1], functions=F[k0:k1 + 1], notes=notes, sections=[
            dict(section=rs, lo=lo, hi=hi, after=[[m[0], m[2], m[3]] for m in b],
                 before=[[m[0], m[2], m[3]] for m in a]) for rs, lo, hi, b, a, _, v in nb]))
    ehn, flags = L.eh_notes(objs)
    away = L.elsewhere(LM)
    for nt in away:
        out.append("  ELSEWHERE: " + nt)
    problems["elsewhere"] += len(away)
    for nt in ehn:
        out.append("  EH: " + nt)
    for nt in flags:
        out.append("  FLAGS: " + nt)
    for nt in order:
        out.append("  LINK ORDER: " + nt)
    for an in anomalies:
        out.append("  ANOMALY: " + an)
    problems["flags"] += len(flags)
    problems["eh"] += len(ehn)
    problems["link_order"] += len(order)
    problems["anomalies"] += len(anomalies)
    unused = [it for lst in L.items.values() for it in lst if it.retail is None and not it.name.startswith("<")]
    if unused and verbose:
        out.append("  items with no retail address: " + ", ".join(
            "%s(%s+0x%X)" % (i.label(), i.sec, i.start) for i in unused))
    if verbose:
        out.append("  items:")
        for it in sorted(L.placed(), key=lambda i: (str(i.rsec), i.retail)):
            out.append("    %-34s %-9s +0x%04X size 0x%04X -> 0x%08X %-8s %s" % (
                it.label()[:34], it.sec, it.start, it.size, it.retail, it.rsec,
                ("users " + ", ".join(fname(u) for u in sorted(it.users))[:120]) if it.users else
                ("(%s)" % it.via)))
        for nm, addrs in sorted(L.externs.items()):
            out.append("    extern %-27s -> %s  users %s" % (nm[:27], ", ".join("0x%08X" % x for x in sorted(addrs)),
                                                           ", ".join(fname(u) for u in sorted(L.extern_users[nm]))[:100]))
    info.update(runs=[list(r) for r in runs], objects=obj_info,
                boundaries=[dict(after=F[p], earliest_after=F[lo], latest_after=F[hi], reasons=r)
                            for p, lo, hi, r in bounds],
                base_groups={L.obj.sections[k].name: {"0x%08X" % b: [i.label() for i in its] for b, its in g.items()}
                             for k, g in groups.items()},
                eh=ehn, flags=flags, anomalies=anomalies, link_order=order, elsewhere=away,
                problems=sum(problems.values()), kinds=dict(problems))
    return out, info

def report_objects():
    out = ["Retail objects with an EH frame block (collect2 table @0x01026C00, %d entries), in .data order:"
           % len(FRAME_TABLE)]
    for k, o in enumerate(EHOBJS):
        t = ("text 0x%08X..0x%08X, %2d FDEs" % (o["text"][0], o["text"][1], len(o["fdes"]))) if o["text"] \
            else "no functions (a data-only object)"
        gap = o["frame"][0] - EHOBJS[k - 1]["frame"][1] if k else 0
        out.append("  #%-2d frame 0x%08X..0x%08X%s  %s  %s%s" % (
            o["id"], o["frame"][0], o["frame"][1], " eh" if o["eh"] else "   ", t, ", ".join(o["files"]),
            "  [.data before it: 0x%X bytes]" % gap if gap else ""))
    out.append("  (an object's .data precedes its frame block; \".data before it\" is that, plus the .data of any "
               "objects without a block linked in between)")
    return out

def report_retail(lo, hi):
    out = ["Known retail data in 0x%08X..0x%08X:" % (lo, hi)]
    seen = set()
    for m in LANDMARKS:
        if lo <= m[0] < hi and (m[0], m[2]) not in seen:
            seen.add((m[0], m[2]))
            out.append("  0x%08X %-8s %s" % (m[0], rsec(m[0]), m[2]))
    return out

def analyze(jobs):
    """FileLayouts of every compiled file of the tree, and the owned landmarks."""
    rels = [r for r in score.sources([]) if not r.endswith(".S")]
    results = compile_all(rels, jobs)
    sm = symmap()
    layouts, failed = [], []
    for rel, ok, err in results:
        if not ok:
            failed.append((rel, err))
            continue
        obj = ElfObject(score.obj_path(rel))
        mapped, orphans = score.map_functions(rel, obj)
        layouts.append(FileLayout(rel, obj, mapped, sm))
    for L in layouts:
        L.resolve_externs(layouts)
    for L in layouts:
        L.match_frame_1to1()
    LM = Landmarks(layouts)
    match_data_only_frames(layouts, LM)
    LM = Landmarks(layouts)
    return layouts, LM, failed

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--all", action="store_true", help="report every file, not only those with problems")
    ap.add_argument("--objects", action="store_true", help="list the retail EH objects and exit")
    ap.add_argument("--retail", nargs=2, metavar=("LO", "HI"), help="list what is known at these retail addresses")
    ap.add_argument("--json", metavar="FILE")
    ap.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 4))
    a = ap.parse_args()
    if a.objects:
        print("\n".join(report_objects()))
        return 0
    if a.retail:
        print("\n".join(report_retail(int(a.retail[0], 16), int(a.retail[1], 16))))
        return 0
    want = score.sources(a.files) if a.files else None
    if a.files and not want:
        print("no such source file: %s" % " ".join(a.files))
        return 1
    layouts, LM, failed = analyze(a.jobs)
    for rel, err in failed:
        if want is None or rel in want:
            print("%s: does not compile with the retail compiler\n%s" % (rel, err.strip()[-600:]))
    infos, problem_files = [], []
    for L in layouts:
        lines, info = report_file(L, LM, a.verbose)
        infos.append(info)
        show = (L.rel in want) if want is not None else (a.all or a.verbose or info["problems"])
        if show:
            print("\n".join(lines))
            print()
        if info["problems"]:
            problem_files.append(info)
    overlaps = LM.overlaps()
    if want is None:
        print("\n".join(report_objects()))
        print()
        if overlaps:
            print("OVERLAP: retail bytes claimed by two files:")
            for x, y in overlaps:
                print("  0x%08X %s (%s)  /  0x%08X %s (%s)" % (x[0], x[2], x[3], y[0], y[2], y[3]))
            print()
        print("Files with layout problems: %d of %d" % (len(problem_files), len(infos)))
        for i in problem_files:
            print("  %-34s %s" % (i["file"], ", ".join("%s %d" % (k, v) for k, v in sorted(i["kinds"].items()) if v)))
    else:
        for x, y in overlaps:
            if x[3] in want or y[3] in want:
                print("OVERLAP: 0x%08X %s (%s)  /  0x%08X %s (%s)" % (x[0], x[2], x[3], y[0], y[2], y[3]))
    if a.json:
        json.dump(dict(files=infos, eh_objects=[dict(o, fdes=[list(f) for f in o["fdes"]]) for o in EHOBJS],
                       overlaps=[[list(x), list(y)] for x, y in overlaps]),
                  open(a.json, "w"), indent=1)
    return 0

if __name__ == "__main__":
    sys.exit(main())
