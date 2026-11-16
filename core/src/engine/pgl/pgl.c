/* pgl.c - GT3 main executable, unit u117, 0x00254828..0x00257A44:
 * the end of the graphics library's C part (see pgl_internal.h).
 *
 * CLUT / TEX0 setters, image transfers, the pgl matrix mode / stack and
 * attribute stack, frustum / ortho, packet helpers, light and light-model
 * setters, the 2D primitives (flat / textured sprites, lines), 3D lines
 * through the VU0 combined matrix with homogeneous clipping, the sprite Z/F
 * and the CPU lighting fallback (__pglShading).
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/pgl_internal.h"

/* ======================================================================== */
/* CLUT / TEX0                                                              */
/* ======================================================================== */

/* TEX0.TCC fix-up after a CLUT change: with a CT24 palette (clutPsm 1, sent
 * as CPSM CT32) the indexed formats T8 / T4 / T8H / T4HL / T4HH get TCC 0
 * (RGB only), every other case TCC 1.  Callers: gs_set_clut and
 * gfx_set_texture_buffer. */
// FUNC 0x00254828 gs_fix_tex0_tcc
void gs_fix_tex0_tcc(GsContext *ctx)
{
    u64 tex0;

    if (ctx->clutPsm != GS_CLUT_CT24) {
        ctx->reg[GS_REG_TEX0] |= GS_FIELD_MASK(GS_TEX0_TCC);
        return;
    }
    tex0 = ctx->reg[GS_REG_TEX0];
    switch ((s32)GS_FIELD_GET(tex0, GS_TEX0_PSM)) {
    case SCE_GS_PSMT8:
    case SCE_GS_PSMT4:
    case SCE_GS_PSMT8H:
    case SCE_GS_PSMT4HL:
    case SCE_GS_PSMT4HH:
        ctx->reg[GS_REG_TEX0] = tex0 & ~GS_FIELD_MASK(GS_TEX0_TCC);      /* RGB */
        return;
    default:
        ctx->reg[GS_REG_TEX0] = tex0 | GS_FIELD_MASK(GS_TEX0_TCC);       /* RGBA */
        return;
    }
}

/* Sets the CLUT of the shadowed TEX0 and TEX2: CBP = cbpWordAddr >> 6,
 * CPSM = cpsm (CT24 = 1 is sent as CT32 = 0), CSM 0, CSA 0 (TEX0 only),
 * CLD 0 in TEX0 and 1 in TEX2; then the TCC fix-up.  Only TEX0 is marked
 * dirty (both contexts).  Caller: RacePS2Base::drawAfterimage.
 * (Matching, 78.5%: retail sets the fields one by one (an and / or per
 * field on both registers, no folded masks: sceGsTex0-style bitfields),
 * the format store first and the rest in a statement macro (retail's sb
 * is scheduled ahead of the loads).  Left: cbpWordAddr >> 6 and the
 * CT24 test land in other registers / slots; 56 + 120 + 6 forms of the
 * format test, the field order and the macro's extent tried.) */
// FUNC 0x002548C0 gs_set_clut
void gs_set_clut(u32 cbpWordAddr, int cpsm)
{
    GsContext *ctx = GS_SPR_CTX;
    GsTex0Bits *tex0 = GS_TEX0_BITS(ctx, GS_REG_TEX0);
    GsTex0Bits *tex2 = GS_TEX0_BITS(ctx, GS_REG_TEX2);

    ctx->clutPsm = cpsm;
    do {
        if (cpsm == GS_CLUT_CT24)
            cpsm = SCE_GS_PSMCT32;
        tex0->CBP = cbpWordAddr >> 6;
        tex2->CBP = cbpWordAddr >> 6;
        tex0->CPSM = cpsm;
        tex2->CPSM = cpsm;
        tex0->CSM = 0;
        tex2->CSM = 0;
        tex0->CSA = 0;
        tex0->CLD = GS_CLD_KEEP;
        tex2->CLD = GS_CLD_LOAD;
    } while (0);
    gs_fix_tex0_tcc(ctx);
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
}

/* TEX0.CSA = csa (CLUT entry offset).  Caller: RacePS2Base::drawAfterimage. */
// FUNC 0x002549E0 gs_set_clut_csa
void gs_set_clut_csa(int csa)
{
    GsContext *ctx = GS_SPR_CTX;

    GS_TEX0_BITS(ctx, GS_REG_TEX0)->CSA = csa;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
}

/* ======================================================================== */
/* Image transfers                                                          */
/* ======================================================================== */

/* Same machine code as the bootstrap's gs_build_image_transfer_packet
 * (0x0100E0A0, src/gs_driver.cpp).
 * Writes the 6-qword host->local transfer header at pkt: GIFtag (A+D x4)
 * BITBLTBUF = dstBitblt[0] << 32 (DBP/DBW/DPSM), TRXPOS = (dy << 16 | dx) << 32,
 * TRXREG = w | h << 32, TRXDIR = 0, then GIFtag IMAGE EOP with the qword
 * count.  qwc = w*h scaled by the DPSM (byte 3 of dstBitblt[0]): CT32 *4,
 * CT24 *3, CT16/16S *2 -> (+15)>>4; T8/T8H (+15)>>4; T4/T4HH/T4HL (+31)>>5;
 * any other PSM: qwc = w*h unscaled (quirk).  Returns qwc.  Callers:
 * tex1_upload_packet, tex1_upload_to_vram, gs_upload_image(_sync). */
// FUNC 0x00254A30 gs_build_image_transfer_packet
int gs_build_image_transfer_packet(u32 *pkt, const u32 *dstBitblt,
                                   int dx, int dy, int w, int h)
{
    u32 dpsm = ((const u8 *)dstBitblt)[3];
    s32 qwc = w * h;                            /* pixels, then qwords */

    if (dpsm < 0x2D) {                          /* the jump table ends at PSMT4HH */
        switch (dpsm) {
        case SCE_GS_PSMCT32:
            qwc = ((qwc << 2) + 0xF) >> 4;
            break;
        case SCE_GS_PSMCT24:
            qwc = ((qwc << 1) + qwc + 0xF) >> 4;
            break;
        case SCE_GS_PSMCT16:
        case SCE_GS_PSMCT16S:
            qwc = ((qwc << 1) + 0xF) >> 4;
            break;
        case SCE_GS_PSMT8:
        case SCE_GS_PSMT8H:
            qwc = (qwc + 0xF) >> 4;
            break;
        case SCE_GS_PSMT4:
        case SCE_GS_PSMT4HH:
        case SCE_GS_PSMT4HL:
            qwc = (qwc + 0x1F) >> 5;
            break;
        default:
            break;
        }
    }

    do {
        PKT64U(pkt, 0) = GIF_NREG(1) | 4;                   /* NLOOP 4, PACKED */
        PKT64U(pkt, 2) = GIF_REG_AD;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 4);
        q[0].w[0] = 0;
        q[0].w[1] = dstBitblt[0];                           /* DBP/DBW/DPSM */
        q[1].d = SCE_GS_BITBLTBUF;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 8);
        q[0].w[0] = 0;
        q[0].w[1] = ((u32)dy << 16) | (u32)dx;              /* DSAY/DSAX */
        q[1].d = SCE_GS_TRXPOS;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 12);
        q[0].w[0] = (u32)w;
        q[0].w[1] = (u32)h;
        q[1].d = SCE_GS_TRXREG;
    } while (0);
    do {
        PKT64U(pkt, 16) = GS_TRXDIR_HOST_TO_LOCAL;
        PKT64U(pkt, 18) = SCE_GS_TRXDIR;
    } while (0);
    do {
        PKT64U(pkt, 20) = (u64)(s64)(s32)((u32)qwc | GIF_EOP) | GIF_FLG_IMAGE;
        PKT64U(pkt, 22) = 0;
    } while (0);
    return qwc;
}

/* Queues an EE -> GS image upload in the packet (list 0):
 * cnt(6) + FLUSH + DIRECTHL 6 with the transfer header, then a DMA ref tag
 * to the image (qwc qwords at src) with DIRECTHL qwc.
 * BITBLTBUF word: DBP = fbWordAddr >> 6, DBW = fbw >> 6, DPSM = psm.
 * Callers: 7 (texture uploads: MpegImage::drawIpic, GSBuffer, the fonts ...).
 * (Matching, 77.8%: retail stores the BITBLTBUF word before it loads cur
 * (here: the word through a union, alias set 0).  Left: the header's
 * constants get other registers (retail DIRECTHL $a1, CNT $v1), which
 * moves the cnt store; 960 forms searched - u32 / array / union word,
 * cur as pointer or as GS_CUR_WORD (gs_download_image's exact form), all
 * 120 orders of the five header statements.) */
// FUNC 0x00254B28 gs_upload_image
void gs_upload_image(u32 fbWordAddr, int psm, int fbw, int dx, int dy,
                     int w, int h, const void *src)
{
    GsContext *ctx = GS_SPR_CTX;
    union { u32 w; u8 b[4]; } bitblt;          /* sp+0x00 (DPSM read as byte 3) */
    u32 *hdr, *ref;
    int qwc;

    bitblt.w = gs_bitblt_word(fbWordAddr, psm, fbw);
    hdr = ctx->cur;
    hdr[3] = VIF_DIRECTHL(GS_XFER_HDR_QWC);
    hdr[2] = VIF_FLUSH;
    hdr[0] = DMATAG_CNT(GS_XFER_HDR_QWC);
    ctx->cur = hdr + 4 + GS_XFER_HDR_QWC * 4;
    hdr[1] = 0;
    qwc = gs_build_image_transfer_packet(hdr + 4, &bitblt.w, dx, dy, w, h);
    ref = ctx->cur;
    ref[1] = DMA_ADDR(src);
    ctx->cur = ref + 4;
    ref[0] = DMATAG_REF(qwc);
    ref[3] = VIF_DIRECTHL(qwc);
    ref[2] = 0;
}

/* The same upload done at once on the GIF channel: header built on the
 * stack, D-cache written back, gfx_dma_send of header and image, waits.
 * Caller: TexSet::loadImage. */
// FUNC 0x00254C00 gs_upload_image_sync
void gs_upload_image_sync(u32 fbWordAddr, int psm, int fbw, int dx, int dy,
                          int w, int h, const void *src)
{
    u32 hdr[GS_XFER_HDR_QWC * 4] __attribute__((aligned(16)));  /* sp+0x00 */
    u32 bitblt[4];                                              /* sp+0x60 */
    int qwc;

    bitblt[0] = gs_bitblt_word(fbWordAddr, psm, fbw);
    qwc = gs_build_image_transfer_packet(hdr, bitblt, dx, dy, w, h);
    SYNC_L();
    FlushCache(WRITEBACK_DCACHE);
    gfx_dma_lock(GFX_CH_GIF);
    gfx_dma_send(GFX_CH_GIF, hdr, GS_XFER_HDR_QWC);
    gfx_dma_send(GFX_CH_GIF, src, qwc);
    gfx_dma_sync(GFX_CH_GIF);
    gfx_dma_unlock(GFX_CH_GIF);
}

/* Queues a GS -> EE download (local -> host transfer) in the packet:
 * PGL_MARK_READBACK on the GIF list and then on the VIF1 list (the GIF
 * list stops there, the VIF1 list waits until it has), then on list 0:
 * cnt(6) with IRQ + FLUSHA + DIRECTHL 6 with the transfer header
 * (gs_build_image_readback_packet), and the readback record of u118's
 * handlers (GsReadbackRec: a cnt tag word with IRQ and the image qwc, the
 * dst address - with the SPR bit for a scratchpad pointer -,
 * readbackCallback, readbackArg; gs_readback_begin starts the transfer to
 * it, gs_readback_end calls the callback and resumes both lists).  The
 * previous list is selected again.  Callers: GSBuffer::download,
 * flare_readback.
 * (Matching: one BITBLTBUF word (sp+0, the list number spilled at sp+4),
 * cur as an integer (GS_CUR_WORD: retail stores the word before it loads
 * cur), the destination copied once into a local (retail: the dst
 * parameter's register, modified in place, is allocated last).) */
