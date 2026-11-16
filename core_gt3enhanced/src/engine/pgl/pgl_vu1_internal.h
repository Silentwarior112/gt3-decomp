/* pgl_vu1_internal.h - unit u115 of the GT3 main executable, 0x0024D8C0..0x00250A58.
 *
 * A stretch of the no-FDE graphics library (G001, plain C): Polyphony's
 * "pgl" GS / VU0 / VU1 driver ("__pglShading" is its own name), an
 * OpenGL-like layer over the drawing context in scratchpad RAM.  Files:
 *   format_part2.c       the tail of the library's printf engine with a
 *                            character-output callback (%d %u %t %o %i %x %X
 *                            %c %s; floats are skipped)
 *   vu1_lighting.c the OpenGL-style lighting state compiled for the
 *                            VU1 microcode (light list, eye-space light
 *                            vectors, light x material colours, the
 *                            per-model lighting packets), the VIF1 packets of
 *                            prebuilt VU1 models, the environment-map matrix
 *   vu1_prim.c     immediate-mode primitives: a vertex array (xyz,
 *                            normals, colours, texture coordinates) sent to
 *                            the VU1 primitive program
 *   gs_driver_core.c    the driver core shared with the bootstrap
 *                            (gs_flush_dirty_regs, context / driver init) and
 *                            the COP0 performance counter reads
 *
 * The shared G001 types (GsContext, GsLight, GsMaterial, GfxDriverState,
 * VifModel, GsLibState ...) and the library's prototypes come from the
 * canonical engine/pgl/G001_flare.h; this header keeps what only this unit uses.
 *
 * Conventions: offsets in comments are the retail ones; "?" marks a guess.
 * VU0 macro-mode code is inline asm, one asm statement per stretch of the
 * listing (the VU0 registers are not touched by compiled C, so they carry
 * over between statements).
 */
#ifndef U115_H
#define U115_H

#include "types.h"
#include "vu0asm.h"
#include <stddef.h>                     /* offsetof */
#include "libc.h"                       /* memset */
#include "eekernel.h"                   /* AddIntcHandler, EnableIntc, INTC_* */
#include "libgraph.h"                   /* sceGsResetGraph, GS_CSR, GS_IMR */
#include "engine/pgl/G001_flare.h"

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The bits of a float (lw of a float field). */
static inline u32 u115_f2u(f32 f) { union { f32 f; u32 u; } c; c.f = f; return c.u; }

/* Aliasing-safe view of packet dwords. */
typedef u64 u115_u64a __attribute__((may_alias));
#define U115_PKT64(p, w) (*(u115_u64a *)((u32 *)(p) + (w)))   /* dword at word index w */

/* The scratchpad context pointer as retail loads it: 0x70003000 whole into
 * a register (lui + ori) at the function start, never folded into the
 * field offsets, and a wrapper that only passes it on does not tail-call
 * (the bootstrap's gs_internal.h).  egcs: a one-instruction asm in a
 * statement expression; GCC 14 the plain constant.  Same pointer value. */
#ifndef GS_SPR_CTX
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif
#endif

/* Physical (DMA) address of a pointer. */
#define U115_DMA_ADDR(p) ((u32)(p) & 0x0FFFFFFF)

/* pref 0, 0x40(p): the binary prefetches the next packet line after each
 * block it writes (no effect on the result). */
#define U115_PREF(p) __asm__ __volatile__("pref 0, 0x40(%0)" : : "r"(p))

/* A packet quadword: a union, so egcs gives its accesses alias set 0 (they
 * conflict with every other store and stay in source order); the copies
 * are lq / sq. */
typedef union { u128 q; u32 w[4]; f32 f[4]; } U115Qw;
#define U115_QW(p)      (((U115Qw *)(p))->q)
/* One quadword copy as a statement macro (its loop notes keep each lq / sq
 * pair together, as in pgl_send_matrix). */
