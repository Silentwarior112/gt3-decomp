/* T167_rigid_body.cpp - RigidBodyManager: the loose course objects
 * ("gadgets") of a race (0x0019F9F8..0x001A2590).
 *
 * The body matrix of the gadget being processed is kept in the VU0
 * registers: Gadget::loadMatrix (and init) puts the rotation rows into
 * vf20..vf22 and the position into vf23; rb_rotate applies the rotation
 * (body -> world), rb_rotate_inverse its transpose from vf24..vf26 (world ->
 * body), which applyImpulse / carImpulse / groundPenalty first build with
 * vu0_transpose_vf20_to_vf24.  These registers survive the calls in
 * between (no other code of the step touches VU0).
 *
 * The original moves the operands through GPRs (ldl/ldr + pcpyld +
 * qmtc2.ni in, qmfc2.ni + pcpyud + sdl/sdr out, unaligned): the egcs
 * helpers below do so in asm; the GCC 14 ones go through an aligned
 * temporary instead (same values).
 *
 * The rigid body world is the render world (x, y up, z); a car box (T376,
 * (x, y) horizontal, z up) maps to it as (x, y, z) -> (x, z, -y).
 */
#include "game/race/pause/race_pause_internal.h"
#include "vu0asm.h"
#include "game/car/physics/T167_rigid_body.h"

// DATA_START 0x0032FEC8

/* ======================================================================== */
/* VU0 helpers                                                              */
/* ======================================================================== */
/* VU0 macro mode, as the original: the operands move unaligned through the
 * 128-bit GPRs (ldl / ldr, lwl / lwr + pcpyld, qmtc2.ni in; qmfc2.ni +
 * pcpyud, sdl / sdr + swl / swr out).  A 12-byte operand's w lane is the
 * sign of its z (lwl / lwr sign-extend).  Each asm names its memory operand
 * ("m": the offsets fold into the address, `7+%1' is 7 + offset(base)).
 * GCC 14 takes the same values through an aligned temporary. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)

/* vf20..vf23 = the rows of m (vnop; lqc2 x4; m 16-byte aligned) */
static inline void vu0_load_vf20_23(const Matrix44 *m)
{
    __asm__ volatile(
        "vnop\n\t"
        "lqc2         $vf20, 0x00+%0\n\t"
        "lqc2         $vf21, 0x10+%0\n\t"
        "lqc2         $vf22, 0x20+%0\n\t"
        "lqc2         $vf23, 0x30+%0\n\t"
        : : "m"(*m));
}

/* vf23 = (t.x, t.y, t.z, sign bits of t.z) */
static inline void vu0_set_vf23(const Vec3 *t)
{
    u128 lo, hi;

    __asm__ volatile(
        "ldl          %0, 7+%2\n\t"
        "ldr          %0, %2\n\t"
        "lwl          %1, 11+%2\n\t"
        "lwr          %1, 8+%2\n\t"
        "pcpyld       %1, %1, %0\n\t"
        "qmtc2.ni     %1, $vf23\n\t"
        : "=&r"(lo), "=&r"(hi) : "m"(*t));
}

/* out.xyz = vf20 * p.x + vf21 * p.y + vf22 * p.z + vf23 * p.w
 * (vmulax / vmadday / vmaddaz / vmaddw $vf16, $vf23, $vf16w) */
static inline void vu0_xform_vf20(Vec3 *out, const Vec4 *p)
{
    u128 lo, hi;

    __asm__ volatile(
        "ldl          %0, 7+%2\n\t"
        "ldr          %0, %2\n\t"
        "ldl          %1, 15+%2\n\t"
        "ldr          %1, 8+%2\n\t"
        "pcpyld       %1, %1, %0\n\t"
        "qmtc2.ni     %1, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf23, $vf16w\n\t"
        "qmfc2.ni     %0, $vf16\n\t"
        "pcpyud       %1, %0, %0\n\t"
        : "=&r"(lo), "=&r"(hi) : "m"(*p));
    __asm__ volatile(
        "sdl          %1, 7+%0\n\t"
        "sdr          %1, %0\n\t"
        "swl          %2, 11+%0\n\t"
        "swr          %2, 8+%0\n\t"
        : "=m"(*out) : "r"(lo), "r"(hi));
}

/* vf24..vf27 = the transpose of vf20..vf23 (vmove; the rows through the
 * GPRs: pextlw / pextuw / pcpyld / pcpyud) */
static inline void vu0_transpose_vf20_to_vf24(void)
{
    u128 r0, r1, r2, r3, t0, t1, t2, t3;

    __asm__ volatile(
        "vmove.xyzw   $vf24, $vf20\n\t"
        "vmove.xyzw   $vf25, $vf21\n\t"
        "vmove.xyzw   $vf26, $vf22\n\t"
        "vmove.xyzw   $vf27, $vf23\n\t");
    __asm__ volatile("qmfc2.ni     %0, $vf24" : "=r"(r0));
    __asm__ volatile("qmfc2.ni     %0, $vf25" : "=r"(r1));
    __asm__ volatile("qmfc2.ni     %0, $vf26" : "=r"(r2));
    __asm__ volatile("qmfc2.ni     %0, $vf27" : "=r"(r3));
    __asm__ volatile("pextlw       %0, %1, %2" : "=r"(t0) : "r"(r1), "r"(r0));
    __asm__ volatile("pextuw       %0, %1, %2" : "=r"(t1) : "r"(r1), "r"(r0));
    __asm__ volatile("pextlw       %0, %1, %2" : "=r"(t2) : "r"(r3), "r"(r2));
    __asm__ volatile("pextuw       %0, %1, %2" : "=r"(t3) : "r"(r3), "r"(r2));
    __asm__ volatile("pcpyld       %0, %1, %2" : "=r"(r0) : "r"(t2), "r"(t0));
    __asm__ volatile("pcpyud       %0, %1, %2" : "=r"(r1) : "r"(t0), "r"(t2));
    __asm__ volatile("pcpyld       %0, %1, %2" : "=r"(r2) : "r"(t3), "r"(t1));
    __asm__ volatile("pcpyud       %0, %1, %2" : "=r"(r3) : "r"(t1), "r"(t3));
    __asm__ volatile("qmtc2.ni     %0, $vf24" : : "r"(r0));
    __asm__ volatile("qmtc2.ni     %0, $vf25" : : "r"(r1));
    __asm__ volatile("qmtc2.ni     %0, $vf26" : : "r"(r2));
    __asm__ volatile("qmtc2.ni     %0, $vf27" : : "r"(r3));
}

#else /* GCC 14 */

union Vu0Vec {
    f32 f[4];
    u32 u[4];
} __attribute__((aligned(16)));

/* The w lane of a 12-byte operand: lwl/lwr sign-extends the z word into the
 * upper half of the 64-bit register that pcpyld then puts in lane 3. */
static inline u32 vu0_w_of(const Vu0Vec *v)
{
    return (u32)((s32)v->u[2] >> 31);
}

static inline void vu0_load_vf20_23(const Matrix44 *m)
{
    __asm__ volatile(
        "vnop\n\t"
        "lqc2         $vf20, 0x00(%0)\n\t"
        "lqc2         $vf21, 0x10(%0)\n\t"
        "lqc2         $vf22, 0x20(%0)\n\t"
        "lqc2         $vf23, 0x30(%0)\n\t"
        : : "r"(m) : "memory");
}

static inline void vu0_set_vf23(const Vec3 *t)
{
    Vu0Vec v;
    v.f[0] = t->x;
    v.f[1] = t->y;
    v.f[2] = t->z;
    v.u[3] = vu0_w_of(&v);
    __asm__ volatile("lqc2         $vf23, 0(%0)\n\t" : : "r"(&v) : "memory");
}

static inline void vu0_xform_vf20(Vec3 *out, const Vec4 *p)
{
    Vu0Vec v;
    v.f[0] = p->x;
    v.f[1] = p->y;
    v.f[2] = p->z;
    v.f[3] = p->w;
    __asm__ volatile(
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf23, $vf16w\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(&v) : "memory");
    out->x = v.f[0];
    out->y = v.f[1];
    out->z = v.f[2];
}