// FUNC 0x00254CB0 gs_download_image
void gs_download_image(u32 fbWordAddr, int psm, int fbw, int sx, int sy,
                       int w, int h, void *dst)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 bitblt;
    u32 *p;
    u32 dstAddr, seg;
    int savedList;
    int qwc;

    savedList = ctx->curList;                               /* sp+0x04 */
    gfx_select_list(PGL_LIST_GIF);
    gfx_put_irq_marker(PGL_MARK_READBACK);
    gfx_select_list(PGL_LIST_VIF1);
    gfx_put_irq_marker(PGL_MARK_READBACK);
    bitblt = gs_bitblt_word(fbWordAddr, psm, fbw);
    p = (u32 *)GS_CUR_WORD(ctx);
    p[2] = VIF_FLUSHA;
    p[0] = DMATAG_IRQ | DMATAG_CNT(GS_XFER_HDR_QWC);
    p[3] = VIF_DIRECTHL(GS_XFER_HDR_QWC);
    p[1] = 0;
    p += 4;
    qwc = gs_build_image_readback_packet((u64 *)p, &bitblt, sx, sy, w, h);
    p += GS_XFER_HDR_QWC * 4;
    p[0] = qwc | (DMATAG_IRQ | DMATAG_CNT(0));              /* GsReadbackRec */
    dstAddr = (u32)dst;
    seg = dstAddr & 0x70000000;
    dstAddr = DMA_ADDR(dstAddr);
    if (seg == 0x70000000)
        dstAddr |= DMA_SPR;                                 /* scratchpad destination */
    p[1] = dstAddr;
    p[2] = (u32)ctx->readbackCallback;
    p[3] = ctx->readbackArg;
    p += 4;
    GS_CUR_WORD(ctx) = (u32)p;
    gfx_select_list(savedList);
}

/* ======================================================================== */
/* Matrix mode / stack (VU0)                                                */
/* ======================================================================== */

/* Stores the current matrix vf28..vf31 to VU0 data memory at qword qaddr. */
#define VU0_STORE_CUR_MATRIX(qaddr)                                         \
    __asm__ __volatile__(                                                   \
        "ctc2.i     %0, $vi10\n\t"                                          \
        "vsqi.xyzw  $vf28, ($vi10++)\n\t"                                   \
        "vsqi.xyzw  $vf29, ($vi10++)\n\t"                                   \
        "vsqi.xyzw  $vf30, ($vi10++)\n\t"                                   \
        "vsqi.xyzw  $vf31, ($vi10++)\n\t"                                   \
        "vnop\n\t"                                                          \
        "vnop\n\t"                                                          \
        : : "r"(qaddr) : "memory")

/* Loads vf28..vf31 from VU0 data memory at qword qaddr. */
#define VU0_LOAD_CUR_MATRIX(qaddr)                                          \
    __asm__ __volatile__(                                                   \
        "ctc2.i     %0, $vi10\n\t"                                          \
        "vlqi.xyzw  $vf28, ($vi10++)\n\t"                                   \
        "vlqi.xyzw  $vf29, ($vi10++)\n\t"                                   \
        "vlqi.xyzw  $vf30, ($vi10++)\n\t"                                   \
        "vlqi.xyzw  $vf31, ($vi10++)\n\t"                                   \
        "vnop\n\t"                                                          \
        "vnop\n\t"                                                          \
        : : "r"(qaddr) : "memory")

/* glMatrixMode (PGL_MTX_*): parks the current matrix (vf28..vf31) of the
 * old mode in VU0 data memory (mode m at qword 4 + 4m) and loads the new
 * mode's.  Modes other than 0..2 neither store nor load.  35 callers. */
// FUNC 0x00254E20 pgl_matrix_mode
void pgl_matrix_mode(int mode)
{
    GsContext *ctx = GS_SPR_CTX;

    if (ctx->matrixMode == mode)
        return;
    __asm__ __volatile__("vnop");
    switch (ctx->matrixMode) {
    case PGL_MTX_MODELVIEW:
        VU0_STORE_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_MODELVIEW));
        break;
    case PGL_MTX_PROJECTION:
        VU0_STORE_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_PROJECTION));
        break;
    case PGL_MTX_TEXTURE:
        VU0_STORE_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_TEXTURE));
        break;
    }
    ctx->matrixMode = mode;
    switch ((u32)mode) {                    /* a GLenum: unsigned */
    case PGL_MTX_MODELVIEW:
        VU0_LOAD_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_MODELVIEW));
        return;
    case PGL_MTX_PROJECTION:
        VU0_LOAD_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_PROJECTION));
        return;
    case PGL_MTX_TEXTURE:
        VU0_LOAD_CUR_MATRIX(VU0_MATRIX_QW(PGL_MTX_TEXTURE));
        return;
    }
}

/* The matrix stacks' bound checks are compiled out in the original: their
 * messages are in .rodata (0x003453E8..0x003454DC; egcs emits the strings of
 * dead code), the code is not.  The name is the original's (pglPushMatrix,
 * pglPopMatrix: the %s, __FUNCTION__ in the original). */
#define PGL_STACK_CHECK_ON  0
#define PGL_STACK_ERROR(msg, name)     if (PGL_STACK_CHECK_ON) printf(msg, name)

/* glPushMatrix: vf28..vf31 to the VU0 stack of the mode (vi11 / vi12 /
 * vi13, post-increment).  92 callers. */
// FUNC 0x00254F68 pgl_push_matrix
void pgl_push_matrix(void)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (ctx->matrixMode) {
    case PGL_MTX_MODELVIEW:
        PGL_STACK_ERROR("%s: Stack overflow. (MODELVIEW)\n", "pglPushMatrix");
        __asm__ __volatile__(
            "vsqi.xyzw $vf28, ($vi11++)\n\t"
            "vsqi.xyzw $vf29, ($vi11++)\n\t"
            "vsqi.xyzw $vf30, ($vi11++)\n\t"
            "vsqi.xyzw $vf31, ($vi11++)\n\t" : : : "memory");
        return;
    case PGL_MTX_PROJECTION:
        PGL_STACK_ERROR("%s: Stack overflow. (PROJECTION)\n", "pglPushMatrix");
        __asm__ __volatile__(
            "vsqi.xyzw $vf28, ($vi12++)\n\t"
            "vsqi.xyzw $vf29, ($vi12++)\n\t"
            "vsqi.xyzw $vf30, ($vi12++)\n\t"
            "vsqi.xyzw $vf31, ($vi12++)\n\t" : : : "memory");
        return;
    case PGL_MTX_TEXTURE:
        PGL_STACK_ERROR("%s: Stack overflow. (TEXTURE)\n", "pglPushMatrix");
        __asm__ __volatile__(
            "vsqi.xyzw $vf28, ($vi13++)\n\t"
            "vsqi.xyzw $vf29, ($vi13++)\n\t"
            "vsqi.xyzw $vf30, ($vi13++)\n\t"
            "vsqi.xyzw $vf31, ($vi13++)\n\t" : : : "memory");
        return;
    }
}

/* glPopMatrix: vf31..vf28 from the mode's VU0 stack (pre-decrement); the
 * matrices are marked changed first.  92 callers. */
// FUNC 0x00254FE8 pgl_pop_matrix
void pgl_pop_matrix(void)
{
    GsContext *ctx = GS_SPR_CTX;
    int mode = ctx->matrixMode;

    ctx->matrixDirty = 1;
    switch (mode) {
    case PGL_MTX_MODELVIEW:
        PGL_STACK_ERROR("%s: Modelview stack underflow.\n", "pglPopMatrix");
        __asm__ __volatile__(
            "vlqd.xyzw $vf31, (--$vi11)\n\t"
            "vlqd.xyzw $vf30, (--$vi11)\n\t"
            "vlqd.xyzw $vf29, (--$vi11)\n\t"
            "vlqd.xyzw $vf28, (--$vi11)\n\t" : : : "memory");
        return;
    case PGL_MTX_PROJECTION:
        PGL_STACK_ERROR("%s: Projection stack underflow.\n", "pglPopMatrix");
        __asm__ __volatile__(
            "vlqd.xyzw $vf31, (--$vi12)\n\t"
            "vlqd.xyzw $vf30, (--$vi12)\n\t"
            "vlqd.xyzw $vf29, (--$vi12)\n\t"
            "vlqd.xyzw $vf28, (--$vi12)\n\t" : : : "memory");
        return;
    case PGL_MTX_TEXTURE:
        PGL_STACK_ERROR("%s: Texture stack underflow.\n", "pglPopMatrix");
        __asm__ __volatile__(
            "vlqd.xyzw $vf31, (--$vi13)\n\t"
            "vlqd.xyzw $vf30, (--$vi13)\n\t"
            "vlqd.xyzw $vf29, (--$vi13)\n\t"
            "vlqd.xyzw $vf28, (--$vi13)\n\t" : : : "memory");
        return;
    }
}

/* ======================================================================== */
/* Attribute stack                                                          */
/* ======================================================================== */

/* glPushAttrib-like: copies the state groups of mask (byte-wise, the stack
 * pointer g_pglAttribSp is not aligned) and then the mask itself:
 *   PGL_ATTR_FOG          fogStart, fogAmount, FOGCOL
 *   PGL_ATTR_SCISSOR      SCISSOR
 *   PGL_ATTR_VIEWPORT     winX..winH, depthNear, depthFar
 *   PGL_ATTR_DEPTH        ZBUF, depthScale, depthTest, zMask
 *   PGL_ATTR_DRAW_BUFFER  XYOFFSET, SCANMSK, fieldShift, halfHeight, tgtA
 *   PGL_ATTR_TARGET_B     tgtB, clearColorB
 * 21 callers.
 * (Matching, 82.0%: each push is a statement macro (retail's 8-aligned
 * block-end labels), the mask an addressable copy of the parameter (its
 * reloads after the copies; retail's first test uses the parameter
 * register).  Left: the stack pointer / mask take $v1 / $a1, retail $a1 /
 * $v1; declaration orders and a temporary pointer per push tried.) */
// FUNC 0x00255070 pgl_push_attrib
void pgl_push_attrib(u32 m)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 mask = m;
    u8 *sp = g_pglAttribSp;

    if (mask & PGL_ATTR_FOG) {
        do { memcpy(sp, &ctx->fogStart, 4); sp += 4; } while (0);
        do { memcpy(sp, &ctx->fogAmount, 4); sp += 4; } while (0);
        do { memcpy(sp, &ctx->gen[GS_GEN_FOGCOL], 8); sp += 8; } while (0);
    }
    if (mask & PGL_ATTR_SCISSOR) {
        do { memcpy(sp, &ctx->reg[GS_REG_SCISSOR], 8); sp += 8; } while (0);
    }
    if (mask & PGL_ATTR_VIEWPORT) {
        do { memcpy(sp, &ctx->winX, 16); sp += 16; } while (0);   /* winX..winH */
        do { memcpy(sp, &ctx->depthNear, 4); sp += 4; } while (0);
        do { memcpy(sp, &ctx->depthFar, 4); sp += 4; } while (0);
    }
    if (mask & PGL_ATTR_DEPTH) {
        do { memcpy(sp, &ctx->reg[GS_REG_ZBUF], 8); sp += 8; } while (0);
        do { memcpy(sp, &ctx->depthScale, 4); sp += 4; } while (0);
        do { memcpy(sp, &ctx->depthTest, 1); sp += 1; } while (0);
        do { memcpy(sp, &ctx->zMask, 1); sp += 1; } while (0);
    }
    if (mask & PGL_ATTR_DRAW_BUFFER) {
        do { memcpy(sp, &ctx->reg[GS_REG_XYOFFSET], 8); sp += 8; } while (0);
        do { memcpy(sp, &ctx->gen[GS_GEN_SCANMSK], 8); sp += 8; } while (0);
        do { memcpy(sp, &ctx->fieldShift, 1); sp += 1; } while (0);
        do { memcpy(sp, &ctx->halfHeight, 1); sp += 1; } while (0);
        do { memcpy(sp, &ctx->tgtA, sizeof(GsRenderTarget)); sp += sizeof(GsRenderTarget); } while (0);
    }
    if (mask & PGL_ATTR_TARGET_B) {
        do { memcpy(sp, &ctx->tgtB, sizeof(GsRenderTarget)); sp += sizeof(GsRenderTarget); } while (0);
        do { memcpy(sp, &ctx->clearColorB, 4); sp += 4; } while (0);
    }
    do { memcpy(sp, &mask, 4); sp += 4; } while (0);
    g_pglAttribSp = sp;
}

