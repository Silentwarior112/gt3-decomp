/* montgomery.h - Montgomery modular arithmetic of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), unit "montgomery" (.text 0x010085D0..0x0100B238).
 *
 * The unit also defines five members of the bignum classes, declared in
 * bignum.h (they are called by the bignum unit too):
 *   BigNum::divmod      0x010085D0   handle-level q = a / b, r = a % b
 *   BigNumRep::shr1     0x01008B98   this >>= 1
 *   BigNumRep::shr      0x01008C08   this >>= n
 *   BigNumRep::shl1     0x01008D80   this <<= 1
 *   BigNumRep::shl      0x01008E00   this <<= n
 *
 * The Montgomery context is used only by the RSA driver modpow (0x0100B238,
 * in src/gzip_montgomery_tail.cpp).  ImageLoader::verify (0x010044A8) calls
 * that driver as modpow(signature, BigNum(66001), in-file modulus).
 *
 * Polyphony C++, GCC 2.9x-ee; there are no symbols, so the names are
 * descriptive.  The class has no vtable.
 */
#ifndef BOOT_MONTGOMERY_H
#define BOOT_MONTGOMERY_H

#include "types.h"
#include "bignum.h"

/* ------------------------------------------------------------------------ */
/* Montgomery - REDC context for one odd modulus N (0x34 bytes)               */
/* ------------------------------------------------------------------------ */
/* It is built on the stack only, by modpow at sp+0, with the destructor
 * called as __dt(&ctx, 2).  The six handles are constructed in field order
 * and released in reverse order.
 *
 * src/gzip_montgomery_tail.cpp (modpow 0x0100B238) includes this header
 * and uses this class directly. */
class Montgomery {
public:
    BigNum R;       /* +0x00 2^k, the smallest power of two >= N               */
    BigNum Rinv;    /* +0x08 (R - N)^-1 mod N  ( = R^-1 mod N, since N <= R < 2N ) */
    BigNum N;       /* +0x10 modulus (shares the caller's rep)                 */
    BigNum Nprime;  /* +0x18 R - (N^-1 mod R)  ( = -N^-1 mod R, REDC constant ) */
    BigNum T;       /* +0x20 scratch: x*y (2k bits)                            */
    BigNum m;       /* +0x28 scratch: T*N' mod R (k bits)                      */
    u32    k;       /* +0x30 log2(R): bit count used for the truncations / >> k */

    explicit Montgomery(const BigNum &n);                   /* 0x01009128 */
    ~Montgomery();                                          /* 0x01008FC0 (GCC 2.9x __dt(this, flags)) */

    /* x = x * y * R^-1 mod N (REDC).  x and y must be in [0, N). */
    void   mulmod(BigNum &x, const BigNum &y);              /* 0x01009F68 */

    /* returns base^exp mod N (GCC 2.9x hidden return slot in $a0, this in $a1). */
    BigNum powmod(const BigNum &base, const BigNum &exp);   /* 0x0100A5D0 */
};
static_assert(sizeof(Montgomery) == 0x34, "Montgomery context: fields up to +0x30");

/* 0x0100B238: RSA public-key operation base^exp mod `mod`.  Defined in
 * src/gzip_montgomery_tail.cpp (unit "gzip"), where the mapping pass had
 * filed it: Montgomery ctx(mod); return ctx.powmod(base, exp). */
BigNum modpow(const BigNum &base, const BigNum &exp, const BigNum &mod);

/* 0x01008B90: `jr ra; nop` ending the division object file.  It is an
 * unreferenced weak duplicate of bignum_lock_stub (0x01004B00); the identity
 * is inferred (see src/montgomery.cpp).  Formerly F_01008B90. */
void bignum_lock_stub_dup_01008B90(void *obj, int unlock);

/* 0x0100B2E0: the same `jr ra; nop` ending the Montgomery object file;
 * defined in src/gzip_montgomery_tail.cpp (unit "gzip").  Unreferenced weak
 * duplicate of bignum_lock_stub, identity inferred.  Formerly F_0100B2E0. */
void bignum_lock_stub_dup_0100B2E0(void *obj, int unlock);

#endif /* BOOT_MONTGOMERY_H */
