/* libm.c - newlib libm (fdlibm 5.x, double precision), 0x00274F70..0x00276EA8.
 *
 * Archive order: s_cos.o, s_sin.o, w_sqrt.o, e_rem_pio2.o, e_sqrt.o,
 * k_cos.o, k_rem_pio2.o, k_sin.o, s_fabs.o, s_floor.o, s_isnan.o,
 * s_lib_ver.o (data only: _LIB_VERSION), s_matherr.o, s_scalbn.o,
 * s_copysign.o.  Callers in the game: cos / sin
 * (flare_build_disc) and sqrt (color_similarity, color_candidate_set,
 * matrix_slerp_rotation); the rest is pulled in by them.
 *
 * Every double operation is a dp-bit call in the binary (see libgcc2_part1_internal.h); the
 * source below is plain fdlibm.  Its `static const double' constants are
 * defined here in fdlibm's order, one block per original object (prefixed:
 * the 15 objects share this file), because the compiler folds every read of
 * them into a literal-pool copy (decl_constant_value) and the variables
 * themselves still precede those literals in .rodata, as at 0x00347C98..:
 * that order keeps every literal at its retail offset.  Constants whose low
 * word is 0 are built in registers (lui / dsll32), the rest are pool loads.
 * Matching flags (ref/compile_flags.json): -O2 -G0 -fno-builtin, newlib's own
 * (no $gp access; fabs is called, not expanded inline).
 */
#include "runtime/libgcc/libgcc2_part1_internal.h"

// FUNC 0x00274F70 cos
double cos(double x)
{
    /* implicit: dpsub (the double arithmetic: soft-float calls) */
    double y[2], z = 0.0;
    int n, ix;

    GET_HIGH_WORD(ix, x);
    ix &= 0x7fffffff;
    if (ix <= 0x3fe921fb)               /* |x| ~< pi/4 */
        return __kernel_cos(x, z);
    else if (ix >= 0x7ff00000)          /* cos(Inf or NaN) is NaN */
        return x - x;
    else {                              /* argument reduction needed */
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
        case 0:  return  __kernel_cos(y[0], y[1]);
        case 1:  return FD_NEG(__kernel_sin(y[0], y[1], 1));
        case 2:  return FD_NEG(__kernel_cos(y[0], y[1]));
        default: return  __kernel_sin(y[0], y[1], 1);
        }
    }
}

// FUNC 0x00275078 sin
double sin(double x)
{
    /* implicit: dpsub (the double arithmetic: soft-float calls) */
    double y[2], z = 0.0;
    int n, ix;

    GET_HIGH_WORD(ix, x);
    ix &= 0x7fffffff;
    if (ix <= 0x3fe921fb)               /* |x| ~< pi/4 */
        return __kernel_sin(x, z, 0);
    else if (ix >= 0x7ff00000)          /* sin(Inf or NaN) is NaN */
        return x - x;
    else {                              /* argument reduction needed */
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
        case 0:  return  __kernel_sin(y[0], y[1], 1);
        case 1:  return  __kernel_cos(y[0], y[1]);
        case 2:  return FD_NEG(__kernel_sin(y[0], y[1], 1));
        default: return FD_NEG(__kernel_cos(y[0], y[1]));
        }
    }
}

// FUNC 0x00275170 sqrt
/* w_sqrt.c: __ieee754_sqrt plus the SVID/XOPEN/POSIX error handling for
 * x < 0 (_LIB_VERSION is 1 = _XOPEN_ in this build, so a negative argument
 * returns NaN (0.0/0.0, folded to the literal 0x7FF8000000000000), calls
 * matherr, which returns 0, and sets errno = EDOM).  _LIB_VERSION is loaded
 * once for the _IEEE_ and _SVID_ tests (kept in $s2 across the isnan call:
 * isnan is declared const, libgcc2_part1_internal.h) and again for the _POSIX_ test. */
