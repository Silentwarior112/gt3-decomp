"""Lupo 2 (a later revision of the GT3 code base, GT Concept, PAPX-905.09)
against GT3 US: which Lupo 2 function is which GT3 function, and what changed.

Lupo 2 is a recompile with another compiler set-up, so its bytes cannot be
compared with GT3's; what a function *does* can: the functions it calls, the
virtual slots it calls through, the float constants it loads, the struct
offsets it reads and writes, the integer constants it tests.  This tool pairs
the two builds' functions and compares those features.

The Lupo 2 executable is not part of the repository: its path comes from
<repo>/local_paths.json, key "lupo2_elf" (or $GT3_LUPO2_ELF).  Its class and
vtable tables are in ref/lupo2/ (vtables_L2.csv / vtables_US.csv, the
.gnu.linkonce names in lupo2_functions.csv).

    python tools/lupo2.py map                 build ref/lupo2/map.json (run first)
    python tools/lupo2.py classes             shared / Lupo-2-only / GT3-only classes, vtable length changes
    python tools/lupo2.py class RaceCarModel  one class's vtable, slot by slot, with our source functions
    python tools/lupo2.py dis 0x001E1288      a Lupo 2 function, annotated (--us: a GT3 US function)
    python tools/lupo2.py who 0x001E1288      what a Lupo 2 address is in GT3 (and the reverse with --us)
    python tools/lupo2.py cmp 0x001E1288      a paired function's features, Lupo 2 against GT3
    python tools/lupo2.py changed [--area src/game/car/physics] [--min 0.3]
                                              paired functions ranked by how much their features differ
    python tools/lupo2.py unmatched [--near ADDR]
                                              Lupo 2 functions with no GT3 twin (new code), with their callers
"""
import argparse
import bisect
import collections
import csv
import difflib
import json
import os
import re
import struct
import sys

import paths
import funcmap

REF = os.path.join(paths.ROOT, "ref", "lupo2")
MAP = os.environ.get("L2_MAP") or os.path.join(REF, "map.json")
FIXES = os.path.join(REF, "pairs_fix.csv")      # pairs established by hand (see fixes())
SP, GP, FP, RA, ZERO = 29, 28, 30, 31, 0


# ---------------------------------------------------------------- images

class Image:
    def __init__(self, path, name):
        self.name = name
        self.b = open(path, "rb").read()
        b = self.b
        phoff, = struct.unpack_from("<I", b, 0x1C)
        n, = struct.unpack_from("<H", b, 0x2C)
        self.entry, = struct.unpack_from("<I", b, 0x18)
        self.segs = []
        for i in range(n):
            t, off, va, pa, fs, ms, fl, al = struct.unpack_from("<8I", b, phoff + i * 32)
            if t == 1:
                self.segs.append((va, off, fs))
        self.gp = self._find_gp()

    def off(self, a):
        for va, off, fs in self.segs:
            if va <= a < va + fs:
                return off + a - va
        return None

    def word(self, a):
        o = self.off(a)
        return struct.unpack_from("<I", self.b, o)[0] if o is not None else None

    def float_at(self, a):
        w = self.word(a)
        return None if w is None else struct.unpack("<f", struct.pack("<I", w))[0]

    def _find_gp(self):
        """crt0 builds _gp in a register (lui / addiu $a0: SetupThread's
        argument) and moves it to $gp; follow the constants it builds."""
        reg = {}
        for a in range(self.entry, self.entry + 0x200, 4):
            w = self.word(a)
            op, rs, rt, rd, fn, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63, w & 0xFFFF
            if op == 0x0F:
                reg[rt] = imm << 16
            elif op in (0x09, 0x0D) and rs in reg:
                reg[rt] = (reg[rs] + (imm - 0x10000 if op == 0x09 and imm & 0x8000 else imm)) & 0xFFFFFFFF
            elif op == 0 and fn in (0x21, 0x25, 0x2D) and rt == ZERO and rs in reg:     # move rd, rs
                reg[rd] = reg[rs]
            if reg.get(GP):
                return reg[GP]
        return None


_L2 = _US = None


def l2():
    global _L2
    if _L2 is None:
        p = os.environ.get("GT3_LUPO2_ELF") or paths.local("lupo2_elf")
        if not p or not os.path.isfile(p):
            sys.exit("the Lupo 2 executable: set \"lupo2_elf\" in <repo>/local_paths.json (or $GT3_LUPO2_ELF)")
        _L2 = Image(p, "L2")
    return _L2


def us():
    global _US
    if _US is None:
        _US = Image(paths.RETAIL_ELF, "US")
    return _US


# ---------------------------------------------------------------- functions

_FN = {}


