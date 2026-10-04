"""Rewrite the "files" list of every unit in ref/units.json from where its
functions' FUNC markers actually are (plus the unit's headers/tools entries,
which carry no markers)."""
import json, os, re, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
P = os.path.join(ROOT, "ref", "units.json")
units = json.load(open(P, encoding="utf-8"))

pat = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+(0x01[0-9A-Fa-f]{6})\b")
where = {}
for sub in ("src", "sdk", "runtime", "include"):
    for dp, _, fns in os.walk(os.path.join(ROOT, sub)):
        for fn in fns:
            p = os.path.join(dp, fn)
            rel = os.path.relpath(p, ROOT).replace("\\", "/")
            for line in open(p, encoding="utf-8", errors="replace"):
                m = pat.match(line)
                if m:
                    where[int(m.group(1), 16)] = rel

for name, spec in units.items():
    rng = [(int(lo, 16), int(hi, 16)) for lo, hi in spec["ranges"]]
    files = collections.Counter(f for a, f in where.items() if any(lo <= a < hi for lo, hi in rng))
    extra = [f for f in spec["files"] if (f.startswith("include/") or f.startswith("tools/"))
             and os.path.exists(os.path.join(ROOT, f)) and f not in files]
    new = sorted(files) + extra
    if new != spec["files"]:
        print("%-14s %s -> %s" % (name, spec["files"], new))
    spec["files"] = new

json.dump(units, open(P, "w", encoding="utf-8"), indent=2)
