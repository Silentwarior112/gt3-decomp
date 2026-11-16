"""Plan the second pass's canonical headers (run tools/inventory.py first).

Writes ref/pass2/headers.json: for every canonical header include/game/<TU>_<stem>.h,
the classes it defines (every class shared by 2+ units or known from RTTI), the
functions it declares (every function another unit calls or names) and the globals it
declares (every address 2+ units declare).  Ownership:

  class    the owner translation unit from the inventory (RTTI typeinfo, methods,
           else the unit with the fullest definition);
  function the translation unit that holds it;
  global   in .data between frame blocks: the unit whose frame block follows it (the
           linker interleaved each unit's .data with its .eh_frame); else the unit
           whose sources mention it most.

Also writes the agent groups (ref/pass2/groups.json, in base-first subsystem order)
and stub headers (include guard, game.h, forward declarations) so that includes of
not-yet-written headers resolve.

    python tools/pass2_plan.py [--per-group N] [--no-stubs]
"""
import argparse
import collections
import json
import os
import re

import coreelf as E
import paths
import layout

ap = argparse.ArgumentParser()
ap.add_argument("--per-group", type=int, default=30, help="classes per agent group")
ap.add_argument("--no-stubs", action="store_true")
a = ap.parse_args()

R = paths.REF
P2 = os.path.join(R, "pass2")
GAME = os.path.join(paths.ROOT, "include", "game")
inv = json.load(open(os.path.join(P2, "inventory.json")))
funcs = json.load(open(os.path.join(R, "funcs.json")))
tus = {t["id"]: t for t in json.load(open(os.path.join(R, "tus.json")))}
units = json.load(open(os.path.join(R, "units.json")))
fidx = {int(f["start"], 16): f for f in funcs}


def unit_of_addr(x):
    for u, s in units.items():
        lo, hi = (int(v, 16) for v in s["ranges"][0])
        if lo <= x < hi:
            return u
    return None


tu_units = collections.defaultdict(set)
for f in funcs:
    tu_units[f["tu"]].add(unit_of_addr(int(f["start"], 16)))

# file stems per (unit, TU) from the first pass's file names
stems = {}
for u, d, fns in layout.unit_dirs():
    for fn in fns:
        m = re.match(r"^([TG]\d{3})_(.+?)\.(cpp|c|cc|S)$", fn)
        if m:
            stem = re.sub(r"_u\d{3}$", "", m.group(2))
            stems.setdefault(m.group(1), stem)


def header_of_tu(tu):
    st = stems.get(tu, "misc")
    return "include/game/%s_%s.h" % (tu, st)


def main_tu_of_unit(u, mention=None):
    """The TU of unit u whose sources mention `mention` most (else its largest TU)."""
    best, bestn = None, -1
    for fn, d in [(os.path.basename(f), os.path.join(paths.ROOT, os.path.dirname(f))) for f in layout.files_of_unit(u)]:
        m = re.match(r"^([TG]\d{3})_", fn)
        if not m:
            continue
        n = 0
        if mention:
            n = open(os.path.join(d, fn), encoding="utf-8", errors="replace").read().count(mention)
        if n > bestn:
            best, bestn = m.group(1), n
    return best or units[u]["tus"][0]


headers = collections.OrderedDict()


def H(tu):
    h = header_of_tu(tu)
    if h not in headers:
        owner_units = sorted(x for x in tu_units.get(tu, ()) if x)
        headers[h] = {"tu": tu, "units": owner_units, "lang": "c" if tu.startswith("G") else "cpp",
                      "classes": [], "functions": [], "globals": []}
    return headers[h]


# ---- classes
for name, c in sorted(inv["classes"].items()):
    if not c["shared"]:
        continue
    tu = c["owner_tu"] or main_tu_of_unit(c["owner_unit"], name.split("::")[-1])
    c["header"] = header_of_tu(tu)
    H(tu)["classes"].append(name)
    if tu.startswith("G"):
        H(tu)["lang"] = "cpp" if (c["rtti"] or c["methods_defined"]) else H(tu)["lang"]

# ---- functions other units call or name
mentions = collections.defaultdict(set)          # address -> units naming F_<addr>
for u, d, fns in layout.unit_dirs():
    for fn in fns:
        t = open(os.path.join(d, fn), encoding="utf-8", errors="replace").read()
        for m in set(re.findall(r"\bF_(00[0-9A-F]{6})\b", t)):
            mentions[int(m, 16)].add(u)
fmark = {}
for f in inv["functions"]:
    fmark.setdefault(int(f["addr"], 16), f)
for addr, f in sorted(fmark.items()):
    fu = f["unit"]
    callers = {unit_of_addr(int(c, 16)) for c in fidx[addr]["callers"]} if addr in fidx else set()
    ext = (callers | mentions.get(addr, set())) - {fu, None}
    if not ext and not fidx.get(addr, {}).get("data_xrefs"):
        continue
    if "::" in f["name"].split("(")[0]:
        continue                      # methods are declared with their class
    tu = fidx[addr]["tu"]
    H(tu)["functions"].append({"addr": "0x%08X" % addr, "name": f["name"], "sig": f["sig"], "file": f["file"],
                               "used_by": sorted(ext)})

# ---- globals declared by 2+ units
img = E.Image()
frames = sorted(img.frames)
frame_tu = {}
for t in tus.values():
    if t.get("block"):
        frame_tu[int(t["block"], 16)] = t["id"]


