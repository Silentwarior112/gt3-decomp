"""Verify the data references of a compiled function against the retail code
(the core's version of the bootstrap's tools/dataref.py).

The scorer masks relocated fields, so `lui/addiu %hi/%lo(sym)` and
`%gp_rel(sym)` compare equal whatever they point at. This checks what they
point at:

  * a %hi/%lo pair (or a lone %gp_rel / literal-pool reference) gives, in the
    retail instructions aligned with it, a concrete retail address R;
  * a named symbol with a known retail address must satisfy
    R == addr(sym) + addend.  Addresses: the address comments on the
    declarations (tools/hybrid.py's declared()), a g_XXXXXXXX-style name, the
    class index's vtables, and the function map (function pointers);
  * a section-relative reference (string literals, statics: the assembler
    turns local symbols into section symbol + offset) cannot be checked alone,
    but every reference to the same section of the object must imply the same
    retail base (R - offset).

verify(...) -> {"verified", "mismatch", "unresolved", "notes": [...]}
"""
import struct

from elfobj import R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL

GP = 0x003579F0


def sx16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


_DATAMAP = None


def datamap():
    """name -> retail address of every declared global, vtable and table name."""
    global _DATAMAP
    if _DATAMAP is None:
        import hybrid
        vt = hybrid.vtables()
        m = dict(vt)
        secondary = _secondary_vtables()
        for name, addr in hybrid.table_names().items():
            # ref/data_symbols.csv names the *secondary* vtable of a class
            # with two bases _vt$<class> (43 classes, e.g. LevelButtonList
            # 0x0030BA40); egcs's _vt$<class> is the primary one (0x0030BA58,
            # hybrid.vtables()), the secondary is _vt$<class>$<base>.
            # GarageCar has seven {secondary, primary} copies (one per TU
            # that emits them), so hybrid.vtables() has no `$*' entry for it:
            # skip any csv name that is a secondary vtable.
            if name in vt and (("%s$*" % name) in vt or addr in secondary):
                continue
            m[name] = addr
        m.update(hybrid.declared())
        _DATAMAP = m
    return _DATAMAP


def _secondary_vtables():
    """Addresses of every secondary vtable in ref/classes.json."""
    import json
    import os
    import paths
    out = set()
    for c in json.load(open(os.path.join(paths.REF, "classes.json"))).values():
        for v in c.get("vtables", []):
            if v.get("kind") == "secondary":
                out.add(int(v["addr"], 16))
    return out


def address_of(name, symmap):
    import hybrid
    a = datamap().get(name)
    if a is None:
        a = symmap.get(name)
    if a is None:
        m = hybrid.ADDR_IN_NAME.search(name)
        if m:
            a = int(m.group(1), 16)
    return a


def named_bases(obj):
    """section index -> retail base implied by the section's own named data
    (defined STT_OBJECT symbols with a declared address: a `/* 0x... */`
    declaration or a g_XXXXXXXX name), the most common one per section.
    verify() takes it as the section's base before any reference: else the
    first section-relative reference defines it, and a non-exact function's
    misaligned one (u057's collideCars, whose `lui / addiu` of g_002C0568
    pairs with retail's of another address) made every later reference to
    the object's .data differ (the assembler turns references to an
    object's own globals into section + offset).  Sections without named
    data keep the first-sighting rule.  (Only a whole g_XXXXXXXX name
    counts: `_GLOBAL_$F$g_002B3BF0', u042's T123 frame info, is not that
    variable.  Weak symbols do not count: egcs's vtables are weak, and the
    declared address of a vtable that several objects emit is one copy's,
    u009's SmartIndexT<int>.)  File statics in .bss / .sbss (egcs's
    `.lcomm`) count too: the old assembler gives their local symbols no type
    (STT_NOTYPE, size 0; u078's T333 curve sets)."""
    import re
    votes = {}
    for y in obj.symbols:
        if not (0 < y.shndx < len(obj.sections)):
            continue
        lcomm = (y.type == 0 and y.bind == 0 and obj.sections[y.shndx].type == 8)   # SHT_NOBITS
        if ((y.type != 1 and not lcomm) or y.bind == 2
                or y.name.startswith(("_GLOBAL_$", "_vt$"))):
            continue
        sec = obj.sections[y.shndx]
        if sec.flags & 0x4:                                 # SHF_EXECINSTR
            continue
        a = datamap().get(y.name)
        if a is None:
            m = re.match(r"^[gDBR]_([0-9A-F]{8})$", y.name)
            a = int(m.group(1), 16) if m else None
        if a is None:
            continue
        b = (a - y.value) & 0xFFFFFFFF
        v = votes.setdefault(y.shndx, {})
        v[b] = v.get(b, 0) + 1
    return {k: max(sorted(v), key=lambda b: v[b]) for k, v in votes.items()}


def gp0(obj):
    """The object's GP0 (.reginfo ri_gp_value): the old assembler biases
    GPREL16 / LITERAL addends against section symbols by it."""
    v = getattr(obj, "_gp0", None)
    if v is None:
        ri = obj.section(".reginfo")
        v = struct.unpack_from("<I", ri.data, 0x14)[0] if ri is not None and ri.size >= 0x18 else 0
        obj._gp0 = v
    return v


def verify(obj, y, sec, pairs, ow, rw, symmap, section_bases):
    """pairs: [(obj_index|None, retail_index|None, ok)] from the scorer's alignment.
    section_bases: shared across the functions of one object: section index ->
    inferred retail base (filled in here)."""
    o2r = {i: j for i, j, ok in pairs if i is not None and j is not None}
    rels = sorted([r for r in obj.relocs.get(sec.index, []) if y.value <= r.offset < y.value + y.size],
                  key=lambda r: r.offset)
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
        want = address_of(sym.name, symmap)
        if want is None:
            res["unresolved"] += 1
            return
        if (want + addend) & 0xFFFFFFFF == retail_addr:
            res["verified"] += 1
        else:
            res["mismatch"] += 1
            res["notes"].append("%s: %s%+d -> expected 0x%08X, retail uses 0x%08X"
                                % (where, sym.name, addend, (want + addend) & 0xFFFFFFFF, retail_addr))

    pending_hi = []
    for r in rels:
        i = (r.offset - y.value) // 4
        if r.type == R_MIPS_HI16:
            pending_hi.append((r, i))
        elif r.type == R_MIPS_LO16:
            lo_o = ow[i] if i < len(ow) else 0
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
            if i not in o2r or i >= len(ow):
                res["unresolved"] += 1
                continue
            addend = sx16(ow[i])
            if r.sym is not None and r.sym.type == 3:
                addend += gp0(obj)
            retail = (GP + sx16(rw[o2r[i]])) & 0xFFFFFFFF
            check(r.sym, addend, retail, "insn %d gp" % i)
    res["unresolved"] += len(pending_hi)
    return res
