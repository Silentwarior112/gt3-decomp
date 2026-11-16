"""Progress of the core decompilation, per work unit: functions claimed by FUNC
markers, files, and (with --compile) whether every file of the unit compiles.
Writes ref/progress.json and prints a table.

    python tools/status.py [--compile] [--todo]     --todo: only units not done
"""
import argparse
import json
import os
import subprocess

import check_coverage as cov
import paths
import layout

ap = argparse.ArgumentParser()
ap.add_argument("--compile", action="store_true")
ap.add_argument("--todo", action="store_true")
args = ap.parse_args()

R = paths.REF
units = json.load(open(os.path.join(R, "units.json")))
funcs = json.load(open(os.path.join(R, "funcs.json")))
claimed = cov.claims()
prog_p = os.path.join(R, "progress.json")
prog = json.load(open(prog_p)) if os.path.exists(prog_p) else {}

tot_f = tot_c = tot_i = done_i = 0
rows = []
for u, s in units.items():
    lo, hi = (int(x, 16) for x in s["ranges"][0])
    fs = [f for f in funcs if lo <= int(f["start"], 16) < hi]
    c = sum(1 for f in fs if int(f["start"], 16) in claimed)
    files = layout.files_of_unit(u)
    p = prog.get(u, {})
    if args.compile and files:
        r = subprocess.run(["sh", os.path.join(paths.TOOLS, "cc.sh"), "--unit", u], cwd=paths.ROOT,
                           capture_output=True, text=True)
        p["compile_ok"] = r.returncode == 0
        p["compile_fail"] = [L[5:] for L in r.stdout.splitlines() if L.startswith("FAIL")]
    p.update({"funcs": len(fs), "claimed": c, "files": len(files), "instrs": s["instrs"]})
    p["done"] = c == len(fs) and p.get("compile_ok", False)
    prog[u] = p
    tot_f += len(fs)
    tot_c += c
    tot_i += s["instrs"]
    if c == len(fs):
        done_i += s["instrs"]
    rows.append((u, p))
json.dump(prog, open(prog_p, "w"), indent=1)
for u, p in rows:
    if args.todo and p["done"]:
        continue
    cs = "-" if "compile_ok" not in p else ("ok" if p["compile_ok"] else "FAIL " + " ".join(p.get("compile_fail", [])[:3]))
    print("%s  %4d/%-4d funcs  %2d files  compile %s" % (u, p["claimed"], p["funcs"], p["files"], cs))
print("total: %d/%d functions claimed (%.1f%%), %d/%d units complete, %.1f%% of instructions in complete units" % (
    tot_c, tot_f, 100.0 * tot_c / tot_f, sum(1 for u, p in rows if p["claimed"] == p["funcs"]), len(rows),
    100.0 * done_i / tot_i))
