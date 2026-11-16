"""The program linked from the C alone (no original code, no second copy of
its data): the size the original had, everything symbolic.

Every source is compiled with the matching flags (tools/mcc.py: the -G of
ref/compile_flags.json, the assembler passes) into <build>/obj/.  The link
then lays the objects out in the original's order:

  .text    crt0, collect2's file, every object in the order of its code
  .data    crt0's, collect2's tables, every object's .data (frame info first)
  .rodata  every object's .rodata, then .gcc_except_table, .lit4/.lit8,
  .sdata   ($gp = the start of the small data + 0x7FF0, as the original's)
  .sbss / .bss

An object's data section whose bytes are the original's at the address its
references imply (tools/datalayout.py) takes that place; the original's data
no object produces yet stays as "leftover" pieces (raw bytes from the relink
of the split, pointers made symbolic), so the program is complete at every
step of moving the rest of the data into C.  Every piece gets a start symbol;
an original address is `piece start + offset' wherever the C or the leftover
data needs one, so nothing depends on fixed addresses.

    python tools/fulllink.py [--build-dir build/full] [--jobs N] [--pack CORE.GT3]
"""
import argparse
import collections
import concurrent.futures as cf
import glob
import json
import os
import re
import struct
import subprocess
import sys

import paths
import mcc
import funcmap
import hybrid
import datalayout
import layout
from elfobj import ElfObject, R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL

ROOT = paths.ROOT
SRC = os.path.join(ROOT, "src")


DATA_TU = re.compile(r"^\s*//\s*DATA_TU\s+frame\s+0x([0-9A-Fa-f]{8})")


def data_tu_files():
    """{frame block: rel} of the data-only C++ files (no functions, so no FDE
    names their frame info): a `// DATA_TU frame 0x002D296C' line."""
    out = {}
    for dp, _, fns in os.walk(os.path.join(ROOT, "src")):
        for fn in fns:
            if fn.endswith((".cpp", ".cc")):
                p = os.path.join(dp, fn)
                for L in open(p, encoding="utf-8", errors="replace"):
                    m = DATA_TU.match(L)
                    if m:
                        out[int(m.group(1), 16)] = os.path.relpath(p, ROOT).replace(os.sep, "/")
                        break
    return out


TOOLCHAIN = None                # --toolchain (default: tools/paths.py)


def tool(name):
    return os.path.join(paths.ee_bin(TOOLCHAIN), paths.EE_PREFIX + name + paths.EXE)


def run(cmd, cwd=ROOT):
    # SCE ee-gcc's driver gets its own environment (paths.egcs_env)
    env = dict(paths.egcs_env(mcc.CC) if cmd[0] == mcc.CC else os.environ)
    env["PATH"] = paths.ee_bin(TOOLCHAIN) + os.pathsep + env.get("PATH", "")    # the toolchain's DLLs
    return subprocess.run(cmd, capture_output=True, text=True, errors="replace", env=env, cwd=cwd)


def sources():
    out = []
    for dp, _, fns in os.walk(SRC):
        for fn in fns:
            if fn.endswith((".c", ".cpp", ".cc", ".S")):
                out.append(os.path.relpath(os.path.join(dp, fn), ROOT).replace("\\", "/"))
    return sorted(out)


OVERRIDE = {}                   # rel -> object generated at link time in its place


def obj_of(build, rel):
    if rel in OVERRIDE:
        return OVERRIDE[rel]
    return os.path.join(build, "obj", rel.replace("/", "__") + ".o")


COLLECT2 = "src/runtime/startup/collect2_ctors.c"
CTOR_LIST, DTOR_LIST = 0x0028F020, 0x0028F1E0
REG_FRAME, DEREG_FRAME = 0x001000C8, 0x00100108


def compile_one(build, rel):
    """(rel, ok, messages): the object with the matching flags and, for C/C++,
    the stab function list (<obj>.funcs.json) from a -g assembly pass."""
    o = obj_of(build, rel)
    src = os.path.join(ROOT, rel)
    fj = o[:-2] + ".funcs.json"
    deps = [src, os.path.join(paths.REF, "compile_flags.json")]
    if os.path.exists(o) and (rel.endswith(".S") or os.path.exists(fj)) and \
            os.path.getmtime(o) >= max(os.path.getmtime(p) for p in deps) and not _headers_newer(o):
        return rel, True, ""
    os.makedirs(os.path.dirname(o), exist_ok=True)
    if rel.endswith(".S"):
        # egcs's driver and assembler: the same ABI as the compiled objects
        r = run([mcc.CC, "-Iinclude", "-Isrc", "-I" + os.path.dirname(rel), "-c", rel,
                 "-o", os.path.relpath(o, ROOT).replace(os.sep, "/")])
        return rel, r.returncode == 0 and os.path.exists(o), (r.stdout + r.stderr).strip()
    # into a new file first: a failed compile keeps the last good object
    # (--keep-going links it while another agent's edit is half done)
    tmp = o[:-2] + ".new.o"
    ok, msg = mcc.compile(rel, tmp)
    if not ok:
        if os.path.exists(tmp):
            os.remove(tmp)
        return rel, False, msg
    os.replace(tmp, o)
    s = o[:-2] + ".g.s"
    ok2, msg2 = mcc.compile(rel, s, asm=True, debug=True)
    if ok2:
        with open(fj, "w", encoding="utf-8") as f:
            json.dump(funcmap.stab_functions(open(s, encoding="utf-8", errors="replace").read()), f)
        os.remove(s)
    return rel, ok2, (msg + msg2).strip()


_HDR_TIME = None


def _headers_newer(o):
    """Any header under include/ or a unit folder newer than the object (a
    coarse but safe rebuild rule)."""
    global _HDR_TIME
    if _HDR_TIME is None:
        t = 0
        for d in (os.path.join(ROOT, "include"), SRC):
            for dp, _, fns in os.walk(d):
                for fn in fns:
                    if fn.endswith((".h", ".inc")):
                        t = max(t, os.path.getmtime(os.path.join(dp, fn)))
        _HDR_TIME = t
    return _HDR_TIME > os.path.getmtime(o)


