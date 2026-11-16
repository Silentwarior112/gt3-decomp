/* color_filter.c - frame-buffer filters of the graphics library
 * (G001), 0x002582F8..0x0025A038.
 *
 * Two filters, each a VIF1 packet for a VU1 micro program (MSCALF 0x763 /
 * 0x78D) that patches and kicks the GIF data page by page.  Both read a
 * CT32 frame-buffer page as a PSMT8 texture (128x64: the bytes of the 64x32
 * pixels), so that a sprite pass can pick one colour byte of every pixel;
 * the byte becomes a CLUT index:
 *   pfx_color_correction (0x002582F8, caller RacePS2Base::drawView):
 *     a tone-curve CLUT (per channel a quadratic Bezier 0 -> c -> 1 over the
 *     input range [lo, hi], 16x16 PSMCT24) and a saturation colour matrix
 *     (luma weights 0.3 / 0.59 / 0.11): three sprite passes read the R, B
 *     and G bytes of the frame buffer through the curve CLUT and add them,
 *     modulated by the matrix column of that channel, into a work page;
 *     then eight sprites per CLUT block in context 2 use the work page as
 *     CLUT for the 8x32 index texture (pfx_upload_index_texture) to write
 *     the pixels back in frame-buffer order.
 *   pfx_split_rgb_planes (caller RacePS2Base::captureAfterimage): the same machinery
 *     with an identity CT32 CLUT, splitting the frame into its R, G and B
 *     bytes, written (FBMSK 0x00FFFFFF) as the alpha bytes of three planes
 *     that RacePS2Base::drawAfterimage reads back as PSMT8H textures.
 * The GIF data is written through a small builder (PfxPacket, 0x00259030..)
 * after a 6-qword header the VU1 program reads (the header words are only
 * named by position: the micro program is not decompiled).
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/gs_driver_internal.h"

/* EE MMI plzcw: leading bits equal to bit 31, minus 1 (low word). */
static inline u32 u118_plzcw(u32 v)
{
    u32 r;

    __asm__("plzcw %0, %1" : "=r"(r) : "r"(v));
    return r;
}

/* max.s / min.s as volatile asm: this file's clamps are scheduling
 * barriers in the binary (nothing moves across them: pfx_color_correction
 * keeps its float code in source order and its argument moves after it,
 * which egcs's MAX_EXPR / MIN_EXPR (-ffast-math) do not give: 63% against
 * exact).  The light setters (gs_lights.c) use the C forms. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 pfx_maxf(f32 a, f32 b)
{
    f32 r;

    __asm__ __volatile__("max.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}

static inline f32 pfx_minf(f32 a, f32 b)
{
    f32 r;

    __asm__ __volatile__("min.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
#else
#define pfx_maxf ee_maxf
#define pfx_minf ee_minf
#endif

/* The R5900 FPU accumulator (cc1 has no patterns for it): ACC = a * b,
 * ACC += a * b, d = ACC - a * b; volatile asm statements, as written in the
 * original's loop (the C between them stays in place).  GCC 14 computes the
 * same in C (acc_, declared by the user). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define ACC_MULA(a, b)     __asm__ __volatile__("mula.s %0, %1" : : "f"(a), "f"(b))
#define ACC_MADDA(a, b)    __asm__ __volatile__("madda.s %0, %1" : : "f"(a), "f"(b))
#define ACC_MSUB(d, a, b)  __asm__ __volatile__("msub.s %0, %1, %2" : "=f"(d) : "f"(a), "f"(b))
#define ACC_DECL
#else
#define ACC_MULA(a, b)     (acc_ = (a) * (b))
#define ACC_MADDA(a, b)    (acc_ += (a) * (b))
#define ACC_MSUB(d, a, b)  ((d) = acc_ - (a) * (b))
#define ACC_DECL           f32 acc_;
#endif

/* CSM1 CLUT index order: bits 3 and 4 of the index swapped. */
#define PFX_CLUT_SWIZZLE(i)  (((i) & 0xE7) | (((i) & 8) << 1) | (((i) & 0x10) >> 1))

/* The CLUT block of sprite i of a context-2 pass: bits 1 and 2 swapped. */
#define PFX_BLOCK_SWIZZLE(i) (((i) & 1) | (((i) & 2) << 1) | (((i) & 4) >> 1))

/* One qword of the packet builder (a statement macro). */
#define PFX_SET_QWORD(q, lo, hi)  do { (q)[0] = (lo); (q)[1] = (hi); } while (0)
/* A FRAME write from its FBW and FBP / PSM / FBMSK parts (temporaries: the
 * two ORs stay as written, FBW first; one expression would be
 * reassociated by fold). */
#define PFX_SET_FRAME(q, fbwBits, rest, reg)  do {                      \
        u64 w_ = (fbwBits);                                             \
        u64 v_ = (rest);                                                \
                                                                        \
        (q)[0] = w_ | v_;                                               \
        (q)[1] = (reg);                                                 \
    } while (0)

/* GIF tags of the filters. */
#define GIF_AD_TAG(n)       (GIF_NREG(1) | (u64)(n))            /* PACKED A+D x n */
#define GIF_IMAGE_TAG(qwc)  (GIF_FLG_IMAGE | GIF_EOP | (u64)(qwc))
/* {UV XYZ3 UV XYZ2} x2 (two sprites per loop), NOP descriptors above. */
#define PFX_REGS_2_SPRITES  (0xFFFFFFFF00000000ull | GIF_REGS_UV_SPRITE << 16 | GIF_REGS_UV_SPRITE)
/* {TEX0_2 PRIM UV XYZ3 UV XYZ2}: one sprite with its CLUT and PRIM. */
#define PFX_REGS_CLUT_SPRITE (0xFFFFFFFFFF000000ull | GIF_REGS_UV_SPRITE << 8 | \
                              GIF_REG(1, SCE_GS_PRIM) | GIF_REG(0, SCE_GS_TEX0_2))
/* {TEX0_2 NOP UV XYZ3 UV XYZ2}: the same with PRIM left to the block's tag. */
#define PFX_REGS_CLUT_SPRITE_NOPRIM (GIF_REGS_UV_SPRITE << 8 | GIF_REG(1, SCE_GS_NOP) | \
                                     GIF_REG(0, SCE_GS_TEX0_2))
_Static_assert(PFX_REGS_2_SPRITES == 0xFFFFFFFF53D353D3ull, "PFX_REGS_2_SPRITES");
_Static_assert(PFX_REGS_CLUT_SPRITE == 0xFFFFFFFFFF53D307ull, "PFX_REGS_CLUT_SPRITE");
_Static_assert(PFX_REGS_CLUT_SPRITE_NOPRIM == 0x53D3F7ull, "PFX_REGS_CLUT_SPRITE_NOPRIM");

