"""core.gt3 images: build one from an ELF, sign it, check it, put it on a card.

    python tools/gt3image.py info FILE                 describe a core.gt3, check its signature
    python tools/gt3image.py pack ELF OUT [--seed S]   ELF -> core.gt3 signed with a fresh key
    python tools/gt3image.py card CORE.GT3 DIR         a PCSX2 folder memory card holding it
    python tools/gt3image.py elf CORE.GT3 OUT          core.gt3 -> its program as an ELF
                                                       (the retail one gives baserom/BASE_SCUS-97102)

The container, as the US bootstrap reads it (decomp/bootstrap/src/imageloader.cpp,
imageloader_source.cpp):

    u16  loadFlags      LE; the bootstrap boots the source whose copy has the
                        highest value (the disc's is 0x0101); not signed
    u32  rawSize        LE, size of the inflated body
    ...  raw DEFLATE    no zlib header (wbits -15)

    body:   u16 modLen, modulus (big-endian), u16 sigLen, signature (big-endian),
            payload: u32 nSection, u32 entry, nSection x (u32 target, u32 size, data)

ImageLoader::verify (0x010044A8) accepts the image when pow(signature, 66001,
modulus) equals SHA-512(payload) as integers. The modulus is the one in the
file, so an image signed with any key of public exponent 66001 is accepted;
`pack` makes one (reproducibly: the key comes from --seed).

On a memory card the bootstrap looks for /BASCUS-97102GAMEDATA/core.gt3 on a
formatted PS2 card in either slot (unless START is held at boot); on equal
loadFlags slot 2 beats slot 1 beats the disc. `card` writes a PCSX2 folder
card (a directory: _pcsx2_superblock of a formatted 8 MB card, and the file).
"""
import argparse
import hashlib
import math
import os
import random
import struct
import sys
import zlib

EXPONENT = 66001                    # GT3 A-Spec US (the immediate in ImageLoader::verify)
DISC_FLAGS = 0x0101                 # loadFlags of the retail disc copy
CARD_DIR = "BASCUS-97102GAMEDATA"
CARD_FILE = "core.gt3"