/* glPopAttrib-like: pops the mask, then the groups in reverse order, and
 * re-applies them: the draw buffer through gfx_set_draw_buffer (base << 6,
 * psm, width, height) and dirty XYOFFSET + SCANMSK; ZBUF dirty,
 * gs_update_depth_range and the depth test on / off; the viewport through
 * gs_update_scissor_and_viewport + gs_update_depth_range; SCISSOR dirty;
 * the fog through gfx_set_fog_params(fogStart, fogAmount) and FOGCOL dirty.
 * Target B and clearColorB are only restored.  20 callers.
 * (Matching, 94.7%: each pop a statement macro, the bytes read as signed
 * chars through an int (retail lb).  Left: the mask's register ($v1, retail
 * $a0) and the dirty / constant pair of the |= ($v0 / $v1 swapped).) */
// FUNC 0x002552A0 pgl_pop_attrib
void pgl_pop_attrib(void)
{
    GsContext *ctx = GS_SPR_CTX;
    s8 *sp = (s8 *)g_pglAttribSp;
    u32 mask;                                               /* sp+0x00 */

    do { sp -= 4; memcpy(&mask, sp, 4); } while (0);
    if (mask & PGL_ATTR_TARGET_B) {
        do { sp -= 4; memcpy(&ctx->clearColorB, sp, 4); } while (0);
        do { sp -= sizeof(GsRenderTarget); memcpy(&ctx->tgtB, sp, sizeof(GsRenderTarget)); } while (0);
    }
    if (mask & PGL_ATTR_DRAW_BUFFER) {
        do { sp -= sizeof(GsRenderTarget); memcpy(&ctx->tgtA, sp, sizeof(GsRenderTarget)); } while (0);
        gfx_set_draw_buffer((u32)ctx->tgtA.base << 6, ctx->tgtA.psm, ctx->tgtA.width, ctx->tgtA.height);
        do { s32 v_; sp -= 1; v_ = *sp; ctx->halfHeight = v_; } while (0);
        do { s32 v_; sp -= 1; v_ = *sp; ctx->fieldShift = v_; } while (0);
        do { sp -= 8; memcpy(&ctx->gen[GS_GEN_SCANMSK], sp, 8); } while (0);
        do { sp -= 8; memcpy(&ctx->reg[GS_REG_XYOFFSET], sp, 8); } while (0);
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_SCANMSK) | GS_DIRTY_BOTH(GS_REG_XYOFFSET);
    }
    if (mask & PGL_ATTR_DEPTH) {
        do { s32 v_; sp -= 1; v_ = *sp; ctx->zMask = v_; } while (0);
        do { s32 v_; sp -= 1; v_ = *sp; ctx->depthTest = v_; } while (0);
        do { sp -= 4; memcpy(&ctx->depthScale, sp, 4); } while (0);
        do { sp -= 8; memcpy(&ctx->reg[GS_REG_ZBUF], sp, 8); } while (0);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ZBUF);
        gs_update_depth_range(ctx);
        if (ctx->depthTest != 0)
            gfx_enable(PGL_CAP_DEPTH_TEST);
        else
            gfx_disable(PGL_CAP_DEPTH_TEST);
    }
    if (mask & PGL_ATTR_VIEWPORT) {
        do { sp -= 4; memcpy(&ctx->depthFar, sp, 4); } while (0);
        do { sp -= 4; memcpy(&ctx->depthNear, sp, 4); } while (0);
        do { sp -= 16; memcpy(&ctx->winX, sp, 16); } while (0);
        gs_update_scissor_and_viewport(ctx);
        gs_update_depth_range(ctx);
    }
    if (mask & PGL_ATTR_SCISSOR) {
        do { sp -= 8; memcpy(&ctx->reg[GS_REG_SCISSOR], sp, 8); } while (0);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_SCISSOR);
    }
    if (mask & PGL_ATTR_FOG) {
        do { sp -= 8; memcpy(&ctx->gen[GS_GEN_FOGCOL], sp, 8); } while (0);
        do { sp -= 4; memcpy(&ctx->fogAmount, sp, 4); } while (0);
        do { sp -= 4; memcpy(&ctx->fogStart, sp, 4); } while (0);
        gfx_set_fog_params(ctx->fogStart, ctx->fogAmount);
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_FOGCOL);
    }
    g_pglAttribSp = (u8 *)sp;
}

/* ======================================================================== */
/* Projection matrices                                                      */
/* ======================================================================== */

/* glFrustum, multiplied into the current matrix (row-vector layout, the
 * transpose of the OpenGL matrix).  The 2n terms use the 7th argument:
 *   [ 2f'/(r-l)    0            0               0 ]
 *   [ 0            2f'/(t-b)    0               0 ]
 *   [ (r+l)/(r-l)  (t+b)/(t-b)  -(f+n)/(f-n)   -1 ]
 *   [ 0            0            -2fn/(f-n)      0 ]      f' = focal
 * 18 callers (cameras). */
// FUNC 0x002555A8 pgl_frustum
void pgl_frustum(float l, float r, float b, float t, float n, float f,
                 float focal)
{
    f32 m[16] __attribute__((aligned(16)));
    f32 invWidth = 1.0f / (r - l);
    f32 invHeight = 1.0f / (t - b);
    f32 invDepth = 1.0f / (f - n);

    m[0] = 2.0f * focal * invWidth;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 2.0f * focal * invHeight;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = (r + l) * invWidth;
    m[9] = (t + b) * invHeight;
    m[10] = -(f + n) * invDepth;
    m[11] = -1.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = f * (-2.0f * n) * invDepth;
    m[15] = 0.0f;
    pgl_mult_matrix(m);
}

/* glOrtho, multiplied into the current matrix:
 *   [ 2/(r-l)  0  0  0 ] [ 0 2/(t-b) 0 0 ] [ 0 0 -2/(f-n) 0 ]
 *   [ (l+r)/(l-r)  (b+t)/(b-t)  (n+f)/(n-f)  1 ]
 * 10 callers. */
// FUNC 0x00255680 pgl_ortho
void pgl_ortho(float l, float r, float b, float t, float n, float f)
{
    f32 m[16] __attribute__((aligned(16)));

    m[0] = 2.0f / (r - l);
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 2.0f / (t - b);
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = -2.0f / (f - n);
    m[11] = 0.0f;
    m[12] = (l + r) / (l - r);
    m[13] = (b + t) / (b - t);
    m[14] = (n + f) / (n - f);
    m[15] = 1.0f;
    pgl_mult_matrix(m);
}

/* ======================================================================== */
/* Packet helpers                                                           */
/* ======================================================================== */

/* Takes qwc qwords from the end of the packet buffer; returns the new end
 * (the block).  48 callers. */
// FUNC 0x00255760 gfx_alloc_tail
void *gfx_alloc_tail(int qwc)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->end = (u32 *)((u8 *)ctx->end - qwc * 16);
    return ctx->end;
}

/* Opens a DIRECT block of qwc qwords (cnt + FLUSH + DIRECT header, none
 * for qwc 0) and returns where its data go.  Callers: RaceSmoke::draw
 * (0x001A4F28), weather_draw_quad, weather_draw. */
// FUNC 0x00255780 gfx_open_direct
u32 *gfx_open_direct(int qwc)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p = ctx->cur;

    if (qwc != 0) {
        ctx->cur = p + 4 + qwc * 4;
        p[0] = DMATAG_CNT(qwc);
        p[2] = VIF_FLUSH;
        p[3] = VIF_DIRECT(qwc);
        p[1] = 0;
    }
    return p + 4;
}

/* The PRIM attribute bits with bits added.  Caller: RaceSmoke::draw
 * (0x001A4F28). */
// FUNC 0x002557D0 gs_prim_attr_with
u32 gs_prim_attr_with(u32 bits)
{
    GsContext *ctx = GS_SPR_CTX;

    return ctx->primAttr | bits;
}

/* ======================================================================== */
/* Lights                                                                   */
/* ======================================================================== */

/* glLightfv(light, pname, params), light = PGL_CAP_LIGHT0 + i (i < 8): the
 * vector parameters (ambient, diffuse, specular, spot direction, position)
 * go to gs_set_light_param_v (the 16 bytes at params, read unaligned into
 * one quadword), the scalars (spot exponent / cutoff, attenuation) to
 * gs_set_light_param_f (params[0]); pnames 2, 3, 11, 12 are ignored.
 * 7 callers. */
// FUNC 0x002557E8 pgl_lightfv
void pgl_lightfv(int light, u32 pname, const float *params)
{
    GsContext *ctx = GS_SPR_CTX;
    GsVec4Arg v;

    if ((u32)(light - PGL_CAP_LIGHT0) >= 8)
        return;
    switch (pname) {
    case PGL_LIGHT_AMBIENT:
    case PGL_LIGHT_DIFFUSE:
    case PGL_LIGHT_SPECULAR:
    case PGL_LIGHT_SPOT_DIRECTION:
    case PGL_LIGHT_POSITION:
        v = u117_load_u128(params);
        gs_set_light_param_v(ctx, &ctx->lights[light - PGL_CAP_LIGHT0], pname, v);
        break;
    case PGL_LIGHT_SPOT_EXPONENT:
    case PGL_LIGHT_SPOT_CUTOFF:
    case PGL_LIGHT_CONST_ATTEN:
    case PGL_LIGHT_LINEAR_ATTEN:
    case PGL_LIGHT_QUAD_ATTEN:
        gs_set_light_param_f(ctx, &ctx->lights[light - PGL_CAP_LIGHT0], pname, params[0]);
        break;
    }
}

/* glLightModelfv: AMBIENT the global ambient (4 floats), LOCAL_VIEWER
 * (lightFlags 0x40 = params[0] != 0), 2 ignored, EYE the viewer position
 * eyeWorld (4 floats).  6 callers. */
// FUNC 0x00255898 pgl_light_modelfv
void pgl_light_modelfv(int pname, const float *params)
{
    GsContext *ctx = GS_SPR_CTX;

    switch ((u32)pname) {                   /* a GLenum: unsigned */
    case PGL_LIGHTMODEL_AMBIENT:
        ctx->globalAmbient.x = params[0];
        ctx->globalAmbient.y = params[1];
        ctx->globalAmbient.z = params[2];
        ctx->globalAmbient.w = params[3];
        break;
    case PGL_LIGHTMODEL_LOCAL_VIEWER:
        if (params[0] != 0.0f)
            ctx->lightFlags |= PGL_LF_LOCAL_VIEWER;
        else
            ctx->lightFlags &= ~PGL_LF_LOCAL_VIEWER;
        break;
    case 2:
        break;
    case PGL_LIGHTMODEL_EYE:
        ctx->eyeWorld.x = params[0];
        ctx->eyeWorld.y = params[1];
        ctx->eyeWorld.z = params[2];
        ctx->eyeWorld.w = params[3];
        break;
    }
}

/* Same machine code as the bootstrap's float_to_gs_param_encode
 * (0x0100E198, src/gs_driver.cpp).
 * Encodes a non-negative float (a specular / spot exponent) for the
 * lighting code: 11 greedy bits of v in steps 64, 32, ..., 1/16
 * (bits = integer part << 4 | 4 fraction bits), top = position (10..0) of
 * the highest set step (0 if none).  Returns (bits >> 4) | bitrev4(bits &
 * 0xF) << 6 | top << 16.  float_pow_encoded decodes this format.
 * Callers: gs_set_material_param, gs_set_light_param_f, the init. */
