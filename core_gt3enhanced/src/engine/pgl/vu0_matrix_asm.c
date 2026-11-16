/* vu0_matrix_asm.c - GT3 main executable, unit u117,
 * 0x00257A44..0x00257EF4: the hand-written VU0 matrix routines of the
 * graphics library (see pgl_internal.h for the register conventions and the VU0
 * micro subroutines).
 *
 * Hand-written assembly in the original: the block starts at 0x00257A44,
 * 4 bytes past an 8-byte boundary (egcs aligns every function it compiles
 * to 8), the routines use $a4 / $a5 ($t0 / $t1 in o32 names) as scratch,
 * save $ra with sq, share tails (pgl_rotate jumps to vu0_mult_matrix_regs,
 * pgl_mult_matrix / pgl_mult_matrix_90 branch to their vcallms tails) and
 * the last one (F_00257EF0) falls through into the next routine.
 * The matching build (VU0M_RETAIL_ASM) keeps these routines as the retail
 * instructions in top-level asm blocks (numeric registers, delay slots
 * explicit, `$L<fn>_<pc>' labels after the retail address), as u118 does
 * for the rest of the module; the C under #else (GCC 14, -DNON_MATCHING)
 * is the equivalent with the VU0 instructions as inline asm.  The three
 * identity / reset routines compile to the retail code from C, and
 * F_00257EF0 stays C in both builds (see there).
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/pgl_internal.h"

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define VU0M_RETAIL_ASM 1
#define VU0M_BEGIN(name) \
    "\t.text\n\t.p2align\t3\n\t.set\tpush\n\t.set\tnoreorder\n" VU0M_ENTRY(name)
#define VU0M_ENTRY(name) \
    "\t.globl\t" #name "\n\t.ent\t" #name "\n\t.type\t" #name ", @function\n" #name ":\n"
#define VU0M_SIZE(name)  "\t.end\t" #name "\n\t.size\t" #name ", .-" #name "\n"
#define VU0M_END         "\t.set\tpop\n\t.previous\n"
/* GsContext.matrixDirty = 1 (the scratchpad context at 0x70003000). */
#define VU0M_SET_DIRTY \
    "\tlui\t$2, 0x7000\n" \
    "\tori\t$2, $2, 0x3000\n" \
    "\taddiu\t$3, $0, 0x1\n" \
    "\tsb\t$3, 0x0($2)\n"
/* A micro subroutine call, then matrixDirty = 1 and return (the sb in the
 * jr delay slot): the tails at 0x00257D7C and 0x00257E18. */
#define VU0M_SET_DIRTY_VCALLMS(ms) \
    "\tlui\t$2, 0x7000\n" \
    "\tori\t$2, $2, 0x3000\n" \
    "\taddiu\t$3, $0, 0x1\n" \
    "\tvcallms\t" ms "\n" \
    "\tjr\t$31\n" \
    "\tsb\t$3, 0x0($2)\n"
/* The unaligned load of row i of m ($4) into $a4 and on to vf (the last
 * qmtc2.ni of a function sits in its jr / b delay slot: written there). */
#define VU0M_LOAD_ROW(off0, off7, off8, off15) \
    "\tldl\t$8, " off7 "($4)\n" \
    "\tldr\t$8, " off0 "($4)\n" \
    "\tldl\t$9, " off15 "($4)\n" \
    "\tldr\t$9, " off8 "($4)\n" \
    "\tpcpyld\t$8, $9, $8\n"
/* The unaligned store of $a5 (a row read with qmfc2) to m ($4). */
#define VU0M_STORE_ROW(off0, off7, off8, off15) \
    "\tsdl\t$9, " off7 "($4)\n" \
    "\tsdr\t$9, " off0 "($4)\n" \
    "\tpcpyud\t$9, $9, $9\n" \
    "\tsdl\t$9, " off15 "($4)\n" \
    "\tsdr\t$9, " off8 "($4)\n"
#endif

/* Loads vf1..vf4 (or vf28..vf31) from a 4x4 float matrix: lqc2 when it is
 * qword aligned, else unaligned dword loads + pcpyld + qmtc2.            */
#define VU0_LOADQ_UNALIGNED(m, i, vf)                                       \
    do {                                                                    \
        u64 lo_, hi_, t_;                                                   \
        memcpy(&lo_, (const u8 *)(m) + (i) * 16, 8);                        \
        memcpy(&hi_, (const u8 *)(m) + (i) * 16 + 8, 8);                    \
        __asm__ __volatile__(                                               \
            "pcpyld %0, %1, %2\n\t"                                         \
            "qmtc2  %0, " vf "\n\t"                                         \
            : "=&r"(t_) : "r"(hi_), "r"(lo_));                              \
    } while (0)

