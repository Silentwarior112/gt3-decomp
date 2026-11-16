/* gs_display_driver_internal.h - unit u116 of the GT3 main executable, 0x00250A58..0x00254828.
 *
 * The middle of the no-FDE graphics library (G001, plain C): Polyphony's
 * GS / VIF1 / GIF display driver, the same driver as the bootstrap's
 * src/gs_driver.cpp (decomp/bootstrap; 32 of the 82 functions are the same
 * machine code), here in a later version with more GL-style state setters:
 *   gs_display_driver.c   display mode / DMA channels / vsync, the frame
 *                           packet writer (lists, sub-packets, IRQ markers),
 *                           enable/disable switches, depth / alpha / blend /
 *                           fog / texture register setters, clears and the
 *                           frame-buffer copy effects (motion blur, fades).
 *
 * The shared G001 types (GsContext, GfxDriverState, EeDmaChanRegs ...), the
 * library's prototypes and the values its functions take (PGL_CAP_*,
 * PGL_TEXPARAM_*, PGL_FX_* ...) come from the canonical engine/pgl/G001_flare.h.
 * This header keeps what only this unit uses: the two packet headers, the
 * SCISSOR view, register field / packet / hardware constants, the in-unit
 * helpers and the prototypes of the functions no other unit calls.
 */
#ifndef U116_H
#define U116_H

#include "types.h"
#include "eekernel.h"           /* semaphores, threads, AddDmacHandler, FlushCache, DIntr */
#include "libgraph.h"           /* sceGsResetGraph, GS_PMODE.., SCE_GS_* register addresses */
#include "engine/pgl/G001_flare.h"

/* The scratchpad context pointer (bootstrap gs_internal.h, pgl_font_internal.h, gs_driver_internal.h):
 * retail loads 0x70003000 whole into a register (lui + ori) at every use and
 * never folds it into the field offsets; a wrapper that only passes it on
 * does not tail-call.  Under egcs a one-instruction asm in a statement
 * expression, GCC 14 gets the plain constant.  Same pointer value. */
#ifndef GS_SPR_CTX
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif
#endif

/* ======================================================================== */
/* Packet headers (forward-declared by G001_flare.h)                        */
/* ======================================================================== */

/* The header at the start of a frame packet (gfx_begin_packet returns it,
 * gfx_kick_frame_lists starts its two chains). */
typedef struct GsFrameHeader {
    u32 *vif1Chain;         /* 0x00 list 0 start (VIF1 source chain) */
    u32 *gifChain;          /* 0x04 list 1 start (GIF source chain) */
    u32 *save0;             /* 0x08 tail0 saved by gfx_begin_subpacket */
    u32 *save1;             /* 0x0C tail1 saved by gfx_begin_subpacket */
} GsFrameHeader;

/* A sub-packet: two DMA "call" targets (ended by ret tags) built by
 * gfx_begin_subpacket / gfx_close_subpacket and replayed by
 * gfx_call_subpacket.  Lives in the packet buffer. */
typedef struct GsSubPacketHeader {
    u32 *vif1Chain;         /* 0x00 list 0 entry */
    u32 *gifChain;          /* 0x04 list 1 entry */
    u64  written;           /* 0x08 registers the sub-packet emitted */
} GsSubPacketHeader;

/* The GS SCISSOR register as libgraph declares it (sceGsScissor): a view of
 * GsContext.reg[GS_REG_SCISSOR]. */
typedef struct __attribute__((may_alias)) GsScissor {
    u64 SCAX0 : 11; u64 pad11 : 5;
    u64 SCAX1 : 11; u64 pad27 : 5;
    u64 SCAY0 : 11; u64 pad43 : 5;
    u64 SCAY1 : 11; u64 pad59 : 5;
} GsScissor;

/* The GS TEST and ZBUF registers as libgraph declares them (sceGsTest,
 * sceGsZbuf): views of GsContext.reg[GS_REG_TEST] / [GS_REG_ZBUF]. */
typedef struct __attribute__((may_alias)) GsTest {
    u64 ATE : 1;  u64 ATST : 3; u64 AREF : 8; u64 AFAIL : 2;
    u64 DATE : 1; u64 DATM : 1; u64 ZTE : 1;  u64 ZTST : 2;
    u64 pad19 : 45;
} GsTest;

typedef struct __attribute__((may_alias)) GsZbuf {
    u64 ZBP : 9;  u64 pad09 : 15; u64 PSM : 4; u64 pad28 : 4;
    u64 ZMSK : 1; u64 pad33 : 31;
} GsZbuf;

