/* libgcc_math.c - GCC 2.9x-ee libgcc arithmetic, 0x01023678 .. 0x01026940
 *
 * The objects below were pulled out of libgcc.a in archive (alphabetical)
 * member order by the two libkernl printf formatters (_kernel_vprintf
 * 0x0101CCA8 and _print_double 0x0101CB40); everything here is stock
 * compiler runtime - no game logic, no crypto:
 *
 *   _divdi3.o     0x01023678  __divdi3        libgcc2.c L_divdi3
 *   _fixunsdfdi.o 0x01023D68  __fixunsdfdi    libgcc2.c L_fixunsdfdi
 *   _floatdidf.o  0x01023E58  __floatdidf     libgcc2.c L_floatdidf
 *   _moddi3.o     0x01023EF0  __moddi3        libgcc2.c L_moddi3
 *   _udivdi3.o    0x01024558  __udivdi3       libgcc2.c L_udivdi3
 *   _umoddi3.o    0x01024B28  __umoddi3       libgcc2.c L_umoddi3
 *   dp-bit.o      0x01025068  config/fp-bit.c compiled for double
 *   fp-bit.o      0x01025D60  config/fp-bit.c compiled for float (FLOAT)
 *   (__muldi3, used by dp-bit's multiply, lives at 0x01018070 - it was
 *    already pulled in earlier by the bignum code; see cxxrt.h.)
 *
 * 64-bit division: W_TYPE_SIZE is 32 (Wtype = SI, DWtype = DI) even though
 * the R5900 has 64-bit GPRs, so each 64-bit operand is split into 32-bit
 * halves.  Each of the four division objects inlines its own copy of the
 * static __udivmoddi4 and carries its own static __clz_tab (four identical
 * 256-byte tables in .rodata, 0x010407C0/0x010408C0/0x010409C0/0x01040AC0).
 * udiv_qrnnd is longlong.h's generic __udiv_qrnnd_c (two 16-bit digit steps
 * with divu, so UDIV_NEEDS_NORMALIZATION = 1); umul_ppmm is the MIPS `multu`.
 * Every divu is paired with gcc's -mcheck-zero-division guard
 * (`beql divisor,$0 -> break 7`, scheduled just before or just after the
 * divu), not written out below.
 *
 * Soft float: the EE FPU is single precision only, so every double operation
 * is a call into dp-bit.  This toolchain's libgcc was configured with the
 * US Software GOFAST libcall names (gcc config/gofast.h, INIT_GOFAST_OPTABS):
 * the evidence in the binary is
 *   - neither object contains _eq_f2/_ne_f2/_gt_f2/.../_le_f2 (fp-bit.c only
 *     builds those `#ifndef US_SOFTWARE_GOFAST`); the compiler uses the
 *     `compare` routine for every relational test (__fixunsdfdi's `a < 0`
 *     calls it),
 *   - `-a` in __fixunsdfdi is compiled as `0.0 - a` via the subtract routine:
 *     gcc 2.9x's expand_unop only falls back to subtract-from-zero when the
 *     neg optab has no libfunc ("The US Software GOFAST library needs this").
 * So the double routines are dpadd/dpsub/dpmul/dpdiv/dpcmp/litodp/dptoli/
 * dptoul/dptofp and the float ones fpadd/fpsub/fpmul/fpdiv/fpcmp/sitofp/
 * fptosi/fptoui/fptodp; negation keeps __negdf2/__negsf2 and the helpers
 * __make_dp/__make_fp.  The equivalent standard libgcc name is given next to
 * each one.  Argument passing is the EE ABI: doubles travel as 64-bit values
 * in GPRs ($a0/$a1, result in $v0), floats in $f12/$f13 (result in $f0).
 *
 * fp-bit behaviour reproduced from the binary (the first item is a build
 * option - fp-bit.c compiled with NO_DENORMALS; the others are stock gcc
 * 2.95 fp-bit.c but easy to miss):
 *   - unpack: exponent 0 is always CLASS_ZERO - denormal inputs are flushed
 *     to zero (no normalising loop), fraction/normal_exp left unwritten;
 *   - pack: a NaN always gets the "quiet" bit ORed in (`|| 1`); for float
 *     that bit is fp-bit's QUIET_NAN 0x100000 (not the IEEE 0x400000);
 *   - multiply's right-normalise loop only shifts `low` when `high` is odd.
 */
#include "types.h"
#include "cxxrt.h"

/* ======================================================================= */
/*  libgcc2.c types (32-bit word)                                          */
/* ======================================================================= */

typedef unsigned char       UQItype;
typedef unsigned short      UHItype;
typedef int                 SItype;
typedef unsigned int        USItype;
typedef long long           DItype;
typedef unsigned long long  UDItype;
typedef double              DFtype;
typedef float               SFtype;

typedef SItype  Wtype;
typedef USItype UWtype;
typedef DItype  DWtype;
typedef UDItype UDWtype;
typedef int     word_type;

#define W_TYPE_SIZE 32
#define WORD_SIZE   32                              /* sizeof(Wtype) * BITS_PER_UNIT */
#define HIGH_WORD_COEFF      (((UDWtype)1) << WORD_SIZE)

/* Little-endian DWunion: low word first. */
struct DWstruct { Wtype low, high; };
typedef union {
    struct DWstruct s;
    DWtype ll;
} DWunion;

/* ---- longlong.h generic C helpers ----------------------------------------- */

#define __BITS4           (W_TYPE_SIZE / 4)
#define __ll_B            ((UWtype)1 << (W_TYPE_SIZE / 2))
#define __ll_lowpart(t)   ((UWtype)(t) & (__ll_B - 1))
#define __ll_highpart(t)  ((UWtype)(t) >> (W_TYPE_SIZE / 2))

#define sub_ddmmss(sh, sl, ah, al, bh, bl)                                  \
    do {                                                                    \
        UWtype __x;                                                         \
        __x = (al) - (bl);                                                  \
        (sh) = (ah) - (bh) - (__x > (al));                                  \
        (sl) = __x;                                                         \
    } while (0)

/* MIPS longlong.h: `multu u,v` -> HI:LO. */
#define umul_ppmm(w1, w0, u, v)                                             \
    do {                                                                    \
        UDItype __p = (UDItype)(UWtype)(u) * (UDItype)(UWtype)(v);          \
        (w1) = (UWtype)(__p >> 32);                                         \
        (w0) = (UWtype)__p;                                                 \
    } while (0)

/* __udiv_qrnnd_c: (n1:n0) / d with d normalised (msb set), two 16-bit
 * quotient digits, each corrected at most twice. */
#define udiv_qrnnd(q, r, n1, n0, d)                                         \
    do {                                                                    \
        UWtype __d1, __d0, __q1, __q0;                                      \
        UWtype __r1, __r0, __m;                                             \
        __d1 = __ll_highpart(d);                                            \
        __d0 = __ll_lowpart(d);                                             \
                                                                            \
        __r1 = (n1) % __d1;                                                 \
        __q1 = (n1) / __d1;                                                 \
        __m = (UWtype)__q1 * __d0;                                          \
        __r1 = __r1 * __ll_B | __ll_highpart(n0);                           \
        if (__r1 < __m) {                                                   \
            __q1--, __r1 += (d);                                            \
            if (__r1 >= (d)) /* i.e. we didn't get carry when adding */     \
                if (__r1 < __m)                                             \
                    __q1--, __r1 += (d);                                    \
        }                                                                   \
        __r1 -= __m;                                                        \
                                                                            \
        __r0 = __r1 % __d1;                                                 \
        __q0 = __r1 / __d1;                                                 \
        __m = (UWtype)__q0 * __d0;                                          \
        __r0 = __r0 * __ll_B | __ll_lowpart(n0);                            \
        if (__r0 < __m) {                                                   \
            __q0--, __r0 += (d);                                            \
            if (__r0 >= (d))                                                \
                if (__r0 < __m)                                             \
                    __q0--, __r0 += (d);                                    \
        }                                                                   \
        __r0 -= __m;                                                        \
                                                                            \
        (q) = (UWtype)__q1 * __ll_B | __q0;                                 \
        (r) = __r0;                                                         \
    } while (0)

/* Generic count_leading_zeros over a 256-entry bit-length table. */
#define count_leading_zeros(count, x, clz_tab)                              \
    do {                                                                    \
        UWtype __xr = (x);                                                  \
        UWtype __a;                                                         \
        __a = __xr < ((UWtype)1 << 2 * __BITS4)                             \
            ? (__xr < ((UWtype)1 << __BITS4) ? 0 : __BITS4)                 \
            : (__xr < ((UWtype)1 << 3 * __BITS4) ? 2 * __BITS4 : 3 * __BITS4); \
        (count) = W_TYPE_SIZE - ((clz_tab)[__xr >> __a] + __a);             \
    } while (0)

/* ---- __clz_tab: bit length of 0..255 (static per division object) -------- */