/* The float bits of pi (the reduction subtracts them under a sign mask). */
#define F32_PI_BITS     0x40490FDBu

/* sin / cos of an angle in degrees, left in vf1 = (sin, cos, ?, ?): both
 * lanes are reduced to an argument in [-pi/2, pi/2] and the VU0 micro
 * subroutine 0x1D0 (VU0_MS_SIN, the Taylor polynomial of sin to x^9)
 * evaluates them:
 *   y lane: r = (90 - a) * pi/180 - (a < 0 ? pi : 0) + k * pi, k =
 *           trunc(a / 180), negated when (k + (a < 0 ? -1 : 0)) is odd:
 *           sin(r) = cos(a);
 *   x lane: the same for a - 90: cos(a - 90) = sin(a).
 * (z / w: the sign-extension words of the two GPRs.)  The result is read
 * with qmfc2.i (pgl_sin_deg ...).  8 callers.
 * (Retail: the two lanes interleaved by hand, $a4 / $a5 scratch, the
 * trapping `add' for the parity, cvt.w.s truncation.) */
// FUNC 0x00257A44 vu0_sincos_deg
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_sincos_deg)
"\tlui\t$8, 0x42B4\n"
"\tori\t$8, $8, 0x0\n"
"\tlui\t$9, 0x3BB6\n"
"\tmtc1\t$8, $f0\n"
"\tori\t$9, $9, 0xB61\n"
"\tlui\t$2, 0x3C8E\n"
"\tsub.s\t$f13, $f12, $f0\n"
"\tmtc1\t$9, $f1\n"
"\tlui\t$8, 0x4049\n"
"\tori\t$2, $2, 0xFA35\n"
"\tori\t$8, $8, 0xFDB\n"
"\tmtc1\t$2, $f8\n"
"\tmul.s\t$f2, $f12, $f1\n"
"\tmfc1\t$2, $f12\n"
"\tmul.s\t$f3, $f13, $f1\n"
"\tmfc1\t$3, $f13\n"
"\tsub.s\t$f12, $f0, $f12\n"
"\tsra\t$2, $2, 31\n"
"\tsub.s\t$f13, $f0, $f13\n"
"\tsra\t$3, $3, 31\n"
"\tcvt.w.s\t$f2, $f2\n"
"\tand\t$4, $2, $8\n"
"\tcvt.w.s\t$f3, $f3\n"
"\tand\t$5, $3, $8\n"
"\tmul.s\t$f12, $f12, $f8\n"
"\tmtc1\t$4, $f6\n"
"\tmul.s\t$f13, $f13, $f8\n"
"\tmtc1\t$5, $f7\n"
"\tcvt.s.w\t$f4, $f2\n"
"\tmfc1\t$4, $f2\n"
"\tcvt.s.w\t$f5, $f3\n"
"\tmfc1\t$5, $f3\n"
"\tsub.s\t$f12, $f12, $f6\n"
"\tmtc1\t$8, $f0\n"
"\tsub.s\t$f13, $f13, $f7\n"
"\tlui\t$8, 0x3F80\n"
"\tmul.s\t$f4, $f4, $f0\n"
"\tadd\t$4, $4, $2\n"
"\tmul.s\t$f5, $f5, $f0\n"
"\tadd\t$5, $5, $3\n"
"\tandi\t$4, $4, 0x1\n"
"\tandi\t$5, $5, 0x1\n"
"\tsll\t$4, $4, 31\n"
"\tsll\t$5, $5, 31\n"
"\tadd.s\t$f12, $f12, $f4\n"
"\tor\t$4, $4, $8\n"
"\tadd.s\t$f13, $f13, $f5\n"
"\tor\t$5, $5, $8\n"
"\tmtc1\t$4, $f2\n"
"\tmtc1\t$5, $f3\n"
"\tmul.s\t$f12, $f12, $f2\n"
"\tmul.s\t$f13, $f13, $f3\n"
"\tmfc1\t$8, $f12\n"
"\tmfc1\t$9, $f13\n"
"\tpextlw\t$8, $8, $9\n"
"\tqmtc2.i\t$8, $vf1\n"
"\tvcallms\t0x1D0\n"
"\tjr\t$31\n"
"\tnop\n"
VU0M_SIZE(vu0_sincos_deg)
VU0M_END);
#else
void vu0_sincos_deg(float deg)
{
    const f32 pi = u117_u2f(F32_PI_BITS);           /* 3.14159274 */
    const f32 degToRad = u117_u2f(0x3C8EFA35);      /* pi / 180 */
    const f32 inv180 = u117_u2f(0x3BB60B61);        /* 1 / 180 */
    f32 aCos = deg;                                 /* the lane giving cos(deg) */
    f32 aSin = deg - 90.0f;                         /* the lane giving sin(deg) */
    s32 halfTurnsCos = (s32)(aCos * inv180);        /* cvt.w.s: truncation */
    s32 halfTurnsSin = (s32)(aSin * inv180);
    s32 negCos = (s32)u117_f2u(aCos) >> 31;         /* -1 when negative */
    s32 negSin = (s32)u117_f2u(aSin) >> 31;
    f32 rCos, rSin;
    u64 tmp;

    rCos = (90.0f - aCos) * degToRad - u117_u2f((u32)negCos & F32_PI_BITS) + (f32)halfTurnsCos * pi;
    rSin = (90.0f - aSin) * degToRad - u117_u2f((u32)negSin & F32_PI_BITS) + (f32)halfTurnsSin * pi;
    rCos *= u117_u2f(((u32)((halfTurnsCos + negCos) & 1) << 31) | 0x3F800000);  /* +-1.0 */
    rSin *= u117_u2f(((u32)((halfTurnsSin + negSin) & 1) << 31) | 0x3F800000);
    __asm__ __volatile__(
        "pextlw     %0, %1, %2\n\t"             /* (rSin, rCos, ...) */
        "qmtc2.i    %0, $vf1\n\t"
        "vcallms    " VU0_MS_SIN "\n\t"
        : "=&r"(tmp) : "r"((s64)(s32)u117_f2u(rCos)), "r"((s64)(s32)u117_f2u(rSin)));
}
#endif

