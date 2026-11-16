/* vu0_current_matrix.c - the VU0-resident current matrix of the graphics
 * library (G001), 0x00257EF4..0x00258150.
 *
 * The current matrix (of GsContext.matrixMode) lives in VU0 registers
 * vf28..vf31 (one row each, row vector convention: v' = v.x * vf28 + v.y *
 * vf29 + v.z * vf30 + v.w * vf31).  Every function that changes it sets
 * GsContext.matrixDirty (pgl_sync_matrices then rebuilds the combined
 * matrix).  vf0 is the VU0 constant (0, 0, 0, 1).
 *
 * The vu0_* routines are hand-written EE assembly in the original (the
 * module that starts in u117 at 0x00257A44, see vu0_matrix_asm.c):
 *   - vu0_scale_inv_y starts 4 bytes past an 8-byte boundary, has no jr and
 *     runs on into vu0_scale; its div.s lacks the two hazard nops egcs's
 *     divsf3 always emits (u124's notes: 6 such div.s in the image);
 *   - the rotations save $ra with sq / lq and pop the frame before their
 *     VU0 code, with the last VU0 instruction in the jr delay slot (egcs
 *     saves $ra with sd, and never moves the epilogue above an asm or puts
 *     an asm into a delay slot);
 *   - vu0_apply_matrix's unaligned-source path lies before its entry (in
 *     vu0_invert_rigid's range), and its first vnop sits in a bnez delay
 *     slot;
 *   - $a4..$a7 scratch, MMI / COP2 macro instructions throughout.
 * The matching build (VU0M_RETAIL_ASM) keeps them as the retail
 * instructions in top-level asm blocks (numeric registers, delay slots
 * explicit, `$L<fn>_<pc>' labels after the retail address), as u124's
 * fastmath does; the C under #else (GCC 14, -DNON_MATCHING) is the
 * equivalent with the VU0 instructions as inline asm.  pgl_set_scale and
 * pgl_set_normal_matrix compile to the retail code from C.
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/gs_driver_internal.h"

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
#endif

// FUNC 0x00257EF4 vu0_scale_inv_y
/* f14 = 1.0, f13 = 1.0 / s, f12 = 1.0, and no jr $ra: it runs on into
 * vu0_scale (0x00257F04), so row 1 of the matrix is divided by s.  No
 * calls; u117's F_00257EF0 (one instruction before) runs into it.  The
 * matching build has both routines in one asm block (the fall-through
 * stays a fall-through wherever the object is linked). */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_scale_inv_y)
"\tlui\t$2, 0x3F80\n"
"\tmtc1\t$2, $f14\n"
"\tdiv.s\t$f13, $f14, $f12\n"
"\tmtc1\t$2, $f12\n"
VU0M_SIZE(vu0_scale_inv_y)
VU0M_ENTRY(vu0_scale)
VU0M_SET_DIRTY
"\tmfc1\t$4, $f12\n"
"\tmfc1\t$5, $f13\n"
"\tmfc1\t$6, $f14\n"
"\tvnop\n"
"\tqmtc2.ni\t$4, $vf1\n"
"\tqmtc2.ni\t$5, $vf2\n"
"\tqmtc2.ni\t$6, $vf3\n"
"\tvmulx.xyzw\t$vf28, $vf28, $vf1x\n"
"\tvmulx.xyzw\t$vf29, $vf29, $vf2x\n"
"\tjr\t$31\n"
"\tvmulx.xyzw\t$vf30, $vf30, $vf3x\n"
VU0M_SIZE(vu0_scale)
VU0M_END);
#else
void vu0_scale_inv_y(f32 s)
{
    vu0_scale(1.0f, 1.0f / s, 1.0f);       /* falls through into 0x00257F04 */
}
#endif

// FUNC 0x00257F04 vu0_scale
/* glScalef: rows 0, 1, 2 times x, y, z.  (Matching build: in the block
 * above.) */
