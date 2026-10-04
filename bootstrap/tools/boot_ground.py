"""Groundwork for BOOTSTRAP_SCUS-97102 decompilation.

Produces, in OUT dir:
  full.txt        annotated disassembly of all text
  funcs.json      function table: start, end, callees, callers, strings, gprefs
  funcs.csv       same, flat
  datamap.txt     annotated dump of data sections (code ptrs, strings)
  chunks/NN_<lo>_<hi>.txt   per-chunk annotated listings for agents
"""
import struct, json, os, sys
from capstone import Cs, CS_ARCH_MIPS, CS_MODE_MIPS64, CS_MODE_LITTLE_ENDIAN

import paths

P = paths.retail_elf()
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ref")
os.makedirs(OUT, exist_ok=True)
os.makedirs(os.path.join(OUT, "chunks"), exist_ok=True)

BASE = 0x01000000
FOFF = 0x1000
FILESZ = 0x41E6B
TEXT_LO = 0x01000000
TEXT_HI = 0x01026B88          # end of .text.sceGsResetGraph
ENTRY = 0x01000008
GP = 0x01049CF0

SECTIONS = [
    (".text",    0x01000000, 0x010269E8),
    (".text.sceGsGetGParam", 0x010269E8, 0x010269F4),
    (".text.sceGsResetGraph", 0x010269F8, 0x01026B88),
    (".data",    0x01026C00, 0x0103DDD8),
    (".rodata",  0x0103DE00, 0x01040BC2),
    (".rdata",   0x01040C00, 0x01040F98),
    (".gcc_except_table", 0x01041000, 0x01041CE8),
    (".lit4",    0x01041D00, 0x01041D0C),
    (".sdata",   0x01041D80, 0x01041E6B),
    (".sbss",    0x01041E80, 0x01041EC8),
    (".bss",     0x01041F00, 0x0104DE94),
]

d = open(P, "rb").read()
img = d[FOFF:FOFF + FILESZ]

def word(va):
    o = va - BASE
    if 0 <= o <= FILESZ - 4:
        return struct.unpack_from("<I", img, o)[0]
    return None

def in_text(va):
    return TEXT_LO <= va < TEXT_HI and (va & 3) == 0

def section_of(va):
    for n, lo, hi in SECTIONS:
        if lo <= va < hi:
            return n
    return None

def cstr(va):
    o = va - BASE
    if not (0 <= o < FILESZ):
        return None
    e = img.find(b"\0", o, min(o + 96, FILESZ))
    if e < 0:
        return None
    s = img[o:e]
    if len(s) >= 3 and all(0x20 <= c < 0x7F for c in s):
        return s.decode()
    return None

md = Cs(CS_ARCH_MIPS, CS_MODE_MIPS64 | CS_MODE_LITTLE_ENDIAN)

MMI_FN = {0x00:"madd",0x01:"maddu",0x04:"plzcw",0x10:"mfhi1",0x11:"mthi1",
          0x12:"mflo1",0x13:"mtlo1",0x18:"mult1",0x19:"multu1",0x1A:"div1",
          0x1B:"divu1",0x20:"madd1",0x21:"maddu1",0x30:"pmfhl",0x31:"pmthl",
          0x34:"psllh",0x36:"psrlh",0x37:"psrah",0x3C:"psllw",0x3E:"psrlw",
          0x3F:"psraw",0x08:"mmi0",0x28:"mmi1",0x09:"mmi2",0x29:"mmi3"}

