/* bignum.cpp - arbitrary-precision integers, part 1 (GT3 US bootstrap,
 * BOOTSTRAP_SCUS-97102, .text 0x01005BC0..0x010085D0).
 *
 * Polyphony C++ (GCC 2.9x-ee, -O2, sjlj EH) that the bootstrap uses for the
 * RSA check of core.gt3: ImageLoader::verify imports the in-file modulus /
 * signature and the SHA-512 digest as big-endian byte strings
 * (BigNumRep(const u8*, int)), the Montgomery code (montgomery unit) builds
 * R, R^-1 mod N and N' with modInverse, multiplies with mul / mulTrunc and
 * reduces with the shift-subtract division cores here.
 * See include/bignum.h for the two classes and the refcount protocol.
 *
 * Object files.  The .eh_frame embedded in .data has one CIE per object file
 * (its "eh" augmentation, when present, points at that file's slice of
 * .gcc_except_table) and a zero terminator after each file's FDEs.  This
 * range is SIX object files, not one; they are kept in one source file here
 * with divider comments:
 *   A 0x01005BC0..0x01007148  CIE 0x01033D80 "eh" (except table 0x01041388):
 *     gcd, modInverse, then weak out-of-line copies of two inlines:
 *     BigNum::swap and the refcount hook (see F_01007140 below).
 *   B 0x01007148..0x01007788  CIE 0x01033E50, no EH: the BigNumRep basics
 *     (~BigNumRep .. bitLength).  No landing pads anywhere in it.
 *   C 0x01007788..0x01007988  CIE 0x0103401C "eh" (0x01041618): addMag, add.
 *   D 0x01007988..0x01007B70  CIE 0x010340B0 "eh" (0x01041638): subMag, sub.
 *   E 0x01007B70..0x010081B8  CIE 0x01034144 "eh" (0x01041658):
 *     BigNumRep::mulTrunc, BigNum::mul, BigNum::mulTrunc, hook copy.
 *   F 0x010081B8..0x01008B98  CIE 0x0103426C "eh" (0x01041678): the two
 *     division cores here; BigNum::divmod 0x010085D0 and the hook copy
 *     0x01008B90 continue the file in the montgomery unit.
 * GCC 2.9x emitted inline functions it had to call out of line as WEAK
 * global symbols at the end of each object file.  Every copy's code stays
 * in .text, and the linker binds every reference to the first copy it saw.
 * So all calls go to BigNum::swap 0x01007128 (file A) and to the hook
 * 0x01004B00 (imageloader's file); the later copies are dead bytes.
 *
 * EH note: every landing pad of this unit is a pure cleanup (release the
 * live handles, destroy stack reps with __dt(&rep, 2), or operator delete a
 * rep whose constructor threw) followed by `jal __throw` (0x01017CA8, resume
 * unwinding) or, inside cleanups, `jal std::terminate` (0x010180D8).  There
 * are no catch clauses; RAII scopes below are the faithful representation.
 *
 * Library calls: malloc 0x01011390, free 0x01011368, memcpy 0x0101B010,
 * memset 0x0101B1B8, operator new 0x01019988, operator delete 0x01019910,
 * __muldi3 0x01018070 (the u64 limb product in mulTrunc).
 */
#include "types.h"
#include "libc.h"
#include "bignum.h"

/* ======================================================================== */
/* Object file A (0x01005BC0..0x01007148): gcd, modInverse + weak copies    */
/* ======================================================================== */

// FUNC 0x01005BC0 BigNum::gcd
/* Euclid on handles: returns gcd(a, b) (hidden return slot in $a0).
 * NO CALLERS in the binary - dead code linked in with object file A.
 * Loop: if y == 0 return x; x %= y; if x == 0 return y; y %= x.
 * Each %= is operator%= (bignum.h) expanded inline: copy-on-write detach of
 * the dividend (new 0x18 rep + out-of-line BigNum::swap 0x01007128), a stack
 * rep copy of the dividend at sp+0x30 that receives the quotient, then
 * divmodInPlace(stack, *divisor.rep, *dividend.rep) and __dt(stack, 2).
 * A null dividend rep skips the whole %= (its null test covers both). */
