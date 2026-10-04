"""Pull missed-function-start addresses out of ref/map_notes.md boundary fixes
and validate each against the disassembly before accepting it.

Writes ref/extra_starts.txt (one 0x-address per line + reason), read by
boot_ground.py.
"""
import os, re, struct, json

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "..", "ref")
BIN = paths.retail_elf()
d = open(BIN, "rb").read()
def word(va):
    return struct.unpack_from("<I", d, 0x1000 + va - 0x01000000)[0]

TEXT_LO, TEXT_HI = 0x01000000, 0x01026B88
funcs = {f["start"] for f in json.load(open(os.path.join(REF, "funcs.json")))}

ADDR = re.compile(r"0x0?1[0-9A-Fa-f]{6}")
cands = {}
for line in open(os.path.join(REF, "map_notes.md"), encoding="utf-8"):
    if not line.startswith("- boundary fix"):
        continue
    m = re.match(r"- boundary fix (0x[0-9A-Fa-f]+): (.*)", line.strip())
    if not m:
        continue
    head, text = int(m.group(1), 16), m.group(2)
    low = text.lower()
    if "not a function start" in low or "not a real function" in low or low.startswith("not an error") or "stray" in low:
        continue
    if re.match(r"(probable )?missed (function )?start", low) and not re.match(r"(probable )?missed (function )?starts?:\s*0x", low):
        # the header address itself is the missed start
        cands.setdefault(head, text[:100])
    # addresses listed after "missed start(s):"
    k = low.find("missed start")
    if k >= 0:
        for a in ADDR.findall(text[k:]):
            v = int(a, 16)
            cands.setdefault(v, text[:100])
    k = low.find("missed function start at")
    if k >= 0:
        for a in ADDR.findall(text[k:k + 40]):
            cands.setdefault(int(a, 16), text[:100])

def plausible(a):
    if not (TEXT_LO <= a < TEXT_HI) or a & 3:
        return False, "out of text/unaligned"
    if a in funcs:
        return True, "already a start"
    # preceded by jr ra (+slot), j (+slot), or nop padding, or syscall stub end
    p1, p2 = word(a - 4), word(a - 8)
    uncond_b = lambda w: (w >> 16) == 0x1000
    if p2 == 0x03E00008 or (p2 >> 26) == 2 or uncond_b(p2):
        return True, "after jr/j/b + delay slot"
    if p1 == 0:
        k = a - 4
        while k > TEXT_LO and word(k) == 0:
            k -= 4
        if (word(k - 4) == 0x03E00008 or (word(k - 4) >> 26) == 2 or
                uncond_b(word(k - 4)) or word(k) == 0x03E00008):
            return True, "after return + nop pad"
    return False, "not preceded by return/jump (prev=%08X,%08X)" % (p2, p1)

acc, rej = [], []
for a in sorted(cands):
    ok, why = plausible(a)
    (acc if ok else rej).append((a, why, cands[a]))

with open(os.path.join(REF, "extra_starts.txt"), "w", encoding="utf-8") as fh:
    for a, why, txt in acc:
        fh.write("0x%08X  # %s | %s\n" % (a, why, txt))
print("accepted %d, rejected %d" % (len(acc), len(rej)))
for a, why, txt in rej:
    print("REJECT 0x%08X  %s | %s" % (a, why, txt[:90]))
