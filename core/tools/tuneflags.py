"""Measure which compiler flags each source file was built with: score every
file under each candidate flag set and keep the one under which it matches best.

    python tools/tuneflags.py              measure, and print the per-file choices
    python tools/tuneflags.py --write      also rewrite ref/compile_flags.json

Every candidate has the div_nops assembler pass (tools/mcc.py).  A file is
ranked by its functions exact with data references, then exact, then matched
bytes (less 4 per wrong data reference): -G8 can change only where short
strings live (.sdata), which the data references show.
Candidates: -G0 / -G8 (small data: game code reads $gp-relative data, Sony's
libraries do not), with and without -ffast-math, and for C++ with and
without -fno-exceptions.  A file keeps the default flags (the last rule of
ref/compile_flags.json) unless another set matches more bytes.  Rules that
are not plain measurements (they carry "evidence") are kept as they are.
Output: build/tuneflags/choices.json.
"""
import argparse
import collections
import json
import os
import sys

import mcc
import paths
import score

DEFAULT = ["-O2", "-G0", "-ffast-math"]
CANDIDATES = [DEFAULT, ["-O2", "-G8", "-ffast-math"], ["-O2", "-G0"], ["-O2", "-G8"]]
CXX_EXTRA = [["-O2", "-G0", "-ffast-math", "-fno-exceptions"], ["-O2", "-G8", "-ffast-math", "-fno-exceptions"]]
OUT = os.path.join(paths.ROOT, "build", "tuneflags")


def measure(flags, jobs):
    """{file: (matched bytes, exact functions)} with every file under `flags`."""
    tag = "_".join(f.strip("-") for f in flags)
    score.OUT = os.path.join(OUT, tag)
    mcc._rules = [{"glob": "*", "flags": flags, "asm_pass": "div_nops"}]
    rels, records, failed, _ = score.run([], jobs)
    per = collections.defaultdict(lambda: [0, 0, 0])      # exact with data, exact, matched bytes
    for r in records:
        per[r["file"]][0] += 1 if r.get("exact_with_data") else 0
        per[r["file"]][1] += 1 if r["exact"] else 0
        per[r["file"]][2] += r["mbytes"] - 4 * r.get("data_refs_bad", 0)
    for f in failed:
        per[f] = [-1, -1, -1]
    return dict(per)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--apply", action="store_true", help="write ref/compile_flags.json from the last measurement")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    if a.apply:
        choice = json.load(open(os.path.join(OUT, "choices.json")))["choice"]
        write_rules(choice)
        return 0
    results = {}
    for flags in CANDIDATES + CXX_EXTRA:
        print("measuring %s" % " ".join(flags), flush=True)
        results[" ".join(flags)] = measure(flags, a.jobs)
    files = sorted(results[" ".join(DEFAULT)])
    choice, gain = {}, collections.Counter()
    for f in files:
        best, best_v = " ".join(DEFAULT), results[" ".join(DEFAULT)].get(f, [0, 0, 0])
        for k, per in results.items():
            if "-fno-exceptions" in k and not f.endswith((".cpp", ".cc")):
                continue
            v = per.get(f, [-1, -1, -1])
            if tuple(v) > tuple(best_v):
                best, best_v = k, v
        choice[f] = best
        gain[best] += 1
    json.dump(dict(choice=choice, results={k: v for k, v in results.items()}),
              open(os.path.join(OUT, "choices.json"), "w"), indent=1)
    print("files per best flag set:")
    for k, n in gain.most_common():
        print("  %4d  %s" % (n, k))
    tot = {k: sum(max(v[0], 0) for v in per.values()) for k, per in results.items()}
    best_total = sum(max(results[choice[f]][f][0], 0) for f in files)
    print("functions exact with data: default %d, per-file best %d (+%.1f%%)" % (
        tot[" ".join(DEFAULT)], best_total, 100.0 * (best_total - tot[" ".join(DEFAULT)]) / max(tot[" ".join(DEFAULT)], 1)))
    if a.write:
        write_rules(choice)
    return 0


def write_rules(choice):
    cf = json.load(open(mcc.FLAGS))
    kept = [r for r in cf["rules"] if r.get("evidence")]
    have = {r["glob"] for r in kept}
    rules = kept + [{"glob": f, "flags": choice[f].split(), "asm_pass": "div_nops", "measured": True}
                    for f in sorted(choice) if choice[f] != " ".join(DEFAULT) and f not in have]
    rules.append({"glob": "src/*", "flags": DEFAULT, "asm_pass": "div_nops"})
    cf["rules"] = rules
    with open(mcc.FLAGS, "w", encoding="utf-8") as fh:
        json.dump(cf, fh, indent=1)
    print("wrote %s: %d rules" % (mcc.FLAGS, len(rules)))


if __name__ == "__main__":
    sys.exit(main())
