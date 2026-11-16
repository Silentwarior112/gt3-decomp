/* ipu_internal.h - unit u122 of the GT3 main executable, 0x00262AE0..0x00265468.
 *
 * The middle of Polyphony's IPU-assisted MPEG decoder (G002, plain C built
 * without exceptions, right after the C++ mpegif class): the bitstream is
 * parsed with IPU commands (FDEC / VDEC / BDEC), motion compensation is done
 * by the EE (MMI), colour conversion by the IPU (CSC).  The code at
 * 0x00262AE0..0x00263368 (the last loop of mpeg_mc_chroma, the macroblock
 * get / put and the reference fetch) belongs to the player's object, in
 * u121's mpeg_player.c (ref/matching/u121/notes.md, "Merge (G002
 * mpeg)").  Files:
 *   dma_util.c    64-byte-rounded DMA buffers, the CPCOND0 claim,
 *                           the scratchpad DMA copy
 *   ipu.c         the IPU command layer: BDEC, CSC, BCLR, DMA
 *                           stop / restart, bit reader (FDEC), init (SETIQ /
 *                           SETVQ / SETTH), VDEC tables, the decoder callback
 *                           table, the IPU / fromIPU interrupt handshake
 *   ipu_image.c   still-image decoder: one IDEC of a whole
 *                           IPU-format picture into RGBA32 (gt2k_menu::Ipic),
 *                           and its fromIPU DMA / IPU INTC handlers (on into
 *                           u123's range, to 0x00265530: one object,
 *                           ref/matching/u122/notes.md, "Merge (G002 ipu
 *                           image)")
 *
 * Hardware: IPU (0x10002000..), DMAC channels 3 (fromIPU), 4 (toIPU),
 * 8 (fromSPR), 9 (toSPR), D_STAT / D_PCR / D_SQWC, D_ENABLER / D_ENABLEW.
 * Every register write in this code is "store, read back, sync.l" (PUT32).
 *
 * The canonical header engine/media/G002_mpeg_frameq.h declares what other units
 * call and the types G002 shares (IpuDmaEnv, IpuBdecParam, MpegCallback,
 * MpegCallbackSlot, MpegMB, g_mpeg_callbacks, g_mb_height).  This header
 * keeps what only the unit uses: the register map, the interrupt handshake
 * state, the unit's own data and internal functions.
 */
#ifndef U122_H
#define U122_H

#include "engine/media/G002_mpeg_frameq.h"      /* own translation unit */
#include "eekernel.h"                   /* DIntr / EIntr, INTC / DMAC handlers, semaphores, threads */
#include "libc.h"                       /* memalign, free, memset */

/* ======================================================================== */
/* Registers                                                                */
/* ======================================================================== */

#define IPU_CMD      0x10002000   /* write: command; read (64-bit): data, bit 63 BUSY */
#define IPU_CTRL     0x10002010   /* bit 31 BUSY, 30 RST, 24-26 PCT, 23 MP1, 22 QST,
                                     21 IVF, 20 AS, 16-17 IDP, 15 SCD, 14 ECD,
                                     4-7 OFC, 0-3 IFC */
#define IPU_BP       0x10002020   /* 0-6 BP, 8-11 IFC, 16-17 FP */
#define IPU_IN_FIFO  0x10007010

#define D3_CHCR      0x1000B000   /* fromIPU */
#define D3_MADR      0x1000B010
#define D3_QWC       0x1000B020
#define D4_CHCR      0x1000B400   /* toIPU */
#define D4_MADR      0x1000B410
#define D4_QWC       0x1000B420
#define D4_TADR      0x1000B430
#define D8_CHCR      0x1000D000   /* fromSPR */
#define D8_MADR      0x1000D010
#define D8_QWC       0x1000D020
#define D8_SADR      0x1000D080
#define D9_CHCR      0x1000D400   /* toSPR */
#define D9_MADR      0x1000D410
#define D9_QWC       0x1000D420
#define D9_SADR      0x1000D480
#define DMAC_STAT    0x1000E010   /* D_STAT: CIS bits 0-9 write-1-clear, CIM bits 16-25 write-1-toggle */
#define DMAC_PCR     0x1000E020   /* D_PCR: bits 0-9 CPC (CPCOND0 sources) */
#define DMAC_SQWC    0x1000E030   /* D_SQWC: 0-7 SQWC, 16-23 TQWC */
#define DMAC_ENABLER 0x1000F520
#define DMAC_ENABLEW 0x1000F590

