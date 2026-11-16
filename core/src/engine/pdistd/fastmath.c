/* fastmath.c - single-precision math library (0x00269FC8..0x0026A7C8).
 *
 * The binary has no symbols for these; they are named after the libm
 * functions they compute (checked by behaviour; not newlib's code):
 *   atan2f  0x00269FC8   atanf   0x0026A088   cosf    0x0026A1A0
 *   expf    0x0026A270   logf    0x0026A388   powf    0x0026A4A0
 *   sincosf 0x0026A4D0   sinf    0x0026A5F0   tanf    0x0026A6C0
 * plus two 3-instruction "functions" (0x0026A37C, 0x0026A7B8) that are the
 * fall-through tails of expf and tanf.
 *
 * atan2f and powf are compiled C (-G8: atan2f loads pi/2 and pi from the
 * object's .lit4 through $gp; every div.s carries the compiler's two hazard
 * nops).  The other seven are hand-written assembly, which the matching
 * build keeps as the retail instructions (FASTMATH_RETAIL_ASM below):
 *   - they use the R5900 FPU accumulator (adda.s / suba.s / mula.s /
 *     madda.s set ACC; madd.s / msub.s compute fd = ACC +- fs*ft WITHOUT
 *     changing ACC), which cc1 has no patterns for (its only madd.s is the
 *     4-operand MIPS IV one);
 *   - their div.s lack the two nops that egcs's divsf3 puts before every
 *     div.s (970 of the image's 976 div.s have them; the 6 without are 5 here
 *     and 0x00257EFC), and tanf has a single nop before one;
 *   - tanf makes 0.0 as `sub.s $f5, $f5, $f5` of an unset register, cosf
 *     complements with `nor $3, $2, $2` (egcs: `nor rd, $0, rs`), and float
 *     constants are built in $2..$7 with lui / ori / mtc1 (egcs uses li.s,
 *     i.e. $at or the .lit4 pool as in atan2f).
 * Their C under #else (GCC 14, -DNON_MATCHING) writes the accumulator
 * sequences as the float arithmetic they compute, in the same order.
 *
 * Float constants have 9 significant digits rounded UP: GCC 14 converts
 * decimal literals for the R5900 rounding toward zero (one ulp low), egcs to
 * nearest (and egcs takes no hex float with an `f' in it); a decimal in
 * [f, f + ulp/2) gives f with both, see gs_driver_internal.h.  The hex spelling (and the
 * bits) are in the comments.
 */
#include "engine/media/G002_mpeg_frameq.h"
#include "engine/system/system_util_internal.h"

#define M_PI_2_F    1.57079638f         /* 0x3FC90FDB 0x1.921fb6p+0f pi/2 */
#define M_PI_F      3.14159275f         /* 0x40490FDB 0x1.921fb6p+1f pi */
#define M_PI_4_F    0.785398186f        /* 0x3F490FDB 0x1.921fb6p-1f pi/4 */
#define M_1_PI_F    0.318309874f        /* 0x3EA2F983 0x1.45f306p-2f 1/pi */
#define M_2_PI_F    0.636619747f        /* 0x3F22F983 0x1.45f306p-1f 2/pi */
#define M_LOG2E_F   1.44269503f         /* 0x3FB8AA3B 0x1.715476p+0f log2(e) */
#define M_LN2_F     0.693147183f        /* 0x3F317218 0x1.62e43p-1f ln(2) */

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* Matching build: the hand-written functions are the retail instructions in
 * top-level asm blocks (registers numeric, delay slots explicit, `$L<fn>_<pc>'
 * labels named after the retail address's low bits).  The div.s padding pass
 * of tools/mcc.py leaves asm alone (#APP), so their unpadded div.s stay. */
#define FASTMATH_RETAIL_ASM 1
#define FM_ASM_BEGIN(name) \
    "\t.text\n\t.p2align\t3\n\t.set\tpush\n\t.set\tnoreorder\n" FM_ASM_ENTRY(name)
#define FM_ASM_ENTRY(name) \
    "\t.globl\t" #name "\n\t.ent\t" #name "\n\t.type\t" #name ", @function\n" #name ":\n"
#define FM_ASM_SIZE(name)   "\t.end\t" #name "\n\t.size\t" #name ", .-" #name "\n"
#define FM_ASM_END          "\t.set\tpop\n\t.previous\n"
#endif

#ifndef FASTMATH_RETAIL_ASM
/* The odd polynomial of cosf / sinf / sincosf:
 * sign * sin(t) ~= st - st^3/6 + ... for t in [-pi/2, pi/2], st = t * sign.
 * Instruction order: t2 = t*t, st = t*sign, t4 = t2*t2, st3 = t2*st,
 * ACC = st + 0, st5 = t4*st, st7 = t4*st3, ACC += c3*st3, st9 = st5*t4,
 * ACC += c5*st5, ACC += c7*st7, result = ACC + c9*st9.  Inlined in each. */
static inline f32 u124_sin_poly(f32 t, f32 sign)
{
    f32 t2  = t * t;
    f32 st  = t * sign;
    f32 t4  = t2 * t2;
    f32 st3 = t2 * st;
    f32 st5 = t4 * st;
    f32 st7 = t4 * st3;
    f32 st9 = st5 * t4;
    f32 acc;

    acc  = st + 0.0f;                           /* adda.s st, $zero-reg      */
    acc += -0.166666568f * st3;                 /* 0xBE2AAAA4 -0x1.555548p-3f   */
    acc += 0.00833302551f * st5;                /* 0x3C08873E  0x1.110e7cp-7f  */
    acc += -0.000198074137f * st7;              /* 0xB94FB21F -0x1.9f643ep-13f */
    return acc + 2.601887e-06f * st9;           /* 0x362E9C14  0x1.5d3828p-19f (madd.s) */
}
#endif

// FUNC 0x00269FC8 atan2f
/* atan2(y, x) via atanf of the smaller / larger magnitude ratio, then the
 * octant fixes (pi/2 - r when |y| >= |x|, pi - r when x < 0, negate when
 * y < 0).  Quirk: returns 0 whenever x + y == 0, i.e. also for (1, -1) and
 * (-1, 1) (true results 3pi/4, -pi/4), not only for (0, 0).  One temporary
 * `c' holds x + y and then the two constants (retail: $f1 throughout, the
 * constant loads ahead of their branches; with the constants written into
 * the subtractions the loads sink into the branches and r leaves $f0).
 * 15 callers. */
