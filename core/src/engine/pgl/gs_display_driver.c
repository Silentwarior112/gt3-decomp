/* gs_display_driver.c - Polyphony's GS / VIF1 / GIF display driver
 * (G001, plain C), 0x00250A58..0x00254828.  The bootstrap links the same
 * driver (decomp/bootstrap src/gs_driver.cpp); the functions marked
 * "bootstrap" below are its machine code, the rest are additions of this
 * later version (GL-style state setters, sub-packets, IRQ markers, the
 * frame-buffer copy effects).  Types and the library's interface in
 * engine/pgl/G001_flare.h, the unit's own constants in gs_display_driver_internal.h.
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/gs_display_driver_internal.h"

/* .data 0x002FC1E8..0x002FC1FC, in definition order.  Right before it lies
 * the 0x1000-byte attribute stack at 0x002FB1E8 (u115's range: the
 * bootstrap's driver object has its 0x1000-byte buffer and then the DMA
 * channel table too).  After it, 0x002FC200..0x002FC220, libgraph's .data
 * (version tag, sceGsGParam s_gs_gparam): its object was linked between this
 * one and u118's register tables, as in the bootstrap; its code is in
 * function sections at the end of .text (u142). */

/* DMAC channel register bases for channels 0..2 (bootstrap s_gsDmaChan;
 * u118 uses them too).  12 bytes: .data even under -G8. */
EeDmaChanRegs *g_gsDmaChan[3] = {   /* 0x002FC1E8 */
    (EeDmaChanRegs *)0x10008000,    /* D0 VIF0 */
    (EeDmaChanRegs *)0x10009000,    /* D1 VIF1 */
    (EeDmaChanRegs *)0x1000A000,    /* D2 GIF  */
};

/* gfx_depth_func's PGL_DEPTHFUNC_* -> TEST.ZTST.  Sized by its initialiser:
 * egcs never puts an array defined with `[]' into small data (here 4 bytes,
 * .data, as retail; `[4]' would be .sdata under -G8). */
u8 g_gsDepthFuncZtst[] = {          /* 0x002FC1F8 */
    GS_ZTST_NEVER, GS_ZTST_ALWAYS, GS_ZTST_GREATER, GS_ZTST_GEQUAL,
};

// FUNC 0x00250A58 gfx_init_vu0_constants_spr
/* bootstrap 0x0100C6C8.  gfx_init_vu0_constants on the scratchpad context. */
void gfx_init_vu0_constants_spr(void)
{
    gfx_init_vu0_constants(GS_SPR_CTX);
}

// FUNC 0x00250A80 gfx_init_lights_material_spr
/* bootstrap 0x0100C6F0.  gfx_init_lights + gfx_init_material. */
void gfx_init_lights_material_spr(void)
{
    gfx_init_lights(GS_SPR_CTX);
    gfx_init_material(GS_SPR_CTX);
}

// FUNC 0x00250AB8 gfx_reset_texture_state_spr
/* bootstrap 0x0100C728. */
void gfx_reset_texture_state_spr(void)
{
    gfx_reset_texture_state(GS_SPR_CTX);
}

// FUNC 0x00250AE0 gfx_packet_is_open
/* Returns ctx->packetOpen (the byte only, not the sub-packet flag). */
u8 gfx_packet_is_open(void)
{
    return GS_SPR_CTX->packetOpen;
}

// FUNC 0x00250AF0 gfx_change_display_mode
/* bootstrap 0x0100C7E0.  If (inter, omode, ffmd) differ from the stored
 * bytes: store them, recompute the DISPLAY2/PMODE shadows and re-program the
 * CRT (tail jump to gfx_reset_graph_stored_mode). */
void gfx_change_display_mode(int inter, int omode, int ffmd)
{
    GfxDriverState *g = &g_gfx;

    if (g->inter == inter && g->omode == omode && g->ffmd == ffmd)
        return;
    gfx_store_display_mode(inter, omode, ffmd);
    gfx_update_display_regs(g);
    gfx_reset_graph_stored_mode();
}

// FUNC 0x00250B58 gfx_store_display_mode
/* bootstrap 0x0100C848.  Mode bytes and the SMODE2 shadow: interlaced 1
 * (field) / 3 (frame mode), non-interlaced 2. */
void gfx_store_display_mode(int inter, int omode, int ffmd)
{
    GfxDriverState *g = &g_gfx;

    g->omode = (u8)omode;
    g->inter = (u8)inter;
    g->ffmd  = (u8)ffmd;
    if (inter == SCE_GS_INTERLACE) {
        if (ffmd == SCE_GS_FRAME)
            g->smode2 = GS_SMODE2_FRAME;
        else
            g->smode2 = GS_SMODE2_FIELD;
    } else {
        g->smode2 = GS_SMODE2_NOINTER;
    }
}

// FUNC 0x00250BA0 gfx_reset_graph_stored_mode
/* bootstrap 0x0100C890.  sceGsResetGraph(5, inter, omode, ffmd) (tail jump). */
void gfx_reset_graph_stored_mode(void)
{
    GfxDriverState *g = &g_gfx;

    sceGsResetGraph(SCE_GS_RESET_MODE, g->inter, g->omode, g->ffmd);
}

// FUNC 0x00250BC0 gfx_set_display_enable
/* Read circuit 2 on/off: on == 0 writes PMODE = shadow & ~EN2, on == 1
 * PMODE = shadow | EN2, any other value does nothing.  Only the hardware
 * register is written, the shadow keeps its value. */
void gfx_set_display_enable(u8 on)
{
    if (on != 0) {
        if (on == 1)
            GS_REG_PMODE = g_gfx.pmode | GS_PMODE_EN2;
    } else {
        GS_REG_PMODE = g_gfx.pmode & ~(u64)GS_PMODE_EN2;
    }
}

// FUNC 0x00250C10 gfx_update_display_regs
/* bootstrap 0x0100C8B0.  DISPLAY2 shadow: MAGH = (screenW + 0x9FF) /
 * screenW (break 7 on zero), MAGV = (NTSC ? 0x1C0 : 0x200) / screenH,
 * DX = dispDX * MAGH + 0x28C, DY = (NTSC ? 0x19 : 0x24, doubled when
 * interlaced) + dispDY, DW = MAGH * dispW - 1, DH = MAGV * h - 1 (h doubled
 * when interlaced, halved in field mode); PMODE = 0x66, BGCOLOR = 0. */
void gfx_update_display_regs(GfxDriverState *g)
{
    s32 magh, magv, lines, top, dh;
    s32 dx, dy, dw;
    u64 display;

    lines = (g->omode == SCE_GS_NTSC) ? 0x1C0 : 0x200;     /* MAGV numerator */
    top   = (g->omode == SCE_GS_NTSC) ? 0x19 : 0x24;       /* first display line */
    dx = g->dispDX;
    dy = g->dispDY;
    dw = g->dispW;
    dh = g->dispH;
    magh = (s32)(g->screenW + 0x9FF) / (s32)g->screenW;
    magv = lines / (s32)g->screenH;
    if (g->inter == SCE_GS_INTERLACE) {
        top <<= 1;
        dh <<= 1;
    }
    if (g->ffmd == SCE_GS_FIELD)
        dh >>= 1;

    display = (u64)((u32)(dx * magh + 0x28C) & 0xFFFF)     /* DX */
            | ((u64)((u32)(top + dy) & 0xFFF) << 12)        /* DY */
            | ((u64)(s64)(magh - 1) << 23)                  /* MAGH */
            | ((u64)(s64)(magv - 1) << 27)                  /* MAGV */
            | ((u64)(s64)(magh * dw - 1) << 32)             /* DW */
            | ((u64)(s64)(magv * dh - 1) << 44);            /* DH */

    g_gfx.display2 = display;
    g_gfx.pmode    = GS_PMODE_INIT;
    g_gfx.bgcolor  = 0;
}

// FUNC 0x00250CF8 gfx_set_display_area
/* bootstrap 0x0100C998.  DISPFB DBX/DBY and the display size; tail-calls
 * gfx_update_display_regs. */
void gfx_set_display_area(int dbx, int dby, int w, int h)
{
    GfxDriverState *g = &g_gfx;

    g->dbx   = (u16)dbx;
    g->dby   = (u16)dby;
    g->dispW = (u16)w;
    g->dispH = (u16)h;
    gfx_update_display_regs(g);
}

// FUNC 0x00250D18 gfx_set_screen_size
/* bootstrap 0x0100C9B8. */
void gfx_set_screen_size(int w, int h)
{
    GfxDriverState *g = &g_gfx;

    g->screenW = (u16)w;
    g->screenH = (u16)h;
    gfx_update_display_regs(g);
}

// FUNC 0x00250D30 gfx_set_display_fb
/* bootstrap 0x0100C9D0.  Frame flip: DISPFB2 = FBP | FBW << 9 | (psm & 0xF)
 * << 15 | DBX << 32 | DBY << 43 (kept in the shadow), then PMODE, SMODE2,
 * DISPFB2, DISPLAY2, BGCOLOR from the shadows; sync.l.  fbh is unused (the
 * code overwrites $a3 in its first instruction). */
void gfx_set_display_fb(u32 fbWordAddr, int psm, int fbw, int fbh)
{
    GfxDriverState *g = &g_gfx;
    u64 dispfb;
    u64 pmode, smode2;

    dispfb = (u64)(fbWordAddr >> 11)                        /* FBP */
           | ((u64)((fbw >> 6) & 0x3F) << 9)                /* FBW */
           | ((u64)(psm & 0xF) << 15)                       /* PSM */
           | ((u64)(g->dbx & 0x7FF) << 32)                  /* DBX */
           | ((u64)(g->dby & 0x7FF) << 43);                 /* DBY */
    pmode  = g->pmode;
    smode2 = g->smode2;
    GS_REG_PMODE    = pmode;
    g->dispfb2      = dispfb;
    GS_REG_SMODE2   = smode2;
    GS_REG_DISPFB2  = dispfb;
    GS_REG_DISPLAY2 = g->display2;
    GS_REG_BGCOLOR  = g->bgcolor;
    SYNC_L();
}

// FUNC 0x00250DD8 gfx_dma_channel_init
/* bootstrap 0x0100CA78.  Channel ch: CHCR = chcr, MADR/TADR/ASR0/ASR1/SADR
 * = 0, busy/tagIrq = 0, mutex semaphore (max 1, init 1; the other SemaParam
 * fields are stack garbage, as in the binary). */
void gfx_dma_channel_init(int ch, u32 chcr)
{
    EeDmaChanRegs *d = g_gsDmaChan[ch];
    GfxDriverState *g = &g_gfx;
    struct SemaParam param;                 /* sp+0x00 */

    d->chcr = chcr;
    d->madr = 0;
    d->tadr = 0;
    d->asr0 = 0;
    d->asr1 = 0;
    d->sadr = 0;
    g->chan[ch].busy = 0;
    g->chan[ch].tagIrq = 0;
    param.initCount = 1;
    param.maxCount  = 1;
    g->chan[ch].sema = CreateSema(&param);
}

// FUNC 0x00250E68 gfx_dma_init
/* bootstrap 0x0100CB08.  Channels VIF0 / VIF1 / GIF (CHCR TTE / TTE|TIE /
 * TIE), D_STAT written back with its own channel 0..2 status and mask bits
 * (pending status cleared, set masks flipped off), D_PCR priority control
 * off, D_CTRL.DMAE, sync.l, dmac_channel_handler on channels 0..2 and
 * enabled.  The core calls the syscall-0x12 stub at 0x00285970
 * (symbols.csv: AddDmacHandler2) with three arguments, as AddDmacHandler. */
void gfx_dma_init(void)
{
    GfxDriverState *g = &g_gfx;

    gfx_dma_channel_init(DMAC_VIF0, D_CHCR_TTE);
    gfx_dma_channel_init(DMAC_VIF1, D_CHCR_TTE | D_CHCR_TIE);
    gfx_dma_channel_init(DMAC_GIF, D_CHCR_TIE);
    D_STAT = D_STAT & D_STAT_CIS_CIM_012;
    D_PCR  = D_PCR & D_PCR_KEEP;
    D_CTRL = D_CTRL | D_CTRL_DMAE;
    SYNC_L();
    g->dmacId[DMAC_VIF0] = AddDmacHandler(DMAC_VIF0, dmac_channel_handler, 0);
    g->dmacId[DMAC_VIF1] = AddDmacHandler(DMAC_VIF1, dmac_channel_handler, 0);
    g->dmacId[DMAC_GIF]  = AddDmacHandler(DMAC_GIF, dmac_channel_handler, 0);
    EnableDmac(DMAC_VIF0);
    EnableDmac(DMAC_VIF1);
    EnableDmac(DMAC_GIF);                   /* tail call */
}

// FUNC 0x00250F60 gfx_dma_lock
/* bootstrap 0x0100CC78.  WaitSema on channel ch's mutex (tail call). */
void gfx_dma_lock(int ch)
{
    WaitSema(g_gfx.chan[ch].sema);
}

// FUNC 0x00250F80 gfx_dma_unlock
/* bootstrap 0x0100CC98.  SignalSema on channel ch's mutex (tail call). */
void gfx_dma_unlock(int ch)
{
    SignalSema(g_gfx.chan[ch].sema);
}

// FUNC 0x00250FA0 gfx_dma_sync
/* bootstrap 0x0100CCB8.  Waits until channel ch is idle (0) or 0x78 sleeps
 * passed (-1: busy and waiters dropped, CHCR.STR cleared until it reads 0).
 * The stack node {next, tid} sits in the channel's waiter list meanwhile;
 * the DMAC and vblank handlers wake the listed threads. */
