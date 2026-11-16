"""Build ref/symbols.csv (functions) and ref/data_symbols.csv (data) from every naming
source the project has.  Precedence, highest first:

  abi       GT3Hooks' hand-identified engine addresses (docs/engine_abi_addresses.txt)
  boot      same code as a decompiled bootstrap function (ref/libmatch.json)
  lupo2     the Lupo 2 linkonce name map (docs/symbols/lupo2_to_gt3_map.csv), HIGH
  tf        typeinfo functions from the class index (docs/symbols/classes_US.csv)
  vslot     a vtable slot whose Lupo 2 counterpart is named (vtables_US/_L2.csv)
  lupo2m    the Lupo 2 name map, MEDIUM
  dtor      a vtable slot with the deleting-destructor shape
  vslotgen  any other vtable slot: Class::virtual_<slot>

The GT3Hooks docs folder is read from <repo>/../docs (GT3_DOCS overrides).
"""
import collections
import csv
import json
import os
import re

import coreelf as E
import paths

DOCS = os.environ.get("GT3_DOCS") or os.path.join(paths.REPO, "..", "docs")
SYM = os.path.join(DOCS, "symbols")
OP_DELETE = 0x0027C0C0

img = E.Image()
funcs = json.load(open(os.path.join(paths.REF, "funcs.json")))
fstart = {int(f["start"], 16): f for f in funcs}

# ---- mangled -> demangled (Lupo 2 catalogue: "0xADDR size T name" + mangled line)
demangle = {}
lines = open(os.path.join(SYM, "lupo2_symbols_demangled.txt"), encoding="utf-8").read().splitlines()
for i, L in enumerate(lines):
    m = re.match(r"^0x([0-9A-F]{8})\s+\d+\s+([TD])\s+(.*)$", L)
    if m and i + 1 < len(lines):
        demangle[lines[i + 1].strip()] = m.group(3).strip()

# ---- class index
classes = list(csv.DictReader(open(os.path.join(SYM, "classes_US.csv"), encoding="utf-8")))
vt_us = list(csv.DictReader(open(os.path.join(SYM, "vtables_US.csv"), encoding="utf-8")))
classes_l2 = list(csv.DictReader(open(os.path.join(SYM, "classes_L2.csv"), encoding="utf-8")))
vt_l2 = list(csv.DictReader(open(os.path.join(SYM, "vtables_L2.csv"), encoding="utf-8")))
l2_names = {}
for r in csv.DictReader(open(os.path.join(SYM, "lupo2_functions.csv"), encoding="utf-8")):
    if r["name"]:
        l2_names[int(r["start"], 16)] = r["name"]

dem_of_class = {c["class_name"]: c["demangled"] for c in classes}
for c in classes_l2:
    dem_of_class.setdefault(c["class_name"], c["demangled"])

syms = {}
RANK = {"abi": 0, "syscall": 1, "boot": 1, "lupo2": 2, "tf": 3, "vslot": 4, "lupo2m": 5, "dtor": 6, "vslotgen": 7}


def put(addr, name, source, cls="", note=""):
    if addr not in fstart:
        return
    old = syms.get(addr)
    if old and RANK[old["source"]] <= RANK[source]:
        return
    syms[addr] = {"name": name, "source": source, "class": cls, "note": note}


# abi (hand-identified)
p = os.path.join(DOCS, "engine_abi_addresses.txt")
data_syms = {}
if os.path.exists(p):
    for L in open(p, encoding="utf-8"):
        m = re.match(r"^(\w+)\s+0x([0-9A-Fa-f]+)\s+\S+\s+\S+\s+(.*)$", L)
        if not m:
            continue
        nm, a, verdict = m.group(1), int(m.group(2), 16), m.group(3)
        if "data addr" in verdict:
            data_syms[a] = (nm, "abi")
        else:
            put(a, nm, "abi")

# syscall stubs: addiu $v1, $zero, N; syscall; jr $ra (names from the bootstrap's table)
sc_names = {}
p = os.path.join(paths.REPO, "bootstrap", "sdk", "libkernl", "syscalls.S")
if os.path.exists(p):
    txt = open(p, encoding="utf-8").read()
    for m in re.finditer(r"//\s*FUNC\s+0x[0-9A-Fa-f]+\s+(\w+)\s*\n//\s*#(-?0x[0-9A-Fa-f]+)", txt):
        sc_names.setdefault(int(m.group(2), 0) & 0xFFFF, m.group(1))
    for m in re.finditer(r"\bA?STUB\((\w+),\s*(-?0x[0-9A-Fa-f]+|-?\d+)\)", txt):
        sc_names[int(m.group(2), 0) & 0xFFFF] = m.group(1)
