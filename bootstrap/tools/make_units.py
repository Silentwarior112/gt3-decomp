"""Split the annotated listing into per-unit files for decompilation.

Reads ref/units.json ({unit: {"ranges": [[lo, hi], ...], ...}}), writes
ref/units/<unit>.txt (annotated disassembly of just those ranges) and
ref/units/<unit>_symbols.csv (the symbol rows for functions in the unit).
Also checks every function belongs to exactly one unit.
"""
import csv, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "..", "ref")
units = json.load(open(os.path.join(REF, "units.json")))
funcs = json.load(open(os.path.join(REF, "funcs.json")))
rows = list(csv.DictReader(open(os.path.join(REF, "symbols.csv"), encoding="utf-8")))
os.makedirs(os.path.join(REF, "units"), exist_ok=True)

def unit_of(a):
    hit = [u for u, spec in units.items() if any(lo <= a < hi for lo, hi in spec["ranges_int"])]
    return hit

for u, spec in units.items():
    spec["ranges_int"] = [(int(lo, 16), int(hi, 16)) for lo, hi in spec["ranges"]]

bad = 0
for f in funcs:
    h = unit_of(f["start"])
    if len(h) != 1:
        print("function 0x%08X in %d units: %s" % (f["start"], len(h), h)); bad += 1
    else:
        for lo, hi in units[h[0]]["ranges_int"]:
            if lo <= f["start"] < hi and f["end"] > hi:
                print("function 0x%08X crosses unit range end 0x%08X" % (f["start"], hi)); bad += 1

# listing: attach header/comment lines to the next instruction address
lines = open(os.path.join(REF, "full.txt"), encoding="utf-8").read().splitlines()
blocks, pending = [], []
for L in lines:
    m = re.match(r"0x([0-9A-F]{8}):", L)
    if m:
        blocks.append((int(m.group(1), 16), pending + [L])); pending = []
    else:
        pending.append(L)

for u, spec in units.items():
    out = []
    for lo, hi in spec["ranges_int"]:
        out.append("")
        out.append("#################### range 0x%08X..0x%08X ####################" % (lo, hi))
        for a, bl in blocks:
            if lo <= a < hi:
                out.extend(bl)
    open(os.path.join(REF, "units", u + ".txt"), "w", encoding="utf-8").write("\n".join(out) + "\n")
    sel = [r for r in rows if any(lo <= int(r["addr"], 16) < hi for lo, hi in spec["ranges_int"])]
    with open(os.path.join(REF, "units", u + "_symbols.csv"), "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, list(rows[0].keys())); w.writeheader(); w.writerows(sel)
    ins = sum((hi - lo) // 4 for lo, hi in spec["ranges_int"])
    print("%-14s funcs=%-4d ins=%-5d" % (u, len(sel), ins))
print("problems:", bad)
sys.exit(1 if bad else 0)
