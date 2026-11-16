"""Write the tasks of the integration step after phase 1 (ref/PASS2_INTEGRATE.md):
ref/pass2/integration/<part>/TASK.md for each partition of the canonical headers,
with every note item (ref/pass2/groups/*/notes.md, ref/pass2/integration_todo.md)
that mentions the partition's translation units or classes.

    python tools/pass2_integrate.py              write all
    python tools/pass2_integrate.py i3           print that partition's launcher message
"""
import os
import re
import sys

import paths
import pass2_tasks as pt

ROOT = paths.ROOT.replace("\\", "/")
P2 = os.path.join(paths.REF, "pass2")

# consecutive phase-1 groups (subsystem order of tools/pass2_plan.py)
PARTS = [
    ("i1", "runtime, SDK wrappers, graphics libraries, engine base", ["h%02d" % i for i in range(0, 7)]),
    ("i2", "database and save data", ["h%02d" % i for i in range(7, 12)]),
    ("i3", "menu framework", ["h%02d" % i for i in range(12, 17)]),
    ("i4", "dynamics and cameras", ["h%02d" % i for i in range(17, 23)] + ["h42"]),
    ("i5", "race", ["h%02d" % i for i in range(23, 31)]),
    ("i6", "menus", ["h%02d" % i for i in range(31, 42)]),
]


def todo_items():
    p = os.path.join(P2, "integration_todo.md")
    out, cur = [], []
    for L in open(p, encoding="utf-8").read().split("\n") + [""]:
        if re.match(r"^- ", L) or not L.strip() or L.startswith(("#", "Done", "Collected")):
            if cur:
                out.append(("todo", " ".join(x.strip() for x in cur)))
            cur = [L] if L.startswith("- ") else []
        else:
            cur.append(L)
    return out


def part_task(pid, title, gids, items):
    groups = {g["id"]: g for g in pt.groups}
    hdrs = [h for gid in gids for h in groups[gid]["headers"]]
    keys = set()
    for h in hdrs:
        keys.add(pt.headers[h]["tu"])
        keys.update(c.rsplit("::", 1)[-1] for c in pt.headers[h]["classes"] if len(c.rsplit("::", 1)[-1]) >= 5)
    pat = re.compile(r"\b(%s)\b" % "|".join(re.escape(k) for k in sorted(keys, key=len, reverse=True)))
    mine = [(g, t) for g, t in items if pat.search(t)]
    L = ["You are doing the integration step of the second pass of the GT3 main executable decompilation "
         "(PlayStation 2 EE): partition %s (%s), %d canonical headers, %d note items." % (pid, title, len(hdrs), len(mine)),
         "",
         "Project root: %s  (use the Bash tool; start every command with: cd \"%s\" && ...)" % (ROOT, ROOT),
         "Read these completely first: ref/CONTEXT.md, ref/PASS2_HEADERS.md and ref/PASS2_INTEGRATE.md. "
         "Follow PASS2_INTEGRATE.md exactly.",
         "Your notes: ref/pass2/integration/%s/notes.md. Scratch: build/scratch/%s/. "
         "Do not build tools or test harnesses beyond small scratch tests." % (pid, pid),
         "",
         "Final answer, at most 12 lines: items applied / not applied / for other partitions (counts and the "
         "important ones); renames phase 2 must follow; check results (hdrcheck, vtcheck, hdrall).",
         "",
         "## Your headers (phase-1 groups %s)" % ", ".join(gids)]
    for gid in gids:
        L.append("- %s: %s" % (gid, ", ".join(os.path.basename(h) for h in groups[gid]["headers"])))
    L.append("")
    L.append("## Note items that mention your translation units or classes")
    L.append("(group) text. Items from your own groups are included: they may list what the next step must do.")
    for g, t in mine:
        L.append("- (%s) %s" % (g, t.lstrip("-* ").strip()[:900]))
    return "\n".join(L) + "\n", len(mine)


def launcher(pid):
    return ("Do the integration step of the second pass of the GT3 main executable decompilation, partition %s. "
            "Your complete task is in %s/ref/pass2/integration/%s/TASK.md: read it first and follow it exactly."
            % (pid, ROOT, pid))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(launcher(sys.argv[1]))
        sys.exit(0)
    items = pt.note_items() + todo_items()
    for pid, title, gids in PARTS:
        d = os.path.join(P2, "integration", pid)
        os.makedirs(d, exist_ok=True)
        text, n = part_task(pid, title, gids, items)
        open(os.path.join(d, "TASK.md"), "w", encoding="utf-8", newline="\n").write(text)
        print("%s: %d items, %d chars" % (pid, n, len(text)))
