/* ipu.c - the IPU command layer of the MPEG decoder (G002),
 * 0x00263688..0x00264E58: BDEC, CSC, BCLR, toIPU / fromIPU DMA stop and
 * restart, the FDEC bit reader, IPU init (SETIQ / SETVQ / SETTH), the
 * decoder callback table, the IPU / fromIPU interrupt handshake and the
 * VDEC table lookups.  Much of it follows Sony's libipu (sceIpuInit,
 * sceIpuStopDMA, sceIpuRestartDMA, sceIpuSync), written against the
 * registers directly.
 *
 * Waits on IPU_CTRL.BUSY come in two forms: a plain poll (IPU_SYNC) and
 * ipu_wait, which while the IPU is busy calls the "input exhausted"
 * callback whenever IPU_BP.IFC is 0, so a starved IPU gets more bitstream.
 *
 * The original builds register values in one local (r here: r = command;
 * r |= field; ...; PUT32(reg, r)) that is reused for the following reads
 * (IPU_BP polls, status), and loads the fromIPU CHCR address into a local
 * before DIntr: the retail register allocation shows both (matching pass,
 * ref/matching/u122/notes.md).
 */
#include "engine/media/ipu_internal.h"

/* .data, in the original's order (values from the original's bytes). */

/* The MPEG default intra quantiser matrix in zigzag order (ISO 13818-2
 * 6.3.11), then the flat non-intra matrix's row of 16s (sent four times). */
u8 g_ipu_default_iq[80] __attribute__((aligned(16))) = {   /* 0x002FF0D0 */
     8, 16, 16, 19, 16, 19, 22, 22,
    22, 22, 22, 22, 26, 24, 26, 27,
    27, 27, 26, 26, 26, 26, 27, 27,
    27, 29, 29, 29, 34, 34, 34, 29,
    29, 29, 27, 27, 29, 29, 32, 32,
    34, 34, 37, 38, 37, 35, 35, 34,
    35, 38, 38, 40, 40, 40, 48, 48,
    46, 46, 56, 56, 58, 69, 69, 83,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};

/* The VQCLUT for SETVQ (RGB555: greys, red 0x001F, green 0x03E0, blue
 * 0x7C00, white 0x7FFF). */
u16 g_ipu_vqclut[16] __attribute__((aligned(16))) = {      /* 0x002FF120 */
    0x0000, 0x0421, 0x0842, 0x03E0, 0x1084, 0x14A5, 0x18C6, 0x1CE7,
    0x001F, 0x2529, 0x294A, 0x7C00, 0x318C, 0x35AD, 0x7FFF, 0x39CE
};

/* The decoder callbacks (mpeg_set_callback); initialised: the original has
 * them in .data, not in a common. */
MpegCallbackSlot g_mpeg_callbacks[5] = { { 0 } };           /* 0x002FF140 */

/* .sbss / .bss: file statics (the original's are between other objects'
 * statics: u121's g_mpeg_thread_running before, ipu_decode_image's after). */
static s32 g_ipu_th0;               /* 0x003532C4 SETTH th0 (never written: 0) */
static s32 g_ipu_th1;               /* 0x003532C8 SETTH th1 (never written: 0) */
static IpuSyncState g_ipu_sync;     /* 0x004A7620 */

// FUNC 0x00263688 ipu_bdec
/* Decode one macroblock's blocks with BDEC into p->out (fromIPU DMA to the
 * scratchpad, 0x30 quadwords = 384 halfwords).  Returns IPU_BDEC_NOTHING
 * when the macroblock is neither intra nor coded, else after the DMA end
 * (CPCOND0): IPU_BDEC_START_CODE if IPU_CTRL.SCD, IPU_BDEC_VLC_ERROR if
 * IPU_CTRL.ECD, IPU_BDEC_OK otherwise.  Caller mpeg_video_decode_slice.
 * Retail keeps one register variable (r: the IPU_CTRL value, the IPU_BP
 * reads, the final status) in $s0, the command in its own, and the
 * fromIPU CHCR address set before DIntr (as ipu_csc_chunk). */