def compile_all(build, jobs, progress=False, keep_going=False):
    rels = sources()
    # objects of sources that are gone (merged files) must not be linked
    keep = {os.path.normcase(os.path.abspath(obj_of(build, r))) for r in rels}
    for p in glob.glob(os.path.join(build, "obj", "*.o")):
        if os.path.normcase(os.path.abspath(p)) not in keep:
            os.remove(p)
            for x in (p[:-2] + ".funcs.json",):
                if os.path.exists(x):
                    os.remove(x)
    res = []
    with cf.ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = [ex.submit(compile_one, build, r) for r in rels]
        for i, fut in enumerate(cf.as_completed(futs)):
            res.append(fut.result())
            if progress:
                print("[%d/%d] %s" % (i + 1, len(rels), res[-1][0]), flush=True)
    bad = [(r, m) for r, ok, m in res if not ok]
    for r, m in bad[:20]:
        print("FAIL %s\n%s" % (r, m[-1500:]))
    if bad and keep_going:
        stale = [r for r, _ in bad if os.path.exists(obj_of(build, r))]
        print("   --keep-going: %d failed, %d of them linked from their last good object"
              % (len(bad), len(stale)))
        return rels, len(stale) == len(bad)
    return rels, not bad


# ------------------------------------------------------------------ layout

VRAM, ROM = 0x00100000, 0x1000
# the original's data, three runs: initialised (.data, .rodata,
# .gcc_except_table), small ($gp: .lit4 / .lit8 / .sdata), zero (.sbss / .bss)
# (the original's .data starts at 0x0028E780: the link script's ALIGN(128)
# after .text, the 0x20 bytes before it are that padding, as in Sony's
# app.cmd; collect2's frame table is the first data)
REGIONS = [("init", 0x0028E780, 0x0034FA00, (".data", ".rodata", ".gcc_except_table")),
           ("small", 0x0034FA00, 0x003531E3, (".lit4", ".lit8", ".sdata")),
           ("zero", 0x00353200, 0x004B2C0C, (".sbss", ".bss"))]


def retail():
    return datalayout.image()


def rbytes(lo, hi):
    return retail()[lo - VRAM + ROM:hi - VRAM + ROM]


def rword(a):
    return struct.unpack_from("<I", retail(), a - VRAM + ROM)[0]


def region_of(kind_sec):
    for name, lo, hi, kinds in REGIONS:
        if kind_sec in kinds:
            return name, lo, hi
    return None


def masked(obj, sec):
    """The section's bytes, and the set of byte offsets a relocation rewrites."""
    data = bytes(sec.data)
    mask = set()
    for r in obj.relocs.get(sec.index, []):
        n = 4 if r.type in (R_MIPS_32, 12) else 0
        mask.update(range(r.offset, r.offset + n))
    return data, mask


def same_bytes(obj, sec, base):
    return match_ratio(obj, sec, base) == 1.0


def match_ratio(obj, sec, base):
    """Share of the section's unrelocated bytes equal to retail's at base."""
    if sec.type == 8:                                   # NOBITS
        return 1.0
    data, mask = masked(obj, sec)
    ret = rbytes(base, base + sec.size)
    if len(ret) != len(data):
        return 0.0
    n = [i for i in range(len(data)) if i not in mask]
    if not n:
        return 1.0
    return sum(1 for i in n if data[i] == ret[i]) / len(n)


def search(obj, sec, lo, hi):
    """Retail bases in [lo, hi) where the section's bytes (relocated words
    masked) are found, for a section never referenced from code."""
    data, mask = masked(obj, sec)
    if len(data) < 16 or sum(1 for b in data if b) < 8:
        return []
    pat = b"".join(b"(?s:.)" if i in mask else re.escape(data[i:i + 1]) for i in range(len(data)))
    hay = rbytes(lo, hi)
    al = max(sec.align, 1)
    return [lo + m.start() for m in re.finditer(pat, hay) if (lo + m.start()) % al == 0]


COMMON = "<common>"                     # Layout.placed's rel of a common symbol
SHN_COMMON, SHN_MIPS_SCOMMON = 0xFFF2, 0xFF03


