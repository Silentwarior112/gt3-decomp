/* bignum.h - arbitrary-precision integer library of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), part 1 (unit "bignum", 0x01005BC0..0x010085D0).
 *
 * Polyphony C++ (GCC 2.9x-ee), no symbols anywhere (the Lupo 2 dump has no
 * bignum/RSA code); class and method names are descriptive.  The library is
 * two-level:
 *
 *   BigNumRep - the value: sign-magnitude, little-endian array of 32-bit limbs
 *               (words[0] = least significant), heap block of 0x18 bytes, with
 *               an intrusive reference count.  Also used directly as a plain
 *               stack object (refcount left at 0) for temporaries.
 *   BigNum    - an 8-byte copy-on-write handle {+0 unused, +4 BigNumRep*}.
 *               rep == NULL means the value zero.
 *
 * Every refcount change is bracketed by a call to the empty hook
 * bignum_lock_stub(rep, 0 = addref / 1 = release) at 0x01004B00; a release
 * that drops the count to <= 0 destroys the rep with the GCC 2.9x deleting
 * destructor  __dt(rep, 3)  (0x01007148 with flag 3 = free words + operator
 * delete).  Stack reps are destroyed with __dt(&tmp, 2) (free words only).
 *
 * None of these classes has a vtable.  The data words that reference these
 * functions (0x01033DA4..0x010342E4) are the pc_begin fields of DWARF2
 * .eh_frame FDEs embedded in .data (one CIE per object file; this range is
 * six object files - see the header of src/bignum.cpp).
 *
 * All the small handle helpers below (refcounting, copy-on-write detach,
 * +=, -=, %=, operator*, operator-) are inline in the original too: they
 * appear expanded inside gcd / modInverse / mul / mulTrunc here and inside
 * the Montgomery code of the "montgomery" unit (0x010085D0..0x0100B238).
 *
 * Functions of this header that are DEFINED IN THE MONTGOMERY UNIT
 * (src/montgomery.cpp), declared here because they are members of these
 * classes and are called by this unit:
 *   BigNum::divmod        0x010085D0   (handle-level q = a / b, r = a % b)
 *   BigNumRep::shr1       0x01008B98
 *   BigNumRep::shr        0x01008C08
 *   BigNumRep::shl1       0x01008D80
 *   BigNumRep::shl        0x01008E00
 */
#ifndef BOOT_BIGNUM_H
#define BOOT_BIGNUM_H

#include "types.h"
#include "libc.h"

/* 0x01004B00: `jr ra; nop` - compiled-out lock/debug hook called as
 * (rep, 0) before every refcount++ and (rep, 1) before every refcount--.
 * Lives in the imageloader unit's address range and is DEFINED THERE
 * (src/imageloader.cpp, which includes this header); name from
 * ref/symbols.csv.
 * In the original it is an inline function that GCC 2.9x emitted out of line
 * as a weak symbol at the end of every object file that calls it; the linker
 * bound all calls to this first copy.  The later copies (same 8 bytes, no
 * callers) are bignum_lock_stub_dup_01007140 / _010081B0 (this unit),
 * bignum_lock_stub_dup_01008B90 (src/montgomery.cpp) and
 * bignum_lock_stub_dup_0100B2E0 (src/gzip_montgomery_tail.cpp), declared in
 * montgomery.h. */
void bignum_lock_stub(void *obj, int unlock);

/* The sign flag type.  The original evidently declares the flag as a C++
 * `bool`: this egcs makes bool 4 bytes on the EE (lw/sw, like the retail
 * accesses) and gives it an alias set of its own, separate from the u32
 * limbs.  That is visible in the retail code: GCSE moves loads of the sign
 * flags across limb stores (mulTrunc, the Montgomery code), which it cannot
 * do for a u32 field.  GCC 14's bool is 1 byte, so the behaviour build keeps
 * a 4-byte u32 instead.  Every write of the flag stores 0 or 1, so both
 * builds hold the same 4 bytes. */
#ifdef BOOT_OLD_GCC
typedef bool BigNumSign;
#else
typedef u32 BigNumSign;
#endif

/* ------------------------------------------------------------------------ */
/* BigNumRep - the value object (operator new(0x18))                         */
/* ------------------------------------------------------------------------ */
struct BigNumRep {
    u32  unused00;   /* +0x00 never read or written by any bignum code        */
    s32  refcount;   /* +0x04 number of BigNum handles; 0 for stack temporaries */
    u32  nwords;     /* +0x08 significant limbs (0 == zero); kept normalised    */
                     /*       (no zero top limb) by setWord / trims            */
    u32  capacity;   /* +0x0C allocated limbs, always 0 or a power of two       */
    BigNumSign negative; /* +0x10 sign flag 0/1 (sign-magnitude), see BigNumSign */
    u32 *words;      /* +0x14 malloc'ed limb array, words[0] = least significant */

    /* --- inline constructors (never emitted out of line) ------------------ */

    /* new BigNumRep(): all fields zero (value 0, no buffer). */
    BigNumRep() : refcount(0), nwords(0), capacity(0), negative(0), words(0) {}

