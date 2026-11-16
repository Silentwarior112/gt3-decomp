/* dp_bit.c - dp-bit.o (gcc config/fp-bit.c compiled for double) of the
 * no-FDE stretch G005, 0x00278FB0..0x00279CA8 of the GT3 US main executable.
 *
 * All 15 functions are the same machine code as the bootstrap's
 * runtime/libgcc_math.c (0x01025068..0x01025D60).  The EE FPU is single
 * precision only, so every double operation in the game (several hundred
 * call sites) is a call into these GOFAST-named routines; doubles travel as
 * 64-bit values in GPRs.
 *
 * The source is gcc 2.9x config/fp-bit.c itself (byte-exact, all 15): built
 * with US_SOFTWARE_GOFAST, FLOAT_BIT_ORDER_MISMATCH (MIPS little-endian:
 * pack_d / unpack_d go through the IEEE bitfield struct) and NO_DENORMALS
 * (unpack_d's `fraction == 0 || 1'; the dead normalising loop still leaves
 * its alignment nop); isnan / isinf / iszero / flip_sign / nan() are its
 * INLINE functions (store-flag tests: `xori; beqz').
 *
 * fp-bit behaviour worth knowing (the first item is the NO_DENORMALS build
 * option; the others are stock fp-bit.c but easy to miss):
 *   - unpack: exponent 0 is always CLASS_ZERO - denormal inputs are flushed
 *     to zero (no normalising loop), fraction/normal_exp left unwritten;
 *   - pack: a NaN always gets the "quiet" bit ORed in;
 *   - multiply's right-normalise loop only shifts `low` when `high` is odd.
 * The static NaN (`thenan`) is the .bss object at 0x004AD228.
 *
 * Symbols of the toolchain's dp-bit.o (nm): fp-bit.c's `#define pack_d
 * __pack_d`, `unpack_d __unpack_d`, `__fpcmp_parts __fpcmp_parts_d` make
 * those three global (T __pack_d, __unpack_d, __fpcmp_parts_d); _fpadd_parts
 * is a local `t`, thenan a local `b thenan.3`; nan(), _fpmul_parts and
 * _fpdiv_parts are inlined.  They are static here: nothing outside the unit
 * calls them.
 */
#include "runtime/libgcc/G005_dp_bit.h"           /* own TU */
#include "runtime/libgcc/libgcc2_part2_internal.h"                       /* fp_number_type_d */

/* fp-bit's IEEE "special" number predicates and nan(), INLINE static
 * functions.  Named *_d / fp_* because isnan() and nan() are libm's.
 * nan()'s `static fp_number_type thenan;` is the object's whole .bss
 * (0x004AD228, 0x18 bytes, 8-aligned; the SDK's dp-bit.o: `b thenan.3'):
 * zero, i.e. {CLASS_SNAN, sign 0, exp 0, fraction 0}, which packs to +qNaN.
 * Used by _fpadd_parts_d, dpmul and dpdiv.  fp-bit.o's float twin is
 * 0x004AD240 (u135). */
static inline fp_number_type_d *
nan_d(void)
{
    static fp_number_type_d thenan;     /* 0x004AD228 (.bss) */

    return &thenan;
}

static inline int
fp_isnan(fp_number_type_d *x)
{
    return x->class == CLASS_SNAN || x->class == CLASS_QNAN;
}

static inline int
fp_isinf(fp_number_type_d *x)
{
    return x->class == CLASS_INFINITY;
}

static inline int
fp_iszero(fp_number_type_d *x)
{
    return x->class == CLASS_ZERO;
}

static inline void
flip_sign(fp_number_type_d *x)
{
    x->sign = !x->sign;
}

/* ======================================================================= */
/*  dp-bit.o - double precision                                            */
/* ======================================================================= */

