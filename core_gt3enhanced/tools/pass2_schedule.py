"""Phase-2 launch order: the units in subsystem order (base libraries first, as in
phase 1), never two at once that own the same canonical header (units that share
a translation unit).  A unit is done when ref/pass2/units/<unit>/notes.md exists.

    python tools/pass2_schedule.py                      the whole order, done ones marked
    python tools/pass2_schedule.py --next N u120 u121   the next N units to start while
                                                        u120 and u121 are running
"""
import json
import os
import sys

import paths

P2 = os.path.join(paths.REF, "pass2")
units = json.load(open(os.path.join(paths.REF, "units.json")))
headers = json.load(open(os.path.join(P2, "headers.json")))

# unit-number ranges in launch order (tools/pass2_plan.py ORDER)
ORDER = [(125, 142), (113, 118), (121, 124), (110, 112), (119, 120), (95, 109), (63, 75), (76, 87), (88, 94),
         (42, 62), (0, 41)]


def order():
    out = []
    for lo, hi in ORDER:
        for n in range(lo, hi + 1):
            u = "u%03d" % n
            if u in units and u not in out:
                out.append(u)
    out += [u for u in units if u not in out]
    return out


def owned(u):
    s = units[u]
    return {h for h, e in headers.items() if e["tu"] in s["tus"] or u in e["units"]}


def done(u):
    return os.path.exists(os.path.join(P2, "units", u, "notes.md"))


def main():
    a = sys.argv[1:]
    seq = order()
    if a[:1] == ["--next"]:
        n = int(a[1])
        running = a[2:]
        busy = set().union(*[owned(r) for r in running]) if running else set()
        picked = []
        for u in seq:
            if len(picked) >= n:
                break
            if done(u) or u in running or u in picked or owned(u) & busy:
                continue
            picked.append(u)
            busy |= owned(u)
        print(" ".join(picked))
        return
    nd = sum(done(u) for u in seq)
    print("%d/%d units done" % (nd, len(seq)))
    print(" ".join(("[%s]" % u) if done(u) else u for u in seq))


if __name__ == "__main__":
    main()