#define CLZ_TAB_INIT {                                                      \
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,        \
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,        \
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,        \
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,        \
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,        \
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,        \
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,        \
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8         \
}

/* Verified byte-identical to each other and to the table above. */
static const UQItype __clz_tab_divdi3[256]  = CLZ_TAB_INIT;   /* @0x010407C0 (.rodata) */
static const UQItype __clz_tab_moddi3[256]  = CLZ_TAB_INIT;   /* @0x010408C0 (.rodata) */
static const UQItype __clz_tab_udivdi3[256] = CLZ_TAB_INIT;   /* @0x010409C0 (.rodata) */
static const UQItype __clz_tab_umoddi3[256] = CLZ_TAB_INIT;   /* @0x01040AC0 (.rodata) */

/* libgcc2.c: `#if defined (L_divdi3) || defined (L_moddi3) static inline`,
 * i.e. inlined into __divdi3 / __moddi3 as the 32-bit-half negate
 * low = -low; high = -high - (low != 0). */
static inline DWtype
__negdi2(DWtype u)
{
    DWunion w;
    DWunion uu;

    uu.ll = u;

    w.s.low = -uu.s.low;
    w.s.high = -uu.s.high - ((UWtype)w.s.low > 0);

    return w.ll;
}

/* libgcc2.c __udivmoddi4 (UDIV_NEEDS_NORMALIZATION variant).  Not a function
 * of its own in the binary: every one of __divdi3/__moddi3/__udivdi3/
 * __umoddi3 contains an inlined copy using its private __clz_tab (passed
 * here as `clz_tab`).  In the two quotient-only users the compiler dropped
 * the dead remainder arithmetic; in the two remainder-only users it dropped
 * the dead quotient corrections.  __moddi3/__umoddi3 pass rp = &w (their
 * stack slot) and still test `rp != 0` at run time, except on the
 * "d1 > n1" path where the store to the slot is unconditional. */
static inline __attribute__((always_inline)) UDWtype
__udivmoddi4(UDWtype n, UDWtype d, UDWtype *rp, const UQItype *clz_tab)
{
    DWunion ww;
    DWunion nn, dd;
    DWunion rr;
    UWtype d0, d1, n0, n1, n2;
    UWtype q0, q1;
    UWtype b, bm;

    nn.ll = n;
    dd.ll = d;

    d0 = dd.s.low;
    d1 = dd.s.high;
    n0 = nn.s.low;
    n1 = nn.s.high;

    if (d1 == 0) {
        if (d0 > n1) {
            /* 0q = nn / 0D */

            count_leading_zeros(bm, d0, clz_tab);

            if (bm != 0) {
                /* Normalize, i.e. make the most significant bit of the
                   denominator set. */
                d0 = d0 << bm;
                n1 = (n1 << bm) | (n0 >> (W_TYPE_SIZE - bm));
                n0 = n0 << bm;
            }

            udiv_qrnnd(q0, n0, n1, n0, d0);
            q1 = 0;

            /* Remainder in n0 >> bm. */
        } else {
            /* qq = NN / 0d */

            if (d0 == 0) {
                /* Divide intentionally by zero: on the EE this is the
                   `divu 1,$0` whose zero-check executes `break 7`. */
                volatile UWtype zero = d0;
                d0 = 1 / zero;
            }

            count_leading_zeros(bm, d0, clz_tab);

            if (bm == 0) {
                /* From (n1 >= d0) /\ (the most significant bit of d0 is
                   set), conclude (the most significant bit of n1 is set)
                   /\ (the leading quotient digit q1 = 1).  This special
                   case is necessary, not an optimization.  (Shift counts
                   of W_TYPE_SIZE are undefined.) */
                n1 -= d0;
                q1 = 1;
            } else {
                /* Normalize. */
                b = W_TYPE_SIZE - bm;

                d0 = d0 << bm;
                n2 = n1 >> b;
                n1 = (n1 << bm) | (n0 >> b);
                n0 = n0 << bm;

                udiv_qrnnd(q1, n1, n2, n1, d0);
            }

            /* n1 != d0... */

            udiv_qrnnd(q0, n0, n1, n0, d0);

            /* Remainder in n0 >> bm. */
        }

        if (rp != 0) {
            rr.s.low = n0 >> bm;
            rr.s.high = 0;
            *rp = rr.ll;
        }
    } else {
        if (d1 > n1) {
            /* 00 = nn / DD */

            q0 = 0;
            q1 = 0;

            /* Remainder in n1n0. */
            if (rp != 0) {
                rr.s.low = n0;
                rr.s.high = n1;
                *rp = rr.ll;
            }
        } else {
            /* 0q = NN / dd */

            count_leading_zeros(bm, d1, clz_tab);
            if (bm == 0) {
                /* From (n1 >= d1) /\ (the most significant bit of d1 is
                   set), conclude (the most significant bit of n1 is set)
                   /\ (the quotient digit q0 = 0 or 1).  The condition on
                   the next line takes advantage of that n1 >= d1 (true
                   due to program flow). */
                if (n1 > d1 || n0 >= d0) {
                    q0 = 1;
                    sub_ddmmss(n1, n0, n1, n0, d1, d0);
                } else
                    q0 = 0;

                q1 = 0;

                if (rp != 0) {
                    rr.s.low = n0;
                    rr.s.high = n1;
                    *rp = rr.ll;
                }
            } else {
                UWtype m1, m0;
                /* Normalize. */

                b = W_TYPE_SIZE - bm;

                d1 = (d1 << bm) | (d0 >> b);
                d0 = d0 << bm;
                n2 = n1 >> b;
                n1 = (n1 << bm) | (n0 >> b);
                n0 = n0 << bm;

                udiv_qrnnd(q0, n1, n2, n1, d1);
                umul_ppmm(m1, m0, q0, d0);

                if (m1 > n1 || (m1 == n1 && m0 > n0)) {
                    q0--;
                    sub_ddmmss(m1, m0, m1, m0, d1, d0);
                }

                q1 = 0;

                /* Remainder in (n1n0 - m1m0) >> bm. */
                if (rp != 0) {
                    sub_ddmmss(n1, n0, n1, n0, m1, m0);
                    rr.s.low = (n1 << b) | (n0 >> bm);
                    rr.s.high = n1 >> bm;
                    *rp = rr.ll;
                }
            }
        }
    }

    ww.s.low = q0;
    ww.s.high = q1;
    return ww.ll;
}

/* ======================================================================= */
/*  Soft-float entry points (GOFAST names, see header)                     */
/* ======================================================================= */

/* dp-bit.o - declared in cxxrt.h:
 *   dpadd dpsub dpmul dpdiv dpcmp litodp dptoli dptoul __negdf2 __make_dp dptofp
 * fp-bit.o - only fptodp (float -> double) is live; the rest are unused.   */
SFtype fpadd(SFtype arg_a, SFtype arg_b);
SFtype fpsub(SFtype arg_a, SFtype arg_b);
SFtype fpmul(SFtype arg_a, SFtype arg_b);
SFtype fpdiv(SFtype arg_a, SFtype arg_b);
int    fpcmp(SFtype arg_a, SFtype arg_b);
SFtype sitofp(SItype arg_a);
SItype fptosi(SFtype arg_a);
USItype fptoui(SFtype arg_a);
SFtype __negsf2(SFtype arg_a);

/* ======================================================================= */
/*  libgcc2.c: 64-bit division and DF <-> DI conversions                   */
/* ======================================================================= */

// FUNC 0x01023678 __divdi3
//   Signed 64-bit divide: negate negative operands (flag c toggled for each,
//   kept in $t7), unsigned divide with no remainder, negate the quotient if
//   exactly one operand was negative.  Only caller: _kernel_vprintf (%d).
DWtype
__divdi3(DWtype u, DWtype v)
{
    word_type c = 0;
    DWunion uu, vv;
    DWtype w;

    uu.ll = u;
    vv.ll = v;

    if (uu.s.high < 0)
        c = ~c,
        uu.ll = __negdi2(uu.ll);
    if (vv.s.high < 0)
        c = ~c,
        vv.ll = __negdi2(vv.ll);

    w = __udivmoddi4(uu.ll, vv.ll, (UDWtype *)0, __clz_tab_divdi3);
    if (c)
        w = __negdi2(w);

    return w;
}