int gfx_dma_sync(int ch)
{
    GfxDriverState *g = &g_gfx;
    GfxWaitNode node;                       /* sp+0x00 */
    GfxWaitNode *it, *prev;
    int tid;
    int ret = 0;
    int sleepsLeft = 0x78;

    if (g->chan[ch].busy == 0)
        return 0;

    tid = GetThreadId();
    node.tid = tid;
    DIntr();
    node.next = g->chan[ch].waiters;
    g->chan[ch].waiters = &node;
    EIntr();

    while (g_gfx.chan[ch].busy != 0) {
        if (--sleepsLeft < 0) {
            EeDmaChanRegs *d;
            g->chan[ch].busy = 0;
            g->chan[ch].waiters = NULL;
            d = g_gsDmaChan[ch];
            if (d->chcr & D_CHCR_STR) {
                do {
                    d->chcr = d->chcr & ~D_CHCR_STR;
                } while (d->chcr & D_CHCR_STR);
            }
            ret = -1;
            break;
        }
        SleepThread();
    }

    DIntr();
    prev = NULL;
    for (it = g->chan[ch].waiters; it != NULL; it = it->next) {
        if (it == &node) {
            if (prev != NULL)
                prev->next = node.next;
            else
                g->chan[ch].waiters = node.next;
            break;
        }
        prev = it;
    }
    EIntr();
    CancelWakeupThread(tid);
    return ret;
}

// FUNC 0x00251150 gfx_dma_send
/* bootstrap 0x0100CE68.  Normal-mode transfer from memory: sync, busy = 1,
 * QWC, MADR, CHCR = (CHCR & ~(DIR | MOD | STR)) | STR | DIR; sync.l. */
void gfx_dma_send(int ch, const void *madr, u32 qwc)
{
    EeDmaChanRegs *d = g_gsDmaChan[ch];

    gfx_dma_sync(ch);
    g_gfx.chan[ch].busy = 1;
    d->qwc = qwc;
    d->madr = gs_dma_src_addr((u32)madr);
    d->chcr = (d->chcr & D_CHCR_MODE_KEEP) | D_CHCR_STR | D_CHCR_DIR_FROM_MEM;
    SYNC_L();
}

// FUNC 0x00251210 gfx_kick_frame_lists
/* bootstrap 0x0100CF28.  Kicks a finished frame (hdr, else ctx->frameHdr,
 * nothing if both NULL): locks and syncs channels 1 and 2, zeroes the
 * rendezvous counters, starts the VIF1 and GIF chains, unlocks. */
void gfx_kick_frame_lists(GsFrameHeader *hdr)
{
    GfxDriverState *g = &g_gfx;
    GsContext *ctx = GS_SPR_CTX;

    if (hdr == NULL) {
        hdr = ctx->frameHdr;
        if (hdr == NULL)
            return;
    }
    gfx_dma_lock(DMAC_VIF1);
    gfx_dma_lock(DMAC_GIF);
    gfx_dma_sync(DMAC_VIF1);
    gfx_dma_sync(DMAC_GIF);
    g->pathSync16 = 0;
    g->pathSync17 = 0;
    g->pathSync18 = 0;
    g->finishCount = 0;
    if (hdr->vif1Chain != NULL)
        dma_start_source_chain(DMAC_VIF1, hdr->vif1Chain);
    if (hdr->gifChain != NULL)
        dma_start_source_chain(DMAC_GIF, hdr->gifChain);
    gfx_dma_unlock(DMAC_VIF1);
    gfx_dma_unlock(DMAC_GIF);               /* tail call */
}

// FUNC 0x002512D0 vu_upload_microcode
/* bootstrap 0x0100CFE8.  Uploads byteSize bytes of VU microcode to vuAddr
 * through DMA channel ch with a stack source chain: per <= 0x100
 * instructions (dwords) a ref tag + FLUSHE + MPG, then an end tag; sync.l,
 * FlushCache(0), lock, start, sync, unlock.  Quirks: the SPR bit of a
 * scratchpad source is masked off again in the tag; no bound check on the
 * stack buffer. */
void vu_upload_microcode(int ch, u32 vuAddr, int byteSize, const void *code)
{
    u32 chain[0x100 / 4] __attribute__((aligned(16)));    /* sp+0x00 */
    u32 *tag;
    u32 src = gs_dma_src_addr((u32)code);
    s32 left;

    left = ((byteSize + 15) >> 4) << 1;     /* instructions, rounded up to qwords */
    tag = chain;
    while (left != 0) {
        s32 n = (left < 0x101) ? left : 0x100;             /* MPG NUM 0 = 256 */
        tag[0] = DMATAG_REF((u32)(n >> 1));
        tag[1] = src & 0x0FFFFFFF;
        tag[2] = VIF_FLUSHE;
        tag[3] = ((n & 0xFF) << 16) | VIF_MPG | vuAddr;
        src += (u32)n << 3;
        vuAddr += n;
        left -= n;
        tag += 4;
    }
    gs_put_tag(tag, DMATAG_END);
    SYNC_L();
    FlushCache(WRITEBACK_DCACHE);
    gfx_dma_lock(ch);
    dma_start_source_chain(ch, chain);
    gfx_dma_sync(ch);
    gfx_dma_unlock(ch);
}

// FUNC 0x002513E0 gfx_enable
/* glEnable-style switch, the counterpart of gfx_disable (109-entry jump
 * table @0x00344E60; 93 callers); the capabilities are G001_flare.h's
 * PGL_CAP_*:
 *   PATH3: if a packet is active emit MSKPATH3(0); path3Masked = 0
 *   TEXTURE / BLEND / FOG / ANTIALIAS: PRIM attr |= TME / ABE / FGE / AA1
 *   DEST_ALPHA: TEST.DATE = 1   ALPHA_TEST: TEST.ATE = 1   (TEST dirty)
 *   DEPTH_TEST: TEST.ZTE = 1, ZTST = zTestMethod; ZBUF.ZMSK = zMask;
 *      depthTest = 1
 *   DITHER: DTHE = 1            COLOR_CLAMP: COLCLAMP = 1
 *   TEXGEN, COLORGEN, COLOR_MATERIAL, FLAG24, FLAG27, TEX_MATRIX,
 *      NORMAL_MATRIX: their flag byte = 1
 *   CULL_FACE / VU1_PROG_33E: cullEnable / vu1ProgAlt = 1, then gs_select_vu1_prog
 *   SCALE: scaleEnable = 1, matrixDirty = 1
 *   CPU_LIGHTING: lightFlags |= 0x80   LIGHT0 + i: light i on, lightFlags |= 1
 *   0, 0x15..0x63, >= 0x6D: nothing.
 * Register changes mark the register dirty (both contexts).
 * Matching: retail aligns the case labels after BLEND and CPU_LIGHTING with
 * a nop, the trace of a loop note: those two statements were macros with a
 * do { } while (0) wrapper (an empty do-while after the statement gives the
 * same code).  DEPTH_TEST assigns the TEST / ZBUF bitfields (lwu of
 * zTestMethod: the u32 widened to the u64 field before the mask). */
void gfx_enable(u32 what)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (what) {
    case 0x00:
        break;
    case PGL_CAP_PATH3:
        if (GS_PACKET_ACTIVE(ctx))
            gfx_mskpath3(0);
        ctx->path3Masked = 0;
        break;
    case PGL_CAP_ALPHA_TEST:
        ctx->reg[GS_REG_TEST] |= GS_FIELD_MASK(GS_TEST_ATE);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
        break;
    case PGL_CAP_DEST_ALPHA:
        ctx->reg[GS_REG_TEST] |= GS_FIELD_MASK(GS_TEST_DATE);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
        break;
    case PGL_CAP_DEPTH_TEST:
        ((GsTest *)&ctx->reg[GS_REG_TEST])->ZTST = ctx->zTestMethod;
        ctx->depthTest = 1;
        ((GsZbuf *)&ctx->reg[GS_REG_ZBUF])->ZMSK = ctx->zMask;
        ((GsTest *)&ctx->reg[GS_REG_TEST])->ZTE = 1;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ZBUF) | GS_DIRTY_BOTH(GS_REG_TEST);
        break;
    case PGL_CAP_BLEND:
        do { ctx->primAttr |= GS_PRIM_ABE; } while (0);     /* a statement macro (see above) */
        break;
    case PGL_CAP_FOG:
        ctx->primAttr |= GS_PRIM_FGE;
        break;
    case PGL_CAP_ANTIALIAS:
        ctx->primAttr |= GS_PRIM_AA1;
        break;
    case PGL_CAP_TEXTURE:
        ctx->primAttr |= GS_PRIM_TME;
        break;
    case PGL_CAP_DITHER:
        ctx->gen[GS_GEN_DTHE] |= GS_DTHE_ON;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_DTHE);
        break;
    case PGL_CAP_TEXGEN:
        ctx->texGenEnable = 1;
        break;
    case PGL_CAP_COLORGEN:
        ctx->colorGenEnable = 1;
        break;
    case PGL_CAP_COLOR_CLAMP:
        ctx->gen[GS_GEN_COLCLAMP] |= GS_COLCLAMP_CLAMP;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_COLCLAMP);
        break;
    case PGL_CAP_COLOR_MATERIAL:
        ctx->colorMaterial = 1;
        break;
    case PGL_CAP_CPU_LIGHTING:
        do { ctx->lightFlags |= PGL_LF_CPU_LIGHTING; } while (0);   /* a statement macro */
        break;
    case PGL_CAP_LIGHT0 + 0: case PGL_CAP_LIGHT0 + 1:
    case PGL_CAP_LIGHT0 + 2: case PGL_CAP_LIGHT0 + 3:
    case PGL_CAP_LIGHT0 + 4: case PGL_CAP_LIGHT0 + 5:
    case PGL_CAP_LIGHT0 + 6: case PGL_CAP_LIGHT0 + 7:
        ctx->lights[what - PGL_CAP_LIGHT0].enable = 1;
        ctx->lightFlags |= PGL_LF_LIGHTS;
        break;
    case PGL_CAP_FLAG24:
        ctx->flag24 = 1;
        break;
    case PGL_CAP_CULL_FACE:
        ctx->cullEnable = 1;
        gs_select_vu1_prog(ctx);                /* 0x0025AA00, tail jump */
        return;
    case PGL_CAP_VU1_PROG_33E:
        ctx->vu1ProgAlt = 1;
        gs_select_vu1_prog(ctx);                /* 0x0025AA00, tail jump */
        return;
    case PGL_CAP_FLAG27:
        ctx->flag27 = 1;
        break;
    case PGL_CAP_TEX_MATRIX:
        ctx->texMatrixEnable = 1;
        break;
    case PGL_CAP_SCALE:
        ctx->scaleEnable = 1;
        ctx->matrixDirty = 1;
        break;
    case PGL_CAP_NORMAL_MATRIX:
        ctx->normalMtxEnable = 1;
        break;
    default:                                                /* 0x15..0x63, >= 0x6D */
        break;
    }
}

// FUNC 0x00251670 gfx_disable
/* bootstrap 0x0100D0F8.  glDisable-style switch (109-entry jump table), the
 * mirror of gfx_enable:
 *   PATH3: if a packet is active emit MSKPATH3(1); path3Masked = 1
 *   TEXTURE / BLEND / FOG / ANTIALIAS: PRIM attr &= ~TME / ~ABE / ~FGE / ~AA1
 *   DEST_ALPHA: TEST.DATE = 0   ALPHA_TEST: TEST.ATE = 0   (TEST dirty)
 *   DEPTH_TEST: TEST.ZTE = 1, ZTST = ALWAYS; ZBUF.ZMSK = 1; depthTest = 0
 *   DITHER: DTHE = 0            COLOR_CLAMP: COLCLAMP = 0
 *   the flag bytes = 0; CULL_FACE / VU1_PROG_33E then gs_select_vu1_prog
 *   SCALE: scaleEnable = 0, matrixDirty = 1
 *   CPU_LIGHTING: lightFlags &= 0x7F   LIGHT0 + i: light i off, lightFlags |= 1
 *   0, 0x15..0x63, >= 0x6D: nothing. */
