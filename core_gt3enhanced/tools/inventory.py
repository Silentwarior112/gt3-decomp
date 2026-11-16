"""Second-pass inventory of the decomp tree: what the first pass declared, per class,
function and global, and who owns each.  Writes ref/pass2/:

  inventory.json      classes (every definition, fields, methods, bases), functions
                      (FUNC markers with their definition line), globals (every
                      declaration of an address), ownership
  classes/<Class>.txt one dossier per class: owner, RTTI facts, the merged field table
                      (offset -> names and types used by each unit), every definition
  globals.txt         every global address with the names and types the units use

Ownership of a class: the translation unit of its typeinfo function (RTTI classes),
else the one defining most of its methods (FUNC markers "Class::method"), else the
unit with the fullest non-partial definition.

    python tools/inventory.py
"""
import collections
import csv
import json
import os
import re

import paths
import layout

R = paths.REF
OUT = os.path.join(R, "pass2")
SRC = os.path.join(paths.ROOT, "src")

funcs = json.load(open(os.path.join(R, "funcs.json")))
fidx = {int(f["start"], 16): f for f in funcs}
units = json.load(open(os.path.join(R, "units.json")))
cls_rtti = json.load(open(os.path.join(R, "classes.json")))
rtti_by_name = {e["name"]: e for e in cls_rtti.values()}


def unit_of_addr(a):
    for u, s in units.items():
        lo, hi = (int(x, 16) for x in s["ranges"][0])
        if lo <= a < hi:
            return u
    return None


