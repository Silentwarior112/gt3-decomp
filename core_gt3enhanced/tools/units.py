"""Split the main executable into work units for the decompilation: ref/units.json.

A work unit is an address-contiguous run of whole translation units (ref/tus.json)
of about TARGET instructions.  A translation unit larger than MAXU is cut into
parts at function boundaries; in code with no FDEs (C and Sony libraries) the cut
prefers a function that follows alignment padding, which usually starts an object.

    python tools/units.py [--target N] [--max N]
"""
import argparse
import collections
import csv
import json
import math
import os

import paths

ap = argparse.ArgumentParser()
ap.add_argument("--target", type=int, default=3000)
ap.add_argument("--max", type=int, default=4500)
args = ap.parse_args()
TARGET, MAXU = args.target, args.max

R = paths.REF
funcs = json.load(open(os.path.join(R, "funcs.json")))
tus = json.load(open(os.path.join(R, "tus.json")))
syms = {int(r["addr"], 16): r for r in csv.DictReader(open(os.path.join(R, "symbols.csv"), encoding="utf-8"))}
lm = json.load(open(os.path.join(R, "libmatch.json")))

by_tu = collections.defaultdict(list)
for f in funcs:
    by_tu[f["tu"]].append(f)
for v in by_tu.values():
    v.sort(key=lambda f: int(f["start"], 16))
order = sorted((u for u in tus if u["nfuncs"]), key=lambda u: int(u["lo"], 16))


def ninstr(fs):
    return sum(f["size"] for f in fs) // 4


pieces = []          # (tu_id, kind, [funcs])
for u in order:
    fs = by_tu[u["id"]]
    n = ninstr(fs)
    if n <= MAXU:
        pieces.append((u["id"], u["kind"], fs))
        continue
    parts = math.ceil(n / TARGET)
    goal = n / parts
    cur, acc = [], 0
    for i, f in enumerate(fs):
        sz = f["size"] // 4
        cut = acc >= goal * 0.8 and (u["kind"] == "fde" or f["how"] == "after-pad" or acc >= goal * 1.25)
        if cur and cut and len(pieces) >= 0:
            pieces.append((u["id"], u["kind"], cur))
            cur, acc = [], 0
        cur.append(f)
        acc += sz
    if cur:
        pieces.append((u["id"], u["kind"], cur))

units = []
cur, acc = [], 0
for p in pieces:
    n = ninstr(p[2])
    big = n > TARGET * 0.8
    if cur and (acc + n > TARGET or big or cur[-1][1] != p[1]):
        units.append(cur)
        cur, acc = [], 0
    cur.append(p)
    acc += n
    if big:
        units.append(cur)
        cur, acc = [], 0
if cur:
    units.append(cur)

out = collections.OrderedDict()
for i, u in enumerate(units):
    fs = [f for p in u for f in p[2]]
    lo, hi = fs[0]["start"], fs[-1]["end"]
    classes = collections.Counter()
    named = boot = 0
    for f in fs:
        a = int(f["start"], 16)
        s = syms.get(a)
        if s:
            named += 1
            if s["class"]:
                classes[s["class"]] += 1
            elif "::" in s["name"]:
                classes[s["name"].split("(")[0].rsplit("::", 1)[0]] += 1
        if f["start"] in lm:
            boot += 1
    tl = []
    for p in u:
        if not tl or tl[-1] != p[0]:
            tl.append(p[0])
    split = [p[0] for p in u if len(by_tu[p[0]]) != len(p[2])]
    out["u%03d" % i] = {
        "ranges": [[lo, hi]],
        "tus": tl,
        "split_tus": sorted(set(split)),
        "lang": "c" if u[0][1] == "gap" else "cpp",
        "nfuncs": len(fs),
        "instrs": ninstr(fs),
        "named": named,
        "boot": boot,
        "classes": [c for c, n in classes.most_common(6)],
    }
json.dump(out, open(os.path.join(R, "units.json"), "w"), indent=1)
sizes = [u["instrs"] for u in out.values()]
print("%d units (target %d, max %d): instrs min %d max %d mean %d; c units %d" % (
    len(out), TARGET, MAXU, min(sizes), max(sizes), sum(sizes) // len(sizes),
    sum(1 for u in out.values() if u["lang"] == "c")))