def data_owner(x):
    if not (E.EH_FRAME_LO <= x < E.EH_FRAME_HI) or img.in_frame_block(x):
        return None
    for lo, hi in frames:
        if lo > x:
            return frame_tu.get(lo)
    return None


for addr, gs in inv["globals"].items():
    us = {g["unit"] for g in gs}
    if len(us) < 2:
        continue
    x = int(addr, 16)
    tu = data_owner(x)
    how = "data"
    if not tu:
        cnt = collections.Counter(g["unit"] for g in gs)
        u = cnt.most_common(1)[0][0]
        f0 = [g for g in gs if g["unit"] == u][0]["file"]
        m = re.search(r"/([TG]\d{3})_", f0)
        tu = m.group(1) if m else main_tu_of_unit(u)
        how = "most-used"
    H(tu)["globals"].append({"addr": addr, "owner_by": how, "variants": [
        {"unit": g["unit"], "name": g["name"], "type": g["type"] + g["arr"], "comment": g["comment"]} for g in gs]})

# ---- groups: subsystem order, base first
ORDER = [("runtime-sdk", 125, 142), ("graphics-libs", 113, 124), ("engine-base", 110, 112),
         ("pdi-mpeg", 119, 120), ("database-save", 95, 109), ("menu-framework", 63, 75),
         ("dynamics", 76, 87), ("cameras", 88, 94), ("race", 42, 62), ("menus", 0, 41)]


def subsystem(h):
    us = headers[h]["units"] or ["u999"]
    n = int(us[0][1:])
    for i, (nm, lo, hi) in enumerate(ORDER):
        if lo <= n <= hi:
            return i, nm
    return len(ORDER), "other"


groups = []
cur, cur_key, cur_n = [], None, 0
for h in sorted(headers, key=lambda h: (subsystem(h)[0], headers[h]["units"][:1], h)):
    if not (headers[h]["classes"] or headers[h]["functions"] or headers[h]["globals"]):
        continue
    key = subsystem(h)
    w = len(headers[h]["classes"]) + len(headers[h]["functions"]) // 8 + len(headers[h]["globals"]) // 8 + 1
    if cur and (key != cur_key or cur_n + w > a.per_group):
        groups.append({"id": "h%02d" % len(groups), "subsystem": cur_key[1], "wave": cur_key[0], "headers": cur})
        cur, cur_n = [], 0
    cur.append(h)
    cur_key = key
    cur_n += w
if cur:
    groups.append({"id": "h%02d" % len(groups), "subsystem": cur_key[1], "wave": cur_key[0], "headers": cur})

json.dump(headers, open(os.path.join(P2, "headers.json"), "w"), indent=1)
json.dump(groups, open(os.path.join(P2, "groups.json"), "w"), indent=1)
cls_map = {n: c.get("header") for n, c in inv["classes"].items() if c.get("header")}
json.dump(cls_map, open(os.path.join(P2, "class_headers.json"), "w"), indent=1)

# ---- stubs
if not a.no_stubs:
    os.makedirs(GAME, exist_ok=True)
    for h, e in headers.items():
        p = os.path.join(paths.ROOT, h)
        if os.path.exists(p) and "STUB: written by tools/pass2_plan.py" not in open(p, encoding="utf-8").read():
            continue                       # an agent has written it
        guard = "GAME_" + re.sub(r"\W", "_", os.path.basename(h)).upper()
        L = ["/* %s - canonical header of translation unit %s." % (os.path.basename(h), e["tu"]),
             " * STUB: written by tools/pass2_plan.py; the second pass replaces it. */",
             "#ifndef " + guard, "#define " + guard, "", '#include "game/game.h"', ""]
        if e["lang"] == "cpp":
            byns = collections.defaultdict(list)
            for n in e["classes"]:
                parts = re.sub(r"<.*", "", n).split("::")
                byns["::".join(parts[:-1])].append(parts[-1])
            all_names = set(inv["classes"])
            for ns, names in sorted(byns.items()):
                if "<" in ns or ns in all_names:
                    continue                   # templates, and classes nested in a class
                if ns:
                    L.append("namespace %s {" % ns.replace("::", " { namespace "))
                for nm in sorted(set(names)):
                    if inv["classes"].get((ns + "::" if ns else "") + nm, {}).get("rtti") or True:
                        L.append("class %s;" % nm)
                if ns:
                    L.append("}" * (ns.count("::") + 1))
        L += ["", "#endif /* %s */" % guard, ""]
        open(p, "w", encoding="utf-8", newline="\n").write("\n".join(L))

n_cls = sum(len(e["classes"]) for e in headers.values())
n_fn = sum(len(e["functions"]) for e in headers.values())
n_gl = sum(len(e["globals"]) for e in headers.values())
print("%d canonical headers: %d classes, %d functions, %d globals; %d agent groups" % (
    len(headers), n_cls, n_fn, n_gl, len(groups)))
for g in groups:
    c = sum(len(headers[h]["classes"]) for h in g["headers"])
    f = sum(len(headers[h]["functions"]) for h in g["headers"])
    gl = sum(len(headers[h]["globals"]) for h in g["headers"])
    print("  %s %-15s %2d headers, %3d classes, %3d functions, %3d globals" % (g["id"], g["subsystem"], len(g["headers"]), c, f, gl))