#define U115_QW_COPY(dst, src)  do { U115_QW(dst) = U115_QW(src); } while (0)

/* Unaligned 64-bit view (ldl / ldr). */
typedef struct { u64 v; } __attribute__((packed, may_alias)) u115_u64u;

/* The hand-written VU0 blocks that run through the GPRs: retail's
 * allocation shows that some of them did not declare $2 ($v0) clobbered
 * (the compiler keeps using $v0 right after them).  egcs gets the same
 * list; GCC 14 the complete one. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define U115_CLOBBER_V0
#else
#define U115_CLOBBER_V0 , "$2"
#endif

/* ---- VU0 macro-mode idioms of this unit ---------------------------------
 * vf20..vf23 = vf28..vf31 (the current matrix registers). */
#define U115_VU_MOVE_CUR_TO_VF20() __asm__ __volatile__(                \
        "vmove.xyzw   $vf20, $vf28\n\t"                                  \
        "vmove.xyzw   $vf21, $vf29\n\t"                                  \
        "vmove.xyzw   $vf22, $vf30\n\t"                                  \
        "vmove.xyzw   $vf23, $vf31\n\t" : : : "memory")
/* vf20..vf23 = VU0 data memory qwords q..q+3 (a parked matrix). */
#define U115_VU_LOAD_MEM_TO_VF20(q) __asm__ __volatile__(               \
        "ctc2.i       %0, $vi10\n\t"                                     \
        "vlqi.xyzw    $vf20, ($vi10++)\n\t"                              \
        "vlqi.xyzw    $vf21, ($vi10++)\n\t"                              \
        "vlqi.xyzw    $vf22, ($vi10++)\n\t"                              \
        "vlqi.xyzw    $vf23, ($vi10++)\n\t"                              \
        "vnop\n\t"                                                       \
        "vnop\n\t" : : "r"(q) : "memory")
/* The inverse of a rigid matrix in vf20..vf23, in place: the 3x3 rotation
 * is transposed through the GPRs (qmfc2 / pextlw / pextuw / pcpyld /
 * pcpyud / qmtc2; the new rows get w = 0) and vf23.xyz = R^T * -t
 * (vf23.w kept).  Three asm statements: the vsub (vf23 = -t), the
 * transposition, the product; the first row goes through a
 * compiler-chosen register t (retail has $a2 where that is free and $a3
 * where $a2 holds a live pointer), the rest through $v0, $v1, $a0, $a1.
 * (As separate statements a register that dies just before or is born
 * just after is not in conflict with the clobbers: retail's allocation.)
 * t is read and written ("+r"): it is then live from the function start,
 * a global pseudo that global-alloc places after the live pointers
 * (pgl_update_eye_lights: ctx $a2, t $a3); with "=r" it is block-local and
 * local-alloc takes $a2 first.  Its incoming value is never used (the
 * first instruction overwrites it), so GCC 14 gets "=r" (no uninitialized
 * read).  ("+r" may be an exact form, not the original: it computes the
 * same thing; how the original declared the row operand is not known.) */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define U115_VU_T_OUT           "+r"
#else
#define U115_VU_T_OUT           "=r"
#endif
#define U115_VU_TRANSPOSE_TO_VF20_ASM                                    \
        "qmfc2.ni     %0, $vf20\n\t"                                     \
        "qmfc2.ni     $2, $vf21\n\t"                                     \
        "qmfc2.ni     $4, $vf22\n\t"                                     \
        "pextlw       $3, $2, %0\n\t"                                    \
        "pextuw       $5, $2, %0\n\t"                                    \
        "pextlw       $2, $0, $4\n\t"                                    \
        "pextuw       $4, $0, $4\n\t"                                    \
        "pcpyld       %0, $2, $3\n\t"                                    \
        "pcpyud       $2, $3, $2\n\t"                                    \
        "pcpyld       $4, $4, $5\n\t"                                    \
        "qmtc2.ni     %0, $vf20\n\t"                                     \
        "qmtc2.ni     $2, $vf21\n\t"                                     \
        "qmtc2.ni     $4, $vf22\n\t"
