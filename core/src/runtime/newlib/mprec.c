/* mprec.c - newlib mprec.o (0x00284178..0x00285197): the Bigint
 * arithmetic shared by strtod and dtoa (David M. Gay's dtoa.c helpers), in
 * newlib's reentrant form (free lists hang off struct _reent).
 *
 * Configuration visible in the code: IEEE_8087 (little-endian doubles),
 * Pack_32 (32-bit words, multiplied as 16-bit halves: no 64-bit ULLong
 * path), no Sudden_Underflow, no DEBUG checks.  The callers are all in the
 * dtoa / strtod code of unit u138 (0x00282BF0).
 *
 * Doubles are soft-float on the EE (64-bit GPR pairs of the dp-bit
 * routines): ratio's division is dpdiv (0x00279728) and _mprec_log10's
 * multiplication dpmul (0x00279480), written here as C double operators.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

/* IEEE double constants (mprec.h). */
#define Exp_shift   20
#define Exp_msk1    0x100000
#define Exp_mask    0x7ff00000
#define P           53
#define Bias        1023
#define Exp_1       0x3ff00000
#define Ebits       11
#define Frac_mask   0xfffff

/* Copy the sign, word count and words of y into x (sizes included): a call
 * of memcpy. */
#define Bcopy(x, y) memcpy((char *)&(x)->_sign, (char *)&(y)->_sign, \
                           (y)->_wds * sizeof(s32) + 2 * sizeof(int))

/* Store two 16-bit halves into *a (little-endian: b = high, c = low), a++. */
#define Storeinc(a, b, c) (((unsigned short *)(a))[1] = (unsigned short)(b), \
                           ((unsigned short *)(a))[0] = (unsigned short)(c), \
                           (a)++)

// FUNC 0x00284178 _Balloc
//   Bigint of 1 << k words.  The free-list array (16 heads) is calloc'ed on
//   first use (NULL if that fails); a list head is reused if present, else a
//   fresh one is _calloc_r'ed (NULL on failure).  _sign and _wds are cleared.
_Bigint *_Balloc(struct _reent *ptr, int k)
{
    int x;
    _Bigint *rv;

    if (ptr->_freelist == NULL) {
        /* Allocate a list of pointers to the mprec objects */
        ptr->_freelist = (struct _Bigint **)_calloc_r(ptr, sizeof(struct _Bigint *), 16);
        if (ptr->_freelist == NULL)
            return NULL;
    }

    if ((rv = ptr->_freelist[k]) != 0) {
        ptr->_freelist[k] = rv->_next;
    } else {
        x = 1 << k;
        /* Allocate an mprec Bigint and stick it in the freelist */
        rv = (_Bigint *)_calloc_r(ptr, 1, sizeof(_Bigint) + (x - 1) * sizeof(rv->_x));
        if (rv == NULL)
            return NULL;
        rv->_k = k;
        rv->_maxwds = x;
    }
    rv->_sign = rv->_wds = 0;
    return rv;
}

// FUNC 0x00284220 _Bfree
//   Push v (if not NULL) on the free list for its size.
void _Bfree(struct _reent *ptr, _Bigint *v)
{
    if (v) {
        v->_next = ptr->_freelist[v->_k];
        ptr->_freelist[v->_k] = v;
    }
}

// FUNC 0x00284250 __multadd
//   b = b * m + a (m, a small).  Grows b (Balloc k+1, Bcopy, Bfree) if the
//   final carry does not fit.  Note: the Balloc result is not checked.
_Bigint *__multadd(struct _reent *ptr, _Bigint *b, int m, int a)
{
    int i, wds;
    u32 *x, y;
    u32 xi, z;
    _Bigint *b1;

    wds = b->_wds;
    x = b->_x;
    i = 0;
    do {
        xi = *x;
        y = (xi & 0xffff) * m + a;
        z = (xi >> 16) * m + (y >> 16);
        a = (int)(z >> 16);
        *x++ = (z << 16) + (y & 0xffff);
    } while (++i < wds);
    if (a) {
        if (wds >= b->_maxwds) {
            b1 = _Balloc(ptr, b->_k + 1);
            Bcopy(b1, b);                       /* implicit: memcpy */
            _Bfree(ptr, b);
            b = b1;
        }
        b->_x[wds++] = a;
        b->_wds = wds;
    }
    return b;
}

// FUNC 0x00284368 __s2b
//   Bigint from the decimal digit string s (nd digits, nd0 before the
//   decimal point, which is skipped), the first 9 digits already folded
//   into y9.
_Bigint *__s2b(struct _reent *ptr, const char *s, int nd0, int nd, u32 y9)
{
    _Bigint *b;
    int i, k;
    s32 x, y;

    x = (nd + 8) / 9;
    for (k = 0, y = 1; x > y; y <<= 1, k++)
        ;
    b = _Balloc(ptr, k);
    b->_x[0] = y9;
    b->_wds = 1;

    i = 9;
    if (9 < nd0) {
        s += 9;
        do
            b = __multadd(ptr, b, 10, *s++ - '0');
        while (++i < nd0);
        s++;
    } else
        s += 10;
    for (; i < nd; i++)
        b = __multadd(ptr, b, 10, *s++ - '0');
    return b;
}

