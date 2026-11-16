/* pgl_font_internal.h - unit u114 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00249B28..0x0024D8C0, part of the no-FDE stretch G001: Polyphony's
 * graphics library "pgl" (the GS / VIF1 / VU1 driver shared with the
 * bootstrap, here in a later, larger version).  Plain C.
 *
 * Files (address order):
 *   font.c      0x00249B28..0x0024B0D8  font-set relocation, the
 *                         bitmap text renderer (sprites in a GIF packet,
 *                         MMI kerning, kanji glyph cache)
 *   fontprint.c 0x0024B0D8..0x0024B5B0  font_vprintf and its
 *                         alignment, the library's sprintf, text measuring
 *   vecmath.c   0x0024B5B0..0x0024B888  normalize, cross,
 *                         gluLookAt, reflection matrix, depth-range shift
 *   texregs.c   0x0024B930..0x0024BE68  GsTexRegs field setters,
 *                         VRAM size of a texture
 *   render.c    0x0024BF10..0x0024D0C0  box culling (VU0 clip),
 *                         material colour scales, 2D polygon fan, the VU1
 *                         strip batcher, texture / material selection
 *   fontfmt.c   0x0024D128..0x0024D4F8  text packet close, text
 *                         measuring
 *   format_part1.c    0x0024D4F8..0x0024D8C0  the %s / number field
 *                         writers of the library's printf (fmt_vprintf,
 *                         u115's format_part2.c continues the object)
 *
 * The shared G001 types (GsContext, GsLibState, PglFont, FontSet, GsTexRegs,
 * VifModel ...) and the prototypes of the library come from the canonical
 * engine/pgl/G001_flare.h; this header keeps what only this unit uses.
 */
#ifndef U114_H
#define U114_H

#include "types.h"
#include <stdarg.h>
#include <stddef.h>                     /* offsetof */
#include "engine/pgl/G001_flare.h"
#include "engine/media/G002_mpeg_frameq.h"      /* strlen (the MMI one, 0x00269A68) */

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The scratchpad context pointer (bootstrap gs_internal.h, gs_driver_internal.h): retail
 * loads 0x70003000 whole into a register (lui + ori) and never folds it
 * into the field offsets; under egcs a one-instruction asm in a statement
 * expression, GCC 14 gets the plain constant.  Same pointer value. */
#ifndef GS_SPR_CTX
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif
#endif

/* The draw buffer GsContext.tgtA as retail reads it: through a union, i.e.
 * in alias set 0, so a load of its offsets stays behind every earlier store
 * (font_set_start / font_set_pen / font_locate_f load yoff only after the
 * startX store; u113's flare_project shows the same behind its *sx store).
 * What union the original had is not known; this view has the header's
 * layout (as the bootstrap's view structs).  GCC 14 builds without strict
 * aliasing: the same code there. */
typedef union GsTargetView { GsRenderTarget t; } GsTargetView;
#define GS_TGT_A(ctx) (((GsTargetView *)&(ctx)->tgtA)->t)

/* EE COP1 max.s (the listing's max.s; the EE FPU has no NaNs).  Under egcs
 * a volatile asm: retail keeps the code around it in source order (the
 * lastChar store after the max.s in font_putc / font_measure_char), and the
 * font file has no -ffast-math to fold a ternary into max.s. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 u114_maxf(f32 a, f32 b)
{
    f32 r;
    __asm__ volatile("max.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
#else
static inline f32 u114_maxf(f32 a, f32 b) { return a > b ? a : b; }
#endif

/* EE COP1 rsqrt.s: a / sqrt(b).  A volatile asm, as in the other units
 * (u032's icon_rsqrt, u055, u060): retail never pads an rsqrt.s with the
 * hazard nops the compiler's own pattern gets, and keeps its operands in
 * the registers a volatile asm gives (vec3_normalize exact only so). */
