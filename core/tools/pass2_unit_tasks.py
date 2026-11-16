"""Write the phase-2 task of every unit, ref/pass2/units/<unit>/TASK.md (run after
phase 1, tools/pass2_names.py and tools/callcheck.py --all > ref/pass2/callcheck_before.txt).

Each task lists: the unit's own canonical headers; the canonical headers of the
shared classes it defines locally (to delete); a rename table for every F_/g_
address and renamed name it uses; the phase-1 notes that mention it; its functions
with '?' markers; its call-check baseline.

    python tools/pass2_unit_tasks.py              write all
    python tools/pass2_unit_tasks.py --write u017 rewrite these tasks (just before launching: picks up
                                                  the notes of units finished since)
    python tools/pass2_unit_tasks.py u017         print that unit's launcher message
"""
import collections
import csv
import glob
import json
import os
import re
import sys

import paths
import layout

ROOT = paths.ROOT.replace("\\", "/")
P2 = os.path.join(paths.REF, "pass2")
units = json.load(open(os.path.join(paths.REF, "units.json")))
MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+(0x00[0-9A-Fa-f]{6})\s+(.*?)\s*(?:\*/)?\s*$")


def load():
    headers = json.load(open(os.path.join(P2, "headers.json")))
    inv = json.load(open(os.path.join(P2, "inventory.json")))
    canon = list(csv.DictReader(open(os.path.join(P2, "canonical.csv"), encoding="utf-8")))
    base = {}
    p = os.path.join(P2, "callcheck_before.txt")
    if os.path.exists(p):
        for L in open(p, encoding="utf-8"):
            m = re.match(r"^(u\d{3}): (\d+)/(\d+)", L)
            if m:
                base[m.group(1)] = (int(m.group(2)), int(m.group(3)))
    notes = []
    for f in sorted(glob.glob(os.path.join(P2, "groups", "*", "notes.md")) +
                    glob.glob(os.path.join(P2, "integration", "*", "notes.md")) +
                    glob.glob(os.path.join(P2, "units", "*", "notes.md"))) + [os.path.join(paths.REF, "followups.md")]:
        gid = os.path.basename(os.path.dirname(f)) if f.endswith("notes.md") else "first-pass followups"
        cur = []
        for L in open(f, encoding="utf-8", errors="replace").read().split("\n") + [""]:
            if not L.strip() or re.match(r"^\s{0,2}[-*] |^#", L):     # a bullet / paragraph per item
                if cur:
                    notes.append((gid, " ".join(x.strip() for x in cur)))
                cur = [L] if L.strip() and not L.startswith("#") else []
            else:
                cur.append(L)
    return headers, inv, canon, base, notes


