"""Extract the SHA-512 round-constant table K[0..79] from the bootstrap ELF
(.rodata 0x0103DF18..0x0103E198) and check it against src/sha512.cpp.

Usage (from decomp/bootstrap):
    python tools/sha512_ktable.py          print the C initializer
    python tools/sha512_ktable.py --check  verify src/sha512.cpp's sha512_K[]
"""
import os, re, struct, sys

import paths

ELF = paths.retail_elf()
K_VA, K_COUNT = 0x0103DF18, 80
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def file_off(va):
    return va - 0x01000000 + 0x1000     # single PT_LOAD


def read_k():
    data = open(ELF, "rb").read()
    return list(struct.unpack_from("<%dQ" % K_COUNT, data, file_off(K_VA)))


def main():
    k = read_k()
    if "--check" in sys.argv:
        src = open(os.path.join(ROOT, "src", "sha512.cpp"), encoding="utf-8").read()
        body = src[src.index("const u64 sha512_K[80]"):]
        body = body[:body.index("};")]
        vals = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{16})ULL", body)]
        ok = vals == k
        print("sha512_K in src/sha512.cpp %s the ELF (%d values)" % ("MATCHES" if ok else "DIFFERS FROM", len(vals)))
        sys.exit(0 if ok else 1)
    print("const u64 sha512_K[80] = {  /* @0x%08X */" % K_VA)
    for i in range(0, K_COUNT, 2):
        print("    0x%016XULL, 0x%016XULL," % (k[i], k[i + 1]))
    print("};")


if __name__ == "__main__":
    main()