// FUNC 0x00255940 float_to_gs_param_encode
u32 float_to_gs_param_encode(float v)
{
    s32 bits = 0;
    s32 top = 0;
    f32 step = 64.0f;
    s32 i;
    s32 frac, fracRev;

    for (i = 10; i >= 0; i--) {
        bits <<= 1;
        if (step <= v) {                    /* c.le.s */
            v -= step;
            if (top < i)
                top = i;
            bits |= 1;
        }
        step *= 0.5f;
    }
    frac = bits & 0xF;
    bits >>= 4;
    fracRev = 0;
    for (i = 0; i < 4; i++) {
        fracRev <<= 1;
        fracRev |= (frac >> i) & 1;
    }
    bits |= fracRev << 6;
    return bits | (top << 16);
}

/* Same machine code as the bootstrap's float_pow_encoded (0x0100E228).
 * x ** e with e in float_to_gs_param_encode's format: for top = enc >> 16
 * >= 4 square-and-multiply over the integer bits, then the reversed
 * fraction bits through repeated sqrt.s (shift 9 - top: wrong for
 * exponents below 0.5, srav uses the low 5 bits).  Caller:
 * pgl_shade_vertices (spot and specular exponents without a table). */
// FUNC 0x002559D0 float_pow_encoded
float float_pow_encoded(float x, int enc)
{
    f32 r = 1.0f;
    f32 p = x;                          /* x ** (2 ** k) */
    s32 top;
    s32 n;

    EE_SQRTF(x);                        /* x = sqrt(x), in place */
    top = enc >> 16;
    for (n = top - 4; n >= 0; n--) {
        if (enc & 1)
            r *= p;
        p *= p;
        enc >>= 1;
    }
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    enc >>= 9 - top;                    /* srav: only the count's low 5 bits */
#else
    enc >>= (9 - top) & 31;
#endif
    for (n = 4; n >= 0; n--) {
        if (enc & 1)
            r *= x;
        enc >>= 1;
        EE_SQRTF(x);
    }
    return r;
}

/* ======================================================================== */
/* Sine / cosine (VU0)                                                      */
/* ======================================================================== */

/* sin(deg): vu0_sincos_deg leaves (sin, cos) in vf1.xy.  The three readers
 * move the lanes to the FPU in one asm each (qmfc2.i, dsrl32 for y, mtc1):
 * retail has no truncation between the 64-bit GPR and mtc1, which egcs adds
 * to any C conversion of the 64-bit value.  No callers (dead). */
// FUNC 0x00255A48 pgl_sin_deg
float pgl_sin_deg(float deg)
{
    f32 s;

    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "qmfc2.i    $2, $vf1\n\t"
        "mtc1       $2, %0\n\t"               /* the x lane */
        : "=f"(s) : : "$2");
    return s;
}

/* cos(deg) (vf1.y).  Callers: CarRenderCtx::setup, gs_set_light_param_f
 * (spotCosCutoff). */
// FUNC 0x00255A70 pgl_cos_deg
float pgl_cos_deg(float deg)
{
    f32 c;

    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "qmfc2.i    $2, $vf1\n\t"
        "dsrl32     $2, $2, 0\n\t"
        "mtc1       $2, %0\n\t"               /* the y lane */
        : "=f"(c) : : "$2");
    return c;
}

/* *s = sin(deg), *c = cos(deg).  7 callers. */
// FUNC 0x00255A98 pgl_sincos_deg
void pgl_sincos_deg(float deg, float *s, float *c)
{
    f32 sv, cv;

    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "qmfc2.i    $2, $vf1\n\t"
        "mtc1       $2, %0\n\t"
        "dsrl32     $2, $2, 0\n\t"
        "mtc1       $2, %1\n\t"
        : "=f"(sv), "=f"(cv) : : "$2");
    *s = sv;
    *c = cv;
}

/* A pseudo-angle of the vector (a, b) in 45-degree units, -1 .. 7, with
 * one division ("diamond" / octant angle):
 *   b >  |a|  : a / b        (-1 .. 1)
 *   a >= |b|  : 2 - b / a    ( 1 .. 3)
 *   b <= -|a| : a / b + 4    ( 3 .. 5)  - the code falls through into
 *                                          pseudo_angle_q4 (no jr $ra)
 *   else      : 6 - b / a    ( 5 .. 7)
 * Caller: pgl_envmap_rotate (the view azimuth).
 * (Matching: this C compiles to retail's 0x00255AE0..0x00255B70 word for
 * word - all 36 instructions, the last return path included.  0x00255B5C,
 * after the two div.s padding nops, is that path, not a function:
 * ref/not_starts.txt since the 2026-10-16 regen.) */
// FUNC 0x00255AE0 pseudo_angle
float pseudo_angle(float a, float b)
{
    if (a < b) {
        if (-a < b)
            return a / b;
        return 6.0f - b / a;
    }
    if (-a < b)
        return 2.0f - b / a;
    return a / b + 4.0f;                /* the path at 0x00255B5C */
}

/* ======================================================================== */
/* Simple state setters                                                     */
/* ======================================================================== */

/* SCANMSK.MSK by mode (PGL_SCANMASK_*): OFF 0; FIELD field | 2 (the rows
 * of the current field's parity masked), OTHER_FIELD field ^ 3; EVEN 2,
 * ODD 3.  Any mode marks SCANMSK dirty.  4 callers. */
// FUNC 0x00255B70 gs_set_scanmask_mode
void gs_set_scanmask_mode(u32 mode)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (mode) {
    case PGL_SCANMASK_OFF:
        ctx->gen[GS_GEN_SCANMSK] &= ~GS_FIELD_MASK(GS_SCANMSK_MSK);
        break;
    case PGL_SCANMASK_EVEN:
        ctx->gen[GS_GEN_SCANMSK] = GS_FIELD_SET(ctx->gen[GS_GEN_SCANMSK], GS_SCANMSK_MSK,
                                                GS_SCANMSK_EVEN);
        break;
    case PGL_SCANMASK_ODD:
        ctx->gen[GS_GEN_SCANMSK] |= GS_SCANMSK_ODD;        /* both MSK bits */
        break;
    case PGL_SCANMASK_FIELD:
        ctx->gen[GS_GEN_SCANMSK] = GS_FIELD_SET(ctx->gen[GS_GEN_SCANMSK], GS_SCANMSK_MSK,
                                                (u8)ctx->field | GS_SCANMSK_EVEN);
        break;
    case PGL_SCANMASK_OTHER_FIELD:
        ctx->gen[GS_GEN_SCANMSK] = GS_FIELD_SET(ctx->gen[GS_GEN_SCANMSK], GS_SCANMSK_MSK,
                                                (u8)ctx->field ^ GS_SCANMSK_ODD);
        break;
    }
    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_SCANMSK);
}

/* The constant vector of lighting mode 5 (GsContext.constVector, sent with
 * constColor by u115's mode-5 packet); its 6 callers (F_001A8208, the course
 * and car lighting setups ...) set both. */
// FUNC 0x00255C30 pgl_set_const_vector
void pgl_set_const_vector(float x, float y, float z, float w)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->constVector.x = x;
    ctx->constVector.y = y;
    ctx->constVector.z = z;
    ctx->constVector.w = w;
}

/* The constant colour of lighting mode 5: constColor = (r, g, b) * 255,
 * a * 128. */
// FUNC 0x00255C50 pgl_set_const_color
void pgl_set_const_color(float r, float g, float b, float a)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->constColor.x = r * 255.0f;
    ctx->constColor.y = g * 255.0f;
    ctx->constColor.z = b * 255.0f;
    ctx->constColor.w = a * 128.0f;
}

/* ======================================================================== */
/* 2D primitives                                                            */
/* ======================================================================== */

/* An untextured sprite in the current colour (color), coordinates in
 * pixels relative to the draw buffer offset (no draw-window origin):
 * cnt(5) + FLUSH + DIRECT 5, GIFtag A+D x4 {RGBAQ, PRIM (attr | SPRITE,
 * TME off), XYZF3, XYZF2}.  13 callers (fades, movie frames ...).
 * (Matching: the bootstrap's packet form - the GIF tag and every A+D entry
 * in a statement macro, the data words through GsPktDword (alias set 0:
 * they stay in order with the other stores) - and the XY words as int
 * arithmetic in a u64: retail's dsll32 / dsra32 is the truncation at the
 * store.  96.6%: local-alloc swaps $v0 / $v1 between xoff and the cnt tag
 * constant; the statement orders (48) and XY forms tried do not move it.) */
// FUNC 0x00255C90 gs_draw_flat_sprite
void gs_draw_flat_sprite(int x0, int y0, int x1, int y1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 yShift = ctx->halfHeight ? GS_XY_SHIFT_FIELD : GS_XY_SHIFT;
    u64 xy0, xy1;
    u64 prim;
    u32 *p;

    gs_flush_dirty_regs();
    p = ctx->cur;
    prim = GS_PRIM_UNTEXTURED(ctx->primAttr, GS_PRIM_SPRITE);
    xy0 = ((ctx->tgtA.yoff + (y0 << yShift)) << 16) | (ctx->tgtA.xoff + (x0 << GS_XY_SHIFT));
    xy1 = ((ctx->tgtA.yoff + (y1 << yShift)) << 16) | (ctx->tgtA.xoff + (x1 << GS_XY_SHIFT));
    p[0] = DMATAG_CNT(5);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(5);
    ctx->cur = p + 4 + 5 * 4;
    p[1] = 0;
    do {
        PKT64(p, 4) = GIF_NREG(1) | GIF_EOP | 4;    /* NLOOP 4, EOP, PACKED A+D */
        PKT64(p, 6) = GIF_REG_AD;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(p + 8);
        q[0].w[0] = ctx->color;
        q[0].w[1] = GS_RGBAQ_Q_ONE;
        q[1].d = SCE_GS_RGBAQ;
    } while (0);
    do {
        PKT64(p, 12) = prim;
        PKT64(p, 14) = SCE_GS_PRIM;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(p + 16);
        q[0].w[0] = xy0;
        q[1].d = SCE_GS_XYZF3;
        q[0].w[1] = ctx->spriteZF;
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(p + 20);
        q[0].w[0] = xy1;
        q[1].d = SCE_GS_XYZF2;
        q[0].w[1] = ctx->spriteZF;
    } while (0);
}

/* The same flat sprite cut into vertical strips at 64-pixel boundaries
 * (2n + 4 A+D registers, one sprite per strip).  Caller:
 * ReplayBlur::invertDepth. */