#define U115_VU_INVERT_VF20(t)                                          \
        __asm__ __volatile__("vsub.xyz     $vf23, $vf0, $vf23" : : : "memory"); \
        __asm__ __volatile__(U115_VU_TRANSPOSE_TO_VF20_ASM              \
                             : U115_VU_T_OUT(t) : : "memory", "$2", "$3", "$4", "$5"); \
        __asm__ __volatile__(                                           \
            "vmulax.xyz   " VU0_ACC ", $vf20, $vf23x\n\t"               \
            "vmadday.xyz  " VU0_ACC ", $vf21, $vf23y\n\t"               \
            "vmaddz.xyz   $vf23, $vf22, $vf23z\n\t" : : : "memory")
/* vf2 = normalize(vf2) (x, y, z summed; all four lanes scaled). */
#define U115_VU_NORMALIZE_VF2_ASM                                        \
        "vmul.xyzw    $vf1, $vf2, $vf2\n\t"                              \
        "vaddy.x      $vf1, $vf1, $vf1y\n\t"                             \
        "vaddz.x      $vf1, $vf1, $vf1z\n\t"                             \
        "vrsqrt       " VU0_Q ", $vf0w, $vf1x\n\t"                              \
        "vwaitq\n\t"                                                     \
        "vmulq.xyzw   $vf2, $vf2, " VU0_Q "\n\t"

/* ======================================================================== */
/* Hardware registers                                                       */
/* ======================================================================== */

#define GIF_CTRL_REG        REG32(0x10003000)   /* GIF_CTRL */
#define GIF_MODE_REG        REG32(0x10003010)   /* GIF_MODE */
#define VIF0_FBRST_REG      REG32(0x10003810)   /* VIF0_FBRST */
#define VIF0_ERR_REG        REG32(0x10003820)   /* VIF0_ERR */
#define VIF1_FBRST_REG      REG32(0x10003C10)   /* VIF1_FBRST */
#define VIF1_ERR_REG        REG32(0x10003C20)   /* VIF1_ERR */
#define GS_CSR_REG          REG64(GS_CSR)
#define GS_IMR_REG          REG64(GS_IMR)

/* GS IMR store (the bootstrap's GS_IMR_PUT / GFX_SET_IMR).  Retail writes
 * IMR through a plain pointer: egcs folds that constant address into the
 * store (lui $at + offset), which it never does for a volatile access; so
 * egcs gets the non-volatile store (it compiles to the retail
 * instructions), GCC 14 keeps it volatile.  The shadow first, in a
 * do { } while (0): its loop notes keep gfx_init's CSR revision read ahead
 * of the IMR writes. */
#ifdef BOOT_OLD_GCC
#define GS_IMR_PUT(v)       (*(u64 *)GS_IMR = (v))
#else
#define GS_IMR_PUT(v)       (GS_IMR_REG = (v))
#endif
#define GFX_SET_IMR(v)      do { g_gfx.imr = (v); GS_IMR_PUT(v); } while (0)

#define GS_CSR_FINISH       0x0002              /* FINISH event (write 1 to clear) */
#define GS_CSR_VSINT        0x0008              /* VSYNC event */
#define GS_IMR_FINISH_ONLY  ((u64)(s64)-0x201)  /* ~0x200: every source masked except FINISHMSK */
#define GIF_CTRL_RST        1
#define GIF_MODE_IMT        4                   /* intermittent PATH3 */
#define VIF_FBRST_RST       1
#define VIF_ERR_ME0_ME1     6                   /* ignore DMAtag mismatch and VIFcode errors */
#define VU_FBRST_RS0_RS1    0x202               /* COP2 vi28: reset VU0 and VU1 */