/* ALPHA: the four 2-bit selectors as one byte (PGL_BLEND_SEL), FIX. */
typedef struct __attribute__((may_alias)) GsAlpha {
    u64 ABCD : 8; u64 pad08 : 24; u64 FIX : 8; u64 pad40 : 24;
} GsAlpha;

/* XYOFFSET (sceGsXyoffset). */
typedef struct __attribute__((may_alias)) GsXyoffset {
    u64 OFX : 16; u64 pad16 : 16; u64 OFY : 16; u64 pad48 : 16;
} GsXyoffset;

/* TEX0, TEX1, CLAMP (sceGsTex0, sceGsTex1, sceGsClamp). */
typedef struct __attribute__((may_alias)) GsTex0 {
    u64 TBP0 : 14; u64 TBW : 6; u64 PSM : 6; u64 TW : 4; u64 TH : 4;
    u64 TCC : 1;   u64 TFX : 2; u64 CBP : 14; u64 CPSM : 4; u64 CSM : 1;
    u64 CSA : 5;   u64 CLD : 3;
} GsTex0;

typedef struct __attribute__((may_alias)) GsTex1 {
    u64 LCM : 1;  u64 pad01 : 1; u64 MXL : 3; u64 MMAG : 1; u64 MMIN : 3;
    u64 MTBA : 1; u64 pad10 : 9; u64 L : 2;   u64 pad21 : 11; u64 K : 12;
    u64 pad44 : 20;
} GsTex1;

typedef struct __attribute__((may_alias)) GsClamp {
    u64 WMS : 2;   u64 WMT : 2;   u64 MINU : 10; u64 MAXU : 10;
    u64 MINV : 10; u64 MAXV : 10; u64 pad44 : 20;
} GsClamp;

/* FOGCOL (sceGsFogcol), a view of GsContext.gen[GS_GEN_FOGCOL]. */
typedef struct __attribute__((may_alias)) GsFogcol {
    u64 FCR : 8; u64 FCG : 8; u64 FCB : 8; u64 pad24 : 40;
} GsFogcol;

/* TEX2 (sceGsTex2). */
typedef struct __attribute__((may_alias)) GsTex2 {
    u64 pad00 : 20; u64 PSM : 6; u64 pad26 : 11; u64 CBP : 14;
    u64 CPSM : 4;   u64 CSM : 1; u64 CSA : 5;     u64 CLD : 3;
} GsTex2;

/* MIPTBP1 / MIPTBP2 (sceGsMiptbp1 / 2: levels 1..3 / 4..6). */
typedef struct __attribute__((may_alias)) GsMiptbp {
    u64 TBP1 : 14; u64 TBW1 : 6; u64 TBP2 : 14; u64 TBW2 : 6;
    u64 TBP3 : 14; u64 TBW3 : 6; u64 pad60 : 4;
} GsMiptbp;

/* TEXA (sceGsTexa), a view of GsContext.gen[GS_GEN_TEXA]. */
typedef struct __attribute__((may_alias)) GsTexa {
    u64 TA0 : 8; u64 pad08 : 7; u64 AEM : 1; u64 pad16 : 16;
    u64 TA1 : 8; u64 pad40 : 24;
} GsTexa;

_Static_assert(sizeof(GsFrameHeader) == 0x10, "GsFrameHeader");
_Static_assert(sizeof(GsSubPacketHeader) == 0x10, "GsSubPacketHeader");
_Static_assert(sizeof(GsScissor) == 8, "GsScissor");

/* ======================================================================== */
/* Register shadows                                                         */
/* ======================================================================== */

/* GsContext.dirty bits GS_DIRTY_* and the register field macros GS_FIELD_*:
 * G001_flare.h. */
#define GS_ENABLE_GEN_CTX1  0x000003FF00001FFFull   /* list 0's emit mask at a frame start: no context 2 */

/* The GS register fields as "shift, width" (passed through GS_FIELD_* so
 * that the comma splits the arguments). */
