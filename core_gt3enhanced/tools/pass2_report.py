"""Second-pass summary: per-unit call-check counts before (ref/pass2/callcheck_before.txt)
and now, units below their baseline, and the phase-2 totals.

    python tools/callcheck.py --all > build/callcheck_after.txt
    python tools/pass2_report.py build/callcheck_after.txt
"""
import os
import re
import sys

import paths


def load(p):
    out = {}
    for L in open(p, encoding="utf-8", errors="replace"):
        m = re.match(r"^(u\d{3}): (\d+)/(\d+)", L)
        if m:
            out[m.group(1)] = (int(m.group(2)), int(m.group(3)))
    return out


def main():
    before = load(os.path.join(paths.REF, "pass2", "callcheck_before.txt"))
    after = load(sys.argv[1])
    worse = []
    fb = tb = fa = ta = 0
    for u in sorted(before):
        b = before[u]
        a = after.get(u)
        fb += b[0]
        tb += b[1]
        if a is None:
            worse.append((u, b, None))
            continue
        fa += a[0]
        ta += a[1]
        if a[0] < b[0]:
            worse.append((u, b, a))
    print("calls found: before %d/%d (%.1f%%), now %d/%d (%.1f%%)" % (
        fb, tb, 100.0 * fb / tb, fa, ta, 100.0 * fa / max(ta, 1)))
    print("units below their baseline: %d" % len(worse))
    for u, b, a in worse:
        print("  %s before %d/%d now %s" % (u, b[0], b[1], "%d/%d" % a if a else "missing"))
    notes = [u for u in before if os.path.exists(os.path.join(paths.REF, "pass2", "units", u, "notes.md"))]
    print("units with phase-2 notes: %d/%d" % (len(notes), len(before)))


if __name__ == "__main__":
    main()