static inline void vu0_transpose_vf20_to_vf24(void)
{
    f32 rows[4][4] __attribute__((aligned(16)));
    f32 cols[4][4] __attribute__((aligned(16)));
    int i, j;
    __asm__ volatile(
        "sqc2         $vf20, 0x00(%0)\n\t"
        "sqc2         $vf21, 0x10(%0)\n\t"
        "sqc2         $vf22, 0x20(%0)\n\t"
        "sqc2         $vf23, 0x30(%0)\n\t"
        : : "r"(rows) : "memory");
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            cols[i][j] = rows[j][i];
    __asm__ volatile(
        "lqc2         $vf24, 0x00(%0)\n\t"
        "lqc2         $vf25, 0x10(%0)\n\t"
        "lqc2         $vf26, 0x20(%0)\n\t"
        "lqc2         $vf27, 0x30(%0)\n\t"
        : : "r"(cols) : "memory");
}

#endif

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft).  A volatile asm (a scheduling
 * barrier), as T077's icon_rsqrt: retail keeps the code after an rsqrt.s
 * after it (rb_rotation_between, applyImpulse, wallImpulse). */
static inline f32 ee_rsqrt(f32 num, f32 sq)
{
    f32 r;
    __asm__ volatile("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
}

/* The car box planes and the contact points: a Vec4 plane (n, d) used as
 * the Vec3 n (rb_dot), a shape's Vec4 point as its Vec3 xyz. */
static inline const Vec3 *xyz(const Vec4 *v)
{
    return (const Vec3 *)v;
}

/* ======================================================================== */
/* Data (values from the original's; race_pause_internal.h / T167_rigid_body.h declare     */
/* them, so the const tables have external linkage and are written here)    */
/* ======================================================================== */

/* .rodata 0x0032FED8..0x0032FF58, before the vtable and the type names. */
const Vec3 g_0032FED8 = { 0.0f, 1.0f, 0.0f };     /* 0x0032FED8 the body's up */
/* The velocity steps c[k] of the integrator (Forest-Ruth / Yoshida:
 * 1 / (2 - 2^(1/3)), -2^(1/3) / (2 - 2^(1/3)), the first again). */
const f32 g_0032FEE8[4] = { 1.3512071f, -1.7024144f, 1.3512071f, 0.0f };   /* 0x0032FEE8 */
/* The outward face normals of a car box (w 0). */
const Vec4 g_0032FEF8[6] = {                /* 0x0032FEF8 */
    {  0.0f,  0.05f, -1.0f, 0.0f },         /* front */
    { -1.0f,  0.05f,  0.0f, 0.0f },         /* the -x side */
    {  0.0f,  0.05f,  1.0f, 0.0f },         /* rear */
    {  1.0f,  0.05f,  0.0f, 0.0f },         /* the +x side */
    {  0.0f,  1.0f,   0.0f, 0.0f },         /* top */
    {  0.0f, -1.0f,   0.0f, 0.0f },         /* bottom (never a plane) */
};

/* .data 0x002C04A8..0x002C07E8: the light direction of the shadows, then
 * (globals without an initialiser: zeros at the end of the file's .data,
 * in the order of their first declaration) the template shape and the car
 * box planes. */
Vec3        g_002C04A8 = { 0.0f, 1.0f, 0.0f };  /* 0x002C04A8 */
GadgetShape g_002C04B8;                     /* 0x002C04B8 T179 copies the course's first object in */
Vec4        g_002C0568[RB_MAX_CAR_BOXES][RB_BOX_PLANES];  /* 0x002C0568 */

/* .sdata 0x00352174..0x00352180 (zeros, after Gadget::step's guard and
 * wallImpulse's FLT_MAX): the statics of the inline Get / Set accessors. */
GadgetShape **g_00352174;                   /* 0x00352174 */
CarBoxList   *g_00352178;                   /* 0x00352178 */
RaceCourse   *g_0035217C;                   /* 0x0035217C */

/* ======================================================================== */
/* Vector / matrix helpers                                                  */
/* ======================================================================== */

// FUNC 0x0019F9F8 rb_cross
//   out = a x b (each component stored before the next is read: out may be
//   a or b).
void rb_cross(Vec3 *out, const Vec3 *a, const Vec3 *b)
{
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

// FUNC 0x0019FA60 rb_dot
f32 rb_dot(const Vec3 *a, const Vec3 *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

// FUNC 0x0019FA90 rb_rotate
//   out = the body rotation (vf20..vf22) applied to in: body -> world.
void rb_rotate(Vec3 *out, const Vec3 *in)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* The original's body is this asm (VU0 macro mode; the operands
     * unaligned through GPRs: ldl / ldr + lwl / lwr + pcpyld, qmtc2.ni in;
     * qmfc2.ni + pcpyud, sdl / sdr + swl / swr out).  The w lane is the
     * sign of z (lwl / lwr sign-extend). */
    __asm__ volatile(
        "vnop\n\t"
        "ldl          $3, 7(%1)\n\t"
        "ldr          $3, 0(%1)\n\t"
        "lwl          $2, 11(%1)\n\t"
        "lwr          $2, 8(%1)\n\t"
        "pcpyld       $2, $2, $3\n\t"
        "qmtc2.ni     $2, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddz.xyzw  $vf16, $vf22, $vf16z\n\t"
        "qmfc2.ni     $2, $vf16\n\t"
        "pcpyud       $3, $2, $2\n\t"
        "sdl          $2, 7(%0)\n\t"
        "sdr          $2, 0(%0)\n\t"
        "swl          $3, 11(%0)\n\t"
        "swr          $3, 8(%0)\n\t"
        : : "r"(out), "r"(in) : "$2", "$3", "memory");
#else
    Vu0Vec v;
    v.f[0] = in->x;
    v.f[1] = in->y;
    v.f[2] = in->z;
    v.u[3] = vu0_w_of(&v);
    __asm__ volatile(
        "vnop\n\t"
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddz.xyzw  $vf16, $vf22, $vf16z\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(&v) : "memory");
    out->x = v.f[0];
    out->y = v.f[1];
    out->z = v.f[2];
#endif
}

// FUNC 0x0019FAD8 rb_rotate_inverse
//   out = vf24..vf26 (the transposed body rotation) applied to in: world ->
//   body.
void rb_rotate_inverse(Vec3 *out, const Vec3 *in)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* The original's body is this asm (VU0 macro mode; the operands
     * unaligned through GPRs: ldl / ldr + lwl / lwr + pcpyld, qmtc2.ni in;
     * qmfc2.ni + pcpyud, sdl / sdr + swl / swr out).  The w lane is the
     * sign of z (lwl / lwr sign-extend). */
    __asm__ volatile(
        "vnop\n\t"
        "ldl          $3, 7(%1)\n\t"
        "ldr          $3, 0(%1)\n\t"
        "lwl          $2, 11(%1)\n\t"
        "lwr          $2, 8(%1)\n\t"
        "pcpyld       $2, $2, $3\n\t"
        "qmtc2.ni     $2, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf16y\n\t"
        "vmaddz.xyzw  $vf16, $vf26, $vf16z\n\t"
        "qmfc2.ni     $2, $vf16\n\t"
        "pcpyud       $3, $2, $2\n\t"
        "sdl          $2, 7(%0)\n\t"
        "sdr          $2, 0(%0)\n\t"
        "swl          $3, 11(%0)\n\t"
        "swr          $3, 8(%0)\n\t"
        : : "r"(out), "r"(in) : "$2", "$3", "memory");
#else
    Vu0Vec v;
    v.f[0] = in->x;
    v.f[1] = in->y;
    v.f[2] = in->z;
    v.u[3] = vu0_w_of(&v);
    __asm__ volatile(
        "vnop\n\t"
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf16y\n\t"
        "vmaddz.xyzw  $vf16, $vf26, $vf16z\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(&v) : "memory");
    out->x = v.f[0];
    out->y = v.f[1];
    out->z = v.f[2];
#endif
}

