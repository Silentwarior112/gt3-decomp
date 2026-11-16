/* u138 / G007: newlib stdlib/strtod.c - David Gay's correctly rounded
 * strtod (IEEE_8087, no Sudden_Underflow, FLT_ROUNDS 1).  The Bigint
 * arithmetic (Balloc, s2b, d2b, mult, diff, ratio, ulp, ...) is mprec.c, in
 * u139; tens / bigtens / tinytens are its tables.
 *
 * Types as the code has them: e and L are `long` (64-bit: sd/ld, and
 * L = aadj is __fixdfdi), y is a 32-bit ULong, z an `unsigned long` (64-bit:
 * rv = tens[k - 9] * rv + z converts it with __floatdidf and the unsigned
 * fix-up).  e1 and the bigint exponents are int.
 *
 * Negation: egcs' soft-float `-x` is a subtraction 0.0 - x (dpsub(0, x)),
 * not a sign flip, so it is written that way: strtod("-0") returns +0.0. */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

/* mprec.h #defines Balloc, Bfree, s2b, i2b, mult, pow5mult, lshift, cmp,
 * diff, ulp, d2b, ratio and tens, bigtens, tinytens to the link names; the
 * code below spells the link names (_Balloc, __s2b, __mprec_tens, ...). */
#define HUGE_VAL    (__infinity[0].d)

/* Bcopy(x, y): memcpy of _sign, _wds and the words */
#define Bcopy(x, y) memcpy((char *)&x->_sign, (char *)&y->_sign, \
                           y->_wds * sizeof(u32) + 2 * sizeof(int))

union double_words {
    double d;
    u32 w[2];   /* little-endian: w[1] is the sign / exponent word */
};
#define word0(x) ((x).w[1])
#define word1(x) ((x).w[0])

#define Exp_msk1    0x100000
#define Exp_mask    0x7ff00000
#define P           53
#define Bias        1023
#define Emin        (-1022)
#define Ten_pmax    22
#define Bndry_mask  0xfffff
#define Bndry_mask1 0xfffff
#define LSB         1
#define Log2P       1
#define Tiny0       0
#define Tiny1       1
#define n_bigtens   5
#define Big0        0x7fefffff
#define Big1        0xffffffff
#define DBL_DIG_    15
#define DBL_MAX_EXP_ 1024
#define DBL_MAX_10_EXP_ 308
#define FLT_RADIX_  2

/* Matching: exact under the reference compiler 991111-01 (with the file's
 * label_frags rule, ref/compile_flags.json).  991111b gives the same 953
 * instructions but a frame 0x20 smaller (retail's three spill slots at
 * sp+0x60..0x68 land at sp+0x44..0x4C): a build difference of the compiler's
 * reload, as u139's __ulp / __b2d (ref/matching/u138/notes.md). */