static inline f32 u114_rsqrt(f32 a, f32 b)
{
    f32 r;
    __asm__ volatile("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}

/* The VU0 clip flags of a point behind the eye (w < 0) are remapped (the
 * two flags of each axis swapped and inverted); then the 6 flags of the
 * point.  A statement macro: the retail code has its loop notes (the label
 * after the remap is aligned, a nop in front of it). */
#define VU0_CLIP_REMAP(clip, w) do {                                        \
        if ((w) < 0)                                                        \
            (clip) = (((clip) >> 1) | 0x2A) ^ (((clip) << 1) | 0x15);       \
        (clip) &= 0x3F;                                                     \
    } while (0)

/* ~a as the R5900 `not` (nor rd, rs, $zero): egcs's one_cmpl template is
 * `nor rd, $zero, rs` (bootstrap gs_texture.cpp's ee_not64). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline u64 u114_not64(u64 a)
{
    u64 r;
    __asm__("not %0, %1" : "=r"(r) : "r"(a));
    return r;
}
#else
static inline u64 u114_not64(u64 a) { return ~a; }
#endif

/* An unaligned u64 (egcs: uld = ldl / ldr): font_locate_3d loads a point's
 * x, y with it. */
typedef struct U64Unaligned { u64 v; } __attribute__((packed)) U64Unaligned;

/* The packet pointer of the VU1 kick header (strip_batch_end, tri_draw16):
 * retail keeps it in $a2 in both and writes word by word through it, each
 * increment explicit (sw 0($a2); addiu $a2, 4).  egcs does that for a
 * register variable bound to a hard register (CSE records no equivalences
 * for hard registers, so the stores are not folded into offsets from the
 * tag pointer); a pseudo gets the offsets.  Whether the original pinned it
 * is not known (an exact form; it computes the same).  GCC 14: a plain
 * register variable. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define U114_IN_A2 __asm__("$6")
#else
#define U114_IN_A2
#endif

/* pref 0, 0x40(p) */
static inline void u114_pref40(const void *p)
{
    __asm__ volatile("pref 0, 0x40(%0)" : : "r"(p));
}

/* The kerning between two glyphs, the MMI loop shared by font_putc and
 * font_measure_char.  In C terms:
 *     k = 0x80;
 *     for (base = 0; rows > 0; rows -= 16, base += 16)
 *         for (i = 0; i < 16; i++) {
 *             s8 s = sat8(sat8(right[base + i] + left[base + i]) + (i >= rows ? 0x7F : 0));
 *             h = (i & 1) ? (u8)s : (s & 0x7F);     (psrlh 8 / pand 0x007F)
 *             k = min(k, h);                          (pminh, signed halfwords)
 *         }
 *     return k & 0x7F;
 * = the smallest gap between the previous glyph's right profile and this
 * glyph's left profile.  The loads run 16 bytes ahead (ldl/ldr, unaligned).
 *
 * Under egcs the loop is C and each MMI instruction (and each unaligned
 * qword-half load) a volatile asm of its own, so they stay in source order
 * while the compiler allocates the registers: retail's two copies differ
 * in every register but the three scratch qwords $2 / $3 / $5, which are
 * the same in both (register variables here).  The profile halves are
 * u128 (TImode: their allocation priority doubles, as retail's order
 * shows).  font_measure_char is exact with it.  GCC 14 keeps the whole loop
 * as one asm block (the same instructions, fixed registers). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define MMI_OP2(op, d, a, b) __asm__ volatile(#op " %0, %1, %2" : "=r"(d) : "r"(a), "r"(b))
#define MMI_OP1(op, d, a)    __asm__ volatile(#op " %0, %1" : "=r"(d) : "r"(a))
#define MMI_SHI(op, d, a, s) __asm__ volatile(#op " %0, %1, %2" : "=r"(d) : "r"(a), "i"(s))
#define MMI_MTSAB(n)         __asm__ volatile("mtsab %0, 0" : : "r"(n))
/* d = the unaligned 8 bytes at p + o */
#define MMI_ULD(d, p, o)     __asm__ volatile("ldl %0, %2(%1)\n\tldr %0, %3(%1)" \
                                              : "=r"(d) : "r"(p), "i"((o) + 7), "i"(o))