// FUNC 0x0019FB20 rb_rotation_between
//   The rotation taking the direction `from' to `to' (axis a = from x to,
//   normalized; cos c = from . to, sin s = |from x to|), Rodrigues' formula:
//   m3x3 = a a^T + c (I - a a^T) + s K(a); translation 0, m[3][3] 1.  The
//   outer product a a^T is also written to a local 3x3 (unused afterwards).
//   Quirk: when s == 0 the "identity" written has m[2][0] = 1 as well
//   (0x20($s0) = 1.0).
//   Matching (72.8%): the head (rsqrt.s / sqrt.s volatile asms) is retail's; the
//   zero arm's stores come out reordered (sched1 puts the stores that are the
//   last use of the 0.0 / 1.0 registers first; retail keeps source order) and
//   the x / y / z registers and the Rodrigues products are scheduled
//   differently.  Tried: axis / x, y, z orders, inline row / element setters
//   for the zero arm (a pointer row helper orders row 0 only).
void rb_rotation_between(Matrix44 *m, const Vec3 *from, const Vec3 *to)
{
    Vec3 fromN;                             /* sp+0x00 from, normalized */
    Vec3 toN;                               /* sp+0x10 to, normalized */
    Vec3 axis;                              /* sp+0x20 */
    f32  outer[3][3];                       /* sp+0x30 a a^T (dead) */
    f32  invFrom, invTo, cosA, sinA, invSin, x, y, z;

    invFrom = ee_rsqrt(1.0f, from->x * from->x + from->y * from->y + from->z * from->z);
    invTo = ee_rsqrt(1.0f, to->x * to->x + to->y * to->y + to->z * to->z);
    fromN.x = from->x * invFrom;
    fromN.y = from->y * invFrom;
    toN.x = to->x * invTo;
    toN.y = to->y * invTo;
    fromN.z = from->z * invFrom;
    toN.z = to->z * invTo;
    cosA = fromN.x * toN.x + fromN.y * toN.y + fromN.z * toN.z;
    rb_cross(&axis, &fromN, &toN);
    sinA = boot_sqrtf_v(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    invSin = 1.0f / sinA;
    axis.z = axis.z * invSin;
    axis.x = axis.x * invSin;
    axis.y = axis.y * invSin;
    x = axis.x;
    y = axis.y;
    z = axis.z;
    if (sinA == 0.0f) {
        m->m[3][3] = 1.0f;
        m->m[3][2] = 0.0f;
        m->m[0][0] = 1.0f;
        m->m[0][1] = 0.0f;
        m->m[0][2] = 0.0f;
        m->m[0][3] = 0.0f;
        m->m[1][0] = 0.0f;
        m->m[1][1] = 1.0f;
        m->m[1][2] = 0.0f;
        m->m[1][3] = 0.0f;
        m->m[2][0] = 1.0f;                  /* sic */
        m->m[2][1] = 0.0f;
        m->m[2][2] = 1.0f;
        m->m[2][3] = 0.0f;
        m->m[3][0] = 0.0f;
        m->m[3][1] = 0.0f;
        return;
    }
    m->m[3][3] = 1.0f;
    m->m[3][2] = 0.0f;
    m->m[0][3] = 0.0f;
    outer[0][0] = x * x;
    outer[0][1] = x * y;
    outer[0][2] = x * z;
    outer[1][0] = x * y;
    outer[1][1] = y * y;
    outer[1][2] = y * z;
    outer[2][0] = x * z;
    outer[2][1] = y * z;
    outer[2][2] = z * z;
    m->m[1][3] = 0.0f;
    m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f;
    m->m[3][1] = 0.0f;
    m->m[1][0] = (x * y + cosA * (0.0f - x * y)) + sinA * -z;
    m->m[2][0] = (x * z + cosA * (0.0f - x * z)) + sinA * y;
    m->m[2][1] = (y * z + cosA * (0.0f - y * z)) + sinA * -x;
    m->m[2][2] = (z * z + cosA * (1.0f - z * z)) + sinA * 0.0f;
    m->m[0][0] = (x * x + cosA * (1.0f - x * x)) + sinA * 0.0f;
    m->m[0][1] = (x * y + cosA * (0.0f - x * y)) + sinA * z;
    m->m[0][2] = (x * z + cosA * (0.0f - x * z)) + sinA * -y;
    m->m[1][1] = (y * y + cosA * (1.0f - y * y)) + sinA * 0.0f;
    m->m[1][2] = (y * z + cosA * (0.0f - y * z)) + sinA * x;
}

// FUNC 0x0019FDA0 rb_euler_matrix
//   The rotation of the Euler angles (rx, ry, rz); translation 0, m[3][3] 1.
//   Matching (93.8%): row-major statements; retail computes cos y * cos z and
//   cos y * sin z before (sin x sin z) * sin y.  An in-place hill-climb over the
//   16 statements, temporaries for the shared products and -cos x found no
//   better form.
void rb_euler_matrix(Matrix44 *m, f32 rx, f32 ry, f32 rz)
{
    f32 sinX, cosX, sinY, cosY, sinZ, cosZ; /* sp+0x00 .. sp+0x14 */

    sincosf(rx, &sinX, &cosX);
    sincosf(ry, &sinY, &cosY);
    sincosf(rz, &sinZ, &cosZ);
    m->m[0][0] = cosY * cosZ - sinX * sinZ * sinY;
    m->m[0][1] = -cosX * sinZ;
    m->m[0][2] = sinY * cosZ + sinX * sinZ * cosY;
    m->m[0][3] = 0.0f;
    m->m[1][0] = cosY * sinZ + sinX * cosZ * sinY;
    m->m[1][1] = cosX * cosZ;
    m->m[1][2] = sinY * sinZ - sinX * cosZ * cosY;
    m->m[1][3] = 0.0f;
    m->m[2][0] = -cosX * sinY;
    m->m[2][1] = sinX;
    m->m[2][2] = cosX * cosY;
    m->m[2][3] = 0.0f;
    m->m[3][0] = 0.0f;
    m->m[3][1] = 0.0f;
    m->m[3][2] = 0.0f;
    m->m[3][3] = 1.0f;
}

// FUNC 0x0019FEB0 rb_matrix_to_euler
//   The inverse of rb_euler_matrix (cos rx = sqrt(1 - m[2][1]^2) >= 0).
void rb_matrix_to_euler(Vec3 *out, const Matrix44 *m)
{
    f32 sinX = m->m[2][1];
    f32 cosX = boot_sqrtf_v(1.0f - sinX * sinX);
    f32 cosYcX = m->m[2][2] / cosX;
    f32 sinYcX = -m->m[2][0] / cosX;
    f32 sinZcX = -m->m[0][1] / cosX;
    f32 cosZcX = m->m[1][1] / cosX;

    out->x = atan2f(sinX, cosX);
    out->y = atan2f(sinYcX, cosYcX);
    out->z = atan2f(sinZcX, cosZcX);
}

// FUNC 0x0019FF80 rb_wall_callback
//   RaceCourse::forEachPoint's callback for each wall vertex in reach (ctx:
//   a WallCtx): the first collision only.
void rb_wall_callback(const WallSeg *wall, void *ctx)
{
    WallCtx *wc = (WallCtx *)ctx;

    if (wc->hit == 0)
        wc->hit = wc->self->collideWall(wall, wc->oldPos, wc->points);
}

/* ======================================================================== */
/* Gadget: one rigid body                                                   */
/* ======================================================================== */

// FUNC 0x0019FFC0 Gadget::init
//   From the runway record: the position, the shape, zero velocities, the
//   first three contact counters 2 (it starts resting), the ground plane;
//   the rotation becomes (the rotation of (0, 1, 0) onto the ground normal)
//   * (the record's Euler angles), composed on the G001 matrix stack, then
//   the gadget is lifted / lowered so that its first contact point lies on
//   the ground plane.
//   Matching (97.7%): only the loads of p.x / p.z and plane.x / plane.z swap
//   (sched2 prefers the plane loads: the p loads depend on the store asm).
//   Tried: the store asm with p as input, other asm splits.
void Gadget::init(const GadgetDesc *desc)
{
    Vec3        up;                         /* sp+0x00 */
    Matrix44    m __attribute__((aligned(16)));    /* sp+0x10 */
    Vec3        p;                          /* sp+0x50 contact point 0 (world) */
    GadgetShape *s;

    shape = desc->shape;
    pos.x = desc->pos.x;
    pos.y = desc->pos.y;
    pos.z = desc->pos.z;
    rot.x = desc->rot.x;
    rot.y = desc->rot.y;
    rot.z = desc->rot.z;
    vel.x = 0.0f;
    vel.y = 0.0f;
    vel.z = 0.0f;
    angVel.x = 0.0f;
    angVel.y = 0.0f;
    angVel.z = 0.0f;
    clearContacts();
    contact[0] = RB_CONTACT_RESTING;
    contact[1] = RB_CONTACT_RESTING;
    contact[2] = RB_CONTACT_RESTING;
    sector = -1;
    s = RigidBodyManager::GetGadgetShape(shape);
    findGround();
    up = g_0032FED8;
    rb_rotation_between(&m, &up, xyz(&plane));
    pgl_push_matrix();
    vu0_load_identity_regs();
    pgl_mult_matrix(&m.m[0][0]);
    rb_euler_matrix(&m, desc->rot.x, desc->rot.y, desc->rot.z);
    pgl_mult_matrix(&m.m[0][0]);
    pgl_get_matrix(&m.m[0][0]);
    pgl_pop_matrix();
    rb_matrix_to_euler(&rot, &m);
    rotationMatrix(&m);
    vu0_load_vf20_23(&m);
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* vf23 = pos and contact point 0 transformed in one asm: retail uses
     * the same two temporaries ($3, $2) for both loads (vu0_set_vf23 +
     * vu0_xform_vf20 give the first load other registers). */
    {
        u128 lo, hi;

        __asm__ volatile(
            "ldl          %0, 7+%2\n\t"
            "ldr          %0, %2\n\t"
            "lwl          %1, 11+%2\n\t"
            "lwr          %1, 8+%2\n\t"
            "pcpyld       %1, %1, %0\n\t"
            "qmtc2.ni     %1, $vf23\n\t"
            "ldl          %0, 7+%3\n\t"
            "ldr          %0, %3\n\t"
            "ldl          %1, 15+%3\n\t"
            "ldr          %1, 8+%3\n\t"
            "pcpyld       %1, %1, %0\n\t"
            "qmtc2.ni     %1, $vf16\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
            "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf16z\n\t"
            "vmaddw.xyzw  $vf16, $vf23, $vf16w\n\t"
            "qmfc2.ni     %1, $vf16\n\t"
            "pcpyud       %0, %1, %1\n\t"
            : "=&r"(lo), "=&r"(hi) : "m"(pos), "m"(s->points[0]));
        __asm__ volatile(
            "sdl          %1, 7+%0\n\t"
            "sdr          %1, %0\n\t"
            "swl          %2, 11+%0\n\t"
            "swr          %2, 8+%0\n\t"
            : "=m"(p) : "r"(hi), "r"(lo));
    }
#else
    vu0_set_vf23(&pos);
    vu0_xform_vf20(&p, &s->points[0]);
#endif
    pos.y = pos.y + (-(p.x * plane.x + p.z * plane.z + plane.w) / plane.y - p.y);
}

// FUNC 0x001A01D0 Gadget::clearContacts
void Gadget::clearContacts()
{
    int i;
    for (i = 0; i < 8; i++)                 /* loop.c reverses it (retail: &contact[7] down) */
        contact[i] = 0;
}

// FUNC 0x001A0200 Gadget::findGround
//   The ground plane under pos (RaceCourse::getPlane, the sector as hint);
//   1 when on the road.
int Gadget::findGround()
{
    int sec = sector;                       /* sp+0x00 */
    int attr;                               /* sp+0x04 (unused) */

    RigidBodyManager::GetCourse()->getPlane(&pos.x, &sec, &attr, &plane.x);
    sector = (s16)sec;
    return sec >= 0;
}

// FUNC 0x001A0258 Gadget::step
//   One step of dt.  Unless resting (3 contact counters at 2): the Euler
//   angles and the body angular velocity are integrated with the 4th-order
//   symplectic scheme d0 c0 d1 c1 d2 c2 d3 (c = g_0032FEE8, d = g_0049C3E0
//   = the half sums, built on the first call); the torque is always 0 (only
//   Euler's equations act); the angles are wrapped to [0, 2 pi); position
//   and velocity get gravity; the angular velocity is damped (0.999) and
//   clamped to +-20; the ground plane is looked up again (off the road: 0,
//   the gadget is removed).  Then always: the contact points in world
//   space, the ground contact (not resting), the car boxes, and (not
//   resting) the runway walls between the old and the new position.
//   Returns 1 while the gadget lives.
//   The Euler-angle rates of the body angular velocity w (sx, cx = sin, cos
//   rot.x; sz, cz of rot.z):  rot.y' = (wx sz - wy cz) / cx,
//   rot.x' = -(wx cz + wy sz),  rot.z' = -(rot.y' sx + wz).
int Gadget::step(f32 dt)
{
    Vec3        points[8];                  /* sp+0x00 the contact points (world) */
    Vec3        force;                      /* sp+0x60 */
    Vec3        torque;                     /* sp+0x70 (always 0) */
    Vec3        oldPos;                     /* sp+0x80 */
    WallCtx     ctx;                        /* sp+0x90 */
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    int         numPoints = s->count;       /* sp+0xB0 */
    int         numResting = 0, resting, i;

    force.x = 0.0f;
    force.y = -s->mass * RB_GRAVITY;
    oldPos.x = pos.x;
    oldPos.y = pos.y;
    oldPos.z = pos.z;
    force.z = 0.0f;
    torque.x = 0.0f;
    torque.y = 0.0f;
    torque.z = 0.0f;
    for (i = 0; i < numPoints; i++)
        if (contact[i] == RB_CONTACT_RESTING)
            numResting++;
    resting = numResting >= RB_RESTING_POINTS;  /* sp+0xB4 */

    if (!resting) {
        f32 wx = angVel.x, wy = angVel.y, wz = angVel.z;
        f32 Ix = s->inertia.x, Iy = s->inertia.y, Iz = s->inertia.z;
        f32 sinX, cosX, sinZ, cosZ;         /* sp+0xA0 .. sp+0xAC */
        f32 invCosX, posStep, rateY;

        sincosf(rot.x, &sinX, &cosX);
        sincosf(rot.z, &sinZ, &cosZ);
        invCosX = 1.0f / cosX;
#ifdef BOOT_OLD_GCC
        /* A function-local static with a dynamic initialiser (egcs: its
         * guard in .sdata 0x0035216C, set before the elements are stored). */
        static f32 d[4] = {                 /* 0x0049C3E0 */
            g_0032FEE8[0] * 0.5f, (g_0032FEE8[0] + g_0032FEE8[1]) * 0.5f,
            (g_0032FEE8[0] + g_0032FEE8[1]) * 0.5f, g_0032FEE8[0] * 0.5f
        };
#else
        /* GCC 14: the same steps without thread-safe static guards. */
        f32 *d = g_0049C3E0;

        if (g_0035216C == 0) {
            g_0035216C = 1;
            d[0] = g_0032FEE8[0] * 0.5f;
            d[1] = (g_0032FEE8[0] + g_0032FEE8[1]) * 0.5f;
            d[2] = (g_0032FEE8[0] + g_0032FEE8[1]) * 0.5f;
            d[3] = g_0032FEE8[0] * 0.5f;
        }
#endif
        /* angles += d0 dt * (the Euler-angle rates of w) */
        posStep = dt * d[0];
        rateY = (wx * sinZ - wy * cosZ) * invCosX;
        rot.x = rot.x + -(wx * cosZ + wy * sinZ) * posStep;
        rot.y = rot.y + rateY * posStep;
        rot.z = rot.z + -(rateY * sinX + wz) * posStep;
        for (i = 0; i < 3; i++) {
            /* w += c[i] dt * I^-1 (torque + (I w) x w) */
            f32 velStep = dt * g_0032FEE8[i];
            angVel.x = angVel.x + ((torque.x + (Iy - Iz) * wy * wz) * velStep) / Ix;
            angVel.y = angVel.y + ((torque.y + (Iz - Ix) * wz * wx) * velStep) / Iy;
            angVel.z = angVel.z + ((torque.z + (Ix - Iy) * wx * wy) * velStep) / Iz;
            /* angles += d[i + 1] dt * rates */
            sincosf(rot.x, &sinX, &cosX);
            sincosf(rot.z, &sinZ, &cosZ);
            wx = angVel.x;
            wy = angVel.y;
            wz = angVel.z;
            invCosX = 1.0f / cosX;
            posStep = dt * d[i + 1];
            rateY = (wx * sinZ - wy * cosZ) * invCosX;
            rot.y = rot.y + rateY * posStep;
            rot.x = rot.x + -(wx * cosZ + wy * sinZ) * posStep;
            rot.z = rot.z + -(rateY * sinX + wz) * posStep;
        }
        if (rot.x < 0.0f)
            rot.x = rot.x + RB_TWO_PI;
        if (rot.x >= RB_TWO_PI)
            rot.x = rot.x - RB_TWO_PI;
        if (rot.y < 0.0f)
            rot.y = rot.y + RB_TWO_PI;
        if (rot.y >= RB_TWO_PI)
            rot.y = rot.y - RB_TWO_PI;
        if (rot.z < 0.0f)
            rot.z = rot.z + RB_TWO_PI;
        if (rot.z >= RB_TWO_PI)
            rot.z = rot.z - RB_TWO_PI;

        pos.x = pos.x + vel.x * dt;
        pos.y = pos.y + vel.y * dt;
        pos.z = pos.z + vel.z * dt;
        vel.x = vel.x + force.x * s->invMass * dt;
        vel.y = vel.y + force.y * s->invMass * dt;
        vel.z = vel.z + force.z * s->invMass * dt;
        angVel.x = wx * RB_SPIN_DAMPING;
        angVel.y = wy * RB_SPIN_DAMPING;
        angVel.z = wz * RB_SPIN_DAMPING;
        if (angVel.x > RB_SPIN_MAX)
            angVel.x = RB_SPIN_MAX;
        if (angVel.y > RB_SPIN_MAX)
            angVel.y = RB_SPIN_MAX;
        if (angVel.z > RB_SPIN_MAX)
            angVel.z = RB_SPIN_MAX;
        if (angVel.x < -RB_SPIN_MAX)
            angVel.x = -RB_SPIN_MAX;
        if (angVel.y < -RB_SPIN_MAX)
            angVel.y = -RB_SPIN_MAX;
        if (angVel.z < -RB_SPIN_MAX)
            angVel.z = -RB_SPIN_MAX;
    }
    /* (the original tests sp+0xB4 again here: a second `if') */
    if (!resting && findGround() == 0)
        return 0;

    loadMatrix();
    worldPoints(points);
    if (!resting)
        groundContact(points, numPoints);
    collideCars(points, numPoints);
    if (!resting) {
        f32 x0, x1, z0, z1;
        if (oldPos.x < pos.x) {
            x0 = oldPos.x;
            x1 = pos.x;
        } else {
            x1 = oldPos.x;
            x0 = pos.x;
        }
        if (oldPos.z < pos.z) {
            z0 = oldPos.z;
            z1 = pos.z;
        } else {
            z1 = oldPos.z;
            z0 = pos.z;
        }
        ctx.oldPos = &oldPos;
        ctx.self = this;
        ctx.points = points;
        ctx.hit = 0;
        RigidBodyManager::GetCourse()->forEachPoint(rb_wall_callback, &ctx, sector,
                                                    x0, z0, x1, z1);
    }
    return 1;
}

// FUNC 0x001A08E8 Gadget::loadMatrix
//   vf20..vf22 = the rotation of the Euler angles, vf23 = the position.
void Gadget::loadMatrix()
{
    Matrix44 m __attribute__((aligned(16)));    /* sp+0x00 */

    rotationMatrix(&m);
    vu0_load_vf20_23(&m);
    vu0_set_vf23(&pos);
}

// FUNC 0x001A0940 Gadget::worldPoints
//   out[i] = the body matrix (vf20..vf23) * the shape's contact point i.
//   Matching (60.5%): retail's loop is not optimised by loop.c (the constant 12
//   stays in the loop, neither address is strength-reduced: a GCSE insertion
//   after the loop start makes it `phony'); ours is reduced and reversed.
//   Tried: s->count in the condition, while / do forms, "m" / "r" store
//   operands with and without a memory clobber, the shape re-read per point.
void Gadget::worldPoints(Vec3 *out)
{
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    int numPoints = s->count;
    int i;

    for (i = 0; i < numPoints; i++)
        vu0_xform_vf20(&out[i], &s->points[i]);
}

// FUNC 0x001A09D8 Gadget::groundPenalty
//   No callers.  A penalty-force variant of groundContact: for each point
//   below the ground plane, acc += n * (-10 depth) - 0.999 * (the point's
//   velocity); then the gadget is moved out by the deepest distance.
//   `unused' ($a2) is never read.  The transpose built first is not used
//   (rb_rotate works on vf20..vf22).
void Gadget::groundPenalty(Vec3 *acc, void *unused, const Vec3 *pts, int numPoints)
{
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    Vec3        push;                       /* sp+0x00 */
    Vec3        spin;                       /* sp+0x10 w x r (body) */
    Vec3        pointVel;                   /* sp+0x20 */
    f32         minDist;
    int         i;

    vu0_transpose_vf20_to_vf24();
    minDist = 0.0f;
    for (i = 0; i < numPoints; i++) {
        f32 dist = pts[i].x * plane.x + pts[i].y * plane.y + pts[i].z * plane.z + plane.w;
        f32 k;
        if (!(dist < 0.0f))
            continue;
        if (dist < minDist)
            minDist = dist;
        k = dist * RB_PENALTY_K;
        push.x = plane.x * k;
        push.y = plane.y * k;
        push.z = plane.z * k;
        rb_cross(&spin, &angVel, xyz(&s->points[i]));
        rb_rotate(&pointVel, &spin);
        pointVel.x = pointVel.x + vel.x;
        pointVel.y = pointVel.y + vel.y;
        pointVel.z = pointVel.z + vel.z;
        push.x = push.x + pointVel.x * RB_PENALTY_DAMPING;
        push.y = push.y + pointVel.y * RB_PENALTY_DAMPING;
        push.z = push.z + pointVel.z * RB_PENALTY_DAMPING;
        acc->x = acc->x + push.x;
        acc->y = acc->y + push.y;
        acc->z = acc->z + push.z;
    }
    if (minDist != 0.0f) {
        pos.x = pos.x - minDist * plane.x;
        pos.y = pos.y - minDist * plane.y;
        pos.z = pos.z - minDist * plane.z;
    }
}

// FUNC 0x001A0C48 Gadget::groundContact
//   The points below the ground plane: the gadget is lifted by the largest
//   vertical depth (depth = ground height at (x, z) - y) and that point gets
//   the impulse; a point's counter is incremented when it becomes the
//   deepest so far, cleared when it is above the plane.
void Gadget::groundContact(Vec3 *pts, int numPoints)
{
    f32 depthMax = 0.0f;
    int deepest = -1;
    int i;

    for (i = 0; i < numPoints; i++) {
        f32 xn = pts[i].x * plane.x;
        f32 zn = pts[i].z * plane.z;
        if (xn + pts[i].y * plane.y + zn + plane.w < 0.0f) {
            f32 depth = -(xn + zn + plane.w) / plane.y - pts[i].y;
            if (depthMax < depth) {
                depthMax = depth;
                deepest = i;
                contact[i]++;
            }
        } else
            contact[i] = 0;
    }
    pos.y = pos.y + depthMax;
    if (deepest >= 0)
        applyImpulse(deepest, xyz(&plane));
}

// FUNC 0x001A0D28 Gadget::applyImpulse
//   A collision impulse at contact point `point' along the unit normal
//   `normal' (world): restitution 0.25 (0 when the point moves slower than
//   0.9), j = -(1 + e) v.n / (1/m + n.((I^-1 (r x n)) x r)); none when
//   j < 0 (returns 0).  Coulomb friction (mu 0.5): the tangential velocity
//   is removed up to 0.5 j/m (factor k <= 1), on the linear and the angular
//   velocity.
//   Matching (99.2%): only the place of e = 0.25 (lui / mtc1): retail sets it
//   before the first call, egcs schedules it after (any source position after
//   the transpose; before it, the prologue).
int Gadget::applyImpulse(int point, const Vec3 *normal)
{
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    const Vec3  *r = (const Vec3 *)&s->points[point];
    Vec3        nBody;                      /* sp+0x00 the normal in the body frame */
    Vec3        rn;                         /* sp+0x10 r x nBody */
    Vec3        pointVel;                   /* sp+0x20 the point's velocity */
    Vec3        tmp;                        /* sp+0x30 */
    Vec3        irn;                        /* sp+0x40 I^-1 (r x nBody) */
    Vec3        tangent;                    /* sp+0x50 the tangential velocity */
    f32         e, denom, j, jm, vn, k;

    vu0_transpose_vf20_to_vf24();
    e = RB_RESTITUTION;
    rb_rotate_inverse(&nBody, normal);
    rb_cross(&rn, r, &nBody);
    irn.x = rn.x / s->inertia.x;
    irn.y = rn.y / s->inertia.y;
    irn.z = rn.z / s->inertia.z;
    rb_cross(&tmp, &irn, r);
    rb_rotate(&tmp, &tmp);
    denom = s->invMass + rb_dot(normal, &tmp);
    rb_cross(&tmp, &angVel, r);
    rb_rotate(&pointVel, &tmp);
    pointVel.x = pointVel.x + vel.x;
    pointVel.y = pointVel.y + vel.y;
    pointVel.z = pointVel.z + vel.z;
    if (pointVel.x * pointVel.x + pointVel.y * pointVel.y + pointVel.z * pointVel.z < RB_REST_SPEED2)
        e = 0.0f;
    j = (-(e + 1.0f) * rb_dot(normal, &pointVel)) / denom;
    jm = j * s->invMass;
    if (j < 0.0f)
        return 0;
    vn = rb_dot(&pointVel, normal);
    tangent.x = pointVel.x - vn * normal->x;
    tangent.y = pointVel.y - vn * normal->y;
    tangent.z = pointVel.z - vn * normal->z;
    k = ee_rsqrt(jm * RB_FRICTION, rb_dot(&tangent, &tangent));    /* (0.5 j/m) / |vt| */
    if (1.0f < k)
        k = 1.0f;
    vel.x = vel.x + (normal->x * jm - k * tangent.x);
    vel.y = vel.y + (normal->y * jm - k * tangent.y);
    vel.z = vel.z + (normal->z * jm - k * tangent.z);
    rb_rotate_inverse(&tangent, &tangent);
    rb_cross(&tmp, r, &tangent);
    k = k * s->mass;
    angVel.x = angVel.x + (j * rn.x - k * tmp.x) / s->inertia.x;
    angVel.y = angVel.y + (j * rn.y - k * tmp.y) / s->inertia.y;
    angVel.z = angVel.z + (j * rn.z - k * tmp.z) / s->inertia.z;
    return 1;
}

// FUNC 0x001A10F8 Gadget::collideCars
//   For each car box (cars with a controller, in the gadget's sector or a
//   neighbour of it) whose 5 planes all have the bounding sphere within
//   reach: the first contact point inside the box gets the car impulse,
//   the contact counters are cleared (the gadget wakes up), the car is told
//   (car_report_crash: a course object, with the impulse) and the search
//   ends.
//   Matching (81.0%): the radius test as a while loop (retail peels its first
//   plane: duplicate_loop_exit_test).  Retail strength-reduces the plane
//   pointer of the outer loop and spills s and pts; ours hoists %hi of the
//   table by GCSE (outer loop phony).  Tried: no planes local, a table pointer.
void Gadget::collideCars(Vec3 *pts, int numPoints)
{
    CarBoxList  *boxes = RigidBodyManager::GetCarBoxes();
    int         count = boxes->count;
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);    /* sp+0x04 */
    int         i, j, k;

    for (i = 0; i < count; i++) {
        CarBox *car = &boxes->box[i];
        Vec4   *planes = g_002C0568[i];

        if (car_no_controller(car->car) != 0)
            continue;
        if (car->sector != sector &&
            runway_is_neighbour(RigidBodyManager::GetCourse()->data->runway, car->sector, sector) == 0)
            continue;
        j = 0;
        while (j < RB_BOX_PLANES &&
               !(s->radius < planes[j].x * pos.x + planes[j].y * pos.y + planes[j].z * pos.z + planes[j].w))
            j++;
        if (j != RB_BOX_PLANES)
            continue;
        for (k = 0; k < numPoints; k++) {
            f32 impulse;
            for (j = 0; j < RB_BOX_PLANES; j++)
                if (0.0f < planes[j].x * pts[k].x + planes[j].y * pts[k].y + planes[j].z * pts[k].z +
                           planes[j].w)
                    break;
            if (j != RB_BOX_PLANES)
                continue;
            impulse = carImpulse(k, i, &pts[k], car);
            clearContacts();
            pod_crash_reaction(car->car, CRASH_KIND_GADGET, CRASH_PART_GADGET);   /* Lupo 2 (L2 0x001DE7B0) */
            car_report_crash(car->car, CRASH_KIND_GADGET, CRASH_PART_GADGET, impulse);
            return;
        }
    }
}