/* Same machine code as the bootstrap's vu0_load_identity_regs
 * (0x0100E404, src/gs_driver.cpp, also hand-written there): vf28..vf31 =
 * identity rows via vsub / vmove / vmr32 / vaddw on vf0, then the matrices
 * are marked changed.  55 callers. */
// FUNC 0x00257B30 vu0_load_identity_regs
void vu0_load_identity_regs(void)
{
    GsContext *ctx = GS_CTX;

    __asm__ __volatile__(
        "vsub.xyzw  $vf29, $vf0, $vf0\n\t"
        "vsub.xyzw  $vf28, $vf0, $vf0\n\t"
        "vmove.xyzw $vf31, $vf0\n\t"
        "vmr32.xyzw $vf30, $vf0\n\t"
        "vaddw.y    $vf29, $vf0, $vf0w\n\t"
        "vaddw.x    $vf28, $vf0, $vf0w\n\t"
        : : : "memory");
    ctx->matrixDirty = 1;
}

/* The xyz of rows 0..2 to identity (rotation / scale reset; the w
 * components and the translation row vf31 stay): billboards.  Callers:
 * flare_draw_sun, flare_draw_sun_disc. */
// FUNC 0x00257B5C vu0_reset_rotation
void vu0_reset_rotation(void)
{
    GsContext *ctx = GS_CTX;

    __asm__ __volatile__(
        "vsub.xyz   $vf29, $vf0, $vf0\n\t"
        "vsub.xyz   $vf28, $vf0, $vf0\n\t"
        "vmr32.xyz  $vf30, $vf0\n\t"
        "vaddw.y    $vf29, $vf29, $vf0w\n\t"
        "vaddw.x    $vf28, $vf28, $vf0w\n\t"
        : : : "memory");
    ctx->matrixDirty = 1;
}

/* vf31.xyz = 0 (translation reset).  7 callers. */
// FUNC 0x00257B84 vu0_reset_translation
void vu0_reset_translation(void)
{
    GsContext *ctx = GS_CTX;

    __asm__ __volatile__("vsub.xyz $vf31, $vf0, $vf0" : : : "memory");
    ctx->matrixDirty = 1;
}

/* glLoadMatrix: vf28..vf31 = m (16 floats, any alignment).  6 callers
 * (cameras). */