def functions(which):
    """[(start, end, name)] sorted: Lupo 2 from its frame info and linkonce
    sections (ref/lupo2/lupo2_functions.csv), GT3 US from ref/funcs.json with
    our names (FUNC markers)."""
    if which in _FN:
        return _FN[which]
    out = []
    if which == "L2":
        for r in csv.DictReader(open(os.path.join(REF, "lupo2_functions.csv"))):
            out.append((int(r["start"], 16), int(r["end"], 16), r["name"] or ""))
        out += l2_more_functions(out)
    else:
        names = us_names()
        for f in json.load(open(os.path.join(paths.REF, "funcs.json"))):
            a = int(f["start"], 16)
            out.append((a, int(f["end"], 16), names.get(a, ("", ""))[1]))
    out.sort()
    _FN[which] = out
    return out


def l2_more_functions(known):
    """Lupo 2 functions its frame info does not list (leaves without an FDE):
    the fixes file's starts, `jal` targets and vtable entries outside every
    known function; each ends at its last `jr $ra' (scan_end) or at the next
    start."""
    img = l2()
    known = sorted(known)
    ks = [f[0] for f in known]
    text_hi = max(f[1] for f in known)

    def inside(a):
        i = bisect.bisect_right(ks, a) - 1
        return i >= 0 and ks[i] <= a < known[i][1]
    ends = {}
    cand = set()
    for r in fix_rows():
        if r["verdict"] == "start":
            a = int(r["l2"], 16)
            cand.add(a)
            if r.get("end"):
                ends[a] = int(r["end"], 16)
    for a in range(0x00100000, text_hi, 4):
        w = img.word(a)
        if w is not None and w >> 26 == 0x03:
            cand.add(((w & 0x03FFFFFF) << 2) | (a & 0xF0000000))
    for c, ms in vtables("L2").items():
        cand.update(ms)
    new = sorted(a for a in cand if 0x00100000 <= a < text_hi and a % 4 == 0 and a not in ks and not inside(a))
    allstarts = sorted(set(ks) | set(new))
    out = []
    for a in new:
        j = bisect.bisect_right(allstarts, a)
        limit = allstarts[j] if j < len(allstarts) else text_hi
        out.append((a, min(ends.get(a) or scan_end(img, a, limit), limit), ""))
    return out


