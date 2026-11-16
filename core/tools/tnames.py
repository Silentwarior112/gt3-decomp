#!/usr/bin/env python3
"""The type names (egcs's mangled class names) in each original file's
.rodata / .sdata ranges, in address order: which classes the original file
saw (ref/DATA.md; with tools/hdrnames.py, the include set becomes a lookup).
The u001 pilot's build/scratch/u001/dp/tnames.py.

    python tools/tnames.py src/game/race/core/T121_race_entry.cpp [...]
    python tools/tnames.py --unit u042
    python tools/tnames.py --json out.json           every file
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import datalayout as dl                                 # noqa: E402

TN = re.compile(r"^(?:\d+[A-Za-z_]\w*|Q\d\w+|t\d+\w+)$")


def file_names():
    img = dl.image()
    fr = dl.file_ranges()
    out = {}
    for rel in dl.tu_order():
        names = []
        for area in (".rodata", ".sdata"):
            r = fr.get(rel, {}).get(area)
            if not r:
                continue
            lo, hi = r
            a = (lo + 7) & ~7
            while a < hi:
                off = a - dl.VRAM + dl.ROM
                e = img.find(b"\0", off, off + 200)
                if e > off:
                    s = img[off:e]
                    # after a NUL, or after a base-class table's 0x40 flag byte
                    if all(0x20 < c < 0x7F for c in s) and img[off - 1] in (0, 0x40):
                        t = s.decode()
                        if TN.match(t) and len(t) > 2:
                            names.append(t)
                a += 8
        out[rel] = names
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--unit")
    ap.add_argument("--json")
    a = ap.parse_args()
    out = file_names()
    if a.json:
        json.dump(out, open(a.json, "w"), indent=0)
    rels = list(a.files)
    if a.unit:
        import layout
        rels += [r for r in layout.files_of_unit(a.unit) if r in out]
    for rel in rels:
        print("%s: %s" % (rel, " ".join(out.get(rel.replace("\\", "/"), [])) or "(none)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