def dis1(va, w):
    op = w >> 26
    if w == 0x42000038: return "ei", ""
    if w == 0x42000039: return "di", ""
    if w == 0: return "nop", ""
    rs=(w>>21)&31; rt=(w>>16)&31; rd=(w>>11)&31; sa=(w>>6)&31; fn=w&0x3F
    simm = (w & 0xFFFF) - 0x10000 if w & 0x8000 else w & 0xFFFF
    if op == 0x1E: return "lq", "$%d, %d($%d)" % (rt, simm, rs)
    if op == 0x1F: return "sq", "$%d, %d($%d)" % (rt, simm, rs)
    if op == 0x1C:
        m = MMI_FN.get(fn, "mmi?")
        if fn in (0x18,0x19,0x1A,0x1B,0x00,0x01,0x20,0x21):
            return m, "$%d,$%d,$%d" % (rd, rs, rt)
        if fn in (0x10,0x12): return m, "$%d" % rd
        if fn in (0x11,0x13): return m, "$%d" % rs
        return m, "%08X" % w
    if op == 0x12: return "cop2", "%08X" % w
    if op == 0x11 and ((w >> 21) & 31) == 0x10:
        # EE COP1.S extras capstone rejects
        C1 = {0x28: "max.s", 0x29: "min.s", 0x04: "sqrt.s", 0x16: "rsqrt.s",
              0x18: "adda.s", 0x19: "suba.s", 0x1A: "mula.s", 0x1C: "madda.s",
              0x1D: "msuba.s", 0x1E: "madd.s", 0x1F: "msub.s"}
        if fn in C1:
            fd=(w>>6)&31; fs=(w>>11)&31; ft=(w>>16)&31
            if fn == 0x04: return "sqrt.s", "$f%d, $f%d" % (fd, ft)
            if fn in (0x18,0x19,0x1A,0x1C,0x1D): return C1[fn], "$f%d, $f%d" % (fs, ft)
            return C1[fn], "$f%d, $f%d, $f%d" % (fd, fs, ft)
    if op == 0 and fn == 0x28: return "mfsa", "$%d" % rd
    if op == 0 and fn == 0x29: return "mtsa", "$%d" % rs
    if op == 0 and fn == 0x18 and rd: return "mult3", "$%d,$%d,$%d" % (rd, rs, rt)
    if op == 0 and fn == 0x19 and rd: return "multu3", "$%d,$%d,$%d" % (rd, rs, rt)
    for i in md.disasm(struct.pack("<I", w), va):
        return i.mnemonic, i.op_str
    return "<bad>", "%08X" % w

# ---------------- pass 1: raw decode all text ----------------
N = (TEXT_HI - TEXT_LO) // 4
words = [word(TEXT_LO + 4*i) for i in range(N)]
ins = [dis1(TEXT_LO + 4*i, words[i]) for i in range(N)]

def is_jr_ra(w):  return w == 0x03E00008
def is_jal(w):    return (w >> 26) == 3
def is_j(w):      return (w >> 26) == 2
def jtarget(w):   return ((w & 0x03FFFFFF) << 2) | (TEXT_LO & 0xF0000000)

def is_branch(w):
    op = w >> 26
    if op in (1, 4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17): return True
    if op == 0x11 and ((w >> 21) & 31) == 8: return True  # bc1
    return False

def btarget(va, w):
    simm = (w & 0xFFFF) - 0x10000 if w & 0x8000 else w & 0xFFFF
    return va + 4 + (simm << 2)

# ---------------- code pointers found in data ----------------
data_code_ptrs = {}   # va_in_text -> [data_va,...]
landing_pads = {}     # va_in_text -> [except_table_va,...]  (NOT function starts)
for name, lo, hi in SECTIONS:
    if name == ".gcc_except_table":
        a = lo
        while a + 4 <= hi:
            w = word(a)
            if w is not None and in_text(w):
                landing_pads.setdefault(w, []).append(a)
            a += 4
        continue
    if name.startswith(".text") or name in (".sbss", ".bss"): continue
    a = lo
    while a + 4 <= hi:
        w = word(a)
        if w is not None and in_text(w):
            data_code_ptrs.setdefault(w, []).append(a)
        a += 4

# ---------------- jump tables (jr $reg switch) ----------------
# A run of >=3 consecutive code pointers is either a switch jump table
# (targets cluster inside ONE function) or a function-pointer array such as a
# ctor list (targets are spread out and look like prologues). Decide per run.
ptr_runs = []
for name, lo, hi in SECTIONS:
    if name not in (".rodata", ".rdata", ".data"): continue
    a = lo
    while a + 4 <= hi:
        run = []
        b = a
        while b + 4 <= hi:
            w = word(b)
            if w is not None and in_text(w):
                run.append(w); b += 4
            else:
                break
        if len(run) >= 3:
            ptr_runs.append((a, run))
        a = max(b, a + 4)

jumptab_targets = set()
for a, run in ptr_runs:
    span = max(run) - min(run)
    if span < 0x3000:          # switch table: targets cluster tightly
        jumptab_targets.update(run)
    # else: function pointer array; targets stay eligible as starts

# ---------------- function starts ----------------
starts = set([ENTRY, TEXT_LO, 0x010269E8, 0x010269F8])
for i in range(N):
    w = words[i]
    if is_jal(w):
        t = jtarget(w)
        if in_text(t): starts.add(t)
for t in data_code_ptrs:
    if t not in jumptab_targets:
        starts.add(t)