BigNum BigNum::gcd(const BigNum &a, const BigNum &b)
{
    BigNum x(a);   /* sp+0x00 */
    BigNum y(b);   /* sp+0x10 */

    for (;;) {
        if (y.isZero())
            return x;
        x %= y;
        if (x.isZero())
            return y;
        y %= x;
    }
}

// FUNC 0x01006200 BigNum::modInverse
/* Extended Euclid: returns a^-1 mod m (the coefficient of a), normalised to
 * be non-negative by one `+= m`.  Sole caller: the Montgomery context ctor
 * F_01009128 (R^-1 mod N and N^-1 mod R).  Stack handles: x0 sp+0x00,
 * x1 sp+0x10, r0 sp+0x20, r1 sp+0x30, q sp+0x40, r sp+0x50; temporaries
 * sp+0x60/0xA0 (detach; sp+0x60 is also operator-'s returned copy),
 * sp+0x70 (x1*q), sp+0x80 (mul target), sp+0x90 (copy of x0 inside
 * operator-).  The loop's quotient/remainder come from
 * the montgomery unit's handle divmod 0x010085D0.  `r` doubles as the swap
 * temporary for the coefficients.
 * Quirk: r0 and r1 are made unique (cloned if shared) before the loop even
 * though nothing writes them in place - kept as in the binary. */
BigNum BigNum::modInverse(const BigNum &a, const BigNum &m)
{
    BigNum x0;          /* coefficient, starts 0 (null rep) */
    BigNum x1(1);       /* coefficient, starts 1: new BigNumRep(1) */
    BigNum r0(m);
    BigNum r1(a);
    BigNum q;
    BigNum r;

    r0.unique();
    r1.unique();

    while (!r1.isZero()) {
        q.divmod(r0, r1, r);        /* q = r0 / r1, r = r0 % r1 (0x010085D0) */
        r0 = r1;
        r1 = r;
        r  = x1;
        x1 = x0 - x1 * q;           /* temporaries released in reverse order */
        x0 = r;
    }

    if (x0.sign() < 0)
        x0 += m;

    return x0;
}

// FUNC 0x01007128 BigNum::swap
/* Exchange the rep pointers of two handles (no refcount traffic).
 * An inline member emitted out of line (weak) at the end of object file A,
 * whose gcd / modInverse call it.  As the first copy linked, it also serves
 * the Montgomery file's calls (F_01009128, F_01009F68, F_0100A5D0), whose own
 * copy at 0x0100B2C8 is unreferenced.  Inlined in file E (BigNum::mul /
 * mulTrunc) and in BigNum::divmod. */
void BigNum::swap(BigNum &o)
{
    BigNumRep *t = rep;
    rep = o.rep;
    o.rep = t;
}

// FUNC 0x01007140 bignum_lock_stub_dup_01007140
/* Empty function (`jr ra; nop`), the last FDE of object file A (FDE
 * 0x01033E3C, pc_begin word 0x01033E44, pc_range 8).  No callers.
 * Identified as file A's weak out-of-line copy of the refcount hook
 * bignum_lock_stub (0x01004B00, 8 bytes, also `jr ra; nop`), which gcd and
 * modInverse call; all those calls bind to the first copy, 0x01004B00.
 * This is an inference, not provable from the code alone.  The evidence: an
 * 8-byte empty function ends exactly the object files that call 0x01004B00
 * (this one, E 0x010081B0, F 0x01008B90, Montgomery 0x0100B2E0), and no
 * other bignum file.  The "eh" files C (add) and D (sub) have none.  This is
 * the same weak-copy pattern as BigNum::swap 0x01007128 / 0x0100B2C8. */
void bignum_lock_stub_dup_01007140(void *obj, int unlock) /* ? identity inferred, see above */
{
}

/* ======================================================================== */
/* Object file B (0x01007148..0x01007788): BigNumRep basics, no EH           */
/* ======================================================================== */

// FUNC 0x01007148 BigNumRep::~BigNumRep
/* GCC 2.9x destructor __dt(this, int flags): free(words) (unconditionally,
 * even when NULL), then operator delete(this) if flags & 1.  Called with 3
 * from every inline release() (`delete rep`) and with 2 for stack reps. */
BigNumRep::~BigNumRep()
{
    free(words);
}