int ipu_bdec(IpuBdecParam *p)
{
    int intra = p->mb_type & 1;     /* macroblock_intra */
    u32 r, cmd, chcr;
    int timeout;

    if (!intra && !(p->mb_type & 2))    /* nor macroblock_pattern */
        return IPU_BDEC_NOTHING;

    r = IPU_CTRL_PCT(p->pict_type);
    if (p->alt_scan)
        r |= IPU_CTRL_AS;
    r |= IPU_CTRL_IDP(p->dc_prec);
    if (p->intra_vlc)
        r |= IPU_CTRL_IVF;
    if (p->q_scale_type)
        r |= IPU_CTRL_QST;
    ipu_wait();
    PUT32(IPU_CTRL, r);

    cmd = intra ? (IPU_CMD_BDEC | IPU_BDEC_MBI) : IPU_CMD_BDEC;
    if (p->dc_reset)
        cmd |= IPU_BDEC_DCR;
    cmd |= IPU_QSC(p->qscale);
    if (p->dct_type)
        cmd |= IPU_BDEC_DT;

    PUT32(D3_MADR, DMA_PHYS(p->out) | DMA_MADR_SPR);
    PUT32(D3_QWC, 0x30);
    IPU_SYNC();
    chcr = D3_CHCR;
    DIntr();
    dma_start_suspended(chcr);
    EIntr();
    PUT32(IPU_CMD, cmd);

    timeout = IPU_BDEC_TIMEOUT;
    while (IPU_BUSY() && --timeout > 0) {
        r = REG32(IPU_BP);
        if (((r >> 8) & 0xF) == 0)
            mpeg_feed_input();
    }
    if (timeout == 0)
        r = REG32(IPU_CTRL);
    WAIT_CPCOND0();
    r = REG32(IPU_CTRL);
    if (r & IPU_CTRL_SCD)
        return IPU_BDEC_START_CODE;
    return (r & IPU_CTRL_ECD) ? IPU_BDEC_VLC_ERROR : IPU_BDEC_OK;
}

// FUNC 0x00263938 ipu_csc_rgb32
/* Colour-convert nmb macroblocks of 8-bit YCbCr (MpegMB, 0x180 bytes each)
 * at src into RGBA32 (0x400 bytes each) at dst, at most 768 macroblocks per
 * CSC, between the "stop DMA" and "restart DMA" callbacks (the bitstream DMA
 * shares the IPU).  (? 768: the fromIPU QWC is 0xC000 then; the CSC limit
 * would be 2047.)  Caller mpeg_player_on_picture (callback slot 3). */
void ipu_csc_rgb32(u32 dst, u32 src, int nmb)
{
    int chunks, rest, i;

    mpeg_cb_stop_dma();
    chunks = nmb / 768;
    rest = nmb % 768;
    for (i = 0; i < chunks; i++) {
        ipu_csc_chunk(dst, src, 768);
        dst += 768 * 0x400;
        src += 768 * sizeof(MpegMB);
    }
    if (rest != 0)
        ipu_csc_chunk(dst, src, rest);
    mpeg_cb_restart_dma();
}

// FUNC 0x002639F0 ipu_csc_chunk
/* One CSC (RGB32, no dither) of nmb macroblocks: BCLR, toIPU src (normal
 * mode), fromIPU dst, arm the interrupt handshake, start both channels,
 * issue CSC, sleep until the IPU interrupt and the fromIPU DMA end. */