// FUNC 0x01023D68 __fixunsdfdi
//   libgcc2.c double -> unsigned 64-bit.  Every floating operation is an
//   explicit dp-bit call in the binary (shown as such).  `a / 2^32` was
//   folded by the compiler into `a * 2^-32` (0x3DF0000000000000); the
//   UDI -> DF conversion of v uses gcc's halve-convert-double idiom when
//   bit 63 is set (its sticky `v & 1` is always 0 here, v's low word being
//   0); `-a` is `0.0 - a` (GOFAST has no negate libcall).
//   Caller: _print_double.
UDWtype
__fixunsdfdi(DFtype a)
{
    DFtype b;
    DFtype vd;
    UDWtype v;

    if (dpcmp(a, 0.0) < 0)                          /* a < 0 */
        return 0;

    /* Compute high word of result, as a flonum. */
    b = dpmul(a, 0x1p-32);                          /* a / HIGH_WORD_COEFF */
    /* Convert that to fixed (but not to DWtype!), and shift it into the
       high word. */
    v = (UWtype)dptoul(b);
    v <<= WORD_SIZE;
    /* Remove high part from the DFtype, leaving the low part as flonum:
       a -= (DFtype)v, with the unsigned DI -> DF conversion expanded. */
    if ((DWtype)v >= 0) {
        vd = __floatdidf(v);
    } else {
        vd = __floatdidf((v >> 1) | (v & 1));
        vd = dpadd(vd, vd);
    }
    a = dpsub(a, vd);
    /* Convert that to fixed (but not to DWtype!) and add it in.  Sometimes
       A comes out negative.  This is significant, since A has more bits than
       a long int does. */
    if (dpcmp(a, 0.0) < 0)                          /* a < 0 */
        v -= (UWtype)dptoul(dpsub(0.0, a));         /* (UWtype)(-a) */
    else
        v += (UWtype)dptoul(a);
    return v;
}

// FUNC 0x01023E58 __floatdidf
//   libgcc2.c signed 64-bit -> double: (double)(int)hi * 2^16 * 2^16
//   + (double)(unsigned)lo, the unsigned SI -> DF conversion done as litodp
//   plus 2^32 when the low word's top bit is set.  Only caller: __fixunsdfdi.
DFtype
__floatdidf(DWtype u)
{
    DFtype d;
    DFtype lo;
    Wtype  low;

    d = litodp((Wtype)(u >> WORD_SIZE));
    d = dpmul(d, 0x1p16);                           /* *= HIGH_HALFWORD_COEFF (0x40F0000000000000) */
    d = dpmul(d, 0x1p16);                           /* *= HIGH_HALFWORD_COEFF */
    /* d += (UWtype)(u & (HIGH_WORD_COEFF - 1)); */
    low = (Wtype)(u & (HIGH_WORD_COEFF - 1));
    lo = litodp(low);
    if (low < 0)
        lo = dpadd(lo, 0x1p32);                     /* 0x41F0000000000000 */
    return dpadd(d, lo);
}

// FUNC 0x01023EF0 __moddi3
//   Signed 64-bit remainder: only the dividend's sign (kept in $s0) decides
//   the sign of the result; the divisor is just made positive.  w lives at
//   sp+0 and is passed as __udivmoddi4's rp.  Only caller: _kernel_vprintf.
DWtype
__moddi3(DWtype u, DWtype v)
{
    word_type c = 0;
    DWunion uu, vv;
    DWtype w;

    uu.ll = u;
    vv.ll = v;

    if (uu.s.high < 0)
        c = ~c,
        uu.ll = __negdi2(uu.ll);
    if (vv.s.high < 0)
        vv.ll = __negdi2(vv.ll);

    (void)__udivmoddi4(uu.ll, vv.ll, (UDWtype *)&w, __clz_tab_moddi3);
    if (c)
        w = __negdi2(w);

    return w;
}

// FUNC 0x01024558 __udivdi3
//   Unsigned 64-bit divide (quotient only).  Allocates an unused 16-byte
//   frame.  Only caller: _kernel_vprintf (%u/%x/%o digit extraction).
UDWtype
__udivdi3(UDWtype n, UDWtype d)
{
    return __udivmoddi4(n, d, (UDWtype *)0, __clz_tab_udivdi3);
}

// FUNC 0x01024B28 __umoddi3
//   Unsigned 64-bit remainder via __udivmoddi4 with rp = &w (sp+0).
//   Only caller: _kernel_vprintf.
UDWtype
__umoddi3(UDWtype u, UDWtype v)
{
    UDWtype w;

    (void)__udivmoddi4(u, v, &w, __clz_tab_umoddi3);

    return w;
}

/* ======================================================================= */
/*  fp-bit.c common definitions                                            */
/* ======================================================================= */

typedef enum {
    CLASS_SNAN,         /* 0 */
    CLASS_QNAN,         /* 1 */
    CLASS_ZERO,         /* 2 */
    CLASS_NUMBER,       /* 3 */
    CLASS_INFINITY      /* 4 */
} fp_class_type;

#define fp_isnan(x)   ((x)->class == CLASS_SNAN || (x)->class == CLASS_QNAN)
#define fp_isinf(x)   ((x)->class == CLASS_INFINITY)
#define fp_iszero(x)  ((x)->class == CLASS_ZERO)

/* Preserve the sticky-bit when shifting fractions to the right. */
#define LSHIFT(a) { a = (a & 1) | (a >> 1); }

/* F_D_BITOFF: bit offset between the MSB of a float and a double mantissa,
   (52+8) - (23+7). */
#define F_D_BITOFF 30

/* ---- double parameters (dp-bit.o) ---- */
#define NGARDS_D        8
#define GARDROUND_D     0x7f
#define GARDMASK_D      0xff
#define GARDMSB_D       0x80
#define EXPBITS_D       11
#define EXPBIAS_D       1023
#define FRACBITS_D      52
#define EXPMAX_D        0x7ff
#define QUIET_NAN_D     0x8000000000000ULL                           /* 1 << 51 */
#define FRAC_NBITS_D    64
#define NORMAL_EXPMIN_D (-(EXPBIAS_D) + 1)                           /* -1022 */
#define IMPLICIT_1_D    ((UDItype)1 << (FRACBITS_D + NGARDS_D))      /* 1 << 60 */
#define IMPLICIT_2_D    ((UDItype)1 << (FRACBITS_D + 1 + NGARDS_D))  /* 1 << 61 */
#define FRACHIGH_D      0x8000000000000000ULL

/* ---- float parameters (fp-bit.o) ---- */
#define NGARDS_F        7
#define GARDROUND_F     0x3f
#define GARDMASK_F      0x7f
#define GARDMSB_F       0x40
#define EXPBITS_F       8
#define EXPBIAS_F       127
#define FRACBITS_F      23
#define EXPMAX_F        0xff
#define QUIET_NAN_F     0x100000u       /* fp-bit's value (IEEE quiet bit would be 0x400000) */
#define FRAC_NBITS_F    32
#define NORMAL_EXPMIN_F (-(EXPBIAS_F) + 1)                           /* -126 */
#define IMPLICIT_1_F    ((USItype)1 << (FRACBITS_F + NGARDS_F))      /* 0x40000000 */
#define IMPLICIT_2_F    ((USItype)1 << (FRACBITS_F + 1 + NGARDS_F))  /* 0x80000000 */
#define FRACHIGH_F      0x80000000u

#define MAX_SI_INT      ((SItype)0x7fffffff)
#define MAX_USI_INT     ((USItype)~0)

/* Unpacked double (fp-bit fp_number_type, dp-bit flavour). */
typedef struct {
    fp_class_type class;        /* +0x00 */
    unsigned int  sign;         /* +0x04 */
    int           normal_exp;   /* +0x08 unbiased */
                                /* +0x0C padding */
    union {
        UDItype ll;             /* +0x10 1.52 mantissa << 8 (implicit bit 60) */
        USItype l[2];
    } fraction;
} fp_number_type_d;             /* 0x18 */
_Static_assert(sizeof(fp_number_type_d) == 0x18, "dp-bit fp_number_type size");

/* Unpacked float (fp-bit fp_number_type, FLOAT flavour). */
typedef struct {
    fp_class_type class;        /* +0x00 */
    unsigned int  sign;         /* +0x04 */
    int           normal_exp;   /* +0x08 unbiased */
    union {
        USItype ll;             /* +0x0C 1.23 mantissa << 7 (implicit bit 30) */
        UHItype l[2];
    } fraction;
} fp_number_type_f;             /* 0x10 */
_Static_assert(sizeof(fp_number_type_f) == 0x10, "fp-bit fp_number_type size");

/* FLO_union_type.  The original declares an IEEE bitfield struct
 * {fraction:52, exp:11, sign:1} (FLOAT_BIT_ORDER_MISMATCH, little-endian);
 * pack/unpack below use the equivalent shifts and masks on value_raw. */
typedef union { DFtype value; UDItype value_raw; } FLO_union_type_d;
typedef union { SFtype value; USItype value_raw; } FLO_union_type_f;

DFtype __make_dp(fp_class_type class, unsigned int sign, int exp, UDItype frac);
SFtype __make_fp(fp_class_type class, unsigned int sign, int exp, USItype frac);

/* fp-bit nan(): `static fp_number_type thenan;` - zero-initialised .bss,
 * i.e. {CLASS_SNAN, sign 0, exp 0, fraction 0}; packs to +qNaN. */
static inline fp_number_type_d *
nan_d(void)
{
    static fp_number_type_d thenan;     /* @0x0104DD00 (.bss, 0x18 bytes) */
    return &thenan;
}