static inline int u114_glyph_kerning(const s8 *left, const s8 *right, s32 rows)
{
    u128 zero, mask, sat, k;
    u128 l0, l1, r0, r1;                        /* the profiles, 8 bytes each */
    register u128 v asm("$2");
    register u128 pad asm("$3");
    register u128 t asm("$5");
    int kern;

    zero = 0;
    k = 0x80;
    MMI_ULD(l0, left, 0);
    MMI_OP2(pnor, mask, zero, zero);
    MMI_ULD(l1, left, 8);
    MMI_SHI(psrlh, mask, mask, 9);              /* 0x007F per halfword */
    MMI_ULD(r0, right, 0);
    MMI_OP2(ppacb, sat, mask, mask);            /* 0x7F per byte */
    MMI_ULD(r1, right, 8);
    while (rows > 0) {
        int n = 16 - rows;                      /* rows past the end of the profile */

        if (n < 0)
            n = 0;
        right += 16;
        rows -= 16;
        MMI_MTSAB(n);
        left += 16;
        MMI_OP2(qfsrv, pad, sat, zero);         /* 0x7F in the top n bytes */
        MMI_OP2(pcpyld, t, l1, l0);
        MMI_OP2(pcpyld, v, r1, r0);
        MMI_ULD(l0, left, 0);
        MMI_OP2(paddsb, v, v, t);
        MMI_OP2(paddsb, v, v, pad);
        MMI_ULD(l1, left, 8);
        MMI_SHI(psrlh, t, v, 8);
        MMI_OP2(pand, v, v, mask);
        MMI_ULD(r0, right, 0);
        MMI_OP2(pminh, v, v, t);
        MMI_OP1(prevh, t, v);
        MMI_ULD(r1, right, 8);
        MMI_OP2(pminh, v, v, t);
        MMI_OP2(pcpyud, t, v, v);
        MMI_OP2(pminh, v, v, t);
        MMI_SHI(psrlw, t, v, 16);
        MMI_OP2(pminh, v, v, t);
        MMI_OP2(pminh, k, k, v);
    }
    __asm__ volatile("andi %0, %1, 0x7f" : "=r"(kern) : "r"(k));
    return kern;
}
#else
static inline int u114_glyph_kerning(const s8 *left, const s8 *right, s32 rows)
{
    int k;
    /* %0 k, %1 left, %2 right, %3 rows (positional: egcs has no named
     * asm operands; and inlined here egcs cannot reload "+r" operands, so
     * left / right / rows are outputs with matching inputs) */
    __asm__ volatile(
        "por     $25, $0, $0\n\t"
        "por     %0, $0, $0\n\t"
        "addiu   %0, $0, 0x80\n\t"
        "ldl     $13, 7(%1)\n\t"
        "ldr     $13, 0(%1)\n\t"
        "pnor    $9, $25, $25\n\t"
        "ldl     $12, 15(%1)\n\t"
        "ldr     $12, 8(%1)\n\t"
        "psrlh   $9, $9, 9\n\t"
        "ldl     $10, 7(%2)\n\t"
        "ldr     $10, 0(%2)\n\t"
        "ppacb   $24, $9, $9\n\t"
        "ldl     $7, 15(%2)\n\t"
        "ldr     $7, 8(%2)\n\t"
        "blez    %3, 2f\n\t"
        "1:\n\t"
        "addiu   $2, $0, 16\n\t"
        "addiu   $3, $0, -1\n\t"
        "subu    $2, $2, %3\n\t"
        "addiu   %2, %2, 16\n\t"
        "slt     $3, $3, $2\n\t"
        "addiu   %3, %3, -16\n\t"
        "movz    $2, $0, $3\n\t"
        "mtsab   $2, 0\n\t"
        "addiu   %1, %1, 16\n\t"
        "qfsrv   $3, $24, $25\n\t"
        "pcpyld  $5, $12, $13\n\t"
        "pcpyld  $2, $7, $10\n\t"
        "ldl     $13, 7(%1)\n\t"
        "ldr     $13, 0(%1)\n\t"
        "paddsb  $2, $2, $5\n\t"
        "paddsb  $2, $2, $3\n\t"
        "ldl     $12, 15(%1)\n\t"
        "ldr     $12, 8(%1)\n\t"
        "psrlh   $5, $2, 8\n\t"
        "pand    $2, $2, $9\n\t"
        "ldl     $10, 7(%2)\n\t"
        "ldr     $10, 0(%2)\n\t"
        "pminh   $2, $2, $5\n\t"
        "prevh   $5, $2\n\t"
        "ldl     $7, 15(%2)\n\t"
        "ldr     $7, 8(%2)\n\t"
        "pminh   $2, $2, $5\n\t"
        "pcpyud  $5, $2, $2\n\t"
        "pminh   $2, $2, $5\n\t"
        "psrlw   $5, $2, 16\n\t"
        "pminh   $2, $2, $5\n\t"
        "pminh   %0, %0, $2\n\t"
        "bgtz    %3, 1b\n\t"
        "2:\n\t"
        "andi    %0, %0, 0x7F\n\t"
        : "=&r"(k), "=r"(left), "=r"(right), "=r"(rows)
        : "1"(left), "2"(right), "3"(rows)
        : "$2", "$3", "$5", "$7", "$9", "$10", "$12", "$13", "$24", "$25", "memory");
    return k;
}
#endif