// FUNC 0x00257B9C pgl_load_matrix
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(pgl_load_matrix)
VU0M_SET_DIRTY
"\tandi\t$8, $4, 0xF\n"
"\tbnez\t$8, $Llm_7BD0\n"
"\tvnop\n"
"\tlqc2\t$vf28, 0x0($4)\n"
"\tlqc2\t$vf29, 0x10($4)\n"
"\tlqc2\t$vf30, 0x20($4)\n"
"\tlqc2\t$vf31, 0x30($4)\n"
"\tjr\t$31\n"
"\tnop\n"
"$Llm_7BD0:\n"
VU0M_LOAD_ROW("0x0", "0x7", "0x8", "0xF")
"\tqmtc2.ni\t$8, $vf28\n"
VU0M_LOAD_ROW("0x10", "0x17", "0x18", "0x1F")
"\tqmtc2.ni\t$8, $vf29\n"
VU0M_LOAD_ROW("0x20", "0x27", "0x28", "0x2F")
"\tqmtc2.ni\t$8, $vf30\n"
VU0M_LOAD_ROW("0x30", "0x37", "0x38", "0x3F")
"\tjr\t$31\n"
"\tqmtc2.ni\t$8, $vf31\n"
VU0M_SIZE(pgl_load_matrix)
VU0M_END);
#else
void pgl_load_matrix(const float *m)
{
    GsContext *ctx = GS_CTX;

    ctx->matrixDirty = 1;
    __asm__ __volatile__("vnop");
    if (((u32)m & 0xF) == 0) {
        __asm__ __volatile__(
            "lqc2   $vf28, 0x00(%0)\n\t"
            "lqc2   $vf29, 0x10(%0)\n\t"
            "lqc2   $vf30, 0x20(%0)\n\t"
            "lqc2   $vf31, 0x30(%0)\n\t"
            : : "r"(m) : "memory");
        return;
    }
    VU0_LOADQ_UNALIGNED(m, 0, "$vf28");
    VU0_LOADQ_UNALIGNED(m, 1, "$vf29");
    VU0_LOADQ_UNALIGNED(m, 2, "$vf30");
    VU0_LOADQ_UNALIGNED(m, 3, "$vf31");
}
#endif

/* glGetFloatv(current matrix): m = vf28..vf31 (any alignment).  22
 * callers. */
// FUNC 0x00257C34 pgl_get_matrix
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(pgl_get_matrix)
"\tandi\t$8, $4, 0xF\n"
"\tbnez\t$8, $Lgm_7C54\n"
"\tvnop\n"
"\tsqc2\t$vf28, 0x0($4)\n"
"\tsqc2\t$vf29, 0x10($4)\n"
"\tsqc2\t$vf30, 0x20($4)\n"
"\tjr\t$31\n"
"\tsqc2\t$vf31, 0x30($4)\n"
"$Lgm_7C54:\n"
"\tqmfc2.i\t$9, $vf28\n"
VU0M_STORE_ROW("0x0", "0x7", "0x8", "0xF")
"\tqmfc2.ni\t$9, $vf29\n"
VU0M_STORE_ROW("0x10", "0x17", "0x18", "0x1F")
"\tqmfc2.ni\t$9, $vf30\n"
VU0M_STORE_ROW("0x20", "0x27", "0x28", "0x2F")
"\tqmfc2.ni\t$9, $vf31\n"
VU0M_STORE_ROW("0x30", "0x37", "0x38", "0x3F")
"\tjr\t$31\n"
"\tnop\n"
VU0M_SIZE(pgl_get_matrix)
VU0M_END);
#else
void pgl_get_matrix(float *m)
{
    u64 lo, hi;

    __asm__ __volatile__("vnop");
    if (((u32)m & 0xF) == 0) {
        __asm__ __volatile__(
            "sqc2   $vf28, 0x00(%0)\n\t"
            "sqc2   $vf29, 0x10(%0)\n\t"
            "sqc2   $vf30, 0x20(%0)\n\t"
            "sqc2   $vf31, 0x30(%0)\n\t"
            : : "r"(m) : "memory");
        return;
    }
    __asm__ __volatile__("qmfc2.i %0, $vf28\n\tpcpyud %1, %0, %0" : "=&r"(lo), "=r"(hi));
    memcpy((u8 *)m + 0x00, &lo, 8);
    memcpy((u8 *)m + 0x08, &hi, 8);
    __asm__ __volatile__("qmfc2 %0, $vf29\n\tpcpyud %1, %0, %0" : "=&r"(lo), "=r"(hi));
    memcpy((u8 *)m + 0x10, &lo, 8);
    memcpy((u8 *)m + 0x18, &hi, 8);
    __asm__ __volatile__("qmfc2 %0, $vf30\n\tpcpyud %1, %0, %0" : "=&r"(lo), "=r"(hi));
    memcpy((u8 *)m + 0x20, &lo, 8);
    memcpy((u8 *)m + 0x28, &hi, 8);
    __asm__ __volatile__("qmfc2 %0, $vf31\n\tpcpyud %1, %0, %0" : "=&r"(lo), "=r"(hi));
    memcpy((u8 *)m + 0x30, &lo, 8);
    memcpy((u8 *)m + 0x38, &hi, 8);
}
#endif