class Image:
    def __init__(self, entry, sections, flags=DISC_FLAGS, modulus=b"", signature=b""):
        self.entry = entry
        self.sections = sections            # [(target, bytes)]
        self.flags = flags
        self.modulus = modulus
        self.signature = signature

    @classmethod
    def read(cls, raw):
        flags, raw_size = struct.unpack_from("<HI", raw, 0)
        body = zlib.decompress(raw[6:], -15)
        if len(body) != raw_size:
            raise ValueError("inflated size 0x%X, header says 0x%X" % (len(body), raw_size))
        n, = struct.unpack_from("<H", body, 0)
        modulus = body[2:2 + n]
        p = 2 + n
        n, = struct.unpack_from("<H", body, p)
        signature = body[p + 2:p + 2 + n]
        p += 2 + n
        count, entry = struct.unpack_from("<II", body, p)
        p += 8
        sections = []
        for _ in range(count):
            target, size = struct.unpack_from("<II", body, p)
            sections.append((target, body[p + 8:p + 8 + size]))
            p += 8 + size
        if p != len(body):
            raise ValueError("body has 0x%X bytes after the last section" % (len(body) - p))
        return cls(entry, sections, flags, modulus, signature)

    @classmethod
    def from_elf(cls, data):
        """The PT_LOAD segments of an ELF (their file bytes; .bss is cleared by crt0)."""
        if data[:4] != b"\x7fELF":
            raise ValueError("not an ELF")
        entry, phoff = struct.unpack_from("<II", data, 24)
        phentsize, phnum = struct.unpack_from("<HH", data, 42)
        sections = []
        for i in range(phnum):
            ptype, off, vaddr, _, filesz = struct.unpack_from("<IIIII", data, phoff + i * phentsize)
            if ptype == 1 and filesz:
                sections.append((vaddr, data[off:off + filesz]))
        return cls(entry, sections)

    def payload(self):
        out = [struct.pack("<II", len(self.sections), self.entry)]
        for target, data in self.sections:
            out += [struct.pack("<II", target, len(data)), data]
        return b"".join(out)

    def digest(self):
        return hashlib.sha512(self.payload()).digest()

    def verifies(self, e=EXPONENT):
        """The bootstrap's check: sig^e mod n == SHA-512(payload), full equality."""
        if not self.modulus or not self.signature:
            return False
        n = int.from_bytes(self.modulus, "big")
        s = int.from_bytes(self.signature, "big")
        return n > 1 and pow(s, e, n) == int.from_bytes(self.digest(), "big")

    def sign(self, seed="gt3-decomp", e=EXPONENT, bits=1024):
        n, d = make_key(e, bits, random.Random(seed))
        sig = pow(int.from_bytes(self.digest(), "big"), d, n)
        self.modulus = n.to_bytes(bits // 8, "big")
        self.signature = sig.to_bytes(bits // 8, "big")

    def to_elf(self):
        """The image as an ELF: one PT_LOAD per section (file bytes at 0x1000,
        page-aligned), a .text section header over the first and .shstrtab.
        For the retail US image this is baserom/BASE_SCUS-97102 byte for byte
        (the layout of GT3Hooks' gt3pack.py, which made that file)."""
        names = b"\0.text\0.shstrtab\0"
        out = bytearray(0x1000)
        out[0:7] = b"\x7fELF\x01\x01\x01"
        struct.pack_into("<HHII", out, 16, 2, 8, 1, self.entry)        # ET_EXEC, EM_MIPS
        struct.pack_into("<I", out, 28, 52)                              # program headers
        struct.pack_into("<I", out, 36, 546455553)                       # e_flags of the retail bases
        struct.pack_into("<HHHHHH", out, 40, 52, 32, len(self.sections), 40, 3, 2)
        offs = []
        for i, (target, data) in enumerate(self.sections):
            offs.append(len(out))
            out += data
            out += b"\0" * (-len(out) % 0x1000)
            struct.pack_into("<8I", out, 52 + i * 32, 1, offs[i], target, target,
                             len(data), len(data), 7, 0x1000)            # PT_LOAD, RWX
        str_off = len(out)
        out += names + b"\0" * (-len(names) % 4)
        sh_off = len(out)
        target, data = self.sections[0]
        out += b"\0" * 40                                                # [0] NULL
        out += struct.pack("<10I", 1, 1, 6, target, offs[0], len(data), 0, 0, 0x40, 0)   # .text
        out += struct.pack("<10I", 7, 3, 0, 0, str_off, len(names), 0, 0, 1, 0)          # .shstrtab
        struct.pack_into("<I", out, 32, sh_off)
        return bytes(out)

    def pack(self, level=9):
        body = (struct.pack("<H", len(self.modulus)) + self.modulus
                + struct.pack("<H", len(self.signature)) + self.signature + self.payload())
        z = zlib.compressobj(level, zlib.DEFLATED, -15)
        return struct.pack("<HI", self.flags, len(body)) + z.compress(body) + z.flush()


def make_key(e, bits, rng):
    """(n, d) of an RSA key with public exponent e."""
    while True:
        p, q = prime(bits // 2, e, rng), prime(bits // 2, e, rng)
        n = p * q
        if p != q and n.bit_length() == bits:
            return n, pow(e, -1, (p - 1) * (q - 1))


def prime(bits, e, rng):
    while True:
        c = rng.getrandbits(bits) | (3 << (bits - 2)) | 1
        if math.gcd(e, c - 1) == 1 and probable_prime(c, rng):     # 66001 = 13 * 5077
            return c


def probable_prime(n, rng, rounds=32):
    for s in (3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % s == 0:
            return n == s
    d, r = n - 1, 0
    while d % 2 == 0:
        d, r = d // 2, r + 1
    for _ in range(rounds):
        x = pow(rng.randrange(2, n - 1), d, n)
        if x in (1, n - 1):
            continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


# ---------------------------------------------------------------- PCSX2 folder card

def superblock_8mb():
    """Block 0 of a formatted 8 MB PS2 memory card, as PCSX2 keeps it in a folder
    card's _pcsx2_superblock (PCSX2 treats the card as formatted when byte 0x16 is 'o')."""
    sb = bytearray(0x2000)
    magic = b"Sony PS2 Memory Card Format 1.2.0.0"
    sb[0:len(magic)] = magic
    struct.pack_into("<HHHH", sb, 0x28, 0x200, 2, 16, 0xFF00)      # page, pages/cluster, pages/block
    struct.pack_into("<IIIIII", sb, 0x30, 0x2000, 0x29, 0x1FC7, 0, 0x3FF, 0x3FE)
    struct.pack_into("<I", sb, 0x50, 8)                            # ifc_list[0]
    sb[0xD0:0x150] = b"\xFF" * 0x80                                # bad_block_list: none
    sb[0x150], sb[0x151] = 2, 0x2B                                 # PS2 card, flags
    return bytes(sb)


def write_card(core_gt3, card):
    """A PCSX2 folder memory card at `card` holding /BASCUS-97102GAMEDATA/core.gt3."""
    if os.path.exists(card) and not os.path.isfile(os.path.join(card, "_pcsx2_superblock")):
        raise SystemExit("%s exists and is not a folder memory card" % card)
    os.makedirs(os.path.join(card, CARD_DIR), exist_ok=True)
    with open(os.path.join(card, "_pcsx2_superblock"), "wb") as f:
        f.write(superblock_8mb())
    with open(os.path.join(card, CARD_DIR, CARD_FILE), "wb") as f:
        f.write(core_gt3)


# ---------------------------------------------------------------- commands

def describe(raw):
    img = Image.read(raw)
    lines = ["loadFlags 0x%04X, %d bytes packed, 0x%X inflated" % (img.flags, len(raw), struct.unpack_from("<I", raw, 2)[0]),
             "modulus %d bits, signature %d bytes: %s" % (
                 int.from_bytes(img.modulus, "big").bit_length(), len(img.signature),
                 "VERIFIES (e=%d)" % EXPONENT if img.verifies() else "does NOT verify"),
             "entry 0x%08X, payload SHA-512 %s..." % (img.entry, img.digest().hex()[:32])]
    lines += ["section %d: 0x%08X, 0x%X bytes" % (i, t, len(d)) for i, (t, d) in enumerate(img.sections)]
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("info"); p.add_argument("file")
    p = sub.add_parser("pack"); p.add_argument("elf"); p.add_argument("out")
    p.add_argument("--seed", default="gt3-decomp", help="key seed (same seed, same key)")
    p.add_argument("--flags", type=lambda s: int(s, 0), default=DISC_FLAGS)
    p = sub.add_parser("card"); p.add_argument("core"); p.add_argument("dir")
    p = sub.add_parser("elf"); p.add_argument("core"); p.add_argument("out")
    a = ap.parse_args()
    if a.cmd == "info":
        print("\n".join(describe(open(a.file, "rb").read())))
    elif a.cmd == "pack":
        img = Image.from_elf(open(a.elf, "rb").read())
        img.flags = a.flags
        img.sign(a.seed)
        raw = img.pack()
        open(a.out, "wb").write(raw)
        print("\n".join(["wrote " + a.out] + describe(raw)))
    elif a.cmd == "elf":
        open(a.out, "wb").write(Image.read(open(a.core, "rb").read()).to_elf())
        print("wrote " + a.out)
    elif a.cmd == "card":
        write_card(open(a.core, "rb").read(), a.dir)
        print("wrote %s/%s/%s" % (a.dir, CARD_DIR, CARD_FILE))
    return 0


if __name__ == "__main__":
    sys.exit(main())
