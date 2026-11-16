/* gs_driver.c - the GS driver proper of the graphics library
 * (G001), 0x0025A038..0x0025ABD0: background colour, the LCG that
 * unscrambles the VU1 code, the dirty-register flush, the INTC / DMAC
 * handlers, the GS->EE readback, the VU1 program selection, an
 * image-readback packet and the DIRECT packet allocator.
 *
 * The functions marked "bootstrap" are the same machine code as the
 * bootstrap's (decomp/bootstrap/src/gs_driver.cpp / gs_state.cpp, address
 * in each comment); their source is reused from there.  In the core the
 * dirty-register flush sits between the LCG and the handlers, so the
 * bootstrap's gs_state.cpp split does not apply: one module here.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/gs_driver_internal.h"

// FUNC 0x0025A038 pgl_color
// The current colour (glColor-like): color = rgba (untextured
// primitives); colorTex = each colour byte c -> (c * 129) >> 8 (MMI pextlb /
// dsll 7 + daddu / dsrl 8 / ppacb; 0x80 = 1.0 for the texture modulation),
// alpha byte kept.  With colorMaterial (PGL_CAP_COLOR_MATERIAL) it
// tail-calls gs_set_material_param(colorMaterialFaces, colorMaterialKind,
// rgba & 0xFF000000) - quirk: only the alpha byte is passed (no caller
// found enables the switch).  (bootstrap 0x0100ED58)
void pgl_color(u32 rgba)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 scaled;
    u32 mask;
    u32 alpha;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    u64 halves, halvesScaled;
#else
    int i;
#endif

    ctx->color = rgba;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    __asm__ __volatile__("pextlb %0, $0, %1" : "=r"(halves) : "r"(rgba));  /* bytes -> halfwords */
    halvesScaled = ((halves << 7) + halves) >> 8;
    mask = 0xFF000000;
    __asm__ __volatile__("ppacb  %0, $0, %1" : "=r"(scaled) : "r"(halvesScaled));
#else
    scaled = 0;
    for (i = 0; i < 4; i++) {
        u32 c = (rgba >> (8 * i)) & 0xFF;
        scaled |= (((c * 129) >> 8) & 0xFF) << (8 * i);
    }
    mask = 0xFF000000;
#endif
    alpha = rgba & mask;
    ctx->colorTex = (scaled & ee_not32(mask)) | alpha;
    if (ctx->colorMaterial)
        gs_set_material_param(ctx->colorMaterialFaces, ctx->colorMaterialKind, alpha);
}

// FUNC 0x0025A090 pack_rgba_floats_to_u32
// r, g, b scaled by `scale` (5th float arg, f16), a by 128.0; each
// max(x, 0) + 0.5, min(.., 255.0), truncated; returns r | g << 8 | b << 16 |
// a << 24.  Hand-written EE assembly in the original (works in place on
// $f12..$f16, COP1 max.s / min.s).  (bootstrap 0x0100EDB0)
u32 pack_rgba_floats_to_u32(float r, float g, float b, float a, float scale)
{
    s32 ir, ig, ib, ia;

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    __asm__(
        "mul.s   $f12, $f12, $f16\n\t"
        "lui     %0, 0x4300\n\t"
        "mul.s   $f13, $f13, $f16\n\t"
        "mtc1    %0, $f1\n\t"
        "mul.s   $f14, $f14, $f16\n\t"
        "mtc1    $0, $f0\n\t"
        "mul.s   $f15, $f15, $f1\n\t"
        "lui     %0, 0x3F00\n\t"
        "max.s   $f12, $f12, $f0\n\t"
        "mtc1    %0, $f1\n\t"
        "max.s   $f13, $f13, $f0\n\t"
        "lui     %0, 0x437F\n\t"
        "max.s   $f14, $f14, $f0\n\t"
        "mtc1    %0, $f2\n\t"
        "max.s   $f15, $f15, $f0\n\t"
        "add.s   $f12, $f12, $f1\n\t"
        "add.s   $f13, $f13, $f1\n\t"
        "add.s   $f14, $f14, $f1\n\t"
        "add.s   $f15, $f15, $f1\n\t"
        "min.s   $f12, $f12, $f2\n\t"
        "min.s   $f13, $f13, $f2\n\t"
        "min.s   $f14, $f14, $f2\n\t"
        "min.s   $f15, $f15, $f2\n\t"
        "cvt.w.s $f12, $f12\n\t"
        "cvt.w.s $f13, $f13\n\t"
        "cvt.w.s $f14, $f14\n\t"
        "cvt.w.s $f15, $f15\n\t"
        "mfc1    %0, $f12\n\t"
        "mfc1    %1, $f13\n\t"
        "mfc1    %2, $f14\n\t"
        "mfc1    %3, $f15"
        : "=&r"(ir), "=r"(ig), "=r"(ib), "=r"(ia)
        : "f"(r), "f"(g), "f"(b), "f"(a), "f"(scale)
        : "$f0", "$f1", "$f2");
#else
    r *= scale;
    g *= scale;
    b *= scale;
    a *= 128.0f;
    r = ee_minf(ee_maxf(r, 0.0f) + 0.5f, 255.0f);
    g = ee_minf(ee_maxf(g, 0.0f) + 0.5f, 255.0f);
    b = ee_minf(ee_maxf(b, 0.0f) + 0.5f, 255.0f);
    a = ee_minf(ee_maxf(a, 0.0f) + 0.5f, 255.0f);
    ir = (s32)r;
    ig = (s32)g;
    ib = (s32)b;
    ia = (s32)a;
#endif
    ir = ir | (ig << 8) | (ib << 16);
    return ir | (ia << 24);
}