// FUNC 0x00255DF0 gs_draw_flat_sprite_strips
void gs_draw_flat_sprite_strips(int x0, int y0, int x1, int y1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 yShift = ctx->halfHeight ? GS_XY_SHIFT_FIELD : GS_XY_SHIFT;
    u64 prim;
    s32 nStrips, swap;
    u64 x, xEnd;
    u32 *p, *q;
    u32 zf;
    u64 xyTop, xyBottom;

    gs_flush_dirty_regs();
    prim = GS_PRIM_UNTEXTURED(ctx->primAttr, GS_PRIM_SPRITE);
    if (x1 < x0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    nStrips = ((x1 - 1) >> 6) - (x0 >> 6);  /* strips - 1 */
    x = x0 << GS_XY_SHIFT;
    xEnd = x1 << GS_XY_SHIFT;
    p = gs_open_direct(ctx, nStrips * 2 + 5);
    do {
        PKT64(p, 6) = GIF_REG_AD;
        PKT64(p, 4) = (u64)(s64)(s32)((nStrips * 2 + 4) | GIF_EOP) | GIF_NREG(1);
    } while (0);
    do {
        GsPktDword *e = (GsPktDword *)(p + 8);
        e[0].w[0] = ctx->color;
        e[0].w[1] = GS_RGBAQ_Q_ONE;
        e[1].d = SCE_GS_RGBAQ;
    } while (0);
    q = p + 16;
    do {
        PKT64(p, 12) = prim;
        PKT64(p, 14) = SCE_GS_PRIM;
    } while (0);
    zf = ctx->spriteZF;
    xyTop = *(u32a *)&ctx->tgtA.xoff + ((u32)y0 << (yShift + 16));     /* xoff | yoff << 16 */
    xyBottom = *(u32a *)&ctx->tgtA.xoff + ((u32)y1 << (yShift + 16));
    while (nStrips-- != 0) {
        u64 nextX = (x + (64 << GS_XY_SHIFT)) & ~0x3F0;  /* next 64-pixel boundary */

        GS_AD_PUT(q, xyTop + x, zf, SCE_GS_XYZF3);
        GS_AD_PUT(q, xyBottom + nextX, zf, SCE_GS_XYZF2);
        x = nextX;
    }
    GS_AD_PUT(q, xyTop + x, zf, SCE_GS_XYZF3);
    GS_AD_PUT(q, xyBottom + xEnd, zf, SCE_GS_XYZF2);
}

/* Same machine code as the bootstrap's gs_draw_textured_sprite
 * (0x0100E2A0, src/gs_driver.cpp).
 * Flushes dirty registers and appends DMA cnt(4) + FLUSH + DIRECT 4 +
 * GIFtag REGLIST NREG 6 EOP {PRIM, RGBAQ, UV, XYZF3, UV, XYZF2}:
 * PRIM = primAttr | 0x116 (SPRITE, TME, FST), RGBAQ = {colorTex, Q 1.0},
 * UV = (u | v << 16) << 4 + 0x80008 (half-texel), XY = ((x + winX) << 4) +
 * tgtA.xoff and (y << (halfHeight ? 3 : 4)) + (winY << 4) + tgtA.yoff,
 * XYZF high word = spriteZF.  Callers: RaceSmoke::render (0x001A4CC8),
 * ReplayBlur::copyFrameTo.
 * (Matching, 91.0% as the bootstrap's: its block form - the tag words in
 * variables, the GIF tag in a statement macro, an int PRIM, retail's store
 * order - then local-alloc swaps two pairs after the call: retail keeps
 * the context in $v1 and cur + 0x50 in $v0, and 0x80008 / FLUSH in $a6 /
 * $a5.  Searched: 720 orders of the block's seven statements, 128 forms of
 * the XY arithmetic, 24 orders of the last four stores; the reference
 * compiler 991111-01 gives the same.) */
// FUNC 0x00256020 gs_draw_textured_sprite
void gs_draw_textured_sprite(int x0, int y0, int x1, int y1,
                             int u0, int v0, int u1, int v1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 yShift = ctx->halfHeight ? GS_XY_SHIFT_FIELD : GS_XY_SHIFT;
    u64 uv0, uv1;
    u32 xy0, xy1;

    {                                       /* window origin and draw offset, read once */
        s32 winX = ctx->winX;
        s32 winY = ctx->winY << GS_XY_SHIFT;
        s32 xoff = ctx->tgtA.xoff;
        s32 yoff = ctx->tgtA.yoff;
        xy0 = ((u32)((y0 << yShift) + winY + yoff) << 16) | (u32)(((x0 + winX) << GS_XY_SHIFT) + xoff);
        xy1 = ((u32)((y1 << yShift) + winY + yoff) << 16) | (u32)(((x1 + winX) << GS_XY_SHIFT) + xoff);
    }
    uv0 = (u64)(s64)(s32)(((u32)v0 << 16) | ((u32)u0 & 0xFFFF)) << GS_XY_SHIFT;
    uv1 = (u64)(s64)(s32)(((u32)v1 << 16) | ((u32)u1 & 0xFFFF)) << GS_XY_SHIFT;

    gs_flush_dirty_regs();
    {
        GsContext *c2 = GS_SPR_CTX;             /* re-read after the call */
        u32 *p = c2->cur;
        u32 *q;
        int prim;
        u32 rgba, zf;
        u32 tc = DMATAG_CNT(4);
        u32 tf = VIF_FLUSH;
        u32 td = VIF_DIRECT(4);

        c2->cur = p + 4 + 4 * 4;
        q = p + 4;
        prim = c2->primAttr | GS_PRIM_SPRITE | GS_PRIM_TME | GS_PRIM_FST;
        rgba = c2->colorTex;
        zf = c2->spriteZF;
        uv0 += (GS_UV_HALF_TEXEL << 16) | GS_UV_HALF_TEXEL;
        uv1 += (GS_UV_HALF_TEXEL << 16) | GS_UV_HALF_TEXEL;
        p[0] = tc;
        p[2] = tf;
        p[3] = td;
        p[1] = 0;
        do {
            PKT64(q, 0) = GIF_NREG(6) | GIF_FLG_REGLIST | GIF_EOP | 1;  /* NLOOP 1 */
            PKT64(q, 2) = GIF_REGS_SPRITE_UV;
        } while (0);
        PKT64(q, 4) = prim;
        PKT64(q, 8) = uv0;
        q[10] = xy0;
        PKT64(q, 12) = uv1;
        q[14] = xy1;
        q[6] = rgba;
        q[7] = GS_RGBAQ_Q_ONE;
        q[11] = zf;
        q[15] = zf;
    }
}

/* gs_draw_textured_sprite with float coordinates (pixels, texels): XY =
 * x * 16 + winX * 16 (as float, truncated) + tgtA offset, y scaled by 8 for
 * field rendering; UV = (s32)(u * 16) + 8.  9 callers.
 * (Matching: the scale as an int select converted once (retail: movn,
 * then cvt.s.w), the float parameters scaled in place, and after the call
 * gs_draw_textured_sprite's block, which is exact here.  84.7%: the
 * scheduler issues the products of x0 / y0 before those of x1 / y1 (retail
 * x1 first), which renumbers the conversions; 72 statement orders tried.) */
// FUNC 0x00256188 gs_draw_textured_sprite_f
void gs_draw_textured_sprite_f(float x0, float y0, float x1, float y1,
                               float u0, float v0, float u1, float v1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 yMul = ctx->halfHeight ? 8 : 16;
    f32 yScale = (f32)yMul;
    f32 winX = (f32)(ctx->winX << GS_XY_SHIFT);
    f32 winY = (f32)(ctx->winY << GS_XY_SHIFT);
    u32 xy0, xy1, uv0, uv1;

    x1 = winX + x1 * 16.0f;
    y1 = winY + y1 * yScale;
    x0 = winX + x0 * 16.0f;
    y0 = winY + y0 * yScale;
    u0 *= 16.0f;
    v0 *= 16.0f;
    u1 *= 16.0f;
    v1 *= 16.0f;
    xy0 = ((ctx->tgtA.yoff + (s32)y0) << 16) | ((ctx->tgtA.xoff + (s32)x0) & 0xFFFF);
    xy1 = ((ctx->tgtA.yoff + (s32)y1) << 16) | ((ctx->tgtA.xoff + (s32)x1) & 0xFFFF);
    uv0 = (((s32)v0 + GS_UV_HALF_TEXEL) << 16) | (((s32)u0 + GS_UV_HALF_TEXEL) & 0xFFFF);
    uv1 = (((s32)v1 + GS_UV_HALF_TEXEL) << 16) | (((s32)u1 + GS_UV_HALF_TEXEL) & 0xFFFF);
    gs_flush_dirty_regs();
    {
        GsContext *c2 = GS_SPR_CTX;             /* re-read after the call */
        u32 *p = c2->cur;
        u32 *q;
        int prim;
        u32 rgba, zf;
        u32 tc = DMATAG_CNT(4);
        u32 tf = VIF_FLUSH;
        u32 td = VIF_DIRECT(4);

        c2->cur = p + 4 + 4 * 4;
        q = p + 4;
        prim = c2->primAttr | GS_PRIM_SPRITE | GS_PRIM_TME | GS_PRIM_FST;
        rgba = c2->colorTex;
        zf = c2->spriteZF;
        p[0] = tc;
        p[2] = tf;
        p[3] = td;
        p[1] = 0;
        do {
            PKT64(q, 0) = GIF_NREG(6) | GIF_FLG_REGLIST | GIF_EOP | 1;  /* NLOOP 1 */
            PKT64(q, 2) = GIF_REGS_SPRITE_UV;
        } while (0);
        PKT64(q, 4) = prim;
        PKT64(q, 8) = (u64)(s64)(s32)uv0;
        q[10] = xy0;
        PKT64(q, 12) = (u64)(s64)(s32)uv1;
        q[14] = xy1;
        q[6] = rgba;
        q[7] = GS_RGBAQ_Q_ONE;
        q[11] = zf;
        q[15] = zf;
    }
}

/* A textured sprite cut into strips at 32-pixel boundaries (the texture
 * cache), u interpolated across: u = u0 + (u1 - u0) * (x - x0) / (x1 - x0)
 * (integer division, 12.4 units).  x0 > x1 swaps x and u (not v).  Nothing
 * for a zero width.  9 callers (blur, logger graph ...).
 * (Matching: retail does the strip arithmetic in 64 bits - daddiu for the
 * next x, daddu for UV and XY, dsll32 / dsra32 at the XY stores - and
 * writes the packet a dword at a time (q + 8 per store pair): the 64-bit
 * locals and the GsPktDword cursor.  67.6%: retail converts both x before
 * the y sums and tests the swap early; here the scheduler interleaves
 * them, which renumbers the FPU and GPR temporaries (24 orders of the eight
 * coordinate statements tried, the U/V order U0 V0 U1 V1 kept).) */
// FUNC 0x00256360 gs_draw_textured_sprite_strips_f
void gs_draw_textured_sprite_strips_f(float x0, float y0, float x1, float y1,
                                      float u0, float v0, float u1, float v1)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 yScale = 16.0f;
    f32 winX, winY;
    u64 prim;
    s32 X0, X1, U0, U1, V0, V1, Y0, Y1, swap;   /* 12.4 */
    s32 width, nStrips, du;
    s64 curX, xEnd;
    s32 curU, uEnd;
    u64 y0Hi, y1Hi, v0Hi, v1Hi;                 /* << 16: the high halves */
    u32 *p;
    GsPktDword *q;
    u32 zf;

    if (ctx->halfHeight != 0)
        yScale = 8.0f;
    gs_flush_dirty_regs();
    winX = (f32)ctx->winX * 16.0f;
    winY = (f32)ctx->winY * 16.0f;
    prim = (u16)(ctx->primAttr | GS_PRIM_SPRITE | GS_PRIM_TME | GS_PRIM_FST);
    X0 = ctx->tgtA.xoff + (s32)(winX + x0 * 16.0f);
    X1 = ctx->tgtA.xoff + (s32)(winX + x1 * 16.0f);
    Y0 = ctx->tgtA.yoff + (s32)(winY + y0 * yScale);
    Y1 = ctx->tgtA.yoff + (s32)(winY + y1 * yScale);
    U0 = (s32)(u0 * 16.0f + 8.0f);
    V0 = (s32)(v0 * 16.0f + 8.0f);
    U1 = (s32)(u1 * 16.0f + 8.0f);
    V1 = (s32)(v1 * 16.0f + 8.0f);
    if (X1 < X0) {
        swap = X0; X0 = X1; X1 = swap;
        swap = U0; U0 = U1; U1 = swap;
    }
    width = X1 - X0;
    nStrips = ((X1 - 1) >> 9) - (X0 >> 9);  /* strips - 1 */
    du = U1 - U0;
    curX = X0;
    xEnd = X1;
    curU = U0;
    uEnd = U1;
    if (width == 0)
        return;
    p = gs_open_direct(ctx, nStrips * 2 + 5);
    do {
        PKT64(p, 4) = GIF_NREG(2) | GIF_FLG_REGLIST | 1;    /* NLOOP 1 */
        PKT64(p, 6) = GIF_REGS_RGBAQ_PRIM;
    } while (0);
    p[8] = ctx->colorTex;
    p[9] = GS_RGBAQ_Q_ONE;
    PKT64(p, 10) = prim;
    zf = ctx->spriteZF;
    y0Hi = Y0 << 16;
    v1Hi = V1 << 16;
    y1Hi = Y1 << 16;
    v0Hi = V0 << 16;
    q = (GsPktDword *)(p + 16);
    do {
        PKT64(p, 14) = GIF_REGS_UV_SPRITE;
        PKT64(p, 12) = (u64)(s64)(s32)((nStrips + 1) | GIF_EOP) | GIF_NREG(4) | GIF_FLG_REGLIST;
    } while (0);
    while (nStrips-- != 0) {
        s64 nextX = (curX + (32 << GS_XY_SHIFT)) & ~0x1F0;    /* next 32-pixel boundary */
        s32 nextU = U0 + (du * ((s32)nextX - X0)) / width;

        q->d = v0Hi + curU;                     /* UV */
        q++;
        q->w[1] = zf;
        q->w[0] = y0Hi + curX;                  /* XYZF3 */
        q++;
        q->d = v1Hi + nextU;                    /* UV */
        q++;
        q->w[1] = zf;
        q->w[0] = y1Hi + nextX;                 /* XYZF2 */
        q++;
        curX = nextX;
        curU = nextU;
    }
    q->d = v0Hi + curU;
    q++;
    q->w[1] = zf;
    q->w[0] = y0Hi + curX;
    q++;
    q->d = v1Hi + uEnd;
    q++;
    q->w[1] = zf;
    q->w[0] = y1Hi + xEnd;
}