/* The sprites of the passes. */
#define PFX_PRIM_SPRITE      (GS_PRIM_SPRITE | GS_PRIM_TME | GS_PRIM_FST)  /* 0x116 */
#define PFX_PRIM_SPRITE_ADD  (PFX_PRIM_SPRITE | GS_PRIM_ABE)               /* 0x156 */
#define PFX_PRIM_SPRITE_CTX2 (PFX_PRIM_SPRITE | GS_PRIM_CTXT2)             /* 0x316 */

/* Register values of the GS state blocks. */
#define PFX_SCISSOR_FULL    0x0FFF00000FFF0000ull   /* SCISSOR 0..0x7FF x 0..0x7FF */
#define PFX_XYOFFSET_1      0x0000800000008000ull   /* (2048, 2048) */
#define PFX_XYOFFSET_2      0x00003F8000000000ull   /* (0, 1016) */
/* ALPHA: Cs * FIX / 128 + Cd with FIX 0x80 (additive). */
#define PFX_ALPHA_ADD       (((u64)0x80 << 32) | PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD))
#define PFX_TEST            0x31002         /* ATST ALWAYS, AFAIL FB_ONLY, ZTE, ZTST ALWAYS */
#define PFX_PRMODECONT_PRIM 1               /* PRMODECONT.AC: attributes from PRIM */
#define PFX_TEX1_LCM        1               /* TEX1.LCM: fixed LOD */
#define PFX_ZBUF_NO_WRITE   0x100000000ull  /* ZBUF.ZMSK */
#define PFX_CLAMP_U_NO_BIT3 0x3F73          /* CLAMP: U region repeat u & 0x3F7 (MINU 0x3F7, MAXU 0) */
#define PFX_UV_8x32         0x02000080      /* UV (8, 32): the index texture's far corner */
/* The context-1 passes' TEX0, CBP in bits 37..50; the low word (TBP0, TBW,
 * PSM, TW, TH bits 0..1) is patched by the micro program per page:
 *   colour: CLD 5 (load), TCC, TFX MODULATE, TH bits 2..3 = 1;
 *   split:  CLD 5, TCC, TFX DECAL, TH bits 2..3 = 2. */
#define PFX_TEX0_CLUT_LOAD_MODULATE 0xA000000580000000ull
#define PFX_TEX0_CLUT_LOAD_DECAL    0xA000000E40000000ull
/* The index texture: TBW 1, PSMT8, 8x32, TCC, DECAL, CLD 5; CBP and TBP0
 * added. */
#define PFX_TEX0_INDEX      0xA000000D4D304000ull

/* The GS registers the packets clobber (re-sent on the next flush).  The
 * colour packet marks COLCLAMP but not FBA; the split packet does not mark
 * ALPHA although its setup block writes ALPHA_1 (both as in the binary). */
#define PFX_DIRTY_CTX_REGS  (GS_DIRTY_BOTH(GS_REG_FRAME) | GS_DIRTY_BOTH(GS_REG_XYOFFSET) | \
                             GS_DIRTY_BOTH(GS_REG_ZBUF) | GS_DIRTY_BOTH(GS_REG_SCISSOR) |     \
                             GS_DIRTY_BOTH(GS_REG_TEST) | GS_DIRTY_BOTH(GS_REG_CLAMP) |       \
                             GS_DIRTY_BOTH(GS_REG_TEX2) | GS_DIRTY_BOTH(GS_REG_TEX1) |        \
                             GS_DIRTY_BOTH(GS_REG_TEX0))
#define PFX_COLOR_DIRTY     (PFX_DIRTY_CTX_REGS | GS_DIRTY_BOTH(GS_REG_ALPHA) |               \
                             GS_DIRTY_GEN(GS_GEN_COLCLAMP) | GS_DIRTY_GEN(GS_GEN_PABE) |      \
                             GS_DIRTY_GEN(GS_GEN_PRMODECONT))
#define PFX_SPLIT_DIRTY     (PFX_DIRTY_CTX_REGS | GS_DIRTY_BOTH(GS_REG_FBA) |                 \
                             GS_DIRTY_GEN(GS_GEN_PABE) | GS_DIRTY_GEN(GS_GEN_PRMODECONT))
_Static_assert(PFX_COLOR_DIRTY == 0x0000003107FB07FBull, "PFX_COLOR_DIRTY");
_Static_assert(PFX_SPLIT_DIRTY == 0x0000003007DF07DFull, "PFX_SPLIT_DIRTY");

// FUNC 0x002582F8 pfx_color_correction
/* workAddr: a free VRAM area (word address): the work page, the CLUT at
 * +0x800, the index texture at +0x900.  r, g, b: the curve control points,
 * clamped to [0, 1]; lo, hi: the input range; saturation: 1.0 keeps the
 * colours, 0.0 gives grey.  The source is the current texture's TBP0 (TEX0
 * shadow), the destination the current FRAME; the scissor is tgtA's size. */
void pfx_color_correction(f32 r, f32 g, f32 b, f32 lo, f32 hi, f32 saturation,
                          u32 workAddr)
{
    GsContext *ctx = GS_SPR_CTX;
    GsRenderTarget *tgt = &ctx->tgtA;
    u32 clutAddr = workAddr + 0x800;
    u32 idxAddr = workAddr + 0x900;
    u32 texAddr = (u32)(s32)(ctx->reg[GS_REG_TEX0] & 0x3FFF) << 6;     /* TEX0.TBP0 */
    u64 scissor = (((u64)tgt->height - 1) << 48) | (((u64)tgt->width - 1) << 16);
    u32 fbAddr = (u32)*(const u16a *)&ctx->reg[GS_REG_FRAME] << 11;     /* FRAME.FBP (lhu) */
    u32 fbw = ((const u8 *)&ctx->reg[GS_REG_FRAME])[2];                /* FRAME.FBW byte (lbu) */
    f32 keep, rest, lumR, lumG, lumB, diagR, diagG, diagB;
    u32 colR, colG, colB;

    pfx_build_curve_clut(clutAddr, r, g, b, lo, hi);
    pfx_upload_index_texture(idxAddr);

    /* The saturation matrix, 0x80 = 1.0: out = keep * in + rest * luma */
    keep = pfx_minf(saturation * 128.0f, 255.0f);
    rest = pfx_maxf(128.0f - keep, 0.0f);
    lumR = rest * U118_F_LUMA_R;
    lumG = rest * U118_F_LUMA_G;
    lumB = rest * U118_F_LUMA_B;
    diagR = pfx_minf(pfx_maxf(keep + lumR, 0.0f), 255.0f);
    diagG = pfx_minf(pfx_maxf(keep + lumG, 0.0f), 255.0f);
    diagB = pfx_minf(pfx_maxf(keep + lumB, 0.0f), 255.0f);
    /* the matrix column of each input channel as an RGBAQ colour */
    colR = (s32)diagR | ((s32)lumR << 8) | ((s32)lumR << 16);
    colG = (s32)lumG | ((s32)diagG << 8) | ((s32)lumG << 16);
    colB = (s32)lumB | ((s32)lumB << 8) | ((s32)diagB << 16);
    pfx_build_color_packet(texAddr, fbAddr, fbw, workAddr, clutAddr, idxAddr,
                           scissor, colR, colG, colB);
}