// FUNC 0x002844B0 __hi0bits
//   Number of leading zero bits of x (32 for x == 0).
int __hi0bits(u32 x)
{
    int k = 0;

    if (!(x & 0xffff0000)) {
        k = 16;
        x <<= 16;
    }
    if (!(x & 0xff000000)) {
        k += 8;
        x <<= 8;
    }
    if (!(x & 0xf0000000)) {
        k += 4;
        x <<= 4;
    }
    if (!(x & 0xc0000000)) {
        k += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000)) {
        k++;
        if (!(x & 0x40000000))
            return 32;
    }
    return k;
}

// FUNC 0x00284538 __lo0bits
//   Shift *y right past its trailing zero bits and return their count;
//   32 (and *y unchanged) for 0.
int __lo0bits(u32 *y)
{
    int k;
    u32 x = *y;

    if (x & 7) {
        if (x & 1)
            return 0;
        if (x & 2) {
            *y = x >> 1;
            return 1;
        }
        *y = x >> 2;
        return 2;
    }
    k = 0;
    if (!(x & 0xffff)) {
        k = 16;
        x >>= 16;
    }
    if (!(x & 0xff)) {
        k += 8;
        x >>= 8;
    }
    if (!(x & 0xf)) {
        k += 4;
        x >>= 4;
    }
    if (!(x & 0x3)) {
        k += 2;
        x >>= 2;
    }
    if (!(x & 1)) {
        k++;
        x >>= 1;
        if (!x)                 /* the original reads `if (!x & 1)` */
            return 32;
    }
    *y = x;
    return k;
}

// FUNC 0x002845F8 __i2b
//   One-word Bigint holding i (Balloc result not checked).
_Bigint *__i2b(struct _reent *ptr, int i)
{
    _Bigint *b;

    b = _Balloc(ptr, 1);
    b->_x[0] = i;
    b->_wds = 1;
    return b;
}

// FUNC 0x00284630 __multiply
//   c = a * b, schoolbook, 16 bits of b at a time.
_Bigint *__multiply(struct _reent *ptr, _Bigint *a, _Bigint *b)
{
    _Bigint *c;
    int k, wa, wb, wc;
    u32 carry, y, z;
    u32 *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    u32 z2;

    if (a->_wds < b->_wds) {
        c = a;
        a = b;
        b = c;
    }
    k = a->_k;
    wa = a->_wds;
    wb = b->_wds;
    wc = wa + wb;
    if (wc > a->_maxwds)
        k++;
    c = _Balloc(ptr, k);
    for (x = c->_x, xa = x + wc; x < xa; x++)
        *x = 0;
    xa = a->_x;
    xae = xa + wa;
    xb = b->_x;
    xbe = xb + wb;
    xc0 = c->_x;
    for (; xb < xbe; xb++, xc0++) {
        if ((y = *xb & 0xffff) != 0) {
            x = xa;
            xc = xc0;
            carry = 0;
            do {
                z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
                carry = z >> 16;
                z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
                carry = z2 >> 16;
                Storeinc(xc, z2, z);
            } while (x < xae);
            *xc = carry;
        }
        if ((y = *xb >> 16) != 0) {
            x = xa;
            xc = xc0;
            carry = 0;
            z2 = *xc;
            do {
                z = (*x & 0xffff) * y + (*xc >> 16) + carry;
                carry = z >> 16;
                Storeinc(xc, z, z2);
                z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
                carry = z2 >> 16;
            } while (x < xae);
            *xc = z2;
        }
    }
    for (xc0 = c->_x, xc = xc0 + wc; wc > 0 && !*--xc; --wc)
        ;
    c->_wds = wc;
    return c;
}

// FUNC 0x00284860 __pow5mult
//   b * 5^k: the k & 3 part with one multadd by p05[] = {5, 25, 125}, the
//   rest by squaring 625 (cached in ptr->_p5s and its _next chain).
_Bigint *__pow5mult(struct _reent *ptr, _Bigint *b, int k)
{
    _Bigint *b1, *p5, *p51;
    int i;
    static const int p05[3] = { 5, 25, 125 };   /* 0x00348FC0 */

    if ((i = k & 3) != 0)
        b = __multadd(ptr, b, p05[i - 1], 0);

    if (!(k >>= 2))
        return b;
    if (!(p5 = ptr->_p5s)) {
        /* first time */
        p5 = ptr->_p5s = __i2b(ptr, 625);
        p5->_next = 0;
    }
    for (;;) {
        if (k & 1) {
            b1 = __multiply(ptr, b, p5);
            _Bfree(ptr, b);
            b = b1;
        }
        if (!(k >>= 1))
            break;
        if (!(p51 = p5->_next)) {
            p51 = p5->_next = __multiply(ptr, p5, p5);
            p51->_next = 0;
        }
        p5 = p51;
    }
    return b;
}