/* COP0 performance counter control (gfx_init): CTE; counter 0 event 1
 * (CPU cycles), counter 1 event 6; both count in EXL, kernel, supervisor
 * and user mode. */
#define PCCR_GFX            0x8003783E

/* 0.2f / 0.8f / -1/6 with the bits of the retail .lit4 (rounded to
 * nearest).  GCC 14 converts decimal float literals for the R5900 by
 * truncation, egcs to nearest (and egcs takes no hex float with an `f' in
 * it), so they have 9 significant digits rounded UP: a decimal in
 * [f, f + ulp/2) gives f with both.  The hex spelling is in the comment. */
#define GS_F_0_2        0.200000003f        /* 0x3E4CCCCD 0x1.99999Ap-3f */
#define GS_F_0_8        0.800000012f        /* 0x3F4CCCCD 0x1.99999Ap-1f */
#define GS_F_M1_6       (-0.166666672f)     /* 0xBE2AAAAB -0x1.555556p-3f, .lit4 0x00350A74 (gp-0x6F7C) */
#define GS_F_FLT_MAX    3.40282347e+38f     /* 0x7F7FFFFF 0x1.fffffep+127f, .sdata 0x003530D8 */

/* ======================================================================== */
/* Packet constants (the same names as pgl_font_internal.h)                              */
/* ======================================================================== */

/* GsContext.dirty bits (GS_DIRTY_*, GS_DIRTY_ALL: G001_flare.h) of
 * gfx_reset_texture_state: TEXA (gen 7 -> bit 39) + CLAMP, TEX1, TEX0,
 * MIPTBP1, MIPTBP2 of both contexts (TEX2 is not included). */
#define GS_DIRTY_TEX_STATE      0x000000801E801E80ull

/* DMA tags and VIF codes: G001_flare.h (VIF_UNPACK: FLG, addresses relative
 * to TOPS: the VU1 data are double buffered, see the lighting file; USN). */
#define VIF_STROW               0x30000000u     /* the 4 row words follow */
/* UNPACK formats */
enum {
    VIF_S_32  = 0x60,
    VIF_S_8   = 0x62,
    VIF_V2_32 = 0x64,
    VIF_V3_32 = 0x68,
    VIF_V4_32 = 0x6C,
    VIF_V4_8  = 0x6E
};

/* STMASK values (2 bits per field, rows 0..3): 0xC0C0C0C0 x, y, z written,
 * w write-protected (the default, set at the list starts); 0x3F3F3F3F only
 * w written; 0x55555555 every field from the row registers (STROW). */
#define STMASK_DEFAULT          0xC0C0C0C0u
#define STMASK_W_ONLY           0x3F3F3F3Fu
#define STMASK_ROW              0x55555555u

/* GIF tag fields: G001_flare.h.  A+D tag, PACKED, NREG 1, over the low dword lo (NLOOP | EOP; the two
 * users extend it differently: pgl_send_dirty_regs sign-, gs_flush_dirty_regs
 * zero-extends, the same value for NLOOP >= 0). */
#define GIF_AD_TAG(lo)          (GIF_NREG(1) | (u64)(lo))
/* REGS ST (2), RGBAQ (1), XYZF2 (4) */
#define GIF_REGS_ST_RGBAQ_XYZF2 0x412u

/* PRIM */
enum {
    GS_PRIM_TRIANGLE_STRIP = 4,
    GS_PRIM_IIP            = 0x008,
    GS_PRIM_TME            = 0x010
};

/* GS register values of the context init. */
#define GS_TEXA_TA1_DEFAULT     0x80                     /* TEXA.TA1 */
#define GS_TEXA_AEM             0x0000000000008000ull
#define GS_TEX0_TCC             0x0000000400000000ull    /* TEX0.TCC (bit 34) */
#define GS_TEX1_DEFAULT         0x60                     /* MMAG 1, MMIN 1 (bilinear) */
#define GS_TEX2_DEFAULT         0x2000000001300000ull    /* PSM PSMT8, CLD 1 */
#define GS_ZBUF_ZMSK            0x0000000100000000ull    /* ZBUF.ZMSK (bit 32) */
#define GS_COLCLAMP_CLAMP       1
#define GS_PRMODECONT_AC        1

