"""Extract the startup/warning-screen texture (unit "startup") from the bootstrap ELF.

The bootstrap embeds a gzip file (FNAME 'startupa.img') in .data at 0x01027F38.
startup_screen_start (0x01000A50) inflates it with GZIP::InflateBase into a
0x40000-byte buffer, then tex1_relocate / tex1_set_vram_base(0xA0000) /
tex1_upload_to_vram (src/tex1.cpp) turn it into a live 'Tex1' texture set.

Writes into data/:
  startupa_img.gz     the gzip member exactly as stored in the ELF
                      (0x01027F38..0x01033C92, 0xBD5B bytes) -> g_startupaImgGz
  startupa_tex1.bin   the inflated 'Tex1' image (0x38020 bytes; reference only,
                      the ELF does not contain it)

Boundary note: the gzip member ends at 0x01033C92 (ISIZE at 0x01033C8F).
0x01033C93 is one zero pad byte and 0x01033C94 starts a GCC .eh_frame CIE of
the next object ({len 0x10, id 0, ver 1, "", 1, -8, ra 64, def_cfa sp}), so the
blob is NOT 0x01027F38..0x01033C98 as the mapping pass said.

The script checks: gzip magic/method/FNAME, that zlib inflates the member
completely with nothing but the pad byte + CIE after it, ISIZE == 0x38020, the
'Tex1' magic, and it prints the Tex1 header, the GsTexRegs entries and the
transfer records (see include/startup_screen.h for the layout).

Usage: python tools/extract_startup_img.py      (from decomp/bootstrap)
"""
import os
import struct
import sys
import zlib

import paths

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
ELF = paths.retail_elf()
OUT = os.path.join(ROOT, "data")

BLOB_VA = 0x01027F38
SCAN_END_VA = 0x01033CA0          # scan a little past the end to find it exactly
TEX1_SIZE = 0x38020


def off(va):
    return va - 0x01000000 + 0x1000


def main():
    data = open(ELF, "rb").read()
    raw = data[off(BLOB_VA):off(SCAN_END_VA)]

    # gzip header: 1F 8B, CM 8, FLG 0x08 (FNAME only), MTIME, XFL, OS, name\0
    if raw[0:3] != b"\x1f\x8b\x08":
        sys.exit("no gzip header at 0x%08X" % BLOB_VA)
    flg = raw[3]
    if flg != 0x08:
        sys.exit("unexpected gzip FLG 0x%02X (expected FNAME only)" % flg)
    name_end = raw.index(b"\0", 10)
    fname = raw[10:name_end].decode("latin-1")
    mtime = struct.unpack_from("<I", raw, 4)[0]

    d = zlib.decompressobj(16 + 15)
    tex = d.decompress(raw)
    if not d.eof:
        sys.exit("gzip member does not end before 0x%08X" % SCAN_END_VA)
    blob_len = len(raw) - len(d.unused_data)
    blob = raw[:blob_len]
    tail = d.unused_data
    if tail[:1] != b"\0" or struct.unpack_from("<II", tail, 1) != (0x10, 0):
        sys.exit("unexpected bytes after the gzip member: %s" % tail[:9].hex())
    isize = struct.unpack_from("<I", blob, blob_len - 4)[0]
    if len(tex) != TEX1_SIZE or isize != TEX1_SIZE:
        sys.exit("inflated size 0x%X / ISIZE 0x%X, expected 0x%X" % (len(tex), isize, TEX1_SIZE))
    if tex[0:4] != b"Tex1":
        sys.exit("inflated image does not start with 'Tex1'")

    os.makedirs(OUT, exist_ok=True)
    for fn, b in (("startupa_img.gz", blob), ("startupa_tex1.bin", tex)):
        with open(os.path.join(OUT, fn), "wb") as f:
            f.write(b)
        print("wrote data/%s (0x%X bytes)" % (fn, len(b)))

    print("gzip member 0x%08X..0x%08X (0x%X bytes), FNAME '%s', MTIME 0x%08X, "
          "pad byte @0x%08X, .eh_frame CIE @0x%08X"
          % (BLOB_VA, BLOB_VA + blob_len - 1, blob_len, fname, mtime,
             BLOB_VA + blob_len, BLOB_VA + blob_len + 1))

    # ---- Tex1 header (Tex1Image) ----
    magic, self_, p08, size = struct.unpack_from("<4sIII", tex, 0)
    vbase, vsize, ntex, nxfer = struct.unpack_from("<4H", tex, 0x10)
    otex, oxfer, optr, oanim = struct.unpack_from("<4I", tex, 0x18)
    print("Tex1: self 0x%X ptr08 0x%X size 0x%X vramBase 0x%X vramSize 0x%X "
          "nTex %d nXfer %d tex @0x%X xfer @0x%X ptrTable 0x%X clutAnim 0x%X"
          % (self_, p08, size, vbase, vsize, ntex, nxfer, otex, oxfer, optr, oanim))
    for i in range(ntex):
        tex0, tex1, mip1, mip2, clamp = struct.unpack_from("<5Q", tex, otex + i * 0x28)
        w = ((clamp >> 14) & 0x3FF) + 1
        h = ((clamp >> 34) & 0x3FF) + 1
        print("  tex[%d] TEX0 %016X TBP0 0x%X TBW %d PSM 0x%X TW %d TH %d CBP 0x%X CSA %d CLD %d"
              " | TEX1 %X (MXL %d) | CLAMP %X -> %dx%d"
              % (i, tex0, tex0 & 0x3FFF, (tex0 >> 14) & 0x3F, (tex0 >> 20) & 0x3F,
                 (tex0 >> 26) & 0xF, (tex0 >> 30) & 0xF, (tex0 >> 37) & 0x3FFF,
                 (tex0 >> 56) & 0x1F, tex0 >> 61, tex1, (tex1 >> 2) & 7, clamp, w, h))
    for i in range(nxfer):
        dofs, dbp, dbw, dpsm, w, h = struct.unpack_from("<IHBBHH", tex, oxfer + i * 12)
        print("  xfer[%d] data @0x%X DBP 0x%X DBW %d DPSM 0x%X %dx%d"
              % (i, dofs, dbp, dbw, dpsm, w, h))


if __name__ == "__main__":
    main()
