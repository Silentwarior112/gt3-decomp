/* texregs.c - unit u114, 0x0024B930..0x0024BE68 (G001, no FDE: C).
 *
 * Builders of a texture's GS register block (GsTexRegs: TEX0, TEX1,
 * MIPTBP1, MIPTBP2, CLAMP), used by the texture loaders
 * GSBuffer::makeTexRegs (T514) / TexSet::loadImage (T515), and the VRAM
 * footprint of a texture.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_font_internal.h"

// FUNC 0x0024B930 texregs_init
// TEX0 = TCC (RGBA), TEX1 = MMAG | MMIN linear (0x60), the rest 0.
void texregs_init(GsTexRegs *t)
{
    t->miptbp1 = 0;
    t->miptbp2 = 0;
    t->clamp = 0;
    t->tex0 = 1ull << 34;       /* TCC */
    t->tex1 = 0x60;
}

// FUNC 0x0024B958 texregs_set_buffer
// Base pointer / buffer width (addr and width >> 6) of mip level 0..6:
// level 0 = TEX0.TBP0 / TBW (+ PSM, and TCC is set), 1..3 = MIPTBP1
// TBP1..3 / TBW1..3, 4..6 = MIPTBP2 TBP4..6 / TBW4..6.
void texregs_set_buffer(GsTexRegs *t, u32 level, u32 addr, s32 psm, s32 width)
{
    u32 tbp = addr >> 6;
    s32 tbw = width >> 6;

    switch (level) {
    case 0:
        t->tex0 = ((((((t->tex0 & ~0x3FFFull) | (tbp & 0x3FFF)) & 0xFFFFFFFFFFF03FFFull) |
                     ((u64)(tbw & 0x3F) << 14)) & 0xFFFFFFFFFC0FFFFFull) |
                   ((u64)(psm & 0x3F) << 20)) | (1ull << 34);
        break;
    case 1:
        t->miptbp1 = (((t->miptbp1 & ~0x3FFFull) | (tbp & 0x3FFF)) & 0xFFFFFFFFFFF03FFFull) |
                     ((u64)(tbw & 0x3F) << 14);
        break;
    case 2:
        t->miptbp1 = (((t->miptbp1 & 0xFFFFFFFC000FFFFFull) | ((u64)(tbp & 0x3FFF) << 20)) &
                      0xFFFFFF03FFFFFFFFull) | ((u64)(tbw & 0x3F) << 34);
        break;
    case 3:
        t->miptbp1 = (((t->miptbp1 & 0xFFC000FFFFFFFFFFull) | ((u64)(tbp & 0x3FFF) << 40)) &
                      0xF03FFFFFFFFFFFFFull) | ((u64)(tbw & 0x3F) << 54);
        break;
    case 4:
        t->miptbp2 = (((t->miptbp2 & ~0x3FFFull) | (tbp & 0x3FFF)) & 0xFFFFFFFFFFF03FFFull) |
                     ((u64)(tbw & 0x3F) << 14);
        break;
    case 5:
        t->miptbp2 = (((t->miptbp2 & 0xFFFFFFFC000FFFFFull) | ((u64)(tbp & 0x3FFF) << 20)) &
                      0xFFFFFF03FFFFFFFFull) | ((u64)(tbw & 0x3F) << 34);
        break;
    case 6:
        t->miptbp2 = (((t->miptbp2 & 0xFFC000FFFFFFFFFFull) | ((u64)(tbp & 0x3FFF) << 40)) &
                      0xF03FFFFFFFFFFFFFull) | ((u64)(tbw & 0x3F) << 54);
        break;
    }
}

// FUNC 0x0024BBA0 texregs_set_size
// TEX0.TW / TH = log2 of w / h rounded up (1 for <= 2, at most 10).  The
// fields are bitfield stores (the masks of the TH store come after both
// loops, as the bitfield expansion places them; with explicit masks GCSE
// hoists logW & 0xF into the second loop's preheader).
// Callers: T449 RaceTirePacket::drawWheelTexture, makeTexRegs, loadImage.
void texregs_set_size(u64 *tex0, s32 w, s32 h)
{
    s32 logW, logH;

    for (logW = 1; logW < 10 && (1 << logW) < w; logW++)
        ;
    for (logH = 1; logH < 10 && (1 << logH) < h; logH++)
        ;
    ((GsTex0Bits *)tex0)->tw = logW;
    ((GsTex0Bits *)tex0)->th = logH;
}