// FUNC 0x00278FB0 pack_d (dp-bit)
//   (__pack_d in dp-bit.o.)  Unpacked parts -> IEEE double bits (returned in $v0).  NaN (class < 2):
//   exp 0x7FF, fraction | quiet bit; INF: exp 0x7FF, frac 0; ZERO or a zero
//   fraction: exp 0, frac 0 (sign kept); exponent below -1022: denormal
//   shift (>56 -> 0) then >> 8 guard bits; above 1023: infinity; otherwise
//   round to nearest even on the 8 guard bits, renormalise once on carry
//   into bit 61, rebias.  The three bitfield stores are fraction, exp, sign.
static DFtype
pack_d(fp_number_type_d *src)
{
    FLO_union_type_d dst;
    UDItype fraction = src->fraction.ll;    /* wasn't unsigned before? */
    int sign = src->sign;
    int exp = 0;

    if (fp_isnan(src)) {
        exp = EXPMAX_D;
        if (src->class == CLASS_QNAN || 1) {
            fraction |= QUIET_NAN_D;
        }
    } else if (fp_isinf(src)) {
        exp = EXPMAX_D;
        fraction = 0;
    } else if (fp_iszero(src)) {
        exp = 0;
        fraction = 0;
    } else if (fraction == 0) {
        exp = 0;
    } else {
        if (src->normal_exp < NORMAL_EXPMIN_D) {
            /* This number's exponent is too low to fit into the bits
               available in the number, so we'll store 0 in the exponent and
               shift the fraction to the right to make up for it. */
            int shift = NORMAL_EXPMIN_D - src->normal_exp;

            exp = 0;

            if (shift > FRAC_NBITS_D - NGARDS_D) {
                /* No point shifting, since it's more that 64 out. */
                fraction = 0;
            } else {
                /* Shift by the value */
                fraction >>= shift;
            }
            fraction >>= NGARDS_D;
        } else if (src->normal_exp > EXPBIAS_D) {
            exp = EXPMAX_D;
            fraction = 0;
        } else {
            exp = src->normal_exp + EXPBIAS_D;
            /* IF the gard bits are the all zero, but the first, then we're
               half way between two numbers, choose the one which makes the
               lsb of the answer 0. */
            if ((fraction & GARDMASK_D) == GARDMSB_D) {
                if (fraction & (1 << NGARDS_D))
                    fraction += GARDROUND_D + 1;
            } else {
                /* Add a one to the guards to round up */
                fraction += GARDROUND_D;
            }
            if (fraction >= IMPLICIT_2_D) {
                fraction >>= 1;
                exp += 1;
            }
            fraction >>= NGARDS_D;
        }
    }

    /* FLOAT_BIT_ORDER_MISMATCH: the bitfield stores. */
    dst.bits.fraction = fraction;
    dst.bits.exp = exp;
    dst.bits.sign = sign;
    return dst.value;
}