// FUNC 0x01007190 BigNumRep::BigNumRep(const u8 *, int)
/* Constructor importing a big-endian byte string (mapping pass called it
 * setFromBytesBE; it zeroes the refcount, i.e. it is a ctor used after
 * operator new(0x18) by F_010043A8 / F_010044A8).
 * Reserves (len+3)/4 limbs zero-filled, ORs bytes in from the END of the
 * string (last byte -> bits 0..7 of words[0]), then sets nwords to the whole
 * power-of-two CAPACITY and trims zero top limbs.  Returns this. */
BigNumRep::BigNumRep(const u8 *bytes, int len)
    : refcount(0), nwords(0), capacity(0), negative(0), words(0)
{
    reserve((u32)(len + 3) >> 2, true);     /* srl: unsigned */

    u32 i = 0;
    for (int j = len - 1; j >= 0; j--, i++)
        words[i >> 2] |= (u32)bytes[j] << ((i & 3) * 8);

    nwords = capacity;
    while (nwords != 0 && words[nwords - 1] == 0)
        nwords--;
}

// FUNC 0x01007278 BigNumRep::assign
/* Deep copy of o's value into this rep's own buffer (grown with reserve(n,
 * false) only when capacity < o.nwords), limbs above nwords zeroed up to
 * capacity.  No self-assignment guard.  Sole caller: divmod core 0x010081B8
 * (r = a).  Returns this. */
BigNumRep &BigNumRep::assign(const BigNumRep &o)
{
    if (capacity < o.nwords)
        reserve(o.nwords, false);

    u32 n = o.nwords;
    negative = o.negative;
    nwords = n;
    memcpy(words, o.words, n * 4);
    memset(words + nwords, 0, (capacity - nwords) * 4);
    return *this;
}

// FUNC 0x01007308 BigNumRep::setInt
/* this = v (sign-magnitude).  reserve(1, zero-fill) first; v == 0 gives
 * nwords 0 / words[0] 0 / positive (unlike the BigNumRep(int) ctor, which
 * keeps nwords 1 for zero).  Callers: mul/mulTrunc (0), divmod cores (0 for
 * the quotient).  Returns this. */
BigNumRep &BigNumRep::setInt(int v)
{
    reserve(1, true);
    if (v == 0) {
        nwords = 0;
        words[0] = 0;
        negative = 0;
        return *this;
    }
    nwords = 1;
    if (v < 0) {
        negative = 1;
        words[0] = -v;
    } else {
        negative = 0;
        words[0] = v;
    }
    return *this;
}

// FUNC 0x01007390 BigNumRep::cmpMag
/* Unsigned magnitude compare: -1 if |this| < |o|, +1 if greater, 0 equal.
 * Longer limb count wins (relies on normalised nwords); then limbs from the
 * top down (signed index, sltu limb compares). */
int BigNumRep::cmpMag(const BigNumRep &o) const
{
    if (nwords < o.nwords)
        return -1;
    if (o.nwords < nwords)
        return 1;
    for (int i = (int)nwords - 1; i >= 0; i--) {
        if (words[i] < o.words[i])
            return -1;
        if (o.words[i] < words[i])
            return 1;
    }
    return 0;
}

// FUNC 0x01007408 BigNumRep::cmp
/* Signed compare -1/0/+1.  Same sign: magnitude compare, negated when both
 * are negative (sign re-read after the call).  Different signs: -1 if this
 * is negative, else 1 if o is negative, else 0 - the last case is
 * unreachable (the signs differ), and egcs folds the pair into
 * sltu $v0, $zero, o.negative.  Note -0 (negative flag
 * set with nwords 0, producible by add/sub) compares below +0.
 * Callers: ImageLoader::verify (digest vs. decrypted signature), the divmod
 * cores, Montgomery ctor / mulmod. */
int BigNumRep::cmp(const BigNumRep &o) const
{
    if (negative == o.negative) {
        int c = cmpMag(o);
        return negative ? -c : c;
    }
    if (negative)
        return -1;
    if (o.negative)
        return 1;
    return 0;
}

// FUNC 0x01007460 BigNumRep::operator==
/* 1 iff same limb count and identical limbs; the sign is NOT compared.
 * No callers (unused). */
