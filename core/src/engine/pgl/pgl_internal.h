/* pgl_internal.h - unit u117 of the GT3 main executable, 0x00254828..0x00257EF4.
 *
 * The end of the no-FDE graphics library (G001, plain C): Polyphony's GS /
 * VIF1 / GIF driver (the bootstrap's src/gs_driver.cpp in a later version)
 * with its OpenGL-like "pgl" layer on VU0 macro mode:
 *   pgl.c      CLUT / TEX0 setters, image upload / download
 *                        packets, the matrix mode / matrix stack (VU0 data
 *                        memory and vi11..vi13), the attribute stack
 *                        (glPushAttrib-like), frustum / ortho, packet
 *                        helpers, light / light-model setters, sin / cos
 *                        wrappers, 2D sprites and lines, 3D lines through
 *                        the combined matrix with clipping, the sprite Z/F,
 *                        and the CPU lighting fallback __pglShading.
 *   vu0_matrix_asm.c  the hand-written VU0 matrix routines (sin / cos,
 *                        identity, load / store / multiply / rotate /
 *                        translate the current matrix vf28..vf31).
 *
 * The shared G001 types (GsContext, GsLight, GsMaterial, GsVec4 ...), the
 * library's prototypes and the values its functions take (PGL_CAP_*,
 * PGL_MTX_*, PGL_ATTR_*, PGL_LIGHT_* ...) come from the canonical
 * engine/pgl/G001_flare.h.  This header keeps what only this unit uses: register
 * field / packet constants, the VU0 micro subroutine entries, small helpers
 * and the prototypes of the functions no other unit calls.
 *
 * VU0 register conventions of the library (macro mode):
 *   vf28..vf31  the current matrix (row-vector convention: v' = v * M, row
 *               3 = translation), of the mode in GsContext.matrixMode; the
 *               other modes' matrices are parked in VU0 data memory at qword
 *               4 + 4 * mode;
 *   vf24..vf27  the combined matrix, rebuilt by pgl_sync_matrices when
 *               GsContext.matrixDirty is set (every matrix change sets it);
 *   vi11..vi13  the matrix stack pointers of modes 0..2 (VU0 data memory);
 *   vf20..vf23  scratch, or the viewport constants (viewScale, viewCenter,
 *               limits, vBB0) for the primitives drawn through VU0.
 * The VU0 microcode (0x00349600, uploaded by gfx_init; read with the
 * toolchain's objdump, the upper words as macro-mode COP2 instructions)
 * has these micro subroutines (vcallms byte addresses):
 *   0x000  vf28..vf31 = vf1..vf4 x vf28..vf31   (current = m x current)
 *   0x090  vf28..vf31 = vf28..vf31 x vf1..vf4   (current = current x m)
 *   0x120  vf24..vf27 = vf28..vf31 x VU0 qwords 8..11 (u115: combined =
 *          current x projection)
 *   0x1D0  vf1 = sin(vf1) per lane: x - x^3/6 + x^5/120 - x^7/5040 +
 *          x^9/362880 (the Taylor polynomial; arguments in [-pi/2, pi/2])
 *   0x2D0  u113's arctangent.
 */
#ifndef U117_H
#define U117_H

#include "types.h"
#include "libc.h"               /* memcpy, printf */
#include "eekernel.h"           /* FlushCache */
#include "libgraph.h"           /* SCE_GS_PSM*, SCE_GS_* register addresses */
#include "engine/pgl/G001_flare.h"

/* The scratchpad context pointer as retail loads it: whole (lui + ori) at
 * the function's start, never folded into the field offsets (the
 * bootstrap's gs_internal.h, pgl_effects_internal.h).  GCC 14 gets the plain constant. */
#ifndef GS_SPR_CTX
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif
#endif

/* ======================================================================== */
/* Register shadows                                                         */
/* ======================================================================== */

/* GsContext.dirty bits GS_DIRTY_* and the register field macros GS_FIELD_*:
 * G001_flare.h.  The GS register fields as "shift, width" (passed through
 * GS_FIELD_* so that the comma splits the arguments): */

