"""Extract the GS driver's (unit "gs") binary blobs from the bootstrap ELF.

Writes into data/:
  gs_vu0_microcode.bin            0x398  bytes @0x01040C00 (.rdata)  -> g_gsVu0Microcode
  gs_vu1_microcode_scrambled.bin  0x3DA8 bytes @0x01034730 (.data)   -> g_gsVu1Microcode
                                  (exactly as stored in the ELF; gfx_init descrambles
                                   it in place with xor_unscramble_with_lcg)
  gs_vu1_microcode.bin            the same after the LCG XOR (reference only - lets the
                                  VU1 program be disassembled; NOT what the ELF holds)

The descramble reproduces lcg_rand / xor_unscramble_with_lcg (0x0100E4A8 /
0x0100E4E8): state = state * 0x5D588B65 + 1 (mod 2^32), seed 0x32277070
(.sdata 0x01041E34), byte ^= low byte of each new state.  The script checks
that the result ends with the VU "NOP[E] / NOP" pair, i.e. is valid microcode.

Usage: python tools/gs_extract.py      (from decomp/bootstrap)
"""
import os
import struct
import sys

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
ELF = paths.retail_elf()
OUT = os.path.join(ROOT, "data")


def off(va):
    return va - 0x01000000 + 0x1000


def main():
    data = open(ELF, "rb").read()
    os.makedirs(OUT, exist_ok=True)

    vu0 = data[off(0x01040C00):off(0x01040C00) + 0x398]
    vu1s = data[off(0x01034730):off(0x01034730) + 0x3DA8]

    state = 0x32277070
    vu1 = bytearray(vu1s)
    for i in range(len(vu1)):
        state = (state * 0x5D588B65 + 1) & 0xFFFFFFFF
        vu1[i] ^= state & 0xFF

    # last two VU instruction pairs: {lower NOP 0x8000033C? / upper NOP[E] 0x400002FF}, {.., 0x2FF}
    tail = struct.unpack_from("<4I", vu1, len(vu1) - 16)
    if tail[1] != 0x400002FF or tail[3] != 0x000002FF:
        sys.exit("descrambled VU1 code does not end with NOP[E]/NOP: %r" % (tail,))
    tail0 = struct.unpack_from("<4I", vu0, len(vu0) - 16)
    if tail0[1] != 0x400002FF or tail0[3] != 0x000002FF:
        sys.exit("VU0 code does not end with NOP[E]/NOP: %r" % (tail0,))

    for name, blob in (("gs_vu0_microcode.bin", vu0),
                       ("gs_vu1_microcode_scrambled.bin", vu1s),
                       ("gs_vu1_microcode.bin", bytes(vu1))):
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(blob)
        print("wrote data/%s (%d bytes)" % (name, len(blob)))


if __name__ == "__main__":
    main()
