/* mpeg_player_internal.h - unit u121 of the GT3 main executable, 0x0025F390..0x00262AE0.
 *
 * The first part of the no-FDE stretch G002 (plain C): an MPEG-2 video
 * player built on the IPU.  The bitstream is fed to the IPU by DMA channel 4
 * (toIPU) from a ring of 2^n elements of 2^m bytes chained by "ref" DMA tags;
 * the IPU does the VLC / IDCT work (ipu_bdec, the FDEC bit readers ... of the
 * next unit, u122); this unit does the syntax (headers, macroblock modes,
 * motion vectors), the motion compensation in EE MMI code, the frame
 * reordering and an output queue of colour-converted frames consumed by the
 * movie streamer mpegif (T529, unit u120).
 *
 * One file, one object of the original (0x0025F390..0x00263368, on into
 * u122's range; ref/matching/u121/notes.md, "Merge (G002 mpeg)"):
 *   mpeg_player.c   the player object at scratchpad 0x70002344
 *                             (create / delete, decoder thread, callbacks,
 *                             PES input); the video input buffer at
 *                             scratchpad 0x70002300 (the DMA-tag ring and
 *                             the IPU DMA callbacks add / stop / restart);
 *                             the output frame queue (malloc'ed, 3 RGBA
 *                             frames); the MPEG-2 PES packet header / PTS /
 *                             DTS parser; the video decoder (start codes,
 *                             sequence / GOP / picture headers and
 *                             extensions, slices, macroblock modes, motion
 *                             vectors, frame stores); the MMI pixel code
 *                             (store / add the IDCT result, luma and chroma
 *                             half-pel prediction); the macroblock get / put
 *                             and the reference window fetch
 *
 * Frame stores are macroblock-ordered: macroblock (x, y) is the 0x180 bytes
 * (Y 16x16, Cb 8x8, Cr 8x8, "raw8" order of the IPU) at
 * frame + (x * mb_height + y) * 0x180 - column-major.
 *
 * The canonical header engine/media/G002_mpeg_frameq.h declares what other units
 * call and the types G002 shares (IpuDmaEnv, IpuBdecParam, MpegCallback,
 * MpegCallbackSlot, g_mpeg_callbacks, g_mb_height); engine/media/T529_mpegif.h the
 * five .sdata words the player shares with mpegif.  This header keeps what
 * only the unit uses: the layouts of the types G002 forward-declares
 * (MpegPlayer, MpegViBuf, MpegFrameQueue, MpegMv), the decoder's own data
 * and the unit's internal functions.
 */
#ifndef U121_H
#define U121_H

#include "engine/media/G002_mpeg_frameq.h"      /* own translation unit */
#include "engine/media/T529_mpegif.h"           /* g_mpeg_start_sema, g_frameq_filled_once, g_mpeg_wake_thread,
                                         * g_dec_state, g_dec_state_sema */
#include "runtime/startup/G000_collect2_ctors.h"   /* _gp */
#include "eekernel.h"                   /* ThreadParam, SemaParam, threads / semaphores, DIntr / EIntr */
#include "libc.h"                       /* memset, memcpy, malloc, free, memalign */

/* ======================================================================== */
/* Hardware                                                                 */
/* ======================================================================== */

#define D3_CHCR         0x1000B000  /* DMAC ch3 fromIPU */
#define D3_MADR         0x1000B010
#define D3_QWC          0x1000B020
#define D4_CHCR         0x1000B400  /* DMAC ch4 toIPU */
#define D4_MADR         0x1000B410
#define D4_QWC          0x1000B420
#define D4_TADR         0x1000B430
#define IPU_CTRL        0x10002010
#define DMAC_ENABLER    0x1000F520
#define DMAC_ENABLEW    0x1000F590  /* bit 16 = suspend all DMA */

/* CHCR bits used here. */
#define CHCR_MOD_CHAIN  0x00000004  /* MOD = 1: source chain */
#define CHCR_STR        0x00000100  /* start */