/* TEX0 (TEX2 has the same CBP .. CLD fields). */
#define GS_TEX0_PSM         20, 6
#define GS_TEX0_TCC         34, 1       /* 0: RGB, 1: RGBA (texture alpha used) */
#define GS_TEX0_CBP         37, 14      /* CLUT base, 64-word blocks */
#define GS_TEX0_CPSM        51, 4
#define GS_TEX0_CSM         55, 1
#define GS_TEX0_CSA         56, 5
#define GS_TEX0_CLD         61, 3       /* CLUT buffer load control */
#define GS_CLD_KEEP         0           /* do not load the CLUT buffer */
#define GS_CLD_LOAD         1           /* load it */

/* TEX0 / TEX2 as Sony's sceGsTex0 bitfields (libgraph): the CLUT setters
 * assign fields of the shadow registers (egcs: one ld, and / or per field,
 * one sd). */
typedef struct {
    u64 TBP0:14, TBW:6, PSM:6, TW:4, TH:4, TCC:1, TFX:2;
    u64 CBP:14, CPSM:4, CSM:1, CSA:5, CLD:3;
} GsTex0Bits;
#define GS_TEX0_BITS(ctx, r)    ((GsTex0Bits *)&(ctx)->reg[r])

/* gs_set_clut's CT24 palette: sent as CPSM CT32 (0), TCC 0 for the indexed
 * texture formats (GsContext.clutPsm). */
#define GS_CLUT_CT24        SCE_GS_PSMCT24

/* SCANMSK.MSK (bits 0..1). */
#define GS_SCANMSK_MSK      0, 2
#define GS_SCANMSK_EVEN     2           /* rows with even Y not drawn */
#define GS_SCANMSK_ODD      3           /* rows with odd Y not drawn */

/* ======================================================================== */
/* Packets                                                                  */
/* ======================================================================== */

/* DMA tags, VIF codes and the GIF tag fields: G001_flare.h. */
#define GIF_FLG_PACKED      (0ull << 58)
/* The same fields as seen from the tag's high word (the 3D line). */
#define GIF_HI_FLG_PACKED   0x00000000u
/* Register descriptor i (4 bits) of a tag's high dword. */
#define GIF_REGS_RGBAQ_PRIM (GIF_REG(0, SCE_GS_RGBAQ) | GIF_REG(1, SCE_GS_PRIM))       /* 0x01 */
#define GIF_REGS_LINE       (GIF_REGS_RGBAQ_PRIM | GIF_REG(2, SCE_GS_XYZF3) | \
                             GIF_REG(3, SCE_GS_XYZF2))                                  /* 0x4C01 */
#define GIF_REGS_SPRITE_UV  (GIF_REG(0, SCE_GS_PRIM) | GIF_REG(1, SCE_GS_RGBAQ) | \
                             GIF_REG(2, SCE_GS_UV) | GIF_REG(3, SCE_GS_XYZF3) | \
                             GIF_REG(4, SCE_GS_UV) | GIF_REG(5, SCE_GS_XYZF2))          /* 0x43C310 */
#define GIF_REGS_UV_SPRITE  (GIF_REG(0, SCE_GS_UV) | GIF_REG(1, SCE_GS_XYZF3) | \
                             GIF_REG(2, SCE_GS_UV) | GIF_REG(3, SCE_GS_XYZF2))          /* 0x43C3 */
#define GIF_REGS_LINE_3D    (GIF_REG(0, SCE_GS_RGBAQ) | GIF_REG(1, SCE_GS_XYZF2) | \
                             GIF_REG(2, SCE_GS_XYZF2))                                  /* 0x441 */

/* GS PRIM (and GsContext.primAttr bits). */
enum {
    GS_PRIM_LINE       = 0x001,
    GS_PRIM_LINE_STRIP = 0x002,
    GS_PRIM_SPRITE     = 0x006,
    GS_PRIM_TME        = 0x010,
    GS_PRIM_FST        = 0x100      /* UV (texel) coordinates instead of STQ */
};
/* PRIM type and TME of the attribute bits replaced: untextured primitive. */
#define GS_PRIM_UNTEXTURED(attr, type)  ((u16)(((attr) | (type)) & ~GS_PRIM_TME))

