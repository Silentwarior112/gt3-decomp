"""The source tree and the work units.

The sources are grouped by what they are (src/game/, src/engine/, src/sdk/,
src/runtime/; include/ mirrors it).  The work units (ref/units.json: 143
address ranges of the original program, the first passes' unit of work)
still name the stretches the period-accurate build links from the C
(hybrid.txt) and the per-unit notes (ref/matching/<u>/, ref/data/<u>/).
ref/file_units.json is the frozen map from each source file to its work unit
(the unit of its first function; a merged file belongs to its first part's
unit; a data-only file to its neighbour's).  A file added later and not in
the map gets the unit of its first FUNC marker.

    python tools/layout.py --files u042      the sources of a work unit
    python tools/layout.py --unit src/game/race/core/T121_race_entry.cpp
    python tools/layout.py --tree            the folders and their file counts
"""
import argparse
import collections
import json
import os
import sys

import paths

ROOT = paths.ROOT
SRC = os.path.join(ROOT, "src")
SOURCE_EXT = (".c", ".cpp", ".cc", ".S")
_FU = None


def file_units():
    """{src-relative path (src/...): work unit}."""
    global _FU
    if _FU is None:
        _FU = json.load(open(os.path.join(paths.REF, "file_units.json")))
    return _FU


def sources():
    """Every compilable source under src/, as src/... paths, sorted."""
    out = []
    for dp, _, fns in os.walk(SRC):
        for f in fns:
            if f.endswith(SOURCE_EXT):
                out.append(os.path.relpath(os.path.join(dp, f), ROOT).replace(os.sep, "/"))
    return sorted(out)


def _unit_of_address(a):
    units = json.load(open(os.path.join(paths.REF, "units.json")))
    for u, s in units.items():
        for lo, hi in s["ranges"]:
            if int(lo, 16) <= a < int(hi, 16):
                return u
    return None


def unit_of_file(rel):
    """The work unit of a source (src/... path), or None."""
    rel = rel.replace("\\", "/")
    u = file_units().get(rel)
    if u is not None:
        return u
    import funcmap
    ms = funcmap.markers().get(rel)
    return _unit_of_address(min(a for _, a, _ in ms)) if ms else None


def files_of_unit(u):
    """The sources of a work unit (src/... paths), sorted."""
    have = set(sources())
    out = [f for f, v in file_units().items() if v == u and f in have]
    out += [f for f in have if f not in file_units() and unit_of_file(f) == u]
    return sorted(set(out))


def files_and_headers_of_unit(u):
    """The sources and private headers (the old unit header, *_internal.h,
    and other headers that lived in the unit's folder) of a work unit."""
    return sorted(f for f, v in file_units().items() if v == u and os.path.isfile(os.path.join(ROOT, f)))


def unit_dirs():
    """[(unit, absolute folder, [file names])]: each work unit's files grouped
    by the folder they are in (what the first passes' tools read as
    src/<unit>/)."""
    out = []
    by = collections.defaultdict(lambda: collections.defaultdict(list))
    for f, u in file_units().items():
        if os.path.isfile(os.path.join(ROOT, f)):
            by[u][os.path.dirname(f)].append(os.path.basename(f))
    for u in sorted(by):
        for d in sorted(by[u]):
            out.append((u, os.path.join(ROOT, d), sorted(by[u][d])))
    return out


HEADER_TREES = ("game", "engine", "sdk", "runtime")      # include/<tree>/...: the canonical headers
_HDR = None


def _headers():
    global _HDR
    if _HDR is None:
        _HDR = {}
        inc = os.path.join(ROOT, "include")
        for t in HEADER_TREES:
            for dp, _, fns in os.walk(os.path.join(inc, t)):
                for f in fns:
                    if f.endswith(".h"):
                        _HDR[f] = os.path.relpath(os.path.join(dp, f), inc).replace(os.sep, "/")
    return _HDR


def header_names():
    """The canonical headers' file names (unique), sorted."""
    return sorted(_headers())


def header_rel(fn):
    """A canonical header's path as it is #included (from include/)."""
    return _headers()[os.path.basename(fn)]


def header_path(fn):
    """A canonical header's absolute path."""
    return os.path.join(ROOT, "include", header_rel(fn))


def link_order(rels):
    """rels in the original's link order: by first function, data-only files
    where datalayout.tu_order() puts them (DATA_TU / LINK_AFTER), so a
    build never depends on where a file lives in the tree."""
    import datalayout
    import funcmap
    marks = funcmap.markers()
    tu = {r: i for i, r in enumerate(datalayout.tu_order())}
    return sorted(rels, key=lambda r: (min((a for _, a, _ in marks.get(r, [])), default=1 << 31),
                                       tu.get(r, 1 << 30), r))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--files", metavar="UNIT")
    ap.add_argument("--unit", metavar="FILE")
    ap.add_argument("--tree", action="store_true")
    a = ap.parse_args()
    if a.files:
        fs = files_of_unit(a.files)
        if not fs:
            sys.exit("no sources for %s" % a.files)
        print("\n".join(fs))
    elif a.unit:
        print(unit_of_file(a.unit) or "?")
    elif a.tree:
        c = collections.Counter(os.path.dirname(f) for f in sources())
        for d in sorted(c):
            print("%-34s %3d" % (d, c[d]))
    else:
        ap.print_help()


if __name__ == "__main__":
    main()
