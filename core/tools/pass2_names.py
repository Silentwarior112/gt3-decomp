"""Collect the canonical names the phase-1 headers chose: every declaration in
include/game/*.h that carries an address comment (functions, methods, virtuals,
globals).  A declaration may span lines; its address is in the comment after its ';'
(or in a comment inside the declaration).  Writes ref/pass2/canonical.csv:

    addr, kind (func|method|virtual|global), canonical name (qualified), header,
    first-pass name (FUNC marker or g_ name), declaration

    python tools/pass2_names.py
"""
import csv
import json
import os
import re

import inventory as inv
import paths
import layout

P2 = os.path.join(paths.REF, "pass2")
ADDR = re.compile(r"0x(00[1-4][0-9A-Fa-f]{5})\b")
ATTR = re.compile(r"\s*__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)\s*")


def scan(path):
    raw = open(path, encoding="utf-8", errors="replace").read()
    if "STUB: written by tools/pass2_plan.py" in raw:
        return []
    m0 = inv.mask_comments(raw)
    # each conditional's branches one at a time: with both, a `class X {'
    # written twice leaves a brace open and what follows lands in the wrong
    # scope (T537's typeinfo node after its namespace, u110).  The first
    # branches' names, then the other branches' at addresses not seen
    first = _scan(path, raw, _branches(m0, True))
    seen = {a for a, *_ in first}
    return first + [t for t in _scan(path, raw, _branches(m0, False)) if t[0] not in seen]


def _branches(masked, keep_first):
    """masked with, in every #if group that has an #else / #elif, the other
    branches (keep_first) or the first one blanked (same length)."""
    lines = masked.split("\n")

    def word(L):
        d = L.lstrip()
        if not d.startswith("#"):
            return None
        w = d[1:].lstrip().split(None, 1)[:1]
        return w[0] if w else ""
    has_else, st = {}, []
    for i, L in enumerate(lines):
        w = word(L)
        if w in ("if", "ifdef", "ifndef"):
            st.append(i)
            has_else[i] = False
        elif w in ("else", "elif") and st:
            has_else[st[-1]] = True
        elif w == "endif" and st:
            st.pop()
    out, st = [], []                        # st: [#if line, in its first branch]
    for i, L in enumerate(lines):
        w = word(L)
        if w is not None:
            if w in ("if", "ifdef", "ifndef"):
                st.append([i, True])
            elif w in ("else", "elif") and st:
                st[-1][1] = False
            elif w == "endif" and st:
                st.pop()
            out.append(L)
            continue
        blank = any((not f) if keep_first else (f and has_else[g]) for g, f in st)
        out.append(" " * len(L) if blank else L)
    return "\n".join(out)


