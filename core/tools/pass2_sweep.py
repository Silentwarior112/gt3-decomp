"""Write the tasks of the second-pass sweep (ref/PASS2_SWEEP.md):
ref/pass2/sweep/<part>/TASK.md per header partition (the partitions of
tools/pass2_integrate.py), with the open items of ref/pass2/after_phase2.md and
the rename proposals of ref/pass2/units/*/notes.md that mention the
partition's translation units, classes or functions.

    python tools/pass2_sweep.py              write all
    python tools/pass2_sweep.py s3           print that partition's launcher message
"""
import csv
import glob
import os
import re
import sys

import paths
import pass2_integrate as pi
import pass2_tasks as pt

ROOT = paths.ROOT.replace("\\", "/")
P2 = os.path.join(paths.REF, "pass2")
SKIP = re.compile(r"(?i)rename table|renames? (from|in) the (task|rename)|applied|calls (per|use|follow)|"
                  r"locals (and|are|were|renamed)|renamed throughout|no locals")


def bullets(path, tag):
    out, cur = [], []
    for L in open(path, encoding="utf-8", errors="replace").read().split("\n") + [""]:
        if not L.strip() or re.match(r"^\s{0,2}[-*] |^#", L):
            if cur:
                out.append((tag, " ".join(x.strip() for x in cur)))
            cur = [L] if L.strip() and not L.startswith("#") else []
        else:
            cur.append(L)
    return out


def items():
    p = os.path.join(P2, "after_phase2.md")
    text = open(p, encoding="utf-8").read().split("\nDone:\n")[0]
    tmp = os.path.join(paths.ROOT, "build", "after_phase2_open.md")
    open(tmp, "w", encoding="utf-8").write(text)
    out = [("cleanup", t) for _, t in bullets(tmp, "x") if not t.startswith(("Proposals from", "# After"))]
    for f in sorted(glob.glob(os.path.join(P2, "units", "*", "notes.md"))):
        u = os.path.basename(os.path.dirname(f))
        for _, t in bullets(f, u):
            low = t.lower()
            if SKIP.search(low) and "propos" not in low:
                continue
            if "propos" in low or (("->" in t or "→" in t) and
                                   ("rename" in low or "should be" in low or "wanted" in low)):
                out.append((u, t))
    return out


def part_keys(gids):
    groups = {g["id"]: g for g in pt.groups}
    hdrs = [h for gid in gids for h in groups[gid]["headers"]]
    keys = set()
    for h in hdrs:
        keys.add(pt.headers[h]["tu"])
        keys.update(c.rsplit("::", 1)[-1] for c in pt.headers[h]["classes"] if len(c.rsplit("::", 1)[-1]) >= 5)
    # function names / F_ addresses declared in these headers now
    hset = set(hdrs)
    for r in csv.DictReader(open(os.path.join(P2, "canonical.csv"), encoding="utf-8")):
        if r["header"] in hset:
            short = r["name"].rsplit("::", 1)[-1]
            if short.startswith("F_") or (len(short) >= 8 and r["kind"] != "global" and "_" in short):
                keys.add(short)
    return hdrs, keys


UNITS = None


def owner_units(hdrs):
    """The units whose translation units these headers declare."""
    global UNITS
    if UNITS is None:
        import json
        UNITS = json.load(open(os.path.join(paths.REF, "units.json")))
    out = set()
    for h in hdrs:
        e = pt.headers[h]
        out.update(u for u, s in UNITS.items() if e["tu"] in s["tus"])
    return out


def matcher(keys):
    # a key may be followed by '_' (G004_libcdvd.h, T482_car_equipments.h)
    return re.compile(r"(?<![A-Za-z0-9_])(%s)(?![A-Za-z0-9])" % "|".join(
        re.escape(k) for k in sorted(keys, key=len, reverse=True)))


def task(pid, title, gids, allitems):
    hdrs, keys = part_keys(gids)
    pat = matcher(keys)
    own = owner_units(hdrs)
    mine = [(g, t) for g, t in allitems if pat.search(t) or g in own]
    cl = [x for x in mine if x[0] == "cleanup"]
    rn = [x for x in mine if x[0] != "cleanup"]
    L = ["You are doing the sweep of the second pass of the GT3 main executable decompilation (PlayStation 2 EE): "
         "partition %s (%s), %d canonical headers, %d cleanup items, %d rename proposals." % (
             pid, title, len(hdrs), len(cl), len(rn)),
         "",
         "Project root: %s  (use the Bash tool; start every command with: cd \"%s\" && ...)" % (ROOT, ROOT),
         "Read these completely first: ref/CONTEXT.md, ref/PASS2_UNITS.md and ref/PASS2_SWEEP.md. "
         "Follow PASS2_SWEEP.md exactly.",
         "Your notes: ref/pass2/sweep/%s/notes.md. Scratch: build/scratch/%s/." % (pid, pid),
         "",
         "Final answer, at most 12 lines: items applied / skipped / for other partitions (counts and the important "
         "ones), renames made, check results.",
         "",
         "## Your headers"]
    L += ["- " + os.path.basename(h) for h in hdrs]
    L.append("")
    L.append("## Cleanup items (ref/pass2/after_phase2.md) that mention your headers")
    L += ["- " + t.lstrip("-* ").strip()[:1500] for _, t in cl] or ["- (none)"]
    L.append("")
    L.append("## Rename and other proposals from the units' notes that mention your headers (unit) text")
    L += ["- (%s) %s" % (g, t.lstrip("-* ").strip()[:1200]) for g, t in rn] or ["- (none)"]
    return "\n".join(L) + "\n", len(cl), len(rn)


PARTS = [("s" + p[1:], t, g) for p, t, g in pi.PARTS]


def launcher(pid):
    return ("Do the sweep of the second pass of the GT3 main executable decompilation, partition %s. Your complete "
            "task is in %s/ref/pass2/sweep/%s/TASK.md: read it first and follow it exactly." % (pid, ROOT, pid))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(launcher(sys.argv[1]))
        sys.exit(0)
    allitems = items()
    print("items: %d cleanup, %d proposals" % (sum(1 for g, _ in allitems if g == "cleanup"),
                                               sum(1 for g, _ in allitems if g != "cleanup")))
    for pid, title, gids in PARTS:
        d = os.path.join(P2, "sweep", pid)
        os.makedirs(d, exist_ok=True)
        text, nc, nr = task(pid, title, gids, allitems)
        open(os.path.join(d, "TASK.md"), "w", encoding="utf-8", newline="\n").write(text)
        print("%s (%s): %d cleanup, %d proposals, %d chars" % (pid, title, nc, nr, len(text)))
