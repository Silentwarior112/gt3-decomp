"""Where each compiled object's data sections sat in the original, and what of
the original's data no object produces yet.

Sources of an object section's retail base:
  * build/score/layout.json (tools/score.py, full run): the base every
    reference of the object's code to the section implies (dataref.py);
  * the frame table collect2 wrote at 0x0028E780: one pointer per C++
    object, to the start of its .data (its exception frame info, egcs's
    `__FRAME_BEGIN__'), whose first FDE names one of the object's functions.

    python tools/datalayout.py                 summary per section kind
    python tools/datalayout.py --gaps          also every stretch no object covers
    python tools/datalayout.py --json out.json the map, for tools/fulllink.py

The original's .data (0x0028E760..): crt0's, collect2's frame table and
constructor / destructor lists, then every object's .data in link order; then
.rodata, .gcc_except_table, .sdata (see the summary for the boundaries).
"""
import argparse
import collections
import json
import os
import struct
import sys

import paths
import funcmap

ROOT = paths.ROOT
IMAGE = paths.RETAIL_ELF                # the original program (the relink's core.bin is the same bytes)
# the byte-identical relink of the split (tools/relink.py): its symbols name
# what the original code references; tools/fulllink.py --relink sets it
RELINK_DIR = os.path.join(ROOT, "build", "regress", "link")
VRAM, ROM = 0x00100000, 0x1000
FRAME_TABLE = 0x0028E780
DATA_LO, DATA_HI = 0x0028E760, 0x003531E3                               # initialised data
BSS_LO, BSS_HI = 0x00353200, 0x004B2C0C

_IMG = None


def image():
    global _IMG
    if _IMG is None:
        if not os.path.exists(IMAGE):
            sys.exit("no %s: run tools/build.py (it makes it from baserom/CORE.GT3)" % IMAGE)
        _IMG = open(IMAGE, "rb").read()
    return _IMG


def word(a):
    return struct.unpack_from("<I", image(), a - VRAM + ROM)[0]


def frame_table():
    out, a = [], FRAME_TABLE
    while word(a):
        out.append(word(a))
        a += 4
    return out


def first_fde_pc(blk):
    """pc_begin of the first FDE of the frame info at blk (CIE first)."""
    cie_len = word(blk)
    fde = blk + 4 + cie_len
    return word(fde + 8)


def file_of_function():
    """function address -> source file (FUNC markers)."""
    out = {}
    for rel, ms in funcmap.markers().items():
        for _, a, _ in ms:
            out.setdefault(a, rel)
    return out


def obj_path(rel):
    return os.path.join(ROOT, "build", "score", "obj", rel.replace("/", "__") + ".o")


def frame_offset(rel):
    """Offset of egcs's __FRAME_BEGIN__ in the object's .data (the file's own
    initialised variables come first), or None."""
    import hybrid
    if hybrid.TC_BIN is None:
        hybrid.TC_BIN = paths.ee_bin(None)
    p = obj_path(rel)
    if not os.path.exists(p):
        return None
    for v, t, n in hybrid.nm(p, "--defined-only"):
        if n == "__FRAME_BEGIN__":
            return v
    return None


def build_map():
    # the code's references (score.py's last full run; none in a fresh build)
    p = os.path.join(ROOT, "build", "score", "layout.json")
    lay = json.load(open(p)) if os.path.exists(p) else {}
    fof = file_of_function()
    sections = collections.defaultdict(dict)        # rel -> {sec: [base, size, how]}
    for rel, secs in lay.items():
        for s, (b, sz) in secs.items():
            sections[rel][s] = [b, sz, "refs" if b is not None else None]
    for blk in frame_table():
        rel = fof.get(first_fde_pc(blk))
        if rel is None:
            continue
        off = frame_offset(rel)
        if off is None:
            continue
        base = blk - off
        cur = sections[rel].get(".data")
        if cur is None:
            sections[rel][".data"] = [base, None, "frame"]
        else:
            if cur[0] is not None and cur[0] != base:
                cur[2] = "CONFLICT refs 0x%08X frame 0x%08X" % (cur[0], base)
            else:
                cur[2] = "frame"
            cur[0] = base
        # the frame info's pointer to the object's .gcc_except_table
        et = except_table_base(rel, base)
        if et is not None:
            sz = (sections[rel].get(".gcc_except_table") or [None, None])[1]
            sections[rel][".gcc_except_table"] = [et, sz, "frame"]
    return sections


