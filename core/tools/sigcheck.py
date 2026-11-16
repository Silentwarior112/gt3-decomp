"""Calls whose callee is defined only with another parameter list.

A changed signature (a parameter order, a type) must change the header, the
definition and every call together. C++ compiles each file alone, so a
definition left with the old parameters is a separate overload: every file
compiles, and only the link fails ("UNRESOLVED" in the hybrid report). This
reads the objects tools/score.py compiles (build/score/obj/, refreshed for the
files it scores) and lists every function symbol some object uses and none
defines while a function of the same name is defined with other parameters.

    python tools/sigcheck.py            all objects
    python tools/sigcheck.py u082 u084  only references from these units
    python tools/sigcheck.py --obj build/regress/obj     another build's objects
"""
import collections
import glob
import os
import sys

import paths
import funcmap
import hybrid

OBJ = os.path.join(paths.ROOT, "build", "score", "obj")



def live(objs):
    """The objects whose source file still exists (merged files leave stale
    objects in build/score/obj/: the u040 pilot's sigcheck saw their names)."""
    srcs = set()
    for dp, _, fns in os.walk(os.path.join(paths.ROOT, "src")):
        for fn in fns:
            rel = os.path.relpath(os.path.join(dp, fn), paths.ROOT).replace(os.sep, "/")
            srcs.add(rel.replace("/", "__") + ".o")
    return [o for o in objs if os.path.basename(o) in srcs]

def main():
    args = sys.argv[1:]
    obj_dir = OBJ
    if "--obj" in args:
        i = args.index("--obj")
        obj_dir = args[i + 1]
        del args[i:i + 2]
    units = [a for a in args if not a.startswith("-")]
    hybrid.TC_BIN = paths.ee_bin(None)
    objs = live(sorted(glob.glob(os.path.join(obj_dir, "**", "*.o"), recursive=True)))
    if not objs:
        print("no objects in %s: run tools/score.py first" % obj_dir)
        return 1
    hybrid.nm_all(objs, "--defined-only")
    hybrid.nm_all(objs, "-u")
    defined, used = set(), collections.defaultdict(set)
    for o in objs:
        defined |= {n for _, t, n in hybrid.nm(o, "--defined-only") if t in "TtWw"}
    for o in objs:
        if units and not any(("__%s__" % u) in os.path.basename(o) or ("/%s/" % u) in o.replace("\\", "/")
                             for u in units):
            continue
        for _, t, n in hybrid.nm(o, "-u"):
            used[n].add(os.path.basename(o))
    by_name = collections.defaultdict(set)
    for d in defined:
        q = funcmap.demangle(d)
        if q:
            by_name[q].add(d)
    bad = 0
    for n in sorted(used):
        if n in defined:
            continue
        q = funcmap.demangle(n)
        others = by_name.get(q) if q else None
        if others:
            bad += 1
            print("%s\n  used by   %s\n  defined   %s" % (n, ", ".join(sorted(used[n])), ", ".join(sorted(others))))
    print("%d call(s) to a function defined only with another parameter list" % bad)
    unres = unresolved(used, defined)
    for n in unres:
        print("%s\n  used by   %s\n  no definition and no original address (declare it with /* 0x... */ on its line)"
              % (n, ", ".join(sorted(used[n]))))
    print("%d symbol(s) the hybrid link could not resolve" % len(unres))
    return 1 if bad or unres else 0


def unresolved(used, defined):
    """What tools/hybrid.py's link would report UNRESOLVED: used, defined by no
    C object, and with no original address by any of the link's rules."""
    import re
    import relink
    auto = set()
    for fn in ("undefined_syms_auto.txt", "undefined_funcs_auto.txt"):
        p = os.path.join(relink.SPLAT, fn)
        if os.path.exists(p):
            for L in open(p):
                m = re.match(r"\s*(\S+)\s*=", L)
                if m:
                    auto.add(m.group(1))
    asm_glob = set()
    asm = glob.glob(os.path.join(paths.ROOT, "build", "*", "link", "obj", "asm", "*.s.o"))
    asm += glob.glob(os.path.join(paths.ROOT, "build", "*", "link", "obj", "asm", "data", "*.o"))
    if asm:
        hybrid.nm_all(asm, "--defined-only", "-g")
        for p in asm:
            asm_glob |= {n for _, t, n in hybrid.nm(p, "--defined-only", "-g")}
    every = set(defined)
    for o in live(glob.glob(os.path.join(OBJ, "**", "*.o"), recursive=True)):
        every |= {n for _, t, n in hybrid.nm(o, "--defined-only")}
    decl, names, vts = hybrid.declared(), hybrid.table_names(), hybrid.vtables()
    marks = funcmap.by_name()
    out = []
    for n in sorted(used):
        if (n in every or n in asm_glob or n in auto or n in marks or n in decl or n in names
                or n in hybrid.CRT0_SYMBOLS or n in hybrid.SUPPORT or hybrid.ADDR_IN_NAME.search(n)
                or n in ("_gp", "__bss_end", "__heap_start", "_end")):
            continue
        if n.startswith("_vt$") and (n in vts or re.sub(r"^(_vt\$[^$]+)\$.*$", r"\1$*", n) in vts):
            continue
        if funcmap.demangle(n):                         # a C++ function: the link maps it by the function map
            continue
        if hybrid.extern_line(n) is not None:
            continue
        if n in DEAD_STORE_VTABLES:                     # tools/fulllink.py: only a dead vptr store uses it
            continue
        out.append(n)
    return out


# (tools/fulllink.py's list; ref/matching/followups.md, T074)
DEAD_STORE_VTABLES = {"_vt$Q29gt2k_menu11CommentMenu"}


if __name__ == "__main__":
    sys.exit(main())