#define GS_RGBAQ_Q_ONE      0x3F800000u     /* RGBAQ.Q = 1.0f (the high word) */
#define GS_FOG_NONE         0xFF            /* XYZF.F: the fog coefficient of no fog */
#define GS_Z24_MAX          0xFFFFFF        /* XYZF.Z (24 bits) */

/* Image transfers: the header (GIFtag A+D, BITBLTBUF, TRXPOS, TRXREG,
 * TRXDIR, GIFtag IMAGE) is 6 qwords. */
#define GS_XFER_HDR_QWC     6
#define GS_TRXDIR_HOST_TO_LOCAL 0

/* gfx_dma_* channels (GfxDriverState.chan). */
enum { GFX_CH_VIF0 = 0, GFX_CH_VIF1 = 1, GFX_CH_GIF = 2 };

/* XY offsets: 12.4 fixed point; a texel centre is half a texel (8) in. */
#define GS_XY_SHIFT         4
#define GS_XY_SHIFT_FIELD   3               /* GsContext.halfHeight: y in half-height lines */
#define GS_UV_HALF_TEXEL    8

/* The w added before ftoi4 to a 2D strip vertex whose segment is outside
 * one plane: 2048.0f, i.e. XYZF2.ADC (bit 15 of the fourth word) - the
 * vertex moves without drawing a segment. */
#define GS_ADC_W_FLOAT      0x45000000u

/* Physical (DMA) address of a pointer. */
#define DMA_ADDR(p)         ((u32)(p) & 0x0FFFFFFF)
/* DMA destination bit for a scratchpad (0x7xxxxxxx) address. */
#define DMA_SPR             0x80000000u

/* Aliasing-safe view of a packet dword at word index w. */
typedef u64 u64a __attribute__((may_alias));
typedef u32 u32a __attribute__((may_alias));
#define PKT64(p, w)   (*(u64a *)((u32 *)(p) + (w)))

/* The same dword written through a union (the bootstrap's gs_driver.cpp):
 * egcs gives a union member access alias set 0, so such a store stays in
 * order with every other access (the retail packet builders' strictly
 * serial stores). */
typedef union GsPktDword { u64 d; u32 w[2]; } GsPktDword;
#define PKT64U(p, w)  (((GsPktDword *)((u32 *)(p) + (w)))->d)

/* One A+D qword at q (u32 *): data low word, data high word, register
 * address; q is advanced past it.  The packet builders' statement macro
 * (retail: the qword pointer copied, q + 16, then the three stores). */
#define GS_AD_PUT(q, lo, hi, reg)                                           \
    do {                                                                    \
        GsPktDword *ad_ = (GsPktDword *)(q);                                \
        (q) += 4;                                                           \
        ad_[0].w[0] = (lo);                                                 \
        ad_[0].w[1] = (hi);                                                 \
        ad_[1].d = (reg);                                                   \
    } while (0)

/* GsContext.cur read / written as the integer it was in the original (an
 * address word ORed with addrBits): its loads and stores are in the alias
 * set of unsigned int, so they stay in order with the u32 packet and
 * BITBLTBUF stores around them, as retail's do (gs_download_image). */
#define GS_CUR_WORD(ctx)    (*(u32 *)&(ctx)->cur)

/* The BITBLTBUF word of a transfer: buffer address / 64, width / 64, PSM
 * (DBP DBW DPSM, the high word; for a readback SBP SBW SPSM). */
static inline u32 gs_bitblt_word(u32 fbWordAddr, int psm, int fbw)
{
    return ((fbWordAddr >> 6) & 0xFFFF) | (((fbw >> 6) & 0xFF) << 16) | ((u32)psm << 24);
}

/* Opens a list-0 DIRECT block of qwc qwords at the packet pointer (no
 * header when qwc is 0): DMA cnt tag + VIF FLUSH + VIF DIRECT.  Returns the
 * block start (the header qword); the data follow at +4 words. */
static inline u32 *gs_open_direct(GsContext *ctx, s32 qwc)
{
    u32 *p = ctx->cur;

    if (qwc != 0) {
        p[0] = DMATAG_CNT(qwc);
        p[2] = VIF_FLUSH;
        p[3] = VIF_DIRECT(qwc);
        ctx->cur = p + 4 + qwc * 4;     /* retail: p + (qwc * 16 + 16) */
        p[1] = 0;
    }
    return p;
}

