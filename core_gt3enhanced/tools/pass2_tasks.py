"""Write the TASK.md of every phase-1 group (ref/pass2/groups/<group>/TASK.md) from
ref/pass2/headers.json and groups.json (tools/pass2_plan.py), and print the short
message that starts an agent on a group.

    python tools/pass2_tasks.py                    write all
    python tools/pass2_tasks.py --only h28 h29     write these (groups not started yet)
    python tools/pass2_tasks.py h05                print that group's launcher message
"""
import json
import os
import re
import sys

import paths

ROOT = paths.ROOT.replace("\\", "/")
P2 = os.path.join(paths.REF, "pass2")
headers = json.load(open(os.path.join(P2, "headers.json")))
groups = json.load(open(os.path.join(P2, "groups.json")))
inv = json.load(open(os.path.join(P2, "inventory.json")))


def dossier(name):
    return "ref/pass2/classes/%s.txt" % re.sub(r"[^\w]+", "_", name).strip("_")


def note_items():
    """Every bullet / paragraph of the groups' notes so far: (group, text)."""
    out = []
    gd = os.path.join(P2, "groups")
    for gid in sorted(os.listdir(gd)):
        p = os.path.join(gd, gid, "notes.md")
        if not os.path.exists(p):
            continue
        cur = []
        for L in open(p, encoding="utf-8", errors="replace").read().split("\n") + [""]:
            if not L.strip() or re.match(r"^\s{0,2}[-*] |^#", L):
                if cur:
                    out.append((gid, " ".join(x.strip() for x in cur)))
                cur = [L] if L.strip() and not L.startswith("#") else []
            else:
                cur.append(L)
    return out


NOTES = None


def earlier_notes(g):
    """The notes of other groups that mention this group's translation units or classes."""
    global NOTES
    if NOTES is None:
        NOTES = note_items()
    keys = set()
    for h in g["headers"]:
        keys.add(headers[h]["tu"])
        keys.update(c.rsplit("::", 1)[-1] for c in headers[h]["classes"] if len(c.rsplit("::", 1)[-1]) >= 5)
    pat = re.compile(r"\b(%s)\b" % "|".join(re.escape(k) for k in sorted(keys, key=len, reverse=True))) if keys else None
    return [(gid, t) for gid, t in NOTES if gid != g["id"] and pat and pat.search(t)]


def task(g):
    L = ["You are writing canonical C/C++ headers for the second pass of the GT3 main executable "
         "decompilation (PlayStation 2 EE). Group %s (%s), %d headers." % (g["id"], g["subsystem"], len(g["headers"])),
         "",
         "Project root: %s  (use the Bash tool; start every command with: cd \"%s\" && ...)" % (ROOT, ROOT),
         "Read these completely first: ref/CONTEXT.md and ref/PASS2_HEADERS.md. Follow PASS2_HEADERS.md exactly.",
         "Write only the headers listed below (each exists as a stub: replace it completely) and "
         "ref/pass2/groups/%s/notes.md. Do not build tools or test harnesses." % g["id"],
         "Before finishing, `python tools/hdrcheck.py <header>` must print OK for each of your headers "
         "(or fail only because a header of another group is still a stub, which your notes must say).",
         "",
         "Final answer, at most 12 lines: headers written; classes / functions / globals declared; hdrcheck "
         "results; layout conflicts you settled; anything the next phase must know.",
         ""]
    for h in g["headers"]:
        e = headers[h]
        L.append("## %s   (translation unit %s, owner unit %s, %s)" % (h, e["tu"], ", ".join(e["units"]), e["lang"]))
        if e["classes"]:
            L.append("Classes (dossier each):")
            for c in e["classes"]:
                ci = inv["classes"][c]
                L.append("- %s  %s  [%s; %d definitions in %d units]" % (
                    c, dossier(c), "RTTI" if ci["rtti"] else ci["why"], ci["n_defs"], len(ci["users"])))
        if e["functions"]:
            L.append("Functions other units use (address, the owner's name, definition line, file, used by):")
            for f in e["functions"]:
                L.append("- %s %s | %s | %s | %s" % (f["addr"], f["name"], f["sig"][:140], f["file"],
                                                     ",".join(f["used_by"][:8]) + ("..." if len(f["used_by"]) > 8 else "")))
        if e["globals"]:
            L.append("Globals (address, owner by %s; each unit's name, type and comment):" % "/".join(
                sorted({x["owner_by"] for x in e["globals"]})))
            for gl in e["globals"]:
                vs = "; ".join("%s %s %s %s" % (v["unit"], v["type"], v["name"], v["comment"][:50]) for v in gl["variants"][:6])
                L.append("- %s: %s" % (gl["addr"], vs))
        L.append("")
    en = earlier_notes(g)
    if en:
        L.append("## What earlier groups' notes say about your translation units and classes")
        L.append("Groups that included your headers (still stubs then) wrote what they need from you: layouts, sizes, "
                 "names their overrides use, types that belong in your headers. Satisfy these where the listing "
                 "agrees; where it does not, the listing wins and your notes say why.")
        for gid, t in en[:120]:
            L.append("- (%s) %s" % (gid, t.lstrip("-* ").strip()[:700]))
        if len(en) > 120:
            L.append("- ... %d more: grep ref/pass2/groups/*/notes.md for your translation units" % (len(en) - 120))
        L.append("")
    return "\n".join(L) + "\n"


def launcher(gid):
    return ("Write the canonical headers of second-pass group %s of the GT3 main executable decompilation. "
            "Your complete task is in %s/ref/pass2/groups/%s/TASK.md: read it first and follow it exactly." % (gid, ROOT, gid))


if __name__ == "__main__":
    only = sys.argv[2:] if sys.argv[1:2] == ["--only"] else None
    if len(sys.argv) > 1 and not only:
        print(launcher(sys.argv[1]))
    else:
        for g in groups:
            if only and g["id"] not in only:
                continue
            d = os.path.join(P2, "groups", g["id"])
            os.makedirs(d, exist_ok=True)
            open(os.path.join(d, "TASK.md"), "w", encoding="utf-8", newline="\n").write(task(g))
        print("wrote %d group tasks" % (len(only) if only else len(groups)))