def except_table_base(rel, data_base):
    """Retail base of the object's .gcc_except_table: the first .data word
    relocated against it (R_MIPS_32) holds its retail address + addend."""
    import elfobj
    p = obj_path(rel)
    if not os.path.exists(p):
        return None
    obj = elfobj.ElfObject(p)
    data = obj.section(".data")
    if data is None:
        return None
    for r in sorted(obj.relocs.get(data.index, []), key=lambda r: r.offset):
        if r.type == 2 and r.sym is not None and r.sym.type == 3 and \
                obj.sections[r.sym.shndx].name == ".gcc_except_table":
            addend = struct.unpack_from("<I", data.data, r.offset)[0]
            return (word(data_base + r.offset) - addend) & 0xFFFFFFFF
    return None


REF = __import__("re").compile(r"%(?:hi|lo|gp_rel)\(([A-Za-z_.$][\w.$]*)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\)")
LINE = __import__("re").compile(r"^\s*/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/")
DATA_START = __import__("re").compile(r"^\s*//\s*DATA_START\s+0x([0-9A-Fa-f]{8})\b")


def tu_refs():
    """{source file: {data address: number of references}}: what the original
    code of each file's functions (FUNC markers) references (the split's
    %hi / %lo / %gp_rel operands, with their addends), cached in
    build/score/file_refs.json."""
    import bisect
    import glob
    cache = os.path.join(ROOT, "build", "score", "file_refs.json")
    srcs = glob.glob(os.path.join(ROOT, "ref", "splat", "asm", "u*.s")) + \
        [os.path.join(ROOT, "ref", "funcs.json")] + _marker_files()
    if os.path.exists(cache) and all(os.path.getmtime(cache) > os.path.getmtime(s) for s in srcs):
        return {t: {int(a): n for a, n in v.items()} for t, v in json.load(open(cache)).items()}
    fof = file_of_function()
    funcs = sorted((int(f["start"], 16), int(f["end"], 16), fof.get(int(f["start"], 16)))
                   for f in json.load(open(os.path.join(ROOT, "ref", "funcs.json"))))
    starts = [f[0] for f in funcs]
    import hybrid
    if hybrid.TC_BIN is None:
        hybrid.TC_BIN = paths.ee_bin(None)
    addr_of = {n: v for v, t, n in hybrid.nm(os.path.join(RELINK_DIR, "core.elf"))
               if v is not None}
    out = collections.defaultdict(collections.Counter)
    for s in srcs:
        for L in open(s, encoding="utf-8", errors="replace"):
            m = LINE.match(L)
            if not m:
                continue
            for r in REF.finditer(L):
                a = int(m.group(1), 16)
                i = bisect.bisect_right(starts, a) - 1
                if i < 0 or a >= funcs[i][1] or funcs[i][2] is None:
                    continue
                v = addr_of.get(r.group(1))
                if v is None or not (DATA_LO <= v < BSS_HI):
                    continue
                t = v + (int(r.group(2), 0) if r.group(2) else 0)
                out[funcs[i][2]][t] += 1
    # anchors: `// DATA_START 0x00308858' in a source file, where the file's
    # data of an area starts that its code does not reference (u001's T004:
    # its .rodata is only type names, so without the anchor T003's range ran
    # over it); taken as one reference of that file
    for p in _marker_files():
        for L in open(p, encoding="utf-8", errors="replace"):
            m = DATA_START.match(L)
            if m:
                out[os.path.relpath(p, ROOT).replace(os.sep, "/")][int(m.group(1), 16)] += 1
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    # written whole and renamed: other agents read it at the same time
    tmp = "%s.%d.tmp" % (cache, os.getpid())
    with open(tmp, "w") as f:
        json.dump({t: dict(v) for t, v in out.items()}, f)
    os.replace(tmp, cache)
    return {t: dict(v) for t, v in out.items()}