/* ======================================================================== */
/* Packet constants                                                         */
/* ======================================================================== */

/* GsContext.dirty bits: GS_DIRTY_* (G001_flare.h).  The GS_REG_* bits
 * first..last, and such a mask for context 2 / both: */
#define GS_REG_BITS(first, last) (((2ull << (last)) - 1) & ~((1ull << (first)) - 1))
#define GS_DIRTY_CTX2_BITS(m)   ((u64)(m) << 16)
#define GS_DIRTY_BOTH_BITS(m)   ((u64)(m) * 0x10001ull)

/* DMA tags and VIF codes (VIF_UNPACK: FLG, addresses relative to TOPS, USN;
 * VIF_UNPACK_M: with the STMASK write mask): G001_flare.h.  VIF_UNPACK_S is
 * signed (no USN). */
#define VIF_UNPACK_S(fmt, num, addr) (((u32)(fmt) << 24) | ((u32)(num) << 16) | 0x8000u | (addr))
enum {
    VIF_S_8   = 0x62,           /* (0x72 with the mask bit) */
    VIF_V2_16 = 0x65,
    VIF_V2_32 = 0x64,
    VIF_V3_32 = 0x68,
    VIF_V4_16 = 0x6D,
    VIF_V4_8  = 0x6E
};

/* The strip batcher's STMASK: x, y, z of every row write-protected, only w
 * written (0x3F per row), so the strip lengths (UNPACK S-8 to 0x40) land in
 * the w of the st vectors. */
#define STRIP_STMASK            0x3F3F3F3Fu

/* GIF tag low word: NLOOP | EOP (the other fields: G001_flare.h). */
#define GIF_TAG_LO(nloop, eop)  ((u64)(u32)((nloop) | ((eop) ? GIF_EOP : 0)))
/* A+D packed tag: NREG 1 (REGS = 0xE). */
#define GIF_AD_TAG(nloop, eop)  (GIF_NREG(1) | GIF_TAG_LO(nloop, eop))

/* GS register descriptors (GIF REGS nibbles). */
enum {
    GIF_REG_PRIM  = 0x0,
    GIF_REG_RGBAQ = 0x1,
    GIF_REG_ST    = 0x2,
    GIF_REG_UV    = 0x3,
    GIF_REG_XYZF2 = 0x4,
    GIF_REG_XYZF3 = 0xC
};

/* GS register addresses written through A+D. */
enum {
    GS_TEX0_1 = 0x06, GS_TEX0_2 = 0x07, GS_CLAMP_1 = 0x08, GS_CLAMP_2 = 0x09,
    GS_TEX1_1 = 0x14, GS_TEX1_2 = 0x15, GS_TEXFLUSH = 0x3F,
    GS_ALPHA_1 = 0x42, GS_ALPHA_2 = 0x43, GS_TEST_1 = 0x47, GS_TEST_2 = 0x48
};

/* PRIM: primitive types and attribute bits (GsContext.primAttr holds the
 * attributes). */
enum {
    GS_PRIM_TRIANGLE       = 3,
    GS_PRIM_TRIANGLE_STRIP = 4,
    GS_PRIM_TRIANGLE_FAN   = 5,
    GS_PRIM_SPRITE         = 6,
    GS_PRIM_IIP            = 0x008,
    GS_PRIM_TME            = 0x010,
    GS_PRIM_ABE            = 0x040,
    GS_PRIM_FST            = 0x100,
    GS_PRIM_CTXT2          = 0x200
};

