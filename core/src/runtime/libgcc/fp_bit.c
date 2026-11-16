/* fp_bit.c - libgcc fp-bit.o: gcc config/fp-bit.c compiled for FLOAT
 * (soft single precision), 0x0027A400..0x0027AFE0 of the GT3 US main executable.
 *
 * The same machine code as the bootstrap's runtime/libgcc_math.c (fp-bit part,
 * 0x01025D60..) and as fp-bit.o in the toolchain's own libgcc.a.  The libgcc of
 * this toolchain uses the US Software GOFAST names (fpadd/fpsub/.../fptodp);
 * the standard libgcc name is noted next to each.  The EE FPU does single
 * precision in hardware, so the arithmetic routines here have no callers: the
 * object is linked for fptodp (__extendsfdf2, 44 callers - every float passed
 * to a varargs printf) and fptoui (__fixunssfsi, 8 callers), and __make_fp for
 * dp-bit's dptofp.
 *
 * The source is fp-bit.c as the MIPS t-file generates it: FLOAT,
 * FLOAT_BIT_ORDER_MISMATCH (little-endian: pack/unpack go through the
 * bitfield view of FLO_union_type), US_SOFTWARE_GOFAST, NO_DENORMALS
 * (unpack_f flushes denormals to zero).  pack_f always ORs fp-bit's
 * QUIET_NAN 0x100000 into a NaN; the static NaN (fp-bit's nan()'s `thenan')
 * is nan_f()'s local static, .bss 0x004AD240.  libgcc.a's symbol names: pack_f = __pack_f,
 * unpack_f = __unpack_f, _fpadd_parts_f = _fpadd_parts (static).
 */
#include "runtime/libgcc/fp_bit_internal.h"
#include "runtime/libgcc/G006_fp_bit.h"

#define NO_DENORMALS

typedef fp_number_type_f fp_number_type;
typedef FLO_union_type_f FLO_union_type;
typedef USItype fractype;
typedef SFtype  FLO_type;
typedef SItype  intfrac;

/* The argument of the public entry points, seen as FLO_union_type.  fp-bit
 * passes the address of the float argument itself (retail spills $f12/$f13
 * to the frame and passes that address); GCC 14 gets a union copy, which is
 * defined type punning there. */
#ifdef BOOT_OLD_GCC
#define FLO_ARG(x) ((FLO_union_type *)&(x))
#else
#define FLO_ARG(x) (&(FLO_union_type){ .value = (x) })
#endif

/* IEEE "special" number predicates (fp-bit's INLINE functions).
 * nan()'s `static fp_number_type thenan;' is the object's whole .bss
 * (0x004AD240, 0x10 bytes, 8-aligned; the toolchain's fp-bit.o: local
 * `b thenan.3'): zero, i.e. {CLASS_SNAN, sign 0, exp 0, fraction 0}, which
 * packs to +qNaN.  Used by _fpadd_parts_f, fpmul and fpdiv. */

static __inline__ fp_number_type *
nan_f(void)
{
    static fp_number_type thenan;       /* .bss 0x004AD240 */

    return &thenan;
}

static __inline__ int
fp_isnan(fp_number_type *x)
{
    return x->class == CLASS_SNAN || x->class == CLASS_QNAN;
}

static __inline__ int
fp_isinf(fp_number_type *x)
{
    return x->class == CLASS_INFINITY;
}

static __inline__ int
fp_iszero(fp_number_type *x)
{
    return x->class == CLASS_ZERO;
}

static __inline__ void
flip_sign(fp_number_type *x)
{
    x->sign = !x->sign;
}

