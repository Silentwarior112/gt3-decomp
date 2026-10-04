/* gs_state.cpp - the GS driver's shadow-register flush and material setters
 * (GT3 US bootstrap, BOOTSTRAP_SCUS-97102), unit "gs", .text
 * 0x0100EE48..0x0100F130.
 *
 * The last of the driver's three translation units in link order
 * (tools/layout.py; see src/gs_driver.cpp for the whole picture):
 *   - its .data (the register-number tables @0x01039510..) comes AFTER
 *     libgraph's .data ('PsIIlibgraph2221' @0x010394F0, sceGsGParam
 *     @0x01039500), while the main driver's s_gsDmaChan (@0x010394D8) comes
 *     before it: libgraph's object was linked between the main driver and
 *     this one.  (libgraph was built with -ffunction-sections: its code is in
 *     .text.sceGsGetGParam / .text.sceGsResetGraph at the end of the image,
 *     so it leaves no trace between the two in .text.)
 *   - its .rodata (gs_store_material_vec's jump table @0x0103E5E0) follows the
 *     main driver's (0x0103E360..0x0103E5E0) directly;
 *   - no EH frame block in retail covers 0x0100EE48..0x0100F130: compiled
 *     with -fno-exceptions (ref/compile_flags.json).
 * The data fixes the boundary only to somewhere after dma_start_source_chain
 * (0x0100EC20) and before gs_collect_dirty_regs; gs_emit_masked_regs, whose
 * only caller is gs_collect_dirty_regs, is placed here with it, the other
 * functions in between (gs_compute_display_period, gs_set_bg_color,
 * pack_rgba_floats_to_u32) stay in src/gs_driver.cpp.
 */
#include "types.h"
#include "gs_driver.h"
#include "gs_internal.h"

/* @0x01039510 / 0x01039520 / 0x01039530 .data - GS register numbers in shadow
 * order.  Writable data in the original: retail has them in .data (between
 * libgraph's GParam and the allocator's EH frame block), and egcs would put
 * `const` tables in .rodata.  Nothing writes them.                         */
static u8 s_gsRegsGeneral[GS_GEN_COUNT] = {
    0x46 /*COLCLAMP*/, 0x44 /*DIMX*/, 0x45 /*DTHE*/, 0x3D /*FOGCOL*/, 0x49 /*PABE*/,
    0x1A /*PRMODECONT*/, 0x22 /*SCANMSK*/, 0x3B /*TEXA*/, 0x1C /*TEXCLUT*/, 0x3F /*TEXFLUSH*/,
};
static u8 s_gsRegsCtx1[GS_REG_COUNT] = {
    0x4C /*FRAME_1*/, 0x18 /*XYOFFSET_1*/, 0x4A /*FBA_1*/, 0x4E /*ZBUF_1*/, 0x40 /*SCISSOR_1*/,
    0x42 /*ALPHA_1*/, 0x47 /*TEST_1*/, 0x08 /*CLAMP_1*/, 0x16 /*TEX2_1*/, 0x14 /*TEX1_1*/,
    0x06 /*TEX0_1*/, 0x34 /*MIPTBP1_1*/, 0x36 /*MIPTBP2_1*/,
};
static u8 s_gsRegsCtx2[GS_REG_COUNT] = {
    0x4D /*FRAME_2*/, 0x19 /*XYOFFSET_2*/, 0x4B /*FBA_2*/, 0x4F /*ZBUF_2*/, 0x41 /*SCISSOR_2*/,
    0x43 /*ALPHA_2*/, 0x48 /*TEST_2*/, 0x09 /*CLAMP_2*/, 0x17 /*TEX2_2*/, 0x15 /*TEX1_2*/,
    0x07 /*TEX0_2*/, 0x35 /*MIPTBP1_2*/, 0x37 /*MIPTBP2_2*/,
};

// FUNC 0x0100EE48 gs_emit_masked_regs
// For each bit of mask (LSB first, do-while: runs at least once) emits
// {shadow[i], regNums[i]} as one A+D qword when the bit is set; returns the
// new output pointer.  Both values are loaded before either store (the
// binary's lbu precedes the first sd, and egcs cannot move a byte load over a
// store through another pointer), so an `out` overlapping regNums / shadow
// sees the old bytes, as in the original.
u64 *gs_emit_masked_regs(u64 *out, u64 mask, const u8 *regNums, const u64 *shadow)
{
    do {
        shadow++;
        if ((mask & 1) == 1) {
            u64 v = *(shadow - 1);
            u8 r = *regNums;
            out += 2;
            *(out - 2) = v;
            *(out - 1) = r;
        }
        mask >>= 1;
        regNums++;
    } while (mask != 0);
    return out;
}

// FUNC 0x0100EE80 gs_collect_dirty_regs
// mask = dirty & enable; if 0 returns 0.  Removes mask from dirty, adds it to
// written, emits general regs (bits 32-41), context-1 regs (bits 0-12) and
// context-2 regs (bits 16-28, same shadows) as A+D qwords at out; returns
// the number of qwords.
int gs_collect_dirty_regs(GsContext *ctx, u64 *out)
{
    u64 dirty = ctx->dirty;
    u64 mask = dirty & ctx->enable;
    u64 *p = out;
    u64 m;

    if (mask == 0)
        return 0;
    ctx->dirty = dirty & ~mask;
    ctx->written |= mask;

    /* Each group's bits extracted in two steps (shift, then mask) into the
     * one variable m: egcs then keeps the shift result in m's register
     * (no separate temporary), as in the binary. */
    m = mask >> 32;
    m &= 0x3FF;
    if (m != 0)
        p = gs_emit_masked_regs(p, m, s_gsRegsGeneral, ctx->gen);
    m = mask;
    m &= 0x1FFF;
    if (m != 0)
        p = gs_emit_masked_regs(p, m, s_gsRegsCtx1, ctx->reg);
    m = mask >> 16;
    m &= 0x1FFF;
    if (m != 0)
        p = gs_emit_masked_regs(p, m, s_gsRegsCtx2, ctx->reg);
    return (int)((u8 *)p - (u8 *)out) >> 4;
}