/* Per-channel bit of D_STAT.CIS and D_PCR.CPC (the mask bit CIM is << 16). */
#define DMA_CH_FROM_IPU  0x008    /* channel 3 */
#define DMA_CH_TO_IPU    0x010    /* channel 4 */
#define DMA_CH_FROM_SPR  0x100    /* channel 8 */
#define DMA_CH_TO_SPR    0x200    /* channel 9 */
#define DMAC_PCR_CPC_ALL 0x3FF

/* CHCR bits. */
#define CHCR_MOD_MASK       0x0000000C  /* MOD: 0 normal, 1 chain, 2 interleave */
#define CHCR_MOD_INTERLEAVE 0x00000008
#define CHCR_STR            0x00000100  /* start / busy */
#define DMAC_ENABLEW_CPND   0x00010000  /* suspend all channels */

/* DMA addresses: MADR takes the physical address (bit 31 = scratchpad),
 * SADR the scratchpad offset. */
#define DMA_PHYS(a)     ((u32)(a) & 0x0FFFFFFF)
#define DMA_MADR_SPR    0x80000000
#define DMA_SADR(a)     ((u32)(a) & 0x3FFF)
#define IS_SPR_ADDR(a)  (((u32)(a) & 0x70000000) == 0x70000000)

/* IPU_CTRL bits. */
#define IPU_CTRL_RST        0x40000000
#define IPU_CTRL_PCT(t)     (((t) << 24) & 0x07000000)  /* picture_coding_type */
#define IPU_CTRL_PCT_MASK   0x07000000
#define IPU_CTRL_QST        0x00400000  /* q_scale_type */
#define IPU_CTRL_IVF        0x00200000  /* intra_vlc_format */
#define IPU_CTRL_AS         0x00100000  /* alternate_scan */
#define IPU_CTRL_IDP(p)     (((p) << 16) & 0x00030000)  /* intra_dc_precision */
#define IPU_CTRL_SCD        0x00008000  /* start code detected */
#define IPU_CTRL_ECD        0x00004000  /* VLC error */
#define IPU_CTRL_OFC(c)     (((c) >> 4) & 0xF)          /* output FIFO quadwords */

/* IPU commands (bits 28-31) and their fields. */
#define IPU_CMD_BCLR   0x00000000   /* | bp (0-6) */
#define IPU_CMD_IDEC   0x10000000
#define IPU_CMD_BDEC   0x20000000
#define IPU_CMD_VDEC   0x30000000   /* | tbl << 26 */
#define IPU_CMD_FDEC   0x40000000   /* | bits to skip (0-5) */
#define IPU_CMD_SETIQ  0x50000000
#define IPU_CMD_SETVQ  0x60000000
#define IPU_CMD_CSC    0x70000000   /* | MBC (0-10); OFM 0 = RGB32, DTE 0 = no dither */
#define IPU_CMD_SETTH  0x90000000   /* | th0 (0-8) | th1 << 16 */

#define IPU_BDEC_MBI       0x08000000   /* macroblock_intra */
#define IPU_BDEC_DCR       0x04000000   /* reset the DC predictors */
#define IPU_BDEC_DT        0x02000000   /* dct_type (field DCT) */
#define IPU_QSC(q)         (((q) << 16) & 0x001F0000)  /* quantiser_scale_code (BDEC, IDEC) */
#define IPU_IDEC_DTD(d)    ((d) << 24)  /* decode dct_type from the stream */
#define IPU_SETIQ_NONINTRA 0x08000000   /* IQM: the non-intra matrix */
#define IPU_VDEC_TBL(t)    ((t) << 26)

/* VDEC tables. */
#define VDEC_MB_ADDR_INC   0
#define VDEC_MB_TYPE       1
#define VDEC_MOTION_CODE   2
#define VDEC_DMVECTOR      3

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* R5900 "sync.l".  As in u121, the original's had no "memory" clobber
 * (egcs keeps values in registers across it: spr_dma_copy); GCC 14 keeps
 * the clobber, so no memory access moves across the barrier there. */
#ifdef BOOT_OLD_GCC
#define SYNC_L() __asm__ __volatile__("sync.l")
#else
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")
#endif

/* A register write as the retail code does it: sw, lw (read back), sync.l.
 * An inline function, not a do { } while (0) macro (as u121's put32): the
 * macro's loop notes end CSE's block and are scheduling barriers, the
 * original reuses register addresses and values across its writes. */
static __inline__ void put32(u32 reg, u32 v)
{
    REG32(reg) = v;
    (void)REG32(reg);
    SYNC_L();
}
#define PUT32(reg, v) put32((reg), (u32)(v))

/* IPU_CTRL.BUSY.  The nops inside the retail poll loops are the assembler's
 * R5900 short-loop padding (every loop is padded to 6 instructions). */
