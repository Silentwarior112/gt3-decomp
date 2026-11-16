/* vu0_math.c - VU0 math helpers of the graphics library (G001),
 * 0x00245C10..0x00245E58: atan2 through a VU0 micro subroutine, the VU0
 * random generator (vrinit / vrnext) and a small LCG.
 */
#include "engine/pgl/G001_flare.h"   /* this translation unit (G001) */
#include "vu0asm.h"
#include "engine/pgl/pgl_effects_internal.h"

/* The octant tables of vu0_atan2f (octant = 4 y < 0 | 2 x < 0 | 1 |y| > |x|):
 * the sign of the arctangent and the base it is added to. */
const f32 g_00343CE8[8] = {             /* 0x00343CE8 */
    1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f
};
const f32 g_00343D08[10] = {            /* 0x00343D08 */
    0.0f, 1.5707964f, 3.1415927f, 1.5707964f, 0.0f, -1.5707964f, -3.1415927f, -1.5707964f,
    0.0f, 0.0f                          /* (not indexed: octant < 8) */
};

/* The LCG state (lcg_rand_seed / lcg_rand12): initialised data, .sdata. */
u32 g_003530AC = 0;                     /* 0x003530AC */

/* vf1.x = a, vf2.x = b: the arguments of the arctangent micro subroutine. */
static inline void vu0_atan_args(f32 a, f32 b)
{
    __asm__ volatile("qmtc2.ni %0, $vf1" : : "r"(a));
    __asm__ volatile("qmtc2.ni %0, $vf2" : : "r"(b));
}

// FUNC 0x00245C10 vu0_atan2f
/* atan2(y, x).  The axes are answered directly (y == 0: 0 for x >= 0, else
 * pi; x == 0: +-pi/2 by the sign of y).  Otherwise vf1.x = max(|x|, |y|),
 * vf2.x = min and the VU0 micro subroutine at 0x2D0 computes the arctangent
 * of the ratio into vf4.x (vcallms; qmfc2.i waits for it).  The octant
 * (4: y < 0, 2: x < 0, 1: |y| > |x|) indexes a sign table and a base table:
 * result = +-atan + base.
 * The result comes back through one asm (qmfc2.i + mtc1: retail keeps the
 * mtc1 right behind the qmfc2).  98.8%: retail moves the second argument
 * through $v1 (the first through $v0); here both qmtc2 reloads take $v0.
 * Tried: u32 conversions (union, locals, an inline with u32 parameters),
 * both transfers in one asm (retail fills the b delay slot with the second
 * qmtc2, so they are two asms). */
f32 vu0_atan2f(f32 y, f32 x)
{
    f32 ax, ay;
    f32 sign;
    f32 base;
    f32 atan;
    u32 bits;
    int octant = 0;

    if (y == 0.0f) {
        if (0.0f <= x)
            return 0.0f;
        return 3.1415927f;
    }
    if (x == 0.0f) {
        if (0.0f <= y)
            return 1.5707964f;
        return -1.5707964f;
    }
    ax = x;
    if (ax < 0.0f)
        ax = -ax;
    ay = y;
    if (ay < 0.0f)
        ay = -ay;
    if (ay <= ax)
        vu0_atan_args(ax, ay);
    else
        vu0_atan_args(ay, ax);
    __asm__ volatile("vnop\n\tvnop\n\tvcallms 0x2D0");

    if (y < 0.0f)
        octant = 4;
    if (x < 0.0f)
        octant |= 2;
    if (ax < ay)
        octant |= 1;
    sign = -1.0f;
    base = g_00343D08[octant];
    if (0.0f <= g_00343CE8[octant])
        sign = 1.0f;
    __asm__ volatile("qmfc2.i %1, $vf4\n\tmtc1 %1, %0" : "=f"(atan), "=r"(bits));   /* waits for the subroutine */
    if (sign < 0.0f)
        atan = -atan;
    return atan + base;
}

// FUNC 0x00245D5C vu0_rand
/* VU0 random number: vrnext gives a float in [1, 2); returns it - 1.
 * Hand-written assembly in the original, like vu0_rand_seed below: the
 * transfer register is $t0 ($8), where egcs takes $v0 (vu0_atan2f), and the
 * compiled form `asm(... mtc1 $8,%0); return r - 1.0f;' gives the value $f1
 * and the 1.0 $f0, the other way round.  The egcs build assembles the retail
 * instructions; the modern build keeps the equivalent C.
 *   vrnext.x $vf1,R; qmfc2.ni $t0,$vf1; mtc1 $t0,$f0; lui $at,0x3F80;
 *   mtc1 $at,$f1; jr $ra; sub.s $f0,$f0,$f1 */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