class Layout:
    """Every object's data sections at their retail bases where verified, and
    the leftover stretches of each region."""

    def __init__(self, build, rels):
        self.build = build
        self.rels = rels
        self.objs = {r: ElfObject(obj_of(build, r)) for r in rels}
        dm = datalayout.build_map()                     # rel -> {sec: [base, size, how]}
        fr = datalayout.file_ranges()                   # rel -> {area: (lo, hi)}
        decl = address_names()[0]
        self.placed = {name: [] for name, _, _, _ in REGIONS}    # region -> [(base, size, rel, secname)]
        self.unplaced = {name: [] for name, _, _, _ in REGIONS}  # region -> [(rel, secname)]
        self.notes = collections.Counter()
        self.reloc_hints = set()                        # retail addresses of words some C object relocates
        pending = []
        for rel in rels:
            obj = self.objs[rel]
            for sec in obj.sections:
                reg = region_of(sec.name)
                if reg is None or sec.size == 0:
                    continue
                name, lo, hi = reg
                ent = dm.get(rel, {}).get(sec.name) or [None, None, None]
                # the code's references (where they disagree with the frame
                # table, the references: CONFLICT refs <a> frame <b>)
                refs_b = ent[0] if ent[2] == "refs" else \
                    int(ent[2].split()[2], 16) if ent[2] and ent[2].startswith("CONFLICT") else None
                if sec.name == ".gcc_except_table":
                    b, how = ent[0], ent[2]                 # the frame info's pointer to it
                else:
                    # the section's own names' declared addresses, else its
                    # bytes near the file's original range, else references
                    rng = fr.get(rel, {}).get(datalayout.AREA_OF_SEC[sec.name])
                    b, how = datalayout.choose_base(obj, sec, rng, decl, refs_b,
                                                    fr.get(rel, {}).get(".frame") if sec.name == ".data" else None)
                if b is not None:
                    self.reloc_hints.update(b + r.offset for r in obj.relocs.get(sec.index, [])
                                            if r.type == R_MIPS_32)
                if b is not None and b % max(sec.align, 1):
                    self.notes["base not aligned for the section"] += 1
                    b = None
                # placed only where the original's bytes are the section's (a
                # section that is longer swallows what follows it: T004's
                # .sdata took the ";1" of the IOP module paths); a .data
                # whose own variables are exact may differ in its frame info
                # (it describes code that is not the original's yet; the
                # frame table points at each object's own)
                ok = b is not None and lo <= b and b + sec.size <= hi + 0x20
                if ok and sec.type == 8:
                    # no bytes to compare: every declared name in it must agree
                    ys = [y for y in obj.symbols if y.shndx == sec.index and y.name in decl]
                    ok = all(decl[y.name] == b + y.value for y in ys)
                    if not ok:
                        self.notes["base rejected (.bss names disagree)"] += 1
                elif ok and not same_bytes(obj, sec, b):
                    ok = sec.name == ".data" and self._frame_only(obj, sec, b, fr.get(rel, {}))
                    self.notes["placed, frame info differs" if ok else "base rejected (bytes)"] += 1
                if ok:
                    self.placed[name].append((b, sec.size, rel, sec.name))
                    self.notes["placed (%s)" % (how or "?").split("(")[0]] += 1
                else:
                    pending.append((rel, sec, reg, b, fr.get(rel, {}).get(datalayout.AREA_OF_SEC.get(sec.name))))
        for name in self.placed:
            self.placed[name] = self._no_overlap(name, self.placed[name])
        # sections no reference places: by their bytes, where nothing else is
        # (inside the file's own range when it has one: anywhere else the
        # bytes are another file's, T518's .rodata on T459's g_003413C0)
        for rel, sec, (name, lo, hi), b, rng in pending:
            if sec.type != 8:
                slo, shi = (max(lo, rng[0] - 0x100), min(hi, rng[1] + 0x100)) if rng else (lo, hi)
                hits = [h for h in search(self.objs[rel], sec, slo, shi) if self._free(name, h, sec.size)]
                if len(hits) == 1:
                    self.placed[name].append((hits[0], sec.size, rel, sec.name))
                    self.placed[name] = self._no_overlap(name, self.placed[name])
                    self.notes["placed (bytes)"] += 1
                    continue
            self.unplaced[name].append((rel, sec.name))
            self.notes["unplaced"] += 1
        self._neighbours()
        self._commons(rels)

    def _neighbours(self):
        """Sections nothing places (no reference, bytes found many times: T115's
        .rodata, 111 places): right after the previous file's section of the
        same name in link order, or right before the next one's, where the
        bytes are the section's (the u040 pilot)."""
        order = {r: i for i, r in enumerate(datalayout.tu_order())}
        changed = True
        while changed:
            changed = False
            at = {}                                     # (rel, secname) -> base
            for name in self.placed:
                for b, s, rel, sn in self.placed[name]:
                    if rel != COMMON:
                        at[(rel, sn)] = (b, s)
            for name in self.unplaced:
                for rel, sn in list(self.unplaced[name]):
                    if rel not in order:
                        continue
                    obj = self.objs[rel]
                    sec = obj.section(sn)
                    if sec is None or sec.type == 8:
                        continue
                    al = max(sec.align, 1)
                    cands = []
                    same = sorted((order[r], b, s) for (r, n), (b, s) in at.items() if n == sn and r in order)
                    prev = [x for x in same if x[0] < order[rel]]
                    nxt = [x for x in same if x[0] > order[rel]]
                    if prev:
                        e = prev[-1][1] + prev[-1][2]
                        cands.append(e + (-e) % al)
                    if nxt:
                        b = nxt[0][1] - sec.size
                        cands.append(b - b % al)
                    for b in cands:
                        if self._free(name, b, sec.size) and same_bytes(obj, sec, b):
                            self.placed[name].append((b, sec.size, rel, sn))
                            self.placed[name] = self._no_overlap(name, self.placed[name])
                            self.unplaced[name].remove((rel, sn))
                            self.notes["placed (link-order neighbour)"] += 1
                            changed = True
                            break

    def _commons(self, rels):
        """Common symbols (egcs's typeinfo nodes, uninitialised globals): the
        original linker allocated them; here each goes to its declared
        address (leftover_asm defines it there), the rest to *(COMMON)."""
        self.commons = {}                               # name -> (size, align)
        defined = set()
        for rel in rels:
            for y in self.objs[rel].symbols:
                if not y.name:
                    continue
                if y.shndx in (SHN_COMMON, SHN_MIPS_SCOMMON):
                    sz, al = self.commons.get(y.name, (0, 1))
                    self.commons[y.name] = (max(sz, y.size), max(al, y.value or 1))
                elif 0 < y.shndx < 0xFF00 and y.bind in (1, 2):
                    defined.add(y.name)
        _, lo, hi, _ = next(r for r in REGIONS if r[0] == "zero")
        for n, (sz, al) in sorted(self.commons.items()):
            a = address_of(n)
            if n in defined or a is None or not (lo <= a and a + sz <= hi) or a % al:
                self.notes["common left to the linker"] += 1
                continue
            if not self._free("zero", a, sz):
                self.notes["common over another section"] += 1
                continue
            self.placed["zero"].append((a, sz, COMMON, n))
            self.notes["common at its address"] += 1
        self.placed["zero"] = self._no_overlap("zero", self.placed["zero"])

    @staticmethod
    def _frame_only(obj, sec, b, rr):
        """A .data at b whose bytes differ only in its frame info, which sits
        where the original's does and ends no later (the next object's .data
        follows it)."""
        fb = next((y.value for y in obj.symbols if y.name == "__FRAME_BEGIN__" and y.shndx == sec.index), None)
        if fb is None or ".frame" not in rr or b + fb != rr[".frame"][0] or b + sec.size > rr[".frame"][1]:
            return False
        data, mask = masked(obj, sec)
        ret = rbytes(b, b + fb)
        return all(data[i] == ret[i] for i in range(fb) if i not in mask)

    def _free(self, name, base, size):
        return all(base + size <= b or b + s <= base for b, s, _, _ in self.placed[name])

    def _no_overlap(self, name, lst):
        out = []
        for b, s, rel, sn in sorted(lst):
            if out and b < out[-1][0] + out[-1][1]:
                self.notes["dropped (overlap)"] += 1
                if rel != COMMON:
                    self.unplaced[name].append((rel, sn))
                continue
            out.append((b, s, rel, sn))
        return out

    def pieces(self, name):
        """[(lo, hi, ('c', rel, sec) | ('common', name, size) | ('left',))]
        covering the region."""
        _, lo, hi, _ = next(r for r in REGIONS if r[0] == name)
        out, cur = [], lo
        for b, s, rel, sn in self.placed[name]:
            if b < cur or b >= hi:
                continue
            if b > cur:
                out.append((cur, b, ("left",)))
            out.append((b, b + s, ("common", sn, s) if rel == COMMON else ("c", rel, sn)))
            cur = b + s
        if cur < hi:
            out.append((cur, hi, ("left",)))
        return out