#define GS_TEST_ATE         0, 1
#define GS_TEST_ATST        1, 3        /* PGL_ATEST_* */
#define GS_TEST_AREF        4, 8
#define GS_TEST_AFAIL       12, 2       /* PGL_AFAIL_* */
#define GS_TEST_DATE        14, 1
#define GS_TEST_DATM        15, 1
#define GS_TEST_ZTE         16, 1
#define GS_TEST_ZTST        17, 2       /* GS_ZTST_* */
#define GS_ZBUF_ZBP         0, 9
#define GS_ZBUF_PSM         24, 4
#define GS_ZBUF_ZMSK        32, 1
#define GS_TEX0_TBP0        0, 14
#define GS_TEX0_TBW         14, 6
#define GS_TEX0_PSM         20, 6
#define GS_TEX0_TW          26, 4
#define GS_TEX0_TH          30, 4
#define GS_TEX0_TCC         34, 1
#define GS_TEX0_TFX         35, 2
#define GS_TEX0_CSA         56, 5
#define GS_TEX1_LCM         0, 1
#define GS_TEX1_MXL         2, 3
#define GS_TEX1_MMAG        5, 1
#define GS_TEX1_MMIN        6, 3
#define GS_TEX1_L           19, 2
#define GS_TEX1_K           32, 12
#define GS_TEX2_PSM         20, 6
#define GS_TEX2_CSA         56, 5
#define GS_CLAMP_WMS        0, 2
#define GS_CLAMP_WMT        2, 2
#define GS_CLAMP_MINU       4, 10
#define GS_CLAMP_MAXU       14, 10
#define GS_CLAMP_MINV       24, 10
#define GS_CLAMP_MAXV       34, 10
#define GS_TEXA_AEM         15, 1
#define GS_MIPTBP_TBP1      0, 14       /* MIPTBP1: levels 1..3, MIPTBP2: 4..6 */
#define GS_MIPTBP_TBW1      14, 6
#define GS_MIPTBP_TBP2      20, 14
#define GS_MIPTBP_TBW2      34, 6
#define GS_MIPTBP_TBP3      40, 14
#define GS_MIPTBP_TBW3      54, 6

/* TEST.ZTST */
enum { GS_ZTST_NEVER = 0, GS_ZTST_ALWAYS = 1, GS_ZTST_GEQUAL = 2, GS_ZTST_GREATER = 3 };

#define GS_DTHE_ON          1               /* DTHE.DTHE */
#define GS_COLCLAMP_CLAMP   1               /* COLCLAMP.CLAMP (else the colour wraps) */

/* GsContext.lightFlags bits: PGL_LF_* (G001_flare.h). */

/* GsContext.matrixMode, and where VU0 parks the matrix not in vf28..vf31. */
enum { PGL_MODELVIEW = 0, PGL_PROJECTION = 1 };
#define VU0_MODELVIEW_QW    4
#define VU0_PROJECTION_QW   8

/* Packet-buffer segments (GsContext.addrBits by PGL_PKT_* mode). */
#define SEG_CACHED          0x00000000u
#define SEG_UNCACHED        0x20000000u
#define SEG_UNCACHED_ACCEL  0x30000000u

/* Byte / half / word views of the shadows (the binary stores sub-fields
 * with sb / sh / sw): FRAME FBP (u16 0), FBW (u8 2), PSM (u8 3), FBMSK (u32
 * 1); XYOFFSET OFX (u16 0), OFY (u16 2); ALPHA selectors (u8 0), FIX (u8 4);
 * TEXA TA0 (u8 0), TA1 (u8 4); FOGCOL r, g, b (u8 0..2); TEX0 byte 7 (CSA,
 * CLD). */
typedef u64 u64a __attribute__((may_alias));
typedef u32 u32a __attribute__((may_alias));
typedef u16 u16a __attribute__((may_alias));
typedef u8  u8a  __attribute__((may_alias));
#define SHADOW_U8(r)   ((u8a *)&(r))
#define SHADOW_U16(r)  ((u16a *)&(r))
#define SHADOW_U32(r)  ((u32a *)&(r))

/* "A packet is active": packetOpen or subPacketOpen (one u16 test). */
#define GS_PACKET_ACTIVE(ctx) ((ctx)->packetOpen || (ctx)->subPacketOpen)

/* ======================================================================== */
/* Packets                                                                  */
/* ======================================================================== */

/* DMA source-chain tags (word 0: ID << 28 | QWC; bit 31 IRQ; bits 16..25
 * carry the gfx_put_irq_marker code to the DMAC handler, CHCR.TAG). */
#define DMATAG_NEXT         0x20000000u
#define DMATAG_END          0x70000000u