void ipu_csc_chunk(u32 dst, u32 src, int nmb)
{
    u32 cmd = IPU_CMD_CSC;
    u32 chcr;
    u32 r;

    cmd |= nmb & 0x7FF;

    ipu_bclr(0);
    PUT32(D4_CHCR, REG32(D4_CHCR) & ~CHCR_MOD_MASK);
    PUT32(D4_MADR, DMA_PHYS(src));
    r = (u32)(nmb * sizeof(MpegMB)) >> 4;
    PUT32(D4_QWC, r);
    IPU_SYNC();
    FlushCache(WRITEBACK_DCACHE);
    SYNC_L();
    DIntr();
    dma_start_suspended(D4_CHCR);
    EIntr();
    PUT32(D3_MADR, DMA_PHYS(dst));
    r = (u32)(nmb << 10) >> 4;              /* 0x400 bytes per macroblock */
    PUT32(D3_QWC, r);
    ipu_sync_arm();
    chcr = D3_CHCR;     /* retail keeps it in $s0 from before DIntr */
    DIntr();
    dma_start_suspended(chcr);
    EIntr();
    PUT32(IPU_CMD, cmd);
    ipu_sync_wait();
}

// FUNC 0x00263C00 ipu_align_byte
/* Skip to the next byte boundary of the bitstream (IPU_BP.BP).
 * Caller mpeg_video_next_start_code. */
void ipu_align_byte(void)
{
    u32 bp;

    ipu_wait();
    bp = REG32(IPU_BP) & 0x7F;
    ipu_flushbits(((bp + 7) & 0xF8) - bp);
}

// FUNC 0x00263C38 ipu_bclr
/* BCLR: clear the input FIFO and set the bit pointer to bp.  Callers
 * mpeg_vibuf_restart_dma, ipu_csc_chunk, ipu_restart_dma, ipu_decode_image. */
void ipu_bclr(u32 bp)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_BCLR | (bp & 0x7F));
    ipu_wait();
}

// FUNC 0x00263C78 ipu_restart_dma
/* Restart what ipu_stop_dma saved (cf. Sony's sceIpuRestartDMA): fromIPU if
 * it had work left, BCLR to the saved bit pointer, then toIPU rewound by
 * the quadwords the IPU had already taken in (IFC + FP).  No callers
 * (mpeg_vibuf_restart_dma does its own restart). */
void ipu_restart_dma(IpuDmaEnv *env)
{
    u32 bp = env->ipubp;
    u32 taken, qwc, madr;

    if (env->d3madr != 0 && env->d3qwc != 0) {
        PUT32(D3_MADR, env->d3madr);
        PUT32(D3_QWC, env->d3qwc);
        PUT32(D3_CHCR, env->d3chcr | CHCR_STR);
    }
    ipu_bclr(bp);
    taken = ((bp >> 8) & 0xF) + ((bp >> 16) & 3);  /* IFC + FP */
    qwc = env->d4qwc + taken;
    madr = env->d4madr - taken * 16;
    if (qwc != 0 && madr != 0) {
        PUT32(D4_MADR, madr);
        PUT32(D4_TADR, env->d4tadr);
        PUT32(D4_QWC, qwc);
        PUT32(D4_CHCR, env->d4chcr | CHCR_STR);
    }
}

// FUNC 0x00263D90 ipu_stop_dma
/* Stop toIPU, save its registers, wait for the IPU output FIFO to drain
 * (IPU_CTRL.OFC == 0), stop fromIPU, save it and IPU_BP / IPU_CTRL
 * (cf. Sony's sceIpuStopDMA).  Caller mpeg_vibuf_stop_dma (callback slot 1). */
void ipu_stop_dma(IpuDmaEnv *env)
{
    DIntr();
    dma_stop_suspended(D4_CHCR);
    EIntr();
    env->d4madr = REG32(D4_MADR);
    env->d4tadr = REG32(D4_TADR);
    env->d4qwc = REG32(D4_QWC);
    env->d4chcr = REG32(D4_CHCR);
    while (IPU_CTRL_OFC(REG32(IPU_CTRL)) != 0)
        ;
    DIntr();
    dma_stop_suspended(D3_CHCR);
    EIntr();
    env->d3madr = REG32(D3_MADR);
    env->d3qwc = REG32(D3_QWC);
    env->d3chcr = REG32(D3_CHCR);
    env->ipubp = REG32(IPU_BP);
    env->ipuctrl = REG32(IPU_CTRL);
}

