"""Find which source files break the boot when built at -O2 (and work at -O0).

    python tools/bisect_opt.py [--marker N] [--seconds S] [file-or-glob ...]

Builds with the given files at -O0 and everything else at -O2, boot-tests up
to marker N (default 3 = the bootstrap's IRX loads, see boot_test.MARKERS),
and bisects the file set down to the smallest set that must stay at -O0.
Without arguments it starts from every source file.
"""
import argparse, fnmatch, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import build, boot_test

INVERT = False   # --invert: everything -O0, the tested set at -O2

def try_set(o0, marker, seconds, tag):
    if INVERT:
        args = [sys.executable, os.path.join(HERE, "build.py"), "--no-install", "--opt=-O0"] + \
               ["--opt-file=%s=-O2" % f for f in o0]
    else:
        args = [sys.executable, os.path.join(HERE, "build.py"), "--no-install"] + ["--opt-file=%s=-O0" % f for f in o0]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode:
        print(r.stdout[-2000:], r.stderr[-2000:]); raise SystemExit("build failed")
    elf = os.path.join(ROOT, "build", "scus_971.02.%s" % tag)
    import shutil; shutil.copyfile(os.path.join(ROOT, "build", "scus_971.02"), elf)
    boot_test.MARKERS[:] = [m for m in boot_test.MARKERS if m[0][0].isdigit() and int(m[0][0]) <= marker]
    ok = boot_test.run(elf, boot_test.ISO, seconds, tag)
    print("  -O0 set of %d file(s) -> %s" % (len(o0), "BOOTS to marker %d" % marker if ok else "hangs"))
    return ok

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--marker", type=int, default=3)
    ap.add_argument("--seconds", type=int, default=25)
    ap.add_argument("--invert", action="store_true",
                    help="base -O0; find the files that must be -O2 to boot")
    ap.add_argument("files", nargs="*")
    a = ap.parse_args()
    global INVERT
    INVERT = a.invert
    allf = build.sources()
    cand = [f for f in allf if not a.files or any(fnmatch.fnmatch(f, p) for p in a.files)]
    print("candidates: %d file(s)" % len(cand))
    if not try_set(cand, a.marker, a.seconds, "bisect-all"):
        print("even with every candidate at -O0 it does not boot: not (only) an -O2 problem in these files")
        return 1
    # classic bisection for a minimal culprit subset (assumes one culprit file,
    # then re-checks the remainder for more)
    culprits = []
    rest = list(cand)
    step = 0
    while rest:
        if try_set(culprits, a.marker, a.seconds, "bisect-%d" % step):
            break
        # invariant: culprits + rest[:hi] boots, culprits + rest[:lo] does not
        lo, hi = 0, len(rest)
        while hi - lo > 1:
            mid = (lo + hi) // 2
            step += 1
            if try_set(culprits + rest[:mid], a.marker, a.seconds, "bisect-%d" % step):
                hi = mid
            else:
                lo = mid
        culprits.append(rest[lo])
        print("culprit: %s" % rest[lo])
        rest = rest[:lo] + rest[lo + 1:]
    print("files that must be -O0 to boot: %s" % culprits)
    return 0

if __name__ == "__main__":
    sys.exit(main())