bool BigNumRep::operator==(const BigNumRep &o) const
{
    if (nwords != o.nwords)
        return false;
    for (u32 i = 0; i < nwords; i++)
        if (words[i] != o.words[i])
            return false;
    return true;
}

// FUNC 0x010074C0 BigNumRep::operator!=
/* Exact complement of operator== (magnitude only).  No callers (unused). */
bool BigNumRep::operator!=(const BigNumRep &o) const
{
    if (nwords != o.nwords)
        return true;
    for (u32 i = 0; i < nwords; i++)
        if (words[i] != o.words[i])
            return true;
    return false;
}

// FUNC 0x01007520 BigNumRep::swapMag
/* Swap the whole value (nwords, capacity, negative, words buffer) of two
 * reps, leaving the refcounts in place.  Used by add/sub to commit a
 * stack-temporary result and by divmodInPlace. */
void BigNumRep::swapMag(BigNumRep &o)
{
    u32 n = nwords;
    nwords = o.nwords;
    o.nwords = n;

    /* the other three fields: all six loads first, then the stores */
    u32 oc = o.capacity;
    BigNumSign os = o.negative;
    u32 *ow = o.words;
    u32 c = capacity;
    BigNumSign s = negative;
    u32 *w = words;
    capacity = oc;
    negative = os;
    words = ow;
    o.capacity = c;
    o.negative = s;
    o.words = w;
}

// FUNC 0x01007568 BigNumRep::reserve
/* Ensure capacity >= n limbs.  New capacity = smallest power of two >= n
 * (starting from 1); malloc(cap*4), copy the nwords live limbs from the old
 * buffer and free it (only if there was one); with zeroFill, clear limbs
 * [nwords, cap) of the new buffer.  Nothing happens (no zeroing either) when
 * the capacity already suffices.  The loop's nops are the R5900 short-loop
 * workaround padding. */
void BigNumRep::reserve(u32 n, bool zeroFill)
{
    if (capacity < n) {
        u32 cap = 1;
        while (cap < n)
            cap <<= 1;

        u32 *buf = (u32 *)malloc(cap * 4);
        if (words) {
            memcpy(buf, words, nwords * 4);
            free(words);
        }
        if (zeroFill)
            memset(buf + nwords, 0, (cap - nwords) * 4);
        words = buf;
        capacity = cap;
    }
}

// FUNC 0x01007638 BigNumRep::setWord
/* Store one limb and keep nwords normalised:
 *  - idx < nwords: store; if val == 0, trim zero top limbs.
 *  - idx >= nwords, val != 0: reserve(idx+1, zero-fill), store,
 *    nwords = idx+1.  (Quirk: if the capacity was already large enough,
 *    limbs between the old nwords and idx are NOT cleared; all callers write
 *    limbs in ascending order so this never matters.)
 *  - idx >= nwords, val == 0: nothing.
 * Callers: addMag, subMag, shl1 (montgomery unit). */
void BigNumRep::setWord(u32 idx, u32 val)
{
    if (idx < nwords) {
        words[idx] = val;
        if (val == 0) {
            while (nwords != 0 && words[nwords - 1] == 0)
                nwords--;
        }
    } else if (val != 0) {
        reserve(idx + 1, true);
        words[idx] = val;
        nwords = idx + 1;
    }
}

// FUNC 0x01007710 BigNumRep::bitLength
/* Number of significant bits: 32*i + b + 1 for the highest set bit b of the
 * highest non-zero limb i; 0 for zero.  Callers: BigNum::mul (product size)
 * and powmod F_0100A5D0 (exponent bit count). */
u32 BigNumRep::bitLength() const
{
    if (nwords == 0)
        return 0;
    for (int i = (int)nwords - 1; i >= 0; i--) {
        u32 w = words[i];
        if (w == 0)
            continue;
        for (int b = 31; b >= 0; b--)
            if ((w >> b) & 1)
                return b + 1 + i * 32;
    }
    return 0;
}

/* ======================================================================== */
/* Object file C (0x01007788..0x01007988): addMag, add                      */
/* ======================================================================== */

