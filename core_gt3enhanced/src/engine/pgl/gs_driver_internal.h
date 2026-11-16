/* gs_driver_internal.h - unit u118 of the GT3 main executable, 0x00257EF4..0x0025B0C8.
 *
 * The last stretch of the no-FDE graphics library (G001, plain C: the
 * Polyphony GS / VIF1 / VU0 driver whose bootstrap copy is decompiled in
 * decomp/bootstrap/src/gs_driver.cpp + gs_state.cpp).  Files:
 *   vu0_current_matrix.c    the VU0-resident current matrix (vf28..vf31):
 *                             scale, rotate x/y/z, transpose, rigid inverse,
 *                             vector transform; GsContext.scale / normalMtx
 *   gs_context.c    small getters / setters of the scratchpad
 *                             drawing context (draw buffer, draw area, ...)
 *   color_filter.c  frame-buffer filters: the colour correction
 *                             (tone-curve CLUT + saturation matrix) and the
 *                             R / G / B plane split, VU1 packets built with
 *                             a small GIF-packet builder
 *   gs_driver.c     background colour, LCG, dirty-register flush,
 *                             the INTC / DMAC handlers, readback, the VU1
 *                             program selection, image-readback packet,
 *                             DIRECT alloc
 *   gs_lights.c     material / light setters (GL-like), and the
 *                             4x4 matrix copy at 0x0025B03C
 *
 * The shared G001 types (GsContext, GsLight, GsMaterial, GfxDriverState,
 * GsReadbackRec ...), the library's prototypes and the values its functions
 * take (PGL_CULL_*, PGL_FIELD_*, PGL_SCANMASK_*, PGL_LIGHT_*, PGL_MARK_* ...)
 * come from the canonical engine/pgl/G001_flare.h.  This header keeps what only
 * this unit uses: hardware register / packet constants, small helpers, the
 * colour filters' packet builder, the unit's .data / .sdata objects and the
 * prototypes of the functions no other unit calls.
 */
#ifndef U118_H
#define U118_H

#include "types.h"
#include "eekernel.h"           /* iWakeupThread, ExitHandler */
#include "libgraph.h"           /* SCE_GS_* register numbers / PSMs, GS_CSR, GS_BUSDIR */
#include "engine/pgl/G001_flare.h"

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The bits of a float (mfc1) and back (mtc1). */
static inline u32 u118_f2u(f32 f) { union { f32 f; u32 u; } c; c.f = f; return c.u; }
static inline f32 u118_u2f(u32 u) { union { f32 f; u32 u; } c; c.u = u; return c.f; }

/* EE COP1 max.s / min.s (the EE FPU has no NaNs).  Under -ffast-math egcs
 * folds `a >= b ? a : b' to MAX_EXPR(a, b), operands in source order (the
 * `>' form swaps them): max.s / min.s show the order of the call's
 * arguments. */
static inline f32 ee_maxf(f32 a, f32 b) { return a >= b ? a : b; }
static inline f32 ee_minf(f32 a, f32 b) { return a <= b ? a : b; }

/* The retail `not` (bootstrap gs_driver.cpp): egcs's one_cmpl is
 * `nor rd, $0, rs' and folds a constant, the binary has `not' (asm). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline u32 ee_not32(u32 a)
{
    u32 r;

    __asm__("not %0, %1" : "=r"(r) : "r"(a));
    return r;
}
#else
static inline u32 ee_not32(u32 a) { return ~a; }
#endif

/* An unaligned doubleword (ldl/ldr, sdl/sdr). */
typedef struct __attribute__((packed)) U118U64U { u64 v; } U118U64U;

/* Aliasing-safe views of packet words / shadow sub-fields. */
typedef u64 u64a __attribute__((may_alias));
typedef u32 u32a __attribute__((may_alias));
typedef u16 u16a __attribute__((may_alias));

/* Float constants with the retail bits.  GCC 14 converts decimal float
 * literals for the R5900 rounding toward zero (one ulp low), egcs to nearest
 * (and egcs takes no hex float with an `f' in it), so they have 9
 * significant digits rounded UP: a decimal in [f, f + ulp/2) gives f with
 * both.  The hex spelling is in the comment. */
