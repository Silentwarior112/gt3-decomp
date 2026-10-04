/* gs_driver.cpp - Polyphony's hand-written GS / VIF / GIF display driver of the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102), unit "gs", .text 0x0100BE10..0x0100F130.
 *
 * The unit is three translation units of the original (tools/layout.py), all
 * compiled with -fno-exceptions (no EH frame block covers any of them), in
 * link order with what retail links between them:
 *   src/gs_texture.cpp  0x0100BE10..0x0100BF98  gfx_set_texture_regs;
 *                       .data s_gsTexOverride @0x01034640
 *     (data-only: the VU1 microcode blob, .data 0x01034730..0x010384D8)
 *   src/gs_driver.cpp   0x0100BF98..0x0100EE48  this file: the driver proper;
 *                       .data g_010384D8 @0x010384D8, s_gsDmaChan @0x010394D8,
 *                       .rodata 0x0103E360..0x0103E5E0, .lit4, .sdata, .bss
 *     (libgraph: .data 0x010394F0..; its code is in function sections)
 *   src/gs_state.cpp    0x0100EE48..0x0100F130  register flush + materials;
 *                       .data 0x01039510.. (register-number tables),
 *                       .rodata 0x0103E5E0..
 * The exact boundaries between gfx_set_texture_regs / gfx_init_vu0_constants
 * and dma_start_source_chain / gs_collect_dirty_regs are not pinned by data
 * (see the two files' headers).
 *
 * See include/gs_driver.h for the data structures.  Summary of what the code
 * does (all verified against the listing):
 *   - gfx_init: sceGsResetGraph, GS IMR, display registers, VIF/GIF/VU reset,
 *     DMA channels 0-2 (mutex semaphores + DMAC handlers), INTC handlers for
 *     VBLANK_S / VIF1 / GS, context init in scratchpad, VU0 + VU1 microcode
 *     upload (the VU1 program is stored LCG-XOR-scrambled in .data and
 *     descrambled once before upload - light obfuscation, nothing to do with
 *     core.gt3), COP0 performance counters.
 *   - a shadow-register state machine (GsContext.reg/gen + dirty mask) that is
 *     flushed into a VIF1 DMA source chain as DIRECT/A+D blocks;
 *   - frame packets made of two interleaved source chains (VIF1 and GIF),
 *     kicked by gfx_kick_frame_lists, completion tracked by the DMAC handler
 *     and waited for with SleepThread / iWakeupThread (gfx_dma_sync);
 *   - vblank counting / pacing (gfx_vsync), display-mode changes, frame-buffer
 *     flips (gfx_set_display_fb), clears, textured sprites, image uploads;
 *   - OpenGL-like light/material state consumed by the VU1 microcode.
 * Nothing in this unit touches core.gt3, hashing or signatures.
 *
 * Address-segment note: ctx->cur/end/tail pointers may carry segment bits
 * (ctx->addrBits); DMA tag addresses are always masked with 0x0FFFFFFF and
 * scratchpad (0x7xxxxxxx) sources get the SPR bit 0x80000000.
 */
#include "types.h"
#include "eekernel.h"
#include "libc.h"
#include "libgraph.h"
#include "gs_driver.h"
#include "gs_internal.h"

/* ======================================================================== */
/* Hardware registers                                                       */
/* ======================================================================== */

#define GIF_CTRL     REG32(0x10003000)
#define GIF_MODE     REG32(0x10003010)
#define GIF_STAT     REG32(0x10003020)
#define VIF0_FBRST   REG32(0x10003810)
#define VIF0_ERR     REG32(0x10003820)
#define VIF1_STAT    REG32(0x10003C00)
#define VIF1_FBRST   REG32(0x10003C10)
#define VIF1_ERR     REG32(0x10003C20)
#define VIF1_CODE    REG32(0x10003C80)
#define D_CTRL       REG32(0x1000E000)
#define D_STAT       REG32(0x1000E010)
#define D_PCR        REG32(0x1000E020)

#define GS_REG_PMODE    REG64(GS_PMODE)
#define GS_REG_SMODE2   REG64(GS_SMODE2)
#define GS_REG_DISPFB2  REG64(GS_DISPFB2)
#define GS_REG_DISPLAY2 REG64(GS_DISPLAY2)
#define GS_REG_BGCOLOR  REG64(GS_BGCOLOR)
#define GS_REG_CSR      REG64(GS_CSR)
#define GS_REG_IMR      REG64(GS_IMR)
#define GS_REG_BUSDIR   REG64(GS_BUSDIR)

/* GS IMR store.  The retail code writes IMR through a plain (non-volatile)
 * pointer: egcs folds that constant address into the store (`lui $at` +
 * offset), which it never does for a volatile access.  Under the retail
 * compiler the store is therefore written non-volatile - the functions using
 * it compile to the retail instructions, so the access is the retail one;
 * GCC 14 keeps it volatile.                                                */
#ifdef BOOT_OLD_GCC
#define GS_IMR_PUT(v)   (*(u64 *)GS_IMR = (v))
#else
#define GS_IMR_PUT(v)   (GS_REG_IMR = (v))
#endif

/* GS IMR and its shadow (shadow first).  A do { } while (0) macro in the
 * original: egcs keeps the loop notes of such a block in the RTL, and its
 * scheduler moves nothing across them - in gfx_init the CSR revision read
 * stays ahead of the IMR writes instead of being interleaved with them.  */
#define GFX_SET_IMR(v)  do { g_gfx.imr = (v); GS_IMR_PUT(v); } while (0)

/* 0.2f and 0.8f with the bits of the retail .lit4 (0x3E4CCCCD, 0x3F4CCCCD:
 * rounded to nearest).  GCC 14 converts a decimal float literal for the R5900
 * by truncation (its EE float format rounds toward zero), one ulp lower
 * (0x3E4CCCCC, 0x3F4CCCCC), so the behaviour build spells the exact values in
 * hex; egcs keeps the decimal literals (the same instructions as before).  */
#ifdef BOOT_OLD_GCC
#define GS_F_0_2    0.2f
#define GS_F_0_8    0.8f
#else
#define GS_F_0_2    0x1.99999Ap-3f
#define GS_F_0_8    0x1.99999Ap-1f
#endif

/* COP0 performance counter control (PCCR = 0: counters stopped). */
#define EE_PCCR_CLEAR() do { __asm__ __volatile__("mtps $0, 0\n\tsync.p" : : : "memory"); } while (0)

/* One EE DMAC channel register block (Dn_CHCR at +0x00). */
typedef struct EeDmaChanRegs {
    volatile u32 chcr;  u32 pad04[3];   /* +0x00 */
    volatile u32 madr;  u32 pad14[3];   /* +0x10 */
    volatile u32 qwc;   u32 pad24[3];   /* +0x20 */
    volatile u32 tadr;  u32 pad34[3];   /* +0x30 */
    volatile u32 asr0;  u32 pad44[3];   /* +0x40 */
    volatile u32 asr1;  u32 pad54[3];   /* +0x50 */
    u32 pad60[8];                       /* +0x60 */
    volatile u32 sadr;                  /* +0x80 */
} EeDmaChanRegs;

#define D1 ((EeDmaChanRegs *)0x10009000)   /* VIF1 */
#define D2 ((EeDmaChanRegs *)0x1000A000)   /* GIF  */

/* R5900 "sync.l" (encoded `sync` 0x0000000F) and "sync.p" (0x0000040F). */
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")
#define SYNC_P() __asm__ __volatile__("sync.p" : : : "memory")

/* Register get / put as statement macros (do { } while (0)), the style of
 * the original's VIF1 resume (FBRST.STC) and GIF DMA restart (QWC = 0,
 * CHCR |= STR, then sync.l).  It shows twice in the binary:
 *   - egcs aligns the first label inside such a "loop" to 8 bytes
 *     (.p2align 3): the retail handlers' padding nop in front of their exit
 *     sequence (intc_vif1_handler 0x0100E874, dmac_channel_handler 0x0100EA64);
 *   - the loop notes weight the register references inside, which decides
 *     local register allocation in the restart (D2 base in $v0, CHCR in $v1). */
#define GS_REG_GET(var, reg)  do { (var) = (reg); } while (0)
#define GS_REG_PUT(reg, val)  do { (reg) = (val); } while (0)
#define GS_VIF1_RESUME()      GS_REG_PUT(VIF1_FBRST, 8)
/* iWakeupThread every thread of a waiter list (a statement macro as well:
 * its loop notes decide the handlers' register allocation - the waiter
 * list base and the channel stride in intc_vblank_handler's s3 / s2). */
#define GS_WAKE_WAITERS(head)  do {                                     \
        GfxWaitNode *w_;                                                \
        for (w_ = (head); w_ != NULL; w_ = w_->next)                    \
            iWakeupThread(w_->tid);                                     \
    } while (0)
#define GS_GIF_RESTART()  do {                                          \
        u32 c_;                                                         \
        GS_REG_GET(c_, D2->chcr);                                       \
        GS_REG_PUT(D2->qwc, 0);                                         \
        GS_REG_PUT(D2->chcr, c_ | 0x100);                               \
        SYNC_L();                                                       \
    } while (0)

/* Aliasing-safe views for packet words / shadow sub-fields. */
typedef u64 u64a __attribute__((may_alias));
typedef u32 u32a __attribute__((may_alias));
typedef u16 u16a __attribute__((may_alias));
#define PKT64(p, w)   (*(u64a *)((u32 *)(p) + (w)))   /* dword at word index w */

/* The same dword written through a union.  egcs gives a direct union member
 * access alias set 0 (type punning through a union), so such a store
 * conflicts with every later load: in gs_build_image_transfer_packet the
 * dstBitblt word is read only after the BITBLTBUF address store, as in the
 * binary.  (For GCC 14 the union access is also the aliasing-safe form.) */
typedef union GsPktDword { u64 d; u32 w[2]; } GsPktDword;
#define PKT64U(p, w)  (((GsPktDword *)((u32 *)(p) + (w)))->d)

/* The GS SCISSOR register as SCE's libgraph declares it (sceGsScissor):
 * 11-bit bitfields.  gs_update_scissor_and_viewport writes the shadow
 * through it - egcs's bitfield stores give the retail and-mask/or
 * sequence, with the four fields in SCAX0, SCAY0, SCAX1, SCAY1 order.    */
typedef struct __attribute__((may_alias)) GsScissor {
    u64 SCAX0 : 11; u64 pad11 : 5;
    u64 SCAX1 : 11; u64 pad27 : 5;
    u64 SCAY0 : 11; u64 pad43 : 5;
    u64 SCAY1 : 11; u64 pad59 : 5;
} GsScissor;
static_assert(sizeof(GsScissor) == 8, "GsScissor");

/* "A packet is active": packetOpen or subPacketOpen.  egcs merges the two
 * adjacent byte tests into one word-mode load (`ld +0x1B8; and
 * 0xFFFF000000000000`), as in the binary.                                  */
#define GS_PACKET_ACTIVE(ctx) ((ctx)->packetOpen || (ctx)->subPacketOpen)

/* GS_SPR_CTX (the scratchpad context pointer, loaded whole) and ee_maxf /
 * ee_minf (COP1 max.s / min.s) are in gs_internal.h, shared with
 * src/gs_texture.cpp and src/gs_state.cpp. */

/* Physical DMA address of a buffer pointer. */
#define DMA_ADDR(p)   ((u32)(p) & 0x0FFFFFFF)

/* DMA source address of a buffer: physical address, plus the SPR bit
 * (0x80000000) for a scratchpad (0x7xxxxxxx) pointer.  An inline function
 * in the original: its parameter copy shows in every user (the masked value
 * stays in the argument's register, the segment test gets a temporary). */
static inline u32 gs_dma_src_addr(u32 a)
{
    u32 seg = a & 0x70000000;

    a &= 0x0FFFFFFF;
    if (seg == 0x70000000)
        a |= 0x80000000;
    return a;
}

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* File-local in the original: the retail object has it in .bss between the
 * startup-screen buffers and the allocator object (a C++ global without an
 * initialiser would go to .data under egcs).                              */
static GfxDriverState g_gfx;                           /* @0x01048180 .bss, 0xC0 */

/* @0x010384D8 .data - 0x1000 zero bytes right after the VU1 microcode; only
 * its address is stored (g_gsVuWork).  Explicitly initialised: cc1plus
 * writes an initialised variable out where it is defined but a global
 * without an initialiser only at the end of the unit, after everything
 * else, and retail has this buffer BEFORE s_gsDmaChan in the object's .data
 * (0x010384D8 / 0x010394D8).  Zero either way.                            */
u8 g_010384D8[0x1000] = { 0 };

/* @0x010394D8 .data - DMAC channel register bases for channels 0..2. */
static EeDmaChanRegs *s_gsDmaChan[3] = {
    (EeDmaChanRegs *)0x10008000,    /* D0 VIF0 */
    (EeDmaChanRegs *)0x10009000,    /* D1 VIF1 */
    (EeDmaChanRegs *)0x1000A000,    /* D2 GIF  */
};
/* (libgraph's .data - 'PsIIlibgraph2221' @0x010394F0 and sceGsGParam
 *  @0x01039500 - follows: its object was linked between this one and
 *  src/gs_state.cpp, whose register-number tables come next @0x01039510.) */

/* .sdata 0x01041E24..0x01041E37, in definition order: s_gfxInitialized;
 * FLT_MAX (0x7F7FFFFF, the constant-pool literal of gfx_init_vu0_constants,
 * which egcs puts in .sdata but addresses absolutely); g_gsVuWork;
 * s_vu1CodeUnscrambled; s_lcgState.  All explicitly initialised (.sdata, not
 * .sbss), so the later ones are defined next to their users below.        */