// FUNC 0x0027A400 pack_f
//   Unpacked parts -> IEEE single, returned via mtc1 in $f0.  As pack_d
//   with 7 guard bits: NaN -> exp 0xFF, frac | 0x100000; denormal shift
//   (>25 -> 0) then >> 7; exponent > 127 -> infinity; round half-even on
//   guard pattern 0x40/0x7F, renormalise on carry into bit 31.  The three
//   bitfield stores mask and merge into the (uninitialised) result word.
static FLO_type
pack_f(fp_number_type *src)
{
    FLO_union_type dst;
    fractype fraction = src->fraction.ll;
    int sign = src->sign;
    int exp = 0;

    if (fp_isnan(src)) {
        exp = EXPMAX_F;
        if (src->class == CLASS_QNAN || 1) {
            fraction |= QUIET_NAN_F;
        }
    } else if (fp_isinf(src)) {
        exp = EXPMAX_F;
        fraction = 0;
    } else if (fp_iszero(src)) {
        exp = 0;
        fraction = 0;
    } else if (fraction == 0) {
        exp = 0;
    } else {
        if (src->normal_exp < NORMAL_EXPMIN_F) {
            /* Too small for the exponent field: store 0 there and shift
               the fraction right instead. */
            int shift = NORMAL_EXPMIN_F - src->normal_exp;

            exp = 0;

            if (shift > FRAC_NBITS_F - NGARDS_F) {
                fraction = 0;
            } else {
                fraction >>= shift;
            }
            fraction >>= NGARDS_F;
        } else if (src->normal_exp > EXPBIAS_F) {
            exp = EXPMAX_F;
            fraction = 0;
        } else {
            exp = src->normal_exp + EXPBIAS_F;
            /* Half way between two numbers: choose the even one. */
            if ((fraction & GARDMASK_F) == GARDMSB_F) {
                if (fraction & (1 << NGARDS_F))
                    fraction += GARDROUND_F + 1;
            } else {
                /* Add a one to the guards to round up. */
                fraction += GARDROUND_F;
            }
            if (fraction >= IMPLICIT_2_F) {     /* bgezl: sign bit set */
                fraction >>= 1;
                exp += 1;
            }
            fraction >>= NGARDS_F;
        }
    }

    dst.bits.fraction = fraction;
    dst.bits.exp = exp;
    dst.bits.sign = sign;
    return dst.value;
}

// FUNC 0x0027A510 unpack_f
//   IEEE single -> parts: exp 0 -> ZERO (denormals flushed, fraction and
//   normal_exp left unwritten); exp 0xFF -> INFINITY or QNAN/SNAN by bit
//   0x100000 (raw fraction kept); otherwise NUMBER, normal_exp = exp - 127,
//   fraction = (frac23 << 7) | 0x40000000.
static void
unpack_f(FLO_union_type *src, fp_number_type *dst)
{
    fractype fraction;
    int exp;
    int sign;

    fraction = src->bits.fraction;
    exp = src->bits.exp;
    sign = src->bits.sign;

    dst->sign = sign;
    if (exp == 0) {
        /* Looks like 0. */
        if (fraction == 0
#ifdef NO_DENORMALS
            || 1
#endif
            ) {
            dst->class = CLASS_ZERO;
        } else {
            /* Denormal: no implicit leading one, shift until there is one. */
            dst->normal_exp = exp - EXPBIAS_F + 1;
            fraction <<= NGARDS_F;

            dst->class = CLASS_NUMBER;
            while (fraction < IMPLICIT_1_F) {
                fraction <<= 1;
                dst->normal_exp--;
            }
            dst->fraction.ll = fraction;
        }
    } else if (exp == EXPMAX_F) {
        if (fraction == 0) {
            /* Attached to a zero fraction: infinity. */
            dst->class = CLASS_INFINITY;
        } else {
            /* Non-zero fraction: a NaN. */
            if (fraction & QUIET_NAN_F) {
                dst->class = CLASS_QNAN;
            } else {
                dst->class = CLASS_SNAN;
            }
            /* Keep the fraction part as the NaN number. */
            dst->fraction.ll = fraction;
        }
    } else {
        dst->normal_exp = exp - EXPBIAS_F;
        dst->class = CLASS_NUMBER;
        dst->fraction.ll = (fraction << NGARDS_F) | IMPLICIT_1_F;
    }
}