#ifndef VU0M_RETAIL_ASM
void vu0_scale(f32 x, f32 y, f32 z)
{
    GS_SPR_CTX->matrixDirty = 1;
    __asm__ __volatile__(
        "vnop\n\t"
        "qmtc2.ni    %0, $vf1\n\t"
        "qmtc2.ni    %1, $vf2\n\t"
        "qmtc2.ni    %2, $vf3\n\t"
        "vmulx.xyzw  $vf28, $vf28, $vf1x\n\t"
        "vmulx.xyzw  $vf29, $vf29, $vf2x\n\t"
        "vmulx.xyzw  $vf30, $vf30, $vf3x"
        : : "r"(u118_f2u(x)), "r"(u118_f2u(y)), "r"(u118_f2u(z)) : "memory");
}
#endif

// FUNC 0x00257F40 pgl_set_scale
/* GsContext.scale.xyz = (x, y, z) (w untouched) and matrixDirty: with
 * PGL_CAP_SCALE on, pgl_sync_matrices scales the first three columns of
 * the combined matrix by it.  Callers RaceTirePacket::draw,
 * MenuTireModel::draw.  (The flag written last in the source: egcs then
 * schedules its sb first, as retail; written first, sched1 puts the z
 * store ahead of it.) */
void pgl_set_scale(f32 x, f32 y, f32 z)
{
    GsContext *ctx = GS_SPR_CTX;

    ctx->scale.x = x;
    ctx->scale.y = y;
    ctx->scale.z = z;
    ctx->matrixDirty = 1;
}

// FUNC 0x00257F60 vu0_rotate_x
/* M = Rx(deg) * M: row1' = c*row1 + s*row2, row2' = c*row2 - s*row1.
 * vu0_sincos_deg leaves (sin, cos) of the angle in vf1.x / vf1.y;
 * matrixDirty is set before the call (jal delay slot). */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_rotate_x)
"\taddiu\t$29, $29, -0x10\n"
"\tsq\t$31, 0x0($29)\n"
"\tlui\t$2, 0x7000\n"
"\tori\t$2, $2, 0x3000\n"
"\taddiu\t$3, $0, 0x1\n"
"\tjal\tvu0_sincos_deg\n"
"\tsb\t$3, 0x0($2)\n"
"\tlq\t$31, 0x0($29)\n"
"\taddiu\t$29, $29, 0x10\n"
"\tvsubx.x\t$vf2, $vf0, $vf1x\n"
"\tvmulay.xyzw\tACC, $vf29, $vf1y\n"
"\tvmaddx.xyzw\t$vf3, $vf30, $vf1x\n"
"\tvmulay.xyzw\tACC, $vf30, $vf1y\n"
"\tvmaddx.xyzw\t$vf30, $vf29, $vf2x\n"
"\tjr\t$31\n"
"\tvmulw.xyzw\t$vf29, $vf3, $vf0w\n"
VU0M_SIZE(vu0_rotate_x)
VU0M_END);
#else
void vu0_rotate_x(f32 deg)
{
    GS_SPR_CTX->matrixDirty = 1;
    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "vsubx.x     $vf2, $vf0, $vf1x\n\t"         /* vf2.x = -sin */
        "vmulay.xyzw " VU0_ACC ", $vf29, $vf1y\n\t"
        "vmaddx.xyzw $vf3, $vf30, $vf1x\n\t"
        "vmulay.xyzw " VU0_ACC ", $vf30, $vf1y\n\t"
        "vmaddx.xyzw $vf30, $vf29, $vf2x\n\t"
        "vmulw.xyzw  $vf29, $vf3, $vf0w"
        : : : "memory");
}
#endif

// FUNC 0x00257FA0 vu0_rotate_y
/* M = Ry(deg) * M: row0' = c*row0 - s*row2, row2' = s*row0 + c*row2. */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_rotate_y)
"\taddiu\t$29, $29, -0x10\n"
"\tsq\t$31, 0x0($29)\n"
"\tlui\t$2, 0x7000\n"
"\tori\t$2, $2, 0x3000\n"
"\taddiu\t$3, $0, 0x1\n"
"\tjal\tvu0_sincos_deg\n"
"\tsb\t$3, 0x0($2)\n"
"\tlq\t$31, 0x0($29)\n"
"\taddiu\t$29, $29, 0x10\n"
"\tvsubx.x\t$vf2, $vf0, $vf1x\n"
"\tvmulax.xyzw\tACC, $vf28, $vf1x\n"
"\tvmaddy.xyzw\t$vf3, $vf30, $vf1y\n"
"\tvmulay.xyzw\tACC, $vf28, $vf1y\n"
"\tvmaddx.xyzw\t$vf28, $vf30, $vf2x\n"
"\tjr\t$31\n"
"\tvmulw.xyzw\t$vf30, $vf3, $vf0w\n"
VU0M_SIZE(vu0_rotate_y)
VU0M_END);
#else
void vu0_rotate_y(f32 deg)
{
    GS_SPR_CTX->matrixDirty = 1;
    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "vsubx.x     $vf2, $vf0, $vf1x\n\t"
        "vmulax.xyzw " VU0_ACC ", $vf28, $vf1x\n\t"
        "vmaddy.xyzw $vf3, $vf30, $vf1y\n\t"
        "vmulay.xyzw " VU0_ACC ", $vf28, $vf1y\n\t"
        "vmaddx.xyzw $vf28, $vf30, $vf2x\n\t"
        "vmulw.xyzw  $vf30, $vf3, $vf0w"
        : : : "memory");
}
#endif