def scan_end(img, a, limit):
    """A leaf's end: the `jr $ra' (and its delay slot) no branch jumps past."""
    e, far = a, a
    while e < min(limit, a + 0x4000):
        w = img.word(e)
        if w is None:
            break
        op = w >> 26
        if op in (1, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (op == 0x11 and (w >> 21) & 31 == 8):
            far = max(far, e + 4 + (sx(w & 0xFFFF) << 2))
        e += 4
        if w == 0x03E00008 and e >= far:
            return e + 4
    return e


_FIX = None


def fix_rows():
    """ref/lupo2/pairs_fix.csv: l2,us,verdict,end,note.  verdict: right (l2's
    twin is us), new (l2 has no GT3 twin), wrong (l2 is not us), start (a
    Lupo 2 function the frame info does not list)."""
    global _FIX
    if _FIX is None:
        _FIX = list(csv.DictReader(open(FIXES))) if os.path.exists(FIXES) else []
    return _FIX


_USN = None


def us_names():
    """{GT3 US address: (source file, marker name)}."""
    global _USN
    if _USN is None:
        _USN = {}
        for f, ms in funcmap.markers().items():
            for _, a, nm in ms:
                _USN.setdefault(a, (f, nm))
    return _USN


def bounds(which, a):
    fs = functions(which)
    starts = [f[0] for f in fs]
    i = bisect.bisect_right(starts, a) - 1
    if i >= 0 and fs[i][0] <= a < fs[i][1]:
        return fs[i]
    # a leaf without frame info: up to the `jr $ra` no branch jumps past
    # (an early return is not the end), and never into the next known function
    img = l2() if which == "L2" else us()
    j = bisect.bisect_right(starts, a)
    limit = min(a + 0x4000, starts[j]) if j < len(starts) else a + 0x4000
    return (a, min(scan_end(img, a, limit), limit), "")


# ---------------------------------------------------------------- features

def decode(img, lo, hi):
    """[(addr, word)]"""
    return [(a, img.word(a)) for a in range(lo, hi, 4)]


def sx(imm):
    return imm - 0x10000 if imm & 0x8000 else imm


def features(which, a):
    """What a function does, as comparable sets: calls (targets in order),
    vslots (virtual call slots), floats, offsets (struct displacements off
    registers other than sp / gp), imms (integer constants), and size."""
    img = l2() if which == "L2" else us()
    lo, hi, name = bounds(which, a)
    ins = decode(img, lo, hi)
    calls, vslots, floats, offs, imms = [], collections.Counter(), collections.Counter(), collections.Counter(), \
        collections.Counter()
    hi16 = {}                    # reg -> lui value
    lastload = {}                # reg -> (base reg, disp) of the last lw into it
    # reg -> [imm, used] of `addiu reg, base, imm': Lupo 2 addresses a vtable
    # entry so (addiu v1, vptr, 0x38; lh 0(v1); lw 4(v1)) where GT3 loads
    # 0x38(vptr) / 0x3C(vptr); both are counted GT3's way
    addoff = {}
    nbr = 0
    written = None               # the register the previous instruction overwrote (no longer a lui value)
    for pc, w in ins:
        op, rs, rt, rd, fn, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, w & 63, w & 0xFFFF
        if written is not None:
            hi16.pop(written, None)
        written = rd if op == 0 and fn not in (8, 0x0C, 0x0D, 0x0F) else \
            rt if op in (0x08, 0x0A, 0x0B, 0x0C, 0x0E, 0x18, 0x19, 0x1A, 0x1B, 0x20, 0x21, 0x23, 0x24, 0x25,
                         0x27, 0x37, 0x1E) or (op in (0x09, 0x0D) and rs not in hi16) else None
        if op in (0x21, 0x23, 0x25) and rs in addoff and sx(imm) in (0, 4):
            # a load through an added offset: the displacement is the sum
            ao = addoff[rs]
            if not ao[1]:
                ao[1] = True
                imms[ao[0]] -= 1
                if imms[ao[0]] <= 0:
                    del imms[ao[0]]
            offs[ao[0] + sx(imm)] += 1
            if op == 0x23:
                lastload[rt] = (rs, ao[0] + sx(imm))
            addoff.pop(rt, None)
            continue
        if op == 0x09 and rs not in (SP, GP, ZERO) and rs not in hi16 and rt != SP:
            addoff[rt] = [sx(imm), False]
        elif op == 0 and rd in addoff:
            del addoff[rd]
        elif op not in (0, 1, 4, 5, 6, 7, 0x09, 0x14, 0x15, 0x16, 0x17, 0x28, 0x29, 0x2B, 0x39, 0x3F) \
                and rt in addoff:                               # anything else writing rt
            del addoff[rt]
        if op == 0x03:                                          # jal
            calls.append(((w & 0x03FFFFFF) << 2) | (pc & 0xF0000000))
        elif op == 0x02:                                        # j out of the function: a tail call
            t = ((w & 0x03FFFFFF) << 2) | (pc & 0xF0000000)
            if not lo <= t < hi:
                calls.append(t)
        elif op == 0 and fn == 9:                               # jalr
            base = lastload.get(rs)
            if base and (base[1] - 0xC) % 8 == 0 and base[1] >= 0xC:
                vslots[(base[1] - 0xC) // 8] += 1
            else:
                vslots["indirect"] += 1
        if op in (1, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17) or (op == 0x11 and rs == 8):
            nbr += 1
        if op == 0x0F:                                          # lui
            hi16[rt] = imm << 16
            continue
        if op in (0x09, 0x0D) and rs in hi16 and rt != SP:      # lui + addiu / ori: an address or a constant
            v = (hi16[rs] + (sx(imm) if op == 0x09 else imm)) & 0xFFFFFFFF
            f = struct.unpack("<f", struct.pack("<I", v))[0]
            if op == 0x0D and 1e-6 < abs(f) < 1e7:
                floats[round(f, 5)] += 1                        # li.s through an integer register
            continue
        if op in (0x09, 0x0D, 0x0A, 0x0B, 0x0C) and rs not in (SP, GP) and imm not in (0, 1, 0xFFFF):
            imms[sx(imm) if op in (0x09, 0x0A) else imm] += 1
        if op == 0x31 or op == 0x35:                            # lwc1 / ldc1: a float constant?
            addr = None
            if rs == GP and img.gp:
                addr = (img.gp + sx(imm)) & 0xFFFFFFFF
            elif rs in hi16:
                addr = (hi16[rs] + sx(imm)) & 0xFFFFFFFF
            if addr is not None:
                f = img.float_at(addr)
                if f is not None and (f == 0 or 1e-7 < abs(f) < 1e9):
                    floats[round(f, 5)] += 1
                continue
        if op in (0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2B, 0x31, 0x39, 0x37, 0x3F, 0x1E, 0x1F) and \
                rs not in (SP, GP, ZERO) and rs not in hi16:
            offs[sx(imm)] += 1
            if op == 0x23:
                lastload[rt] = (rs, sx(imm))
    return dict(lo=lo, hi=hi, name=name, n=len(ins), branches=nbr, calls=calls, vslots=vslots, floats=floats,
                offsets=offs, imms=imms)


# ---------------------------------------------------------------- the map

def vtables(which):
    """{class: [method address per slot]}"""
    p = os.path.join(REF, "vtables_%s.csv" % which)
    out = collections.defaultdict(dict)
    for r in csv.DictReader(open(p)):
        out[r["class_name"]][int(r["slot_index"])] = int(r["method_addr"], 16)
    return {c: [v[i] for i in sorted(v)] for c, v in out.items()}


def our_symbols():
    """{mangled symbol: GT3 US address} from the last full score run (build/score/symmap.json)."""
    p = os.path.join(paths.ROOT, "build", "score", "symmap.json")
    return json.load(open(p)) if os.path.exists(p) else {}


def shape(f):
    return (f["n"], len(f["calls"]), sum(f["vslots"].values()), f["branches"])


def jac(x, y):
    u = sum((x | y).values())
    return sum((x & y).values()) / u if u else None


def sim(fa, fb):
    """How alike two functions look without knowing either's callees: size,
    call count, float constants, struct offsets, integer constants, virtual
    slots (0..1)."""
    na, nb = fa["n"], fb["n"]
    s, w = 2.0 * min(na, nb) / max(na, nb, 1), 2.0
    ca, cb = len(fa["calls"]), len(fb["calls"])
    s += min(ca, cb) / max(ca, cb) if max(ca, cb) else 1.0
    w += 1
    for k, kw in (("floats", 2), ("offsets", 2), ("imms", 1), ("vslots", 1)):
        j = jac(fa[k], fb[k])
        if j is not None:
            s += kw * j
            w += kw
    return s / w


def align(xs, ys, score, threshold):
    """Order-preserving pairs of xs and ys (Needleman-Wunsch on score, gaps
    free) whose score reaches threshold."""
    n, m = len(xs), len(ys)
    if not n or not m or n * m > 40000:
        return []
    S = [[score(x, y) for y in ys] for x in xs]
    D = [[0.0] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            s = S[i - 1][j - 1]
            D[i][j] = max(D[i - 1][j], D[i][j - 1], D[i - 1][j - 1] + (s if s >= threshold else -1))
    out, i, j = [], n, m
    while i and j:
        s = S[i - 1][j - 1]
        if s >= threshold and D[i][j] == D[i - 1][j - 1] + s:
            out.append((xs[i - 1], ys[j - 1]))
            i, j = i - 1, j - 1
        elif D[i][j] == D[i - 1][j]:
            i -= 1
        else:
            j -= 1
    return out[::-1]


def similar_shape(a, b):
    na, nb = a["n"], b["n"]
    return min(na, nb) / max(na, nb, 1) > 0.5 and abs(len(a["calls"]) - len(b["calls"])) <= max(2, len(b["calls"]) // 2)


def strings_of(which, a):
    """The C strings a function's code addresses (lui + addiu pairs)."""
    img = l2() if which == "L2" else us()
    lo, hi, _ = bounds(which, a)
    hi16, out = {}, []
    for pc, w in decode(img, lo, hi):
        op, rs, rt, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
        if op == 0x0F:
            hi16[rt] = imm << 16
        elif op == 0x09 and rs in hi16:
            s = c_string(img, (hi16[rs] + sx(imm)) & 0xFFFFFFFF)
            if s:
                out.append(s)
    return out


def c_string(img, a):
    o = img.off(a)
    if o is None:
        return None
    e = img.b.find(b"\0", o, o + 200)
    if e <= o + 2:
        return None
    s = img.b[o:e]
    return s.decode("latin-1") if all(32 <= c < 127 or c in (9, 10) for c in s) else None


def mangled_key(sym):
    """A mangled name with Lupo 2's `unsigned long` parameters read as GT3's
    `unsigned int` (u32 was long there)."""
    return sym.replace("Ul", "Ui")


def build_map():
    pairs = {}                                                  # L2 -> (US, how, votes)
    l2v, usv = vtables("L2"), vtables("US")
    # 0. the pairs established by hand (ref/lupo2/pairs_fix.csv) are fixed;
    #    a Lupo-2-only function is never paired, a wrong pair never made
    lone, wrong = set(), set()
    for r in fix_rows():
        a = int(r["l2"], 16)
        if r["verdict"] == "right" and r["us"]:
            pairs[a] = (int(r["us"], 16), "fix", 1)
        elif r["verdict"] == "new":
            lone.add(a)
        elif r["verdict"] == "wrong" and r["us"]:
            wrong.add((a, int(r["us"], 16)))
    fixed_us = {v[0] for v in pairs.values()}

    def free(a, b):
        return a not in pairs and a not in lone and (a, b) not in wrong and b not in fixed_us
    # 1. names: Lupo 2's linkonce copies against our compiled symbols
    syms = our_symbols()
    by_key = collections.defaultdict(set)
    by_dem = collections.defaultdict(set)
    for s, a in syms.items():
        by_key[mangled_key(s)].add(a)
        d = funcmap.demangle(s)
        if d:
            by_dem[d].add(a)
    for lo, hi, nm in functions("L2"):
        if not nm:
            continue
        hit = by_key.get(mangled_key(nm))
        if not hit:
            d = funcmap.demangle(nm)
            hit = by_dem.get(d) if d else None
        if hit and len(hit) == 1 and free(lo, next(iter(hit))):
            pairs[lo] = (next(iter(hit)), "name", 1)
    # 1b. the earlier cross-build map's HIGH pairs (ref/lupo2/lupo2_to_gt3_map.csv)
    for r in csv.DictReader(open(os.path.join(REF, "lupo2_to_gt3_map.csv"))):
        if r["confidence"] == "HIGH" and r["us_addr"]:
            a = int(r["lupo2_addr"], 16)
            if free(a, int(r["us_addr"], 16)):
                pairs[a] = (int(r["us_addr"], 16), "fingerprint", 1)
    # 1c. strings only one function of each build addresses
    def unique_strings(which):
        refs = collections.defaultdict(set)
        for lo, hi, nm in functions(which):
            for s in set(strings_of(which, lo)):
                refs[s].add(lo)
        return {s: next(iter(fs)) for s, fs in refs.items() if len(fs) == 1 and len(s) >= 4}
    ul2, uus = unique_strings("L2"), unique_strings("US")
    for s, a in ul2.items():
        if s in uus and free(a, uus[s]):
            pairs[a] = (uus[s], "string", 1)
    # 2. vtables of the shared classes, slot by slot (equal lengths; else the
    #    common prefix while the shapes agree)
    fcache = {}

    def feat(which, a):
        k = (which, a)
        if k not in fcache:
            fcache[k] = features(which, a)
        return fcache[k]
    for c, sl in l2v.items():
        su = usv.get(c)
        if not su:
            continue
        n = min(len(sl), len(su))
        for i in range(n):
            a, b = sl[i], su[i]
            if not free(a, b):
                continue
            if len(sl) != len(su) and not similar_shape(feat("L2", a), feat("US", b)):
                break
            pairs[a] = (b, "vtable", 1)
    # 3. calls: a paired function's callees pair up where both call lists
    #    agree around them (difflib on the mapped sequences), repeated
    l2starts = [f[0] for f in functions("L2")]
    usstarts = [f[0] for f in functions("US")]

    def fsim(x, y):
        fa, fb = feat("L2", x), feat("US", y)
        s = sim(fa, fb)
        # tiny functions (accessors) all look alike: only a near-exact match
        if min(fa["n"], fb["n"]) <= 6 and s < 0.95:
            return 0.0
        return s
    rounds = 0
    while rounds < 12:
        rounds += 1
        before = len(pairs)
        used = {v[0] for v in pairs.values()}
        votes = collections.defaultdict(collections.Counter)
        for a, (b, how, _) in list(pairs.items()):
            fa, fb = feat("L2", a), feat("US", b)
            ca, cb = fa["calls"], fb["calls"]
            if not ca or not cb:
                continue
            seq_a = [pairs[x][0] if x in pairs else ("?", x) for x in ca]
            sm = difflib.SequenceMatcher(None, seq_a, cb, autojunk=False)
            for tag, i1, i2, j1, j2 in sm.get_opcodes():
                if tag != "replace":
                    continue
                if i2 - i1 == j2 - j1:
                    for k in range(i2 - i1):
                        x, y = ca[i1 + k], cb[j1 + k]
                        if y not in used and free(x, y):
                            votes[x][y] += 1
                else:
                    xs = list(dict.fromkeys(x for x in ca[i1:i2] if x not in pairs and x not in lone))
                    ys = list(dict.fromkeys(y for y in cb[j1:j2] if y not in used))
                    for x, y in align(xs, ys, fsim, float(os.environ.get("L2_T_CALLS", "0.7"))):
                        if free(x, y):
                            votes[x][y] += 1
        for x, c in votes.items():
            if x in pairs:
                continue
            (y, n), = c.most_common(1)
            if y in used:
                continue
            if n >= 1 and (len(c) == 1 or n > 2 * sum(c.values()) // 3):
                # two callers agreeing are evidence enough (a changed
                # function's shape may differ); one needs the shape too
                if n >= 2 or similar_shape(feat("L2", x), feat("US", y)):
                    pairs[x] = (y, "calls", n)
                    used.add(y)
        # link order: both builds keep a source file's functions in order,
        # so the unpaired functions between two consecutive pairs align
        anchors = sorted((a, v[0]) for a, v in pairs.items())
        for (a1, b1), (a2, b2) in zip(anchors, anchors[1:]):
            if not b1 < b2 or b2 - b1 > 0x8000 or a2 - a1 > 0x8000:
                continue
            xs = [x for x in l2starts[bisect.bisect_right(l2starts, a1):bisect.bisect_left(l2starts, a2)]
                  if x not in pairs and x not in lone]
            ys = [y for y in usstarts[bisect.bisect_right(usstarts, b1):bisect.bisect_left(usstarts, b2)]
                  if y not in used]
            if len(xs) == 1 and len(ys) == 1:
                # the one function between two pairs on each side: a changed
                # function looks less alike, so a lower bar (marked)
                if fsim(xs[0], ys[0]) >= 0.35 and free(xs[0], ys[0]):
                    pairs[xs[0]] = (ys[0], "order-gap", 1)
                    used.add(ys[0])
                continue
            for x, y in align(xs, ys, fsim, float(os.environ.get("L2_T_ORDER", "0.65"))):
                if free(x, y):
                    pairs[x] = (y, "order", 1)
                    used.add(y)
        if len(pairs) == before:
            break
    out = {"%08X" % a: {"us": "%08X" % b, "how": how, "votes": v} for a, (b, how, v) in sorted(pairs.items())}
    json.dump(out, open(MAP + ".tmp", "w"), indent=0)
    os.replace(MAP + ".tmp", MAP)
    return out


_M = None


def the_map():
    global _M
    if _M is None:
        if not os.path.exists(MAP):
            sys.exit("no %s: run `python tools/lupo2.py map` first" % MAP)
        _M = {int(k, 16): (int(v["us"], 16), v["how"]) for k, v in json.load(open(MAP)).items()}
    return _M


def reverse_map():
    return {b: a for a, (b, _) in the_map().items()}


# ---------------------------------------------------------------- reports

def l2_name(a):
    for lo, hi, nm in functions("L2"):
        if lo == a:
            return nm
    return ""


def describe_l2(a):
    m = the_map().get(a)
    if m:
        f, nm = us_names().get(m[0], ("?", "?"))
        return "L2 %08X = US %08X %s (%s, by %s)" % (a, m[0], nm, f, m[1])
    nm = l2_name(a)
    return "L2 %08X %s (no GT3 twin)" % (a, nm)


def compare(a):
    m = the_map().get(a)
    if not m:
        return None
    fa, fb = features("L2", a), features("US", m[0])
    rev = the_map()
    ca = collections.Counter(rev[x][0] if x in rev else ("L2", x) for x in fa["calls"])
    cb = collections.Counter(fb["calls"])
    parts = {}
    score, weight = 0.0, 0.0
    for k, (x, y), w in (("calls", (ca, cb), 3), ("vslots", (fa["vslots"], fb["vslots"]), 2),
                         ("floats", (fa["floats"], fb["floats"]), 2), ("offsets", (fa["offsets"], fb["offsets"]), 2),
                         ("imms", (fa["imms"], fb["imms"]), 1)):
        only_a, only_b = x - y, y - x
        tot = sum((x | y).values())
        d = (sum(only_a.values()) + sum(only_b.values())) / max(tot, 1)
        parts[k] = (only_a, only_b, d)
        if tot:
            score += w * d
            weight += w
    size = abs(fa["n"] - fb["n"]) / max(fa["n"], fb["n"], 1)
    score += 2 * size
    weight += 2
    return dict(l2=fa, us=fb, parts=parts, diff=score / weight, size=size)


def fmt_call(x):
    if isinstance(x, tuple):
        return "L2:%08X?" % x[1]
    f, nm = us_names().get(x, ("", ""))
    return nm or "%08X" % x


def cmd_cmp(a):
    c = compare(a)
    if c is None:
        print(describe_l2(a))
        return
    print(describe_l2(a))
    print("  size: L2 %d insns / US %d insns, branches %d / %d, difference %.2f" % (
        c["l2"]["n"], c["us"]["n"], c["l2"]["branches"], c["us"]["branches"], c["diff"]))
    for k, (oa, ob, d) in c["parts"].items():
        if not oa and not ob:
            continue
        show = (lambda v: fmt_call(v)) if k == "calls" else (lambda v: ("0x%X" % v if isinstance(v, int) and k == "offsets" else str(v)))
        print("  %-8s only L2: %s" % (k, ", ".join("%s%s" % (show(x), "" if n == 1 else " x%d" % n) for x, n in oa.items()) or "-"))
        print("  %-8s only US: %s" % ("", ", ".join("%s%s" % (show(x), "" if n == 1 else " x%d" % n) for x, n in ob.items()) or "-"))


def cmd_dis(a, which):
    import rabbitizer
    img = l2() if which == "L2" else us()
    lo, hi, nm = bounds(which, a)
    m, rev = the_map() if os.path.exists(MAP) else {}, None
    if os.path.exists(MAP):
        rev = reverse_map()
    print("%s %08X..%08X %s" % (which, lo, hi, nm or (us_names().get(lo, ("", ""))[1] if which == "US" else "")))
    for pc, w in decode(img, lo, hi):
        t = rabbitizer.Instruction(w, vram=pc, category=rabbitizer.InstrCategory.R5900).disassemble()
        note = ""
        if w >> 26 == 3:
            tgt = ((w & 0x03FFFFFF) << 2) | (pc & 0xF0000000)
            if which == "L2":
                if tgt in m:
                    note = "  -> US %s" % fmt_call(m[tgt][0])
                else:
                    note = "  -> %s" % (l2_name(tgt) or "(no GT3 twin)")
            else:
                note = "  -> %s" % fmt_call(tgt)
        op, rs, imm = w >> 26, (w >> 21) & 31, w & 0xFFFF
        if op in (0x31, 0x23) and rs == GP and img.gp:
            ad = (img.gp + sx(imm)) & 0xFFFFFFFF
            note += "  [gp %08X%s]" % (ad, (" = %g" % img.float_at(ad)) if op == 0x31 else "")
        print("  %08X  %08X  %s%s" % (pc, w, t, note))


def cmd_classes():
    l2v, usv = vtables("L2"), vtables("US")
    only_l2 = sorted(set(l2v) - set(usv))
    only_us = sorted(set(usv) - set(l2v))
    grown = [(c, len(usv[c]), len(l2v[c])) for c in sorted(set(l2v) & set(usv)) if len(usv[c]) != len(l2v[c])]
    print("shared %d, Lupo 2 only %d, GT3 only %d, vtable length changed %d" % (
        len(set(l2v) & set(usv)), len(only_l2), len(only_us), len(grown)))
    print("\nLupo 2 only:\n  " + "\n  ".join(only_l2))
    print("\nGT3 only:\n  " + "\n  ".join(only_us))
    print("\nvtable length (GT3 -> Lupo 2):")
    for c, a, b in grown:
        print("  %-50s %3d -> %3d" % (c, a, b))


def cmd_class(name):
    l2v, usv = vtables("L2"), vtables("US")
    cs = [c for c in set(l2v) | set(usv) if c == name or re.sub(r"^\d+", "", c) == name or name in c]
    for c in sorted(cs)[:6]:
        sl, su = l2v.get(c, []), usv.get(c, [])
        print("%s: Lupo 2 %d slots, GT3 %d slots" % (c, len(sl), len(su)))
        for i in range(max(len(sl), len(su))):
            a = sl[i] if i < len(sl) else None
            b = su[i] if i < len(su) else None
            nm = us_names().get(b, ("", ""))[1] if b else ""
            d = ""
            if a and os.path.exists(MAP):
                c2 = compare(a) if a in the_map() else None
                d = (" diff %.2f" % c2["diff"]) if c2 else " (unpaired)"
            print("  [%2d] L2 %-9s US %-9s %s%s" % (i, "%08X" % a if a else "-", "%08X" % b if b else "-", nm, d))


def area_of_us(b):
    f = us_names().get(b, ("", ""))[0]
    return os.path.dirname(f)


def cmd_changed(area, mn, limit, plausible=False):
    rows = []
    for a, (b, how) in the_map().items():
        if area and not area_of_us(b).startswith(area):
            continue
        c = compare(a)
        if not c or c["diff"] < mn:
            continue
        if plausible and min(c["l2"]["n"], c["us"]["n"]) < 0.5 * max(c["l2"]["n"], c["us"]["n"]):
            continue
        rows.append((c["diff"], a, b, how, c))
    rows.sort(reverse=True)
    for d, a, b, how, c in rows[:limit]:
        f, nm = us_names().get(b, ("?", "?"))
        print("%.2f  L2 %08X (%4d)  US %08X (%4d)  %-9s %-46s %s" % (d, a, c["l2"]["n"], b, c["us"]["n"], how,
                                                                    nm[:46], f))
    print("%d paired functions at difference >= %.2f%s" % (len(rows), mn, " (size within 2x)" if plausible else ""))


def l2_area(lo):
    """The area of an unpaired Lupo 2 function: its nearest paired neighbour's
    GT3 folder (link order keeps a source file's functions together)."""
    m = the_map()
    keys = sorted(m)
    i = bisect.bisect_left(keys, lo)
    near = [keys[j] for j in (i - 1, i) if 0 <= j < len(keys)]
    if not near:
        return ""
    k = min(near, key=lambda x: abs(x - lo))
    return area_of_us(m[k][0])


def cmd_unmatched(near, limit, area=""):
    m = the_map()
    callers = collections.defaultdict(set)
    for lo, hi, nm in functions("L2"):
        for x in features("L2", lo)["calls"]:
            callers[x].add(lo)
    rows = []
    for lo, hi, nm in functions("L2"):
        if lo in m or (near and not (near - 0x4000 <= lo <= near + 0x4000)):
            continue
        ar = l2_area(lo)
        if area and not ar.startswith(area):
            continue
        cs = sorted(callers.get(lo, ()))
        known = [describe_l2(x) for x in cs if x in m][:3]
        rows.append((lo, hi, nm, len(cs), known, ar))
    for lo, hi, nm, nc, known, ar in rows[:limit]:
        print("L2 %08X %5d bytes %-40s near %-28s callers %d%s" % (lo, hi - lo, nm[:40], ar, nc,
                                                                   ("  e.g. " + " | ".join(known)) if known else ""))
    print("%d Lupo 2 functions with no GT3 twin%s" % (len(rows), " near %08X" % near if near else ""))


def cmd_summary():
    """Per GT3 folder: paired functions, how many differ (>= 0.35, size
    within 2x), and the unpaired Lupo 2 functions placed there."""
    pairs = collections.Counter()
    diff = collections.Counter()
    for a, (b, how) in the_map().items():
        ar = area_of_us(b)
        pairs[ar] += 1
        c = compare(a)
        if c and c["diff"] >= 0.35 and min(c["l2"]["n"], c["us"]["n"]) >= 0.5 * max(c["l2"]["n"], c["us"]["n"]):
            diff[ar] += 1
    un = collections.Counter(l2_area(lo) for lo, hi, nm in functions("L2") if lo not in the_map())
    print("%-34s %6s %8s %9s" % ("folder", "paired", "differ", "unpaired"))
    for ar in sorted(set(pairs) | set(un)):
        print("%-34s %6d %8d %9d" % (ar or "?", pairs[ar], diff[ar], un[ar]))


def cmd_fixes(files):
    """Merge pairs_fix.csv files (l2,us,verdict,end,note) into FIXES: one
    row per Lupo 2 address; rows that disagree are reported and left out."""
    norm = lambda x: ("0x%08X" % int(x, 16)) if x and x.strip() else ""
    rows = collections.defaultdict(list)
    for f in [FIXES] + files if os.path.exists(FIXES) else files:
        for r in csv.DictReader(open(f, encoding="utf-8")):
            if not r.get("l2") or r.get("verdict") not in ("right", "new", "wrong", "start"):
                continue
            rows[norm(r["l2"])].append(dict(l2=norm(r["l2"]), us=norm(r.get("us")), verdict=r["verdict"],
                                             end=norm(r.get("end")), note=(r.get("note") or "").strip(),
                                             src=os.path.basename(os.path.dirname(f)) or "ref"))
    out, clash = [], []
    for a in sorted(rows):
        rs = rows[a]
        starts = [r for r in rs if r["verdict"] == "start"]
        rest = [r for r in rs if r["verdict"] != "start"]
        if starts:
            out.append(starts[0])
        claims = {(r["verdict"], r["us"]) for r in rest if r["verdict"] in ("right", "new")}
        if len(claims) > 1:
            clash.append((a, rest))
            continue
        if claims:
            out.append(next(r for r in rest if r["verdict"] in ("right", "new")))
        for us in sorted({r["us"] for r in rest if r["verdict"] == "wrong"} - {c[1] for c in claims}):
            out.append(next(r for r in rest if r["verdict"] == "wrong" and r["us"] == us))
    taken = collections.defaultdict(list)
    for r in out:
        if r["verdict"] == "right":
            taken[r["us"]].append(r["l2"])
    for us, l2s in taken.items():
        if len(l2s) > 1:
            print("note: US %s is the twin of %d Lupo 2 functions (%s): copies?" % (us, len(l2s), ", ".join(l2s)))
    with open(FIXES + ".tmp", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["l2", "us", "verdict", "end", "note"])
        for r in out:
            w.writerow([r["l2"], r["us"], r["verdict"], r["end"], r["note"]])
    os.replace(FIXES + ".tmp", FIXES)
    print("%s: %d rows (%s)" % (FIXES, len(out), ", ".join("%s %d" % kv for kv in
                                                           collections.Counter(r["verdict"] for r in out).items())))
    for a, rs in clash:
        print("disagree, left out: L2 %s: %s" % (a, "; ".join("%s %s %s (%s)" % (r["verdict"], r["us"], r["note"][:40],
                                                                               r["src"]) for r in rs)))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd")
    sub.add_parser("map")
    sub.add_parser("classes")
    p = sub.add_parser("class"); p.add_argument("name")
    p = sub.add_parser("dis"); p.add_argument("addr"); p.add_argument("--us", action="store_true")
    p = sub.add_parser("who"); p.add_argument("addr"); p.add_argument("--us", action="store_true")
    p = sub.add_parser("cmp"); p.add_argument("addr")
    sub.add_parser("summary")
    p = sub.add_parser("changed"); p.add_argument("--area", default=""); p.add_argument("--min", type=float, default=0.3)
    p.add_argument("--plausible", action="store_true", help="only pairs whose sizes are within 2x")
    p.add_argument("--limit", type=int, default=200)
    p = sub.add_parser("unmatched"); p.add_argument("--near"); p.add_argument("--limit", type=int, default=300)
    p.add_argument("--area", default="")
    p = sub.add_parser("fixes", help="merge hand-made pairs_fix.csv files into ref/lupo2/pairs_fix.csv")
    p.add_argument("files", nargs="+")
    a = ap.parse_args()
    if a.cmd == "map":
        m = build_map()
        c = collections.Counter(v["how"] for v in m.values())
        print("paired %d Lupo 2 functions of %d (%s)" % (len(m), len(functions("L2")),
                                                        ", ".join("%s %d" % kv for kv in c.items())))
    elif a.cmd == "classes":
        cmd_classes()
    elif a.cmd == "class":
        cmd_class(a.name)
    elif a.cmd == "dis":
        cmd_dis(int(a.addr, 16), "US" if a.us else "L2")
    elif a.cmd == "who":
        x = int(a.addr, 16)
        if a.us:
            r = reverse_map().get(x)
            print("US %08X %s = %s" % (x, us_names().get(x, ("", ""))[1], ("L2 %08X" % r) if r else "no Lupo 2 twin"))
        else:
            print(describe_l2(x))
    elif a.cmd == "cmp":
        cmd_cmp(int(a.addr, 16))
    elif a.cmd == "changed":
        cmd_changed(a.area, a.min, a.limit, a.plausible)
    elif a.cmd == "unmatched":
        cmd_unmatched(int(a.near, 16) if a.near else None, a.limit, a.area)
    elif a.cmd == "summary":
        cmd_summary()
    elif a.cmd == "fixes":
        cmd_fixes(a.files)
    else:
        ap.print_help()


if __name__ == "__main__":
    main()