// FUNC 0x0100EF50 gs_store_material_vec
// Stores a 4-float vector (passed in the 128-bit register $a3 in the binary,
// written with sq) into material[face]: kind 0 ambient, 1 diffuse, 2 diffuse
// and ambient, 3 +0x30, 4 +0x20 (jump table @0x0103E5E0; kind >= 5 stores
// nothing); always lightFlags |= 4.  (The listing's '0x103ef40 .rodata'
// annotations on the sq instructions are bogus - the base is computed.)
void gs_store_material_vec(GsContext *ctx, int face, u32 kind, GsVec4Arg v)
{
    switch (kind) {
    case 0:
        *(GsVec4Arg *)&ctx->material[face].ambient = v;
        break;
    case 1:
        *(GsVec4Arg *)&ctx->material[face].diffuse = v;
        break;
    case 2:                         /* (emitted diffuse first, ambient in the delay slot) */
        *(GsVec4Arg *)&ctx->material[face].ambient = v;
        *(GsVec4Arg *)&ctx->material[face].diffuse = v;
        break;
    case 4:
        *(GsVec4Arg *)&ctx->material[face].specular = v;
        break;
    case 3:
        *(GsVec4Arg *)&ctx->material[face].emission = v;
        break;
    }
    ctx->lightFlags |= 4;
}

// FUNC 0x0100EFF0 gs_set_material_param
// glMaterial-like setter.  kind 11: value (unsigned) -> float clamped to
// [0, 128], stored with its encoding as shininess of face 0 (faceMask bit 0)
// and/or face 1 (bit 1), lightFlags |= 4.  Other kinds: the RGBA bytes of
// value -> {r,g,b,a} / 255 (VU0: pextlb/pextlh, qmtc2, vitof0, vmulx by
// 0x3B808081 = 1/255, qmfc2 - VU float rounding may differ in the last bit
// from this C) stored via gs_store_material_vec.  Quirk: both the bit-0 and
// the bit-1 call pass face 0 (a1 = 0 in both), so face 1 never gets vectors.
// Callers: gs_set_bg_color (never with bgImmediate set in this binary).
// Matching (96.8%): retail loads 128.0 before 0.0 and gives 128.0 $f0; see
// MATCHING.md, "Inline asm and MMI", max/min.
void gs_set_material_param(int faceMask, int kind, u32 value)
{
    GsContext *ctx = GS_SPR_CTX;
    GsVec4Arg v;

    if (kind == 11) {
        f32 raw = (f32)value;                               /* u32 -> float */
        f32 f = ee_minf(ee_maxf(raw, 0.0f), 128.0f);
        u32 enc = float_to_gs_param_encode(f);
        if (faceMask & 1) {
            ctx->material[0].shininess = f;
            ctx->material[0].zero44 = 0;
            ctx->material[0].shininessEnc = enc;
        }
        if (faceMask & 2) {
            ctx->material[1].shininess = f;
            ctx->material[1].shininessEnc = enc;
            ctx->material[1].zero44 = 0;
        }
        ctx->lightFlags |= 4;
        return;                     /* (this early return is why the last
                                     *  call below is not a tail call)  */
    }

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* VU0 macro code: bytes -> words (pextlb / pextlh with $0), to floats
     * (vitof0), times 1/255 (0x3B808081), back in one GPR. */
    __asm__ __volatile__(
        "pextlb     %0, $0, %1\n\t"
        "pextlh     %0, $0, %0\n\t"
        "vnop\n\t"
        "qmtc2.ni   %0, $vf1\n\t"
        "vitof0.xyzw $vf1, $vf1\n\t"
        "lui        %0, 0x3B80\n\t"
        "ori        %0, %0, 0x8081\n\t"
        "qmtc2.ni   %0, $vf2\n\t"
        "vmulx.xyzw $vf1, $vf1, $vf2x\n\t"
        "qmfc2.ni   %0, $vf1"
        : "=&r"(v) : "r"(value));
#else
    {
        /* 1/255 = 0x3B808081, the VU0 constant above (rounded to nearest).
         * GCC 14 folds `1.0f / 255.0f` with the R5900's truncating float
         * format to 0x3B808080, so it gets the exact value in hex. */
#ifdef BOOT_OLD_GCC
        const f32 k = 1.0f / 255.0f;
#else
        const f32 k = 0x1.010102p-8f;
#endif
        v.x = (f32)(s32)(value & 0xFF) * k;
        v.y = (f32)(s32)((value >> 8) & 0xFF) * k;
        v.z = (f32)(s32)((value >> 16) & 0xFF) * k;
        v.w = (f32)(s32)((value >> 24) & 0xFF) * k;
    }
#endif
    if (faceMask & 1)
        gs_store_material_vec(ctx, 0, (u32)kind, v);
    if (faceMask & 2)
        gs_store_material_vec(ctx, 0, (u32)kind, v);        /* face 0 again (quirk) */
}
