/* gzip_montgomery_tail.cpp - the three functions at .text 0x0100B238..0x0100B2E4.
 *
 * NOT gzip code.  The unit split filed this range under "gzip" because the
 * mapping pass matched F_0100B238 to Lupo 2's GZIP::InflateBase::inflate(void*,
 * const void*, unsigned, bool) by shape (4 args, three calls).  The code says
 * otherwise:
 *   - F_0100B238's callees are the Montgomery context ctor (0x01009128),
 *     Montgomery::powmod (0x0100A5D0) and ~Montgomery (0x01008FC0), and its only
 *     caller, ImageLoader::verify, passes four BigNum handles (ret, signature,
 *     exponent 66001, in-file modulus);
 *   - the .eh_frame FDEs under the CIE at .data 0x0103445C (FDE records
 *     0x01034478..0x01034614, zero terminator 0x01034624) list, in order,
 *     0x01008FC0, 0x01009128, 0x01009F68, 0x0100A5D0, 0x0100B238, 0x0100B2C8,
 *     0x0100B2E0: one C++-with-EH object file, the Montgomery/RSA one;
 *   - the real GZIP::InflateBase::inflate / setOutputData / heap_clear are at
 *     0x01012E98 / 0x01012E88 / 0x01012E80 (src/gzip_inflate.cpp).
 * So this is the tail of the Montgomery object file (0x01008FC0..0x0100B2E8,
 * the rest is in the montgomery unit): the RSA modexp driver plus two
 * out-of-line weak copies of inline functions that GCC 2.9x emitted at the end
 * of the file (their first copies are BigNum::swap 0x01007128 and
 * bignum_lock_stub 0x01004B00).
 */
#include "types.h"
#include "bignum.h"
#include "montgomery.h"   /* class Montgomery (0x34 bytes), modpow declaration */

// FUNC 0x0100B238 modpow
/* RSA public-key operation driver: returns base^exp mod `mod`.
 *   Montgomery ctx(mod);           // F_01009128(&ctx, &mod), ctx on the stack @sp+0
 *   return ctx.powmod(base, exp);  // F_0100A5D0(&ret, &ctx, &base, &exp) (GCC 2.9x
 *                                  //   hidden return pointer first, then this)
 *   // ~Montgomery                 // F_01008FC0(&ctx, 2): in-charge 2 = no delete
 * EH: a throw from powmod runs ~Montgomery(&ctx, 2) then resumes unwinding via
 * __throw (0x01017CA8); a throw out of the ctor resumes unwinding directly; a
 * throw from the destructor during unwinding calls std::terminate (0x010180D8)
 * - plain RAII, no catch handler.
 * Returns the hidden return pointer.  Sole caller: ImageLoader::verify
 * (0x010044A8), as modpow(signature, BigNum(66001), in-file modulus).
 * (Mapping-pass name "GZIP::InflateBase::inflate" was wrong, see file header.) */
BigNum modpow(const BigNum &base, const BigNum &exp, const BigNum &mod)
{
    Montgomery ctx(mod);
    return ctx.powmod(base, exp);
}

// FUNC 0x0100B2C8 BigNum::swap (Montgomery TU-local copy)
/* Byte-identical to BigNum::swap at 0x01007128: exchanges the rep pointers
 * (+4) of two handles, no refcount traffic.  A second out-of-line copy of the
 * inline member emitted at the end of the Montgomery TU; no callers (the
 * Montgomery code calls 0x01007128).  Written as a TU-local function so it does
 * not clash with BigNum::swap.  (Mapping-pass name "swapOutputData" was wrong.) */
static void BigNum_swap_localcopy(BigNum &self, BigNum &other)
{
    BigNumRep *theirs = other.rep;
    BigNumRep *mine = self.rep;
    self.rep = theirs;
    other.rep = mine;
}

// FUNC 0x0100B2E0 bignum_lock_stub_dup_0100B2E0
/* Empty function (`jr ra; nop`), the last FDE of the Montgomery object file
 * (FDE 0x01034614, pc_begin word 0x0103461C, pc_range 8); no callers.
 * Inferred (not provable from the empty body) to be this file's weak
 * out-of-line copy of the refcount hook bignum_lock_stub (0x01004B00, also
 * `jr ra; nop`), which ~Montgomery / the ctor / mulmod / powmod call through
 * the BigNum handles; all calls bind to the first copy.  An 8-byte empty
 * function ends exactly the object files that call the hook; its twins are
 * bignum_lock_stub_dup_01007140 / _010081B0 (bignum unit) and _01008B90
 * (montgomery unit), named the same way.  (Mapping-pass name
 * "GZIP::InflateBase::heap_clear" was wrong: the real heap_clear is 0x01012E80.) */
void bignum_lock_stub_dup_0100B2E0(void *obj, int unlock) /* ? identity inferred */
{
    (void)obj;
    (void)unlock;
}