/* The GS state block of the colour correction: DMAtag cnt 0x12 + VIF1
 * {MSKPATH3 on, FLUSHA, DIRECTHL 0x11} + 16 A+D writes; ctx->cur advances
 * 0x130 bytes (integrated into pfx_build_color_packet; as
 * pfx_write_split_setup, the end pointer taken before the last write). */
static inline void pfx_color_setup(u32 fbw, u32 workPage, u32 fbPage, u64 scissor)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *pw = ctx->cur;
    u64a *pq = (u64a *)pw;
    u32 *next;

    pw[0] = DMATAG_CNT(0x12);
    pw[1] = 0;
    pw[2] = VIF_MSKPATH3_ON;
    pw[3] = VIF_NOP;
    pw[4] = VIF_NOP;
    pw[5] = VIF_NOP;
    pw[6] = VIF_FLUSHA;
    pw[7] = VIF_DIRECTHL(0x11);
    PFX_SET_QWORD(pq + 4, GIF_AD_TAG(16) | GIF_EOP, GIF_REG_AD);
    PFX_SET_QWORD(pq + 6, PFX_SCISSOR_FULL, SCE_GS_SCISSOR_1);
    PFX_SET_QWORD(pq + 8, scissor, SCE_GS_SCISSOR_2);
    PFX_SET_FRAME(pq + 10, (u32)(fbw << 16), (workPage & 0x1FF) | 0xFF00000000000000ull, SCE_GS_FRAME_1);
                                                                /* the work page, CT32, FBMSK 0xFF000000 */
    PFX_SET_FRAME(pq + 12, (u32)(fbw << 16), (fbPage & 0x1FF) | 0xFF00000001000000ull, SCE_GS_FRAME_2);
                                                                /* the frame, CT24, FBMSK 0xFF000000 */
    PFX_SET_QWORD(pq + 14, PFX_XYOFFSET_1, SCE_GS_XYOFFSET_1);
    PFX_SET_QWORD(pq + 16, PFX_XYOFFSET_2, SCE_GS_XYOFFSET_2);
    PFX_SET_QWORD(pq + 18, PFX_ALPHA_ADD, SCE_GS_ALPHA_1);
    PFX_SET_QWORD(pq + 20, PFX_PRMODECONT_PRIM, SCE_GS_PRMODECONT);
    PFX_SET_QWORD(pq + 22, 0, SCE_GS_CLAMP_2);
    PFX_SET_QWORD(pq + 24, PFX_TEX1_LCM, SCE_GS_TEX1_1);
    PFX_SET_QWORD(pq + 26, PFX_TEX1_LCM, SCE_GS_TEX1_2);
    PFX_SET_QWORD(pq + 28, PFX_TEST, SCE_GS_TEST_1);
    PFX_SET_QWORD(pq + 30, PFX_TEST, SCE_GS_TEST_2);
    PFX_SET_QWORD(pq + 32, PFX_ZBUF_NO_WRITE, SCE_GS_ZBUF_1);
    PFX_SET_QWORD(pq + 34, PFX_ZBUF_NO_WRITE, SCE_GS_ZBUF_2);
    next = (u32 *)(pq + 38);
    PFX_SET_QWORD(pq + 36, 0, SCE_GS_PABE);
    ctx->cur = next;
}

// FUNC 0x002584A0 pfx_build_color_packet
/* The colour-correction packet (see the file header).  10 arguments: colG
 * and colB on the stack. */