# ------------------------------------------------------------------ the link

CODE_LO, CODE_HI = 0x00100000, 0x0028E760


BLOB_HI = 0x0030729C            # the split had 0x0028E760..here as a binary (eh_tables.bin)


def pointer_words(lo, hi, func_starts, labels, hints):
    """{address: target} of the original's pointer words in [lo, hi): the
    relink's own relocations where the split's data has them (BLOB_HI..);
    words equal to a function start or to a label of the relink (spimdisasm
    left the load script's pointers to the ";1" suffix 0x00350B40 raw); the
    words the C objects relocate, at their retail bases (hints); and below
    BLOB_HI, where nothing else tells, an aligned value inside the data that
    is not two small halfwords (0x004A004C).  Not every in-range value: the
    instruction words of libkernl's alarm patch (0x00307A18, copied into the
    kernel) are, and rewriting them crashed the kernel."""
    global _RELINK_PTRS
    if _RELINK_PTRS is None:
        _RELINK_PTRS = {}
        p = os.path.join(datalayout.RELINK_DIR, "obj", "asm", "data", "data.rodata.s.o")
        o = ElfObject(p)
        sec = o.section(".rodata")
        for r in o.relocs.get(sec.index, []):
            if r.type == R_MIPS_32:
                a = BLOB_HI + r.offset
                _RELINK_PTRS[a] = rword(a)
    out = {}
    for a in range((lo + 3) & ~3, hi - 3, 4):
        v = rword(a)
        in_data = 0x0028E760 <= v < 0x004B2C0C
        if a in hints and (in_data or CODE_LO <= v < CODE_HI):
            out[a] = v                                  # the C's own relocation
            continue
        if any(l <= a < h for l, h, _ in NOT_POINTERS) or not_a_pointer(a, v):
            continue
        if a in _RELINK_PTRS or v in func_starts or (in_data and v in labels) or \
                (a < BLOB_HI and in_data and v % 4 == 0 and (v & 0xFFFF) >= 0x100):
            out[a] = v
    return out


# stretches of the original's data that hold no pointers whatever their words
# look like (spimdisasm symbolized instruction words of these as `.word D_...')
NOT_POINTERS = [(0x00307A18, 0x00308178, "libkernl's alarm patch code and callback stub (copied into the kernel)")]


def not_a_pointer(a, v):
    """A word that only looks like a pointer: the tail of a string ("REE\\0"
    after "UNIT_DEG": the byte before it is text), or an odd address in the
    .bss (no table points into the middle of a buffer)."""
    b = rbytes(a - 1, a + 4)
    if 0x20 <= b[0] < 0x7F and all(c == 0 or 0x20 <= c < 0x7F for c in b[1:4]):
        return True
    return v >= 0x00353200 and v % 4 != 0


def relink_labels():
    """Addresses the relink (spimdisasm's split) has a symbol for."""
    return {v for v, t, n in hybrid.nm(os.path.join(datalayout.RELINK_DIR, "core.elf")) if v is not None}


_RELINK_PTRS = None

# egcs names an anonymous namespace after the preprocessed file, with a random
# suffix: retail's "34_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7", ours another
ANON = re.compile(r"\d+_GLOBAL_\$N\$\S*?\.(?:ii|cxx)[A-Za-z0-9]{6}")


