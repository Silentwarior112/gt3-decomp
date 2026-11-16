/* T432 - BigNum (? name), 0x0021B790..0x0021C220: numbers stored as byte
 * arrays, least significant byte first, with shift / add / subtract /
 * compare / long division, bit fields inside a byte buffer, and the
 * conversion of a number to decimal digits.  T431 builds the ranking codes
 * with it (bits_put from RankingCodeBlock::RankingCodeBlock and
 * RankingRecord::encode, bignum_to_decimal from RankingRecord::encode). */
#include "game/data/gcf_internal.h"
#include "engine/pdistd/T432_bignum.h"

/* Caller: bignum_div10.  The value of the first 4 bytes (more bytes shift
 * out of the u32). */
// FUNC 0x0021B790 bignum_to_u32
u32 bignum_to_u32(const BigNum *n)
{
    u32 value = 0;
    int i;

    for (i = n->len - 1; i >= 0; i--)
        value = (value << 8) | n->data[i];
    return value;
}

/* Caller: bignum_divmod.  n <<= 1 (the top bit is lost). */
// FUNC 0x0021B7D0 bignum_shl1
BigNum *bignum_shl1(BigNum *n)
{
    u32 carry = 0;
    u32 i;

    if (n->len == 0)
        return n;
    for (i = 0; i < n->len; i++) {
        u8 byte = n->data[i];
        n->data[i] = carry | (byte << 1);
        carry = byte >> 7;
    }
    return n;
}

/* Caller: bignum_divmod.  n >>= 1. */
// FUNC 0x0021B828 bignum_shr1
BigNum *bignum_shr1(BigNum *n)
{
    int i = n->len - 1;
    u32 carry;

    if (n->len == 0)
        return n;
    carry = 0;
    for (; i >= 0; i--) {
        u8 byte = n->data[i];
        n->data[i] = carry | (byte >> 1);
        carry = (byte << 7) & 0x80;
    }
    return n;
}

/* Caller: bignum_divmod.  a += b over a's length (b must be as long). */
// FUNC 0x0021B878 bignum_add
BigNum *bignum_add(BigNum *a, const BigNum *b)
{
    u32 carry = 0;
    u32 i;

    for (i = 0; i < a->len; i++) {
        u32 sum = carry + a->data[i] + b->data[i];
        a->data[i] = sum;
        carry = sum >> 8;
    }
    return a;
}

/* Caller: bignum_divmod.  a -= b over a's length.  When b's byte plus the
 * borrow wraps to 0 (0xFF + 1) the byte of a is left alone and the borrow
 * carries on: subtracting 0x100 from that byte, which is right. */
// FUNC 0x0021B8D0 bignum_sub
BigNum *bignum_sub(BigNum *a, const BigNum *b)
{
    u32 i = 0;
    u32 borrow = 0;

    for (; i < a->len; i++) {
        u32 sub = (b->data[i] + borrow) & 0xFF;
        u8 old;
        u8 diff;

        if (sub < borrow)
            continue;
        old = a->data[i];
        diff = old - sub;
        a->data[i] = diff;
        borrow = old < diff;
    }
    return a;
}

/* Byte i of n, 0 past its length (the shorter number of a pair counts as
 * padded with zero bytes). */
static inline u8 bignum_byte(const BigNum *n, u32 i)
{
    if (i >= n->len)
        return 0;
    return n->data[i];
}

/* Caller: bignum_divmod.  -1 / 0 / 1. */
// FUNC 0x0021B940 bignum_cmp
int bignum_cmp(const BigNum *a, const BigNum *b)
{
    u32 len = a->len < b->len ? b->len : a->len;

    for (int i = len - 1; i >= 0; i--) {
        u8 x = bignum_byte(a, i);
        u8 y = bignum_byte(b, i);
        if (x < y)
            return -1;
        if (y < x)
            return 1;
    }
    return 0;
}

/* No callers.  1 if a == b (zero-padded). */
// FUNC 0x0021B9D8 bignum_equal
int bignum_equal(const BigNum *a, const BigNum *b)
{
    u32 len = a->len < b->len ? b->len : a->len;

    for (u32 i = 0; i < len; i++) {
        if (bignum_byte(a, i) != bignum_byte(b, i))
            return 0;
    }
    return 1;
}

/* No callers.  1 if a != b (zero-padded). */
// FUNC 0x0021BA58 bignum_not_equal
int bignum_not_equal(const BigNum *a, const BigNum *b)
{
    u32 len = a->len < b->len ? b->len : a->len;

    for (u32 i = 0; i < len; i++) {
        if (bignum_byte(a, i) != bignum_byte(b, i))
            return 1;
    }
    return 0;
}

/* Caller: bignum_div10.  q = a / b, r = a % b by shift-and-subtract, with
 * len + 1 byte work copies on the stack (alloca: retail rounds len + 1 up
 * to 16 itself; len = q's length: a and b are copied with q's length too).
 * Returns q. */