// FUNC 0x001A1370 Gadget::carImpulse
//   Contact point `point' (world p) inside car box `box': the gadget is
//   pushed out through the nearest face (pushOut), then an impulse along
//   that face's normal, j = -1.01 (v_rel . n) / (1/m + n.((I'^-1 (r x n)) x
//   r)) with the inertia scaled by 1.5 and v_rel = the point's velocity -
//   the car's velocity at p (v + yawRate x (p - pos) in the car's frame,
//   mapped (x, y, z) -> (x, z, -y)); no friction.  Returns j.
//   The difference p - car stored first into sp+0x00 is overwritten by the
//   cross product before use (dead).  Latent: pushOut's -1 (no face below
//   100000) would index the plane before the box's first.
//   Matching (99.5%): only the order of the argument moves of
//   rb_rotate_inverse (retail a0 then a1).  Tried: the plane as a Vec4 pointer
//   through xyz(), a pointer to nBody, the face in a variable.
f32 Gadget::carImpulse(int point, int box, const Vec3 *p, CarBox *car)
{
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    const Vec3  *r = (const Vec3 *)&s->points[point];
    Vec3        tmp;                        /* sp+0x00 */
    Vec3        irn;                        /* sp+0x10 I'^-1 (r x nBody) */
    Vec3        carVel;                     /* sp+0x20 the car's velocity at p */
    Vec3        pointVel;                   /* sp+0x30 the point's velocity */
    Vec3        rel;                        /* sp+0x40 */
    Vec3        nBody;                      /* sp+0x50 */
    Vec3        rn;                         /* sp+0x60 r x nBody */
    const Vec3  *normal;
    f32         Ix, Iy, Iz, denom, j, jm;

    const CarBoxState *st = car_box_state(car);
    tmp.x = p->x - st->pos[0];
    tmp.y = p->y - st->pos[2];
    tmp.z = p->z + st->pos[1];
    carVel.x = st->vel[0] + tmp.z * st->yawRate;
    carVel.y = st->vel[2];
    carVel.z = -st->vel[1] - tmp.x * st->yawRate;
    rb_cross(&tmp, &angVel, r);
    rb_rotate(&pointVel, &tmp);
    pointVel.x = pointVel.x + vel.x;
    pointVel.y = pointVel.y + vel.y;
    pointVel.z = pointVel.z + vel.z;
    rel.x = pointVel.x - carVel.x;
    rel.y = pointVel.y - carVel.y;
    rel.z = pointVel.z - carVel.z;
    normal = (const Vec3 *)&g_002C0568[box][pushOut(box, p)];
    vu0_transpose_vf20_to_vf24();
    rb_rotate_inverse(&nBody, normal);
    rb_cross(&rn, r, &nBody);
    Ix = s->inertia.x * RB_CAR_INERTIA;
    Iy = s->inertia.y * RB_CAR_INERTIA;
    Iz = s->inertia.z * RB_CAR_INERTIA;
    irn.x = rn.x / Ix;
    irn.y = rn.y / Iy;
    irn.z = rn.z / Iz;
    rb_cross(&tmp, &irn, r);
    rb_rotate(&tmp, &tmp);
    denom = s->invMass + rb_dot(normal, &tmp);
    j = (rb_dot(normal, &rel) * RB_CAR_IMPULSE_GAIN) / denom;
    jm = j * s->invMass;
    vel.x = vel.x + jm * normal->x;
    vel.y = vel.y + jm * normal->y;
    vel.z = vel.z + jm * normal->z;
    angVel.x = angVel.x + (j * rn.x) / Ix;
    angVel.y = angVel.y + (j * rn.y) / Iy;
    angVel.z = angVel.z + (j * rn.z) / Iz;
    return j;
}

