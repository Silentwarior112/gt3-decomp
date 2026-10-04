/* montgomery.cpp - arbitrary-precision integers, part 2, and Montgomery
 * modular exponentiation (GT3 US bootstrap, BOOTSTRAP_SCUS-97102, .text
 * 0x010085D0..0x0100B238).
 *
 * Object-file layout.  The .eh_frame in .data (0x01033D80..0x01034624) has
 * one CIE per object file, and each file's FDE list ends with a zero
 * terminator.  This splits the bignum code into small files, and it
 * splits this range into four of them:
 *   - 0x010081B8..0x01008B98 division file (CIE 0x0103426C, "eh", except
 *     table 0x01041678): the two division cores (bignum unit), then
 *     BigNum::divmod and the 8-byte bignum_lock_stub_dup_01008B90 (here);
 *   - 0x01008B98..0x01008D80 shr1 / shr (CIE 0x01034390, no EH augmentation);
 *   - 0x01008D80..0x01008FC0 shl1 / shl (CIE 0x010343E8, no EH);
 *   - 0x01008FC0..0x0100B2E8 Montgomery file (CIE 0x0103445C, "eh", except
 *     table 0x01041764): ~Montgomery, the ctor, mulmod and powmod (here),
 *     then modpow, a second copy of BigNum::swap and the empty
 *     bignum_lock_stub_dup_0100B2E0
 *     (src/gzip_montgomery_tail.cpp).
 * Weak inline copies: GCC 2.9x emitted inline functions it had to call out
 * of line as weak copies at the end of each object file that uses them, and
 * the linker bound every call to the first copy.  BigNum::swap (called out
 * of line from unique()) has its first copy at 0x01007128 and a dead twin at
 * 0x0100B2C8.  Likewise an 8-byte `jr ra; nop` ends exactly those object
 * files of the family that call the refcount hook bignum_lock_stub
 * (0x01004B00): 0x01007140, 0x010081B0, 0x01008B90 and 0x0100B2E0.  The
 * files that do not call the hook (the BigNumRep basics, add, sub, shr,
 * shl) have none, even the add and sub files, which are "eh" files.  So these
 * four are, by inference, unreferenced duplicates of the hook.
 *
 * EH: every landing pad in this range is a cleanup.  Pads release live
 * handles, run __dt(&stackrep, 2), or operator delete a rep whose
 * constructor threw, then call __throw (0x01017CA8), or std::terminate
 * (0x010180D8) when inside a cleanup.  There are no catch clauses, so
 * plain RAII scopes are the faithful form.
 *
 * Library calls: operator new 0x01019988, operator delete 0x01019910,
 * memcpy 0x0101B010, memmove 0x0101B0BC, memset 0x0101B1B8.
 * All refcount changes go through bignum_lock_stub 0x01004B00 (bignum.h).
 */
#include "types.h"
#include "libc.h"
#include "bignum.h"
#include "montgomery.h"

/* ------------------------------------------------------------------------ */
/* Handle-level inlines expanded in this unit (no out-of-line copies)       */
/* ------------------------------------------------------------------------ */
/* In the original these are presumably inline members/operators of BigNum.
 * They are file-local here because only this unit expands them. */

/* Three-way compare of two handles.  Zero handles (null rep or nwords 0)
 * are resolved by sign, so the reps are dereferenced only when both are
 * non-zero.  Otherwise this is the signed BigNumRep::cmp (0x01007408).
 * The reps go through locals: the retail code reuses the isZero() loads as
 * the call arguments (b.rep straight into $a1), which egcs does when the
 * value is a pseudo, not when it is loaded directly into an argument. */
static inline int bn_compare(const BigNum &a, const BigNum &b)
{
    if (a.isZero())
        return -b.sign();
    if (b.isZero())
        return a.sign();
    BigNumRep *ra = a.rep;
    BigNumRep *rb = b.rep;
    return ra->cmp(*rb);
}

/* h <<= 1 on a handle: a zero value is left alone (no detach, no call),
 * otherwise copy-on-write, then BigNumRep::shl1 (not shl(1)). */
static inline void bn_shl1(BigNum &h)
{
    if (!h.isZero()) {
        h.unique();
        h.rep->shl1();
    }
}

/* h >>= n on a handle, same shape as bn_shl1. */
static inline void bn_shr(BigNum &h, u32 n)
{
    if (!h.isZero()) {
        h.unique();
        h.rep->shr(n);
    }
}