# ------------------------------------------------------------------ scanning
def mask_comments(text):
    """The text with comments and string/char literals replaced by spaces (same
    length), so braces and keywords can be found by position."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == "\\":
                    j += 1
                if text[j] == "\n":
                    break
                j += 1
            for k in range(i + 1, min(j, n)):
                out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


DEF_RE = re.compile(r"\b(class|struct|union)\s+([A-Za-z_]\w*(?:::\w+)*)\s*(?:final\s*)?(:\s*[^{;()]+)?\{")
NS_RE = re.compile(r"\bnamespace\s+([A-Za-z_]\w*)\s*\{")
OFF_RE = re.compile(r"(?:\+|@)?\s*0x([0-9A-Fa-f]{1,5})\b")


def match_brace(masked, i):
    """Index just past the brace block starting at masked[i] == '{'."""
    d = 0
    n = len(masked)
    while i < n:
        c = masked[i]
        if c == "{":
            d += 1
        elif c == "}":
            d -= 1
            if d == 0:
                return i + 1
        i += 1
    return n


def namespaces_at(masked, pos, ns_spans):
    return [name for (lo, hi, name) in ns_spans if lo < pos < hi]


def parse_body(raw, masked, lo, hi):
    """Top-level members of a class body raw[lo:hi] (inside the braces)."""
    fields, methods = [], []
    i = lo
    d = 0
    start = lo
    while i < hi:
        c = masked[i]
        if c == "{":
            if d == 0:
                # a method body or a nested type: skip it
                j = match_brace(masked, i)
                stmt = raw[start:i].strip()
                if "(" in masked[start:i]:
                    methods.append(stmt.split("\n")[-1].strip()[:200])
                i = j
                start = i
                # a nested type ends with ';' after the brace: let the loop eat it
                continue
        elif c == ";":
            stmt_m = masked[start:i]
            stmt_r = raw[start:i + 1]
            # the comment on the same line as the ';'
            eol = raw.find("\n", i)
            eol = hi if eol < 0 else min(eol, hi)
            tail = raw[i + 1:eol]
            if re.match(r"\s*/\*", tail) and "*/" not in tail:
                close = raw.find("*/", i)               # a comment continued on the next lines
                if 0 <= close < hi:
                    tail = raw[i + 1:close + 2]
            line_start = raw.rfind("\n", 0, i) + 1
            line_text = raw[line_start:eol]
            s = re.sub(r"^(\s*(public|private|protected)\s*:)+", "", stmt_m).strip()
            if s and not s.startswith(("friend", "using", "typedef", "enum", "static_assert", "template")):
                s1 = re.sub(r"\s+", " ", s)
                for kw in ("public:", "private:", "protected:"):
                    s1 = s1.replace(kw, "")
                s1 = s1.strip()
                if "(" in s1 and not re.match(r"^[\w:<>\s\*&]+\(\s*\*\s*\w+\s*\)\s*\(", s1):
                    methods.append(s1[:200])
                else:
                    m = re.match(r"^(?P<type>.+?)\s*(?P<ptr>[\*&]*)\s*(?P<name>\w+)\s*(?P<arr>(\[[^\]]*\])*)\s*(?P<bits>:\s*\d+)?$", s1)
                    # a function-pointer member: `R (*name[n])(args)`
                    fp = re.match(r"^(?P<type>.+?)\(\s*\*\s*(?P<name>\w+)\s*(?P<arr>(\[[^\]]*\])*)\s*\)\s*\(.*\)$", s1)
                    if fp:
                        m = fp
                    if m and not s1.startswith(("static ",)):
                        com = ""
                        # the comment right after this member's ';', else one earlier on the line
                        mc = re.match(r"\s*(?:/\*(.*?)\*/|//(.*)$)", tail, re.S) or re.search(r"/\*(.*?)\*/|//(.*)$", line_text)
                        if mc:
                            com = (mc.group(1) or mc.group(2) or "").strip()
                        off = None
                        mo = OFF_RE.search(com)
                        if mo:
                            off = int(mo.group(1), 16)
                        ptr = "(*)" if fp else m.group("ptr")
                        fields.append({"type": (m.group("type") + ptr).strip(), "name": m.group("name"),
                                       "arr": m.group("arr") or "", "off": off, "comment": com[:120],
                                       "bits": bool(m.groupdict().get("bits"))})
            start = i + 1
        i += 1
    return fields, methods


def scan_file(path, rel, unit):
    raw = open(path, encoding="utf-8", errors="replace").read()
    masked = mask_comments(raw)
    ns_spans = []
    for m in NS_RE.finditer(masked):
        b = m.end() - 1
        ns_spans.append((b, match_brace(masked, b), m.group(1)))
    defs = []
    for m in DEF_RE.finditer(masked):
        b = m.end() - 1
        e = match_brace(masked, b)
        # skip definitions nested inside other class bodies (handled as members)
        name = m.group(2)
        ns = namespaces_at(masked, m.start(), ns_spans)
        qname = "::".join(ns + [name]) if "::" not in name else name
        bases = []
        if m.group(3):
            for part in m.group(3)[1:].split(","):
                part = re.sub(r"\b(public|private|protected|virtual)\b", "", part).strip()
                if part:
                    bases.append(part)
        line = raw.count("\n", 0, m.start()) + 1
        pre = raw[max(0, raw.rfind("\n", 0, max(0, m.start() - 400))):m.start()]
        head = raw[m.start():b]
        partial = bool(re.search(r"partial", pre[-300:] + head, re.I))
        fields, methods = parse_body(raw, masked, b + 1, e - 1)
        defs.append({"kind": m.group(1), "name": qname, "file": rel, "unit": unit, "line": line,
                     "bases": bases, "partial": partial, "fields": fields, "methods": methods,
                     "text": raw[m.start():e + 1][:12000]})
    return raw, defs


MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+(0x00[0-9A-Fa-f]{6})\s+(.*?)\s*(?:\*/)?\s*$")
GLOB_RE = re.compile(r"^\s*extern\s+(?:\"C\"\s+)?(?P<decl>[^;(]*?\b(?P<name>\w+)\s*(?P<arr>(\[[^\]]*\])*))\s*;(?P<com>.*)$")


def scan_funcs_and_globals(raw, rel, unit):
    lines = raw.split("\n")
    fns, globs = [], []
    for i, L in enumerate(lines):
        m = MARK.match(L)
        if m:
            sig = ""
            for j in range(i + 1, min(i + 12, len(lines))):
                t = lines[j].strip()
                if not t or t.startswith(("//", "/*", "*")):
                    continue
                sig = t
                if "{" in t or t.endswith(")") or t.endswith(";"):
                    break
            fns.append({"addr": m.group(1).upper().replace("0X", "0x"), "name": m.group(2), "file": rel,
                        "unit": unit, "line": i + 1, "sig": sig[:240]})
        g = GLOB_RE.match(L)
        if g:
            name = g.group("name")
            com = g.group("com")
            addr = None
            mg = re.match(r"g_(00[0-9A-F]{6})$", name)
            if mg:
                addr = "0x" + mg.group(1)
            else:
                ma = re.search(r"0x(00[2-4][0-9A-Fa-f]{5})\b", com)
                if ma:
                    addr = "0x" + ma.group(1).upper()
            if addr:
                decl = g.group("decl")
                typ = decl[:decl.rfind(name)].strip()
                globs.append({"addr": addr, "name": name, "type": typ, "arr": g.group("arr") or "",
                              "comment": com.strip()[:160], "file": rel, "unit": unit})
    return fns, globs


# ------------------------------------------------------------------ main
def main():
    os.makedirs(os.path.join(OUT, "classes"), exist_ok=True)
    all_defs, all_fns, all_globs = [], [], []
    for rel in sorted(layout.file_units()):
        u, fn, p = layout.unit_of_file(rel), os.path.basename(rel), os.path.join(paths.ROOT, rel)
        if not os.path.isfile(p) or not fn.endswith((".h", ".c", ".cpp", ".cc", ".S")):
            continue
        if fn.endswith(".S"):
            raw = open(p, encoding="utf-8", errors="replace").read()
            defs = []
        else:
            raw, defs = scan_file(p, rel, u)
        fns, globs = scan_funcs_and_globals(raw, rel, u)
        all_defs += defs
        all_fns += fns
        all_globs += globs

    # methods defined per class (from FUNC marker names)
    defined_in = collections.defaultdict(collections.Counter)      # class -> Counter(tu)
    for f in all_fns:
        nm = f["name"].split("(")[0]
        if "::" in nm:
            cls = nm.rsplit("::", 1)[0]
            a = int(f["addr"], 16)
            tu = fidx[a]["tu"] if a in fidx else None
            defined_in[cls][tu] += 1
            f["class"] = cls
            f["tu"] = tu
        else:
            a = int(f["addr"], 16)
            f["tu"] = fidx[a]["tu"] if a in fidx else None

    by_name = collections.defaultdict(list)
    for d in all_defs:
        by_name[d["name"]].append(d)

    # ownership
    tu_unit = {}
    for f in funcs:
        tu_unit.setdefault(f["tu"], unit_of_addr(int(f["start"], 16)))
    classes = {}
    for name, ds in by_name.items():
        users = sorted({d["unit"] for d in ds})
        owner_tu, why = None, None
        r = rtti_by_name.get(name)
        if r and r.get("tu"):
            owner_tu, why = r["tu"], "rtti"
        elif defined_in.get(name):
            owner_tu, why = defined_in[name].most_common(1)[0][0], "methods"
        else:
            full = [d for d in ds if not d["partial"]] or ds
            best = max(full, key=lambda d: len(d["fields"]))
            owner_tu, why = None, "fullest"
            owner_unit = best["unit"]
        owner_unit = tu_unit.get(owner_tu) if owner_tu else owner_unit
        classes[name] = {"name": name, "owner_tu": owner_tu, "owner_unit": owner_unit, "why": why,
                         "users": users, "n_defs": len(ds), "shared": len(users) > 1 or bool(r),
                         "rtti": r["mangled"] if r else None,
                         "bases_rtti": r["bases"] if r else [],
                         "methods_defined": dict(defined_in.get(name, {}))}

    # globals
    gl = collections.defaultdict(list)
    for g in all_globs:
        gl[g["addr"]].append(g)

    json.dump({"classes": classes, "defs": [{k: v for k, v in d.items() if k != "text"} for d in all_defs],
               "functions": all_fns, "globals": {k: v for k, v in sorted(gl.items())}},
              open(os.path.join(OUT, "inventory.json"), "w"), indent=0)

    # dossiers
    for name, c in classes.items():
        ds = by_name[name]
        L = ["# %s" % name,
             "owner: %s (%s), decided by %s; defined in %d unit(s): %s" % (
                 c["owner_tu"], c["owner_unit"], c["why"], len(c["users"]), ", ".join(c["users"]))]
        if c["rtti"]:
            r = rtti_by_name[name]
            L.append("RTTI %s, typeinfo %s, bases (RTTI) %s" % (r["mangled"], r["typeinfo_fn"], ", ".join(r["bases"]) or "-"))
            for v in r["vtables"]:
                L.append("  vtable %s %s this_delta %s, %s slots:" % (v["addr"], v["kind"], v["this_delta"], v["len"]))
                for s in v["slots"]:
                    L.append("    [%2d] %-6s %s %s" % (s["slot"], s["off"], s["fn"], s["name"] or ""))
        if c["methods_defined"]:
            L.append("methods defined (FUNC markers), by translation unit: %s" % c["methods_defined"])
            for f in all_fns:
                if f.get("class") == name:
                    L.append("  %s %-50s %s  [%s]" % (f["addr"], f["name"][:50], f["sig"][:120], f["file"]))
        # merged field table
        tab = collections.defaultdict(lambda: collections.defaultdict(set))
        for d in ds:
            for fl in d["fields"]:
                if fl["off"] is not None:
                    tab[fl["off"]][(fl["name"], fl["type"] + fl["arr"])].add(d["unit"])
        if tab:
            L.append("")
            L.append("merged fields (offset as documented in comments; original egcs layout):")
            for off in sorted(tab):
                parts = ["%s %s (%s)" % (t, n, ",".join(sorted(us))) for (n, t), us in sorted(tab[off].items())]
                L.append("  0x%04X  %s" % (off, " | ".join(parts)))
        L.append("")
        for d in sorted(ds, key=lambda d: (d["unit"] != c["owner_unit"], d["partial"], d["unit"])):
            L.append("----- %s:%d  (%s%s)" % (d["file"], d["line"], "owner unit, " if d["unit"] == c["owner_unit"] else "",
                                           "partial" if d["partial"] else "full"))
            L.append(d["text"])
            L.append("")
        fn = re.sub(r"[^\w]+", "_", name).strip("_")
        open(os.path.join(OUT, "classes", fn + ".txt"), "w", encoding="utf-8", newline="\n").write("\n".join(L) + "\n")

    G = ["# globals declared by the first pass: address, then each unit's name, type and comment", ""]
    for a, gs in sorted(gl.items()):
        G.append(a)
        for g in gs:
            G.append("    %-6s %-28s %-36s %s" % (g["unit"], g["name"], (g["type"] + g["arr"])[:36], g["comment"][:90]))
    open(os.path.join(OUT, "globals.txt"), "w", encoding="utf-8", newline="\n").write("\n".join(G) + "\n")

    shared = [c for c in classes.values() if c["shared"]]
    print("definitions: %d of %d classes/structs; shared (2+ units or RTTI): %d; functions: %d; globals: %d" % (
        len(all_defs), len(classes), len(shared), len(all_fns), len(gl)))
    own = collections.Counter(c["why"] for c in shared)
    print("ownership of shared classes:", dict(own))


if __name__ == "__main__":
    main()