class Linker:
    def __init__(self, build, rels, lay):
        self.build, self.rels, self.lay = build, rels, lay
        self.dir = os.path.join(build, "link")
        os.makedirs(self.dir, exist_ok=True)
        marks = funcmap.markers()
        self.order = layout.link_order(rels)
        self.index = {r: i for i, r in enumerate(self.order)}
        # code: function address -> (object, symbol value)
        entries, defined = [], set()
        objs = [obj_of(build, r) for r in self.order]
        hybrid.nm_all(objs, "--defined-only")
        self.syms = {}                                  # rel -> {name: (value, type)}
        anon = ANON
        for r, o in zip(self.order, objs):
            self.syms[r] = {n: (v, t) for v, t, n in hybrid.nm(o, "--defined-only") if v is not None}
            defined |= {n for n, (v, t) in self.syms[r].items() if t in "TtWw"}
            fj = o[:-2] + ".funcs.json"
            if os.path.exists(fj):
                # the -g pass's anonymous namespace has another name (egcs names
                # it after the preprocessed file, with a random suffix): the
                # object's own symbol with the namespace left out
                own = {anon.sub("<anon>", n): n for n in self.syms[r]}
                for e in json.load(open(fj)):
                    s = e[0]
                    if "_GLOBAL_$N$" in s:
                        s = own.get(anon.sub("<anon>", s), s)
                    entries.append((s, e[1], e[2]))
        fmap, _ = funcmap.build(entries, marks, defined)
        file_of = {a: rel for rel, ms in marks.items() for _, a, _ in ms}
        self.code = {}                                  # address -> (rel, value, name, in .text)
        for sym, a in fmap.items():
            rel = file_of.get(a)
            cands = [rel] if rel in self.syms and sym in self.syms[rel] else \
                [r for r in self.order if sym in self.syms[r]]
            if cands:
                r = cands[0]
                o = lay.objs[r]
                y = next((y for y in o.symbols if y.name == sym), None)
                in_text = y is not None and 0 < y.shndx < len(o.sections) and o.sections[y.shndx].name == ".text"
                self.code[a] = (r, self.syms[r][sym][0], sym, in_text)
        self.aliases, self.no_code = self._weak_copies(marks)
        self.func_starts = sorted(self.code)
        # collect2's frame table: each entry the object's own frame info (its
        # global _GLOBAL_$F$ symbol), wherever the object's .data is
        self.frame_sym = {}
        fof = datalayout.file_of_function()
        data_tu = data_tu_files()                       # frame block -> a data-only file
        for blk in datalayout.frame_table():
            r = data_tu.get(blk) or fof.get(datalayout.first_fde_pc(blk))
            f = [n for n in self.syms.get(r, {}) if n.startswith("_GLOBAL_$F$")]
            if len(f) == 1:
                self.frame_sym[blk] = f[0]
        self.pieces = {name: lay.pieces(name) for name, _, _, _ in REGIONS}
        self.piece_sym = {}                             # (region, index) -> symbol
        self.data_starts = set()
        for name, ps in self.pieces.items():
            for i, (lo, hi, k) in enumerate(ps):
                self.piece_sym[(name, i)] = "__p_%s_%d" % (name, i)
                self.data_starts.add(lo)
        self.redirect = self._redirects(address_names()[0])
        self.redirect_lo = [r[0] for r in self.redirect]

    def _weak_copies(self, marks):
        """Retail functions no symbol maps to: egcs's weak copies (an inline
        function, a typeinfo function emitted in every object that needs it;
        the linker keeps one).  The original's data that points at a copy (a
        vtable, a table) gets the kept one: the mapped address whose marker
        has the same name (and parameter list, for overloads).  Without this
        such a pointer went into the end of the function before the copy.
        First the copy in the object of the copy's own file (as retail: each
        object's vtable points at its own copy)."""
        name_at = {a: n for ms in marks.values() for _, a, n in ms}
        file_at = {a: r for r, ms in marks.items() for _, a, _ in ms}
        by_key = collections.defaultdict(list)
        for a in self.code:
            by_key[funcmap._marker_key(name_at.get(a, ""))].append(a)
        params = lambda a: (lambda m: funcmap.encode_params(m.group(1)) if m else None)(
            re.search(r"\(([^)]*)\)", name_at.get(a, "")))
        aliases, none = {}, []
        for f in json.load(open(os.path.join(paths.REF, "funcs.json"))):
            a = int(f["start"], 16)
            if a in self.code:
                continue
            key = funcmap._marker_key(name_at.get(a, ""))
            r = file_at.get(a)
            if key and r in self.syms:
                o = self.lay.objs[r]
                akey = ANON.sub("<anon>", key)
                own = [y for y in o.symbols if y.type == 2 and 0 < y.shndx < len(o.sections)
                       and ((funcmap.demangle(y.name) or y.name).replace(" ", "") == key
                            or ANON.sub("<anon>", y.name) == akey)]
                if len(own) > 1:
                    pc = params(a)
                    own = [y for y in own if funcmap.param_code(y.name) == pc]
                if len(own) == 1:
                    y = own[0]
                    self.code[a] = (r, y.value, y.name, o.sections[y.shndx].name == ".text")
                    aliases[a] = a
                    continue
            cands = by_key.get(key, []) if key else []
            if len(cands) > 1:
                same = [c for c in cands if params(c) == params(a)]
                cands = same if same else cands
            # copies of one inline function: any is the function
            if cands and len({self.code[c][2] for c in cands}) == 1:
                aliases[a] = cands[0]
            else:
                none.append(a)
        for a, c in aliases.items():
            if c != a:
                self.code[a] = self.code[c]
        return aliases, none

    def align(self, rel, secname):
        sec = self.lay.objs[rel].section(secname)
        return max(sec.align, 1) if sec is not None else 1

    def code_expr(self, a):
        import bisect
        i = bisect.bisect_right(self.func_starts, a) - 1
        if i < 0:
            return None
        f = self.func_starts[i]
        rel, v, sym, in_text = self.code[f]
        if not in_text:                                 # its own section (.text.<name>): by its name
            return sym if a == f else "%s + 0x%X" % (sym, a - f)
        return "__t_%d + 0x%X" % (self.index[rel], v + (a - f))

    def _redirects(self, decl):
        """[(lo, hi, symbol)]: the C's variables whose section is not at its
        original place (so not where their declared address is): the
        original's data pointing there gets the C's copy (else two copies, as
        the hybrid's u119 RPC objects once had).  Global ones: a local one
        cannot be named from the leftover."""
        placed = {(rel, sn) for ps in self.lay.placed.values() for _, _, rel, sn in ps}
        out = []
        for rel in self.rels:
            o = self.lay.objs[rel]
            for y in o.symbols:
                if y.bind not in (1, 2) or y.type != 1 or not (0 < y.shndx < len(o.sections)):
                    continue
                sec = o.sections[y.shndx]
                if region_of(sec.name) is None or (rel, sec.name) in placed:
                    continue
                a = decl.get(y.name)
                if a is not None and 0x0028E760 <= a < 0x004B2C0C:
                    out.append((a, a + max(y.size, 1), y.name))
        out.sort()
        return out

    def data_expr(self, a):
        import bisect
        i = bisect.bisect_right(self.redirect_lo, a) - 1
        if i >= 0 and a < self.redirect[i][1]:
            lo, hi, n = self.redirect[i]
            return n if a == lo else "%s + 0x%X" % (n, a - lo)
        for name, lo, hi, _ in REGIONS:
            if lo <= a < hi or (a == hi and name == "zero"):
                ps = self.pieces[name]
                for i, (plo, phi, k) in enumerate(ps):
                    if plo <= a < phi or (a == phi and i == len(ps) - 1):
                        return "%s + 0x%X" % (self.piece_sym[(name, i)], a - plo)
        return None

    def piece_of(self, a):
        """The piece holding data address a: ('c', rel, sec) / ('common', ...) /
        ('left',), None outside the regions."""
        for name, lo, hi, _ in REGIONS:
            if lo <= a < hi:
                for plo, phi, k in self.pieces[name]:
                    if plo <= a < phi:
                        return k
        return None

    def expr(self, a):
        if CODE_LO <= a < CODE_HI:
            return self.code_expr(a)
        return self.data_expr(a)

    def collect2_source(self):
        """build/<x>/link/collect2_tables.c: the C file collect2 wrote at link
        time (gcc 2.9x write_c_file_glob), from the original's lists: the
        frame table (each object's own frame info), reg_frame / dereg_frame
        (src/runtime/startup/collect2_ctors.c, included where collect2 wrote them),
        __CTOR_LIST__ / __DTOR_LIST__ (count, entries, reg_frame /
        dereg_frame, 0) and __main_reference; reg_frame's `ob' is
        reg_frame_ob.  Names that are no C identifiers go through asm labels
        (as collect2's write_list_with_asm).  An entry with no symbol here
        (a frame table entry of a data-only object: it has no source yet)
        keeps the original's bytes through a linker-script name."""
        def words(a):
            out = []
            while True:
                w = rword(a)
                out.append(w)
                if w == 0:
                    return out
                a += 4
        self.c2_names = {}                              # script names of entries without a symbol
        ext, frames = [], []
        for i, blk in enumerate(datalayout.frame_table()):
            n = self.frame_sym.get(blk)
            if n is None:
                n = "__c2_frame_%d" % i
                self.c2_names[n] = self.data_expr(blk)
            ext.append('extern char c2_f%d[] __asm__("%s");' % (i, n))
            frames.append("c2_f%d" % i)

        def entries(lst, last_addr, last_name, tag):
            out = []
            for j, a in enumerate(lst[1:-1]):
                if a == last_addr:
                    out.append(last_name)
                    continue
                sym = self.code[a][2] if a in self.code else None
                if sym is None:
                    sym = "__c2_%s_%d" % (tag, j)
                    self.c2_names[sym] = self.code_expr(a)
                ext.append('extern entry_pt c2_%s%d __asm__("%s");' % (tag, j, sym))
                out.append("c2_%s%d" % (tag, j))
            return lst[0], out
        nc, ctors = entries(words(CTOR_LIST), REG_FRAME, "reg_frame", "c")
        nd, dtors = entries(words(DTOR_LIST), DEREG_FRAME, "dereg_frame", "d")
        inc = os.path.relpath(os.path.join(ROOT, COLLECT2), self.dir).replace(os.sep, "/")
        L = ["/* collect2's file (write_c_file_glob), written by tools/fulllink.py at link",
             " * time from the original's lists; compiled as %s. */" % COLLECT2,
             "typedef void entry_pt();", ""] + ext + ["",
             "void *frame_table[] = {"] + ["\t%s," % f for f in frames] + ["\t0", "};", "",
             '#include "%s"' % inc, "",
             "struct object reg_frame_ob;", "",
             "void (*__CTOR_LIST__[])() = {", "\t(void (*)()) %d," % nc] + ["\t%s," % c for c in ctors] + \
            ["\t0", "};", "void (*__DTOR_LIST__[])() = {", "\t(void (*)()) %d," % nd] + \
            ["\t%s," % d for d in dtors] + ["\t0", "};", "",
             "extern entry_pt __main;", "entry_pt *__main_reference = __main;", ""]
        p = os.path.join(self.dir, "collect2_tables.c")
        open(p, "w").write("\n".join(L))
        return p

    def dump_map(self):
        """build/<x>/link/addrmap.json: the retail address of every function
        and piece with the expression that places it (tools/fulladdr.py turns
        them into the new link's addresses)."""
        m = {"code": [[a, self.code_expr(a)] for a in self.func_starts],
             "data": [[lo, hi, self.piece_sym[(name, i)], "%s:%s" % (k[1], k[2]) if k[0] == "c" else
                       "common:%s" % k[1] if k[0] == "common" else "left"]
                      for name, ps in self.pieces.items() for i, (lo, hi, k) in enumerate(ps)],
             "unplaced": [[name, rel, sn, self.lay.objs[rel].section(sn).size]
                          for name, us in self.lay.unplaced.items() for rel, sn in us]}
        with open(os.path.join(self.dir, "addrmap.json"), "w") as f:
            json.dump(m, f)

    def leftover_asm(self):
        """build/<x>/link/leftover.s: every leftover piece in its own section."""
        L = [".set noreorder", ""]
        starts = set(self.code)
        labels = relink_labels()
        n_ptr = 0
        self.n_frame = 0
        for name, ps in self.pieces.items():
            for i, (lo, hi, k) in enumerate(ps):
                if k[0] == "common":
                    # the common's definition at its address (the C's commons resolve to it)
                    sz = k[2]
                    al = max(self.lay.commons[k[1]][1], 1)
                    L += ['.section .lo_%s_%d, "aw", @nobits' % (name, i), ".p2align %d" % (al.bit_length() - 1),
                          ".globl %s" % k[1], ".type %s, @object" % k[1], ".size %s, %d" % (k[1], sz),
                          "%s:" % k[1], "    .space %d" % sz, ""]
                    continue
                if k[0] != "left":
                    continue
                sec = ".lo_%s_%d" % (name, i)
                if name == "zero":
                    L += ['.section %s, "aw", @nobits' % sec, ".p2align 0", "    .space 0x%X" % (hi - lo), ""]
                    continue
                L += ['.section %s, "aw", @progbits' % sec, ".p2align 0"]
                ptrs = pointer_words(lo, hi, starts, labels, self.lay.reloc_hints)
                a = lo
                raw = []

                def flush():
                    if raw:
                        for j in range(0, len(raw), 16):
                            L.append("    .byte " + ",".join("0x%02X" % b for b in raw[j:j + 16]))
                        raw.clear()
                while a < hi:
                    if a in ptrs and a + 4 <= hi:
                        e = self.expr(ptrs[a])
                        if datalayout.FRAME_TABLE <= a < 0x0028F020 and ptrs[a] in self.frame_sym:
                            e = self.frame_sym[ptrs[a]]
                            self.n_frame += 1
                        if e is not None:
                            flush()
                            L.append("    .4byte %s" % e)
                            n_ptr += 1
                            a += 4
                            continue
                    raw.append(rbytes(a, a + 1)[0])
                    a += 1
                flush()
                L.append("")
        p = os.path.join(self.dir, "leftover.s")
        open(p, "w").write("\n".join(L) + "\n")
        return p, n_ptr

    def script(self, undefined):
        """The linker script: the pieces in order, and every name the C needs."""
        rel_obj = lambda r: os.path.relpath(obj_of(self.build, r), ROOT).replace(os.sep, "/")
        left = os.path.relpath(os.path.join(self.dir, "leftover.o"), ROOT).replace(os.sep, "/")
        T = ["ENTRY(_start)", "SECTIONS", "{", "    . = 0x%08X;" % CODE_LO, "    .text : {"]
        for r in self.order:
            # the start symbol after the input section's own alignment (ld pads before it)
            T.append("        . = ALIGN(%d); __t_%d = .; %s(.text)" % (self.align(r, ".text"), self.index[r], rel_obj(r)))
        T.append("    }")
        out_names = {"init": ".data", "small": ".sdata", "zero": ".bss"}
        aligns = {"init": "ALIGN(128)", "small": "ALIGN(0x100)", "zero": "ALIGN(0x100)"}
        for name, lo, hi, kinds in REGIONS:
            T.append("    . = %s;" % aligns[name])
            if name == "small":
                T.append("    __small_start = .;")
            if name == "zero":
                T.append("    _fbss = .;")
            T.append("    %s%s : {" % (out_names[name], " (NOLOAD)" if name == "zero" else ""))
            if name == "zero":
                # small commons ($gp-relative: egcs's typeinfo nodes, -G8 globals)
                # and the unplaced .sbss first, inside the $gp window
                T.append("        *(.scommon)")
                for r, sn in self.lay.unplaced[name]:
                    if sn == ".sbss":
                        T.append("        %s(%s)" % (rel_obj(r), sn))
                # the pieces keep the original's alignment (crt0's _args is
                # 64-byte aligned: the kernel writes it with quadword stores)
                T.append("        . = ALIGN(0x100);")
            for i, (plo, phi, k) in enumerate(self.pieces[name]):
                s = self.piece_sym[(name, i)]
                if k[0] == "c":
                    T.append("        . = ALIGN(%d); %s = .; %s(%s)" % (self.align(k[1], k[2]), s, rel_obj(k[1]), k[2]))
                elif k[0] == "common":
                    T.append("        . = ALIGN(%d); %s = .; %s(.lo_%s_%d)"
                             % (max(self.lay.commons[k[1]][1], 1), s, left, name, i))
                else:
                    T.append("        %s = .; %s(.lo_%s_%d)" % (s, left, name, i))
            for r, sn in self.lay.unplaced[name]:
                if not (name == "zero" and sn == ".sbss"):
                    T.append("        %s(%s)" % (rel_obj(r), sn))
            if name == "zero":
                T.append("        *(COMMON)")
            T.append("    }")
        T += ["    _end = .;", "    __bss_end = .;", "    _gp = __small_start + 0x7FF0;"]
        for n, e in sorted(undefined.items()):
            T.append("    %s = %s;" % (n, e))
        T += ["    /DISCARD/ : { *(.reginfo) *(.mdebug*) *(.comment) *(.pdr) *(.note*) *(.gnu.attributes) }",
              "}"]
        p = os.path.join(self.dir, "full.ld")
        open(p, "w").write("\n".join(T) + "\n")
        return p


