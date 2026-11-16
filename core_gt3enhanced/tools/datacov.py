#!/usr/bin/env python3
"""Coverage of a unit's original data by its compiled sections, per area:
every section at its chosen base (datalayout.choose_base; a section with no
range is searched for by its bytes in the whole area), EXACT or not, in
address order over the unit's span, with the gaps (original bytes no
section covers) and overlaps between them.

    python tools/datacov.py --unit u040
    python tools/datacov.py src/game/menu/records/T09*.cpp

(The u040 pilot's view, build/scratch/u040/cov.py: a unit's whole span shows
data that only other data points at, which per-file ranges give to the
previous file, ref/data/u040/notes.md.)
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import datalayout                                       # noqa: E402
import paths                                            # noqa: E402
import score                                            # noqa: E402
import hybrid                                           # noqa: E402

AREA = {k: (lo, hi) for k, lo, hi in datalayout.AREAS}


def img_at(a, n):
    off = a - datalayout.VRAM + datalayout.ROM
    return datalayout.image()[off:off + n]


def search(obj, sec, lo, hi):
    data, mask = datalayout.masked_bytes(obj, sec)
    if len(data) < 4:
        return []
    pat = b"".join(b"(?s:.)" if i in mask else re.escape(data[i:i + 1]) for i in range(len(data)))
    hay = img_at(lo, hi - lo)
    al = max(sec.align, 1)
    return [lo + m.start() for m in re.finditer(pat, hay) if (lo + m.start()) % al == 0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--unit")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    os.chdir(paths.ROOT)
    import layout
    pats = list(a.files) + (layout.files_of_unit(a.unit) if a.unit else [])
    hybrid.TC_BIN = paths.ee_bin(None)
    rels, records, failed, orphans = score.run(pats, a.jobs)
    for rel, err in failed.items():
        print("FAIL %s\n%s" % (rel, err[-1500:]))
    fresh = score.LAST_LAYOUT
    fr = datalayout.file_ranges()
    decl = hybrid.declared()
    per_area = {}
    for rel in sorted(fresh):
        obj, bases = fresh[rel]
        rr = fr.get(rel, {})
        for sec in obj.sections:
            area = datalayout.AREA_OF_SEC.get(sec.name)
            if area in (None, ".gcc_except_table", ".bss") or sec.size == 0:
                continue
            rng = rr.get(area)
            base, how = datalayout.choose_base(obj, sec, rng, decl, bases.get(sec.index),
                                               rr.get(".frame") if sec.name == ".data" else None)
            if base is None or rng is None:
                lo, hi = AREA[area]
                hits = search(obj, sec, lo, hi)
                if len(hits) >= 1 and base is None:
                    base, how = hits[0], "search(%d)" % len(hits)
            data, mask = datalayout.masked_bytes(obj, sec)
            exact = None
            if base is not None:
                ret = img_at(base, sec.size)
                # (a base outside the image's bytes, e.g. a .data section whose
                # variables' declared addresses are in .bss: no image there)
                exact = len(ret) == len(data) and all(i in mask or data[i] == ret[i] for i in range(len(data)))
            per_area.setdefault(area, []).append((base, sec.size, rel.split("/")[-1][:24], sec.name, how, exact, rng))
    for area, lst in per_area.items():
        print("== %s" % area)
        rngs = [r for *_, r in lst if r]
        span_lo = min(r[0] for r in rngs) if rngs else None
        span_hi = max(r[1] for r in rngs) if rngs else None
        placed = sorted(x for x in lst if x[0] is not None)
        cur = span_lo
        for base, size, rel, sn, how, exact, rng in placed:
            if cur is not None and base > cur:
                print("   GAP   0x%08X..0x%08X (%d)" % (cur, base, base - cur))
            if cur is not None and base < cur:
                print("   OVERLAP 0x%08X..0x%08X (%d)" % (base, min(cur, base + size), min(cur, base + size) - base))
            print("   0x%08X..0x%08X %5d %-24s %-8s %-10s %s  orig %s" % (
                base, base + size, size, rel, sn, how, "EXACT" if exact else "differs",
                "0x%08X..0x%08X" % rng if rng else "?"))
            cur = max(cur or 0, base + size)
        if span_hi is not None and cur is not None and cur < span_hi:
            print("   GAP   0x%08X..0x%08X (%d)  (to the last range's end)" % (cur, span_hi, span_hi - cur))
        for x in lst:
            if x[0] is None:
                print("   UNPLACED %s %s %d bytes" % (x[2], x[3], x[1]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