void gfx_disable(u32 what)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (what) {
    case 0x00:
        break;
    case PGL_CAP_PATH3:
        if (GS_PACKET_ACTIVE(ctx))
            gfx_mskpath3(1);
        ctx->path3Masked = 1;
        break;
    case PGL_CAP_ALPHA_TEST:
        ctx->reg[GS_REG_TEST] &= ~GS_FIELD_MASK(GS_TEST_ATE);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
        break;
    case PGL_CAP_DEST_ALPHA:
        ctx->reg[GS_REG_TEST] &= ~GS_FIELD_MASK(GS_TEST_DATE);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
        break;
    case PGL_CAP_DEPTH_TEST: {
        u64 test = ctx->reg[GS_REG_TEST];
        u64 zbuf = ctx->reg[GS_REG_ZBUF];
        u64 dirty = ctx->dirty;
        test &= ~GS_FIELD_MASK(GS_TEST_ZTST);
        test |= GS_FIELD_VAL(GS_TEST_ZTST, GS_ZTST_ALWAYS);
        zbuf |= GS_FIELD_MASK(GS_ZBUF_ZMSK);
        test |= GS_FIELD_MASK(GS_TEST_ZTE);
        dirty |= GS_DIRTY_BOTH(GS_REG_ZBUF) | GS_DIRTY_BOTH(GS_REG_TEST);
        ctx->reg[GS_REG_ZBUF] = zbuf;
        ctx->dirty = dirty;
        ctx->reg[GS_REG_TEST] = test;
        ctx->depthTest = 0;
        break;
    }
    case PGL_CAP_BLEND:
        ctx->primAttr &= ~GS_PRIM_ABE;
        break;
    case PGL_CAP_FOG:
        ctx->primAttr &= ~GS_PRIM_FGE;
        break;
    case PGL_CAP_ANTIALIAS:
        ctx->primAttr &= ~GS_PRIM_AA1;
        break;
    case PGL_CAP_TEXTURE:
        ctx->primAttr &= ~GS_PRIM_TME;
        break;
    case PGL_CAP_DITHER:
        ctx->gen[GS_GEN_DTHE] &= ~(u64)GS_DTHE_ON;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_DTHE);
        break;
    case PGL_CAP_TEXGEN:
        ctx->texGenEnable = 0;
        break;
    case PGL_CAP_COLORGEN:
        ctx->colorGenEnable = 0;
        break;
    case PGL_CAP_COLOR_CLAMP:
        ctx->gen[GS_GEN_COLCLAMP] &= ~(u64)GS_COLCLAMP_CLAMP;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_COLCLAMP);
        break;
    case PGL_CAP_COLOR_MATERIAL:
        ctx->colorMaterial = 0;
        break;
    case PGL_CAP_CPU_LIGHTING:
        ctx->lightFlags &= ~PGL_LF_CPU_LIGHTING;
        break;
    case PGL_CAP_LIGHT0 + 0: case PGL_CAP_LIGHT0 + 1:
    case PGL_CAP_LIGHT0 + 2: case PGL_CAP_LIGHT0 + 3:
    case PGL_CAP_LIGHT0 + 4: case PGL_CAP_LIGHT0 + 5:
    case PGL_CAP_LIGHT0 + 6: case PGL_CAP_LIGHT0 + 7:
        ctx->lights[what - PGL_CAP_LIGHT0].enable = 0;
        ctx->lightFlags |= PGL_LF_LIGHTS;
        break;
    case PGL_CAP_FLAG24:
        ctx->flag24 = 0;
        break;
    case PGL_CAP_CULL_FACE:
        ctx->cullEnable = 0;
        gs_select_vu1_prog(ctx);                /* 0x0025AA00, tail jump */
        return;
    case PGL_CAP_VU1_PROG_33E:
        ctx->vu1ProgAlt = 0;
        gs_select_vu1_prog(ctx);                /* 0x0025AA00, tail jump */
        return;
    case PGL_CAP_FLAG27:
        ctx->flag27 = 0;
        break;
    case PGL_CAP_TEX_MATRIX:
        ctx->texMatrixEnable = 0;
        break;
    case PGL_CAP_SCALE:
        ctx->scaleEnable = 0;
        ctx->matrixDirty = 1;
        break;
    case PGL_CAP_NORMAL_MATRIX:
        ctx->normalMtxEnable = 0;
        break;
    default:                                                /* 0x15..0x63, >= 0x6D */
        break;
    }
}

// FUNC 0x002518A8 gfx_mskpath3
/* bootstrap 0x0100D330.  Switches to list 0 (VIF1), flushes dirty
 * registers, then appends DMA cnt (qwc 0) + VIF FLUSH + MSKPATH3(mask). */
void gfx_mskpath3(int mask)
{
    GsContext *ctx = GS_SPR_CTX;
    u8 masked = (u8)mask;
    u32 *p;

    gfx_select_list(PGL_LIST_VIF1);
    gs_flush_dirty_regs();
    p = ctx->cur;
    ctx->cur = p + 4;
    p[0] = DMATAG_CNT(0);
    p[1] = 0;
    p[2] = VIF_FLUSH;
    p[3] = (masked << 15) | VIF_MSKPATH3;
}

// FUNC 0x00251918 gfx_cull_face
/* glCullFace: cullFace = mode (PGL_CULL_*), then gs_select_vu1_prog (tail
 * jump).  (The bootstrap's name; see G001_flare.h.) */
void gfx_cull_face(u32 mode)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->cullFace = mode;
    gs_select_vu1_prog(ctx);                    /* 0x0025AA00, tail jump */
}

// FUNC 0x00251930 gfx_front_face
/* glFrontFace: frontFace = mode (PGL_FRONT_*; the rear-view mirror draws
 * with CW), then gs_select_vu1_prog (tail jump). */
void gfx_front_face(u32 mode)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->frontFace = mode;
    gs_select_vu1_prog(ctx);                    /* 0x0025AA00, tail jump */
}

// FUNC 0x00251948 gfx_set_packet_buffer
/* bootstrap 0x0100D3A0.  base, size, cur = segment-mapped start, end =
 * segment-mapped buf + size (NULL stays NULL); for an uncached buffer the
 * D-cache is written back first (sync.l + FlushCache(0), tail call). */
void gfx_set_packet_buffer(void *buf, u32 size)
{
    GsContext *ctx = GS_SPR_CTX;
    u8 *bufEnd;
    u32 cur, end;

    ctx->bufBase = (u8 *)buf;
    ctx->bufSize = size;
    cur = 0;
    if (buf != NULL)
        cur = DMA_ADDR(buf) | ctx->addrBits;
    ctx->cur = (u32 *)cur;
    bufEnd = (u8 *)buf + size;
    end = 0;
    if (bufEnd != NULL)
        end = DMA_ADDR(bufEnd) | ctx->addrBits;
    ctx->end = (u32 *)end;
    if (ctx->uncachedBuf != PGL_PKT_CACHED) {
        SYNC_L();
        FlushCache(WRITEBACK_DCACHE);       /* tail call */
    }
}

// FUNC 0x002519B8 gfx_get_packet_ptr
/* Returns ctx->cur.  No callers. */
u32 *gfx_get_packet_ptr(void)
{
    return GS_SPR_CTX->cur;
}

// FUNC 0x002519C8 gfx_get_packet_space
/* ctx->end - ctx->cur in bytes.  No callers. */
int gfx_get_packet_space(void)
{
    GsContext *ctx = GS_SPR_CTX;

    return (int)((u32)ctx->end - (u32)ctx->cur);
}

// FUNC 0x002519E0 gfx_set_packet_access
/* Changes how the packet buffer is addressed (PGL_PKT_*): cached (segment
 * bits 0), uncached (0x20000000), uncached-accelerated (0x30000000); other
 * values and the current mode do nothing.  With a packet open the cur /
 * end / tail0 / tail1 pointers are re-mapped to the new segment (NULL
 * stays NULL), the D-cache is written back when leaving cached mode, and
 * the mode is stored.  Quirk: with no packet open only addrBits changes -
 * uncachedBuf keeps the old mode. */
#define PKT_REMAP(p, bits) do {                                                    u32 mapped_ = 0;                                                            if ((p) != NULL)                                                                mapped_ = DMA_ADDR(p) | (bits);                                         (p) = (u32 *)mapped_;                                                   } while (0)

void gfx_set_packet_access(u32 mode)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 old = ctx->uncachedBuf;

    if (old == mode)
        return;
    switch (mode) {
    case PGL_PKT_UNCACHED:
        ctx->addrBits = SEG_UNCACHED;
        break;
    case PGL_PKT_CACHED:
        ctx->addrBits = SEG_CACHED;
        break;
    case PGL_PKT_UNCACHED_ACCEL:
        ctx->addrBits = SEG_UNCACHED_ACCEL;
        break;
    default:
        return;
    }

    if (ctx->packetOpen == 0)
        return;

    PKT_REMAP(ctx->cur, ctx->addrBits);
    PKT_REMAP(ctx->end, ctx->addrBits);
    PKT_REMAP(ctx->tail0, ctx->addrBits);
    PKT_REMAP(ctx->tail1, ctx->addrBits);
    if (old == PGL_PKT_CACHED) {
        SYNC_L();
        FlushCache(WRITEBACK_DCACHE);
    }
    ctx->uncachedBuf = mode;
}

// FUNC 0x00251B10 gfx_begin_packet
/* bootstrap 0x0100D438.  Opens a frame packet at ctx->cur (NULL if one is
 * already open):
 *   cur+0x00  GsFrameHeader {vif1Chain = cur+0x20, gifChain = cur+0x10}
 *   cur+0x10  reserved first tag of the GIF list (tail1)
 *   cur+0x20  VIF1 list: DMA cnt (qwc 1) + BASE 0 + OFFSET 0x1B3 (the VU1
 *             double buffer), then STCYCL 4/4, STMOD 0, STMASK 0xC0C0C0C0
 * cur = +0x40; all registers dirty; list 0 current; the statistics zeroed;
 * packetOpen = 1. */
GsFrameHeader *gfx_begin_packet(void)
{
    GsContext *ctx = GS_SPR_CTX;
    GsFrameHeader *hdr;
    u32 *p;

    if (ctx->packetOpen)
        return NULL;

    p = ctx->cur;
    ctx->frameHdr = (GsFrameHeader *)p;
    p += 4;
    hdr = ctx->frameHdr;
    hdr->gifChain = p;
    ctx->tail1 = p;
    p += 4;
    p[0] = DMATAG_CNT(1);
    p[1] = 0;
    p[2] = VIF_BASE(0);
    p[3] = VIF_OFFSET(VU1_BUF_OFFSET);
    p[4] = VIF_STCYCL(4, 4);
    p[5] = VIF_STMOD(0);
    p[6] = VIF_STMASK;
    p[7] = STMASK_DEFAULT;
    hdr->vif1Chain = p;
    ctx->nParts = 0;
    ctx->tail0 = NULL;
    ctx->curList = PGL_LIST_VIF1;
    ctx->enableList0 = GS_ENABLE_GEN_CTX1;
    ctx->enableList1 = 0;
    ctx->matrixDirty = 1;
    ctx->dirty = GS_DIRTY_ALL;
    ctx->enable = GS_ENABLE_GEN_CTX1;
    ctx->stmask = STMASK_DEFAULT;
    ctx->vu1MatrixSent = 0;
    ctx->halfHeight = 0;
    ctx->fieldShift = 0;
    ctx->nVerts = 0;
    ctx->nPrims = 0;
    ctx->packetOpen = 1;
    ctx->cur = p + 8;
    return hdr;
}

// FUNC 0x00251C08 gfx_end_packet
/* bootstrap 0x0100D530.  Closes an open sub-packet, then (packet open) DMA
 * end tags at the current list's write position and at the other list's
 * reserved tail; cur past the first; packetOpen = 0; D-cache written back
 * unless the buffer is uncached. */
void gfx_end_packet(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *end0, *end1, *cur;

    if (ctx->subPacketOpen)
        gfx_close_subpacket();
    if (ctx->packetOpen == 0)
        return;

    if (ctx->curList == PGL_LIST_VIF1) {
        end0 = ctx->cur;
        end1 = ctx->tail1;
        cur = end0;
    } else {
        end1 = ctx->cur;
        end0 = ctx->tail0;
        cur = end1;
    }
    gs_put_tag(end0, DMATAG_END);
    gs_put_tag(end1, DMATAG_END);
    ctx->cur = cur + 4;
    ctx->packetOpen = 0;
    if (ctx->uncachedBuf == PGL_PKT_CACHED) {
        SYNC_L();
        FlushCache(WRITEBACK_DCACHE);
    }
}

// FUNC 0x00251CB8 gfx_begin_subpacket
/* Starts a sub-packet (a pair of DMA call targets replayed later by
 * gfx_call_subpacket).  Closes an open one, flushes the dirty registers;
 * inside a frame packet the current write position becomes the current
 * list's tail and both tails are saved in frameHdr->save0/1 (restored by
 * gfx_close_subpacket), cur += 0x10.  Then at cur: the GsSubPacketHeader
 * (ctx->subHdr), the GIF list entry qword (tail1) and the VIF1 list entry:
 * DMA cnt (qwc 0) + STMASK 0xC0C0C0C0.  The emit mask becomes list 0's,
 * tail0 = NULL; when list 1 was current curList is reset to 0 and
 * gfx_select_list(1) re-enters list 1.  subPacketOpen = 1, written = 0,
 * vu1MatrixSent = 0; returns the header. */
GsSubPacketHeader *gfx_begin_subpacket(void)
{
    GsContext *ctx = GS_SPR_CTX;
    GsSubPacketHeader *sub;
    u32 *cur;

    if (ctx->subPacketOpen)
        gfx_close_subpacket();
    gs_flush_dirty_regs();
    cur = ctx->cur;
    if (ctx->packetOpen) {
        GsFrameHeader *hdr;
        if (ctx->curList == PGL_LIST_VIF1)
            ctx->tail0 = cur;
        else
            ctx->tail1 = cur;
        hdr = ctx->frameHdr;
        cur += 4;
        hdr->save0 = ctx->tail0;
        hdr->save1 = ctx->tail1;
    }
    ctx->subHdr = (GsSubPacketHeader *)cur;
    cur += 4;
    sub = ctx->subHdr;
    sub->gifChain = cur;
    ctx->tail1 = cur;
    cur += 4;
    ctx->enable = ctx->enableList0;
    sub->vif1Chain = cur;
    cur[0] = DMATAG_CNT(0);
    cur[2] = VIF_STMASK;
    cur[1] = 0;
    cur[3] = STMASK_DEFAULT;
    cur += 4;
    ctx->stmask = STMASK_DEFAULT;
    ctx->cur = cur;
    ctx->tail0 = NULL;
    if (ctx->curList == PGL_LIST_GIF) {
        ctx->curList = PGL_LIST_VIF1;
        gfx_select_list(PGL_LIST_GIF);
        sub = ctx->subHdr;
    }
    ctx->subPacketOpen = 1;
    ctx->vu1MatrixSent = 0;
    ctx->written = 0;
    return sub;
}