// FUNC 0x01007788 BigNumRep::addMag
/* |this| += |o| (sign untouched).  reserve(max(nwords, o.nwords), zero-fill)
 * - one limb short of the loop, so a final carry-out grows the buffer again
 * inside setWord - then limb loop i = 0 .. max inclusive with a 64-bit
 * accumulator (daddu of zero-extended limbs + carry); limbs missing on
 * either side read as 0 and this->nwords is re-read every iteration because
 * setWord changes it.  Returns this. */
BigNumRep &BigNumRep::addMag(const BigNumRep &o)
{
    u32 n = nwords;
    if (n < o.nwords)
        n = o.nwords;
    reserve(n, true);
    u32 end = n + 1;

    u32 carry = 0;
    for (u32 i = 0; i < end; i++) {
        u64 t = carry;
        u32 a;
        if (i < nwords) a = words[i]; else a = 0;
        t += a;
        u32 b;
        if (i < o.nwords) b = o.words[i]; else b = 0;
        t += b;
        setWord(i, (u32)t);
        carry = (u32)(t >> 32);
    }
    return *this;
}

// FUNC 0x01007888 BigNumRep::add
/* Signed this += o.  Same signs: addMag.  Different signs: if |o| > |this|
 * compute |o| - |this| in a stack copy of o (sp+0x00, carrying o's sign) and
 * swapMag it into this, destroying the old value with __dt(&tmp, 2);
 * otherwise subMag in place (sign of this kept - so equal magnitudes can
 * leave a "-0").  Returns this.  The handle operator+= (bignum.h) calls it. */
BigNumRep &BigNumRep::add(const BigNumRep &o)
{
    if (negative == o.negative) {
        addMag(o);
        return *this;
    }
    if (o.cmpMag(*this) <= 0) {
        subMag(o);
    } else {
        BigNumRep t(o);
        t.subMag(*this);
        swapMag(t);
    }
    return *this;
}

/* ======================================================================== */
/* Object file D (0x01007988..0x01007B70): subMag, sub                      */
/* ======================================================================== */

// FUNC 0x01007988 BigNumRep::subMag
/* |this| -= |o|, assuming |this| >= |o| (no borrow-out handling).  Loops
 * over the ORIGINAL nwords; subtrahend limb + borrow computed first, and if
 * that addition wraps (o limb 0xFFFFFFFF with borrow 1, i.e. subtracting
 * 2^32) the limb is left unchanged and the borrow stays 1 (setWord skipped).
 * Otherwise d = w - s, borrow = (w < d), setWord(i, d) - which may trim
 * nwords, hence the per-iteration re-read of this->nwords.  Returns this. */
BigNumRep &BigNumRep::subMag(const BigNumRep &o)
{
    u32 n = nwords;
    u32 borrow = 0;
    for (u32 i = 0; i < n; i++) {
        u32 v;
        if (i < o.nwords) v = o.words[i]; else v = 0;
        u32 s = v + borrow;
        if (s < borrow)
            continue;
        u32 w;
        if (i < nwords) w = words[i]; else w = 0;
        u32 d = w - s;
        borrow = w < d;
        setWord(i, d);
    }
    return *this;
}

// FUNC 0x01007A60 BigNumRep::sub
/* Signed this -= o.  Different signs: addMag.  Same signs: if |o| > |this|
 * compute |o| - |this| in a stack copy of o, swapMag it in and flip the sign
 * (xori 1); otherwise subMag in place.  Returns this.  Callers: modInverse
 * (via operator-=), divmod cores, Montgomery ctor / mulmod / powmod. */
BigNumRep &BigNumRep::sub(const BigNumRep &o)
{
    if (negative != o.negative) {
        addMag(o);
        return *this;
    }
    if (o.cmpMag(*this) <= 0) {
        subMag(o);
    } else {
        BigNumRep t(o);
        t.subMag(*this);
        swapMag(t);
        negative = !negative;
    }
    return *this;
}

/* ======================================================================== */
/* Object file E (0x01007B70..0x010081B8): multiplication + weak hook copy  */
/* ======================================================================== */

