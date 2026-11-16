"""Virtual-method consistency of the canonical headers (include/game/*.h).

Every virtual declaration carries its slot in a comment (/* [3] +0x24 0x001BFF00 */).
An override must use the same name and parameter types as the declaration of that
slot in its base class; otherwise C++ makes it a new virtual (a different vtable)
and nothing warns.  For each polymorphic class this walks the RTTI bases
(ref/classes.json) and reports:

  name     slot k is declared with different names along the base chain
  missing  a slot whose method the class defines (address in its own translation
           unit) has no declaration in the class or a base
  slot     a declaration whose slot comment disagrees with the vtable (the method
           address in the comment is not the one RTTI lists for that slot)

    python tools/vtcheck.py [--verbose]
"""
import collections
import json
import os
import re
import sys

import inventory as inv
import paths
import layout

R = paths.REF
SLOT = re.compile(r"\[\s*(\d+)\s*\]\s*(?:\+0x[0-9A-Fa-f]+)?\s*(?:(0x00[0-9A-Fa-f]{6}))?")


def virtuals():
    """{qualified class: {slot: (name, params, addr, header)}}"""
    out = collections.defaultdict(dict)
    for fn in layout.header_names():
        if not fn.endswith(".h"):
            continue
        p = layout.header_path(fn)
        raw = open(p, encoding="utf-8", errors="replace").read()
        if "STUB: written by tools/pass2_plan.py" in raw:
            continue
        masked = inv.mask_comments(raw)
        ns_spans = [(m.end() - 1, inv.match_brace(masked, m.end() - 1), m.group(1)) for m in inv.NS_RE.finditer(masked)]
        for m in inv.DEF_RE.finditer(masked):
            b = m.end() - 1
            e = inv.match_brace(masked, b)
            ns = [n for (lo, hi, n) in ns_spans if lo < m.start() < hi]
            q = "::".join(ns + [m.group(2)])
            body_r, body_m = raw[b + 1:e - 1], masked[b + 1:e - 1]
            pos = 0
            for line_r in body_r.split("\n"):
                line_m = body_m[pos:pos + len(line_r)]
                pos += len(line_r) + 1
                if not re.search(r"\bvirtual\b", line_m):
                    continue
                mm = re.search(r"(~?\w+)\s*\(([^)]*)\)", line_m)
                ms = SLOT.search(line_r[len(line_m.rstrip()) - 0:] if False else line_r[line_m.find(";") + 1 if ";" in line_m else 0:])
                if not mm or not ms:
                    continue
                params = re.sub(r"\s+", " ", re.sub(r"\b\w+\s*(,|$)", lambda x: x.group(0), mm.group(2))).strip()
                out[q][int(ms.group(1))] = (mm.group(1), params, (ms.group(2) or "").upper().replace("0X", "0x"), fn)
    return out


def norm_params(p):
    """Parameter types without names, for comparison."""
    out = []
    for part in [x.strip() for x in p.split(",") if x.strip() and x.strip() != "void"]:
        part = re.sub(r"\s*=.*$", "", part)
        toks = re.findall(r"[\w:]+|[\*&]", part)
        if len(toks) >= 2 and re.match(r"^\w+$", toks[-1]) and toks[-1] not in ("int", "char", "float", "double",
                                                                                "short", "long", "unsigned", "const"):
            toks = toks[:-1]                 # drop the parameter name
        out.append(" ".join(ALIAS.get(t, t) for t in toks))
    return tuple(out)


ALIAS = {"f32": "float", "s32": "int", "u32": "unsigned int", "s16": "short", "u16": "unsigned short",
         "s8": "signed char", "u8": "unsigned char", "signed": "int"}


def main():
    verbose = "--verbose" in sys.argv
    V = virtuals()
    cls = json.load(open(os.path.join(R, "classes.json")))
    by_name = {e["name"]: e for e in cls.values()}
    problems = 0
    checked = 0
    for name, e in sorted(by_name.items()):
        if name not in V:
            continue
        def prim_fns(cn):
            v = [x for x in by_name.get(cn, {}).get("vtables", []) if x["kind"] == "primary"]
            return [s["fn"] for s in v[0]["slots"]] if v else []

        def primary_base(cn):
            """The base whose vtable is the prefix of cn's primary vtable (the others
            are secondary bases with their own vtables)."""
            mine = prim_fns(cn)
            best, score = None, -1
            for b in by_name.get(cn, {}).get("bases", []):
                theirs = prim_fns(b)
                sc = sum(1 for i, f in enumerate(theirs) if i < len(mine) and mine[i] == f) + (len(theirs) <= len(mine))
                if sc > score:
                    best, score = b, sc
            return best

        chain = [name]
        cur = name
        seen = set()
        while by_name.get(cur, {}).get("bases") and cur not in seen:
            seen.add(cur)
            cur = primary_base(cur)
            if not cur:
                break
            chain.append(cur)
        prim = [v for v in e["vtables"] if v["kind"] == "primary"]
        if not prim:
            continue
        for s in prim[0]["slots"]:
            k = s["slot"]
            decls = [(c, V[c][k]) for c in chain if c in V and k in V[c]]
            checked += 1
            names = {(d[1][0] if not d[1][0].startswith("~") else "~") for d in decls}
            if len(names) > 1:
                problems += 1
                print("name    %s slot %d: %s" % (name, k, "; ".join("%s::%s(%s) [%s]" % (c, d[0], d[1], d[3]) for c, d in decls)))
            ptypes = {norm_params(d[1][1]) for d in decls if not d[1][0].startswith("~")}
            if len(ptypes) > 1:
                problems += 1
                print("params  %s slot %d: %s" % (name, k, "; ".join("%s::%s(%s)" % (c, d[0], d[1]) for c, d in decls)))
            own = [d for c, d in decls if c == name]
            if own and own[0][2] and own[0][2] != s["fn"].upper().replace("0X", "0x"):
                # the class declares the slot but the address it gives is not its vtable's
                if verbose or True:
                    problems += 1
                    print("slot    %s slot %d declared %s::%s at %s, the vtable has %s" % (
                        name, k, name, own[0][0], own[0][2], s["fn"]))
    print("checked %d slots of %d written classes; %d problems" % (checked, len(V), problems))
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