/* R5900 "sync.l".  The original's had no "memory" clobber: egcs keeps
 * vb->tag / vb->data in registers across it (mpeg_vibuf_reset).  GCC 14
 * keeps the clobber, so no memory access moves across the barrier there. */
#ifdef BOOT_OLD_GCC
#define SYNC_L() __asm__ __volatile__("sync.l")
#else
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")
#endif

/* Every register store of this unit is `sw; lw (read back); sync.l'.  An
 * inline function, not a do { } while (0) macro: the loop notes of the
 * latter end CSE's block, and the original reuses values across them. */
static __inline__ void put32(u32 reg, u32 v)
{
    REG32(reg) = v;
    (void)REG32(reg);
    SYNC_L();
}
#define PUT32(reg, v) put32((reg), (u32)(v))

/* Write a DMAC channel CHCR with the DMAC suspended (D_ENABLEW bit 16 set,
 * then D_ENABLER's old value back).  The callers bracket it with DIntr() /
 * EIntr().  A plain block (statement context only): as an inline function
 * the CHCR address and value would be set up before the first store, and
 * the original computes them after it. */
#define DMAC_PUT_CHCR_SUSPENDED(chcr_reg, v) {          \
        u32 enabler_ = REG32(DMAC_ENABLER);             \
        PUT32(DMAC_ENABLEW, enabler_ | 0x10000);        \
        PUT32(chcr_reg, v);                             \
        PUT32(DMAC_ENABLEW, enabler_);                  \
    }

/* The physical address (low 28 bits, what DMA registers and tags take) and
 * the uncached view (| 0x20000000), as the code computes them. */
#define PHYS(p)         ((u32)(p) & 0x0FFFFFFF)
#define UNCACHED_VIEW(p) ((void *)(PHYS(p) | 0x20000000))

/* Scratchpad layout used by the player. */
#define SPR_MC_REF      ((u8 *)0x70000000)   /* reference area fetched by mpeg_fetch_ref_mbs */
#define SPR_MC_PRED     ((u8 *)0x70000600)   /* 0x180-byte prediction macroblock */
#define SPR_MB_COEF     ((s16 *)0x70000780)  /* 0x300 bytes: IDCT output, s16[384] */
#define SPR_THREAD_STACK ((void *)0x70000B00) /* decoder thread stack */
#define SPR_THREAD_STACK_SIZE 0xC00
#define SPR_VIBUF       0x70002300           /* MpegViBuf, 0x44 bytes */
#define SPR_PLAYER      0x70002344           /* MpegPlayer, 0x24 bytes */

#define MB_BYTES        0x180               /* one macroblock: Y 0x100, Cb 0x40, Cr 0x40 */
#define MB_QWORDS       24                  /* sizeof(MpegMB) / 16 (MpegMB: engine/media/G002_mpeg_frameq.h) */
#define MB_COEF_BYTES   0x300               /* its IDCT output (s16) */

/* ======================================================================== */
/* MPEG-2 video constants (ISO 13818-2)                                     */
/* ======================================================================== */

/* Start codes (32-bit, prefix 0x000001). */
#define SC_PICTURE          0x100
#define SC_SLICE_FIRST      0x101
#define SC_SLICE_LAST       0x1AF
#define SC_USER_DATA        0x1B2
#define SC_SEQUENCE_HEADER  0x1B3
#define SC_EXTENSION        0x1B5
#define SC_SEQUENCE_END     0x1B7
#define SC_GROUP            0x1B8
#define START_CODE_PREFIX   0x000001        /* ipu_showbits24() at a start code */

/* extension_start_code_identifier */
#define EXT_SEQUENCE            1
#define EXT_SEQUENCE_DISPLAY    2
#define EXT_PICTURE_CODING      8

/* picture_coding_type */
#define PCT_I   1
#define PCT_P   2
#define PCT_B   3
#define PCT_D   4

