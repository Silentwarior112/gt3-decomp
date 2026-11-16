"""Annotated listings of the main executable, in the format the ref/ files use:

    ========== F_00101A40 __tf10GameObject ==========
    ; tu T012  size 0x30  callers 3  data xrefs: 0x00308780 (_vt$10GameObject)
    0x00101A48: 24428780  addiu    $v0, $v0, -0x7880        ; &0x00308780 _vt$10GameObject

Notes: `; call NAME` (jal), `; tail-> NAME` (j to a function), `; -> 0xADDR` (branch),
`; &ADDR [name] 'string'` (lui/addiu, lui/ori pairs), `; [ADDR] name` (load/store through a
lui base), `; gp+-N = ADDR [name] (value)` ($gp-relative; $gp = 0x003579F0),
`; switch table @ADDR: case i -> target` at jr $reg, `; EH ref from 0xDATA` on
instructions that exception tables point at.
"""
import csv
import json
import os
import struct

import coreelf as E
import paths


class Ctx:
    def __init__(self, img=None):
        self.img = img or E.Image()
        R = paths.REF
        self.funcs = json.load(open(os.path.join(R, "funcs.json")))
        self.fidx = {int(f["start"], 16): f for f in self.funcs}
        self.starts = set(self.fidx)
        self.names = {}
        p = os.path.join(R, "symbols.csv")
        if os.path.exists(p):
            for r in csv.DictReader(open(p, encoding="utf-8")):
                self.names[int(r["addr"], 16)] = r["name"]
        self.dnames = {}
        p = os.path.join(R, "data_symbols.csv")
        if os.path.exists(p):
            for r in csv.DictReader(open(p, encoding="utf-8")):
                self.dnames[int(r["addr"], 16)] = r["name"]
        self.jt = {}
        p = os.path.join(R, "jumptab_targets.json")
        if os.path.exists(p):
            self.jt = {int(k, 16): int(v, 16) for k, v in json.load(open(p)).items()}
        # code pointers from data that are not function starts or jump-table targets: EH refs
        self.ehref = {}
        img = self.img
        for a in range(E.TEXT_HI, E.IMG_END - 3, 4):
            if E.EH_FRAME_LO <= a < E.EH_FRAME_HI:
                continue
            v = img.word(a)
            if img.in_text(v) and v not in self.starts and v not in self.jt:
                self.ehref.setdefault(v, []).append(a)

    def fname(self, a):
        n = self.names.get(a)
        return "F_%08X %s" % (a, n) if n else "F_%08X" % a

    def fshort(self, a):
        return "F_%08X" % a

    def dname(self, a):
        n = self.dnames.get(a)
        if n:
            return n
        return None

    def describe_addr(self, a):
        img = self.img
        if img.in_text(a):
            if a in self.starts:
                return "&" + self.fname(a)
            return "&text 0x%08X" % a
        if not img.in_ram(a):
            return None
        s = "0x%08X" % a
        n = self.dname(a)
        if n:
            s += " " + n
        sec = img.section_of(a)
        if sec and sec != ".data":
            s += " " + sec
        st = img.cstr(a, 2, 80) if img.in_image(a) else None
        if st is not None:
            s += " %r" % st
        return s