void pfx_build_color_packet(u32 texAddr, u32 fbAddr, u32 fbw, u32 workAddr,
                            u32 clutAddr, u32 idxAddr, u64 scissor,
                            u32 colR, u32 colG, u32 colB)
{
    u32 texPage = texAddr >> 11;
    u32 fbPage = fbAddr >> 11;
    u32 workPage = workAddr >> 11;
    u32 clutBlk = clutAddr >> 6;
    u32 idxBlk = idxAddr >> 6;
    GsContext *ctx = GS_SPR_CTX;
    u64 *pkt, *end;
    u32 *hdr;
    int texIdx, loop4Start;
    PfxPacket pk;
    s64 i;
    s32 size;

    pfx_color_setup(fbw, workPage, fbPage, scissor);

    pkt = (u64 *)ctx->cur;
    hdr = (u32 *)(pkt + 2);
    pkt_begin(&pk, pkt + 2 + 2 * PFX_HDR_QWC);          /* data after the 6-qword header */

    /* pass 1: R byte, colR; context 1, the curve CLUT loaded */
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE);
    texIdx = pkt_ad(&pk, SCE_GS_TEX0_1, ((u64)clutBlk << 37) | PFX_TEX0_CLUT_LOAD_MODULATE);
    pkt_ad(&pk, SCE_GS_RGBAQ, colR);
    pkt_ad(&pk, SCE_GS_CLAMP_1, 0);
    pkt_tag_reglist(&pk, 8, PFX_REGS_2_SPRITES, 8);
    for (i = 0; i < 8; i++) {
        u64 xy = ((i >> 2) << 24) + ((i & 3) << 8);

        pkt_qword(&pk, i << 8, xy + 0x80008000);
        pkt_qword(&pk, (i << 8) + 0x04000080, xy + 0x81008080);
        pkt_qword(&pk, (i << 8) + 0x00100000, xy + 0x80008080);
        pkt_qword(&pk, (i << 8) + 0x04100080, xy + 0x81008100);
    }

    /* pass 2: B byte, colB, added */
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE_ADD);
    pkt_ad(&pk, SCE_GS_RGBAQ, colB);
    pkt_tag_reglist(&pk, 8, PFX_REGS_2_SPRITES, 8);
    for (i = 0; i < 8; i++) {
        u64 xy = ((i >> 2) << 24) + ((i & 3) << 8);

        pkt_qword(&pk, (i << 8) + 0x80, xy + 0x80008000);
        pkt_qword(&pk, (i << 8) + 0x04000100, xy + 0x81008080);
        pkt_qword(&pk, (i << 8) + 0x00100080, xy + 0x80008080);
        pkt_qword(&pk, (i << 8) + 0x04100100, xy + 0x81008100);
    }

    /* pass 3: G byte, colG, added; CLAMP_1 region repeat (U & 0x3F7) */
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE_ADD);
    pkt_ad(&pk, SCE_GS_RGBAQ, colG);
    pkt_ad(&pk, SCE_GS_CLAMP_1, PFX_CLAMP_U_NO_BIT3);
    pkt_tag_reglist(&pk, 8, PFX_REGS_2_SPRITES, 8);
    for (i = 0; i < 8; i++) {
        u64 xy = ((i >> 2) << 24) + ((i & 3) << 8);

        pkt_qword(&pk, (i << 8) + 0x00200040, xy + 0x80008000);
        pkt_qword(&pk, (i << 8) + 0x042000C0, xy + 0x81008080);
        pkt_qword(&pk, (i << 8) + 0x00300040, xy + 0x80008080);
        pkt_qword(&pk, (i << 8) + 0x043000C0, xy + 0x81008100);
    }

    /* pass 4: context 2, the work page as CLUT (one block per sprite), the
     * index texture as PSMT8; XYZ left 0 for the VU1 patch */
    pkt_tag_reglist(&pk, 6, PFX_REGS_CLUT_SPRITE, 8);
    loop4Start = pk.total;
    for (i = 0; i < 8; i++) {
        pkt_qword(&pk, ((u64)((workPage << 5) + PFX_BLOCK_SWIZZLE(i) * 4) << 37) | ((u64)idxBlk | PFX_TEX0_INDEX),
                  PFX_PRIM_SPRITE_CTX2);                /* TEX0_2 (CBP), PRIM */
        pkt_qword(&pk, 0, 0);
        pkt_qword(&pk, PFX_UV_8x32, 0);
    }
    pkt_set_eop(&pk);

    end = pkt_end(&pk);
    size = (s32)((u32)end - (u32)pkt);
    ((u32a *)end)[0] = VIF_MSCALF(0x763);
    ((u32a *)end)[1] = VIF_FLUSH;
    ((u32a *)end)[3] = VIF_MSKPATH3_OFF;
    ((u32a *)end)[2] = VIF_NOP;
    ((u32a *)pkt)[0] = (size / 16) | DMATAG_CNT(0);
    ((u32a *)pkt)[1] = 0;
    ((u32a *)pkt)[2] = VIF_NOP;
    ((u32a *)pkt)[3] = VIF_UNPACK_V4_32(pk.total + PFX_HDR_QWC);  /* header + data */
    {
        u64 y16 = ((scissor >> 32) & 0xFFF) << 4;       /* SCAY0 (12.4) */
        s64 yPage = (s64)y16 >> 9;                      /* SCAY0 / 32 */
        u64 yOff = y16 & 0xFE00;
        u64 x16 = (scissor & 0xFFF) << 4;               /* SCAX0 (12.4) */
        s64 xPage = (s64)x16 >> 10;                     /* SCAX0 / 64 */
        u64 xOff = x16 & 0xFC00;
        s32 hPages = (s32)((((scissor >> 48) & 0xFFF) >> 5) - yPage);
        s32 wPages = (s32)((((scissor >> 16) & 0xFFF) >> 6) - xPage);
        u32 v;
        s64 n;

        hdr[1] = pk.start + PFX_HDR_QWC;
        hdr[2] = (pk.total + 1) / 2;
        hdr[18] = (u32)(((yOff + 0x3F80) << 16) | xOff);
        ctx->cur = (u32 *)(end + 2);
        hdr[22] = (u32)(((yOff + 0x4180) << 16) | (xOff + 0x80));
        hdr[19] = (u32)(yOff + 0x3780) << 16;
        hdr[23] = (u32)(yOff + 0x3980) << 16;
        hdr[14] = wPages;
        hdr[15] = hPages;
        hdr[3] = !(wPages & 1) && !(hPages & 1);           /* both even */
        /* (fbw - wPages - 1) * 32 as a float-like word: normalised with
         * plzcw, exponent field 0x41 - lz, sign bit set */
        v = (fbw - wPages - 1) << 5;
        n = (s64)(u118_plzcw(v) & 0x1F) - 7;
        hdr[10] = (u32)(((u64)(0x3A - n) << 23) |
                        (((u64)v << (n & 63)) & 0x7FFFFF) | 0x80000000);
        /* TEX0 low word of the source pages: TBP0 = page, TBW 2, PSMT8,
         * TW 128, TH (low bits) */
        hdr[8] = ((texPage + xPage + yPage * 10) << 5) + 0x9D308000;
        ctx->dirty |= PFX_COLOR_DIRTY;
        hdr[5] = loop4Start;
        hdr[9] = 0x94000000;
        hdr[4] = texIdx;
    }
}

// FUNC 0x00258DB0 pfx_build_curve_clut
/* Uploads (DIRECTHL image transfer, 0x36 qwords) the 16x16 PSMCT24
 * tone-curve CLUT at clutAddr: entry k (CSM1 order) is, per channel c in
 * {r, g, b} (clamped to [0, 1]), 255 * B(t) with B(t) = t*t + 2c*t*(1 - t)
 * and t = (swz(k) / 255 - lo) / (hi - lo) clamped to [0, 1] (1 / (hi - lo)
 * is 0 when lo == hi).  Quirk: entry 0's t = -lo / (hi - lo) is not
 * clamped. */