f32 atan2f(f32 y, f32 x)
{
    f32 r = 0.0f;
    f32 c;
    int q = 0;

    c = x + y;
    if (c == 0.0f)
        return r;
    if (y < 0.0f) {
        y = -y;
        q = 4;
    }
    if (x < 0.0f) {
        x = -x;
        q |= 2;
    }
    if (y < x) {
        r = y / x;
    } else {
        q |= 1;
        r = x / y;
    }
    r = atanf(r);
    c = M_PI_2_F;
    if (q & 1)
        r = c - r;
    c = M_PI_F;
    if (q & 2)
        r = c - r;
    if (q & 4)
        r = -r;
    return r;
}

// FUNC 0x0026A088 atanf
/* Range reduction on a = |x|: a > tan(3pi/8) -> 1/a (result pi/2 - r),
 * a > tan(pi/8) -> (1 - a) / (1 + a) (result pi/4 - r); both quotients are
 * computed up front.  Then the rational approximation
 * r = a (1 + 0.7778 a^2 + 0.06772 a^4) / (1 + 1.1111 a^2 + 0.2381 a^4),
 * computed as num * (1 / den), and the sign of x.  The middle range is
 * flagged with lui 0xFFFF (any negative value).  Only caller atan2f. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(atanf)
"\tdaddu\t$4, $0, $0\n"
"\tabs.s\t$f9, $f12\n"
"\tmfc1\t$5, $f12\n"
"\tlui\t$3, 0x3F80\n"
"\tmtc1\t$3, $f6\n"
"\tlui\t$3, 0x3ED4\n"
"\tlui\t$2, 0x401A\n"
"\tori\t$3, $3, 0x13CD\n"
"\tori\t$2, $2, 0x827A\n"
"\tdiv.s\t$f4, $f6, $f9\n"
"\tmtc1\t$4, $f7\n"
"\tsub.s\t$f3, $f6, $f9\n"
"\tmtc1\t$2, $f2\n"
"\tadd.s\t$f1, $f6, $f9\n"
"\tmtc1\t$3, $f0\n"
"\tc.lt.s\t$f2, $f9\n"
"\tdiv.s\t$f8, $f3, $f1\n"
"\tbc1f\t$Latanf_E0\n"
"\tc.lt.s\t$f0, $f9\n"
"\tmov.s\t$f9, $f4\n"
"\tb\t$Latanf_F0\n"
"\tori\t$4, $4, 0x1\n"
"$Latanf_E0:\n"
"\tbc1f\t$Latanf_F0\n"
"\tnop\n"
"\tmov.s\t$f9, $f8\n"
"\tlui\t$4, 0xFFFF\n"
"$Latanf_F0:\n"
"\tmul.s\t$f5, $f9, $f9\n"
"\tlui\t$3, 0x3F8E\n"
"\tadda.s\t$f6, $f7\n"
"\tori\t$3, $3, 0x38E4\n"
"\tmtc1\t$3, $f1\n"
"\tlui\t$3, 0x3E73\n"
"\tmul.s\t$f2, $f5, $f5\n"
"\tori\t$3, $3, 0xCF3D\n"
"\tmul.s\t$f4, $f5, $f9\n"
"\tmtc1\t$3, $f0\n"
"\tmadda.s\t$f1, $f5\n"
"\tlui\t$3, 0x3F47\n"
"\tori\t$3, $3, 0x1C72\n"
"\tmadd.s\t$f0, $f0, $f2\n"
"\tmtc1\t$3, $f3\n"
"\tadda.s\t$f9, $f7\n"
"\tlui\t$3, 0x3D8A\n"
"\tmul.s\t$f5, $f4, $f5\n"
"\tori\t$3, $3, 0xB356\n"
"\tmtc1\t$3, $f1\n"
"\tdiv.s\t$f0, $f6, $f0\n"
"\tlui\t$3, 0x3FC9\n"
"\tmadda.s\t$f3, $f4\n"
"\tori\t$3, $3, 0xFDB\n"
"\tmadd.s\t$f1, $f1, $f5\n"
"\tmtc1\t$3, $f3\n"
"\tlui\t$3, 0x3F49\n"
"\tori\t$3, $3, 0xFDB\n"
"\tmtc1\t$3, $f2\n"
"\tmul.s\t$f1, $f1, $f0\n"
"\tbgez\t$4, $Latanf_17C\n"
"\tnop\n"
"\tsub.s\t$f1, $f2, $f1\n"
"\tb\t$Latanf_188\n"
"\tnop\n"
"$Latanf_17C:\n"
"\tblez\t$4, $Latanf_188\n"
"\tnop\n"
"\tsub.s\t$f1, $f3, $f1\n"
"$Latanf_188:\n"
"\tbgez\t$5, $Latanf_194\n"
"\tnop\n"
"\tneg.s\t$f1, $f1\n"
"$Latanf_194:\n"
"\tjr\t$31\n"
"\tmov.s\t$f0, $f1\n"
FM_ASM_SIZE(atanf)
FM_ASM_END);
#else
f32 atanf(f32 x)
{
    int range = 0;
    f32 a = __builtin_fabsf(x);
    f32 inv = 1.0f / a;
    f32 mob = (1.0f - a) / (1.0f + a);
    f32 z, z2, a3, a5, acc, den, rden, num, r;

    if (2.41421366f < a) {              /* 0x401A827A 0x1.3504f4p+1f */
        a = inv;
        range = 1;
    } else if (0.414213568f < a) {      /* 0x3ED413CD 0x1.a8279ap-2f */
        a = mob;
        range = -0x10000;
    }

    z  = a * a;
    acc = 1.0f + 0.0f;                          /* adda.s 1.0, 0.0 */
    z2 = z * z;
    a3 = z * a;
    acc += 1.11111117f * z;                     /* 0x3F8E38E4 0x1.1c71c8p+0f */
    den = acc + 0.238095239f * z2;              /* 0x3E73CF3D 0x1.e79e7ap-3f */
    acc = a + 0.0f;
    a5 = a3 * z;
    rden = 1.0f / den;
    acc += 0.777777792f * a3;                   /* 0x3F471C72 0x1.8e38e4p-1f */
    num = acc + 0.0677248687f * a5;             /* 0x3D8AB356 0x1.1566acp-4f */
    r = num * rden;

    if (range < 0)
        r = M_PI_4_F - r;
    else if (range > 0)
        r = M_PI_2_F - r;
    if ((s32)u124_f2u(x) < 0)
        r = -r;
    return r;
}
#endif