// FUNC 0x00251DB8 gfx_close_subpacket
/* bootstrap 0x0100D5E0.  Ends the sub-packet opened by gfx_begin_subpacket:
 * dirty = written (also saved in the header), both lists terminated with
 * DMA ret tags, subPacketOpen = 0; inside a frame packet the list tails are
 * reloaded from frameHdr->save0/1 (segment-mapped) and a next tag to the
 * new cur is written into the current list's tail. */
void gfx_close_subpacket(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *cur, *ret0, *ret1, *link;
    u32 tail0, tail1;
    u64 written;

    if (ctx->subPacketOpen == 0)
        return;

    written = ctx->written;
    ctx->dirty = written;
    ctx->subHdr->written = written;
    cur = ctx->cur;
    if (ctx->curList == PGL_LIST_VIF1) {
        ret0 = cur;
        ret1 = ctx->tail1;
    } else {
        ret0 = ctx->tail0;
        ret1 = cur;
    }
    gs_put_tag(ret0, DMATAG_RET);
    gs_put_tag(ret1, DMATAG_RET);
    cur += 4;
    ctx->cur = cur;
    ctx->subPacketOpen = 0;
    if (ctx->packetOpen == 0)
        return;

    tail0 = 0;
    if (ctx->frameHdr->save0 != NULL)
        tail0 = DMA_ADDR(ctx->frameHdr->save0) | ctx->addrBits;
    ctx->tail0 = (u32 *)tail0;
    tail1 = 0;
    if (ctx->frameHdr->save1 != NULL)
        tail1 = DMA_ADDR(ctx->frameHdr->save1) | ctx->addrBits;
    ctx->tail1 = (u32 *)tail1;
    link = (u32 *)(ctx->curList ? tail1 : tail0);
    gs_put_next_tag(link, cur);
}

// FUNC 0x00251EB8 gfx_call_subpacket
/* Replays a sub-packet inside the open frame packet (nothing otherwise):
 * on list 1 a DMA call tag to sp->gifChain, on list 0 (after flushing the
 * dirty registers) a call tag to sp->vif1Chain with VIF FLUSH; dirty =
 * sp->written (the registers the sub-packet changed are re-sent
 * afterwards); then the previous list is selected again (tail call). */
void gfx_call_subpacket(const GsSubPacketHeader *sp)
{
    GsContext *ctx = GS_SPR_CTX;
    int prevList;

    if (ctx->packetOpen == 0)
        return;

    prevList = ctx->curList;
    gfx_select_list(PGL_LIST_GIF);
    {
        u32 *p = ctx->cur;
        p[0] = DMATAG_CALL;
        p[1] = DMA_ADDR(sp->gifChain);
        p[2] = 0;
        p[3] = 0;
        ctx->cur = p + 4;
    }
    gfx_select_list(PGL_LIST_VIF1);
    gs_flush_dirty_regs();
    {
        u32 *p = ctx->cur;
        p[0] = DMATAG_CALL;
        p[1] = DMA_ADDR(sp->vif1Chain);
        p[2] = VIF_FLUSH;
        p[3] = VIF_NOP;
        ctx->cur = p + 4;
        ctx->dirty = sp->written;
    }
    gfx_select_list(prevList);              /* tail call */
}

// FUNC 0x00251FA8 gfx_select_list
/* Switches the packet writer to list `which` (PGL_LIST_VIF1 / _GIF);
 * returns the previous list.  No-op if already current or when switching to
 * the GIF list while PATH3 is masked; without an active packet only the
 * byte changes.  Otherwise the write position becomes the old list's
 * reserved tail slot, a next tag (-> cur + 0x10) goes into the new list's
 * tail, cur += 0x10 and the emit masks are swapped.  (The bootstrap's
 * 0x0100D6E0.)  The GIF -> VIF1 half is at 0x00252030 (code of this
 * function; a data word at 0x00351184 equal to that address made it a false
 * start, ref/not_starts.txt). */
int gfx_select_list(int which)
{
    GsContext *ctx = GS_SPR_CTX;
    int prev = ctx->curList;
    u32 *cur;

    if (which == prev)
        return prev;
    if (ctx->path3Masked != 0 && which == PGL_LIST_GIF)
        return prev;
    if (!GS_PACKET_ACTIVE(ctx)) {
        ctx->curList = (u8)which;
        return prev;
    }

    cur = ctx->cur;
    if (ctx->curList == PGL_LIST_VIF1) {
        ctx->tail0 = cur;
        cur += 4;
        ctx->enableList0 = ctx->enable;
        ctx->enable = ctx->enableList1;
        gs_put_next_tag(ctx->tail1, cur);
    } else {
        ctx->tail1 = cur;               /* 0x0025202C: GIF -> VIF1 */
        cur += 4;
        ctx->enableList1 = ctx->enable;
        ctx->enable = ctx->enableList0;
        gs_put_next_tag(ctx->tail0, cur);
    }
    ctx->cur = cur;
    ctx->curList = (u8)which;
    return prev;
}

// FUNC 0x00252078 gfx_put_irq_marker
/* Appends an interrupt marker (PGL_MARK_WAIT / _SIGNAL) to the current list
 * of the active packet (NULL without one) and returns a pointer to the word
 * holding `code`:
 *   VIF1 list: DMA cnt (qwc 0), VIF NOP, NOP, then word 3 = VIF FLUSH with
 *           the i-bit and code (the NUM field carries the marker bits) ->
 *           returns &word 3;
 *   GIF list: for a SIGNAL first an empty GIF packet (DMA cnt qwc 1 +
 *           GIFtag NLOOP 0 EOP, A+D), then a DMA cnt tag with the IRQ bit
 *           and code (QWC = code's low half, the marker bits in the tag's
 *           upper half, CHCR.TAG for the handler) -> returns &word 0.
 * u118's VIF1 i-bit and DMAC handlers step g_gfx.pathSync*. */
u32 *gfx_put_irq_marker(u32 code)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p;

    if (!GS_PACKET_ACTIVE(ctx))
        return NULL;
    p = ctx->cur;
    if (ctx->curList == PGL_LIST_VIF1) {
        p[0] = DMATAG_CNT(0);
        p[3] = code | VIF_IBIT | VIF_FLUSH;
        p[1] = VIF_NOP;
        p[2] = VIF_NOP;
        ctx->cur = p + 4;
        p += 3;
    } else {
        if (code & PGL_MARK_SIGNAL) {
            u32 *q = p + 4;
            gs_put_tag(p, DMATAG_CNT(1));
            GS_PUT_AD(q, GIF_NREG(1) | GIF_EOP, GIF_REG_AD);    /* NLOOP 0 */
            p = q;
        }
        ctx->cur = p + 4;
        gs_put_tag(p, code | DMATAG_IRQ | DMATAG_CNT(0));
    }
    return p;
}

// FUNC 0x00252140 gfx_sync_pipeline
/* bootstrap 0x0100D7B0.  Waits for the whole render path with forced
 * recovery, one shared budget of 0x1000000 polls: gfx_dma_sync(1) and (2);
 * VIF1_STAT FQC / VPS (over budget VIF1_FBRST = RST each poll); VU1 busy
 * (VPU_STAT $vi29 VBS1; over budget FBRST($vi28) |= RS1); GIF_STAT APATH
 * (over budget GIF_CTRL = RST, GIF_MODE = IMT). */
void gfx_sync_pipeline(void)
{
    s32 budget = 0x1000000;
    u32 vpuStat;

    gfx_dma_sync(DMAC_VIF1);
    gfx_dma_sync(DMAC_GIF);

    if (VIF1_STAT & VIF1_STAT_BUSY) {
        do {
            if (--budget < 0)
                VIF1_FBRST = VIF_FBRST_RST;
        } while (VIF1_STAT & VIF1_STAT_BUSY);
    }

    __asm__ __volatile__("cfc2 %0, $vi29" : "=r"(vpuStat));
    if (vpuStat & VPU_STAT_VBS1) {
        do {
            if (--budget < 0) {
                u32 fbrst;
                __asm__ __volatile__(
                    "cfc2   %0, $vi28\n\t"
                    "ori    %0, %0, 0x200\n\t"          /* FBRST_RS1 */
                    "ctc2   %0, $vi28\n\t"
                    : "=&r"(fbrst) : : "memory");
            }
            __asm__ __volatile__("cfc2 %0, $vi29" : "=r"(vpuStat));
        } while (vpuStat & VPU_STAT_VBS1);
    }

    if (GIF_STAT & GIF_STAT_APATH) {
        do {
            if (--budget < 0) {
                GIF_CTRL = GIF_CTRL_RST;
                GIF_MODE = GIF_MODE_IMT;
            }
        } while (GIF_STAT & GIF_STAT_APATH);
    }
}

// FUNC 0x00252260 gfx_wait_vblank_event
/* bootstrap 0x0100D8D0.  Sleeps until the vblank counter changes (stack
 * node in g_gfx.vsyncWaiters, woken by the vblank handler), unlinks the
 * node and CancelWakeupThread(tid). */
void gfx_wait_vblank_event(void)
{
    GfxDriverState *g = &g_gfx;
    GfxWaitNode node;                       /* sp+0x00 */
    GfxWaitNode *it, *prev;
    u64 seen;
    int tid;

    tid = GetThreadId();
    node.tid = tid;
    DIntr();
    node.next = g->vsyncWaiters;
    g->vsyncWaiters = &node;
    seen = g->vblankCount;
    EIntr();

    while (seen == g->vblankCount)
        SleepThread();

    DIntr();
    prev = NULL;
    for (it = g->vsyncWaiters; it != NULL; it = it->next) {
        if (it == &node) {
            if (prev != NULL)
                prev->next = node.next;
            else
                g->vsyncWaiters = node.next;
            break;
        }
        prev = it;
    }
    EIntr();
    CancelWakeupThread(tid);
}

// FUNC 0x00252338 gfx_vsync
/* bootstrap 0x0100D9A8.  Frame pacing.  n == 0: returns the vblank count.
 * n > 0: waits until n-1 vblanks passed since the last call, then for one
 * more vblank; returns the vblanks elapsed.  n < 0: returns 1 at once if -n
 * vblanks already passed, else waits for them and returns 0 (unsigned
 * 64-bit compares).  Then lastVsync = count, ctx->field = g.field when
 * interlaced (else 0). */
int gfx_vsync(int n)
{
    GfxDriverState *g = &g_gfx;
    GsContext *ctx = GS_SPR_CTX;
    u64 last;
    int ret;

    if (n == 0)
        return (s32)g->vblankCount;

    last = g->lastVsync;
    if (n > 0) {
        u64 minElapsed = (u64)(s64)(n - 1);
        while (g->vblankCount - last < minElapsed)
            gfx_wait_vblank_event();
        gfx_wait_vblank_event();
        ret = (s32)(g->vblankCount - last);
    } else {
        u64 minElapsed;
        last = g->lastVsync;
        minElapsed = (u64)(s64)(s32)(0u - (u32)n);          /* negu */
        if (g->vblankCount - last >= minElapsed) {
            ret = 1;
        } else {
            while (g->vblankCount - last < minElapsed)
                gfx_wait_vblank_event();
            ret = 0;
        }
    }

    g->lastVsync = g->vblankCount;
    if (g->inter == SCE_GS_INTERLACE)
        ctx->field = (s8)g->field;
    else
        ctx->field = 0;
    return ret;
}

// FUNC 0x00252460 gfx_dma_sync_frame
/* gfx_dma_sync(1); gfx_dma_sync(2) (tail call): waits for both frame
 * chains (VIF1 and GIF). */
void gfx_dma_sync_frame(void)
{
    gfx_dma_sync(DMAC_VIF1);
    gfx_dma_sync(DMAC_GIF);
}

// FUNC 0x00252480 gfx_set_target_b
/* Describes the secondary render target tgtB (no register shadow is
 * touched): base = addr >> 6, fbw = w >> 6 (rounded down, unlike
 * gfx_set_draw_buffer), psm, w, h, xoff/yoff = (0x800 - w/2) << 4 /
 * (0x800 - h/2) << 4, xend/yend = offset + size * 16.  The member order
 * is gfx_set_draw_buffer's (the bootstrap's): it decides the store order. */
void gfx_set_target_b(u32 fbWordAddr, int psm, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 xoff = (0x800 - (w >> 1)) << 4;
    s32 yoff = (0x800 - (h >> 1)) << 4;

    ctx->tgtB.width  = w;
    ctx->tgtB.height = h;
    ctx->tgtB.xoff   = xoff;
    ctx->tgtB.yoff   = yoff;
    ctx->tgtB.xend   = xoff + (w << 4);
    ctx->tgtB.yend   = yoff + (h << 4);
    ctx->tgtB.base   = fbWordAddr >> 6;
    ctx->tgtB.fbw    = w >> 6;
    ctx->tgtB.psm    = psm;
}