#define IPU_BUSY()   ((s32)REG32(IPU_CTRL) < 0)
#define IPU_SYNC()   do { } while (IPU_BUSY())
/* IPU_BP.IFC: quadwords in the input FIFO. */
#define IPU_BP_IFC() ((REG32(IPU_BP) >> 8) & 0xF)

/* Quadword tbl[i] into the IPU input FIFO: lq, sq, lq (read back), sync.l.
 * egcs keeps a u128 in one 128-bit GPR, so the original is C (retail
 * schedules the loads and the FIFO address as compiled code does: the
 * read-back goes to another register, the address is kept across puts).
 * Table and index are separate parameters: ipu_set_default_nonintra_matrix
 * addresses its row as table + 0x40 (lq 0x40(base)), which a constant
 * &tbl[4] argument folds away.  GCC 14 splits TImode into two 64-bit
 * accesses, and the FIFO needs one sq: there the instructions are asm. */
static __inline__ void ipu_fifo_put(const u128 *tbl, int i)
{
#ifdef BOOT_OLD_GCC
    REG128(IPU_IN_FIFO) = tbl[i];
    (void)REG128(IPU_IN_FIFO);
    SYNC_L();
#else
    u64 tmp;
    __asm__ __volatile__("lq %0, 0(%1)\n\tsq %0, 0(%2)\n\tlq %0, 0(%2)\n\tsync.l"
                         : "=&r"(tmp) : "r"(&tbl[i]), "r"(IPU_IN_FIFO) : "memory");
#endif
}

/* Wait for CPCOND0 (the D_PCR-selected channels done): `1: bc0f 1b; nop`.
 * The retail loop has 5 padding nops before the bc0f and one more nop
 * after the delay slot. */
#define WAIT_CPCOND0() \
    __asm__ __volatile__(".set push\n\t.set noreorder\n1:\tbc0f 1b\n\tnop\n\tnop\n\t.set pop" : : : "memory")

/* Set STR of a channel's CHCR with all DMA suspended: D_ENABLEW = ENABLER |
 * CPND, CHCR |= STR, D_ENABLEW = ENABLER & ~CPND.  Inlined at every use in
 * the retail code, always between DIntr() and EIntr() (written by the
 * callers). */
static __inline__ void dma_start_suspended(u32 chcr)
{
    u32 enabler = REG32(DMAC_ENABLER);

    PUT32(DMAC_ENABLEW, enabler | DMAC_ENABLEW_CPND);
    PUT32(chcr, REG32(chcr) | CHCR_STR);
    PUT32(DMAC_ENABLEW, enabler & ~DMAC_ENABLEW_CPND);
}

/* Clear STR the same way; D_ENABLEW gets the old ENABLER value back. */
static __inline__ void dma_stop_suspended(u32 chcr)
{
    u32 enabler = REG32(DMAC_ENABLER);

    PUT32(DMAC_ENABLEW, enabler | DMAC_ENABLEW_CPND);
    PUT32(chcr, REG32(chcr) & ~CHCR_STR);
    PUT32(DMAC_ENABLEW, enabler);
}

/* D_STAT: clear channel status bit `cis` and, if its mask bit (cis << 16)
 * is set, toggle it off. */
static __inline__ void dstat_ch_off(u32 cis)
{
    u32 stat = REG32(DMAC_STAT);

    PUT32(DMAC_STAT, cis);
    if (stat & (cis << 16))
        PUT32(DMAC_STAT, cis << 16);
}

/* D_STAT: clear a pending status bit, and toggle the mask bit on if off. */
static __inline__ void dstat_ch_on(u32 cis)
{
    u32 stat = REG32(DMAC_STAT);

    if (stat & cis)
        PUT32(DMAC_STAT, cis);
    if (!(stat & (cis << 16)))
        PUT32(DMAC_STAT, cis << 16);
}

/* ======================================================================== */
/* Structures                                                               */
/* ======================================================================== */

/* Decoder callback slots (g_mpeg_callbacks), as mpeg_player_create installs
 * them (G002_mpeg_frameq.h). */
enum {
    MPEG_CB_INPUT = 0,      /* IPU input exhausted: mpeg_vibuf_add_dma */
    MPEG_CB_STOP_DMA,       /* mpeg_vibuf_stop_dma */
    MPEG_CB_RESTART_DMA,    /* mpeg_vibuf_restart_dma */
    MPEG_CB_PICTURE,        /* mpeg_player_on_picture */
    MPEG_CB_SIZE,           /* mpeg_player_on_size */
    MPEG_CB_COUNT
};

