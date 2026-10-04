"""Make div.s (insn divsf3) ineligible for delay slots in SCE ee-gcc 2.9-ee-991111b.

    python tools/patch_divs.py            # (re)creates sdk/ee-gcc-2.9-991111b-divs

Copies the pristine compiler sdk/ee-gcc-2.9-991111b to
sdk/ee-gcc-2.9-991111b-divs and patches its cc1.exe and cc1plus.exe.  Needs
`pip install capstone`.  Only the matching tools use the patched copy.

Why: the retail compiler never puts a div.s in a delay slot, and every retail
div.s has two nops in front of it (the R5900 FPU hazard padding that
tools/mcc.py's fix_fpu_div_hazard adds).  So the retail divsf3 pattern was
not delay-slot eligible, most likely a later SCE revision that emitted the
nops itself (length 3).  Our 991111b has a plain one-instruction div.s.  reorg then puts
it in delay slots, and startup_screen_thread comes out different.  An asm
div.s is no substitute: reorg's scans stop at an asm, whereas the retail
reorg looked past the div.s (it stole the `addu` out of the following `b`'s
slot into the earlier `beqz`'s slot).

What: in eligible_for_delay and eligible_for_annul_false (insn-attrtab.c,
compiled at -O0 in these binaries), every candidate switch is a byte table
indexed by insn code + 1 plus a dword table of case targets.  The divsf3
entry (code 97) is moved from the "return 1" / "evaluate" case to the
function's own "return 0" case.  That is 4 bytes per executable and nothing
else changes: scheduling, output templates and the nop padding pass stay as
they were.  The tables are found by pattern (see find_tables) and every
patched byte is checked, so a different build fails loudly instead of being
patched wrongly.
"""
import os, re, shutil, struct, sys, bisect

import paths

SRC = paths.sdk("ee-gcc-2.9-991111b")
DST = paths.sdk("ee-gcc-2.9-991111b-divs")
LIBDIR = os.path.join("lib", "gcc-lib", "ee", "2.9-ee-991111")
CODE_DIVSF3 = 97        # insn code of divsf3 in this md (RTL dumps: "{divsf3}")


class PE:
    def __init__(self, data):
        self.b = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        assert data[pe:pe + 4] == b"PE\0\0"
        nsec = struct.unpack_from("<H", data, pe + 6)[0]
        optsz = struct.unpack_from("<H", data, pe + 20)[0]
        opt = pe + 24
        self.imagebase = struct.unpack_from("<I", data, opt + 28)[0]
        self.secs = []
        sh = opt + optsz
        for _ in range(nsec):
            name = data[sh:sh + 8].rstrip(b"\0").decode()
            vsize, va, rsize, raw = struct.unpack_from("<IIII", data, sh + 8)
            self.secs.append((name, va, vsize, raw, rsize))
            sh += 40

    def va2off(self, v):
        rva = v - self.imagebase
        for _, va, vs, raw, rs in self.secs:
            if va <= rva < va + max(vs, rs):
                return raw + rva - va
        raise ValueError(hex(v))


def find_tables(data):
    """[(byte_table_offset, target_kind_of_code, ret0_index)] for every
    candidate switch of eligible_for_delay / eligible_for_annul_*."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    p = PE(data)
    name, va, vs, raw, rs = [s for s in p.secs if s[0] == ".text"][0]
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.skipdata = True
    ins = [(i.address, i.mnemonic, i.op_str) for i in md.disasm(data[raw:raw + rs], p.imagebase + va)]
    addrs = [i[0] for i in ins]

    def first(a):
        k = bisect.bisect_left(addrs, a)
        return ins[k]

    def kind(t):
        _, m, o = first(t)
        return "ret1" if (m, o) == ("mov", "eax, 1") else "ret0" if (m, o) == ("xor", "eax, eax") else "eval"

    # eligible_for_*: push ebp; mov ebp, esp; sub esp, N; cmp [ebp+0xc], 1; jl; call abort
    funcs = [ins[k - 2][0] for k in range(2, len(ins) - 2)
             if ins[k][1:] == ("cmp", "dword ptr [ebp + 0xc], 1") and ins[k - 1][1] == "sub"
             and ins[k - 2][1:] == ("mov", "ebp, esp") and ins[k + 1][1] == "jl" and ins[k + 2][1] == "call"]
    out = []
    for f in funcs:
        k0 = bisect.bisect_left(addrs, f)
        k1 = bisect.bisect_left(addrs, min([g for g in funcs if g > f] + [f + 0x8000]))
        seq = ins[k0:k1]
        for j in range(len(seq) - 1):
            mm = re.match(r"(\w)l, byte ptr \[e\w\w \+ (0x[0-9a-f]+)\]$", seq[j][2])
            mj = re.match(r"dword ptr \[e(\w)x\*4 \+ (0x[0-9a-f]+)\]$", seq[j + 1][2])
            if seq[j][1] != "mov" or seq[j + 1][1] != "jmp" or not mm or not mj or mm.group(1) != mj.group(1):
                continue
            bound = None
            for q in range(j - 1, max(0, j - 8), -1):
                mc = re.match(r"dword ptr \[ebp - 0x[0-9a-f]+\], (0x[0-9a-f]+)$", seq[q][2])
                if seq[q][1] == "cmp" and mc:
                    bound = int(mc.group(1), 16)
                    break
            # candidate switches cover every insn code (code + 1 <= 0x215 here)
            if bound != 0x215:
                continue
            bt = p.va2off(int(mm.group(2), 16))
            dt = p.va2off(int(mj.group(2), 16))
            tbl = data[bt:bt + bound + 1]
            nt = max(tbl) + 1
            targets = struct.unpack_from("<%dI" % nt, data, dt)
            zero = [x for x in range(nt) if kind(targets[x]) == "ret0"]
            out.append((bt + CODE_DIVSF3 + 1, kind(targets[tbl[CODE_DIVSF3 + 1]]), zero))
    return out


def patch(data):
    data = bytearray(data)
    tables = find_tables(bytes(data))
    if len(tables) != 4:
        sys.exit("expected 4 candidate switches (3 delay types + annul-false), found %d" % len(tables))
    for off, kind, zero in tables:
        if len(zero) != 1:
            sys.exit("no unique 'return 0' case in the switch at file offset %#x" % off)
        if kind == "ret0":
            continue                                    # already patched
        data[off] = zero[0]
    for off, kind, zero in find_tables(bytes(data)):
        assert kind == "ret0", hex(off)
    return bytes(data)


def main():
    if not os.path.isdir(SRC):
        sys.exit("missing " + SRC)
    if os.path.isdir(DST):
        shutil.rmtree(DST)
    shutil.copytree(SRC, DST)
    for exe in ("cc1.exe", "cc1plus.exe"):
        path = os.path.join(DST, LIBDIR, exe)
        old = open(path, "rb").read()
        new = patch(old)
        n = sum(1 for a, b in zip(old, new) if a != b)
        open(path, "wb").write(new)
        print("%s: %d bytes changed" % (exe, n))
    print("wrote", DST)


if __name__ == "__main__":
    main()