// FUNC 0x0025A128 pgl_color4f
/* pgl_color(pack(r, g, b, a, 255.0)) (tail jump); 31 callers
 * (transition objects, MovieRender::render, ...). */
void pgl_color4f(float r, float g, float b, float a)
{
    pgl_color(pack_rgba_floats_to_u32(r, g, b, a, 255.0f));
}

// FUNC 0x0025A150 pgl_color3f
// Packs (r, g, b, alpha 1.0) with scale 255.0 and tail-jumps
// pgl_color.  (bootstrap 0x0100E478)
void pgl_color3f(float r, float g, float b)
{
    pgl_color(pack_rgba_floats_to_u32(r, g, b, 1.0f, 255.0f));
}

// FUNC 0x0025A180 pgl_color4fv
/* The same from a float[4] {r, g, b, a}.  Callers
 * gt2k_menu::LoggerRaceGraph::drawCurve / drawCursor. */
void pgl_color4fv(const f32 *rgba)
{
    pgl_color(pack_rgba_floats_to_u32(rgba[0], rgba[1], rgba[2], rgba[3], 255.0f));
}

/* The file's .sdata, 0x003530E0..0x003530E8 in definition order (bootstrap
 * s_vu1CodeUnscrambled / s_lcgState).  Both explicitly initialised: an
 * uninitialised one would be a common, not .sdata. */
s8  g_003530E0 = 0;                 /* 0x003530E0 the VU1 code is unscrambled */
u32 g_003530E4 = 0x32277070;        /* 0x003530E4 LCG state */

// FUNC 0x0025A1B8 lcg_rand
// state = state * 0x5D588B65 + 1 (32-bit; state gp-0x490C, initially
// 0x32277070); returns the new state.  (bootstrap 0x0100E4A8)
int lcg_rand(void)
{
    /* 64-bit multiply (multu, hi:lo combined, daddiu) truncated back to 32
     * bits: the same low 32 bits as a 32-bit multiply. */
    g_003530E4 = g_003530E4 * 0x5D588B65ull + 1;
    return (s32)g_003530E4;
}

// FUNC 0x0025A1F8 xor_unscramble_with_lcg
// Once only (flag gp-0x4910): XORs len bytes at buf with the low byte of
// successive lcg_rand() values - gfx_init's unscrambling of the VU1
// microcode.  (bootstrap 0x0100E4E8)
void xor_unscramble_with_lcg(u8 *buf, int len)
{
    int i;

    if (g_003530E0 == 0) {
        for (i = 0; i < len; i++) {
            *buf ^= lcg_rand();
            buf++;
        }
        g_003530E0 = 1;
    }
}