// FUNC 0x0027A5A0 _fpadd_parts_f
//   fp-bit `_fpadd_parts': special cases (inf + -inf -> static NaN @0x004AD240;
//   0 + 0 copies *a with two unaligned ldl/ldr/sdl/sdr doublewords), alignment
//   only when the exponents differ by < 32, renormalise below 0x40000000,
//   sticky shift on carry into bit 31.  Callers: fpadd, fpsub.
static fp_number_type *
_fpadd_parts_f(fp_number_type *a, fp_number_type *b, fp_number_type *tmp)
{
    intfrac tfraction;

    /* Put commonly used fields in local variables. */
    int a_normal_exp;
    int b_normal_exp;
    fractype a_fraction;
    fractype b_fraction;

    if (fp_isnan(a)) {
        return a;
    }
    if (fp_isnan(b)) {
        return b;
    }
    if (fp_isinf(a)) {
        /* Adding infinities with opposite signs yields a NaN. */
        if (fp_isinf(b) && a->sign != b->sign)
            return nan_f();
        return a;
    }
    if (fp_isinf(b)) {
        return b;
    }
    if (fp_iszero(b)) {
        if (fp_iszero(a)) {
            *tmp = *a;
            tmp->sign = a->sign & b->sign;
            return tmp;
        }
        return a;
    }
    if (fp_iszero(a)) {
        return b;
    }

    /* Got two numbers: shift the smaller and increment its exponent until
       they are the same. */
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
            /* Something's up: choose the biggest. */
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
        if (a->sign) {
            tfraction = -a_fraction + b_fraction;
        } else {
            tfraction = a_fraction - b_fraction;
        }
        if (tfraction >= 0) {
            tmp->sign = 0;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = tfraction;
        } else {
            tmp->sign = 1;
            tmp->normal_exp = a_normal_exp;
            tmp->fraction.ll = -tfraction;
        }
        /* and renormalise it */
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
    /* Now the fraction is added, shift down to renormalise the number. */
    if (tmp->fraction.ll >= IMPLICIT_2_F) {
        LSHIFT(tmp->fraction.ll);
        tmp->normal_exp++;
    }
    return tmp;
}