// FUNC 0x01007B70 BigNumRep::mulTrunc
/* this = (|a| * |b|) mod 2^maxBits, sign = a.sign ^ b.sign.
 * Schoolbook product into nw = (maxBits+31)/32 limbs: reserve(nw, false),
 * clear the WHOLE capacity, then for each limb i < min(a.nwords, nw) of a
 * accumulate a[i]*b[j-i] into limbs j = i .. min(i+b.nwords, nw)-1 with a
 * 64-bit accumulator (u64 product via __muldi3 0x01018070; the lq/sq around
 * the call just preserve $a2/$a3/$t0/$t1), then ripple the carry upward
 * while it is non-zero and j < nw.  Finally mask the top limb to
 * maxBits & 31 bits (if non-zero), trim zero top limbs and store nwords and
 * the sign.  this must not alias a or b (it is cleared first).
 * The source reads a.words[i] per row and both signs at the end; egcs's
 * GCSE (PRE) hoists those loads to the loop entry (onto both the loop and
 * the zero-trip path), which the bool sign type (BigNumSign) allows.
 * b.nwords / b.words / this->words are re-read per row.
 * Callers: BigNum::mul (maxBits = bitlen(a)+bitlen(b), i.e. exact) and
 * BigNum::mulTrunc (Montgomery REDC's mod-R products). */
BigNumRep &BigNumRep::mulTrunc(const BigNumRep &a, const BigNumRep &b, u32 maxBits)
{
    u32 nw = (maxBits + 31) >> 5;
    reserve(nw, false);
    memset(words, 0, capacity * 4);
    u32 na = a.nwords;
    if (nw < na)
        na = nw;

    for (u32 i = 0; i < na; i++) {
        u32 carry = 0;
        u32 end = i + b.nwords;
        if (nw < end)
            end = nw;
        u64 ai = a.words[i];
        u32 j;
        for (j = i; j < end; j++) {
            u64 t = (u64)carry + words[j];
            t += ai * (u64)b.words[j - i];
            words[j] = (u32)t;
            carry = (u32)(t >> 32);
        }
        for (; carry != 0 && j < nw; j++) {
            u64 t = (u64)words[j] + carry;
            words[j] = (u32)t;
            carry = (u32)(t >> 32);
        }
    }

    if (maxBits & 31)
        words[nw - 1] &= (1u << (maxBits & 31)) - 1;

    while (nw != 0 && words[nw - 1] == 0)
        nw--;
    nwords = nw;
    negative = a.negative != b.negative;
    return *this;
}

// FUNC 0x01007E00 BigNum::mul
/* this = a * b (exact).  If a or b is zero (null rep or nwords 0): set an
 * existing rep to 0 via setInt(0) (a null rep stays null) and return.
 * Otherwise detach this->rep for overwrite, keeping NO value (new zeroed
 * rep if null; if shared, swap in a new zeroed rep through a temporary
 * handle, which then releases the old rep), then
 * rep->mulTrunc(*a.rep, *b.rep, bitlen(a) + bitlen(b)).
 * The detach is written out here, not as an inline helper: its EH landing
 * pad (a plain rethrow) is emitted at the end of the function in retail,
 * where only code in the function's own body puts it, and the swap is
 * inlined (no call of BigNum::swap 0x01007128).
 * rep / a.rep / b.rep are re-read after the detach (a or b may be *this);
 * egcs's PRE turns those reads into per-path reloads into $s1..$s3.
 * Called on a fresh default-constructed temporary by operator* (modInverse,
 * powmod F_0100A5D0).  Returns *this. */
BigNum &BigNum::mul(const BigNum &a, const BigNum &b)
{
    if (a.isZero() || b.isZero()) {
        if (rep)
            rep->setInt(0);
        return *this;
    }
    if (!rep) {
        rep = new BigNumRep();
        rep->addRef();
    } else if (rep->refcount != 1) {
        BigNum tmp(new BigNumRep());
        BigNumRep *t = rep;
        rep = tmp.rep;
        tmp.rep = t;
    }
    BigNumRep *r = rep;
    BigNumRep *ra = a.rep;
    BigNumRep *rb = b.rep;
    r->mulTrunc(*ra, *rb, ra->bitLength() + rb->bitLength());
    return *this;
}

// FUNC 0x01008000 BigNum::mulTrunc
/* this = a * b mod 2^bits: identical shell to BigNum::mul (same written-out
 * detach) but with the caller's bit limit.  Sole caller: Montgomery mulmod F_01009F68 (T = x*y
 * with 2k bits, m = T*N' mod R with k bits, m*N with 2k bits).
 * Returns *this. */