/* The file's .data, 0x002FC220..0x002FC24D: the GS register numbers of the
 * shadows in GsContext.gen[] / reg[] order (GS_GEN_*, GS_REG_*), for
 * gs_collect_dirty_regs.  Not const: retail has them in .data (bootstrap
 * gs_state.cpp's s_gsRegs*), after libgraph's .data (u116's comment);
 * nothing writes them.  Each array is 8-aligned (6 / 3 zero bytes after the
 * first two). */
u8 g_gsRegsGeneral[GS_GEN_COUNT] = {        /* 0x002FC220 */
    SCE_GS_COLCLAMP, SCE_GS_DIMX, SCE_GS_DTHE, SCE_GS_FOGCOL, SCE_GS_PABE,
    SCE_GS_PRMODECONT, SCE_GS_SCANMSK, SCE_GS_TEXA, SCE_GS_TEXCLUT, SCE_GS_TEXFLUSH,
};   /* 0x002FC220 */
u8 g_gsRegsCtx1[GS_REG_COUNT] = {           /* 0x002FC230 */
    SCE_GS_FRAME_1, SCE_GS_XYOFFSET_1, SCE_GS_FBA_1, SCE_GS_ZBUF_1, SCE_GS_SCISSOR_1,
    SCE_GS_ALPHA_1, SCE_GS_TEST_1, SCE_GS_CLAMP_1, SCE_GS_TEX2_1, SCE_GS_TEX1_1,
    SCE_GS_TEX0_1, SCE_GS_MIPTBP1_1, SCE_GS_MIPTBP2_1,
};   /* 0x002FC230 */
u8 g_gsRegsCtx2[GS_REG_COUNT] = {           /* 0x002FC240 */
    SCE_GS_FRAME_2, SCE_GS_XYOFFSET_2, SCE_GS_FBA_2, SCE_GS_ZBUF_2, SCE_GS_SCISSOR_2,
    SCE_GS_ALPHA_2, SCE_GS_TEST_2, SCE_GS_CLAMP_2, SCE_GS_TEX2_2, SCE_GS_TEX1_2,
    SCE_GS_TEX0_2, SCE_GS_MIPTBP1_2, SCE_GS_MIPTBP2_2,
};   /* 0x002FC240 */

// FUNC 0x0025A258 gs_emit_masked_regs
// For each bit of mask (LSB first, do-while) emits {shadow[i], regNums[i]}
// as one A+D qword when the bit is set; returns the new output pointer.
// (bootstrap 0x0100EE48)
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

// FUNC 0x0025A290 gs_collect_dirty_regs
// mask = dirty & enable; if 0 returns 0.  Removes mask from dirty, adds it
// to written, emits general regs (bits 32-41), context-1 regs (bits 0-12)
// and context-2 regs (bits 16-28, same shadows) as A+D qwords at out;
// returns the number of qwords.  (bootstrap 0x0100EE80)
int gs_collect_dirty_regs(GsContext *ctx, u64 *out)
{
    u64 dirty = ctx->dirty;
    u64 mask = dirty & ctx->enable;
    u64 *p = out;
    u64 bits;

    if (mask == 0)
        return 0;
    ctx->dirty = dirty & ~mask;
    ctx->written |= mask;

    bits = mask >> 32;                                      /* general registers */
    bits &= (1u << GS_GEN_COUNT) - 1;
    if (bits != 0)
        p = gs_emit_masked_regs(p, bits, g_gsRegsGeneral, ctx->gen);
    bits = mask;                                            /* context 1 */
    bits &= (1u << GS_REG_COUNT) - 1;
    if (bits != 0)
        p = gs_emit_masked_regs(p, bits, g_gsRegsCtx1, ctx->reg);
    bits = mask >> 16;                                      /* context 2, the same shadows */
    bits &= (1u << GS_REG_COUNT) - 1;
    if (bits != 0)
        p = gs_emit_masked_regs(p, bits, g_gsRegsCtx2, ctx->reg);
    return (int)((u8 *)p - (u8 *)out) >> 4;
}