static inline fp_number_type_f *
nan_f(void)
{
    static fp_number_type_f thenan;     /* @0x0104DD18 (.bss, 0x10 bytes) */
    return &thenan;
}

/* ======================================================================= */
/*  dp-bit.o - double precision                                            */
/* ======================================================================= */

// FUNC 0x01025068 pack_d (dp-bit)
//   Unpacked parts -> IEEE double bits (returned in $v0).  NaN (class < 2):
//   exp 0x7FF, fraction | quiet bit; INF: exp 0x7FF, frac 0; ZERO or a zero
//   fraction: exp 0, frac 0 (sign kept); exponent below -1022: denormal
//   shift (>56 -> 0) then >> 8 guard bits; above 1023: infinity; otherwise
//   round to nearest even on the 8 guard bits, renormalise once on carry
//   into bit 61, rebias.  The three bitfield stores are fraction, exp, sign.
static DFtype
pack_d(fp_number_type_d *src)
{
    FLO_union_type_d dst;
    UDItype fraction = src->fraction.ll;
    unsigned int sign = src->sign;
    int exp = 0;

    if (fp_isnan(src)) {
        exp = EXPMAX_D;
        fraction |= QUIET_NAN_D;
    } else if (fp_isinf(src)) {
        exp = EXPMAX_D;
        fraction = 0;
    } else if (fp_iszero(src)) {
        fraction = 0;
    } else if (fraction == 0) {
        /* exp stays 0 */
    } else {
        if (src->normal_exp < NORMAL_EXPMIN_D) {
            /* This number's exponent is too low to fit into the bits
               available in the number, so store 0 in the exponent and
               shift the fraction to the right to make up for it. */
            int shift = NORMAL_EXPMIN_D - src->normal_exp;

            if (shift > FRAC_NBITS_D - NGARDS_D) {
                /* No point shifting, since it's more than 64 out. */
                fraction = 0;
            } else {
                fraction >>= shift;
            }
            fraction >>= NGARDS_D;
        } else if (src->normal_exp > EXPBIAS_D) {
            exp = EXPMAX_D;
            fraction = 0;
        } else {
            exp = src->normal_exp + EXPBIAS_D;
            /* If the guard bits are all zero but the first, we're half way
               between two numbers: choose the one which makes the lsb of
               the answer 0. */
            if ((fraction & GARDMASK_D) == GARDMSB_D) {
                if (fraction & (1 << NGARDS_D))
                    fraction += GARDROUND_D + 1;
            } else {
                /* Add a one to the guards to round up. */
                fraction += GARDROUND_D;
            }
            if (fraction >= IMPLICIT_2_D) {
                fraction >>= 1;
                exp += 1;
            }
            fraction >>= NGARDS_D;
        }
    }

    dst.value_raw = (fraction & (((UDItype)1 << FRACBITS_D) - 1))           /* bits.fraction */
                  | ((UDItype)(exp & EXPMAX_D) << FRACBITS_D)               /* bits.exp */
                  | ((UDItype)(sign & 1) << (FRACBITS_D + EXPBITS_D));      /* bits.sign */
    return dst.value;
}

// FUNC 0x01025198 unpack_d (dp-bit)
//   IEEE double bits (read with one `ld`) -> unpacked parts.  sign always
//   stored; exp 0 -> CLASS_ZERO (denormals flushed, fraction/normal_exp not
//   written); exp 0x7FF -> INFINITY if the fraction is 0, else QNAN/SNAN by
//   bit 51 with the raw fraction kept; otherwise NUMBER with
//   normal_exp = exp - 1023 and fraction = (frac52 << 8) | 1 << 60.
static void
unpack_d(FLO_union_type_d *src, fp_number_type_d *dst)
{
    UDItype fraction = src->value_raw & (((UDItype)1 << FRACBITS_D) - 1);
    int exp = (int)(src->value_raw >> FRACBITS_D) & EXPMAX_D;
    int sign = (int)(src->value_raw >> (FRACBITS_D + EXPBITS_D));

    dst->sign = sign;
    if (exp == 0) {
        /* Zero - and, in this build, any denormal. */
        dst->class = CLASS_ZERO;
    } else if (exp == EXPMAX_D) {
        /* Huge exponent */
        if (fraction == 0) {
            /* Attached to a zero fraction - means infinity */
            dst->class = CLASS_INFINITY;
        } else {
            /* Non zero fraction, means nan */
            if (fraction & QUIET_NAN_D)
                dst->class = CLASS_QNAN;
            else
                dst->class = CLASS_SNAN;
            /* Keep the fraction part as the nan number */
            dst->fraction.ll = fraction;
        }
    } else {
        /* Nothing strange about this number */
        dst->normal_exp = exp - EXPBIAS_D;
        dst->class = CLASS_NUMBER;
        dst->fraction.ll = (fraction << NGARDS_D) | IMPLICIT_1_D;
    }
}

// FUNC 0x01025238 _fpadd_parts_d (fp-bit `_fpadd_parts`, dp-bit)
//   Adds two unpacked doubles into *tmp (or returns one of the operands /
//   the static NaN).  NaN operands propagate (a first); inf + -inf -> NaN;
//   0 + 0 copies *a (3 x ld/sd) with sign = a.sign & b.sign.  Exponents are
//   aligned one sticky right-shift at a time when they differ by < 64,
//   otherwise the smaller operand's fraction is zeroed.  Different signs:
//   signed difference, magnitude + sign, then left-renormalise while
//   0 < frac < 1<<60; a carry into bit 61 is shifted back (sticky).
//   Callers: dpadd, dpsub.
static fp_number_type_d *
_fpadd_parts_d(fp_number_type_d *a, fp_number_type_d *b, fp_number_type_d *tmp)
{
    DItype tfraction;

    /* Put commonly used fields in local variables. */
    int a_normal_exp;
    int b_normal_exp;
    UDItype a_fraction;
    UDItype b_fraction;

    if (fp_isnan(a))
        return a;
    if (fp_isnan(b))
        return b;
    if (fp_isinf(a)) {
        /* Adding infinities with opposite signs yields a NaN. */
        if (fp_isinf(b) && a->sign != b->sign)
            return nan_d();
        return a;
    }
    if (fp_isinf(b))
        return b;
    if (fp_iszero(b)) {
        if (fp_iszero(a)) {
            *tmp = *a;
            tmp->sign = a->sign & b->sign;
            return tmp;
        }
        return a;
    }
    if (fp_iszero(a))
        return b;

    /* Got two numbers.  Shift the smaller and increment the exponent till
       they're the same. */
    {
        int diff;

        a_normal_exp = a->normal_exp;
        b_normal_exp = b->normal_exp;
        a_fraction = a->fraction.ll;
        b_fraction = b->fraction.ll;

        diff = a_normal_exp - b_normal_exp;

        if (diff < 0)
            diff = -diff;
        if (diff < FRAC_NBITS_D) {
            /* ??? This does shifts one bit at a time.  Optimize. */
            while (a_normal_exp > b_normal_exp) {
                b_normal_exp++;
                LSHIFT(b_fraction);
            }
            while (b_normal_exp > a_normal_exp) {
                a_normal_exp++;
                LSHIFT(a_fraction);
            }
        } else {
            /* Something's up.. choose the biggest */
            if (a_normal_exp > b_normal_exp) {
                b_normal_exp = a_normal_exp;
                b_fraction = 0;
            } else {
                a_normal_exp = b_normal_exp;
                a_fraction = 0;
            }
        }
    }

    if (a->sign != b->sign) {
        if (a->sign)
            tfraction = -a_fraction + b_fraction;
        else
            tfraction = a_fraction - b_fraction;
        if (tfraction >= 0) {
            tmp->sign = 0;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = tfraction;
        } else {
            tmp->sign = 1;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = -tfraction;
        }
        /* and renormalize it */
        while (tmp->fraction.ll < IMPLICIT_1_D && tmp->fraction.ll) {
            tmp->fraction.ll <<= 1;
            tmp->normal_exp--;
        }
    } else {
        tmp->sign = a->sign;
        tmp->normal_exp = a_normal_exp;
        tmp->fraction.ll = a_fraction + b_fraction;
    }
    tmp->class = CLASS_NUMBER;
    /* Now the fraction is added, we have to shift down to renormalize the
       number. */
    if (tmp->fraction.ll >= IMPLICIT_2_D) {
        LSHIFT(tmp->fraction.ll);
        tmp->normal_exp++;
    }
    return tmp;
}

// FUNC 0x01025478 dpadd (fp-bit `add`; = __adddf3)
//   Frame: a @sp+0, b @sp+0x20, tmp @sp+0x40, args spilled @sp+0x60/0x68.
//   Callers: __fixunsdfdi, __floatdidf.
DFtype
dpadd(DFtype arg_a, DFtype arg_b)
{
    fp_number_type_d a;
    fp_number_type_d b;
    fp_number_type_d tmp;
    fp_number_type_d *res;
    FLO_union_type_d ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_d(&ua, &a);
    unpack_d(&ub, &b);

    res = _fpadd_parts_d(&a, &b, &tmp);

    return pack_d(res);
}