void pfx_build_curve_clut(u32 clutAddr, f32 r, f32 g, f32 b, f32 lo, f32 hi)
{
    u64 *p;
    u8 *out;
    f32 negLo, invRange, t, t2, outR, outG, outB, in;
    int i, idx;
    ACC_DECL

    gs_flush_dirty_regs();
    p = (u64 *)gfx_alloc_direct(0x36);
    negLo = -lo;                                        /* (both paths: delay slot) */
    if (lo != hi)
        invRange = 1.0f / (hi - lo);
    else
        invRange = 0.0f;
    /* The image-transfer header, a qword at a time (statement blocks: each
     * is a scheduling block in the binary, its qword through a copy of the
     * moving pointer). */
    do {
        u64 *q = p;

        p += 2;
        q[0] = GIF_AD_TAG(4);
        q[1] = GIF_REG_AD;
    } while (0);
    do {                                                /* DBP, DBW 1, DPSM CT24 */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0;
        ((u32 *)q)[1] = ((clutAddr >> 6) & 0x3FFF) | (SCE_GS_PSMCT24 << 24) | (1 << 16);
        q[1] = SCE_GS_BITBLTBUF;
    } while (0);
    do {                                                /* (0, 0) */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0;
        ((u32 *)q)[1] = 0;
        q[1] = SCE_GS_TRXPOS;
    } while (0);
    do {                                                /* 16 x 16 */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0x10;
        ((u32 *)q)[1] = 0x10;
        q[1] = SCE_GS_TRXREG;
    } while (0);
    do {                                                /* host -> local */
        u64 *q = p;

        p += 2;
        q[0] = 0;
        q[1] = SCE_GS_TRXDIR;
    } while (0);
    do {                                                /* 256 x 3 bytes */
        u64 *q = p;

        p += 2;
        q[0] = GIF_IMAGE_TAG(0x30);
        q[1] = 0;
    } while (0);
    out = (u8 *)p;

    r = pfx_minf(r, 1.0f);
    g = pfx_minf(g, 1.0f);
    b = pfx_minf(b, 1.0f);
    r = pfx_maxf(r, 0.0f);
    g = pfx_maxf(g, 0.0f);
    b = pfx_maxf(b, 0.0f);
    t = negLo * invRange;
    r += r;
    g += g;
    b += b;
    t2 = t * t;
    i = 0;
    do {
        /* out = (t*t + c*t - c*t2) * 255 per channel (c doubled above) */
        i++;
        ACC_MULA(t, t);
        idx = PFX_CLUT_SWIZZLE(i);                      /* the next entry's index */
        ACC_MADDA(r, t);
        ACC_MSUB(outR, r, t2);
        ACC_MULA(t, t);
        in = (f32)idx;
        ACC_MADDA(g, t);
        ACC_MSUB(outG, g, t2);
        in = in * U118_F_1_255;
        outR *= 255.0f;
        ACC_MULA(t, t);
        ACC_MADDA(b, t);
        ACC_MSUB(outB, b, t2);
        in = in + negLo;
        outG *= 255.0f;
        outB *= 255.0f;
        t = in * invRange;
        outR = pfx_minf(outR, 255.0f);
        outG = pfx_minf(outG, 255.0f);
        outB = pfx_minf(outB, 255.0f);
        t = pfx_minf(t, 1.0f);
        outR = pfx_maxf(outR, 0.0f);
        outG = pfx_maxf(outG, 0.0f);
        outB = pfx_maxf(outB, 0.0f);
        t = pfx_maxf(t, 0.0f);
        t2 = t * t;
        out[0] = (s32)outR;
        out[1] = (s32)outG;
        out[2] = (s32)outB;
        out += 3;
    } while (i < 0x100);
}

/* The 8x32 PSMT8 index texture (.rodata 0x00345590..0x00345690, the file's
 * only .rodata): the indices 0..255, each group of 32 (k = 0, 0x20, ...)
 * as k+0..7, k+16..23, k+12..15, k+8..11, k+28..31, k+24..27 (the PSMT8
 * column swizzle).  Read by pfx_upload_index_texture only. */
const u8 g_00345590[0x100] = {      /* 0x00345590 */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x0C, 0x0D, 0x0E, 0x0F, 0x08, 0x09, 0x0A, 0x0B, 0x1C, 0x1D, 0x1E, 0x1F, 0x18, 0x19, 0x1A, 0x1B,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x2C, 0x2D, 0x2E, 0x2F, 0x28, 0x29, 0x2A, 0x2B, 0x3C, 0x3D, 0x3E, 0x3F, 0x38, 0x39, 0x3A, 0x3B,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x4C, 0x4D, 0x4E, 0x4F, 0x48, 0x49, 0x4A, 0x4B, 0x5C, 0x5D, 0x5E, 0x5F, 0x58, 0x59, 0x5A, 0x5B,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x6C, 0x6D, 0x6E, 0x6F, 0x68, 0x69, 0x6A, 0x6B, 0x7C, 0x7D, 0x7E, 0x7F, 0x78, 0x79, 0x7A, 0x7B,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x8C, 0x8D, 0x8E, 0x8F, 0x88, 0x89, 0x8A, 0x8B, 0x9C, 0x9D, 0x9E, 0x9F, 0x98, 0x99, 0x9A, 0x9B,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xAC, 0xAD, 0xAE, 0xAF, 0xA8, 0xA9, 0xAA, 0xAB, 0xBC, 0xBD, 0xBE, 0xBF, 0xB8, 0xB9, 0xBA, 0xBB,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
    0xCC, 0xCD, 0xCE, 0xCF, 0xC8, 0xC9, 0xCA, 0xCB, 0xDC, 0xDD, 0xDE, 0xDF, 0xD8, 0xD9, 0xDA, 0xDB,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xEC, 0xED, 0xEE, 0xEF, 0xE8, 0xE9, 0xEA, 0xEB, 0xFC, 0xFD, 0xFE, 0xFF, 0xF8, 0xF9, 0xFA, 0xFB,
};   /* 0x00345590 */

// FUNC 0x00259008 pfx_upload_index_texture
/* Uploads the 8x32 PSMT8 index texture g_00345590 (tail jump; buffer width
 * 64). */
void pfx_upload_index_texture(u32 vramAddr)
{
    gs_upload_image(vramAddr, SCE_GS_PSMT8, 0x40, 0, 0, 8, 0x20, g_00345590);
}

// FUNC 0x00259030 pkt_begin
/* (The fields in declaration order: sched1 then puts the stores whose
 * register dies first, as retail.) */
void pkt_begin(PfxPacket *pk, u64 *p)
{
    pk->base = p;
    pk->tag = p;
    pk->n = 0;
    pk->start = 0;
    pk->total = 0;
}

// FUNC 0x00259048 pkt_begin_after
/* A builder continuing after prev's data; returns its start.  No
 * references.  (The return value: retail keeps pkt_end's result in $v0 to
 * the end and copies it to $a0 for the stores; as a void function egcs
 * stores $v0 directly.) */
u64 *pkt_begin_after(PfxPacket *pk, PfxPacket *prev)
{
    u64 *p = pkt_end(prev);

    pk->base = p;
    pk->tag = p;
    pk->n = 0;
    pk->start = prev->start + prev->total;
    pk->total = 0;
    return p;
}