# .data runs to 0x00308280: libkernl's .data (u139's kernel patch tables at
# 0x00308128.., u140's topId 0x003081B8 .. _fio_alt_version 0x00308260, u141's
# 0x00308264..0x00308270) comes after the 0x00308100 once taken as the end;
# .rodata opens with T000's "bad_alloc" at 0x00308280 (u140's notes)
AREAS = [(".data", 0x0028F2AC, 0x00308280), (".rodata", 0x00308280, 0x00349A00),
         (".gcc_except_table", 0x00349A00, 0x0034F9EC), (".lit4", 0x0034FA00, 0x00350AB8),
         (".sdata", 0x00350AB8, 0x003531E3), (".bss", 0x00353200, 0x004B2C0C)]


def area_of(a):
    for k, lo, hi in AREAS:
        if lo <= a < hi:
            return k
    return None


def _marker_files():
    out = []
    for top in ("src",):
        for dp, _, fns in os.walk(os.path.join(ROOT, top)):
            out += [os.path.join(dp, f) for f in fns if f.endswith((".c", ".cpp", ".S"))]
    return out


_ANCHORS = None


def anchors():
    """{data address: source file} of the `// DATA_START 0x...' lines (read
    once per run)."""
    global _ANCHORS
    if _ANCHORS is None:
        out = {}
        for p in _marker_files():
            for L in open(p, encoding="utf-8", errors="replace"):
                m = DATA_START.match(L)
                if m:
                    out[int(m.group(1), 16)] = os.path.relpath(p, ROOT).replace(os.sep, "/")
        _ANCHORS = out
    return _ANCHORS


def tu_order():
    """Source files in link order (by their first function).  A data-only
    file (`// DATA_TU frame 0x...', no functions) comes right after the file
    whose frame block is the nearest below its own (u104's T472: its
    `DATA_START' anchor of .rodata then counts, so T471's range stops at
    T472's curve)."""
    first = {}
    for a, rel in file_of_function().items():
        if rel not in first or a < first[rel]:
            first[rel] = a
    order = sorted(first, key=first.get)
    import fulllink
    data_tu = fulllink.data_tu_files()
    if data_tu:
        fof = file_of_function()
        below = sorted((blk, fof.get(first_fde_pc(blk))) for blk, _ in frame_blocks() if blk not in data_tu)
        for blk, rel in sorted(data_tu.items()):
            prev = [r for b, r in below if b < blk and r in order]
            if rel not in order:
                order.insert(order.index(prev[-1]) + 1 if prev else 0, rel)
    # `// LINK_AFTER <file>': an object whose code is in function sections
    # (at the end of .text: u142's libgraph) was linked after that file;
    # its data lies there (between u116's and u118's GS drivers).  Also a
    # data-only C file (no functions, no frame info, so in no order yet):
    # u139's infconst.c (newlib's __infinity) after callocr.o
    for rel, after in link_after().items():
        if after in order and rel != after:
            if rel in order:
                order.remove(rel)
            order.insert(order.index(after) + 1, rel)
    return order


LINK_AFTER = __import__("re").compile(r"^\s*//\s*LINK_AFTER\s+(\S+)")


def link_after():
    """{source file: the file it was linked after} of the `// LINK_AFTER
    <file>' lines."""
    out = {}
    for p in _marker_files():
        for L in open(p, encoding="utf-8", errors="replace"):
            m = LINK_AFTER.match(L)
            if m:
                out[os.path.relpath(p, ROOT).replace(os.sep, "/")] = m.group(1)
                break
    return out