/* picture_structure */
#define PS_TOP_FIELD    1
#define PS_BOTTOM_FIELD 2
#define PS_FRAME        3

/* macroblock_type flags (ipu_vdec_mb_type) */
#define MB_INTRA        0x01
#define MB_PATTERN      0x02
#define MB_MOTION_BWD   0x04
#define MB_MOTION_FWD   0x08
#define MB_QUANT        0x10

/* frame_motion_type / field_motion_type */
#define MT_FIELD        1   /* frame pictures: field prediction; field pictures: field prediction */
#define MT_FRAME        2   /* frame pictures: frame prediction; field pictures: 16x8 MC */
#define MT_DUAL_PRIME   3
#define MT_NONE         (-0x27) /* what macroblock_modes leaves when the macroblock has no vectors */

/* ======================================================================== */
/* Player states                                                            */
/* ======================================================================== */

/* g_dec_state's values DEC_*: engine/media/T529_mpegif.h. */

/* MpegViBuf.state */
#define VIBUF_STOPPED       0   /* the decoder thread has finished */
#define VIBUF_RUNNING       1   /* mpeg_vibuf_reset has armed the DMA */
#define VIBUF_CREATED       3   /* created, the thread has not started yet */

/* mpeg_player_put_pes results PUT_PES_*: engine/media/G002_mpeg_frameq.h. */

/* ======================================================================== */
/* Structures                                                               */
/* ======================================================================== */

/* One source-chain DMA tag (only the low doubleword is written). */
typedef struct MpegDmaTag {
    u64 tag;                /* 0x00 qwc | id << 28 | addr << 32 */
    u64 pad;                /* 0x08 */
} MpegDmaTag;

/* DMA tag IDs (bits 28..30 of the tag word). */
#define DMATAG_REFE     0x00000000
#define DMATAG_NEXT     0x20000000
#define DMATAG_REF      0x30000000
#define DMATAG_ID_CLEAR 0x8FFFFFFF  /* clears bits 28..30: the ID of a tag word, and of CHCR.TAG
                                     * (CHCR bits 16..31 = the upper half of the last tag read) */

/* Video input buffer (scratchpad 0x70002300).  g_vibuf_n elements of
 * g_vibuf_elm bytes; tag[i] points at element i, tag[n] is "next" back to
 * tag[0].  Elements dmaStart .. dmaStart+dmaN-1 are queued to the IPU, the
 * last one with a "refe" tag; readBytes are written but not yet queued. */
struct MpegViBuf {
    IpuDmaEnv env;          /* 0x00 saved by ipu_stop_dma (callback 1) */
    u8 *data;               /* 0x24 cached */
    MpegDmaTag *tag;        /* 0x28 uncached, n + 1 tags */
    int dmaStart;           /* 0x2C */
    int dmaN;               /* 0x30 */
    int readBytes;          /* 0x34 */
    int sema;               /* 0x38 mutex */
    int priority;           /* 0x3C the decoder thread priority (create argument; not read in G002) */
    int state;              /* 0x40 VIBUF_* */
};
EGCS_SIZE(struct MpegViBuf, 0x44);

/* Output frame queue (malloc 0x28): count slots of elemSize bytes. */
struct MpegFrameQueue {
    u8 *buf;                /* 0x00 mpeg_dma_alloc(bufSize) */
    int bufSize;            /* 0x04 */
    int elemSize;           /* 0x08 */
    int count;              /* 0x0C */
    int writeIdx;           /* 0x10 next slot to fill */
    int filled;             /* 0x14 */
    int freeSema;           /* 0x18 counts free slots; -1 once deleted */
    int lockSema;           /* 0x1C mutex */
    int priority;           /* 0x20 priority mpeg_frameq_abort runs at (mpegif's prioHigh) */
    int threadPriority;     /* 0x24 the decoder thread priority (create argument), not read */
};
EGCS_SIZE(struct MpegFrameQueue, 0x28);