// FUNC 0x0024BC68 texregs_set_mode
// One field: 0 TEX0.TCC, 1 TEX0.TFX, 2 TEX1.MMIN, 3 TEX1.MMAG, 4 CLAMP.WMS,
// 5 CLAMP.WMT, 6 TEX1.LCM, 7 TEX1.MXL, 8 TEX1.K, 9 TEX1.L (PGL_TEXPARAM_*).
void texregs_set_mode(GsTexRegs *t, u32 what, s32 v)
{
    switch (what) {
    case PGL_TEXPARAM_TCC:
        t->tex0 = (t->tex0 & 0xFFFFFFFBFFFFFFFFull) | ((u64)(v & 1) << 34);
        break;
    case PGL_TEXPARAM_TFX:
        t->tex0 = (t->tex0 & 0xFFFFFFE7FFFFFFFFull) | ((u64)(v & 3) << 35);
        break;
    case PGL_TEXPARAM_MMIN:
        t->tex1 = (t->tex1 & ~0x1C0ull) | ((u64)(v & 7) << 6);
        break;
    case PGL_TEXPARAM_MMAG:
        t->tex1 = (t->tex1 & ~0x20ull) | ((u64)(v & 1) << 5);
        break;
    case PGL_TEXPARAM_WMS:
        t->clamp = (t->clamp & ~3ull) | (v & 3);
        break;
    case PGL_TEXPARAM_WMT:
        t->clamp = (t->clamp & ~0xCull) | ((v & 3) << 2);
        break;
    case PGL_TEXPARAM_LCM:
        t->tex1 = (t->tex1 & ~1ull) | (v & 1);
        break;
    case PGL_TEXPARAM_MXL:
        t->tex1 = (t->tex1 & ~0x1Cull) | ((v & 7) << 2);
        break;
    case PGL_TEXPARAM_K:
        t->tex1 = (t->tex1 & 0xFFFFF000FFFFFFFFull) | ((u64)(v & 0xFFF) << 32);
        break;
    case PGL_TEXPARAM_L:
        t->tex1 = (t->tex1 & 0xFFFFFFFFFFE7FFFFull) | ((u64)(v & 3) << 19);
        break;
    }
}

// FUNC 0x0024BDC8 texregs_set_clamp_region
// CLAMP MINU / MAXU (axis 0) or MINV / MAXV (axis 1).
void texregs_set_clamp_region(GsTexRegs *t, s32 axis, s32 lo, s32 hi)
{
    switch (axis) {
    case PGL_REGION_U:
        t->clamp = (((t->clamp & ~0x3FF0ull) | ((u64)(lo & 0x3FF) << 4)) & 0xFFFFFFFFFF003FFFull) |
                   ((u64)(hi & 0x3FF) << 14);
        break;
    case PGL_REGION_V:
        t->clamp = (((t->clamp & 0xFFFFFFFC00FFFFFFull) | ((u64)(lo & 0x3FF) << 24)) &
                    0xFFFFF003FFFFFFFFull) | ((u64)(hi & 0x3FF) << 34);
        break;
    }
}

// FUNC 0x0024BE68 tex_vram_size
// VRAM size of a w x h texture of the pixel format psm, in 32-bit words:
// w, h rounded up to the format's page (PSMCT32/24, Z32/24 and PSMT8H /
// 4HL / 4HH 64x32; PSMCT16(S), Z16(S) 64x64; PSMT8 128x64; PSMT4 128x128),
// *alignedW (if given) = the rounded width; -1 for another psm.  (The
// << 11 overflows from 1024 x 1024 on, as in the code.)  Callers:
// VramAllocator::loadImage (T513), GSBuffer::vramSize (T514),
// TexSet::loadImage (T515).
s32 tex_vram_size(u32 psm, s32 w, s32 h, s32 *alignedW)
{
    s32 pageW, pageH, shift, roundW, roundH;

    switch (psm) {
    case GS_PSMCT32: case GS_PSMCT24: case GS_PSMT8H: case GS_PSMT4HL: case GS_PSMT4HH:
    case GS_PSMZ32: case GS_PSMZ24:
        pageW = 0x40; pageH = 0x20; shift = 11;
        break;
    case GS_PSMCT16: case GS_PSMCT16S: case GS_PSMZ16: case GS_PSMZ16S:
        pageW = 0x40; pageH = 0x40; shift = 12;
        break;
    case GS_PSMT8:
        pageW = 0x80; pageH = 0x40; shift = 13;
        break;
    case GS_PSMT4:
        pageW = 0x80; pageH = 0x80; shift = 14;
        break;
    default:
        return -1;
    }
    roundW = (w + pageW - 1) & -pageW;
    roundH = (h + pageH - 1) & -pageH;
    if (alignedW != NULL)
        *alignedW = roundW;
    return (roundW * roundH << 11) >> shift;
}