def owners():
    """{data address: TU}: each address the original code references, given
    to one of the TUs referencing it so that, per area, owners follow the link
    order (the longest such chain; references off it are uses of another
    unit's data)."""
    import bisect
    order = {t: i for i, t in enumerate(tu_order())}
    refs = tu_refs()
    by_addr = collections.defaultdict(set)
    for t, v in refs.items():
        if t in order:
            for a in v:
                by_addr[a].add(t)
    # an anchored address belongs to the anchoring file, also where an
    # earlier file references it (u007's T011: ArcadeRender's vtable, which
    # T010's constructor references, opens T011's .rodata)
    for a, t in anchors().items():
        if t in order and a in by_addr:
            by_addr[a] = {t}
    out = {}
    for k, lo, hi in AREAS:
        addrs = sorted(a for a in by_addr if lo <= a < hi)
        # longest non-decreasing chain of TU indices, one candidate per address
        tails, tail_item, prev = [], [], {}
        for a in addrs:
            for t in sorted(by_addr[a], key=lambda t: -order[t]):
                x = order[t]
                i = bisect.bisect_right(tails, x)
                item = (a, t)
                prev[item] = tail_item[i - 1] if i > 0 else None
                if i == len(tails):
                    tails.append(x)
                    tail_item.append(item)
                else:
                    tails[i] = x
                    tail_item[i] = item
        it = tail_item[-1] if tail_item else None
        while it is not None:
            out[it[0]] = it[1]
            it = prev[it]
    # an anchor the chain dropped still opens its file's data: u138's
    # locale.o .data (0x00307590) lies before mallocr.o's, but our
    # mallocr_part1.c starts with freer.o's _free_r, before locale's
    # code, so the chain kept mallocr's addresses and the 24 bytes counted
    # to u137's stdlib (ref/data/u137/notes.md; it changed no other range)
    for a, t in anchors().items():
        if t in order and area_of(a) is not None:
            out[a] = t
    return out


def tu_ranges():
    """{area: [(TU, first owned address, the next TU's first owned address)]}
    in address order: where each TU's data of that area lies in the original
    (its own unreferenced data after it, up to the next TU's, included)."""
    own = owners()
    out = {}
    for k, lo, hi in AREAS:
        firsts = {}
        for a, t in own.items():
            if lo <= a < hi and (t not in firsts or a < firsts[t]):
                firsts[t] = a
        seq = sorted((a, t) for t, a in firsts.items())
        out[k] = [(t, a, seq[i + 1][0] if i + 1 < len(seq) else hi) for i, (a, t) in enumerate(seq)]
    return out


def frame_blocks():
    """[(block, length)] of collect2's frame table in order: each C++
    object's frame info (CIE, FDEs, the 0 terminator)."""
    out = []
    for blk in frame_table():
        a = blk
        while True:
            n = word(a)
            a += 4 + n
            if n == 0:
                break
        out.append((blk, a - blk))
    return out


def object_fdes(obj):
    """[(function symbol, pc range)] of an object's frame info (its FDEs in
    .data after __FRAME_BEGIN__: the object's function order), from the u119
    pilot's fdeorder.py."""
    sec = obj.section(".data")
    if sec is None:
        return []
    fb = next((y.value for y in obj.symbols if y.name == "__FRAME_BEGIN__" and y.shndx == sec.index), None)
    if fb is None:
        return []
    rel_at = {r.offset: r for r in obj.relocs.get(sec.index, [])}
    text = {}
    for y in obj.symbols:
        if y.type == 2 and y.name:
            text.setdefault((y.shndx, y.value), y.name)
    out, a = [], fb
    while a + 4 <= len(sec.data):
        n = struct.unpack_from("<I", sec.data, a)[0]
        if n == 0:
            break
        if struct.unpack_from("<I", sec.data, a + 4)[0] != 0:        # not the CIE
            r = rel_at.get(a + 8)
            v = struct.unpack_from("<I", sec.data, a + 8)[0]
            shndx = r.sym.shndx if r and r.sym else None
            name = r.sym.name if r and r.sym and r.sym.type != 3 else text.get((shndx, v), "?+0x%X" % v)
            out.append((name, struct.unpack_from("<I", sec.data, a + 12)[0]))
        a += 4 + n
    return out


def orig_fdes(blk):
    """[(pc, pc range)] of the original's frame info at blk."""
    out, a = [], blk
    while True:
        n = word(a)
        if n == 0:
            return out
        if word(a + 4) != 0:
            out.append((word(a + 8), word(a + 12)))
        a += 4 + n


def masked_bytes(obj, sec):
    """(bytes, set of offsets a 32-bit relocation rewrites)."""
    data = bytes(sec.data) if sec.type != 8 else bytes(sec.size)
    mask = set()
    for r in obj.relocs.get(sec.index, []):
        if r.type in (2, 12):                           # R_MIPS_32, R_MIPS_GPREL32
            mask.update(range(r.offset, r.offset + 4))
    return data, mask