// FUNC 0x0025A360 intc_vblank_handler
// INTC_VBLANK_S handler: reads and zeroes COP0 PCR0/PCR1 and accumulates
// them into perfCount0/1, samples GS CSR.FIELD (bit 13), increments the
// 64-bit vblank counter, calls vsyncCallback(count) if set, then
// iWakeupThread for every thread in the vsync waiter list and in the three
// DMA channel waiter lists; sync.l; ei; returns 1.  (bootstrap 0x0100E630)
int intc_vblank_handler(int cause)
{
    GfxDriverState *g = &g_gfx;
    GfxVsyncFunc cb;
    u64 pc0, pc1;
    int i;

    __asm__ __volatile__(
        "mfpc   %0, 0\n\t"
        "mtpc   $0, 0\n\t"
        "sync.p\n\t"
        "mfpc   %1, 1\n\t"
        "mtpc   $0, 1\n\t"
        "sync.p\n\t"
        : "=&r"(pc0), "=&r"(pc1) : : "memory");

    cb = g->vsyncCallback;
    g->perfCount0 += pc0;
    g->perfCount1 += pc1;
    g->field = (u8)((GS_REG_CSR >> GS_CSR_FIELD) & 1);
    g_gfx.vblankCount++;
    if (cb != NULL)
        cb((s32)g_gfx.vblankCount);

    GS_WAKE_WAITERS(g->vsyncWaiters);     /* implicit: iWakeupThread (macro) */
    for (i = GFX_CH_VIF0; i <= GFX_CH_GIF; i++)
        GS_WAKE_WAITERS(g->chan[i].waiters);

    ExitHandler();
    return 1;
}

// FUNC 0x0025A4A0 intc_vif1_handler
// INTC_VIF1 handler (VIF1 i-bit stall).  VIF1_STAT bit 11 (INT) clear: just
// FBRST.STC.  Otherwise the NUM field of VIF1_CODE carries the
// gfx_put_irq_marker code (PGL_MARK_*):
//   READBACK: --pathSync18, resume (STC) unless it went negative (the GIF
//     list has not reached its READBACK yet);
//   else WAIT: --pathSync16, resume unless negative; no marker: resume;
//   then SIGNAL: ++pathSync17 and when it reaches 0 (the GIF list waits)
//     restart GIF DMA.
// Each exit: sync.l; ei; return 1.  (bootstrap 0x0100E770)
int intc_vif1_handler(int cause)
{
    GfxDriverState *g = &g_gfx;
    u32 code;

    if (((VIF1_STAT >> VIF1_STAT_INT) & 1) == 0) {
        GS_VIF1_RESUME();
        ExitHandler();
        return 1;
    }
    code = VIF1_CODE;
    if (code & PGL_MARK_READBACK) {
        g->pathSync18--;
        if (g->pathSync18 >= 0)
            GS_VIF1_RESUME();
        ExitHandler();
        return 1;
    }
    if (code & PGL_MARK_WAIT) {
        g->pathSync16--;
        if (g->pathSync16 >= 0)
            GS_VIF1_RESUME();
    } else {
        GS_VIF1_RESUME();
    }
    if (code & PGL_MARK_SIGNAL) {
        g->pathSync17++;
        if ((u8)g->pathSync17 == 0)
            GS_GIF_RESTART();
    }
    ExitHandler();
    return 1;
}

// FUNC 0x0025A5B8 intc_gs_finish_handler
// INTC_GS handler: clears CSR.FINISH, ++finishCount; the second event of a
// readback (with the VIF1 DMA marker) starts gs_readback_begin.
// (bootstrap 0x0100E888)
int intc_gs_finish_handler(int cause)
{
    GfxDriverState *g = &g_gfx;

    GS_REG_CSR = GS_CSR_FINISH;
    g->finishCount++;
    if (g->finishCount == 2)
        gs_readback_begin(g);
    ExitHandler();
    return 1;
}

