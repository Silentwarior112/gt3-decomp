/* mpeg_player.c - the MPEG-2 player of the intro movie (G002,
 * plain C), 0x0025F390..0x00263368: one object of the original.
 *
 *   - the player object (scratchpad 0x70002344): create / delete, the
 *     decoder thread, the size / picture callbacks and the PES input path
 *     (the caller is mpegif, T529, u120);
 *   - the video input buffer (scratchpad 0x70002300): a ring of g_vibuf_n
 *     elements chained to DMA channel 4 (toIPU) by "ref" tags, the last
 *     queued element "refe", tag[n] "next" back to tag[0], and the three IPU
 *     DMA callbacks of the decoder (no data / stop / restart) that work on
 *     it (the structure of Sony's sample vibuf.c, with the sizes global);
 *   - the output frame queue: a ring of `count' frames, a mutex and a
 *     counting semaphore of free slots; the decoder thread fills slots
 *     (begin_write / end_write), the movie code reads the oldest one (peek /
 *     end_read);
 *   - the MPEG-2 PES packet header parser (video streams 0xE0..0xEF), with
 *     the 33-bit PTS / DTS;
 *   - the video decoder (EE side), syntax per ISO 13818-2: start codes,
 *     sequence / GOP / picture headers and the sequence, sequence-display
 *     and picture-coding extensions; slices, macroblock modes and motion
 *     vectors.  The bit reader, the VLC tables and the block decode are IPU
 *     wrappers (ipu.c: ipu_getbits / ipu_showbits / ipu_flushbits,
 *     ipu_vdec_mb_addr_inc / ipu_vdec_mb_type / ipu_vdec_motion_code /
 *     ipu_vdec_dmvector, ipu_bdec).  Motion compensation only uses the
 *     first vector of each direction (frame prediction).  Pictures go out
 *     through callback 3 in display order (B pictures at once, I / P
 *     pictures when the next reference starts).  Much of it is MSSG's
 *     mpeg2decode (getpic.c, motion.c, gethdr.c) cut down to MPEG-2 main
 *     profile without scalability: the matching forms follow its source
 *     where retail shows it (chained PMV resets, Get_Hdr's switch,
 *     decode_motion_vector's expressions, macroblock_modes);
 *   - the MMI pixel code (store / add the IDCT result, luma and chroma
 *     half-pel prediction) and the macroblock get / put and reference window
 *     fetch of the frame store.
 *
 * One object in the original, merged from the split's seven files (u121's
 * player, vibuf, frameq, pes, video and mc files and u122's mc file;
 * ref/matching/u121/notes.md, "Merge (G002 mpeg)"): the code interleaves
 * them (mpeg_vibuf_set_geometry among the player's functions,
 * mpeg_mb_put_block among the decoder's), and the statics show one object:
 * the ring geometry and the thread flag are file statics in one .sbss run
 * (0x003532A0..0x003532C4, before every common) that the player's and the
 * ring's functions share.  Functions in address order; the data in the
 * original's order per section.
 */
#include "engine/media/mpeg_player_internal.h"

/* ======================================================================== */
/* Data, in the original's order (values from the original's bytes)        */
/* ======================================================================== */

/* .data: the decoder's macroblock state and f_codes.  Zeros in the
 * original's .data, so initialised (a C global without an initialiser would
 * be a common). */
IpuBdecParam g_mpeg_bdec = { 0 };       /* 0x002FF098 the macroblock being decoded */
u32 g_mpeg_f_code[2][2] = { { 0 } };    /* 0x002FF0C0 f_code[s][t] */

/* .rodata: the mutexes of the input ring and of the frame queue (max 1,
 * init 1). */
const struct SemaParam g_vibuf_sema_param = { 0, 1, 1, 0, 0, 0 };  /* 0x00345A78 */
const struct SemaParam g_frameq_sema_param = { 0, 1, 1, 0, 0, 0 }; /* 0x00345A90 */

/* .sdata: the size callback's word, then the decoder's (initialised, zeros
 * too). */
int g_00353124 = 0;                     /* 0x00353124 cleared by the size callback, never read */
u8 *g_ref_back = 0;                     /* 0x00353128 newest reference frame */
u8 *g_ref_fwd = 0;                      /* 0x0035312C older reference frame */
u8 *g_frame_b = 0;                      /* 0x00353130 B-picture store */
u8 *g_frame_cur = 0;                    /* 0x00353134 picture being decoded */
int g_mb_width = 0;                     /* 0x00353138 */
int g_mb_height = 0;                    /* 0x0035313C */
int g_dec_state = 0;                    /* 0x00353140 (T529_mpegif.h) */
int g_dec_state_sema = 0;               /* 0x00353144 (T529_mpegif.h) */
int g_horizontal_size = 0;              /* 0x00353148 */
int g_vertical_size = 0;                /* 0x0035314C */
int g_mpeg2 = 0;                        /* 0x00353150 */
int g_progressive_sequence = 0;         /* 0x00353154 */
u32 g_picture_coding_type = 0;          /* 0x00353158 */
u32 g_picture_structure = 0;            /* 0x0035315C */
u32 g_top_field_first = 0;              /* 0x00353160 */
u32 g_frame_pred_frame_dct = 0;         /* 0x00353164 */
u8 *g_mc_ref_area = SPR_MC_REF;         /* 0x00353168 (=0x70000000) */
u8 *g_mc_pred = SPR_MC_PRED;            /* 0x0035316C (=0x70000600) */

/* .sbss: file statics in definition order, the original's (between T526's
 * static g_00353298 and the IPU layer's g_ipu_th0, before every common,
 * u114's notes): the ring geometry (mpeg_vibuf_set_geometry), then the
 * decoder thread's flag. */
static int g_vibuf_log2n;               /* 0x003532A0 */
static int g_vibuf_n;                   /* 0x003532A4 elements */
static int g_vibuf_n_mask;              /* 0x003532A8 */
static int g_vibuf_log2elm;             /* 0x003532AC */
static int g_vibuf_elm;                 /* 0x003532B0 element bytes */
static int g_vibuf_elm_mask;            /* 0x003532B4 */
static int g_vibuf_size;                /* 0x003532B8 total bytes */
static int g_vibuf_size_mask;           /* 0x003532BC */
static int g_mpeg_thread_running;       /* 0x003532C0 */

/* .bss: the frame stores, a file static (between T526's static g_004A75E0
 * and the IPU layer's g_ipu_sync). */
static MpegFrames g_mpeg_frames;        /* 0x004A7608 */

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The tag word of a full element: qwc = element bytes / 16, id "ref"
 * (sra: signed, then the 16-bit QWC field). */
#define VIBUF_REF_TAGW()    (((g_vibuf_elm >> 4) & 0xFFFF) | DMATAG_REF)

/* Tag word | physical address of element idx in the upper word.  A macro:
 * as an inline function its call is expanded before the store's address,
 * and the original loads vb->tag before vb->data. */
#define VIBUF_TAG(tagw, data, idx) \
    ((u64)(tagw) | ((u64)PHYS((u32)(data) + ((idx) << g_vibuf_log2elm)) << 32))

/* The physical address one past the tags (where D4_MADR is left after the
 * "next" tag has been processed). */
#define VIBUF_TAGS_END(vb)  PHYS(&(vb)->tag[g_vibuf_n + 1])

/* PTS_DTS_flags (the top two bits of the second flags byte). */
#define PES_NO_PTS      0
#define PES_PTS_ONLY    2
#define PES_PTS_DTS     3

/* Call decoder callback `slot' with its installed argument. */
#define MPEG_CALLBACK(slot, data) \
    g_mpeg_callbacks[slot].func(g_mpeg_callbacks[slot].arg, (data))

/* Zero the vector predictors PMV[r][s][t] of direction s: the chained
 * assignment of MSSG's mpeg2decode (getpic.c), stored r = 1 / t = 1 first.
 * (A do { } while (0) block here ends CSE's extended block and changes the
 * code around it.) */
#define PMV_RESET_DIR(mv, s) \
    ((mv).pmv[0][s][0] = (mv).pmv[0][s][1] = (mv).pmv[1][s][0] = (mv).pmv[1][s][1] = 0)

