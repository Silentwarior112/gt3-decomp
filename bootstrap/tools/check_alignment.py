"""Compare the alignment of every annotated global in the rebuilt ELF with its
retail address.

A decompiled global is annotated with its retail address (`// @0x0104xxxx` or
`/* @0x0104xxxx ...`) on its definition line. Buffers handed to SIF DMA / RPC
or VIF/GIF DMA must be 16- or 64-byte aligned; the retail link may provide
that by attribute or by luck. This lists every symbol whose retail address had
a higher power-of-two alignment (capped at 64) than its rebuilt address.

    python tools/check_alignment.py [build/scus_971.02.elf]
"""
import os, re, subprocess, sys

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
ELF = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "scus_971.02.elf")
NM = paths.ee_tool("nm")

def align_of(a, cap=64):
    n = 1
    while n < cap and a % (n * 2) == 0:
        n *= 2
    return n

# new addresses and sizes (data symbols only)
syms = {}
out = subprocess.run([NM, "-S", "--defined-only", ELF], capture_output=True, text=True).stdout
for L in out.splitlines():
    p = L.split()
    if len(p) == 4 and p[2] in "bBdDrRsSgG":
        syms.setdefault(p[3], (int(p[0], 16), int(p[1], 16)))

decl = re.compile(r"^\s*(?:static\s+|extern\s+\"C\"\s+|const\s+|volatile\s+|unsigned\s+|signed\s+|struct\s+|union\s+)*"
                  r"[\w:<>\s\*]+?\b(\w+)\s*(\[[^\]]*\])*\s*(?:__attribute__\s*\(\(.*?\)\))?\s*(?:=|;).*?@\s*(0x01[0-9A-Fa-f]{6})")
rows = []
for sub in ("src", "sdk", "runtime"):
    for dp, _, fns in os.walk(os.path.join(ROOT, sub)):
        for fn in fns:
            if not fn.endswith((".c", ".cpp", ".cc")):
                continue
            path = os.path.join(dp, fn)
            for ln, L in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
                m = decl.match(L)
                if not m or "(" in L.split("@")[0].split("=")[0].replace("__attribute__", ""):
                    continue
                name, ra = m.group(1), int(m.group(3), 16)
                if not (0x01026C00 <= ra < 0x0104DEB0):
                    continue
                cands = [s for s in syms if s == name or s.startswith(name + ".")]
                if not cands:
                    continue
                na, size = syms[cands[0]]
                want, have = align_of(ra), align_of(na)
                if want > have and size >= 16:
                    rows.append((want, os.path.relpath(path, ROOT).replace("\\", "/"), ln, name, ra, na, size))

rows.sort(key=lambda r: (-r[0], r[1], r[2]))
for want, rel, ln, name, ra, na, size in rows:
    print("%-40s retail 0x%08X (%2d-aligned)  now 0x%08X (%2d)  size 0x%X  %s:%d" % (
        name, ra, want, na, align_of(na), size, rel, ln))
print("%d symbol(s) less aligned than in the retail file" % len(rows))