// FUNC 0x00257FE0 vu0_rotate_z
/* M = Rz(deg) * M: row0' = c*row0 + s*row1, row1' = c*row1 - s*row0. */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_rotate_z)
"\taddiu\t$29, $29, -0x10\n"
"\tsq\t$31, 0x0($29)\n"
"\tlui\t$2, 0x7000\n"
"\tori\t$2, $2, 0x3000\n"
"\taddiu\t$3, $0, 0x1\n"
"\tjal\tvu0_sincos_deg\n"
"\tsb\t$3, 0x0($2)\n"
"\tlq\t$31, 0x0($29)\n"
"\taddiu\t$29, $29, 0x10\n"
"\tvsubx.x\t$vf2, $vf0, $vf1x\n"
"\tvmulay.xyzw\tACC, $vf28, $vf1y\n"
"\tvmaddx.xyzw\t$vf3, $vf29, $vf1x\n"
"\tvmulay.xyzw\tACC, $vf29, $vf1y\n"
"\tvmaddx.xyzw\t$vf29, $vf28, $vf2x\n"
"\tjr\t$31\n"
"\tvmulw.xyzw\t$vf28, $vf3, $vf0w\n"
VU0M_SIZE(vu0_rotate_z)
VU0M_END);
#else
void vu0_rotate_z(f32 deg)
{
    GS_SPR_CTX->matrixDirty = 1;
    vu0_sincos_deg(deg);
    __asm__ __volatile__(
        "vsubx.x     $vf2, $vf0, $vf1x\n\t"
        "vmulay.xyzw " VU0_ACC ", $vf28, $vf1y\n\t"
        "vmaddx.xyzw $vf3, $vf29, $vf1x\n\t"
        "vmulay.xyzw " VU0_ACC ", $vf29, $vf1y\n\t"
        "vmaddx.xyzw $vf29, $vf28, $vf2x\n\t"
        "vmulw.xyzw  $vf28, $vf3, $vf0w"
        : : : "memory");
}
#endif

// FUNC 0x00258020 vu0_transpose
/* Transposes vf28..vf31 through the GPRs (qmfc2, pextlw / pextuw, pcpyld /
 * pcpyud, qmtc2).  Caller RaceSmoke::draw (u058, 0x001A4F28). */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_transpose)