// FUNC 0x002590A0 pkt_tag_ad
/* New block: GIFtag NREG 1, NLOOP 0 (counted by pkt_ad), REGS A+D.  (The
 * qword written through the statement macro: its loop notes keep the tag's
 * constants after the builder's stores, as in retail.) */
void pkt_tag_ad(PfxPacket *pk)
{
    u64 *t = pk->tag + 2 * pk->n;

    pk->n = 1;
    pk->tag = t;
    pk->total++;
    PFX_SET_QWORD(t, GIF_NREG(1), GIF_REG_AD);
}

// FUNC 0x002590E0 pkt_tag_prim
/* New block: GIFtag NREG 1, PRE with PRIM = prim, REGS A+D (then NOP).
 * (Plain stores here: retail schedules the constants first.) */
void pkt_tag_prim(PfxPacket *pk, u64 prim)
{
    u64 *t = pk->tag + 2 * pk->n;

    pk->n = 1;
    pk->tag = t;
    pk->total++;
    t[0] = GIF_PRIM(prim) | GIF_NREG(1) | GIF_PRE;
    t[1] = (u64)-2;                                     /* 0xFFFFFFFFFFFFFFFE: A+D, NOP x15 */
}

// FUNC 0x00259128 pkt_tag_reglist
/* New block: GIFtag REGLIST, NREG nreg, NLOOP nloop, REGS regs.  (nreg and
 * nloop are ints: the int nloop is what keeps the last `or' in nreg's
 * register, as retail.) */
void pkt_tag_reglist(PfxPacket *pk, s32 nreg, u64 regs, s32 nloop)
{
    u64 *t = pk->tag + 2 * pk->n;

    pk->n = 1;
    pk->tag = t;
    pk->total++;
    PFX_SET_QWORD(t, (u64)nloop | GIF_FLG_REGLIST | ((u64)nreg << 60), regs);
}

// FUNC 0x00259170 pkt_set_eop
void pkt_set_eop(PfxPacket *pk)
{
    *(u32a *)pk->tag |= GIF_EOP;
}

// FUNC 0x00259188 pkt_ad
/* One A+D write in the current block; the tag's NLOOP is rewritten to the
 * block's data count (two stores: cleared, then the count).  Returns the
 * qword position of the write. */
int pkt_ad(PfxPacket *pk, u64 reg, u64 data)
{
    u64 *q = pk->tag + 2 * pk->n;

    PFX_SET_QWORD(q, data, reg);
    pk->n++;
    pk->total++;
    *(u32a *)pk->tag &= 0xFFFF8000;                     /* NLOOP cleared */
    *(u32a *)pk->tag += pk->n - 1;
    return pk->total - 1;
}

// FUNC 0x002591E8 pkt_qword
/* One raw qword (REGLIST data); returns its position (callers ignore it). */
int pkt_qword(PfxPacket *pk, u64 lo, u64 hi)
{
    u64 *q = pk->tag + 2 * pk->n;
    int pos = pk->total;

    q[0] = lo;
    q[1] = hi;
    pk->n++;
    pk->total = pos + 1;
    return pos;
}

// FUNC 0x00259218 pkt_position
/* No references: pk->start + pk->total.  (A function of its own in the
 * list since the 2026-10-16 regen; it was counted into pkt_qword.) */
int pkt_position(PfxPacket *pk)
{
    return pk->start + pk->total;
}

// FUNC 0x00259228 pkt_end
u64 *pkt_end(PfxPacket *pk)
{
    return pk->base + 2 * pk->total;
}

// FUNC 0x00259240 pfx_build_identity_clut32
/* Uploads a 16x16 PSMCT32 CLUT (0x46 qwords; the VIF NOP before the
 * DIRECTHL becomes a FLUSH) whose entry k (CSM1 order) is k in all four
 * bytes.  Callers pfx_split_rgb_planes, RacePS2Base::drawAfterimage. */
void pfx_build_identity_clut32(u32 clutAddr)
{
    u64 *p;
    u8 *out;
    int i;

    gs_flush_dirty_regs();
    p = (u64 *)gfx_alloc_direct(0x46);
    ((u32 *)p)[-2] = VIF_FLUSH;                         /* (was NOP) */
    do {
        u64 *q = p;

        p += 2;
        q[0] = GIF_AD_TAG(4);
        q[1] = GIF_REG_AD;
    } while (0);
    do {                                                /* DBP, DBW 1, DPSM CT32 */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0;
        ((u32 *)q)[1] = ((clutAddr >> 6) & 0x3FFF) | (SCE_GS_PSMCT32 << 24) | (1 << 16);
        q[1] = SCE_GS_BITBLTBUF;
    } while (0);
    do {                                                /* (0, 0) */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0;
        ((u32 *)q)[1] = 0;
        q[1] = SCE_GS_TRXPOS;
    } while (0);
    do {                                                /* 16 x 16 */
        u64 *q = p;

        p += 2;
        ((u32 *)q)[0] = 0x10;
        ((u32 *)q)[1] = 0x10;
        q[1] = SCE_GS_TRXREG;
    } while (0);
    do {                                                /* host -> local */
        u64 *q = p;

        p += 2;
        q[0] = 0;
        q[1] = SCE_GS_TRXDIR;
    } while (0);
    do {                                                /* 256 x 4 bytes */
        u64 *q = p;

        p += 2;
        q[0] = GIF_IMAGE_TAG(0x40);
        q[1] = 0;
    } while (0);
    out = (u8 *)p;
    for (i = 0; i < 0x100; i++) {
        int v = PFX_CLUT_SWIZZLE(i);

        out[0] = v;
        out[1] = v;
        out[2] = v;
        out[3] = v;
        out += 4;
    }
}

// FUNC 0x00259368 pfx_split_rgb_planes
/* The plane split: identity CLUT at workAddr + 0x1800, index texture at
 * + 0x1900, then the packet (pfx_build_split_packet) over a w x h scissor,
 * the current texture as source and the three planes (word addresses) that
 * get the red, green and blue bytes in their alpha bytes.  Caller
 * RacePS2Base::captureAfterimage (640 x 224). */
