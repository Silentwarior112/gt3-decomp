"""Which retail functions are byte-identical to functions of a prebuilt
library (the toolchain's libgcc.a / libstdc++.a) or object file?

    python tools/match_lib.py [--json out.json] <lib.a | obj.o> [...]

Every function of every member is extracted with its relocations; bytes under
a relocation are masked, and the pattern is looked for at every retail
function start (ref/funcs.json) and, failing that, anywhere in .text.
Reports per retail unit how many functions the libraries account for.
"""
import argparse, collections, csv, json, os, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
from elfobj import ElfObject, RELOC_FIELD
import paths

BIN = paths.retail_elf()
AR = paths.ee_tool("ar")
TEXT_LO, TEXT_HI = 0x01000000, 0x01026B88

img = open(BIN, "rb").read()
text = img[0x1000:0x1000 + (TEXT_HI - TEXT_LO)]
funcs = {f["start"]: f for f in json.load(open(os.path.join(ROOT, "ref", "funcs.json")))}
units = json.load(open(os.path.join(ROOT, "ref", "units.json")))
names = {int(r["addr"], 16): r["name"] for r in csv.DictReader(open(os.path.join(ROOT, "ref", "symbols.csv"), encoding="utf-8"))}

def unit_of(a):
    for u, s in units.items():
        for lo, hi in s["ranges"]:
            if int(lo, 16) <= a < int(hi, 16):
                return u
    return "?"

def lib_functions(d):
    obj = ElfObject(d)
    out = []
    for y, s in obj.functions():
        words = [struct.unpack_from("<I", s.data, y.value + 4 * i)[0] for i in range(y.size // 4)]
        mask = [0xFFFFFFFF] * len(words)
        for r in obj.relocs.get(s.index, []):
            k = (r.offset - y.value) // 4
            if 0 <= k < len(words):
                mask[k] &= ~RELOC_FIELD.get(r.type, 0xFFFFFFFF) & 0xFFFFFFFF
        while len(words) > 1 and words[-1] == 0 and mask[-1] == 0xFFFFFFFF and not (words[-2] >> 26 in (2, 3) or words[-2] == 0x03E00008):
            words.pop(); mask.pop()                     # alignment padding
        out.append((y.name, words, mask))
    return out

def matches_at(off, words, mask):
    if off < 0 or off + 4 * len(words) > len(text):
        return False
    for i, (w, m) in enumerate(zip(words, mask)):
        t, = struct.unpack_from("<I", text, off + 4 * i)
        if (t & m) != (w & m):
            return False
    return True

def members(path):
    if not path.endswith(".a"):
        return [(os.path.basename(path), open(path, "rb").read())]
    tmp = tempfile.mkdtemp()
    subprocess.run([AR, "x", os.path.abspath(path)], cwd=tmp, check=True)
    return [(f, open(os.path.join(tmp, f), "rb").read()) for f in sorted(os.listdir(tmp))]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("libs", nargs="+")
    ap.add_argument("--json")
    a = ap.parse_args()
    found = {}                                         # retail addr -> (lib, member, symbol, insns)
    for lib in a.libs:
        for mname, d in members(lib):
            if d[:4] != b"\x7fELF":
                continue
            for sym, words, mask in lib_functions(d):
                if len(words) < 3:                     # too short to be evidence
                    continue
                for addr in funcs:
                    if addr not in found and matches_at(addr - TEXT_LO, words, mask):
                        # the whole retail function must be covered (padding aside)
                        f = funcs[addr]
                        if 4 * len(words) <= f["end"] - addr:
                            found[addr] = (os.path.basename(lib), mname, sym, len(words))
    per = collections.defaultdict(lambda: [0, 0])
    for addr in funcs:
        u = unit_of(addr)
        per[u][1] += 1
        if addr in found:
            per[u][0] += 1
    for addr in sorted(found):
        lib, mname, sym, n = found[addr]
        print("0x%08X %-40s <- %s(%s) %s  [%d insns]" % (addr, names.get(addr, "?")[:40], lib, mname, sym, n))
    print()
    for u in units:
        if per[u][0]:
            print("%-14s %3d of %3d functions come from the libraries" % (u, per[u][0], per[u][1]))
    print("total: %d retail functions identical to library functions" % len(found))
    if a.json:
        json.dump({"0x%08X" % k: v for k, v in sorted(found.items())}, open(a.json, "w"), indent=1)

if __name__ == "__main__":
    main()