static u8   s_gfxInitialized = 0;               /* @0x01041E24 .sdata */

/* g_gsVu1Microcode (@0x01034730, 0x3DA8 bytes, scrambled) and g_gsVu0Microcode
 * (@0x01040C00, 0x398 bytes) are binary blobs extracted by tools/gs_extract.py
 * into data/ and declared in gs_driver.h.                                  */

/* Header qword of a list-0 block: DMA cnt tag (qwc) + VIF FLUSH + VIF
 * DIRECT (qwc).                                                            */
static inline void gs_put_cnt_direct(u32 *p, u32 qwc)
{
    p[0] = qwc | 0x10000000;                /* cnt */
    p[2] = 0x11000000;                      /* FLUSH */
    p[3] = qwc | 0x50000000;                /* DIRECT */
    p[1] = 0;
}

/* DMA next tag (0x20000000, TADR = next) at link.  An inline function in the
 * original: gfx_select_list's two calls get their own copies of the
 * arguments, which shows in its register allocation.                    */
static inline void gs_put_next_tag(u32 *link, u32 *next)
{
    link[0] = 0x20000000;                   /* next */
    link[1] = DMA_ADDR(next);
    link[2] = 0;
    link[3] = 0;
}

/* One A+D qword (data, register address) at q; q advances one qword. */
#define GS_PUT_AD(q, d, a) do { PKT64(q, 0) = (d); PKT64(q, 2) = (a); (q) += 4; } while (0)
/* One REGLIST XYZ entry (xy word, z word); q advances two words. */
#define GS_PUT_XYZ(q, xy, zz) do { (q)[0] = (xy); (q)[1] = (zz); (q) += 2; } while (0)

/* ~a as the R5900 `not` (nor rd, rs, $zero) on a 32-bit value (gs_set_bg_color's
 * colour mask).  egcs never emits that operand order outside MIPS16 (its
 * one_cmpl template is `nor rd, $zero, rs`), so the retail `not`s come from
 * inline asm (see ee_not64 in src/gs_texture.cpp).                        */
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

/* x = sqrt(x) with the EE sqrt.s.  The retail code has it as a volatile asm
 * statement (never moved out of or duplicated by the loop optimiser, unlike
 * types.h's boot_sqrtf). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define EE_SQRTF(x) __asm__ __volatile__("sqrt.s %0, %1" : "=f"(x) : "f"(x))
#else
#define EE_SQRTF(x) ((x) = boot_sqrtf(x))
#endif

/* ======================================================================== */
/* Code                                                                     */
/* ======================================================================== */

// FUNC 0x0100BF98 gfx_wait_vsync_poll
// Polling vblank wait: writes CSR.VSINT (8) to clear the event, spins until
// CSR bit 3 is set again or 0x3A980 polls elapsed, then clears it again.
// Callers: main (before gfx_init) and gfx_init (before sceGsResetGraph).
void gfx_wait_vsync_poll(void)
{
    u32 n = 0x3A980;

    GS_REG_CSR = 8;
    for (;;) {
        u64 csr = GS_REG_CSR;
        n--;                                    /* decremented in the branch delay slot */
        if (csr & 8)
            break;
        if (n == 0)
            break;
    }
    GS_REG_CSR = 8;
}

// FUNC 0x0100BFE8 gfx_reset_paths
// Resets VIF0, VIF1 (FBRST.RST), VU0 and VU1 (COP2 control reg vi28 = FBRST
// |= RS0|RS1 = 0x202), the GIF (GIF_CTRL.RST, GIF_MODE = IMT) and sets
// VIF0_ERR/VIF1_ERR = 6 (ME0|ME1: ignore DMAtag-mismatch and VIFcode errors,
// i-bit interrupts stay enabled).  Polyphony's sceGsResetPath.
void gfx_reset_paths(void)
{
    u32 fbrst;

    VIF0_FBRST = 1;
    VIF1_FBRST = 1;
    /* cfc2 / ori 0x202 / ctc2 on $vi28 (VU FBRST): no C equivalent. */
    __asm__ __volatile__(
        "cfc2   %0, $vi28\n\t"
        "ori    %0, %0, 0x202\n\t"
        "ctc2   %0, $vi28\n\t"
        : "=&r"(fbrst) : : "memory");
    GIF_CTRL  = 1;
    GIF_MODE  = 4;
    VIF0_ERR  = 6;
    VIF1_ERR  = 6;
}

