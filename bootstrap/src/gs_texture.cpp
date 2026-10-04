/* gs_texture.cpp - the GS driver's texture register setter (GT3 US bootstrap,
 * BOOTSTRAP_SCUS-97102), unit "gs", .text 0x0100BE10..0x0100BF98.
 *
 * The first of the driver's three translation units in link order
 * (tools/layout.py; see src/gs_driver.cpp for the whole picture):
 *   - its .data is s_gsTexOverride @0x01034640, which lies BEFORE the VU1
 *     microcode blob (g_gsVu1Microcode, .data 0x01034730..0x010384D8, linked
 *     between the objects), while the main driver's .data (g_010384D8,
 *     s_gsDmaChan) comes after it - so gfx_set_texture_regs, the only user,
 *     is in an object of its own, linked before the blob and the main driver;
 *   - no EH frame block in retail covers 0x0100BE10 (the collect2 table at
 *     0x01026C00 lists every object compiled with exceptions): compiled with
 *     -fno-exceptions like the rest of the driver (ref/compile_flags.json).
 * gfx_wait_vsync_poll and gfx_reset_paths (0x0100BF98, 0x0100BFE8) have no
 * data references, so the boundary could also be after them; they are kept
 * with gfx_init, their caller, in src/gs_driver.cpp.
 */
#include "types.h"
#include "gs_driver.h"
#include "gs_internal.h"

/* ~a as the R5900 `not` (nor rd, rs, $zero).  egcs never emits that operand
 * order outside MIPS16 (its one_cmpl template is `nor rd, $zero, rs`, as in
 * every other complement of the binary), so the three retail `not`s
 * (0x0100BED0, 0x0100ED80, 0x0100F554) come from inline asm.             */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline u64 ee_not64(u64 a)
{
    u64 r;
    __asm__("not %0, %1" : "=r"(r) : "r"(a));
    return r;
}
#else
static inline u64 ee_not64(u64 a) { return ~a; }
#endif

/* @0x01034640 .data - texture register override (disabled). */
static GsTexOverride s_gsTexOverride = {
    0xE000001800000000ull,      /* tex0Mask  (TEX0.TFX bits 35-36, CLD 61-63)     */
    0x00000000000001E0ull,      /* tex1Mask  (TEX1 MMAG / MMIN)                   */
    0,                          /* clampMask                                      */
    0x00010060,                 /* unk18                                          */
    0,                          /* enable                                         */
    { 0, 0, 0 },
};

// FUNC 0x0100BE10 gfx_set_texture_regs
// Loads a texture's register block into the context shadows: CLAMP_1, TEX0_1
// (with the CLD bits 61-63 stripped), TEX1_1 and marks CLAMP/TEX1/TEX0 dirty in
// both contexts (0x06800680).  If TEX0.CLD != 0 the full TEX0 (CLD kept) goes
// to the TEX2 shadow - with PSM forced to PSMT8 and CSA cleared when CSA != 0 -
// to trigger the CLUT load.  With mipmapping (TEX1.MXL != 0) TEX1.K gets the
// context LOD bias (lodBias << TEX1.L, in 1/16 units) added and MIPTBP1 (and
// MIPTBP2 when MXL >= 4) are loaded.  s_gsTexOverride can pin bits to their
// current value (disabled in this binary).  Caller: startup screen F_01000B40.
void gfx_set_texture_regs(const GsTexRegs *t)
{
    GsContext *ctx = GS_SPR_CTX;
    u64 tex0  = t->tex0;
    u64 tex1  = t->tex1;
    u64 clamp = t->clamp;
    u64 dirty = ctx->dirty;
    s32 mxl;

    if (s_gsTexOverride.enable) {
        GsTexOverride *o = &s_gsTexOverride;
        tex0  &= ~o->tex0Mask;
        tex0  |= ctx->reg[GS_REG_TEX0] & o->tex0Mask;
        tex1  &= ~o->tex1Mask;
        tex1  |= ctx->reg[GS_REG_TEX1] & o->tex1Mask;
        clamp &= ~o->clampMask;
        clamp |= ctx->reg[GS_REG_CLAMP] & o->clampMask;
    }

    ctx->reg[GS_REG_CLAMP] = clamp;
    ctx->reg[GS_REG_TEX0]  = tex0 & (~0ull >> 3);             /* drop CLD */
    dirty |= 0x06800680;                                       /* CLAMP|TEX1|TEX0, ctx 1+2 */
    ctx->reg[GS_REG_TEX1]  = tex1;

    if ((tex0 >> 61) != 0) {                                   /* TEX0.CLD */
        if (((tex0 >> 56) & 0x1F) != 0) {                      /* TEX0.CSA */
            tex0 &= ee_not64(0x1F00000003F00000ull);           /* clear CSA and PSM */
            tex0 |= 0x01300000;                                /* PSM = PSMT8 */
        }
        ctx->reg[GS_REG_TEX2] = tex0;
        dirty |= 0x01000100;                                   /* TEX2 */
    }

    mxl = (s32)(tex1 & 0x1C);                                  /* TEX1.MXL << 2 */
    if (mxl != 0) {
        s32 k = (s32)((tex1 >> 32) & 0xFFF);                   /* TEX1.K (s7.4) */
        s32 l = (s32)((tex1 >> 19) & 3);                       /* TEX1.L */
        k = (s32)((u32)k << 20) >> 16;                         /* sext12(K) * 16 */
        k += ctx->lodBias << l;                                /* + bias (1/16 units) */
        k >>= 4;
        ctx->reg[GS_REG_TEX1] = (ctx->reg[GS_REG_TEX1] & 0xFFFFF000FFFFFFFFull)
                              | ((u64)(k & 0xFFF) << 32);
        dirty |= 0x08000800;                                   /* MIPTBP1 */
        ctx->reg[GS_REG_MIPTBP1] = t->miptbp1;
        if ((u32)mxl >= 0xD) {                                 /* MXL >= 4 */
            dirty |= 0x10001000;                               /* MIPTBP2 */
            ctx->reg[GS_REG_MIPTBP2] = t->miptbp2;
        }
    }
    ctx->dirty = dirty;
}