#define U118_F_LUMA_R 0.300000012f      /* 0x3E99999A 0x1.333334p-2f (gp-0x6F54) luma weight of red   */
#define U118_F_LUMA_G 0.589999974f      /* 0x3F170A3D 0x1.2E147Ap-1f (gp-0x6F50) luma weight of green */
#define U118_F_LUMA_B 0.11f             /* 0x3DE147AE 0x1.C28F5Cp-4f (gp-0x6F4C) luma weight of blue  */
#define U118_F_1_255  0.00392156886f    /* 0x3B808081 0x1.010102p-8f 1/255 (gp-0x6F48, VU0 literal) */

/* R5900 "sync.l". */
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")

/* The scratchpad context pointer (bootstrap gs_internal.h): loaded whole
 * into a register by every function (egcs: a one-instruction asm). */
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif

/* ======================================================================== */
/* Hardware                                                                 */
/* ======================================================================== */

#define VIF1_STAT    REG32(0x10003C00)
#define VIF1_FBRST   REG32(0x10003C10)
#define VIF1_CODE    REG32(0x10003C80)      /* the last VIF code processed */
#define GS_REG_CSR      REG64(GS_CSR)
#define GS_REG_BUSDIR   REG64(GS_BUSDIR)

#define VIF1_STAT_INT       11              /* bit: stalled on a VIF code with the i-bit */
#define VIF1_STAT_FDR       0x00800000u     /* FIFO direction: VIF1 -> memory (a readback) */
#define VIF1_FBRST_STC      8               /* cancel the stall: resume */
#define GS_CSR_FIELD        13              /* bit: the field being displayed (1 odd) */
#define GS_CSR_FINISH       2               /* FINISH event (write 1: clear) */
#define GS_BUSDIR_TO_HOST   1               /* local -> host (image download) */
#define GS_BUSDIR_TO_GS     0

/* EE DMAC channel registers, through G001_flare.h's EeDmaChanRegs. */
#define D1 ((EeDmaChanRegs *)0x10009000)   /* VIF1 */
#define D2 ((EeDmaChanRegs *)0x1000A000)   /* GIF  */

/* Dn_CHCR fields. */
#define D_CHCR_DIR_FROM_MEM 0x001
#define D_CHCR_MOD_GET(c)   (((c) >> 2) & 3)    /* 0 normal, 1 chain, 2 interleave */
#define D_CHCR_MOD_CHAIN    0x004
#define D_CHCR_ASP_GET(c)   (((c) >> 4) & 3)    /* call stack depth */
#define D_CHCR_STR          0x100           /* start / busy */
#define D_CHCR_TAG_ID(c)    (((c) >> 28) & 7)   /* ID of the last tag read */
#define D_CHCR_TAG_IRQ      0x80000000u     /* IRQ bit of the last tag read */
#define D_CHCR_TAG_CODE(c)  ((c) & 0x00070000u) /* PGL_MARK_* bits of the last tag (gfx_put_irq_marker) */
/* DMA source-chain tag IDs as CHCR.TAG shows them. */
enum { DMATAG_ID_RET = 6, DMATAG_ID_END = 7 };
/* A source chain started: DIR from memory, MOD chain, STR; the high half
 * writes 0x1000 into the read-only TAG field (as in the binary). */
#define D_CHCR_START_CHAIN  0x10000105u
#define D_CHCR_KEEP_TIE     0xFEC0u         /* kept by dma_start_source_chain: TTE, TIE (ASP cleared) */
#define D_CHCR_KEEP_ASP_TIE 0xFEF0u         /* kept by gs_readback_end: ASP, TTE, TIE */
#define D_CHCR_TO_NORMAL    0xFFFFFEF0u     /* gs_readback_begin: DIR to memory, MOD normal, STR off */

/* gfx_dma_* channels (GfxDriverState.chan). */
enum { GFX_CH_VIF0 = 0, GFX_CH_VIF1 = 1, GFX_CH_GIF = 2 };

/* DMA source address of a buffer: physical address, plus the SPR bit for a
 * scratchpad (0x7xxxxxxx) pointer. */
static inline u32 gs_dma_src_addr(u32 a)
{
    u32 seg = a & 0x70000000;

    a &= 0x0FFFFFFF;
    if (seg == 0x70000000)
        a |= 0x80000000;
    return a;
}