/* glRotate(deg, x, y, z): builds the rotation about the axis (normalised
 * with vrsqrt) in vf1..vf4 (row-vector layout: c I + (1 - c) a a^T +
 * s [a]x transposed, vf4 = (0, 0, 0, 1)) and multiplies it into the
 * current matrix (tail jump to vu0_mult_matrix_regs: current = R x
 * current).  Caller: modelset_exec.
 * (Matching build: one asm block with vu0_mult_matrix_regs, pgl_mult_matrix
 * and pgl_mult_matrix_90, whose branches go between them.  Retail saves $ra
 * with sq and pops the frame before the VU0 code.) */
// FUNC 0x00257CBC pgl_rotate
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(pgl_rotate)
"\taddiu\t$29, $29, -0x20\n"
"\tsq\t$31, 0x0($29)\n"
"\tswc1\t$f13, 0x10($29)\n"
"\tswc1\t$f14, 0x14($29)\n"
"\tswc1\t$f15, 0x18($29)\n"
"\tjal\tvu0_sincos_deg\n"
"\tsw\t$0, 0x1C($29)\n"
"\tlq\t$31, 0x0($29)\n"
"\tvnop\n"
"\tlqc2\t$vf5, 0x10($29)\n"
"\taddiu\t$29, $29, 0x20\n"
"\tvmul.xyzw\t$vf2, $vf5, $vf5\n"
"\tvmulw.xyzw\t$vf6, $vf1, $vf0w\n"
"\tvmulx.xyzw\t$vf7, $vf5, $vf5x\n"
"\tvmulw.xyzw\t$vf4, $vf0, $vf0w\n"
"\tvaddy.x\t$vf2, $vf2, $vf2y\n"
"\tvsuby.w\t$vf6, $vf0, $vf6y\n"
"\tvmuly.xyzw\t$vf8, $vf5, $vf5y\n"
"\tvmr32.xyzw\t$vf3, $vf4\n"
"\tvaddz.x\t$vf2, $vf2, $vf2z\n"
"\tvmulz.xyzw\t$vf9, $vf5, $vf5z\n"
"\tvrsqrt\tQ, $vf0w, $vf2x\n"
"\tvsub.xyzw\t$vf10, $vf0, $vf0\n"
"\tvsub.xyzw\t$vf11, $vf0, $vf0\n"
"\tvsub.xyzw\t$vf12, $vf0, $vf0\n"
"\tvmr32.xyzw\t$vf2, $vf3\n"
"\tvaddz.y\t$vf10, $vf10, $vf5z\n"
"\tvsubz.x\t$vf11, $vf11, $vf5z\n"
"\tvaddy.x\t$vf12, $vf12, $vf5y\n"
"\tvmr32.xyzw\t$vf1, $vf2\n"
"\tvsuby.z\t$vf10, $vf10, $vf5y\n"
"\tvaddx.z\t$vf11, $vf11, $vf5x\n"
"\tvsubx.y\t$vf12, $vf12, $vf5x\n"
"\tvwaitq\n"
"\tvmulq.xw\t$vf6, $vf6, Q\n"
"\tvmulq.xyzw\t$vf7, $vf7, Q\n"
"\tvmulq.xyzw\t$vf8, $vf8, Q\n"
"\tvmulq.xyzw\t$vf9, $vf9, Q\n"
"\tvmulay.xyzw\tACC, $vf1, $vf6y\n"
"\tvmaddaw.xyzw\tACC, $vf7, $vf6w\n"
"\tvmaddx.xyz\t$vf1, $vf10, $vf6x\n"
"\tvmulay.xyzw\tACC, $vf2, $vf6y\n"
"\tvmaddaw.xyzw\tACC, $vf8, $vf6w\n"
"\tvmaddx.xyz\t$vf2, $vf11, $vf6x\n"
"\tvmulay.xyzw\tACC, $vf3, $vf6y\n"
"\tvmaddaw.xyzw\tACC, $vf9, $vf6w\n"
"\tj\tvu0_mult_matrix_regs\n"
"\tvmaddx.xyz\t$vf3, $vf12, $vf6x\n"
VU0M_SIZE(pgl_rotate)
VU0M_ENTRY(vu0_mult_matrix_regs)
"$Lmm_7D7C:\n"
VU0M_SET_DIRTY_VCALLMS("0x0")
VU0M_SIZE(vu0_mult_matrix_regs)
VU0M_ENTRY(pgl_mult_matrix)
"\tandi\t$8, $4, 0xF\n"
"\tbnez\t$8, $Lmm_7DB4\n"
"\tvnop\n"
"\tlqc2\t$vf1, 0x0($4)\n"
"\tlqc2\t$vf2, 0x10($4)\n"
"\tlqc2\t$vf3, 0x20($4)\n"
"\tb\t$Lmm_7D7C\n"
"\tlqc2\t$vf4, 0x30($4)\n"
"$Lmm_7DB4:\n"
VU0M_LOAD_ROW("0x0", "0x7", "0x8", "0xF")
"\tqmtc2.ni\t$8, $vf1\n"
VU0M_LOAD_ROW("0x10", "0x17", "0x18", "0x1F")
"\tqmtc2.ni\t$8, $vf2\n"
VU0M_LOAD_ROW("0x20", "0x27", "0x28", "0x2F")
"\tqmtc2.ni\t$8, $vf3\n"
VU0M_LOAD_ROW("0x30", "0x37", "0x38", "0x3F")
"\tb\t$Lmm_7D7C\n"
"\tqmtc2.ni\t$8, $vf4\n"
"$Lmm_7E18:\n"                               /* pgl_mult_matrix_90's tail */
VU0M_SET_DIRTY_VCALLMS("0x90")
VU0M_SIZE(pgl_mult_matrix)
VU0M_ENTRY(pgl_mult_matrix_90)
"\tandi\t$8, $4, 0xF\n"
"\tbnez\t$8, $Lmm_7E50\n"
"\tvnop\n"
"\tlqc2\t$vf1, 0x0($4)\n"
"\tlqc2\t$vf2, 0x10($4)\n"
"\tlqc2\t$vf3, 0x20($4)\n"
"\tb\t$Lmm_7E18\n"
"\tlqc2\t$vf4, 0x30($4)\n"
"$Lmm_7E50:\n"
VU0M_LOAD_ROW("0x0", "0x7", "0x8", "0xF")
"\tqmtc2.ni\t$8, $vf1\n"
VU0M_LOAD_ROW("0x10", "0x17", "0x18", "0x1F")
"\tqmtc2.ni\t$8, $vf2\n"
VU0M_LOAD_ROW("0x20", "0x27", "0x28", "0x2F")
"\tqmtc2.ni\t$8, $vf3\n"
VU0M_LOAD_ROW("0x30", "0x37", "0x38", "0x3F")
"\tb\t$Lmm_7E18\n"
"\tqmtc2.ni\t$8, $vf4\n"
VU0M_SIZE(pgl_mult_matrix_90)
VU0M_END);
#else
void pgl_rotate(float deg, float x, float y, float z)
{
    f32 axis[4] __attribute__((aligned(16)));  /* sp+0x10 */

    axis[0] = x;
    axis[1] = y;
    axis[2] = z;
    ((u32 *)axis)[3] = 0;
    vu0_sincos_deg(deg);                        /* vf1 = (s, c) */
    __asm__ __volatile__(
        "vnop\n\t"
        "lqc2         $vf5, 0(%0)\n\t"          /* v = axis */
        "vmul.xyzw    $vf2, $vf5, $vf5\n\t"
        "vmulw.xyzw   $vf6, $vf1, $vf0w\n\t"    /* (s, c, ., .) */
        "vmulx.xyzw   $vf7, $vf5, $vf5x\n\t"    /* v * vx */
        "vmulw.xyzw   $vf4, $vf0, $vf0w\n\t"    /* (0, 0, 0, 1) */
        "vaddy.x      $vf2, $vf2, $vf2y\n\t"
        "vsuby.w      $vf6, $vf0, $vf6y\n\t"    /* w = 1 - c */
        "vmuly.xyzw   $vf8, $vf5, $vf5y\n\t"    /* v * vy */
        "vmr32.xyzw   $vf3, $vf4\n\t"           /* (0, 0, 1, 0) */
        "vaddz.x      $vf2, $vf2, $vf2z\n\t"    /* |v|^2 */
        "vmulz.xyzw   $vf9, $vf5, $vf5z\n\t"    /* v * vz */
        "vrsqrt       " VU0_Q ", $vf0w, $vf2x\n\t"     /* 1 / |v| */
        "vsub.xyzw    $vf10, $vf0, $vf0\n\t"
        "vsub.xyzw    $vf11, $vf0, $vf0\n\t"
        "vsub.xyzw    $vf12, $vf0, $vf0\n\t"
        "vmr32.xyzw   $vf2, $vf3\n\t"           /* (0, 1, 0, 0) */
        "vaddz.y      $vf10, $vf10, $vf5z\n\t"
        "vsubz.x      $vf11, $vf11, $vf5z\n\t"
        "vaddy.x      $vf12, $vf12, $vf5y\n\t"
        "vmr32.xyzw   $vf1, $vf2\n\t"           /* (1, 0, 0, 0) */
        "vsuby.z      $vf10, $vf10, $vf5y\n\t"  /* (0, vz, -vy) */
        "vaddx.z      $vf11, $vf11, $vf5x\n\t"  /* (-vz, 0, vx) */
        "vsubx.y      $vf12, $vf12, $vf5x\n\t"  /* (vy, -vx, 0) */
        "vwaitq\n\t"
        "vmulq.xw     $vf6, $vf6, " VU0_Q "\n\t"       /* s / |v|, (1 - c) / |v| */
        "vmulq.xyzw   $vf7, $vf7, " VU0_Q "\n\t"
        "vmulq.xyzw   $vf8, $vf8, " VU0_Q "\n\t"
        "vmulq.xyzw   $vf9, $vf9, " VU0_Q "\n\t"
        "vmulay.xyzw  " VU0_ACC ", $vf1, $vf6y\n\t"
        "vmaddaw.xyzw " VU0_ACC ", $vf7, $vf6w\n\t"
        "vmaddx.xyz   $vf1, $vf10, $vf6x\n\t"
        "vmulay.xyzw  " VU0_ACC ", $vf2, $vf6y\n\t"
        "vmaddaw.xyzw " VU0_ACC ", $vf8, $vf6w\n\t"
        "vmaddx.xyz   $vf2, $vf11, $vf6x\n\t"
        "vmulay.xyzw  " VU0_ACC ", $vf3, $vf6y\n\t"
        "vmaddaw.xyzw " VU0_ACC ", $vf9, $vf6w\n\t"
        "vmaddx.xyz   $vf3, $vf12, $vf6x\n\t"
        : : "r"(axis) : "memory");
    vu0_mult_matrix_regs();                     /* j 0x00257D7C */
}
#endif

