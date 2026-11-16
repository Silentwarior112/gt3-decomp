"""Coverage check: every function in ref/funcs.json must be claimed exactly once in the
decomp tree by a marker line `// FUNC 0x00XXXXXX <name>` (also accepted:
`/* FUNC 0x00XXXXXX ...`), at line start ignoring whitespace.

Usage: python tools/check_coverage.py                 whole tree, all functions
       python tools/check_coverage.py --unit <name>   only that unit's functions
                                                      (ranges from ref/units.json)
"""
import collections
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REF = os.path.join(ROOT, "ref")

PAT = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+(0x00[0-9A-Fa-f]{6})\b")


def claims(roots=("src", "include")):
    claimed = collections.defaultdict(list)
    for sub in roots:
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, sub)):
            for fn in filenames:
                if not fn.endswith((".c", ".cpp", ".cc", ".h", ".hpp", ".s", ".S", ".inc")):
                    continue
                p = os.path.join(dirpath, fn)
                rel = os.path.relpath(p, ROOT).replace("\\", "/")
                for line in open(p, encoding="utf-8", errors="replace"):
                    m = PAT.match(line)
                    if m:
                        claimed[int(m.group(1), 16)].append(rel)
    return claimed


def main():
    args = sys.argv[1:]
    unit = args[1] if len(args) >= 2 and args[0] == "--unit" else None
    funcs = json.load(open(os.path.join(REF, "funcs.json")))
    want = {int(f["start"], 16): f for f in funcs}
    if unit:
        spec = json.load(open(os.path.join(REF, "units.json")))[unit]
        rng = [(int(lo, 16), int(hi, 16)) for lo, hi in spec["ranges"]]
        want = {a: f for a, f in want.items() if any(lo <= a < hi for lo, hi in rng)}
    claimed = claims()
    allf = {int(f["start"], 16) for f in funcs}
    missing = [a for a in want if a not in claimed]
    dupes = {a: fs for a, fs in claimed.items() if a in want and len(fs) > 1}
    if unit:
        lo_hi = [(int(lo, 16), int(hi, 16)) for lo, hi in spec["ranges"]]
        unknown = [a for a in claimed if a not in allf and any(lo <= a < hi for lo, hi in lo_hi)]
    else:
        unknown = [a for a in claimed if a not in allf]
    print("%sfunctions: %d  claimed: %d  missing: %d  duplicated: %d  unknown-addr markers: %d" % (
        ("[%s] " % unit) if unit else "", len(want), len([a for a in want if a in claimed]),
        len(missing), len(dupes), len(unknown)))
    for a in sorted(missing)[:400]:
        print("MISSING  0x%08X size=0x%X" % (a, want[a]["size"]))
    for a in sorted(dupes):
        print("DUPE     0x%08X in %s" % (a, ", ".join(sorted(dupes[a]))))
    for a in sorted(unknown):
        print("UNKNOWN  0x%08X in %s  (not a function start in ref/funcs.json)" % (a, ", ".join(sorted(set(claimed[a])))))
    sys.exit(1 if (missing or dupes or unknown) else 0)


if __name__ == "__main__":
    main()