// FUNC 0x00263F28 ipu_dma_reset
/* Stop toIPU and fromIPU and zero their QWC / MADR (and toIPU's TADR).
 * Callers mpeg_player_thread, ipu_decode_image. */
void ipu_dma_reset(void)
{
    DIntr();
    dma_stop_suspended(D4_CHCR);
    EIntr();
    PUT32(D4_QWC, 0);
    PUT32(D4_MADR, 0);
    PUT32(D4_TADR, 0);
    DIntr();
    dma_stop_suspended(D3_CHCR);
    EIntr();
    PUT32(D3_QWC, 0);
    PUT32(D3_MADR, 0);
}

// FUNC 0x00264070 ipu_getbits
/* Read n bits (1..32): ipu_showbits(n) then ipu_flushbits(n). */
u32 ipu_getbits(int n)
{
    u32 bits = ipu_showbits(n);

    ipu_flushbits(n);
    return bits;
}

// FUNC 0x002640B0 ipu_showbits
/* Peek at the next n bits: FDEC 0, then the low word of the 64-bit
 * IPU_CMD read, shifted right by 32 - n (srlv by -n). */
u32 ipu_showbits(int n)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_FDEC);
    ipu_wait();
    return (u32)(REG64(IPU_CMD) & 0xFFFFFFFF) >> (32 - n);
}

// FUNC 0x00264120 ipu_showbits24
/* Peek at the next 24 bits (start-code prefix test).  Caller
 * mpeg_video_next_start_code. */
u32 ipu_showbits24(void)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_FDEC);
    ipu_wait();
    return (u32)(REG64(IPU_CMD) & 0xFFFFFFFF) >> 8;
}

// FUNC 0x00264180 ipu_flushbits
/* Skip n bits: FDEC n.  Does not wait for it. */
void ipu_flushbits(int n)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_FDEC | (n & 0x7F));
}

// FUNC 0x002641C8 ipu_fdec8
/* FDEC 8 with no wait at all.  Callers mpeg_video_next_start_code,
 * ipu_decode_image. */
void ipu_fdec8(void)
{
    PUT32(IPU_CMD, IPU_CMD_FDEC | 8);
}

// FUNC 0x002641F0 ipu_init
/* Reset the IPU and load its tables: RST, BCLR 0, the default intra and
 * non-intra matrices, the VQCLUT (SETVQ), the thresholds (SETTH th0 / th1,
 * both 0: nothing writes g_ipu_th0 / g_ipu_th1), RST and BCLR 0 again
 * (cf. Sony's sceIpuInit).  Callers mpeg_player_thread, ipu_decode_image. */
void ipu_init(void)
{
    u32 r;

    PUT32(IPU_CTRL, IPU_CTRL_RST);
    IPU_SYNC();
    PUT32(IPU_CMD, IPU_CMD_BCLR);
    IPU_SYNC();
    ipu_set_default_intra_matrix();
    ipu_set_default_nonintra_matrix();
    ipu_fifo_put(IPU_QWORDS(g_ipu_vqclut), 0);
    ipu_fifo_put(IPU_QWORDS(g_ipu_vqclut), 1);
    PUT32(IPU_CMD, IPU_CMD_SETVQ);
    IPU_SYNC();
    r = IPU_CMD_SETTH;
    r |= g_ipu_th0 & 0x1FF;
    r |= (g_ipu_th1 << 16) & 0x1FF0000;
    PUT32(IPU_CMD, r);
    IPU_SYNC();
    PUT32(IPU_CTRL, IPU_CTRL_RST);
    IPU_SYNC();
    PUT32(IPU_CMD, IPU_CMD_BCLR);
    IPU_SYNC();
}

// FUNC 0x002643D0 ipu_load_intra_matrix
/* SETIQ intra from the bitstream (load_intra_quantiser_matrix), feeding the
 * input FIFO while the IPU works.  Caller mpeg_video_sequence_header. */
