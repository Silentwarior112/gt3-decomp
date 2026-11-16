"""Progress of the matching pass (ref/MATCHING.md) and the next units to start.

    python tools/match_schedule.py                    units done / left
    python tools/match_schedule.py --next 2 u005 u006   the next 2, skipping running ones

A unit is done when ref/matching/<unit>/notes.md exists (pending.md alone is
not a result).  Units are taken in order.  A done unit whose pending.md is
newer than its notes.md has items other units left for it after it finished.
A unit stopped part way (a session break) has progress.md instead of
notes.md: it is listed first, and its agent continues from that file.
"""
import argparse
import json
import os

import paths


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--next", type=int, default=0)
    ap.add_argument("--pass", dest="pass_", default="matching", choices=("matching", "data"),
                    help="ref/<pass>/<unit>/ (data: ref/DATA.md)")
    ap.add_argument("running", nargs="*")
    a = ap.parse_args()
    units = list(json.load(open(os.path.join(paths.REF, "units.json"))))
    base = os.path.join(paths.REF, a.pass_)
    done = [u for u in units if os.path.exists(os.path.join(base, u, "notes.md"))]
    left = [u for u in units if u not in done and u not in a.running]
    partial = [u for u in left if os.path.exists(os.path.join(base, u, "progress.md"))]
    left = partial + [u for u in left if u not in partial]
    print("done %d of %d; running %d; left %d" % (len(done), len(units), len(a.running), len(left)))
    if partial:
        print("stopped part way (continue from progress.md): " + " ".join(partial))
    # a revisit writes pending.md's "handled" line and notes.md together:
    # within half a minute is not newer
    revisit = [u for u in done if os.path.exists(os.path.join(base, u, "pending.md")) and
               os.path.getmtime(os.path.join(base, u, "pending.md")) >
               os.path.getmtime(os.path.join(base, u, "notes.md")) + 30]
    if revisit:
        print("done units with newer pending items (revisit): " + " ".join(revisit))
    if a.next:
        print(" ".join(left[:a.next]))
    return 0


if __name__ == "__main__":
    main()