# vtables only a dead vptr store references (an abstract base's inline
# constructor, the derived constructor stores its own right after): retail
# has no such vtable, so any value does; ref/matching/followups.md
DEAD_STORE_VTABLES = {"_vt$Q29gt2k_menu11CommentMenu"}

_EXTERN_LINES = None


def extern_lines():
    """{name: address} of every `extern ... name ...; /* 0x00XXXXXX' line
    (hybrid.extern_line's rule, read once for all names)."""
    global _EXTERN_LINES
    if _EXTERN_LINES is None:
        pat = re.compile(r"^\s*extern\b([^;]*);\s*/[*/]\s*0x(00[1-4][0-9A-Fa-f]{5})")
        found = collections.defaultdict(set)
        for top in ("include", "src"):
            for dp, _, fns in os.walk(os.path.join(ROOT, top)):
                for fn in fns:
                    if fn.endswith((".h", ".c", ".cpp")):
                        for L in open(os.path.join(dp, fn), encoding="utf-8", errors="replace"):
                            m = pat.match(L)
                            if m:
                                for w in re.findall(r"[A-Za-z_$][\w$]*", m.group(1)):
                                    found[w].add(int(m.group(2), 16))
        _EXTERN_LINES = {n: next(iter(v)) for n, v in found.items() if len(v) == 1}
    return _EXTERN_LINES