// FUNC 0x0021BAD8 bignum_divmod
/* 87.8%: left are the address registers of the loops: retail keeps &power
 * (and in the second loop &rem, &quot) in s-registers and computes &rem
 * afresh for each bignum_cmp; here GCSE keeps &rem (the first loop's test)
 * in one register for the whole function.  Both loops are entered at their
 * test (loop.c calls them phony), so this is GCSE / CSE's choice.  Tried:
 * the order of each block's stores, memset by field or expression, the
 * first loop as for (;;) with a break and with the comparison reversed;
 * the reference compiler gives the same. */
BigNum *bignum_divmod(BigNum *q, const BigNum *a, const BigNum *b, BigNum *r)
{
    u32 len = q->len;
    BigNum divisor;                         /* fp+0x00 b shifted left */
    BigNum power;                           /* fp+0x10 the same shift of 1 */
    BigNum rem;                             /* fp+0x20 the remainder */
    BigNum quot;                            /* fp+0x30 the quotient */

    divisor.len = len + 1;
    divisor.data = (u8 *)__builtin_alloca(len + 1);
    memset(divisor.data, 0, len + 1);
    memcpy(divisor.data, b->data, len);

    power.len = len + 1;
    power.data = (u8 *)__builtin_alloca(len + 1);
    memset(power.data, 0, len + 1);
    power.data[0] = 1;

    rem.data = (u8 *)__builtin_alloca(len + 1);
    rem.len = len + 1;
    memset(rem.data, 0, len + 1);
    memcpy(rem.data, a->data, len);

    quot.data = (u8 *)__builtin_alloca(len + 1);
    quot.len = len + 1;
    memset(quot.data, 0, len + 1);

    while (bignum_cmp(&divisor, &rem) < 0) {
        bignum_shl1(&divisor);
        bignum_shl1(&power);
    }
    while (bignum_cmp(b, &rem) <= 0) {
        while (bignum_cmp(&rem, &divisor) < 0) {
            bignum_shr1(&divisor);
            bignum_shr1(&power);
        }
        bignum_sub(&rem, &divisor);
        bignum_add(&quot, &power);
    }
    memcpy(q->data, quot.data, q->len);
    memcpy(r->data, rem.data, r->len);
    return q;
}

/* Caller: bignum_to_decimal. */
// FUNC 0x0021BCB0 bignum_is_zero
int bignum_is_zero(const BigNum *n)
{
    u32 len = n->len;
    u32 i;

    if (len == 0)
        return 1;
    for (i = 0; i < len; i++) {
        if (n->data[i] != 0)
            return 0;
    }
    return 1;
}

/* Caller: bits_get.  The 8 bits at bit offset `bit' (LSB first). */
// FUNC 0x0021BD00 bits_get8
u32 bits_get8(const u8 *buf, u32 bit)
{
    u32 shift = bit & 7;                    /* the offset inside the first byte */
    u32 index = bit >> 3;

    if (shift == 0)
        return buf[index];
    const u8 *p = buf + index;
    int rest = 8 - shift;                   /* the bits taken from the first byte */
    u8 lo = 0xFF << shift;                  /* the masks as bytes: retail's andi 0xFF */
    u8 hi = 0xFFu >> rest;
    return (((p[0] & lo) >> shift) | ((p[1] & hi) << rest)) & 0xFF;
}

/* No callers.  nbits bits (up to 32) at bit offset `bit', LSB first. */
// FUNC 0x0021BD60 bits_get
u32 bits_get(const u8 *buf, u32 bit, u32 nbits)
{
    u32 value = 0;
    int shift = 0;

    while (nbits >= 8) {
        nbits -= 8;
        value |= bits_get8(buf, bit) << shift;
        bit += 8;
        shift += 8;
    }
    if (nbits != 0)
        value |= (bits_get8(buf, bit) & ((1 << nbits) - 1) & 0xFF) << shift;
    return value;
}

/* Caller: bits_put.  Stores 8 bits at bit offset `bit'. */
// FUNC 0x0021BE10 bits_put8
void bits_put8(u8 *buf, u32 bit, u8 byte)
{
    u32 shift = bit & 7;
    u32 index = bit >> 3;

    if (shift == 0) {
        buf[index] = byte;
        return;
    }
    u8 *p = buf + index;
    int rest = 8 - shift;
    u8 lo = 0xFF << shift;                  /* the bits of the first byte */
    u8 hi = 0xFFu >> rest;                  /* the bits of the second */
    p[0] = (byte << shift) | (p[0] & ~lo);
    p[1] = (byte >> rest) | (p[1] & ~hi);
}

/* Callers: RankingCodeBlock::RankingCodeBlock, RankingRecord::encode
 * (T431).  Stores the low nbits bits of value at bit offset `bit', LSB
 * first. */