/* a % b by value.  The local copy r receives %= (bignum.h operator%=: a
 * stack rep receives the quotient, divmodInPlace 0x010083D0, __dt(&q, 2)),
 * then r is copied into the return slot - through %='s returned reference,
 * as in bignum.h operator-. */
static inline BigNum operator%(const BigNum &a, const BigNum &b)
{
    BigNum r = a;
    return r %= b;
}

/* Bit i of a magnitude; bits past nwords read as 0.  There is no null
 * check on the rep. */
static inline u32 bn_testBit(const BigNumRep &v, u32 i)
{
    u32 w = i >> 5;
    if (w >= v.nwords)
        return 0;
    return (v.words[w] >> (i & 31)) & 1;
}

/* Assignable reference to one limb: `ref = v` is setWord(idx, v).  shl1's
 * carry-out goes through one (presumably `(*this)[nwords] = 1` with an
 * operator[] returning this proxy in the original).  It is an 8-byte stack
 * temporary whose two stores ({this, nwords} at sp+0 / sp+4) stay in the
 * retail code although nothing reads them back. */
struct BigNumRepWordRef {
    BigNumRep *rep;
    u32 idx;
    BigNumRepWordRef(BigNumRep &r, u32 i) : rep(&r), idx(i) {}
    void operator=(u32 v) { rep->setWord(idx, v); }
};

/* ------------------------------------------------------------------------ */
/* Tail of the division object file                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x010085D0 BigNum::divmod
/* Handle-level division: this = a / b, r = a % b; returns *this ($v0 = $a0).
 * If a is zero (null rep or nwords 0), both r and *this are set to separate
 * fresh BigNum(0) reps (r first).  Quirk: these are the BigNumRep(int)
 * zeros with nwords = 1 and words[0] = 0, so isZero() is false for them.  b
 * is not looked at in that case.  Otherwise *this and then r are detached
 * for writing, keeping their values (a null rep gets a fresh empty rep; a
 * shared one is cloned into a temporary handle and swapped in, the swap
 * written out rather than a call of BigNum::swap; the temporary then
 * releases the old rep).  The detach is written out in the body, not as an
 * inline helper: that is what gives the retail frame (one stack slot shared
 * by all handle temporaries) and puts the operator-delete landing pads at
 * the end of the function.  Then the core runs:
 * rep->divmod(*a.rep, *b.rep, *r.rep) (0x010081B8, shift-subtract, which
 * never terminates for a zero b rep; a null b.rep is passed through as-is
 * and dereferenced by the core).
 * Sole caller: BigNum::modInverse (0x01006200), q.divmod(r0, r1, r). */
BigNum &BigNum::divmod(const BigNum &a, const BigNum &b, BigNum &r)
{
    if (a.isZero()) {
        r = BigNum(0);          /* temporary handle at sp+0, released after = */
        *this = BigNum(0);
        return *this;
    }

    if (rep == 0) {
        rep = new BigNumRep();
        rep->addRef();
    } else if (rep->refcount != 1) {
        BigNum tmp(new BigNumRep(*rep));
        BigNumRep *old = rep;
        rep = tmp.rep;
        tmp.rep = old;
    }
    if (r.rep == 0) {
        r.rep = new BigNumRep();
        r.rep->addRef();
    } else if (r.rep->refcount != 1) {
        BigNum tmp(new BigNumRep(*r.rep));
        BigNumRep *old = r.rep;
        r.rep = tmp.rep;
        tmp.rep = old;
    }
    rep->divmod(*a.rep, *b.rep, *r.rep);
    return *this;
}

// FUNC 0x01008B90 bignum_lock_stub_dup_01008B90
/* Empty function (`jr ra; nop`), the last FDE of the division object file
 * (FDE 0x0103437C, pc_begin word at 0x01034384; the file's FDE list ends
 * with the zero terminator at 0x0103438C).  No callers.  It is the weak
 * out-of-line copy of the refcount hook bignum_lock_stub (0x01004B00) that
 * this file emitted, because divmod calls the hook.  Its twins are
 * bignum_lock_stub_dup_01007140 / _010081B0 (bignum unit) and _0100B2E0
 * (Montgomery file).  Identity inferred from placement: the body has no
 * observable arguments.  Formerly F_01008B90. */
void bignum_lock_stub_dup_01008B90(void *obj, int unlock) /* ? identity inferred, see above */
{
    (void)obj;
    (void)unlock;
}