// FUNC 0x0026A1A0 cosf
/* cos(x) = s * sin(x - pi/2 - k pi), k = trunc(x / pi) - (x < 0),
 * s = -1 when k is even, +1 when odd (sign bit = (x < 0) ^ (~n & 1)).
 * pi is formed as pi/2 + pi/2.  cvt.w.s truncates.  36 callers. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(cosf)
"\tlui\t$2, 0x3EA2\n"
"\tlui\t$3, 0x3FC9\n"
"\tori\t$2, $2, 0xF983\n"
"\tori\t$3, $3, 0xFDB\n"
"\tmtc1\t$2, $f1\n"
"\tmul.s\t$f1, $f12, $f1\n"
"\tmfc1\t$4, $f12\n"
"\tmtc1\t$3, $f0\n"
"\tcvt.w.s\t$f1, $f1\n"
"\tslti\t$4, $4, 0x0\n"
"\tadd.s\t$f3, $f0, $f0\n"
"\tmtc1\t$0, $f4\n"
"\tsuba.s\t$f12, $f0\n"
"\tmfc1\t$2, $f1\n"
"\tnor\t$3, $2, $2\n"
"\tsubu\t$2, $2, $4\n"
"\tandi\t$3, $3, 0x1\n"
"\tmtc1\t$2, $f0\n"
"\txor\t$4, $4, $3\n"
"\tcvt.s.w\t$f0, $f0\n"
"\tsll\t$4, $4, 31\n"
"\tlui\t$3, 0x3F80\n"
"\tor\t$4, $4, $3\n"
"\tmsub.s\t$f12, $f0, $f3\n"
"\tmtc1\t$4, $f2\n"
"\tmul.s\t$f1, $f12, $f12\n"
"\tlui\t$3, 0xBE2A\n"
"\tmul.s\t$f12, $f12, $f2\n"
"\tori\t$3, $3, 0xAAA4\n"
"\tlui\t$2, 0x3C08\n"
"\tmtc1\t$3, $f7\n"
"\tori\t$2, $2, 0x873E\n"
"\tlui\t$3, 0xB94F\n"
"\tmul.s\t$f0, $f1, $f1\n"
"\tmtc1\t$2, $f6\n"
"\tmul.s\t$f1, $f1, $f12\n"
"\tori\t$3, $3, 0xB21F\n"
"\tadda.s\t$f12, $f4\n"
"\tmtc1\t$3, $f5\n"
"\tmul.s\t$f3, $f0, $f12\n"
"\tlui\t$2, 0x362E\n"
"\tmul.s\t$f4, $f0, $f1\n"
"\tori\t$2, $2, 0x9C14\n"
"\tmadda.s\t$f7, $f1\n"
"\tmtc1\t$2, $f2\n"
"\tmul.s\t$f0, $f3, $f0\n"
"\tmadda.s\t$f6, $f3\n"
"\tmadda.s\t$f5, $f4\n"
"\tmadd.s\t$f0, $f2, $f0\n"
"\tjr\t$31\n"
"\tnop\n"
FM_ASM_SIZE(cosf)
FM_ASM_END);
#else
f32 cosf(f32 x)
{
    int n = (int)(x * M_1_PI_F);
    int neg = (s32)u124_f2u(x) < 0;
    f32 pi = M_PI_2_F + M_PI_2_F;
    f32 k = (f32)(n - neg);
    f32 sign = u124_u2f(((u32)(neg ^ (~n & 1)) << 31) | 0x3F800000);
    f32 t = (x - M_PI_2_F) - k * pi;            /* suba.s; msub.s */

    return u124_sin_poly(t, sign);
}
#endif

