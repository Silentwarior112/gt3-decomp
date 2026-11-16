/* ipu_image.c - the IPU still-image decoder (G002),
 * 0x00264E58..0x00265530: one IDEC of a whole IPU-format intra picture into
 * RGBA32, the output drained by fromIPU DMA in chunks of up to 0x8C00
 * quadwords, and its two interrupt handlers (fromIPU DMA end, IPU INTC):
 * they clear the bits of g_ipu_image_pending and wake the thread
 * g_ipu_image_tid, as ipu_sync_*_handler (ipu.c) do for the movie
 * decoder's CSC.
 *
 * One object in the original, merged from the split's two files (this one
 * and u123's G002_ipu_intr_u123.c; ref/matching/u122/notes.md, "Merge (G002
 * ipu image)"): the handlers follow ipu_image_out_chunk, and the four .sbss
 * words they share with ipu_decode_image are statics of one object.
 */
#include "engine/media/ipu_internal.h"

/* .sbss, 0x003532CC..0x003532DC: file statics in the original's order
 * (between the IPU layer's g_ipu_th1 and pjoy's g_pjoy_flush_forward,
 * before every common, u114's notes): the waiting thread, the two handler
 * ids, the pending bits. */
static s32 g_ipu_image_tid;                 /* 0x003532CC thread waiting in ipu_decode_image */
static s32 g_image_intc_id;                 /* 0x003532D0 INTC handler id */
static s32 g_image_dmac_id;                 /* 0x003532D4 DMAC handler id */
static volatile s32 g_ipu_image_pending;    /* 0x003532D8 IPU_PENDING_DMA | IPU_PENDING_INTC */

// FUNC 0x00264E58 ipu_decode_image
/* Decode img into width * height RGBA32 pixels at dst, in macroblock order
 * (16 x 16-pixel blocks of 0x400 bytes; the consumer is MpegImage::setIpic,
 * T249).  Sequence: IPU interrupt off, channel 3 / 4 status cleared and
 * masked, the handlers below in, DMA reset, ipu_init, BCLR 0; toIPU streams
 * the whole image from its header ((size + 15) / 16 + 1 quadwords, normal
 * mode); the first fromIPU chunk is queued, 17 x FDEC 8 skip the 16-byte
 * header and the flags byte; IPU_CTRL = flags (bit 2 cleared) << 16; IDEC
 * with QSC 1 and DTD = flags bit 2.  Then, interrupts off, sleep until the
 * handlers have cleared g_ipu_image_pending, queue the next chunk (setting the DMA
 * bit again) until the whole picture is out; finally DMA reset, ipu_init,
 * handlers removed.  Caller gt2k_menu::Ipic::decode. */