/* VIF1 codes (CMD << 24 | NUM << 16 | IMMEDIATE; bit 31 the i-bit). */
#define VIF_OFFSET(o)       (0x02000000u | (o))
#define VIF_BASE(b)         (0x03000000u | (b))
#define VIF_STMOD(m)        (0x05000000u | (m))
#define VIF_MSKPATH3        0x06000000u     /* | mask << 15 */
#define VIF_FLUSHE          0x10000000u
#define VIF_MPG             0x4A000000u     /* | num << 16 | VU address */
#define VIF_IBIT            0x80000000u     /* interrupt (u118's intc_vif1_handler reads the code) */

/* STMASK of the list starts: x, y, z written, w write-protected. */
#define STMASK_DEFAULT      0xC0C0C0C0u
/* The VU1 double buffer (VIF BASE 0, OFFSET 0x1B3) set at a frame start. */
#define VU1_BUF_OFFSET      0x1B3

/* GIF tag register descriptors (the tag fields: G001_flare.h). */
#define GIF_REGS_XYZ3_XYZ2  ((u64)SCE_GS_XYZ3 | (u64)SCE_GS_XYZ2 << 4) /* 0x5D */

/* GS PRIM (and GsContext.primAttr bits). */
enum {
    GS_PRIM_SPRITE = 0x006,
    GS_PRIM_IIP    = 0x008,
    GS_PRIM_TME    = 0x010,
    GS_PRIM_FGE    = 0x020,
    GS_PRIM_ABE    = 0x040,
    GS_PRIM_AA1    = 0x080,
    GS_PRIM_FST    = 0x100
};

#define GS_RGBAQ_Q_ONE      0x3F800000u     /* RGBAQ.Q = 1.0f (the high word) */

/* Header qword of a list-0 block: DMA cnt (qwc) + VIF FLUSH + DIRECT qwc. */
static inline void gs_put_cnt_direct(u32 *p, u32 qwc)
{
    p[0] = DMATAG_CNT(qwc);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(qwc);
    p[1] = 0;
}

/* A DMA next tag (TADR = next) at link. */
static inline void gs_put_next_tag(u32 *link, u32 *next)
{
    link[0] = DMATAG_NEXT;
    link[1] = (u32)next & 0x0FFFFFFF;
    link[2] = 0;
    link[3] = 0;
}

/* A DMA tag with only word 0 set, the rest zero (end / ret tags). */
static inline void gs_put_tag(u32 *p, u32 tag)
{
    p[0] = tag;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
}

/* Aliasing-safe view of a packet dword at word index w. */
#define PKT64(p, w)   (*(u64a *)((u32 *)(p) + (w)))
typedef union GsPktDword { u64 d; u32 w[2]; } GsPktDword;
#define PKT64U(p, w)  (((GsPktDword *)((u32 *)(p) + (w)))->d)

/* One A+D qword (data, register address) at q; q advances one qword. */
#define GS_PUT_AD(q, d, a) do { PKT64(q, 0) = (d); PKT64(q, 2) = (a); (q) += 4; } while (0)
/* The same through a block-local copy of q taken before q advances (the
 * original's idiom where retail gives each such qword its own base
 * register: gs_fill_target_b, gs_copy_target): GS_PUT_AD2 with the data
 * as two words (ALPHA {ABCD, FIX}, RGBAQ {RGBA, Q}), GS_PUT_ADV with the
 * union view (alias set 0: the strip loops, whose target-field loads are
 * then redone on every pass, as retail does). */
#define GS_PUT_AD2(q, lo, hi, a) \
    do { u32 *t_ = (q); (q) += 4; t_[0] = (lo); t_[1] = (hi); PKT64(t_, 2) = (a); } while (0)
#define GS_PUT_ADV(q, data, reg) \
    do { GsPktDword *t_ = (GsPktDword *)(q); (q) += 4; t_[0].d = (data); t_[1].d = (reg); } while (0)
/* GS_PUT_AD2 through the union view (gs_emit_render_target_regs). */
#define GS_PUT_AD2U(q, lo, hi, reg) \
    do { GsPktDword *t_ = (GsPktDword *)(q); (q) += 4; t_[0].w[0] = (lo); t_[0].w[1] = (hi); t_[1].d = (reg); } while (0)

/* libgraph's SCE_GS_SET_TEX0 (all twelve fields in its order: constant
 * fields fold only where adjacent, gs_copy_target keeps two constants). */
#ifndef SCE_GS_SET_TEX0
#define SCE_GS_SET_TEX0(tbp0, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((u64)(tbp0) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | ((u64)(tw) << 26) |  \
     ((u64)(th) << 30) | ((u64)(tcc) << 34) | ((u64)(tfx) << 35) | ((u64)(cbp) << 37) | \
     ((u64)(cpsm) << 51) | ((u64)(csm) << 55) | ((u64)(csa) << 56) | ((u64)(cld) << 61))