// FUNC 0x0021BE88 bits_put
/* 90.4%: the masks as bytes, as in bits_put8 (retail's andi 0xFF).  Left:
 * local-alloc gives `rest' $v1 and the high mask $a0 (retail: the other way
 * round, so the index, global, takes $a1).  Tried: every order of the
 * else-branch statements (73 orders), rest / mask / lo / hi as variables or
 * inline (u32, int, u8), the masks through inline helpers, the else branch
 * or the whole tail as an inline function, no index variable; the
 * reference compiler gives the same.  With the masks as inline (u8) casts
 * the registers are retail's, but egcs drops the two andi (96.6%, 71 of 73
 * instructions). */
void bits_put(u8 *buf, u32 bit, u32 value, u32 nbits)
{
    if (nbits < 32)
        value &= (1 << nbits) - 1;
    while (nbits >= 8) {
        bits_put8(buf, bit, value & 0xFF);
        nbits -= 8;
        value >>= 8;
        bit += 8;
    }
    if (nbits != 0) {
        u32 shift = bit & 7;
        u32 index = bit >> 3;

        if (shift == 0) {
            u32 mask = ((1 << nbits) - 1) & 0xFF;
            buf[index] = (buf[index] & ~mask) | (mask & value);
        } else {
            int rest = 8 - shift;
            int mask = ((1 << nbits) - 1) & 0xFF;
            u8 *p = buf + index;
            u8 lo = mask << shift;
            u8 hi = mask >> rest;
            p[0] = (value << shift) | (p[0] & ~lo);
            p[1] = (value >> rest) | (p[1] & ~hi);
        }
    }
}

/* Caller: bignum_to_decimal.  n /= 10, returns the remainder.  The divisor
 * is written as 4 bytes whatever n's length (a length < 4 writes past the
 * copy). */
// FUNC 0x0021BFB0 bignum_div10
u32 bignum_div10(BigNum *n)
{
    u32 divisor = 10;                       /* retail keeps it in $s0 over the memset */
    u32 len = n->len;
    BigNum dividend;                        /* fp+0x00 */
    BigNum ten;                             /* fp+0x10 */
    BigNum rem;                             /* fp+0x20 */

    dividend.len = len;
    ten.len = len;
    rem.len = len;
    dividend.data = (u8 *)__builtin_alloca(len);
    ten.data = (u8 *)__builtin_alloca(len);
    rem.data = (u8 *)__builtin_alloca(len);

    memset(ten.data, 0, len);
    ten.data[0] = divisor;
    ten.data[1] = divisor >> 8;
    ten.data[2] = divisor >> 16;
    ten.data[3] = divisor >> 24;
    memcpy(dividend.data, n->data, dividend.len);
    bignum_divmod(n, &dividend, &ten, &rem);
    return bignum_to_u32(&rem);
}

/* Caller: bignum_to_decimal.  Reverses lo..hi (inclusive) in place. */
// FUNC 0x0021C080 bytes_reverse
void bytes_reverse(char *lo, char *hi)
{
    while (lo < hi) {
        char t = *lo;
        *lo = *hi;
        *hi = t;
        lo++;
        hi--;
    }
}

/* Caller: RankingRecord::encode (T431).  Writes the decimal digits of the
 * len-byte number `data' to out (no terminator), most significant first,
 * and returns the number of digits.  The digits go round the outsize-byte
 * buffer: with more digits than outsize the last outsize stay (rotated back
 * into order).  fill >= 0: the digits are left-padded with `fill' to outsize
 * and outsize is returned. */
// FUNC 0x0021C0B8 bignum_to_decimal
u32 bignum_to_decimal(char *out, u32 outsize, const u8 *data, u32 len, int fill)
{
    BigNum num;                             /* fp+0x00 the copy being divided */
    u32 pos;
    u32 digits;

    num.data = (u8 *)__builtin_alloca(len);
    num.len = len;
    memcpy(num.data, data, len);
    pos = 0;
    digits = 0;
    while (!bignum_is_zero(&num)) {
        digits++;
        out[pos] = bignum_div10(&num) + '0';
        pos++;
        if (!(pos < outsize))
            pos = 0;
    }
    if (outsize < digits) {
        bytes_reverse(out, out + pos - 1);
        bytes_reverse(out + pos, out + outsize - 1);
    } else {
        if (fill >= 0) {
            memset(out + digits, fill, outsize - digits);
            bytes_reverse(out, out + outsize - 1);
            digits = outsize;
        } else {
            bytes_reverse(out, out + digits - 1);
        }
    }
    return digits;
}

/* No callers.  Empty. */
// FUNC 0x0021C1E0 F_0021C1E0
void F_0021C1E0(void)
{
}

/* No callers.  1 if the two buffers have the same length and bytes. */
// FUNC 0x0021C1E8 bytes_equal
int bytes_equal(const u8 *a, u32 alen, const u8 *b, u32 blen)
{
    if (alen != blen)
        return 0;
    return memcmp(a, b, alen) == 0;         /* 0x0027D8FC (symbols.csv: memcpy_small) */
}