/* VU1 data addresses (TOPS relative): vertices at 0 (xyz), 0x40 (st, also
 * the strip lengths), 0x80 (rgba), the GIF tag at 0xC0; the combined matrix
 * and the viewport (pgl_send_matrix) at 0xC1, the lighting block at 0xC9,
 * the dirty GS registers (pgl_send_dirty_regs) at 0xD9, GsContext.morphWeight at
 * 0xF2. */
#define VU1_STRIP_XYZ           0x00
#define VU1_STRIP_ST            0x40
#define VU1_STRIP_RGBA          0x80
#define VU1_STRIP_GIFTAG        0xC0
#define VU1_MATRIX_ADDR         0xC1
#define VU1_LIGHT_ADDR          0xC9
#define VU1_REGS_ADDR           0xD9
#define VU1_FB70_ADDR           0xF2
#define VU1_MAX_VERTS           64

/* MSCAL entry adjustments: each block uploaded in front of a VU1 run moves
 * the entry back (matrix 1, registers 5 instructions); a lit model part
 * enters 0x3C later.  (? Code in front of the entry that takes the uploaded
 * block; the microcode was not read.) */
#define VU1_ENTRY_MATRIX        1
#define VU1_ENTRY_REGS          5
#define VU1_ENTRY_LIT           0x3C

/* GsContext.vu1LightProg: the lighting variant of the VU1 model program
 * (sent with ITOP; plus vu1LightOfs 0 / -2 / -4 / -6 by vertex format).
 * The shininess variants: < 6, < 12, else. */
enum {
    VU1L_DEFAULT        = 0x04E,    /* set per model; local viewer with >= 2 VU lights (? no lighting) */
    VU1L_SPEC2_S6       = 0x057,    /* 2 lights, diffuse + specular */
    VU1L_SPEC2_S12      = 0x082,
    VU1L_SPEC2          = 0x0AF,
    VU1L_SPEC4_S6       = 0x0DE,    /* 4 lights, diffuse + specular */
    VU1L_SPEC4_S12      = 0x115,
    VU1L_SPEC4          = 0x14C,
    VU1L_DIFFUSE        = 0x185,    /* 4 lights, diffuse */
    VU1L_LOCAL_S6       = 0x1AD,    /* the eye-position packet (lighting mode 4, local viewer) */
    VU1L_LOCAL_S12      = 0x1E1,
    VU1L_LOCAL          = 0x21A,
    VU1L_ENVMTX         = 0x257,    /* lighting mode 2 */
    VU1L_ENVMAP         = 0x297,    /* lighting mode 3 */
    VU1L_CONST          = 0x2DB     /* lighting mode 5 */
};

/* GsContext.vu1Prog of gfx_context_init (0x320 / 0x33E + 0xA / 0x14 by
 * gs_select_vu1_prog later). */
#define VU1_PRIM_PROG_DEFAULT   0x33E

/* ======================================================================== */
/* Lighting                                                                 */
/* ======================================================================== */

/* GsContext.lightFlags bits: PGL_LF_* (G001_flare.h). */

/* VifModel.mode: bits 0..4 the lighting mode, 5..7 the vertex format. */
#define VIFMODEL_LMODE(m)       ((m) & 0x1F)
#define VIFMODEL_FORMAT(m)      (((m) & 0xE0) >> 5)
enum {
    PGL_LMODE_NONE   = 0,
    PGL_LMODE_LIGHTS = 1,           /* OpenGL lights (pgl_send_lights_mode1 picks the program) */
    PGL_LMODE_ENVMTX = 2,           /* the environment matrix (pgl_build_env_matrix) */
    PGL_LMODE_ENVMAP = 3,           /* the env-map texture matrix (pgl_build_envmap_matrix) */
    PGL_LMODE_LOCAL  = 4,           /* lights with the eye position (pgl_send_lights_mode4) */
    PGL_LMODE_CONST  = 5            /* two constant vectors (constVector, constColor) */
};