double sqrt(double x)
{
    /* implicit: dpcmp (the double arithmetic: soft-float calls) */
    struct exception exc;
    double z;

    z = __ieee754_sqrt(x);
    if (_LIB_VERSION == _IEEE_ || isnan(x))
        return z;
    if (x < 0.0) {
        exc.type = DOMAIN;
        exc.name = "sqrt";
        exc.err = 0;
        exc.arg1 = exc.arg2 = x;
        if (_LIB_VERSION == _SVID_)
            exc.retval = 0.0;
        else
#ifdef BOOT_OLD_GCC
            exc.retval = 0.0 / 0.0;
#else
            INSERT_WORDS(exc.retval, 0x7ff80000, 0);   /* the NaN egcs folds 0.0/0.0 to */
#endif
        if (_LIB_VERSION == _POSIX_)
            *__errno() = EDOM;
        else if (!matherr(&exc)) {
            *__errno() = EDOM;
        }
        if (exc.err != 0)
            *__errno() = exc.err;
        return exc.retval;
    } else
        return z;
}

/* ---- e_rem_pio2.c ------------------------------------------------------- */

/* Table of constants for 2/pi, 396 hex digits (476 decimal) of 2/pi. */
const int two_over_pi[66] = {          /* 0x00347CA8 */
    0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62,
    0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
    0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129,
    0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
    0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8,
    0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
    0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5,
    0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
    0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3,
    0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
    0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
};

const int npio2_hw[32] = {             /* 0x00347DB0 */
    0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C,
    0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C,
    0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB, 0x403AB41B, 0x403C463A,
    0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C,
    0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB,
    0x404858EB, 0x404921FB,
};

const double rem_pio2_zero    = 0.00000000000000000000e+00; /* 0x00347E30 */
const double rem_pio2_half    = 5.00000000000000000000e-01; /* 0x00347E38 */
const double rem_pio2_two24   = 1.67772160000000000000e+07; /* 0x00347E40 */
const double rem_pio2_invpio2 = 6.36619772367581382433e-01; /* 0x00347E48 */
const double rem_pio2_pio2_1  = 1.57079632673412561417e+00; /* 0x00347E50 */
const double rem_pio2_pio2_1t = 6.07710050650619224932e-11; /* 0x00347E58 */
const double rem_pio2_pio2_2  = 6.07710050630396597660e-11; /* 0x00347E60 */
const double rem_pio2_pio2_2t = 2.02226624879595063154e-21; /* 0x00347E68 */
const double rem_pio2_pio2_3  = 2.02226624871116645580e-21; /* 0x00347E70 */
const double rem_pio2_pio2_3t = 8.47842766036889956997e-32; /* 0x00347E78 */

#define zero    rem_pio2_zero
#define half    rem_pio2_half
#define two24   rem_pio2_two24
#define invpio2 rem_pio2_invpio2
#define pio2_1  rem_pio2_pio2_1
#define pio2_1t rem_pio2_pio2_1t
#define pio2_2  rem_pio2_pio2_2
#define pio2_2t rem_pio2_pio2_2t
#define pio2_3  rem_pio2_pio2_3
#define pio2_3t rem_pio2_pio2_3t

// FUNC 0x00275280 __ieee754_rem_pio2
/* Returns x rem pi/2 in y[0] + y[1] and the quadrant n.
 * Matching: exact under the reference compiler 2.9-ee-991111-01; under
 * 991111b the code is identical but the frame is 0x10 smaller (0xC0, retail
 * 0xD0: retail has 0x30 of locals for tx[3]), as in __ieee754_sqrt,
 * __kernel_cos and floor: a difference of the compiler build. */