// FUNC 0x001A16C0 Gadget::pushOutAlong
//   No callers.  Car box `box': the face hit first by the ray p + t dir
//   (faces facing the ray only, the smallest t below 100000); the gadget is
//   moved back by t dir.  Returns the face, -1 if none.
int Gadget::pushOutAlong(int box, const Vec3 *p, const Vec3 *dir)
{
    Vec4 *planes = g_002C0568[box];
    f32  best = RB_FACE_DEPTH_MAX;
    int  face = -1;
    int  j;

    for (j = 0; j < RB_BOX_PLANES; j++) {
        f32 dn = rb_dot(dir, xyz(&planes[j]));
        f32 t;
        if (0.0f <= dn)
            continue;
        t = (rb_dot(p, xyz(&planes[j])) + planes[j].w) / dn;
        if (t < best) {
            best = t;
            face = j;
        }
    }
    if (face < 0)
        return -1;
    pos.x = pos.x - best * dir->x;
    pos.y = pos.y - best * dir->y;
    pos.z = pos.z - best * dir->z;
    return face;
}

// FUNC 0x001A1808 Gadget::pushOut
//   Car box `box', the point p inside: the face of the smallest depth
//   -(n.p + d) (below 100000); the gadget is moved out by it along the face
//   normal.  Returns the face, -1 if none.
//   Matching (86.2%): retail keeps `if (face < 0) return -1;' as a fall-through
//   after bgez; egcs here hoists box * 80 above the branch (interblock
//   scheduling) and reorg inverts it.  Tried: planes local (66.9), if / else
//   forms, a pointer to the face, direct indexing.
int Gadget::pushOut(int box, const Vec3 *p)
{
    f32  best = RB_FACE_DEPTH_MAX;
    int  face = -1;
    int  j;

    for (j = 0; j < RB_BOX_PLANES; j++) {
        f32 depth = -(rb_dot(p, xyz(&g_002C0568[box][j])) + g_002C0568[box][j].w);
        if (depth < best) {
            best = depth;
            face = j;
        }
    }
    if (face < 0)
        return -1;
    const Vec4 *n = &g_002C0568[box][face];
    pos.x = pos.x + best * n->x;
    pos.y = pos.y + best * n->y;
    pos.z = pos.z + best * n->z;
    return face;
}