/* current = vf1..vf4 x current (VU0 micro subroutine 0x000, VU0_MS_MULT),
 * the matrices marked changed.  The shared tail of pgl_rotate and
 * pgl_mult_matrix; no callers of its own.  (Matching build: in the block
 * above.) */
// FUNC 0x00257D7C vu0_mult_matrix_regs
#ifndef VU0M_RETAIL_ASM
void vu0_mult_matrix_regs(void)
{
    GsContext *ctx = GS_CTX;

    __asm__ __volatile__("vcallms " VU0_MS_MULT : : : "memory");
    ctx->matrixDirty = 1;
}
#endif

/* glMultMatrix: vf1..vf4 = m (any alignment), then vu0_mult_matrix_regs
 * (branch): current = m x current, m applied first (in the object's
 * space).  The 6 instructions at 0x00257E18 inside this function's range
 * are pgl_mult_matrix_90's tail.  23 callers (cameras, pgl_frustum /
 * pgl_ortho ...).  (Matching build: in the block above.) */
// FUNC 0x00257D94 pgl_mult_matrix
#ifndef VU0M_RETAIL_ASM
void pgl_mult_matrix(const float *m)
{
    __asm__ __volatile__("vnop");
    if (((u32)m & 0xF) == 0) {
        __asm__ __volatile__(
            "lqc2   $vf1, 0x00(%0)\n\t"
            "lqc2   $vf2, 0x10(%0)\n\t"
            "lqc2   $vf3, 0x20(%0)\n\t"
            "lqc2   $vf4, 0x30(%0)\n\t"
            : : "r"(m) : "memory");
    } else {
        VU0_LOADQ_UNALIGNED(m, 0, "$vf1");
        VU0_LOADQ_UNALIGNED(m, 1, "$vf2");
        VU0_LOADQ_UNALIGNED(m, 2, "$vf3");
        VU0_LOADQ_UNALIGNED(m, 3, "$vf4");
    }
    vu0_mult_matrix_regs();
}
#endif