// FUNC 0x0025A618 dmac_channel_handler
// DMAC handler for channels 0-2.  In chain mode (CHCR.MOD == 1):
//   tag ID 7 (end), or 6 (ret) with ASP != 0 -> completion;
//   last tag without IRQ bit (CHCR bit 31 clear) -> tagIrq = 1, completion;
//   IRQ-flagged tag: ch 1 (VIF1) -> ++finishCount, == 2 starts
//     gs_readback_begin (the transfer header of a download has passed);
//     ch 2 (GIF) -> the gfx_put_irq_marker code in the tag (PGL_MARK_*):
//     READBACK: ++pathSync18 == 0 resumes VIF1 (STC; the GIF list is not
//     restarted); otherwise WAIT: --pathSync17, restart GIF unless negative
//     (no WAIT: restart GIF); then SIGNAL: ++pathSync16 == 0 resumes VIF1;
//     ch 0 -> nothing.  No completion for IRQ-flagged tags.
// Normal mode: if finishCount == 2 the readback is over -> finishCount = 0,
//   gs_readback_end; else completion.
// Completion: busy = 0 and iWakeupThread every waiter of the channel.
// (bootstrap 0x0100E8E8)
int dmac_channel_handler(int ch)
{
    GfxDriverState *g = &g_gfx;
    u32 chcr = g_gsDmaChan[ch]->chcr;

    if (D_CHCR_MOD_GET(chcr) == 1) {                        /* chain mode */
        switch (D_CHCR_TAG_ID(chcr)) {
        case DMATAG_ID_END:
            goto complete;
        case DMATAG_ID_RET:
            if (D_CHCR_ASP_GET(chcr) != 0)
                goto complete;
            break;
        }
        if ((s32)chcr >= 0) {                               /* no D_CHCR_TAG_IRQ */
            g->chan[ch].tagIrq = 1;
            goto complete;
        }
        switch (ch) {
        case GFX_CH_VIF1:
            g->finishCount++;
            if (g->finishCount == 2)
                gs_readback_begin(g);
            break;
        case GFX_CH_GIF:
            if (chcr & PGL_MARK_READBACK) {
                g->pathSync18++;
                if ((u8)g->pathSync18 == 0)
                    GS_VIF1_RESUME();
            } else {
                if (!(chcr & PGL_MARK_WAIT) || (--g->pathSync17, g->pathSync17 >= 0))
                    GS_GIF_RESTART();
                if (chcr & PGL_MARK_SIGNAL) {
                    g->pathSync16++;
                    if ((u8)g->pathSync16 == 0)
                        GS_VIF1_RESUME();
                }
            }
            break;
        }
        ExitHandler();
        return 1;
    }

    if (g->finishCount == 2) {
        g->finishCount = 0;
        gs_readback_end(g);
        ExitHandler();
        return 1;
    }

complete:
    g->chan[ch].busy = 0;
    GS_WAKE_WAITERS(g->chan[ch].waiters);   /* implicit: iWakeupThread (macro) */
    ExitHandler();
    return 1;
}

// FUNC 0x0025A830 gs_readback_begin
// Starts a GS->EE image download: the GsReadbackRec at D1_TADR (read
// through the uncached segment) gives qwc and destination; remembered in
// g->readback.
// VIF1_STAT.FDR = 1, GS BUSDIR = 1, D1 QWC/MADR set and CHCR = (CHCR &
// 0xFFFFFEF0) | 0x100 (to memory, normal, start); sync.l.
// (bootstrap 0x0100EB00)
void gs_readback_begin(GfxDriverState *g)
{
    u32 chcr = D1->chcr;
    GsReadbackRec *rec = (GsReadbackRec *)((D1->tadr & 0x0FFFFFFF) | 0x20000000); /* uncached */
    u16 qwc = rec->h.qwc;
    u32 madr;

    g->readback = rec;
    madr = rec->madr;
    VIF1_STAT = VIF1_STAT_FDR;
    GS_REG_BUSDIR = GS_BUSDIR_TO_HOST;
    D1->qwc  = qwc;
    D1->madr = madr;
    D1->chcr = (chcr & D_CHCR_TO_NORMAL) | D_CHCR_STR;
    SYNC_L();
}

