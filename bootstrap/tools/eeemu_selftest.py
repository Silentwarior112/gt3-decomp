"""eeemu_selftest.py - checks the instructions tools/eeemu.py adds to PDIFF_emu.

    python tools/eeemu_selftest.py [--n N] [--quick]

1. Decode tables.  Every MMI / SA encoding eeemu implements is disassembled with
   rabbitizer (R5900 mode, an independent decoder) and the mnemonic must match
   eeemu's table; every MMI encoding eeemu leaves out must be one rabbitizer calls
   INVALID too (so nothing that exists is silently missing).
2. Semantics.  An independent reference model of each instruction, written here
   from the EE Core Instruction Set Manual in a different style (byte strings and
   int.from_bytes lanes, one explicit rule per instruction), is run against
   eeemu's decoded instruction on random and edge-case register states (GPRs,
   128-bit HI/LO, SA): rd (128 bits, or 64 with the upper half preserved), HI, LO
   and SA must agree bit for bit.  Plus hand-computed known answers.
3. Base-ISA changes: LWR/LWL, DADD/DADDI/DSUB traps, division by zero, trap
   immediates, LQ/SQ address masking.
4. In context: the retail bootstrap's MMI libc routines (strlen, memset, memcpy,
   memmove, memcmp, strcmp, strcpy, strcat, strncpy) run in eeemu on random
   buffers at every alignment and must give the C-library answer.  The retail code
   is known-good, so this checks pceqb, ppac5, ppach, plzcw, pcpyld, pcpyud, pcpyh,
   por, pand, pnor, psubb, psubw, pxor and lq/sq where the bootstrap uses them.

Exit status 0 = all passed.  tools/difftest.py runs the quick form first.
"""
import argparse, os, random, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import eeemu as E
import paths

RETAIL = paths.RETAIL_ELF
RETAIL_GP = 0x01049CF0
M64, M128 = (1 << 64) - 1, (1 << 128) - 1

# ------------------------------------------------------------------ reference model
# 128-bit values as 16 little-endian bytes; lanes via int.from_bytes.


def B(v):
    return (v & M128).to_bytes(16, "little")


def L(v, w, signed=False):
    """lanes of width w bits, lane 0 first."""
    n = w // 8
    b = B(v)
    return [int.from_bytes(b[i:i + n], "little", signed=signed) for i in range(0, 16, n)]


def J(ls, w):
    n = w // 8
    return int.from_bytes(b"".join((x % (1 << w)).to_bytes(n, "little") for x in ls), "little")


def clamp(x, w, signed):
    if signed:
        lo, hi = -(1 << (w - 1)), (1 << (w - 1)) - 1
    else:
        lo, hi = 0, (1 << w) - 1
    return max(lo, min(hi, x))


def sext(x, w):
    x %= 1 << w
    return x - (1 << w) if x >> (w - 1) else x


def tdiv(a, b):
    """C-style truncating division."""
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


class St:
    """register state visible to one MMI instruction."""

    def __init__(self, rs, rt, rd, hi, lo, sa):
        self.rs, self.rt, self.rd, self.hi, self.lo, self.sa = rs, rt, rd, hi, lo, sa


def lanewise(w, f, signed=False):
    return lambda s: J([f(a, b) for a, b in zip(L(s.rs, w, signed), L(s.rt, w, signed))], w)


def sat_lanewise(w, f, signed):
    return lambda s: J([clamp(f(a, b), w, signed) for a, b in zip(L(s.rs, w, signed), L(s.rt, w, signed))], w)


def cmp_lanewise(w, pred, signed):
    return lambda s: J([(1 << w) - 1 if pred(a, b) else 0 for a, b in zip(L(s.rs, w, signed), L(s.rt, w, signed))], w)


def interleave(w, lo_half):
    def f(s):
        a, b = L(s.rs, w), L(s.rt, w)
        n = len(a) // 2
        base = 0 if lo_half else n
        out = []
        for i in range(n):
            out.append(b[base + i])
            out.append(a[base + i])
        return J(out, w)
    return f


def packeven(w):
    return lambda s: J(L(s.rt, w)[0::2] + L(s.rs, w)[0::2], w)


def ppac5(s):
    out = []
    for x in L(s.rt, 32):
        r, g_, b_, a = (x >> 3) & 31, (x >> 11) & 31, (x >> 19) & 31, x >> 31
        out.append(r | g_ << 5 | b_ << 10 | a << 15)
    return J(out, 32)


def pext5(s):
    out = []
    for x in L(s.rt, 32):
        r, g_, b_, a = x & 31, (x >> 5) & 31, (x >> 10) & 31, (x >> 15) & 1
        out.append(r << 3 | g_ << 11 | b_ << 19 | a << 31)
    return J(out, 32)


def plzcw_ref(x):
    bits = format(x, "032b")
    n = 0
    for c in bits[1:]:
        if c != bits[0]:
            break
        n += 1
    return n


def hl_words(s):
    return L(s.lo, 32), L(s.hi, 32)


# Each entry: name -> fn(St) returning dict with any of: rd (128-bit value), rd64
# (low 64 bits only, upper half of rd preserved), hi, lo (128-bit), sa.
REF = {}


def R(name):
    def deco(f):
        REF[name] = f
        return f
    return deco