/* The player (scratchpad 0x70002344). */
struct MpegPlayer {
    int thread;             /* 0x00 decoder thread */
    void *stack;            /* 0x04 0x70000B00 */
    MpegViBuf *vibuf;       /* 0x08 */
    MpegFrameQueue *frames; /* 0x0C */
    int maxWidth;           /* 0x10 */
    int maxHeight;          /* 0x14 */
    int width;              /* 0x18 from the size callback */
    int height;             /* 0x1C */
    int mbCount;            /* 0x20 (width / 16) * (height / 16) */
};
EGCS_SIZE(struct MpegPlayer, 0x24);

/* Dual prime: the flag and the differential vector (mpeg_video_motion_vector
 * reaches them through a pointer to this part). */
typedef struct MpegDmv {
    int dmv;                /* 0x00 */
    int dmvector[2];        /* 0x04 */
} MpegDmv;
EGCS_SIZE(MpegDmv, 0xC);

/* Motion-vector state of a slice (on mpeg_video_decode_slice's stack).
 * Indices as in ISO 13818-2: r = first / second vector, s = forward /
 * backward, t = horizontal / vertical. */
struct MpegMv {
    int field_in_frame;     /* 0x00 field vectors in a frame picture (vertical PMV halved around the update) */
    int mv_format_frame;    /* 0x04 mv_format == frame */
    int mv_count;           /* 0x08 motion_vector_count */
    int motion_type;        /* 0x0C frame_ / field_motion_type (MT_*) */
    int stwtype;            /* 0x10 MSSG's spatial_temporal_weight type: 0 per coded macroblock, 8 for a skipped one in an I picture; never read */
    MpegDmv dual;           /* 0x14 dual prime: flag and dmvector */
    u8 pad_20[0x10];        /* 0x20 */
    int pmv[2][2][2];       /* 0x30 PMV[r][s][t] */
    int field_select[2][2]; /* 0x50 motion_vertical_field_select[r][s] */
};
EGCS_SIZE(struct MpegMv, 0x60);

/* The three frame stores (0x004A7608). */
typedef struct MpegFrames {
    int allocated;          /* 0x00 */
    int size;               /* 0x04 */
    u8 *buf[3];             /* 0x08 */
} MpegFrames;
EGCS_SIZE(MpegFrames, 0x14);

/* The reference window mpeg_fetch_ref_mbs fills (scratchpad 0x70000000,
 * g_mc_ref_area): the 2 x 2 macroblocks whose upper left one holds a motion
 * vector's target, luma 32 x 32 and each chroma plane 16 x 16.  Only the
 * rows the predictor reads are written. */
typedef struct MpegRefWindow {
    u8 y[32][32];           /* 0x000 */
    u8 cb[16][16];          /* 0x400 */
    u8 cr[16][16];          /* 0x500 */
} MpegRefWindow;
EGCS_SIZE(MpegRefWindow, 0x600);

/* ======================================================================== */
/* Data (all used only by this unit)                                        */
/* ======================================================================== */

/* Defined by mpeg_player.c.  Its file statics are not declared
 * here: the input ring's geometry (0x003532A0..0x003532BC),
 * g_mpeg_thread_running (0x003532C0) and the frame stores g_mpeg_frames
 * (0x004A7608). */
extern const struct SemaParam g_vibuf_sema_param;   /* 0x00345A78 {0, 1, 1}: vibuf mutex */
extern const struct SemaParam g_frameq_sema_param;  /* 0x00345A90 {0, 1, 1}: frame-queue mutex */

extern IpuBdecParam g_mpeg_bdec;            /* 0x002FF098 the macroblock being decoded */
extern u32 g_mpeg_f_code[2][2];             /* 0x002FF0C0 f_code[s][t] */