BigNum &BigNum::mulTrunc(const BigNum &a, const BigNum &b, u32 bits)
{
    if (a.isZero() || b.isZero()) {
        if (rep)
            rep->setInt(0);
        return *this;
    }
    if (!rep) {
        rep = new BigNumRep();
        rep->addRef();
    } else if (rep->refcount != 1) {
        BigNum tmp(new BigNumRep());
        BigNumRep *t = rep;
        rep = tmp.rep;
        tmp.rep = t;
    }
    rep->mulTrunc(*a.rep, *b.rep, bits);
    return *this;
}

// FUNC 0x010081B0 bignum_lock_stub_dup_010081B0
/* Empty function (`jr ra; nop`), the last FDE of object file E (FDE
 * 0x01034258, pc_begin word 0x01034260, pc_range 8).  No callers.
 * Identified as file E's weak out-of-line copy of the refcount hook
 * bignum_lock_stub 0x01004B00, which BigNum::mul / mulTrunc call (the calls
 * bind to 0x01004B00).  Same inference and evidence as
 * bignum_lock_stub_dup_01007140. */
void bignum_lock_stub_dup_010081B0(void *obj, int unlock) /* ? identity inferred */
{
}

/* ======================================================================== */
/* Object file F, first part (0x010081B8..0x010085D0): division cores.      */
/* BigNum::divmod 0x010085D0 and the hook copy 0x01008B90 that end this     */
/* file are in src/montgomery.cpp.                                           */
/* ======================================================================== */

// FUNC 0x010081B8 BigNumRep::divmod
/* Binary shift-subtract long division: this = a / b, r = a % b.
 * d = stack copy of b (sp+0x00), p = BigNumRep(1) (sp+0x20); r.assign(a);
 * this = 0.  Double d until d >= r counting n, p <<= n (p = 2^n).  Then
 * while b <= r: halve d until d <= r counting n, p >>= n, r -= d, this += p.
 * All compares are the SIGNED cmp, so the result is only meaningful for
 * non-negative operands (b == 0 never terminates).  Quotient sign =
 * a.sign ^ b.sign; r keeps a's sign.  Temporaries destroyed p then d
 * (__dt(&x, 2)).  Sole caller: handle divmod 0x010085D0 (montgomery unit).
 * Returns this. */
BigNumRep &BigNumRep::divmod(const BigNumRep &a, const BigNumRep &b, BigNumRep &r)
{
    BigNumRep d(b);
    BigNumRep p(1);

    r.assign(a);
    setInt(0);

    u32 n = 0;
    while (d.cmp(r) < 0) {
        d.shl1();
        n++;
    }
    p.shl(n);

    while (b.cmp(r) <= 0) {
        n = 0;
        while (r.cmp(d) < 0) {
            d.shr1();
            n++;
        }
        p.shr(n);
        r.sub(d);
        add(p);
    }

    negative = a.negative != b.negative;
    return *this;
}

// FUNC 0x010083D0 BigNumRep::divmodInPlace
/* In-place variant: on entry *this holds the dividend; on exit *this is the
 * quotient and r the remainder (r's previous value is discarded - the
 * dividend is moved into r with swapMag, then this = 0).  Same
 * shift-subtract loop as divmod.
 * Quirk (reproduced): the final sign is computed AFTER this was zeroed, so
 * quotient.sign = (0 ^ b.sign) - the dividend's sign is never folded in.
 * Callers: BigNum::operator%= (gcd) and powmod F_0100A5D0, always with a
 * stack copy of the dividend as this and the dividend's own rep as r, so
 * effectively r %= b.  Returns this. */
BigNumRep &BigNumRep::divmodInPlace(const BigNumRep &b, BigNumRep &r)
{
    BigNumRep d(b);
    BigNumRep p(1);

    swapMag(r);
    setInt(0);

    u32 n = 0;
    while (d.cmp(r) < 0) {
        d.shl1();
        n++;
    }
    p.shl(n);

    while (b.cmp(r) <= 0) {
        n = 0;
        while (r.cmp(d) < 0) {
            d.shr1();
            n++;
        }
        p.shr(n);
        r.sub(d);
        add(p);
    }

    negative = negative != b.negative;
    return *this;
}