// FUNC 0x0027A7D8 fpadd
//   fp-bit `add' (= __addsf3).  Args arrive in $f12/$f13 and are spilled to
//   the stack (their addresses are passed to unpack_f); a @sp+0, b @sp+0x10,
//   tmp @sp+0x20.  No callers.
FLO_type
fpadd(FLO_type arg_a, FLO_type arg_b)
{
    fp_number_type a;
    fp_number_type b;
    fp_number_type tmp;
    fp_number_type *res;

    unpack_f(FLO_ARG(arg_a), &a);
    unpack_f(FLO_ARG(arg_b), &b);

    res = _fpadd_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

// FUNC 0x0027A830 fpsub
//   fp-bit `sub' (= __subsf3): as fpadd with b.sign ^= 1.  No callers.
FLO_type
fpsub(FLO_type arg_a, FLO_type arg_b)
{
    fp_number_type a;
    fp_number_type b;
    fp_number_type tmp;
    fp_number_type *res;

    unpack_f(FLO_ARG(arg_a), &a);
    unpack_f(FLO_ARG(arg_b), &b);

    b.sign ^= 1;

    res = _fpadd_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

/* fp-bit _fpmul_parts (FLOAT), inlined into fpmul: one `multu' of the two
 * 31-bit fractions gives the 64-bit product (no __muldi3 here). */
static __inline__ fp_number_type *
_fpmul_parts_f(fp_number_type *a, fp_number_type *b, fp_number_type *tmp)
{
    fractype low = 0;
    fractype high = 0;

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
        if (fp_iszero(a)) {
            return nan_f();
        }
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

    /* Multiplying two 32-bit numbers to get a 64-bit number on a machine
       with DI. */
    {
        DItype answer = (DItype)(a->fraction.ll) * (DItype)(b->fraction.ll);

        high = answer >> 32;
        low = answer;
    }

    tmp->normal_exp = a->normal_exp + b->normal_exp;
    tmp->sign = a->sign != b->sign;
    tmp->normal_exp += 2;       /* fp-bit: FLOAT */
    while (high >= IMPLICIT_2_F) {
        tmp->normal_exp++;
        if (high & 1) {         /* stock fp-bit: low only shifted when high is odd */
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
            /* half way, so round to even */
            high += GARDROUND_F + 1;
        } else if (low) {
            /* but we really weren't half way */
            high += GARDROUND_F + 1;
        }
    }
    tmp->fraction.ll = high;
    tmp->class = CLASS_NUMBER;
    return tmp;
}

// FUNC 0x0027A898 fpmul
//   fp-bit `multiply' (= __mulsf3): unpack_f both, inlined _fpmul_parts,
//   pack_f.  a @sp+0, b @sp+0x10, tmp @sp+0x20.  No callers.
FLO_type
fpmul(FLO_type arg_a, FLO_type arg_b)
{
    fp_number_type a;
    fp_number_type b;
    fp_number_type tmp;
    fp_number_type *res;

    unpack_f(FLO_ARG(arg_a), &a);
    unpack_f(FLO_ARG(arg_b), &b);

    res = _fpmul_parts_f(&a, &b, &tmp);

    return pack_f(res);
}

/* fp-bit _fpdiv_parts (FLOAT), inlined into fpdiv (this version has no tmp
 * argument: the result is built in *a). */
static __inline__ fp_number_type *
_fpdiv_parts_f(fp_number_type *a, fp_number_type *b)
{
    fractype bit;
    fractype numerator;
    fractype denominator;
    fractype quotient;

    if (fp_isnan(a)) {
        return a;
    }
    if (fp_isnan(b)) {
        return b;
    }

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

    /* quotient = (numerator / denominator)
                  * 2^(numerator exponent - denominator exponent) */
    {
        a->normal_exp = a->normal_exp - b->normal_exp;
        numerator = a->fraction.ll;
        denominator = b->fraction.ll;

        if (numerator < denominator) {
            /* Fraction will be less than 1.0 */
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
                /* half way, so round to even */
                quotient += GARDROUND_F + 1;
            } else if (numerator) {
                /* but we really weren't half way, more bits exist */
                quotient += GARDROUND_F + 1;
            }
        }

        a->fraction.ll = quotient;
        return (a);
    }
}

// FUNC 0x0027AA90 fpdiv
//   fp-bit `divide' (= __divsf3): unpack_f both, inlined _fpdiv_parts (0/0,
//   inf/inf -> static NaN; x/inf -> 0; x/0 -> inf), pack_f.  a @sp+0,
//   b @sp+0x10 (no tmp).  No callers.
FLO_type
fpdiv(FLO_type arg_a, FLO_type arg_b)
{
    fp_number_type a;
    fp_number_type b;
    fp_number_type *res;

    unpack_f(FLO_ARG(arg_a), &a);
    unpack_f(FLO_ARG(arg_b), &b);

    res = _fpdiv_parts_f(&a, &b);

    return pack_f(res);
}

// FUNC 0x0027ABF0 __fpcmp_parts_f
//   fp-bit `__fpcmp_parts' (FLOAT): <0 / 0 / >0, NaN -> 1.  Only caller: fpcmp.
static int
__fpcmp_parts_f(fp_number_type *a, fp_number_type *b)
{
    if (fp_isnan(a) || fp_isnan(b)) {
        return 1;               /* how to indicate unordered compare? */
    }
    if (fp_isinf(a) && fp_isinf(b)) {
        /* +inf > -inf, but +inf != +inf */
        return b->sign - a->sign;
    }
    /* but not both... */
    if (fp_isinf(a)) {
        return a->sign ? -1 : 1;
    }
    if (fp_isinf(b)) {
        return b->sign ? 1 : -1;
    }
    if (fp_iszero(a) && fp_iszero(b)) {
        return 0;
    }
    if (fp_iszero(a)) {
        return b->sign ? 1 : -1;
    }
    if (fp_iszero(b)) {
        return a->sign ? -1 : 1;
    }
    /* now both are "normal" */
    if (a->sign != b->sign) {
        /* opposite signs */
        return a->sign ? -1 : 1;
    }
    /* same sign; exponents? */
    if (a->normal_exp > b->normal_exp) {
        return a->sign ? -1 : 1;
    }
    if (a->normal_exp < b->normal_exp) {
        return a->sign ? 1 : -1;
    }
    /* same exponents; check size */
    if (a->fraction.ll > b->fraction.ll) {
        return a->sign ? -1 : 1;
    }
    if (a->fraction.ll < b->fraction.ll) {
        return a->sign ? 1 : -1;
    }
    /* after all that, they're equal */
    return 0;
}

// FUNC 0x0027AD08 fpcmp
//   fp-bit `compare' (= __cmpsf2): unpack_f both, return __fpcmp_parts.
//   No callers.
int
fpcmp(FLO_type arg_a, FLO_type arg_b)
{
    fp_number_type a;
    fp_number_type b;

    unpack_f(FLO_ARG(arg_a), &a);
    unpack_f(FLO_ARG(arg_b), &b);

    return __fpcmp_parts_f(&a, &b);
}

// FUNC 0x0027AD58 sitofp
//   fp-bit `si_to_float' (= __floatsisf): 0 -> ZERO; INT_MIN -> constant
//   0xCF000000 (-2^31f) loaded straight into $f0; else |i| with normal_exp 30
//   shifted left until bit 30 is set.  No callers.
FLO_type
sitofp(SItype arg_a)
{
    fp_number_type in;

    in.class = CLASS_NUMBER;
    in.sign = arg_a < 0;
    if (!arg_a) {
        in.class = CLASS_ZERO;
    } else {
        in.normal_exp = FRACBITS_F + NGARDS_F;
        if (in.sign) {
            /* Special case for minint, since there is no +ve integer
               representation for it */
            if (arg_a == (SItype)0x80000000) {
                return -2147483648.0;
            }
            in.fraction.ll = (-arg_a);
        } else
            in.fraction.ll = arg_a;

        while (in.fraction.ll < (1LL << (FRACBITS_F + NGARDS_F))) {
            in.fraction.ll <<= 1;
            in.normal_exp -= 1;
        }
    }
    return pack_f(&in);
}

// FUNC 0x0027AE10 fptosi
//   fp-bit `float_to_si' (= __fixsfsi): zero/NaN/|x|<1 -> 0; inf or exp > 30
//   -> 0x7FFFFFFF / 0x80000000 by sign; else fraction >> (30 - exp), negated
//   if negative.  No callers.
SItype
fptosi(FLO_type arg_a)
{
    fp_number_type a;
    SItype tmp;

    unpack_f(FLO_ARG(arg_a), &a);
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
    tmp = a.fraction.ll >> ((FRACBITS_F + NGARDS_F) - a.normal_exp);
    return a.sign ? (-tmp) : (tmp);
}

// FUNC 0x0027AEA0 fptoui
//   fp-bit `float_to_usi' (= __fixunssfsi, US_SOFTWARE_GOFAST only):
//   zero/NaN/negative/|x|<1 -> 0 (the sign IS tested); inf or exp > 31 ->
//   0xFFFFFFFF; exp == 31 -> fraction << 1; else fraction >> (30 - exp).
//   8 callers in the game (float -> unsigned casts).
USItype
fptoui(FLO_type arg_a)
{
    fp_number_type a;

    unpack_f(FLO_ARG(arg_a), &a);
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
    else if (a.normal_exp > (FRACBITS_F + NGARDS_F))
        return a.fraction.ll << (a.normal_exp - (FRACBITS_F + NGARDS_F));
    else
        return a.fraction.ll >> ((FRACBITS_F + NGARDS_F) - a.normal_exp);
}

// FUNC 0x0027AF38 __negsf2
//   fp-bit `negate': unpack_f, flip_sign, pack_f.  No callers.
FLO_type
__negsf2(FLO_type arg_a)
{
    fp_number_type a;

    unpack_f(FLO_ARG(arg_a), &a);
    flip_sign(&a);
    return pack_f(&a);
}

// FUNC 0x0027AF70 __make_fp
//   Builds unpacked float parts {class, sign, exp, frac} on the stack and
//   packs them (result in $f0).  Only caller: dptofp (0x00279C50).
SFtype
__make_fp(fp_class_type class,
          unsigned int sign,
          int exp,
          USItype frac)
{
    fp_number_type in;

    in.class = class;
    in.sign = sign;
    in.normal_exp = exp;
    in.fraction.ll = frac;
    return pack_f(&in);
}

// FUNC 0x0027AFA0 fptodp
//   fp-bit `sf_to_df' (= __extendsfdf2): float ($f12) -> double: unpack_f and
//   __make_dp (0x00279C20) with the fraction widened by F_D_BITOFF
//   (zero-extend, << 32, >> 2).  44 callers: every float passed through
//   varargs (printf %f).  Forwards unwritten normal_exp/fraction for
//   ZERO/INF (ignored by pack_d).
DFtype
fptodp(SFtype arg_a)
{
    fp_number_type in;

    unpack_f(FLO_ARG(arg_a), &in);
    return __make_dp(in.class, in.sign, in.normal_exp,
                     ((UDItype)in.fraction.ll) << F_D_BITOFF);
}