#endif
/* A float variable's bits (gs_set_depth_bias_at: mfc1 into a GPR).  egcs
 * reads them through a pointer cast (a union local costs an extra move per
 * value); GCC 14 (strict aliasing) through a union. */
#ifdef BOOT_OLD_GCC
#define F32_BITS(f) (*(u32 *)&(f))
#else
static inline u32 f32_bits(f32 f) { union { f32 f; u32 u; } b; b.f = f; return b.u; }
#define F32_BITS(f) f32_bits(f)
#endif

/* One PACKED A+D qword: the data, then the register address. */
typedef struct GsAD { u64 data; u64 addr; } GsAD;
static inline void gs_set_ad(GsAD *ad, u64 data, u64 addr)
{
    ad->data = data;
    ad->addr = addr;
}

static inline u32 gs_pack_xy(u32 x, u32 y) { return (y << 16) | x; }

/* Two words as one dword, `hi' in the upper half: a GPR half of a VU0
 * vector (gs_set_depth_bias_at's pcpyld).  The inline's arguments are fresh
 * registers: retail's allocation of the four float bit patterns. */
static inline u64 gs_pack_u32x2(u32 lo, u32 hi) { return ((u64)hi << 32) | lo; }

/* GS_PUT_AD through the union view (alias set 0). */
#define GS_PUT_ADU(q, d, a) do { PKT64U(q, 0) = (d); PKT64U(q, 2) = (a); (q) += 4; } while (0)
/* One REGLIST XYZ entry (xy word, z word); q advances two words. */
#define GS_PUT_XYZ(q, xy, zz) do { (q)[0] = (xy); (q)[1] = (zz); (q) += 2; } while (0)

/* Physical (DMA) address of a buffer pointer. */
#define DMA_ADDR(p)   ((u32)(p) & 0x0FFFFFFF)

/* DMA source address: physical, plus the SPR bit for a 0x7xxxxxxx pointer. */
static inline u32 gs_dma_src_addr(u32 a)
{
    u32 seg = a & 0x70000000;

    a &= 0x0FFFFFFF;
    if (seg == 0x70000000)
        a |= 0x80000000;
    return a;
}

/* ======================================================================== */
/* Hardware                                                                 */
/* ======================================================================== */

#define GIF_CTRL     REG32(0x10003000)
#define GIF_MODE     REG32(0x10003010)
#define GIF_STAT     REG32(0x10003020)
#define VIF1_STAT    REG32(0x10003C00)
#define VIF1_FBRST   REG32(0x10003C10)
#define D_CTRL       REG32(0x1000E000)
#define D_STAT       REG32(0x1000E010)
#define D_PCR        REG32(0x1000E020)

#define GIF_CTRL_RST        1
#define GIF_MODE_IMT        4               /* intermittent PATH3 */
#define GIF_STAT_APATH      0x00000C00u     /* the path transferring (0 idle) */
#define VIF_FBRST_RST       1
#define VIF1_STAT_BUSY      0x1F000003u     /* FQC (FIFO count) | VPS (VIF status) */
#define VPU_STAT_VBS1       0x100           /* COP2 vi29: VU1 busy */
#define FBRST_RS1           0x200           /* COP2 vi28: reset VU1 */
#define D_CTRL_DMAE         1
#define D_STAT_CIS_CIM_012  0x00070007u     /* channel 0..2 status (write 1: clear) and mask (write 1: flip) bits */
#define D_PCR_KEEP          0x7FF8FFF8u     /* clears PCE and CPC / CDE of channels 0..2 */
#define D_CHCR_DIR_FROM_MEM 0x001
#define D_CHCR_TTE          0x040           /* transfer the tag */
#define D_CHCR_TIE          0x080           /* tag interrupt enable */
#define D_CHCR_STR          0x100           /* start / busy */
#define D_CHCR_MODE_KEEP    0xFFFFFEF0u     /* clears DIR, MOD (normal mode) and STR */

#define GS_REG_PMODE    REG64(GS_PMODE)
#define GS_REG_SMODE2   REG64(GS_SMODE2)
#define GS_REG_DISPFB2  REG64(GS_DISPFB2)
#define GS_REG_DISPLAY2 REG64(GS_DISPLAY2)
#define GS_REG_BGCOLOR  REG64(GS_BGCOLOR)
#define GS_PMODE_EN2    2                   /* read circuit 2 on */
#define GS_PMODE_INIT   0x66                /* EN2, CRTMD 1, MMOD 1 (blend by ALP = 0), AMOD 1 */