/* pgl_mult_matrix in the other order: vf1..vf4 = m, then the VU0 micro
 * subroutine 0x090 (VU0_MS_POSTMULT): current = current x m, m applied
 * after the current matrix (GL terms: M = m M, a pre-multiplication); then
 * the matrices are marked changed (the tail at 0x00257E18).  Callers:
 * RacePS2Base::drawCarReflections (the car shadow pass), CarRender::drawModel.
 * (Matching build: in the block above.) */
// FUNC 0x00257E30 pgl_mult_matrix_90
#ifndef VU0M_RETAIL_ASM
void pgl_mult_matrix_90(const float *m)
{
    GsContext *ctx;

    __asm__ __volatile__("vnop");
    if (((u32)m & 0xF) == 0) {
        __asm__ __volatile__(
            "lqc2   $vf1, 0x00(%0)\n\t"
            "lqc2   $vf2, 0x10(%0)\n\t"
            "lqc2   $vf3, 0x20(%0)\n\t"
            "lqc2   $vf4, 0x30(%0)\n\t"
            : : "r"(m) : "memory");
    } else {
        VU0_LOADQ_UNALIGNED(m, 0, "$vf1");
        VU0_LOADQ_UNALIGNED(m, 1, "$vf2");
        VU0_LOADQ_UNALIGNED(m, 2, "$vf3");
        VU0_LOADQ_UNALIGNED(m, 3, "$vf4");
    }
    ctx = GS_CTX;
    __asm__ __volatile__("vcallms " VU0_MS_POSTMULT : : : "memory");
    ctx->matrixDirty = 1;
}
#endif