/* Statement macros of the bootstrap driver (see gs_driver.cpp there). */
#define GS_REG_GET(var, reg)  do { (var) = (reg); } while (0)
#define GS_REG_PUT(reg, val)  do { (reg) = (val); } while (0)
#define GS_VIF1_RESUME()      GS_REG_PUT(VIF1_FBRST, VIF1_FBRST_STC)
#define GS_WAKE_WAITERS(head)  do {                                     \
        GfxWaitNode *w_;                                                \
        for (w_ = (head); w_ != NULL; w_ = w_->next)                    \
            iWakeupThread(w_->tid);                                     \
    } while (0)
#define GS_GIF_RESTART()  do {                                          \
        u32 c_;                                                         \
        GS_REG_GET(c_, D2->chcr);                                       \
        GS_REG_PUT(D2->qwc, 0);                                         \
        GS_REG_PUT(D2->chcr, c_ | D_CHCR_STR);                          \
        SYNC_L();                                                       \
    } while (0)

/* ======================================================================== */
/* Packets                                                                  */
/* ======================================================================== */

/* DMA source-chain tag (word 0: ID << 28 | QWC). */

/* VIF1 codes (CMD << 24 | NUM << 16 | IMMEDIATE). */
#define VIF_MSKPATH3_ON     0x06008000u     /* MSKPATH3: PATH3 masked */
#define VIF_MSKPATH3_OFF    0x06000000u     /* MSKPATH3: PATH3 unmasked */
#define VIF_MSCALF(a)       (0x15000000u | (a))     /* start the VU1 program at a (after the GIF is idle) */
/* UNPACK V4-32 of num qwords to VU1 address 0, relative to TOPS (FLG), USN. */
#define VIF_UNPACK_V4_32(num) (0x6C00C000u | ((u32)(num) << 16))

/* GIF tag fields (G001_flare.h) and PRIM in the tag (bits 47..57). */
#define GIF_PRIM(p)         ((u64)(p) << 47)
/* Register lists of the filters (REGLIST, 2 registers per qword). */
#define GIF_REGS_UV_SPRITE  (GIF_REG(0, SCE_GS_UV) | GIF_REG(1, SCE_GS_XYZ3) | \
                             GIF_REG(2, SCE_GS_UV) | GIF_REG(3, SCE_GS_XYZ2))   /* 0x53D3 */

/* GS PRIM (and GsContext.primAttr bits). */
enum {
    GS_PRIM_SPRITE = 0x006,
    GS_PRIM_TME    = 0x010,
    GS_PRIM_ABE    = 0x040,
    GS_PRIM_FST    = 0x100,             /* UV (texel) coordinates */
    GS_PRIM_CTXT2  = 0x200              /* GS context 2 */
};

/* GsContext.dirty bits: GS_DIRTY_* (G001_flare.h). */

/* The GIF packet builder of the colour filters (a stack local, 0x14 bytes):
 * blocks of one GIF tag and its data, the tag's NLOOP kept up to date by
 * pkt_ad. */
typedef struct PfxPacket {
    u64 *base;      /* +0x00 first qword                                     */
    u64 *tag;       /* +0x04 GIF tag of the current block                    */
    s32  n;         /* +0x08 qwords in the current block, its tag included   */
    s32  start;     /* +0x0C qword position of base in a chained packet (? set
                     *       only by pkt_begin_after; the filters add it to the
                     *       VU1 header)                                     */
    s32  total;     /* +0x10 qwords written                                  */
} PfxPacket;

/* The VU1 header in front of the filters' GIF data: 6 qwords unpacked
 * with them (VIF_UNPACK_V4_32(6 + data)); words 1 / 2 the data start and
 * size, the others parameters of the micro program (named by position). */
#define PFX_HDR_QWC         6

/* VU1 programs of the primitives (GsContext.vu1Prog, gs_select_vu1_prog):
 * a base and the culling variants. */
#define VU1_PROG_BASE       0x320
#define VU1_PROG_BASE_33E   0x33E           /* with PGL_CAP_VU1_PROG_33E (GsContext.vu1ProgAlt) */
#define VU1_PROG_CULL_CW    0xA             /* + : culls the clockwise faces (BACK + CCW, FRONT + CW) */
#define VU1_PROG_CULL_CCW   0x14            /* + : culls the counter-clockwise faces (FRONT + CCW, BACK + CW) */