void ipu_load_intra_matrix(void)
{
    u32 r;

    mpeg_feed_input();
    r = IPU_CMD_SETIQ;
    IPU_SYNC();
    PUT32(IPU_CMD, r);
    while (IPU_BUSY()) {
        r = REG32(IPU_BP);
        if (((r >> 8) & 0xF) == 0)
            mpeg_feed_input();
    }
}

// FUNC 0x00264480 ipu_load_nonintra_matrix
/* SETIQ non-intra from the bitstream (load_non_intra_quantiser_matrix).
 * Caller mpeg_video_sequence_header. */
void ipu_load_nonintra_matrix(void)
{
    u32 r;

    mpeg_feed_input();
    r = IPU_CMD_SETIQ | IPU_SETIQ_NONINTRA;
    IPU_SYNC();
    PUT32(IPU_CMD, r);
    while (IPU_BUSY()) {
        r = REG32(IPU_BP);
        if (((r >> 8) & 0xF) == 0)
            mpeg_feed_input();
    }
}

// FUNC 0x00264530 ipu_set_default_intra_matrix
/* The MPEG default intra matrix (64 bytes, zigzag order) through the input
 * FIFO, then SETIQ intra.  Caller ipu_init. */
void ipu_set_default_intra_matrix(void)
{
    IPU_SYNC();
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 0);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 1);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 2);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 3);
    IPU_SYNC();
    PUT32(IPU_CMD, IPU_CMD_SETIQ);
    IPU_SYNC();
}

// FUNC 0x00264610 ipu_set_default_nonintra_matrix
/* The flat non-intra matrix: the quadword of 16s four times (retail: one
 * lq, four sq), then SETIQ non-intra.  Caller ipu_init. */
void ipu_set_default_nonintra_matrix(void)
{
    IPU_SYNC();
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 4);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 4);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 4);
    ipu_fifo_put(IPU_QWORDS(g_ipu_default_iq), 4);
    IPU_SYNC();
    PUT32(IPU_CMD, IPU_CMD_SETIQ | IPU_SETIQ_NONINTRA);
    IPU_SYNC();
}

// FUNC 0x002646E8 ipu_wait
/* Wait while the IPU is busy, feeding the input FIFO whenever it is empty. */
void ipu_wait(void)
{
    while (IPU_BUSY()) {
        if (IPU_BP_IFC() == 0)
            mpeg_feed_input();
    }
}

// FUNC 0x00264758 ipu_sync_init
/* Install the IPU / fromIPU interrupt handshake on g_ipu_sync: IPU
 * interrupt off, channel 3 and 4 status cleared and masked, the INTC_IPU
 * and DMAC_FROM_IPU handlers (arg = the state), CPCOND0 = channel 3, and a
 * binary semaphore (max 1, initial 0; the other SemaParam fields are left
 * as they are on the stack).  Caller mpeg_player_create (priority = the
 * decoder thread's). */
void ipu_sync_init(u32 priority)
{
    IpuSyncState *s = &g_ipu_sync;
    struct SemaParam sp;

    s->priority = priority;
    DisableIntc(INTC_IPU);
    dstat_ch_off(DMA_CH_FROM_IPU);
    dstat_ch_off(DMA_CH_TO_IPU);
    s->intc_id = AddIntcHandler2(INTC_IPU, ipu_sync_intc_handler, 0, s);
    s->dmac_id = AddDmacHandler2(DMAC_FROM_IPU, ipu_sync_dma_handler, 0, s);
    dmac_claim_cpcond(DMA_CH_FROM_IPU, DMA_CH_FROM_IPU);
    sp.initCount = 0;
    sp.maxCount = 1;
    s->sema = CreateSema(&sp);
}