def _scan(path, raw, masked):
    # blank the preprocessor directives, continuation lines included (same length)
    masked = re.sub(r"(?m)^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", lambda x: re.sub(r"[^\n]", " ", x.group(0)), masked)
    ns_spans = [(m.end() - 1, inv.match_brace(masked, m.end() - 1), m.group(1)) for m in inv.NS_RE.finditer(masked)]
    cls_spans = [(m.end() - 1, inv.match_brace(masked, m.end() - 1), m.group(2)) for m in inv.DEF_RE.finditer(masked)]
    out = []
    start = 0
    i = -1
    n = len(masked)
    while i + 1 < n:
        i += 1
        ch = masked[i]
        if ch == "}":
            start = i + 1
            continue
        if ch == "{":
            seg = re.sub(r"(?m)^[ \t]*#.*$", lambda x: " " * len(x.group(0)), masked[start:i])
            head = re.sub(r"\s+", " ", seg).strip()
            head = re.sub(r"^(public|private|protected)\s*:\s*", "", head)
            # a function body (an inline definition): `T f(args) const {`, `C(args) : m(0) {`
            if "(" in head and re.search(r"\)\s*(const\s*)?(throw\s*\([^)]*\)\s*)?$|\)\s*:\s*[\w:]+\s*\(.*\)$", head) \
                    and not head.startswith(("namespace", "extern", "enum", "struct", "class", "union", "typedef")):
                e = inv.match_brace(masked, i)              # just past the body's '}'
                eol = raw.find("\n", e)
                eol = len(raw) if eol < 0 else eol
                k = len(seg) - len(seg.lstrip())
                # its address: after the body, else on the line of its '{' (not a comment above)
                bol = raw.rfind("\n", 0, i) + 1
                ma = ADDR.search(raw[e:eol] + " " + raw[max(bol, start + k):i])
                if ma:
                    st = start
                    ns = [nn for (a, b, nn) in ns_spans if a < st < b]
                    cls = [nn for (a, b, nn) in cls_spans if a < st < b]
                    scope = "::".join(ns + cls[-1:]) if cls else "::".join(ns)
                    decl = re.sub(r"\s*:\s*[\w:]+\s*\(.*$", "", head) if re.search(r"\)\s*:", head) else head
                    mp = re.match(r"[^(]*\(\s*\*\s*(\w+)\s*\(", decl)
                    m = mp or re.search(r"(~?\w+|operator\s*[^\s(]+)\s*\(", decl)
                    if m:
                        kind = "virtual" if re.search(r"\bvirtual\b", decl) else ("method" if cls else "func")
                        q = (scope + "::" if scope else "") + m.group(1)
                        out.append(("0x" + ma.group(1).upper(), kind, q,
                                    os.path.relpath(path, paths.ROOT).replace("\\", "/"), decl + " {...}"))
                i = e - 1
            # a variable with a brace initialiser: `T x[] = {   /* 0x... */ ... };`
            # (its address on the '{' line, else after the '};')
            # (an attribute after the name, `x[] __attribute__((aligned(64))) =',
            # is not part of it: u075's T276)
            head_na = ATTR.sub(" ", head).rstrip()
            mv = re.search(r"(\w+)\s*(?:\[[^\]]*\]\s*)*=$", head_na)
            if mv and not head.startswith(("typedef", "enum", "using")) and "(" not in head_na:
                e = inv.match_brace(masked, i)
                bol = raw.rfind("\n", 0, i) + 1
                eol1 = raw.find("\n", i)
                eol1 = len(raw) if eol1 < 0 else eol1
                eol2 = raw.find("\n", e)
                eol2 = len(raw) if eol2 < 0 else eol2
                ma = ADDR.search(raw[i:eol1] + " " + raw[bol:i] + " " + raw[e:eol2])
                if ma:
                    ns = [nn for (a, b, nn) in ns_spans if a < start < b]
                    cls = [nn for (a, b, nn) in cls_spans if a < start < b]
                    scope = "::".join(ns + cls[-1:])
                    out.append(("0x" + ma.group(1).upper(), "global",
                                (scope + "::" if scope else "") + mv.group(1),
                                os.path.relpath(path, paths.ROOT).replace("\\", "/"), head + " {...}"))
                i = e - 1
            start = i + 1
            continue
        if ch != ";":
            continue
        # blank the preprocessor lines (same length, so offsets into raw stay valid)
        seg = re.sub(r"(?m)^[ \t]*#.*$", lambda x: " " * len(x.group(0)), masked[start:i])
        decl = re.sub(r"\s+", " ", seg).strip()
        eol = raw.find("\n", i)
        eol = len(raw) if eol < 0 else eol
        k = len(seg) - len(seg.lstrip())                    # skip the comments before the declaration
        com = raw[i + 1:eol] + " " + raw[start + k:i]      # after the ';', else inside the declaration
        st = start
        start = i + 1
        decl = re.sub(r"^(public|private|protected)\s*:\s*", "", decl)
        # an attribute is not part of the declarator (`f32 x[4]
        # __attribute__((aligned(16)));' names x, u094's T411)
        decl = ATTR.sub(" ", decl).strip()
        # nor is an initialiser (`void *top = (void *)0x01F00000;' is no
        # function, u125's T532): cut at the first `=' outside parentheses
        depth = 0
        for j, c in enumerate(decl):
            depth += c in "(["
            depth -= c in ")]"
            if c == "=" and depth == 0 and decl[j + 1:j + 2] != "=" and decl[j - 1:j] not in "=!<>":
                if not re.search(r"\boperator\s*$", decl[:j]):
                    decl = decl[:j].rstrip()
                break
        if "->" in decl or re.search(r"\w\s*\.\s*[A-Za-z_]", decl):
            continue                      # a member assignment in a body, no declaration
        ma = ADDR.search(com)
        if not ma or not decl or decl.startswith(("typedef", "using", "#", "static_assert", "EGCS_SIZE")):
            continue
        if re.match(r"^(class|struct|union|enum)\s+[\w:]+$", decl):
            continue                      # a forward declaration
        addr = "0x" + ma.group(1).upper()
        ns = [n for (a, b, n) in ns_spans if a < st < b]
        cls = [n for (a, b, n) in cls_spans if a < st < b]
        scope = "::".join(ns + cls[-1:]) if cls else "::".join(ns)
        # a declarator in parentheses: `T (*name(args))[4]` (a function returning a
        # pointer) or `T (*name[25])(args)` / `T (*name)(args)` (a function pointer)
        mp = re.match(r"[^(]*\(\s*\*\s*(\w+)\s*([\[\)\(])", decl)     # only the first '(' is the declarator
        if mp and mp.group(2) != "(":
            if cls:
                continue                  # a data member
            nm, kind = mp.group(1), "global"
        elif "(" in decl:
            m = mp or re.search(r"(~?\w+|operator\s*[^\s(]+)\s*\(", decl)
            if not m:
                continue
            nm = m.group(1)
            kind = "virtual" if re.search(r"\bvirtual\b", decl) else ("method" if cls else "func")
        else:
            if cls:
                continue                  # a data member: its comment is an offset, not an address
            # a definition with an initialiser: `int g_00350B0C = -1;' names
            # g_00350B0C (the data pass's definitions), not the value
            decl = re.sub(r"\s*=[^=].*$", "", decl)
            m = re.search(r"(\w+)\s*(\[[^\]]*\])*\s*$", decl)
            if not m:
                continue
            nm = m.group(1)
            kind = "global"
        q = (scope + "::" if scope else "") + nm
        # the declaration as written (masked text blanks string literals: extern "C")
        shown = re.sub(r"/\*.*?\*/|//[^\n]*", " ", raw[st + k:i], flags=re.S)
        shown = re.sub(r"(?m)^[ \t]*#.*$", " ", shown)
        shown = re.sub(r"^(public|private|protected)\s*:\s*", "", re.sub(r"\s+", " ", shown).strip())
        out.append((addr, kind, q, os.path.relpath(path, paths.ROOT).replace("\\", "/"), shown or decl))
    return out