/* ======================================================================== */
/* VU0                                                                      */
/* ======================================================================== */

/* VU0 data memory qword where the matrix of a mode is parked. */
#define VU0_MATRIX_QW(mode) (4 + 4 * (mode))

/* VU0 micro subroutines (vcallms operands, see the file comment). */
#define VU0_MS_MULT         "0x0"           /* current = m x current */
#define VU0_MS_POSTMULT     "0x90"          /* current = current x m */
#define VU0_MS_SIN          "0x1D0"         /* vf1 = sin(vf1) */

/* The six VU0 clip flags of a point (vclipw, the CLIP register vi18):
 * bit 2k = x_k > |w|, bit 2k + 1 = x_k < -|w| (k = x, y, z). */
#define VU0_CLIP_MASK       0x3F
/* For w < 0 the flags become those of x_k > w / x_k < -w (the planes of the
 * homogeneous clip): each pair swapped and inverted. */
#define VU0_CLIP_FLAGS_NEG_W(code) \
    (((((code) >> 1) | 0x2A) ^ (((code) << 1) | 0x15)) & VU0_CLIP_MASK)
/* The same as a statement on the unmasked code (vu0_clip_code). */
#define VU0_CLIP_NEG_W(code) \
    do { (code) = (((code) >> 1) | 0x2A) ^ (((code) << 1) | 0x15); } while (0)

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* R5900 "sync.l" (encoded `sync` 0x0000000F). */
#define SYNC_L() __asm__ __volatile__("sync.l" : : : "memory")

/* The bits of a float and back. */
static inline u32 u117_f2u(f32 f) { union { f32 f; u32 u; } c; c.f = f; return c.u; }
static inline f32 u117_u2f(u32 u) { union { f32 f; u32 u; } c; c.u = u; return c.f; }

/* A quadword at any alignment into a 128-bit GPR (ldl / ldr pairs and
 * pcpyld, which egcs never emits for a u128: an asm in the original). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline GsVec4Arg u117_load_u128(const void *p)
{
    u128 v, t;

    __asm__ __volatile__(
        "ldl        %0, 7(%2)\n\t"
        "ldr        %0, 0(%2)\n\t"
        "ldl        %1, 15(%2)\n\t"
        "ldr        %1, 8(%2)\n\t"
        "pcpyld     %0, %1, %0\n\t"
        : "=&r"(v), "=&r"(t) : "r"(p) : "memory");
    return v;
}
#else
static inline GsVec4Arg u117_load_u128(const void *p)
{
    GsVec4Arg v;

    memcpy(&v, p, sizeof(v));
    return v;
}
#endif

/* x = sqrt(x) in place as the EE sqrt.s (the bootstrap's EE_SQRTF: an asm
 * keeps retail's placement of the square roots). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define EE_SQRTF(x) __asm__ __volatile__("sqrt.s %0, %1" : "=f"(x) : "f"(x))
#else
#define EE_SQRTF(x) ((x) = __builtin_sqrtf(x))
#endif

/* EE COP1 max.s / min.s. */
static inline f32 ee_maxf(f32 a, f32 b) { return a > b ? a : b; }
static inline f32 ee_minf(f32 a, f32 b) { return a < b ? a : b; }
/* clamp(x, lo, hi) as max.s then min.s */
static inline f32 ee_clampf(f32 x, f32 lo, f32 hi) { return ee_minf(ee_maxf(x, lo), hi); }

/* GsLight.spotCutoff of a light that is not a spot light. */
#define PGL_SPOT_CUTOFF_NONE 180.0f

/* ======================================================================== */
/* Functions of the unit that no other unit calls (address order)          */
/* ======================================================================== */

float float_pow_encoded(float x, int enc);                             /* 0x002559D0 */
float pgl_sin_deg(float deg);                                          /* 0x00255A48 (no callers) */
void pgl_set_sprite_depth_i(int z);                                    /* 0x00256C68 (no callers) */
int  clip_line_homogeneous(float *a, float *b);                        /* 0x00256CB8 */
int  vu0_clip_code(const float *v);                                    /* 0x00257060 */
void vu0_mult_matrix_regs(void);                                       /* 0x00257D7C (shared tail) */

#endif /* U117_H */