# prologue after end: addiu/daddiu sp,sp,-X right after (jr ra + slot) or nops
for i in range(2, N):
    w = words[i]
    op = w >> 26
    if op in (0x09, 0x19):  # addiu/daddiu
        rs=(w>>21)&31; rt=(w>>16)&31
        if rs == 29 and rt == 29 and (w & 0x8000):
            j = i - 1
            while j >= 0 and words[j] == 0: j -= 1
            if j >= 1 and (is_jr_ra(words[j-1]) or is_j(words[j-1]) or
                           words[j] == 0x0000000C or
                           (words[j] and is_jr_ra(words[j]))):
                starts.add(TEXT_LO + 4*i)

# starts confirmed by the mapping pass (tools/extract_fixes.py)
_extra = os.path.join(OUT, "extra_starts.txt")
if os.path.exists(_extra):
    for line in open(_extra, encoding="utf-8"):
        tok = line.split("#")[0].strip()
        if tok:
            starts.add(int(tok, 16))

starts = sorted(s for s in starts if in_text(s))

# build regions
funcs = []
for k, s in enumerate(starts):
    e = starts[k+1] if k+1 < len(starts) else TEXT_HI
    funcs.append({"start": s, "end": e})

# merge regions with no return AND fallthrough? keep simple: flag regions
# whose predecessor branches past their start (means our split was wrong)
# -- left to reviewers; heuristics above are conservative.

# ---------------- annotate ----------------
# register const tracking per function for lui/addiu-ori pairs
fname = lambda a: "F_%08X" % a
startset = set(f["start"] for f in funcs)

def annotate_range(lo, hi, sink):
    regs = {}
    for va in range(lo, hi, 4):
        i = (va - TEXT_LO) // 4
        w = words[i]
        m, o = ins[i]
        note = ""
        op = w >> 26
        rs=(w>>21)&31; rt=(w>>16)&31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        if op == 0x0F:  # lui
            regs[rt] = imm << 16
        elif op in (0x09, 0x19) and rs in regs:  # addiu/daddiu
            a = (regs[rs] + simm) & 0xFFFFFFFF
            sec = section_of(a)
            if sec:
                s = cstr(a)
                note = "  ; %s %s%s" % (hex(a), sec, (" %r" % s) if s and not sec.startswith(".text") else "")
                if in_text(a):
                    note = "  ; &%s" % (fname(a) if a in startset else hex(a))
            regs[rt] = a
        elif op == 0x0D and rs in regs:  # ori
            a = regs[rs] | imm
            sec = section_of(a)
            if sec:
                s = cstr(a)
                note = "  ; %s %s%s" % (hex(a), sec, (" %r" % s) if s else "")
            regs[rt] = a
        elif op in (0x20,0x21,0x23,0x24,0x25,0x28,0x29,0x2B,0x31,0x39,0x37,0x3F,0x1E,0x1F,0x22,0x26,0x2A,0x2E,0x27,0x1B,0x1A,0x2C,0x2D,0x30):
            basereg = rs
            if basereg == 28:
                a = (GP + simm) & 0xFFFFFFFF
                sec = section_of(a) or "?"
                v = word(a & ~3)
                s = cstr(v) if v else None
                extra = ""
                if v is not None:
                    extra = " [%08X%s]" % (v, (" %r" % s) if s else "")
                    if in_text(v) and v in startset:
                        extra = " [&%s]" % fname(v)
                note = "  ; gp%+#x = %s %s%s" % (simm, hex(a), sec, extra)
            elif basereg in regs:
                a = (regs[basereg] + simm) & 0xFFFFFFFF
                sec = section_of(a)
                if sec:
                    v = word(a & ~3) if not sec.startswith(".text") else None
                    s = cstr(a)
                    note = "  ; %s %s%s" % (hex(a), sec, (" %r" % s) if s else "")
            if op in (0x0F,): pass
        if is_jal(w):
            t = jtarget(w)
            note = "  ; call %s" % (fname(t) if t in startset else hex(t))
        elif is_j(w):
            t = jtarget(w)
            if t in startset and not (lo <= t < hi):
                note = "  ; tail-> %s" % fname(t)
            else:
                note = "  ; -> %s" % hex(t)
        elif is_branch(w):
            note = "  ; -> %s" % hex(btarget(va, w))
        # kill reg tracking on jal/branch boundaries? keep simple (over-approx)
        if va in startset:
            sink.append("")
            sink.append("========== %s ==========" % fname(va))
            xr = data_code_ptrs.get(va)
            if xr:
                sink.append("; data xrefs: %s" % ", ".join(hex(x) for x in xr))
            regs = {}
        if va in landing_pads:
            sink.append("; exception landing pad next (gcc_except_table %s)" %
                        ", ".join(hex(x) for x in landing_pads[va]))
        sink.append("0x%08X: %08X  %-8s %s%s" % (va, w, m, o, note))

