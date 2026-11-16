"""Find the main executable's functions that are the same code as a function of the
decompiled bootstrap (decomp/bootstrap): the Sony libraries (libcdvd 2240, libpad 2200,
libmc 2240, libkernl 2240, libgraph 2221), libc, the GCC runtime and Polyphony's pdistd
are shared.  Writes ref/libmatch.json: core address -> bootstrap address, name, file.

Two functions match when their instructions are equal with every relocated field masked
(jal/j targets, lui immediates, %lo / $gp-relative offsets).  Unique matches are anchors;
each anchor's delta is then extended over its neighbours, because libraries are linked
object by object.
"""
import collections
import json
import os
import re
import struct

import coreelf as E
import paths

BOOT = os.path.join(paths.REPO, "bootstrap")
BOOT_ELF = os.path.join(BOOT, "baserom", "SCUS_971.02")
BBASE, BFOFF = 0x01000000, 0x1000


GPR_LOADS = {0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x37, 0x1A, 0x1B, 0x1E, 0x22, 0x26}


def dest_gpr(x):
    """The general register an instruction writes, or None."""
    op = x >> 26
    if op == 0:
        return (x >> 11) & 31
    if op in (0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x18, 0x19) or op in GPR_LOADS:
        return (x >> 16) & 31
    if op == 3:
        return 31
    return None


def fingerprint(words):
    """Instruction words with the relocated fields masked: jal/j targets, lui
    immediates, $gp-relative offsets, and the %lo immediate of an instruction that
    uses a register holding a lui value.  A register stops counting as a lui base
    when a load writes it: a struct offset off a pointer loaded from memory is not a
    relocation (the bootstrap's gfx_get_packet_ptr matched a different field that
    way).  Other writes keep it, because the scan is linear and the lui value may
    still reach the use on another path."""
    out = []
    lui_regs = set()
    for x in words:
        op = x >> 26
        rs, rt = (x >> 21) & 31, (x >> 16) & 31
        d = dest_gpr(x)
        if op in (2, 3):                       # j / jal
            out.append(x & 0xFC000000)
        elif op == 0x0F:                       # lui
            out.append(x & 0xFFFF0000)
        elif (op in (0x09, 0x0D, 0x19) or op in E.LOADSTORE_OPS) and (rs == 28 or rs in lui_regs):
            out.append(x & 0xFFFF0000)         # $gp-relative or %lo
        else:
            out.append(x)
        if op == 0x0F:
            lui_regs.add(rt)
        elif d is not None and (op in GPR_LOADS or (op in (0x09, 0x0D, 0x19) and rs in lui_regs)):
            # loaded data, or a completed %hi/%lo address: offsets off it are fields
            lui_regs.discard(d)
    return tuple(out)


def fingerprint_loose(words):
    """Like fingerprint(), but every register that ever held a lui value stays a
    lui base: finds the matches the linear scan of fingerprint() misses when the lui
    value reaches its use on another path.  plausible() then vets them."""
    out = []
    lui_regs = set()
    for x in words:
        op = x >> 26
        rs, rt = (x >> 21) & 31, (x >> 16) & 31
        if op in (2, 3):
            out.append(x & 0xFC000000)
        elif op == 0x0F:
            lui_regs.add(rt)
            out.append(x & 0xFFFF0000)
        elif (op in (0x09, 0x0D, 0x19) or op in E.LOADSTORE_OPS) and (rs == 28 or rs in lui_regs):
            out.append(x & 0xFFFF0000)
        else:
            out.append(x)
    return tuple(out)


def plausible(cw, bw):
    """A loose match is kept unless an instruction that only the loose rule masks is
    a load/store whose two offsets both look like struct fields (0..0xFFF): those
    differ for real, while relocated %lo offsets are usually large or negative."""
    fc, fb = fingerprint(cw), fingerprint(bw)
    for i, (a, b) in enumerate(zip(cw, bw)):
        if a == b or fc[i] == fb[i]:
            continue
        if (a >> 26) in E.LOADSTORE_OPS:
            ia, ib = E.simm16(a), E.simm16(b)
            if 0 <= ia < 0x1000 and 0 <= ib < 0x1000:
                return False
    return True