/* A line in the current colour (pixels, relative to the draw window):
 * cnt(3) + FLUSH + DIRECT 3, GIFtag REGLIST {RGBAQ, PRIM (attr | LINE, TME
 * off), XYZF3, XYZF2}.  Caller: RacePS2Base::drawMirror.
 * (Matching: the coordinates turned into 12.4 in the parameters, the GIF
 * tag and each XYZF word pair in statement macros.  93.9%: originX /
 * originY / prim rotate through $a1..$a3; 24 orders of the coordinate
 * statements and 24 of the setup statements tried.) */
// FUNC 0x002566F0 gs_draw_line
void gs_draw_line(int x0, int y0, int x1, int y1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 yShift = ctx->halfHeight ? GS_XY_SHIFT_FIELD : GS_XY_SHIFT;
    s32 originX, originY;
    u64 prim;
    u32 *p;

    gs_flush_dirty_regs();
    originX = ctx->tgtA.xoff + (ctx->winX << GS_XY_SHIFT);
    originY = ctx->tgtA.yoff + (ctx->winY << GS_XY_SHIFT);
    p = ctx->cur;
    prim = GS_PRIM_UNTEXTURED(ctx->primAttr, GS_PRIM_LINE);
    x0 = originX + (x0 << GS_XY_SHIFT);
    y0 = originY + (y0 << yShift);
    x1 = originX + (x1 << GS_XY_SHIFT);
    y1 = originY + (y1 << yShift);
    p[0] = DMATAG_CNT(3);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(3);
    ctx->cur = p + 4 + 3 * 4;
    p[1] = 0;
    do {
        PKT64(p, 4) = GIF_NREG(4) | GIF_FLG_REGLIST | GIF_EOP | 1;  /* NLOOP 1 */
        PKT64(p, 6) = GIF_REGS_LINE;
    } while (0);
    p[8] = ctx->color;
    p[9] = GS_RGBAQ_Q_ONE;
    PKT64(p, 10) = prim;
    do {
        p[12] = ((u32)y0 << 16) | ((u32)x0 & 0xFFFF);
        p[13] = ctx->spriteZF;
    } while (0);
    do {
        p[14] = ((u32)y1 << 16) | ((u32)x1 & 0xFFFF);
        p[15] = ctx->spriteZF;
    } while (0);
}

/* gs_draw_line with float pixel coordinates (truncated after scaling by 16
 * or 8).  Caller: F_0020A360 (the camera groove meter's arc).
 * (Matching: gs_draw_line's packet form, each coordinate one sum.  94.3%:
 * y0 / y1 take $f21 / $f22 (retail the other way round), winY / prim
 * $t1 / $t3; 96 statement orders and sum forms tried: the order that gives
 * the FPU registers rotates the GPRs.) */
// FUNC 0x00256838 gs_draw_line_f
void gs_draw_line_f(float x0, float y0, float x1, float y1)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 yScale = 16.0f;
    s32 X0, Y0, X1, Y1;
    u64 prim;
    u32 *p;

    if (ctx->halfHeight != 0)
        yScale = 8.0f;
    gs_flush_dirty_regs();
    p = ctx->cur;
    prim = GS_PRIM_UNTEXTURED(ctx->primAttr, GS_PRIM_LINE);
    X0 = ctx->tgtA.xoff + (s32)(x0 * 16.0f) + (ctx->winX << GS_XY_SHIFT);
    X1 = ctx->tgtA.xoff + (s32)(x1 * 16.0f) + (ctx->winX << GS_XY_SHIFT);
    Y1 = ctx->tgtA.yoff + (s32)(y1 * yScale) + (ctx->winY << GS_XY_SHIFT);
    Y0 = ctx->tgtA.yoff + (s32)(y0 * yScale) + (ctx->winY << GS_XY_SHIFT);
    p[0] = DMATAG_CNT(3);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(3);
    ctx->cur = p + 4 + 3 * 4;
    p[1] = 0;
    do {
        PKT64(p, 4) = GIF_NREG(4) | GIF_FLG_REGLIST | GIF_EOP | 1;  /* NLOOP 1 */
        PKT64(p, 6) = GIF_REGS_LINE;
    } while (0);
    p[8] = ctx->color;
    p[9] = GS_RGBAQ_Q_ONE;
    PKT64(p, 10) = prim;
    do {
        p[12] = ((u32)Y0 << 16) | ((u32)X0 & 0xFFFF);
        p[13] = ctx->spriteZF;
    } while (0);
    do {
        p[14] = ((u32)Y1 << 16) | ((u32)X1 & 0xFFFF);
        p[15] = ctx->spriteZF;
    } while (0);
}

/* ======================================================================== */
/* Primitives through the VU0 combined matrix                               */
/* ======================================================================== */

/* Viewport constants into vf20..vf23 (vf23 viewScale, vf22 viewCenter,
 * vf21 limits, vf20 vBB0). */
#define VU0_LOAD_VIEWPORT(ctx)                                              \
    __asm__ __volatile__(                                                   \
        "lqc2   $vf23, 0xB80(%0)\n\t"                                       \
        "lqc2   $vf22, 0xB90(%0)\n\t"                                       \
        "lqc2   $vf21, 0xBA0(%0)\n\t"                                       \
        "lqc2   $vf20, 0xBB0(%0)\n\t"                                       \
        : : "r"(ctx) : "memory")

/* A 2D line strip of n points (x, y pairs, any alignment) through the
 * combined matrix vf24..vf27 (z = 0, w = 1): one PACKED XYZF2 qword per
 * point after the perspective divide, the clamp to the limits and the
 * viewport.  The clip codes (vu0_clip_code's) of consecutive points are
 * ANDed: a segment whose ends lie outside the same plane gets w + 2048
 * (XYZF2.ADC: no drawing kick).  Needs n >= 2.  Caller:
 * LoggerRaceGraph::drawCurve.
 * (Matching: the tags in statement macros; the point read straight into a
 * GPR (retail ldl / ldr, no stack copy: an asm, as the quadword loads);
 * vf9 built from 0x45000000 by dsll32 in the asm (retail lui; the folded
 * constant is ori / dsll32 15); one asm per VU0 step, the clip flags read
 * by a volatile one (non-volatile, loop.c hoists it out of the loop).
 * 95.1%: retail loads pos[3] before the flags' vnop (a volatile read here
 * cannot pass it; reading it first moves the registers), and the second
 * tag's constants / the packet pointer are scheduled differently.) */
// FUNC 0x002569C0 pgl_draw_line_strip_2d
void pgl_draw_line_strip_2d(const float *xy, int n)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 clipPos[4] __attribute__((aligned(16)));       /* sp+0x00 */
    u64 prim;
    u64 point;
    u32 *p, *q;
    u32 code, clip, prevClip;

    if (n < 2)
        return;
    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    prim = (ctx->primAttr & ~GS_PRIM_TME) | GS_PRIM_LINE_STRIP;
    p = gs_open_direct(ctx, n + 3);
    do {
        PKT64(p, 4) = GIF_NREG(2) | GIF_FLG_REGLIST | 1;    /* NLOOP 1 */
        PKT64(p, 6) = GIF_REGS_RGBAQ_PRIM;
    } while (0);
    p[8] = ctx->color;
    p[9] = GS_RGBAQ_Q_ONE;
    PKT64(p, 10) = prim;
    q = p + 16;
    do {
        PKT64(p, 14) = SCE_GS_XYZF2;
        PKT64(p, 12) = (u64)(s64)(s32)(n | GIF_EOP) | GIF_NREG(1) | GIF_FLG_PACKED;
    } while (0);
    VU0_LOAD_VIEWPORT(ctx);
    {
        u64 adc = GS_ADC_W_FLOAT;               /* vf9 = (0, 0, 0, 2048.0) */

        __asm__ __volatile__(
            "dsll32     %0, %0, 0\n\t"
            "pcpyld     %0, %0, $0\n\t"
            "qmtc2.i    %0, $vf9\n\t"
            : "+r"(adc));
    }
    prevClip = 0;
    do {
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
        __asm__("ldl        %0, 7(%1)\n\tldr        %0, 0(%1)" : "=&r"(point) : "r"(xy));
#else
        memcpy(&point, xy, 8);
#endif
        xy += 2;
        __asm__ __volatile__(
            "qmtc2      %1, $vf1\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
            "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
            "vdiv       " VU0_Q ", $vf0w, $vf1w\n\t"
            "vmini.xyzw $vf2, $vf1, $vf21\n\t"
            ".word      0x4BE109FF\n\t"         /* vclipw.xyzw $vf1, $vf1w */
            "sqc2       $vf1, 0(%0)\n\t"
            "vnop\n\t"
            "vnop\n\t"
            "vnop"
            : : "r"(clipPos), "r"(point) : "memory");
        __asm__ __volatile__("vnop\n\tcfc2       %0, $vi18" : "=r"(code));
        clip = code & VU0_CLIP_MASK;
        if (clipPos[3] < 0) {                   /* w < 0 */
            VU0_CLIP_NEG_W(code);
            clip = code & VU0_CLIP_MASK;
        }
        __asm__ __volatile__(
            "vwaitq\n\t"
            "vmulq.xyzw   $vf2, $vf2, " VU0_Q "\n\t"
            "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
            "vmadd.xyzw   $vf2, $vf2, $vf23\n\t"
            : : : "memory");
        if (prevClip & clip)                    /* the segment is outside a plane */
            __asm__ __volatile__("vadd.xyzw $vf2, $vf2, $vf9");
        __asm__ __volatile__("vftoi4.xyzw  $vf2, $vf2");
        prevClip = clip;
        __asm__ __volatile__("sqc2         $vf2, 0(%0)" : : "r"(q) : "memory");
        q += 4;
    } while (--n != 0);
}

/* Sprite depth from an eye-space distance z: the point (0, 0, -z, 1)
 * through the projection matrix (vf28..vf31 when the projection is
 * current, else its parked copy at VU0 qword 8), divided, clamped (z to
 * -1..1), through the viewport and converted: spriteZF = Z | F << 24 with F
 * the viewport-mapped w (the fog), GS_FOG_NONE when z <= 0.  4 callers.
 * (Matching: the constant setup is an asm of its own (retail's mfc1 of z
 * sits between it and the qmtc2), and the no-fog case is a statement
 * macro: its loop note 8-aligns the else label, retail's nop.) */