// FUNC 0x0100C050 gfx_init_vu0_constants
// vu0_load_identity_regs() puts an identity matrix in VU0 vf28..vf31; this
// then sets VU0 integer regs vi11 = 0x10, vi12 = 0x50, vi13 = 0x60 and stores
// that matrix (vsqi) to VU0 data memory qwords 4-7, 8-11 and 12-15.  Then it
// writes the context's constant floats (1.0, FLT_MAX x4 from .sdata
// 0x01041E28, 255.0, 0.5, 1.0) and stores &g_010384D8 into g_gsVuWork.
// Callers: gfx_context_init, gfx_init_vu0_constants_spr.
void gfx_init_vu0_constants(GsContext *ctx)
{
    vu0_load_identity_regs();

    /* VU0 macro-mode code (ctc2 / ctc2.i / vsqi / vnop): inline asm only,
     * one statement per operation as in the binary.                       */
    __asm__ __volatile__("ctc2       %0, $vi11" : : "r"(0x10));
    __asm__ __volatile__("ctc2       %0, $vi12" : : "r"(0x50));
    __asm__ __volatile__("ctc2       %0, $vi13" : : "r"(0x60));
    __asm__ __volatile__(
        "ctc2.i     %0, $vi10\n\t"
        "vsqi.xyzw  $vf28, ($vi10++)\n\t"
        "vsqi.xyzw  $vf29, ($vi10++)\n\t"
        "vsqi.xyzw  $vf30, ($vi10++)\n\t"
        "vsqi.xyzw  $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(4) : "memory");
    __asm__ __volatile__(
        "ctc2.i     %0, $vi10\n\t"
        "vsqi.xyzw  $vf28, ($vi10++)\n\t"
        "vsqi.xyzw  $vf29, ($vi10++)\n\t"
        "vsqi.xyzw  $vf30, ($vi10++)\n\t"
        "vsqi.xyzw  $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(8) : "memory");
    __asm__ __volatile__(
        "ctc2.i     %0, $vi10\n\t"
        "vsqi.xyzw  $vf28, ($vi10++)\n\t"
        "vsqi.xyzw  $vf29, ($vi10++)\n\t"
        "vsqi.xyzw  $vf30, ($vi10++)\n\t"
        "vsqi.xyzw  $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(0xC) : "memory");

    ctx->flag00 = 1;
    ctx->flag01 = 0;
    ctx->flag02 = 0;
    ctx->limits[0] = 3.40282347e+38f;       /* FLT_MAX */
    ctx->limits[1] = 3.40282347e+38f;
    ctx->limits[2] = 3.40282347e+38f;
    ctx->limits[3] = 3.40282347e+38f;
    ctx->colorMax = 255.0f;
    ctx->fBB0 = 0.5f;
    ctx->fBB4 = 0.5f;
    ctx->fBB8 = 1.0f;
    ctx->fBBC = 1.0f;
    ctx->f1D4 = 0;
    ctx->f1D8 = 1.0f;
    g_gsVuWork = g_010384D8;
}

void *g_gsVuWork = 0;                       /* @0x01041E2C .sdata */

// FUNC 0x0100C158 gfx_init_lights
// Clears the 8 light records (memset ctx+0x2E0, 0x680) and gives light 0 the
// OpenGL GL_LIGHT0 defaults; also the vector at +0xA00 {0,0,0,1}, the global
// ambient {0.2,0.2,0.2,1} (.lit4 0x01041D00 = 0x3E4CCCCD) and lightFlags = 7,
// lightMode = 2.  Quirk kept: the 0.0f used for several fields is obtained by
// storing 0 to light0+0xC8 and reading it back (sw/lwc1 at 0x0100C188).
void gfx_init_lights(GsContext *ctx)
{
    f32 zero;

    memset(ctx->lights, 0, 0x680);
    ctx->lights[0].atten.z = 0.0f;          /* stored (sw $0) and read back */
    zero = ctx->lights[0].atten.z;
    ctx->lights[0].ambient.w  = 1.0f;
    ctx->lights[0].diffuse.x  = 1.0f;
    ctx->lights[0].diffuse.y  = 1.0f;
    ctx->lights[0].diffuse.z  = 1.0f;
    ctx->lights[0].diffuse.w  = 1.0f;
    ctx->lights[0].specular.x = 1.0f;
    ctx->lights[0].specular.y = 1.0f;
    ctx->lights[0].specular.z = 1.0f;
    ctx->lights[0].specular.w = 1.0f;
    ctx->lights[0].position.z = 1.0f;
    ctx->lights[0].atten.y    = 1.0f;
    ctx->lights[0].atten.w    = 1.0f;
    ctx->lights[0].spotDir.z  = -1.0f;
    ctx->lights[0].qB0.w      = 180.0f;
    ctx->lights[0].spotExpEnc = float_to_gs_param_encode(zero);
    ctx->globalAmbient.x = GS_F_0_2;
    ctx->globalAmbient.y = GS_F_0_2;
    ctx->globalAmbient.z = GS_F_0_2;
    ctx->globalAmbient.w = 1.0f;
    ctx->qA00.x = zero;
    ctx->qA00.y = zero;
    ctx->qA00.z = zero;
    ctx->qA00.w = 1.0f;
    ctx->lightFlags = 7;
    ctx->lightMode = 2;
}

// FUNC 0x0100C230 gfx_init_material
// Clears both material blocks (memset ctx+0x960, 0xA0) and sets the OpenGL
// material defaults on face 0: ambient {0.2,0.2,0.2,1} (.lit4 0x01041D04),
// diffuse {0.8,0.8,0.8,1} (.lit4 0x01041D08), specular/emission w = 1,
// shininessEnc = encode(0.0).  Also +0xAF0..+0xB0C = 1.0, +0xB10..+0xB1C = 0,
// bgParamKind = 2 (ambient+diffuse), bgParamMask = 1, bgImmediate = 0.
void gfx_init_material(GsContext *ctx)
{
    memset(ctx->material, 0, 0xA0);
    ctx->material[0].ambient.x  = GS_F_0_2;
    ctx->material[0].ambient.y  = GS_F_0_2;
    ctx->material[0].ambient.z  = GS_F_0_2;
    ctx->material[0].ambient.w  = 1.0f;
    ctx->material[0].diffuse.x  = GS_F_0_8;
    ctx->material[0].diffuse.y  = GS_F_0_8;
    ctx->material[0].diffuse.z  = GS_F_0_8;
    ctx->material[0].diffuse.w  = 1.0f;
    ctx->material[0].specular.w = 1.0f;
    ctx->material[0].emission.w = 1.0f;
    ctx->material[0].shininessEnc = float_to_gs_param_encode(0.0f);
    ctx->qAF0.x = 1.0f;
    ctx->qAF0.y = 1.0f;
    ctx->qAF0.z = 1.0f;
    ctx->qAF0.w = 1.0f;
    ctx->qB00.x = 1.0f;
    ctx->qB00.y = 1.0f;
    ctx->qB00.z = 1.0f;
    ctx->qB00.w = 1.0f;
    ctx->bgParamKind = 2;
    ctx->bgParamMask = 1;
    ctx->bgImmediate = 0;
    ctx->qB10.x = 0.0f;
    ctx->qB10.y = 0.0f;
    ctx->qB10.z = 0.0f;
    ctx->qB10.w = 0.0f;
}

// FUNC 0x0100C300 gfx_reset_texture_state
// Default texture state (renamed from gfx_reset_blend_state - nothing here is
// blending): TEXA {TA0 0, AEM 0, TA1 0x80}; TEX0 = TCC (1 << 34); TEX1 = 0x60
// (MMAG/MMIN linear); TEX2 = 0x2000000001300000 (PSMT8, CLD 1); MIPTBP1/2 and
// CLAMP = 0.  Dirty |= 0x801E801E80 = TEXA + CLAMP/TEX1/TEX0/MIPTBP1/MIPTBP2 in
// both contexts.  Quirk: TEX2 is reset but not marked dirty.
void gfx_reset_texture_state(GsContext *ctx)
{
    u8 *texa = (u8 *)&ctx->gen[GS_GEN_TEXA];

    texa[0] = 0;                                    /* TA0 */
    texa[4] = 0x80;                                 /* TA1 */
    ctx->gen[GS_GEN_TEXA] &= 0xFFFFFFFFFFFF7FFFull; /* AEM = 0 */
    ctx->dirty |= 0x000000801E801E80ull;
    ctx->reg[GS_REG_TEX0] = 0x0000000400000000ull;
    ctx->reg[GS_REG_TEX1] = 0x60;
    ctx->reg[GS_REG_TEX2] = 0x2000000001300000ull;
    ctx->reg[GS_REG_MIPTBP2] = 0;
    ctx->reg[GS_REG_CLAMP] = 0;
    ctx->reg[GS_REG_MIPTBP1] = 0;
}

// FUNC 0x0100C378 gfx_context_init
// Master context init: memset(ctx, 0, 0xBC0); COLCLAMP.CLAMP = 1,
// PRMODECONT.AC = 1, ZBUF.ZMSK = 0, everything dirty, PRIM attr = IIP (8),
// bgColor 0xFFFFFFFF, bgBlend 0x80808080, videoMode 2, pal 1, displayPeriod
// 0x33E; then texture state, VU0 constants, lights and (tail call) material.
// Callers: gfx_init (ctx = 0x70003000), gfx_context_init_spr.
void gfx_context_init(GsContext *ctx)
{
    memset(ctx, 0, 0xBC0);
    ctx->gen[GS_GEN_COLCLAMP] |= 1;
    ctx->gen[GS_GEN_PRMODECONT] |= 1;
    ctx->reg[GS_REG_ZBUF] &= 0xFFFFFFFEFFFFFFFFull;
    ctx->uncachedBuf = 0;
    ctx->f15C = 1.0f;
    ctx->dirty = GS_DIRTY_ALL;
    ctx->primAttr = 8;
    ctx->f158 = 0;
    ctx->fB40 = 0;
    ctx->bgColor = 0xFFFFFFFF;
    ctx->fB44 = 0;
    ctx->fB4C = 0;
    ctx->frameMode = 0;
    ctx->periodAdjust = 0;
    ctx->bgBlend = 0x80808080;
    ctx->fB48 = 1.0f;
    ctx->videoMode = 2;
    ctx->pal = 1;
    ctx->displayPeriod = 0x33E;
    gfx_reset_texture_state(ctx);
    gfx_init_vu0_constants(ctx);
    gfx_init_lights(ctx);
    gfx_init_material(ctx);                 /* tail call */
}

// FUNC 0x0100C478 gfx_init_default
// gfx_init(fullReset 1, interlace 1, omode, frame mode 1) - tail jump.
// Called from main.
void gfx_init_default(int omode)
{
    gfx_init(1, 1, omode, 1);
}

// FUNC 0x0100C490 gfx_init
// One-shot driver init (guard s_gfxInitialized @0x01041E24).  See file
// header.  fullReset (low byte) selects gfx_wait_vsync_poll + sceGsResetGraph
// (0, ...) versus a plain CSR.FLUSH.  GS IMR = ~0x200 (only FINISH unmasked),
// display 640x448, VU0 microcode (0x398 bytes @0x01040C00) and the descrambled
// VU1 microcode (0x3DA8 bytes @0x01034730) uploaded to VU address 0, COP0
// PCCR = 0x8003783E (CTE, counter 0 = CPU cycles, counter 1 event 6).
void gfx_init(int fullReset, int inter, int omode, int ffmd)
{
    GsContext *ctx = GS_SPR_CTX;
    GfxDriverState *g = &g_gfx;
    u8 doReset = (u8)fullReset;

    if (s_gfxInitialized == 1)
        return;

    memset((void *)g, 0, 0xC0);
    gfx_store_display_mode(inter, omode, ffmd);
    if (doReset) {
        gfx_wait_vsync_poll();
        sceGsResetGraph(0, (short)inter, (short)omode, (short)ffmd);
    } else {
        GS_REG_CSR = 0x100;                         /* FLUSH */
    }
    g->gsRevision = (u8)((GS_REG_CSR >> 16) & 0xFF);
    GFX_SET_IMR((u64)(s64)-0x201);                  /* ~0x200: only FINISH unmasked */
    GS_REG_CSR = 2;                                 /* clear FINISH */
    gfx_set_screen_size(0x280, 0x1C0);
    gfx_set_display_area(0, 0, 0x280, 0x1C0);
    gfx_reset_paths();
    gfx_dma_init();
    xor_unscramble_with_lcg(g_gsVu1Microcode, 0x3DA8);
    g->vsyncCallback = 0;
    g->intcIdVblank = AddIntcHandler(INTC_VBLANK_S, intc_vblank_handler, 0);
    g->intcIdVif1   = AddIntcHandler(INTC_VIF1, intc_vif1_handler, 0);
    g->intcIdGs     = AddIntcHandler(INTC_GS, intc_gs_finish_handler, 0);
    EnableIntc(INTC_VBLANK_S);
    EnableIntc(INTC_VIF1);
    EnableIntc(INTC_GS);
    gfx_context_init(ctx);
    vu_upload_microcode(0, 0, 0x398, g_gsVu0Microcode);
    vu_upload_microcode(1, 0, 0x3DA8, g_gsVu1Microcode);

    /* COP0 performance counters: PCCR = 0; PCR0 = PCR1 = 0; PCCR = 0x8003783E. */
    __asm__ __volatile__(
        "mtps   $0, 0\n\t"
        "sync.p\n\t"
        "mtpc   $0, 0\n\t"
        "mtpc   $0, 1\n\t"
        "mtps   %0, 0\n\t"
        "sync.p\n\t"
        : : "r"(0x8003783E) : "memory");

    s_gfxInitialized = 1;
}

// FUNC 0x0100C6A0 gfx_context_init_spr
// gfx_context_init on the scratchpad context.  No callers in this binary.
void gfx_context_init_spr(void)
{
    gfx_context_init(GS_SPR_CTX);
}

// FUNC 0x0100C6C8 gfx_init_vu0_constants_spr
// gfx_init_vu0_constants(GS_CTX).  No callers in this binary.
void gfx_init_vu0_constants_spr(void)
{
    gfx_init_vu0_constants(GS_SPR_CTX);
}

// FUNC 0x0100C6F0 gfx_init_lights_material_spr
// gfx_init_lights + gfx_init_material on GS_CTX.  No callers in this binary.
void gfx_init_lights_material_spr(void)
{
    gfx_init_lights(GS_SPR_CTX);
    gfx_init_material(GS_SPR_CTX);
}

// FUNC 0x0100C728 gfx_reset_texture_state_spr
// gfx_reset_texture_state(GS_CTX).  No callers in this binary.
void gfx_reset_texture_state_spr(void)
{
    gfx_reset_texture_state(GS_SPR_CTX);
}

// FUNC 0x0100C750 gfx_shutdown
// Undo gfx_init (if initialised): DMA shutdown, disable and remove the INTC
// handlers (GS, VIF1, VBLANK_S), mask every GS interrupt (IMR = 0xFFFFFFFF,
// also in the shadow), stop the COP0 counters (PCCR = 0).  Called by main
// before launching core.gt3.
void gfx_shutdown(void)
{
    if (s_gfxInitialized == 0)
        return;
    gfx_dma_shutdown();
    DisableIntc(INTC_GS);
    DisableIntc(INTC_VIF1);
    DisableIntc(INTC_VBLANK_S);
    RemoveIntcHandler(INTC_GS, g_gfx.intcIdGs);
    RemoveIntcHandler(INTC_VIF1, g_gfx.intcIdVif1);
    RemoveIntcHandler(INTC_VBLANK_S, g_gfx.intcIdVblank);
    g_gfx.imr = 0xFFFFFFFFull;
    GS_IMR_PUT(0xFFFFFFFFull);
    EE_PCCR_CLEAR();                        /* counters off */
    s_gfxInitialized = 0;
}

// FUNC 0x0100C7E0 gfx_change_display_mode
// If (inter, omode, ffmd) differ from the stored bytes (compared against the
// full int arguments): store them, recompute DISPLAY2/PMODE shadows and
// re-program the CRT via gfx_reset_graph_stored_mode (tail jump).
// Caller: F_01000C88 (1, 2, 0).
void gfx_change_display_mode(int inter, int omode, int ffmd)
{
    GfxDriverState *g = &g_gfx;

    if (g->inter == inter && g->omode == omode && g->ffmd == ffmd)
        return;
    gfx_store_display_mode(inter, omode, ffmd);
    gfx_update_display_regs(g);
    gfx_reset_graph_stored_mode();          /* tail jump to 0x0100C890 */
}

// FUNC 0x0100C848 gfx_store_display_mode
// Stores the mode bytes and derives the SMODE2 shadow: INT (bit 0) when
// interlaced, FFMD (bit 1) = frame mode; non-interlaced always gets 2.
void gfx_store_display_mode(int inter, int omode, int ffmd)
{
    GfxDriverState *g = &g_gfx;

    g->omode = (u8)omode;
    g->inter = (u8)inter;
    g->ffmd  = (u8)ffmd;
    if (inter == 1) {
        if (ffmd == 1)                      /* binary: bne ffmd, inter (== 1) */
            g->smode2 = 3;
        else
            g->smode2 = 1;
    } else {
        g->smode2 = 2;
    }
}

// FUNC 0x0100C890 gfx_reset_graph_stored_mode
// sceGsResetGraph(5, inter, omode, ffmd) with the stored bytes (tail jump):
// mode 5 re-issues SetGsCrt without resetting the GS.  Only reached by the
// `j` from gfx_change_display_mode.
void gfx_reset_graph_stored_mode(void)
{
    GfxDriverState *g = &g_gfx;

    sceGsResetGraph(5, g->inter, g->omode, g->ffmd);
}

// FUNC 0x0100C8B0 gfx_update_display_regs
// Recomputes the DISPLAY2 shadow: MAGH = (screenW + 0x9FF) / screenW
// (break 7 on zero), MAGV = (NTSC ? 0x1C0 : 0x200) / screenH (no zero check),
// DX = dispDX * MAGH + 0x28C (masked 0xFFFF), DY = (NTSC ? 0x19 : 0x24,
// doubled when interlaced) + dispDY, DW = MAGH * dispW - 1, DH = MAGV * h - 1
// with h = dispH (doubled when interlaced, halved in field mode).  Also
// PMODE = 0x66 (EN2, CRTMD 1, MMOD, AMOD) and BGCOLOR = 0.  Reads go through
// `g`, the stores use the absolute address of g_gfx.
void gfx_update_display_regs(GfxDriverState *g)
{
    s32 magh, magv, dyBase, dhBase, dh;
    s32 dx, dy, dw;
    u64 display;

    dhBase = (g->omode == SCE_GS_NTSC) ? 0x1C0 : 0x200;
    dyBase = (g->omode == SCE_GS_NTSC) ? 0x19 : 0x24;
    dx = g->dispDX;
    dy = g->dispDY;
    dw = g->dispW;
    dh = g->dispH;
    magh   = (s32)(g->screenW + 0x9FF) / (s32)g->screenW;
    magv   = dhBase / (s32)g->screenH;
    if (g->inter == 1) {
        dyBase <<= 1;
        dh <<= 1;
    }
    if (g->ffmd == 0)
        dh >>= 1;

    display = (u64)((u32)(dx * magh + 0x28C) & 0xFFFF)
            | ((u64)((u32)(dyBase + dy) & 0xFFF) << 12)
            | ((u64)(s64)(magh - 1) << 23)
            | ((u64)(s64)(magv - 1) << 27)
            | ((u64)(s64)(magh * dw - 1) << 32)
            | ((u64)(s64)(magv * dh - 1) << 44);

    g_gfx.display2 = display;
    g_gfx.pmode    = 0x66;
    g_gfx.bgcolor  = 0;
}

// FUNC 0x0100C998 gfx_set_display_area
// Stores DISPFB DBX/DBY and the display width/height, then tail-calls
// gfx_update_display_regs.  Callers: gfx_init, F_01000C88 (0, 0, 640, 448).
void gfx_set_display_area(int dbx, int dby, int w, int h)
{
    GfxDriverState *g = &g_gfx;

    g->dbx   = (u16)dbx;
    g->dby   = (u16)dby;
    g->dispW = (u16)w;
    g->dispH = (u16)h;
    gfx_update_display_regs(g);
}

// FUNC 0x0100C9B8 gfx_set_screen_size
// Stores the screen size used for MAGH/MAGV and tail-calls
// gfx_update_display_regs.  Callers: gfx_init, F_01000C88 (640, 448).
void gfx_set_screen_size(int w, int h)
{
    GfxDriverState *g = &g_gfx;

    g->screenW = (u16)w;
    g->screenH = (u16)h;
    gfx_update_display_regs(g);
}

// FUNC 0x0100C9D0 gfx_set_display_fb
// Frame flip: DISPFB2 = FBP (fbWordAddr >> 11) | FBW ((fbw >> 6) & 0x3F) << 9 |
// PSM (psm & 0xF - only 4 of the 5 PSM bits) << 15 | DBX << 32 | DBY << 43,
// kept in the shadow, then PMODE, SMODE2, DISPFB2, DISPLAY2 and BGCOLOR are
// written from the shadows; sync.l.  The 4th parameter (the height; caller
// F_01000C88 passes 0x1C0) is unused: the retail code overwrites $a3 in its
// first instruction.  It is declared because the caller sets $a3 for it.
void gfx_set_display_fb(u32 fbWordAddr, int psm, int fbw, int fbh)
{
    GfxDriverState *g = &g_gfx;
    u64 dispfb;
    u64 pmode, smode2;

    dispfb = (u64)(fbWordAddr >> 11)
           | ((u64)((fbw >> 6) & 0x3F) << 9)
           | ((u64)(psm & 0xF) << 15)
           | ((u64)(g->dbx & 0x7FF) << 32)
           | ((u64)(g->dby & 0x7FF) << 43);
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

// FUNC 0x0100CA78 gfx_dma_channel_init
// Channel ch (0..2): CHCR = chcr (0x40 TTE / 0xC0 TTE|TIE / 0x80 TIE), MADR,
// TADR, ASR0, ASR1, SADR = 0, busy/tagIrq = 0, and a mutex semaphore
// (maxCount 1, initCount 1; the other SemaParam fields are stack garbage, as
// in the binary).
void gfx_dma_channel_init(int ch, u32 chcr)
{
    EeDmaChanRegs *d = s_gsDmaChan[ch];
    GfxDriverState *g = &g_gfx;
    struct SemaParam sp;                    /* sp+0x00 */

    d->chcr = chcr;
    d->madr = 0;
    d->tadr = 0;
    d->asr0 = 0;
    d->asr1 = 0;
    d->sadr = 0;
    g->chan[ch].busy = 0;
    g->chan[ch].tagIrq = 0;
    sp.initCount = 1;
    sp.maxCount  = 1;
    g->chan[ch].sema = CreateSema(&sp);
}

// FUNC 0x0100CB08 gfx_dma_init
// Channels 0/1/2 init (CHCR 0x40 / 0xC0 / 0x80), D_STAT written back with its
// own bits 0-2 and 16-18 (clears pending ch0-2 status; quirk: also toggles
// those channels' CIM bits that were set), D_PCR &= 0x7FF8FFF8, D_CTRL.DMAE =
// 1, sync.l, dmac_channel_handler installed on channels 0-2 and enabled.
void gfx_dma_init(void)
{
    GfxDriverState *g = &g_gfx;

    gfx_dma_channel_init(0, 0x40);
    gfx_dma_channel_init(1, 0xC0);
    gfx_dma_channel_init(2, 0x80);
    D_STAT = D_STAT & 0x00070007;
    D_PCR  = D_PCR & 0x7FF8FFF8;
    D_CTRL = D_CTRL | 1;
    SYNC_L();
    g->dmacId[0] = AddDmacHandler(DMAC_VIF0, dmac_channel_handler, 0);
    g->dmacId[1] = AddDmacHandler(DMAC_VIF1, dmac_channel_handler, 0);
    g->dmacId[2] = AddDmacHandler(DMAC_GIF, dmac_channel_handler, 0);
    EnableDmac(DMAC_VIF0);
    EnableDmac(DMAC_VIF1);
    EnableDmac(DMAC_GIF);                   /* tail call */
}

// FUNC 0x0100CC00 gfx_dma_shutdown
// DisableDmac 0/1/2, RemoveDmacHandler with the saved ids, DeleteSema on the
// three channel mutexes.  Caller: gfx_shutdown.
void gfx_dma_shutdown(void)
{
    DisableDmac(DMAC_VIF0);
    DisableDmac(DMAC_VIF1);
    DisableDmac(DMAC_GIF);
    RemoveDmacHandler(DMAC_VIF0, g_gfx.dmacId[0]);
    RemoveDmacHandler(DMAC_VIF1, g_gfx.dmacId[1]);
    RemoveDmacHandler(DMAC_GIF, g_gfx.dmacId[2]);
    DeleteSema(g_gfx.chan[0].sema);
    DeleteSema(g_gfx.chan[1].sema);
    DeleteSema(g_gfx.chan[2].sema);         /* tail call */
}

// FUNC 0x0100CC78 gfx_dma_lock
// WaitSema on channel ch's mutex (tail call).
void gfx_dma_lock(int ch)
{
    WaitSema(g_gfx.chan[ch].sema);
}

// FUNC 0x0100CC98 gfx_dma_unlock
// SignalSema on channel ch's mutex (tail call).
void gfx_dma_unlock(int ch)
{
    SignalSema(g_gfx.chan[ch].sema);
}

// FUNC 0x0100CCB8 gfx_dma_sync
// Waits until channel ch is no longer busy.  Returns 0 at once if idle.
// Otherwise links a stack node {next, tid} into the channel's waiter list
// (under DIntr/EIntr) and sleeps; the DMAC handler (completion) and the
// vblank handler (every frame) iWakeupThread the listed threads.  After 0x78
// sleeps without completion: busy = 0, the whole waiter list is dropped, the
// channel is stopped (CHCR.STR cleared until it reads back 0) and -1 is
// returned.  Finally the node is unlinked and CancelWakeupThread(tid) drops
// pending wakeups.
int gfx_dma_sync(int ch)
{
    GfxDriverState *g = &g_gfx;
    GfxWaitNode node;                       /* sp+0x00 */
    GfxWaitNode *p, *prev;
    int tid;
    int ret = 0;
    int tries = 0x78;

    if (g->chan[ch].busy == 0)
        return 0;

    tid = GetThreadId();
    node.tid = tid;
    DIntr();
    node.next = g->chan[ch].waiters;
    g->chan[ch].waiters = &node;
    EIntr();

    /* busy re-read through g_gfx, not g: the binary has a second address
     * register for it in the loop. */
    while (g_gfx.chan[ch].busy != 0) {
        if (--tries < 0) {
            EeDmaChanRegs *d;
            g->chan[ch].busy = 0;
            g->chan[ch].waiters = NULL;
            d = s_gsDmaChan[ch];
            if (d->chcr & 0x100) {
                do {
                    d->chcr = d->chcr & 0xFFFFFEFF;
                } while (d->chcr & 0x100);
            }
            ret = -1;
            break;
        }
        SleepThread();
    }

    DIntr();
    prev = NULL;
    for (p = g->chan[ch].waiters; p != NULL; p = p->next) {
        if (p == &node) {
            if (prev != NULL)
                prev->next = node.next;
            else
                g->chan[ch].waiters = node.next;
            break;
        }
        prev = p;
    }
    EIntr();
    CancelWakeupThread(tid);
    return ret;
}

// FUNC 0x0100CE68 gfx_dma_send
// Normal-mode transfer: gfx_dma_sync(ch), busy = 1, QWC = qwc, MADR = madr
// (0x0FFFFFFF-masked, SPR bit for 0x7xxxxxxx), CHCR = (CHCR & 0xFFFFFEF0) |
// 0x101 (DIR from memory, MOD normal, STR); sync.l.  Caller: the Tex1 upload
// F_0100BD38 (GIF channel 2: transfer packet, then pixel data).
void gfx_dma_send(int ch, const void *madr, u32 qwc)
{
    EeDmaChanRegs *d = s_gsDmaChan[ch];

    gfx_dma_sync(ch);
    g_gfx.chan[ch].busy = 1;
    d->qwc = qwc;
    d->madr = gs_dma_src_addr((u32)madr);
    d->chcr = (d->chcr & 0xFFFFFEF0) | 0x101;
    SYNC_L();
}

// FUNC 0x0100CF28 gfx_kick_frame_lists
// Kicks a finished frame: hdr (or ctx->frameHdr when NULL; nothing if that is
// NULL too) = {VIF1 chain, GIF chain}.  Locks and syncs channels 1 and 2,
// zeroes the rendezvous counters and finishCount, starts each non-NULL chain
// with dma_start_source_chain, unlocks.  Caller: startup screen F_01000B40.
void gfx_kick_frame_lists(GsFrameHeader *hdr)
{
    GfxDriverState *g = &g_gfx;
    GsContext *ctx = GS_SPR_CTX;

    if (hdr == NULL) {
        hdr = ctx->frameHdr;
        if (hdr == NULL)
            return;
    }
    gfx_dma_lock(1);
    gfx_dma_lock(2);
    gfx_dma_sync(1);
    gfx_dma_sync(2);
    g->pathSync16 = 0;
    g->pathSync17 = 0;
    g->pathSync18 = 0;
    g->finishCount = 0;
    if (hdr->vif1Chain != NULL)
        dma_start_source_chain(1, hdr->vif1Chain);
    if (hdr->gifChain != NULL)
        dma_start_source_chain(2, hdr->gifChain);
    gfx_dma_unlock(1);
    gfx_dma_unlock(2);                      /* tail call */
}

// FUNC 0x0100CFE8 vu_upload_microcode
// Uploads byteSize bytes of VU microcode to VU address vuAddr (in dwords)
// through DMA channel ch (0 VIF0 / 1 VIF1).  Builds a source chain on the
// stack: per <= 0x100 dwords a ref tag (0x30000000 | qwc) pointing at the code
// with vifcodes FLUSHE (0x10000000) and MPG (0x4A000000 | num << 16 | addr),
// then an end tag (0x70000000); sync.l, FlushCache(0), lock, start the chain,
// sync, unlock.  Quirks: the SPR bit computed for a scratchpad source is
// masked off again when the tag address is stored; no bound check on the
// 16-qword stack buffer (8 chunks + end for the 0x3DA8-byte VU1 program).
void vu_upload_microcode(int ch, u32 vuAddr, int byteSize, const void *code)
{
    u32 chain[0x100 / 4] __attribute__((aligned(16)));    /* sp+0x00 */
    u32 *t;
    u32 src = gs_dma_src_addr((u32)code);
    s32 left;

    left = ((byteSize + 15) >> 4) << 1;     /* dwords, rounded up to qwords */
    t = chain;
    while (left != 0) {
        s32 n = (left < 0x101) ? left : 0x100;
        t[0] = (u32)(n >> 1) | 0x30000000;                  /* ref, qwc = n/2 */
        t[1] = src & 0x0FFFFFFF;
        t[2] = 0x10000000;                                  /* FLUSHE */
        t[3] = ((n & 0xFF) << 16) | 0x4A000000 | vuAddr;    /* MPG */
        src += (u32)n << 3;
        vuAddr += n;
        left -= n;
        t += 4;
    }
    t[0] = 0x70000000;                                      /* end */
    t[1] = 0;
    t[2] = 0;
    t[3] = 0;
    SYNC_L();
    FlushCache(WRITEBACK_DCACHE);
    gfx_dma_lock(ch);
    dma_start_source_chain(ch, chain);
    gfx_dma_sync(ch);
    gfx_dma_unlock(ch);
}

// FUNC 0x0100D0F8 gfx_disable
// Render-state "disable" switch (109-entry jump table @0x0103E360; renamed
// from gfx_state_command since every case turns something off).  Caller
// F_01000B40 uses 5 (depth test), 4 (alpha test), 6 (alpha blend).
//   1: PATH3 mask on - if a packet is active emit MSKPATH3(1); path3Masked = 1
//   2/6/8/9: PRIM attr &= ~TME / ~ABE / ~FGE / ~AA1
//   3: TEST.DATE = 0     4: TEST.ATE = 0   (TEST dirty, both contexts)
//   5: TEST.ZTE = 1, ZTST = ALWAYS; ZBUF.ZMSK = 1; f1D1 = 0
//   7: DTHE = 0          0xC: COLCLAMP = 0
//   0xA,0xB,0xD,0xE,0x10,0x13,0x14: clear flag byte 6,7,0x22,0x24,0x27,0x28,0x2A
//   0xF / 0x11: periodAdjust / pal = 0, then gs_compute_display_period
//   0x12: flag29 = 0, flag00 = 1
//   0x64: lightFlags &= 0x7F   0x65..0x6C: light (what - 0x65) off, lightFlags |= 1
//   0, 0x15..0x63, >= 0x6D: nothing.
void gfx_disable(u32 what)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (what) {
    case 0x00:
        break;
    case 0x01:
        if (GS_PACKET_ACTIVE(ctx))
            gfx_mskpath3(1);
        ctx->path3Masked = 1;
        break;
    case 0x04:
        ctx->reg[GS_REG_TEST] &= ~1ull;                     /* ATE */
        ctx->dirty |= 0x00400040;
        break;
    case 0x03:
        ctx->reg[GS_REG_TEST] &= (u64)(s64)-0x4001;         /* DATE */
        ctx->dirty |= 0x00400040;
        break;
    case 0x05: {
        u64 test = ctx->reg[GS_REG_TEST];
        u64 zbuf = ctx->reg[GS_REG_ZBUF];
        u64 dirty = ctx->dirty;
        test &= 0xFFFFFFFFFFF9FFFFull;
        test |= 0x20000;                                    /* ZTST = ALWAYS */
        zbuf |= 1ull << 32;                                 /* ZMSK */
        test |= 0x10000;                                    /* ZTE */
        dirty |= 0x00480048;                                /* ZBUF|TEST */
        ctx->reg[GS_REG_ZBUF] = zbuf;
        ctx->dirty = dirty;
        ctx->reg[GS_REG_TEST] = test;
        ctx->f1D1 = 0;
        break;
    }
    case 0x06:
        ctx->primAttr &= 0xFFBF;                            /* ABE */
        break;
    case 0x08:
        ctx->primAttr &= 0xFFDF;                            /* FGE */
        break;
    case 0x09:
        ctx->primAttr &= 0xFF7F;                            /* AA1 */
        break;
    case 0x02:
        ctx->primAttr &= 0xFFEF;                            /* TME */
        break;
    case 0x07:
        ctx->gen[GS_GEN_DTHE] &= ~1ull;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_DTHE);            /* 1 << 34 */
        break;
    case 0x0A:
        ctx->flag06 = 0;
        break;
    case 0x0B:
        ctx->flag07 = 0;
        break;
    case 0x0C:
        ctx->gen[GS_GEN_COLCLAMP] &= ~1ull;
        ctx->dirty |= GS_DIRTY_GEN(GS_GEN_COLCLAMP);        /* 1 << 32 */
        break;
    case 0x0D:
        ctx->bgImmediate = 0;
        break;
    case 0x64:
        ctx->lightFlags &= 0x7F;
        break;
    case 0x65: case 0x66: case 0x67: case 0x68:
    case 0x69: case 0x6A: case 0x6B: case 0x6C:
        ctx->lights[what - 0x65].enable = 0;
        ctx->lightFlags |= 1;
        break;
    case 0x0E:
        ctx->flag24 = 0;
        break;
    case 0x0F:
        ctx->periodAdjust = 0;
        gs_compute_display_period(ctx);                     /* tail jump */
        return;
    case 0x11:
        ctx->pal = 0;
        gs_compute_display_period(ctx);                     /* tail jump */
        return;
    case 0x10:
        ctx->flag27 = 0;
        break;
    case 0x13:
        ctx->flag28 = 0;
        break;
    case 0x12:
        ctx->flag29 = 0;
        ctx->flag00 = 1;
        break;
    case 0x14:
        ctx->flag2A = 0;
        break;
    default:                                                /* 0, 0x15..0x63 */
        break;
    }
}

// FUNC 0x0100D330 gfx_mskpath3
// Switches to list 0 (VIF1), flushes dirty registers, then appends one qword:
// DMA cnt tag (qwc 0) + VIF FLUSH + MSKPATH3(mask & 0xFF -> bit 15).
void gfx_mskpath3(int mask)
{
    GsContext *ctx = GS_SPR_CTX;
    u8 m = (u8)mask;
    u32 *p;

    gfx_select_list(0);
    gs_flush_dirty_regs();
    p = ctx->cur;
    ctx->cur = p + 4;
    p[0] = 0x10000000;                      /* cnt, qwc 0 */
    p[1] = 0;
    p[2] = 0x11000000;                      /* FLUSH */
    p[3] = (m << 15) | 0x06000000;          /* MSKPATH3 */
}

// FUNC 0x0100D3A0 gfx_set_packet_buffer
// Sets the packet buffer: base, size, cur = segment-mapped start, end =
// segment-mapped buf + size (NULL stays NULL for both).  If the buffer is
// used uncached (ctx->uncachedBuf) the D-cache is written back first (sync.l +
// FlushCache(0), tail call).  Caller: F_01000B40 (0x01046180, 0x2000).
void gfx_set_packet_buffer(void *buf, u32 size)
{
    GsContext *ctx = GS_SPR_CTX;
    u8 *endp;
    u32 cur, end;

    ctx->bufBase = (u8 *)buf;
    ctx->bufSize = size;
    cur = 0;
    if (buf != NULL)
        cur = DMA_ADDR(buf) | ctx->addrBits;
    ctx->cur = (u32 *)cur;
    endp = (u8 *)buf + size;
    end = 0;
    if (endp != NULL)
        end = DMA_ADDR(endp) | ctx->addrBits;
    ctx->end = (u32 *)end;
    if (ctx->uncachedBuf != 0) {
        SYNC_L();
        FlushCache(WRITEBACK_DCACHE);       /* tail call */
    }
}

// FUNC 0x0100D410 gfx_get_packet_ptr
// Returns ctx->cur.  Unreferenced (inlined everywhere else).
u32 *gfx_get_packet_ptr(void)
{
    return GS_SPR_CTX->cur;
}

// FUNC 0x0100D420 gfx_get_packet_space
// Returns ctx->end - ctx->cur in bytes.  Unreferenced.
int gfx_get_packet_space(void)
{
    GsContext *ctx = GS_SPR_CTX;

    return (int)((u32)ctx->end - (u32)ctx->cur);
}

// FUNC 0x0100D438 gfx_begin_packet
// Opens a frame packet at ctx->cur (returns NULL if one is already open):
//   cur+0x00  GsFrameHeader {vif1Chain = cur+0x20, gifChain = cur+0x10}
//   cur+0x10  reserved first tag of the GIF list (tail1)
//   cur+0x20  VIF1 list: DMA cnt tag (qwc 1) + VIF BASE 0 + OFFSET 0x1B3,
//             then STCYCL 4/4, STMOD 0, STMASK, mask 0xC0C0C0C0
// cur = +0x40; all registers dirty; emit mask = general + context 1 for list 0
// (and saved as enableList0), list 1 mask 0; list 0 current; packetOpen = 1;
// assorted state bytes/counters reset.  Caller: F_01000B40.
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
    p[0] = 0x10000001;                      /* cnt, qwc 1 */
    p[1] = 0;
    p[2] = 0x03000000;                      /* BASE 0 */
    p[3] = 0x020001B3;                      /* OFFSET 0x1B3 */
    p[4] = 0x01000404;                      /* STCYCL CL 4 WL 4 */
    p[5] = 0x05000000;                      /* STMOD 0 */
    p[6] = 0x20000000;                      /* STMASK */
    p[7] = 0xC0C0C0C0;                      /* mask data */
    hdr->vif1Chain = p;
    ctx->fB7C = 0;
    ctx->tail0 = NULL;
    ctx->curList = 0;
    ctx->enableList0 = GS_ENABLE_GEN_CTX1;
    ctx->enableList1 = 0;
    ctx->flag00 = 1;
    ctx->dirty = GS_DIRTY_ALL;
    ctx->enable = GS_ENABLE_GEN_CTX1;
    ctx->stmask = 0xC0C0C0C0;
    ctx->flag02 = 0;
    ctx->halfHeight = 0;
    ctx->f1BD = 0;
    ctx->fB74 = 0;
    ctx->fB78 = 0;
    ctx->packetOpen = 1;
    ctx->cur = p + 8;
    return hdr;
}

// FUNC 0x0100D530 gfx_end_packet
// Closes the frame packet: closes an open sub-packet first; if a packet is
// open, writes DMA end tags (0x70000000) at the current list's write position
// and at the other list's reserved tail, advances cur past the first,
// packetOpen = 0, and writes back the D-cache unless the buffer is uncached.
void gfx_end_packet(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *a, *b, *cur;

    if (ctx->subPacketOpen)
        gfx_close_subpacket();
    if (ctx->packetOpen == 0)
        return;

    if (ctx->curList == 0) {
        a = ctx->cur;
        b = ctx->tail1;
        cur = a;
    } else {
        b = ctx->cur;
        a = ctx->tail0;
        cur = b;
    }
    a[0] = 0x70000000; a[1] = 0; a[2] = 0; a[3] = 0;       /* end */
    b[0] = 0x70000000; b[1] = 0; b[2] = 0; b[3] = 0;       /* end */
    ctx->cur = cur + 4;
    ctx->packetOpen = 0;
    if (ctx->uncachedBuf == 0) {
        SYNC_L();
        FlushCache(WRITEBACK_DCACHE);
    }
}

// FUNC 0x0100D5E0 gfx_close_subpacket
// Dead in practice: subPacketOpen is never set in this binary (and subHdr /
// frameHdr->save0/1 are never written).  Restores dirty = written (also into
// subHdr[1]), terminates both lists with DMA ret tags (0x60000000), clears
// subPacketOpen and, if a packet is open, reloads the list tails from
// frameHdr->save0/save1 (segment-mapped) and writes a next tag (0x20000000 ->
// new cur) into the current list's tail.
void gfx_close_subpacket(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *cur, *a, *b, *link;
    u32 t0, t1;
    u64 written;

    if (ctx->subPacketOpen == 0)
        return;

    written = ctx->written;
    ctx->dirty = written;
    ctx->subHdr[1] = written;
    cur = ctx->cur;
    if (ctx->curList == 0) {
        a = cur;
        b = ctx->tail1;
    } else {
        a = ctx->tail0;
        b = cur;
    }
    a[0] = 0x60000000; a[1] = 0; a[2] = 0; a[3] = 0;       /* ret */
    b[0] = 0x60000000; b[1] = 0; b[2] = 0; b[3] = 0;       /* ret */
    cur += 4;
    ctx->cur = cur;
    ctx->subPacketOpen = 0;
    if (ctx->packetOpen == 0)
        return;

    t1 = 0;
    if (ctx->frameHdr->save0 != NULL)
        t1 = DMA_ADDR(ctx->frameHdr->save0) | ctx->addrBits;
    ctx->tail0 = (u32 *)t1;
    t0 = 0;
    if (ctx->frameHdr->save1 != NULL)
        t0 = DMA_ADDR(ctx->frameHdr->save1) | ctx->addrBits;
    ctx->tail1 = (u32 *)t0;
    link = (u32 *)(ctx->curList ? t0 : t1);
    link[0] = 0x20000000;                                  /* next */
    link[1] = DMA_ADDR(cur);
    link[2] = 0;
    link[3] = 0;
}

// FUNC 0x0100D6E0 gfx_select_list
// Switches the packet writer to list `which` (0 VIF1 / 1 GIF); returns the
// previous list.  No-op if already current, or when switching to 1 while
// PATH3 is masked.  Without an active packet only the byte changes.
// Otherwise the current write position becomes the old list's reserved tail
// slot, a next tag (0x20000000 -> cur + 0x10) is written into the new list's
// tail, cur = cur + 0x10, and the emit masks are swapped (the old list's mask
// saved in enableList0/1, the new list's loaded into enable).
int gfx_select_list(int which)
{
    GsContext *ctx = GS_SPR_CTX;
    int prev = ctx->curList;
    u32 *cur;

    if (which == prev)
        return prev;
    if (ctx->path3Masked != 0 && which == 1)
        return prev;
    if (!GS_PACKET_ACTIVE(ctx)) {
        ctx->curList = (u8)which;
        return prev;
    }

    cur = ctx->cur;
    if (ctx->curList == 0) {
        ctx->tail0 = cur;
        cur += 4;
        ctx->enableList0 = ctx->enable;
        ctx->enable = ctx->enableList1;
        gs_put_next_tag(ctx->tail1, cur);
    } else {
        ctx->tail1 = cur;
        cur += 4;
        ctx->enableList1 = ctx->enable;
        ctx->enable = ctx->enableList0;
        gs_put_next_tag(ctx->tail0, cur);
    }
    ctx->cur = cur;
    ctx->curList = (u8)which;
    return prev;
}

// FUNC 0x0100D7B0 gfx_sync_pipeline
// Waits for the whole render path with forced recovery, one shared budget of
// 0x1000000 polls: gfx_dma_sync(1), gfx_dma_sync(2); VIF1_STAT & 0x1F000003
// (FQC/VPS) - over budget VIF1_FBRST = RST each poll; VU1 busy (VPU_STAT
// $vi29 & 0x100) - over budget FBRST($vi28) |= RS1 (0x200); GIF_STAT.APATH
// (0xC00) - over budget GIF_CTRL = RST, GIF_MODE = IMT.  Callers: F_01000B40,
// Tex1 upload F_0100BD38.
void gfx_sync_pipeline(void)
{
    s32 budget = 0x1000000;
    u32 v;

    gfx_dma_sync(1);
    gfx_dma_sync(2);

    if (VIF1_STAT & 0x1F000003) {
        do {
            if (--budget < 0)
                VIF1_FBRST = 1;
        } while (VIF1_STAT & 0x1F000003);
    }

    __asm__ __volatile__("cfc2 %0, $vi29" : "=r"(v));
    if (v & 0x100) {
        do {
            if (--budget < 0) {
                u32 fb;
                __asm__ __volatile__(
                    "cfc2   %0, $vi28\n\t"
                    "ori    %0, %0, 0x200\n\t"
                    "ctc2   %0, $vi28\n\t"
                    : "=&r"(fb) : : "memory");
            }
            __asm__ __volatile__("cfc2 %0, $vi29" : "=r"(v));
        } while (v & 0x100);
    }

    if (GIF_STAT & 0xC00) {
        do {
            if (--budget < 0) {
                GIF_CTRL = 1;
                GIF_MODE = 4;
            }
        } while (GIF_STAT & 0xC00);
    }
}

// FUNC 0x0100D8D0 gfx_wait_vblank_event
// Sleeps until the vblank counter changes: links a stack node {next, tid}
// into g_gfx.vsyncWaiters and samples the counter under DIntr/EIntr, then
// SleepThread while unchanged (the vblank handler wakes every listed thread),
// unlinks the node under DIntr/EIntr and CancelWakeupThread(tid).
void gfx_wait_vblank_event(void)
{
    GfxDriverState *g = &g_gfx;
    GfxWaitNode node;                       /* sp+0x00 */
    GfxWaitNode *p, *prev;
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
    for (p = g->vsyncWaiters; p != NULL; p = p->next) {
        if (p == &node) {
            if (prev != NULL)
                prev->next = node.next;
            else
                g->vsyncWaiters = node.next;
            break;
        }
        prev = p;
    }
    EIntr();
    CancelWakeupThread(tid);
}

// FUNC 0x0100D9A8 gfx_vsync
// Frame pacing.  n == 0: returns the vblank count (low 32 bits).
// n > 0: waits until at least n-1 vblanks passed since the last gfx_vsync,
// then always waits for one more vblank event; returns vblanks elapsed.
// n < 0: if at least -n vblanks already passed returns 1 without waiting,
// else waits until they have and returns 0.  (Unsigned 64-bit comparisons.)
// Both wait paths then set lastVsync = count and ctx->field = g.field when
// interlaced (else 0).  Callers: main, F_01000880, F_01000C88, ImageLoader
// F_010049C8/F_01004F58/F_01005458.
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
        u64 want = (u64)(s64)(n - 1);
        while (g->vblankCount - last < want)
            gfx_wait_vblank_event();
        gfx_wait_vblank_event();
        ret = (s32)(g->vblankCount - last);
    } else {
        u64 want;
        last = g->lastVsync;
        want = (u64)(s64)(s32)(0u - (u32)n);                /* negu */
        if (g->vblankCount - last >= want) {
            ret = 1;
        } else {
            while (g->vblankCount - last < want)
                gfx_wait_vblank_event();
            ret = 0;
        }
    }

    g->lastVsync = g->vblankCount;
    if (g->inter == 1)
        ctx->field = (s8)g->field;
    else
        ctx->field = 0;
    return ret;
}