/* ======================================================================== */
/* Globals of the unit (defined in gs_driver.c and               */
/* color_filter.c, at their original places)                      */
/* ======================================================================== */

extern u8  g_gsRegsGeneral[GS_GEN_COUNT]; /* @0x002FC220 .data 46 44 45 3D 49 1A 22 3B 1C 3F */
extern u8  g_gsRegsCtx1[GS_REG_COUNT];  /* @0x002FC230 .data FRAME_1 .. MIPTBP2_1 numbers */
extern u8  g_gsRegsCtx2[GS_REG_COUNT];  /* @0x002FC240 .data FRAME_2 .. MIPTBP2_2 numbers */
extern s8  g_003530E0;                  /* 0x003530E0 gp-0x4910 .sdata, the VU1 code is unscrambled (init 0) */
extern u32 g_003530E4;                  /* 0x003530E4 gp-0x490C .sdata, LCG state (init 0x32277070) */
extern const u8 g_00345590[0x100];      /* 0x00345590 .rodata 8x32 PSMT8 index texture: the
                                         * bytes 0..255 in the GS PSMT8 column order (00..07
                                         * 10..17, 0C..0F 08..0B 1C..1F 18..1B, 20.. ...) */

/* ======================================================================== */
/* Functions of the unit that no other unit calls (address order)          */
/* ======================================================================== */

/* color_filter.c */
void pfx_build_color_packet(u32 texAddr, u32 fbAddr, u32 fbw, u32 workAddr,
                            u32 clutAddr, u32 idxAddr, u64 scissor,
                            u32 colR, u32 colG, u32 colB);      /* 0x002584A0 */
void pfx_build_curve_clut(u32 clutAddr, f32 r, f32 g, f32 b,
                          f32 lo, f32 hi);                      /* 0x00258DB0 */
void pfx_upload_index_texture(u32 vramAddr);                    /* 0x00259008 */
void pkt_begin(PfxPacket *pk, u64 *p);                          /* 0x00259030 */
u64 *pkt_begin_after(PfxPacket *pk, PfxPacket *prev);           /* 0x00259048 */
void pkt_tag_ad(PfxPacket *pk);                                 /* 0x002590A0 */
void pkt_tag_prim(PfxPacket *pk, u64 prim);                     /* 0x002590E0 */
void pkt_tag_reglist(PfxPacket *pk, s32 nreg, u64 regs, s32 nloop); /* 0x00259128 */
void pkt_set_eop(PfxPacket *pk);                                /* 0x00259170 */
int  pkt_ad(PfxPacket *pk, u64 reg, u64 data);                  /* 0x00259188 */
int  pkt_qword(PfxPacket *pk, u64 lo, u64 hi);                  /* 0x002591E8 */
int  pkt_position(PfxPacket *pk);                               /* 0x00259218 */
u64 *pkt_end(PfxPacket *pk);                                    /* 0x00259228 */
void pfx_build_split_packet(u32 texAddr, u32 fbAddrR, u32 fbAddrG, u32 fbAddrB,
                            u32 workAddr, u32 clutAddr, u32 idxAddr, u64 scissor,
                            u32 fbw);                           /* 0x00259448 */
void pfx_write_split_setup(u32 fbPage, u32 unused, u64 scissor); /* 0x00259E90 (no callers) */

/* gs_driver.c */
int  lcg_rand(void);                                            /* 0x0025A1B8 */
u64 *gs_emit_masked_regs(u64 *out, u64 mask, const u8 *regNums,
                         const u64 *shadow);                    /* 0x0025A258 */
void gs_readback_begin(GfxDriverState *g);                      /* 0x0025A830 */
void gs_readback_end(GfxDriverState *g);                        /* 0x0025A8A8 */
u32 *gfx_alloc_direct(s32 qwc);                                 /* 0x0025AB80 */

/* gs_lights.c */
void gs_store_material_vec(GsContext *ctx, int face, u32 kind, GsVec4Arg v); /* 0x0025ABD0 */
void mat44_copy(void *dst, const void *src);                    /* 0x0025AFE8 (entry 0x0025B03C) */

#endif /* U118_H */