def main():
    invj = json.load(open(os.path.join(P2, "inventory.json")))
    first = {}
    for f in invj["functions"]:
        first.setdefault(f["addr"], f["name"])
    rows = []
    for fn in layout.header_names():
        if fn.endswith(".h"):
            rows += scan(layout.header_path(fn))
    with open(os.path.join(P2, "canonical.csv"), "w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh)
        w.writerow(["addr", "kind", "name", "header", "first_pass", "decl"])
        for addr, kind, q, h, decl in sorted(rows):
            fp = first.get(addr) or ("g_%s" % addr[2:] if kind == "global" else "")
            w.writerow([addr, kind, q, h, fp, decl[:200]])
    print("canonical declarations: %d (%d headers written)" % (len(rows), len({r[3] for r in rows})))
    class_map()


def class_map():
    """Rebuild ref/pass2/class_headers.json (shared class -> its canonical header)
    from where each class or typedef is defined now: integration moves classes
    between headers.  A class found nowhere keeps its planned header if that
    header still names it, else maps to '?'."""
    import hdrall
    p = os.path.join(P2, "class_headers.json")
    old = json.load(open(p))
    hs = [fn for fn in layout.header_names() if fn.endswith(".h")]
    found = {}
    for name, where in hdrall.type_names(hs, templates=True).items():
        for ns, fn in where.items():
            found.setdefault((ns + "::" if ns else "") + name, "include/" + layout.header_rel(fn))
    new, moved, lost = {}, 0, []
    for c, h in old.items():
        if c in found:
            new[c] = found[c]
            moved += found[c] != h
        elif os.path.exists(os.path.join(paths.ROOT, h)) and re.search(
                r"\b%s\b" % re.escape(c.rsplit("::", 1)[-1]), open(os.path.join(paths.ROOT, h), encoding="utf-8",
                                                                 errors="replace").read()):
            new[c] = h
        else:
            new[c] = "?"
            lost.append(c)
    for c, h in found.items():
        new.setdefault(c, h)
    json.dump(new, open(p, "w"), indent=1, sort_keys=True)
    print("class_headers.json: %d classes (%d moved since the last run, %d not found: %s)" % (
        len(new), moved, len(lost), ", ".join(lost[:12])))


if __name__ == "__main__":
    main()