// FUNC 0x0100DAD0 gfx_set_draw_buffer
// Sets the draw buffer (target A): FRAME_1 shadow bytes FBP = addr >> 11,
// FBW = (w + 63) >> 6, PSM; XYOFFSET_1 OFX/OFY = (0x800 - w/2) << 4 /
// (0x800 - h/2) << 4 (hword stores into the shadows), FRAME|XYOFFSET dirty in
// both contexts, halfHeight = 0, tgtA = {addr >> 6, fbw, psm, w, h, xoff,
// yoff, xoff + w*16, yoff + h*16}; then gfx_set_draw_area(0, 0, w, h) (jump).
// Caller: F_01000B40 (fb, PSMCT24, 640, 448).
void gfx_set_draw_buffer(u32 fbWordAddr, int psm, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 ox, oy, fbw;

    ox = (0x800 - (w >> 1)) << 4;
    oy = (0x800 - (h >> 1)) << 4;
    fbw = (w + 0x3F) >> 6;
    ctx->tgtA.width  = w;
    ctx->tgtA.height = h;
    ctx->tgtA.xoff = (u16)ox;
    ctx->tgtA.yoff = (u16)oy;
    ctx->tgtA.xend = (u16)(ox + (w << 4));
    ctx->tgtA.yend = (u16)(oy + (h << 4));
    ctx->tgtA.base = (u16)(fbWordAddr >> 6);
    ctx->tgtA.fbw  = (u8)fbw;
    ctx->tgtA.psm  = (u8)psm;
    ((u16a *)&ctx->reg[GS_REG_FRAME])[0] = (u16)(fbWordAddr >> 11); /* FBP */
    ((u8 *)&ctx->reg[GS_REG_FRAME])[2] = (u8)fbw;                   /* FBW */
    ((u8 *)&ctx->reg[GS_REG_FRAME])[3] = (u8)psm;                   /* PSM */
    ((u16a *)&ctx->reg[GS_REG_XYOFFSET])[0] = (u16)ox;              /* OFX */
    ((u16a *)&ctx->reg[GS_REG_XYOFFSET])[2] = (u16)oy;              /* OFY */
    ctx->dirty |= 0x00030003;               /* FRAME|XYOFFSET, both contexts */
    ctx->halfHeight = 0;
    gfx_set_draw_area(0, 0, w, h);          /* j 0x0100DB88 */
}