// FUNC 0x002524E8 gfx_set_draw_buffer
/* Sets the draw buffer (target A): FRAME_1 shadow FBP = addr >> 11, FBW =
 * (w + 63) >> 6, PSM; XYOFFSET OFX/OFY = (0x800 - w/2) << 4 / (0x800 -
 * h/2) << 4; FRAME|XYOFFSET dirty in both contexts; halfHeight = 0; tgtA =
 * {addr >> 6, fbw, psm, w, h, xoff, yoff, xoff + w*16, yoff + h*16}; then
 * gfx_set_draw_area(0, 0, w, h) (jump).  (The bootstrap's 0x0100DAD0.)
 * The code from 0x00252520 on is part of this function (a data word at
 * 0x0035118C equal to that address made it a false start,
 * ref/not_starts.txt).  The statement order is the bootstrap's (exact
 * there and here): tgtA first, then the shadows. */
void gfx_set_draw_buffer(u32 fbWordAddr, int psm, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 xoff = (0x800 - (w >> 1)) << 4;
    s32 yoff = (0x800 - (h >> 1)) << 4;
    s32 fbw = (w + 0x3F) >> 6;

    ctx->tgtA.width  = w;
    ctx->tgtA.height = h;
    ctx->tgtA.xoff   = xoff;
    ctx->tgtA.yoff   = yoff;
    ctx->tgtA.xend   = xoff + (w << 4);
    ctx->tgtA.yend   = yoff + (h << 4);
    ctx->tgtA.base   = fbWordAddr >> 6;
    ctx->tgtA.fbw    = fbw;
    ctx->tgtA.psm    = psm;
    SHADOW_U16(ctx->reg[GS_REG_FRAME])[0] = fbWordAddr >> 11;           /* FBP */
    SHADOW_U8(ctx->reg[GS_REG_FRAME])[2] = fbw;                         /* FBW */
    SHADOW_U8(ctx->reg[GS_REG_FRAME])[3] = psm;                         /* PSM */
    ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFX = xoff;
    ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = yoff;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_FRAME) | GS_DIRTY_BOTH(GS_REG_XYOFFSET);
    ctx->halfHeight = 0;
    gfx_set_draw_area(0, 0, w, h);          /* j 0x00252BB8 */
}

// FUNC 0x002525A0 gs_update_depth_range
/* Recomputes the VU1 depth transform (the z lanes of the viewport vectors)
 * from the depth range (glDepthRange near / far), the format's depthScale
 * and the two offsets:
 *   viewScale.z  = (near - far) * depthScale * 0.5
 *   viewCenter.z = (1 - (near + far) * 0.5 + depthBias) * depthScale
 *                  + (depthOffset + 1)
 * vu1MatrixSent = 0 (re-send the constants). */
void gs_update_depth_range(GsContext *ctx)
{
    f32 far = ctx->depthFar;
    f32 near = ctx->depthNear;
    f32 mid = (near + far) * 0.5f;
    f32 offset = ctx->depthOffset + 1.0f;
    f32 scale = ctx->depthScale;

    ctx->vu1MatrixSent = 0;
    ctx->viewScale.z = (near - far) * scale * 0.5f;
    ctx->viewCenter.z = ((1.0f - mid) + ctx->depthBias) * scale + offset;
}

// FUNC 0x00252600 gfx_set_depth_buffer
/* Sets the Z buffer: depthScale = the largest Z of the format (PSMZ32:
 * 4294836224.0 = 2^32 - 2^17, PSMZ24: 16776191.0, PSMZ16 / 16S: 65531.0;
 * other formats keep the old scale); ZBUF shadow ZBP = (addr >> 11) & 0x1FF,
 * PSM = psm & 0xF; spriteZF = (int)depthScale | 0xFF000000 (Z, F 0xFF); ZBUF
 * dirty (both contexts); then gs_update_depth_range (tail jump). */
void gfx_set_depth_buffer(u32 zbWordAddr, int psm)
{
    GsContext *ctx = GS_SPR_CTX;

    switch ((u32)psm) {
    case SCE_GS_PSMZ32:
        ctx->depthScale = 4294836224.0f;    /* 0x4F7FFE00, gp-0x6F6C */
        break;
    case SCE_GS_PSMZ24:
        ctx->depthScale = 16776191.0f;      /* 0x4B7FFBFF, gp-0x6F68 */
        break;
    case SCE_GS_PSMZ16:
    case SCE_GS_PSMZ16S:
        ctx->depthScale = 65531.0f;         /* 0x477FFB00, gp-0x6F64 */
        break;
    }
    ctx->spriteZF = (u32)(s32)ctx->depthScale | 0xFF000000;
    ((GsZbuf *)&ctx->reg[GS_REG_ZBUF])->ZBP = zbWordAddr >> 11;
    ((GsZbuf *)&ctx->reg[GS_REG_ZBUF])->PSM = psm;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ZBUF);
    gs_update_depth_range(ctx);
}

// FUNC 0x002526D0 gfx_depth_func
/* zTestMethod = g_gsDepthFuncZtst[func] (PGL_DEPTHFUNC_*; no range check);
 * while depth testing is on it goes straight into TEST.ZTST (TEST dirty,
 * both contexts). */
void gfx_depth_func(int func)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 ztst = g_gsDepthFuncZtst[func];

    ctx->zTestMethod = ztst;
    if (ctx->depthTest) {
        ctx->reg[GS_REG_TEST] = GS_FIELD_SET(ctx->reg[GS_REG_TEST], GS_TEST_ZTST, ztst);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
    }
}

// FUNC 0x00252730 gfx_alpha_func
/* Alpha test: TEST.ATST = atst & 7 (PGL_ATEST_*), TEST.AREF =
 * (int)clamp(ref * 128, 0, 255) (ref 1.0 = 0x80); TEST dirty (both
 * contexts). */
void gfx_alpha_func(int atst, float ref)
{
    GsContext *ctx = GS_SPR_CTX;

    ((GsTest *)&ctx->reg[GS_REG_TEST])->ATST = atst;
    ref = ee_clampf(ref * 128.0f, 0.0f, 255.0f);
    ((GsTest *)&ctx->reg[GS_REG_TEST])->AREF = (s32)ref;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
}

// FUNC 0x002527A8 gfx_alpha_fail
/* TEST.AFAIL = afail & 3 (PGL_AFAIL_*); TEST dirty (both contexts). */
void gfx_alpha_fail(int afail)
{
    GsContext *ctx = GS_SPR_CTX;

    ((GsTest *)&ctx->reg[GS_REG_TEST])->AFAIL = afail;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
}

// FUNC 0x002527E8 gfx_blend_func
/* ALPHA shadow: byte 0 (the A/B/C/D selectors, PGL_BLEND_SEL) = abcd, FIX
 * (byte 4) = (int)clamp(fix * 128, 0, 255); ALPHA dirty (both contexts). */
void gfx_blend_func(int abcd, float fix)
{
    GsContext *ctx = GS_SPR_CTX;

    ((GsAlpha *)&ctx->reg[GS_REG_ALPHA])->ABCD = abcd;
    ((GsAlpha *)&ctx->reg[GS_REG_ALPHA])->FIX = (s32)ee_clampf(fix * 128.0f, 0.0f, 255.0f);
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ALPHA);
}

// FUNC 0x00252838 gfx_dest_alpha_func
/* TEST.DATM: PGL_ATEST_GEQUAL sets it (pass where the frame buffer alpha
 * bit is 1), PGL_ATEST_LESS clears it (pass on 0), other values leave it;
 * TEST is marked dirty in every case.  `ref' (gfx_alpha_func's second
 * parameter) is not read; the three callers pass 0.5 in $f12. */
void gfx_dest_alpha_func(int mode, float ref)
{
    GsContext *ctx = GS_SPR_CTX;

    if (mode == PGL_ATEST_GEQUAL)
        ctx->reg[GS_REG_TEST] |= GS_FIELD_MASK(GS_TEST_DATM);
    else if (mode == PGL_ATEST_LESS)
        ctx->reg[GS_REG_TEST] &= ~GS_FIELD_MASK(GS_TEST_DATM);
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
}

// FUNC 0x00252890 gfx_alpha_func_raw
/* Alpha test with a raw reference: TEST.ATST = atst & 7, TEST.AREF = ref;
 * TEST dirty (both contexts). */
void gfx_alpha_func_raw(int atst, u8 ref)
{
    GsContext *ctx = GS_SPR_CTX;

    ((GsTest *)&ctx->reg[GS_REG_TEST])->ATST = atst;
    ((GsTest *)&ctx->reg[GS_REG_TEST])->AREF = ref;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEST);
}

// FUNC 0x002528E0 gfx_blend_func_raw
/* ALPHA byte 0 = abcd, FIX = fix; ALPHA dirty (both contexts). */
void gfx_blend_func_raw(u8 abcd, u8 fix)
{
    GsContext *ctx = GS_SPR_CTX;

    SHADOW_U8(ctx->reg[GS_REG_ALPHA])[0] = abcd;
    SHADOW_U8(ctx->reg[GS_REG_ALPHA])[4] = fix;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ALPHA);
}

// FUNC 0x00252908 gfx_color_mask
/* glColorMask: each non-zero channel flag enables writes of its byte
 * (r 0xFF, g 0xFF00, b 0xFF0000, a 0xFF000000); tail-calls
 * gfx_set_frame_write_mask with the mask of writable bits. */
void gfx_color_mask(u8 r, u8 g, u8 b, u8 a)
{
    u32 writable = 0;

    if (r)
        writable |= 0xFF;
    if (g)
        writable |= 0xFF00;
    if (b)
        writable |= 0xFF0000;
    if (a)
        writable |= 0xFF000000;
    gfx_set_frame_write_mask(writable);
}

// FUNC 0x00252950 gfx_set_frame_write_mask
/* FRAME.FBMSK (the shadow's high word) = ~mask (mask = bits to write);
 * FRAME dirty (both contexts). */
void gfx_set_frame_write_mask(u32 mask)
{
    GsContext *ctx = GS_SPR_CTX;

    SHADOW_U32(ctx->reg[GS_REG_FRAME])[1] = ~mask;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_FRAME);
}

// FUNC 0x00252978 gfx_depth_mask
/* glDepthMask: zMask = (write == 0); while depth testing is on it goes
 * straight into ZBUF.ZMSK (ZBUF dirty, both contexts). */
void gfx_depth_mask(u8 write)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->zMask = (write == 0);
    if (ctx->depthTest) {
        ((GsZbuf *)&ctx->reg[GS_REG_ZBUF])->ZMSK = ctx->zMask;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_ZBUF);
    }
}

// FUNC 0x002529E0 gfx_set_field_mode
/* Field (interlace) rendering mode (PGL_FIELD_*; 10-entry jump table
 * @0x003451E0): fieldShift = 0, then XYOFFSET.OFY (shadow) = tgtA.yoff + an
 * offset and halfHeight, XYOFFSET dirty (both contexts):
 *   FRAME 0: + 0, full height            HALF_CUR 1: + field * 8, half
 *   HALF_OTHER 2: + (1 - field) * 8, half HALF_EVEN 3: + 0, half
 *   HALF_ODD 4: + 8, half                 FRAME_SHIFT 9: + field * 8, full, fieldShift = 1
 *   5..8 and >= 10: nothing more.  (field = ctx->field, signed; 8 = half a
 * pixel in 12.4)
 * Matching: the field byte (written by the vblank handler) is read through
 * a volatile lvalue: retail's lbu + sll 24 / sra (a plain s8 read is lb). */
void gfx_set_field_mode(u32 mode)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->fieldShift = 0;
    switch (mode) {
    case PGL_FIELD_FRAME:
        ctx->halfHeight = 0;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff;
        break;
    case PGL_FIELD_HALF_CUR:
        ctx->halfHeight = 1;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff + GS_FIELD_VOLATILE(ctx) * 8;
        break;
    case PGL_FIELD_HALF_OTHER:
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff + (1 - GS_FIELD_VOLATILE(ctx)) * 8;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        ctx->halfHeight = 1;
        break;
    case PGL_FIELD_HALF_EVEN:
        ctx->halfHeight = 1;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff;
        break;
    case PGL_FIELD_HALF_ODD:
        ctx->halfHeight = 1;
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff + 8;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        break;
    case PGL_FIELD_FRAME_SHIFT:
        ctx->halfHeight = 0;
        ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY = ctx->tgtA.yoff + GS_FIELD_VOLATILE(ctx) * 8;
        ctx->fieldShift = 1;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
        break;
    }
}

// FUNC 0x00252B40 gfx_offset_xy
/* Shifts the XYOFFSET shadow by (dx, dy) pixels: OFX += (int)(dx * 16),
 * OFY += (int)(dy * (halfHeight ? 8 : 16)); XYOFFSET dirty (both
 * contexts).  Sub-pixel jitter of the draw buffer. */
void gfx_offset_xy(float dx, float dy)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 yUnit = 16.0f;

    if (ctx->halfHeight)
        yUnit = 8.0f;
    ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFX += (s32)(dx * 16.0f);
    ((GsXyoffset *)&ctx->reg[GS_REG_XYOFFSET])->OFY += (s32)(dy * yUnit);
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_XYOFFSET);
}

// FUNC 0x00252BB8 gfx_set_draw_area
/* bootstrap 0x0100DB88.  Draw window (y and h halved, arithmetic shift,
 * when halfHeight is set), then gs_update_scissor_and_viewport (tail). */
void gfx_set_draw_area(int x, int y, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;

    if (ctx->halfHeight) {
        y >>= 1;
        h >>= 1;
    }
    ctx->winX = x;
    ctx->winY = y;
    ctx->winW = w;
    ctx->winH = h;
    gs_update_scissor_and_viewport(ctx);
}