int __ieee754_rem_pio2(double x, double *y)
{
    /* implicit: dpadd, dpsub, dpmul, dpcmp, litodp, dptoli (the double arithmetic: soft-float calls) */
    double z, w, t, r, fn;
    double tx[3];
    int i, j, n, ix, hx;
    int e0, nx;
    u32 low;

    GET_HIGH_WORD(hx, x);               /* high word of x */
    ix = hx & 0x7fffffff;
    if (ix <= 0x3fe921fb)               /* |x| ~<= pi/4, no need for reduction */
        { y[0] = x; y[1] = 0; return 0; }
    if (ix < 0x4002d97c) {              /* |x| < 3pi/4, special case with n = +-1 */
        if (hx > 0) {
            z = x - pio2_1;
            if (ix != 0x3ff921fb) {     /* 33+53 bit pi is good enough */
                y[0] = z - pio2_1t;
                y[1] = (z - y[0]) - pio2_1t;
            } else {                    /* near pi/2, use 33+33+53 bit pi */
                z -= pio2_2;
                y[0] = z - pio2_2t;
                y[1] = (z - y[0]) - pio2_2t;
            }
            return 1;
        } else {                        /* negative x */
            z = x + pio2_1;
            if (ix != 0x3ff921fb) {     /* 33+53 bit pi is good enough */
                y[0] = z + pio2_1t;
                y[1] = (z - y[0]) + pio2_1t;
            } else {                    /* near pi/2, use 33+33+53 bit pi */
                z += pio2_2;
                y[0] = z + pio2_2t;
                y[1] = (z - y[0]) + pio2_2t;
            }
            return -1;
        }
    }
    if (ix <= 0x413921fb) {             /* |x| ~<= 2^19*(pi/2), medium size */
        t  = fabs(x);
        n  = (int)(t * invpio2 + half);
        fn = (double)n;
        r  = t - fn * pio2_1;
        w  = fn * pio2_1t;              /* 1st round good to 85 bit */
        if (n < 32 && ix != npio2_hw[n - 1]) {
            y[0] = r - w;               /* quick check no cancellation */
        } else {
            u32 high;
            j  = ix >> 20;
            y[0] = r - w;
            GET_HIGH_WORD(high, y[0]);
            i = j - ((high >> 20) & 0x7ff);
            if (i > 16) {               /* 2nd iteration needed, good to 118 */
                t  = r;
                w  = fn * pio2_2;
                r  = t - w;
                w  = fn * pio2_2t - ((t - r) - w);
                y[0] = r - w;
                GET_HIGH_WORD(high, y[0]);
                i = j - ((high >> 20) & 0x7ff);
                if (i > 49) {           /* 3rd iteration need, 151 bits acc */
                    t  = r;             /* will cover all possible cases */
                    w  = fn * pio2_3;
                    r  = t - w;
                    w  = fn * pio2_3t - ((t - r) - w);
                    y[0] = r - w;
                }
            }
        }
        y[1] = (r - y[0]) - w;
        if (hx < 0) { y[0] = FD_NEG(y[0]); y[1] = FD_NEG(y[1]); return -n; }
        else         return n;
    }
    /*
     * all other (large) arguments
     */
    if (ix >= 0x7ff00000) {             /* x is inf or NaN */
        y[0] = y[1] = x - x; return 0;
    }
    /* set z = scalbn(|x|,ilogb(x)-23) */
    GET_LOW_WORD(low, x);
#ifndef BOOT_OLD_GCC
    z = 0.0;    /* fdlibm reads z uninitialised here (its high word is replaced
                   below); egcs gives retail's code only without the store */
#endif
    SET_LOW_WORD(z, low);
    e0 = (int)((ix >> 20) - 1046);      /* e0 = ilogb(z)-23; */
    SET_HIGH_WORD(z, ix - ((int)e0 << 20));
    for (i = 0; i < 2; i++) {
        tx[i] = (double)((int)(z));
        z     = (z - tx[i]) * two24;
    }
    tx[2] = z;
    nx = 3;
    while (tx[nx - 1] == zero) nx--;    /* skip zero term */
    n = __kernel_rem_pio2(tx, y, e0, nx, 2, two_over_pi);
    if (hx < 0) { y[0] = FD_NEG(y[0]); y[1] = FD_NEG(y[1]); return -n; }
    return n;
}

#undef zero
#undef half
#undef two24
#undef invpio2
#undef pio2_1
#undef pio2_1t
#undef pio2_2
#undef pio2_2t
#undef pio2_3
#undef pio2_3t

/* ---- e_sqrt.c ----------------------------------------------------------- */

const double e_sqrt_one  = 1.0;        /* 0x00347EF8 */
const double e_sqrt_tiny = 1.0e-300;   /* 0x00347F00 */

#define one  e_sqrt_one
#define tiny e_sqrt_tiny

// FUNC 0x002757A0 __ieee754_sqrt
/* Bit-by-bit square root.  The rounding test `z = one - tiny; if (z >= one)'
 * and `z = one + tiny; if (z > one)' are constant-folded to 1.0 by the
 * compiler: the binary calls dpcmp(1.0, 1.0) for both tests.
 * Matching: exact under 991111-01; under 991111b the frame is 0x10 smaller
 * (0x70, retail 0x80) and one `sra' is scheduled a slot earlier. */