// FUNC 0x0026A270 expf
/* exp(|x|) = 2^n * e^r, n = trunc(|x| log2 e), r = |x| - n ln2, with the
 * Pade form e^r ~= (Q + P) / (Q - P), P odd (r/2 + ... r^7), Q even
 * (1 + ... r^6).  The ACC holding P is reused for Q (madd.s does not write
 * ACC): S = P + Q, D = S - 2P.  2^n is built as the bits (n + 127) << 23
 * (no overflow check: |x| > ~88 gives garbage).  x < 0 returns D / (S 2^n),
 * x >= 0 returns S 2^n / D - the binary does that last division by falling
 * through into 0x0026A37C (expf_div_tail).  Callers RaceCarModel::drawTires,
 * ReplayBlur::drawZ, ReplayBlur::draw, powf. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(expf)
"\tlui\t$3, 0x3FB8\n"
"\tmfc1\t$6, $f12\n"
"\tabs.s\t$f12, $f12\n"
"\tori\t$3, $3, 0xAA3B\n"
"\tlui\t$2, 0x3F31\n"
"\tmtc1\t$3, $f1\n"
"\tori\t$2, $2, 0x7218\n"
"\tlui\t$3, 0x3F80\n"
"\tmtc1\t$2, $f0\n"
"\tmul.s\t$f1, $f1, $f12\n"
"\tmtc1\t$3, $f13\n"
"\tcvt.w.s\t$f2, $f1\n"
"\tcvt.s.w\t$f1, $f2\n"
"\tmfc1\t$5, $f2\n"
"\tmul.s\t$f1, $f1, $f0\n"
"\tsub.s\t$f12, $f12, $f1\n"
"\tmul.s\t$f2, $f12, $f12\n"
"\tmtc1\t$0, $f1\n"
"\tlui\t$2, 0x3C83\n"
"\tlui\t$4, 0x3F00\n"
"\tori\t$2, $2, 0x4835\n"
"\tmtc1\t$4, $f3\n"
"\tlui\t$3, 0x38B7\n"
"\tmtc1\t$2, $f11\n"
"\tmul.s\t$f0, $f2, $f12\n"
"\tori\t$3, $3, 0x5147\n"
"\tmul.s\t$f9, $f2, $f2\n"
"\tmtc1\t$3, $f8\n"
"\tlui\t$2, 0x3378\n"
"\tlui\t$3, 0x3DEC\n"
"\tori\t$2, $2, 0x4D94\n"
"\tori\t$3, $3, 0x4EC5\n"
"\tmul.s\t$f4, $f0, $f2\n"
"\tmtc1\t$2, $f6\n"
"\tmul.s\t$f5, $f9, $f0\n"
"\tmtc1\t$3, $f7\n"
"\tmul.s\t$f10, $f0, $f0\n"
"\tlui\t$2, 0x3ABE\n"
"\tmula.s\t$f3, $f12\n"
"\tori\t$2, $2, 0xF4A9\n"
"\tmadda.s\t$f11, $f0\n"
"\tmtc1\t$2, $f3\n"
"\tmadda.s\t$f8, $f4\n"
"\tlui\t$2, 0x3659\n"
"\tmadda.s\t$f6, $f5\n"
"\tori\t$2, $2, 0x43E2\n"
"\tmadd.s\t$f1, $f1, $f1\n"
"\tmtc1\t$2, $f0\n"
"\tmadda.s\t$f7, $f2\n"
"\taddiu\t$5, $5, 0x7F\n"
"\tmadda.s\t$f3, $f9\n"
"\tsll\t$5, $5, 23\n"
"\tmadda.s\t$f0, $f10\n"
"\tmtc1\t$5, $f2\n"
"\tadd.s\t$f1, $f1, $f1\n"
"\tmadd.s\t$f0, $f13, $f13\n"
"\tsub.s\t$f1, $f0, $f1\n"
"\tmul.s\t$f0, $f0, $f2\n"
"\tbgez\t$6, $Lexpf_374\n"
"\tnop\n"
"\tnop\n"
"\tnop\n"
"\tdiv.s\t$f0, $f1, $f0\n"
"\tjr\t$31\n"
"\tnop\n"
"$Lexpf_374:\n"
"\tnop\n"
"\tnop\n"
FM_ASM_SIZE(expf)
FM_ASM_ENTRY(expf_div_tail)
"\tdiv.s\t$f0, $f0, $f1\n"
"\tjr\t$31\n"
"\tnop\n"
FM_ASM_SIZE(expf_div_tail)
FM_ASM_END);
#else
f32 expf(f32 x)
{
    u32 bits = u124_f2u(x);
    f32 a = __builtin_fabsf(x);
    int n = (int)(M_LOG2E_F * a);
    f32 r = a - (f32)n * M_LN2_F;
    f32 r2 = r * r;
    f32 r3 = r2 * r;
    f32 r4 = r2 * r2;
    f32 r5 = r3 * r2;
    f32 r7 = r4 * r3;
    f32 r6 = r3 * r3;
    f32 acc, p, s, d;

    acc  = 0.5f * r;                            /* mula.s  */
    acc += 0.016025642f * r3;                   /* 0x3C834835 0x1.06906ap-6f */
    acc += 8.74125909e-05f * r5;                /* 0x38B75147 0x1.6ea28ep-14f */
    acc += 5.78125566e-08f * r7;                /* 0x33784D94 0x1.f09b28p-25f */
    p = acc;                                    /* madd.s ACC + 0*0; ACC kept */
    acc += 0.115384616f * r2;                   /* 0x3DEC4EC5 0x1.d89d8ap-4f */
    acc += 0.00145687641f * r4;                 /* 0x3ABEF4A9 0x1.7de952p-10f */
    acc += 3.23750328e-06f * r6;                /* 0x365943E2 0x1.b287c4p-19f */
    p = p + p;
    s = acc + 1.0f;                             /* madd.s ACC + 1*1: Q + P */
    d = s - p;                                  /* Q - P */
    s = s * u124_u2f((u32)(n + 0x7F) << 23);
    if ((s32)bits < 0)
        return d / s;
    return s / d;                               /* via 0x0026A37C */
}
#endif