VU0M_SET_DIRTY
"\tvnop\n"
"\tqmfc2.ni\t$8, $vf28\n"
"\tqmfc2.ni\t$9, $vf29\n"
"\tqmfc2.ni\t$10, $vf30\n"
"\tqmfc2.ni\t$11, $vf31\n"
"\tpextlw\t$4, $9, $8\n"
"\tpextuw\t$5, $9, $8\n"
"\tpextlw\t$6, $11, $10\n"
"\tpextuw\t$7, $11, $10\n"
"\tpcpyld\t$8, $6, $4\n"
"\tpcpyud\t$9, $4, $6\n"
"\tpcpyld\t$10, $7, $5\n"
"\tpcpyud\t$11, $5, $7\n"
"\tqmtc2.ni\t$8, $vf28\n"
"\tqmtc2.ni\t$9, $vf29\n"
"\tqmtc2.ni\t$10, $vf30\n"
"\tjr\t$31\n"
"\tqmtc2.ni\t$11, $vf31\n"
VU0M_SIZE(vu0_transpose)
VU0M_END);
#else
void vu0_transpose(void)
{
    GS_SPR_CTX->matrixDirty = 1;
    __asm__ __volatile__(
        "vnop\n\t"
        "qmfc2.ni    $8, $vf28\n\t"
        "qmfc2.ni    $9, $vf29\n\t"
        "qmfc2.ni    $10, $vf30\n\t"
        "qmfc2.ni    $11, $vf31\n\t"
        "pextlw      $4, $9, $8\n\t"
        "pextuw      $5, $9, $8\n\t"
        "pextlw      $6, $11, $10\n\t"
        "pextuw      $7, $11, $10\n\t"
        "pcpyld      $8, $6, $4\n\t"
        "pcpyud      $9, $4, $6\n\t"
        "pcpyld      $10, $7, $5\n\t"
        "pcpyud      $11, $5, $7\n\t"
        "qmtc2.ni    $8, $vf28\n\t"
        "qmtc2.ni    $9, $vf29\n\t"
        "qmtc2.ni    $10, $vf30\n\t"
        "qmtc2.ni    $11, $vf31"
        : : : "$4", "$5", "$6", "$7", "$8", "$9", "$10", "$11", "memory");
}
#endif

// FUNC 0x00258078 vu0_invert_rigid
/* Inverse of a rigid transform: the 3x3 part transposed (w of rows 0..2
 * becomes 0), translation row vf31.xyz = -(t.x*row0 + t.y*row1 + t.z*row2)
 * of the new rows (vf31.w kept).  The matching build's block also holds
 * vu0_apply_matrix: its unaligned-source path (0x002580D0..0x002580EB)
 * counts into this routine's range. */
#ifdef VU0M_RETAIL_ASM
__asm__(VU0M_BEGIN(vu0_invert_rigid)
VU0M_SET_DIRTY
"\tvsub.xyz\t$vf31, $vf0, $vf31\n"
"\tqmfc2.ni\t$8, $vf28\n"
"\tqmfc2.ni\t$9, $vf29\n"
"\tqmfc2.ni\t$10, $vf30\n"
"\tpextlw\t$4, $9, $8\n"
"\tpextuw\t$5, $9, $8\n"
"\tpextlw\t$6, $0, $10\n"
"\tpextuw\t$7, $0, $10\n"
"\tpcpyld\t$8, $6, $4\n"
"\tpcpyud\t$9, $4, $6\n"
"\tpcpyld\t$10, $7, $5\n"
"\tqmtc2.ni\t$8, $vf28\n"
"\tqmtc2.ni\t$9, $vf29\n"
"\tqmtc2.ni\t$10, $vf30\n"
"\tvmulax.xyz\tACC, $vf28, $vf31x\n"
"\tvmadday.xyz\tACC, $vf29, $vf31y\n"
"\tjr\t$31\n"
"\tvmaddz.xyz\t$vf31, $vf30, $vf31z\n"
"$Lir_80D0:\n"                              /* vu0_apply_matrix: src unaligned */
"\tldr\t$8, 0x0($5)\n"
"\tldl\t$8, 0x7($5)\n"
"\tldr\t$9, 0x8($5)\n"
"\tldl\t$9, 0xF($5)\n"
"\tpcpyld\t$8, $9, $8\n"
"\tb\t$Lir_80FC\n"
"\tqmtc2.ni\t$8, $vf1\n"
VU0M_SIZE(vu0_invert_rigid)
VU0M_ENTRY(vu0_apply_matrix)
"\tandi\t$2, $5, 0xF\n"
"\tbnez\t$2, $Lir_80D0\n"
"\tvnop\n"
"\tlqc2\t$vf1, 0x0($5)\n"
"$Lir_80FC:\n"
"\tvmulax.xyzw\tACC, $vf28, $vf1x\n"
"\tvmadday.xyzw\tACC, $vf29, $vf1y\n"
"\tvmaddaz.xyzw\tACC, $vf30, $vf1z\n"
"\tvmaddw.xyzw\t$vf1, $vf31, $vf1w\n"
"\tandi\t$2, $4, 0xF\n"
"\tbnel\t$2, $0, $Lir_8120\n"
"\tqmfc2.ni\t$8, $vf1\n"
"\tjr\t$31\n"
"\tsqc2\t$vf1, 0x0($4)\n"
"$Lir_8120:\n"
"\tsdr\t$8, 0x0($4)\n"
"\tsdl\t$8, 0x7($4)\n"
"\tpcpyud\t$8, $8, $8\n"
"\tsdr\t$8, 0x8($4)\n"
"\tjr\t$31\n"
"\tsdl\t$8, 0xF($4)\n"
VU0M_SIZE(vu0_apply_matrix)
VU0M_END);
#else
void vu0_invert_rigid(void)
{
    GS_SPR_CTX->matrixDirty = 1;
    __asm__ __volatile__(
        "vsub.xyz    $vf31, $vf0, $vf31\n\t"
        "qmfc2.ni    $8, $vf28\n\t"
        "qmfc2.ni    $9, $vf29\n\t"
        "qmfc2.ni    $10, $vf30\n\t"
        "pextlw      $4, $9, $8\n\t"
        "pextuw      $5, $9, $8\n\t"
        "pextlw      $6, $0, $10\n\t"
        "pextuw      $7, $0, $10\n\t"
        "pcpyld      $8, $6, $4\n\t"
        "pcpyud      $9, $4, $6\n\t"
        "pcpyld      $10, $7, $5\n\t"
        "qmtc2.ni    $8, $vf28\n\t"
        "qmtc2.ni    $9, $vf29\n\t"
        "qmtc2.ni    $10, $vf30\n\t"
        "vmulax.xyz  " VU0_ACC ", $vf28, $vf31x\n\t"
        "vmadday.xyz " VU0_ACC ", $vf29, $vf31y\n\t"
        "vmaddz.xyz  $vf31, $vf30, $vf31z"
        : : : "$4", "$5", "$6", "$7", "$8", "$9", "$10", "memory");
}
#endif

