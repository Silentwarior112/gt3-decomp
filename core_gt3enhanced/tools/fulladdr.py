#!/usr/bin/env python3
"""Retail address -> the full C link's address (and back), from
build/full/link/addrmap.json (tools/fulllink.py) and core.elf's symbols.

    python tools/fulladdr.py 0x00308B00 0x00350B40     retail -> new
    python tools/fulladdr.py --new 0x003091F8           new -> retail
    python tools/fulladdr.py --cmp 0x00308B00 0x40      retail vs new bytes
"""
import argparse
import bisect
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import datalayout                                       # noqa: E402
import hybrid                                           # noqa: E402
import paths                                            # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class AddrMap:
    def __init__(self, build=os.path.join(ROOT, "build", "full")):
        link = os.path.join(build, "link")
        m = json.load(open(os.path.join(link, "addrmap.json")))
        self.elf_path = os.path.join(link, "core.elf")
        hybrid.TC_BIN = paths.ee_bin(None)
        self.sym = {n: v for v, t, n in hybrid.nm(self.elf_path) if v is not None}
        self.code = m["code"]
        self.code_lo = [a for a, _ in self.code]
        self.data = m["data"]
        self.data_lo = [d[0] for d in self.data]
        self._elf = None

    def eval(self, e):
        s, _, off = e.partition(" + ")
        return self.sym[s.strip()] + (int(off, 16) if off else 0)

    def new(self, a):
        """The new address of retail address a (None when unknown)."""
        if 0x00100000 <= a < 0x0028E760:
            i = bisect.bisect_right(self.code_lo, a) - 1
            if i < 0:
                return None
            f, e = self.code[i]
            return self.eval(e) + (a - f) if e else None
        i = bisect.bisect_right(self.data_lo, a) - 1
        if i < 0:
            return None
        lo, hi, s, k = self.data[i]
        if a > hi:
            return None
        return self.sym[s] + (a - lo)

    def piece(self, a):
        i = bisect.bisect_right(self.data_lo, a) - 1
        return self.data[i] if i >= 0 else None

    def retail(self, n):
        """The retail address of new address n (data pieces only)."""
        best = None
        for lo, hi, s, k in self.data:
            v = self.sym.get(s)
            if v is not None and v <= n < v + (hi - lo):
                best = lo + (n - v)
        return best

    def elf(self):
        if self._elf is None:
            self._elf = open(self.elf_path, "rb").read()
        return self._elf

    def new_bytes(self, n, size):
        """Bytes at new address n from core.elf's program headers."""
        e = self.elf()
        phoff, phnum = struct.unpack_from("<I", e, 0x1C)[0], struct.unpack_from("<H", e, 0x2C)[0]
        for i in range(phnum):
            t, off, va, pa, fsz, msz = struct.unpack_from("<6I", e, phoff + i * 32)
            if t == 1 and va <= n < va + msz:
                k = n - va
                b = e[off + k:off + min(fsz, k + size)] if k < fsz else b""
                return b + bytes(size - len(b))
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addr", nargs="+")
    ap.add_argument("--new", action="store_true", help="the arguments are new addresses")
    ap.add_argument("--cmp", action="store_true", help="ADDR SIZE: retail bytes vs the new link's")
    a = ap.parse_args()
    m = AddrMap()
    vals = [int(x, 16) for x in a.addr]
    if a.cmp:
        lo, size = vals[0], vals[1] if len(vals) > 1 else 0x40
        img = datalayout.image()
        for p in range(lo, lo + size, 16):
            r = img[p - 0x00100000 + 0x1000:p - 0x00100000 + 0x1010]
            n = m.new(p)
            nb = m.new_bytes(n, 16) if n is not None else None
            print("%08X %s | %s %s" % (p, r.hex(" "), ("%08X" % n) if n else "--------",
                                       nb.hex(" ") if nb else "?"))
        return 0
    for v in vals:
        if a.new:
            print("new 0x%08X -> retail %s" % (v, "0x%08X" % m.retail(v) if m.retail(v) is not None else "?"))
        else:
            n = m.new(v)
            p = m.piece(v) if not (0x00100000 <= v < 0x0028E760) else None
            print("retail 0x%08X -> new %s %s" % (v, "0x%08X" % n if n is not None else "?",
                                                 "(%s %s)" % (p[2], p[3]) if p else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