def boot_funcs():
    data = open(BOOT_ELF, "rb").read()
    fs = json.load(open(os.path.join(BOOT, "ref", "funcs.json")))
    names = {}
    pat = re.compile(r"^\s*//\s*FUNC\s+(0x01[0-9A-Fa-f]{6})\s+(.*)$")
    for sub in ("src", "sdk", "runtime", "include"):
        for dp, dn, fn in os.walk(os.path.join(BOOT, sub)):
            for f in fn:
                if not f.endswith((".c", ".cpp", ".cc", ".h", ".S", ".s")):
                    continue
                p = os.path.join(dp, f)
                rel = os.path.relpath(p, BOOT).replace("\\", "/")
                for line in open(p, encoding="utf-8", errors="replace"):
                    m = pat.match(line)
                    if m:
                        names[int(m.group(1), 16)] = (m.group(2).strip(), rel)
    out = []
    for f in fs:
        s, n = f["start"], f["size"]
        o = s - BBASE + BFOFF
        ws = [struct.unpack_from("<I", data, o + 4 * i)[0] for i in range(n // 4)]
        while ws and ws[-1] == 0:
            ws.pop()
        nm, rel = names.get(s, ("F_%08X" % s, None))
        out.append({"start": s, "fp": fingerprint_loose(ws), "words": ws, "name": nm, "file": rel})
    return out


def core_funcs(img):
    fs = json.load(open(os.path.join(paths.REF, "funcs.json")))
    out = []
    for f in fs:
        s, n = int(f["start"], 16), f["size"]
        ws = [img.word(s + 4 * i) for i in range(n // 4)]
        while ws and ws[-1] == 0:
            ws.pop()
        out.append({"start": s, "fp": fingerprint_loose(ws), "words": ws})
    return out


def main():
    img = E.Image()
    B = boot_funcs()
    C = core_funcs(img)
    cidx = {c["start"]: i for i, c in enumerate(C)}
    by_fp_c = collections.defaultdict(list)
    for c in C:
        by_fp_c[c["fp"]].append(c["start"])
    by_fp_b = collections.defaultdict(list)
    for b in B:
        by_fp_b[b["fp"]].append(b["start"])
    match = {}                                  # core -> boot index
    for i, b in enumerate(B):
        if len(b["fp"]) < 3:
            continue
        cs, bs = by_fp_c.get(b["fp"], []), by_fp_b[b["fp"]]
        if len(cs) == 1 and len(bs) == 1:
            match[cs[0]] = (i, "unique")
    # extend anchors over neighbours (object-by-object linking)
    changed = True
    while changed:
        changed = False
        for cstart, (bi, how) in list(match.items()):
            ci = cidx[cstart]
            for step in (-1, 1):
                bj, cj = bi + step, ci + step
                while 0 <= bj < len(B) and 0 <= cj < len(C):
                    if C[cj]["start"] in match:
                        break
                    if B[bj]["fp"] != C[cj]["fp"] or not B[bj]["fp"]:
                        break
                    match[C[cj]["start"]] = (bj, "neighbour")
                    changed = True
                    bj += step
                    cj += step
    rejected = [c for c, (bi, how) in match.items() if not plausible(C[cidx[c]]["words"], B[bi]["words"])]
    for c in rejected:
        print("rejected 0x%08X ~ bootstrap 0x%08X %s (a struct offset differs)" % (c, B[match[c][0]]["start"], B[match[c][0]]["name"]))
        del match[c]
    res = {}
    for cstart, (bi, how) in sorted(match.items()):
        b = B[bi]
        res["0x%08X" % cstart] = {"boot": "0x%08X" % b["start"], "name": b["name"], "file": b["file"], "how": how}
    json.dump(res, open(os.path.join(paths.REF, "libmatch.json"), "w"), indent=1)
    files = collections.Counter(v["file"] for v in res.values())
    print("matched %d core functions to %d bootstrap functions" % (len(res), len({v["boot"] for v in res.values()})))
    for f, n in sorted(files.items(), key=lambda x: (str(x[0]))):
        print("  %-40s %d" % (f, n))


if __name__ == "__main__":
    main()