/* The MMI pixel code (mpeg_mb_put_block, mpeg_mc_luma, mpeg_mc_chroma):
 * three leaf routines on 128-bit MMI registers: pextlb / pextub widen 16
 * bytes to two vectors of 8 halfwords, paddh / psrlh average them, ppacb
 * packs back to bytes; pmaxh / pminh clamp.  What they compute:
 *   - mpeg_mb_put_block: dst = clamp(coef, 0, 255), or with add
 *     clamp(dst + coef, 0, 255) (paddh: a 16-bit sum);
 *   - half-pel interpolation: (a + b + 1) >> 1 and (a + b + c + d + 2) >> 2
 *     (paddh with vectors of 1 / 2);
 *   - the rounding average with the destination: (p + d + 1) >> 1, the +1
 *     from `pcgth t, sum, -1; psrlh t, t, 15' (sum > -1 is always true for
 *     byte sums);
 *   - aligned and unaligned source rows (the code tests src & 0xF and uses
 *     lq / ld or ldr+ldl pairs) load the same bytes.
 * Reference areas (mpeg_fetch_ref_mbs): luma rows of 0x20 bytes at
 * 0x70000000, Cb / Cr rows of 0x10 bytes at +0x400 / +0x500.  Prediction
 * (0x70000600): Y 16x16 (rows of 16), Cb at +0x100 and Cr at +0x140 (8x8,
 * rows of 8).
 *
 * Under the matching compiler this is the original form (below).  GCC 14
 * keeps a 128-bit value in two 64-bit registers, so an asm operand cannot
 * be one MMI register there: its build (and -DNON_MATCHING) assembles the
 * original instructions instead, as whole functions (the #else parts).  Both
 * are the same code; plain C would be several times slower, and the intro
 * movie then falls behind (ref/followups.md, "Behaviour seen in hybrid
 * boots").
 */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* The original form: C control flow (loops, alignment tests) around
 * one-instruction asm statements on u128 variables - egcs keeps a TImode
 * value in one 128-bit GPR.  The statements are volatile, so they stay in
 * source order (the listing interleaves the low and high halves by hand),
 * and asm loads and stores keep reorg from filling the delay slots around
 * them, as in the original.  ldl / ldr merge into the old register value,
 * hence their "0" inputs and the zero initialisers of every variable they
 * load (the `por r, $0, $0' at the top of luma / chroma).  The vector
 * constants are 128-bit literals, which this compiler accepts (movti:
 * dli high, pcpyld, dli low). */
#define MMI_PREF(p)         __asm__ __volatile__("pref 0, 0(%0)" : : "r"(p))
#define MMI_LQ(r, p, o)     __asm__ __volatile__("lq %0, " #o "(%1)" : "=r"(r) : "r"(p))
#define MMI_SQ(r, p, o)     __asm__ __volatile__("sq %0, " #o "(%1)" : : "r"(r), "r"(p))
#define MMI_LD(r, p, o)     __asm__ __volatile__("ld %0, " #o "(%1)" : "=r"(r) : "r"(p))
#define MMI_SD(r, p, o)     __asm__ __volatile__("sd %0, " #o "(%1)" : : "r"(r), "r"(p))
#define MMI_LDR(r, p, o)    __asm__ __volatile__("ldr %0, " #o "(%1)" : "=r"(r) : "r"(p), "0"(r))
#define MMI_LDL(r, p, o)    __asm__ __volatile__("ldl %0, " #o "(%1)" : "=r"(r) : "r"(p), "0"(r))
#define MMI_PCPYLD(d, a, b) __asm__ __volatile__("pcpyld %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PEXTLB(d, a, b) __asm__ __volatile__("pextlb %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PEXTUB(d, a, b) __asm__ __volatile__("pextub %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PEXTLB0(d, a)   __asm__ __volatile__("pextlb %0, $0, %1" : "=r"(d) : "r"(a))
#define MMI_PEXTUB0(d, a)   __asm__ __volatile__("pextub %0, $0, %1" : "=r"(d) : "r"(a))
#define MMI_PADDH(d, a, b)  __asm__ __volatile__("paddh %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PCGTH(d, a, b)  __asm__ __volatile__("pcgth %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PSRLH(d, a, n)  __asm__ __volatile__("psrlh %0, %1, " #n : "=r"(d) : "r"(a))
#define MMI_PPACB(d, a, b)  __asm__ __volatile__("ppacb %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_PNOR0(d)        __asm__ __volatile__("pnor %0, $0, $0" : "=r"(d))
#define MMI_PMAXH0(d, a)    __asm__ __volatile__("pmaxh %0, %1, $0" : "=r"(d) : "r"(a))
#define MMI_PMINH(d, a, b)  __asm__ __volatile__("pminh %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
/* The pointer step of mpeg_mb_put_block's loops (an asm too: the delay
 * slot of the loop branch stays empty). */
#define MMI_ADDIU32(p)      __asm__ __volatile__("addiu %0, %0, 32" : "=r"(p) : "0"(p))
/* The stores of the macroblock moves (mpeg_get_mb .. mpeg_put_mb) clobber
 * "memory" (see there). */
#define MMI_SQ_MEM(r, p, o) __asm__ __volatile__("sq %0, " #o "(%1)" : : "r"(r), "r"(p) : "memory")
#define MMI_SD_MEM(r, p, o) __asm__ __volatile__("sd %0, " #o "(%1)" : : "r"(r), "r"(p) : "memory")

#define MMI_ONES 0x00010001000100010001000100010001     /* 1 in every halfword */
#define MMI_TWOS 0x00020002000200020002000200020002     /* 2 in every halfword */
#else
/* GCC 14 and -DNON_MATCHING: the original instructions as whole functions
 * (numeric registers; the arguments arrive in $4.. as in the original).
 * They use only caller-saved registers and no stack. */
#define MC_ASM_BEGIN(name) \
    "\t.text\n\t.p2align\t3\n\t.set\tnoreorder\n\t.set\tnomacro\n\t.globl\t" #name "\n" \
    "\t.ent\t" #name "\n\t.type\t" #name ", @function\n" #name ":\n"
#define MC_ASM_END(name) \
    "\t.set\tmacro\n\t.set\treorder\n\t.end\t" #name "\n\t.size\t" #name ", .-" #name "\n\t.previous\n"
#endif

// FUNC 0x0025F390 mpeg_player_create
// Sets up the IPU interrupt handshake (ipu_sync_init), the callbacks, the
// input buffer and a queue of 3 frames of width*height*4 bytes, then creates
// and starts the decoder thread (stack in the scratchpad, 0x70000B00, 0xC00
// bytes).  mpegif passes qpriority = its prioHigh, priority = prioDecode.
MpegPlayer *mpeg_player_create(int width, int height, int qpriority, int priority)
{
    MpegPlayer *pl = (MpegPlayer *)SPR_PLAYER;
    struct ThreadParam param;
    int tid;

    memset(pl, 0, sizeof(MpegPlayer));
    pl->maxWidth = width;
    pl->maxHeight = height;
    ipu_sync_init(priority);
    mpeg_set_callback(4, (MpegCallback)mpeg_player_on_size, pl);
    pl->vibuf = mpeg_vibuf_create(priority);
    pl->vibuf->state = VIBUF_CREATED;
    mpeg_set_callback(0, (MpegCallback)mpeg_vibuf_add_dma, pl->vibuf);
    mpeg_set_callback(1, (MpegCallback)mpeg_vibuf_stop_dma, pl->vibuf);
    mpeg_set_callback(2, (MpegCallback)mpeg_vibuf_restart_dma, pl->vibuf);
    pl->frames = mpeg_frameq_create(width * height * 4, 3, qpriority, priority);
    mpeg_set_callback(3, (MpegCallback)mpeg_player_on_picture, pl);

    param.entry = mpeg_player_thread;
    param.stack = SPR_THREAD_STACK;
    param.stackSize = SPR_THREAD_STACK_SIZE;
    param.gpReg = _gp;
    param.initPriority = priority;
    pl->stack = SPR_THREAD_STACK;
    param.option = 0;
    tid = CreateThread(&param);
    StartThread(tid, pl);
    ipu_sync_set_thread(tid);
    pl->thread = tid;
    return pl;
}

// FUNC 0x0025F510 mpeg_player_delete
void mpeg_player_delete(MpegPlayer *pl)
{
    int tid;

    mpeg_frameq_delete(pl->frames);
    mpeg_vibuf_delete(pl->vibuf);
    ipu_sync_term();
    tid = pl->thread;
    TerminateThread(tid);
    DeleteThread(tid);
    g_mpeg_thread_running = 0;
}

// FUNC 0x0025F568 mpeg_player_on_size
/* The sequence-size callback, reached only through the callback table
 * (slot 4).  size = {width, height} in pixels. */
int mpeg_player_on_size(MpegPlayer *pl, int *size)
{
    int width = size[0];
    int height = size[1];

    pl->width = width;
    pl->height = height;
    pl->mbCount = (width >> 4) * (height >> 4);
    g_00353124 = 0;
    return 0;
}

// FUNC 0x0025F598 mpeg_player_get_width
int mpeg_player_get_width(MpegPlayer *pl)
{
    return pl->width;
}

// FUNC 0x0025F5A0 mpeg_player_get_height
int mpeg_player_get_height(MpegPlayer *pl)
{
    return pl->height;
}

// FUNC 0x0025F5A8 mpeg_player_on_picture
// Picture callback (slot 3): colour-convert the decoded picture (a frame
// store, mbCount macroblocks) into the next free slot of the output queue.
int mpeg_player_on_picture(MpegPlayer *pl, void *pic)
{
    ipu_csc_rgb32((u32)mpeg_frameq_begin_write(pl->frames), (u32)pic, pl->mbCount);
    mpeg_frameq_end_write(pl->frames);
    return 0;
}

// FUNC 0x0025F5F8 mpeg_player_thread
// Decoder thread: start the input DMA, decode the stream to its end, stop,
// then sleep forever (the thread is terminated by mpeg_player_delete).
void mpeg_player_thread(void *arg)
{
    MpegPlayer *pl = (MpegPlayer *)arg;
    MpegViBuf *vb = pl->vibuf;

    g_mpeg_thread_running = 1;
    ipu_dma_reset();
    ipu_init();
    mpeg_frameq_reset(pl->frames);
    mpeg_vibuf_reset(vb);
    SignalSema(g_mpeg_start_sema);
    mpeg_video_decode_main();
    mpeg_frameq_reset(pl->frames);
    ipu_dma_reset();
    ipu_init();
    WaitSema(vb->sema);
    vb->state = VIBUF_STOPPED;
    SignalSema(vb->sema);
    if (g_mpeg_wake_thread != -1)
        WakeupThread(g_mpeg_wake_thread);
    for (;;)
        SleepThread();
}

// FUNC 0x0025F698 mpeg_vibuf_set_geometry
// Ring geometry: 2^log2n elements of 2^log2elm bytes (mpegif: 2^5 or 2^4
// elements of 16 KB).
void mpeg_vibuf_set_geometry(int log2n, int log2elm)
{
    int n = 1 << log2n;
    int elm = 1 << log2elm;
    int size = 1 << (log2elm + log2n);

    g_vibuf_n_mask = n - 1;
    g_vibuf_elm_mask = elm - 1;
    g_vibuf_size_mask = size - 1;
    g_vibuf_log2n = log2n;
    g_vibuf_log2elm = log2elm;
    g_vibuf_n = n;
    g_vibuf_elm = elm;
    g_vibuf_size = size;
}

// FUNC 0x0025F6E0 mpeg_player_put_pes
// Copy the payload of one video PES packet into the input ring (through its
// uncached view).  Returns PUT_PES_OK, PUT_PES_STOPPED (the thread has
// finished), PUT_PES_EMPTY (no payload) or PUT_PES_NOT_READY (the buffer is
// not running yet, or has no room).
int mpeg_player_put_pes(MpegPlayer *pl, const u8 *pkt)
{
    MpegViBuf *vb = pl->vibuf;
    const u8 *payload;  /* sp+0x00 */
    int len;            /* sp+0x04 */
    s64 pts;            /* sp+0x08 */
    s64 dts;            /* sp+0x10 */
    u8 *ptr1;           /* sp+0x18 */
    int len1;           /* sp+0x1C */
    u8 *ptr2;           /* sp+0x20 */
    int len2;           /* sp+0x24 */
    int state;

    WaitSema(vb->sema);
    state = vb->state;
    SignalSema(vb->sema);
    switch (state) {
    case VIBUF_STOPPED:
        return PUT_PES_STOPPED;
    case VIBUF_CREATED:
        return PUT_PES_NOT_READY;
    }
    mpeg_pes_parse(pkt, &payload, &len, &pts, &dts);   /* result ignored */
    if (len == 0)
        return PUT_PES_EMPTY;
    mpeg_vibuf_begin_put(vb, &ptr1, &len1, &ptr2, &len2);
    if (len1 + len2 < len)
        return PUT_PES_NOT_READY;
    if (len1 >= len) {
        memcpy(UNCACHED_VIEW(ptr1), payload, len);
    } else {
        memcpy(UNCACHED_VIEW(ptr1), payload, len1);
        memcpy(UNCACHED_VIEW(ptr2), payload + len1, len - len1);
    }
    mpeg_vibuf_end_put(vb, len);
    return PUT_PES_OK;
}

// FUNC 0x0025F820 mpeg_player_is_frameq_full
int mpeg_player_is_frameq_full(MpegPlayer *pl)
{
    return mpeg_frameq_is_full(pl->frames);
}

// FUNC 0x0025F840 mpeg_player_peek_frame
// The oldest decoded frame, or 0 (also while the thread is not running).
u8 *mpeg_player_peek_frame(MpegPlayer *pl)
{
    if (g_mpeg_thread_running == 0)
        return 0;
    return mpeg_frameq_peek(pl->frames);
}

// FUNC 0x0025F870 mpeg_player_release_frame
void mpeg_player_release_frame(MpegPlayer *pl)
{
    if (g_mpeg_thread_running != 0)
        mpeg_frameq_end_read(pl->frames);
}

// FUNC 0x0025F890 mpeg_player_abort_frameq
void mpeg_player_abort_frameq(MpegPlayer *pl)
{
    mpeg_frameq_abort(pl->frames);
}

// FUNC 0x0025F898 mpeg_vibuf_create
MpegViBuf *mpeg_vibuf_create(int priority)
{
    MpegViBuf *vb = (MpegViBuf *)SPR_VIBUF;

    memset(vb, 0, sizeof(MpegViBuf));
    vb->priority = priority;
    vb->tag = (MpegDmaTag *)UNCACHED_VIEW(mpeg_dma_alloc((g_vibuf_n + 1) * sizeof(MpegDmaTag)));
    vb->data = (u8 *)mpeg_dma_alloc(g_vibuf_size);
    FlushCache(0);
    vb->sema = CreateSema((struct SemaParam *)&g_vibuf_sema_param);
    return vb;
}

// FUNC 0x0025F938 mpeg_vibuf_delete
void mpeg_vibuf_delete(MpegViBuf *vb)
{
    DeleteSema(vb->sema);
    mpeg_dma_free((void *)PHYS(vb->tag));
    mpeg_dma_free(vb->data);
}

// FUNC 0x0025F978 mpeg_vibuf_get_size
int mpeg_vibuf_get_size(void)
{
    return g_vibuf_size;
}

// FUNC 0x0025F980 mpeg_player_get_buffered
// Bytes in the input ring not yet consumed by the IPU DMA.
int mpeg_player_get_buffered(MpegPlayer *pl)
{
    MpegViBuf *vb = pl->vibuf;

    return vb->readBytes + (vb->dmaN << g_vibuf_log2elm);
}

// FUNC 0x0025F9A0 mpeg_vibuf_reset
// Empty the ring, rebuild the tags and arm channel 4 in chain mode (not
// started: the first "no data" callback starts it).
int mpeg_vibuf_reset(MpegViBuf *vb)
{
    u32 tagw;
    int i;

    vb->readBytes = 0;
    vb->dmaN = 0;
    vb->dmaStart = 0;
    tagw = VIBUF_REF_TAGW();
    for (i = 0; i < g_vibuf_n; i++)
        vb->tag[i].tag = VIBUF_TAG(tagw, vb->data, i);
    vb->tag[i].tag = (u64)DMATAG_NEXT | ((u64)PHYS(vb->tag) << 32);
    PUT32(D4_QWC, 0);
    PUT32(D4_MADR, PHYS(vb->data));
    PUT32(D4_TADR, PHYS(vb->tag));
    DIntr();
    DMAC_PUT_CHCR_SUSPENDED(D4_CHCR, CHCR_MOD_CHAIN);  /* STR clear */
    EIntr();
    vb->state = VIBUF_RUNNING;
    return 0;
}

// FUNC 0x0025FB18 mpeg_vibuf_begin_put
// Free space at the write position, as one or two pieces (wrap); two
// elements are always kept free in front of the DMA.
void mpeg_vibuf_begin_put(MpegViBuf *vb, u8 **ptr1, int *len1, u8 **ptr2, int *len2)
{
    int pos, toEnd, space;

    WaitSema(vb->sema);
    pos = (((vb->dmaStart + vb->dmaN) << g_vibuf_log2elm) + vb->readBytes) & g_vibuf_size_mask;
    toEnd = g_vibuf_size - pos;
    space = ((g_vibuf_n - 2 - vb->dmaN) << g_vibuf_log2elm) - vb->readBytes;
    if (toEnd >= space) {
        *len1 = space;
        *len2 = 0;
        *ptr1 = vb->data + pos;
        *ptr2 = 0;
    } else {
        *len1 = toEnd;
        *ptr1 = vb->data + pos;
        *ptr2 = vb->data;
        *len2 = space - (g_vibuf_size - pos);
    }
    SignalSema(vb->sema);
}

// FUNC 0x0025FC00 mpeg_vibuf_end_put
void mpeg_vibuf_end_put(MpegViBuf *vb, int n)
{
    WaitSema(vb->sema);
    vb->readBytes += n;
    SignalSema(vb->sema);
}

// FUNC 0x0025FC48 mpeg_vibuf_add_dma
// "No data" callback (slot 0): stop channel 4, retire the elements it has
// finished, chain the complete elements written since (the old last tag
// becomes "ref", the new last one "refe") and restart the channel if
// anything is queued.
int mpeg_vibuf_add_dma(MpegViBuf *vb, void *arg)
{
    u32 chcr, curElem, tagw;
    int done, nNew, first, idx, i, read;

    if (g_mpeg_wake_thread != -1)
        WakeupThread(g_mpeg_wake_thread);
    WaitSema(vb->sema);
    tagw = CHCR_MOD_CHAIN;      /* one variable for both: the constant waits in $s2 across DIntr */
    DIntr();
    DMAC_PUT_CHCR_SUSPENDED(D4_CHCR, tagw);     /* stop */
    EIntr();
    chcr = REG32(D4_CHCR);
    if (REG32(D4_MADR) != VIBUF_TAGS_END(vb))
        curElem = (REG32(D4_MADR) - (u32)vb->data) >> g_vibuf_log2elm;
    else
        curElem = 0;

    done = ((int)curElem - vb->dmaStart + g_vibuf_n) & g_vibuf_n_mask;
    vb->dmaStart = (vb->dmaStart + done) & g_vibuf_n_mask;
    vb->dmaN = vb->dmaN - done;
    read = vb->readBytes;
    vb->readBytes = read & g_vibuf_elm_mask;
    nNew = read >> g_vibuf_log2elm;
    if (nNew > 0) {
        first = vb->dmaStart + vb->dmaN;
        tagw = VIBUF_REF_TAGW();
        idx = (first + (g_vibuf_n - 1)) & g_vibuf_n_mask;   /* the old "refe" */
        vb->tag[idx].tag = VIBUF_TAG(tagw, vb->data, idx);
        idx = first & g_vibuf_n_mask;
        for (i = 0; i < nNew - 1; i++) {
            vb->tag[idx].tag = VIBUF_TAG(tagw, vb->data, idx);
            idx = (idx + 1) & g_vibuf_n_mask;
        }
        tagw &= DMATAG_ID_CLEAR;                                    /* refe */
        vb->tag[idx].tag = VIBUF_TAG(tagw, vb->data, idx);
        vb->dmaN = vb->dmaN + nNew;
    }
    if (vb->dmaN != 0) {
        if (nNew > 0)
            chcr = (chcr & DMATAG_ID_CLEAR) | DMATAG_REF;  /* CHCR.TAG id: continue past the old end */
        DIntr();
        DMAC_PUT_CHCR_SUSPENDED(D4_CHCR, chcr | CHCR_STR);
        EIntr();
    }
    SignalSema(vb->sema);
    return 0;
}

// FUNC 0x0025FF08 mpeg_vibuf_stop_dma
// Callback slot 1: stop the IPU DMA, saving its state in vb->env.
int mpeg_vibuf_stop_dma(MpegViBuf *vb, void *arg)
{
    WaitSema(vb->sema);
    ipu_stop_dma(&vb->env);
    SignalSema(vb->sema);
    return 0;
}

// FUNC 0x0025FF48 mpeg_vibuf_restart_dma
// Callback slot 2: restart the IPU DMA from the saved env: rewind channel 4
// by the quadwords still in the IPU input FIFO (IFC + FP), fixing up the
// tag / chain position and the ring counters when the rewind crosses an
// element (or the start of the ring); restart channel 3 if it had work;
// reset the IPU bit pointer (only if data is queued); restore IPU_CTRL.
int mpeg_vibuf_restart_dma(MpegViBuf *vb, void *arg)
{
    u32 bp, fifoQwc, data, madr, qwc, tadr, chcr, tagsEnd, elemOld, elemNew, oldPos, endPos;
    int tagId, n, start;

    WaitSema(vb->sema);
    bp = vb->env.ipubp;
    fifoQwc = ((bp >> 8) & 0xF) + ((bp >> 16) & 3);    /* IFC + FP */
    data = (u32)vb->data;
    madr = vb->env.d4madr - fifoQwc * 16;
    qwc = vb->env.d4qwc + fifoQwc;
    tadr = vb->env.d4tadr;
    chcr = vb->env.d4chcr;
    if (madr < PHYS(data)) {
        /* rewound past the start of the ring: continue from its end */
        qwc = (data - madr) >> 4;
        madr += g_vibuf_size;
        tadr = PHYS(vb->tag);
        if (vb->env.d4madr == data)
            tagId = 0;
        else if (vb->env.d4madr == data + (g_vibuf_n << g_vibuf_log2elm))
            tagId = 0;
        else
            tagId = 3;
        n = g_vibuf_n;
        chcr = (chcr & DMATAG_ID_CLEAR) | ((u32)tagId << 28);
        if ((u32)((n - vb->dmaStart) & g_vibuf_n_mask) >= (u32)vb->dmaN) {
            vb->dmaStart = n - 1;
            vb->dmaN = vb->dmaN + 1;
        }
    } else {
        tagsEnd = VIBUF_TAGS_END(vb);
        elemOld = (vb->env.d4madr != tagsEnd) ? (vb->env.d4madr - data) >> g_vibuf_log2elm : 0;
        elemNew = (madr != tagsEnd) ? (madr - data) >> g_vibuf_log2elm : 0;
        if (elemOld != elemNew) {
            /* rewound into the previous element */
            start = vb->dmaStart;
            n = vb->dmaN;
            endPos = data + (((start + n) & g_vibuf_n_mask) << g_vibuf_log2elm);
            oldPos = data + ((vb->env.d4madr - data) & g_vibuf_size_mask);
            tagId = (oldPos ^ endPos) == 0 ? 0 : 3;
            tadr = PHYS((u32)vb->tag + elemOld * 16);
            chcr = (chcr & DMATAG_ID_CLEAR) | ((u32)tagId << 28);
            qwc = (data + (elemOld << g_vibuf_log2elm) - madr) >> 4;
            if ((u32)(((int)elemNew + g_vibuf_n - start) & g_vibuf_n_mask) >= (u32)n) {
                vb->dmaStart = elemNew;
                vb->dmaN = n + 1;
            }
        }
    }

    if (vb->env.d3madr != 0 && vb->env.d3qwc != 0) {
        PUT32(D3_MADR, vb->env.d3madr);
        PUT32(D3_QWC, vb->env.d3qwc);
        DIntr();
        DMAC_PUT_CHCR_SUSPENDED(D3_CHCR, vb->env.d3chcr | CHCR_STR);
        EIntr();
    }
    if (vb->dmaN != 0)
        ipu_bclr(bp & 0x7F);
    PUT32(D4_MADR, madr);
    PUT32(D4_TADR, tadr);
    PUT32(D4_QWC, qwc);
    if (vb->dmaN != 0) {
        DIntr();
        DMAC_PUT_CHCR_SUSPENDED(D4_CHCR, chcr | CHCR_STR);
        EIntr();
    }
    PUT32(IPU_CTRL, vb->env.ipuctrl);
    SignalSema(vb->sema);
    return 0;
}

// FUNC 0x002602E8 mpeg_frameq_create
// threadPriority: the decoder thread priority (mpeg_player_create's
// priority), only stored.
MpegFrameQueue *mpeg_frameq_create(int elemSize, int count, int priority, int threadPriority)
{
    MpegFrameQueue *q;
    struct SemaParam param;     /* only maxCount / initCount are set */

    q = (MpegFrameQueue *)malloc(sizeof(MpegFrameQueue));
    memset(q, 0, sizeof(MpegFrameQueue));
    q->lockSema = CreateSema((struct SemaParam *)&g_frameq_sema_param);
    q->bufSize = elemSize * count;
    q->buf = (u8 *)mpeg_dma_alloc(q->bufSize);
    memset(q->buf, 0, q->bufSize);
    q->elemSize = elemSize;
    q->priority = priority;
    q->threadPriority = threadPriority;
    param.maxCount = count;
    q->count = count;
    q->filled = 0;
    q->writeIdx = 0;
    param.initCount = count;
    q->freeSema = CreateSema(&param);
    return q;
}

// FUNC 0x002603B8 mpeg_frameq_delete
void mpeg_frameq_delete(MpegFrameQueue *q)
{
    if (q->freeSema != -1) {
        DeleteSema(q->freeSema);
        q->freeSema = -1;
    }
    DeleteSema(q->lockSema);
    mpeg_dma_free(q->buf);
    free(q);
}

// FUNC 0x00260410 mpeg_frameq_reset
void mpeg_frameq_reset(MpegFrameQueue *q)
{
    WaitSema(q->lockSema);
    q->filled = 0;
    q->writeIdx = 0;
    memset(q->buf, 0, q->bufSize);
    SignalSema(q->lockSema);
}

// FUNC 0x00260458 mpeg_frameq_abort
// Delete the free-slot semaphore (releasing a writer blocked on it) at the
// queue's priority, then restore the caller's priority: the EE kernel's
// ChangeThreadPriority returns the previous priority (mpegif uses it the
// same way, T529 0x0025E970).
void mpeg_frameq_abort(MpegFrameQueue *q)
{
    int tid = GetThreadId();
    int oldPriority = ChangeThreadPriority(tid, q->priority);

    if (q->freeSema != -1) {
        DeleteSema(q->freeSema);
        q->freeSema = -1;
    }
    ChangeThreadPriority(tid, oldPriority);
}

// FUNC 0x002604C8 mpeg_frameq_is_full
int mpeg_frameq_is_full(MpegFrameQueue *q)
{
    int full;

    WaitSema(q->lockSema);
    full = q->filled == q->count;
    SignalSema(q->lockSema);
    return full;
}

// FUNC 0x00260518 mpeg_frameq_is_empty
int mpeg_frameq_is_empty(MpegFrameQueue *q)
{
    int empty;

    WaitSema(q->lockSema);
    empty = q->filled == 0;
    SignalSema(q->lockSema);
    return empty;
}

// FUNC 0x00260560 mpeg_frameq_begin_write
// Wait for a free slot (while the semaphore exists) and return it.
u8 *mpeg_frameq_begin_write(MpegFrameQueue *q)
{
    u8 *slot;

    for (;;) {
        if (q->freeSema != -1)
            WaitSema(q->freeSema);
        if (q->freeSema == -1 || !mpeg_frameq_is_full(q))
            break;
    }
    WaitSema(q->lockSema);
    slot = q->buf + q->writeIdx * q->elemSize;
    SignalSema(q->lockSema);
    return slot;
}

// FUNC 0x002605E8 mpeg_frameq_end_write
void mpeg_frameq_end_write(MpegFrameQueue *q)
{
    WaitSema(q->lockSema);
    q->filled++;
    if (g_frameq_filled_once == 0 && q->filled == q->count)
        g_frameq_filled_once = 1;
    q->writeIdx = (q->writeIdx + 1) % q->count;
    SignalSema(q->lockSema);
}

// FUNC 0x00260660 mpeg_frameq_peek
// The oldest filled slot, or 0 when empty or deleted.
u8 *mpeg_frameq_peek(MpegFrameQueue *q)
{
    u8 *slot;

    if (q->freeSema == -1 || mpeg_frameq_is_empty(q))
        return 0;
    WaitSema(q->lockSema);
    slot = q->buf + ((q->writeIdx - q->filled + q->count) % q->count) * q->elemSize;
    SignalSema(q->lockSema);
    return slot;
}

// FUNC 0x002606F0 mpeg_frameq_end_read
void mpeg_frameq_end_read(MpegFrameQueue *q)
{
    WaitSema(q->lockSema);
    if (q->filled > 0)
        q->filled--;
    SignalSema(q->lockSema);
    if (q->freeSema != -1)
        SignalSema(q->freeSema);
}

// FUNC 0x00260748 mpeg_pes_parse
// *data / *len: the payload; *pts / *dts: -1 when absent.  Returns 0 on
// success, 1 for a packet that is not an MPEG-2 video PES or a bad stamp.
int mpeg_pes_parse(const u8 *pkt, const u8 **data, int *len, s64 *pts, s64 *dts)
{
    u32 startCode, ptsDtsFlags;
    int packetLen, headerLen;

    *len = 0;
    startCode = (pkt[0] << 24) | pkt[3] | ((pkt[2] << 8) | (pkt[1] << 16));
    if ((startCode >> 8) != START_CODE_PREFIX)
        return 1;
    if ((startCode & 0xF0) != 0xE0)             /* video stream 0xE0..0xEF */
        return 1;
    packetLen = pkt[5] | (pkt[4] << 8);         /* PES_packet_length */
    if ((pkt[6] & 0xC0) != 0x80)                /* '10': an MPEG-2 PES header */
        return 1;
    headerLen = pkt[8];                         /* PES_header_data_length */
    *dts = -1;
    ptsDtsFlags = pkt[7] >> 6;
    *len = packetLen - headerLen - 3;
    *data = pkt + headerLen + 9;
    *pts = -1;
    switch (ptsDtsFlags) {
    case PES_NO_PTS:
        return 0;
    case PES_PTS_ONLY:
    case PES_PTS_DTS:
        return mpeg_pes_parse_timestamps(pkt, 9, pts, dts) != 0 ? 1 : 0;
    default:
        return 1;
    }
}

// FUNC 0x00260880 mpeg_pes_parse_timestamps
// PTS at pkt[off] ('0010' or '0011' prefix), then the DTS ('0001') if the
// prefix was '0011'.  Each stamp is 3 + 15 + 15 bits, each part followed by
// a marker bit.  Returns 0 ok, 1 bad prefix.
int mpeg_pes_parse_timestamps(const u8 *pkt, int off, s64 *pts, s64 *dts)
{
    u32 first = pkt[off];
    u32 prefix = first >> 4;
    const u8 *mid, *low;
    u64 stamp;

    off++;
    if ((u32)(prefix - 2) >= 2)                 /* '0010' or '0011' */
        return 1;
    mid = pkt + off;
    off += 2;
    low = pkt + off;
    off += 2;
    stamp = (u64)((first >> 1) & 7) << 30;
    *pts = stamp;
    stamp |= ((u64)(mid[1] | (mid[0] << 8)) >> 1) << 15;
    *pts = stamp;
    stamp |= (u64)(low[1] | (low[0] << 8)) >> 1;
    *pts = stamp;
    if ((prefix ^ 3) != 0)
        return 0;

    first = pkt[off];
    off++;
    if ((first >> 4) != 1)                      /* '0001' */
        return 1;
    mid = pkt + off;
    off += 2;
    low = pkt + off;
    stamp = (u64)((first >> 1) & 7) << 30;
    *dts = stamp;
    stamp |= ((u64)(mid[1] | (mid[0] << 8)) >> 1) << 15;
    *dts = stamp;
    stamp |= (u64)(low[1] | (low[0] << 8)) >> 1;
    *dts = stamp;
    return 0;
}

// FUNC 0x00260998 mpeg_video_decode_main
// The decoder loop: for each sequence (a picture start found), set up the
// frame stores and decode pictures until the sequence end code or a stop
// request (g_dec_state DEC_STOP_REQUEST -> DEC_STOPPED).  Leaves g_dec_state
// = DEC_ENDED.
int mpeg_video_decode_main(void)
{
    MpegCallbackSlot *cb = g_mpeg_callbacks;
    int total = 0;      /* pictures since the start */
    int npics;          /* pictures of this sequence */

    while (mpeg_video_next_picture() != 0) {
        mpeg_video_setup_sequence();
        npics = 0;
        do {
            total++;
            mpeg_video_decode_picture(npics++);
            if (total == 1) {
                WaitSema(g_dec_state_sema);
                g_dec_state = DEC_RUNNING;
                SignalSema(g_dec_state_sema);
            }
            WaitSema(g_dec_state_sema);
            if (g_dec_state == DEC_STOP_REQUEST) {
                g_dec_state = DEC_STOPPED;
                SignalSema(g_dec_state_sema);
                goto end;
            }
            SignalSema(g_dec_state_sema);
        } while (mpeg_video_next_picture() != 0);
        if (npics != 0)     /* always true here */
            cb[3].func(cb[3].arg, g_ref_back);      /* the last reference picture */
    }
end:
    mpeg_video_release_frames();
    WaitSema(g_dec_state_sema);
    g_dec_state = DEC_ENDED;
    SignalSema(g_dec_state_sema);
    return 0;
}

// FUNC 0x00260AB8 mpeg_video_setup_sequence
// Macroblock size of the sequence, the size callback (4), and three frame
// stores of mb_width * mb_height * 0x180 bytes.  Returns 1 (nothing done)
// for an MPEG-1 stream (no sequence extension).
int mpeg_video_setup_sequence(void)
{
    int size[2];        /* {width, height} for the size callback */
    int mbHeight, bytes;

    if (g_mpeg2 == 0)
        return 1;
    g_mb_width = (g_horizontal_size + 15) / 16;
    if (g_progressive_sequence != 0)
        mbHeight = (g_vertical_size + 15) / 16;
    else
        mbHeight = (g_vertical_size + 31) / 32;    /* quirk: the standard is 2 * ((v + 31) / 32) */
    size[0] = g_mb_width * 16;
    size[1] = mbHeight * 16;
    g_mb_height = mbHeight;
    MPEG_CALLBACK(4, size);
    bytes = g_mb_width * g_mb_height * MB_BYTES;
    if (g_mpeg_frames.allocated == 0) {
        mpeg_frames_alloc(bytes);
    } else if (g_mpeg_frames.size != bytes) {
        mpeg_frames_free();
        mpeg_frames_alloc(bytes);
    }
    mpeg_frames_assign();
    return 0;
}

// FUNC 0x00260BD0 mpeg_video_release_frames
void mpeg_video_release_frames(void)
{
    mpeg_frames_free();
}

// FUNC 0x00260BD8 mpeg_video_output_picture
// After picture npics (> 0) of a sequence: a B picture goes out itself, an
// I / P picture releases the previous reference.
void mpeg_video_output_picture(int npics)
{
    if (npics != 0) {
        if (g_picture_coding_type == PCT_B)
            MPEG_CALLBACK(3, g_frame_b);
        else
            MPEG_CALLBACK(3, g_ref_fwd);
    }
}

// FUNC 0x00260C30 mpeg_frames_alloc
void mpeg_frames_alloc(int size)
{
    int i;

    for (i = 0; i < 3; i++)
        g_mpeg_frames.buf[i] = (u8 *)memalign(0x80, size);
    g_mpeg_frames.size = size;
    g_mpeg_frames.allocated = 1;
}

// FUNC 0x00260CA8 mpeg_frames_free
// Quirk: buf[] keeps the freed pointers (a second call frees them again).
void mpeg_frames_free(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (g_mpeg_frames.buf[i] != 0)
            free(g_mpeg_frames.buf[i]);
    }
    g_mpeg_frames.allocated = 0;
    g_mpeg_frames.size = 0;
    g_frame_b = 0;
    g_ref_fwd = 0;
    g_ref_back = 0;
}

// FUNC 0x00260D20 mpeg_frames_assign
void mpeg_frames_assign(void)
{
    u8 **buf = g_mpeg_frames.buf;

    g_frame_b = buf[0];
    g_ref_fwd = buf[1];
    g_ref_back = buf[2];
}

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
// FUNC 0x00260D48 mpeg_mb_put_block
// Store one macroblock of IDCT output (s16[384], raw8 order) into a frame:
// dst = clamp(coef, 0, 255), or with add: clamp(dst + coef, 0, 255).
// 12 passes of 32 values, the next 64 coefficient bytes prefetched.  Quirk:
// the first pref goes through s before it is set (whatever the caller left
// in $a3).
void mpeg_mb_put_block(u8 *dst, const s16 *coef, int add)
{
    u128 *s, *d;
    u128 max;

    MMI_PREF(s);
    s = (u128 *)coef;
    d = (u128 *)dst;
    MMI_PNOR0(max);
    MMI_PSRLH(max, max, 8);
    if (!add) {
        u128 *end = s + 0x30, *next;
        u128 c0, c1, c2, c3;
        do {
            MMI_LQ(c0, s, 0);
            next = s + 4;
            if (next != end)
                MMI_PREF(next);
            MMI_LQ(c1, s, 16);
            MMI_PMAXH0(c0, c0); MMI_PMINH(c0, c0, max);
            MMI_LQ(c2, s, 32);
            MMI_PMAXH0(c1, c1); MMI_PMINH(c1, c1, max);
            MMI_PPACB(c1, c1, c0);
            MMI_LQ(c3, s, 48);
            MMI_PMAXH0(c2, c2); MMI_PMINH(c2, c2, max);
            MMI_PMAXH0(c3, c3); MMI_PMINH(c3, c3, max);
            MMI_PPACB(c3, c3, c2);
            MMI_SQ(c1, d, 0);
            MMI_SQ(c3, d, 16);
            MMI_ADDIU32(d);
            s = next;
        } while (s != end);
    } else {
        u128 *end = s + 0x30, *next;
        u128 c0, c1, c2, c3, d0, d1, w0, w1, w3, w2;
        do {
            MMI_LQ(d0, d, 0);
            MMI_LQ(c0, s, 0);
            next = s + 4;
            if (next != end)
                MMI_PREF(next);
            MMI_PEXTLB0(w0, d0); MMI_PEXTUB0(w1, d0);
            MMI_LQ(c1, s, 16);
            MMI_PADDH(c0, c0, w0); MMI_PMAXH0(c0, c0); MMI_PMINH(c0, c0, max);
            MMI_LQ(d1, d, 16);
            MMI_LQ(c2, s, 32);
            MMI_PADDH(c1, c1, w1); MMI_PMAXH0(c1, c1); MMI_PMINH(c1, c1, max);
            MMI_PPACB(d0, c1, c0);
            MMI_PEXTLB0(w2, d1); MMI_PEXTUB0(w3, d1);
            MMI_LQ(c3, s, 48);
            MMI_PADDH(c2, c2, w2); MMI_PMAXH0(c2, c2); MMI_PMINH(c2, c2, max);
            MMI_PADDH(c3, c3, w3); MMI_PMAXH0(c3, c3); MMI_PMINH(c3, c3, max);
            MMI_PPACB(d1, c3, c2);
            MMI_SQ(d0, d, 0);
            MMI_SQ(d1, d, 16);
            MMI_ADDIU32(d);
            s = next;
        } while (s != end);
    }
}
#else
/* mpeg_mb_put_block (0x00260D48) */
__asm__(MC_ASM_BEGIN(mpeg_mb_put_block)
    "	pref	0, 0($7)\n"
    "	daddu	$7, $5, $0\n"
    "	daddu	$10, $4, $0\n"
    "	pnor	$9, $0, $0\n"
    "	psrlh	$9, $9, 8\n"
    "	bnez	$6, .Lmpeg_mb_put_block_3\n"
    "	addiu	$12, $7, 768\n"
    "	addiu	$6, $7, 768\n"
    ".Lmpeg_mb_put_block_1:\n"
    "	lq	$3, 0($7)\n"
    "	addiu	$5, $7, 64\n"
    "	beq	$5, $6, .Lmpeg_mb_put_block_2\n"
    "	nop\n"
    "	pref	0, 0($5)\n"
    ".Lmpeg_mb_put_block_2:\n"
    "	lq	$2, 16($7)\n"
    "	pmaxh	$3, $3, $0\n"
    "	pminh	$3, $3, $9\n"
    "	lq	$4, 32($7)\n"
    "	pmaxh	$2, $2, $0\n"
    "	pminh	$2, $2, $9\n"
    "	ppacb	$2, $2, $3\n"
    "	lq	$3, 48($7)\n"
    "	pmaxh	$4, $4, $0\n"
    "	pminh	$4, $4, $9\n"
    "	pmaxh	$3, $3, $0\n"
    "	pminh	$3, $3, $9\n"
    "	ppacb	$3, $3, $4\n"
    "	sq	$2, 0($10)\n"
    "	sq	$3, 16($10)\n"
    "	addiu	$10, $10, 32\n"
    "	daddu	$7, $5, $0\n"
    "	bne	$7, $6, .Lmpeg_mb_put_block_1\n"
    "	nop\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mb_put_block_3:\n"
    "	lq	$8, 0($10)\n"
    "	lq	$6, 0($7)\n"
    "	addiu	$11, $7, 64\n"
    "	beq	$11, $12, .Lmpeg_mb_put_block_4\n"
    "	nop\n"
    "	pref	0, 0($11)\n"
    ".Lmpeg_mb_put_block_4:\n"
    "	pextlb	$3, $0, $8\n"
    "	pextub	$4, $0, $8\n"
    "	lq	$2, 16($7)\n"
    "	paddh	$6, $6, $3\n"
    "	pmaxh	$6, $6, $0\n"
    "	pminh	$6, $6, $9\n"
    "	lq	$5, 16($10)\n"
    "	lq	$3, 32($7)\n"
    "	paddh	$2, $2, $4\n"
    "	pmaxh	$2, $2, $0\n"
    "	pminh	$2, $2, $9\n"
    "	ppacb	$8, $2, $6\n"
    "	pextlb	$4, $0, $5\n"
    "	pextub	$5, $0, $5\n"
    "	lq	$2, 48($7)\n"
    "	paddh	$3, $3, $4\n"
    "	pmaxh	$3, $3, $0\n"
    "	pminh	$3, $3, $9\n"
    "	paddh	$2, $2, $5\n"
    "	pmaxh	$2, $2, $0\n"
    "	pminh	$2, $2, $9\n"
    "	ppacb	$5, $2, $3\n"
    "	sq	$8, 0($10)\n"
    "	sq	$5, 16($10)\n"
    "	addiu	$10, $10, 32\n"
    "	daddu	$7, $11, $0\n"
    "	bne	$7, $12, .Lmpeg_mb_put_block_3\n"
    "	nop\n"
    "	jr	$31\n"
    "	nop\n"
    MC_ASM_END(mpeg_mb_put_block));
#endif

// FUNC 0x00260E60 mpeg_video_decode_picture
void mpeg_video_decode_picture(int npics)
{
    int mbTotal;

    mpeg_video_rotate_frames();
    mbTotal = g_mb_width * g_mb_height;
    if ((g_picture_structure ^ PS_FRAME) != 0)
        mbTotal >>= 1;              /* field picture */
    while (mpeg_video_decode_slice(mbTotal) >= 0)
        ;
    mpeg_video_output_picture(npics);
}

// FUNC 0x00260EC0 mpeg_video_decode_slice
// One slice.  Returns 0 at the next start code (more slices follow) or on a
// VLC error of the block decode, -1 when the picture is complete (or the
// start code is not a slice: then it has been consumed), else an error of
// the vectors (never: mpeg_video_motion_vectors returns 0).
int mpeg_video_decode_slice(int mb_total)
{
    MpegMv mv;              /* sp+0x00 */
    int mbAddr = 0;         /* sp+0x60 */
    int mbInc = 0;          /* sp+0x64 */
    int mbType;             /* sp+0x68 (a skipped first macroblock reuses it unset) */
    int dctType;            /* sp+0x6C */
    int result, notIntra, mbX, mbY;
    s16 *coef = SPR_MB_COEF;    /* $s3 from the entry: only the bdec.out store uses it */

    result = mpeg_video_slice_header(&mbAddr, &mbInc);
    if (result != 1)
        return result;
    g_mpeg_bdec.dc_reset = result;
    PMV_RESET_DIR(mv, 1);
    PMV_RESET_DIR(mv, 0);
    if (mbAddr < mb_total) {
        do {
            if (mbInc == 0) {
                if (ipu_showbits(23) == 0)
                    return 0;               /* next start code */
                mbInc = ipu_vdec_mb_addr_inc();
            }
            if (mbInc == 1) {
                /* coded macroblock */
                mpeg_video_macroblock_modes(&mbType, &dctType, &mv);
                mv.stwtype = 0;
                if (mbType & MB_QUANT)
                    g_mpeg_bdec.qscale = ipu_getbits(5);    /* quantiser_scale_code */
                if (mbType & MB_MOTION_FWD) {
                    result = mpeg_video_motion_vectors(&mv, 0, g_mpeg_f_code[0][0] - 1, g_mpeg_f_code[0][1] - 1);
                    if (result != 0)
                        return result;
                }
                if (mbType & MB_MOTION_BWD) {
                    result = mpeg_video_motion_vectors(&mv, 1, g_mpeg_f_code[1][0] - 1, g_mpeg_f_code[1][1] - 1);
                    if (result != 0)
                        return result;
                }
                memset(SPR_MB_COEF, 0, MB_COEF_BYTES);
                g_mpeg_bdec.pict_type = g_picture_coding_type;
                g_mpeg_bdec.mb_type = mbType;
                g_mpeg_bdec.dct_type = dctType;
                g_mpeg_bdec.out = coef;
                if (ipu_bdec(&g_mpeg_bdec) == 3)    /* VLC error ($a1 = dctType at the call: a leftover) */
                    return 0;
                g_mpeg_bdec.dc_reset = 0;
                if (g_picture_coding_type == PCT_D)
                    ipu_flushbits(1);       /* end_of_macroblock */
                if (!(mbType & MB_INTRA)) {
                    g_mpeg_bdec.dc_reset = 1;
                } else {
                    /* intra: reset the vector predictors */
                    PMV_RESET_DIR(mv, 1);
                    PMV_RESET_DIR(mv, 0);
                }
                if (g_picture_coding_type == PCT_P && !(mbType & (MB_MOTION_FWD | MB_INTRA))) {
                    /* P macroblock without forward vector: zero vector */
                    PMV_RESET_DIR(mv, 0);
                    if (g_picture_structure == PS_FRAME) {
                        mv.motion_type = MT_FRAME;
                    } else {
                        mv.motion_type = MT_FIELD;
                        mv.field_select[0][0] = (g_picture_structure == PS_BOTTOM_FIELD);
                    }
                }
            } else {
                /* skipped macroblock: no residual, the previous modes */
                memset(SPR_MB_COEF, 0, MB_COEF_BYTES);
                g_mpeg_bdec.dc_reset = 1;
                if (g_picture_coding_type == PCT_P)
                    PMV_RESET_DIR(mv, 0);
                if (g_picture_structure == PS_FRAME) {
                    mv.motion_type = MT_FRAME;
                } else {
                    mv.motion_type = MT_FIELD;
                    mv.field_select[0][0] = mv.field_select[0][1] = (g_picture_structure == PS_BOTTOM_FIELD);
                }
                if (g_picture_coding_type == PCT_I)
                    mv.stwtype = 8;
                mbType &= ~MB_INTRA;
            }
            notIntra = (mbType ^ MB_INTRA) & 1;
            mbY = mbAddr / g_mb_width;
            mbX = mbAddr % g_mb_width;
            if (notIntra)
                mpeg_video_motion_compensate(&mv, mbType, mbX, mbY);
            mpeg_mb_put_block(g_frame_cur + (mbX * g_mb_height + mbY) * MB_BYTES, SPR_MB_COEF, notIntra);
            mbAddr++;
            mbInc--;
        } while (mbAddr < mb_total);
    }
    return -1;
}

// FUNC 0x00261260 mpeg_video_macroblock_modes
// macroblock_type, frame/field_motion_type and dct_type, and the derived
// vector count / format / dual-prime flags.  Always returns 0.  MSSG
// mpeg2decode's macroblock_modes without the scalability and concealment
// parts; motion_type starts at MT_NONE (-39) instead of 0.  Two of MSSG's
// output pointers survive as locals (retail computes &mv->motion_type and
// &mv->dual.dmv at entry, in $fp and a stack slot).
int mpeg_video_macroblock_modes(int *mb_type_out, int *dct_type_out, MpegMv *mv)
{
    int *pmotion_type = &mv->motion_type;
    int *pdmv = &mv->dual.dmv;
    int macroblock_type;
    int motion_type = MT_NONE;
    int motion_vector_count, mv_format, dmv, mvscale;
    int dct_type;

    macroblock_type = ipu_vdec_mb_type(g_picture_coding_type);
    if (macroblock_type & (MB_MOTION_FWD | MB_MOTION_BWD)) {
        if (g_picture_structure == PS_FRAME)
            motion_type = g_frame_pred_frame_dct ? MT_FRAME : ipu_getbits(2);
        else
            motion_type = ipu_getbits(2);
    }
    if (g_picture_structure == PS_FRAME) {
        motion_vector_count = (motion_type == MT_FIELD) ? 2 : 1;
        mv_format = (motion_type == MT_FRAME) ? 1 : 0;
    } else {
        motion_vector_count = (motion_type == MT_FRAME) ? 2 : 1;
        mv_format = 0;
    }
    dmv = (motion_type == MT_DUAL_PRIME);
    mvscale = (mv_format == 0 && g_picture_structure == PS_FRAME);
    dct_type = (g_picture_structure == PS_FRAME && !g_frame_pred_frame_dct
                && (macroblock_type & (MB_PATTERN | MB_INTRA))) ? ipu_getbits(1) : 0;
    *mb_type_out = macroblock_type;
    *dct_type_out = dct_type;
    *pmotion_type = motion_type;
    mv->mv_count = motion_vector_count;
    mv->mv_format_frame = mv_format;
    mv->field_in_frame = mvscale;
    *pdmv = dmv;
    return 0;
}

// FUNC 0x002613C0 mpeg_video_rotate_frames
// The store for the next picture: the B store for a B picture, else the
// older reference, which becomes the newest one.
void mpeg_video_rotate_frames(void)
{
    u8 *older;

    if (g_picture_coding_type == PCT_B) {
        g_frame_cur = g_frame_b;
    } else {
        older = g_ref_fwd;
        g_ref_fwd = g_ref_back;
        g_ref_back = older;
        g_frame_cur = older;
    }
}

// FUNC 0x002613F0 mpeg_video_slice_header
// At a slice start code (0x101..0x1AF): read the header and the first
// macroblock_address_increment; *mb_addr = (row - 1) * mb_width + increment
// - 1 (the first macroblock), *mb_inc = 1; returns 1.  Any other start code
// is skipped and -1 returned.
int mpeg_video_slice_header(int *mb_addr, int *mb_inc)
{
    u32 code;
    int inc;

    mpeg_video_next_start_code();
    code = ipu_showbits(32);
    if ((u32)(code - SC_SLICE_FIRST) >= SC_SLICE_LAST - SC_SLICE_FIRST + 1) {
        ipu_flushbits(32);
        return -1;
    }
    ipu_flushbits(32);
    mpeg_video_slice_qscale();
    inc = ipu_vdec_mb_addr_inc();
    *mb_inc = inc;
    *mb_addr = ((int)(code & 0xFF) - 1) * g_mb_width + inc - 1;    /* slice_vertical_position */
    *mb_inc = 1;
    return 1;
}

// FUNC 0x002614A0 mpeg_video_next_picture
// Parse headers up to the next picture start code: 1 with the picture
// header read, 0 at the sequence end code.  (MSSG mpeg2decode's Get_Hdr.)
int mpeg_video_next_picture(void)
{
    u32 code;

    for (;;) {
        mpeg_video_next_start_code();
        code = ipu_getbits(32);
        switch (code) {
        case SC_SEQUENCE_HEADER:
            mpeg_video_sequence_header();
            break;
        case SC_GROUP:
            mpeg_video_gop_header();
            break;
        case SC_PICTURE:
            mpeg_video_picture_header();
            return 1;
        case SC_SEQUENCE_END:
            return 0;
        }
    }
}

// FUNC 0x00261560 mpeg_video_next_start_code
void mpeg_video_next_start_code(void)
{
    ipu_align_byte();
    while (ipu_showbits24() != START_CODE_PREFIX)
        ipu_fdec8();                /* skip a byte */
}

// FUNC 0x002615A8 mpeg_video_sequence_header
void mpeg_video_sequence_header(void)
{
    g_horizontal_size = ipu_getbits(12);
    g_vertical_size = ipu_getbits(12);
    ipu_getbits(4);             /* aspect_ratio_information */
    ipu_getbits(4);             /* frame_rate_code */
    ipu_getbits(18);            /* bit_rate_value */
    ipu_flushbits(1);           /* marker */
    ipu_getbits(10);            /* vbv_buffer_size_value */
    ipu_getbits(1);             /* constrained_parameters_flag */
    if (ipu_getbits(1) != 0)
        ipu_load_intra_matrix();        /* intra_quantiser_matrix */
    if (ipu_getbits(1) != 0)
        ipu_load_nonintra_matrix();     /* non_intra_quantiser_matrix */
    mpeg_video_extension_and_user_data();
}

// FUNC 0x00261638 mpeg_video_gop_header
void mpeg_video_gop_header(void)
{
    ipu_getbits(1);             /* drop_frame_flag */
    ipu_getbits(5);             /* hours */
    ipu_getbits(6);             /* minutes */
    ipu_flushbits(1);           /* marker */
    ipu_getbits(6);             /* seconds */
    ipu_getbits(6);             /* pictures */
    ipu_getbits(1);             /* closed_gop */
    ipu_getbits(1);             /* broken_link */
    mpeg_video_extension_and_user_data();
}

// FUNC 0x00261690 mpeg_video_picture_header
void mpeg_video_picture_header(void)
{
    ipu_getbits(10);            /* temporal_reference */
    g_picture_coding_type = ipu_getbits(3);
    ipu_getbits(16);            /* vbv_delay */
    if ((u32)(g_picture_coding_type - PCT_P) < 2) {     /* P or B */
        ipu_getbits(1);         /* full_pel_forward_vector */
        ipu_getbits(3);         /* forward_f_code */
    }
    if (g_picture_coding_type == PCT_B) {
        ipu_getbits(1);         /* full_pel_backward_vector */
        ipu_getbits(3);         /* backward_f_code */
    }
    while (ipu_getbits(1) != 0) /* extra_bit_picture */
        ipu_flushbits(8);       /* extra_information_picture */
    mpeg_video_extension_and_user_data();
}

// FUNC 0x00261720 mpeg_video_slice_qscale
void mpeg_video_slice_qscale(void)
{
    g_mpeg_bdec.qscale = ipu_getbits(5);    /* quantiser_scale_code */
    ipu_getbits(1);             /* extra_bit_slice */
}

// FUNC 0x00261748 mpeg_video_extension_and_user_data
// Extensions (0x1B5: sequence 1, sequence display 2, picture coding 8) and
// user data (0x1B2, skipped).
void mpeg_video_extension_and_user_data(void)
{
    u32 code;

    mpeg_video_next_start_code();
    for (;;) {
        code = ipu_showbits(32);
        if (code != SC_EXTENSION && code != SC_USER_DATA)
            break;
        ipu_flushbits(32);
        if (code != SC_EXTENSION) {
            mpeg_video_next_start_code();
            continue;
        }
        switch (ipu_getbits(4)) {   /* extension_start_code_identifier */
        case EXT_SEQUENCE:
            mpeg_video_sequence_extension();
            break;
        case EXT_SEQUENCE_DISPLAY:
            mpeg_video_sequence_display_extension();
            break;
        case EXT_PICTURE_CODING:
            mpeg_video_picture_coding_extension();
            break;
        }
        mpeg_video_next_start_code();
    }
}

// FUNC 0x00261850 mpeg_video_sequence_extension
void mpeg_video_sequence_extension(void)
{
    u32 hSizeExt, vSizeExt;

    g_mpeg2 = 1;
    ipu_getbits(8);             /* profile_and_level_indication */
    g_progressive_sequence = ipu_getbits(1);
    ipu_getbits(2);             /* chroma_format */
    hSizeExt = ipu_getbits(2);  /* horizontal_size_extension */
    vSizeExt = ipu_getbits(2);  /* vertical_size_extension */
    ipu_getbits(12);            /* bit_rate_extension */
    ipu_flushbits(1);           /* marker */
    ipu_getbits(8);             /* vbv_buffer_size_extension */
    ipu_getbits(1);             /* low_delay */
    ipu_getbits(2);             /* frame_rate_extension_n */
    ipu_getbits(5);             /* frame_rate_extension_d */
    g_horizontal_size = (hSizeExt << 12) | (g_horizontal_size & 0xFFF);
    g_vertical_size = (vSizeExt << 12) | (g_vertical_size & 0xFFF);
}

// FUNC 0x00261908 mpeg_video_sequence_display_extension
void mpeg_video_sequence_display_extension(void)
{
    ipu_getbits(3);             /* video_format */
    if (ipu_getbits(1) != 0) {  /* colour_description */
        ipu_getbits(8);         /* colour_primaries */
        ipu_getbits(8);         /* transfer_characteristics */
        ipu_getbits(8);         /* matrix_coefficients */
    }
    ipu_getbits(14);            /* display_horizontal_size */
    ipu_flushbits(1);           /* marker */
    ipu_getbits(14);            /* display_vertical_size */
}

// FUNC 0x00261960 mpeg_video_picture_coding_extension
void mpeg_video_picture_coding_extension(void)
{
    u32 dcPrecision, qScaleType, intraVlcFormat, alternateScan;

    g_mpeg_f_code[0][0] = ipu_getbits(4);
    g_mpeg_f_code[0][1] = ipu_getbits(4);
    g_mpeg_f_code[1][0] = ipu_getbits(4);
    g_mpeg_f_code[1][1] = ipu_getbits(4);
    dcPrecision = ipu_getbits(2);
    g_picture_structure = ipu_getbits(2);
    g_top_field_first = ipu_getbits(1);
    g_frame_pred_frame_dct = ipu_getbits(1);
    ipu_getbits(1);             /* concealment_motion_vectors */
    qScaleType = ipu_getbits(1);
    intraVlcFormat = ipu_getbits(1);
    alternateScan = ipu_getbits(1);
    ipu_getbits(1);             /* repeat_first_field */
    ipu_getbits(1);             /* chroma_420_type */
    ipu_getbits(1);             /* progressive_frame */
    if (ipu_getbits(1) != 0) {  /* composite_display_flag */
        ipu_getbits(1);         /* v_axis */
        ipu_getbits(3);         /* field_sequence */
        ipu_getbits(1);         /* sub_carrier */
        ipu_getbits(7);         /* burst_amplitude */
        ipu_getbits(8);         /* sub_carrier_phase */
    }
    g_mpeg_bdec.dc_prec = dcPrecision;
    g_mpeg_bdec.intra_vlc = intraVlcFormat;
    g_mpeg_bdec.q_scale_type = qScaleType;
    g_mpeg_bdec.alt_scan = alternateScan;
}

// FUNC 0x00261A90 mpeg_video_motion_vectors
// motion_vectors(s): one vector (copied to PMV[1][s]) or two with their
// field selects.  Always returns 0.
int mpeg_video_motion_vectors(MpegMv *mv, int s, int r_size_h, int r_size_v)
{
    if (mv->mv_count == 1) {
        mpeg_video_motion_vector(mv, mv->pmv[0][s], r_size_h, r_size_v);
        mv->pmv[1][s][0] = mv->pmv[0][s][0];
        mv->pmv[1][s][1] = mv->pmv[0][s][1];
    } else {
        mv->field_select[0][s] = ipu_getbits(1);
        mpeg_video_motion_vector(mv, mv->pmv[0][s], r_size_h, r_size_v);
        mv->field_select[1][s] = ipu_getbits(1);
        mpeg_video_motion_vector(mv, mv->pmv[1][s], r_size_h, r_size_v);
    }
    return 0;
}

// FUNC 0x00261B78 mpeg_video_motion_vector
// motion_vector(r, s): horizontal then vertical component (the vertical
// predictor halved around the update for field vectors in a frame picture),
// with the dual-prime dmvectors (MSSG's motion_vector, its dmv / dmvector
// arguments reached through one pointer: retail keeps &mv->dual in $s4).
void mpeg_video_motion_vector(MpegMv *mv, int *pmv, int r_size_h, int r_size_v)
{
    int mvscale = mv->field_in_frame;
    MpegDmv *d = &mv->dual;
    int motion_code, motion_residual;

    motion_code = ipu_vdec_motion_code();
    motion_residual = (r_size_h != 0 && motion_code != 0) ? ipu_getbits(r_size_h) : 0;
    mpeg_video_decode_motion_vector(&pmv[0], r_size_h, motion_code, motion_residual);
    if (d->dmv)
        d->dmvector[0] = ipu_vdec_dmvector();

    motion_code = ipu_vdec_motion_code();
    motion_residual = (r_size_v != 0 && motion_code != 0) ? ipu_getbits(r_size_v) : 0;
    if (mvscale)
        pmv[1] >>= 1;
    mpeg_video_decode_motion_vector(&pmv[1], r_size_v, motion_code, motion_residual);
    if (mvscale)
        pmv[1] <<= 1;
    if (d->dmv)
        d->dmvector[1] = ipu_vdec_dmvector();
}

// FUNC 0x00261C98 mpeg_video_decode_motion_vector
// decode_motion_vector: update the predictor with wrap-around to
// [-16 << r_size, (16 << r_size) - 1].  MSSG mpeg2decode's (motion.c)
// without full_pel_vector, written as there: `+ 1' last in the sum (egcs
// adds it to vec first, as the original does).
void mpeg_video_decode_motion_vector(int *pred, int r_size, int code, int residual)
{
    int lim, vec;

    lim = 16 << r_size;
    vec = *pred;
    if (code > 0) {
        vec += ((code - 1) << r_size) + residual + 1;
        if (vec >= lim)
            vec -= lim + lim;
    } else if (code < 0) {
        vec -= ((-code - 1) << r_size) + residual + 1;
        if (vec < -lim)
            vec += lim + lim;
    }
    *pred = vec;
}

// FUNC 0x00261D08 mpeg_video_motion_compensate
// Prediction of a non-intra macroblock in the scratchpad (0x70000600): the
// current macroblock is fetched first (mpeg_get_mb), then forward from the
// older reference (also for any P macroblock), backward from the newer one,
// averaged when both; then stored back to the current frame (mpeg_put_mb),
// where mpeg_mb_put_block adds the residual.
void mpeg_video_motion_compensate(MpegMv *mv, u32 mb_type, int mb_x, int mb_y)
{
    int y = mb_y * 16;
    int x = mb_x * 16;
    int avg = 0;

    mpeg_get_mb((MpegMB *)g_mc_pred, g_frame_cur, x, y);
    if ((mb_type & MB_MOTION_FWD) || g_picture_coding_type == PCT_P) {
        mpeg_mc_form_prediction(g_ref_fwd, x, y, mv->pmv[0][0][0], mv->pmv[0][0][1], 0);
        avg = 1;
    }
    if (mb_type & MB_MOTION_BWD)
        mpeg_mc_form_prediction(g_ref_back, x, y, mv->pmv[0][1][0], mv->pmv[0][1][1], avg);
    mpeg_put_mb((const MpegMB *)g_mc_pred, g_frame_cur, x, y);
}

// FUNC 0x00261DE0 mpeg_mc_form_prediction
// Fetch the reference area for vector (dx, dy) (half-pel units) into the
// scratchpad, then predict Y (16x16) and Cb / Cr (8x8, vector / 2) into
// g_mc_pred, averaging with what is there when avg.
void mpeg_mc_form_prediction(u8 *ref, int x, int y, int dx, int dy, int avg)
{
    mpeg_fetch_ref_mbs(g_mc_ref_area, ref, x, y, dx, dy);
    mpeg_mc_luma(g_mc_pred, g_mc_ref_area, x, y, dx, dy, avg);
    dx >>= 1;
    dy >>= 1;
    x >>= 1;
    y >>= 1;
    mpeg_mc_chroma(g_mc_pred + 0x100, g_mc_ref_area + 0x400, x, y, dx, dy, avg);    /* Cb */
    mpeg_mc_chroma(g_mc_pred + 0x140, g_mc_ref_area + 0x500, x, y, dx, dy, avg);    /* Cr */
}

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
// FUNC 0x00261ED0 mpeg_mc_luma
// 16x16 luma prediction for the half-pel vector (dx, dy) at (x, y): src is
// the fetched area, offset by the integer part modulo 16; with avg the
// result is averaged with what dst holds.  Variables: lo / hi the low and
// high 8 pixels of a row as halfwords (the sums), tlo / thi the second
// operand, t and u0..u2 the rest of a 2x2 average.  The aligned copy does
// 4 passes of 4 rows.
void mpeg_mc_luma(u8 *dst, const u8 *src, int x, int y, int dx, int dy, int avg)
{
    int halfY = dy & 1;
    int halfX;
    int i;
    u128 hi = 0, thi = 0, tlo = 0, lo = 0, t = 0;
    u128 m1 = -1, zero = 0;
    u128 u0, u1, u2;

    x += dx >> 1;
    y += dy >> 1;
    src = src + ((y & 15) << 5) + (x & 15);
    halfX = dx & 1;
    if (!halfX) {
        if (!halfY) {
            if (avg) {
                for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                        MMI_PCPYLD(lo, hi, lo);
                    } else {
                        MMI_LQ(lo, src, 0);
                    }
                    MMI_LQ(hi, dst, 0);
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                    MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            } else if ((u32)src & 15) {
                for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                    MMI_PCPYLD(lo, hi, lo);
                    MMI_SQ(lo, dst, 0);
                }
            } else {
                for (i = 3; i >= 0; i--, src += 0x80, dst += 0x40) {
                    MMI_LQ(lo, src, 0); MMI_LQ(hi, src, 32); MMI_LQ(tlo, src, 64); MMI_LQ(thi, src, 96);
                    MMI_SQ(lo, dst, 0); MMI_SQ(hi, dst, 16); MMI_SQ(tlo, dst, 32); MMI_SQ(thi, dst, 48);
                }
            }
        } else {
            if (avg) {
                for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39); MMI_LDR(tlo, src, 40); MMI_LDL(tlo, src, 47);
                        MMI_PCPYLD(hi, tlo, hi);
                    } else {
                        MMI_LQ(lo, src, 0); MMI_LQ(hi, src, 32);
                    }
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PADDH(lo, lo, MMI_ONES); MMI_PADDH(hi, hi, MMI_ONES);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_LQ(tlo, dst, 0);
                    MMI_PEXTUB(thi, zero, tlo); MMI_PEXTLB(tlo, zero, tlo);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                    MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            } else {
                for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39); MMI_LDR(tlo, src, 40); MMI_LDL(tlo, src, 47);
                        MMI_PCPYLD(hi, tlo, hi);
                    } else {
                        MMI_LQ(lo, src, 0); MMI_LQ(hi, src, 32);
                    }
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PADDH(lo, lo, MMI_ONES); MMI_PADDH(hi, hi, MMI_ONES);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            }
        }
    } else if (!halfY) {
        if (avg) {
            for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                    MMI_PCPYLD(lo, hi, lo);
                } else {
                    MMI_LQ(lo, src, 0);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(tlo, src, 9); MMI_LDL(tlo, src, 16);
                MMI_PCPYLD(hi, tlo, hi);
                MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PADDH(lo, lo, MMI_ONES); MMI_PADDH(hi, hi, MMI_ONES);
                MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                MMI_LQ(tlo, dst, 0);
                MMI_PEXTUB(thi, zero, tlo); MMI_PEXTLB(tlo, zero, tlo);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                MMI_PPACB(tlo, hi, lo);
                MMI_SQ(tlo, dst, 0);
            }
        } else {
            for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                    MMI_PCPYLD(lo, hi, lo);
                } else {
                    MMI_LQ(lo, src, 0);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(tlo, src, 9); MMI_LDL(tlo, src, 16);
                MMI_PCPYLD(hi, tlo, hi);
                MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PADDH(lo, lo, MMI_ONES); MMI_PADDH(hi, hi, MMI_ONES);
                MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                MMI_PPACB(tlo, hi, lo);
                MMI_SQ(tlo, dst, 0);
            }
        }
    } else {
        if (avg) {
            for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                    MMI_PCPYLD(lo, hi, lo);
                    MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39); MMI_LDR(tlo, src, 40); MMI_LDL(tlo, src, 47);
                    MMI_PCPYLD(tlo, tlo, hi);
                } else {
                    MMI_LQ(lo, src, 0); MMI_LQ(tlo, src, 32);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(thi, src, 9); MMI_LDL(thi, src, 16);
                MMI_PCPYLD(hi, thi, hi);
                MMI_LDR(thi, src, 33); MMI_LDL(thi, src, 40); MMI_LDR(t, src, 41); MMI_LDL(t, src, 48);
                MMI_PCPYLD(thi, t, thi);
                MMI_PEXTLB(t, zero, lo); MMI_PEXTUB(u0, zero, lo);
                MMI_PEXTLB(u1, zero, hi); MMI_PEXTUB(u2, zero, hi);
                MMI_PEXTLB(lo, zero, tlo); MMI_PEXTUB(hi, zero, tlo);
                MMI_PEXTLB(tlo, zero, thi); MMI_PEXTUB(thi, zero, thi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(t, t, u1); MMI_PADDH(lo, lo, t);
                MMI_PADDH(hi, hi, thi); MMI_PADDH(u0, u0, u2); MMI_PADDH(hi, hi, u0);
                MMI_PADDH(lo, lo, MMI_TWOS); MMI_PADDH(hi, hi, MMI_TWOS);
                MMI_PSRLH(lo, lo, 2); MMI_PSRLH(hi, hi, 2);
                MMI_LQ(tlo, dst, 0);
                MMI_PEXTUB(thi, zero, tlo); MMI_PEXTLB(tlo, zero, tlo);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                MMI_PPACB(tlo, hi, lo);
                MMI_SQ(tlo, dst, 0);
            }
        } else {
            for (i = 15; i >= 0; i--, src += 0x20, dst += 0x10) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 8); MMI_LDL(hi, src, 15);
                    MMI_PCPYLD(lo, hi, lo);
                    MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39); MMI_LDR(tlo, src, 40); MMI_LDL(tlo, src, 47);
                    MMI_PCPYLD(tlo, tlo, hi);
                } else {
                    MMI_LQ(lo, src, 0); MMI_LQ(tlo, src, 32);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(thi, src, 9); MMI_LDL(thi, src, 16);
                MMI_PCPYLD(hi, thi, hi);
                MMI_LDR(thi, src, 33); MMI_LDL(thi, src, 40); MMI_LDR(t, src, 41); MMI_LDL(t, src, 48);
                MMI_PCPYLD(thi, t, thi);
                MMI_PEXTLB(t, zero, lo); MMI_PEXTUB(u0, zero, lo);
                MMI_PEXTLB(u1, zero, hi); MMI_PEXTUB(u2, zero, hi);
                MMI_PEXTLB(lo, zero, tlo); MMI_PEXTUB(hi, zero, tlo);
                MMI_PEXTLB(tlo, zero, thi); MMI_PEXTUB(thi, zero, thi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(t, t, u1); MMI_PADDH(lo, lo, t);
                MMI_PADDH(hi, hi, thi); MMI_PADDH(u0, u0, u2); MMI_PADDH(hi, hi, u0);
                MMI_PADDH(lo, lo, MMI_TWOS); MMI_PADDH(hi, hi, MMI_TWOS);
                MMI_PSRLH(lo, lo, 2); MMI_PSRLH(hi, hi, 2);
                MMI_PPACB(tlo, hi, lo);
                MMI_SQ(tlo, dst, 0);
            }
        }
    }
}