def match_count(obj, sec, base, data_mask=None):
    """Bytes of the section (relocated words left out) equal to the original's at base."""
    data, mask = data_mask or masked_bytes(obj, sec)
    off = base - VRAM + ROM
    img = image()
    if off < 0 or off + len(data) > len(img):
        return 0
    ret = img[off:off + len(data)]
    return sum(1 for i in range(len(data)) if i not in mask and data[i] == ret[i])


def best_base(obj, sec, lo, hi, slack=0x100):
    """(base, matched bytes, comparable bytes): the base near [lo, hi) (the
    file's original range of that area) where most of the section's bytes
    are the original's; None when nothing is comparable."""
    dm = masked_bytes(obj, sec)
    n = len(dm[0]) - len(dm[1])
    if n <= 0:
        return None
    if not any(dm[0][i] for i in range(len(dm[0])) if i not in dm[1]):
        return None         # only zeros: they match anywhere (T435's guard, T529's table, u099's notes)
    al = max(sec.align, 1)
    start = (lo - slack) - ((lo - slack) % al)
    best = None
    b = start
    while b <= min(hi, lo + slack) and b >= 0:
        c = match_count(obj, sec, b, dm)
        if best is None or c > best[1] or (c == best[1] and abs(b - lo) < abs(best[0] - lo)):
            best = (b, c)
        b += al
    return (best[0], best[1], n) if best else None


AREA_OF_SEC = {".data": ".data", ".rodata": ".rodata", ".lit4": ".lit4", ".lit8": ".lit4",
               ".sdata": ".sdata", ".gcc_except_table": ".gcc_except_table",
               ".sbss": ".bss", ".bss": ".bss"}


def file_ranges():
    """{rel: {area: (lo, hi)}}: tu_ranges, and for a C++ file's .data the
    frame table's word (.frame: its frame info; .data up to its end).

    .data (the u119 pilot's rules): a C++ object's .data lies between the
    previous frame block's end and its own frame block; a file referencing
    an earlier object's variable does not start below that end (T526 and
    T525's g_002FE280); another file's range between the previous block and
    this one (a C object without frame info) ends the gap before this file
    starts (T528's range ran across u113..u118's C data); and every other
    file's range that runs into a frame block ends there (u118's over T528 /
    T527)."""
    import bisect
    out = collections.defaultdict(dict)
    for area, lst in tu_ranges().items():
        for rel, lo, hi in lst:
            out[rel][area] = (lo, hi)
    fof = file_of_function()
    import fulllink
    data_tu = fulllink.data_tu_files()              # data-only files: `// DATA_TU frame 0x...'
    blocks = sorted(frame_blocks())
    starts = [b for b, _ in blocks]
    owner = {blk: data_tu.get(blk) or fof.get(first_fde_pc(blk)) for blk, _ in blocks}
    for rel, rr in out.items():
        if ".data" in rr:
            lo, hi = rr[".data"]
            i = bisect.bisect_right(starts, lo)
            if i < len(starts) and starts[i] < hi and owner.get(starts[i]) != rel:
                rr[".data"] = (lo, starts[i])
    prev_end = AREAS[0][1]
    # in address order: collect2's table is not (it has u001's T009 after
    # T008, the link has it before T007), and the previous block's end must
    # be the one just below; a .data that is only frame info is 4-aligned
    # (T003's at 0x0028F7C4), so the 8-rounded end is capped at blk
    for blk, n in blocks:
        rel = owner[blk]
        if rel is not None:
            start = min(prev_end, blk)
            others = [rr[".data"][1] for r, rr in out.items()
                      if r != rel and ".data" in rr and prev_end <= rr[".data"][0] < blk]
            # a data-only object (DATA_TU) owns the whole gap before its frame
            # block: other files' references into it are uses of its data
            # (u079's T339: T342 reads g_002D4628)
            if others and blk not in data_tu:
                start = min(max(others + [start]), blk)
            lo = out[rel].get(".data", (start, blk))[0]
            if lo < blk:
                lo = max(lo, prev_end)
            out[rel][".data"] = (min(lo, start), blk + n)
            out[rel][".frame"] = (blk, blk + n)
        prev_end = (blk + n + 3) & ~3
    return out