/* The packed GS colour word's Q (1.0f) and ALPHA (Cs - Cd) * As + Cd. */
#define GS_Q_ONE                0x3F800000u
#define GS_ALPHA_BLEND          0x44u

/* The REGLIST registers of one text sprite: UV, XYZF3, UV, XYZF2 (0x43C3). */
#define FONT_SPRITE_REGS        ((u64)(GIF_REG_UV | (GIF_REG_XYZF3 << 4) | (GIF_REG_UV << 8) | \
                                       (GIF_REG_XYZF2 << 12)))

/* The REGLIST GIF tag of a closed sprite run (n = NLOOP | EOP), as a
 * statement macro: font_flush is exact only with its loop notes (the tag
 * is computed before them, the run pointer cleared after them). */
#define FONT_PUT_RUN_TAG(q, n) do {                                         \
        (q)[1] = FONT_SPRITE_REGS;                                          \
        (q)[0] = (GIF_NREG(4) | GIF_FLG_REGLIST) | (s32)(n);                 \
    } while (0)

/* GS register values built as Sony's libgraph SCE_GS_SET_* macros build
 * them: the term order decides egcs's constant folding (font_put_kanji's
 * TEX0_2 / CLAMP_2 keep each constant term apart). */
#define GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld)       \
    ((u64)(tbp) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | ((u64)(tw) << 26) |    \
     ((u64)(th) << 30) | ((u64)(tcc) << 34) | ((u64)(tfx) << 35) |                 \
     ((u64)(cbp) << 37) | ((u64)(cpsm) << 51) | ((u64)(csm) << 55) |               \
     ((u64)(csa) << 56) | ((u64)(cld) << 61))
#define GS_SET_CLAMP(wms, wmt, minu, maxu, minv, maxv)                              \
    ((u64)(wms) | ((u64)(wmt) << 2) | ((u64)(minu) << 4) | ((u64)(maxu) << 14) |   \
     ((u64)(minv) << 24) | ((u64)(maxv) << 34))
#define GS_REGION_CLAMP 2

/* GS pixel storage formats (TEX0.PSM, BITBLTBUF.DPSM). */
enum {
    GS_PSMCT32 = 0x00, GS_PSMCT24 = 0x01, GS_PSMCT16 = 0x02, GS_PSMCT16S = 0x0A,
    GS_PSMT8 = 0x13, GS_PSMT4 = 0x14, GS_PSMT8H = 0x1B, GS_PSMT4HL = 0x24, GS_PSMT4HH = 0x2C,
    GS_PSMZ32 = 0x30, GS_PSMZ24 = 0x31, GS_PSMZ16 = 0x32, GS_PSMZ16S = 0x3A
};

/* VU1 data addresses of the strip / triangle programs (strip_batch_end,
 * tri_draw16): vertices at 0 (xyz), 0x40 (st, also the strip lengths), 0x80
 * (rgba), the GIF tag at 0xC0; the matrices (pgl_send_matrix) at 0xC1, the
 * dirty GS registers (pgl_send_dirty_regs) at 0xD9.  Each of those two
 * uploads moves the MSCAL entry back by 1 / 5 instructions (? into VU1 code
 * that takes the uploaded block first; the microcode was not read). */
#define VU1_STRIP_ST            0x40
#define VU1_STRIP_RGBA          0x80
#define VU1_STRIP_GIFTAG        0xC0
#define VU1_MATRIX_ADDR         0xC1
#define VU1_REGS_ADDR           0xD9
#define VU1_MAX_VERTS           64

/* texregs_set_mode fields: gfx_tex_param's PGL_TEXPARAM_* (G001_flare.h). */

/* GS TEX0 as bitfields (libgraph's sceGsTex0 layout): texregs_set_size
 * stores TW / TH through it (the retail mask / shift order is the bitfield
 * expansion's). */
typedef struct GsTex0Bits {
    u64 tbp0 : 14;
    u64 tbw  : 6;
    u64 psm  : 6;
    u64 tw   : 4;
    u64 th   : 4;
    u64 tcc  : 1;
    u64 tfx  : 2;
    u64 cbp  : 14;
    u64 cpsm : 4;
    u64 csm  : 1;
    u64 csa  : 5;
    u64 cld  : 3;
} GsTex0Bits;