for nm, w, op in (("paddw", 32, lambda a, b: a + b), ("psubw", 32, lambda a, b: a - b),
                  ("paddh", 16, lambda a, b: a + b), ("psubh", 16, lambda a, b: a - b),
                  ("paddb", 8, lambda a, b: a + b), ("psubb", 8, lambda a, b: a - b)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(lanewise(w, op))
for nm, w, signed, op in (("paddsw", 32, True, lambda a, b: a + b), ("psubsw", 32, True, lambda a, b: a - b),
                          ("paddsh", 16, True, lambda a, b: a + b), ("psubsh", 16, True, lambda a, b: a - b),
                          ("paddsb", 8, True, lambda a, b: a + b), ("psubsb", 8, True, lambda a, b: a - b),
                          ("padduw", 32, False, lambda a, b: a + b), ("psubuw", 32, False, lambda a, b: a - b),
                          ("padduh", 16, False, lambda a, b: a + b), ("psubuh", 16, False, lambda a, b: a - b),
                          ("paddub", 8, False, lambda a, b: a + b), ("psubub", 8, False, lambda a, b: a - b)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(sat_lanewise(w, op, signed))
for nm, w, signed, pred in (("pcgtw", 32, True, lambda a, b: a > b), ("pcgth", 16, True, lambda a, b: a > b),
                            ("pcgtb", 8, True, lambda a, b: a > b), ("pceqw", 32, False, lambda a, b: a == b),
                            ("pceqh", 16, False, lambda a, b: a == b), ("pceqb", 8, False, lambda a, b: a == b)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(cmp_lanewise(w, pred, signed))
for nm, w, pick in (("pmaxw", 32, max), ("pmaxh", 16, max), ("pminw", 32, min), ("pminh", 16, min)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(lanewise(w, pick, True))
for nm, w, lo_half in (("pextlw", 32, True), ("pextlh", 16, True), ("pextlb", 8, True),
                       ("pextuw", 32, False), ("pextuh", 16, False), ("pextub", 8, False)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(interleave(w, lo_half))
for nm, w in (("ppacw", 32), ("ppach", 16), ("ppacb", 8)):
    REF[nm] = (lambda f: lambda s: {"rd": f(s)})(packeven(w))
REF["ppac5"] = lambda s: {"rd": ppac5(s)}
REF["pext5"] = lambda s: {"rd": pext5(s)}
REF["pabsw"] = lambda s: {"rd": J([min(abs(x), 0x7FFFFFFF) for x in L(s.rt, 32, True)], 32)}
REF["pabsh"] = lambda s: {"rd": J([min(abs(x), 0x7FFF) for x in L(s.rt, 16, True)], 16)}


@R("padsbh")
def _(s):
    a, b = L(s.rs, 16), L(s.rt, 16)
    return {"rd": J([a[i] - b[i] for i in range(4)] + [a[i] + b[i] for i in range(4, 8)], 16)}


@R("qfsrv")
def _(s):
    whole = (s.rs << 128) | s.rt
    return {"rd": (whole >> (s.sa * 8)) & M128}


@R("pand")
def _(s):
    return {"rd": s.rs & s.rt}


@R("por")
def _(s):
    return {"rd": s.rs | s.rt}


@R("pxor")
def _(s):
    return {"rd": s.rs ^ s.rt}


@R("pnor")
def _(s):
    return {"rd": M128 ^ (s.rs | s.rt)}


@R("pcpyld")
def _(s):
    return {"rd": J([L(s.rt, 64)[0], L(s.rs, 64)[0]], 64)}


@R("pcpyud")
def _(s):
    return {"rd": J([L(s.rs, 64)[1], L(s.rt, 64)[1]], 64)}


@R("pcpyh")
def _(s):
    h = L(s.rt, 16)
    return {"rd": J([h[0]] * 4 + [h[4]] * 4, 16)}


@R("pexeh")
def _(s):
    h = L(s.rt, 16)
    return {"rd": J([h[2], h[1], h[0], h[3], h[6], h[5], h[4], h[7]], 16)}


@R("prevh")
def _(s):
    h = L(s.rt, 16)
    return {"rd": J(h[0:4][::-1] + h[4:8][::-1], 16)}


@R("pexch")
def _(s):
    h = L(s.rt, 16)
    return {"rd": J([h[0], h[2], h[1], h[3], h[4], h[6], h[5], h[7]], 16)}


@R("pexew")
def _(s):
    x = L(s.rt, 32)
    return {"rd": J([x[2], x[1], x[0], x[3]], 32)}


@R("pexcw")
def _(s):
    x = L(s.rt, 32)
    return {"rd": J([x[0], x[2], x[1], x[3]], 32)}


@R("prot3w")
def _(s):
    x = L(s.rt, 32)
    return {"rd": J([x[1], x[2], x[0], x[3]], 32)}


@R("pinth")
def _(s):
    a, b = L(s.rs, 16), L(s.rt, 16)
    return {"rd": J([b[0], a[4], b[1], a[5], b[2], a[6], b[3], a[7]], 16)}


@R("pinteh")
def _(s):
    a, b = L(s.rs, 16), L(s.rt, 16)
    return {"rd": J([b[0], a[0], b[2], a[2], b[4], a[4], b[6], a[6]], 16)}


def _vsh(kind):
    def f(s):
        a, b = L(s.rs, 32), L(s.rt, 32)
        out = []
        for i in (0, 2):
            n = a[i] % 32
            if kind == "l":
                r = (b[i] * 2 ** n) % 2 ** 32
            elif kind == "r":
                r = b[i] // 2 ** n
            else:
                r = sext(b[i], 32) // 2 ** n
            out.append(sext(r, 32))
        return {"rd": J(out, 64)}
    return f


REF["psllvw"], REF["psrlvw"], REF["psravw"] = _vsh("l"), _vsh("r"), _vsh("a")


def _imm_shift(w, kind):
    def f(s):
        n = s.sa_field % 16 if w == 16 else s.sa_field
        out = []
        for x in L(s.rt, w):
            if kind == "l":
                out.append(x * 2 ** n)
            elif kind == "r":
                out.append(x // 2 ** n)
            else:
                out.append(sext(x, w) // 2 ** n)
        return {"rd": J(out, w)}
    return f


for nm, w, kind in (("psllh", 16, "l"), ("psrlh", 16, "r"), ("psrah", 16, "a"),
                    ("psllw", 32, "l"), ("psrlw", 32, "r"), ("psraw", 32, "a")):
    REF[nm] = _imm_shift(w, kind)


@R("plzcw")
def _(s):
    x = L(s.rs, 32)
    return {"rd64": plzcw_ref(x[0]) | plzcw_ref(x[1]) << 32}


def _madd(signed, pipe):
    def f(s):
        k = 1 if pipe else 0            # 64-bit lane of HI/LO
        hi, lo = L(s.hi, 64), L(s.lo, 64)
        acc = (hi[k] % 2 ** 32) * 2 ** 32 + lo[k] % 2 ** 32
        a, b = L(s.rs, 32)[0], L(s.rt, 32)[0]
        if signed:
            acc = sext(acc, 64) + sext(a, 32) * sext(b, 32)
        else:
            acc = acc + a * b
        acc %= 2 ** 64
        nlo, nhi = sext(acc % 2 ** 32, 32) % 2 ** 64, sext(acc // 2 ** 32, 32) % 2 ** 64
        lo[k], hi[k] = nlo, nhi
        return {"rd64": nlo, "hi": J(hi, 64), "lo": J(lo, 64)}
    return f


REF["madd"], REF["maddu"], REF["madd1"], REF["maddu1"] = _madd(True, 0), _madd(False, 0), _madd(True, 1), _madd(False, 1)


def _mult1(signed):
    def f(s):
        a, b = L(s.rs, 32)[0], L(s.rt, 32)[0]
        p = sext(a, 32) * sext(b, 32) if signed else a * b
        p %= 2 ** 64
        hi, lo = L(s.hi, 64), L(s.lo, 64)
        lo[1], hi[1] = sext(p % 2 ** 32, 32) % 2 ** 64, sext(p // 2 ** 32, 32) % 2 ** 64
        return {"rd64": lo[1], "hi": J(hi, 64), "lo": J(lo, 64)}
    return f


REF["mult1"], REF["multu1"] = _mult1(True), _mult1(False)


def _div1(signed):
    def f(s):
        a, b = L(s.rs, 32)[0], L(s.rt, 32)[0]
        if signed:
            a, b = sext(a, 32), sext(b, 32)
        if b == 0:
            return {"error": "division by zero"}
        q = tdiv(a, b)
        r = a - q * b
        hi, lo = L(s.hi, 64), L(s.lo, 64)
        lo[1], hi[1] = sext(q, 32) % 2 ** 64, sext(r, 32) % 2 ** 64
        return {"hi": J(hi, 64), "lo": J(lo, 64)}
    return f


REF["div1"], REF["divu1"] = _div1(True), _div1(False)
REF["mfhi1"] = lambda s: {"rd64": L(s.hi, 64)[1]}
REF["mflo1"] = lambda s: {"rd64": L(s.lo, 64)[1]}
REF["mthi1"] = lambda s: {"hi": J([L(s.hi, 64)[0], L(s.rs, 64)[0]], 64)}
REF["mtlo1"] = lambda s: {"lo": J([L(s.lo, 64)[0], L(s.rs, 64)[0]], 64)}
REF["pmfhi"] = lambda s: {"rd": s.hi}
REF["pmflo"] = lambda s: {"rd": s.lo}
REF["pmthi"] = lambda s: {"hi": s.rs}
REF["pmtlo"] = lambda s: {"lo": s.rs}


def _pmfhl(fmt):
    def f(s):
        lo, hi = hl_words(s)
        if fmt == "lw":
            return {"rd": J([lo[0], hi[0], lo[2], hi[2]], 32)}
        if fmt == "uw":
            return {"rd": J([lo[1], hi[1], lo[3], hi[3]], 32)}
        if fmt == "slw":
            out = []
            for i in (0, 2):
                v = sext(hi[i] * 2 ** 32 + lo[i], 64)
                out.append(clamp(v, 32, True))
            return {"rd": J(out, 64)}
        lh, hh = L(s.lo, 16), L(s.hi, 16)
        if fmt == "lh":
            return {"rd": J([lh[0], lh[2], hh[0], hh[2], lh[4], lh[6], hh[4], hh[6]], 16)}
        ls, hs = L(s.lo, 32, True), L(s.hi, 32, True)
        return {"rd": J([clamp(x, 16, True) for x in (ls[0], ls[1], hs[0], hs[1], ls[2], ls[3], hs[2], hs[3])], 16)}
    return f


for fmt in ("lw", "uw", "slw", "lh", "sh"):
    REF["pmfhl." + fmt] = _pmfhl(fmt)


@R("pmthl.lw")
def _(s):
    x = L(s.rs, 32)
    lo, hi = hl_words(s)
    lo[0], hi[0], lo[2], hi[2] = x[0], x[1], x[2], x[3]
    return {"lo": J(lo, 32), "hi": J(hi, 32)}


def _pmulw(signed, mode):
    def f(s):
        a, b = L(s.rs, 32), L(s.rt, 32)
        hi, lo = L(s.hi, 64), L(s.lo, 64)
        out = []
        for k, i in ((0, 0), (1, 2)):
            p = sext(a[i], 32) * sext(b[i], 32) if signed else a[i] * b[i]
            if mode:
                acc = (hi[k] % 2 ** 32) * 2 ** 32 + lo[k] % 2 ** 32
                if signed:
                    acc = sext(acc, 64)
                p = acc + p if mode > 0 else acc - p
            p %= 2 ** 64
            lo[k] = sext(p % 2 ** 32, 32) % 2 ** 64
            hi[k] = sext(p // 2 ** 32, 32) % 2 ** 64
            out.append(p)
        return {"rd": J(out, 64), "hi": J(hi, 64), "lo": J(lo, 64)}
    return f


REF["pmultw"], REF["pmaddw"], REF["pmsubw"] = _pmulw(True, 0), _pmulw(True, 1), _pmulw(True, -1)
REF["pmultuw"], REF["pmadduw"] = _pmulw(False, 0), _pmulw(False, 1)


def _pdivw(signed):
    def f(s):
        a, b = L(s.rs, 32, signed), L(s.rt, 32, signed)
        hi, lo = L(s.hi, 64), L(s.lo, 64)
        for k, i in ((0, 0), (1, 2)):
            if b[i] == 0:
                return {"error": "division by zero"}
            q = tdiv(a[i], b[i])
            lo[k] = sext(q, 32) % 2 ** 64
            hi[k] = sext(a[i] - q * b[i], 32) % 2 ** 64
        return {"hi": J(hi, 64), "lo": J(lo, 64)}
    return f


REF["pdivw"], REF["pdivuw"] = _pdivw(True), _pdivw(False)


@R("pdivbw")
def _(s):
    d = L(s.rt, 16, True)[0]
    if d == 0:
        return {"error": "division by zero"}
    qs, rs_ = [], []
    for x in L(s.rs, 32, True):
        q = tdiv(x, d)
        qs.append(q)
        rs_.append(sext(x - q * d, 16))
    return {"lo": J(qs, 32), "hi": J(rs_, 32)}


def _pmulh(mode):
    def f(s):
        a, b = L(s.rs, 16, True), L(s.rt, 16, True)
        lo, hi = hl_words(s)
        slots = [("lo", 0), ("lo", 1), ("hi", 0), ("hi", 1), ("lo", 2), ("lo", 3), ("hi", 2), ("hi", 3)]
        regs = {"lo": lo, "hi": hi}
        for i, (r, j) in enumerate(slots):
            p = a[i] * b[i]
            if mode:
                p = sext(regs[r][j], 32) + (p if mode > 0 else -p)
            regs[r][j] = p % 2 ** 32
        return {"rd": J([lo[0], hi[0], lo[2], hi[2]], 32), "lo": J(lo, 32), "hi": J(hi, 32)}
    return f


REF["pmulth"], REF["pmaddh"], REF["pmsubh"] = _pmulh(0), _pmulh(1), _pmulh(-1)


def _phm(sign):
    def f(s):
        a, b = L(s.rs, 16, True), L(s.rt, 16, True)
        r = []
        for i in range(4):
            x = a[2 * i + 1] * b[2 * i + 1]
            y = a[2 * i] * b[2 * i]
            r.append((x + y) if sign > 0 else (x - y))
        lo, hi = hl_words(s)
        lo[0], hi[0], lo[2], hi[2] = r[0] % 2 ** 32, r[1] % 2 ** 32, r[2] % 2 ** 32, r[3] % 2 ** 32
        return {"rd": J(r, 32), "lo": J(lo, 32), "hi": J(hi, 32)}
    return f


REF["phmadh"], REF["phmsbh"] = _phm(1), _phm(-1)

# ------------------------------------------------------------------ encodings

ALL = {}        # name -> (funct, sub, uses) ; uses: which of rs/rt/rd fields the encoding has
for sub, nm in E.MMI0_NAMES.items():
    ALL[nm] = ("mmi", 0x08, sub)
for sub, nm in E.MMI1_NAMES.items():
    ALL[nm] = ("mmi", 0x28, sub)
for sub, nm in E.MMI2_NAMES.items():
    ALL[nm] = ("mmi", 0x09, sub)
for sub, nm in E.MMI3_NAMES.items():
    ALL[nm] = ("mmi", 0x29, sub)
for fn, nm in E.MMI_BASE_NAMES.items():
    if nm in ("pmfhl", "pmthl"):
        continue
    ALL[nm] = ("mmi", fn, None)
for k, fmt in enumerate(("lw", "uw", "slw", "lh", "sh")):
    ALL["pmfhl." + fmt] = ("mmi", 0x30, k)
ALL["pmthl.lw"] = ("mmi", 0x31, 0)

RT_ONLY = {"pabsw", "pabsh", "pext5", "ppac5", "pexeh", "prevh", "pexch", "pexew", "pexcw", "prot3w", "pcpyh",
           "psllh", "psrlh", "psrah", "psllw", "psrlw", "psraw"}
RS_ONLY = {"mthi1", "mtlo1", "pmthi", "pmtlo", "pmthl.lw"}
RS_RD = {"plzcw"}
RD_ONLY = {"mfhi1", "mflo1", "pmfhi", "pmflo", "pmfhl.lw", "pmfhl.uw", "pmfhl.slw", "pmfhl.lh", "pmfhl.sh"}
NO_RD = {"div1", "divu1", "pdivw", "pdivuw", "pdivbw", "mthi1", "mtlo1", "pmthi", "pmtlo", "pmthl.lw"}
IMM_SHIFT = {"psllh", "psrlh", "psrah", "psllw", "psrlw", "psraw"}


def encode(name, rs, rt, rd, shamt=0):
    kind, fn, sub = ALL[name]
    if name in RT_ONLY:
        rs = 0
    if name in RS_ONLY:
        rt, rd = 0, 0
    if name in RS_RD:
        rt = 0
    if name in RD_ONLY:
        rs, rt = 0, 0
    if name in NO_RD:
        rd = 0
    sa = sub if sub is not None else 0
    if name in IMM_SHIFT:
        sa = shamt
    return (0x1C << 26) | (rs << 21) | (rt << 16) | (rd << 11) | (sa << 6) | fn, rs, rt, rd

# ------------------------------------------------------------------ 1. decode cross-check


def check_decode(log):
    try:
        import rabbitizer
    except ImportError:
        log("decode: rabbitizer not installed - skipped")
        return 0
    Rz = rabbitizer

    def name_of(w):
        i = Rz.Instruction(w, vram=0x01000000, category=Rz.InstrCategory.R5900)
        return i.getOpcodeName().lower(), i.isValid()

    bad = 0
    for nm in sorted(ALL):
        w = encode(nm, 5, 6, 7, 3)[0]
        rn, ok = name_of(w)
        if rn != nm or not ok:
            bad += 1
            log("decode MISMATCH %s: %08X is %s (valid %s) in rabbitizer" % (nm, w, rn, ok))
    # MMI encodings eeemu does not implement must be invalid for rabbitizer too
    mem = E.Mem.__new__(E.Mem)
    cpu = E.CPU(mem)
    missing = 0
    for fn in range(64):
        subs = range(32) if fn in (0x08, 0x09, 0x28, 0x29, 0x30, 0x31) else [0]
        for sub in subs:
            w = (0x1C << 26) | (5 << 21) | (6 << 16) | (7 << 11) | (sub << 6) | fn
            try:
                cpu.decode(0x01000000, w)
                ours = True
            except E.EmuError:
                ours = False
            rn, ok = name_of(w)
            if ok and rn != "invalid" and not ours:
                # the base functions are defined with sa = 0 fields; rabbitizer
                # flags those itself
                missing += 1
                log("decode: %08X (%s) valid in rabbitizer but not implemented" % (w, rn))
    for w, want in (((0 << 26) | (7 << 11) | 0x28, "mfsa"), ((0 << 26) | (5 << 21) | 0x29, "mtsa"),
                    ((1 << 26) | (5 << 21) | (0x18 << 16) | 3, "mtsab"),
                    ((1 << 26) | (5 << 21) | (0x19 << 16) | 3, "mtsah")):
        rn, ok = name_of(w)
        if rn != want:
            bad += 1
            log("decode MISMATCH %s: %08X is %s" % (want, w, rn))
    log("decode: %d MMI mnemonics + 4 SA instructions checked against rabbitizer, %d mismatches, %d missing"
        % (len(ALL), bad, missing))
    return bad + missing

# ------------------------------------------------------------------ 2. semantics


EDGE32 = [0, 1, 2, 0x7F, 0x80, 0xFF, 0x7FFF, 0x8000, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000, 0x80000001,
          0xFFFFFFFF, 0xFFFFFFFE, 0x00FF00FF, 0x7F7F7F7F, 0x80808080, 0x01010101, 0xFEFEFEFE]


def rnd128(rng):
    k = rng.random()
    if k < 0.3:
        return int.from_bytes(b"".join(rng.choice(EDGE32).to_bytes(4, "little") for _ in range(4)), "little")
    if k < 0.45:
        return int.from_bytes(bytes(rng.choice((0, 1, 0x7F, 0x80, 0xFF, 0xFE, 0x01)) for _ in range(16)), "little")
    if k < 0.55:
        return int.from_bytes(b"".join(rng.choice((0, 1, 0x7FFF, 0x8000, 0xFFFF, 0x8001)).to_bytes(2, "little")
                                       for _ in range(8)), "little")
    return rng.getrandbits(128)


def run_one(name, w, rs, rt, rd, st):
    """execute the encoded instruction in eeemu; returns dict like the reference."""
    mem = E.Mem.__new__(E.Mem)
    cpu = E.CPU(mem)
    cpu.reset_regs()
    cpu.setq(rs, st.rs)
    if rt != rs:
        cpu.setq(rt, st.rt)
    if rd not in (rs, rt):
        cpu.setq(rd, st.rd)
    cpu.set_hi128(st.hi)
    cpu.set_lo128(st.lo)
    cpu.sa = st.sa
    try:
        cpu.decode(0x01000000, w)()
    except E.EmuError as e:
        return {"error": str(e)}
    return {"rdq": cpu.q(rd), "hi": cpu.hi128(), "lo": cpu.lo128(), "sa": cpu.sa}


def compare(name, ref, got, st, rd):
    if "error" in ref or "error" in got:
        return ("error" in ref) == ("error" in got)
    want_rd = st.rd
    if "rd" in ref:
        want_rd = ref["rd"] & M128
    elif "rd64" in ref:
        want_rd = (st.rd & ~M64 & M128) | (ref["rd64"] & M64)
    if rd == 0:
        want_rd = 0
    if got["rdq"] != want_rd:
        return False
    if got["hi"] != ref.get("hi", st.hi) & M128 or got["lo"] != ref.get("lo", st.lo) & M128:
        return False
    return got["sa"] == st.sa


def check_semantics(n, log, rng):
    bad = 0
    total = 0
    for name in sorted(ALL):
        ref_fn = REF.get(name)
        if ref_fn is None:
            log("semantics: no reference for %s" % name)
            bad += 1
            continue
        for k in range(n):
            # distinct registers normally; sometimes rd aliases a source
            regs = rng.sample(range(1, 32), 3)
            if k % 7 == 3:
                regs[2] = regs[0]
            if k % 11 == 5:
                regs[1] = regs[0]
            shamt = rng.randrange(32)
            w, rs, rt, rd = encode(name, regs[0], regs[1], regs[2], shamt)
            st = St(rnd128(rng), rnd128(rng), rnd128(rng), rnd128(rng), rnd128(rng), rng.randrange(16))
            if name.startswith("pdiv") or name in ("div1", "divu1"):
                if k % 5:        # mostly non-zero divisors, some edge cases
                    st.rt = st.rt | int.from_bytes(bytes([1, 0, 0, 0] * 4), "little")
                    st.rs = st.rs | int.from_bytes(bytes([1, 0, 0, 0] * 4), "little")
            # register aliasing: the instruction sees one value per register
            if rs == 0:
                st.rs = 0
            if rt == 0:
                st.rt = 0
            if rs == rt:
                st.rt = st.rs
            if rd == rs and rd:
                st.rd = st.rs
            elif rd == rt and rd:
                st.rd = st.rt
            st.sa_field = shamt
            ref = ref_fn(st)
            got = run_one(name, w, rs, rt, rd, st)
            total += 1
            if not compare(name, ref, got, st, rd):
                bad += 1
                if bad <= 12:
                    log("semantics MISMATCH %s (%08X rs=$%d rt=$%d rd=$%d)\n  rs=%032X rt=%032X rd=%032X\n"
                        "  hi=%032X lo=%032X sa=%d\n  ref=%s\n  got=%s" % (
                            name, w, rs, rt, rd, st.rs, st.rt, st.rd, st.hi, st.lo, st.sa,
                            {k2: ("%X" % v if isinstance(v, int) else v) for k2, v in ref.items()},
                            {k2: ("%X" % v if isinstance(v, int) else v) for k2, v in got.items()}))
    log("semantics: %d instructions x %d random states (%d runs) against the reference model, %d mismatches"
        % (len(ALL), n, total, bad))
    return bad


def known_answers(log):
    """hand-computed results (independent of both models above)."""
    bad = 0
    rs = 0x33333333_22222222_11111111_00000000 | 0x0F0E0D0C_0B0A0908_07060504_03020100
    rt = 0xF3F3F3F3_E2E2E2E2_D1D1D1D1_C0C0C0C0

    def one(name, s, t, want, hi=0, lo=0, sa=0, shamt=0, want_hi=None, want_lo=None):
        nonlocal bad
        w, a, b, d = encode(name, 1, 2, 3, shamt)
        st = St(s, t, 0, hi, lo, sa)
        got = run_one(name, w, a, b, d, st)
        ok = got.get("rdq") == want if want is not None else True
        if want_hi is not None:
            ok = ok and got.get("hi") == want_hi
        if want_lo is not None:
            ok = ok and got.get("lo") == want_lo
        if not ok:
            bad += 1
            log("known answer MISMATCH %s: got %s" % (name, {k: "%X" % v for k, v in got.items() if isinstance(v, int)}))

    one("pextlw", 0x44444444_33333333_22222222_11111111, 0xDDDDDDDD_CCCCCCCC_BBBBBBBB_AAAAAAAA,
        0x22222222_BBBBBBBB_11111111_AAAAAAAA)
    one("pextuw", 0x44444444_33333333_22222222_11111111, 0xDDDDDDDD_CCCCCCCC_BBBBBBBB_AAAAAAAA,
        0x44444444_DDDDDDDD_33333333_CCCCCCCC)
    one("ppacw", 0x44444444_33333333_22222222_11111111, 0xDDDDDDDD_CCCCCCCC_BBBBBBBB_AAAAAAAA,
        0x33333333_11111111_CCCCCCCC_AAAAAAAA)
    one("pextlb", 0x0F0E0D0C_0B0A0908_07060504_03020100, 0x1F1E1D1C_1B1A1918_17161514_13121110,
        0x07170616_05150414_03130212_01110010)
    one("ppacb", 0x0F0E0D0C_0B0A0908_07060504_03020100, 0x1F1E1D1C_1B1A1918_17161514_13121110,
        0x0E0C0A08_06040200_1E1C1A18_16141210)
    one("pcpyld", 0xAAAAAAAA_AAAAAAAA_11111111_11111111, 0xBBBBBBBB_BBBBBBBB_22222222_22222222,
        0x11111111_11111111_22222222_22222222)
    one("pcpyud", 0xAAAAAAAA_AAAAAAAA_11111111_11111111, 0xBBBBBBBB_BBBBBBBB_22222222_22222222,
        0xBBBBBBBB_BBBBBBBB_AAAAAAAA_AAAAAAAA)
    one("pcpyh", 0, 0x7777_6666_5555_4444_3333_2222_1111_0000 + 0x1, 0x4444_4444_4444_4444_0001_0001_0001_0001)
    one("pceqb", 0x00FF00FF_00000000_12345678_9ABCDEF0, 0,
        0xFF00FF00_FFFFFFFF_00000000_00000000)
    one("ppac5", 0, 0x80FFFFFF_00F8F8F8_00000000_7F081018,
        0x0000FFFF_00007FFF_00000000_00000443)
    one("pext5", 0, 0x0000FFFF_00007FFF_00000000_00000443,
        0x80F8F8F8_00F8F8F8_00000000_00081018)
    one("paddsw", 0x7FFFFFFF_80000000_00000001_FFFFFFFF, 0x00000001_FFFFFFFF_7FFFFFFF_00000001,
        0x7FFFFFFF_80000000_7FFFFFFF_00000000)
    one("psubuw", 0x00000001_00000005_FFFFFFFF_00000000, 0x00000002_00000003_00000001_00000001,
        0x00000000_00000002_FFFFFFFE_00000000)
    one("pabsw", 0, 0x80000000_7FFFFFFF_FFFFFFFF_80000001, 0x7FFFFFFF_7FFFFFFF_00000001_7FFFFFFF)
    one("pabsh", 0, 0x8000_7FFF_FFFF_8001_0000_0001_FFFE_1234, 0x7FFF_7FFF_0001_7FFF_0000_0001_0002_1234)
    one("psravw", 0x00000000_00000004_00000000_0000001F, 0x00000000_F0000000_00000000_80000000,
        0xFFFFFFFF_FF000000_FFFFFFFF_FFFFFFFF)
    one("psllvw", 0x00000000_00000001_00000000_00000004, 0x00000000_40000000_00000000_0FFFFFFF,
        0xFFFFFFFF_80000000_FFFFFFFF_FFFFFFF0)
    # plzcw: words 0x00000001 -> 30, 0xFFFFFFFE -> 30 ; upper 64 of rd preserved (was 0)
    one("plzcw", 0xFFFFFFFE_00000001, 0, 0x0000001E_0000001E)
    one("plzcw", 0x80000000_40000000, 0, 0x00000000_00000000)
    one("plzcw", 0xFFFFFFFF_00000000, 0, 0x0000001F_0000001F)
    # madd: HI:LO = 0x00000001:0xFFFFFFFF (= 0x1FFFFFFFF) + 2*3 = 0x200000005
    one("madd", 2, 3, 0x00000000_00000005, hi=1, lo=0xFFFFFFFF, want_hi=2, want_lo=5)
    one("madd", 0xFFFFFFFF, 1, 0xFFFFFFFF_FFFFFFFF, hi=0, lo=0, want_hi=M64, want_lo=M64)   # 0 + (-1)*1
    one("maddu", 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000_00000001, hi=0, lo=0,
        want_hi=0xFFFFFFFF_FFFFFFFE, want_lo=1)                                                   # 0xFFFFFFFE00000001
    # pmulth: products 1*2 .. 8*9 (halfwords 1..8 x 2..9)
    s16 = 0x0008_0007_0006_0005_0004_0003_0002_0001
    t16 = 0x0009_0008_0007_0006_0005_0004_0003_0002
    one("pmulth", s16, t16, (56 << 96) | (30 << 64) | (12 << 32) | 2,
        want_lo=(42 << 96) | (30 << 64) | (6 << 32) | 2, want_hi=(72 << 96) | (56 << 64) | (20 << 32) | 12)
    # qfsrv with SA = 3 bytes
    one("qfsrv", 0x0F0E0D0C_0B0A0908_07060504_03020100, 0x1F1E1D1C_1B1A1918_17161514_13121110,
        0x0201001F_1E1D1C1B_1A191817_16151413, sa=3)
    one("pmfhl.slw", 0, 0, 0xFFFFFFFF_80000000_00000000_7FFFFFFF,
        hi=(0xFFFFFFFF << 64) | 0x00000001, lo=(0x7FFFFFFF << 64) | 0x00000000)
    log("known answers: %s" % ("all match" if not bad else "%d mismatches" % bad))
    return bad

# ------------------------------------------------------------------ 3. base ISA changes


def check_base(log):
    bad = 0
    mem = E.Mem.__new__(E.Mem)
    mem.m = bytearray(E.MEMSIZE)
    mem.orig = {}
    mem.segs = []
    cpu = E.CPU(mem)

    def ins(w):
        return cpu.decode(0x01000000, w)

    def expect(cond, what):
        nonlocal bad
        if not cond:
            bad += 1
            log("base MISMATCH: " + what)

    mem.wr(0x100000, bytes(range(0x80, 0x90)))
    # lwr $2, k($4): k = 1..3 keeps bits 63..32, k = 0 sign-extends
    for k in range(4):
        cpu.reset_regs()
        cpu.g[4] = 0x100000
        cpu.g[2] = 0x1122334455667788
        ins((0x26 << 26) | (4 << 21) | (2 << 16) | k)()
        word = struct.unpack_from("<I", mem.m, 0x100000)[0]
        low = ((0x55667788 & ~(0xFFFFFFFF >> (8 * k))) | (word >> (8 * k))) & 0xFFFFFFFF
        want = E.sext32(low) if k == 0 else (0x1122334400000000 | low)
        expect(cpu.g[2] == want, "lwr k=%d: %016X want %016X" % (k, cpu.g[2], want))
    # lwl always sign-extends (bit 31 comes from memory)
    for k in range(4):
        cpu.reset_regs()
        cpu.g[4] = 0x100000
        cpu.g[2] = 0x1122334455667788
        ins((0x22 << 26) | (4 << 21) | (2 << 16) | k)()
        word = struct.unpack_from("<I", mem.m, 0x100000)[0]
        sh = 8 * (3 - k)
        low = ((word << sh) & 0xFFFFFFFF) | (0x55667788 & ((1 << sh) - 1))
        expect(cpu.g[2] == E.sext32(low), "lwl k=%d" % k)
    # unaligned word via lwl+lwr at every offset
    for off in range(4):
        cpu.reset_regs()
        cpu.g[4] = 0x100000 + off
        ins((0x22 << 26) | (4 << 21) | (2 << 16) | 3)()
        ins((0x26 << 26) | (4 << 21) | (2 << 16) | 0)()
        want = E.sext32(int.from_bytes(bytes(mem.m[0x100000 + off:0x100004 + off]), "little"))
        expect(cpu.g[2] == want, "lwl/lwr pair at +%d" % off)
    # dadd / dsub / daddi overflow traps, daddu / dsubu / daddiu wrap
    for w, a, b, trap in (((0 << 26) | (4 << 21) | (5 << 16) | (2 << 11) | 0x2C, 1 << 62, 1 << 62, True),
                          ((0 << 26) | (4 << 21) | (5 << 16) | (2 << 11) | 0x2C, 1 << 62, (1 << 62) - 1, False),
                          ((0 << 26) | (4 << 21) | (5 << 16) | (2 << 11) | 0x2E, 1 << 63, 1, True),
                          ((0 << 26) | (4 << 21) | (5 << 16) | (2 << 11) | 0x2E, 0, 1 << 63, True),
                          ((0 << 26) | (4 << 21) | (5 << 16) | (2 << 11) | 0x2D, 1 << 62, 1 << 62, False),
                          ((0x18 << 26) | (4 << 21) | (2 << 16) | 0x0001, (1 << 63) - 1, 0, True),
                          ((0x18 << 26) | (4 << 21) | (2 << 16) | 0xFFFF, 1 << 63, 0, True),
                          ((0x19 << 26) | (4 << 21) | (2 << 16) | 0x0001, (1 << 63) - 1, 0, False)):
        cpu.reset_regs()
        cpu.g[4], cpu.g[5] = a & M64, b & M64
        try:
            ins(w)()
            got = False
        except E.EmuError:
            got = True
        expect(got == trap, "overflow trap %08X a=%X b=%X" % (w, a, b))
    # division by zero raises; 0x80000000 / -1 gives LO = 0x80000000, HI = 0
    for fn in (0x1A, 0x1B):
        cpu.reset_regs()
        cpu.g[4], cpu.g[5] = 7, 0
        try:
            ins((4 << 21) | (5 << 16) | fn)()
            got = False
        except E.EmuError:
            got = True
        expect(got, "div%s by zero raises" % ("u" if fn & 1 else ""))
    cpu.reset_regs()
    cpu.g[4], cpu.g[5] = E.sext32(0x80000000), M64
    ins((4 << 21) | (5 << 16) | 0x1A)()
    expect(cpu.lo == E.sext32(0x80000000) and cpu.hi == 0, "div 0x80000000 / -1")
    # trap immediates: teqi $4, 5 traps iff $4 == 5; tgeiu compares unsigned with sign-extended imm
    for rt_, val, imm, trap in ((12, 5, 5, True), (12, 6, 5, False), (14, 6, 5, True), (8, (-1) & M64, 0xFFFF, True),
                                (9, 5, 0xFFFF, False), (10, (-2) & M64, 0xFFFF, True), (11, 5, 0xFFFF, True)):
        cpu.reset_regs()
        cpu.g[4] = val
        try:
            ins((1 << 26) | (4 << 21) | (rt_ << 16) | imm)()
            got = False
        except E.EmuError:
            got = True
        expect(got == trap, "trap-immediate rt=%d val=%X imm=%X" % (rt_, val, imm))
    # mtsab / mtsah / mtsa / mfsa
    cpu.reset_regs()
    cpu.g[4] = 0x1234567D
    ins((1 << 26) | (4 << 21) | (0x18 << 16) | 0x0006)()
    expect(cpu.sa == (0xD ^ 0x6), "mtsab")
    ins((1 << 26) | (4 << 21) | (0x19 << 16) | 0x0001)()
    expect(cpu.sa == ((0x5 ^ 0x1) & 7) * 2, "mtsah")
    ins((0 << 26) | (2 << 11) | 0x28)()
    expect(cpu.g[2] == cpu.sa, "mfsa")
    cpu.g[4] = 9
    ins((4 << 21) | 0x29)()
    expect(cpu.sa == 9, "mtsa")
    # lq / sq ignore the low 4 address bits and move all 128 bits
    cpu.reset_regs()
    mem.wr(0x100100, bytes(range(32)))
    cpu.g[4] = 0x10010B
    ins((0x1E << 26) | (4 << 21) | (2 << 16) | 0x0010)()
    expect(cpu.q(2) == int.from_bytes(bytes(range(16, 32)), "little"), "lq masks the address")
    cpu.g[4] = 0x100207
    ins((0x1F << 26) | (4 << 21) | (2 << 16) | 0x0000)()
    expect(bytes(mem.m[0x100200:0x100210]) == bytes(range(16, 32)), "sq masks the address")
    # an ordinary 64-bit write keeps bits 127..64
    cpu.g[4] = 1
    ins((0 << 26) | (4 << 21) | (4 << 16) | (2 << 11) | 0x2D)()        # daddu $2, $4, $4
    expect(cpu.gh[2] == int.from_bytes(bytes(range(24, 32)), "little"), "64-bit op keeps the upper half")
    log("base ISA: lwl/lwr, dadd/dsub/daddi traps, division by zero, trap immediates, SA moves, lq/sq: %s"
        % ("ok" if not bad else "%d mismatches" % bad))
    return bad

# ------------------------------------------------------------------ 4. retail libc in context


def check_libc(n, log, rng):
    if not os.path.exists(RETAIL):
        log("libc oracle: %s missing - skipped" % RETAIL)
        return 0
    mem = E.Mem(RETAIL)
    cpu = E.CPU(mem, RETAIL_GP)
    base = 0x01200000
    bad = 0
    runs = 0
    F = dict(strlen=0x0100F538, memcpy=0x0101B010, memmove=0x0101B0BC, memset=0x0101B1B8, strcat=0x0101B264,
             strcmp=0x0101B380, strcpy=0x0101B4C4, strncpy=0x0101B5D8, memcmp=0x0102694C)

    def fill(a, bs):
        mem.wr(a, bs)

    def junk(k):
        return bytes(rng.randrange(1, 256) for _ in range(k))

    def s8(x):
        return x - 256 if x > 127 else x

    def sgn(x):
        return (x > 0) - (x < 0)

    for i in range(n):
        mem.reset()
        fill(base, bytes(rng.randrange(256) for _ in range(0x600)))
        a_off, b_off = rng.randrange(32), rng.randrange(32)
        ln = rng.choice([0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 47, 64, rng.randrange(200)])
        A, Bp = base + 0x40 + a_off, base + 0x300 + b_off
        s = junk(ln)
        runs += 9
        # strlen
        fill(A, s + b"\0")
        v, _, _ = cpu.call(F["strlen"], [A])
        if v != ln:
            bad += 1
            log("libc strlen(len %d @+%d) = %d" % (ln, a_off, v))
        # strcpy / strcat
        fill(Bp, b"\xAA" * 0x100)
        cpu.call(F["strcpy"], [Bp, A])
        if mem.rd(Bp, ln + 1) != s + b"\0" or mem.u8(Bp + ln + 1) != 0xAA:
            bad += 1
            log("libc strcpy len %d dst+%d src+%d" % (ln, b_off, a_off))
        t = junk(rng.randrange(40))
        fill(A + ln + 1, b"")
        fill(base + 0x500, t + b"\0")
        cpu.call(F["strcat"], [Bp, base + 0x500])
        if mem.rd(Bp, ln + len(t) + 1) != s + t + b"\0":
            bad += 1
            log("libc strcat len %d+%d dst+%d" % (ln, len(t), b_off))
        # strcmp: equal, or differing at a random position
        u = bytearray(s)
        if u and rng.random() < 0.7:
            p = rng.randrange(len(u))
            u[p] = rng.randrange(256)
            u = u[:p + 1] if u[p] == 0 else u
        u = bytes(u).split(b"\0")[0]
        fill(Bp, u + b"\0")
        v, _, _ = cpu.call(F["strcmp"], [A, Bp])
        want = 0
        for x, y in zip(s + b"\0", u + b"\0"):
            if x != y or x == 0:
                want = x - y
                break
        if E.sx32(v) != want:
            bad += 1
            log("libc strcmp %r %r = %d want %d" % (s[:20], u[:20], E.sx32(v), want))
        # strncpy
        k = rng.randrange(0, 80)
        fill(Bp, b"\xAA" * 0x100)
        cpu.call(F["strncpy"], [Bp, A, k])
        want = (s + b"\0" * 100)[:k]
        if mem.rd(Bp, k) != want or mem.u8(Bp + k) != 0xAA:
            bad += 1
            log("libc strncpy len %d n %d" % (ln, k))
        # memset / memcpy / memmove / memcmp over arbitrary bytes
        k = rng.choice([0, 1, 7, 8, 9, 16, 31, 32, 33, 63, 64, 65, rng.randrange(300)])
        c = rng.randrange(256)
        fill(Bp - 16, b"\x55" * (k + 32))
        cpu.call(F["memset"], [Bp, c, k])
        if mem.rd(Bp - 16, k + 32) != b"\x55" * 16 + bytes([c]) * k + b"\x55" * 16:
            bad += 1
            log("libc memset n %d @+%d" % (k, b_off))
        src = bytes(rng.randrange(256) for _ in range(k))
        fill(A, src)
        fill(Bp - 16, b"\x55" * (k + 32))
        cpu.call(F["memcpy"], [Bp, A, k])
        if mem.rd(Bp - 16, k + 32) != b"\x55" * 16 + src + b"\x55" * 16:
            bad += 1
            log("libc memcpy n %d dst+%d src+%d" % (k, b_off, a_off))
        # memmove with overlap in either direction
        d = rng.randrange(-40, 41)
        region = bytes(rng.randrange(256) for _ in range(k + 0x100))
        fill(base + 0x100, region)
        s0, d0 = base + 0x140 + (a_off & 15), base + 0x140 + (a_off & 15) + d
        cpu.call(F["memmove"], [d0, s0, k])
        exp = bytearray(region)
        o_s, o_d = s0 - (base + 0x100), d0 - (base + 0x100)
        exp[o_d:o_d + k] = region[o_s:o_s + k]
        if mem.rd(base + 0x100, len(region)) != bytes(exp):
            bad += 1
            log("libc memmove n %d delta %d" % (k, d))
        x = bytes(rng.randrange(256) for _ in range(k))
        y = bytearray(x)
        if k and rng.random() < 0.6:
            y[rng.randrange(k)] = rng.randrange(256)
        fill(A, x)
        fill(Bp, bytes(y))
        v, _, _ = cpu.call(F["memcmp"], [A, Bp, k])
        want = 0
        for p, q in zip(x, y):
            if p != q:
                want = p - q
                break
        if sgn(E.sx32(v)) != sgn(want) or (want and E.sx32(v) != want):
            bad += 1
            log("libc memcmp n %d = %d want %d" % (k, E.sx32(v), want))
    log("libc oracle: retail strlen/strcpy/strcat/strcmp/strncpy/memset/memcpy/memmove/memcmp, %d calls, %d wrong"
        % (runs, bad))
    return bad


def check_clut(n, log, rng):
    """retail clut_anim_channel_apply (0x0100B6D0): per colour byte k,
    dest.k = ((a.k * (s16)w + b.k * (s16)(0x1000 - w)) >> 12) & 0xFF with
    w = (int)(lerp * 4096.0f) - computed with pextlw/pextlb/pextlh/pmulth/pmaddh/
    psrlw/ppach/ppacb.  lerp = k / 4096 is exact, so the float model cannot
    matter; |k| up to 40000 also exercises the signed 16-bit truncation."""
    if not os.path.exists(RETAIL):
        return 0
    mem = E.Mem(RETAIL)
    cpu = E.CPU(mem, RETAIL_GP)
    ch, keys, dest = 0x01200000, 0x01200100, 0x01201000
    bad = 0
    for i in range(n):
        mem.reset()
        nkeys = rng.randrange(1, 5)
        ncol = rng.choice([0, 1, 2, 3, 16, rng.randrange(1, 64)])
        pals = []
        for k in range(nkeys):
            pal = [rng.getrandbits(32) for _ in range(ncol)]
            pa = 0x01202000 + 0x400 * k
            mem.wr(pa, struct.pack("<%dI" % ncol, *pal))
            mem.wr(keys + 8 * k, struct.pack("<fI", 0.0, pa))
            pals.append(pal)
        cur, nxt = rng.randrange(nkeys), rng.randrange(nkeys)
        kk = rng.choice([0, 4096, 2048, rng.randrange(4097), rng.randrange(-40000, 40000)])
        lerp = kk / 4096.0
        mem.wr(ch, struct.pack("<HHHHIIHHffhbB", 0, 0, ncol, nkeys, dest, keys, cur, nxt, 0.0, lerp, 0, 1, 1))
        mem.wr(dest, b"\xEE" * (4 * ncol + 8))
        cpu.call(0x0100B6D0, [ch])
        w = kk
        wn = (0x1000 - w)
        s16 = lambda x: ((x + 0x8000) & 0xFFFF) - 0x8000
        want = []
        for a, b in zip(pals[cur], pals[nxt]):
            out = 0
            for k in range(4):
                s = ((a >> 8 * k & 0xFF) * s16(w) + (b >> 8 * k & 0xFF) * s16(wn)) & 0xFFFFFFFF
                out |= ((s >> 12) & 0xFF) << 8 * k
            want.append(out)
        got = list(struct.unpack("<%dI" % ncol, mem.rd(dest, 4 * ncol)))
        if got != want or mem.rd(dest + 4 * ncol, 8) != b"\xEE" * 8 or mem.u8(ch + 0x1F) != 0:
            bad += 1
            if bad < 5:
                log("clut oracle: lerp %d/4096, %d colours: got %s want %s" % (kk, ncol, got[:4], want[:4]))
    log("clut oracle: retail clut_anim_channel_apply (pextlw/pextlb/pextlh/pmulth/pmaddh/psrlw/ppach/ppacb), "
        "%d calls, %d wrong" % (n, bad))
    return bad


# ------------------------------------------------------------------ 5. hardware model


def _synthetic_mem():
    mem = E.Mem.__new__(E.Mem)
    mem.m = bytearray(E.MEMSIZE + E.SPR_SIZE)
    mem.orig = {}
    mem.segs = []
    mem.io = E.IoModel(mem)
    return mem


def check_memmap(log):
    """the address map: mirrors, scratchpad, unmapped ranges, MMIO routing and the
    register model defaults, merged reads, byte-range refusal, 128-bit MMIO."""
    bad = 0

    def expect(cond, what):
        nonlocal bad
        if not cond:
            bad += 1
            log("memmap MISMATCH: " + what)

    def err_kind(f):
        try:
            f()
        except E.EmuError as e:
            return e.kind
        return None

    mem = _synthetic_mem()
    io = mem.io
    mem.w32(0x00123450, 0xCAFEF00D)
    for alias in (0x00123450, 0x80123450, 0xA0123450, 0x20123450, 0x30123450):
        expect(mem.u32(alias) == 0xCAFEF00D, "RAM mirror %08X" % alias)
    mem.w16(0x30123452, 0x1234)                                  # a store through UCAB
    expect(mem.u32(0x00123450) == 0x1234F00D, "UCAB store lands in RAM")
    mem.w32(0x70000010, 0x11111111)                               # scratchpad is its own memory
    expect(mem.u32(0x00000010) == 0 and mem.u32(0x70000010) == 0x11111111, "scratchpad separate from RAM")
    expect(mem.u64(0x70003FF8) == 0, "last scratchpad dword")
    mem.w128(0x70003FF0, (1 << 127) | 5)
    expect(mem.u128(0x70003FF0) == (1 << 127) | 5, "scratchpad qword")
    expect(E.SPR_OFF + 0x10 in mem.orig, "scratchpad writes are journalled")
    for a in (0x02000000, 0x0FFFFFFC, 0x22000000, 0x32000000, 0x40000000, 0x60000000, 0x70004000, 0xC0000000,
              0xFFFFFFFC, 0x81FFFFFE + 2, 0x13000000, 0x10010000):
        expect(err_kind(lambda: mem.u32(a)) == 'range', "%08X must be unmapped" % a)
    expect(err_kind(lambda: mem.u16(0x70003FFF + 1)) == 'range', "past the scratchpad")
    expect(err_kind(lambda: mem.rd(0x10003000, 4)) == 'mmio', "byte-range read of MMIO refused")
    expect(err_kind(lambda: mem.wr(0x12000000, b"\0" * 8)) == 'mmio', "byte-range write of MMIO refused")
    expect(err_kind(lambda: mem.cstr(0xBFC00000)) == 'mmio', "string read from ROM refused")
    expect(mem.index_addr(E.SPR_OFF + 0x20) == 0x70000020, "index_addr scratchpad")
    # MMIO: events, latch, mirrors, idle defaults
    io.reset()
    mem.w32(0x10003C10, 1)                                        # VIF1_FBRST
    expect(io.events[-1] == ('hw', ('w', 0x10003C10, 4), 1, 1), "MMIO store event")
    expect(mem.u32(0xB0003C10) == 1, "kseg1 view of a register reads the latched value")
    expect(io.events[-1] == ('hw', ('r', 0x10003C10, 4), 1, 1), "MMIO load event, physical address")
    mem.u32(0x10003C10)
    mem.u32(0x10003C10)
    expect(io.events[-1] == ('hw', ('r', 0x10003C10, 4), 1, 3) and len(io.events) == 2, "identical reads merge")
    mem.w32(0x10009000, 0x104)                                    # D1_CHCR, STR, normal mode (QWC 0)
    expect(mem.u32(0x10009000) == 0x004, "CHCR reads with STR clear")
    expect(mem.u64(0x12001000) == 0x551B4008, "GS CSR idle value")
    mem.w64(0x12001000, 8)
    expect(mem.u64(0x12001000) == 0x551B4008 and mem.u8(0x12001002) == 0x1B, "GS CSR not latched")
    expect(mem.u32(0x10003020) == 0 and mem.u32(0x1000F000) == 0, "GIF_STAT / INTC_STAT idle")
    mem.w64(0x12000090, 0x123456789A)
    expect(mem.u64(0x12000090) == 0x123456789A and mem.u16(0x12000094) == 0x12, "GS register latch, narrow view")
    n0 = len(io.events)
    mem.w128(0x10006000, 0xA5 << 120 | 0x77)                    # GIF FIFO qword
    expect(io.events[n0] == ('hw', ('w', 0x10006000, 16), 0xA5 << 120 | 0x77, 1), "128-bit MMIO store")
    expect(mem.u128(0x11004010) == 0, "VU0 data memory reads 0")
    io.model(0x10003C00, [0x1F000003, 0x3, 0])                    # VIF1_STAT busy twice, then idle
    seq = [mem.u32(0x10003C00) for _ in range(5)]
    expect(seq == [0x1F000003, 3, 0, 0, 0], "register model sequence")
    io.model(0x10003C00, lambda n, latched: n * 2)
    expect([mem.u32(0x10003C00) for _ in range(3)] == [0, 2, 4], "register model callback")
    seen = []
    io.model(0x1000F180, None, lambda v, w: seen.append((v, w)))
    mem.w8(0x1000F180, 0x41)
    expect(seen == [(0x41, 1)] and mem.u8(0x1000F180) == 0x41, "register write callback + latch")
    log("memmap: RAM mirrors, scratchpad, unmapped ranges, MMIO events / latch / idle defaults / models: %s"
        % ("ok" if not bad else "%d mismatches" % bad))
    return bad


def check_dma(log):
    """the DMA start snapshot: normal mode, a source chain through every tag id,
    the scratchpad bit, IRQ+TIE, an unmapped address."""
    bad = 0

    def expect(cond, what):
        nonlocal bad
        if not cond:
            bad += 1
            log("dma MISMATCH: " + what)

    mem = _synthetic_mem()
    io = mem.io

    def q(*w):
        return struct.pack("<4I", *w)

    def last_dma():
        e = [x for x in io.events if x[1][0] == 'dma']
        return e[-1] if e else None

    payload = bytes(range(48))
    mem.wr(0x00200000, payload)
    mem.w32(0x1000A010, 0x00200000)                               # D2 MADR
    mem.w32(0x1000A020, 3)                                        # QWC
    mem.w32(0x1000A000, 0x101)                                    # normal, from memory, STR
    e = last_dma()
    expect(e is not None and e[1] == ('dma', 2, 'normal') and e[2] == (('data', payload),), "normal-mode snapshot")
    # chain: cnt(1) -> ref(2 @payload) -> call(1, -> sub) ; sub: ret(0) ; next -> end(1)
    base = 0x00300000
    sub = 0x00310000
    endq = 0x00320000
    mem.wr(base, q(0x10000001, 0, 0x11, 0x22) + b"A" * 16 +
           q(0x30000002, 0x00200000, 0x33, 0x44) +
           q(0x50000001, sub, 0, 0) + b"B" * 16 +
           q(0x20000000, endq, 0, 0))
    mem.wr(sub, q(0x60000000, 0, 0, 0))
    mem.wr(endq, q(0x70000001, 0, 0, 0) + b"C" * 16)
    mem.w32(0x10009020, 0)
    mem.w32(0x10009030, base)                                     # D1 TADR
    mem.w32(0x10009000, 0x145)                                    # chain, TTE, from memory, STR
    want = (('tag', 0x10000001, 0, 0x11, 0x22), ('data', b"A" * 16),
            ('tag', 0x30000002, 0x00200000, 0x33, 0x44), ('data', payload[:32]),
            ('tag', 0x50000001, sub, 0, 0), ('data', b"B" * 16),
            ('tag', 0x60000000, 0, 0, 0),
            ('tag', 0x20000000, endq, 0, 0),
            ('tag', 0x70000001, 0, 0, 0), ('data', b"C" * 16), ('end', 'end'))
    e = last_dma()
    expect(e is not None and e[1] == ('dma', 1, 'chain') and e[2] == want, "chain snapshot: %r" % (e and e[2],))
    # scratchpad chain (bit 31) with an IRQ tag and TIE
    mem.wr(0x70001000, q(0x90000001, 0, 0, 0) + b"D" * 16 + q(0x70000000, 0, 0, 0))
    mem.w32(0x10008030, 0x80001000)
    mem.w32(0x10008000, 0x1C5)                                    # TIE too
    e = last_dma()
    expect(e[2] == (('tag', 0x90000001, 0, 0, 0), ('data', b"D" * 16), ('end', 'irq')), "SPR chain / IRQ stop")
    mem.w32(0x10008000, 0x145)                                    # no TIE: goes on to the end tag
    e = last_dma()
    expect(e[2][-1] == ('end', 'end') and len(e[2]) == 4, "IRQ tag without TIE")
    mem.w32(0x10008030, 0x03000000)
    mem.w32(0x10008000, 0x145)
    expect(last_dma()[2] == (('end', 'bad tag address 03000000'),), "unmapped tag address")
    mem.w32(0x10009000, 0x104)                                    # VIF1 DIR = 0: to memory
    expect(last_dma()[1] == ('dma', 1, 'to-memory'), "peripheral -> memory start")
    log("dma: normal / chain (cnt ref call ret next end) / scratchpad / IRQ+TIE / bad address snapshots: %s"
        % ("ok" if not bad else "%d mismatches" % bad))
    return bad


def check_cop(log):
    """COP0 (di/ei/Status, Count, perf counters, eret, cache tags, sync) and COP2
    (decode cross-check with rabbitizer, known answers)."""
    bad = 0

    def expect(cond, what):
        nonlocal bad
        if not cond:
            bad += 1
            log("cop MISMATCH: " + what)

    mem = _synthetic_mem()
    io = mem.io
    cpu = E.CPU(mem)
    cpu.reset_regs()

    def ins(w):
        return cpu.decode(0x01000000, w)()

    ins(0x40026000)                                               # mfc0 $v0, Status
    expect(cpu.g[2] == E.sext32(0x70030C13) and not io.events, "mfc0 Status: reset value, no event")
    ins(0x42000039)                                               # di
    ins(0x0000040F)                                               # sync.p
    ins(0x40026000)
    expect(cpu.g[2] & 0x10000 == 0 and [e[1] for e in io.events] == [('di',), ('sync.p',)], "di + sync.p")
    ins(0x42000038)                                               # ei
    ins(0x0000000F)                                               # sync.l
    ins(0x40026000)
    expect(cpu.g[2] & 0x10000 and [e[1] for e in io.events][-2:] == [('ei',), ('sync.l',)], "ei + sync.l")
    ins(0x40024800)                                               # mfc0 $v0, Count
    ins(0x40034800)                                               # mfc0 $v1, Count
    expect(cpu.g[3] - cpu.g[2] == E.COUNT_STEP, "Count advances per read")
    cpu.g[2] = 0x8003783E
    ins(0x4082C800)                                               # mtps $v0, 0 (PCCR)
    ins(0x4080C801)                                               # mtpc $0, 0
    io.cop0['PCR1'] = 1234
    ins(0x4005C803)                                               # mfpc $a1, 1
    expect(io.events[-3][1:3] == (('mtc0', 'PCCR'), 0x8003783E) and io.events[-2][1:3] == (('mtc0', 'PCR0'), 0)
           and cpu.g[5] == 1234 and io.events[-1][1:3] == (('mfc0', 'PCR1'), 1234), "perf counter moves")
    io.cache_tag = lambda op, addr: 0xABCDE000 | op
    cpu.g[6] = 0x40
    ins(0xBCD00001)                                               # cache 0x10, 1($a2)
    ins(0x4002E000)                                               # mfc0 $v0, TagLo
    expect(io.events[-2][1:3] == (('cache', 0x10), 0x41) and cpu.g[2] == E.sext32(0xABCDE010), "cache load tag")
    # eret: no delay slot, to EPC; clears EXL
    mem.wr(0x00100000, struct.pack("<3I", 0x42000018, 0x24020001, 0))   # eret; li v0,1 (must not run)
    io.cop0['EPC'] = cpu.RET
    cpu.g[2] = 7
    cpu.run(0x00100000, 10)
    expect(cpu.g[2] == 7 and io.cop0['Status'] & 2 == 0 and io.events[-1][1] == ('eret',), "eret")
    # COP2 decode against rabbitizer
    try:
        import rabbitizer as Rz
    except ImportError:
        Rz = None
    ndec = mism = 0
    if Rz is not None:
        def rz_name(op):
            """rabbitizer's mnemonic for the opcode bits `op`, trying the operand
            field values it accepts (it rejects nonzero reserved fields)."""
            for dest in (0xF, 0x1, 0x0):
                for ops in (0, (1 << 16) | (1 << 11), 1 << 16, 1 << 11):
                    i = Rz.Instruction(op | (dest << 21) | ops, vram=0x01000000, category=Rz.InstrCategory.R5900)
                    if i.isValid():
                        return i.getOpcodeName().lower().split('.')[0]    # vilwr.w -> vilwr
            return None
        rng = random.Random(7)
        for lo6 in range(0x40):
            for hi5 in (range(0x20) if lo6 >= 0x3C else [None]):
                op = (0x12 << 26) | (1 << 25) | lo6 | ((hi5 << 6) if hi5 is not None else 0)
                rn = rz_name(op)
                for _ in range(3):                      # eeemu's decode must not depend on the operand fields
                    w = op | (rng.randrange(16) << 21) | (rng.randrange(32) << 16) | (rng.randrange(32) << 11)
                    if hi5 is None:
                        w |= rng.randrange(32) << 6
                    mine = E.vu0_macro_name(w)
                    ndec += 1
                    if mine is not None and mine != rn:
                        mism += 1
                        log("cop2 decode MISMATCH %08X: eeemu %s, rabbitizer %s" % (w, mine, rn))
                if rn is not None and E.vu0_macro_name(op) is None and rn not in (
                        'vclipw', 'vrnext', 'vrget', 'vrinit', 'vrxor'):
                    mism += 1
                    log("cop2 decode: %s (%08X) valid in rabbitizer but not modelled" % (rn, op))
        for w, nm in ((0x4843E000, 'cfc2'), (0x48C3E000, 'ctc2'), (0x48B20800, 'qmtc2'), (0x48320800, 'qmfc2')):
            rn = Rz.Instruction(w, vram=0x01000000, category=Rz.InstrCategory.R5900).getOpcodeName().split('.')[0]
            if rn != nm:
                mism += 1
                log("cop2 decode MISMATCH %08X: %s" % (w, rn))
    bad += mism

    def f(x):
        return struct.unpack("<I", struct.pack("<f", x))[0]

    io.reset()
    for w in (0x4BE0076C, 0x4BE0072C, 0x4BFF033C, 0x4BFE033D, 0x4A800743, 0x4B000703):   # vu0_load_identity_regs
        ins(w)
    one = f(1.0)
    expect([io.vf[k] for k in (28, 29, 30, 31)] == [[one, 0, 0, 0], [0, one, 0, 0], [0, 0, one, 0], [0, 0, 0, one]],
           "identity matrix (vsub/vmove/vmr32/vaddw): %r" % ([io.vf[k] for k in (28, 29, 30, 31)],))
    cpu.g[2] = 4
    ins(0x48C25001)                                               # ctc2.i $v0, $vi10
    ins(0x4BEAE37D)                                               # vsqi.xyzw $vf28, ($vi10++)
    expect(io.vi[10] == 5 and io.events[-1][1:3] == (('w', E.VU0_DATA + 0x40, 16), one), "vsqi")
    cpu.g[18], cpu.gh[18] = (2 << 32) | 1, (255 << 32) | 3        # qmtc2 $s2, $vf1: ints 1 2 3 255
    ins(0x48B20800)
    ins(0x4BE1093C)                                               # vitof0.xyzw vf1, vf1
    cpu.g[18], cpu.gh[18] = f(0.5), 0
    ins(0x48B21000)                                               # qmtc2 $s2, $vf2
    ins(0x4BE20858)                                               # vmulx.xyzw vf1, vf1, vf2x
    ins(0x48320800)                                               # qmfc2 $s2, $vf1
    expect(cpu.g[18] == f(0.5) | (f(1.0) << 32) and cpu.gh[18] == f(1.5) | (f(127.5) << 32), "vitof0 / vmulx / qmfc2")
    cpu.g[2] = 0x202
    ins(0x48C2E000)                                               # ctc2 $v0, $vi28 (FBRST)
    io.model('vi29', [0x100, 0])
    ins(0x4842E800)
    ins(0x4842E800)                                               # cfc2 $v0, $vi29 twice
    expect([e[1:3] for e in io.events[-3:]] == [(('ctc2', 'vi28'), 0x202), (('cfc2', 'vi29'), 0x100),
                                                (('cfc2', 'vi29'), 0)] and cpu.g[2] == 0, "FBRST / VPU-STAT")
    log("cop0/cop2: di/ei/Status, Count, perf counters, cache tags, eret, sync; %d COP2 encodings against "
        "rabbitizer (%d mismatches); VU0 known answers: %s" % (ndec, mism, "ok" if not bad else "%d bad" % bad))
    return bad


def check_hw_retail(log, rng):
    """retail hardware code in context: gfx_init_vu0_constants (0x0100C050, COP2 +
    VU0 memory), gfx_reset_paths (0x0100BFE8, MMIO + COP2 FBRST), SyncDCache
    (0x0101C0F8: DIntr/EIntr, the cache-tag walk) against a model of the cache."""
    if not os.path.exists(RETAIL):
        return 0
    bad = 0

    def expect(cond, what):
        nonlocal bad
        if not cond:
            bad += 1
            log("hw retail MISMATCH: " + what)

    mem = E.Mem(RETAIL)
    io = mem.io
    cpu = E.CPU(mem, RETAIL_GP)
    io.reset()
    ctx = 0x70003000
    cpu.call(0x0100C050, [ctx])
    one = 0x3F800000
    rows = [one, one << 32, one << 64, one << 96]
    vsq = [e[1:3] for e in io.events if e[1][0] == 'w']
    want = [(('w', E.VU0_DATA + 16 * (4 * blk + k), 16), rows[k]) for blk in (1, 2, 3) for k in range(4)]
    expect(vsq == want, "gfx_init_vu0_constants VU0 stores: %r" % vsq[:3])
    expect(mem.u8(ctx) == 1 and mem.u32(ctx + 0xBA0) == 0x7F7FFFFF and mem.u32(ctx + 0xB9C) == 0x437F0000,
           "gfx_init_vu0_constants context fields")
    expect(io.vi[11] == 0x10 and io.vi[12] == 0x50 and io.vi[13] == 0x60 and io.vi[10] == 0x10, "vi10..13")
    io.reset()
    cpu.call(0x0100BFE8, [])
    got = [(e[1][0], E.io_name(e[1][1]) if e[1][0] in 'rw' else e[1][1], e[2]) for e in io.events]
    expect(got == [('w', 'VIF0_FBRST', 1), ('w', 'VIF1_FBRST', 1), ('cfc2', 'vi28', 0), ('ctc2', 'vi28', 0x202),
                   ('w', 'GIF_CTRL', 1), ('w', 'GIF_MODE', 4), ('w', 'VIF0_ERR', 6), ('w', 'VIF1_ERR', 6)],
           "gfx_reset_paths events: %r" % got)
    # SyncDCache over a random range against random tags: exactly the lines in range get DXWBIN
    for _ in range(8):
        io.reset()
        tags = {(idx, way): rng.choice([0, rng.getrandbits(20) << 12, (0x01100000 + rng.randrange(0x4000)) & ~0xFFF])
                for idx in range(0, 0x1000, 0x40) for way in (0, 1)}
        io.cache_tag = lambda op, addr: tags[(addr & 0xFC0, addr & 1)]
        lo = 0x01100000 + rng.randrange(0x3000)
        hi = lo + rng.randrange(0x2000)
        eie = rng.random() < 0.5
        if not eie:
            io.cop0['Status'] &= ~0x10000
        cpu.call(0x0101C0F8, [lo, hi])
        s, e_ = lo & ~0x3F, hi & ~0x3F
        want = [(i | way) for i in range(0, 0x1000, 0x40) for way in (0, 1)
                if s <= ((tags[(i, way)] & 0xFFFFF000) + i) <= e_]
        got = [e[2] for e in io.events if e[1] == ('cache', 0x14)]
        ops = [e[1][0] for e in io.events]
        expect(got == want, "SyncDCache(%08X, %08X) writebacks %r want %r" % (lo, hi, got[:4], want[:4]))
        expect(('di' in ops) == eie and ('ei' in ops) == eie and ops.count('cache') == 128 + len(want),
               "SyncDCache interrupt bracket / op count (EIE %d)" % eie)
    log("hw retail: gfx_init_vu0_constants, gfx_reset_paths, SyncDCache (cache-tag model): %s"
        % ("ok" if not bad else "%d mismatches" % bad))
    return bad


def main(argv=None, log=print):
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, default=300, help="random states per instruction")
    ap.add_argument("--libc", type=int, default=400, help="libc oracle rounds (9 calls each)")
    ap.add_argument("--quick", action="store_true", help="40 states per instruction, 60 libc rounds")
    a = ap.parse_args(argv)
    if a.quick:
        a.n, a.libc = 40, 60
    rng = random.Random(0x5E1F)
    bad = check_decode(log)
    bad += check_semantics(a.n, log, rng)
    bad += known_answers(log)
    bad += check_base(log)
    bad += check_libc(a.libc, log, rng)
    bad += check_clut(max(20, a.libc // 2), log, rng)
    bad += check_memmap(log)
    bad += check_dma(log)
    bad += check_cop(log)
    bad += check_hw_retail(log, rng)
    log("EEEMU SELFTEST %s" % ("PASS" if bad == 0 else "FAIL (%d)" % bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