// FUNC 0x00264868 ipu_sync_term
/* Undo ipu_sync_init and clear the state.  Caller mpeg_player_delete. */
void ipu_sync_term(void)
{
    IpuSyncState *s = &g_ipu_sync;

    DisableIntc(INTC_IPU);
    dstat_ch_off(DMA_CH_FROM_IPU);
    dstat_ch_off(DMA_CH_TO_IPU);
    RemoveIntcHandler(INTC_IPU, s->intc_id);
    RemoveDmacHandler(DMAC_FROM_IPU, s->dmac_id);
    PUT32(DMAC_PCR, REG32(DMAC_PCR) & ~DMA_CH_FROM_IPU);
    DeleteSema(s->sema);
    memset(s, 0, sizeof(*s));
}

// FUNC 0x00264968 mpeg_set_callback
/* Install callback slot `slot`; returns func.  Caller mpeg_player_create
 * (slots 0..4, MPEG_CB_*). */
MpegCallback mpeg_set_callback(int slot, MpegCallback func, void *arg)
{
    g_mpeg_callbacks[slot].func = func;
    g_mpeg_callbacks[slot].arg = arg;
    return func;
}

// FUNC 0x00264990 mpeg_remove_callback
/* Clear the first of the 5 slots holding func.  No callers. */
void mpeg_remove_callback(MpegCallback func)
{
    int slot;

    for (slot = 0; slot < MPEG_CB_COUNT; slot++) {
        if (func == g_mpeg_callbacks[slot].func) {
            g_mpeg_callbacks[slot].arg = 0;
            g_mpeg_callbacks[slot].func = 0;
            return;
        }
    }
}

// FUNC 0x002649D0 mpeg_feed_input
/* While the IPU input FIFO is empty, call the input callback with (arg, 0). */
void mpeg_feed_input(void)
{
    MpegCallbackSlot *cb = &g_mpeg_callbacks[MPEG_CB_INPUT];

    while (IPU_BP_IFC() == 0)
        cb->func(cb->arg, 0);
}

// FUNC 0x00264A48 mpeg_cb_stop_dma
/* The stop-DMA callback with (arg, 0).  Caller ipu_csc_rgb32. */
void mpeg_cb_stop_dma(void)
{
    MpegCallbackSlot *cb = &g_mpeg_callbacks[MPEG_CB_STOP_DMA];

    cb->func(cb->arg, 0);
}

// FUNC 0x00264A78 mpeg_cb_restart_dma
/* The restart-DMA callback with (arg, 0).  Reached by ipu_csc_rgb32's tail
 * jump. */
void mpeg_cb_restart_dma(void)
{
    MpegCallbackSlot *cb = &g_mpeg_callbacks[MPEG_CB_RESTART_DMA];

    cb->func(cb->arg, 0);
}

// FUNC 0x00264AA8 ipu_sync_intc_handler
/* INTC_IPU handler (AddIntcHandler2, arg = &g_ipu_sync): clear the IPU
 * pending bit, wake the waiting thread, disable the IPU interrupt again.
 * Installed by ipu_sync_init, no callers. */
int ipu_sync_intc_handler(int cause, void *arg, void *addr)
{
    IpuSyncState *s = (IpuSyncState *)arg;

    s->pending &= ~IPU_PENDING_INTC;
    iWakeupThread(s->tid);
    iDisableIntc(INTC_IPU);
    ExitHandler();
    return 0;
}

// FUNC 0x00264AE8 ipu_sync_dma_handler
/* DMAC_FROM_IPU handler: clear the DMA pending bit, wake the waiting
 * thread, clear and mask the channel 3 interrupt.  Installed by
 * ipu_sync_init, no callers. */
int ipu_sync_dma_handler(int ch, void *arg, void *addr)
{
    IpuSyncState *s = (IpuSyncState *)arg;

    s->pending &= ~IPU_PENDING_DMA;
    iWakeupThread(s->tid);
    dstat_ch_off(DMA_CH_FROM_IPU);
    ExitHandler();
    return 0;
}

// FUNC 0x00264B60 ipu_sync_arm
/* Before a CSC: both bits pending, IPU interrupt on, channel 3 interrupt
 * cleared and unmasked, D_ENABLEW = 0.  Caller ipu_csc_chunk.  The state is
 * written through a volatile pointer: retail keeps the store out of the
 * delay slot of EIntr's jal (a plain store goes there). */