/* ------------------------------------------------------------------------ */
/* shr object file (0x01008B98..0x01008D80)                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x01008B98 BigNumRep::shr1
/* this >>= 1 (magnitude; the sign is untouched).  Walks the limbs from the
 * top down with a signed index, moving each limb's bit 0 into bit 31 of
 * the limb below.  Then it drops at most ONE zero top limb.  One is enough
 * because the old top limb was non-zero.  A zero value (nwords 0) returns
 * at once.  Leaf.  Callers: the division cores and shr(1). */
void BigNumRep::shr1()
{
    if (nwords == 0)
        return;

    u32 carry = 0;
    for (int i = (int)nwords - 1; i >= 0; i--) {
        u32 v = words[i];
        words[i] = carry | (v >> 1);
        carry = v << 31;
    }

    if (words[nwords - 1] == 0)
        nwords--;
}

// FUNC 0x01008C08 BigNumRep::shr
/* this >>= n (magnitude; the sign is only cleared when the value becomes 0).
 * Only two arguments: the mapping pass's third "nwords hint" is the nwords
 * load in a bnel delay slot.
 *   n == 0: nothing.  n == 1: shr1().
 *   n/32 >= nwords: value becomes 0 (negative = 0, nwords = 0; the buffer is
 *   kept).
 *   n % 32 == 0: memmove the limbs down by n/32 and subtract n/32 from nwords.
 *   Otherwise limb j (j = ws+1 .. nwords-1) is spread over two destination
 *   limbs: j-ws-1 gets v << (32-b) in its HIGH b bits (its low 32-b bits
 *   kept, `& lowMask`), and j-ws gets v >> b in its LOW 32-b bits (its high
 *   b bits kept, `& hiMask`).  The top destination limb keeps b stale
 *   high bits until the final `&= lowMask`.  Trim: if the new top limb is
 *   0, nwords drops by one (the `nn == 1 ? 0 : nn - 1` branch is redundant
 *   but present).
 * Shape notes: hiMask is computed before the loop (computed inside it,
 * GCSE hoists `~lowMask` onto the loop entry edge, which makes loop.c skip
 * the loop as "phony" and lose the retail strength reduction); the
 * destination pair goes through a pointer to words[j-ws-1] as in retail.
 * Callers: the division cores and Montgomery::mulmod (x >>= k, i.e. / R). */
void BigNumRep::shr(u32 n)
{
    if (n == 0)
        return;
    if (n == 1) {
        shr1();
        return;
    }

    u32 ws = n >> 5;            /* whole limbs  */
    u32 bs = n & 31;            /* residual bits */

    if (ws >= nwords) {         /* sltu: unsigned */
        nwords = 0;
        negative = 0;
        return;
    }

    if (bs == 0) {
        memmove(words, words + ws, (nwords - ws) * 4);
        nwords -= ws;
        return;
    }

    u32 lowMask = 0xFFFFFFFFu >> bs;
    u32 hiMask = ~lowMask;
    words[0] = words[ws] >> bs;
    for (u32 j = ws + 1; j < nwords; j++) {
        u32 v = words[j];
        u32 *d = &words[j - ws - 1];
        d[0] = (d[0] & lowMask) | (v << (32 - bs));
        d[1] = (d[1] & hiMask) | (v >> bs);
    }
    words[nwords - ws - 1] &= lowMask;

    u32 nn = nwords - ws;
    if (words[nn - 1] == 0) {
        if (nn == 1)
            nwords = 0;
        else
            nwords = nn - 1;
    } else
        nwords = nn;
}

/* ------------------------------------------------------------------------ */
/* shl object file (0x01008D80..0x01008FC0)                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x01008D80 BigNumRep::shl1
/* this <<= 1 (magnitude).  Limb loop bottom-up; nwords is re-read each pass
 * because the limb stores may alias it.  If the old top limb's bit 31 was
 * set, the carry-out is appended with setWord(nwords, 1) (0x01007638,
 * which grows the buffer and sets nwords + 1).  Before that call, {this,
 * nwords} are stored to sp+0 and sp+4 and never read again: a word-reference
 * proxy (BigNumRepWordRef above, a {rep, index} temporary whose operator= is
 * setWord).  Written that way the code matches the retail bytes exactly.
 * A zero value is a no-op.
 * Callers: the division cores, shl(1) and the Montgomery ctor's R loop. */