for a, f in fstart.items():
    if f["size"] > 16:
        continue
    w0, w1 = img.word(a), img.word(a + 4)
    if (w0 & 0xFFFF0000) == 0x24030000 and w1 == 0x0000000C:
        n = w0 & 0xFFFF
        if n in sc_names:
            put(a, sc_names[n], "syscall", note="syscall %s" % (hex(n - 0x10000) if n & 0x8000 else hex(n)))

# bootstrap matches
lm = json.load(open(os.path.join(paths.REF, "libmatch.json")))
for k, v in lm.items():
    if not v["name"].startswith("F_"):
        put(int(k, 16), v["name"], "boot", note="bootstrap %s %s" % (v["boot"], v["file"]))

# Lupo 2 map
for r in csv.DictReader(open(os.path.join(SYM, "lupo2_to_gt3_map.csv"), encoding="utf-8")):
    if r["status"] != "resolved" or not r["us_addr"]:
        continue
    nm = demangle.get(r["name"], r["name"])
    put(int(r["us_addr"], 16), nm, "lupo2" if r["confidence"] == "HIGH" else "lupo2m", note=r["name"])

# typeinfo functions
for c in classes:
    if c["vtable_kind"] == "primary" and c["typeinfo_fn"]:
        put(int(c["typeinfo_fn"], 16), "__tf" + c["class_name"], "tf", cls=c["demangled"],
            note="typeinfo function for %s" % c["demangled"])

# vtables: owner class of each slot method; Lupo 2 slot names
l2_slot = {}
for r in vt_l2:
    l2_slot[(r["class_name"], int(r["slot_index"]), r["vtable_addr"])] = int(r["method_addr"], 16)
l2_vt_of_class = collections.defaultdict(list)
for c in classes_l2:
    l2_vt_of_class[c["class_name"]].append(c)
us_vt_rows = collections.defaultdict(list)
for r in vt_us:
    us_vt_rows[(r["class_name"], r["vtable_addr"])].append(r)

cls_info = {(c["class_name"], c["vtable_addr"]): c for c in classes}
tf_tu = {}
for c in classes:
    if c["typeinfo_fn"] and int(c["typeinfo_fn"], 16) in fstart:
        tf_tu[c["class_name"]] = fstart[int(c["typeinfo_fn"], 16)]["tu"]


tf_of_fn = {int(c["typeinfo_fn"], 16): c["class_name"] for c in classes if c["typeinfo_fn"]}
prim_us = {c["class_name"]: c for c in classes if c["vtable_kind"] == "primary"}
prim_l2 = {c["class_name"]: c for c in classes_l2 if c["vtable_kind"] == "primary"}


def n_slots_us(cn):
    c = prim_us.get(cn)
    return len(us_vt_rows.get((cn, c["vtable_addr"]), [])) if c else 0


def n_slots_l2(cn):
    c = prim_l2.get(cn)
    return sum(1 for k in l2_slot if k[0] == cn and k[2] == c["vtable_addr"]) if c else -1


def primary_base(cn):
    c = prim_us.get(cn)
    if not c or not c["typeinfo_fn"]:
        return None
    f = fstart.get(int(c["typeinfo_fn"], 16))
    for cal in (f["callees"] if f else []):
        b = tf_of_fn.get(int(cal, 16))
        if b and b != cn:
            return b
    return None


_safe = {}


def safe_slots(cn, depth=0):
    """How many leading primary-vtable slots of cn sit at the same index in Lupo 2:
    all of them when the class and its bases have the same slot counts in both builds,
    else the slots inherited from the nearest base whose layout still agrees."""
    if cn in _safe:
        return _safe[cn]
    nu, nl = n_slots_us(cn), n_slots_l2(cn)
    b = primary_base(cn) if depth < 20 else None
    if b is None:
        r = nu if nu == nl else 0
    else:
        sb = safe_slots(b, depth + 1)
        if sb < n_slots_us(b):
            r = sb
        else:
            r = nu if nu == nl else n_slots_us(b)
    _safe[cn] = r
    return r