/* VifModel.scaleFlags */
#define VIFMODEL_COLOR_255      0x01
#define VIFMODEL_ALPHA_255      0x02

/* pgl_set_vec_param / pgl_set_scalar_param kinds (the vectors of lighting
 * modes 2 and 3; kind 0 multiplies the others in mode 2). */
enum {
    PGL_PARAM_AF0 = 0,              /* qAF0 (callers fade models with it) */
    PGL_PARAM_B00 = 2,              /* qB00 */
    PGL_PARAM_B10 = 3               /* qB10 */
};

/* GsContext.matrixMode */
enum { PGL_MODELVIEW = 0, PGL_PROJECTION = 1, PGL_TEXTURE = 2 };
/* VU0 data memory qwords where a non-current matrix is parked */
#define VU0_MODELVIEW_QW        4
#define VU0_PROJECTION_QW       8
#define VU0_TEXTURE_QW          12
/* VU0 micro subroutine: vf24..vf27 = projection x modelview (current) */
#define VU0_MS_COMBINE          "0x120"

/* ======================================================================== */
/* The printf engine                                                        */
/* ======================================================================== */

/* flags (fmt_put_string / fmt_put_number: u114's FMT_*) */
enum {
    FMT_LEFT  = 0x01,               /* '-' */
    FMT_PLUS  = 0x02,               /* '+' */
    FMT_ZERO  = 0x04,               /* '0' */
    FMT_SPACE = 0x08,               /* ' ' */
    FMT_ALT   = 0x10                /* '#' */
};
/* length modifiers */
enum { FMT_SIZE_INT = 0, FMT_SIZE_SHORT = 1, FMT_SIZE_LONG = 2, FMT_SIZE_LONGLONG = 3 };
/* fmt_put_number bases (0x11: upper-case hex) */
#define FMT_BASE_OCT            8
#define FMT_BASE_DEC            10
#define FMT_BASE_HEX            0x10
#define FMT_BASE_HEX_UPPER      0x11

/* ======================================================================== */
/* Data (only this unit uses them; the data pass defined them in the files  */
/* whose original ranges hold them, named on each line)                     */
/* ======================================================================== */

extern u64       g_fmtPow10[19];            /* 0x002F73A8 .data, 10^18 .. 10^0 (right after GsLibState; format_part2.c) */
extern u8        g_gsVu1Microcode[0x3DA8];  /* 0x002F7440 .data, LCG-XOR scrambled (bootstrap name; gs_driver_core.c) */
extern u8        g_pglAttribStack[0x1000];  /* 0x002FB1E8 .data, zero-filled: the byte stack of
                                             * pgl_push_attrib / pgl_pop_attrib (g_pglAttribSp; gs_driver_core.c) */
extern const u8  g_gsVu0Microcode[0x398];   /* 0x00349600 (bootstrap name; after the end of the .rodata, the
                                             * original's .rdata: src/engine/pgl/vu0_microcode.c) */

/* The environment-map work area (.bss 0x004A68C0..0x004A69A0,
 * g_envmapTexMtx .. g_envmapEye) is vu1_lighting.c's file statics
 * (in the middle of the .bss: commons would be at its end). */

/* vu1_lighting.c defines these (.sdata 0x003530C8..0x003530D4) */
extern f32  g_envmapPhase;                  /* 0x003530C8 gp-0x4928, the view azimuth in turns, -1..0 (init 0) */
extern f32  g_envmapParamA;                 /* 0x003530CC gp-0x4924, pgl_set_envmap_params a (init 4.0) */
extern f32  g_envmapParamB;                 /* 0x003530D0 gp-0x4920, pgl_set_envmap_params b (init 0.25) */
/* gs_driver_core.c defines it (.sdata, with FLT_MAX and g_pglAttribSp) */
extern u8   g_gfxInitialized;               /* 0x003530D4 gp-0x491C, the bootstrap's s_gfxInitialized */

