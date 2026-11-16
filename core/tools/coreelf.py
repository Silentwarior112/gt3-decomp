"""Shared access to the main executable's image: memory map, words, strings,
R5900 decoding (rabbitizer) and the annotated listing format the ref/ files use.

Layout of BASE_SCUS-97102 (one PT_LOAD, file offset = va - 0x00100000 + 0x1000):

    0x00100000 .. 0x0028E760   .text  (crt0 first, entry 0x00100008)
    0x0028E780 .. 0x0028F01C   FDE-pointer table: one entry per C++ translation
                               unit, registered by the last .ctors entry
    0x0028F020 ..              .ctors / .dtors (count-headed, GCC 2.9 format)
    0x0028F2AC .. 0x0030729C   .eh_frame blocks (CIE + FDEs per unit)
    0x0030729C .. 0x003531E3   .data / .rodata / .sdata (no section headers survive)
    0x00353200 .. 0x004B2C0C   .sbss / .bss (crt0 clears; _end = 0x004B2C0C)
    $gp = 0x003579F0
"""
import re
import struct

import rabbitizer

import paths

BASE = 0x00100000
FOFF = 0x1000
IMG_END = 0x003531E3          # .text sh_size end (p_filesz stops 0x18 short)
TEXT_LO = 0x00100000
TEXT_HI = 0x0028E760
ENTRY = 0x00100008
GP = 0x003579F0
FDE_TABLE = 0x0028E780
CTORS = 0x0028F020
EH_FRAME_LO = 0x0028F2AC
EH_FRAME_HI = 0x0030729C
SBSS = 0x00353200
BSS_END = 0x004B2C0C          # _end

R5900 = rabbitizer.InstrCategory.R5900
rabbitizer.config.regNames_fprAbiNames = rabbitizer.Abi.NUMERIC  # $f0.. as in GNU as
rabbitizer.config.regNames_gprAbiNames = rabbitizer.Abi.N32      # EE ABI: $8-$11 = $a4-$a7 (args 5-8), $12-$15 = $t0-$t3
rabbitizer.config.pseudos_pseudoB = True


class Image:
    def __init__(self, path=None):
        self.path = path or paths.retail_elf()
        self.data = open(self.path, "rb").read()
        self.img = self.data[FOFF:FOFF + (IMG_END - BASE)]
        self._ins = {}
        # the frame blocks themselves; ordinary .data sits between them
        self.frames = []
        a = FDE_TABLE
        while True:
            v = self.word(a)
            if not v or not (BASE <= v < IMG_END):
                break
            p = v
            while True:
                ln = self.word(p)
                if not ln or ln > 0x10000:
                    break
                p += 4 + ln
            self.frames.append((v, p + 4))
            a += 4
        self.frames.sort()
        self.sections = [(".text", TEXT_LO, TEXT_HI),
                         (".eh_frame_ptrs", FDE_TABLE, 0x0028F01C),
                         (".ctors", CTORS, EH_FRAME_LO)]
        self.sections += [(".eh_frame", lo, hi) for lo, hi in self.frames]
        self.sections += [(".data", EH_FRAME_LO, IMG_END), (".sbss/.bss", SBSS, BSS_END)]

    def in_frame_block(self, va):
        for lo, hi in self.frames:
            if lo <= va < hi:
                return True
        return False

    # ---- memory ----
    def word(self, va):
        o = va - BASE
        if 0 <= o <= len(self.img) - 4:
            return struct.unpack_from("<I", self.img, o)[0]
        return None

    def half(self, va):
        o = va - BASE
        if 0 <= o <= len(self.img) - 2:
            return struct.unpack_from("<H", self.img, o)[0]
        return None

    def byte(self, va):
        o = va - BASE
        if 0 <= o < len(self.img):
            return self.img[o]
        return None

    def bytes(self, va, n):
        o = va - BASE
        return self.img[o:o + n]

    def in_text(self, va):
        return TEXT_LO <= va < TEXT_HI and (va & 3) == 0

    def in_image(self, va):
        return BASE <= va < IMG_END

    def in_ram(self, va):
        return BASE <= va < BSS_END

    def section_of(self, va):
        for n, lo, hi in self.sections:
            if lo <= va < hi:
                return n
        return None

    def cstr(self, va, minlen=3, maxlen=200):
        o = va - BASE
        if not (0 <= o < len(self.img)):
            return None
        e = self.img.find(b"\0", o, min(o + maxlen, len(self.img)))
        if e < 0:
            return None
        s = self.img[o:e]
        if len(s) >= minlen and all(0x20 <= c < 0x7F or c in (9, 10, 13) for c in s):
            return s.decode("latin-1")
        return None

    # ---- decoding ----
    def ins(self, va):
        i = self._ins.get(va)
        if i is None:
            i = rabbitizer.Instruction(self.word(va), va, R5900)
            self._ins[va] = i
        return i


_BR = re.compile(r"\. \+ 4 \+ \(-?0x[0-9A-Fa-f]+ << 2\)|\. \+ 4 \+ \(-?\d+ << 2\)")
_FN = re.compile(r"\bfunc_([0-9A-Fa-f]+)\b")


def fmt(img, va, name_of=None):
    """(mnemonic, operands) of the instruction at va with branch and jump targets
    written as addresses (or names through name_of(addr) -> str|None)."""
    w = img.word(va)
    # R5900 sqrt.s takes its source in ft (fs = 0); rabbitizer prints it as "c1"
    if (w >> 26) == 0x11 and ((w >> 21) & 31) == 0x10 and (w & 0x3F) == 0x04:
        return "sqrt.s", "$f%d, $f%d" % ((w >> 6) & 31, (w >> 16) & 31)
    i = img.ins(va)
    if not i.isValid():
        return ".word", "0x%08X" % w
    t = i.disassemble()
    parts = t.split(None, 1)
    m = parts[0]
    o = parts[1].strip() if len(parts) > 1 else ""
    if i.isBranch():
        tgt = i.getBranchVramGeneric()
        o = _BR.sub("0x%08X" % tgt, o)
    elif i.isJumpWithAddress():
        tgt = i.getInstrIndexAsVram()
        nm = name_of(tgt) if name_of else None
        o = _FN.sub(nm or ("F_%08X" % tgt), o)
    return m, o


def simm16(w):
    v = w & 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


LOADSTORE_OPS = {0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2B, 0x31, 0x39, 0x37, 0x3F,
                 0x1E, 0x1F, 0x22, 0x26, 0x2A, 0x2E, 0x1B, 0x1A, 0x2C, 0x2D, 0x36, 0x3E}