// FUNC 0x0026A37C expf_div_tail
/* Not a function of its own: the x >= 0 exit of expf (div.s $f0, $f0, $f1;
 * jr $ra), reached by falling through from 0x0026A374.  It has no callers
 * and uses $f0/$f1 as left by expf.  The matching build has these
 * instructions at the end of expf's asm block (one block: the tail is not
 * 8-byte aligned); the C build keeps them as this separate asm block (the
 * C expf divides itself). */
#ifndef FASTMATH_RETAIL_ASM
__asm__("\t.text\n"
        "\t.p2align 2\n"
        "\t.globl\texpf_div_tail\n"
        "\t.type\texpf_div_tail, @function\n"
        "expf_div_tail:\n"
        "\t.set\tpush\n"
        "\t.set\tnoreorder\n"
        "\tdiv.s\t$f0, $f0, $f1\n"
        "\tjr\t$31\n"
        "\tnop\n"
        "\t.set\tpop\n"
        "\t.size\texpf_div_tail, . - expf_div_tail\n"
        "\t.previous\n");
#endif

// FUNC 0x0026A388 logf
/* log(x) = e ln2 + log(m), m = the mantissa forced into [1, 2) with
 * (bits | 0xFF800000) - 0xC0000000, t = m - 1, and the rational
 * approximation (t + 2t^2 + 1.3056t^3 + 0.30556t^4 + 0.018122t^5) /
 * (1 + 2.5t + 2.2222t^2 + 0.83333t^3 + 0.11905t^4 + 0.0039683t^5)
 * (separate mul/add here, no accumulator).  e = (bits >> 23) - 127 with a
 * logical shift: no check of x <= 0 (a negative x adds 256 ln2).  Callers
 * LoggerGraphRoot::virtual_16, ReplayBlur::draw, VariableCameraBase::setLodBias,
 * powf. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(logf)
"\tlui\t$4, 0x3F80\n"
"\tmfc1\t$5, $f12\n"
"\tlui\t$2, 0xFF80\n"
"\tmtc1\t$4, $f0\n"
"\tlui\t$3, 0xC000\n"
"\tor\t$2, $2, $5\n"
"\tsubu\t$2, $2, $3\n"
"\tmtc1\t$4, $f2\n"
"\tlui\t$3, 0x4020\n"
"\tmtc1\t$2, $f12\n"
"\tlui\t$6, 0x4000\n"
"\tmtc1\t$3, $f1\n"
"\tsub.s\t$f12, $f12, $f0\n"
"\tmtc1\t$6, $f6\n"
"\tlui\t$4, 0x400E\n"
"\tsrl\t$5, $5, 23\n"
"\tori\t$4, $4, 0x38E4\n"
"\taddiu\t$2, $0, 0x7F\n"
"\tsubu\t$5, $5, $2\n"
"\tlui\t$3, 0x3FA7\n"
"\tmtc1\t$4, $f4\n"
"\tori\t$3, $3, 0x1C72\n"
"\tmtc1\t$5, $f3\n"
"\tmul.s\t$f11, $f1, $f12\n"
"\tmtc1\t$3, $f8\n"
"\tmul.s\t$f1, $f12, $f12\n"
"\tlui\t$2, 0x3F55\n"
"\tmov.s\t$f0, $f12\n"
"\tori\t$2, $2, 0x5555\n"
"\tcvt.s.w\t$f3, $f3\n"
"\tmtc1\t$2, $f5\n"
"\tadd.s\t$f2, $f2, $f11\n"
"\tlui\t$2, 0x3F31\n"
"\tmul.s\t$f10, $f6, $f1\n"
"\tori\t$2, $2, 0x7218\n"
"\tmul.s\t$f11, $f4, $f1\n"
"\tmtc1\t$2, $f9\n"
"\tmul.s\t$f1, $f1, $f12\n"
"\tlui\t$2, 0x3E9C\n"
"\tlui\t$3, 0x3DF3\n"
"\tori\t$2, $2, 0x71C7\n"
"\tadd.s\t$f0, $f0, $f10\n"
"\tori\t$3, $3, 0xCF3D\n"
"\tadd.s\t$f2, $f2, $f11\n"
"\tmtc1\t$2, $f7\n"
"\tmul.s\t$f10, $f8, $f1\n"
"\tmtc1\t$3, $f6\n"
"\tmul.s\t$f11, $f5, $f1\n"
"\tlui\t$2, 0x3C94\n"
"\tmul.s\t$f1, $f1, $f12\n"
"\tori\t$2, $2, 0x73F2\n"
"\tlui\t$3, 0x3B82\n"
"\tmtc1\t$2, $f5\n"
"\tadd.s\t$f0, $f0, $f10\n"
"\tori\t$3, $3, 0x821\n"
"\tadd.s\t$f2, $f2, $f11\n"
"\tmtc1\t$3, $f4\n"
"\tmul.s\t$f10, $f7, $f1\n"
"\tmul.s\t$f11, $f6, $f1\n"
"\tmul.s\t$f1, $f1, $f12\n"
"\tmul.s\t$f3, $f3, $f9\n"
"\tadd.s\t$f0, $f0, $f10\n"
"\tadd.s\t$f2, $f2, $f11\n"
"\tmul.s\t$f10, $f5, $f1\n"
"\tmul.s\t$f11, $f4, $f1\n"
"\tadd.s\t$f0, $f0, $f10\n"
"\tadd.s\t$f2, $f2, $f11\n"
"\tdiv.s\t$f0, $f0, $f2\n"
"\tjr\t$31\n"
"\tadd.s\t$f0, $f3, $f0\n"
FM_ASM_SIZE(logf)
FM_ASM_END);
#else
f32 logf(f32 x)
{
    u32 bits = u124_f2u(x);
    f32 t = u124_u2f((bits | 0xFF800000) - 0xC0000000) - 1.0f;
    f32 e = (f32)(int)((bits >> 23) - 0x7F);
    f32 t2, t3, t4, t5, num, den;

    den = 1.0f + 2.5f * t;
    t2 = t * t;
    num = t + 2.0f * t2;
    den = den + 2.22222233f * t2;               /* 0x400E38E4 0x1.1c71c8p+1f */
    t3 = t2 * t;
    num = num + 1.30555559f * t3;               /* 0x3FA71C72 0x1.4e38e4p+0f */
    den = den + 0.833333314f * t3;              /* 0x3F555555 0x1.aaaaaap-1f */
    t4 = t3 * t;
    num = num + 0.305555553f * t4;              /* 0x3E9C71C7 0x1.38e38ep-2f */
    den = den + 0.11904762f * t4;               /* 0x3DF3CF3D 0x1.e79e7ap-4f */
    t5 = t4 * t;
    e = e * M_LN2_F;
    num = num + 0.0181216933f * t5;             /* 0x3C9473F2 0x1.28e7e4p-6f */
    den = den + 0.0039682542f * t5;             /* 0x3B820821 0x1.041042p-8f */
    return e + num / den;
}
#endif