// FUNC 0x002580EC vu0_apply_matrix
/* dst = src * M (4-vector, w included).  Either pointer may be unaligned:
 * src then goes through ldr/ldl + pcpyld (that path sits just before the
 * entry, 0x002580D0..0x002580EB, inside vu0_invert_rigid's range), dst
 * through qmfc2 + sdr/sdl.  (Matching build: in the block above.) */
#ifndef VU0M_RETAIL_ASM
void vu0_apply_matrix(f32 *dst, const f32 *src)
{
    if (((u32)src & 0xF) != 0) {
        u64 lo = ((const U118U64U *)src)[0].v;      /* ldr/ldl 0..7  */
        u64 hi = ((const U118U64U *)src)[1].v;      /* ldr/ldl 8..15 */

        __asm__ __volatile__(
            "vnop\n\t"
            "pcpyld      %0, %1, %0\n\t"
            "qmtc2.ni    %0, $vf1"
            : "+r"(lo) : "r"(hi) : "memory");
    } else {
        __asm__ __volatile__(
            "vnop\n\t"
            "lqc2        $vf1, 0(%0)"
            : : "r"(src) : "memory");
    }
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf29, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf31, $vf1w"
        : : : "memory");
    if (((u32)dst & 0xF) != 0) {
        u64 lo, hi;

        __asm__ __volatile__(
            "qmfc2.ni    %0, $vf1\n\t"
            "pcpyud      %1, %0, %0"
            : "=&r"(lo), "=r"(hi) : : "memory");
        ((U118U64U *)dst)[0].v = lo;                /* sdr/sdl 0..7  */
        ((U118U64U *)dst)[1].v = hi;                /* sdr/sdl 8..15 */
    } else {
        __asm__ __volatile__("sqc2 $vf1, 0(%0)" : : "r"(dst) : "memory");
    }
}
#endif

// FUNC 0x00258138 pgl_set_normal_matrix
/* GsContext.normalMtx = m (4x4; tail jump to the matrix copy at
 * 0x0025B03C): with PGL_CAP_NORMAL_MATRIX on, pgl_build_env_matrix (u115)
 * premultiplies the environment matrix of lighting mode 2 by its rows 0..2.
 * Callers RacePS2Base::drawCarReflections (the car shadow pass),
 * CarRender::drawModel.  (The context in a local: GS_SPR_CTX's statement
 * expression inside the call's arguments keeps egcs from the tail jump.) */
void pgl_set_normal_matrix(const f32 *m)
{
    GsContext *ctx = GS_SPR_CTX;

    mat44_copy(ctx->normalMtx, m);
}