double __ieee754_sqrt(double x)
{
    /* implicit: dpadd, dpsub, dpmul, dpdiv, dpcmp (the double arithmetic: soft-float calls) */
    double z;
    int sign = (int)0x80000000;
    u32 r, t1, s1, ix1, q1;
    int ix0, s0, q, m, t, i;

    EXTRACT_WORDS(ix0, ix1, x);

    /* take care of Inf and NaN */
    if ((ix0 & 0x7ff00000) == 0x7ff00000) {
        return x * x + x;               /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                                           sqrt(-inf)=sNaN */
    }
    /* take care of zero */
    if (ix0 <= 0) {
        if (((ix0 & (~sign)) | ix1) == 0) return x;   /* sqrt(+-0) = +-0 */
        else if (ix0 < 0)
            return (x - x) / (x - x);   /* sqrt(-ve) = sNaN */
    }
    /* normalize x */
    m = (ix0 >> 20);
    if (m == 0) {                       /* subnormal x */
        while (ix0 == 0) {
            m -= 21;
            ix0 |= (ix1 >> 11); ix1 <<= 21;
        }
        for (i = 0; (ix0 & 0x00100000) == 0; i++) ix0 <<= 1;
        m -= i - 1;
        ix0 |= (ix1 >> (32 - i));
        ix1 <<= i;
    }
    m -= 1023;                          /* unbias exponent */
    ix0 = (ix0 & 0x000fffff) | 0x00100000;
    if (m & 1) {                        /* odd m, double x to make it even */
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
    }
    m >>= 1;                            /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    q = q1 = s0 = s1 = 0;               /* [q,q1] = sqrt(x) */
    r = 0x00200000;                     /* r = moving bit from right to left */

    while (r != 0) {
        t = s0 + r;
        if (t <= ix0) {
            s0   = t + r;
            ix0 -= t;
            q   += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    r = sign;
    while (r != 0) {
        t1 = s1 + r;
        t  = s0;
        if ((t < ix0) || ((t == ix0) && (t1 <= ix1))) {
            s1  = t1 + r;
            if (((t1 & sign) == sign) && (s1 & sign) == 0) s0 += 1;
            ix0 -= t;
            if (ix1 < t1) ix0 -= 1;
            ix1 -= t1;
            q1  += r;
        }
        ix0 += ix0 + ((ix1 & sign) >> 31);
        ix1 += ix1;
        r >>= 1;
    }

    /* use floating add to find out rounding direction */
    if ((ix0 | ix1) != 0) {
        z = one - tiny;                 /* trigger inexact flag */
        if (z >= one) {
            z = one + tiny;
            if (q1 == (u32)0xffffffff) { q1 = 0; q += 1; }
            else if (z > one) {
                if (q1 == (u32)0xfffffffe) q += 1;
                q1 += 2;
            } else
                q1 += (q1 & 1);
        }
    }
    ix0 = (q >> 1) + 0x3fe00000;
    ix1 =  q1 >> 1;
    if ((q & 1) == 1) ix1 |= sign;
    ix0 += (m << 20);
    INSERT_WORDS(z, ix0, ix1);
    return z;
}

#undef one
#undef tiny

/* ---- k_cos.c ------------------------------------------------------------ */

const double k_cos_one = 1.00000000000000000000e+00; /* 0x00347F08 */
const double k_cos_C1  = 4.16666666666666019037e-02; /* 0x00347F10 */
const double k_cos_C2  = -1.38888888888741095749e-03; /* 0x00347F18 */
const double k_cos_C3  = 2.48015872894767294178e-05; /* 0x00347F20 */
const double k_cos_C4  = -2.75573143513906633035e-07; /* 0x00347F28 */
const double k_cos_C5  = 2.08757232129817482790e-09; /* 0x00347F30 */
const double k_cos_C6  = -1.13596475577881948265e-11; /* 0x00347F38 */

#define one k_cos_one
#define C1  k_cos_C1
#define C2  k_cos_C2
#define C3  k_cos_C3
#define C4  k_cos_C4
#define C5  k_cos_C5
#define C6  k_cos_C6

// FUNC 0x00275AA8 __kernel_cos
/* cos(x + y) on [-pi/4, pi/4], y the tail of x.
 * Matching: exact under 991111-01; under 991111b the frame is 0x10 smaller
 * (0x80, retail 0x90), the code otherwise identical. */
double __kernel_cos(double x, double y)
{
    /* implicit: dpadd, dpsub, dpmul, dptoli (the double arithmetic: soft-float calls) */
    double a, hz, z, r, qx;
    int ix;

    GET_HIGH_WORD(ix, x);
    ix &= 0x7fffffff;                   /* ix = |x|'s high word */
    if (ix < 0x3e400000) {              /* if x < 2**27 */
        if (((int)x) == 0) return one;  /* generate inexact */
    }
    z  = x * x;
    r  = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    if (ix < 0x3FD33333)                /* if |x| < 0.3 */
        return one - (0.5 * z - (z * r - x * y));
    else {
        if (ix > 0x3fe90000) {          /* x > 0.78125 */
            qx = 0.28125;
        } else {
            INSERT_WORDS(qx, ix - 0x00200000, 0);   /* x/4 */
        }
        hz = 0.5 * z - qx;
        a  = one - qx;
        return a - (hz - (z * r - x * y));
    }
}

#undef one
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6

/* ---- k_rem_pio2.c ------------------------------------------------------- */

const int init_jk[4] = { 2, 3, 4, 6 };  /* 0x00347F70 initial value for jk */

const double PIo2[8] = {                /* 0x00347F80 */
    1.57079625129699707031e+00,         /* 0x3FF921FB, 0x40000000 */
    7.54978941586159635335e-08,         /* 0x3E74442D, 0x00000000 */
    5.39030252995776476554e-15,         /* 0x3CF84698, 0x80000000 */
    3.28200341580791294123e-22,         /* 0x3B78CC51, 0x60000000 */
    1.27065575308067607349e-29,         /* 0x39F01B83, 0x80000000 */
    1.22933308981111328932e-36,         /* 0x387A2520, 0x40000000 */
    2.73370053816464559624e-44,         /* 0x36E38222, 0x80000000 */
    2.16741683877804819444e-51,         /* 0x3569F31D, 0x00000000 */
};

const double k_rem_pio2_zero   = 0.0;                        /* 0x00347FC0 */
const double k_rem_pio2_one    = 1.0;                        /* 0x00347FC8 */
const double k_rem_pio2_two24  = 1.67772160000000000000e+07; /* 0x00347FD0 */
const double k_rem_pio2_twon24 = 5.96046447753906250000e-08; /* 0x00347FD8 */

#define zero   k_rem_pio2_zero
#define one    k_rem_pio2_one
#define two24  k_rem_pio2_two24
#define twon24 k_rem_pio2_twon24

// FUNC 0x00275CF8 __kernel_rem_pio2
/* Payne-Hanek reduction for large x: x[] holds x in 24-bit chunks (nx of
 * them, x = sum x[i]*2^(e0-24i)), ipio2 = two_over_pi; result in y[0..prec
 * dependent], return value n & 7.  Called with prec = 2 (53-bit result).
 * `(ih == 0) ? fw : -fw' is expanded as store fw, then negate in place
 * when ih != 0 (expand_expr's singleton form for a NEGATE_EXPR arm). */
int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2)
{
    /* implicit: dpadd, dpsub, dpmul, dpcmp, litodp, dptoli (the double arithmetic: soft-float calls) */
    int jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
    double z, fw, f[20], fq[20], q[20];

    /* initialize jk*/
    jk = init_jk[prec];
    jp = jk;

    /* determine jx,jv,q0, note that 3>q0 */
    jx =  nx - 1;
    jv = (e0 - 3) / 24; if (jv < 0) jv = 0;
    q0 =  e0 - 24 * (jv + 1);

    /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
    j = jv - jx; m = jx + jk;
    for (i = 0; i <= m; i++, j++) f[i] = (j < 0) ? zero : (double)ipio2[j];

    /* compute q[0],q[1],...q[jk] */
    for (i = 0; i <= jk; i++) {
        for (j = 0, fw = 0.0; j <= jx; j++) fw += x[j] * f[jx + i - j];
        q[i] = fw;
    }

    jz = jk;
recompute:
    /* distill q[] into iq[] reversingly */
    for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--) {
        fw    =  (double)((int)(twon24 * z));
        iq[i] =  (int)(z - two24 * fw);
        z     =  q[j - 1] + fw;
    }

    /* compute n */
    z  = scalbn(z, (int)q0);            /* actual value of z */
    z -= 8.0 * floor(z * 0.125);        /* trim off integer >= 8 */
    n  = (int)z;
    z -= (double)n;
    ih = 0;
    if (q0 > 0) {                       /* need iq[jz-1] to determine n */
        i  = (iq[jz - 1] >> (24 - q0)); n += i;
        iq[jz - 1] -= i << (24 - q0);
        ih = iq[jz - 1] >> (23 - q0);
    }
    else if (q0 == 0) ih = iq[jz - 1] >> 23;
    else if (z >= 0.5) ih = 2;

    if (ih > 0) {                       /* q > 0.5 */
        n += 1; carry = 0;
        for (i = 0; i < jz; i++) {      /* compute 1-q */
            j = iq[i];
            if (carry == 0) {
                if (j != 0) {
                    carry = 1; iq[i] = 0x1000000 - j;
                }
            } else  iq[i] = 0xffffff - j;
        }
        if (q0 > 0) {                   /* rare case: chance is 1 in 12 */
            switch (q0) {
            case 1:
                iq[jz - 1] &= 0x7fffff; break;
            case 2:
                iq[jz - 1] &= 0x3fffff; break;
            }
        }
        if (ih == 2) {
            z = one - z;
            if (carry != 0) z -= scalbn(one, (int)q0);
        }
    }

    /* check if recomputation is needed */
    if (z == zero) {
        j = 0;
        for (i = jz - 1; i >= jk; i--) j |= iq[i];
        if (j == 0) {                   /* need recomputation */
            for (k = 1; iq[jk - k] == 0; k++);  /* k = no. of terms needed */

            for (i = jz + 1; i <= jz + k; i++) {   /* add q[jz+1] to q[jz+k] */
                f[jx + i] = (double)ipio2[jv + i];
                for (j = 0, fw = 0.0; j <= jx; j++) fw += x[j] * f[jx + i - j];
                q[i] = fw;
            }
            jz += k;
            goto recompute;
        }
    }

    /* chop off zero terms */
    if (z == 0.0) {
        jz -= 1; q0 -= 24;
        while (iq[jz] == 0) { jz--; q0 -= 24; }
    } else {                            /* break z into 24-bit if necessary */
        z = scalbn(z, -(int)q0);
        if (z >= two24) {
            fw = (double)((int)(twon24 * z));
            iq[jz] = (int)(z - two24 * fw);
            jz += 1; q0 += 24;
            iq[jz] = (int)fw;
        } else iq[jz] = (int)z;
    }

    /* convert integer "bit" chunk to floating-point value */
    fw = scalbn(one, (int)q0);
    for (i = jz; i >= 0; i--) {
        q[i] = fw * (double)iq[i]; fw *= twon24;
    }

    /* compute PIo2[0,...,jp]*q[jz,...,0] */
    for (i = jz; i >= 0; i--) {
        for (fw = 0.0, k = 0; k <= jp && k <= jz - i; k++) fw += PIo2[k] * q[i + k];
        fq[jz - i] = fw;
    }

    /* compress fq[] into y[] */
    switch (prec) {
    case 0:
        fw = 0.0;
        for (i = jz; i >= 0; i--) fw += fq[i];
        y[0] = (ih == 0) ? fw : FD_NEG(fw);
        break;
    case 1:
    case 2:
        fw = 0.0;
        for (i = jz; i >= 0; i--) fw += fq[i];
        y[0] = (ih == 0) ? fw : FD_NEG(fw);
        fw = fq[0] - fw;
        for (i = 1; i <= jz; i++) fw += fq[i];
        y[1] = (ih == 0) ? fw : FD_NEG(fw);
        break;
    case 3:                             /* painful */
        for (i = jz; i > 0; i--) {
            fw      = fq[i - 1] + fq[i];
            fq[i]  += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (i = jz; i > 1; i--) {
            fw      = fq[i - 1] + fq[i];
            fq[i]  += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (fw = 0.0, i = jz; i >= 2; i--) fw += fq[i];
        if (ih == 0) {
            y[0] =  fq[0]; y[1] =  fq[1]; y[2] =  fw;
        } else {
            y[0] = FD_NEG(fq[0]); y[1] = FD_NEG(fq[1]); y[2] = FD_NEG(fw);
        }
    }
    return n & 7;
}

#undef zero
#undef one
#undef two24
#undef twon24

/* ---- k_sin.c ------------------------------------------------------------ */

const double k_sin_half = 5.00000000000000000000e-01; /* 0x00347FE0 */
const double k_sin_S1   = -1.66666666666666324348e-01; /* 0x00347FE8 */
const double k_sin_S2   = 8.33333333332248946124e-03; /* 0x00347FF0 */
const double k_sin_S3   = -1.98412698298579493134e-04; /* 0x00347FF8 */
const double k_sin_S4   = 2.75573137070700676789e-06; /* 0x00348000 */
const double k_sin_S5   = -2.50507602534068634195e-08; /* 0x00348008 */
const double k_sin_S6   = 1.58969099521155010221e-10; /* 0x00348010 */

#define half k_sin_half
#define S1   k_sin_S1
#define S2   k_sin_S2
#define S3   k_sin_S3
#define S4   k_sin_S4
#define S5   k_sin_S5
#define S6   k_sin_S6

// FUNC 0x00276860 __kernel_sin
double __kernel_sin(double x, double y, int iy)
{
    /* implicit: dpadd, dpsub, dpmul, dptoli (the double arithmetic: soft-float calls) */
    double z, r, v;
    int ix;

    GET_HIGH_WORD(ix, x);
    ix &= 0x7fffffff;                   /* high word of x */
    if (ix < 0x3e400000)                /* |x| < 2**-27 */
        { if ((int)x == 0) return x; }  /* generate inexact */
    z = x * x;
    v = z * x;
    r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
    if (iy == 0) return x + v * (S1 + z * r);
    else         return x - ((z * (half * y - v * r) - y) - v * S1);
}

#undef half
#undef S1
#undef S2
#undef S3
#undef S4
#undef S5
#undef S6

/* ---- s_fabs.c ----------------------------------------------------------- */

// FUNC 0x00276A38 fabs
double fabs(double x)
{
    u32 high;

    GET_HIGH_WORD(high, x);
    SET_HIGH_WORD(x, high & 0x7fffffff);
    return x;
}

/* ---- s_floor.c ---------------------------------------------------------- */

const double s_floor_huge = 1.0e300;   /* 0x00348050 */

#define huge s_floor_huge

// FUNC 0x00276A70 floor
/* Bit-twiddling floor; `huge + x > 0.0' only raises inexact (always true
 * for the finite x that reach it).
 * Matching: exact under 991111-01; under 991111b the frame is 0x10 smaller
 * (0x60, retail 0x70), the code otherwise identical. */
double floor(double x)
{
    /* implicit: dpadd, dpcmp (the double arithmetic: soft-float calls) */
    int i0, i1, j0;
    u32 i, j;

    EXTRACT_WORDS(i0, i1, x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if (j0 < 20) {
        if (j0 < 0) {                   /* raise inexact if x != 0 */
            if (huge + x > 0.0) {       /* return 0*sign(x) if |x|<1 */
                if (i0 >= 0) { i0 = i1 = 0; }
                else if (((i0 & 0x7fffffff) | i1) != 0)
                    { i0 = 0xbff00000; i1 = 0; }
            }
        } else {
            i = (0x000fffff) >> j0;
            if (((i0 & i) | i1) == 0) return x;   /* x is integral */
            if (huge + x > 0.0) {       /* raise inexact flag */
                if (i0 < 0) i0 += (0x00100000) >> j0;
                i0 &= (~i); i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400) return x + x;  /* inf or NaN */
        else return x;                  /* x is integral */
    } else {
        i = ((u32)(0xffffffff)) >> (j0 - 20);
        if ((i1 & i) == 0) return x;    /* x is integral */
        if (huge + x > 0.0) {           /* raise inexact flag */
            if (i0 < 0) {
                if (j0 == 20) i0 += 1;
                else {
                    j = i1 + (1 << (52 - j0));
                    if (j < i1) i0 += 1;    /* got a carry */
                    i1 = j;
                }
            }
            i1 &= (~i);
        }
    }
    INSERT_WORDS(x, i0, i1);
    return x;
}

#undef huge

/* ---- s_isnan.c ---------------------------------------------------------- */

// FUNC 0x00276C48 isnan
int isnan(double x)
{
    int hx, lx;

    EXTRACT_WORDS(hx, lx, x);
    hx &= 0x7fffffff;
    hx |= (u32)(lx | (-lx)) >> 31;
    hx = 0x7ff00000 - hx;
    return (int)(((u32)(hx)) >> 31);
}

/* ---- s_lib_ver.c -------------------------------------------------------- */

/* The libm error-handling mode, _XOPEN_ (newlib's fdlibm default).  No code:
 * s_lib_ver.o is data only, archived between s_isnan.o and s_matherr.o, and
 * retail has it in .rodata (const), right after floor's literal and 4 bytes
 * before scalbn's 8-aligned constants.  Defined after sqrt, its only reader,
 * so sqrt still loads it (egcs folds a const whose initialiser it has seen). */
const int _LIB_VERSION = _XOPEN_;      /* 0x00348060 */

/* ---- s_matherr.c -------------------------------------------------------- */

// FUNC 0x00276C80 matherr
int matherr(struct exception *x)
{
    int n = 0;

    if (x->arg1 != x->arg1) return 0;   /* implicit: dpcmp (double compare) */
    return n;
}

/* ---- s_scalbn.c --------------------------------------------------------- */

const double s_scalbn_two54  = 1.80143985094819840000e+16; /* 0x00348068 */
const double s_scalbn_twom54 = 5.55111512312578270212e-17; /* 0x00348070 */
const double s_scalbn_huge   = 1.0e+300;                   /* 0x00348078 */
const double s_scalbn_tiny   = 1.0e-300;                   /* 0x00348080 */

#define two54  s_scalbn_two54
#define twom54 s_scalbn_twom54
#define huge   s_scalbn_huge
#define tiny   s_scalbn_tiny

// FUNC 0x00276CA8 scalbn
double scalbn(double x, int n)
{
    /* implicit: dpadd, dpmul (the double arithmetic: soft-float calls) */
    int k, hx, lx;

    EXTRACT_WORDS(hx, lx, x);
    k = (hx & 0x7ff00000) >> 20;        /* extract exponent */
    if (k == 0) {                       /* 0 or subnormal x */
        if ((lx | (hx & 0x7fffffff)) == 0) return x;  /* +-0 */
        x *= two54;
        GET_HIGH_WORD(hx, x);
        k = ((hx & 0x7ff00000) >> 20) - 54;
        if (n < -50000) return tiny * x;    /*underflow*/
    }
    if (k == 0x7ff) return x + x;       /* NaN or Inf */
    k = k + n;
    if (k > 0x7fe) return huge * copysign(huge, x);   /* overflow  */
    if (k > 0)                          /* normal result */
        { SET_HIGH_WORD(x, (hx & 0x800fffff) | (k << 20)); return x; }
    if (k <= -54) {
        if (n > 50000)                  /* in case integer overflow in n+k */
            return huge * copysign(huge, x);    /*overflow*/
        else return tiny * copysign(tiny, x);   /*underflow*/
    }
    k += 54;                            /* subnormal result */
    SET_HIGH_WORD(x, (hx & 0x800fffff) | (k << 20));
    return x * twom54;
}

#undef two54
#undef twom54
#undef huge
#undef tiny

/* ---- s_copysign.c ------------------------------------------------------- */

// FUNC 0x00276E60 copysign
double copysign(double x, double y)
{
    u32 hx, hy;

    GET_HIGH_WORD(hx, x);
    GET_HIGH_WORD(hy, y);
    SET_HIGH_WORD(x, (hx & 0x7fffffff) | (hy & 0x80000000));
    return x;
}
