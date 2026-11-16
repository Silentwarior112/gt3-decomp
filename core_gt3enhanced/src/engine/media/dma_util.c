/* dma_util.c - DMA helpers of the MPEG decoder (G002),
 * 0x00263368..0x00263688: 64-byte-rounded, 128-aligned DMA buffers, the
 * CPCOND0 claim and the scratchpad DMA copy.
 */
#include "engine/media/ipu_internal.h"

// FUNC 0x00263368 mpeg_dma_alloc
/* memalign(0x80, size rounded up to 64 bytes), then write back the
 * D-cache.  Callers mpegif::preload, mpegif::feed, mpeg_vibuf_create,
 * mpeg_frameq_create. */
void *mpeg_dma_alloc(u32 size)
{
    void *p = memalign(0x80, ((size + 0x3F) >> 6) << 6);

    FlushCache(WRITEBACK_DCACHE);
    return p;
}

// FUNC 0x002633A8 mpeg_dma_free
/* Tail call of free.  Callers mpegif::preload, mpegif::feed,
 * mpeg_vibuf_delete, mpeg_frameq_delete. */
void mpeg_dma_free(void *p)
{
    free(p);
}

// FUNC 0x002633B0 dmac_claim_cpcond
/* Make the channels `pcr` (D_PCR CPC bits) the CPCOND0 sources and clear
 * their D_STAT bits `stat`.  Fails (1) if another CPC bit (0..9) is already
 * set, else 0.  Callers spr_dma_copy, ipu_sync_init. */
int dmac_claim_cpcond(u32 stat, u32 pcr)
{
    u32 cur = REG32(DMAC_PCR);

    if ((cur & DMAC_PCR_CPC_ALL) & ~pcr)
        return 1;
    PUT32(DMAC_PCR, cur | pcr);
    PUT32(DMAC_STAT, stat);
    return 0;
}

// FUNC 0x00263408 spr_dma_copy
/* Copy `size` bytes between main memory and the scratchpad by DMA and wait
 * for it (bc0f on CPCOND0).  dst in the scratchpad: channel 9 toSPR, else
 * channel 8 fromSPR (src is then the scratchpad side).  sqwc_bytes != 0
 * selects interleave mode with D_SQWC = TQWC tqwc_bytes / 16, SQWC
 * sqwc_bytes / 16.  flags bit 0 skips the D-cache write-back.
 * dmac_claim_cpcond's result is ignored.  Always 0.  No callers (reached
 * through a pointer, or unused). */
int spr_dma_copy(u32 dst, u32 src, u32 size, u32 tqwc_bytes, u32 sqwc_bytes, u32 flags)
{
    int to_spr = IS_SPR_ADDR(dst);
    u32 chcr;
    u32 r;

    if (sqwc_bytes == 0) {
        PUT32(to_spr ? D9_CHCR : D8_CHCR, 0);                     /* normal mode */
    } else {
        r = CHCR_MOD_INTERLEAVE;
        PUT32(to_spr ? D9_CHCR : D8_CHCR, r);
        r = ((tqwc_bytes >> 4) << 16) & 0xFF0000;
        r |= (sqwc_bytes >> 4) & 0xFF;
        PUT32(DMAC_SQWC, r);
    }
    if (to_spr) {
        PUT32(D9_SADR, DMA_SADR(dst));
        PUT32(D9_MADR, DMA_PHYS(src));
        PUT32(D9_QWC, size >> 4);
        dmac_claim_cpcond(DMA_CH_TO_SPR, DMA_CH_TO_SPR);
    } else {
        PUT32(D8_SADR, DMA_SADR(src));
        PUT32(D8_MADR, DMA_PHYS(dst));
        PUT32(D8_QWC, size >> 4);
        dmac_claim_cpcond(DMA_CH_FROM_SPR, DMA_CH_FROM_SPR);
    }
    if (!(flags & 1)) {
        FlushCache(WRITEBACK_DCACHE);
        SYNC_L();
    }
    chcr = to_spr ? D9_CHCR : D8_CHCR;
    DIntr();
    dma_start_suspended(chcr);
    EIntr();
    WAIT_CPCOND0();
    if (to_spr)
        PUT32(DMAC_PCR, REG32(DMAC_PCR) & ~DMA_CH_TO_SPR);
    else
        PUT32(DMAC_PCR, REG32(DMAC_PCR) & ~DMA_CH_FROM_SPR);
    return 0;
}