// FUNC 0x0100DB88 gfx_set_draw_area
// Stores the draw window (y and h halved - arithmetic shift - when
// halfHeight is set) and tail-calls gs_update_scissor_and_viewport.  Only
// reached by the `j` at the end of gfx_set_draw_buffer.
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

// FUNC 0x0100DBB8 gs_update_scissor_and_viewport
// SCISSOR shadow = {winX, winX+winW-1, winY, winY+winH-1} (11-bit fields,
// only those bits replaced), SCISSOR dirty in both contexts; viewport floats
// scale (winW/2, -winH/2) and centre ((xoff + winX*16 + winW*8) / 16,
// (yoff + winY*16 + winH*8) / 16) for the VU1 microcode; flag02 = 0.
void gs_update_scissor_and_viewport(GsContext *ctx)
{
    GsScissor *sc = (GsScissor *)&ctx->reg[GS_REG_SCISSOR];
    s32 x = ctx->winX, y = ctx->winY, w = ctx->winW, h = ctx->winH;

    sc->SCAX0 = x;
    sc->SCAY0 = y;
    sc->SCAX1 = x + w - 1;
    sc->SCAY1 = y + h - 1;
    ctx->dirty |= 0x00100010;
    ctx->viewScaleX  = (f32)w * 0.5f;
    ctx->viewScaleY  = (f32)(-h) * 0.5f;
    ctx->viewCenterX = (f32)((s32)ctx->tgtA.xoff + (x << 4) + (w << 3)) * 0.0625f;
    ctx->viewCenterY = (f32)((s32)ctx->tgtA.yoff + (y << 4) + (h << 3)) * 0.0625f;
    ctx->flag02 = 0;
}

