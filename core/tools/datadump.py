#!/usr/bin/env python3
"""The original's data at an address, annotated for writing it as C: each
word with what it is as a pointer (a function's marker name, a named
variable + offset, the string it points at), as a float, and the names
declared at each address (variable boundaries).

    python tools/datadump.py 0x00308DB0 0x90        0x90 bytes from 0x00308DB0
    python tools/datadump.py g_00308B00 0x40        from a declared name
    python tools/datadump.py 0x00314120 --str 20    20 strings from there
    python tools/datadump.py 0x0034FA00 0x10 --floats
    python tools/datadump.py 0x002FC4B0 0x20 --halves   every word as two u16 too
"""
import argparse
import bisect
import collections
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths                                            # noqa: E402
import datalayout                                       # noqa: E402
import funcmap                                          # noqa: E402
import hybrid                                           # noqa: E402

VRAM, ROM = datalayout.VRAM, datalayout.ROM


def rd(a, n):
    img = datalayout.image()
    return img[a - VRAM + ROM:a - VRAM + ROM + n]


def cstr(a, limit=60):
    if not (0x0028E760 <= a < 0x003531E3):
        return None
    b = rd(a, limit + 1)
    end = b.find(b"\x00")
    if end <= 0:
        return None
    s = b[:end]
    if all(0x20 <= c < 0x7F or c in (9, 10) for c in s):
        return s.decode("latin-1")
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addr")
    ap.add_argument("size", nargs="?", default="0x40")
    ap.add_argument("--str", type=int, metavar="N", help="list N strings from addr")
    ap.add_argument("--floats", action="store_true", help="show every word as a float first")
    ap.add_argument("--halves", action="store_true", help="show every word as two u16 (low, high) too")
    a = ap.parse_args()
    hybrid.TC_BIN = paths.ee_bin(None)
    names = collections.defaultdict(set)
    for src in (hybrid.declared(), hybrid.table_names(), hybrid.vtables()):
        for n, v in src.items():
            if v is not None:
                names[v].add(n)
    by_name = {n: v for v, ns in names.items() for n in ns}
    start = by_name.get(a.addr)
    if start is None:
        start = int(a.addr, 16)
    funcs = {}
    for rel, ms in funcmap.markers().items():
        for _, fa, nm in ms:
            funcs[fa] = (nm or "").split("(")[0].strip() or "F_%08X" % fa
    fstarts = sorted(funcs)
    nsorted = sorted(names)

    if a.str:
        p, k = start, 0
        while k < a.str:
            s = cstr(p, 200)
            if s is None:
                p += 1
                continue
            print("0x%08X %s" % (p, repr(s)))
            p += len(s.encode("latin-1")) + 1
            k += 1
        return 0

    size = int(a.size, 0)
    for p in range(start & ~3, start + size, 4):
        w = struct.unpack("<I", rd(p, 4))[0]
        here = ", ".join(sorted(names.get(p, ()))) or ""
        f = struct.unpack("<f", struct.pack("<I", w))[0]
        what = []
        small_halves = w != 0 and (w & 0xFFFF) < 0x100 and (w >> 16) < 0x100      # a u16 table, not a pointer
        if a.halves or small_halves:
            what.append("u16 %d, %d" % (w & 0xFFFF, w >> 16))
        if small_halves:
            pass
        elif 0x00100000 <= w < 0x0028E760:
            i = bisect.bisect_right(fstarts, w) - 1
            if i >= 0:
                what.append("&%s%s" % (funcs[fstarts[i]], "" if fstarts[i] == w else "+0x%X" % (w - fstarts[i])))
        elif 0x0028E760 <= w < 0x004B2C0C:
            j = bisect.bisect_right(nsorted, w) - 1
            if j >= 0 and w - nsorted[j] < 0x10000:
                what.append("&%s%s" % (sorted(names[nsorted[j]])[0], "" if nsorted[j] == w else "+0x%X" % (w - nsorted[j])))
            s = cstr(w)
            if s is not None:
                what.append('"%s"' % s[:40])
        if a.floats or (w and 0x30000000 <= (w & 0x7FFFFFFF) < 0x4E000000 and not what):
            what.append("%gf" % f)
        b = rd(p, 4)
        txt = "".join(chr(c) if 0x20 <= c < 0x7F else "." for c in b)
        print("0x%08X %08X %s  %-40s %s" % (p, w, txt, " ".join(what), ("<- " + here) if here else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