// FUNC 0x0025A8A8 gs_readback_end
// Finishes the download: record callback(madr, arg) if set, BUSDIR = 0,
// VIF1_STAT = 0, then resumes the VIF1 source chain after the record
// (QWC 0, TADR = rec + 0x10, CHCR = (CHCR & 0xFEF0) | 0x10000105) and
// restarts GIF DMA.  (bootstrap 0x0100EB78)
void gs_readback_end(GfxDriverState *g)
{
    GsReadbackRec *rec = g->readback;
    u32 chcr, next;

    if (rec->callback != NULL)
        rec->callback(rec->madr, rec->arg);
    next = ((u32)rec & 0x0FFFFFFF) + sizeof(GsReadbackRec); /* the tag after the record */
    chcr = D1->chcr;
    GS_REG_BUSDIR = GS_BUSDIR_TO_GS;
    VIF1_STAT = 0;
    D1->qwc  = 0;
    D1->tadr = next;
    chcr &= D_CHCR_KEEP_ASP_TIE;
    chcr |= D_CHCR_START_CHAIN;
    D1->chcr = chcr;
    SYNC_L();
    GS_GIF_RESTART();
}

// FUNC 0x0025A950 dma_start_source_chain
// gfx_dma_sync(ch), busy = 1, QWC = 0, TADR = tadr (0x0FFFFFFF-masked, SPR
// bit for 0x7xxxxxxx), CHCR = (CHCR & 0xFEC0) | 0x10000105; sync.l.
// (bootstrap 0x0100EC20)
void dma_start_source_chain(int ch, const void *tadr)
{
    EeDmaChanRegs *d = g_gsDmaChan[ch];

    gfx_dma_sync(ch);
    g_gfx.chan[ch].busy = 1;
    d->qwc = 0;
    d->tadr = gs_dma_src_addr((u32)tadr);
    d->chcr = (d->chcr & D_CHCR_KEEP_TIE) | D_CHCR_START_CHAIN;
    SYNC_L();
}

// FUNC 0x0025AA00 gs_select_vu1_prog
// The VU1 program of the primitives (GsContext.vu1Prog, the MSCAL address
// u114 / u115 send): 0x33E with PGL_CAP_VU1_PROG_33E (vu1ProgAlt), else 0x320;
// with face culling on (cullEnable, PGL_CAP_CULL_FACE) the variant for
// the faces glCullFace (cullFace) and glFrontFace (frontFace) remove:
// +0xA culls the clockwise faces (BACK with CCW front faces, FRONT with
// CW), +0x14 the counter-clockwise ones; FRONT_AND_BACK gives 0 (nothing is
// drawn).  The bootstrap's copy is named gs_compute_display_period (the same
// code; its fields were named displayPeriod / vu1ProgAlt / cullEnable).  Called
// by gfx_enable / gfx_disable and the cull-mode setters (u116).
// (bootstrap 0x0100ECD0)
void gs_select_vu1_prog(GsContext *ctx)
{
    u32 prog = ctx->vu1ProgAlt ? VU1_PROG_BASE_33E : VU1_PROG_BASE;

    if (ctx->cullEnable != 0) {                           /* PGL_CAP_CULL_FACE */
        switch (ctx->cullFace) {
        case PGL_CULL_FRONT:
            prog += (ctx->frontFace == PGL_FRONT_CCW) ? VU1_PROG_CULL_CCW : VU1_PROG_CULL_CW;
            break;
        case PGL_CULL_BACK:
            prog += (ctx->frontFace == PGL_FRONT_CCW) ? VU1_PROG_CULL_CW : VU1_PROG_CULL_CCW;
            break;
        case PGL_CULL_FRONT_AND_BACK:
            prog = 0;
            break;
        default:
            break;
        }
    }
    ctx->vu1Prog = prog;
}