// FUNC 0x00262610 mpeg_mc_chroma
// 8x8 chroma prediction (the vector and position already halved by the
// caller), integer part modulo 8.  Without half-pel x: 4 passes of 2 rows
// (two 8-byte rows paired with pcpyld; lo / hi are then the upper and the
// lower row); with half-pel x: 8 passes of one row.  The last case (both
// half-pel, no average) branches to the nop at 0x00262ADC and falls into
// its loop at 0x00262AE0..0x00262B60 (inside u122's address range; a false
// start, ref/not_starts.txt): it is part of this function.
void mpeg_mc_chroma(u8 *dst, const u8 *src, int x, int y, int dx, int dy, int avg)
{
    int halfY = dy & 1;
    int halfX;
    int i;
    u128 lo = 0, thi = 0, tlo = 0, hi = 0;
    u128 m1 = -1, zero = 0;

    x += dx >> 1;
    y += dy >> 1;
    src = src + ((y & 7) << 4) + (x & 7);
    halfX = dx & 1;
    if (!halfX) {
        if (!halfY) {
            if (avg) {
                for (i = 3; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 16); MMI_LDL(hi, src, 23);
                        MMI_PCPYLD(lo, hi, lo);
                    } else {
                        MMI_LD(lo, src, 0); MMI_LD(hi, src, 16);
                        MMI_PCPYLD(lo, hi, lo);
                    }
                    MMI_LQ(hi, dst, 0);
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                    MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            } else {
                for (i = 3; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 16); MMI_LDL(hi, src, 23);
                        MMI_PCPYLD(lo, hi, lo);
                    } else {
                        MMI_LD(lo, src, 0); MMI_LD(hi, src, 16);
                        MMI_PCPYLD(lo, hi, lo);
                    }
                    MMI_SQ(lo, dst, 0);
                }
            }
        } else {
            u128 k = MMI_ONES;
            if (avg) {
                for (i = 3; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LDR(hi, src, 16); MMI_LDL(hi, src, 23);
                        MMI_PCPYLD(hi, hi, hi);
                    } else {
                        MMI_LD(lo, src, 0); MMI_LD(hi, src, 32);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LD(hi, src, 16);
                        MMI_PCPYLD(hi, hi, hi);
                    }
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PADDH(lo, lo, k); MMI_PADDH(hi, hi, k);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_LQ(thi, dst, 0);
                    MMI_PEXTLB(tlo, zero, thi); MMI_PEXTUB(thi, zero, thi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PCGTH(tlo, lo, m1); MMI_PCGTH(thi, hi, m1);
                    MMI_PSRLH(tlo, tlo, 15); MMI_PSRLH(thi, thi, 15);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            } else {
                for (i = 3; i >= 0; i--, src += 0x20, dst += 0x10) {
                    if ((u32)src & 15) {
                        MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(hi, src, 32); MMI_LDL(hi, src, 39);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LDR(hi, src, 16); MMI_LDL(hi, src, 23);
                        MMI_PCPYLD(hi, hi, hi);
                    } else {
                        MMI_LD(lo, src, 0); MMI_LD(hi, src, 32);
                        MMI_PCPYLD(lo, hi, lo);
                        MMI_LD(hi, src, 16);
                        MMI_PCPYLD(hi, hi, hi);
                    }
                    MMI_PEXTLB(tlo, zero, lo); MMI_PEXTUB(thi, zero, lo);
                    MMI_PEXTLB(lo, zero, hi); MMI_PEXTUB(hi, zero, hi);
                    MMI_PADDH(lo, lo, tlo); MMI_PADDH(hi, hi, thi);
                    MMI_PADDH(lo, lo, k); MMI_PADDH(hi, hi, k);
                    MMI_PSRLH(lo, lo, 1); MMI_PSRLH(hi, hi, 1);
                    MMI_PPACB(tlo, hi, lo);
                    MMI_SQ(tlo, dst, 0);
                }
            }
        }
    } else if (!halfY) {
        u128 k = MMI_ONES;
        if (avg) {
            for (i = 7; i >= 0; i--, src += 0x10, dst += 8) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7);
                } else {
                    MMI_LD(lo, src, 0);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8);
                MMI_PEXTLB(lo, zero, lo); MMI_PEXTLB(hi, zero, hi);
                MMI_PADDH(lo, lo, hi); MMI_PADDH(lo, lo, k);
                MMI_PSRLH(lo, lo, 1);
                MMI_LD(hi, dst, 0);
                MMI_PEXTLB(hi, zero, hi);
                MMI_PADDH(lo, lo, hi);
                MMI_PCGTH(tlo, lo, m1);
                MMI_PSRLH(tlo, tlo, 15);
                MMI_PADDH(lo, lo, tlo);
                MMI_PSRLH(lo, lo, 1);
                MMI_PPACB(lo, zero, lo);
                MMI_SD(lo, dst, 0);
            }
        } else {
            for (i = 7; i >= 0; i--, src += 0x10, dst += 8) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7);
                } else {
                    MMI_LD(lo, src, 0);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8);
                MMI_PEXTLB(lo, zero, lo); MMI_PEXTLB(hi, zero, hi);
                MMI_PADDH(lo, lo, hi); MMI_PADDH(lo, lo, k);
                MMI_PSRLH(lo, lo, 1);
                MMI_PPACB(lo, zero, lo);
                MMI_SD(lo, dst, 0);
            }
        }
    } else {
        u128 k = MMI_TWOS;
        if (avg) {
            for (i = 7; i >= 0; i--, src += 0x10, dst += 8) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(tlo, src, 16); MMI_LDL(tlo, src, 23);
                } else {
                    MMI_LD(lo, src, 0); MMI_LD(tlo, src, 16);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(thi, src, 17); MMI_LDL(thi, src, 24);
                MMI_PEXTLB(lo, zero, lo); MMI_PEXTLB(hi, zero, hi);
                MMI_PEXTLB(tlo, zero, tlo); MMI_PEXTLB(thi, zero, thi);
                MMI_PADDH(lo, lo, hi); MMI_PADDH(tlo, tlo, thi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(lo, lo, k);
                MMI_PSRLH(lo, lo, 2);
                MMI_LD(hi, dst, 0);
                MMI_PEXTLB(hi, zero, hi);
                MMI_PADDH(lo, lo, hi);
                MMI_PCGTH(tlo, lo, m1);
                MMI_PSRLH(tlo, tlo, 15);
                MMI_PADDH(lo, lo, tlo);
                MMI_PSRLH(lo, lo, 1);
                MMI_PPACB(lo, zero, lo);
                MMI_SD(lo, dst, 0);
            }
        } else {
            for (i = 7; i >= 0; i--, src += 0x10, dst += 8) {
                if ((u32)src & 15) {
                    MMI_LDR(lo, src, 0); MMI_LDL(lo, src, 7); MMI_LDR(tlo, src, 16); MMI_LDL(tlo, src, 23);
                } else {
                    MMI_LD(lo, src, 0); MMI_LD(tlo, src, 16);
                }
                MMI_LDR(hi, src, 1); MMI_LDL(hi, src, 8); MMI_LDR(thi, src, 17); MMI_LDL(thi, src, 24);
                MMI_PEXTLB(lo, zero, lo); MMI_PEXTLB(hi, zero, hi);
                MMI_PEXTLB(tlo, zero, tlo); MMI_PEXTLB(thi, zero, thi);
                MMI_PADDH(lo, lo, hi); MMI_PADDH(tlo, tlo, thi);
                MMI_PADDH(lo, lo, tlo); MMI_PADDH(lo, lo, k);
                MMI_PSRLH(lo, lo, 2);
                MMI_PPACB(lo, zero, lo);
                MMI_SD(lo, dst, 0);
            }
        }
    }
}
#else
/* mpeg_mc_luma (0x00261ED0) */
__asm__(MC_ASM_BEGIN(mpeg_mc_luma)
    "	andi	$13, $9, 1\n"
    "	sra	$2, $8, 1\n"
    "	sra	$9, $9, 1\n"
    "	addu	$6, $6, $2\n"
    "	addu	$7, $7, $9\n"
    "	andi	$6, $6, 15\n"
    "	andi	$7, $7, 15\n"
    "	andi	$8, $8, 1\n"
    "	sll	$7, $7, 5\n"
    "	por	$3, $0, $0\n"
    "	addu	$5, $5, $7\n"
    "	por	$9, $0, $0\n"
    "	addu	$5, $5, $6\n"
    "	por	$7, $0, $0\n"
    "	por	$6, $0, $0\n"
    "	por	$12, $0, $0\n"
    "	addiu	$14, $0, -1\n"
    "	pcpyld	$14, $14, $0\n"
    "	addiu	$14, $0, -1\n"
    "	bnez	$8, .Lmpeg_mc_luma_15\n"
    "	por	$11, $0, $0\n"
    "	bnez	$13, .Lmpeg_mc_luma_7\n"
    "	nop\n"
    "	beqz	$10, .Lmpeg_mc_luma_4\n"
    "	addiu	$13, $0, 15\n"
    "	nop\n"
    ".Lmpeg_mc_luma_1:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_2\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	b	.Lmpeg_mc_luma_3\n"
    "	nop\n"
    ".Lmpeg_mc_luma_2:\n"
    "	lq	$6, 0($5)\n"
    ".Lmpeg_mc_luma_3:\n"
    "	lq	$3, 0($4)\n"
    "	pextlb	$7, $11, $6\n"
    "	pextub	$9, $11, $6\n"
    "	pextlb	$6, $11, $3\n"
    "	pextub	$3, $11, $3\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	pcgth	$7, $6, $14\n"
    "	pcgth	$9, $3, $14\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_1\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_4:\n"
    "	andi	$2, $5, 15\n"
    "	beql	$2, $0, .Lmpeg_mc_luma_6\n"
    "	addiu	$13, $0, 3\n"
    "	nop\n"
    ".Lmpeg_mc_luma_5:\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	sq	$6, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_5\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_6:\n"
    "	lq	$6, 0($5)\n"
    "	lq	$3, 32($5)\n"
    "	lq	$7, 64($5)\n"
    "	lq	$9, 96($5)\n"
    "	sq	$6, 0($4)\n"
    "	sq	$3, 16($4)\n"
    "	sq	$7, 32($4)\n"
    "	sq	$9, 48($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 128\n"
    "	bgez	$13, .Lmpeg_mc_luma_6\n"
    "	addiu	$4, $4, 64\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_7:\n"
    "	beqz	$10, .Lmpeg_mc_luma_11\n"
    "	addiu	$13, $0, 15\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	pcpyld	$8, $8, $0\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	nop\n"
    ".Lmpeg_mc_luma_8:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_9\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	ldr	$3, 32($5)\n"
    "	ldl	$3, 39($5)\n"
    "	ldr	$7, 40($5)\n"
    "	ldl	$7, 47($5)\n"
    "	pcpyld	$3, $7, $3\n"
    "	b	.Lmpeg_mc_luma_10\n"
    "	nop\n"
    ".Lmpeg_mc_luma_9:\n"
    "	lq	$6, 0($5)\n"
    "	lq	$3, 32($5)\n"
    ".Lmpeg_mc_luma_10:\n"
    "	pextlb	$7, $11, $6\n"
    "	pextub	$9, $11, $6\n"
    "	pextlb	$6, $11, $3\n"
    "	pextub	$3, $11, $3\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$6, $6, $8\n"
    "	paddh	$3, $3, $8\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	lq	$7, 0($4)\n"
    "	pextub	$9, $11, $7\n"
    "	pextlb	$7, $11, $7\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	pcgth	$7, $6, $14\n"
    "	pcgth	$9, $3, $14\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_8\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_11:\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	pcpyld	$8, $8, $0\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	nop\n"
    ".Lmpeg_mc_luma_12:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_13\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	ldr	$3, 32($5)\n"
    "	ldl	$3, 39($5)\n"
    "	ldr	$7, 40($5)\n"
    "	ldl	$7, 47($5)\n"
    "	pcpyld	$3, $7, $3\n"
    "	b	.Lmpeg_mc_luma_14\n"
    "	nop\n"
    ".Lmpeg_mc_luma_13:\n"
    "	lq	$6, 0($5)\n"
    "	lq	$3, 32($5)\n"
    ".Lmpeg_mc_luma_14:\n"
    "	pextlb	$7, $11, $6\n"
    "	pextub	$9, $11, $6\n"
    "	pextlb	$6, $11, $3\n"
    "	pextub	$3, $11, $3\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$6, $6, $8\n"
    "	paddh	$3, $3, $8\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_12\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_15:\n"
    "	bnez	$13, .Lmpeg_mc_luma_23\n"
    "	nop\n"
    "	beqz	$10, .Lmpeg_mc_luma_19\n"
    "	addiu	$13, $0, 15\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	pcpyld	$8, $8, $0\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    ".Lmpeg_mc_luma_16:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_17\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	b	.Lmpeg_mc_luma_18\n"
    "	nop\n"
    ".Lmpeg_mc_luma_17:\n"
    "	lq	$6, 0($5)\n"
    ".Lmpeg_mc_luma_18:\n"
    "	ldr	$3, 1($5)\n"
    "	ldl	$3, 8($5)\n"
    "	ldr	$7, 9($5)\n"
    "	ldl	$7, 16($5)\n"
    "	pcpyld	$3, $7, $3\n"
    "	pextlb	$7, $11, $6\n"
    "	pextub	$9, $11, $6\n"
    "	pextlb	$6, $11, $3\n"
    "	pextub	$3, $11, $3\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$6, $6, $8\n"
    "	paddh	$3, $3, $8\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	lq	$7, 0($4)\n"
    "	pextub	$9, $11, $7\n"
    "	pextlb	$7, $11, $7\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	pcgth	$7, $6, $14\n"
    "	pcgth	$9, $3, $14\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_16\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_19:\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	pcpyld	$8, $8, $0\n"
    "	lui	$8, 1\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    "	dsll	$8, $8, 16\n"
    "	ori	$8, $8, 1\n"
    ".Lmpeg_mc_luma_20:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_21\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	b	.Lmpeg_mc_luma_22\n"
    "	nop\n"
    ".Lmpeg_mc_luma_21:\n"
    "	lq	$6, 0($5)\n"
    ".Lmpeg_mc_luma_22:\n"
    "	ldr	$3, 1($5)\n"
    "	ldl	$3, 8($5)\n"
    "	ldr	$7, 9($5)\n"
    "	ldl	$7, 16($5)\n"
    "	pcpyld	$3, $7, $3\n"
    "	pextlb	$7, $11, $6\n"
    "	pextub	$9, $11, $6\n"
    "	pextlb	$6, $11, $3\n"
    "	pextub	$3, $11, $3\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$6, $6, $8\n"
    "	paddh	$3, $3, $8\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_20\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_23:\n"
    "	beqz	$10, .Lmpeg_mc_luma_27\n"
    "	addiu	$13, $0, 15\n"
    "	lui	$15, 2\n"
    "	ori	$15, $15, 2\n"
    "	dsll	$15, $15, 16\n"
    "	ori	$15, $15, 2\n"
    "	dsll	$15, $15, 16\n"
    "	ori	$15, $15, 2\n"
    "	pcpyld	$15, $15, $0\n"
    "	lui	$15, 2\n"
    "	ori	$15, $15, 2\n"
    "	dsll	$15, $15, 16\n"
    "	ori	$15, $15, 2\n"
    "	dsll	$15, $15, 16\n"
    "	ori	$15, $15, 2\n"
    "	nop\n"
    ".Lmpeg_mc_luma_24:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_25\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	ldr	$3, 32($5)\n"
    "	ldl	$3, 39($5)\n"
    "	ldr	$7, 40($5)\n"
    "	ldl	$7, 47($5)\n"
    "	pcpyld	$7, $7, $3\n"
    "	b	.Lmpeg_mc_luma_26\n"
    "	nop\n"
    ".Lmpeg_mc_luma_25:\n"
    "	lq	$6, 0($5)\n"
    "	lq	$7, 32($5)\n"
    ".Lmpeg_mc_luma_26:\n"
    "	ldr	$3, 1($5)\n"
    "	ldl	$3, 8($5)\n"
    "	ldr	$9, 9($5)\n"
    "	ldl	$9, 16($5)\n"
    "	pcpyld	$3, $9, $3\n"
    "	ldr	$9, 33($5)\n"
    "	ldl	$9, 40($5)\n"
    "	ldr	$12, 41($5)\n"
    "	ldl	$12, 48($5)\n"
    "	pcpyld	$9, $12, $9\n"
    "	pextlb	$12, $11, $6\n"
    "	pextub	$2, $11, $6\n"
    "	pextlb	$8, $11, $3\n"
    "	pextub	$10, $11, $3\n"
    "	pextlb	$6, $11, $7\n"
    "	pextub	$3, $11, $7\n"
    "	pextlb	$7, $11, $9\n"
    "	pextub	$9, $11, $9\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$12, $12, $8\n"
    "	paddh	$6, $6, $12\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$2, $2, $10\n"
    "	paddh	$3, $3, $2\n"
    "	paddh	$6, $6, $15\n"
    "	paddh	$3, $3, $15\n"
    "	psrlh	$6, $6, 2\n"
    "	psrlh	$3, $3, 2\n"
    "	lq	$7, 0($4)\n"
    "	pextub	$9, $11, $7\n"
    "	pextlb	$7, $11, $7\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	pcgth	$7, $6, $14\n"
    "	pcgth	$9, $3, $14\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$6, $6, 1\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_24\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_luma_27:\n"
    "	lui	$14, 2\n"
    "	ori	$14, $14, 2\n"
    "	dsll	$14, $14, 16\n"
    "	ori	$14, $14, 2\n"
    "	dsll	$14, $14, 16\n"
    "	ori	$14, $14, 2\n"
    "	pcpyld	$14, $14, $0\n"
    "	lui	$14, 2\n"
    "	ori	$14, $14, 2\n"
    "	dsll	$14, $14, 16\n"
    "	ori	$14, $14, 2\n"
    "	dsll	$14, $14, 16\n"
    "	ori	$14, $14, 2\n"
    "	nop\n"
    ".Lmpeg_mc_luma_28:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_luma_29\n"
    "	nop\n"
    "	ldr	$6, 0($5)\n"
    "	ldl	$6, 7($5)\n"
    "	ldr	$3, 8($5)\n"
    "	ldl	$3, 15($5)\n"
    "	pcpyld	$6, $3, $6\n"
    "	ldr	$3, 32($5)\n"
    "	ldl	$3, 39($5)\n"
    "	ldr	$7, 40($5)\n"
    "	ldl	$7, 47($5)\n"
    "	pcpyld	$7, $7, $3\n"
    "	b	.Lmpeg_mc_luma_30\n"
    "	nop\n"
    ".Lmpeg_mc_luma_29:\n"
    "	lq	$6, 0($5)\n"
    "	lq	$7, 32($5)\n"
    ".Lmpeg_mc_luma_30:\n"
    "	ldr	$3, 1($5)\n"
    "	ldl	$3, 8($5)\n"
    "	ldr	$9, 9($5)\n"
    "	ldl	$9, 16($5)\n"
    "	pcpyld	$3, $9, $3\n"
    "	ldr	$9, 33($5)\n"
    "	ldl	$9, 40($5)\n"
    "	ldr	$12, 41($5)\n"
    "	ldl	$12, 48($5)\n"
    "	pcpyld	$9, $12, $9\n"
    "	pextlb	$12, $11, $6\n"
    "	pextub	$2, $11, $6\n"
    "	pextlb	$8, $11, $3\n"
    "	pextub	$10, $11, $3\n"
    "	pextlb	$6, $11, $7\n"
    "	pextub	$3, $11, $7\n"
    "	pextlb	$7, $11, $9\n"
    "	pextub	$9, $11, $9\n"
    "	paddh	$6, $6, $7\n"
    "	paddh	$12, $12, $8\n"
    "	paddh	$6, $6, $12\n"
    "	paddh	$3, $3, $9\n"
    "	paddh	$2, $2, $10\n"
    "	paddh	$3, $3, $2\n"
    "	paddh	$6, $6, $14\n"
    "	paddh	$3, $3, $14\n"
    "	psrlh	$6, $6, 2\n"
    "	psrlh	$3, $3, 2\n"
    "	ppacb	$7, $3, $6\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$13, $13, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$13, .Lmpeg_mc_luma_28\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    MC_ASM_END(mpeg_mc_luma));

/* mpeg_mc_chroma (0x00262610, with its last loop 0x00262AE0..0x00262B60) */
__asm__(MC_ASM_BEGIN(mpeg_mc_chroma)
    "	andi	$12, $9, 1\n"
    "	sra	$2, $8, 1\n"
    "	sra	$9, $9, 1\n"
    "	addu	$6, $6, $2\n"
    "	addu	$7, $7, $9\n"
    "	andi	$6, $6, 7\n"
    "	andi	$7, $7, 7\n"
    "	andi	$8, $8, 1\n"
    "	sll	$7, $7, 4\n"
    "	por	$3, $0, $0\n"
    "	addu	$5, $5, $7\n"
    "	por	$9, $0, $0\n"
    "	addu	$5, $5, $6\n"
    "	por	$7, $0, $0\n"
    "	por	$6, $0, $0\n"
    "	addiu	$13, $0, -1\n"
    "	pcpyld	$13, $13, $0\n"
    "	addiu	$13, $0, -1\n"
    "	bnez	$8, .Lmpeg_mc_chroma_15\n"
    "	por	$11, $0, $0\n"
    "	bnez	$12, .Lmpeg_mc_chroma_7\n"
    "	nop\n"
    "	beqz	$10, .Lmpeg_mc_chroma_4\n"
    "	addiu	$8, $0, 3\n"
    ".Lmpeg_mc_chroma_1:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_2\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$6, 16($5)\n"
    "	ldl	$6, 23($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	b	.Lmpeg_mc_chroma_3\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_2:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$6, 16($5)\n"
    "	pcpyld	$3, $6, $3\n"
    ".Lmpeg_mc_chroma_3:\n"
    "	lq	$6, 0($4)\n"
    "	pextlb	$7, $11, $3\n"
    "	pextub	$9, $11, $3\n"
    "	pextlb	$3, $11, $6\n"
    "	pextub	$6, $11, $6\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	pcgth	$7, $3, $13\n"
    "	pcgth	$9, $6, $13\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	psrlh	$3, $3, 1\n"
    "	psrlh	$6, $6, 1\n"
    "	ppacb	$7, $6, $3\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$8, .Lmpeg_mc_chroma_1\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_4:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_5\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$6, 16($5)\n"
    "	ldl	$6, 23($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	b	.Lmpeg_mc_chroma_6\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_5:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$6, 16($5)\n"
    "	pcpyld	$3, $6, $3\n"
    ".Lmpeg_mc_chroma_6:\n"
    "	sq	$3, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$8, .Lmpeg_mc_chroma_4\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_7:\n"
    "	lui	$12, 1\n"
    "	ori	$12, $12, 1\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 1\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 1\n"
    "	pcpyld	$12, $12, $0\n"
    "	lui	$12, 1\n"
    "	ori	$12, $12, 1\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 1\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 1\n"
    "	beqz	$10, .Lmpeg_mc_chroma_11\n"
    "	addiu	$8, $0, 3\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_8:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_9\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$6, 32($5)\n"
    "	ldl	$6, 39($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	ldr	$6, 16($5)\n"
    "	ldl	$6, 23($5)\n"
    "	pcpyld	$6, $6, $6\n"
    "	b	.Lmpeg_mc_chroma_10\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_9:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$6, 32($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	ld	$6, 16($5)\n"
    "	pcpyld	$6, $6, $6\n"
    ".Lmpeg_mc_chroma_10:\n"
    "	pextlb	$7, $11, $3\n"
    "	pextub	$9, $11, $3\n"
    "	pextlb	$3, $11, $6\n"
    "	pextub	$6, $11, $6\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	paddh	$3, $3, $12\n"
    "	paddh	$6, $6, $12\n"
    "	psrlh	$3, $3, 1\n"
    "	psrlh	$6, $6, 1\n"
    "	lq	$9, 0($4)\n"
    "	pextlb	$7, $11, $9\n"
    "	pextub	$9, $11, $9\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	pcgth	$7, $3, $13\n"
    "	pcgth	$9, $6, $13\n"
    "	psrlh	$7, $7, 15\n"
    "	psrlh	$9, $9, 15\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	psrlh	$3, $3, 1\n"
    "	psrlh	$6, $6, 1\n"
    "	ppacb	$7, $6, $3\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$8, .Lmpeg_mc_chroma_8\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_11:\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_12:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_13\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$6, 32($5)\n"
    "	ldl	$6, 39($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	ldr	$6, 16($5)\n"
    "	ldl	$6, 23($5)\n"
    "	pcpyld	$6, $6, $6\n"
    "	b	.Lmpeg_mc_chroma_14\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_13:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$6, 32($5)\n"
    "	pcpyld	$3, $6, $3\n"
    "	ld	$6, 16($5)\n"
    "	pcpyld	$6, $6, $6\n"
    ".Lmpeg_mc_chroma_14:\n"
    "	pextlb	$7, $11, $3\n"
    "	pextub	$9, $11, $3\n"
    "	pextlb	$3, $11, $6\n"
    "	pextub	$6, $11, $6\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$6, $6, $9\n"
    "	paddh	$3, $3, $12\n"
    "	paddh	$6, $6, $12\n"
    "	psrlh	$3, $3, 1\n"
    "	psrlh	$6, $6, 1\n"
    "	ppacb	$7, $6, $3\n"
    "	sq	$7, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 32\n"
    "	bgez	$8, .Lmpeg_mc_chroma_12\n"
    "	addiu	$4, $4, 16\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_15:\n"
    "	bnez	$12, .Lmpeg_mc_chroma_22\n"
    "	nop\n"
    "	lui	$9, 1\n"
    "	ori	$9, $9, 1\n"
    "	dsll	$9, $9, 16\n"
    "	ori	$9, $9, 1\n"
    "	dsll	$9, $9, 16\n"
    "	ori	$9, $9, 1\n"
    "	pcpyld	$9, $9, $0\n"
    "	lui	$9, 1\n"
    "	ori	$9, $9, 1\n"
    "	dsll	$9, $9, 16\n"
    "	ori	$9, $9, 1\n"
    "	dsll	$9, $9, 16\n"
    "	ori	$9, $9, 1\n"
    "	beqz	$10, .Lmpeg_mc_chroma_19\n"
    "	addiu	$8, $0, 7\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_16:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_17\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	b	.Lmpeg_mc_chroma_18\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_17:\n"
    "	ld	$3, 0($5)\n"
    ".Lmpeg_mc_chroma_18:\n"
    "	ldr	$6, 1($5)\n"
    "	ldl	$6, 8($5)\n"
    "	pextlb	$3, $11, $3\n"
    "	pextlb	$6, $11, $6\n"
    "	paddh	$3, $3, $6\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$3, $3, 1\n"
    "	ld	$6, 0($4)\n"
    "	pextlb	$6, $11, $6\n"
    "	paddh	$3, $3, $6\n"
    "	pcgth	$7, $3, $13\n"
    "	psrlh	$7, $7, 15\n"
    "	paddh	$3, $3, $7\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$3, $11, $3\n"
    "	sd	$3, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 16\n"
    "	bgez	$8, .Lmpeg_mc_chroma_16\n"
    "	addiu	$4, $4, 8\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_19:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_20\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	b	.Lmpeg_mc_chroma_21\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_20:\n"
    "	ld	$3, 0($5)\n"
    ".Lmpeg_mc_chroma_21:\n"
    "	ldr	$6, 1($5)\n"
    "	ldl	$6, 8($5)\n"
    "	pextlb	$3, $11, $3\n"
    "	pextlb	$6, $11, $6\n"
    "	paddh	$3, $3, $6\n"
    "	paddh	$3, $3, $9\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$3, $11, $3\n"
    "	sd	$3, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 16\n"
    "	bgez	$8, .Lmpeg_mc_chroma_19\n"
    "	addiu	$4, $4, 8\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_22:\n"
    "	lui	$12, 2\n"
    "	ori	$12, $12, 2\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 2\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 2\n"
    "	pcpyld	$12, $12, $0\n"
    "	lui	$12, 2\n"
    "	ori	$12, $12, 2\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 2\n"
    "	dsll	$12, $12, 16\n"
    "	ori	$12, $12, 2\n"
    "	beqz	$10, .Lmpeg_mc_chroma_26\n"
    "	addiu	$8, $0, 7\n"
    ".Lmpeg_mc_chroma_23:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_24\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$7, 16($5)\n"
    "	ldl	$7, 23($5)\n"
    "	b	.Lmpeg_mc_chroma_25\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_24:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$7, 16($5)\n"
    ".Lmpeg_mc_chroma_25:\n"
    "	ldr	$6, 1($5)\n"
    "	ldl	$6, 8($5)\n"
    "	ldr	$9, 17($5)\n"
    "	ldl	$9, 24($5)\n"
    "	pextlb	$3, $11, $3\n"
    "	pextlb	$6, $11, $6\n"
    "	pextlb	$7, $11, $7\n"
    "	pextlb	$9, $11, $9\n"
    "	paddh	$3, $3, $6\n"
    "	paddh	$7, $7, $9\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$3, $3, $12\n"
    "	psrlh	$3, $3, 2\n"
    "	ld	$6, 0($4)\n"
    "	pextlb	$6, $11, $6\n"
    "	paddh	$3, $3, $6\n"
    "	pcgth	$7, $3, $13\n"
    "	psrlh	$7, $7, 15\n"
    "	paddh	$3, $3, $7\n"
    "	psrlh	$3, $3, 1\n"
    "	ppacb	$3, $11, $3\n"
    "	sd	$3, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 16\n"
    "	bgez	$8, .Lmpeg_mc_chroma_23\n"
    "	addiu	$4, $4, 8\n"
    "	jr	$31\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_26:\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_27:\n"
    "	andi	$2, $5, 15\n"
    "	beqz	$2, .Lmpeg_mc_chroma_28\n"
    "	nop\n"
    "	ldr	$3, 0($5)\n"
    "	ldl	$3, 7($5)\n"
    "	ldr	$7, 16($5)\n"
    "	ldl	$7, 23($5)\n"
    "	b	.Lmpeg_mc_chroma_29\n"
    "	nop\n"
    ".Lmpeg_mc_chroma_28:\n"
    "	ld	$3, 0($5)\n"
    "	ld	$7, 16($5)\n"
    ".Lmpeg_mc_chroma_29:\n"
    "	ldr	$6, 1($5)\n"
    "	ldl	$6, 8($5)\n"
    "	ldr	$9, 17($5)\n"
    "	ldl	$9, 24($5)\n"
    "	pextlb	$3, $11, $3\n"
    "	pextlb	$6, $11, $6\n"
    "	pextlb	$7, $11, $7\n"
    "	pextlb	$9, $11, $9\n"
    "	paddh	$3, $3, $6\n"
    "	paddh	$7, $7, $9\n"
    "	paddh	$3, $3, $7\n"
    "	paddh	$3, $3, $12\n"
    "	psrlh	$3, $3, 2\n"
    "	ppacb	$3, $11, $3\n"
    "	sd	$3, 0($4)\n"
    "	addiu	$8, $8, -1\n"
    "	addiu	$5, $5, 16\n"
    "	bgez	$8, .Lmpeg_mc_chroma_27\n"
    "	addiu	$4, $4, 8\n"
    "	jr	$31\n"
    "	nop\n"
    MC_ASM_END(mpeg_mc_chroma));
#endif

/* The macroblock moves of the motion compensation (0x00262B60..0x00263368):
 * macroblock get / put on the frame store and the 2x2-macroblock reference
 * window fetch.  Frame store: MpegMB (0x180 bytes) column-major, the
 * macroblock holding pixel (x, y) is number (x >> 4) * g_mb_height +
 * (y >> 4).
 *
 * Under the matching compiler the quadword / doubleword moves are
 * one-instruction asm statements on a u128 variable, as in the MMI routines
 * above: retail keeps every lq / sq (ld / sd) pair in source order around
 * compiled address arithmetic and loops, with the macroblock address
 * computed before the first load and the delay slot of mpeg_get_mb's final
 * jr empty, which compiled u128 copies do not give (sched1 hoists the loads
 * over the mult, reorg moves the last sq into the slot).  The stores clobber
 * "memory" (MMI_SQ_MEM / MMI_SD_MEM): mpeg_fetch_ref_mbs reloads g_mb_height
 * after every copy loop.  GCC 14 (and NON_MATCHING) copies in C. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* One quadword / doubleword from s to d through q (a u128 of the caller). */
#define COPY_Q(d, s) { MMI_LQ(q, s, 0); MMI_SQ_MEM(q, d, 0); }
#define COPY_D(d, s) { MMI_LD(q, s, 0); MMI_SD_MEM(q, d, 0); }
#else
#define COPY_Q(d, s) (*(u128 *)(d) = *(const u128 *)(s))
#define COPY_D(d, s) (*(u64 *)(d) = *(const u64 *)(s))
#endif

/* Macroblock column mbx, row mby of a frame store. */
static __inline__ MpegMB *frame_mb(u8 *frame, int mbx, int mby)
{
    return (MpegMB *)frame + (mbx * g_mb_height + mby);
}

/* 24 quadword copies: 4 lq, then 4 sq, six times.  The four quadwords are
 * the same variables in every group (retail: $v0, $v1, $a1, $a3
 * throughout). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define MB_COPY4(d, s, o0, o1, o2, o3)                                          \
    MMI_LQ(q0, s, o0); MMI_LQ(q1, s, o1); MMI_LQ(q2, s, o2); MMI_LQ(q3, s, o3); \
    MMI_SQ_MEM(q0, d, o0); MMI_SQ_MEM(q1, d, o1);                               \
    MMI_SQ_MEM(q2, d, o2); MMI_SQ_MEM(q3, d, o3)

static __inline__ void mb_copy(MpegMB *dst, const MpegMB *src)
{
    u128 q0, q1, q2, q3;

    MB_COPY4(dst, src, 0x000, 0x010, 0x020, 0x030);
    MB_COPY4(dst, src, 0x040, 0x050, 0x060, 0x070);
    MB_COPY4(dst, src, 0x080, 0x090, 0x0A0, 0x0B0);
    MB_COPY4(dst, src, 0x0C0, 0x0D0, 0x0E0, 0x0F0);
    MB_COPY4(dst, src, 0x100, 0x110, 0x120, 0x130);
    MB_COPY4(dst, src, 0x140, 0x150, 0x160, 0x170);
}
#else
static __inline__ void mb_copy(MpegMB *dst, const MpegMB *src)
{
    int i;

    for (i = 0; i < MB_QWORDS; i++)
        ((u128 *)dst)[i] = ((const u128 *)src)[i];
}
#endif

// FUNC 0x00262B60 mpeg_get_mb
/* Copy the macroblock holding pixel (x, y) out of the frame store.  Caller
 * mpeg_video_motion_compensate. */
void mpeg_get_mb(MpegMB *dst, u8 *frame, int x, int y)
{
    mb_copy(dst, frame_mb(frame, x >> 4, y >> 4));
}

/* The macroblock at column mx, row my, as a macro: the lower row is
 * REF_MB(mbx, mby + 1), which egcs folds to mbx * h + 1 + mby as retail
 * computes it (an inline function would CSE mby + 1 instead). */
#define REF_MB(mx, my) (frame + ((mx) * g_mb_height + (my)) * 0x180)

/* Rows i = lo .. while cond of the macroblock mb into the window: luma row i
 * to window row i + 16 * qy, column 16 * qx (quadword index i * 2 + qofs);
 * chroma row i to window row i + 8 * qy, column 8 * qx (doubleword index
 * i * 2 + dofs).  The chroma row pointers are block-scope variables of the
 * loop body (retail: four separate pointers, hoisted per loop). */
#define REF_LUMA(lo, cond, qofs)                                    \
    for (i = (lo); cond; i++)                                       \
        COPY_Q(&((u128 *)dst)[i * 2 + (qofs)], &((u128 *)mb)[i])
#define REF_CHROMA(lo, cond, dofs)                                  \
    for (i = (lo); cond; i++) {                                     \
        u64 *scb = (u64 *)(mb + 0x100), *scr = (u64 *)(mb + 0x140); \
        u64 *dcb = (u64 *)(dst + 0x400), *dcr = (u64 *)(dst + 0x500); \
                                                                    \
        COPY_D(&dcb[i * 2 + (dofs)], &scb[i]);                      \
        COPY_D(&dcr[i * 2 + (dofs)], &scr[i]);                      \
    }

// FUNC 0x00262C48 mpeg_fetch_ref_mbs
/* Gather the reference area for a motion vector (dx, dy) (half-pel units) of
 * the macroblock at pixel (x, y) into the 2x2-macroblock window at dst (an
 * MpegRefWindow: luma rows of 0x20 bytes, Cb / Cr rows of 0x10 bytes at
 * +0x400 / +0x500).  Only the rows the predictor reads are copied: with a
 * vertical vector, luma rows ry..15 of the upper macroblocks and 0..ry of
 * the lower ones (17 rows), chroma cy..7 and 0..cy (9 rows); the right-hand
 * column is fetched only when dx != 0.  (x and y are multiples of 16, so
 * with dy == 0 the rows start at 0.)  Caller mpeg_mc_form_prediction.
 * Matching (99.7%): only the two half-vector temporaries of the first
 * block differ, dx >> 1 in $a5 and dy >> 1 in $v1 where retail has the
 * reverse (local-alloc priority: dy >> 1 lives shorter after sched1).  Tried:
 * every dependency-respecting order of the six head statements, mbx / mby /
 * ry computed in place or from a sum, hx / hy variables in 20 orders,
 * declaration orders; the reference compiler 991111-01 gives the same. */
void mpeg_fetch_ref_mbs(u8 *dst, u8 *frame, int x, int y, int dx, int dy)
{
    int cy, mbx, mby, ry;
    u8 *mb;
    int i;
    u128 q;

    cy = (y + (dy >> 2)) & 7;       /* first chroma row inside the upper macroblocks */
    x += dx >> 1;
    y += dy >> 1;
    mbx = x >> 4;                   /* macroblock column of the target */
    ry = y & 0xF;                   /* first luma row inside the upper macroblocks */
    mby = y >> 4;                   /* macroblock row of the target */

    if (dx == 0) {
        if (dy == 0) {
            mb = REF_MB(mbx, mby);
            REF_LUMA(0, i < 16, 0);
            REF_CHROMA(0, i < 8, 0);
        } else {
            mb = REF_MB(mbx, mby);                  /* upper */
            REF_LUMA(ry, i < 16, 0);
            REF_CHROMA(cy, i < 8, 0);
            mb = REF_MB(mbx, mby + 1);              /* lower */
            REF_LUMA(0, i <= ry, 32);
            REF_CHROMA(0, i <= cy, 16);
        }
    } else if (dy == 0) {
        mb = REF_MB(mbx, mby);                      /* left */
        REF_LUMA(0, i < 16, 0);
        REF_CHROMA(0, i < 8, 0);
        mb = REF_MB(mbx + 1, mby);                  /* right */
        REF_LUMA(0, i < 16, 1);
        REF_CHROMA(0, i < 8, 1);
    } else {
        mb = REF_MB(mbx, mby);                      /* upper left */
        REF_LUMA(ry, i < 16, 0);
        REF_CHROMA(cy, i < 8, 0);
        mb = REF_MB(mbx, mby + 1);                  /* lower left */
        REF_LUMA(0, i <= ry, 32);
        REF_CHROMA(0, i <= cy, 16);
        mb = REF_MB(mbx + 1, mby);                  /* upper right */
        REF_LUMA(ry, i < 16, 1);
        REF_CHROMA(cy, i < 8, 1);
        mb = REF_MB(mbx + 1, mby + 1);              /* lower right */
        REF_LUMA(0, i <= ry, 33);
        REF_CHROMA(0, i <= cy, 17);
    }
}

// FUNC 0x00263280 mpeg_put_mb
/* Store a predicted macroblock into the frame store at pixel (x, y) (the
 * reverse of mpeg_get_mb).  Reached by a tail jump only
 * (mpeg_video_motion_compensate). */
void mpeg_put_mb(const MpegMB *src, u8 *frame, int x, int y)
{
    mb_copy(frame_mb(frame, x >> 4, y >> 4), src);
}