/* Outside G001 (eekernel.h, libgraph.h, libc.h): AddIntcHandler, the first
 * syscall-0x10 stub 0x00285940 (symbols.csv names both 0x10 stubs
 * AddIntcHandler2); EnableIntc 0x002866F0; sceGsResetGraph 0x0028E5D0;
 * memset 0x0027DB38. */

/* ======================================================================== */
/* Functions of the unit that G001_flare.h does not declare (no callers     */
/* outside the unit)                                                        */
/* ======================================================================== */

/* format_part2.c */
int  fmt_parse_uint(const u8 **pp);                                 /* 0x0024DAE0 */

/* vu1_lighting.c */
void vifmodel_setup_lighting(GsContext *ctx, const VifModel *m);    /* 0x0024DF28 */
int  vifmodel_send_lighting(GsContext *ctx, u32 mode, int resend);  /* 0x0024E028 */
void vifmodel_put_part(GsContext *ctx, const VifPart *part, u32 mscal); /* 0x0024E1B8 */
void pgl_send_lights_diffuse(GsContext *ctx);                       /* 0x0024E8F8 */
void pgl_send_lights_specular4(GsContext *ctx);                     /* 0x0024E9C8 */
void pgl_send_lights_specular2(GsContext *ctx);                     /* 0x0024EB30 */
void pgl_send_lights_mode1(GsContext *ctx);                         /* 0x0024EC68 */
void pgl_send_lights_mode4(GsContext *ctx);                         /* 0x0024ED50 */
void pgl_build_light_list(GsContext *ctx);                          /* 0x0024EE80 */
void pgl_update_eye_lights(GsContext *ctx);                         /* 0x0024EF80 */
void pgl_update_light_colors(GsContext *ctx);                       /* 0x0024F218 */
void pgl_send_mode5(GsContext *ctx);                                /* 0x0024F2D0 */
void pgl_send_mode2(GsContext *ctx);                                /* 0x0024F348 */
void pgl_build_env_matrix(GsContext *ctx);                          /* 0x0024F3C0 */
void pgl_send_mode3(GsContext *ctx);                                /* 0x0024F600 */
void pgl_build_envmap_matrix(GsContext *ctx);                       /* 0x0024F670 */

/* vu1_prim.c */
void *pgl_gen_colors(GsContext *ctx, const void *xyz, const void *normals,
                     const void *colors, int n);                    /* 0x0024FA98 */
void *pgl_gen_texcoords(GsContext *ctx, const void *xyz, const void *normals,
                        const void *uvs, int n);                    /* 0x0024FAE0 */
void *pgl_xform_texcoords(GsContext *ctx, const void *uvs, int n);  /* 0x0024FB90 */
void pgl_put_const_color(GsContext *ctx, int n, const void *uvs);   /* 0x0024FD18 */
void pgl_put_colors(GsContext *ctx, const void *colors, int n);     /* 0x0024FDA8 */
void pgl_draw_strip_vu1(const void *xyz, const void *normals, const void *colors,
                        const void *uvs, int n);                    /* 0x0024FEA8 */
void gs_flush_all_regs(void);                                       /* 0x002501E0 */

/* gs_driver_core.c */
u64  gfx_perf_counter1(void);                                       /* 0x002502F8 */
void gfx_wait_vsync_poll(void);                                     /* 0x00250328 */
void gfx_reset_paths(void);                                         /* 0x00250378 */
void gfx_context_init(GsContext *ctx);                              /* 0x00250708 */
void gfx_context_init_spr(void);                                    /* 0x00250A30 */

#endif /* U115_H */
