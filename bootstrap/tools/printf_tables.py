"""Extract the .rodata tables used by pdistd::PrintFormat (printf unit) from the
GT3 US bootstrap ELF and print them as C initializers.

    python tools/printf_tables.py            # prints the tables to stdout

Tables (va, file offset = va - 0x01000000 + 0x1000):
  0x0103E6A8  633 x u64  raw IEEE-754 double bits, ~10^(i-324), i = 0..632
                         (NOT correctly rounded: most entries differ from the
                         nearest double in the low mantissa bits, so the raw
                         bits must be kept - PrintFormat compares bit patterns)
  0x0103FA80   19 x u64  exact integer powers 10^0 .. 10^18
  0x0103FB20   32 bytes  "0123456789abcdef" "0123456789ABCDEF" (no NULs)
  0x0103FB40   17 words  flag jump table   (chars 0x20..0x30)
  0x0103FB90   52 words  conversion jump table (chars 'E'..'x')
"""
import os
import struct

import paths

BIN = paths.retail_elf()


def rd(data, va, n):
    off = va - 0x01000000 + 0x1000
    return data[off:off + n]


def main():
    data = open(BIN, "rb").read()

    print("/* @0x0103E6A8 .rodata: raw bits of 633 doubles ~= 10^(i-324) (i = 0..632) */")
    print("static const u64 s_pow10_double[633] = {")
    for i in range(0, 633, 3):
        row = []
        for j in range(i, min(i + 3, 633)):
            b = struct.unpack("<Q", rd(data, 0x0103E6A8 + 8 * j, 8))[0]
            row.append("0x%016XULL" % b)
        print("    %s, /* 1e%d.. */" % (", ".join(row), i - 324))
    print("};")
    print()
    print("/* @0x0103FA80 .rodata: 10^0 .. 10^18 */")
    print("static const u64 s_pow10_u64[19] = {")
    for i in range(19):
        v = struct.unpack("<Q", rd(data, 0x0103FA80 + 8 * i, 8))[0]
        print("    %dULL," % v)
    print("};")
    print()
    print("/* @0x0103FB20 .rodata */")
    print('static const char s_digits[] = "%s";' % rd(data, 0x0103FB20, 32).decode("ascii"))
    print()
    print("/* flag jump table @0x0103FB40 */")
    for i in range(17):
        print("/*   %r -> 0x%08X */" % (chr(0x20 + i), struct.unpack("<I", rd(data, 0x0103FB40 + 4 * i, 4))[0]))
    print("/* conversion jump table @0x0103FB90 */")
    for i in range(52):
        print("/*   %r -> 0x%08X */" % (chr(0x45 + i), struct.unpack("<I", rd(data, 0x0103FB90 + 4 * i, 4))[0]))


if __name__ == "__main__":
    main()
