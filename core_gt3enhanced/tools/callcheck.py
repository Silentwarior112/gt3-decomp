"""Cheap fidelity check of decompiled source: does each function's source mention
every function the machine code calls?

For each FUNC block (from its marker to the next marker) the callees of the
original (jal and tail-call j targets, ref/funcs.json) are looked up by name:
F_00XXXXXX, the name on the callee's own FUNC marker, or its ref/symbols.csv name
(the last `::` component, without parameters).  A callee whose name does not
appear in the block's code is reported as missing.  Comments do not count,
except an `implicit:` comment, which names calls the compiler emits without
the source spelling them (a temporary's destructor, an inlined helper, a
macro's call):  /* implicit: ~Vector, memcpy */.  Heuristic: a renamed callee
or a call through a macro can show up as a false report.

    python tools/callcheck.py <unit> [--verbose] [--comments]     or   --all
    --comments: also count names in ordinary comments (the first pass's count)
"""
import collections
import csv
import json
import os
import re
import sys

import check_coverage as cov
import coreelf as E
import paths

R = paths.REF
IMPLICIT = ("__throw", "__rethrow", "__sjthrow", "__sjpopnthrow", "__eh_", "__cp_", "__start_cp_handler",
            "__uncatch", "__check_eh_spec", "std::terminate", "__terminate", "__unwinding_cleanup",
            "__get_eh", "__empty", "__get_dynamic_handler_chain")
COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"', re.S)
IMPLICIT_NOTE = re.compile(r"^(?:/\*|//)\s*implicit:", re.I)


def code_only(text):
    """The block without comments and string literals, keeping `implicit:` comments."""
    return COMMENT.sub(lambda m: m.group(0) if IMPLICIT_NOTE.match(m.group(0)) else
                       ("" if m.group(0)[0] != '"' else '""'), text)


MARK = re.compile(r"^\s*(?://|/\*)\s*FUNC\s+(0x00[0-9A-Fa-f]{6})\s+(.*?)\s*(?:\*/)?\s*$")


def short_names(full):
    """Candidate source spellings of a symbol name."""
    out = set()
    if not full:
        return out
    base = re.sub(r"^\([^)]*\)::", "", full).split("(")[0].strip()   # (anon)::X -> X
    out.add(base)
    if "::" in base:
        last = base.rsplit("::", 1)[1]
        out.add(last)
        if last.startswith("operator new"):
            out.add("new")              # a class's operator new / delete: new / delete expressions
        if last.startswith("operator delete"):
            out.add("delete")
        if last.startswith("~"):
            out.add("delete")           # a deleting destructor is called by delete
            out.add(last[1:])
    return {re.sub(r"<.*>", "", n) for n in out if n}


def main():
    args = sys.argv[1:]
    verbose = "--verbose" in args
    comments = "--comments" in args
    args = [a for a in args if not a.startswith(("--v", "--comments"))]
    units = json.load(open(os.path.join(R, "units.json")))
    todo = list(units) if args and args[0] == "--all" else args
    funcs = {int(f["start"], 16): f for f in json.load(open(os.path.join(R, "funcs.json")))}
    rows = list(csv.DictReader(open(os.path.join(R, "symbols.csv"), encoding="utf-8")))
    syms = {int(r["addr"], 16): r["name"] for r in rows}
    cls = {int(r["addr"], 16): r["class"] for r in rows if r["class"]}
    img = E.Image()
    # names on FUNC markers, and each marker's source block
    marker_name = {}
    blocks = {}
    for dp, dn, fn in os.walk(os.path.join(paths.ROOT, "src")):
        for f in fn:
            if not f.endswith((".c", ".cpp", ".cc", ".h", ".S", ".s")):
                continue
            L = open(os.path.join(dp, f), encoding="utf-8", errors="replace").read().splitlines()
            cur, buf = None, []
            for line in L + ["// FUNC 0x00000000 <eof>"]:
                m = MARK.match(line)
                if m:
                    if cur is not None:
                        blocks[cur] = "\n".join(buf)
                    cur = int(m.group(1), 16)
                    marker_name[cur] = m.group(2)
                    buf = []
                elif cur is not None:
                    buf.append(line)
    tot_calls = tot_found = 0
    for u in todo:
        lo, hi = (int(x, 16) for x in units[u]["ranges"][0])
        n_calls = n_found = 0
        missing = []
        for a, f in sorted(funcs.items()):
            if not (lo <= a < hi) or a not in blocks:
                continue
            text = blocks[a] if comments else code_only(blocks[a])
            callees = [int(c, 16) for c in f["callees"]]
            for va in range(a, a + f["size"], 4):         # tail calls
                x = img.word(va)
                if (x >> 26) == 2:
                    t = (x & 0x03FFFFFF) << 2
                    if t in funcs and not (a <= t < a + f["size"]):
                        callees.append(t)
            for c in sorted(set(callees)):
                nm_any = marker_name.get(c) or syms.get(c) or ""
                if any(nm_any.startswith(p) for p in IMPLICIT):
                    continue                    # exception-handling runtime: emitted by the compiler
                n_calls += 1
                names = {"F_%08X" % c, "0x%08X" % c} | short_names(marker_name.get(c)) | short_names(syms.get(c))
                for full in (marker_name.get(c), syms.get(c)):
                    # constructors / destructors of local objects: the class name is enough
                    base = re.sub(r"<.*?>", "", re.sub(r"^\([^)]*\)::", "", full or "").split("(")[0].strip())
                    if "::" in base:
                        k, last = base.rsplit("::", 1)
                        k = k.rsplit("::", 1)[-1]
                        if last in (k, "~" + k):
                            names.add(k)
                if cls.get(c) and cls.get(c) == (marker_name.get(c) or syms.get(c)):
                    names.add(re.sub(r"<.*>", "", cls[c].rsplit("::", 1)[-1]))
                if "operator delete" in nm_any and "~" in marker_name.get(a, ""):
                    names.add(marker_name[a].split("(")[0].rsplit("::", 1)[-1])   # a deleting destructor
                if "dynamic_cast" in nm_any:
                    names.add("dynamic_cast")
                if nm_any.startswith("__builtin_new") or nm_any.startswith("op_new"):
                    names.add("new")
                if any(re.search(r"(?<![\w])" + re.escape(nm) + r"(?![\w])", text) for nm in names if nm):
                    n_found += 1
                else:
                    missing.append((a, c, marker_name.get(c) or syms.get(c) or ""))
        tot_calls += n_calls
        tot_found += n_found
        print("%s: %d/%d calls found in source (%.1f%%)%s" % (u, n_found, n_calls, 100.0 * n_found / max(n_calls, 1),
              "" if not missing else ", %d missing" % len(missing)))
        if verbose:
            for a, c, nm in missing:
                print("   in F_%08X %s: call F_%08X %s" % (a, marker_name.get(a, ""), c, nm))
    if len(todo) > 1:
        print("total: %d/%d (%.1f%%)" % (tot_found, tot_calls, 100.0 * tot_found / max(tot_calls, 1)))


if __name__ == "__main__":
    main()
