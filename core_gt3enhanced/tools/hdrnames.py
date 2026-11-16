#!/usr/bin/env python3
"""The strings (type names, inline functions' literals) an object emits when
its file includes only the given headers, in order: which classes a header
makes a file see (ref/DATA.md, the include work).  The u001 pilot's
build/scratch/u001/dp/hdrnames.py.

    python tools/hdrnames.py game/race/core/T153_race_course.h [more headers ...]   one file, in this order
    python tools/hdrnames.py --each game/race/core/T153_race_course.h game/...      one file per header
    python tools/hdrnames.py --flags-of src/game/race/core/T121_race_entry.cpp h1 h2   that file's flags

With tools/tnames.py (the type names in each original file's ranges) the
include set of a file becomes a lookup.  Default flags: -O2 -G8 -ffast-math.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mcc                                              # noqa: E402
import paths                                            # noqa: E402

OUT = os.path.join(paths.ROOT, "build", "scratch", "hdrnames")


def names(headers, tag, flags_of=None):
    os.makedirs(OUT, exist_ok=True)
    src = os.path.join(OUT, "%s_%d.cpp" % (tag, os.getpid()))
    with open(src, "w") as f:
        f.write("".join('#include "%s"\n' % h for h in headers) + "int hn_dummy(int x) { return x; }\n")
    s = src[:-4] + ".s"
    rel = os.path.relpath(src, paths.ROOT).replace(os.sep, "/")
    extra = () if flags_of else ("-O2", "-G8", "-ffast-math")
    ok, msg = mcc.compile(rel, s, asm=True, extra=extra, flags_of=flags_of)
    out = []
    if not ok:
        out = ["ERROR: " + (msg.strip().splitlines() or ["?"])[0]]
    else:
        # each literal quoted, with its section (an empty `""` in .sdata shows:
        # the u052 agent's hn.py)
        sec = ".text"
        for L in open(s, encoding="latin-1"):
            ms = re.match(r"\s*\.(rdata|sdata|data|text|lit4|lit8)\b|\s*\.section\s+([\w.]+)", L)
            if ms:
                sec = "." + ms.group(1) if ms.group(1) else ms.group(2)
                continue
            m = re.match(r'\s*\.ascii\s+"(.*)\\000"', L)
            if m:
                out.append('%s"%s"' % ("" if sec in (".rdata", ".rodata") else sec + ":", m.group(1)))
    for p in (src, s):
        if os.path.exists(p):
            os.remove(p)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("headers", nargs="+")
    ap.add_argument("--each", action="store_true", help="one file per header")
    ap.add_argument("--flags-of", help="compile with this source file's flags (ref/compile_flags.json)")
    a = ap.parse_args()
    if a.each:
        for h in a.headers:
            print("%-40s %s" % (h, " ".join(names([h], "each", a.flags_of))))
    else:
        for n in names(a.headers, "all", a.flags_of):
            print(n)
    return 0


if __name__ == "__main__":
    sys.exit(main())