lines = []
annotate_range(TEXT_LO, TEXT_HI, lines)
open(os.path.join(OUT, "full.txt"), "w").write("\n".join(lines))

# ---------------- function metadata ----------------
for f in funcs:
    callees, strs, gprefs = [], [], []
    for va in range(f["start"], f["end"], 4):
        i = (va - TEXT_LO) // 4
        w = words[i]
        if is_jal(w):
            t = jtarget(w)
            if in_text(t): callees.append(t)
    f["callees"] = sorted(set(callees))
    f["size"] = f["end"] - f["start"]
    f["data_xrefs"] = data_code_ptrs.get(f["start"], [])
callers = {}
for f in funcs:
    for c in f["callees"]:
        callers.setdefault(c, []).append(f["start"])
for f in funcs:
    f["callers"] = sorted(set(callers.get(f["start"], [])))

# validation: conditional branches must stay inside their function
viol = []
for f in funcs:
    for va in range(f["start"], f["end"], 4):
        i = (va - TEXT_LO) // 4
        w = words[i]
        if is_branch(w):
            t = btarget(va, w)
            if not (f["start"] <= t < f["end"]):
                viol.append((va, t, f["start"]))
with open(os.path.join(OUT, "boundary_violations.txt"), "w") as fh:
    for va, t, fs in viol:
        fh.write("branch at 0x%08X in F_%08X targets 0x%08X\n" % (va, fs, t))
print("boundary violations:", len(viol))

json.dump(funcs, open(os.path.join(OUT, "funcs.json"), "w"), indent=0)
with open(os.path.join(OUT, "funcs.csv"), "w") as fh:
    fh.write("start,end,size,ncallees,ncallers,data_xrefs\n")
    for f in funcs:
        fh.write("0x%08X,0x%08X,%d,%d,%d,%d\n" % (
            f["start"], f["end"], f["size"], len(f["callees"]),
            len(f["callers"]), len(f["data_xrefs"])))

# ---------------- chunks for agents ----------------
CH = 2600  # instructions per chunk target
chunks, cur, count = [], [], 0
for f in funcs:
    cur.append(f); count += f["size"] // 4
    if count >= CH:
        chunks.append(cur); cur, count = [], 0
if cur: chunks.append(cur)
for ci, ch in enumerate(chunks):
    lo, hi = ch[0]["start"], ch[-1]["end"]
    sink = []
    annotate_range(lo, hi, sink)
    fn = os.path.join(OUT, "chunks", "%02d_%08X_%08X.txt" % (ci, lo, hi))
    open(fn, "w").write("\n".join(sink))

# ---------------- data map ----------------
dm = []
for name, lo, hi in SECTIONS:
    if name.startswith(".text") or name in (".sbss", ".bss"): continue
    dm.append("")
    dm.append("=========== %s [%s..%s) ===========" % (name, hex(lo), hex(hi)))
    a = lo
    while a < hi:
        if a == 0x01027F38:
            dm.append("0x01027F38: ... embedded gzip stream (FNAME 'startupa.img', inflates to 0x38020-byte 'Tex1' image), elided ...")
            a = 0x01033C98
            continue
        w = word(a) if a + 4 <= hi else None
        s = cstr(a)
        if s and len(s) >= 4:
            dm.append("0x%08X: str %r" % (a, s))
            a = (a + len(s) + 1 + 3) & ~3
            continue
        if w is not None and in_text(w):
            tag = fname(w) if w in startset else ("text+%s" % hex(w))
            dm.append("0x%08X: %08X  -> %s" % (a, w, tag))
        elif w is not None and section_of(w) and w >= 0x01026C00:
            dm.append("0x%08X: %08X  -> %s" % (a, w, section_of(w)))
        elif w is not None and w != 0:
            dm.append("0x%08X: %08X" % (a, w))
        a += 4
    # collapse runs of zeros implicitly (skip printing zeros)
dm2, zrun = [], 0
for L in dm:
    if L.endswith(": 00000000"): continue
    dm2.append(L)
open(os.path.join(OUT, "datamap.txt"), "w").write("\n".join(dm2))

print("funcs:", len(funcs))
print("chunks:", len(chunks))
print("bad decodes:", sum(1 for m, _ in ins if m == "<bad>"))
print("text instrs:", N)
big = sorted(funcs, key=lambda f: -f["size"])[:10]
for f in big:
    print("large fn 0x%08X size %d" % (f["start"], f["size"]))
