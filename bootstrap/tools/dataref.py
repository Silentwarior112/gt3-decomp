"""Verify the data references of a compiled function against the retail code.

The scorer masks relocated fields, so `lui/addiu %hi/%lo(sym)` and
`%gp_rel(sym)` compare equal whatever they point at. This module checks what
they point at:

  * a %hi/%lo pair (or a lone %gp_rel / literal-pool reference) gives, in the
    retail instructions aligned with it, a concrete retail address R;
  * a named symbol with a known retail address (a `// @0x...` / `/* @0x... */`
    annotation on its definition, or a function address from the symbol map)
    must satisfy R == addr(sym) + addend;
  * a section-relative reference (string literals, statics: the assembler
    turns local symbols into section symbol + offset) cannot be checked alone,
    but every reference to the same section of the object must imply the same
    retail base (R - offset).

verify(...) -> {"verified", "mismatch", "unresolved", "notes": [...]}
"""
import os, re, struct

from elfobj import R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL, R_MIPS_32

GP = 0x01049CF0
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))

def sx16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v

_DATAMAP = None
DEF = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?(?:static\s+|const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+|union\s+|class\s+)*"
                 r"[\w:<>\s\*]+?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:__attribute__\s*\(\(.*?\)\))?\s*(?:=|;).*?@\s*(0x01[0-9A-Fa-f]{6})")

def datamap():
    """name -> retail address, from `@0x...` annotations on variable definitions."""
    global _DATAMAP
    if _DATAMAP is None:
        m = {}
        for sub in ("src", "sdk", "runtime", "include"):
            for dp, _, fns in os.walk(os.path.join(ROOT, sub)):
                for fn in fns:
                    if not fn.endswith((".c", ".cpp", ".cc", ".h")):
                        continue
                    for L in open(os.path.join(dp, fn), encoding="utf-8", errors="replace"):
                        g = DEF.match(L)
                        if g and "(" not in L.split("@")[0].split("=")[0].replace("__attribute__", ""):
                            a = int(g.group(2), 16)
                            if 0x01026C00 <= a < 0x0104DEB0:
                                m.setdefault(g.group(1), a)
        _DATAMAP = m
    return _DATAMAP

def gp0(obj):
    """The object's GP0 (.reginfo ri_gp_value, bytes 0x14..0x17). The old
    assembler writes 0x4000 into objects that have small data; the in-place
    addend of a GPREL16 / LITERAL relocation against a SECTION symbol is then
    biased by it (objdump: `lw s3,-16384(gp)  R_MIPS_GPREL16 .sdata+0x4000` for
    offset 0). References to named symbols are not biased."""
    v = getattr(obj, "_gp0", None)
    if v is None:
        ri = obj.section(".reginfo")
        v = struct.unpack_from("<I", ri.data, 0x14)[0] if ri is not None and ri.size >= 0x18 else 0
        obj._gp0 = v
    return v

def verify(obj, y, sec, pairs, ow, rw, symmap, section_bases):
    """pairs: [(obj_index|None, retail_index|None, ok)] from the scorer's alignment.
    section_bases: dict shared across the functions of one object:
    section index -> inferred retail base (filled in here)."""
    o2r = {i: j for i, j, ok in pairs if i is not None and j is not None}
    rels = sorted([r for r in obj.relocs.get(sec.index, []) if y.value <= r.offset < y.value + y.size],
                  key=lambda r: r.offset)
    dm = datamap()
    res = {"verified": 0, "mismatch": 0, "unresolved": 0, "notes": []}

    def check(sym, addend, retail_addr, where):
        if sym is None:
            res["unresolved"] += 1
            return
        if sym.type == 3:                                   # section symbol
            base = retail_addr - addend
            prev = section_bases.get(sym.shndx)
            if prev is None:
                section_bases[sym.shndx] = base
                res["unresolved"] += 1                      # first sighting defines the base
            elif prev == base:
                res["verified"] += 1
            else:
                res["mismatch"] += 1
                res["notes"].append("%s: %s+0x%X -> retail 0x%08X, section base 0x%08X vs 0x%08X"
                                    % (where, obj.sections[sym.shndx].name, addend, retail_addr, base, prev))
            return
        want = dm.get(sym.name, symmap.get(sym.name))
        if want is None:
            res["unresolved"] += 1
            return
        if want + addend == retail_addr:
            res["verified"] += 1
        else:
            res["mismatch"] += 1
            res["notes"].append("%s: %s%+d -> expected 0x%08X, retail uses 0x%08X"
                                % (where, sym.name, addend, want + addend, retail_addr))

    pending_hi = []                                          # (reloc, obj index)
    for r in rels:
        i = (r.offset - y.value) // 4
        if r.type == R_MIPS_HI16:
            pending_hi.append((r, i))
        elif r.type == R_MIPS_LO16:
            lo_o = ow[i]
            his = [(h, hi) for h, hi in pending_hi if h.sym is r.sym or (h.sym and r.sym and h.sym.name == r.sym.name
                                                                              and h.sym.shndx == r.sym.shndx)]
            if not his:
                res["unresolved"] += 1
                continue
            for h, hi in his:
                pending_hi.remove((h, hi))
                if hi not in o2r or i not in o2r:
                    res["unresolved"] += 1
                    continue
                addend = ((ow[hi] & 0xFFFF) << 16) + sx16(lo_o)
                retail = (((rw[o2r[hi]] & 0xFFFF) << 16) + sx16(rw[o2r[i]])) & 0xFFFFFFFF
                check(r.sym, addend, retail, "insn %d/%d" % (hi, i))
        elif r.type in (R_MIPS_GPREL16, R_MIPS_LITERAL):
            if i not in o2r:
                res["unresolved"] += 1
                continue
            addend = sx16(ow[i])
            if r.sym is not None and r.sym.type == 3:
                addend += gp0(obj)                      # section-symbol gp refs carry the GP0 bias
            retail = (GP + sx16(rw[o2r[i]])) & 0xFFFFFFFF
            check(r.sym, addend, retail, "insn %d gp" % i)
    res["unresolved"] += len(pending_hi)
    return res