// FUNC 0x00252BE8 gs_update_scissor_and_viewport
/* bootstrap 0x0100DBB8.  SCISSOR shadow = {winX, winX+winW-1, winY,
 * winY+winH-1}, SCISSOR dirty (both contexts); viewport scale (winW/2,
 * -winH/2) and centre ((xoff + winX*16 + winW*8) / 16, (yoff + winY*16 +
 * winH*8) / 16) for the VU1 microcode; vu1MatrixSent = 0. */
void gs_update_scissor_and_viewport(GsContext *ctx)
{
    GsScissor *sc = (GsScissor *)&ctx->reg[GS_REG_SCISSOR];
    s32 x = ctx->winX, y = ctx->winY, w = ctx->winW, h = ctx->winH;

    sc->SCAX0 = x;
    sc->SCAY0 = y;
    sc->SCAX1 = x + w - 1;
    sc->SCAY1 = y + h - 1;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_SCISSOR);
    ctx->viewScale.x  = (f32)w * 0.5f;
    ctx->viewScale.y  = (f32)(-h) * 0.5f;
    ctx->viewCenter.x = (f32)((s32)ctx->tgtA.xoff + (x << 4) + (w << 3)) * 0.0625f;
    ctx->viewCenter.y = (f32)((s32)ctx->tgtA.yoff + (y << 4) + (h << 3)) * 0.0625f;
    ctx->vu1MatrixSent = 0;
}

// FUNC 0x00252D30 gfx_set_scissor
/* glScissor: SCISSOR shadow = {x, x+w-1, y, y+h-1} (y and h halved,
 * arithmetic shift, when halfHeight is set); SCISSOR dirty (both
 * contexts).  The viewport is not touched. */
void gfx_set_scissor(int x, int y, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    GsScissor *sc = (GsScissor *)&ctx->reg[GS_REG_SCISSOR];

    if (ctx->halfHeight) {
        y >>= 1;
        h >>= 1;
    }
    sc->SCAX0 = x;
    sc->SCAY0 = y;
    sc->SCAX1 = x + w - 1;
    sc->SCAY1 = y + h - 1;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_SCISSOR);
}

// FUNC 0x00252DF8 gfx_set_depth_offset
/* depthOffset = ofs, then gs_update_depth_range (tail jump). */
void gfx_set_depth_offset(float ofs)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->depthOffset = ofs;
    gs_update_depth_range(ctx);
}

// FUNC 0x00252E08 gs_set_depth_bias_at
/* Depth bias for a view-space z offset dz at the point (x, y, z), in VU0
 * macro mode: vf1 = (x, y, z, dz) (built in GPRs: pcpyld + qmtc2); vf2 = MV *
 * (x, y, z, 1), vf3 = vf2 with z += dz; both through the projection P and
 * divided by their w; depthBias = vf3.w - vf2.w (after the divide, so both
 * are ~1.0 and the result is ~0 - the z lanes (+0x8 / +0x18) were probably
 * meant; kept as in the binary); then gs_update_depth_range.  MV and P by
 * matrixMode: PGL_MODELVIEW: MV is vf28..vf31 and P is parked at VU0 qword
 * 8, PGL_PROJECTION: MV is parked at qword 4 and P is vf28..vf31.  The
 * argument order (dz first, in $f12) is the register order of the binary;
 * the only caller is the model-set command interpreter (u102).
 * The three blocks before the transform are statement macros (do { }
 * while (0)): the second matrix load's else label is 8-aligned in retail
 * (loop note), the loop notes keep the two identical vmove blocks from
 * being cross-jumped, and the vf1 block's loop weight puts the float
 * arguments in GPRs from the start (mfc1 in the prologue, as retail).
 * Not exact (92.8%, was 81.5%): retail gives dz $a3 and z $a0 and copies dz
 * to $v0 for the shift, here z and dz trade places (global-alloc
 * preferences of the four argument pseudos), so the shift/or order
 * differs and the first mfc1 is not hoisted above the $ra save.  Tried:
 * the bits taken at the top (casts, unions), the pack as an expression,
 * as gs_pack_u32x2 in both argument orders and both statement orders,
 * block-scoped lo / hi, an inline vf1 loader taking the four floats
 * (build/scratch/u116/s2/v_db*.c); the reference compiler gives the same. */
void gs_set_depth_bias_at(float dz, float x, float y, float z)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 clip[8] __attribute__((aligned(16)));   /* sp+0x00: vf2, vf3 */
    u8 mode = ctx->matrixMode;
    u64 lo, hi;

    do {
        if (mode == PGL_MODELVIEW) {
            __asm__ __volatile__(
                "vmove.xyzw   $vf20, $vf28\n\t"
                "vmove.xyzw   $vf21, $vf29\n\t"
                "vmove.xyzw   $vf22, $vf30\n\t"
                "vmove.xyzw   $vf23, $vf31\n\t"
                : : : "memory");
        } else {
            __asm__ __volatile__(
                "ctc2.i       %0, $vi10\n\t"
                "vlqi.xyzw    $vf20, ($vi10++)\n\t"
                "vlqi.xyzw    $vf21, ($vi10++)\n\t"
                "vlqi.xyzw    $vf22, ($vi10++)\n\t"
                "vlqi.xyzw    $vf23, ($vi10++)\n\t"
                "vnop\n\t"
                "vnop\n\t"
                : : "r"(VU0_MODELVIEW_QW) : "memory");
        }
    } while (0);
    __asm__ __volatile__("vnop" : : : "memory");
    do {
        hi = gs_pack_u32x2(F32_BITS(z), F32_BITS(dz));
        lo = gs_pack_u32x2(F32_BITS(x), F32_BITS(y));
        __asm__ __volatile__(
            "pcpyld       %0, %0, %1\n\t"
            "qmtc2.ni     %0, $vf1\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf20, $vf1x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf21, $vf1y\n\t"
            "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf1z\n\t"
            "vmaddw.xyzw  $vf2, $vf23, $vf0w\n\t"
            "vmove.xyw    $vf3, $vf2\n\t"
            "vaddw.z      $vf3, $vf2, $vf1w\n\t"
            : "+r"(hi) : "r"(lo) : "memory");
    } while (0);
    do {
        if (mode == PGL_PROJECTION) {
            __asm__ __volatile__(
                "vmove.xyzw   $vf20, $vf28\n\t"
                "vmove.xyzw   $vf21, $vf29\n\t"
                "vmove.xyzw   $vf22, $vf30\n\t"
                "vmove.xyzw   $vf23, $vf31\n\t"
                : : : "memory");
        } else {
            __asm__ __volatile__(
                "ctc2.i       %0, $vi10\n\t"
                "vlqi.xyzw    $vf20, ($vi10++)\n\t"
                "vlqi.xyzw    $vf21, ($vi10++)\n\t"
                "vlqi.xyzw    $vf22, ($vi10++)\n\t"
                "vlqi.xyzw    $vf23, ($vi10++)\n\t"
                "vnop\n\t"
                "vnop\n\t"
                : : "r"(VU0_PROJECTION_QW) : "memory");
        }
    } while (0);
    __asm__ __volatile__(
        "vnop\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
        "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf3x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf3y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf3z\n\t"
        "vmaddw.xyzw  $vf3, $vf23, $vf3w\n\t"
        "vdiv         " VU0_Q ", $vf0w, $vf2w\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf2, $vf2, " VU0_Q "\n\t"
        "vdiv         " VU0_Q ", $vf0w, $vf3w\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf3, $vf3, " VU0_Q "\n\t"
        "sqc2         $vf2, 0(%0)\n\t"
        "sqc2         $vf3, 16(%0)\n\t"
        : : "r"(clip) : "memory");
    ctx->depthBias = clip[7] - clip[3];
    gs_update_depth_range(ctx);
}

// FUNC 0x00252F58 gs_set_clear_color_f
/* clearColor = pack_rgba_floats_to_u32(r, g, b, a, 255.0). */
void gs_set_clear_color_f(float r, float g, float b, float a)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->clearColor = pack_rgba_floats_to_u32(r, g, b, a, 255.0f);
}

// FUNC 0x00252F90 gs_set_clear_color
/* clearColor = rgba (the bootstrap's 0x0100DD00). */
void gs_set_clear_color(u32 rgba)
{
    GS_SPR_CTX->clearColor = rgba;
}

// FUNC 0x00252FA0 gs_set_clear_depth
/* clearDepth = clamp(1 - d, 0, 1) (GL depth 0 = near maps to the largest
 * GS Z). */
void gs_set_clear_depth(float d)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->clearDepth = ee_clampf(1.0f - d, 0.0f, 1.0f);
}

// FUNC 0x00252FC8 gs_build_clear_packet
/* bootstrap 0x0100DD10.  Appends a clear of target `tgt`:
 *   DMA cnt + VIF FLUSH + DIRECT n
 *   GIFtag A+D x4: TEST_1 = test, ZBUF_1 = ZBUF shadow with ZMSK = zMask,
 *     PRIM = SPRITE, RGBAQ = {rgba, Q 1.0}
 *   GIFtag REGLIST EOP {XYZ3, XYZ2} x k: 64-pixel strips from tgt->xoff
 *     until (x & 0xFFFF) >= tgt->xend, full height, depth z
 * ZBUF_1 and TEST_1 are marked dirty so the normal state returns.
 * Not exact (96.7%; the bootstrap's copy 94.5%): retail keeps the tag
 * pointer in three registers (q -> a copy that the A+D tag is stored
 * through -> adTag, `daddu a0,t2; daddu t4,a0'); here q / adTag share one
 * and the strip loop's alignment nop and two increments follow from that.
 * Tried: the tag through a pointer copy (GS_PUT_ADV, a block-local t_),
 * adTag assigned before / after the tag, an inline returning the tag
 * pointer, the union view for the A+D qwords (build/scratch/u116/s2/
 * v_cp*.c, 130 forms); the reference compiler gives the same. */
void gs_build_clear_packet(GsContext *ctx, u64 test, u32 rgba, u32 z,
                           const GsRenderTarget *tgt)
{
    u32 *pkt = ctx->cur;                    /* DMA tag / VIF codes */
    u32 *q = pkt + 4;                       /* write pointer        */
    u32 *adTag = q;                         /* A+D GIF tag          */
    u32 *reglistTag;                        /* REGLIST GIF tag      */
    u32 xy0, xy1, xend, qwc, nloop;

    GS_PUT_AD(q, GIF_NREG(1) | 4, GIF_REG_AD);              /* NLOOP 4, no EOP */
    GS_PUT_AD(q, test, SCE_GS_TEST_1);
    GS_PUT_AD(q, (ctx->reg[GS_REG_ZBUF] & ~GS_FIELD_MASK(GS_ZBUF_ZMSK))
                 | ((u64)ctx->zMask << 32), SCE_GS_ZBUF_1);    /* ZMSK = zMask (not masked to 1 bit) */
    GS_PUT_AD(q, GS_PRIM_SPRITE, SCE_GS_PRIM);
    GS_PUT_AD2(q, rgba, GS_RGBAQ_Q_ONE, SCE_GS_RGBAQ);

    xy0  = *(const u32a *)&tgt->xoff;                       /* xoff | yoff << 16 */
    xy1  = ((u32)tgt->xoff + 0x400) | ((u32)tgt->yend << 16);
    xend = tgt->xend;
    reglistTag = q;
    q += 4;
    do {
        GS_PUT_XYZ(q, xy0, z);
        GS_PUT_XYZ(q, xy1, z);
        xy0 += 0x400;                                       /* 64 pixels in 12.4 */
        xy1 += 0x400;
    } while ((xy0 & 0xFFFF) < xend);

    qwc = (u32)((u8 *)q - (u8 *)adTag) >> 4;
    if (qwc != 0) {
        gs_put_cnt_direct(pkt, qwc);
        ctx->cur = pkt + 4 + qwc * 4;
    }
    nloop = ((u32)((u8 *)q - (u8 *)reglistTag) >> 4) - 1;
    do {
        PKT64(reglistTag, 0) = (u64)(u32)(nloop | GIF_EOP) | GIF_NREG(2) | GIF_FLG_REGLIST;
        PKT64(reglistTag, 2) = GIF_REGS_XYZ3_XYZ2;
    } while (0);
    ctx->dirty |= GS_DIRTY_CTX1(GS_REG_ZBUF) | GS_DIRTY_CTX1(GS_REG_TEST);
}

// FUNC 0x00253138 gs_clear_buffers
/* bootstrap 0x0100DE80.  Flushes dirty registers, then: COLOR / DEPTH
 * (PGL_CLEAR_*) clear target A with depth (int)(clearDepth * depthScale)
 * and clearColor, TEST = colour only -> 0, colour and depth -> ZTE + ZTST
 * ALWAYS, depth only -> that + alpha test NEVER with AFAIL ZB_ONLY;
 * TARGET_B re-points FRAME / SCISSOR / XYOFFSET_1 to target B and clears it
 * with clearColorB, TEST 0, Z 0. */