// FUNC 0x0100DD00 gs_set_clear_color
// ctx->clearColor = rgba (colour of gs_clear_buffers bit 0).  Caller F_01000B40.
void gs_set_clear_color(u32 rgba)
{
    GS_SPR_CTX->clearColor = rgba;
}

// FUNC 0x0100DD10 gs_build_clear_packet
// Appends a buffer clear of target `tgt` to the packet:
//   DMA cnt tag + VIF FLUSH + DIRECT n
//   GIFtag PACKED A+D x4: TEST_1 = test, ZBUF_1 = ZBUF shadow with ZMSK =
//     ctx->clearZMask, PRIM = SPRITE, RGBAQ = {rgba, Q 1.0}
//   GIFtag REGLIST EOP {XYZ3, XYZ2} x k: 64-pixel-wide (0x400 subpixel) strips
//     from tgt->xoff until (x & 0xFFFF) >= tgt->xend, full height, depth z
// ZBUF_1 and TEST_1 are marked dirty (ctx 1) so the normal state returns.
void gs_build_clear_packet(GsContext *ctx, u64 test, u32 rgba, u32 z,
                           const GsRenderTarget *tgt)
{
    u32 *p = ctx->cur;                      /* DMA tag / VIF codes */
    u32 *q = p + 4;                         /* write pointer        */
    u32 *gif = q;                           /* A+D GIF tag          */
    u32 *regl;                              /* REGLIST GIF tag      */
    u32 xy0, xy1, xend, n, cnt;

    GS_PUT_AD(q, 0x1000000000000004ull, 0xE);               /* NLOOP 4, NREG 1, A+D */
    GS_PUT_AD(q, test, 0x47);                               /* TEST_1 */
    GS_PUT_AD(q, (ctx->reg[GS_REG_ZBUF] & 0xFFFFFFFEFFFFFFFFull)
                 | ((u64)ctx->clearZMask << 32), 0x4E);     /* ZBUF_1 */
    GS_PUT_AD(q, 6, 0);                                     /* PRIM = SPRITE */
    do {                                                    /* RGBAQ, Q = 1.0f */
        q[0] = rgba;
        q[1] = 0x3F800000;
        PKT64(q, 2) = 1;
        q += 4;
    } while (0);

    xy0  = *(const u32a *)&tgt->xoff;                       /* xoff | yoff << 16 */
    xy1  = ((u32)tgt->xoff + 0x400) | ((u32)tgt->yend << 16);
    xend = tgt->xend;
    regl = q;
    q += 4;
    do {
        GS_PUT_XYZ(q, xy0, z);
        GS_PUT_XYZ(q, xy1, z);
        xy0 += 0x400;
        xy1 += 0x400;
    } while ((xy0 & 0xFFFF) < xend);

    n = (u32)((u8 *)q - (u8 *)gif) >> 4;
    if (n != 0) {
        gs_put_cnt_direct(p, n);
        ctx->cur = p + 4 + n * 4;
    }
    cnt = ((u32)((u8 *)q - (u8 *)regl) >> 4) - 1;
    do {                                                    /* EOP, REGLIST, NREG 2 */
        PKT64(regl, 0) = (u64)(u32)(cnt | 0x8000) | 0x2400000000000000ull;
        PKT64(regl, 2) = 0x5D;                              /* XYZ3, XYZ2 */
    } while (0);
    ctx->dirty |= 0x48;                                     /* ZBUF_1|TEST_1 */
}

// FUNC 0x0100DE80 gs_clear_buffers
// Flushes dirty registers, then: mask & 3 clears target A with
// depth = (int)(clearDepth * depthScale) and colour ctx->clearColor, TEST =
//   bit0 only -> 0 (colour, no depth test); bit0|bit1 -> 0x30000 (ZTE,
//   ALWAYS); bit1 only -> 0x32001 (alpha NEVER + AFAIL ZB_ONLY = depth only);
// mask & 4 re-points FRAME/SCISSOR/XYOFFSET_1 to target B
// (gs_emit_render_target_regs) and clears it with clearColorB, TEST 0, Z 0.
// Caller F_01000B40 (mask 1).
void gs_clear_buffers(int mask)
{
    GsContext *ctx = GS_SPR_CTX;

    if (mask == 0)
        return;
    gs_flush_dirty_regs();
    if (mask & 3) {
        u32 z = (u32)(s32)(ctx->clearDepth * ctx->depthScale);   /* cvt.w.s */
        u64 test;
        if ((mask & 1) == 0)
            test = 0x32001;
        else
            test = (mask & 2) ? 0x30000 : 0;
        gs_build_clear_packet(ctx, test, ctx->clearColor, z, &ctx->tgtA);
    }
    if (mask & 4) {
        gs_emit_render_target_regs(ctx);
        gs_build_clear_packet(ctx, 0, ctx->clearColorB, 0, &ctx->tgtB);
    }
}

// FUNC 0x0100DF40 gs_emit_render_target_regs
// Appends DMA cnt(4) + FLUSH + DIRECT 4 + GIFtag(A+D x3, EOP) setting
// FRAME_1 = target B {base >> 5, fbw, psm}, SCISSOR_1 = the current scissor
// rescaled from target A to target B size (x0*wB/wA, (x1+1)*wB/wA - 1, same
// for y; signed divides, break 7 if tgtA.width == 0 - the height divisor is
// not checked) and XYOFFSET_1 = target B offsets; marks FRAME/XYOFFSET/
// SCISSOR (ctx 1) dirty so the next flush restores target A.
void gs_emit_render_target_regs(GsContext *ctx)
{
    GsScissor *sc = (GsScissor *)&ctx->reg[GS_REG_SCISSOR];
    s32 x0, x1, y0, y1;
    u32 frame;
    u32 *p;

    p = ctx->cur;
    x1 = ((s32)sc->SCAX1 * ctx->tgtB.width + ctx->tgtB.width) / ctx->tgtA.width - 1;
    x0 = (s32)sc->SCAX0 * ctx->tgtB.width / ctx->tgtA.width;
    y1 = ((s32)sc->SCAY1 * ctx->tgtB.height + ctx->tgtB.height) / ctx->tgtA.height - 1;
    frame = (ctx->tgtB.psm << 24) | (ctx->tgtB.fbw << 16) | (ctx->tgtB.base >> 5);
    y0 = (s32)sc->SCAY0 * ctx->tgtB.height / ctx->tgtA.height;
    p[0] = 0x10000004;                          /* DMA cnt, qwc 4 */
    p[1] = 0;
    p[2] = 0x11000000;                          /* VIF FLUSH */
    p[3] = 0x50000004;                          /* VIF DIRECT 4 */
    ctx->cur = p + 20;
    /* The statement order above is the retail schedule; every store is to a
       distinct packet word, so the order is behaviour-neutral.  Below, the
       GIF packet goes through GsPktDword (union accesses: alias set 0 under
       egcs, so the tgtB.yoff load stays behind the xoff store as in the
       binary), one do { } while (0) block per register (the blocks are
       scheduling barriers: the tag constants are not hoisted). */
    do {
        PKT64U(p, 4)  = 0x1000000000008003ull;  /* EOP, NLOOP 3, NREG 1 */
        PKT64U(p, 6)  = 0xE;                    /* A+D */
    } while (0);
    do {
        PKT64U(p, 8)  = (s32)frame;
        PKT64U(p, 10) = 0x4C;                   /* FRAME_1 */
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(p + 12);
        q[0].w[0] = (x1 << 16) | x0;
        q[0].w[1] = (y1 << 16) | y0;
        q[1].d = 0x40;                          /* SCISSOR_1 */
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(p + 16);
        q[0].w[0] = ctx->tgtB.xoff;
        q[0].w[1] = ctx->tgtB.yoff;
        q[1].d = 0x18;                          /* XYOFFSET_1 */
    } while (0);
    ctx->dirty |= 0x13;                         /* FRAME|XYOFFSET|SCISSOR_1 */
}

// FUNC 0x0100E0A0 gs_build_image_transfer_packet
// Writes a 6-qword host->local transfer header at pkt: GIFtag(A+D x4)
// BITBLTBUF = dstBitblt[0] << 32 (DBP/DBW/DPSM), TRXPOS = (dy << 16 | dx) << 32,
// TRXREG = w | h << 32, TRXDIR = 0, then GIFtag IMAGE EOP with the qword
// count.  qwc = w*h scaled by the DPSM (byte 3 of dstBitblt[0], jump table
// @0x0103E520): CT32 *4, CT24 *3, CT16/16S *2 -> (+15)>>4; T8/T8H (+15)>>4;
// T4/T4HH/T4HL (+31)>>5; any other PSM: qwc = w*h unscaled (quirk).
// Returns qwc.  Caller: Tex1 upload F_0100BD38 (then gfx_dma_send on GIF).
int gs_build_image_transfer_packet(u32 *pkt, const u32 *dstBitblt,
                                   int dx, int dy, int w, int h)
{
    u32 psm = ((const u8 *)dstBitblt)[3];
    s32 qwc = w * h;

    if (psm < 0x2D) {
        switch (psm) {
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
        PKT64U(pkt, 0)  = 0x1000000000000004ull;                /* NLOOP 4, NREG 1 */
        PKT64U(pkt, 2)  = 0xE;                                  /* A+D */
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 4);
        q[0].w[0] = 0;
        q[0].w[1] = dstBitblt[0];                               /* DBP/DBW/DPSM */
        q[1].d = 0x50;                                          /* BITBLTBUF */
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 8);
        q[0].w[0] = 0;
        q[0].w[1] = ((u32)dy << 16) | (u32)dx;                  /* DSAY/DSAX */
        q[1].d = 0x51;                                          /* TRXPOS */
    } while (0);
    do {
        GsPktDword *q = (GsPktDword *)(pkt + 12);
        q[0].w[0] = (u32)w;
        q[0].w[1] = (u32)h;
        q[1].d = 0x52;                                          /* TRXREG */
    } while (0);
    do {
        PKT64U(pkt, 16) = 0;
        PKT64U(pkt, 18) = 0x53;                                 /* TRXDIR */
    } while (0);
    do {
        PKT64U(pkt, 20) = (u64)(s64)(s32)((u32)qwc | 0x8000) | 0x0800000000000000ull; /* EOP, IMAGE */
        PKT64U(pkt, 22) = 0;
    } while (0);
    return qwc;
}