def annotate(ctx, lo, hi, sink, header_extra=None):
    img = ctx.img
    regs = {}
    last_load = None
    for va in range(lo, hi, 4):
        x = img.word(va)
        op = x >> 26
        rs, rt = (x >> 21) & 31, (x >> 16) & 31
        simm = E.simm16(x)
        if va in ctx.starts:
            f = ctx.fidx[va]
            sink.append("")
            sink.append("========== %s ==========" % ctx.fname(va))
            info = "; tu %s  size 0x%X  callers %d" % (f["tu"], f["size"], len(f["callers"]))
            if f["data_xrefs"]:
                info += "  data xrefs: " + ", ".join(
                    x2 + ((" (" + ctx.dnames[int(x2, 16)] + ")") if int(x2, 16) in ctx.dnames else "")
                    for x2 in f["data_xrefs"][:6])
            sink.append(info)
            if header_extra:
                for L in header_extra(va):
                    sink.append(L)
            regs = {}
            last_load = None
        if va in ctx.ehref:
            sink.append("; EH ref from %s" % ", ".join("0x%08X" % d for d in ctx.ehref[va][:4]))
        m, o = E.fmt(img, va, lambda t: ctx.fshort(t) if t in ctx.starts else None)
        note = ""
        if op == 0x0F:
            regs[rt] = (x & 0xFFFF) << 16
        elif op in (0x09, 0x19) and rs in regs:
            a = (regs[rs] + simm) & 0xFFFFFFFF
            d = ctx.describe_addr(a)
            if d:
                note = "  ; " + ("&" + d if not d.startswith("&") else d)
            regs[rt] = a
        elif op == 0x0D and rs in regs:
            a = regs[rs] | (x & 0xFFFF)
            d = ctx.describe_addr(a)
            if d:
                note = "  ; &" + d
            regs[rt] = a
        elif op in E.LOADSTORE_OPS:
            if rs == 28:
                a = (E.GP + simm) & 0xFFFFFFFF
                n = ctx.dname(a)
                v = img.word(a & ~3) if img.in_image(a) else None
                extra = ""
                if v is not None:
                    if img.in_text(v) and v in ctx.starts:
                        extra = " (=&%s)" % ctx.fshort(v)
                    else:
                        extra = " (=0x%08X)" % v
                        if op in (0x31,) :      # lwc1: show as float
                            extra += " %g" % struct.unpack("<f", struct.pack("<I", v))[0]
                note = "  ; gp%+#x = 0x%08X%s%s" % (simm, a, (" " + n) if n else "", extra)
                if op == 0x23:
                    last_load = (va, a)
            elif rs in regs:
                a = (regs[rs] + simm) & 0xFFFFFFFF
                d = ctx.describe_addr(a)
                if d:
                    note = "  ; [%s]" % d
                    if op == 0x31 and img.in_image(a):
                        v = img.word(a & ~3)
                        note += " = %g" % struct.unpack("<f", struct.pack("<I", v))[0]
                if op == 0x23:
                    last_load = (va, a)
            # a load into a tracked register kills it
            if op not in (0x28, 0x29, 0x2B, 0x3F, 0x1F, 0x39, 0x3E, 0x2C, 0x2D, 0x2A, 0x2E) and rt in regs:
                regs.pop(rt, None)
        elif op == 0 and (x & 0x3F) in (0x21, 0x2D, 0x25) and ((x >> 11) & 31) in regs and ((x >> 11) & 31) not in (rs, rt):
            regs.pop((x >> 11) & 31, None)
        if op == 3:
            t = (x & 0x03FFFFFF) << 2
            note = "  ; call %s" % ctx.fname(t) if t in ctx.starts else "  ; call 0x%08X" % t
        elif op == 2:
            t = (x & 0x03FFFFFF) << 2
            f = ctx.fidx.get(va)
            if t in ctx.starts and not (lo <= t < hi and t > va - 0x10000 and t not in ctx.starts):
                note = "  ; tail-> %s" % ctx.fname(t)
        elif op == 0 and (x & 0x3F) == 0x08 and rs != 31:
            # jr $reg: a switch?
            if last_load and va - last_load[0] <= 0x20:
                tab = last_load[1]
                tg = []
                b = tab
                while img.in_image(b) and len(tg) < 256:
                    v = img.word(b)
                    if not img.in_text(v) or ctx.jt.get(v) is None:
                        break
                    tg.append(v)
                    b += 4
                if tg:
                    sink.append("0x%08X: %08X  %-8s %s  ; switch table @0x%08X (%d cases)" % (va, x, m, o, tab, len(tg)))
                    for i, t in enumerate(tg):
                        sink.append(";     case %d -> 0x%08X" % (i, t))
                    continue
        sink.append("0x%08X: %08X  %-8s %s%s" % (va, x, m, o, note))


def write_full():
    ctx = Ctx()
    lines = []
    annotate(ctx, E.TEXT_LO, E.TEXT_HI, lines)
    open(os.path.join(paths.REF, "full.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("ref/full.txt: %d lines" % len(lines))


if __name__ == "__main__":
    write_full()