/* SMODE2 (INT | FFMD << 1): interlaced field 1, interlaced frame 3,
 * progressive 2 (the value the binary stores for non-interlaced). */
#define GS_SMODE2_FIELD     1
#define GS_SMODE2_FRAME     3
#define GS_SMODE2_NOINTER   2

/* R5900 "sync.l" (encoded `sync` 0x0000000F). */
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")

/* GsContext.field, as gfx_set_field_mode reads it: through a volatile
 * lvalue (the vblank handler updates it). */
#define GS_FIELD_VOLATILE(ctx) (*(volatile s8 *)&(ctx)->field)

/* EE COP1 max.s / min.s.  The original's were hand-written volatile asm,
 * one instruction each (the matching pass of 2026-10-16, u116 notes): a
 * volatile asm is a scheduling barrier whose dependents wait one cycle, and
 * retail's order after every clamp of this unit is that one (the dirty-mask
 * load, the conversion, the constant ...): gfx_alpha_func and
 * gfx_set_fog_color_f are exact only so, gs_fill_target_b and gs_copy_target
 * gain; C's MAX_EXPR (a >= b ? a : b under -ffast-math: max.s a, b) with or
 * without the loop notes of a statement macro does not reach them.  The
 * bounds are loaded before the barrier, as retail shows.  GCC 14 (and
 * NON_MATCHING) get the C, the same result but for -0.0 / NaN operands. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 ee_maxf(f32 a, f32 b) { __asm__ __volatile__("max.s %0, %0, %1" : "+f"(a) : "f"(b)); return a; }
static inline f32 ee_minf(f32 a, f32 b) { __asm__ __volatile__("min.s %0, %0, %1" : "+f"(a) : "f"(b)); return a; }
#else
static inline f32 ee_maxf(f32 a, f32 b) { return a >= b ? a : b; }
static inline f32 ee_minf(f32 a, f32 b) { return a <= b ? a : b; }
#endif
/* clamp(x, lo, hi) as max.s then min.s. */
static inline f32 ee_clampf(f32 x, f32 lo, f32 hi) { x = ee_maxf(x, lo); x = ee_minf(x, hi); return x; }

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

/* 0x002FC1F8 .data {NEVER, ALWAYS, GREATER, GEQUAL}: PGL_DEPTHFUNC_* ->
 * TEST.ZTST (4 entries; defined in gs_display_driver.c).  Declared without
 * its size and not const: retail reaches it with lui/addiu, not $gp (a sized
 * 4-byte array is small data under -G8), and has it in .data (a const one
 * would be .rodata). */
extern u8 g_gsDepthFuncZtst[];              /* 0x002FC1F8 */

/* ======================================================================== */
/* Functions of the unit that no other unit calls (address order)          */
/* ======================================================================== */

void gfx_init_vu0_constants_spr(void);                                 /* 0x00250A58 (no callers) */
void gfx_reset_graph_stored_mode(void);                                /* 0x00250BA0 (no callers) */
void gfx_update_display_regs(GfxDriverState *g);                       /* 0x00250C10 */
void gfx_dma_channel_init(int ch, u32 chcr);                           /* 0x00250DD8 */
void gfx_mskpath3(int mask);                                           /* 0x002518A8 */
u32 *gfx_get_packet_ptr(void);                                         /* 0x002519B8 (no callers) */
int  gfx_get_packet_space(void);                                       /* 0x002519C8 (no callers) */
void gfx_wait_vblank_event(void);                                      /* 0x00252260 */
void gs_build_clear_packet(GsContext *ctx, u64 test, u32 rgba, u32 z,
                           const GsRenderTarget *tgt);                 /* 0x00252FC8 */
void gs_emit_render_target_regs(GsContext *ctx);                       /* 0x002531F8 */
void gs_fill_target_b(GsContext *ctx, u32 blend, float fix);           /* 0x00253358 */
void gs_copy_target(GsContext *ctx, const GsRenderTarget *dst,
                    const GsRenderTarget *src, u32 blend, int dy, int dv,
                    float fix);                                        /* 0x002534B8 */
void gs_emit_xyoffset_integer(GsContext *ctx);                         /* 0x002537A0 */

#endif /* U116_H */