// FUNC 0x0026A4A0 powf
/* x^y = exp(y log x) (no special cases).  Caller CarRenderCtx::setup. */
f32 powf(f32 x, f32 y)
{
    return expf(y * logf(x));
}

// FUNC 0x0026A4D0 sincosf
/* *s = sin(x), *c = cos(x) from one reduction: n = trunc(x / pi),
 * xa = x + (x < 0 ? pi : 0) (pi masked by the sign), u = xa - n pi
 * (= x - k pi, k = n - (x < 0)), sign = (-1)^k; sin from min(u, pi - u),
 * cos from u - pi/2 with -sign.  The cos result is stored first (*c, $a1),
 * then *s ($a0).  The function leaves garbage in $f0 (no return value).
 * 10 callers. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(sincosf)
"\tlui\t$6, 0x3EA2\n"
"\tlui\t$7, 0x4049\n"
"\tori\t$6, $6, 0xF983\n"
"\tori\t$7, $7, 0xFDB\n"
"\tmtc1\t$6, $f0\n"
"\tlui\t$6, 0x3FC9\n"
"\tori\t$6, $6, 0xFDB\n"
"\tmfc1\t$2, $f12\n"
"\tmul.s\t$f0, $f12, $f0\n"
"\tsra\t$2, $2, 31\n"
"\tand\t$3, $7, $2\n"
"\tmtc1\t$6, $f4\n"
"\tmtc1\t$3, $f1\n"
"\tcvt.w.s\t$f0, $f0\n"
"\tadd.s\t$f13, $f12, $f1\n"
"\tmtc1\t$7, $f2\n"
"\tcvt.s.w\t$f1, $f0\n"
"\tmfc1\t$3, $f0\n"
"\tlui\t$7, 0x3F80\n"
"\tmtc1\t$0, $f7\n"
"\tadda.s\t$f13, $f7\n"
"\tmsub.s\t$f14, $f1, $f2\n"
"\txor\t$2, $2, $3\n"
"\tsuba.s\t$f2, $f13\n"
"\tsll\t$2, $2, 31\n"
"\tmadd.s\t$f5, $f1, $f2\n"
"\tor\t$2, $2, $7\n"
"\tsuba.s\t$f13, $f4\n"
"\tmtc1\t$2, $f3\n"
"\tmsub.s\t$f13, $f1, $f2\n"
"\tneg.s\t$f0, $f3\n"
"\tmin.s\t$f14, $f14, $f5\n"
"\tmul.s\t$f2, $f13, $f13\n"
"\tlui\t$7, 0xBE2A\n"
"\tmul.s\t$f13, $f13, $f0\n"
"\tlui\t$6, 0x3C08\n"
"\tmul.s\t$f5, $f14, $f14\n"
"\tori\t$7, $7, 0xAAA4\n"
"\tmul.s\t$f14, $f14, $f3\n"
"\tori\t$6, $6, 0x873E\n"
"\tmul.s\t$f0, $f2, $f2\n"
"\tmtc1\t$7, $f12\n"
"\tmul.s\t$f2, $f2, $f13\n"
"\tmtc1\t$6, $f11\n"
"\tmul.s\t$f1, $f5, $f5\n"
"\tlui\t$7, 0xB94F\n"
"\tmul.s\t$f9, $f5, $f14\n"
"\tlui\t$6, 0x362E\n"
"\tmul.s\t$f3, $f0, $f13\n"
"\tori\t$7, $7, 0xB21F\n"
"\tmul.s\t$f8, $f0, $f2\n"
"\tori\t$6, $6, 0x9C14\n"
"\tmul.s\t$f5, $f1, $f14\n"
"\tmtc1\t$7, $f6\n"
"\tmul.s\t$f10, $f1, $f9\n"
"\tmtc1\t$6, $f4\n"
"\tmul.s\t$f0, $f3, $f0\n"
"\tadda.s\t$f13, $f7\n"
"\tmul.s\t$f1, $f5, $f1\n"
"\tmadda.s\t$f12, $f2\n"
"\tmadda.s\t$f11, $f3\n"
"\tmadda.s\t$f6, $f8\n"
"\tmadd.s\t$f13, $f4, $f0\n"
"\tadda.s\t$f14, $f7\n"
"\tmadda.s\t$f12, $f9\n"
"\tmadda.s\t$f11, $f5\n"
"\tmadda.s\t$f6, $f10\n"
"\tmadd.s\t$f14, $f4, $f1\n"
"\tswc1\t$f13, 0x0($5)\n"
"\tjr\t$31\n"
"\tswc1\t$f14, 0x0($4)\n"
FM_ASM_SIZE(sincosf)
FM_ASM_END);
#else
void sincosf(f32 x, f32 *s, f32 *c)
{
    s32 mask = (s32)u124_f2u(x) >> 31;
    f32 q = x * M_1_PI_F;
    f32 xa = x + u124_u2f(0x40490FDB & (u32)mask);
    int n = (int)q;
    f32 k = (f32)n;
    f32 sign = u124_u2f(((u32)(mask ^ n) << 31) | 0x3F800000);
    f32 u = (xa + 0.0f) - k * M_PI_F;           /* adda.s; msub.s */
    f32 v = (M_PI_F - xa) + k * M_PI_F;         /* suba.s; madd.s: pi - u */
    f32 w = (xa - M_PI_2_F) - k * M_PI_F;       /* suba.s; msub.s: u - pi/2 */

    u = u124_minf(u, v);                        /* min.s */
    *c = u124_sin_poly(w, -sign);
    *s = u124_sin_poly(u, sign);
}
#endif