// FUNC 0x00256B80 pgl_set_sprite_depth
void pgl_set_sprite_depth(float z)
{
    GsContext *ctx = GS_SPR_CTX;
    u64 zAndF;
    u32 fog, zf;

    __asm__ __volatile__("vnop");
    if (ctx->matrixMode == PGL_MTX_PROJECTION) {
        __asm__ __volatile__(
            "vmove.xyzw $vf20, $vf28\n\t"
            "vmove.xyzw $vf21, $vf29\n\t"
            "vmove.xyzw $vf22, $vf30\n\t"
            "vmove.xyzw $vf23, $vf31\n\t");
    } else {
        __asm__ __volatile__(
            "ctc2.i     %0, $vi10\n\t"
            "vlqi.xyzw  $vf20, ($vi10++)\n\t"
            "vlqi.xyzw  $vf21, ($vi10++)\n\t"
            "vlqi.xyzw  $vf22, ($vi10++)\n\t"
            "vlqi.xyzw  $vf23, ($vi10++)\n\t"
            "vnop\n\t"
            "vnop\n\t"
            : : "r"(VU0_MATRIX_QW(PGL_MTX_PROJECTION)) : "memory");
    }
    __asm__ __volatile__(
        "vmove.xyzw   $vf2, $vf0\n\t"
        "vsubw.xyzw   $vf3, $vf0, $vf0w");    /* (-1, -1, -1, 0) */
    __asm__ __volatile__(
        "qmtc2        %1, $vf1\n\t"
        "vsubx.z      $vf2, $vf2, $vf1x\n\t"    /* (0, 0, -z, 1) */
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
        "vmaddw.xyzw  $vf1, $vf23, $vf2w\n\t"
        "lqc2         $vf23, 0xB80(%2)\n\t"
        "lqc2         $vf22, 0xB90(%2)\n\t"
        "lqc2         $vf21, 0xBA0(%2)\n\t"
        "lqc2         $vf20, 0xBB0(%2)\n\t"
        "vdiv         " VU0_Q ", $vf0w, $vf1w\n\t"
        "vmini.xyzw   $vf1, $vf1, $vf21\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf1, $vf1, " VU0_Q "\n\t"
        "vminiw.z     $vf1, $vf1, $vf0w\n\t"
        "vmaxx.z      $vf1, $vf1, $vf3x\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
        "vftoi0.xyzw  $vf1, $vf1\n\t"
        "qmfc2        %0, $vf1\n\t"
        "pcpyud       %0, %0, $0\n\t"           /* (Z, F) words */
        : "=r"(zAndF) : "r"(u117_f2u(z)), "r"(ctx) : "memory");
    zf = (u32)zAndF;
    if (z <= 0.0f) {
        do {
            fog = GS_FOG_NONE;
        } while (0);
    } else {
        fog = (u32)((s64)zAndF >> 32);
    }
    ctx->spriteZF = (fog << 24) | zf;
}

/* pgl_set_sprite_depth((float)z) (tail call).  No callers. */
// FUNC 0x00256C68 pgl_set_sprite_depth_i
void pgl_set_sprite_depth_i(int z)
{
    pgl_set_sprite_depth((f32)z);
}

/* spriteZF = min(fog, 0xFF) << 24 | clamp(z, 0, 0xFFFFFF).  3 callers. */
// FUNC 0x00256C78 gs_set_sprite_zf
void gs_set_sprite_zf(int z, u32 fog)
{
    GsContext *ctx = GS_SPR_CTX;

    if (z < 0)
        z = 0;
    if (z > GS_Z24_MAX)
        z = GS_Z24_MAX;
    if (fog > 0xFF)
        fog = GS_FOG_NONE;
    ctx->spriteZF = (fog << 24) | (u32)z;
}

/* Clips the homogeneous segment a-b (4 floats each, modified in place)
 * against the 6 planes, Cohen-Sutherland style, with vu0_clip_code's codes:
 * returns the OR of the codes when both ends are outside one plane
 * (rejected), 0 when the segment is inside (after clipping).
 * Plane i: even = +k (x_k - w), odd = -k (x_k + w), k = i / 2; the outside
 * end moves to the intersection (b * da - a * db) / (da - db).
 * Caller: pgl_draw_line_3d.
 * (Matching, 83.8%: retail keeps plane / codeA in $s0 / $s1 (here the other
 * way round) and returns the OR of the codes on both early exits through
 * a copy (the second, known 0, is not folded: CSE here does fold it to
 * `li v0, 0'); the for / do loop forms, the declaration order and the
 * distance order were tried.) */
// FUNC 0x00256CB8 clip_line_homogeneous
int clip_line_homogeneous(float *a, float *b)
{
    s32 plane;
    s32 codeA, codeB, axis, j;
    f32 distA, distB, invDiff;

    for (plane = 0; plane < 6; plane++) {
        codeA = vu0_clip_code(a);
        codeB = vu0_clip_code(b);
        if (codeA & codeB)
            return codeA | codeB;
        codeB |= codeA;
        if (codeB == 0)
            return codeB;
        if ((codeB >> plane) & 1) {
            axis = plane >> 1;
            if (plane & 1) {
                distA = a[axis] + a[3];
                distB = b[axis] + b[3];
            } else {
                distA = a[axis] - a[3];
                distB = b[axis] - b[3];
            }
            invDiff = 1.0f / (distA - distB);
            if ((codeA >> plane) & 1) {             /* a is outside: move a */
                for (j = 0; j < 4; j++)
                    a[j] = (b[j] * distA - a[j] * distB) * invDiff;
            } else {                                /* move b */
                for (j = 0; j < 4; j++)
                    b[j] = (b[j] * distA - a[j] * distB) * invDiff;
            }
        }
    }
    return 0;
}

/* A 3D line p0-p1 (3 floats each, any alignment) in the current colour:
 * through the combined matrix, clipped (clip_line_homogeneous), divided,
 * clamped, viewport-mapped, ftoi4; one PACKED packet: cnt(4) + FLUSH +
 * DIRECT 4, GIFtag (PRE, PRIM = attr | LINE, TME off, NREG 3) {RGBAQ,
 * XYZF2, XYZF2}, the colour bytes spread to words with pextlb / pextlh.
 * Callers: RaceSteeringDisplay::draw, particle_draw_line,
 * pgl_draw_line_3f.
 * (Matching: the points read as retail does (ldl / ldr of x, y, then the
 * lw of z, pcpyld), the transform its own asm, the GIF tag's high word as
 * a 64-bit shift of the PRIM (retail dsll 15, then the truncation).
 * 87.9%: $v0 / $v1 swapped in the point loads and the header; the operand
 * forms of the loads were tried.) */
// FUNC 0x00256E60 pgl_draw_line_3d
void pgl_draw_line_3d(const float *p0, const float *p1)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 clipPos[8] __attribute__((aligned(16)));   /* sp+0x00 p0, sp+0x10 p1 */
    u64 xy, tmp;
    u64 rgba;
    u64 prim;
    u32 *p, *q;

    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    VU0_LOAD_VIEWPORT(ctx);
    __asm__ __volatile__("vnop");
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    __asm__ __volatile__("ldl        %0, 7(%1)\n\tldr        %0, 0(%1)" : "=&r"(xy) : "r"(p0));
#else
    memcpy(&xy, p0, 8);
#endif
    __asm__ __volatile__(
        "pcpyld     %0, %1, %2\n\t"
        "qmtc2      %0, $vf1\n\t"
        : "=r"(tmp) : "r"(((const s32 *)p0)[2]), "r"(xy));
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    __asm__ __volatile__("ldl        %0, 7(%1)\n\tldr        %0, 0(%1)" : "=&r"(xy) : "r"(p1));
#else
    memcpy(&xy, p1, 8);
#endif
    __asm__ __volatile__(
        "pcpyld     %0, %1, %2\n\t"
        "qmtc2      %0, $vf2\n\t"
        : "=r"(tmp) : "r"(((const s32 *)p1)[2]), "r"(xy));
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf2z\n\t"
        "vmaddw.xyzw  $vf2, $vf27, $vf0w\n\t"
        "sqc2       $vf1, 0(%0)\n\t"
        "sqc2       $vf2, 0x10(%0)\n\t"
        : : "r"(clipPos) : "memory");
    if (clip_line_homogeneous(&clipPos[4], &clipPos[0]) != 0)
        return;
    __asm__ __volatile__(
        "lqc2       $vf1, 0(%0)\n\t"
        "lqc2       $vf2, 0x10(%0)\n\t"
        "vdiv       " VU0_Q ", $vf0w, $vf1w\n\t"
        "vmini.xyzw $vf1, $vf1, $vf21\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw $vf1, $vf1, " VU0_Q "\n\t"
        "vdiv       " VU0_Q ", $vf0w, $vf2w\n\t"
        "vmini.xyzw $vf2, $vf2, $vf21\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw $vf2, $vf2, " VU0_Q "\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf2, $vf2, $vf23\n\t"
        "vftoi4.xyzw  $vf1, $vf1\n\t"
        "vftoi4.xyzw  $vf2, $vf2\n\t"
        : : "r"(clipPos) : "memory");
    prim = GS_PRIM_UNTEXTURED(ctx->primAttr, GS_PRIM_LINE);
    rgba = (u64)(s64)(s32)ctx->color;
    __asm__ __volatile__(
        "pextlb     %0, %0, %0\n\t"
        "pextlh     %0, %0, %0\n\t"
        "qmtc2      %0, $vf3\n\t"               /* one colour byte per word */
        : "+r"(rgba));
    gs_flush_dirty_regs();
    p = ctx->cur;
    p[4] = GIF_EOP | 1;                         /* NLOOP 1 */
    p[0] = DMATAG_CNT(4);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(4);
    p[1] = 0;
    q = p + 4;
    ctx->cur = p + 4 + 4 * 4;
    q[1] = (prim << 15) | GIF_HI_NREG(3) | GIF_HI_FLG_PACKED | GIF_HI_PRE;
    q[2] = (u32)GIF_REGS_LINE_3D;
    q[3] = 0;
    __asm__ __volatile__(
        "sqc2       $vf3, 0x10(%0)\n\t"
        "sqc2       $vf1, 0x20(%0)\n\t"
        "sqc2       $vf2, 0x30(%0)\n\t"
        : : "r"(q) : "memory");
}

/* pgl_draw_line_3d with the points as floats.  5 callers
 * (LoggerRaceGraph::drawSpeedGrid ...). */
// FUNC 0x00256FF8 pgl_draw_line_3f
void pgl_draw_line_3f(float x0, float y0, float z0,
                      float x1, float y1, float z1)
{
    f32 a[4] __attribute__((aligned(16)));     /* sp+0x00 */
    f32 b[4] __attribute__((aligned(16)));     /* sp+0x10 */

    a[0] = x0;
    a[1] = y0;
    a[2] = z0;
    b[0] = x1;
    b[1] = y1;
    b[2] = z1;
    pgl_draw_line_3d(a, b);
}

/* Sets the STMASK and queues it: DMA cnt 0 tag with VIF STMASK and its data
 * word in the tag's upper half.  Takes the context.  Callers:
 * vif_model_draw, vifmodel_draw_lit, pgl_put_texcoords. */
// FUNC 0x00257030 gs_emit_stmask
void gs_emit_stmask(GsContext *ctx, u32 mask)
{
    u32 *p = ctx->cur;

    PKT64(p, 0) = DMATAG_CNT(0);
    PKT64(p, 2) = ((u64)mask << 32) | VIF_STMASK;
    ctx->cur = p + 4;
    ctx->stmask = mask;
}

/* The clip code of a homogeneous point (16 bytes, any alignment): bit 2k
 * x_k > w, bit 2k + 1 x_k < -w (k = x, y, z).  vclipw compares against
 * |w|, so for w < 0 its flags are swapped and inverted.  Caller:
 * clip_line_homogeneous.
 * (Matching: the read of the clip flags is an asm of its own with an
 * output, not volatile under egcs: sched1 issues the pos[3] load ahead of
 * its vnop, as retail; the w < 0 fix-up is a statement macro, whose loop
 * note 8-aligns the join (retail's nop before jr).  98.1%: retail's
 * pcpyld leaves the quadword in the high half's register ($v0), the
 * shared u117_load_u128 (exact in pgl_lightfv) in the low half's ($v1);
 * an asm with that operand order gets the pcpyld but moves pos[3] / the
 * flags to $v1 / $v0 (85.6%).) */