// FUNC 0x010254D0 dpsub (fp-bit `sub`; = __subdf3)
//   As dpadd with b.sign ^= 1.  Also the compiler's negation (0.0 - x).
//   Callers: _print_double, __fixunsdfdi.
DFtype
dpsub(DFtype arg_a, DFtype arg_b)
{
    fp_number_type_d a;
    fp_number_type_d b;
    fp_number_type_d tmp;
    fp_number_type_d *res;
    FLO_union_type_d ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_d(&ua, &a);
    unpack_d(&ub, &b);

    b.sign ^= 1;

    res = _fpadd_parts_d(&a, &b, &tmp);

    return pack_d(res);
}

/* fp-bit _fpmul_parts, inlined into dpmul.  The 64x64 -> 128 fraction
 * product is built from four 32x32 partial products, each a call to
 * __muldi3 (0x01018070; the R5900 has no dmultu) with zero-extended
 * operands, in the order ml*nl, mh*nl, ml*nh, mh*nh. */
static inline fp_number_type_d *
_fpmul_parts_d(fp_number_type_d *a, fp_number_type_d *b, fp_number_type_d *tmp)
{
    UDItype low = 0;
    UDItype high = 0;

    if (fp_isnan(a)) {
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_isnan(b)) {
        b->sign = a->sign != b->sign;
        return b;
    }
    if (fp_isinf(a)) {
        if (fp_iszero(b))
            return nan_d();
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_isinf(b)) {
        if (fp_iszero(a))
            return nan_d();
        b->sign = a->sign != b->sign;
        return b;
    }
    if (fp_iszero(a)) {
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_iszero(b)) {
        b->sign = a->sign != b->sign;
        return b;
    }

    /* Calculate the mantissa by multiplying both 64bit numbers to get a
       128 bit number. */
    {
        UDItype nl = a->fraction.ll & 0xffffffff;
        UDItype nh = a->fraction.ll >> 32;
        UDItype ml = b->fraction.ll & 0xffffffff;
        UDItype mh = b->fraction.ll >> 32;
        UDItype pp_ll = (UDItype)__muldi3(ml, nl);
        UDItype pp_hl = (UDItype)__muldi3(mh, nl);
        UDItype pp_lh = (UDItype)__muldi3(ml, nh);
        UDItype pp_hh = (UDItype)__muldi3(mh, nh);
        UDItype res2 = 0;
        UDItype res0 = 0;
        UDItype ps_hh__ = pp_hl + pp_lh;
        if (ps_hh__ < pp_hl)
            res2 += (UDItype)1 << 32;
        pp_hl = (UDItype)(USItype)ps_hh__ << 32;
        res0 = pp_ll + pp_hl;
        if (res0 < pp_ll)
            res2++;
        res2 += ((ps_hh__ >> 32) & 0xffffffff) + pp_hh;   /* `and s0,s0,s6` @0x0102569C */
        high = res2;
        low = res0;
    }

    tmp->normal_exp = a->normal_exp + b->normal_exp;
    tmp->sign = a->sign != b->sign;
    tmp->normal_exp += 4;   /* ??????????????? (fp-bit's own comment) */
    while (high >= IMPLICIT_2_D) {
        tmp->normal_exp++;
        if (high & 1) {
            /* Quirk: `low` is only shifted when a 1 falls out of `high`. */
            low >>= 1;
            low |= FRACHIGH_D;
        }
        high >>= 1;
    }
    while (high < IMPLICIT_1_D) {
        tmp->normal_exp--;

        high <<= 1;
        if (low & FRACHIGH_D)
            high |= 1;
        low <<= 1;
    }
    /* Rounding is tricky.  If we only round if it won't make us round
       later. */
    if ((high & GARDMASK_D) == GARDMSB_D) {
        if (high & (1 << NGARDS_D)) {
            /* half way, so round to even */
            high += GARDROUND_D + 1;
        } else if (low) {
            /* but we really weren't half way */
            high += GARDROUND_D + 1;
        }
    }
    tmp->fraction.ll = high;
    tmp->class = CLASS_NUMBER;
    return tmp;
}

// FUNC 0x01025538 dpmul (fp-bit `multiply`; = __muldf3)
//   unpack_d both, inlined _fpmul_parts (NaN/INF/ZERO dispatch, sign = a!=b,
//   inf*0 -> static NaN), pack_d.  Frame: a @sp+0, b @sp+0x20, tmp @sp+0x40.
//   Callers: _print_double, __fixunsdfdi, __floatdidf.
DFtype
dpmul(DFtype arg_a, DFtype arg_b)
{
    fp_number_type_d a;
    fp_number_type_d b;
    fp_number_type_d tmp;
    fp_number_type_d *res;
    FLO_union_type_d ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_d(&ua, &a);
    unpack_d(&ub, &b);

    res = _fpmul_parts_d(&a, &b, &tmp);

    return pack_d(res);
}

/* fp-bit _fpdiv_parts, inlined into dpdiv: restoring division one quotient
 * bit at a time from bit 60 down, round-half-even on the 8 guard bits with
 * the leftover numerator as sticky. */
static inline fp_number_type_d *
_fpdiv_parts_d(fp_number_type_d *a, fp_number_type_d *b)
{
    UDItype bit;
    UDItype numerator;
    UDItype denominator;
    UDItype quotient;

    if (fp_isnan(a))
        return a;
    if (fp_isnan(b))
        return b;

    a->sign = a->sign ^ b->sign;

    if (fp_isinf(a) || fp_iszero(a)) {
        /* inf/inf and 0/0 are NaN; inf/x and 0/x keep a. */
        if (a->class == b->class)
            return nan_d();
        return a;
    }

    if (fp_isinf(b)) {
        a->fraction.ll = 0;
        a->normal_exp = 0;
        return a;
    }
    if (fp_iszero(b)) {
        a->class = CLASS_INFINITY;
        return a;
    }

    /* quotient =
       (numerator / denominator) * 2^(numerator exponent - denominator exponent) */
    {
        a->normal_exp = a->normal_exp - b->normal_exp;
        numerator = a->fraction.ll;
        denominator = b->fraction.ll;

        if (numerator < denominator) {
            /* Fraction will be less than 1.0 */
            numerator *= 2;
            a->normal_exp--;
        }
        bit = IMPLICIT_1_D;
        quotient = 0;
        /* ??? Does divide one bit at a time.  Optimize. */
        while (bit) {
            if (numerator >= denominator) {
                quotient |= bit;
                numerator -= denominator;
            }
            bit >>= 1;
            numerator *= 2;
        }

        if ((quotient & GARDMASK_D) == GARDMSB_D) {
            if (quotient & (1 << NGARDS_D)) {
                /* half way, so round to even */
                quotient += GARDROUND_D + 1;
            } else if (numerator) {
                /* but we really weren't half way, more bits exist */
                quotient += GARDROUND_D + 1;
            }
        }

        a->fraction.ll = quotient;
        return a;
    }
}

// FUNC 0x010257E0 dpdiv (fp-bit `divide`; = __divdf3)
//   unpack_d both, inlined _fpdiv_parts, pack_d.  Frame: a @sp+0,
//   b @sp+0x20.  Caller: _print_double.
DFtype
dpdiv(DFtype arg_a, DFtype arg_b)
{
    fp_number_type_d a;
    fp_number_type_d b;
    fp_number_type_d *res;
    FLO_union_type_d ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_d(&ua, &a);
    unpack_d(&ub, &b);

    res = _fpdiv_parts_d(&a, &b);

    return pack_d(res);
}

// FUNC 0x01025948 __fpcmp_parts_d (fp-bit `__fpcmp_parts`, dp-bit)
//   Three-way compare of unpacked doubles: any NaN -> 1 ("unordered");
//   inf vs inf -> b.sign - a.sign; then inf/zero cases; numbers by sign,
//   then normal_exp (signed), then fraction (unsigned).  Returns -1/0/1.
//   Only caller: dpcmp.
static int
__fpcmp_parts_d(fp_number_type_d *a, fp_number_type_d *b)
{
    if (fp_isnan(a) || fp_isnan(b))
        return 1;               /* still unordered! */
    if (fp_isinf(a) && fp_isinf(b)) {
        /* +inf > -inf, but +inf != +inf */
        return b->sign - a->sign;
    }
    /* but not both... */
    if (fp_isinf(a))
        return a->sign ? -1 : 1;
    if (fp_isinf(b))
        return b->sign ? 1 : -1;
    if (fp_iszero(a) && fp_iszero(b))
        return 0;
    if (fp_iszero(a))
        return b->sign ? 1 : -1;
    if (fp_iszero(b))
        return a->sign ? -1 : 1;
    /* now both are "normal". */
    if (a->sign != b->sign) {
        /* opposite signs */
        return a->sign ? -1 : 1;
    }
    /* same sign; exponents? */
    if (a->normal_exp > b->normal_exp)
        return a->sign ? -1 : 1;
    if (a->normal_exp < b->normal_exp)
        return a->sign ? 1 : -1;
    /* same exponents; check size. */
    if (a->fraction.ll > b->fraction.ll)
        return a->sign ? -1 : 1;
    if (a->fraction.ll < b->fraction.ll)
        return a->sign ? 1 : -1;
    /* after all that, they're equal. */
    return 0;
}