// FUNC 0x0026A5F0 sinf
/* sin(x) = s * sin(x - k pi), k = trunc((x + pi/2) / pi) - (x + pi/2 < 0),
 * s = (-1)^k.  38 callers. */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(sinf)
"\tlui\t$2, 0x3FC9\n"
"\tlui\t$3, 0x3EA2\n"
"\tori\t$2, $2, 0xFDB\n"
"\tori\t$3, $3, 0xF983\n"
"\tmtc1\t$2, $f1\n"
"\tadd.s\t$f4, $f12, $f1\n"
"\tmtc1\t$3, $f0\n"
"\tadd.s\t$f1, $f1, $f1\n"
"\tmtc1\t$0, $f3\n"
"\tmul.s\t$f0, $f4, $f0\n"
"\tmfc1\t$3, $f4\n"
"\tslti\t$3, $3, 0x0\n"
"\tcvt.w.s\t$f0, $f0\n"
"\tadda.s\t$f12, $f3\n"
"\tmfc1\t$2, $f0\n"
"\tandi\t$4, $2, 0x1\n"
"\tsubu\t$2, $2, $3\n"
"\txor\t$3, $3, $4\n"
"\tmtc1\t$2, $f0\n"
"\tcvt.s.w\t$f0, $f0\n"
"\tsll\t$3, $3, 31\n"
"\tlui\t$2, 0x3F80\n"
"\tor\t$3, $3, $2\n"
"\tmsub.s\t$f4, $f0, $f1\n"
"\tmtc1\t$3, $f2\n"
"\tmul.s\t$f1, $f4, $f4\n"
"\tlui\t$2, 0xBE2A\n"
"\tmul.s\t$f4, $f4, $f2\n"
"\tori\t$2, $2, 0xAAA4\n"
"\tlui\t$3, 0x3C08\n"
"\tmtc1\t$2, $f7\n"
"\tori\t$3, $3, 0x873E\n"
"\tlui\t$2, 0xB94F\n"
"\tmul.s\t$f0, $f1, $f1\n"
"\tmtc1\t$3, $f6\n"
"\tmul.s\t$f1, $f1, $f4\n"
"\tori\t$2, $2, 0xB21F\n"
"\tadda.s\t$f4, $f3\n"
"\tmtc1\t$2, $f5\n"
"\tmul.s\t$f3, $f0, $f4\n"
"\tlui\t$3, 0x362E\n"
"\tmul.s\t$f4, $f0, $f1\n"
"\tori\t$3, $3, 0x9C14\n"
"\tmadda.s\t$f7, $f1\n"
"\tmtc1\t$3, $f2\n"
"\tmul.s\t$f0, $f3, $f0\n"
"\tmadda.s\t$f6, $f3\n"
"\tmadda.s\t$f5, $f4\n"
"\tmadd.s\t$f0, $f2, $f0\n"
"\tjr\t$31\n"
"\tnop\n"
FM_ASM_SIZE(sinf)
FM_ASM_END);
#else
f32 sinf(f32 x)
{
    f32 u = x + M_PI_2_F;
    int n = (int)(u * M_1_PI_F);
    int neg = (s32)u124_f2u(u) < 0;
    f32 pi = M_PI_2_F + M_PI_2_F;
    f32 k = (f32)(n - neg);
    f32 sign = u124_u2f(((u32)(neg ^ (n & 1)) << 31) | 0x3F800000);
    f32 t = (x + 0.0f) - k * pi;                /* adda.s x, 0; msub.s */

    return u124_sin_poly(t, sign);
}
#endif

// FUNC 0x0026A6C0 tanf
/* |x| > pi/4: x -= n pi/2 with n = round(x * 2/pi) (trunc of +-0.5), and an
 * odd n returns -1/tan.  tan(x) ~= x (1 - 0.1111 x^2 + 0.001058 x^4) /
 * (1 - 0.4444 x^2 + 0.015873 x^4).  The 0.0 added to both ACC seeds is
 * `sub.s $f5, $f5, $f5` of an unset register (0 on the EE FPU, which has no
 * Inf/NaN).  The even case's division is the fall-through into 0x0026A7B8
 * (tanf_div_tail).  15 callers (cameras, ...). */