// FUNC 0x001A1930 Gadget::collideWall
//   The wall from `wall' to the next vertex (in x / z): 1) the bounding
//   circle overlaps the segment (0 < t < 1): the gadget is pushed out
//   sideways to the radius (on the side of the wall normal (dz, -dx));
//   2) else, when the step crossed the wall line (the new position on the
//   right of it or on it, the old one not on the right) between its ends:
//   the gadget is moved back to the crossing.  Both call wallImpulse and
//   return 1; else 0.  Soft walls (WALL_SOFT) are ignored.
//   Matching (63.6%): retail reloads end->x / end->z for the last test (the
//   move stores kill them: end's base is unknown there) and keeps GCSE copies
//   of wall->x / z and pos.x / z (re-read loads).  The end computation is
//   retail's in this form; a ?: end (68.3%) reloads but computes end
//   differently.  Tried: re-reading wall / pos in the crossing tests, an inline
//   vector subtraction for move.
int Gadget::collideWall(const WallSeg *wall, const Vec3 *oldPos, Vec3 *pts)
{
    const WallSeg *end;
    GadgetShape   *s;
    Vec3          move;                     /* sp+0x00 the step */
    f32           ax, az, px, pz, rx, rz, dx, dz, len2, t, side, oldSide, f;

    if (wall->flags & WALL_SOFT)
        return 0;
    end = wall + 1;
    if (wall->link < 0)
        end += wall->link;
    ax = wall->x;
    az = wall->z;
    px = pos.x;
    pz = pos.z;
    rx = px - ax;
    rz = pz - az;
    dx = end->x - ax;
    dz = end->z - az;
    s = RigidBodyManager::GetGadgetShape(shape);
    len2 = dx * dx + dz * dz;
    t = (dx * rx + dz * rz) / len2;
    if (0.0f < t && t < 1.0f) {
        f32 radius = s->radius;
        f32 qz = t * dz - rz;               /* the closest point of the wall - pos */
        f32 qx = t * dx - rx;
        if (qx * qx + qz * qz < radius * radius) {
            f32 k = ee_rsqrt(radius, len2); /* radius / |d| */
            pos.z = pz + (qz - dx * k);
            pos.x = px + (qx + dz * k);
            wallImpulse(pts, dx, dz);
            return 1;
        }
    }
    side = dx * rz - dz * rx;
    if (side < 0.0f)
        return 0;
    oldSide = dx * (oldPos->z - az) - dz * (oldPos->x - ax);
    if (0.0f < oldSide)
        return 0;
    move.x = px - oldPos->x;
    move.z = pz - oldPos->z;
    move.y = pos.y - oldPos->y;
    if (move.x * (az - pz) - move.z * (ax - px) < 0.0f)
        return 0;
    if (0.0f < move.x * (end->z - pz) - move.z * (end->x - px))
        return 0;
    f = side / (side - oldSide);
    pos.z = pz - f * move.z;
    pos.x = px - f * move.x;
    pos.y = pos.y - f * move.y;
    wallImpulse(pts, dx, dz);
    return 1;
}