def choose_base(obj, sec, rng, decl, refs_base=None, frame=None):
    """(base, how) of an object's data section in the original: the declared
    addresses of its own symbols; for a .data with frame info, the frame
    table's block (frame: its (lo, hi)) less __FRAME_BEGIN__'s offset, when
    the variables before it are at least half the original's there (else
    they are not the original's variables: T503); else where its bytes best
    match near the file's range rng (at least half of them); else the code's
    references."""
    syms = [y for y in obj.symbols if y.shndx == sec.index and y.name and y.type in (0, 1)]
    implied = collections.Counter(decl[y.name] - y.value for y in syms if y.name in decl)
    if implied:
        b, n = implied.most_common(1)[0]
        if b % max(sec.align, 1) == 0:
            return b, "names(%d)" % n
    fb = next((y.value for y in syms if y.name == "__FRAME_BEGIN__"), None)
    if frame and fb is not None and (frame[0] - fb) % max(sec.align, 1) == 0:
        b = frame[0] - fb
        data, mask = masked_bytes(obj, sec)
        idx = [i for i in range(fb) if i not in mask]
        if not idx:
            return b, "frame"
        off = b - VRAM + ROM
        ret = image()[off:off + fb]
        if len(ret) == fb and 2 * sum(1 for i in idx if data[i] == ret[i]) >= len(idx):
            return b, "frame"
    if rng and sec.type != 8:
        bb = best_base(obj, sec, rng[0], rng[1])
        if bb and bb[1] * 2 >= bb[2]:
            return bb[0], "bytes"
        # zeros only (no bytes base): the file's own `DATA_START' at its
        # range's start says where (u057's T168 "")
        if bb is None and refs_base is None and rng[0] in anchors() and rng[0] % max(sec.align, 1) == 0 \
                and rng[0] + sec.size <= rng[1]:
            return rng[0], "anchor"
    if refs_base is not None:
        return refs_base, "refs"
    return None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gaps", action="store_true")
    ap.add_argument("--json")
    a = ap.parse_args()
    m = build_map()
    by_kind = collections.defaultdict(list)
    unknown = collections.Counter()
    conflicts = []
    for rel, secs in m.items():
        for s, (b, sz, how) in secs.items():
            if how and how.startswith("CONFLICT"):
                conflicts.append((rel, s, how))
            if b is None:
                unknown[s] += 1
                continue
            by_kind[s].append((b, sz or 0, rel))
    for s in sorted(by_kind):
        v = sorted(by_kind[s])
        lo, hi = v[0][0], max(b + sz for b, sz, _ in v)
        cov = sum(sz for _, sz, _ in v)
        print("%-18s %4d objects placed (%3d unplaced)  0x%08X..0x%08X  %d bytes, objects %d bytes"
              % (s, len(v), unknown[s], lo, hi, hi - lo, cov))
    for c in conflicts:
        print("  conflict:", *c)
    # coverage of the initialised data region by placed sections
    iv = sorted((b, b + (sz or 0), rel, s) for s, v in by_kind.items() for b, sz, rel in v if sz)
    covered, gaps, last = 0, [], DATA_LO
    for lo, hi, rel, s in iv:
        if lo >= DATA_HI:
            continue
        if lo > last:
            gaps.append((last, lo))
        covered += max(0, min(hi, DATA_HI) - max(lo, last))
        last = max(last, hi)
    if last < DATA_HI:
        gaps.append((last, DATA_HI))
    print("initialised data 0x%08X..0x%08X: %d bytes, %d covered by placed object sections, %d in %d gaps"
          % (DATA_LO, DATA_HI, DATA_HI - DATA_LO, covered, sum(h - l for l, h in gaps), len(gaps)))
    if a.gaps:
        for lo, hi in gaps:
            print("  gap 0x%08X..0x%08X  %d" % (lo, hi, hi - lo))
    if a.json:
        json.dump({rel: secs for rel, secs in m.items()}, open(a.json, "w"), indent=0, sort_keys=True)


if __name__ == "__main__":
    main()