// FUNC 0x0100E198 float_to_gs_param_encode
// Encodes a non-negative float (e.g. a specular / spot exponent) for the VU1
// lighting code: 11 greedy bits of v in steps 64, 32, ..., 1/16
// (bits = integer part << 4 | 4 fraction bits), top = position (10..0) of the
// highest set step (0 if none).  Returns (bits >> 4) | bitrev4(bits & 0xF) << 6
// | top << 16 (integer part and reversed fraction overlap at bit 6 for v >= 64,
// as in the binary).  float_pow_encoded decodes this format.
u32 float_to_gs_param_encode(float v)
{
    s32 bits = 0;
    s32 top = 0;
    f32 step = 64.0f;
    s32 i;
    s32 frac, rev;

    for (i = 10; i >= 0; i--) {
        bits <<= 1;
        if (step <= v) {                    /* c.ole.s */
            v -= step;
            if (top < i)
                top = i;
            bits |= 1;
        }
        step *= 0.5f;
    }
    frac = bits & 0xF;
    bits >>= 4;
    rev = 0;
    for (i = 0; i < 4; i++) {
        rev <<= 1;
        rev |= (frac >> i) & 1;
    }
    bits |= rev << 6;
    return bits | (top << 16);
}

// FUNC 0x0100E228 float_pow_encoded
// Dead code (no references): x ** e with e in float_to_gs_param_encode's
// format.  For top = enc >> 16 >= 4 multiplies in x^(2^k) for the top-3
// integer bits (square-and-multiply), then shifts enc so the reversed
// fraction nibble is at bit 0 (total shift 6 when top >= 4, else 9 - top -
// wrong for exponents below 0.5, and srav uses only the low 5 bits of the
// count) and multiplies in x^(1/2), x^(1/4), ... for 5 bits via repeated EE
// sqrt.s.  Renamed from float_pow_fixed44.
float float_pow_encoded(float x, int enc)
{
    f32 r = 1.0f;
    f32 p = x;
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

// FUNC 0x0100E2A0 gs_draw_textured_sprite
// Flushes dirty registers and appends DMA cnt(4) + FLUSH + DIRECT 4 +
// GIFtag REGLIST NREG 6 EOP {PRIM, RGBAQ, UV, XYZF3, UV, XYZF2}:
// PRIM = primAttr | 0x116 (SPRITE, TME, FST), RGBAQ = {bgBlend, Q 1.0},
// UV = (u | v << 16) << 4 + 0x80008 (half-texel), XY = ((x + winX) << 4) +
// tgtA.xoff and (y << (halfHeight ? 3 : 4)) + (winY << 4) + tgtA.yoff,
// XYZF high word = spriteZF.  Caller: F_01000B40 (the warning-screen image).
void gs_draw_textured_sprite(int x0, int y0, int x1, int y1,
                             int u0, int v0, int u1, int v1)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 sh = ctx->halfHeight ? 3 : 4;
    u64 uv0, uv1;
    u32 xy0, xy1;

    {                                       /* window origin and draw offset, read once */
        s32 wx = ctx->winX;
        s32 wy = ctx->winY << 4;
        s32 xo = ctx->tgtA.xoff;
        s32 yo = ctx->tgtA.yoff;
        xy0 = ((u32)((y0 << sh) + wy + yo) << 16) | (u32)(((x0 + wx) << 4) + xo);
        xy1 = ((u32)((y1 << sh) + wy + yo) << 16) | (u32)(((x1 + wx) << 4) + xo);
    }
    uv0 = (u64)(s64)(s32)(((u32)v0 << 16) | ((u32)u0 & 0xFFFF)) << 4;
    uv1 = (u64)(s64)(s32)(((u32)v1 << 16) | ((u32)u1 & 0xFFFF)) << 4;

    gs_flush_dirty_regs();
    {
        GsContext *c2 = GS_SPR_CTX;         /* re-read after the call */
        u32 *p = c2->cur;
        u32 *q;
        int prim;
        u32 rgba, zf;
        u32 tc = 0x10000004;                /* DMA cnt, QWC 4 (the three tag words are */
        u32 tf = 0x11000000;                /*  set up first: their constants are the  */
        u32 td = 0x50000004;                /*  first loads of the block in the binary) */

        c2->cur = p + 20;
        q = p + 4;
        prim = c2->primAttr | 0x116;        /* SPRITE, TME, FST */
        rgba = c2->bgBlend;
        zf = c2->spriteZF;
        uv0 += 0x80008;                     /* half-texel */
        uv1 += 0x80008;
        p[0] = tc;
        p[2] = tf;
        p[3] = td;
        p[1] = 0;
        do {                                /* NLOOP 1, EOP, REGLIST, NREG 6 */
            PKT64(q, 0) = 0x6400000000008001ull;
            PKT64(q, 2) = 0x43C310;         /* PRIM RGBAQ UV XYZF3 UV XYZF2 */
        } while (0);
        PKT64(q, 4) = prim;
        q[6] = rgba;
        q[7] = 0x3F800000;                  /* Q = 1.0f */
        PKT64(q, 8) = uv0;
        q[10] = xy0;
        q[11] = zf;
        PKT64(q, 12) = uv1;
        q[14] = xy1;
        q[15] = zf;
    }
}

// FUNC 0x0100E404 vu0_load_identity_regs
// VU0 macro code: vf28..vf31 = identity matrix rows ((1,0,0,0) .. (0,0,0,1))
// via vsub / vmove / vmr32 / vaddw on vf0; then sets GS_CTX->flag00 = 1.
// Renamed from gs_init_vu0_constants.  Caller: gfx_init_vu0_constants.
// Hand-written assembly in the original: it starts at 0x0100E404, 4 bytes past
// an 8-byte boundary, straight after gs_draw_textured_sprite's last
// instruction.  egcs starts every function it compiles 8-byte aligned
// (.p2align 3, a padding nop where needed; every other function of the unit
// is), so this one was a top-level asm block at this point of the original
// file, without an alignment directive (tools/layout.py showed the object's
// .text drifting by 4 + 4 bytes here).  It also holds the context pointer in
// $v0 and the 1 in $v1, the opposite of egcs's choice.  The retail compiler
// build assembles the retail instructions at the same point; the modern
// build keeps the equivalent C.
//   lui $v0,0x7000; ori $v0,$v0,0x3000; vsub.xyzw vf29,vf0,vf0;
//   vsub.xyzw vf28,vf0,vf0; vmove.xyzw vf31,vf0; vmr32.xyzw vf30,vf0;
//   vaddw.y vf29,vf0,vf0w; vaddw.x vf28,vf0,vf0w; addiu $v1,$0,1; jr $ra;
//   sb $v1,0($v0)
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
__asm__("\t.text\n"
        "\t.globl\tvu0_load_identity_regs\n"
        "\t.ent\tvu0_load_identity_regs\n"
        "\t.type\tvu0_load_identity_regs,@function\n"
        "vu0_load_identity_regs:\n"
        "\t.set\tnoreorder\n"
        "\tlui\t$2, 0x7000\n"
        "\tori\t$2, $2, 0x3000\n"
        "\tvsub.xyzw\t$vf29, $vf0, $vf0\n"
        "\tvsub.xyzw\t$vf28, $vf0, $vf0\n"
        "\tvmove.xyzw\t$vf31, $vf0\n"
        "\tvmr32.xyzw\t$vf30, $vf0\n"
        "\tvaddw.y\t$vf29, $vf0, $vf0w\n"
        "\tvaddw.x\t$vf28, $vf0, $vf0w\n"
        "\taddiu\t$3, $0, 1\n"
        "\tjr\t$31\n"
        "\tsb\t$3, 0($2)\n"
        "\t.set\treorder\n"
        "\t.end\tvu0_load_identity_regs\n"
        "\t.size\tvu0_load_identity_regs, . - vu0_load_identity_regs\n"
        "\t.previous\n");
#else
void vu0_load_identity_regs(void)
{
    GsContext *ctx = GS_SPR_CTX;

    __asm__ __volatile__(
        "vsub.xyzw  $vf29, $vf0, $vf0\n\t"
        "vsub.xyzw  $vf28, $vf0, $vf0\n\t"
        "vmove.xyzw $vf31, $vf0\n\t"
        "vmr32.xyzw $vf30, $vf0\n\t"
        "vaddw.y    $vf29, $vf0, $vf0w\n\t"
        "vaddw.x    $vf28, $vf0, $vf0w\n\t"
        : : : "memory");
    ctx->flag00 = 1;
}
#endif

// FUNC 0x0100E430 gs_get_display_state
// 0 unless frame mode (g_gfx.ffmd == 1); then 9 if ctx->f1BD, else 4 if the
// current field byte is non-zero, else 3.  Caller F_01000C88 waits one more
// vsync when it returns 4.
int gs_get_display_state(void)
{
    GsContext *ctx = GS_SPR_CTX;

    if (g_gfx.ffmd != 1)
        return 0;
    if (ctx->f1BD)
        return 9;
    return (ctx->field == 0) ? 3 : 4;
}

// FUNC 0x0100E478 gs_set_bg_color_rgb
// Packs (r, g, b, alpha 1.0) with scale 255.0 (pack_rgba_floats_to_u32 gets
// f15 = 1.0 and f16 = 255.0 - the caller's 4th float is not used) and
// tail-jumps gs_set_bg_color.  Caller F_01000B40 (brightness fade: r = g = b).
void gs_set_bg_color_rgb(float r, float g, float b)
{
    gs_set_bg_color(pack_rgba_floats_to_u32(r, g, b, 1.0f, 255.0f));
}

static s8   s_vu1CodeUnscrambled = 0;           /* @0x01041E30 .sdata */
static u32  s_lcgState = 0x32277070;            /* @0x01041E34 .sdata */

// FUNC 0x0100E4A8 lcg_rand
// state = state * 0x5D588B65 + 1 (32-bit, state @0x01041E34 = 0x32277070
// initially); returns the new state.
int lcg_rand(void)
{
    /* 64-bit multiply (multu, hi:lo combined, daddiu) truncated back to 32
     * bits - the constant was an unsigned long (64-bit on the EE).  Same
     * low 32 bits as a 32-bit multiply. */
    s_lcgState = s_lcgState * 0x5D588B65ull + 1;
    return (s32)s_lcgState;
}

// FUNC 0x0100E4E8 xor_unscramble_with_lcg
// Once only (flag @0x01041E30): XORs len bytes at buf with the low byte of
// successive lcg_rand() values.  gfx_init applies it to the VU1 microcode
// (0x01034730, 0x3DA8 bytes) before uploading it - verified: the result is
// valid VU code (filler pairs: upper NOP 0x000002FF / lower 0x80000035 =
// IOR vi0,vi0,vi0; ends with an upper NOP[E] 0x400002FF).
void xor_unscramble_with_lcg(u8 *buf, int len)
{
    int i;

    if (s_vu1CodeUnscrambled == 0) {
        for (i = 0; i < len; i++) {
            *buf ^= lcg_rand();             /* low byte of the LCG value */
            buf++;
        }
        s_vu1CodeUnscrambled = 1;
    }
}

// FUNC 0x0100E548 gs_flush_all_regs
// Dead (no references): marks every register dirty and tail-jumps
// gs_flush_dirty_regs.
void gs_flush_all_regs(void)
{
    GS_SPR_CTX->dirty = GS_DIRTY_ALL;
    gs_flush_dirty_regs();
}

// FUNC 0x0100E570 gs_flush_dirty_regs
// Without an active packet just forgets the dirty set.  Otherwise collects
// the dirty & enabled shadows as A+D pairs at cur+0x20 and, if any, wraps
// them: DMA cnt (n+1) + VIF FLUSH + DIRECT (n+1) + GIFtag (NLOOP n, EOP,
// PACKED, NREG 1, A+D); cur += 0x20 + n*16.
void gs_flush_dirty_regs(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p;
    int n;

    if (!GS_PACKET_ACTIVE(ctx)) {
        ctx->dirty = 0;
        return;
    }
    p = ctx->cur;
    n = gs_collect_dirty_regs(ctx, (u64 *)(p + 8));
    if (n == 0)
        return;
    gs_put_cnt_direct(p, n + 1);
    do {                                                   /* GIFtag NLOOP n, EOP, A+D */
        PKT64(p, 4) = (u64)(u32)((u32)n | 0x8000) | 0x1000000000000000ull;
        PKT64(p, 6) = 0xE;
    } while (0);
    ctx->cur = (u32 *)((u128 *)p + (n + 2));
}

// FUNC 0x0100E630 intc_vblank_handler
// INTC_VBLANK_S handler: reads and zeroes COP0 PCR0/PCR1 and accumulates them
// (sign-extended) into perfCount0/1, samples GS CSR.FIELD (bit 13), increments
// the 64-bit vblank counter, calls vsyncCallback(count) if set, then
// iWakeupThread for every thread in the vsync waiter list and in the three
// DMA channel waiter lists; sync.l; ei; returns 1.
int intc_vblank_handler(int cause)
{
    GfxDriverState *g = &g_gfx;
    void (*cb)(int);
    u64 pc0, pc1;                           /* mfpc: the counter sign-extended to 64 bits;
                                             * the whole register is accumulated */
    int i;

    __asm__ __volatile__(
        "mfpc   %0, 0\n\t"
        "mtpc   $0, 0\n\t"
        "sync.p\n\t"
        "mfpc   %1, 1\n\t"
        "mtpc   $0, 1\n\t"
        "sync.p\n\t"
        : "=&r"(pc0), "=&r"(pc1) : : "memory");

    cb = g->vsyncCallback;
    g->perfCount0 += pc0;
    g->perfCount1 += pc1;
    g->field = (u8)((GS_REG_CSR >> 13) & 1);
    g_gfx.vblankCount++;
    if (cb != NULL)
        cb((s32)g_gfx.vblankCount);

    GS_WAKE_WAITERS(g->vsyncWaiters);
    for (i = 0; i < 3; i++)
        GS_WAKE_WAITERS(g->chan[i].waiters);

    ExitHandler();
    return 1;
}