// FUNC 0x001A1B68 Gadget::wallImpulse
//   The wall normal n = (dz, 0, -dx) / |d|; the impulse at the contact point
//   with the smallest n.p (point 0 when there is none).
void Gadget::wallImpulse(Vec3 *pts, f32 dx, f32 dz)
{
    f32         invLen = ee_rsqrt(1.0f, dx * dx + dz * dz);
    GadgetShape *s = RigidBodyManager::GetGadgetShape(shape);
    int         numPoints = s->count;
    f32         best = RB_FLT_MAX;
    int         i, point = 0;
    Vec3        normal;                     /* sp+0x00 */

    normal.x = dz * invLen;
    normal.y = 0.0f;
    normal.z = -dx * invLen;
    for (i = 0; i < numPoints; i++) {
        f32 d = normal.x * pts[i].x + normal.y * pts[i].y + normal.z * pts[i].z;
        if (d < best) {
            best = d;
            point = i;
        }
    }
    applyImpulse(point, &normal);
}

// FUNC 0x001A1C48 Gadget::rotationMatrix
void Gadget::rotationMatrix(Matrix44 *m)
{
    rb_euler_matrix(m, rot.x, rot.y, rot.z);
}

// FUNC 0x001A1C78 F_001A1C78
//   No callers; empty (between two Gadget members: ? an empty Gadget
//   method).
void F_001A1C78(void)
{
}

// FUNC 0x001A1C80 Gadget::shadowMatrix
//   The planar projection along the light vector L onto the ground plane P:
//   m = (P.L) I - L P^T (row-vector convention).
void Gadget::shadowMatrix(Matrix44 *m)
{
    Vec3 *light = RigidBodyManager::GetLightVector();
    f32  d = rb_dot(xyz(&plane), light);

    m->m[0][0] = d - plane.x * light->x;
    m->m[1][0] = -plane.y * light->x;
    m->m[2][0] = -plane.z * light->x;
    m->m[3][0] = -plane.w * light->x;
    m->m[0][1] = -plane.x * light->y;
    m->m[1][1] = d - plane.y * light->y;
    m->m[2][1] = -plane.z * light->y;
    m->m[3][1] = -plane.w * light->y;
    m->m[0][2] = -plane.x * light->z;
    m->m[1][2] = -plane.y * light->z;
    m->m[2][2] = d - plane.z * light->z;
    m->m[3][2] = -plane.w * light->z;
    m->m[0][3] = 0.0f;
    m->m[1][3] = 0.0f;
    m->m[2][3] = 0.0f;
    m->m[3][3] = d;
}

// FUNC 0x001A1DD0 Gadget::draw
//   When visible: the shadow (the projection pushed with a depth shift of
//   0.005, model view * shadow matrix * body matrix, no depth writes, the
//   model's shadow part), then the model itself (model view * body matrix,
//   its body part).
void Gadget::draw()
{
    Matrix44 body __attribute__((aligned(16)));     /* sp+0x00 */
    Matrix44 shadow __attribute__((aligned(16)));   /* sp+0x40 */

    if (isVisible() == 0)
        return;
    rotationMatrix(&body);
    body.m[3][0] = pos.x;
    body.m[3][1] = pos.y;
    body.m[3][2] = pos.z;
    shadowMatrix(&shadow);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    proj_shift_depth(RB_SHADOW_DEPTH);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    pgl_mult_matrix(&shadow.m[0][0]);
    pgl_mult_matrix(&body.m[0][0]);
    gfx_depth_mask(0);
    modelset_draw_part(RigidBodyManager::GetModel(shape), GADGET_PART_SHADOW);
    gfx_depth_mask(1);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    pgl_push_matrix();
    pgl_mult_matrix(&body.m[0][0]);
    modelset_draw_part(RigidBodyManager::GetModel(shape), GADGET_PART_BODY);
    pgl_pop_matrix();
}

// FUNC 0x001A1EF8 Gadget::isVisible
//   The box pos +- radius in the view (box_visible != 0).
int Gadget::isVisible()
{
    f32 lo[4];                              /* sp+0x00 (w not set) */
    f32 hi[4];                              /* sp+0x10 */
    f32 radius = RigidBodyManager::GetGadgetShape(shape)->radius;

    lo[0] = pos.x - radius;
    lo[1] = pos.y - radius;
    lo[2] = pos.z - radius;
    hi[0] = pos.x + radius;
    hi[1] = pos.y + radius;
    hi[2] = pos.z + radius;
    return box_visible(lo, hi) != 0;
}

/* ======================================================================== */
/* RigidBodyManager                                                         */
/* ======================================================================== */

// FUNC 0x001A1F78 RigidBodyManager::RigidBodyManager
//   A pool of 100 gadgets (operator new[] of 0x1F40 bytes, no cookie: each
//   next = 0 by Gadget's inline constructor).  The lists are set up by
//   reset().  Caller: RacePS2Base's constructor.
RigidBodyManager::RigidBodyManager()
{
    capacity = RB_POOL_SIZE;
    pool = new Gadget[RB_POOL_SIZE]; /* implicit: operator new[] (new-expression) */
    maxShape = 0;
}

