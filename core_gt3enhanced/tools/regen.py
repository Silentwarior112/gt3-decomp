"""Regenerate ref/ from the game file, in order:

  ground.py      functions and translation units (funcs.json, tus.json)
  libmatch.py    code shared with the decompiled bootstrap (libmatch.json)
  names.py       function and data names (symbols.csv, data_symbols.csv)
  classes.py     the class index (classes.json, classes.txt)
  units.py       work units (units.json)          only with --units: the source tree
                                                  is organised by unit, so units.json
                                                  must stay as it is once work started
  listing.py     the whole annotated listing (full.txt)
  gen_splat.py + splat, then packets.py: ref/units/<unit>/ packets with m2c drafts,
  and agent_prompt.py --write: each packet's TASK.md

    python tools/regen.py [--units] [--jobs N]
"""
import argparse
import os
import shutil
import subprocess
import sys

import paths

ap = argparse.ArgumentParser()
ap.add_argument("--units", action="store_true", help="also recompute units.json (changes unit ids!)")
ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
a = ap.parse_args()


def run(*cmd, cwd=paths.TOOLS):
    print("==", " ".join(cmd), flush=True)
    r = subprocess.run([sys.executable] + list(cmd), cwd=cwd)
    if r.returncode:
        sys.exit("failed: %s" % " ".join(cmd))


run("ground.py")
run("libmatch.py")
run("names.py")
run("classes.py")
if a.units or not os.path.exists(os.path.join(paths.REF, "units.json")):
    run("units.py", "--target", "3500", "--max", "5000")
run("listing.py")
run("gen_splat.py")
sp = os.path.join(paths.REF, "splat")
shutil.rmtree(os.path.join(sp, "asm"), ignore_errors=True)
run("-m", "splat", "split", "core.yaml", cwd=sp)
run("packets.py", "--jobs", str(a.jobs))
run("agent_prompt.py", "--write")
print("ref/ regenerated")