void BigNumRep::shl1()
{
    if (nwords == 0)
        return;

    u32 carry = 0;
    for (u32 i = 0; i < nwords; i++) {
        u32 v = words[i];
        words[i] = (v << 1) | carry;
        carry = v >> 31;
    }

    if (carry)
        BigNumRepWordRef(*this, nwords) = 1;     /* setWord(nwords, 1) */
}

// FUNC 0x01008E00 BigNumRep::shl
/* this <<= n (magnitude).
 *   n == 0: nothing.  n == 1: shl1().
 *   n % 32 == 0: reserve(nwords + ws, no zero-fill), memmove the limbs up by
 *   ws, memset the ws low limbs (unconditional, ws >= 1 here), nwords += ws.
 *   Otherwise: reserve(nwords + ws + 1, zero-fill).  From the top limb down
 *   (signed index), limb i is spread over destination limbs i+ws+1 (low b
 *   bits <- v >> (32-b), high bits kept) and i+ws (high bits <- v << b, low
 *   bits kept).  Then clear the stale low bits of limb ws, memset limbs
 *   0..ws-1 (only if ws != 0), and clear the stale high bits of the new top
 *   limb nwords+ws, which may hold old data when reserve did not
 *   reallocate.  nwords += ws, plus 1 if that top limb is non-zero.
 * Quirk: shifting a zero value (nwords 0) by n >= 32 leaves an
 * unnormalised zero with nwords = n/32 (all limbs 0).  The only callers are
 * the division cores, which shift p = 1, so this never happens. */
void BigNumRep::shl(u32 n)
{
    if (n == 0)
        return;
    if (n == 1) {
        shl1();
        return;
    }

    u32 bs = n & 31;
    u32 ws = n >> 5;

    if (bs == 0) {
        reserve(nwords + ws, false);
        memmove(words + ws, words, nwords * 4);
        memset(words, 0, ws * 4);
        nwords += ws;
        return;
    }

    reserve(nwords + ws + 1, true);

    u32 hiMask = 0xFFFFFFFFu << bs;
    u32 loMask = ~hiMask;
    for (int i = (int)nwords - 1; i >= 0; i--) {
        u32 v = words[i];
        u32 hi = v >> (32 - bs);            /* srlv by -bs */
        u32 lo = v << bs;
        u32 &w1 = words[i + ws + 1];
        u32 &w0 = words[i + ws];
        w1 = (w1 & hiMask) | hi;
        w0 = (w0 & loMask) | lo;
    }
    words[ws] &= hiMask;

    if (ws != 0)
        memset(words, 0, ws * 4);

    words[nwords + ws] &= loMask;
    if (words[nwords + ws] != 0)
        nwords += ws + 1;
    else
        nwords += ws;
}

/* ------------------------------------------------------------------------ */
/* Montgomery object file (0x01008FC0..0x0100B2E8; tail in                   */
/* src/gzip_montgomery_tail.cpp)                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x01008FC0 Montgomery::~Montgomery
/* GCC 2.9x __dt(this, flags).  Releases the handles m, T, Nprime, N, Rinv, R
 * (rep pointers at +0x2C, +0x24, +0x1C, +0x14, +0x0C, +0x04): lock stub,
 * --refcount, and __dt(rep, 3) when it drops to <= 0.  Then operator
 * delete(this) if flags & 1.  Sole caller: modpow 0x0100B238 with flags 2
 * (stack object). */
Montgomery::~Montgomery()
{
}

// FUNC 0x01009128 Montgomery::Montgomery
/* Sets up REDC for modulus n (returns this).
 *   R = BigNum(1)  (new BigNumRep(1): nwords 1, words[0] = 1)
 *   Rinv = null, N = n (shared rep, addref), Nprime = null,
 *   T and m = handles on fresh empty reps (new BigNumRep()), k = 0.
 *   while (R < n) { R <<= 1; k++; }   R = 2^k, the smallest power of 2 >= n.
 *     The loop compares against the PARAMETER n, not the member N (same
 *     rep).  The shift is bn_shl1: COW detach (out-of-line swap 0x01007128)
 *     then BigNumRep::shl1.
 *   Rinv   = modInverse(R - N, N)   R - N = R mod N since N <= R < 2N,
 *                                   so Rinv = R^-1 mod N.
 *   Nprime = R - modInverse(N, R)   = -N^-1 mod R, the REDC constant.
 * The temporaries are destroyed as full expressions end: the modInverse
 * result at sp+0 first, then R - N at sp+0x10.  Then R - x at sp+0, then
 * modInverse(N, R) at sp+0x10.
 * Only valid for odd n (N^-1 mod R must exist).  With an even modulus
 * modInverse returns a non-inverse, mulmod computes wrong values, and the
 * RSA compare simply fails.  There is no size, parity or value check on
 * n; the context uses whatever modulus it is handed.
 * Sole caller: modpow 0x0100B238. */
