"""The class index of the main executable as the decompilation uses it:
ref/classes.json and ref/classes.txt.

For every polymorphic class (GT3Hooks' docs/symbols/classes_US.csv, vtables_US.csv):
its typeinfo function, its bases (the other __tf functions its __tf calls), each
vtable (primary / secondary with this-delta) and every slot: index, the offset a
virtual call loads the function pointer from (8*slot + 12 from the vptr), the
method address and its name in ref/symbols.csv.

egcs vtable layout: the vptr points at entry 0 = {s16 delta, s16 index, __tf}; slot k
is entry k+1: {s16 delta at +8(k+1), s16 index, pfn at +8(k+1)+4}.  A virtual call:
    lw   vt, VPTR_OFF(obj)       ; the vptr is a field (egcs puts it after the first
    lh   d, 8k+8(vt)             ;   polymorphic base's data members, often +0)
    lw   fn, 8k+12(vt)
    jalr fn  with a0 = obj + d
"""
import collections
import csv
import json
import os

import coreelf as E
import paths

DOCS = os.environ.get("GT3_DOCS") or os.path.join(paths.REPO, "..", "docs")
SYM = os.path.join(DOCS, "symbols")
R = paths.REF

img = E.Image()
funcs = json.load(open(os.path.join(R, "funcs.json")))
fidx = {int(f["start"], 16): f for f in funcs}
syms = {int(r["addr"], 16): r for r in csv.DictReader(open(os.path.join(R, "symbols.csv"), encoding="utf-8"))}
classes = list(csv.DictReader(open(os.path.join(SYM, "classes_US.csv"), encoding="utf-8")))
vts = list(csv.DictReader(open(os.path.join(SYM, "vtables_US.csv"), encoding="utf-8")))

tf_of = {}
for c in classes:
    if c["typeinfo_fn"]:
        tf_of[int(c["typeinfo_fn"], 16)] = c["class_name"]
dem = {c["class_name"]: c["demangled"] for c in classes}

rows = collections.defaultdict(list)
for r in vts:
    rows[(r["class_name"], r["vtable_addr"])].append(r)

out = {}
for c in classes:
    cn = c["class_name"]
    e = out.setdefault(cn, {"name": c["demangled"], "mangled": cn, "typeinfo_fn": c["typeinfo_fn"],
                            "rtti": c["rtti_helper"], "bases": [], "vtables": [], "tu": None})
    tf = int(c["typeinfo_fn"], 16) if c["typeinfo_fn"] else None
    if tf and tf in fidx and not e["bases"]:
        f = fidx[tf]
        e["tu"] = f["tu"]
        for cal in f["callees"]:
            b = tf_of.get(int(cal, 16))
            if b and b != cn:
                e["bases"].append(dem.get(b, b))
    slots = []
    for r in sorted(rows.get((cn, c["vtable_addr"]), []), key=lambda r: int(r["slot_index"])):
        m = int(r["method_addr"], 16)
        s = syms.get(m)
        slots.append({"slot": int(r["slot_index"]), "off": "+0x%X" % (8 * int(r["slot_index"]) + 12),
                      "fn": "0x%08X" % m, "name": s["name"] if s else None,
                      "tu": fidx[m]["tu"] if m in fidx else None})
    e["vtables"].append({"addr": c["vtable_addr"], "kind": c["vtable_kind"], "this_delta": c["this_delta"],
                         "len": c["vtable_len"], "slots": slots})

json.dump(out, open(os.path.join(R, "classes.json"), "w"), indent=1)
L = ["# Polymorphic classes of the main executable (tools/classes.py). Per vtable slot:",
     "# index, call offset from the vptr, method address, name (ref/symbols.csv), translation unit.", ""]
for cn, e in sorted(out.items(), key=lambda kv: kv[1]["name"]):
    L.append("class %s   [%s]  typeinfo %s (%s)  tu %s%s" % (e["name"], cn, e["typeinfo_fn"], e["rtti"], e["tu"],
             ("  bases: " + ", ".join(e["bases"])) if e["bases"] else ""))
    for v in e["vtables"]:
        L.append("  vtable %s %s this_delta %s, %s slots" % (v["addr"], v["kind"], v["this_delta"], v["len"]))
        for s in v["slots"]:
            L.append("    [%2d] %-6s %s %s  %s" % (s["slot"], s["off"], s["fn"], s["tu"], s["name"] or ""))
    L.append("")
open(os.path.join(R, "classes.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(L))
print("classes: %d (%d with bases)" % (len(out), sum(1 for e in out.values() if e["bases"])))