extern int g_00353124;                  /* 0x00353124 gp-0x48CC cleared by the size callback, never read */
extern u8 *g_ref_back;                  /* 0x00353128 gp-0x48C8 newest reference frame */
extern u8 *g_ref_fwd;                   /* 0x0035312C gp-0x48C4 older reference frame */
extern u8 *g_frame_b;                   /* 0x00353130 gp-0x48C0 B-picture store */
extern u8 *g_frame_cur;                 /* 0x00353134 gp-0x48BC picture being decoded */
extern int g_mb_width;                  /* 0x00353138 gp-0x48B8 */
extern int g_horizontal_size;           /* 0x00353148 gp-0x48A8 */
extern int g_vertical_size;             /* 0x0035314C gp-0x48A4 */
extern int g_mpeg2;                     /* 0x00353150 gp-0x48A0 a sequence extension was seen */
extern int g_progressive_sequence;      /* 0x00353154 gp-0x489C */
extern u32 g_picture_coding_type;       /* 0x00353158 gp-0x4898 PCT_* */
extern u32 g_picture_structure;         /* 0x0035315C gp-0x4894 PS_* */
extern u32 g_top_field_first;           /* 0x00353160 gp-0x4890 */
extern u32 g_frame_pred_frame_dct;      /* 0x00353164 gp-0x488C */
extern u8 *g_mc_ref_area;               /* 0x00353168 gp-0x4888 (=0x70000000) */
extern u8 *g_mc_pred;                   /* 0x0035316C gp-0x4884 (=0x70000600) */

/* ======================================================================== */
/* This unit: the functions G002_mpeg_frameq.h does not declare             */
/* ======================================================================== */

/* mpeg_player.c: the player */
int  mpeg_player_on_size(MpegPlayer *pl, int *size);    /* 0x0025F568 */

/* the input ring */
int  mpeg_vibuf_reset(MpegViBuf *vb);
void mpeg_vibuf_begin_put(MpegViBuf *vb, u8 **ptr1, int *len1, u8 **ptr2, int *len2);
void mpeg_vibuf_end_put(MpegViBuf *vb, int n);

/* the frame queue */
void mpeg_frameq_reset(MpegFrameQueue *q);
void mpeg_frameq_abort(MpegFrameQueue *q);
int  mpeg_frameq_is_full(MpegFrameQueue *q);
int  mpeg_frameq_is_empty(MpegFrameQueue *q);
u8  *mpeg_frameq_begin_write(MpegFrameQueue *q);
void mpeg_frameq_end_write(MpegFrameQueue *q);
u8  *mpeg_frameq_peek(MpegFrameQueue *q);
void mpeg_frameq_end_read(MpegFrameQueue *q);

/* the PES parser */
int  mpeg_pes_parse_timestamps(const u8 *pkt, int off, s64 *pts, s64 *dts);

/* the video decoder */
int  mpeg_video_decode_main(void);
int  mpeg_video_setup_sequence(void);
void mpeg_video_release_frames(void);
void mpeg_video_output_picture(int npics);
void mpeg_frames_alloc(int size);
void mpeg_frames_free(void);
void mpeg_frames_assign(void);
void mpeg_video_decode_picture(int npics);
void mpeg_video_rotate_frames(void);
int  mpeg_video_next_picture(void);
void mpeg_video_gop_header(void);
void mpeg_video_picture_header(void);
void mpeg_video_slice_qscale(void);
void mpeg_video_extension_and_user_data(void);
void mpeg_video_sequence_extension(void);
void mpeg_video_sequence_display_extension(void);
void mpeg_video_picture_coding_extension(void);
int  mpeg_video_motion_vectors(MpegMv *mv, int s, int r_size_h, int r_size_v);
void mpeg_video_decode_motion_vector(int *pred, int r_size, int code, int residual);

/* the MMI pixel code */
void mpeg_mb_put_block(u8 *dst, const s16 *coef, int add);
void mpeg_mc_luma(u8 *dst, const u8 *src, int x, int y, int dx, int dy, int avg);

#endif /* U121_H */