Montgomery::Montgomery(const BigNum &n)
    : R(1), Rinv(), N(n), Nprime(), T(new BigNumRep()), m(new BigNumRep())
{
    k = 0;
    while (bn_compare(R, n) < 0) {
        bn_shl1(R);
        k++;
    }

    Rinv = BigNum::modInverse(R - N, N);
    Nprime = R - BigNum::modInverse(N, R);
}

// FUNC 0x01009F68 Montgomery::mulmod
/* Montgomery product (REDC):  x = x * y * R^-1 mod N, for x, y in [0, N).
 *   T = x * y            mod 2^(2k)  (BigNum::mulTrunc 0x01008000; exact)
 *   m = T * Nprime       mod 2^k     (= -T * N^-1 mod R)
 *   x = m * N            mod 2^(2k)  (exact), then x += T (T + m*N is a
 *                                     multiple of R)
 *   x >>= k                          (bn_shr: COW, BigNumRep::shr 0x01008C08)
 *   if (N <= x) x -= N               (bn_compare(N, x) <= 0; operator-=)
 * $a1/$a2 (x, y) go straight through to the first mulTrunc.  The third
 * mulTrunc's returned reference ($v0, == x since mulTrunc returns *this)
 * receives the += (inline operator+=: a null x would just share T's rep),
 * and the reference operator+= returns receives the >>= k (one chained
 * expression, as below); the compare and -= then use $a1 (x) directly.
 * All of these are the same object x.  x may alias y (squaring): T is
 * formed before x is overwritten.  The result is < N.
 * Sole caller: powmod 0x0100A5D0. */
void Montgomery::mulmod(BigNum &x, const BigNum &y)
{
    T.mulTrunc(x, y, k * 2);
    m.mulTrunc(T, Nprime, k);
    bn_shr(x.mulTrunc(m, N, k * 2) += T, k);    /* (x = m*N) += T, then >>= k */
    if (bn_compare(N, x) <= 0)
        x -= N;
}

// FUNC 0x0100A5D0 Montgomery::powmod
/* Returns base^exp mod N (GCC 2.9x: $a0 = hidden return slot, $a1 = this,
 * $a2 = base, $a3 = exp).  Right-to-left binary exponentiation in the
 * Montgomery domain:
 *   acc = R - N                  Mont(1) = R mod N         (sp+0x00)
 *   xm  = (base * R) % N         Mont(base)                (sp+0x10)
 *     (base * R: BigNum::mul 0x01007E00 on a temporary at sp+0x30, copied
 *     to sp+0x20; % N: copy at sp+0x40, operator%= with stack rep sp+0x60
 *     and divmodInPlace 0x010083D0.)
 *   for bit i = 0, 1, ...:  if exp bit i: mulmod(acc, xm);
 *                           stop when ++i == nbits; else mulmod(xm, xm).
 *   return (acc * Rinv) % N      back out of the Montgomery domain.
 * nbits = exp.rep->bitLength() (0x01007710).  exp.rep is dereferenced
 * without a null check.  Loop quirk: the first bit test runs before any
 * bound check and the exit test is `!=` (bne), so for exp == 0 (nbits 0)
 * the loop only ends when i wraps after 2^32 squarings, in effect a hang.
 * In the RSA path exp is 66001 (17 bits).  exp.rep is re-read each pass.
 * The result is fully reduced, in [0, N).  Sole caller: modpow 0x0100B238. */
BigNum Montgomery::powmod(const BigNum &base, const BigNum &exp)
{
    BigNum acc = R - N;
    BigNum xm = (base * R) % N;

    u32 nbits = exp.rep->bitLength();
    u32 i = 0;
    for (;;) {
        if (bn_testBit(*exp.rep, i))
            mulmod(acc, xm);
        if (++i == nbits)
            break;
        mulmod(xm, xm);
    }

    return (acc * Rinv) % N;
}