/* ipu_bdec results. */
#define IPU_BDEC_OK          0
#define IPU_BDEC_START_CODE  1  /* IPU_CTRL.SCD */
#define IPU_BDEC_NOTHING     2  /* neither intra nor coded */
#define IPU_BDEC_VLC_ERROR   3  /* IPU_CTRL.ECD */
#define IPU_BDEC_TIMEOUT     4000000    /* polls of IPU_CTRL.BUSY */

/* IPU / fromIPU interrupt handshake (g_ipu_sync, cleared with memset).  The
 * same scheme as ipu_decode_image's (its statics g_ipu_image_tid /
 * g_ipu_image_pending and its two handlers), with the state passed to the
 * handlers as their argument. */
typedef struct IpuSyncState {
    s32 pending;            /* 0x00 bit 0 fromIPU DMA, bit 2 IPU interrupt (armed as 5);
                             * signed: the handlers clear a bit with `li -5; and` */
    u32 unk_04;             /* 0x04 never used */
    u32 priority;           /* 0x08 the decoder thread priority (ipu_sync_init); not read */
    s32 tid;                /* 0x0C thread the handlers wake (ipu_sync_set_thread) */
    s32 sema;               /* 0x10 created and deleted, never waited on */
    s32 intc_id;            /* 0x14 AddIntcHandler2(INTC_IPU) */
    s32 dmac_id;            /* 0x18 AddDmacHandler2(DMAC_FROM_IPU) */
    u32 unk_1C;             /* 0x1C never used */
} IpuSyncState;
EGCS_SIZE(IpuSyncState, 0x20);

#define IPU_PENDING_DMA   1 /* fromIPU DMA end not seen yet */
#define IPU_PENDING_INTC  4 /* IPU interrupt not seen yet */

/* IpuImageHeader (an IPU still image): G002_mpeg_frameq.h. */
#define IPU_IMAGE_SKIP_BYTES  17    /* header + flags */
#define IPU_IMAGE_FLAG_DTD    0x04
#define IPU_IMAGE_CHUNK_QWC   0x8C00    /* most quadwords per fromIPU transfer */

/* ======================================================================== */
/* Data (all used only by this unit)                                        */
/* ======================================================================== */

/* The IPU tables (ipu.c's .data, 16-aligned: read as quadwords
 * with lq, ipu_fifo_put).  Byte / halfword tables, not u128: egcs cannot
 * write a 128-bit constant (an initialiser keeps only the low 64 bits).
 * File statics, not declared here: g_ipu_th0 / g_ipu_th1 (.sbss) and
 * g_ipu_sync (.bss) in ipu.c, g_ipu_image_tid, g_image_intc_id,
 * g_image_dmac_id and g_ipu_image_pending (.sbss) in ipu_image.c. */
extern u8 g_ipu_default_iq[80];     /* 0x002FF0D0 [0..63] MPEG default intra matrix (zigzag), [64..79] 16 x 16 (non-intra) */
extern u16 g_ipu_vqclut[16];        /* 0x002FF120 VQCLUT for SETVQ: 16 RGB555 (greys, red, green, blue, white) */
#define IPU_QWORDS(tbl) ((const u128 *)(tbl))

/* ======================================================================== */
/* This unit: the functions G002_mpeg_frameq.h does not declare             */
/* ======================================================================== */

/* dma_util.c */
int    dmac_claim_cpcond(u32 stat, u32 pcr);
int    spr_dma_copy(u32 dst, u32 src, u32 size, u32 tqwc_bytes, u32 sqwc_bytes, u32 flags);

/* ipu.c */
void   ipu_csc_chunk(u32 dst, u32 src, int nmb);
void   ipu_restart_dma(IpuDmaEnv *env);
void   ipu_set_default_intra_matrix(void);
void   ipu_set_default_nonintra_matrix(void);
void   ipu_wait(void);
void   mpeg_remove_callback(MpegCallback func);
void   mpeg_feed_input(void);
void   mpeg_cb_stop_dma(void);
void   mpeg_cb_restart_dma(void);
int    ipu_sync_intc_handler(int cause, void *arg, void *addr);
int    ipu_sync_dma_handler(int ch, void *arg, void *addr);
void   ipu_sync_arm(void);
void   ipu_sync_wait(void);

/* ipu_image.c */
int    ipu_image_out_chunk(u32 dst, u32 size);
/* ipu_decode_image's handlers (AddDmacHandler2 / AddIntcHandler2, no
 * callers): the kernel's 3-parameter handler type; only the return value 0
 * matters, no argument is read. */
int    ipu_from_dma_handler(int ch, void *arg, void *addr);     /* 0x00265468 */
int    ipu_intc_handler(int cause, void *arg, void *addr);      /* 0x002654E8 */

#endif /* U122_H */