def is_deleting_dtor(a):
    f = fstart.get(a)
    if not f:
        return False
    n = f["size"] // 4
    calls_delete = any((img.word(a + 4 * i) >> 26) == 3 and ((img.word(a + 4 * i) & 0x03FFFFFF) << 2) == OP_DELETE
                       for i in range(n))
    tests_flag = any((img.word(a + 4 * i) >> 26) == 0x0C and ((img.word(a + 4 * i) >> 21) & 31) in (5, 17, 16, 18, 19, 20, 21, 22, 23)
                     and (img.word(a + 4 * i) & 0xFFFF) == 1 for i in range(n))
    return calls_delete and tests_flag


slot_users = collections.defaultdict(list)          # method -> [(class, slot, vtlen, kind)]
for (cn, vta), rows in us_vt_rows.items():
    ci = cls_info.get((cn, vta))
    vtlen = int(ci["vtable_len"]) if ci else len(rows)
    kind = ci["vtable_kind"] if ci else "primary"
    for r in rows:
        s = int(r["slot_index"])            # 0 = the first method after the {delta, __tf} entry
        slot_users[int(r["method_addr"], 16)].append((cn, s, vtlen, kind, vta))

for m, users in slot_users.items():
    if m not in fstart:
        continue
    tu = fstart[m]["tu"]
    same_tu = [u for u in users if tf_tu.get(u[0]) == tu]
    pool = same_tu or users
    owner = min(pool, key=lambda u: (u[3] != "primary", u[2], u[1]))
    cn, s = owner[0], owner[1]
    dem = dem_of_class.get(cn, cn)
    # a Lupo 2 name for this slot?  Only when the class has as many slots in both builds:
    # 72 classes gained virtuals between Lupo 2 and US, which shifts the slots after them.
    l2name = None
    us_n = len(us_vt_rows.get((cn, owner[4]), []))
    for c2 in l2_vt_of_class.get(cn, []):
        if c2["vtable_kind"] != owner[3]:
            continue
        if owner[3] == "primary":
            if s >= safe_slots(cn):
                continue
        elif sum(1 for k in l2_slot if k[0] == cn and k[2] == c2["vtable_addr"]) != us_n:
            continue
        a2 = l2_slot.get((cn, s, c2["vtable_addr"]))
        if a2 and a2 in l2_names:
            l2name = demangle.get(l2_names[a2], l2_names[a2])
            break
    slots = ", ".join("%s[%d]@+0x%X" % (dem_of_class.get(u[0], u[0]), u[1], 8 * u[1] + 12) for u in users[:6])
    if l2name:
        put(m, l2name, "vslot", cls=dem, note="vtable slots " + slots)
    elif is_deleting_dtor(m):
        base = dem.split("::")[-1].split("<")[0]
        put(m, "%s::~%s" % (dem, base), "dtor", cls=dem, note="deleting-dtor shape; vtable slots " + slots)
    else:
        put(m, "%s::virtual_%d" % (dem, s), "vslotgen", cls=dem, note="vtable slots " + slots)

# ---- data symbols: vtables, typeinfo name strings
for c in classes:
    va = int(c["vtable_addr"], 16)
    tag = "" if c["vtable_kind"] == "primary" else " (secondary, this_delta %s)" % c["this_delta"]
    data_syms.setdefault(va, ("_vt$" + c["class_name"], "vtable for %s%s, %s slots" % (c["demangled"], tag, c["vtable_len"])))
    if c["name_str"]:
        data_syms.setdefault(int(c["name_str"], 16), ("__tn" + c["class_name"], 'typeinfo name "%s"' % c["class_name"]))

with open(os.path.join(paths.REF, "symbols.csv"), "w", newline="", encoding="utf-8") as fh:
    wr = csv.writer(fh)
    wr.writerow(["addr", "name", "source", "class", "note"])
    for a in sorted(syms):
        s = syms[a]
        wr.writerow(["0x%08X" % a, s["name"], s["source"], s["class"], s["note"]])
seen_dn = set()
with open(os.path.join(paths.REF, "data_symbols.csv"), "w", newline="", encoding="utf-8") as fh:
    wr = csv.writer(fh)
    wr.writerow(["addr", "name", "note"])
    for a in sorted(data_syms):
        nm, note = data_syms[a]
        if nm in seen_dn:                      # secondary vtables share the class name
            nm = "%s_%08X" % (nm, a)
        seen_dn.add(nm)
        wr.writerow(["0x%08X" % a, nm, note])

cnt = collections.Counter(s["source"] for s in syms.values())
print("named %d of %d functions: %s" % (len(syms), len(funcs), dict(sorted(cnt.items(), key=lambda x: RANK[x[0]]))))
print("data symbols:", len(data_syms))