void ipu_decode_image(u32 dst, const IpuImageHeader *img)
{
    u32 size = img->size;
    u8 flags;
    int total, done;
    u32 cur, remain;
    int i;

    total = img->width * img->height * 4;
    flags = img->flags;
    DisableIntc(INTC_IPU);
    dstat_ch_off(DMA_CH_FROM_IPU);
    dstat_ch_off(DMA_CH_TO_IPU);
    /* The handlers take only the cause / channel (they ignore arg). */
    g_image_intc_id = AddIntcHandler2(INTC_IPU, ipu_intc_handler, 0, NULL);
    g_image_dmac_id = AddDmacHandler2(DMAC_FROM_IPU, ipu_from_dma_handler, 0, NULL);
    ipu_dma_reset();
    ipu_init();
    ipu_bclr(0);
    PUT32(D4_CHCR, REG32(D4_CHCR) & ~CHCR_MOD_MASK);
    PUT32(D4_MADR, DMA_PHYS(img));
    PUT32(D4_QWC, ((size + 0xF) >> 4) + 1);
    FlushCache(WRITEBACK_DCACHE);
    SYNC_L();
    g_ipu_image_pending = IPU_PENDING_DMA | IPU_PENDING_INTC;
    g_ipu_image_tid = GetThreadId();
    DIntr();
    dma_start_suspended(D4_CHCR);
    EIntr();

    done = ipu_image_out_chunk(dst, total);
    cur = dst + done;
    remain = total - done;
    IPU_SYNC();
    for (i = IPU_IMAGE_SKIP_BYTES - 1; i >= 0; i--)
        ipu_fdec8();
    PUT32(IPU_CTRL, (flags & ~IPU_IMAGE_FLAG_DTD) << 16);
    IPU_SYNC();
    EnableIntc(INTC_IPU);
    PUT32(IPU_CMD, IPU_IDEC_DTD((flags >> 2) & 1) | IPU_CMD_IDEC | IPU_QSC(1));

    DIntr();
    for (;;) {
        while (g_ipu_image_pending != 0) {
            EIntr();
            SleepThread();
            DIntr();
        }
        if (remain == 0)
            break;
        g_ipu_image_pending |= IPU_PENDING_DMA;
        done += ipu_image_out_chunk(cur, remain);
        cur = dst + done;
        remain = total - done;
    }
    EIntr();

    ipu_dma_reset();
    ipu_init();
    DisableIntc(INTC_IPU);
    dstat_ch_off(DMA_CH_FROM_IPU);
    dstat_ch_off(DMA_CH_TO_IPU);
    RemoveIntcHandler(INTC_IPU, g_image_intc_id);
    RemoveDmacHandler(DMAC_FROM_IPU, g_image_dmac_id);
}

// FUNC 0x002652B0 ipu_image_out_chunk
/* Stop fromIPU, point it at dst for min(size / 16, 0x8C00) quadwords (a
 * signed compare), clear / unmask its interrupt, D_ENABLEW = 0, start it.
 * Returns the bytes queued.  Caller ipu_decode_image. */
int ipu_image_out_chunk(u32 dst, u32 size)
{
    int qwc;

    DIntr();
    dma_stop_suspended(D3_CHCR);
    EIntr();
    PUT32(D3_MADR, DMA_PHYS(dst));
    qwc = size >> 4;
    if (qwc > IPU_IMAGE_CHUNK_QWC)
        qwc = IPU_IMAGE_CHUNK_QWC;
    PUT32(D3_QWC, qwc);
    dstat_ch_on(DMA_CH_FROM_IPU);
    PUT32(DMAC_ENABLEW, 0);
    DIntr();
    dma_start_suspended(D3_CHCR);
    EIntr();
    return qwc << 4;
}

// FUNC 0x00265468 ipu_from_dma_handler
/* DMAC channel 3 (fromIPU) handler: clear the DMA pending bit, wake the
 * decoder, acknowledge the channel-3 status bit (D_STAT write-1-to-clear)
 * and, if its mask bit CIM3 is set, toggle the mask off (every write read
 * back and synced: dstat_ch_off); `sync; ei', 0.  No callers:
 * ipu_decode_image installs it with AddDmacHandler2. */
int ipu_from_dma_handler(int ch, void *arg, void *addr)
{
    g_ipu_image_pending &= ~IPU_PENDING_DMA;
    iWakeupThread(g_ipu_image_tid);
    dstat_ch_off(DMA_CH_FROM_IPU);
    ExitHandler();
    return 0;
}

// FUNC 0x002654E8 ipu_intc_handler
/* INTC_IPU handler: clear the INTC pending bit, wake the decoder, disable
 * the IPU interrupt; `sync; ei', 0.  No callers: ipu_decode_image installs
 * it with AddIntcHandler2. */
int ipu_intc_handler(int cause, void *arg, void *addr)
{
    g_ipu_image_pending &= ~IPU_PENDING_INTC;
    iWakeupThread(g_ipu_image_tid);
    iDisableIntc(INTC_IPU);
    ExitHandler();
    return 0;
}