/* glTranslate(x, y, z): vf31 += x vf28 + y vf29 + z vf30.  54 callers. */
// FUNC 0x00257EB4 pgl_translate
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(pgl_translate)
VU0M_SET_DIRTY
"\tmfc1\t$4, $f12\n"
"\tmfc1\t$5, $f13\n"
"\tmfc1\t$6, $f14\n"
"\tqmtc2.i\t$4, $vf1\n"
"\tqmtc2.ni\t$5, $vf2\n"
"\tqmtc2.ni\t$6, $vf3\n"
"\tvmulax.xyzw\tACC, $vf28, $vf1x\n"
"\tvmaddax.xyzw\tACC, $vf29, $vf2x\n"
"\tvmaddax.xyzw\tACC, $vf30, $vf3x\n"
"\tjr\t$31\n"
"\tvmaddw.xyzw\t$vf31, $vf31, $vf0w\n"
VU0M_SIZE(pgl_translate)
VU0M_END);
#else
void pgl_translate(float x, float y, float z)
{
    GsContext *ctx = GS_CTX;

    ctx->matrixDirty = 1;
    __asm__ __volatile__(
        "qmtc2.i      %0, $vf1\n\t"
        "qmtc2        %1, $vf2\n\t"
        "qmtc2        %2, $vf3\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf1x\n\t"
        "vmaddax.xyzw " VU0_ACC ", $vf29, $vf2x\n\t"
        "vmaddax.xyzw " VU0_ACC ", $vf30, $vf3x\n\t"
        "vmaddw.xyzw  $vf31, $vf31, $vf0w\n\t"
        : : "r"((s64)(s32)u117_f2u(x)), "r"((s64)(s32)u117_f2u(y)),
            "r"((s64)(s32)u117_f2u(z))
        : "memory");
}
#endif

/* A lone nop that falls through into vu0_scale_inv_y (0x00257EF4, u118: s
 * in $f12 untouched, the current matrix scaled by (1, 1 / s, 1) through
 * vu0_scale).  The 7 callers (CarsItem::drawAll, RacePS2Base::drawView
 * ...) call this address.
 * (Kept as C in both builds: retail's lone nop only works where the next
 * unit's code follows at 0x00257EF4; the hybrid link places this unit's
 * code elsewhere, so the tail jump to vu0_scale_inv_y stays.) */
// FUNC 0x00257EF0 F_00257EF0
void F_00257EF0(float s)
{
    vu0_scale_inv_y(s);                     /* fall-through */
}
