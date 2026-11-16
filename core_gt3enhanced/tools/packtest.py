"""Does the retail bootstrap boot a core.gt3 that we packed and signed?

    python tools/packtest.py            make the test cards in build/packtest/
    python tools/packtest.py --run      and boot each one in the private PCSX2 of
                                        bootstrap/tools/boot_test.py (retail bootstrap,
                                        retail disc, the card in slot 2)

Each card is a PCSX2 folder memory card holding /BASCUS-97102GAMEDATA/core.gt3,
packed by gt3image.py from baserom/BASE_SCUS-97102 with loadFlags 0x0101 (the
disc copy's; on a tie the slot 2 card wins):

  T1_resigned  the retail payload, signed with a fresh key        expect: boots
  T2_badsig    the T3 payload carrying T1's signature             expect: hangs before ExecPS2
  T3_edited    the format string "SummerTime=%d" changed to       expect: boots, the EE log
               "SUMMERTIME=%d" (printed every frame), signed               prints SUMMERTIME=

T2 and T3 tell which copy ran: the disc copy prints SummerTime=, the card copy
SUMMERTIME=, and a rejected image never reaches ExecPS2.
"""
import argparse
import os
import re
import subprocess
import sys

import gt3image
import paths

OUT = os.path.join(paths.ROOT, "build", "packtest")
BOOT_TEST = os.path.join(paths.REPO, "bootstrap", "tools", "boot_test.py")
BOOT_OUT = os.path.join(paths.REPO, "bootstrap", "build", "boottest")
OLD, NEW = b"SummerTime=%d\n", b"SUMMERTIME=%d\n"

CARDS = [
    ("T1_resigned", "boots; prints SummerTime="),
    ("T2_badsig", "hangs: no second ExecPS2 restart, no core.gt3 modules"),
    ("T3_edited", "boots; prints SUMMERTIME="),
]


def edited(img):
    (target, data), = img.sections
    if data.count(OLD) != 1:
        sys.exit("expected one %r in the payload, found %d" % (OLD, data.count(OLD)))
    return gt3image.Image(img.entry, [(target, data.replace(OLD, NEW))], img.flags)


def make():
    base = gt3image.Image.from_elf(open(paths.retail_elf(), "rb").read())
    t1 = gt3image.Image(base.entry, base.sections)
    t1.sign("packtest-T1")
    t3 = edited(base)
    t3.sign("packtest-T3")
    t2 = edited(base)
    t2.modulus, t2.signature = t1.modulus, t1.signature
    images = {"T1_resigned": t1, "T2_badsig": t2, "T3_edited": t3}
    os.makedirs(OUT, exist_ok=True)
    lines = []
    for name, _ in CARDS:
        raw = images[name].pack()
        gt3image.write_card(raw, os.path.join(OUT, name + ".ps2"))
        lines += ["%s.ps2/%s/%s" % (name, gt3image.CARD_DIR, gt3image.CARD_FILE)]
        lines += ["  " + L for L in gt3image.describe(raw)]
    return lines


def boot(name, seconds):
    tag = "packtest-" + name
    card = os.path.join(OUT, name + ".ps2")
    subprocess.run([sys.executable, BOOT_TEST, "--baseline", "--card", card, "--tag", tag,
                    "--seconds", str(seconds)], cwd=os.path.dirname(BOOT_TEST))
    out = os.path.join(BOOT_OUT, tag)
    log = open(os.path.join(out, "emulog.txt"), encoding="utf-8", errors="replace").read()
    summary = open(os.path.join(out, "summary.txt"), encoding="utf-8").read()
    marks = {}                                  # boot_test's "%-4s %-18s %5d" marker lines
    for L in summary.splitlines():
        if L[:4].strip() in ("ok", "FAIL") and L[23:29].strip().isdigit():
            marks[L[5:23].strip()] = int(L[23:29])
    return {
        "restarts": marks.get("4 execps2 restart", 0),
        "core irx": marks.get("5 core.gt3 irx", 0),
        "main loop": marks.get("6 gt3 main loop", 0),
        "SummerTime=": len(re.findall(r"SummerTime=", log)),
        "SUMMERTIME=": len(re.findall(r"SUMMERTIME=", log)),
        "dir": out,
    }


def verdict(name, r):
    booted = r["core irx"] > 0 and r["main loop"] > 0
    if name == "T1_resigned":
        return booted and r["SUMMERTIME="] == 0
    if name == "T2_badsig":
        return r["restarts"] < 2 and r["core irx"] == 0 and r["SUMMERTIME="] == 0 and r["SummerTime="] == 0
    return booted and r["SUMMERTIME="] > 0 and r["SummerTime="] == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--run", action="store_true", help="boot each card in the private PCSX2")
    ap.add_argument("--only", help="comma-separated card names (default: all)")
    ap.add_argument("--seconds", type=int, default=30)
    a = ap.parse_args()
    report = ["cards in %s" % OUT] + make()
    print("\n".join(report))
    if a.run:
        names = a.only.split(",") if a.only else [n for n, _ in CARDS]
        rows, ok = [], True
        for name, expect in CARDS:
            if name not in names:
                continue
            r = boot(name, a.seconds)
            good = verdict(name, r)
            ok &= good
            rows.append("%-12s %-4s restarts %d, core irx %d, main loop %5d, SummerTime= %5d, SUMMERTIME= %5d"
                        "   (expected: %s)" % (name, "PASS" if good else "FAIL", r["restarts"], r["core irx"],
                                               r["main loop"], r["SummerTime="], r["SUMMERTIME="], expect))
        report += ["", "boot results (logs in %s/packtest-*):" % BOOT_OUT] + rows
        report += ["", "RESULT: %s" % ("the bootstrap accepts our images and rejects a bad signature" if ok
                                       else "not as expected; see the logs")]
        print("\n".join(report[-len(rows) - 4:]))
    with open(os.path.join(OUT, "report.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(report) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