    /* Deep copy: refcount 0, same sign/length, exact-size buffer. */
    BigNumRep(const BigNumRep &o)
        : refcount(0), nwords(o.nwords), capacity(0), negative(o.negative), words(0)
    {
        if (nwords != 0) {
            reserve(nwords, false);
            memcpy(words, o.words, nwords * 4);
        }
    }

    /* Single-limb value.  Seen instantiated with v = 0 (montgomery divmod),
     * v = 1 (modInverse, divmod cores, Montgomery ctor) and v = 66001
     * (ImageLoader::verify's RSA exponent).  In every instance nwords is 1
     * (even for 0!) and negative is 0; the limb is written AFTER reserve()
     * as `negative ? -v : v` (sign re-read from memory: movz/movn).
     * negative(v < 0) is inferred - no negative instance exists.
     * `v` is assigned in the body (the same value as `negative ? -v : v`):
     * egcs then copies the inlined argument into a register that lives
     * across reserve() instead of substituting the constant, as the retail
     * code does ($s1 = 1 / 66001 before the call; imageloader edit). */
    explicit BigNumRep(int v)
        : refcount(0), nwords(1), capacity(0), negative(v < 0 /* ? */), words(0)
    {
        reserve(1, false);
        if (negative)
            v = -v;
        words[0] = v;
    }

    /* 0x01007190: import a big-endian byte string (callers: F_010043A8
     * sha512 digest -> bignum, F_010044A8 ImageLoader::verify for the
     * in-file modulus and signature). */
    BigNumRep(const u8 *bytes, int len);

    /* 0x01007148: GCC 2.9x __dt(this, flags): free(words); if (flags & 1)
     * operator delete(this).  Flag 3 = `delete rep`, 2 = stack object. */
    ~BigNumRep();

    /* --- refcount protocol (inline everywhere) --------------------------- */
    void addRef()  { bignum_lock_stub(this, 0); refcount++; }
    void release() { bignum_lock_stub(this, 1); if (--refcount <= 0) delete this; }

    /* -1 / 0 / +1 (0 for nwords == 0, whatever the sign flag says). */
    int  sign() const { return nwords ? (negative ? -1 : 1) : 0; }

    /* --- out-of-line members, this unit ----------------------------------- */
    BigNumRep &assign(const BigNumRep &o);                 /* 0x01007278 deep copy into existing buffer */
    BigNumRep &setInt(int v);                              /* 0x01007308 */
    int  cmpMag(const BigNumRep &o) const;                 /* 0x01007390 |this| <=> |o| : -1/0/+1 */
    int  cmp(const BigNumRep &o) const;                    /* 0x01007408 signed compare: -1/0/+1 */
    bool operator==(const BigNumRep &o) const;             /* 0x01007460 magnitude only, unused */
    bool operator!=(const BigNumRep &o) const;             /* 0x010074C0 magnitude only, unused */
    void swapMag(BigNumRep &o);                            /* 0x01007520 swaps +8..+0x14, not refcount */
    void reserve(u32 n, bool zeroFill);                    /* 0x01007568 grow to next pow2 >= n */
    void setWord(u32 idx, u32 val);                        /* 0x01007638 store limb + renormalise */
    u32  bitLength() const;                                /* 0x01007710 */
    BigNumRep &addMag(const BigNumRep &o);                 /* 0x01007788 |this| += |o| */
    BigNumRep &add(const BigNumRep &o);                    /* 0x01007888 signed this += o */
    BigNumRep &subMag(const BigNumRep &o);                 /* 0x01007988 |this| -= |o|, needs |this| >= |o| */
    BigNumRep &sub(const BigNumRep &o);                    /* 0x01007A60 signed this -= o */
    BigNumRep &mulTrunc(const BigNumRep &a, const BigNumRep &b, u32 maxBits); /* 0x01007B70 this = (a*b) mod 2^maxBits */
    BigNumRep &divmod(const BigNumRep &a, const BigNumRep &b, BigNumRep &r);  /* 0x010081B8 this = a/b, r = a%b */
    BigNumRep &divmodInPlace(const BigNumRep &b, BigNumRep &r);               /* 0x010083D0 this /= b, r = old this % b */

    /* --- out-of-line members, montgomery unit ------------------------------ */
    void shr1();           /* 0x01008B98 this >>= 1 */
    void shr(u32 n);       /* 0x01008C08 this >>= n */
    void shl1();           /* 0x01008D80 this <<= 1 */
    void shl(u32 n);       /* 0x01008E00 this <<= n */
};
static_assert(sizeof(BigNumRep) == 0x18, "BigNumRep is operator new(0x18)");

/* ------------------------------------------------------------------------ */
/* BigNum - copy-on-write handle (8 bytes)                                   */
/* ------------------------------------------------------------------------ */
class BigNum {
public:
    u32        unused00;   /* +0x00 never read or written (constructors skip it) */
    BigNumRep *rep;        /* +0x04 NULL == value zero                          */