// FUNC 0x00257060 vu0_clip_code
int vu0_clip_code(const float *v)
{
    s32 pos[4] __attribute__((aligned(16)));   /* sp+0x00 */
    u32 clip;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    GsVec4Arg q;

    q = u117_load_u128(v);
    __asm__ __volatile__(
        "qmtc2      %1, $vf1\n\t"
        ".word      0x4BE109FF\n\t"         /* vclipw.xyzw $vf1, $vf1w */
        "sqc2       $vf1, 0(%0)\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop"
        : : "r"(pos), "r"(q) : "memory");
    __asm__("vnop\n\tcfc2       %0, $vi18" : "=r"(clip));
#else
    u64 lo, hi, tmp;

    memcpy(&lo, v, 8);
    memcpy(&hi, v + 2, 8);
    __asm__ __volatile__(
        "pcpyld     %1, %2, %3\n\t"
        "qmtc2      %1, $vf1\n\t"
        ".word      0x4BE109FF\n\t"         /* vclipw.xyzw $vf1, $vf1w */
        "sqc2       $vf1, 0(%4)\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "cfc2       %0, $vi18\n\t"
        : "=r"(clip), "=&r"(tmp) : "r"(hi), "r"(lo), "r"(pos) : "memory");
#endif
    if (pos[3] < 0)                         /* w < 0 */
        VU0_CLIP_NEG_W(clip);
    return clip & VU0_CLIP_MASK;
}

/* ======================================================================== */
/* CPU lighting (__pglShading)                                              */
/* ======================================================================== */

/* OpenGL-style lighting of n vertices (positions, normals: 3 floats each;
 * colors: optional u32 RGBA per vertex) into out (RGBA8 per vertex):
 *   base = emission + globalAmbient * ambient (material 0); with colors and
 *   colorMaterial the colour first goes into the material
 *   (gs_set_material_param) and base is recomputed, without colorMaterial
 *   colour / 255 is added to base;
 *   per light of lightList: directional (eyePosition.w == 0) or positional
 *   with 1 / (c + d (l + d q)) attenuation and an optional spot (cutoff
 *   != 180: -L.D below spotCosCutoff = 0, else pow by table or
 *   float_pow_encoded); ambient always, diffuse N.L and Blinn specular
 *   (N.H, H = L - V with V the eye vector (local viewer, eyeModel) or
 *   viewDir) when N.L > 0, pow by the material table or float_pow_encoded;
 *   out = clamp(rgb * colorScale, 0, 255), alpha clamp(a * colorScale, 0,
 *   colorScale).
 * Caller: __pglShading.
 * (Matching, 69.6%: 255.0 is a variable (retail divides by it with div.s
 * under -ffast-math, which turns a constant divisor into a product; the
 * 255.0 / 1.0 / 0.0 registers are spilled around the colorMaterial call).
 * Left: the FPU register allocation and the scheduling of the base colour
 * and the light loop; not searched.) */
// FUNC 0x002570C8 pgl_shade_vertices
void pgl_shade_vertices(GsContext *ctx, u32 *out, const float *pos,
                        const float *nrm, const u32 *colors, int n)
{
    GsMaterial *mat = &ctx->material[0];
    f32 scale = ctx->colorScale;
    f32 base[4];                            /* sp+0x00 */
    f32 normal[3];                          /* sp+0x10 */
    f32 alpha;                              /* sp+0x28 */
    f32 r, g, b, a;
    GsLight *lt;
    f32 k255 = 255.0f;

    base[0] = mat->emission.x + ctx->globalAmbient.x * mat->ambient.x;
    base[1] = mat->emission.y + ctx->globalAmbient.y * mat->ambient.y;
    base[2] = mat->emission.z + ctx->globalAmbient.z * mat->ambient.z;
    base[3] = mat->emission.w + ctx->globalAmbient.w * mat->ambient.w;
    while (n-- != 0) {
        if (colors != NULL && ctx->colorMaterial != 0) {
            gs_set_material_param(ctx->colorMaterialFaces, ctx->colorMaterialKind, *colors);
            base[0] = mat->emission.x + ctx->globalAmbient.x * mat->ambient.x;
            base[1] = mat->emission.y + ctx->globalAmbient.y * mat->ambient.y;
            base[2] = mat->emission.z + ctx->globalAmbient.z * mat->ambient.z;
            base[3] = mat->emission.w + ctx->globalAmbient.w * mat->ambient.w;
        }
        r = base[0];
        g = base[1];
        b = base[2];
        a = base[3];
        if (colors != NULL && ctx->colorMaterial == 0) {
            u32 c = *colors;

            r = base[0] + (f32)(c & 0xFF) / k255;
            g = base[1] + (f32)((c >> 8) & 0xFF) / k255;
            b = base[2] + (f32)((c >> 16) & 0xFF) / k255;
            a = base[3] + (f32)((c >> 24) & 0xFF) / k255;
        }
        normal[0] = nrm[0];
        normal[1] = nrm[1];
        normal[2] = nrm[2];
        nrm += 3;
        alpha = a * scale;
        for (lt = ctx->lightList; lt != NULL; lt = lt->next) {
            f32 lx, ly, lz;                 /* to the light (unit) */
            f32 atten, spot;
            f32 ambR, ambG, ambB;
            f32 nDotL, lightScale;

            if (lt->eyePosition.w == 0.0f) {            /* directional */
                atten = 1.0f;
                lx = lt->eyePosition.x;
                ly = lt->eyePosition.y;
                spot = 1.0f;
                lz = lt->eyePosition.z;
            } else {
                f32 dist;

                lx = lt->eyePosition.x - pos[0];
                ly = lt->eyePosition.y - pos[1];
                lz = lt->eyePosition.z - pos[2];
                dist = __builtin_sqrtf(lx * lx + ly * ly + lz * lz);
                if (0.0001f < dist) {
                    f32 inv = 1.0f / dist;

                    lz *= inv;
                    lx *= inv;
                    ly *= inv;
                }
                atten = 1.0f / (lt->constAtten + dist * (lt->linearAtten + dist * lt->quadAtten));
                spot = 1.0f;
                if (lt->spotCutoff != PGL_SPOT_CUTOFF_NONE) {
                    f32 spotDot = -lx * lt->spotDir.x + -ly * lt->spotDir.y + -lz * lt->spotDir.z;

                    spot = 0.0f;
                    if (!(spotDot <= 0.0f) && !(spotDot < lt->spotCosCutoff)) {
                        if (lt->spotTable != NULL)
                            spot = lt->spotTable[(s32)(ee_clampf(spotDot, 0.0f, 1.0f)
                                                       * lt->spotTableScale)];
                        else
                            spot = float_pow_encoded(spotDot, lt->spotExpEnc);
                    }
                }
            }
            nDotL = normal[0] * lx + normal[1] * ly + normal[2] * lz;
            ambG = mat->ambient.y * lt->ambient.y;
            ambR = mat->ambient.x * lt->ambient.x;
            ambB = mat->ambient.z * lt->ambient.z;
            if (nDotL <= 0.0f) {
                lightScale = atten * spot;
                b += lightScale * ambB;
                r += lightScale * ambR;
                g += lightScale * ambG;
            } else {
                f32 difR, difG, difB;
                f32 hx, hy, hz;
                f32 specR, specG, specB;
                f32 nDotH;

                difR = nDotL * mat->diffuse.x * lt->diffuse.x;
                difG = nDotL * mat->diffuse.y * lt->diffuse.y;
                difB = nDotL * mat->diffuse.z * lt->diffuse.z;
                if (ctx->lightFlags & PGL_LF_LOCAL_VIEWER) {
                    f32 vx = pos[0] - ctx->eyeModel.x;
                    f32 vy = pos[1] - ctx->eyeModel.y;
                    f32 vz = pos[2] - ctx->eyeModel.z;
                    f32 len = __builtin_sqrtf(vx * vx + vy * vy + vz * vz);

                    if (0.0001f < len) {
                        f32 inv = 1.0f / len;

                        vz *= inv;
                        vx *= inv;
                        vy *= inv;
                    }
                    hx = lx - vx;
                    hy = ly - vy;
                    hz = lz - vz;
                } else {
                    hx = lx - ctx->viewDir.x;
                    hy = ly - ctx->viewDir.y;
                    hz = lz - ctx->viewDir.z;
                }
                nDotH = normal[0] * hx + normal[1] * hy + normal[2] * hz;
                if (nDotH <= 0.0f) {
                    specR = 0.0f;
                    specG = 0.0f;
                    specB = 0.0f;
                } else {
                    /* rsqrt.s: 1 / |H| */
                    nDotH = ee_minf(nDotH * (1.0f / __builtin_sqrtf(hx * hx + hy * hy + hz * hz)), 1.0f);
                    if (mat->x44.powTable != NULL)
                        nDotH = mat->x44.powTable[(s32)(nDotH * mat->x48.powTableScale + 0.5f)];
                    else
                        nDotH = float_pow_encoded(nDotH, mat->shininessEnc);
                    specG = nDotH * mat->specular.y * lt->specular.y;
                    specR = nDotH * mat->specular.x * lt->specular.x;
                    specB = nDotH * mat->specular.z * lt->specular.z;
                }
                lightScale = atten * spot;
                r += lightScale * (ambR + difR + specR);
                g += lightScale * (ambG + difG + specG);
                b += lightScale * (ambB + difB + specB);
            }
        }
        pos += 3;
        {
            s32 outR = (s32)ee_clampf(r * scale, 0.0f, k255);
            s32 outG = (s32)ee_clampf(g * scale, 0.0f, k255);
            s32 outB = (s32)ee_clampf(b * scale, 0.0f, k255);
            s32 outA = (s32)ee_clampf(alpha, 0.0f, scale);

            if (colors != NULL)
                colors++;
            *out = (((u32)outA << 24) | (u32)outR) | (((u32)outG << 8) | ((u32)outB << 16));
            out++;
        }
    }
}

/* Lights n vertices into a block taken from the end of the packet buffer
 * ((n + 3) / 4 qwords: one RGBA word per vertex) and returns it.  Prints
 * "__pglShading (): sorry." on every call.  Without normals the current
 * normal is replicated into the block itself (3 floats per vertex: writes
 * 12n bytes into the 4n-byte block, past its end - as in the binary).
 * Lighting mode 1 (the VU1 light-list mode) is "not yet implemented",
 * modes 0 and 2..4 are lit by pgl_shade_vertices, others do nothing.
 * Caller: pgl_light_vertices (lightFlags 0x80, PGL_CAP_CPU_LIGHTING).
 * (Matching, 89.3%: the lit cases first in the switch (retail's block
 * order); in the fill loop retail's loop.c keeps two address givs (q[3i]
 * / q[3i + 2] and q[3i + 1]), here one: 36 index forms, a 3-float struct,
 * a float[3] row pointer and a running index were tried.) */
// FUNC 0x00257910 __pglShading
u32 *__pglShading(GsContext *ctx, const float *pos, const float *nrm,
                  const u32 *colors, int n)
{
    u32 *out;

    printf("__pglShading (): sorry.\n");
    out = (u32 *)((u8 *)ctx->end - ((n + 3) >> 2) * 16);
    ctx->end = out;
    if (nrm == NULL) {
        f32 nx = ctx->normal[0];
        f32 ny = ctx->normal[1];
        f32 nz = ctx->normal[2];
        f32 *q = (f32 *)out;
        s32 i;

        nrm = q;
        for (i = 0; i < n; i++) {
            q[i * 3 + 0] = nx;
            q[i * 3 + 1] = ny;
            q[i * 3 + 2] = nz;
        }
    }
    switch (ctx->lightMode) {
    case 0:
    case 2:
    case 3:
    case 4:
        pgl_shade_vertices(ctx, out, pos, nrm, colors, n);
        return out;
    case 1:
        printf("%s: Sorry, Not yet implemented.\n", "__pglShading");
        return out;
    }
    return out;
}