// FUNC 0x00282BF0 _strtod_r
double _strtod_r(struct _reent *ptr, const char *s00, char **se)
{
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign, e1, esign, i, j, k,
        nd, nd0, nf, nz, nz0, sign;
    s64 e;              /* long */
    const char *s, *s0, *s1;
    double aadj, aadj1, adj;
    union double_words rv, rv0;
    s64 L;              /* long */
    u32 y;              /* ULong */
    u64 z;              /* unsigned long */
    _Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;

    /* implicit: Soft-float: the double arithmetic below is fp-bit's dpadd, dpsub,
     * dpmul, dpdiv and dpcmp; (double)int is litodp, (int)double dptoli,
     * (double)z __floatdidf and L = aadj __fixdfdi.  Bcopy() is a memcpy. */
    sign = nz0 = nz = 0;
    rv.d = 0.;
    for (s = s00;; s++)
        switch (*s) {
        case '-':
            sign = 1;
            /* no break */
        case '+':
            /* a sign then NUL: as an empty string (case 0).  Retail tests
             * `bnez -> break2' and falls into `s = s00; b ret': the jump is
             * not inverted around a `goto ret0' (the old `== '\0'' form). */
            if (*++s != '\0')
                goto break2;
            /* no break */
        case 0:
            s = s00;
            goto ret;
        default:
            goto break2;
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
        case ' ':
            continue;
        }
break2:
    if (*s == '0') {
        nz0 = 1;
        while (*++s == '0')
            ;
        if (!*s)
            goto ret;
    }
    s0 = s;
    y = z = 0;
    for (nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
        if (nd < 9)
            y = 10 * y + c - '0';
        else if (nd < 16)
            z = 10 * z + c - '0';
    nd0 = nd;
    if (c == '.') {
        c = *++s;
        if (!nd) {
            for (; c == '0'; c = *++s)
                nz++;
            if (c > '0' && c <= '9') {
                s0 = s;
                nf += nz;
                nz = 0;
                goto have_dig;
            }
            goto dig_done;
        }
        for (; c >= '0' && c <= '9'; c = *++s) {
have_dig:
            nz++;
            if (c -= '0') {
                nf += nz;
                for (i = 1; i < nz; i++)
                    if (nd++ < 9)
                        y *= 10;
                    else if (nd <= DBL_DIG_ + 1)
                        z *= 10;
                if (nd++ < 9)
                    y = 10 * y + c;
                else if (nd <= DBL_DIG_ + 1)
                    z = 10 * z + c;
                nz = 0;
            }
        }
    }
dig_done:
    e = 0;
    if (c == 'e' || c == 'E') {
        if (!nd && !nz && !nz0) {
            s = s00;
            goto ret;
        }
        s00 = s;
        esign = 0;
        switch (c = *++s) {
        case '-':
            esign = 1;
            /* no break */
        case '+':
            c = *++s;
        }
        if (c >= '0' && c <= '9') {
            while (c == '0')
                c = *++s;
            if (c > '0' && c <= '9') {
                e = c - '0';
                s1 = s;
                while ((c = *++s) >= '0' && c <= '9')
                    e = 10 * e + c - '0';
                if (s - s1 > 8)
                    /* Avoid confusion from exponents so large that e might
                     * overflow. */
                    e = 9999999L;
                if (esign)
                    e = -e;
            } else
                e = 0;
        } else
            s = s00;
    }
    if (!nd) {
        if (!nz && !nz0)
            s = s00;
        goto ret;
    }
    e1 = e -= nf;

    /* Now we have nd0 digits, starting at s0, followed by a decimal point,
     * followed by nd-nd0 digits.  The number we're after is the integer
     * represented by those digits times 10**e */

    if (!nd0)
        nd0 = nd;
    k = nd < DBL_DIG_ + 1 ? nd : DBL_DIG_ + 1;
    rv.d = y;
    if (k > 9)
        rv.d = __mprec_tens[k - 9] * rv.d + z;
    bd0 = 0;
    if (nd <= DBL_DIG_) {
        if (!e)
            goto ret;
        if (e > 0) {
            if (e <= Ten_pmax) {
                /* rv = */ rv.d *= __mprec_tens[e];     /* rounded_product */
                goto ret;
            }
            i = DBL_DIG_ - nd;
            if (e <= Ten_pmax + i) {
                /* A fancier test would sometimes let us do this for larger
                 * i values. */
                e -= i;
                rv.d *= __mprec_tens[i];
                /* rv = */ rv.d *= __mprec_tens[e];     /* rounded_product */
                goto ret;
            }
        } else if (e >= -Ten_pmax) {
            /* rv = */ rv.d /= __mprec_tens[-e];        /* rounded_quotient */
            goto ret;
        }
    }
    e1 += nd - k;

    /* Get starting approximation = rv * 10**e1 */

    if (e1 > 0) {
        if ((i = e1 & 15))
            rv.d *= __mprec_tens[i];
        if (e1 &= ~15) {
            if (e1 > DBL_MAX_10_EXP_) {
ovfl:
                ptr->_errno = ERANGE;
                rv.d = HUGE_VAL;
                if (bd0)
                    goto retfree;
                goto ret;
            }
            if (e1 >>= 4) {
                for (j = 0; e1 > 1; j++, e1 >>= 1)
                    if (e1 & 1)
                        rv.d *= __mprec_bigtens[j];
                /* The last multiplication could overflow. */
                word0(rv) -= P * Exp_msk1;
                rv.d *= __mprec_bigtens[j];
                if ((z = word0(rv) & Exp_mask) > Exp_msk1 * (DBL_MAX_EXP_ + Bias - P))
                    goto ovfl;
                if (z > Exp_msk1 * (DBL_MAX_EXP_ + Bias - 1 - P)) {
                    /* set to largest number (Can't trust DBL_MAX) */
                    word0(rv) = Big0;
                    word1(rv) = Big1;
                } else
                    word0(rv) += P * Exp_msk1;
            }
        }
    } else if (e1 < 0) {
        e1 = -e1;
        if ((i = e1 & 15))
            rv.d /= __mprec_tens[i];
        if (e1 &= ~15) {
            e1 >>= 4;
            if (e1 >= 1 << n_bigtens)
                goto undfl;
            for (j = 0; e1 > 1; j++, e1 >>= 1)
                if (e1 & 1)
                    rv.d *= __mprec_tinytens[j];
            /* The last multiplication could underflow. */
            rv0.d = rv.d;
            rv.d *= __mprec_tinytens[j];
            if (!rv.d) {
                rv.d = 2. * rv0.d;      /* (rv0 + rv0) */
                rv.d *= __mprec_tinytens[j];
                if (!rv.d) {
undfl:
                    rv.d = 0.;
                    ptr->_errno = ERANGE;
                    if (bd0)
                        goto retfree;
                    goto ret;
                }
                word0(rv) = Tiny0;
                word1(rv) = Tiny1;
                /* The refinement below will clean this approximation up. */
            }
        }
    }

    /* Now the hard part -- adjusting rv to the correct value.*/

    /* Put digits into bd: true value = bd * 10^e */

    bd0 = __s2b(ptr, s0, nd0, nd, y);

    for (;;) {
        bd = _Balloc(ptr, bd0->_k);
        Bcopy(bd, bd0);
        bb = __d2b(ptr, rv.d, &bbe, &bbbits);    /* rv = bb * 2^bbe */
        bs = __i2b(ptr, 1);

        if (e >= 0) {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        } else {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
            bb2 += bbe;
        else
            bd2 -= bbe;
        bs2 = bb2;
        i = bbe + bbbits - 1;   /* logb(rv) */
        if (i < Emin)           /* denormal */
            j = bbe + (P - Emin);
        else
            j = P + 1 - bbbits;
        bb2 += j;
        bd2 += j;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
            i = bs2;
        if (i > 0) {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0) {
            bs = __pow5mult(ptr, bs, bb5);
            bb1 = __multiply(ptr, bs, bb);
            _Bfree(ptr, bb);
            bb = bb1;
        }
        if (bb2 > 0)
            bb = __lshift(ptr, bb, bb2);
        if (bd5 > 0)
            bd = __pow5mult(ptr, bd, bd5);
        if (bd2 > 0)
            bd = __lshift(ptr, bd, bd2);
        if (bs2 > 0)
            bs = __lshift(ptr, bs, bs2);
        delta = __mdiff(ptr, bb, bd);
        dsign = delta->_sign;
        delta->_sign = 0;
        i = __mcmp(delta, bs);
        if (i < 0) {
            /* Error is less than half an ulp -- check for special case of
             * mantissa a power of two. */
            if (dsign || word1(rv) || word0(rv) & Bndry_mask)
                break;
            delta = __lshift(ptr, delta, Log2P);
            if (__mcmp(delta, bs) > 0)
                goto drop_down;
            break;
        }
        if (i == 0) {
            /* exactly half-way between */
            if (dsign) {
                if ((word0(rv) & Bndry_mask1) == Bndry_mask1 && word1(rv) == 0xffffffff) {
                    /*boundary case -- increment exponent*/
                    word0(rv) = (word0(rv) & Exp_mask) + Exp_msk1;
                    word1(rv) = 0;
                    break;
                }
            } else if (!(word0(rv) & Bndry_mask) && !word1(rv)) {
drop_down:
                /* boundary case -- decrement exponent */
                L = (word0(rv) & Exp_mask) - Exp_msk1;
                word0(rv) = L | Bndry_mask1;
                word1(rv) = 0xffffffff;
                break;
            }
            if (!(word1(rv) & LSB))
                break;
            if (dsign)
                rv.d += __ulp(rv.d);
            else {
                rv.d -= __ulp(rv.d);
                if (!rv.d)
                    goto undfl;
            }
            break;
        }
        if ((aadj = __ratio(delta, bs)) <= 2.) {
            if (dsign)
                aadj = aadj1 = 1.;
            else if (word1(rv) || word0(rv) & Bndry_mask) {
                if (word1(rv) == Tiny1 && !word0(rv))
                    goto undfl;
                aadj = 1.;
                aadj1 = -1.;
            } else {
                /* special case -- power of FLT_RADIX to be rounded down... */
                if (aadj < 2. / FLT_RADIX_)
                    aadj = 1. / FLT_RADIX_;
                else
                    aadj *= 0.5;
                aadj1 = 0.0 - aadj;     /* -aadj */
            }
        } else {
            aadj *= 0.5;
            aadj1 = dsign ? aadj : 0.0 - aadj;  /* -aadj */
        }
        y = word0(rv) & Exp_mask;

        /* Check for overflow */

        if (y == Exp_msk1 * (DBL_MAX_EXP_ + Bias - 1)) {
            rv0.d = rv.d;
            word0(rv) -= P * Exp_msk1;
            adj = aadj1 * __ulp(rv.d);
            rv.d += adj;
            if ((word0(rv) & Exp_mask) >= Exp_msk1 * (DBL_MAX_EXP_ + Bias - P)) {
                if (word0(rv0) == Big0 && word1(rv0) == Big1)
                    goto ovfl;
                word0(rv) = Big0;
                word1(rv) = Big1;
                goto cont;
            } else
                word0(rv) += P * Exp_msk1;
        } else {
            if (y <= (P - 1) * Exp_msk1 && aadj >= 1.) {
                aadj1 = (double)(int)(aadj + 0.5);
                if (!dsign)
                    aadj1 = 0.0 - aadj1;    /* -aadj1 */
            }
            adj = aadj1 * __ulp(rv.d);
            rv.d += adj;
        }
        z = word0(rv) & Exp_mask;
        if (y == z) {
            /* Can we stop now? */
            L = aadj;
            aadj -= L;
            /* The tolerances below are conservative. */
            if (dsign || word1(rv) || word0(rv) & Bndry_mask) {
                if (aadj < .4999999 || aadj > .5000001)
                    break;
            } else if (aadj < .4999999 / FLT_RADIX_)
                break;
        }
cont:
        _Bfree(ptr, bb);
        _Bfree(ptr, bd);
        _Bfree(ptr, bs);
        _Bfree(ptr, delta);
    }
retfree:
    _Bfree(ptr, bb);
    _Bfree(ptr, bd);
    _Bfree(ptr, bs);
    _Bfree(ptr, bd0);
    _Bfree(ptr, delta);
    /* no ret0: retail falls from retfree into ret; case 0 and the exponent
     * test's `s = s00; goto ret' are one block (0x00282EDC, cross-jumped) */
ret:
    if (se)
        *se = (char *)s;
    return sign ? 0.0 - rv.d : rv.d;    /* -rv */
}

// FUNC 0x00283AD8 strtod
double strtod(const char *s00, char **se)
{
    return _strtod_r(_impure_ptr, s00, se);
}

// FUNC 0x00283B08 strtodf
float strtodf(const char *s00, char **se)
{
    return strtod(s00, se);     /* implicit: double -> float: dptofp (__truncdfsf2) */
}