void gs_clear_buffers(int mask)
{
    GsContext *ctx = GS_SPR_CTX;

    if (mask == 0)
        return;
    gs_flush_dirty_regs();
    if (mask & (PGL_CLEAR_COLOR | PGL_CLEAR_DEPTH)) {
        u32 z = (u32)(s32)(ctx->clearDepth * ctx->depthScale);  /* cvt.w.s */
        u64 test;
        if ((mask & PGL_CLEAR_COLOR) == 0)
            test = GS_FIELD_VAL(GS_TEST_ZTST, GS_ZTST_ALWAYS) | GS_FIELD_MASK(GS_TEST_ZTE)
                 | GS_FIELD_VAL(GS_TEST_AFAIL, PGL_AFAIL_ZB_ONLY)
                 | GS_FIELD_MASK(GS_TEST_ATE);              /* 0x32001, ATST NEVER */
        else
            test = (mask & PGL_CLEAR_DEPTH)
                 ? GS_FIELD_VAL(GS_TEST_ZTST, GS_ZTST_ALWAYS) | GS_FIELD_MASK(GS_TEST_ZTE)
                 : 0;
        gs_build_clear_packet(ctx, test, ctx->clearColor, z, &ctx->tgtA);
    }
    if (mask & PGL_CLEAR_TARGET_B) {
        gs_emit_render_target_regs(ctx);
        gs_build_clear_packet(ctx, 0, ctx->clearColorB, 0, &ctx->tgtB);
    }
}

// FUNC 0x002531F8 gs_emit_render_target_regs
/* bootstrap 0x0100DF40.  Appends DMA cnt(4) + FLUSH + DIRECT 4 + GIFtag
 * (A+D x3, EOP): FRAME_1 = target B, SCISSOR_1 = the current scissor
 * rescaled from target A to target B (signed divides, break 7 if
 * tgtA.width == 0), XYOFFSET_1 = target B offsets; FRAME/XYOFFSET/SCISSOR
 * (ctx 1) dirty so the next flush restores target A.
 * The bootstrap's copy stayed at 96%; what makes it exact is the packet
 * writer: the SCISSOR qword through a copy of the write pointer
 * (GS_PUT_AD2U), the last qword through the pointer itself, so the
 * XYOFFSET base register is set inside the SCISSOR block, as retail. */
void gs_emit_render_target_regs(GsContext *ctx)
{
    GsScissor *sc = (GsScissor *)&ctx->reg[GS_REG_SCISSOR];
    s32 x0, x1, y0, y1;
    u32 frame;
    u32 *p, *q;

    frame = (ctx->tgtB.psm << 24) | (ctx->tgtB.fbw << 16) | (ctx->tgtB.base >> 5);   /* PSM, FBW, FBP */
    x0 = (s32)sc->SCAX0 * ctx->tgtB.width / ctx->tgtA.width;
    x1 = ((s32)sc->SCAX1 + 1) * ctx->tgtB.width / ctx->tgtA.width - 1;
    y0 = (s32)sc->SCAY0 * ctx->tgtB.height / ctx->tgtA.height;
    y1 = ((s32)sc->SCAY1 + 1) * ctx->tgtB.height / ctx->tgtA.height - 1;
    p = ctx->cur;
    gs_put_cnt_direct(p, 4);
    ctx->cur = p + 20;
    q = p + 4;
    GS_PUT_AD(q, GIF_NREG(1) | GIF_EOP | 3, GIF_REG_AD);    /* NLOOP 3 */
    GS_PUT_AD(q, (s32)frame, SCE_GS_FRAME_1);
    GS_PUT_AD2U(q, (x1 << 16) | x0, (y1 << 16) | y0, SCE_GS_SCISSOR_1);
    do {                                                    /* the last qword: through q itself */
        ((GsPktDword *)q)[0].w[0] = ctx->tgtB.xoff;
        ((GsPktDword *)q)[0].w[1] = ctx->tgtB.yoff;
        ((GsPktDword *)q)[1].d = SCE_GS_XYOFFSET_1;
    } while (0);
    ctx->dirty |= GS_DIRTY_CTX1(GS_REG_FRAME) | GS_DIRTY_CTX1(GS_REG_XYOFFSET)
                | GS_DIRTY_CTX1(GS_REG_SCISSOR);
}

// FUNC 0x00253358 gs_fill_target_b
/* Blends a flat white sprite over target B (64-pixel strips, full
 * height): DMA cnt + FLUSH + DIRECT, GIFtag A+D (NLOOP 5 + 2 per strip,
 * EOP): DTHE = 0, TEST_1 = 0, ALPHA_1 = {blend, FIX = (int)clamp(fix *
 * 128, 0, 255)}, PRIM = SPRITE|ABE, RGBAQ = 0xFFFFFFFF / Q 1.0, then
 * XYZ3/XYZ2 pairs.  Quirk: the XYZ2 data is the sign-extended 32-bit
 * (yend << 16 | x1), so its Z word is 0xFFFFFFFF whenever yend >= 0x8000
 * (the XYZ3 Z is 0).  DTHE and ALPHA_1/TEST_1 are marked dirty.  Used by
 * gfx_frame_effect PGL_FX_ADD_WHITE and PGL_FX_SCALE_B (the target is the
 * FRAME_1 the caller set). */
void gs_fill_target_b(GsContext *ctx, u32 blend, float fix)
{
    u32 *p, *q;
    s64 f;
    u64 prim = GS_PRIM_SPRITE | GS_PRIM_ABE;
    u64 xy0, xy1;
    s32 nloop, qwc;

    f = (s32)ee_clampf(fix * 128.0f, 0.0f, 255.0f);
    p = ctx->cur;
    q = p + 8;
    GS_PUT_AD(q, 0, SCE_GS_DTHE);                           /* DTHE = 0 */
    GS_PUT_AD(q, 0, SCE_GS_TEST_1);                         /* TEST_1 = 0 */
    GS_PUT_AD2(q, blend, f, SCE_GS_ALPHA_1);
    GS_PUT_AD(q, prim, SCE_GS_PRIM);
    GS_PUT_AD2(q, 0xFFFFFFFF, GS_RGBAQ_Q_ONE, SCE_GS_RGBAQ);   /* white */

    nloop = 5;
    xy0 = *(const u32a *)&ctx->tgtB.xoff;                   /* xoff | yoff << 16, Z 0 */
    xy1 = (ctx->tgtB.xoff + 0x400) | (ctx->tgtB.yend << 16);
    do {
        GS_PUT_ADV(q, xy0, SCE_GS_XYZ3);
        GS_PUT_ADV(q, xy1, SCE_GS_XYZ2);
        nloop += 2;
        xy0 += 0x400;                                       /* 64 pixels in 12.4 */
        xy1 += 0x400;
    } while ((xy0 & 0xFFFF) < ctx->tgtB.xend);

    qwc = nloop + 1;
    if (qwc != 0) {
        gs_put_cnt_direct(p, qwc);
        ctx->cur = p + 4 + qwc * 4;
    }
    do {
        PKT64U(p, 4) = (u64)(nloop | 0x8000) | GIF_NREG(1);    /* EOP */
        PKT64U(p, 6) = GIF_REG_AD;
    } while (0);
    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_DTHE)
                | GS_DIRTY_CTX1(GS_REG_ALPHA) | GS_DIRTY_CTX1(GS_REG_TEST);
}

// FUNC 0x002534B8 gs_copy_target
/* Draws target `src` as a texture over target `dst`'s area (64-pixel
 * sprite strips, FST UVs scaled from dst to src size with a +6 (0.375 px)
 * sample offset): DMA cnt + FLUSH + DIRECT, GIFtag A+D (EOP): TEXFLUSH,
 * DTHE = 0, TEST_1 = 0, ALPHA_1 = {blend, FIX = (int)clamp(fix * 128, 0,
 * 255)}, TEX1_1 = LCM (LOD 0, nearest), TEX0_1 = {src base, fbw, psm, TW
 * = TH = 10, TCC}, CLAMP_1 = REPEAT, PRIM = SPRITE|TME|ABE|FST, RGBAQ =
 * 0x80808080, then per strip UV/XYZ3/UV/XYZ2.  dy shifts the sprite's y
 * (12.4), dv the texture's v (10.4).  Divides are unsigned (break 7 on a
 * zero divisor).  TEX0/TEX1/CLAMP/TEST/ALPHA_1 and DTHE are marked dirty
 * (the FRAME_1 the caller chose stays).
 * The strip loop's packet stores go through the union view (alias set 0):
 * dst->xoff / xend are reloaded on every pass and PRE splits the back
 * edge (retail's `b' into the loop), so loop.c leaves the loop alone;
 * the widths are locals (kept in registers), the `+ dy' sits in the loop
 * (PRE hoists the add, not the shift); TEX0 is libgraph's SET_TEX0. */
void gs_copy_target(GsContext *ctx, const GsRenderTarget *dst,
                    const GsRenderTarget *src, u32 blend, int dy, int dv,
                    float fix)
{
    u64 prim = GS_PRIM_SPRITE | GS_PRIM_TME | GS_PRIM_ABE | GS_PRIM_FST;
    u32 *p;
    u32 *adTag, *q;
    s64 f;
    u32 x, x1, vTop, vBottom, y0, y1, qwc;
    u32 sw, dw;

    f = (s32)ee_clampf(fix * 128.0f, 0.0f, 255.0f);
    p = ctx->cur;
    adTag = p + 4;
    q = adTag + 4;
    GS_PUT_AD(q, 0, SCE_GS_TEXFLUSH);
    GS_PUT_AD(q, 0, SCE_GS_DTHE);                           /* DTHE = 0 */
    GS_PUT_AD(q, 0, SCE_GS_TEST_1);                         /* TEST_1 = 0 */
    GS_PUT_AD2(q, blend, f, SCE_GS_ALPHA_1);
    GS_PUT_AD(q, GS_FIELD_MASK(GS_TEX1_LCM), SCE_GS_TEX1_1);
    GS_PUT_AD(q, SCE_GS_SET_TEX0(src->base, src->fbw, src->psm, 10, 10, 1, 0, 0, 0, 0, 0, 0),
              SCE_GS_TEX0_1);                               /* 1024 x 1024, TCC, MODULATE */
    GS_PUT_AD(q, PGL_WRAP_REPEAT, SCE_GS_CLAMP_1);
    GS_PUT_AD(q, prim, SCE_GS_PRIM);
    GS_PUT_AD2(q, 0x80808080, GS_RGBAQ_Q_ONE, SCE_GS_RGBAQ);

    x  = dst->xoff;
    x1 = x + 0x400;
    y0 = dst->yoff;
    y1 = dst->yend;
    vTop    = (((u32)src->height * 6) / (u32)dst->height + dv) << 16;
    vBottom = (((u32)(y1 - y0 + 6) * (u32)src->height) / (u32)dst->height + dv) << 16;
    sw = src->width;
    dw = dst->width;
    do {
        u32 u0 = ((x - dst->xoff + 6) * sw) / dw;
        u32 u1 = ((x1 - dst->xoff + 6) * sw) / dw;
        GS_PUT_ADV(q, vTop | u0, SCE_GS_UV);
        GS_PUT_ADV(q, ((y0 + dy) << 16) | x, SCE_GS_XYZ3);
        GS_PUT_ADV(q, vBottom | u1, SCE_GS_UV);
        GS_PUT_ADV(q, ((y1 + dy) << 16) | x1, SCE_GS_XYZ2);
        x += 0x400;                                         /* 64 pixels in 12.4 */
        x1 += 0x400;
    } while (x < dst->xend);

    qwc = (u32)((u8 *)q - (u8 *)adTag) >> 4;
    if (qwc != 0) {
        gs_put_cnt_direct(p, qwc);
        ctx->cur = p + 4 + qwc * 4;
    }
    do {
        PKT64(adTag, 0) = (u64)(s32)((qwc - 1) | GIF_EOP) | GIF_NREG(1);    /* NLOOP qwc - 1 */
        PKT64(adTag, 2) = GIF_REG_AD;
    } while (0);
    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_DTHE)
                | GS_DIRTY_CTX1(GS_REG_TEX0) | GS_DIRTY_CTX1(GS_REG_TEX1)
                | GS_DIRTY_CTX1(GS_REG_CLAMP) | GS_DIRTY_CTX1(GS_REG_TEST)
                | GS_DIRTY_CTX1(GS_REG_ALPHA);
}

// FUNC 0x002537A0 gs_emit_xyoffset_integer
/* Appends DMA cnt(2) + FLUSH + DIRECT 2 + GIFtag (A+D x1, EOP) setting
 * XYOFFSET_1 = the shadow with the sub-pixel bits of OFX/OFY cleared;
 * XYOFFSET_1 dirty (restored on the next flush). */
void gs_emit_xyoffset_integer(GsContext *ctx)
{
    u32 *p = ctx->cur;
    u32 *q;
    u64 xyoffset = ctx->reg[GS_REG_XYOFFSET] & 0xFFFFFFF0FFFFFFF0ull;

    gs_put_cnt_direct(p, 2);
    ctx->cur = p + 12;
    q = p + 4;
    GS_PUT_AD(q, GIF_NREG(1) | GIF_EOP | 1, GIF_REG_AD);    /* NLOOP 1 */
    GS_PUT_AD(q, xyoffset, SCE_GS_XYOFFSET_1);
    ctx->dirty |= GS_DIRTY_CTX1(GS_REG_XYOFFSET);
}

/* gfx_frame_effect's ALPHA selectors */
#define BLEND_ADD_FIX   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD)    /* 0x68 Cd + Cs * FIX */
#define BLEND_SUB_FIX   PGL_BLEND_SEL(PGL_SEL_ZERO, PGL_SEL_CS, PGL_SEL_FIX, PGL_SEL_CD)    /* 0x62 Cd - Cs * FIX */
#define BLEND_SRC_FIX   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_ZERO)  /* 0xA8 Cs * FIX */
#define BLEND_DST_FIX   PGL_BLEND_SEL(PGL_SEL_CD, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_ZERO)  /* 0xA9 Cd * FIX */
#define BLEND_MIX_FIX   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD)      /* 0x64 lerp(Cd, Cs, FIX) */
#define BLEND_MIX_ALPHA PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)       /* 0x44 lerp(Cd, Cs, As) */
/* one line in 12.4 (sprite y) and 10.4 (texture v) */
#define LINE_12_4       0x10
#define LINE_10_4       0x10