// FUNC 0x0100E770 intc_vif1_handler
// INTC_VIF1 handler (VIF1 i-bit stall).  VIF1_STAT bit 11 (INT) clear: just
// FBRST.STC.  Otherwise the NUM field of VIF1_CODE carries markers:
//   0x40000: --pathSync18, resume (STC) unless it went negative;
//   else 0x10000: --pathSync16, resume unless negative; no marker: resume;
//   then 0x20000: ++pathSync17 and when it reaches 0 restart GIF DMA
//   (D2_QWC = 0, D2_CHCR |= STR).
// Each exit: sync.l; ei; return 1.
int intc_vif1_handler(int cause)
{
    GfxDriverState *g = &g_gfx;
    u32 code;

    if (((VIF1_STAT >> 11) & 1) == 0) {
        GS_VIF1_RESUME();
        ExitHandler();
        return 1;
    }
    code = VIF1_CODE;
    if (code & 0x40000) {
        g->pathSync18--;
        if (g->pathSync18 >= 0)
            GS_VIF1_RESUME();
        ExitHandler();
        return 1;
    }
    if (code & 0x10000) {
        g->pathSync16--;
        if (g->pathSync16 >= 0)
            GS_VIF1_RESUME();
    } else {
        GS_VIF1_RESUME();
    }
    if (code & 0x20000) {
        g->pathSync17++;
        if ((u8)g->pathSync17 == 0)
            GS_GIF_RESTART();
    }
    ExitHandler();
    return 1;
}

// FUNC 0x0100E888 intc_gs_finish_handler
// INTC_GS handler: clears CSR.FINISH, ++finishCount; the second event of a
// readback (together with the VIF1 DMA marker) starts gs_readback_begin.
int intc_gs_finish_handler(int cause)
{
    GfxDriverState *g = &g_gfx;

    GS_REG_CSR = 2;
    g->finishCount++;
    if (g->finishCount == 2)
        gs_readback_begin(g);
    ExitHandler();
    return 1;
}

// FUNC 0x0100E8E8 dmac_channel_handler
// DMAC handler for channels 0-2.  In chain mode (CHCR.MOD == 1):
//   tag ID 7 (end), or 6 (ret) with ASP != 0 -> completion;
//   last tag without IRQ bit (CHCR bit 31 clear) -> tagIrq = 1, completion;
//   IRQ-flagged tag: ch 1 -> ++finishCount, == 2 starts gs_readback_begin;
//     ch 2 -> 0x40000 in the tag: ++pathSync18 == 0 resumes VIF1 (STC);
//     otherwise 0x10000: --pathSync17, restart GIF unless negative (no
//     0x10000: restart GIF); then 0x20000: ++pathSync16 == 0 resumes VIF1;
//     ch 0 -> nothing.  No completion for IRQ-flagged tags.
// Normal mode: if finishCount == 2 the readback is over -> finishCount = 0,
//   gs_readback_end; else completion.
// Completion: busy = 0 and iWakeupThread every waiter of the channel.
// Every exit: sync.l; ei; return 1.
int dmac_channel_handler(int ch)
{
    GfxDriverState *g = &g_gfx;
    u32 chcr = s_gsDmaChan[ch]->chcr;

    if (((chcr >> 2) & 3) == 1) {                           /* chain mode */
        switch ((chcr >> 28) & 7) {
        case 7:
            goto complete;
        case 6:
            if (((chcr >> 4) & 3) != 0)
                goto complete;
            break;
        }
        if ((s32)chcr >= 0) {
            g->chan[ch].tagIrq = 1;
            goto complete;
        }
        switch (ch) {
        case 1:
            g->finishCount++;
            if (g->finishCount == 2)
                gs_readback_begin(g);
            break;
        case 2:
            if (chcr & 0x40000) {
                g->pathSync18++;
                if ((u8)g->pathSync18 == 0)
                    GS_VIF1_RESUME();
            } else {
                if (!(chcr & 0x10000) || (--g->pathSync17, g->pathSync17 >= 0))
                    GS_GIF_RESTART();
                if (chcr & 0x20000) {
                    g->pathSync16++;
                    if ((u8)g->pathSync16 == 0)
                        GS_VIF1_RESUME();
                }
            }
            break;
        }
        ExitHandler();
        return 1;
    }

    if (g->finishCount == 2) {
        g->finishCount = 0;
        gs_readback_end(g);
        ExitHandler();
        return 1;
    }

complete:
    g->chan[ch].busy = 0;
    GS_WAKE_WAITERS(g->chan[ch].waiters);
    ExitHandler();
    return 1;
}

// FUNC 0x0100EB00 gs_readback_begin
// Starts a GS->EE image download: the record at D1_TADR (read through the
// uncached segment) gives qwc and destination; remembered in g->readback.
// VIF1_STAT.FDR = 1 (0x800000), GS BUSDIR = 1, D1 QWC/MADR set and
// CHCR = (CHCR & 0xFFFFFEF0) | 0x100 (to memory, normal, start); sync.l.
void gs_readback_begin(GfxDriverState *g)
{
    u32 chcr = D1->chcr;
    GsReadbackRec *rec = (GsReadbackRec *)((D1->tadr & 0x0FFFFFFF) | 0x20000000);
    u16 qwc = rec->h.qwc;               /* read before the readback store */
    u32 madr;

    g->readback = rec;
    madr = rec->madr;
    VIF1_STAT = 0x800000;
    GS_REG_BUSDIR = 1;
    D1->qwc  = qwc;
    D1->madr = madr;
    D1->chcr = (chcr & 0xFFFFFEF0) | 0x100;
    SYNC_L();
}

// FUNC 0x0100EB78 gs_readback_end
// Finishes the download: record callback(madr, arg) if set, BUSDIR = 0,
// VIF1_STAT = 0 (FDR off), then resumes the VIF1 source chain after the
// record (QWC 0, TADR = rec + 0x10, CHCR = (CHCR & 0xFEF0) | 0x10000105) and
// restarts GIF DMA (QWC 0, CHCR |= STR); sync.l after each.
void gs_readback_end(GfxDriverState *g)
{
    GsReadbackRec *rec = g->readback;
    u32 chcr, next;

    if (rec->callback != NULL)
        rec->callback(rec->madr, rec->arg);
    next = ((u32)rec & 0x0FFFFFFF) + 0x10;      /* the chain resumes after the record */
    chcr = D1->chcr;
    GS_REG_BUSDIR = 0;
    VIF1_STAT = 0;
    D1->qwc  = 0;
    D1->tadr = next;
    chcr &= 0xFEF0;
    chcr |= 0x10000105;
    D1->chcr = chcr;
    SYNC_L();
    GS_GIF_RESTART();
}

// FUNC 0x0100EC20 dma_start_source_chain
// gfx_dma_sync(ch), busy = 1, QWC = 0, TADR = tadr (0x0FFFFFFF-masked, SPR
// bit for 0x7xxxxxxx), CHCR = (CHCR & 0xFEC0) | 0x10000105 (from memory,
// chain, start; the TAG field gets 0x1000); sync.l.  Callers:
// gfx_kick_frame_lists, vu_upload_microcode.
void dma_start_source_chain(int ch, const void *tadr)
{
    EeDmaChanRegs *d = s_gsDmaChan[ch];

    gfx_dma_sync(ch);
    g_gfx.chan[ch].busy = 1;
    d->qwc = 0;
    d->tadr = gs_dma_src_addr((u32)tadr);
    d->chcr = (d->chcr & 0xFEC0) | 0x10000105;
    SYNC_L();
}

// FUNC 0x0100ECD0 gs_compute_display_period
// displayPeriod = pal ? 0x33E : 0x320; if periodAdjust: videoMode 1 adds
// frameMode ? 0xA : 0x14, videoMode 2 adds frameMode ? 0x14 : 0xA, videoMode
// 3 gives 0.  Only reached by tail jumps from gfx_disable (0xF, 0x11).
void gs_compute_display_period(GsContext *ctx)
{
    u32 period = ctx->pal ? 0x33E : 0x320;

    if (ctx->periodAdjust != 0) {
        switch (ctx->videoMode) {
        case 1:
            period += (ctx->frameMode == 0) ? 0x14 : 0xA;
            break;
        case 2:
            period += (ctx->frameMode == 0) ? 0xA : 0x14;
            break;
        case 3:
            period = 0;
            break;
        default:
            break;
        }
    }
    ctx->displayPeriod = period;
}

// FUNC 0x0100ED58 gs_set_bg_color
// bgColor = rgba; bgBlend = each colour byte c -> (c * 129) >> 8 (MMI
// pextlb / dsll 7 + daddu / dsrl 8 / ppacb - 0..255 to the GS 0..0x80 range),
// alpha byte kept.  If bgImmediate: tail-calls gs_set_material_param(
// bgParamMask, bgParamKind, rgba & 0xFF000000) - quirk: only the alpha byte
// is passed (a0 was masked in place and then moved to a2).  bgImmediate is
// never set in this binary.  Only reached by the `j` from gs_set_bg_color_rgb.
void gs_set_bg_color(u32 rgba)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 scaled;
    u32 mask;
    u32 alpha;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    u64 x, y;
#else
    int i;
#endif

    ctx->bgColor = rgba;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* MMI, as in the original: the bytes of rgba zero-extended to halfword
     * lanes, each lane * 129 >> 8 (no lane overflows: 255 * 129 < 0x10000;
     * the dsrl carries the next lane's low byte into bits 8-15, which ppacb
     * drops), packed back to bytes. */
    __asm__ __volatile__("pextlb %0, $0, %1" : "=r"(x) : "r"(rgba));
    y = ((x << 7) + x) >> 8;
    mask = 0xFF000000;
    __asm__ __volatile__("ppacb  %0, $0, %1" : "=r"(scaled) : "r"(y));
#else
    scaled = 0;
    for (i = 0; i < 4; i++) {
        u32 c = (rgba >> (8 * i)) & 0xFF;
        scaled |= (((c * 129) >> 8) & 0xFF) << (8 * i);
    }
    mask = 0xFF000000;
#endif
    alpha = rgba & mask;
    ctx->bgBlend = (scaled & ee_not32(mask)) | alpha;   /* the retail `not` */
    if (ctx->bgImmediate)
        gs_set_material_param(ctx->bgParamMask, ctx->bgParamKind, alpha);
}

// FUNC 0x0100EDB0 pack_rgba_floats_to_u32
// r, g, b scaled by `scale` (5th float arg, f16), a by 128.0; each
// max(x, 0) + 0.5, min(.., 255.0), truncated; returns r | g << 8 | b << 16 |
// a << 24.
// The float part is hand-written EE assembly in the original: it works in
// place on the argument registers $f12..$f16, uses the COP1 max.s / min.s that
// egcs never emits, and loads its constants through a GPR ($a1, the first
// output operand, which ends up holding r) interleaved with the arithmetic -
// the compiler would use li.s ($at) instead.  The matching build keeps that
// block as inline asm; the C model under #else computes the same values.
u32 pack_rgba_floats_to_u32(float r, float g, float b, float a, float scale)
{
    s32 ir, ig, ib, ia;

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* r, g, b, a, scale are $f12..$f16 (EE EABI float arguments). */
    __asm__(
        "mul.s   $f12, $f12, $f16\n\t"
        "lui     %0, 0x4300\n\t"           /* 128.0 */
        "mul.s   $f13, $f13, $f16\n\t"
        "mtc1    %0, $f1\n\t"
        "mul.s   $f14, $f14, $f16\n\t"
        "mtc1    $0, $f0\n\t"
        "mul.s   $f15, $f15, $f1\n\t"
        "lui     %0, 0x3F00\n\t"           /* 0.5 */
        "max.s   $f12, $f12, $f0\n\t"
        "mtc1    %0, $f1\n\t"
        "max.s   $f13, $f13, $f0\n\t"
        "lui     %0, 0x437F\n\t"           /* 255.0 */
        "max.s   $f14, $f14, $f0\n\t"
        "mtc1    %0, $f2\n\t"
        "max.s   $f15, $f15, $f0\n\t"
        "add.s   $f12, $f12, $f1\n\t"
        "add.s   $f13, $f13, $f1\n\t"
        "add.s   $f14, $f14, $f1\n\t"
        "add.s   $f15, $f15, $f1\n\t"
        "min.s   $f12, $f12, $f2\n\t"
        "min.s   $f13, $f13, $f2\n\t"
        "min.s   $f14, $f14, $f2\n\t"
        "min.s   $f15, $f15, $f2\n\t"
        "cvt.w.s $f12, $f12\n\t"
        "cvt.w.s $f13, $f13\n\t"
        "cvt.w.s $f14, $f14\n\t"
        "cvt.w.s $f15, $f15\n\t"
        "mfc1    %0, $f12\n\t"
        "mfc1    %1, $f13\n\t"
        "mfc1    %2, $f14\n\t"
        "mfc1    %3, $f15"
        : "=&r"(ir), "=r"(ig), "=r"(ib), "=r"(ia)
        : "f"(r), "f"(g), "f"(b), "f"(a), "f"(scale)
        : "$f0", "$f1", "$f2");
#else
    r *= scale;
    g *= scale;
    b *= scale;
    a *= 128.0f;
    r = ee_minf(ee_maxf(r, 0.0f) + 0.5f, 255.0f);
    g = ee_minf(ee_maxf(g, 0.0f) + 0.5f, 255.0f);
    b = ee_minf(ee_maxf(b, 0.0f) + 0.5f, 255.0f);
    a = ee_minf(ee_maxf(a, 0.0f) + 0.5f, 255.0f);
    ir = (s32)r;
    ig = (s32)g;
    ib = (s32)b;
    ia = (s32)a;
#endif
    ir = ir | (ig << 8) | (ib << 16);
    return ir | (ia << 24);
}