// FUNC 0x00284960 __lshift
//   b << k into a new Bigint; b is freed.
_Bigint *__lshift(struct _reent *ptr, _Bigint *b, int k)
{
    int i, k1, n, n1;
    _Bigint *b1;
    u32 *x, *x1, *xe, z;

    n = k >> 5;
    k1 = b->_k;
    n1 = n + b->_wds + 1;
    for (i = b->_maxwds; n1 > i; i <<= 1)
        k1++;
    b1 = _Balloc(ptr, k1);
    x1 = b1->_x;
    for (i = 0; i < n; i++)
        *x1++ = 0;
    x = b->_x;
    xe = x + b->_wds;
    if (k &= 0x1f) {
        k1 = 32 - k;
        z = 0;
        do {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
        } while (x < xe);
        if ((*x1 = z) != 0)
            ++n1;
    } else
        do
            *x1++ = *x++;
        while (x < xe);
    b1->_wds = n1 - 1;
    _Bfree(ptr, b);
    return b1;
}

// FUNC 0x00284AD8 __mcmp
//   Compare: the word-count difference if non-zero, else -1 / 0 / 1 by the
//   words from the top (unsigned).
int __mcmp(_Bigint *a, _Bigint *b)
{
    u32 *xa, *xa0, *xb, *xb0;
    int i, j;

    i = a->_wds;
    j = b->_wds;
    if (i -= j)
        return i;
    xa0 = a->_x;
    xa = xa0 + j;
    xb0 = b->_x;
    xb = xb0 + j;
    for (;;) {
        if (*--xa != *--xb)
            return *xa < *xb ? -1 : 1;
        if (xa <= xa0)
            break;
    }
    return 0;
}

// FUNC 0x00284B40 __mdiff
//   |a - b| with c->_sign = (a < b).  Equal inputs give a one-word zero.
_Bigint *__mdiff(struct _reent *ptr, _Bigint *a, _Bigint *b)
{
    _Bigint *c;
    int i, wa, wb;
    s32 borrow, y;              /* signed shifts needed here */
    u32 *xa, *xae, *xb, *xbe, *xc;
    s32 z;

    i = __mcmp(a, b);
    if (!i) {
        c = _Balloc(ptr, 0);
        c->_wds = 1;
        c->_x[0] = 0;
        return c;
    }
    if (i < 0) {
        c = a;
        a = b;
        b = c;
        i = 1;
    } else
        i = 0;
    c = _Balloc(ptr, a->_k);
    c->_sign = i;
    wa = a->_wds;
    xa = a->_x;
    xae = xa + wa;
    wb = b->_wds;
    xb = b->_x;
    xbe = xb + wb;
    xc = c->_x;
    borrow = 0;
    do {
        y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
        borrow = y >> 16;
        z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
        borrow = z >> 16;
        Storeinc(xc, z, y);
    } while (xb < xbe);
    while (xa < xae) {
        y = (*xa & 0xffff) + borrow;
        borrow = y >> 16;
        z = (*xa++ >> 16) + borrow;
        borrow = z >> 16;
        Storeinc(xc, z, y);
    }
    while (!*--xc)
        wa--;
    c->_wds = wa;
    return c;
}

// FUNC 0x00284CD0 __ulp
//   The value of one unit in the last place of x (gradual underflow handled).
//   Matching: exact under the reference compiler 991111-01; 991111b lacks
//   retail's empty 16-byte frame (the 0xFFFFFFFF mask pseudos of the word
//   stores into the uninitialised union `a' end up used only by reload's
//   (use ...) insns: 991111-01 leaves them without a hard register and gives
//   each an 8-byte stack slot, 991111b gives them $v0).  newlib's own source.
double __ulp(double _x)
{
    union double_union x, a;
    s32 L;

    x.d = _x;

    L = (word0(x) & Exp_mask) - (P - 1) * Exp_msk1;
    if (L > 0) {
        word0(a) = L;
        word1(a) = 0;
    } else {
        L = -L >> Exp_shift;
        if (L < Exp_shift) {
            word0(a) = 0x80000 >> L;
            word1(a) = 0;
        } else {
            word0(a) = 0;
            L -= Exp_shift;
            word1(a) = L >= 31 ? 1 : 1 << (31 - L);
        }
    }
    return a.d;
}

