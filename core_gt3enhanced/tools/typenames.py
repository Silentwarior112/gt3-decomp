#!/usr/bin/env python3
"""Compile a (scratch) C++ file with egcs (tools/mcc.py) and print its data
sections' strings in order (type names, literals), with offsets.

    python tools/typenames.py build/scratch/<u>/t1.cpp [-- -G8]

(Which type names a header brings: a scratch file that includes only it.
The u040 pilot's tn.py.)
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import paths                                            # noqa: E402
ROOT = paths.ROOT
os.chdir(ROOT)
import mcc                                              # noqa: E402
from elfobj import ElfObject                            # noqa: E402

STR = re.compile(rb"[\x20-\x7e\t\n]{2,}\x00")


def main():
    src = sys.argv[1]
    extra = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    rel = os.path.relpath(src, ROOT).replace("\\", "/")
    # (never next to the source: a tmp_*.o in src/ is picked up as a file of the unit)
    tmp_dir = os.path.join(ROOT, "build", "scratch", "typenames")
    os.makedirs(tmp_dir, exist_ok=True)
    out = os.path.join(tmp_dir, "tmp_%d_%s.o" % (os.getpid(), os.path.basename(src)))
    ok, msg = mcc.compile(rel, out, extra=tuple(extra))
    if not ok:
        print(msg)
        return 1
    obj = ElfObject(out)
    for sec in obj.sections:
        if sec.name in (".rodata", ".sdata", ".data") and sec.size and sec.type != 8:
            print("== %s %d" % (sec.name, sec.size))
            buf = bytes(sec.data)
            for m in STR.finditer(buf):
                s = m.start()
                if s == 0 or buf[s - 1] == 0:
                    print("  +%04X %s" % (s, m.group(0)[:-1].decode("latin-1")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
