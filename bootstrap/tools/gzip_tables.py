"""Extract the GZIP unit's data tables from BOOTSTRAP_SCUS-97102 and print them
as C initialisers (pasted into src/gzip_inflate.cpp).

  0x01039C70  u32[256]  crc32 table (reflected poly 0xEDB88320)
  0x0103A070  u16[31]   cplens  - length base values          (+1 pad halfword)
  0x0103A0B0  u16[31]   cplext  - length extra bits (99 = invalid)
  0x0103A0F0  u16[32]   cpdist  - distance base values   (+2 zero padding halfwords)
  0x0103A130  u16[32]   cpdext  - distance extra bits    (+2 zero padding halfwords)
  0x0103A170  u8[19]    border  - code-length code order

File offset = va - 0x01000000 + 0x1000 (single PT_LOAD).

Usage: python tools/gzip_tables.py
"""
import os
import struct

import paths

ELF = paths.retail_elf()
data = open(ELF, "rb").read()


def rd(va, n):
    off = va - 0x01000000 + 0x1000
    return data[off:off + n]


def words(va, count, fmt="I"):
    size = struct.calcsize("<" + fmt)
    return list(struct.unpack("<%d%s" % (count, fmt), rd(va, count * size)))


def emit(name, ctype, va, vals, per_line, fmt):
    print("static const %s %s[%d] = { // @0x%08X (.data)" % (ctype, name, len(vals), va))
    for i in range(0, len(vals), per_line):
        print("    " + ", ".join(fmt % v for v in vals[i:i + per_line]) + ",")
    print("};")
    print()


crc = words(0x01039C70, 256)
# sanity: must be the standard reflected CRC-32 table
std = []
for n in range(256):
    c = n
    for _ in range(8):
        c = (0xEDB88320 ^ (c >> 1)) if c & 1 else c >> 1
    std.append(c)
assert crc == std, "crc table is not the standard CRC-32 table"

emit("crc_table", "u32", 0x01039C70, crc, 6, "0x%08XU")
emit("cplens", "u16", 0x0103A070, words(0x0103A070, 31, "H"), 16, "%d")
emit("cplext", "u16", 0x0103A0B0, words(0x0103A0B0, 31, "H"), 16, "%d")
# 32 entries: [30], [31] are the zero alignment padding, which huft_build reads
# for a dynamic block with nd = 31/32 (no "nd > 30" check).
emit("cpdist", "u16", 0x0103A0F0, words(0x0103A0F0, 32, "H"), 15, "%d")
emit("cpdext", "u16", 0x0103A130, words(0x0103A130, 32, "H"), 15, "%d")
emit("border", "u8", 0x0103A170, list(rd(0x0103A170, 19)), 19, "%d")