// FUNC 0x00284D68 __b2d
//   The top 53 bits of a as a double in [1, 2); *e = the number of
//   significant bits of the top word (32 - hi0bits).
//   Matching: exact under the reference compiler 991111-01; 991111b lacks
//   retail's extra 16-byte frame slot at sp+0 (the same cause as __ulp:
//   the union d is only ever written word by word).
double __b2d(_Bigint *a, int *e)
{
    u32 *xa, *xa0, w, y, z;
    int k;
    union double_union d;
#define d0 word0(d)
#define d1 word1(d)

    xa0 = a->_x;
    xa = xa0 + a->_wds;
    y = *--xa;
    k = __hi0bits(y);
    *e = 32 - k;
    if (k < Ebits) {
        d0 = Exp_1 | y >> (Ebits - k);
        w = xa > xa0 ? *--xa : 0;
        d1 = y << ((32 - Ebits) + k) | w >> (Ebits - k);
        goto ret_d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
        d0 = Exp_1 | y << k | z >> (32 - k);
        y = xa > xa0 ? *--xa : 0;
        d1 = z << k | y >> (32 - k);
    } else {
        d0 = Exp_1 | y;
        d1 = z;
    }
ret_d:
#undef d0
#undef d1
    return d.d;
}

// FUNC 0x00284EE8 __d2b
//   Bigint b with d = b * 2^*e, *bits = significant bits; the sign of d is
//   ignored.  Denormals: *e = 1 - Bias - (P-1) + k, *bits from hi0bits.
_Bigint *__d2b(struct _reent *ptr, double _d, int *e, int *bits)
{
    union double_union d;
    _Bigint *b;
    int de, i, k;
    u32 *x, y, z;
#define d0 word0(d)
#define d1 word1(d)

    d.d = _d;

    b = _Balloc(ptr, 1);
    x = b->_x;

    z = d0 & Frac_mask;
    d0 &= 0x7fffffff;           /* clear sign bit, which we ignore */
    if ((de = (int)(d0 >> Exp_shift)) != 0)
        z |= Exp_msk1;
    if ((y = d1) != 0) {
        if ((k = __lo0bits(&y)) != 0) {
            x[0] = y | z << (32 - k);
            z >>= k;
        } else
            x[0] = y;
        i = b->_wds = (x[1] = z) ? 2 : 1;
    } else {
        k = __lo0bits(&z);
        x[0] = z;
        i = b->_wds = 1;
        k += 32;
    }
    if (de) {
        *e = de - Bias - (P - 1) + k;
        *bits = P - k;
    } else {
        *e = de - Bias - (P - 1) + 1 + k;
        *bits = 32 * i - __hi0bits(x[i - 1]);
    }
#undef d0
#undef d1
    return b;
}

// FUNC 0x00285068 __ratio
//   a / b as a double (exponents re-applied to the b2d mantissas, then a
//   dpdiv).
double __ratio(_Bigint *a, _Bigint *b)
{
    union double_union da, db;
    int k, ka, kb;

    da.d = __b2d(a, &ka);
    db.d = __b2d(b, &kb);
    k = ka - kb + 32 * (a->_wds - b->_wds);
    if (k > 0)
        word0(da) += k * Exp_msk1;
    else {
        k = -k;
        word0(db) += k * Exp_msk1;
    }
    return da.d / db.d;         /* implicit: dpdiv (0x00279728) */
}

/* mprec.c's tables (mprec.h: tens / bigtens / tinytens), defined after ratio
 * and before _mprec_log10, as in newlib's mprec.c: mprec.o's .rodata is
 * __pow5mult's p05 (0x00348FC0, written when pow5mult is compiled), 4 bytes
 * of padding, then these three (8-aligned doubles, every value the
 * original's: the literals' correctly rounded doubles).  strtod (u138) reads
 * them more often than mprec's own code, so the range tool would count them
 * to strtod's range; the anchors give mprec.o's .rodata to this file. */
// DATA_START 0x00348FC0
// DATA_START 0x00349098
// DATA_START 0x003490C0
const double __mprec_tens[25] = {                       /* 0x00348FD0 */
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22, 1e23, 1e24
};

const double __mprec_bigtens[5] = { 1e16, 1e32, 1e64, 1e128, 1e256 };          /* 0x00349098 */

const double __mprec_tinytens[5] = { 1e-16, 1e-32, 1e-64, 1e-128, 1e-256 };    /* 0x003490C0 */

// FUNC 0x00285128 _mprec_log10
//   10^dig: __mprec_tens[dig] for dig < 24, else 1.0 multiplied by 10.0 dig
//   times (dpmul).  No callers.
double _mprec_log10(int dig)
{
    double v = 1.0;

    if (dig < 24)
        return __mprec_tens[dig];
    while (dig > 0) {
        v *= 10;                /* implicit: dpmul (0x00279480) */
        dig--;
    }
    return v;
}