void ipu_sync_arm(void)
{
    volatile IpuSyncState *s = &g_ipu_sync;

    DIntr();
    s->pending = IPU_PENDING_DMA | IPU_PENDING_INTC;
    EIntr();
    EnableIntc(INTC_IPU);
    dstat_ch_on(DMA_CH_FROM_IPU);
    PUT32(DMAC_ENABLEW, 0);
}

// FUNC 0x00264C00 ipu_sync_wait
/* Sleep until both handlers have run (pending == 0).  Reached by
 * ipu_csc_chunk's tail jump. */
void ipu_sync_wait(void)
{
    volatile IpuSyncState *s = &g_ipu_sync;

    DIntr();
    while (s->pending != 0) {
        EIntr();
        SleepThread();
        DIntr();
    }
    EIntr();
}

// FUNC 0x00264C60 ipu_sync_set_thread
/* The thread the handlers wake (the decoder thread mpeg_player_create
 * starts). */
void ipu_sync_set_thread(s32 tid)
{
    g_ipu_sync.tid = tid;
}

// FUNC 0x00264C70 ipu_vdec_mb_type
/* macroblock_type: IPU_CTRL.PCT = pict_type, VDEC table 1; returns the
 * low 16 bits of the result.  Caller mpeg_video_macroblock_modes. */
int ipu_vdec_mb_type(int pict_type)
{
    u32 r;

    ipu_wait();
    r = REG32(IPU_CTRL);
    r = (r & ~IPU_CTRL_PCT_MASK) | IPU_CTRL_PCT(pict_type);
    PUT32(IPU_CTRL, r);
    ipu_wait();
    r = IPU_CMD_VDEC | IPU_VDEC_TBL(VDEC_MB_TYPE);
    PUT32(IPU_CMD, r);
    ipu_wait();
    return (int)(REG64(IPU_CMD) & 0xFFFF);
}

// FUNC 0x00264D00 ipu_vdec_mb_addr_inc
/* macroblock_address_increment: each 11-bit code below 0x18 is skipped,
 * adding 33 unless it is macroblock_stuffing (0xF; macroblock_escape is
 * 0x8, and any other invalid code also counts as an escape); a leading '1'
 * bit means 1 (FDEC 1); otherwise VDEC table 0.  Callers
 * mpeg_video_decode_slice, mpeg_video_slice_header. */
int ipu_vdec_mb_addr_inc(void)
{
    int inc = 0;
    int code;

    for (;;) {
        code = ipu_showbits(11);
        if (code >= 0x18)
            break;
        if ((code ^ 0xF) != 0)
            inc += 33;
        ipu_flushbits(11);
    }
    if (code >= 0x400) {
        ipu_flushbits(1);
        return inc + 1;
    }
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_VDEC | IPU_VDEC_TBL(VDEC_MB_ADDR_INC));
    ipu_wait();
    return inc + (int)(REG64(IPU_CMD) & 0xFFFF);
}

// FUNC 0x00264DA8 ipu_vdec_motion_code
/* motion_code: VDEC table 2, the result as a signed 16-bit value.
 * Caller mpeg_video_motion_vector. */
s16 ipu_vdec_motion_code(void)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_VDEC | IPU_VDEC_TBL(VDEC_MOTION_CODE));
    ipu_wait();
    return (s16)(u16)(REG64(IPU_CMD) & 0xFFFF);
}

// FUNC 0x00264E00 ipu_vdec_dmvector
/* dmvector: VDEC table 3, signed 16-bit.  Caller mpeg_video_motion_vector. */
s16 ipu_vdec_dmvector(void)
{
    ipu_wait();
    PUT32(IPU_CMD, IPU_CMD_VDEC | IPU_VDEC_TBL(VDEC_DMVECTOR));
    ipu_wait();
    return (s16)(u16)(REG64(IPU_CMD) & 0xFFFF);
}