#ifdef FASTMATH_RETAIL_ASM
__asm__(FM_ASM_BEGIN(tanf)
"\tabs.s\t$f2, $f12\n"
"\tlui\t$3, 0x3F49\n"
"\tdaddu\t$5, $0, $0\n"
"\tori\t$3, $3, 0xFDB\n"
"\tlui\t$2, 0x3F22\n"
"\tmtc1\t$3, $f0\n"
"\tori\t$2, $2, 0xF983\n"
"\tmfc1\t$4, $f12\n"
"\tc.lt.s\t$f0, $f2\n"
"\tbc1f\t$Ltanf_730\n"
"\tnop\n"
"\tlui\t$3, 0x3F00\n"
"\tmtc1\t$2, $f0\n"
"\tlui\t$2, 0xBF00\n"
"\tsra\t$4, $4, 31\n"
"\tmul.s\t$f2, $f12, $f0\n"
"\tmovn\t$3, $2, $4\n"
"\tlui\t$2, 0x3FC9\n"
"\tmtc1\t$3, $f0\n"
"\tori\t$2, $2, 0xFDB\n"
"\tmtc1\t$2, $f1\n"
"\tadd.s\t$f2, $f2, $f0\n"
"\tcvt.w.s\t$f2, $f2\n"
"\tcvt.s.w\t$f0, $f2\n"
"\tmfc1\t$5, $f2\n"
"\tandi\t$5, $5, 0x1\n"
"\tmul.s\t$f2, $f0, $f1\n"
"\tsub.s\t$f12, $f12, $f2\n"
"$Ltanf_730:\n"
"\tmul.s\t$f0, $f12, $f12\n"
"\tlui\t$2, 0x3F80\n"
"\tsub.s\t$f5, $f5, $f5\n"
"\tmtc1\t$2, $f4\n"
"\tlui\t$2, 0xBEE3\n"
"\tlui\t$3, 0x3C82\n"
"\tori\t$2, $2, 0x8E39\n"
"\tori\t$3, $3, 0x821\n"
"\tmul.s\t$f2, $f0, $f0\n"
"\tmtc1\t$2, $f1\n"
"\tmul.s\t$f6, $f0, $f12\n"
"\tmtc1\t$3, $f3\n"
"\tadda.s\t$f4, $f5\n"
"\tlui\t$2, 0xBDE3\n"
"\tmadda.s\t$f1, $f0\n"
"\tori\t$2, $2, 0x8E39\n"
"\tmul.s\t$f4, $f2, $f12\n"
"\tmtc1\t$2, $f1\n"
"\tmadd.s\t$f2, $f3, $f2\n"
"\tlui\t$2, 0x3A8A\n"
"\tadda.s\t$f12, $f5\n"
"\tori\t$2, $2, 0xB356\n"
"\tmadda.s\t$f1, $f6\n"
"\tmtc1\t$2, $f0\n"
"\tmadd.s\t$f0, $f0, $f4\n"
"\tbeqz\t$5, $Ltanf_7B0\n"
"\tnop\n"
"\tneg.s\t$f2, $f2\n"
"\tnop\n"
"\tdiv.s\t$f0, $f2, $f0\n"
"\tjr\t$31\n"
"\tnop\n"
"$Ltanf_7B0:\n"
"\tnop\n"
"\tnop\n"
FM_ASM_SIZE(tanf)
FM_ASM_ENTRY(tanf_div_tail)
"\tdiv.s\t$f0, $f0, $f2\n"
"\tjr\t$31\n"
"\tnop\n"
FM_ASM_SIZE(tanf_div_tail)
FM_ASM_END);
#else
f32 tanf(f32 x)
{
    const f32 zero = 0.0f;                      /* $f5 - $f5 */
    int odd = 0;
    f32 z, z2, x3, x5, acc, den, num;

    if (M_PI_4_F < __builtin_fabsf(x)) {
        f32 half = ((s32)u124_f2u(x) >> 31) ? -0.5f : 0.5f;
        int n = (int)(x * M_2_PI_F + half);

        odd = n & 1;
        x = x - (f32)n * M_PI_2_F;
    }

    z  = x * x;
    z2 = z * z;
    x3 = z * x;
    acc = 1.0f + zero;
    acc += -0.444444448f * z;                   /* 0xBEE38E39 -0x1.c71c72p-2f */
    x5 = z2 * x;
    den = acc + 0.0158730168f * z2;             /* 0x3C820821 0x1.041042p-6f */
    acc = x + zero;
    acc += -0.111111112f * x3;                  /* 0xBDE38E39 -0x1.c71c72p-4f */
    num = acc + 0.00105820108f * x5;            /* 0x3A8AB356 0x1.1566acp-10f */
    if (odd)
        return -den / num;
    return num / den;                           /* via 0x0026A7B8 */
}
#endif

// FUNC 0x0026A7B8 tanf_div_tail
/* Not a function of its own: the even-quadrant exit of tanf (div.s $f0,
 * $f0, $f2; jr $ra), reached by falling through from 0x0026A7B0.  As
 * expf_div_tail: the end of tanf's asm block in the matching build, a
 * separate asm block in the C build. */
#ifndef FASTMATH_RETAIL_ASM
__asm__("\t.text\n"
        "\t.p2align 2\n"
        "\t.globl\ttanf_div_tail\n"
        "\t.type\ttanf_div_tail, @function\n"
        "tanf_div_tail:\n"
        "\t.set\tpush\n"
        "\t.set\tnoreorder\n"
        "\tdiv.s\t$f0, $f0, $f2\n"
        "\tjr\t$31\n"
        "\tnop\n"
        "\t.set\tpop\n"
        "\t.size\ttanf_div_tail, . - tanf_div_tail\n"
        "\t.previous\n");
#endif