/* ======================================================================== */
/* Fonts (local to the unit)                                                */
/* ======================================================================== */

/* KanjiFont and the kanji font file KanjiFontFile: G001_flare.h. */

/* A page of the two-byte code map (16 at 0x002F7240): codes 0x80xx..0x8Fxx. */
typedef struct KanaPage {
    u8         first;       /* 0x00 */
    u8         last;        /* 0x01 */
    u16        unk_02;      /* 0x02 */
    const u16 *map;         /* 0x04 second byte - first -> font code */
} KanaPage;

/* Font parameters of font_set_param / font_get_param */
#define FONT_PARAM_SCALE_X    0x1100
#define FONT_PARAM_SCALE_Y    0x1101
#define FONT_PARAM_SPACING_X  0x1102
#define FONT_PARAM_SPACING_Y  0x1103

/* GsLibState.align: FONT_ALIGN_* (G001_flare.h). */

/* Codes: >= 0x80 two bytes; a first byte >= 0xA1 is a kanji (JIS). */
#define FONT_KANJI_FIRST      0xA1

/* mat_scale_set kinds: gs_set_material_param's PGL_MAT_* (G001_flare.h);
 * the GsLibState.matScale rows: */
enum { MATSCALE_AMBIENT = 0, MATSCALE_DIFFUSE = 1, MATSCALE_SPECULAR = 2, MATSCALE_EMISSION = 3 };

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

/* Only this unit uses them (descriptive names; the address is the symbol). */
extern KanaPage    g_kanaPages[16];         /* 0x002F7240 .data, the two-byte code pages 0x80..0x8F (font.c) */

/* the VU1 strip batcher (render.c; its .sbss / .bss state,
 * 0x00353274..0x00353280 and 0x004A6880, are that file's statics) */
extern u8   g_stripBatchOpen;               /* 0x003530B4 gp-0x493C, a batch is open */

/* texture / material selection (texmat_select; the tables, 0x00353284..
 * 0x00353290, are render.c's statics) */
extern s32         g_texselTex;             /* 0x003530B8 gp-0x4938, current texture index */
extern s32         g_texselMat;             /* 0x003530BC gp-0x4934, current material index */
extern s32         g_texselTexBase2;        /* 0x003530C0 gp-0x4930, texture index into table 2 */
extern s32         g_texselMatBase2;        /* 0x003530C4 gp-0x492C, material index into table 2 */

/* ======================================================================== */
/* Functions of the unit that G001_flare.h does not declare (no callers     */
/* outside the unit)                                                        */
/* ======================================================================== */

/* font.c */
void ring_draw_full(u32 colorInner, u32 colorOuter, f32 width);       /* 0x00249B28 */
void font_set_start(s32 x, s32 y);                                    /* 0x00249EF0 */
void font_set_pen(s32 x, s32 y);                                      /* 0x0024A060 */
int  font_put_kanji(int code);                                        /* 0x0024A6B0 */
int  kana_page_lookup(u32 page, int c);                               /* 0x0024B080 */

/* fontprint.c */
int  sprintf_putc(int c);                                             /* 0x0024B3A0 */

/* vecmath.c */
void vec3_normalize(f32 *out, f32 x, f32 y, f32 z);                   /* 0x0024B5B0 */
void vec3_cross(f32 *out, const f32 *a, const f32 *b);                /* 0x0024B5F0 */

/* render.c */
u32  vu0_clip_point(u64 x, u64 y, u64 z);                             /* 0x0024BF10 the float bits, zero-extended */
void mat_scale_set_rgb(u32 kind, f32 r, f32 g, f32 b);                /* 0x0024C0B0 */
void mat_scale_set(u32 kind, f32 r, f32 g, f32 b, f32 a);             /* 0x0024C0C0 */
void texsel_reset(void);                                              /* 0x0024CED0 */

/* fontfmt.c */
void font_flush(void);                                                /* 0x0024D128 */
int  font_measure_kanji(int code);                                    /* 0x0024D1C8 */
int  font_measure_char(int c);                                        /* 0x0024D230 */

#endif /* U114_H */