def task(u, headers, inv, canon, base, notes):
    s = units[u]
    texts = {os.path.basename(f): open(os.path.join(paths.ROOT, f), encoding="utf-8", errors="replace").read()
             for f in sorted(layout.files_and_headers_of_unit(u))}
    alltext = "\n".join(texts.values())
    own = [h for h, e in headers.items() if e["tu"] in s["tus"] or u in e["units"]]
    local = sorted({x["name"] for x in inv["defs"] if x["unit"] == u and inv["classes"][x["name"]].get("shared")})
    cls_hdr = json.load(open(os.path.join(P2, "class_headers.json")))
    by_addr = collections.defaultdict(list)
    for r in canon:
        by_addr[r["addr"]].append(r)
    renames = []
    seen = set()
    for m in sorted(set(re.findall(r"\b([Fg])_(00[0-9A-F]{6})\b", alltext))):
        addr = "0x" + m[1]
        for r in by_addr.get(addr, [])[:1]:
            renames.append((m[0] + "_" + m[1], r["name"], r["header"], r["decl"][:110]))
            seen.add(addr)
    for r in canon:
        fp = r["first_pass"].split("(")[0]
        short_old = fp.rsplit("::", 1)[-1]
        short_new = r["name"].rsplit("::", 1)[-1]
        if r["addr"] in seen or not short_old or short_old == short_new or short_old.startswith(("F_", "g_")):
            continue
        if re.search(r"\b%s\b" % re.escape(short_old), alltext):
            renames.append((fp, r["name"], r["header"], r["decl"][:110]))
            seen.add(r["addr"])
    unsure = []
    for fn, t in texts.items():
        cur, n = None, 0
        for L in t.split("\n") + ["// FUNC 0x00000000 <end>"]:
            m = MARK.match(L)
            if m:
                if cur and n:
                    unsure.append((cur[0], cur[1], n, fn))
                cur, n = (m.group(1), m.group(2)), 0
            elif "/* ?" in L or "(?)" in L or "/*?" in L:
                n += 1
    keys = [u] + list(s["tus"])
    pat = re.compile(r"\b(%s)\b" % "|".join(re.escape(k) for k in keys))
    mine = [(g, L) for g, L in notes if g != u and pat.search(L)]
    b = base.get(u)
    L = ["You are doing the second pass on work unit %s of the GT3 main executable decompilation (PlayStation 2 EE): "
         "move it onto the canonical headers and improve its reverse engineering." % u,
         "",
         "Project root: %s  (use the Bash tool; start every command with: cd \"%s\" && ...)" % (ROOT, ROOT),
         "Read these completely first: ref/CONTEXT.md, ref/DECOMP_RULES.md and ref/PASS2_UNITS.md. Follow PASS2_UNITS.md exactly.",
         "Your sources: src/%s/. Ground truth: ref/units/%s/listing.txt. Unit: %s..%s, %d functions, translation units %s." % (
             u, u, s["ranges"][0][0], s["ranges"][0][1], s["nfuncs"], ", ".join(s["tus"])),
         "Do not build tools or test harnesses. Write the source in steps and compile often: sh tools/cc.sh --unit %s" % u,
         "Before finishing: cc.sh --unit %s OK; check_coverage.py --unit %s clean; callcheck.py %s finds at least %s calls." % (
             u, u, u, ("%d of %d" % b) if b else "as many"),
         "",
         "Final answer, at most 12 lines: what changed (headers switched, classes removed, renames, locals renamed); "
         "unsure functions settled / still open; canonical-header problems (also in your notes).",
         "",
         "## Your own canonical headers (you may fix them; see PASS2_UNITS.md)"]
    L += ["- " + h for h in own] or ["- (none)"]
    L.append("")
    L.append("## Shared classes you define locally: delete these definitions and include the canonical header")
    for c in local:
        L.append("- %s -> %s" % (c, cls_hdr.get(c, "?")))
    if not local:
        L.append("- (none)")
    L.append("")
    L.append("## Rename table (what your code says -> canonical name, its header, declaration)")
    for old, new, h, decl in renames:
        L.append("- %s -> %s  [%s]  %s" % (old, new, h, decl))
    if not renames:
        L.append("- (none)")
    L.append("")
    L.append("## Notes that mention %s (h## = header group, i# = integration, u### = a unit done before you)" % u)
    L.append("Items that name your unit or one of your translation units; many are about headers and need "
             "nothing from you. Apply those that ask for a change in your sources.")
    for g, line in mine[:150]:
        L.append("- (%s) %s" % (g, line.strip("-* ").strip()[:700]))
    if len(mine) > 150:
        L.append("- ... %d more: grep ref/pass2/groups/*/notes.md and ref/pass2/integration/*/notes.md for %s"
                 % (len(mine) - 150, ", ".join(keys)))
    if not mine:
        L.append("- (none)")
    L.append("")
    add = os.path.join(P2, "units", u, "addendum.md")
    if os.path.exists(add):
        L.append("## Coordinator's additions to this task (do these too)")
        L.append(open(add, encoding="utf-8").read().strip())
        L.append("")
    L.append("## Functions with '?' markers (re-examine)")
    for a, nm, n, fn in unsure:
        L.append("- %s %s: %d marker(s) in %s" % (a, nm, n, fn))
    if not unsure:
        L.append("- (none)")
    return "\n".join(L) + "\n"


def launcher(u):
    return ("Do the second pass on work unit %s of the GT3 main executable decompilation. Your complete task is in "
            "%s/ref/pass2/units/%s/TASK.md: read it first and follow it exactly." % (u, ROOT, u))


if __name__ == "__main__":
    only = sys.argv[2:] if sys.argv[1:2] == ["--write"] else None
    if len(sys.argv) > 1 and not only:
        print(launcher(sys.argv[1]))
        sys.exit(0)
    headers, inv, canon, base, notes = load()
    for u in (only or units):
        d = os.path.join(P2, "units", u)
        os.makedirs(d, exist_ok=True)
        open(os.path.join(d, "TASK.md"), "w", encoding="utf-8", newline="\n").write(task(u, headers, inv, canon, base, notes))
    print("wrote %d unit tasks" % len(only or units))