// FUNC 0x01025A60 dpcmp (fp-bit `compare`; = __cmpdf2)
//   unpack_d both and return __fpcmp_parts: <0, 0, >0 (NaN -> 1).  GOFAST
//   uses it for every double relational operator.  Callers: _print_double,
//   __fixunsdfdi (a < 0.0).
int
dpcmp(DFtype arg_a, DFtype arg_b)
{
    fp_number_type_d a;
    fp_number_type_d b;
    FLO_union_type_d ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_d(&ua, &a);
    unpack_d(&ub, &b);

    return __fpcmp_parts_d(&a, &b);
}

// FUNC 0x01025AB0 litodp (fp-bit `si_to_float`; = __floatsidf)
//   int -> double: 0 -> ZERO; INT_MIN returns the constant -2^31
//   (0xC1E0000000000000) directly; else |i| (sign-extended into the 64-bit
//   fraction) with normal_exp 60, shifted left until bit 60 is set.
//   Only caller: __floatdidf.
DFtype
litodp(SItype arg_a)
{
    fp_number_type_d in;

    in.class = CLASS_NUMBER;
    in.sign = arg_a < 0;
    if (!arg_a) {
        in.class = CLASS_ZERO;
    } else {
        in.normal_exp = FRACBITS_D + NGARDS_D;
        if (in.sign) {
            /* Special case for minint, since there is no +ve integer
               representation for it */
            if (arg_a == (SItype)0x80000000)
                return -2147483648.0;
            in.fraction.ll = (-arg_a);
        } else
            in.fraction.ll = arg_a;

        while (in.fraction.ll < ((UDItype)1 << (FRACBITS_D + NGARDS_D))) {
            in.fraction.ll <<= 1;
            in.normal_exp -= 1;
        }
    }
    return pack_d(&in);
}

// FUNC 0x01025B68 dptoli (fp-bit `float_to_si`; = __fixdfsi)
//   double -> int, truncating: zero/NaN/|x|<1 -> 0; inf or exp > 30 ->
//   0x7FFFFFFF / 0x80000000 by sign; else fraction >> (60 - exp), negated
//   for negative input.  No callers (linked as part of dp-bit.o).
SItype
dptoli(DFtype arg_a)
{
    fp_number_type_d a;
    SItype tmp;
    FLO_union_type_d ua;

    ua.value = arg_a;
    unpack_d(&ua, &a);
    if (fp_iszero(&a))
        return 0;
    if (fp_isnan(&a))
        return 0;
    /* get reasonable MAX_SI_INT... */
    if (fp_isinf(&a))
        return a.sign ? (-MAX_SI_INT) - 1 : MAX_SI_INT;
    /* it is a number, but a small one */
    if (a.normal_exp < 0)
        return 0;
    if (a.normal_exp > 30)
        return a.sign ? (-MAX_SI_INT) - 1 : MAX_SI_INT;
    tmp = a.fraction.ll >> ((FRACBITS_D + NGARDS_D) - a.normal_exp);
    return a.sign ? (-tmp) : (tmp);
}

// FUNC 0x01025C00 dptoul (fp-bit `float_to_usi`; = __fixunsdfsi)
//   double -> unsigned int: zero/NaN/negative/|x|<1 -> 0; inf or exp > 31
//   -> 0xFFFFFFFF; else fraction >> (60 - exp) (the << branch for
//   exp > 60 is unreachable).  Only caller: __fixunsdfdi.
USItype
dptoul(DFtype arg_a)
{
    fp_number_type_d a;
    FLO_union_type_d ua;

    ua.value = arg_a;
    unpack_d(&ua, &a);
    if (fp_iszero(&a))
        return 0;
    if (fp_isnan(&a))
        return 0;
    /* it is a negative number */
    if (a.sign)
        return 0;
    /* get reasonable MAX_USI_INT... */
    if (fp_isinf(&a))
        return MAX_USI_INT;
    /* it is a number, but a small one */
    if (a.normal_exp < 0)
        return 0;
    if (a.normal_exp > 31)
        return MAX_USI_INT;
    else if (a.normal_exp > (FRACBITS_D + NGARDS_D))
        return a.fraction.ll << (a.normal_exp - (FRACBITS_D + NGARDS_D));
    else
        return a.fraction.ll >> ((FRACBITS_D + NGARDS_D) - a.normal_exp);
}

// FUNC 0x01025CA0 __negdf2 (fp-bit `negate`)
//   unpack_d, sign = !sign, pack_d.  No callers (GOFAST negates by
//   subtracting from 0.0 instead).
DFtype
__negdf2(DFtype arg_a)
{
    fp_number_type_d a;
    FLO_union_type_d ua;

    ua.value = arg_a;
    unpack_d(&ua, &a);
    a.sign = !a.sign;           /* flip_sign */
    return pack_d(&a);
}

// FUNC 0x01025CD8 __make_dp
//   Builds unpacked parts {class, sign, exp, frac} on the stack and packs
//   them.  Only caller: fptodp (float -> double).
DFtype
__make_dp(fp_class_type class, unsigned int sign, int exp, UDItype frac)
{
    fp_number_type_d in;

    in.class = class;
    in.sign = sign;
    in.normal_exp = exp;
    in.fraction.ll = frac;
    return pack_d(&in);
}

// FUNC 0x01025D08 dptofp (fp-bit `df_to_sf`; = __truncdfsf2)
//   double -> float: keep fraction bits 61..30 ((frac << 2) >> 32) and OR a
//   sticky 1 if any of the 30 dropped bits (mask 0x3FFFFFFF) was set, then
//   __make_fp(class, sign, exp, sffrac).  No callers.  For ZERO/INF inputs
//   unpack_d leaves normal_exp/fraction unwritten, so stack garbage is read
//   and passed on (harmless: pack ignores both for those classes) - the
//   compiler's maybe-uninitialized warning here is that real quirk.
SFtype
dptofp(DFtype arg_a)
{
    fp_number_type_d in;
    USItype sffrac;
    FLO_union_type_d ua;

    ua.value = arg_a;
    unpack_d(&ua, &in);

    sffrac = in.fraction.ll >> F_D_BITOFF;

    /* We set the lowest guard bit in SFFRAC if we discarded any non zero
       bits. */
    if ((in.fraction.ll & (((USItype)1 << F_D_BITOFF) - 1)) != 0)
        sffrac |= 1;

    return __make_fp(in.class, in.sign, in.normal_exp, sffrac);
}

/* ======================================================================= */
/*  fp-bit.o - single precision (dead except fptodp)                        */
/* ======================================================================= */

// FUNC 0x01025D60 pack_f (fp-bit pack_d, FLOAT)
//   Unpacked parts -> IEEE single, returned via mtc1 in $f0.  As pack_d
//   with 7 guard bits: NaN -> exp 0xFF, frac | 0x100000; denormal shift
//   (>25 -> 0) then >> 7; exponent > 127 -> infinity; round half-even on
//   guard pattern 0x40/0x7F, renormalise on carry into bit 31.
static SFtype
pack_f(fp_number_type_f *src)
{
    FLO_union_type_f dst;
    USItype fraction = src->fraction.ll;
    unsigned int sign = src->sign;
    int exp = 0;

    if (fp_isnan(src)) {
        exp = EXPMAX_F;
        fraction |= QUIET_NAN_F;
    } else if (fp_isinf(src)) {
        exp = EXPMAX_F;
        fraction = 0;
    } else if (fp_iszero(src)) {
        fraction = 0;
    } else if (fraction == 0) {
        /* exp stays 0 */
    } else {
        if (src->normal_exp < NORMAL_EXPMIN_F) {
            int shift = NORMAL_EXPMIN_F - src->normal_exp;

            if (shift > FRAC_NBITS_F - NGARDS_F)
                fraction = 0;
            else
                fraction >>= shift;
            fraction >>= NGARDS_F;
        } else if (src->normal_exp > EXPBIAS_F) {
            exp = EXPMAX_F;
            fraction = 0;
        } else {
            exp = src->normal_exp + EXPBIAS_F;
            if ((fraction & GARDMASK_F) == GARDMSB_F) {
                if (fraction & (1 << NGARDS_F))
                    fraction += GARDROUND_F + 1;
            } else {
                fraction += GARDROUND_F;
            }
            if (fraction >= IMPLICIT_2_F) {
                fraction >>= 1;
                exp += 1;
            }
            fraction >>= NGARDS_F;
        }
    }

    dst.value_raw = (fraction & (((USItype)1 << FRACBITS_F) - 1))           /* bits.fraction */
                  | ((USItype)(exp & EXPMAX_F) << FRACBITS_F)               /* bits.exp */
                  | ((USItype)(sign & 1) << (FRACBITS_F + EXPBITS_F));      /* bits.sign */
    return dst.value;
}