    BigNum() : rep(0) {}
    explicit BigNum(BigNumRep *r) : rep(r) { if (rep) rep->addRef(); }
    /* `BigNum(1)`: new BigNumRep(v) then the (still null-checked) addref. */
    explicit BigNum(int v) : rep(new BigNumRep(v)) { if (rep) rep->addRef(); }
    /* Big-endian byte string: new BigNumRep(bytes, len) (0x01007190) then the
     * addref.  ImageLoader::verify builds the signature and modulus handles
     * with it (the allocation inside the constructor leaves no "new pending"
     * flag, unlike BigNum(new BigNumRep(...)); imageloader edit). */
    BigNum(const u8 *bytes, int len) : rep(new BigNumRep(bytes, len)) { if (rep) rep->addRef(); }
    BigNum(const BigNum &o) : rep(o.rep) { if (rep) rep->addRef(); }
    ~BigNum() { if (rep) rep->release(); }

    BigNum &operator=(const BigNum &o)
    {
        if (rep != o.rep) {
            if (rep) rep->release();
            rep = o.rep;
            if (rep) rep->addRef();
        }
        return *this;
    }

    bool isZero() const { return rep == 0 || rep->nwords == 0; }
    /* -1 / 0 / +1.  Two inline levels in the original: the inner result is
     * copied out of BigNumRep::sign's return value ($a0 -> $v0). */
    int  sign() const   { return rep ? rep->sign() : 0; }

    /* 0x01007128: weak out-of-line copy from the gcd/modInverse object file;
     * all out-of-line calls (that file and the Montgomery file) bind to it.
     * BigNum::mul / mulTrunc / divmod have the swap written out instead. */
    void swap(BigNum &o);

    /* Copy-on-write detach keeping the value: if shared, clone the rep into
     * a temporary handle and swap it in; the temporary then drops the old
     * reference. */
    void unique()
    {
        if (rep && rep->refcount != 1) {
            BigNum tmp(new BigNumRep(*rep));
            swap(tmp);
        }
    }

    BigNum &operator+=(const BigNum &b)
    {
        if (b.rep) {
            if (!rep) {
                rep = b.rep;            /* 0 + b: share b's rep */
                rep->addRef();
            } else {
                unique();
                rep->add(*b.rep);
            }
        }
        return *this;
    }

    /* The early returns of -= and %= are visible in the retail code: in
     * egcs's RTL inliner a parameter's constant equivalence (here `this` =
     * a frame address) is dropped at every copied label, and the label that
     * `if (...) return *this;` leaves behind makes the following `rep`
     * accesses go through the `this` register instead of the frame address
     * (see operator- below).  A plain if-block has no such label. */
    BigNum &operator-=(const BigNum &b)
    {
        if (!b.rep)
            return *this;
        if (!rep) {
            rep = new BigNumRep(*b.rep);       /* 0 - b: copy b and flip the sign */
            rep->addRef();
            rep->negative = !rep->negative;
        } else {
            unique();
            rep->sub(*b.rep);
        }
        return *this;
    }

    /* this %= d (d must be non-zero).  A stack rep receives a copy of the
     * dividend and becomes the (discarded) quotient; the remainder lands in
     * this->rep. */
    BigNum &operator%=(const BigNum &d)
    {
        if (!rep)
            return *this;
        unique();
        {
            BigNumRep q(*rep);
            q.divmodInPlace(*d.rep, *rep);
        }
        return *this;
    }

    BigNum &mul(const BigNum &a, const BigNum &b);                 /* 0x01007E00 this = a * b */
    BigNum &mulTrunc(const BigNum &a, const BigNum &b, u32 bits);  /* 0x01008000 this = a * b mod 2^bits */

    /* 0x010085D0 (montgomery unit): this = a / b, r = a % b; returns *this. */
    BigNum &divmod(const BigNum &a, const BigNum &b, BigNum &r);

    static BigNum gcd(const BigNum &a, const BigNum &b);            /* 0x01005BC0 (unused) */
    static BigNum modInverse(const BigNum &a, const BigNum &m);     /* 0x01006200 a^-1 mod m */
};
static_assert(sizeof(BigNum) == 8, "BigNum handle is 8 bytes");

/* Value operators as they appear inlined in modInverse / Montgomery code:
 *   a * b : a default-constructed temporary, .mul(a, b), then a copy of the
 *           returned reference into the result (the temporary is released).
 *   a - b : copy of a, -= b, returned by value. */
inline BigNum operator*(const BigNum &a, const BigNum &b)
{
    BigNum t;
    return t.mul(a, b);
}

inline BigNum operator-(const BigNum &a, const BigNum &b)
{
    BigNum r = a;           /* copy-initialised: closer to retail than r(a) */
    return r -= b;          /* the result is copied out of -='s reference to r
                             * (the same copy and release of r as `r -= b;
                             * return r;`, reached through the `this` register) */
}

/* Unreferenced weak duplicates of bignum_lock_stub (`jr ra; nop`) that end
 * object files A and E (identity inferred; see src/bignum.cpp).
 * Formerly F_01007140 / F_010081B0. */
void bignum_lock_stub_dup_01007140(void *obj, int unlock);  /* 0x01007140 */
void bignum_lock_stub_dup_010081B0(void *obj, int unlock);  /* 0x010081B0 */

#endif /* BOOT_BIGNUM_H */