// FUNC 0x002790E0 unpack_d (dp-bit)
//   (__unpack_d in dp-bit.o.)  IEEE double bits (read with one `ld`) -> unpacked parts.  sign always
//   stored; exp 0 -> CLASS_ZERO (denormals flushed: fp-bit.c built with
//   NO_DENORMALS, fraction/normal_exp not written; the dead normalising
//   loop still leaves its alignment nop after the first `jr`); exp 0x7FF ->
//   INFINITY if the fraction is 0, else QNAN/SNAN by bit 51 with the raw
//   fraction kept; otherwise NUMBER with normal_exp = exp - 1023 and
//   fraction = (frac52 << 8) | 1 << 60.
static void
unpack_d(FLO_union_type_d *src, fp_number_type_d *dst)
{
    UDItype fraction;
    int exp;
    int sign;

    /* FLOAT_BIT_ORDER_MISMATCH: the bitfield reads. */
    fraction = src->bits.fraction;
    exp = src->bits.exp;
    sign = src->bits.sign;

    dst->sign = sign;
    if (exp == 0) {
        /* Hmm.  Looks like 0 */
        if (fraction == 0
            || 1        /* NO_DENORMALS */
            ) {
            /* tastes like zero */
            dst->class = CLASS_ZERO;
        } else {
            /* Zero exponent with non zero fraction - it's denormalized,
               so there isn't a leading implicit one - we'll shift it so
               it gets one. */
            dst->normal_exp = exp - EXPBIAS_D + 1;
            fraction <<= NGARDS_D;

            dst->class = CLASS_NUMBER;
            while (fraction < IMPLICIT_1_D) {
                fraction <<= 1;
                dst->normal_exp--;
            }
            dst->fraction.ll = fraction;
        }
    } else if (exp == EXPMAX_D) {
        /* Huge exponent */
        if (fraction == 0) {
            /* Attached to a zero fraction - means infinity */
            dst->class = CLASS_INFINITY;
        } else {
            /* Non zero fraction, means nan */
            if (fraction & QUIET_NAN_D) {
                dst->class = CLASS_QNAN;
            } else {
                dst->class = CLASS_SNAN;
            }
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

// FUNC 0x00279180 _fpadd_parts_d (fp-bit `_fpadd_parts`, dp-bit)
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
            /* fp-bit's own note: "??? This does shifts one bit at a time.
               Optimize." */
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

// FUNC 0x002793C0 dpadd (fp-bit `add`; = __adddf3)
//   Frame: a @sp+0, b @sp+0x20, tmp @sp+0x40, args spilled @sp+0x60/0x68.
//   Callers: __fixunssfdi, __floatdidf, __floatdisf and 23 game functions.
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

// FUNC 0x00279418 dpsub (fp-bit `sub`; = __subdf3)
//   As dpadd with b.sign ^= 1.  Also the compiler's negation (0.0 - x).
//   Callers: __fixunssfdi and 25 game functions.
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
 * __muldi3 (0x00278F48; the R5900 has no dmultu) with zero-extended
 * operands, in the order ml*nl, mh*nl, ml*nh, mh*nh (fp-bit writes them as
 * UDItype `*`, which this compiler turns into the libcall). */
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
        UDItype pp_ll = ml * nl;
        UDItype pp_hl = mh * nl;
        UDItype pp_lh = ml * nh;
        UDItype pp_hh = mh * nh;
        UDItype res2 = 0;
        UDItype res0 = 0;
        UDItype ps_hh__ = pp_hl + pp_lh;
        if (ps_hh__ < pp_hl)
            res2 += 0x100000000LL;
        pp_hl = (ps_hh__ << 32) & 0xffffffff00000000LL;
        res0 = pp_ll + pp_hl;
        if (res0 < pp_ll)
            res2++;
        res2 += ((ps_hh__ >> 32) & 0xffffffffL) + pp_hh;
        high = res2;
        low = res0;
    }

    tmp->normal_exp = a->normal_exp + b->normal_exp;
    tmp->sign = a->sign != b->sign;
    tmp->normal_exp += 4;   /* fp-bit's own note: "???????????????" */
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

// FUNC 0x00279480 dpmul (fp-bit `multiply`; = __muldf3)
//   unpack_d both, inlined _fpmul_parts (NaN/INF/ZERO dispatch, sign = a!=b,
//   inf*0 -> static NaN; the fraction product is four __muldi3 calls),
//   pack_d.  Frame: a @sp+0, b @sp+0x20, tmp @sp+0x40.
//   Callers: __fixunssfdi, __floatdidf, __floatdisf and 24 game functions.
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

    res = _fpmul_parts_d(&a, &b, &tmp);    /* implicit: __muldi3 (inline _fpmul_parts_d) */

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
        /* fp-bit's own note: "??? Does divide one bit at a time.
           Optimize." */
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

// FUNC 0x00279728 dpdiv (fp-bit `divide`; = __divdf3)
//   unpack_d both, inlined _fpdiv_parts, pack_d.  Frame: a @sp+0,
//   b @sp+0x20.  Callers: CameraCurve::init (0x00206330), __ieee754_sqrt
//   (0x002757A0), _strtod_r (0x00282BF0), __ratio (0x00285068),
//   _print_double (0x00287418).
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

// FUNC 0x00279890 __fpcmp_parts_d (fp-bit `__fpcmp_parts`, dp-bit)
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

// FUNC 0x002799A8 dpcmp (fp-bit `compare`; = __cmpdf2)
//   unpack_d both and return __fpcmp_parts: <0, 0, >0 (NaN -> 1).  GOFAST
//   uses it for every double relational operator.  Callers: __fixunssfdi
//   (a < 0.0) and 22 game functions.
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

// FUNC 0x002799F8 litodp (fp-bit `si_to_float`; = __floatsidf)
//   int -> double: 0 -> ZERO; INT_MIN returns the constant -2^31
//   (0xC1E0000000000000) directly; else |i| (sign-extended into the 64-bit
//   fraction) with normal_exp 60, shifted left until bit 60 is set.
//   Callers: __floatdidf, __floatdisf, draw_wave_text (0x0018A960),
//   __ieee754_rem_pio2 (0x00275280), __kernel_rem_pio2 (0x00275CF8),
//   _strtod_r (0x00282BF0).
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

// FUNC 0x00279AB0 dptoli (fp-bit `float_to_si`; = __fixdfsi)
//   double -> int, truncating: zero/NaN/|x|<1 -> 0; inf or exp > 30 ->
//   0x7FFFFFFF / 0x80000000 by sign; else fraction >> (60 - exp), negated
//   for negative input.  8 callers: F_0018ABC8, anim_frame_reset,
//   anim_frame_step, _strtod_r and the fdlibm kernels (__ieee754_rem_pio2,
//   __kernel_rem_pio2, __kernel_cos, __kernel_sin).
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

// FUNC 0x00279B48 dptoul (fp-bit `float_to_usi`; = __fixunsdfsi)
//   double -> unsigned int: zero/NaN/negative/|x|<1 -> 0; inf or exp > 31
//   -> 0xFFFFFFFF; else fraction >> (60 - exp) (the << branch for
//   exp > 60 is unreachable).  Callers: __fixunssfdi, __fixunsdfdi
//   (0x0028D9B8).
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

// FUNC 0x00279BE8 __negdf2 (fp-bit `negate`)
//   unpack_d, sign = !sign, pack_d.  No callers (GOFAST negates by
//   subtracting from 0.0 instead).
DFtype
__negdf2(DFtype arg_a)
{
    fp_number_type_d a;
    FLO_union_type_d ua;

    ua.value = arg_a;
    unpack_d(&ua, &a);
    flip_sign(&a);
    return pack_d(&a);
}

// FUNC 0x00279C20 __make_dp
//   Builds unpacked parts {class, sign, exp, frac} on the stack and packs
//   them.  Only caller: fptodp (0x0027AFA0, float -> double).  fp-bit names
//   the first parameter `class'; G005_dp_bit.h calls it fp_class (C++).
DFtype
__make_dp(fp_class_type fp_class, unsigned int sign, int exp, UDItype frac)
{
    fp_number_type_d in;

    in.class = fp_class;
    in.sign = sign;
    in.normal_exp = exp;
    in.fraction.ll = frac;
    return pack_d(&in);
}

// FUNC 0x00279C50 dptofp (fp-bit `df_to_sf`; = __truncdfsf2)
//   double -> float: keep fraction bits 61..30 ((frac << 2) >> 32) and OR a
//   sticky 1 if any of the 30 dropped bits (mask 0x3FFFFFFF) was set, then
//   __make_fp(class, sign, exp, sffrac).  28 callers.  For ZERO/INF inputs
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