// FUNC 0x01025E70 unpack_f (fp-bit unpack_d, FLOAT)
//   IEEE single -> parts: exp 0 -> ZERO (denormals flushed); exp 0xFF ->
//   INFINITY or QNAN/SNAN by bit 0x100000 (raw fraction kept); otherwise
//   NUMBER, normal_exp = exp - 127, fraction = (frac23 << 7) | 0x40000000.
static void
unpack_f(FLO_union_type_f *src, fp_number_type_f *dst)
{
    USItype fraction = src->value_raw & (((USItype)1 << FRACBITS_F) - 1);
    int exp = (int)(src->value_raw >> FRACBITS_F) & EXPMAX_F;
    int sign = (int)(src->value_raw >> (FRACBITS_F + EXPBITS_F));

    dst->sign = sign;
    if (exp == 0) {
        dst->class = CLASS_ZERO;
    } else if (exp == EXPMAX_F) {
        if (fraction == 0) {
            dst->class = CLASS_INFINITY;
        } else {
            if (fraction & QUIET_NAN_F)
                dst->class = CLASS_QNAN;
            else
                dst->class = CLASS_SNAN;
            dst->fraction.ll = fraction;
        }
    } else {
        dst->normal_exp = exp - EXPBIAS_F;
        dst->class = CLASS_NUMBER;
        dst->fraction.ll = (fraction << NGARDS_F) | IMPLICIT_1_F;
    }
}

// FUNC 0x01025F00 _fpadd_parts_f (fp-bit `_fpadd_parts`, FLOAT)
//   Single-precision twin of the dp-bit _fpadd_parts: same special cases
//   (inf + -inf -> static NaN @0x0104DD18; 0 + 0 copies *a with two
//   unaligned ldl/ldr/sdl/sdr doublewords), alignment only when the
//   exponents differ by < 32, renormalise below 0x40000000, sticky shift on
//   carry into bit 31.  Callers: fpadd, fpsub.
static fp_number_type_f *
_fpadd_parts_f(fp_number_type_f *a, fp_number_type_f *b, fp_number_type_f *tmp)
{
    SItype tfraction;

    int a_normal_exp;
    int b_normal_exp;
    USItype a_fraction;
    USItype b_fraction;

    if (fp_isnan(a))
        return a;
    if (fp_isnan(b))
        return b;
    if (fp_isinf(a)) {
        if (fp_isinf(b) && a->sign != b->sign)
            return nan_f();
        return a;
    }
    if (fp_isinf(b))
        return b;
    if (fp_iszero(b)) {
        if (fp_iszero(a)) {
            *tmp = *a;
            tmp->sign = a->sign & b->sign;
            return tmp;
        }
        return a;
    }
    if (fp_iszero(a))
        return b;

    {
        int diff;

        a_normal_exp = a->normal_exp;
        b_normal_exp = b->normal_exp;
        a_fraction = a->fraction.ll;
        b_fraction = b->fraction.ll;

        diff = a_normal_exp - b_normal_exp;

        if (diff < 0)
            diff = -diff;
        if (diff < FRAC_NBITS_F) {
            while (a_normal_exp > b_normal_exp) {
                b_normal_exp++;
                LSHIFT(b_fraction);
            }
            while (b_normal_exp > a_normal_exp) {
                a_normal_exp++;
                LSHIFT(a_fraction);
            }
        } else {
            if (a_normal_exp > b_normal_exp) {
                b_normal_exp = a_normal_exp;
                b_fraction = 0;
            } else {
                a_normal_exp = b_normal_exp;
                a_fraction = 0;
            }
        }
    }

    if (a->sign != b->sign) {
        if (a->sign)
            tfraction = -a_fraction + b_fraction;
        else
            tfraction = a_fraction - b_fraction;
        if (tfraction >= 0) {
            tmp->sign = 0;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = tfraction;
        } else {
            tmp->sign = 1;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = -tfraction;
        }
        while (tmp->fraction.ll < IMPLICIT_1_F && tmp->fraction.ll) {
            tmp->fraction.ll <<= 1;
            tmp->normal_exp--;
        }
    } else {
        tmp->sign = a->sign;
        tmp->normal_exp = a_normal_exp;
        tmp->fraction.ll = a_fraction + b_fraction;
    }
    tmp->class = CLASS_NUMBER;
    if (tmp->fraction.ll >= IMPLICIT_2_F) {
        LSHIFT(tmp->fraction.ll);
        tmp->normal_exp++;
    }
    return tmp;
}