void pfx_split_rgb_planes(u32 fbAddrR, u32 fbAddrG, u32 fbAddrB, s32 w, s32 h, u32 workAddr)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 clutAddr = workAddr + 0x1800;
    u32 idxAddr = workAddr + 0x1900;
    u64 scissor = (((u64)h - 1) << 48) | (((u64)w - 1) << 16);
    u32 texAddr = (u32)(s32)(ctx->reg[GS_REG_TEX0] & 0x3FFF) << 6;     /* TEX0.TBP0 */
    u32 fbw = ((const u8 *)&ctx->reg[GS_REG_FRAME])[2];                /* FRAME.FBW byte */

    pfx_build_identity_clut32(clutAddr);
    pfx_upload_index_texture(idxAddr);
    pfx_build_split_packet(texAddr, fbAddrR, fbAddrG, fbAddrB, workAddr, clutAddr,
                           idxAddr, scissor, fbw);
}

// FUNC 0x00259E90 pfx_write_split_setup
/* The GS state block of the plane split: DMAtag cnt 0x14 + VIF1 {MSKPATH3
 * on, FLUSHA, DIRECTHL 0x13} + 18 A+D writes; ctx->cur advances 0x150
 * bytes.  FRAME_1 is the work area (FBW 1: one page per 32 rows), only its
 * alpha written; FBP = fbPage, not shifted here.  The second argument is
 * not read.  No references: an `inline' (extern) function, integrated into
 * pfx_build_split_packet and emitted by egcs at the end of the file, as the
 * binary has it (0x00259E90, after its caller).  The A+D writes are
 * statement macros (each one a scheduling block in the binary); the end
 * pointer is taken before the last one. */
inline void pfx_write_split_setup(u32 fbPage, u32 unused, u64 scissor)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *pw = ctx->cur;
    u64a *pq = (u64a *)pw;
    u32 *next;

    pw[0] = DMATAG_CNT(0x14);
    pw[1] = 0;
    pw[2] = VIF_MSKPATH3_ON;
    pw[3] = VIF_NOP;
    pw[4] = VIF_NOP;
    pw[5] = VIF_NOP;
    pw[6] = VIF_FLUSHA;
    pw[7] = VIF_DIRECTHL(0x13);
    PFX_SET_QWORD(pq + 4, GIF_AD_TAG(18) | GIF_EOP, GIF_REG_AD);
    PFX_SET_QWORD(pq + 6, PFX_SCISSOR_FULL, SCE_GS_SCISSOR_1);
    PFX_SET_QWORD(pq + 8, scissor, SCE_GS_SCISSOR_2);
    PFX_SET_QWORD(pq + 10, (fbPage & 0x1FF) | 0x00FFFFFF00010000ull, SCE_GS_FRAME_1);  /* FBW 1, CT32, FBMSK 0x00FFFFFF */
    PFX_SET_QWORD(pq + 12, PFX_XYOFFSET_1, SCE_GS_XYOFFSET_1);
    PFX_SET_QWORD(pq + 14, PFX_XYOFFSET_2, SCE_GS_XYOFFSET_2);
    PFX_SET_QWORD(pq + 16, PFX_ALPHA_ADD, SCE_GS_ALPHA_1);
    PFX_SET_QWORD(pq + 18, PFX_PRMODECONT_PRIM, SCE_GS_PRMODECONT);
    PFX_SET_QWORD(pq + 20, 0, SCE_GS_CLAMP_2);
    PFX_SET_QWORD(pq + 22, PFX_TEX1_LCM, SCE_GS_TEX1_1);
    PFX_SET_QWORD(pq + 24, PFX_TEX1_LCM, SCE_GS_TEX1_2);
    PFX_SET_QWORD(pq + 26, PFX_TEST, SCE_GS_TEST_1);
    PFX_SET_QWORD(pq + 28, PFX_TEST, SCE_GS_TEST_2);
    PFX_SET_QWORD(pq + 30, 0x101000000ull, SCE_GS_ZBUF_1);        /* PSMZ24, ZMSK */
    PFX_SET_QWORD(pq + 32, 0x101000000ull, SCE_GS_ZBUF_2);
    PFX_SET_QWORD(pq + 34, 0, SCE_GS_PABE);
    PFX_SET_QWORD(pq + 36, 0, SCE_GS_FBA_1);
    PFX_SET_QWORD(pq + 38, 0, SCE_GS_FBA_2);
    next = (u32 *)(pq + 42);
    PFX_SET_QWORD(pq + 40, 0x80000000, SCE_GS_RGBAQ);  /* RGBA (0, 0, 0, 0x80), Q 0 */
    ctx->cur = next;
}

// FUNC 0x00259448 pfx_build_split_packet
/* The plane-split packet: the GS state block; three context-1 sprite
 * passes reading the R, B and G bytes of the frame (as pfx_build_color_packet,
 * with the identity CLUT loaded from clutAddr) into the alpha of the work
 * pages 0, 1, 2 (rows 0, 32, 64 of the FBW-1 work area); then three
 * context-2 passes, each with its own FRAME_2 write (positions in header
 * words 5, 7, 6) and eight sprites using work page 0 / 1 / 2 as CLUT for the
 * index texture.  The FRAME_2 values are header words 12, 16, 20 (fbAddrR,
 * fbAddrG, fbAddrB): paired in order with words 5, 6, 7, page 0 (red) goes
 * to the red plane, page 2 (green) to the green one, page 1 (blue) to the
 * blue one (? the pairing is the micro program's; it agrees with
 * RacePS2Base::drawAfterimage, which writes the planes to R, G, B).  The 9th
 * argument (fbw) is passed but not read. */