// FUNC 0x001A1FE8 RigidBodyManager::~RigidBodyManager
//   Quirk: the new[] pool is freed with the scalar operator delete
//   (0x0027C0C0), not operator delete[].
RigidBodyManager::~RigidBodyManager()
{
    delete pool; /* implicit: operator delete (delete-expression) */
}

// FUNC 0x001A2040 RigidBodyManager::reset
//   No gadget active, all in the free list (pool order); maxShape -1.
void RigidBodyManager::reset()
{
    int n = capacity;
    int i;

    maxShape = -1;
    active = NULL;
    freeList = pool;
    for (i = 0; i < n - 1; i++)
        pool[i].next = &pool[i] + 1;
    pool[n - 1].next = NULL;
}

// FUNC 0x001A20A8 RigidBodyManager::add
//   A gadget from a runway record (0 when the pool is exhausted).
int RigidBodyManager::add(const GadgetDesc *d)
{
    Gadget *g = alloc();

    if (g == NULL)
        return 0;
    g->init(d);
    if (maxShape < d->shape)
        maxShape = d->shape;
    return 1;
}

// FUNC 0x001A2108 RigidBodyManager::resetShapeMass
//   No callers.  Shapes 0..maxShape get the inertia and the mass of the
//   template g_002C04B8.
void RigidBodyManager::resetShapeMass()
{
    GadgetShape *t = &g_002C04B8;
    int          n = maxShape + 1;
    int          i;

    for (i = 0; i < n; i++) {
        GadgetShape *s = GetGadgetShape(i);
        s->inertia.x = t->inertia.x;
        s->inertia.y = t->inertia.y;
        s->inertia.z = t->inertia.z;
        s->mass = t->mass;
        s->invMass = 1.0f / t->mass;
    }
}

// FUNC 0x001A2188 RigidBodyManager::alloc
//   The head of the free list, pushed onto the active list (NULL if none).
Gadget *RigidBodyManager::alloc()
{
    Gadget *g = freeList;

    if (g == NULL)
        return NULL;
    freeList = g->next;
    g->next = active;
    active = g;
    return g;
}

// FUNC 0x001A21B8 RigidBodyManager::release
//   Unlinks g (prev: its predecessor in the active list, NULL for the head)
//   and pushes it onto the free list.
void RigidBodyManager::release(Gadget *g, Gadget *prev)
{
    if (prev != NULL)
        prev->next = g->next;
    else
        active = g->next;
    g->next = freeList;
    freeList = g;
}

// FUNC 0x001A21E0 RigidBodyManager::update
//   Per frame (RacePS2Base::virtual_28): the car boxes, then a step of
//   every active gadget; those off the road are released.
void RigidBodyManager::update(f32 dt)
{
    Gadget *prev = NULL;
    Gadget *g, *next;

    updateCarBoxes();
    g = active;
    while (g != NULL) {
        next = g->next;
        if (g->step(dt) == 0)
            release(g, prev);
        else
            prev = g;
        g = next;
    }
}

// FUNC 0x001A2278 RigidBodyManager::SetLightVector
//   The light direction of the shadows.  Callers: RacePS2Base::drawView /
//   virtual_47 (CourseEnv's first B light, T178).  /* ? */ name only: no
//   symbol; GetLightVector's (lupo2) counterpart, a real (not inline)
//   member, as it sits before updateCarBoxes.
void RigidBodyManager::SetLightVector(const Vec3 *v)
{
    g_002C04A8.x = v->x;
    g_002C04A8.y = v->y;
    g_002C04A8.z = v->z;
}

// FUNC 0x001A22A0 RigidBodyManager::updateCarBoxes
//   For each car (with a controller): the current matrix = translate
//   (x, z, -y) * rotate y (yaw, reset to 0 beyond 32 pi); the 6 face points
//   of the box (front, -x side, rear, +x side at y = 1, the top centre (0, 1, 0),
//   the bottom centre (0, 0, 0): transformed but unused) and the 5 normals
//   of g_0032FEF8 are transformed; plane j = (R n_j, -(R n_j) . p_j).
//   `this' is not used.
//   Matching (87.9%): retail does not hoist the 1.0 out of the outer loop
//   (loop.c skips it) and so allocates differently; ours hoists it.  The
//   plane table indexed directly with casts (not xyz()) and the state through
//   car_box_state() gave the rest.
void RigidBodyManager::updateCarBoxes()
{
    int n = GetCarBoxes()->count;
    int i, j;

    for (i = 0; i < n; i++) {
        CarBox *car = &GetCarBoxes()->box[i];
        Vec4   local[6] __attribute__((aligned(16)));   /* sp+0x00 */
        Vec4   world[6] __attribute__((aligned(16)));   /* sp+0x60 */
        f32    front, rear, halfWidth;

        CarBoxState *st;

        if (car_no_controller(car->car) != 0)
            continue;
        front = car->front;
        rear = car->rear;
        halfWidth = car->halfWidth;
        st = car_box_state(car);
        vu0_load_identity_regs();
        pgl_translate(st->pos[0], st->pos[2], -st->pos[1]);
        if (RB_YAW_WRAP < boot_fabsf(st->yaw))
            st->yaw = 0.0f;
        vu0_rotate_y(st->yaw * RB_RAD_TO_DEG);
        local[0].x = halfWidth;             /* front */
        local[0].y = 1.0f;
        local[0].z = -front;
        local[0].w = 1.0f;
        local[1].x = -halfWidth;            /* the -x side */
        local[1].y = 1.0f;
        local[1].z = -front;
        local[1].w = 1.0f;
        local[2].x = -halfWidth;            /* rear */
        local[2].y = 1.0f;
        local[2].z = rear;
        local[2].w = 1.0f;
        local[3].x = halfWidth;             /* the +x side */
        local[3].y = 1.0f;
        local[3].z = rear;
        local[3].w = 1.0f;
        local[4].x = 0.0f;                  /* top */
        local[4].y = 1.0f;
        local[4].z = 0.0f;
        local[4].w = 1.0f;
        memset(&local[5], 0, sizeof(local[5])); /* bottom */
        local[5].w = 1.0f;
        for (j = 0; j < 6; j++)
            vu0_apply_matrix(&world[j].x, &local[j].x);
        for (j = 0; j < RB_BOX_PLANES; j++) {
            vu0_apply_matrix(&g_002C0568[i][j].x, &g_0032FEF8[j].x);
            g_002C0568[i][j].w = -rb_dot((const Vec3 *)&g_002C0568[i][j], (const Vec3 *)&world[j]);
        }
    }
}

// FUNC 0x001A24B0 RigidBodyManager::draw
//   Callers: RacePS2Base::drawView / virtual_47.
void RigidBodyManager::draw()
{
    Gadget *g;

    for (g = active; g != NULL; g = g->next)
        g->draw();
}

// FUNC 0x001A24E8 RigidBodyManager::GetLightVector
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy */

// FUNC 0x001A24F8 RigidBodyManager::GetModel
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy */

// FUNC 0x001A2510 RigidBodyManager::GetGadgetShape
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy */

// FUNC 0x001A2528 RigidBodyManager::SetGadgetShapes
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy (no callers: T130
 * stores g_00352174 inline).  ? name: no symbol (u057's). */

// FUNC 0x001A2530 RigidBodyManager::GetCarBoxes
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy.  ? name: no
 * symbol (u057's). */

// FUNC 0x001A2538 RigidBodyManager::SetCarBoxes
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy (T130 stores
 * g_00352178 inline).  ? name: no symbol (u057's). */

// FUNC 0x001A2540 RigidBodyManager::GetCourse
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy.  ? name: no
 * symbol (u057's). */

// FUNC 0x001A2548 RigidBodyManager::SetCourse
/* inline in game/car/physics/T167_rigid_body.h; egcs emits this copy (T130 stores
 * g_0035217C inline).  ? name: no symbol (u057's). */

#ifndef BOOT_OLD_GCC
// FUNC 0x001A2550 __tf16RigidBodyManager
//   The __user_type_info __ti16RigidBodyManager (gp-0x4638).
extern "C" const type_info &__tf16RigidBodyManager(void)
{
    if (__ti16RigidBodyManager[0] == 0)
        __rtti_user(__ti16RigidBodyManager, "16RigidBodyManager");
    return *(const type_info *)__ti16RigidBodyManager;
}
#endif