// FUNC 0x0025AA88 gs_build_image_readback_packet
/* GIFtag A+D x5 (EOP): BITBLTBUF {srcBitblt[0] (SBP/SBW/SPSM), 0},
 * TRXPOS {sx | sy << 16, 0}, TRXREG {w, h}, FINISH 0, TRXDIR 1 (local ->
 * host).  Returns the image size in qwords by SPSM (byte 3 of srcBitblt;
 * jump table @0x00345690, 50 entries): CT32 / Z32 *4, CT24 / Z24 *3,
 * CT16 / CT16S *2 bytes per pixel, T8 / T8H (n + 15) >> 4, T4 / T4HL /
 * T4HH (n + 31) >> 5 (signed shifts); anything else 0 (Z16 / Z16S are
 * past the table's 0x32 entries).  Caller gs_download_image. */
int gs_build_image_readback_packet(u64 *pkt, const u32 *srcBitblt,
                                   int sx, int sy, int w, int h)
{
    u32 psm = ((const u8 *)srcBitblt)[3];                   /* BITBLTBUF.SPSM */
    int nPixels = w * h;
    int qwc;

    switch (psm) {
    case SCE_GS_PSMCT32:
    case SCE_GS_PSMZ32:
        qwc = (nPixels * 4 + 0xF) >> 4;
        break;
    case SCE_GS_PSMCT24:
    case SCE_GS_PSMZ24:
        qwc = (nPixels * 3 + 0xF) >> 4;
        break;
    case SCE_GS_PSMCT16:
    case SCE_GS_PSMCT16S:
        qwc = (nPixels * 2 + 0xF) >> 4;
        break;
    case SCE_GS_PSMT8:
    case SCE_GS_PSMT8H:
        qwc = (nPixels + 0xF) >> 4;
        break;
    case SCE_GS_PSMT4:
    case SCE_GS_PSMT4HL:
    case SCE_GS_PSMT4HH:
        qwc = (nPixels + 0x1F) >> 5;
        break;
    default:                                            /* (PSMZ16 / PSMZ16S too) */
        qwc = 0;
        break;
    }
    /* one statement block per qword (scheduling blocks in the binary) */
    do {                                                /* GIFtag A+D x5, EOP */
        u64 *q = pkt + 0;

        q[0] = GIF_NREG(1) | GIF_EOP | 5;
        q[1] = GIF_REG_AD;
    } while (0);
    do {                                                /* SBP SBW SPSM, no destination */
        u64 *q = pkt + 2;

        ((u32 *)q)[0] = srcBitblt[0];
        ((u32 *)q)[1] = 0;
        q[1] = SCE_GS_BITBLTBUF;
    } while (0);
    do {                                                /* SSAX, SSAY */
        u64 *q = pkt + 4;

        ((u32 *)q)[0] = (sy << 16) | sx;
        ((u32 *)q)[1] = 0;
        q[1] = SCE_GS_TRXPOS;
    } while (0);
    do {
        u64 *q = pkt + 6;

        ((u32 *)q)[0] = w;
        ((u32 *)q)[1] = h;
        q[1] = SCE_GS_TRXREG;
    } while (0);
    do {                                                /* the FINISH event of gs_readback_begin */
        u64 *q = pkt + 8;

        q[0] = 0;
        q[1] = SCE_GS_FINISH;
    } while (0);
    do {                                                /* TRXDIR local -> host */
        u64 *q = pkt + 10;

        q[0] = 1;
        q[1] = SCE_GS_TRXDIR;
    } while (0);

    return qwc;
}

// FUNC 0x0025AB80 gfx_alloc_direct
/* Reserves qwc qwords at ctx->cur behind a DMAtag cnt {qwc} + VIF1 {NOP,
 * DIRECTHL qwc} and returns the data pointer (cur + 0x10).  qwc == 0: nothing
 * written, cur not advanced, cur + 0x10 returned anyway.
 * Callers pfx_build_curve_clut, pfx_build_identity_clut32. */
u32 *gfx_alloc_direct(s32 qwc)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p = ctx->cur;

    if (qwc != 0) {
        p[0] = DMATAG_CNT(qwc);
        p[1] = 0;
        p[2] = VIF_NOP;
        p[3] = VIF_DIRECTHL(qwc);
        ctx->cur = (u32 *)((u8 *)p + ((qwc << 4) + 0x10));
    }
    return p + 4;
}
