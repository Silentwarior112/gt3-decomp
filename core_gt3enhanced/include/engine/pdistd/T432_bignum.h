/* T432_bignum.h - byte-array numbers (translation unit T432, unit u099).
 *
 * Numbers stored as byte arrays, least significant byte first: shift, add,
 * subtract, compare, long division, bit fields inside a byte buffer, and the
 * conversion of a number to decimal digits.  T431 builds the ranking codes
 * with bits_put and bignum_to_decimal.  ? names (u099). */
#ifndef GAME_T432_BIGNUM_H
#define GAME_T432_BIGNUM_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

/* A number of len bytes, least significant byte first.  The functions that
 * take two numbers mostly assume the same length. */
struct BigNum {
    u8  *data;                              /* 0x0 */
    u32  len;                               /* 0x4 bytes */
};
EGCS_SIZE(BigNum, 0x8);

u32     bignum_to_u32(const BigNum *n);                         /* 0x0021B790 the first 4 bytes */
BigNum *bignum_shl1(BigNum *n);                                 /* 0x0021B7D0 n <<= 1 */
BigNum *bignum_shr1(BigNum *n);                                 /* 0x0021B828 n >>= 1 */
BigNum *bignum_add(BigNum *a, const BigNum *b);                 /* 0x0021B878 a += b */
BigNum *bignum_sub(BigNum *a, const BigNum *b);                 /* 0x0021B8D0 a -= b */
int     bignum_cmp(const BigNum *a, const BigNum *b);           /* 0x0021B940 -1, 0, 1 */
int     bignum_equal(const BigNum *a, const BigNum *b);         /* 0x0021B9D8 */
int     bignum_not_equal(const BigNum *a, const BigNum *b);     /* 0x0021BA58 */
BigNum *bignum_divmod(BigNum *q, const BigNum *a, const BigNum *b,
                      BigNum *r);                               /* 0x0021BAD8 q = a / b, r = a % b */
int     bignum_is_zero(const BigNum *n);                        /* 0x0021BCB0 */
u32     bits_get8(const u8 *buf, u32 bit);                      /* 0x0021BD00 the 8 bits at bit */
u32     bits_get(const u8 *buf, u32 bit, u32 nbits);            /* 0x0021BD60 */
void    bits_put8(u8 *buf, u32 bit, u8 byte);                   /* 0x0021BE10 */
void    bits_put(u8 *buf, u32 bit, u32 value, u32 nbits);       /* 0x0021BE88 value's low nbits at bit */
u32     bignum_div10(BigNum *n);                                /* 0x0021BFB0 n /= 10, returns the remainder */
void    bytes_reverse(char *lo, char *hi);                      /* 0x0021C080 lo..hi inclusive, in place */
/* The decimal digits of the len-byte number data into out (no terminator),
 * most significant first; the number written.  fill >= 0: left-padded with
 * fill to outsize. */
u32     bignum_to_decimal(char *out, u32 outsize, const u8 *data, u32 len,
                          int fill);                            /* 0x0021C0B8 */
void    F_0021C1E0(void);                                       /* 0x0021C1E0 empty, no callers */
int     bytes_equal(const u8 *a, u32 alen, const u8 *b, u32 blen); /* 0x0021C1E8 no callers */

#endif /* __cplusplus */

#endif /* GAME_T432_BIGNUM_H */