__asm__("\t.text\n"
        "\t.p2align 3\n"
        "\t.globl\tvu0_rand\n"
        "\t.ent\tvu0_rand\n"
        "\t.type\tvu0_rand,@function\n"
        "vu0_rand:\n"
        "\t.set\tnoreorder\n"
        "\t.set\tnoat\n"
        "\tvrnext.x\t$vf1, " VU0_R "\n"
        "\tqmfc2.ni\t$8, $vf1\n"
        "\tmtc1\t$8, $f0\n"
        "\tlui\t$1, 0x3F80\n"
        "\tmtc1\t$1, $f1\n"
        "\tjr\t$31\n"
        "\tsub.s\t$f0, $f0, $f1\n"
        "\t.set\tat\n"
        "\t.set\treorder\n"
        "\t.end\tvu0_rand\n"
        "\t.size\tvu0_rand, . - vu0_rand\n"
        "\t.previous\n");
#else
f32 vu0_rand(void)
{
    f32 r;

    __asm__ volatile("vrnext.x $vf1, " VU0_R "\n\tqmfc2.ni $8, $vf1\n\tmtc1 $8, %0"
                     : "=f"(r) : : "$8");
    return r - 1.0f;
}
#endif

// FUNC 0x00245D78 vu0_rand_seed
/* Seeds the VU0 random generator with the bits of seed (vrinit R, vf1x).
 * The function has no jr $ra: after vrinit (and a nop) it runs on into
 * lcg_rand_seed (0x00245D88), whose jr / sw store the caller's $a0 into the
 * LCG state.  lcgSeed models that leftover $a0 (RaceSmoke::reset 0x001A4630
 * passes this + 4).
 * Hand-written assembly in the original: no compiled function lacks its
 * jr $ra, and the transfer register is $t0 ($8).  The egcs build assembles
 * the retail instructions as a top-level asm block right in front of
 * lcg_rand_seed (whose .p2align 3 gives the nop at 0x00245D84, then the
 * fall-through); the modern build keeps the equivalent C.
 *   mfc1 $t0,$f12; qmtc2.i $t0,$vf1; vrinit R,$vf1x */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
__asm__("\t.text\n"
        "\t.p2align 3\n"
        "\t.globl\tvu0_rand_seed\n"
        "\t.ent\tvu0_rand_seed\n"
        "\t.type\tvu0_rand_seed,@function\n"
        "vu0_rand_seed:\n"
        "\t.set\tnoreorder\n"
        "\tmfc1\t$8, $f12\n"
        "\tqmtc2.i\t$8, $vf1\n"
        "\tvrinit\t" VU0_R ", $vf1x\n"
        "\t.set\treorder\n"
        "\t.end\tvu0_rand_seed\n"
        "\t.size\tvu0_rand_seed, . - vu0_rand_seed\n"
        "\t.previous\n");
#else
void vu0_rand_seed(u32 lcgSeed, f32 seed)
{
    __asm__ volatile("qmtc2.i %0, $vf1\n\tvrinit " VU0_R ", $vf1x\n\tnop"
                     : : "r"(u113_f2u(seed)));
    g_003530AC = lcgSeed;                   /* falls into 0x00245D88 */
}
#endif

// FUNC 0x00245D88 lcg_rand_seed
/* RacePS2Base::virtual_8 seeds it with 0x7F7F7F7F before weather_init. */
void lcg_rand_seed(u32 seed)
{
    g_003530AC = seed;
}

// FUNC 0x00245D90 lcg_rand12
/* state = state * 0x5D588B65 + 1 (32-bit); returns the float with the low
 * 23 bits of the state as mantissa and exponent 0: [1, 2).
 * Retail stores the state in the middle of the computation and leaves the
 * jr delay slot empty: a store egcs keeps out of delay slots, i.e. a
 * volatile one (the volatile view; the variable itself is plain).  80.4%:
 * left is the register pairing of the three constants (retail: multiplier
 * $v1, mask $v0, 1.0f bits $v1; here the other way round; local-alloc
 * gives the multiplier priority).  Tried: unions, *(f32 *)& returns,
 * separate statements, s32 arithmetic, the mask or 1.0f in variables,
 * both compilers (991111-01 gives the same). */
f32 lcg_rand12(void)
{
    u32 state = g_003530AC * 0x5D588B65u + 1;

    *(volatile u32 *)&g_003530AC = state;
    return u113_u2f((state & 0x007FFFFF) | 0x3F800000);    /* 1.0f | mantissa */
}

// FUNC 0x00245DC8 lcg_rand_range
/* A uniform value in [lo, hi). */
f32 lcg_rand_range(f32 lo, f32 hi)
{
    f32 r12 = lcg_rand12();

    return (r12 - 1.0f) * (hi - lo) + lo;
}

// FUNC 0x00245E10 lcg_rand_chance
/* 1 with probability p (rand in [1, 2) < p + 1).  No references. */
int lcg_rand_chance(f32 p)
{
    f32 r12 = lcg_rand12();

    return r12 < p + 1.0f;
}