// FUNC 0x01026138 fpadd (fp-bit `add`, FLOAT; = __addsf3)
//   Args arrive in $f12/$f13 and are spilled (swc1) to sp+0x30/0x34;
//   a @sp+0, b @sp+0x10, tmp @sp+0x20.  No callers (the EE FPU adds singles
//   in hardware); linked only because fptodp shares the object.
SFtype
fpadd(SFtype arg_a, SFtype arg_b)
{
    fp_number_type_f a;
    fp_number_type_f b;
    fp_number_type_f tmp;
    fp_number_type_f *res;
    FLO_union_type_f ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_f(&ua, &a);
    unpack_f(&ub, &b);

    res = _fpadd_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

// FUNC 0x01026190 fpsub (fp-bit `sub`, FLOAT; = __subsf3)
//   As fpadd with b.sign ^= 1.  No callers.
SFtype
fpsub(SFtype arg_a, SFtype arg_b)
{
    fp_number_type_f a;
    fp_number_type_f b;
    fp_number_type_f tmp;
    fp_number_type_f *res;
    FLO_union_type_f ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_f(&ua, &a);
    unpack_f(&ub, &b);

    b.sign ^= 1;

    res = _fpadd_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

/* fp-bit _fpmul_parts (FLOAT), inlined into fpmul: one `multu` of the two
 * 31-bit fractions gives the 64-bit product (no __muldi3 here). */
static inline fp_number_type_f *
_fpmul_parts_f(fp_number_type_f *a, fp_number_type_f *b, fp_number_type_f *tmp)
{
    USItype low = 0;
    USItype high = 0;

    if (fp_isnan(a)) {
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_isnan(b)) {
        b->sign = a->sign != b->sign;
        return b;
    }
    if (fp_isinf(a)) {
        if (fp_iszero(b))
            return nan_f();
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_isinf(b)) {
        if (fp_iszero(a))
            return nan_f();
        b->sign = a->sign != b->sign;
        return b;
    }
    if (fp_iszero(a)) {
        a->sign = a->sign != b->sign;
        return a;
    }
    if (fp_iszero(b)) {
        b->sign = a->sign != b->sign;
        return b;
    }

    {
        /* Multiplying two 32 bit numbers to get a 64 bit number on a
           machine with DI, so we're safe. */
        DItype answer = (DItype)a->fraction.ll * (DItype)b->fraction.ll;

        high = answer >> 32;
        low = answer;
    }

    tmp->normal_exp = a->normal_exp + b->normal_exp;
    tmp->sign = a->sign != b->sign;
    tmp->normal_exp += 2;   /* ??????????????? */
    while (high >= IMPLICIT_2_F) {
        tmp->normal_exp++;
        if (high & 1) {
            low >>= 1;
            low |= FRACHIGH_F;
        }
        high >>= 1;
    }
    while (high < IMPLICIT_1_F) {
        tmp->normal_exp--;

        high <<= 1;
        if (low & FRACHIGH_F)
            high |= 1;
        low <<= 1;
    }
    if ((high & GARDMASK_F) == GARDMSB_F) {
        if (high & (1 << NGARDS_F)) {
            high += GARDROUND_F + 1;
        } else if (low) {
            high += GARDROUND_F + 1;
        }
    }
    tmp->fraction.ll = high;
    tmp->class = CLASS_NUMBER;
    return tmp;
}

// FUNC 0x010261F8 fpmul (fp-bit `multiply`, FLOAT; = __mulsf3)
//   unpack_f both, inlined _fpmul_parts, pack_f.  a @sp+0, b @sp+0x10,
//   tmp @sp+0x20.  No callers.
SFtype
fpmul(SFtype arg_a, SFtype arg_b)
{
    fp_number_type_f a;
    fp_number_type_f b;
    fp_number_type_f tmp;
    fp_number_type_f *res;
    FLO_union_type_f ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_f(&ua, &a);
    unpack_f(&ub, &b);

    res = _fpmul_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

/* fp-bit _fpdiv_parts (FLOAT), inlined into fpdiv. */
static inline fp_number_type_f *
_fpdiv_parts_f(fp_number_type_f *a, fp_number_type_f *b)
{
    USItype bit;
    USItype numerator;
    USItype denominator;
    USItype quotient;

    if (fp_isnan(a))
        return a;
    if (fp_isnan(b))
        return b;

    a->sign = a->sign ^ b->sign;

    if (fp_isinf(a) || fp_iszero(a)) {
        if (a->class == b->class)
            return nan_f();
        return a;
    }

    if (fp_isinf(b)) {
        a->fraction.ll = 0;
        a->normal_exp = 0;
        return a;
    }
    if (fp_iszero(b)) {
        a->class = CLASS_INFINITY;
        return a;
    }

    {
        a->normal_exp = a->normal_exp - b->normal_exp;
        numerator = a->fraction.ll;
        denominator = b->fraction.ll;

        if (numerator < denominator) {
            numerator *= 2;
            a->normal_exp--;
        }
        bit = IMPLICIT_1_F;
        quotient = 0;
        while (bit) {
            if (numerator >= denominator) {
                quotient |= bit;
                numerator -= denominator;
            }
            bit >>= 1;
            numerator *= 2;
        }

        if ((quotient & GARDMASK_F) == GARDMSB_F) {
            if (quotient & (1 << NGARDS_F)) {
                quotient += GARDROUND_F + 1;
            } else if (numerator) {
                quotient += GARDROUND_F + 1;
            }
        }

        a->fraction.ll = quotient;
        return a;
    }
}

// FUNC 0x010263F0 fpdiv (fp-bit `divide`, FLOAT; = __divsf3)
//   unpack_f both, inlined _fpdiv_parts (0/0, inf/inf -> static NaN;
//   x/inf -> 0; x/0 -> inf), pack_f.  No callers.
SFtype
fpdiv(SFtype arg_a, SFtype arg_b)
{
    fp_number_type_f a;
    fp_number_type_f b;
    fp_number_type_f *res;
    FLO_union_type_f ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_f(&ua, &a);
    unpack_f(&ub, &b);

    res = _fpdiv_parts_f(&a, &b);

    return pack_f(res);
}

// FUNC 0x01026550 __fpcmp_parts_f (fp-bit `__fpcmp_parts`, FLOAT)
//   Single-precision twin of the dp-bit __fpcmp_parts (32-bit fraction
//   compare).  Only caller: fpcmp.
static int
__fpcmp_parts_f(fp_number_type_f *a, fp_number_type_f *b)
{
    if (fp_isnan(a) || fp_isnan(b))
        return 1;
    if (fp_isinf(a) && fp_isinf(b))
        return b->sign - a->sign;
    if (fp_isinf(a))
        return a->sign ? -1 : 1;
    if (fp_isinf(b))
        return b->sign ? 1 : -1;
    if (fp_iszero(a) && fp_iszero(b))
        return 0;
    if (fp_iszero(a))
        return b->sign ? 1 : -1;
    if (fp_iszero(b))
        return a->sign ? -1 : 1;
    if (a->sign != b->sign)
        return a->sign ? -1 : 1;
    if (a->normal_exp > b->normal_exp)
        return a->sign ? -1 : 1;
    if (a->normal_exp < b->normal_exp)
        return a->sign ? 1 : -1;
    if (a->fraction.ll > b->fraction.ll)
        return a->sign ? -1 : 1;
    if (a->fraction.ll < b->fraction.ll)
        return a->sign ? 1 : -1;
    return 0;
}

// FUNC 0x01026668 fpcmp (fp-bit `compare`, FLOAT; = __cmpsf2)
//   unpack_f both, return __fpcmp_parts.  No callers.
int
fpcmp(SFtype arg_a, SFtype arg_b)
{
    fp_number_type_f a;
    fp_number_type_f b;
    FLO_union_type_f ua, ub;

    ua.value = arg_a;
    ub.value = arg_b;
    unpack_f(&ua, &a);
    unpack_f(&ub, &b);

    return __fpcmp_parts_f(&a, &b);
}

// FUNC 0x010266B8 sitofp (fp-bit `si_to_float`, FLOAT; = __floatsisf)
//   int -> float: 0 -> ZERO; INT_MIN -> constant 0xCF000000 (-2^31f)
//   loaded straight into $f0; else |i| with normal_exp 30 shifted left until
//   bit 30 is set.  No callers.
SFtype
sitofp(SItype arg_a)
{
    fp_number_type_f in;

    in.class = CLASS_NUMBER;
    in.sign = arg_a < 0;
    if (!arg_a) {
        in.class = CLASS_ZERO;
    } else {
        in.normal_exp = FRACBITS_F + NGARDS_F;
        if (in.sign) {
            if (arg_a == (SItype)0x80000000)
                return -2147483648.0f;
            in.fraction.ll = (-arg_a);
        } else
            in.fraction.ll = arg_a;

        while (in.fraction.ll < ((USItype)1 << (FRACBITS_F + NGARDS_F))) {
            in.fraction.ll <<= 1;
            in.normal_exp -= 1;
        }
    }
    return pack_f(&in);
}

// FUNC 0x01026770 fptosi (fp-bit `float_to_si`, FLOAT; = __fixsfsi)
//   float -> int: zero/NaN/|x|<1 -> 0; inf or exp > 30 -> 0x7FFFFFFF /
//   0x80000000 by sign; else fraction >> (30 - exp), negated if negative.
//   No callers.
SItype
fptosi(SFtype arg_a)
{
    fp_number_type_f a;
    SItype tmp;
    FLO_union_type_f ua;

    ua.value = arg_a;
    unpack_f(&ua, &a);
    if (fp_iszero(&a))
        return 0;
    if (fp_isnan(&a))
        return 0;
    if (fp_isinf(&a))
        return a.sign ? (-MAX_SI_INT) - 1 : MAX_SI_INT;
    if (a.normal_exp < 0)
        return 0;
    if (a.normal_exp > 30)
        return a.sign ? (-MAX_SI_INT) - 1 : MAX_SI_INT;
    tmp = a.fraction.ll >> ((FRACBITS_F + NGARDS_F) - a.normal_exp);
    return a.sign ? (-tmp) : (tmp);
}

// FUNC 0x01026800 fptoui (fp-bit `float_to_usi`, FLOAT; = __fixunssfsi)
//   float -> unsigned int: zero/NaN/negative/|x|<1 -> 0 (the sign IS
//   tested, lw 4(sp) at 0x0102682C); inf or exp > 31 -> 0xFFFFFFFF;
//   exp == 31 -> fraction << 1; else fraction >> (30 - exp).  No callers.
USItype
fptoui(SFtype arg_a)
{
    fp_number_type_f a;
    FLO_union_type_f ua;

    ua.value = arg_a;
    unpack_f(&ua, &a);
    if (fp_iszero(&a))
        return 0;
    if (fp_isnan(&a))
        return 0;
    if (a.sign)
        return 0;
    if (fp_isinf(&a))
        return MAX_USI_INT;
    if (a.normal_exp < 0)
        return 0;
    if (a.normal_exp > 31)
        return MAX_USI_INT;
    else if (a.normal_exp > (FRACBITS_F + NGARDS_F))
        return a.fraction.ll << (a.normal_exp - (FRACBITS_F + NGARDS_F));
    else
        return a.fraction.ll >> ((FRACBITS_F + NGARDS_F) - a.normal_exp);
}

// FUNC 0x01026898 __negsf2 (fp-bit `negate`, FLOAT)
//   unpack_f, sign = !sign, pack_f.  No callers.
SFtype
__negsf2(SFtype arg_a)
{
    fp_number_type_f a;
    FLO_union_type_f ua;

    ua.value = arg_a;
    unpack_f(&ua, &a);
    a.sign = !a.sign;           /* flip_sign */
    return pack_f(&a);
}

// FUNC 0x010268D0 __make_fp
//   Builds unpacked float parts {class, sign, exp, frac} on the stack and
//   packs them (result in $f0).  Only caller: dptofp.
SFtype
__make_fp(fp_class_type class, unsigned int sign, int exp, USItype frac)
{
    fp_number_type_f in;

    in.class = class;
    in.sign = sign;
    in.normal_exp = exp;
    in.fraction.ll = frac;
    return pack_f(&in);
}

// FUNC 0x01026900 fptodp (fp-bit `sf_to_df`; = __extendsfdf2)
//   float ($f12) -> double: unpack_f and __make_dp with the fraction widened
//   by F_D_BITOFF (zero-extend, << 32, >> 2).  The only live routine of
//   fp-bit.o: _kernel_vprintf calls it for %f/%e arguments.  Like dptofp it
//   forwards unwritten normal_exp/fraction for ZERO/INF (ignored by pack_d).
DFtype
fptodp(SFtype arg_a)
{
    fp_number_type_f in;
    FLO_union_type_f ua;

    ua.value = arg_a;
    unpack_f(&ua, &in);
    return __make_dp(in.class, in.sign, in.normal_exp,
                     ((UDItype)in.fraction.ll) << F_D_BITOFF);
}