// FUNC 0x00253828 gfx_frame_effect
/* Full-screen frame-buffer effects between the draw buffer (tgtA) and the
 * secondary target (tgtB), after gfx_texflush + gs_flush_dirty_regs
 * (PGL_FX_*; 9-entry jump table @0x00345210).  Every kind but COPY_B_TO_A
 * first points FRAME_1 at B (gs_emit_render_target_regs).
 *   ADD_A_TO_B: B += A * amount (amount <= 0: B -= A * -amount)
 *   COPY_A_TO_B: B = A * amount        ADD_WHITE: B +/-= white * |amount|
 *   SCALE_B: B = B * amount             COPY_B_TO_A: A = B * amount
 *   MIX_A_INTO_B: B = lerp(B, A, amount)
 *   ALPHA_A_INTO_B: B = lerp(B, A, A's alpha)
 *   FIELD_BLEND, FIELD_BLEND_INV: B = A, then a half-weight copy offset by
 *      one line (dy / dv = +-1 line): lines r and r + 1 of A averaged; then
 *      a lerp by amount with A (unshifted or one line down).  Which lines
 *      pair depends on ctx->field and fieldShift; _INV is the other field (its
 *      field-0 case is FIELD_BLEND's field-1 case and back).  With fieldShift
 *      there is no final lerp (and FIELD_BLEND_INV's second copy has dy ==
 *      dv: B = A).
 *   >= 9: nothing. */
void gfx_frame_effect(u32 kind, float amount)
{
    GsContext *ctx = GS_SPR_CTX;

    gfx_texflush();
    gs_flush_dirty_regs();
    switch (kind) {
    case PGL_FX_ADD_A_TO_B:
        gs_emit_render_target_regs(ctx);
        if (0.0f < amount)
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_ADD_FIX, 0, 0, amount);
        else
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SUB_FIX, 0, 0, -amount);
        return;
    case PGL_FX_COPY_A_TO_B:
        gs_emit_render_target_regs(ctx);
        gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, 0, amount);
        return;
    case PGL_FX_ADD_WHITE:
        gs_emit_render_target_regs(ctx);
        if (0.0f < amount)
            gs_fill_target_b(ctx, BLEND_ADD_FIX, amount);
        else
            gs_fill_target_b(ctx, BLEND_SUB_FIX, -amount);
        return;
    case PGL_FX_SCALE_B:
        gs_emit_render_target_regs(ctx);
        gs_fill_target_b(ctx, BLEND_DST_FIX, amount);
        return;
    case PGL_FX_COPY_B_TO_A:
        gs_emit_xyoffset_integer(ctx);
        gs_copy_target(ctx, &ctx->tgtA, &ctx->tgtB, BLEND_SRC_FIX, 0, 0, amount);
        return;
    case PGL_FX_MIX_A_INTO_B:
        gs_emit_render_target_regs(ctx);
        gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, 0, amount);
        return;
    case PGL_FX_ALPHA_A_INTO_B:
        gs_emit_render_target_regs(ctx);
        gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_ALPHA, 0, 0, amount);
        return;
    case PGL_FX_FIELD_BLEND:
        gs_emit_render_target_regs(ctx);
        if (ctx->field == 0) {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, LINE_10_4, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, LINE_12_4, LINE_10_4, 0.5f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, 0, amount);
        } else if (ctx->fieldShift != 0) {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, 0, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, LINE_10_4, 0.5f);
        } else {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, 0, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, -LINE_12_4, 0, 0.5f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, LINE_10_4, amount);
        }
        return;
    case PGL_FX_FIELD_BLEND_INV:
        gs_emit_render_target_regs(ctx);
        if (ctx->field == 0) {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, 0, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, -LINE_12_4, 0, 0.5f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, LINE_10_4, amount);
        } else if (ctx->fieldShift != 0) {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, 0, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, LINE_12_4, LINE_10_4, 0.5f);
        } else {
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_SRC_FIX, 0, LINE_10_4, 1.0f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, LINE_12_4, LINE_10_4, 0.5f);
            gs_copy_target(ctx, &ctx->tgtB, &ctx->tgtA, BLEND_MIX_FIX, 0, 0, amount);
        }
        return;
    default:
        return;
    }
}

// FUNC 0x00253DD0 gfx_set_fog_color_f
/* FOGCOL shadow = (r, g, b) clamped to [0, 1] * 255 (FOGCOL dirty).  The
 * gs_get_fog_color call comes first: the previous FOGCOL stays in $v0 and
 * is the return value (the callers ignore it). */
u32 gfx_set_fog_color_f(float r, float g, float b)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 old = gs_get_fog_color();

    r = ee_clampf(r, 0.0f, 1.0f);
    g = ee_clampf(g, 0.0f, 1.0f);
    b = ee_clampf(b, 0.0f, 1.0f);
    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCR = (s32)(r * 255.0f);
    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCG = (s32)(g * 255.0f);
    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCB = (s32)(b * 255.0f);
    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_FOGCOL);
    return old;
}

// FUNC 0x00253E90 gfx_set_fog_color
/* FOGCOL shadow = rgb bytes (r | g << 8 | b << 16), FOGCOL dirty; returns
 * the previous FOGCOL (gs_get_fog_color, called first, as above). */
u32 gfx_set_fog_color(u32 rgb)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 old = gs_get_fog_color();

    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCR = rgb;
    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCG = rgb >> 8;
    ((GsFogcol *)&ctx->gen[GS_GEN_FOGCOL])->FCB = rgb >> 16;
    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_FOGCOL);
    return old;
}

// FUNC 0x00253EF0 gfx_set_fog_params
/* Stores start and amount as given (fogStart / fogAmount), then the VU1 fog
 * constants: limits.w = max(start, 0), with f = clamp(amount * 255, 0,
 * 255): viewScale.w = 255 - f, viewCenter.w = f + 0.1. */
void gfx_set_fog_params(float start, float amount)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->fogStart = start;
    ctx->fogAmount = amount;
    start = ee_maxf(start, 0.0f);
    amount = ee_clampf(amount * 255.0f, 0.0f, 255.0f);
    ctx->limits.w = start;
    ctx->viewScale.w = 255.0f - amount;
    ctx->viewCenter.w = amount + 0x1.99999Ap-4f;
}

// FUNC 0x00253F38 gfx_set_texture_buffer
/* Texture buffer of mip level `level` (7-entry jump table @0x00345240):
 * TBP = (addr >> 6) & 0x3FFF, TBW = ((width >> 6) & 0x3F):
 *   0: TEX0 TBP0 / TBW / PSM = psm & 0x3F, then gs_fix_tex0_tcc(ctx), TEX0
 *      dirty
 *   1..3: MIPTBP1 TBP1..3 / TBW1..3, MIPTBP1 dirty
 *   4..6: MIPTBP2 TBP4..6 / TBW4..6, MIPTBP2 dirty
 * (both contexts); other levels nothing.  psm is used by level 0 only. */
void gfx_set_texture_buffer(int level, u32 addr, int psm, int width)
{
    GsContext *ctx = GS_SPR_CTX;

    switch ((u32)level) {
    case 0:
        ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TBP0 = addr >> 6;
        ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TBW = width >> 6;
        ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->PSM = psm;
        gs_fix_tex0_tcc(ctx);
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
        break;
    case 1:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBP1 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBW1 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP1);
        break;
    case 2:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBP2 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBW2 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP1);
        break;
    case 3:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBP3 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP1])->TBW3 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP1);
        break;
    case 4:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBP1 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBW1 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP2);
        break;
    case 5:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBP2 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBW2 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP2);
        break;
    case 6:
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBP3 = addr >> 6;
        ((GsMiptbp *)&ctx->reg[GS_REG_MIPTBP2])->TBW3 = width >> 6;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_MIPTBP2);
        break;
    }
}

// FUNC 0x00254228 gfx_set_texture_size
/* TEX0.TW / TH = log2 of the texture size rounded up, from 1 (sizes <= 2)
 * to at most 10; TEX0 dirty (both contexts). */
void gfx_set_texture_size(int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    int log2W, log2H;

    for (log2W = 1; log2W < 10; log2W++)
        if ((1 << log2W) >= w)
            break;
    for (log2H = 1; log2H < 10; log2H++)
        if ((1 << log2H) >= h)
            break;
    ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TW = log2W;
    ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TH = log2H;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
}

// FUNC 0x00254308 gfx_set_texture_function
/* TEX0.TFX = tfx & 3 (PGL_TFX_*); TEX0 dirty (both contexts). */
void gfx_set_texture_function(int tfx)
{
    GsContext *ctx = GS_SPR_CTX;

    ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TFX = tfx;
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
}

// FUNC 0x00254358 gfx_tex_param
/* glTexParameter-like texture register fields (PGL_TEXPARAM_*; 14-entry
 * jump table @0x00345260), the register then dirty in both contexts:
 *   TCC, TFX: TEX0       MMIN, MMAG, LCM, MXL, L: TEX1      K: TEX1.K =
 *   value >> 4 (12 bits)  WMS, WMT: CLAMP   LOD_BIAS: lodBias = value (no
 *   register)  TA0, TA1 (bytes), AEM: TEXA (a general register)
 *   >= 14: nothing. */
void gfx_tex_param(int pname, int value)
{
    GsContext *ctx = GS_SPR_CTX;

    switch ((u32)pname) {
    case PGL_TEXPARAM_TCC:
        ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TCC = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
        break;
    case PGL_TEXPARAM_TFX:
        ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->TFX = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX0);
        break;
    case PGL_TEXPARAM_MMIN:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->MMIN = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_MMAG:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->MMAG = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_WMS:
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->WMS = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_CLAMP);
        break;
    case PGL_TEXPARAM_WMT:
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->WMT = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_CLAMP);
        break;
    case PGL_TEXPARAM_LCM:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->LCM = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_MXL:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->MXL = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_K:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->K = value >> 4;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_L:
        ((GsTex1 *)&ctx->reg[GS_REG_TEX1])->L = value;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX1);
        break;
    case PGL_TEXPARAM_LOD_BIAS:
        ctx->lodBias = value;
        break;
    case PGL_TEXPARAM_TA0:
        ((GsTexa *)&ctx->gen[GS_GEN_TEXA])->TA0 = value;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_TEXA);
        break;
    case PGL_TEXPARAM_TA1:
        ((GsTexa *)&ctx->gen[GS_GEN_TEXA])->TA1 = value;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_TEXA);
        break;
    case PGL_TEXPARAM_AEM:
        ((GsTexa *)&ctx->gen[GS_GEN_TEXA])->AEM = value;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_TEXA);
        break;
    default:
        break;
    }
}

// FUNC 0x002545D8 gfx_set_texture_region
/* CLAMP region bounds: PGL_REGION_U MINU / MAXU, PGL_REGION_V MINV / MAXV
 * (10 bits each; used with PGL_WRAP_REGION_*); CLAMP dirty (both
 * contexts); other axes nothing. */
void gfx_set_texture_region(int axis, u32 min, u32 max)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (axis) {
    case PGL_REGION_U:
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->MINU = min;
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->MAXU = max;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_CLAMP);
        break;
    case PGL_REGION_V:
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->MINV = min;
        ((GsClamp *)&ctx->reg[GS_REG_CLAMP])->MAXV = max;
        ctx->dirty |= GS_DIRTY_BOTH(GS_REG_CLAMP);
        break;
    }
}

// FUNC 0x00254690 gfx_texflush
/* Marks TEXFLUSH dirty: the next flush emits it. */
void gfx_texflush(void)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->dirty |= GS_DIRTY_GEN(GS_GEN_TEXFLUSH);
}

// FUNC 0x002546B0 gfx_set_clut_mode
/* TEX2 (PSM + CLUT offset CSA) for CLUT textures (PGL_CLUT_*; 5-entry jump
 * table @0x003452A0):
 *   FROM_TEX0: TEX2.PSM and CSA copied from TEX0
 *   T4: PSMT4, CSA from TEX0      T4_CSA0: PSMT4, CSA 0
 *   T8: PSMT8, CSA from TEX0      T8_CSA0: PSMT8, CSA 0
 * TEX2 and TEX0 are marked dirty (both contexts) for any mode, also those
 * that change nothing. */
void gfx_set_clut_mode(int mode)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (mode) {
    case PGL_CLUT_FROM_TEX0:
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->PSM = ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->PSM;
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->CSA = ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->CSA;
        break;
    case PGL_CLUT_T4:
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->PSM = SCE_GS_PSMT4;
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->CSA = ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->CSA;
        break;
    case PGL_CLUT_T8:
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->PSM = SCE_GS_PSMT8;
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->CSA = ((GsTex0 *)&ctx->reg[GS_REG_TEX0])->CSA;
        break;
    case PGL_CLUT_T4_CSA0:
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->PSM = SCE_GS_PSMT4;
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->CSA = 0;
        break;
    case PGL_CLUT_T8_CSA0:
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->PSM = SCE_GS_PSMT8;
        ((GsTex2 *)&ctx->reg[GS_REG_TEX2])->CSA = 0;
        break;
    }
    ctx->dirty |= GS_DIRTY_BOTH(GS_REG_TEX2) | GS_DIRTY_BOTH(GS_REG_TEX0);
}