void pfx_build_split_packet(u32 texAddr, u32 fbAddrR, u32 fbAddrG, u32 fbAddrB,
                            u32 workAddr, u32 clutAddr, u32 idxAddr, u64 scissor,
                            u32 fbw)
{
    u32 texBlk = texAddr >> 6;
    u32 fbPageR = fbAddrR >> 11;
    u32 fbPageG = fbAddrG >> 11;
    u32 fbPageB = fbAddrB >> 11;
    u32 workPage = workAddr >> 11;
    u32 clutBlk = clutAddr >> 6;
    u32 idxBlk = idxAddr >> 6;
    GsContext *ctx = GS_SPR_CTX;
    u64 *pkt, *end;
    u32 *hdr;
    u64 tex0;
    int texIdx, frameIdxR, frameIdxB, frameIdxG;
    PfxPacket pk;
    int i;
    s32 size;

    pfx_write_split_setup(workPage, 0, scissor);

    pkt = (u64 *)ctx->cur;
    hdr = (u32 *)(pkt + 2);
    pkt_begin(&pk, pkt + 2 + 2 * PFX_HDR_QWC);
    tex0 = (u64)idxBlk | PFX_TEX0_INDEX;

    /* R byte -> work page 0 (rows 0..31) */
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE);
    texIdx = pkt_ad(&pk, SCE_GS_TEX0_1, ((u64)clutBlk << 37) | PFX_TEX0_CLUT_LOAD_DECAL);
    pkt_ad(&pk, SCE_GS_CLAMP_1, 0);
    pkt_tag_reglist(&pk, 4, GIF_REGS_UV_SPRITE, 0x20);  /* {UV XYZ3 UV XYZ2}, 32 loops */
    for (i = 0; i < 8; i++) {
        u32 xy = ((i >> 2) << 24) | ((i & 3) << 8);

        pkt_qword(&pk, (u32)(i << 8), xy + 0x80008000);
        pkt_qword(&pk, (u32)(i << 8) + 0x04000080, xy + 0x81008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x00100000, xy + 0x80008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x04100080, xy + 0x81008100);
    }
    /* B byte -> work page 1 (rows 32..63) */
    for (i = 0; i < 8; i++) {
        u32 xy = ((i >> 2) << 24) | ((i & 3) << 8);

        pkt_qword(&pk, (u32)(i << 8) + 0x00000080, xy + 0x82008000);
        pkt_qword(&pk, (u32)(i << 8) + 0x04000100, xy + 0x83008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x00100080, xy + 0x82008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x04100100, xy + 0x83008100);
    }

    /* G byte -> work page 2 (rows 64..95), CLAMP_1 region repeat (U & 0x3F7) */
    pkt_tag_ad(&pk);
    pkt_ad(&pk, SCE_GS_CLAMP_1, PFX_CLAMP_U_NO_BIT3);
    pkt_tag_reglist(&pk, 4, GIF_REGS_UV_SPRITE, 0x10);
    for (i = 0; i < 8; i++) {
        u32 xy = ((i >> 2) << 24) | ((i & 3) << 8);

        pkt_qword(&pk, (u32)(i << 8) + 0x00200040, xy + 0x84008000);
        pkt_qword(&pk, (u32)(i << 8) + 0x042000C0, xy + 0x85008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x00300040, xy + 0x84008080);
        pkt_qword(&pk, (u32)(i << 8) + 0x043000C0, xy + 0x85008100);
    }

    /* three context-2 passes, one per work page: {TEX0_2 NOP UV XYZ3 UV
     * XYZ2}, 8 sprites of 8x32 (one destination page); FRAME_2 written
     * alpha only, its FBP patched by the micro program */
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE_CTX2);
    frameIdxR = pkt_ad(&pk, SCE_GS_FRAME_2, 0x00FFFFFF00000000ull);   /* FBMSK 0x00FFFFFF */
    pkt_tag_reglist(&pk, 6, PFX_REGS_CLUT_SPRITE_NOPRIM, 8);
    for (i = 0; i < 8; i++) {
        pkt_qword(&pk, tex0 | ((u64)((workPage << 5) + PFX_BLOCK_SWIZZLE(i) * 4) << 37), 0);
        pkt_qword(&pk, 0, (u32)(i << 7) + 0x3F800000);
        pkt_qword(&pk, PFX_UV_8x32, (u32)(i << 7) + 0x41800080);
    }
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE_CTX2);
    frameIdxB = pkt_ad(&pk, SCE_GS_FRAME_2, 0x00FFFFFF00000000ull);
    pkt_tag_reglist(&pk, 6, PFX_REGS_CLUT_SPRITE_NOPRIM, 8);
    for (i = 0; i < 8; i++) {
        pkt_qword(&pk, tex0 | ((u64)(((workPage + 1) << 5) + PFX_BLOCK_SWIZZLE(i) * 4) << 37), 0);
        pkt_qword(&pk, 0, (u32)(i << 7) + 0x3F800000);
        pkt_qword(&pk, PFX_UV_8x32, (u32)(i << 7) + 0x41800080);
    }
    pkt_tag_prim(&pk, PFX_PRIM_SPRITE_CTX2);
    frameIdxG = pkt_ad(&pk, SCE_GS_FRAME_2, 0x00FFFFFF00000000ull);
    pkt_tag_reglist(&pk, 6, PFX_REGS_CLUT_SPRITE_NOPRIM, 8);
    for (i = 0; i < 8; i++) {
        pkt_qword(&pk, tex0 | ((u64)(((workPage + 2) << 5) + PFX_BLOCK_SWIZZLE(i) * 4) << 37), 0);
        pkt_qword(&pk, 0, (u32)(i << 7) + 0x3F800000);
        pkt_qword(&pk, PFX_UV_8x32, (u32)(i << 7) + 0x41800080);
    }
    pkt_set_eop(&pk);

    end = pkt_end(&pk);
    size = (s32)((u32)end - (u32)pkt);
    ((u32a *)end)[0] = VIF_MSCALF(0x78D);
    ((u32a *)end)[1] = VIF_FLUSH;
    ((u32a *)end)[3] = VIF_MSKPATH3_OFF;
    ((u32a *)end)[2] = VIF_NOP;
    ((u32a *)pkt)[0] = (size / 16) | DMATAG_CNT(0);
    ((u32a *)pkt)[1] = 0;
    ((u32a *)pkt)[2] = VIF_NOP;
    ((u32a *)pkt)[3] = VIF_UNPACK_V4_32(pk.start + (pk.total + PFX_HDR_QWC));
    {
        s32 wPages = (s32)((((scissor >> 16) & 0xFFF) >> 6) - ((scissor & 0xFFF) >> 6) + 1);
        s32 hPages = (s32)((((scissor >> 48) & 0xFFF) >> 5) - (((scissor >> 32) & 0xFFF) >> 5) + 1);
        s32 nPages = wPages * hPages;                   /* frame-buffer pages covered */

        hdr[0] = 6;
        hdr[1] = pk.start + PFX_HDR_QWC;
        hdr[7] = frameIdxB;
        ctx->dirty |= PFX_SPLIT_DIRTY;
        ctx->cur = (u32 *)(end + 2);
        hdr[2] = (pk.total + 1) / 2;
        hdr[10] = nPages - 1;
        hdr[3] = nPages & 1;
        hdr[8] = texBlk | 0xA9308000;
        hdr[9] = 0xA0000000;
        hdr[12] = fbPageR | 0x40000000;
        hdr[16] = fbPageG | 0x40000000;
        hdr[20] = fbPageB | 0x40000000;
        hdr[21] = 0x34800000;
        hdr[5] = frameIdxR;
        hdr[6] = frameIdxG;
        hdr[4] = texIdx;
        hdr[13] = 0x34800000;
        hdr[17] = 0x34800000;
    }
}