_NAMES = None


def address_names():
    """(declared, table names, vtables), read once (hybrid.declared() rescans
    the tree every call)."""
    global _NAMES
    if _NAMES is None:
        _NAMES = (hybrid.declared(), hybrid.table_names(), hybrid.vtables())
    return _NAMES


def address_of(name):
    """An original address for a name the C uses but does not define."""
    decl, tables, vts = address_names()
    a = decl.get(name)
    if a is None:
        a = tables.get(name)
    if a is None:
        a = vts.get(name)
    if a is None:
        m = hybrid.ADDR_IN_NAME.search(name)
        if m:
            a = int(m.group(1), 16)
    if a is None:
        a = extern_lines().get(name)
    return a


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", default=os.path.join(ROOT, "build", "full"))
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--pack")
    ap.add_argument("--compile-only", action="store_true")
    ap.add_argument("--no-compile", action="store_true", help="link the objects already in <build>/obj/")
    ap.add_argument("--progress", action="store_true", help="print [done/total] lines while compiling")
    ap.add_argument("--keep-going", action="store_true",
                    help="link a file that fails to compile from its last good object")
    ap.add_argument("--toolchain", help="the EE toolchain (its assembler and ld; default: tools/paths.py)")
    ap.add_argument("--relink", default=datalayout.RELINK_DIR,
                    help="the byte-identical relink of the split (tools/relink.py; default %(default)s)")
    args = ap.parse_args()
    datalayout.RELINK_DIR = os.path.abspath(args.relink)
    build = os.path.abspath(args.build_dir)
    global TOOLCHAIN
    TOOLCHAIN = args.toolchain
    hybrid.TC_BIN = paths.ee_bin(TOOLCHAIN)
    if args.no_compile:
        rels = sources()
    else:
        print("== compile (matching flags) into %s" % os.path.join(build, "obj"), flush=True)
        rels, ok = compile_all(build, args.jobs, args.progress, args.keep_going)
        if not ok:
            return 1
    print("   %d sources" % len(rels))
    if args.compile_only:
        return 0
    import time
    t0 = time.time()
    print("== layout: the objects' data at their original places", flush=True)
    lay = Layout(build, rels)
    print("   (%.0f s)" % (time.time() - t0), flush=True)
    print("   " + ", ".join("%s %d" % kv for kv in sorted(lay.notes.items())))
    for name, lo, hi, _ in REGIONS:
        ps = lay.pieces(name)
        c = sum(h - l for l, h, k in ps if k[0] != "left")
        print("   %-5s 0x%08X..0x%08X: %d bytes from the C, %d leftover in %d pieces, %d C sections unplaced"
              % (name, lo, hi, c, (hi - lo) - c, sum(1 for p in ps if p[2][0] == "left"), len(lay.unplaced[name])))

    print("== link", flush=True)
    t0 = time.time()
    lk = Linker(build, rels, lay)
    # collect2's file: written from this first map, compiled in place of
    # collect2_ctors.c's object (it includes that file), then the layout
    # and the map again with it
    src = lk.collect2_source()
    c2o = os.path.join(lk.dir, "collect2_tables.o")
    ok, msg = mcc.compile(os.path.relpath(src, ROOT).replace(os.sep, "/"), c2o, flags_of=COLLECT2)
    if not ok:
        print("collect2's file did not compile:\n" + msg[-3000:])
        return 1
    import shutil
    fj = obj_of(build, COLLECT2)[:-2] + ".funcs.json"
    if os.path.exists(fj):
        shutil.copy(fj, c2o[:-2] + ".funcs.json")
    OVERRIDE[COLLECT2] = c2o
    lay = Layout(build, rels)
    print("   with collect2's file: " + ", ".join("%s %d" % kv for kv in sorted(lay.notes.items())))
    lk = Linker(build, rels, lay)
    lk.collect2_source()                                # its fallback names against this layout
    print("   linker set up (%.0f s): %d functions mapped, %d weak copies given the kept copy, %d without C"
          % (time.time() - t0, len(lk.code) - len(lk.aliases), len(lk.aliases), len(lk.no_code)), flush=True)
    for a in lk.no_code[:40]:
        print("     no C: 0x%08X" % a)
    rsp = os.path.join(lk.dir, "objs.rsp")
    with open(rsp, "w") as f:
        f.write("\n".join(os.path.relpath(obj_of(build, r), ROOT).replace(os.sep, "/") for r in lk.order) + "\n")
    # what the C uses and no object defines
    allo = os.path.join(lk.dir, "all.o")
    r = run([tool("ld"), "-m", "elf32lr5900", "-r", "-o", allo, "@" + rsp])
    if r.returncode:
        print(r.stderr[-3000:])
        return 1
    und = sorted(x for x in run([tool("nm"), "-u", allo]).stdout.split() if x != "U")
    by_script = {"_gp", "_end", "__bss_end", "_fbss"}
    need, unresolved, aliased, how = {}, [], [], collections.Counter()
    at_original = []                                # names given their original address
    for n in und:
        if n in by_script:
            continue
        if n in lk.c2_names:                            # collect2's entries with no symbol here
            need[n] = lk.c2_names[n]
            how["collect2 entry left original"] += 1
            continue
        if n in hybrid.CRT0_SYMBOLS and n != "_args" and n != "_fbss":
            need[n] = "0x%08X" % hybrid.CRT0_SYMBOLS[n]
            how["crt0 constant"] += 1
            continue
        a = address_of(n) if n != "_args" else hybrid.CRT0_SYMBOLS["_args"]
        e = lk.expr(a) if a is not None else None
        if e is None and n in DEAD_STORE_VTABLES:
            need[n] = "0"
            how["dead vptr store"] += 1
            continue
        if e is None:
            unresolved.append(n)
            continue
        need[n] = e
        how["original address"] += 1
        at_original.append(n)
        # an undefined name inside a C section: that section sits on another
        # variable's original place (a base from zero bytes: T435's guard on
        # g_00352AB0, T529's jump table on the SemaParams, u099's notes)
        k = lk.piece_of(a) if not CODE_LO <= a < CODE_HI else None
        if k and k[0] == "c":
            aliased.append("%s 0x%08X in %s %s" % (n, a, k[1], k[2]))
    print("   names the C uses and no object defines: %d (%s); unresolved %d"
          % (len(und), ", ".join("%s %d" % kv for kv in how.items()), len(unresolved)))
    for n in unresolved[:40]:
        print("     unresolved: " + n)
    for x in aliased:
        print("     inside a C section (its base is wrong, or the name is not defined where it is): " + x)
    if len(at_original) <= 20:
        for n in at_original:
            print("     at its original address (no C defines it): " + n)
    t0 = time.time()
    lk.dump_map()
    src, n_ptr = lk.leftover_asm()
    print("   leftover written (%.0f s)" % (time.time() - t0), flush=True)
    r = run([mcc.CC, "-c", os.path.relpath(src, ROOT).replace(os.sep, "/"),
             "-o", os.path.relpath(os.path.join(lk.dir, "leftover.o"), ROOT).replace(os.sep, "/")])
    if r.returncode:
        print(r.stdout + r.stderr)
        return 1
    print("   leftover pieces: %s (%d pointer words symbolic; frame table: %d of %d entries the objects' own frame info)"
          % (os.path.relpath(src, ROOT), n_ptr, lk.n_frame, len(datalayout.frame_table())))
    ld = lk.script(need)
    elf = os.path.join(lk.dir, "core.elf")
    r = run([tool("ld"), "-m", "elf32lr5900", "-T", os.path.relpath(ld, ROOT).replace(os.sep, "/"),
             "-Map", os.path.relpath(os.path.join(lk.dir, "core.map"), ROOT).replace(os.sep, "/"),
             "-o", os.path.relpath(elf, ROOT).replace(os.sep, "/"), "@" + rsp,
             os.path.relpath(os.path.join(lk.dir, "leftover.o"), ROOT).replace(os.sep, "/")])
    if r.returncode or not os.path.exists(elf):
        print("LINK FAILED:\n" + r.stderr[-6000:])
        return 1
    if r.stderr.strip():
        print(r.stderr[-3000:])
    print("   linked %s" % os.path.relpath(elf, ROOT))
    if args.pack:
        import relink
        relink.pack(open(elf, "rb").read(), args.pack)
    return 0


if __name__ == "__main__":
    sys.exit(main())
