"""Size probe: every EGCS_SIZE(T, n) of include/game/**/*.h, compiled together
with egcs (tools/mcc.py -S), sizeof(T) read back from the assembly.  Run it
after growing a struct: it lists every documented size that no longer holds.

    python tools/sizeprobe.py [--only-diff]

Writes build/sizeprobe/probe.cpp, compiles it to probe.s, prints
'T  documented  actual  header' for each entry (only the differing ones with
--only-diff).  Entries that do not compile are dropped (and listed)."""
import os
import re
import subprocess
import sys

import paths

root = paths.ROOT
only_diff = "--only-diff" in sys.argv
os.chdir(root)

ENTRY = re.compile(r"^\s*EGCS_SIZE\(\s*([A-Za-z_][A-Za-z0-9_:<>, ]*?)\s*,\s*([^;]+?)\)\s*;", re.M)

headers = []
for d, _, fs in os.walk(os.path.join("include", "game")):
    for f in sorted(fs):
        if f.endswith(".h"):
            headers.append(os.path.join(d, f).replace("\\", "/"))
headers.sort()

entries = []
for h in headers:
    txt = open(h, encoding="utf-8", errors="replace").read()
    for m in ENTRY.finditer(txt):
        t, n = m.group(1).strip(), m.group(2).strip()
        entries.append((t, n, h))

out_dir = os.path.join("build", "sizeprobe")
os.makedirs(out_dir, exist_ok=True)
cpp = os.path.join(out_dir, "probe.cpp").replace("\\", "/")


def write(ents):
    lines = ['#include "types.h"', '#include "game/game.h"', "#undef EGCS_SIZE",
             "#define EGCS_SIZE(T, n) typedef char BOOT_CAT(probe_unchecked_, __LINE__)"]
    for h in headers:
        lines.append('#include "%s"' % h[len("include/"):])
    lines.append('extern "C" {')
    for i, (t, n, h) in enumerate(ents):
        lines.append("int probe_%d = sizeof(%s);" % (i, t))
        lines.append("int probe_doc_%d = (int)(%s);" % (i, n))
    lines.append("}")
    open(cpp, "w").write("\n".join(lines) + "\n")
    return len(lines) - len(ents) * 2 - 1  # line number of the first probe_ line (1-based) - 1


dropped = []
for attempt in range(12):
    first = write(entries)
    r = subprocess.run([sys.executable, os.path.join("tools", "mcc.py"), cpp, "-S", "-o", os.path.join(out_dir, "probe.s")],
                       capture_output=True, text=True)
    msg = r.stdout + r.stderr
    if r.returncode == 0 and os.path.isfile(os.path.join(out_dir, "probe.s")):
        break
    bad = set()
    for m in re.finditer(r"probe\.cpp:(\d+):", msg):
        ln = int(m.group(1))
        k = ln - first - 1
        if k >= 0:
            bad.add(k // 2)
    if not bad:
        print(msg[-6000:])
        sys.exit("probe does not compile (header errors)")
    for k in sorted(bad, reverse=True):
        if k < len(entries):
            dropped.append(entries.pop(k))
else:
    sys.exit("gave up")

s = open(os.path.join(out_dir, "probe.s")).read()
vals = {}
for m in re.finditer(r"^(probe_(?:doc_)?\d+):\s*\n\s*\.word\s+(-?\w+)", s, re.M):
    vals[m.group(1)] = int(m.group(2), 0)
# zero values go to .bss / .sbss (no .word): treat as 0
ndiff = 0
for i, (t, n, h) in enumerate(entries):
    a = vals.get("probe_%d" % i, 0)
    d = vals.get("probe_doc_%d" % i, 0)
    if a != d:
        ndiff += 1
    if a != d or not only_diff:
        print("%-44s doc 0x%-6X now 0x%-6X %s%s" % (t, d, a, h, "   <-- DIFF" if a != d else ""))
print("entries %d, differing %d, dropped %d" % (len(entries), ndiff, len(dropped)))
for t, n, h in dropped:
    print("  dropped:", t, n, h)
