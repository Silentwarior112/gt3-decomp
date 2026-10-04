"""bignum group: units bignum and montgomery (src/bignum.cpp, src/montgomery.cpp,
include/bignum.h).  modpow 0x0100B238 is in the gzip unit (src/gzip_montgomery_tail.cpp)
and its specs are in the gzip group.

The oracles use BnModel, an exact model of BigNumRep including its quirks: limbs
above nwords are NOT always zero (the stub malloc leaves FRESH_WORD in a new
buffer, the test reps carry random garbage there), reserve() only zero-fills when
it reallocates, and setWord(idx >= nwords, 0) stores nothing - so addMag (and the
quotient accumulation in the division cores) can pull stale limbs into the value
when the sum has zero limbs below a non-zero one.  Signs follow the code: signed
compares in the division loops, the quotient sign a.sign ^ b.sign (divmod) or
0 ^ b.sign (divmodInPlace)."""
from .common import *

UNITS = ("bignum", "montgomery")

BN_GCD, BN_MODINV, BN_SWAP, BN_REP_DTOR = 0x01005BC0, 0x01006200, 0x01007128, 0x01007148
BN_REP_CTOR_BYTES, BN_ASSIGN, BN_SETINT, BN_CMPMAG = 0x01007190, 0x01007278, 0x01007308, 0x01007390
BN_CMP, BN_EQ, BN_NE, BN_SWAPMAG = 0x01007408, 0x01007460, 0x010074C0, 0x01007520
BN_RESERVE, BN_SETWORD, BN_BITLEN = 0x01007568, 0x01007638, 0x01007710
BN_ADDMAG, BN_ADD, BN_SUBMAG, BN_SUB = 0x01007788, 0x01007888, 0x01007988, 0x01007A60
BN_REP_MULTRUNC, BN_MUL, BN_MULTRUNC = 0x01007B70, 0x01007E00, 0x01008000
BN_REP_DIVMOD, BN_REP_DIVMOD_IP, BN_DIVMOD = 0x010081B8, 0x010083D0, 0x010085D0
BN_SHR1, BN_SHR, BN_SHL1, BN_SHL = 0x01008B98, 0x01008C08, 0x01008D80, 0x01008E00
MONT_DTOR, MONT_CTOR, MONT_MULMOD, MONT_POWMOD = 0x01008FC0, 0x01009128, 0x01009F68, 0x0100A5D0


# ================================================================== exact model of BigNumRep

class BnModel:
    """BigNumRep as the code sees it: w = the whole buffer (capacity limbs),
    nw = nwords, neg = negative, rc = refcount.  Each method is the C of
    src/bignum.cpp / src/montgomery.cpp / include/bignum.h step by step."""

    def __init__(self, w, nw, neg, rc=1):
        self.w, self.nw, self.neg, self.rc = list(w), nw, neg, rc

    @property
    def cap(self):
        return len(self.w)

    @staticmethod
    def of(rep):
        return BnModel(rep.words(), rep.nw, rep.neg, rep.refcount)

    @staticmethod
    def empty():                                    # new BigNumRep()
        return BnModel([], 0, 0, 0)

    @staticmethod
    def from_int(v):                                # BigNumRep(int): nwords 1 even for 0
        m = BnModel([], 1, 1 if v < 0 else 0, 0)
        m.reserve(1, False)
        m.w[0] = abs(v) & M32
        return m

    def copy(self):                                 # BigNumRep(const BigNumRep &)
        m = BnModel([], self.nw, self.neg, 0)
        if m.nw:
            m.reserve(m.nw, False)
            m.w[:m.nw] = self.w[:m.nw]
        return m

    def value(self):
        return sum(x << (32 * i) for i, x in enumerate(self.w[:self.nw]))

    def state(self):
        """what read_rep returns for it."""
        return (self.neg, self.value(), self.nw, self.cap, self.rc)

    # ---- bignum.cpp
    def reserve(self, n, zero):
        if self.cap < n:
            c = 1
            while c < n:
                c <<= 1
            buf = [FRESH_WORD] * c
            if self.cap:                            # `if (words)`
                buf[:self.nw] = self.w[:self.nw]
            if zero:
                buf[self.nw:] = [0] * (c - self.nw)
            self.w = buf

    def assign(self, o):
        if self.cap < o.nw:
            self.reserve(o.nw, False)
        n = o.nw
        self.neg = o.neg
        self.nw = n
        self.w[:n] = o.w[:n]
        self.w[n:] = [0] * (self.cap - n)

    def setInt(self, v):
        self.reserve(1, True)
        if v == 0:
            self.nw, self.w[0], self.neg = 0, 0, 0
        else:
            self.nw = 1
            self.neg = 1 if v < 0 else 0
            self.w[0] = abs(v) & M32

    def cmpMag(self, o):
        if self.nw < o.nw:
            return -1
        if o.nw < self.nw:
            return 1
        for i in range(self.nw - 1, -1, -1):
            if self.w[i] < o.w[i]:
                return -1
            if o.w[i] < self.w[i]:
                return 1
        return 0

    def cmp(self, o):
        if self.neg == o.neg:
            c = self.cmpMag(o)
            return -c if self.neg else c
        return -1 if self.neg else (1 if o.neg else 0)

    def eq(self, o):
        return self.nw == o.nw and self.w[:self.nw] == o.w[:o.nw]

    def swapMag(self, o):
        self.nw, o.nw = o.nw, self.nw
        self.w, o.w = o.w, self.w
        self.neg, o.neg = o.neg, self.neg

    def setWord(self, idx, val):
        if idx < self.nw:
            self.w[idx] = val
            if val == 0:
                while self.nw and self.w[self.nw - 1] == 0:
                    self.nw -= 1
        elif val:
            self.reserve(idx + 1, True)
            self.w[idx] = val
            self.nw = idx + 1

    def bitLength(self):
        for i in range(self.nw - 1, -1, -1):
            if self.w[i]:
                return 32 * i + self.w[i].bit_length()
        return 0

    def addMag(self, o):
        n = max(self.nw, o.nw)
        self.reserve(n, True)
        carry = 0
        for i in range(n + 1):
            t = carry + (self.w[i] if i < self.nw else 0) + (o.w[i] if i < o.nw else 0)
            self.setWord(i, t & M32)
            carry = t >> 32

    def subMag(self, o):
        borrow = 0
        for i in range(self.nw):                    # the ORIGINAL nwords
            s = ((o.w[i] if i < o.nw else 0) + borrow) & M32
            if s < borrow:
                continue
            x = self.w[i] if i < self.nw else 0
            d = (x - s) & M32
            borrow = 1 if x < d else 0
            self.setWord(i, d)

    def add(self, o):
        if self.neg == o.neg:
            self.addMag(o)
        elif o.cmpMag(self) > 0:
            t = o.copy()
            t.subMag(self)
            self.swapMag(t)
        else:
            self.subMag(o)

    def sub(self, o):
        if self.neg != o.neg:
            self.addMag(o)
        elif o.cmpMag(self) > 0:
            t = o.copy()
            t.subMag(self)
            self.swapMag(t)
            self.neg ^= 1
        else:
            self.subMag(o)

    def divmod(self, a, b, r):
        d, p = b.copy(), BnModel.from_int(1)
        r.assign(a)
        self.setInt(0)
        self._divloop(b, r, d, p)
        self.neg = 1 if a.neg ^ b.neg else 0

    def divmodInPlace(self, b, r):
        d, p = b.copy(), BnModel.from_int(1)
        self.swapMag(r)
        self.setInt(0)
        self._divloop(b, r, d, p)
        self.neg = 1 if self.neg ^ b.neg else 0

    def _divloop(self, b, r, d, p, limit=200000):
        n = 0
        while d.cmp(r) < 0:
            d.shl1()
            n += 1
            if n > limit:
                raise HarnessError("BnModel: division by zero does not terminate (as in the code)")
        p.shl(n)
        while b.cmp(r) <= 0:
            n = 0
            while r.cmp(d) < 0:
                d.shr1()
                n += 1
            p.shr(n)
            r.sub(d)
            self.add(p)

    # ---- montgomery.cpp (shifts)
    def shr1(self):
        if self.nw == 0:
            return
        carry = 0
        for i in range(self.nw - 1, -1, -1):
            v = self.w[i]
            self.w[i] = (v >> 1) | carry
            carry = (v << 31) & M32
        if self.w[self.nw - 1] == 0:
            self.nw -= 1

    def shr(self, n):
        if n == 0:
            return
        if n == 1:
            return self.shr1()
        ws, bs = n >> 5, n & 31
        if ws >= self.nw:
            self.neg, self.nw = 0, 0
            return
        w = self.w
        if bs == 0:
            w[0:self.nw - ws] = w[ws:self.nw]
            self.nw -= ws
            return
        low = M32 >> bs
        w[0] = w[ws] >> bs
        for j in range(ws + 1, self.nw):
            v = w[j]
            k = j - ws
            w[k - 1] = (w[k - 1] & low) | ((v << (32 - bs)) & M32)
            w[k] = (w[k] & ~low & M32) | (v >> bs)
        w[self.nw - ws - 1] &= low
        nn = self.nw - ws
        if w[nn - 1] != 0:
            self.nw = nn
        else:
            self.nw = 0 if nn == 1 else nn - 1

    def shl1(self):
        if self.nw == 0:
            return
        carry = 0
        for i in range(self.nw):
            v = self.w[i]
            self.w[i] = ((v << 1) | carry) & M32
            carry = v >> 31
        if carry:
            self.setWord(self.nw, 1)

    def shl(self, n):
        if n == 0:
            return
        if n == 1:
            return self.shl1()
        bs, ws = n & 31, n >> 5
        if bs == 0:
            self.reserve(self.nw + ws, False)
            self.w[ws:ws + self.nw] = self.w[0:self.nw]
            self.w[0:ws] = [0] * ws
            self.nw += ws
            return
        self.reserve(self.nw + ws + 1, True)
        hi = (M32 << bs) & M32
        lo = ~hi & M32
        w = self.w
        for i in range(self.nw - 1, -1, -1):
            v = w[i]
            w[i + ws + 1] = (w[i + ws + 1] & hi) | (v >> (32 - bs))
            w[i + ws] = (w[i + ws] & lo) | ((v << bs) & M32)
        w[ws] &= hi
        if ws:
            w[0:ws] = [0] * ws
        w[self.nw + ws] &= lo
        self.nw = self.nw + ws + (1 if w[self.nw + ws] else 0)


class HModel:
    """a BigNum handle: rep = BnModel or None."""

    def __init__(self, rep):
        self.rep = rep

    def is_zero(self):
        return self.rep is None or self.rep.nw == 0

    def detach_keep(self):                          # montgomery.cpp bn_detachKeep
        if self.rep is None:
            self.rep = BnModel.empty()
            self.rep.rc = 1
        elif self.rep.rc != 1:
            c = self.rep.copy()
            c.rc = 1
            self.rep.rc -= 1
            self.rep = c

    def set_new_int(self, v):                       # h = BigNum(v)
        m = BnModel.from_int(v)
        m.rc = 1
        if self.rep is not None:
            self.rep.rc -= 1
        self.rep = m

    def state(self):
        return self.rep.state() if self.rep is not None else None


def handle_divmod(q, a, b, r):
    """BigNum::divmod 0x010085D0 (q = a / b, r = a % b)."""
    if a.is_zero():
        r.set_new_int(0)
        q.set_new_int(0)
        return
    q.detach_keep()
    r.detach_keep()
    q.rep.divmod(a.rep, b.rep, r.rep)


# ================================================================== rep-level specs

def rep_case(rng, fn, name, nargs, extra=(), ret="ptr", maxlimbs=12, pos=False, alias=0.08, model=None):
    """a BigNumRep member: this + (nargs - 1) more reps + extra integer args.
    model(ms, *extra) applies the operation to BnModels (None: no oracle) and
    returns the expected return value or None."""
    reps = [rand_rep(rng, maxlimbs, not pos) for _ in range(nargs)]
    aliased = nargs > 1 and rng.random() < alias

    def run(t):
        ps = [r.put(t) for r in reps]
        if aliased:
            ps[1] = ps[0]
        r = t.call(fn, *(ps + list(extra)), ret=ret)
        after = [read_rep(t, p) for p in ps[:nargs if not aliased else 1]]
        for k, st in enumerate(after):
            t.obs("rep%d" % k, st)
        if model and not aliased:
            ms = [BnModel.of(x) for x in reps]
            want_r = model(ms, *extra)
            for k, (st, m) in enumerate(zip(after, ms)):
                t.check(st == m.state(), "rep%d after %s: %s, model %s" % (k, name, st, m.state()))
            if want_r is not None:
                t.check(s64(r) == want_r, "%s returned %d, model %d" % (name, s64(r), want_r))
    return Case("%s(%s%s%s)" % (name, ", ".join(map(repr, reps)), "".join(", %s" % x for x in extra),
                                ", aliased" if aliased else ""), run)


def gen_bn_addmag(rng, i, n):
    return rep_case(rng, BN_ADDMAG, "addMag", 2, model=lambda m: m[0].addMag(m[1]))


def gen_bn_add(rng, i, n):
    return rep_case(rng, BN_ADD, "add", 2, model=lambda m: m[0].add(m[1]))


def gen_bn_submag(rng, i, n):
    return rep_case(rng, BN_SUBMAG, "subMag", 2, model=lambda m: m[0].subMag(m[1]))


def gen_bn_sub(rng, i, n):
    return rep_case(rng, BN_SUB, "sub", 2, model=lambda m: m[0].sub(m[1]))


def gen_bn_cmp(rng, i, n):
    fn = [BN_CMPMAG, BN_CMP, BN_EQ, BN_NE][i % 4]
    nm = ["cmpMag", "cmp", "operator==", "operator!="][i % 4]
    a = rand_rep(rng, 8)
    b = rand_rep(rng, 8, value=a.value if rng.random() < 0.4 else None)
    if rng.random() < 0.3:
        b.neg = a.neg

    def run(t):
        pa, pb = a.put(t), b.put(t)
        r = t.call(fn, pa, pb)
        ma, mb = BnModel.of(a), BnModel.of(b)
        want = {BN_CMPMAG: ma.cmpMag, BN_CMP: ma.cmp, BN_EQ: lambda o: int(ma.eq(o)),
                BN_NE: lambda o: int(not ma.eq(o))}[fn](mb)
        t.check(s64(r) == want, "%s = %d, model %d" % (nm, s64(r), want))
    return Case("%s(%r, %r)" % (nm, a, b), run)


def gen_bn_bitlen(rng, i, n):
    a = rand_rep(rng, 16)

    def run(t):
        r = t.call(BN_BITLEN, a.put(t))
        t.check(r == a.value.bit_length(), "bitLength")
    return Case("bitLength(%r)" % a, run)


def gen_bn_assign(rng, i, n):
    return rep_case(rng, BN_ASSIGN, "assign", 2, model=lambda m: m[0].assign(m[1]))


def gen_bn_setint(rng, i, n):
    v = rng.choice([0, 1, -1, 0x7FFFFFFF, -0x80000000, rng.randrange(-1 << 31, 1 << 31)])
    return rep_case(rng, BN_SETINT, "setInt", 1, extra=(v & M64,), model=lambda m, x: m[0].setInt(v))


def gen_bn_swapmag(rng, i, n):
    return rep_case(rng, BN_SWAPMAG, "swapMag", 2, ret="void", model=lambda m: m[0].swapMag(m[1]))


def gen_bn_reserve(rng, i, n):
    k = rng.choice([0, 1, 2, 3, 5, 8, 9, 17, rng.randrange(40)])
    return rep_case(rng, BN_RESERVE, "reserve", 1, extra=(k, rng.randrange(2)), ret="void",
                    model=lambda m, k_, z: m[0].reserve(k_, z))


def gen_bn_setword(rng, i, n):
    r = rand_rep(rng, 8)
    idx = rng.choice([0, max(0, r.nw - 1), r.nw, r.nw + 1, r.nw + 5, rng.randrange(r.nw + 1)])
    val = rng.choice([0, 1, rng.getrandbits(32), M32])
    return rep_case(rng, BN_SETWORD, "setWord", 1, extra=(idx, val), ret="void",
                    model=lambda m, i_, v_: m[0].setWord(i_, v_))


def gen_bn_shift(rng, i, n):
    fn, nm = [(BN_SHR1, "shr1"), (BN_SHR, "shr"), (BN_SHL1, "shl1"), (BN_SHL, "shl")][i % 4]
    extra = ()
    if fn in (BN_SHR, BN_SHL):
        extra = (rng.choice([0, 1, 2, 31, 32, 33, 63, 64, 65, rng.randrange(0, 400)]),)
    a = rand_rep(rng, 12)
    if fn == BN_SHL and extra[0] >= 32 and a.nw == 0 and rng.random() < 0.7:
        a = rand_rep(rng, 12, value=rng.getrandbits(40) | 1)       # keep most shl cases non-zero

    def run(t):
        p = a.put(t)
        t.call(fn, p, *extra, ret="void")
        st = read_rep(t, p)
        t.obs("rep", st)
        m = BnModel.of(a)
        {BN_SHR1: m.shr1, BN_SHR: m.shr, BN_SHL1: m.shl1, BN_SHL: m.shl}[fn](*extra)
        t.check(st == m.state(), "%s: %s, model %s" % (nm, st, m.state()))
    return Case("%s(%r%s)" % (nm, a, "".join(", %d" % x for x in extra)), run)


def gen_bn_multrunc_rep(rng, i, n):
    a, b = rand_rep(rng, 10), rand_rep(rng, 10)
    bits = rng.choice([0, 1, 31, 32, 33, 64, a.value.bit_length() + b.value.bit_length(), rng.randrange(0, 700)])
    this = rand_rep(rng, 6)

    def run(t):
        pt, pa, pb = this.put(t), a.put(t), b.put(t)
        t.call(BN_REP_MULTRUNC, pt, pa, pb, bits, ret="ptr")
        neg, v, nw, cap, rc = read_rep(t, pt)
        t.obs("this", (neg, v, nw, cap))
        t.check(v == (a.value * b.value) % (1 << bits) if bits else v == 0, "mulTrunc value")
        t.check(neg == (1 if a.neg ^ b.neg else 0) and (nw == 0 or v >> (32 * nw - 32)), "mulTrunc sign / nwords")
    return Case("BigNumRep::mulTrunc(%r * %r mod 2^%d)" % (a, b, bits), run)


def gen_bn_mul(rng, i, n):
    fn = BN_MUL if i % 2 == 0 else BN_MULTRUNC
    A, B, C = rand_handle_rep(rng, 8, False), rand_handle_rep(rng, 8, False), rand_handle_rep(rng, 4, False)
    bits = rng.randrange(0, 600)
    alias = rng.choice([None, None, None, "this=a", "a=b"])

    def run(t):
        ra = A.put(t) if A else 0
        rb = B.put(t) if B else 0
        rc = C.put(t) if C else 0
        ha, hb, hc = put_handle(t, ra), put_handle(t, rb), put_handle(t, rc)
        if alias == "this=a":
            hc = ha
        elif alias == "a=b":
            hb = ha
        args = [hc, ha, hb] + ([bits] if fn == BN_MULTRUNC else [])
        t.call(fn, *args, ret="ptr")
        res = read_handle(t, hc)
        t.obs("result", res)
        av = sval(A) if A else 0
        bv = (sval(A) if A else 0) if alias == "a=b" else (sval(B) if B else 0)
        if alias != "this=a" and res is not None and av and bv:
            want = abs(av * bv) if fn == BN_MUL else abs(av * bv) % (1 << bits)
            t.check(res[1] == want, "product")
    return Case("BigNum::%s(%r, %r%s%s)" % ("mul" if fn == BN_MUL else "mulTrunc", A, B,
                                            ", %d bits" % bits if fn == BN_MULTRUNC else "",
                                            ", " + alias if alias else ""), run)


def gen_bn_divmod_rep(rng, i, n):
    fn = BN_REP_DIVMOD if i % 2 == 0 else BN_REP_DIVMOD_IP
    a = rand_rep(rng, 8)
    b = rand_rep(rng, 6, False, value=rand_big(rng, 6) or 1)
    r = rand_rep(rng, 4)
    this = rand_rep(rng, 4)

    def run(t):
        pa, pb, pr, pt = a.put(t), b.put(t), r.put(t), this.put(t)
        if fn == BN_REP_DIVMOD:
            t.call(fn, pt, pa, pb, pr, ret="ptr")
        else:
            pt = pa                                  # this = the dividend
            t.call(fn, pt, pb, pr, ret="ptr")
        q, rem = read_rep(t, pt), read_rep(t, pr)
        t.obs("quotient", q)
        t.obs("remainder", rem)
        ma, mb, mr, mt = BnModel.of(a), BnModel.of(b), BnModel.of(r), BnModel.of(this)
        if fn == BN_REP_DIVMOD:
            mt.divmod(ma, mb, mr)
        else:
            mt = ma
            mt.divmodInPlace(mb, mr)
        t.check(q == mt.state() and rem == mr.state(), "quotient %s remainder %s, model %s %s" % (
            q, rem, mt.state(), mr.state()))
        if not a.neg:
            t.check(rem[1] == a.value % b.value, "remainder value")     # the remainder never picks up stale limbs
    return Case("BigNumRep::%s(%r / %r)" % ("divmod" if fn == BN_REP_DIVMOD else "divmodInPlace", a, b), run)


def gen_bn_divmod(rng, i, n):
    a = rand_handle_rep(rng, 8)
    b = rand_rep(rng, 6, False, value=rand_big(rng, 6) or 1)
    q0, r0 = rand_handle_rep(rng, 4), rand_handle_rep(rng, 4)

    def run(t):
        ha = put_handle(t, a.put(t) if a else 0)
        hb = put_handle(t, b.put(t))
        hq = put_handle(t, q0.put(t) if q0 else 0)
        hr = put_handle(t, r0.put(t) if r0 else 0)
        t.call(BN_DIVMOD, hq, ha, hb, hr, ret="ptr")
        q, rem = read_handle(t, hq), read_handle(t, hr)
        t.obs("q", q)
        t.obs("r", rem)
        mq, mr = HModel(BnModel.of(q0) if q0 else None), HModel(BnModel.of(r0) if r0 else None)
        handle_divmod(mq, HModel(BnModel.of(a) if a else None), HModel(BnModel.of(b)), mr)
        t.check(q == mq.state() and rem == mr.state(), "q %s r %s, model %s %s" % (q, rem, mq.state(), mr.state()))
    return Case("BigNum::divmod(%r / %r)" % (a, b), run)


def gen_bn_swap(rng, i, n):
    a, b = rand_handle_rep(rng, 3), rand_handle_rep(rng, 3)

    def run(t):
        ha = put_handle(t, a.put(t) if a else 0)
        hb = put_handle(t, b.put(t) if b else 0)
        t.call(BN_SWAP, ha, hb, ret="void")
    return Case("BigNum::swap", run)


def gen_bn_rep_bytes(rng, i, n):
    L = rng.choice([0, 1, 2, 3, 4, 5, 64, 128, 129, rng.randrange(0, 140)])
    data = bytes(rng.randrange(256) for _ in range(L))
    if L and rng.random() < 0.3:
        z = rng.randrange(L + 1)
        data = b"\0" * z + data[z:]
    off = rng.randrange(16)

    def run(t):
        this = t.halloc(0x18, b"\xCD" * 0x18)
        p = t.put(data, 16, off)
        t.call(BN_REP_CTOR_BYTES, this, p, L, ret="ptr")
        rep = read_rep(t, this)
        t.obs("rep", rep)
        t.check(rep[1] == int.from_bytes(data, "big") and rep[0] == 0, "BE import")
    return Case("BigNumRep(bytes, %d)" % L, run)


def gen_bn_rep_dtor(rng, i, n):
    r = rand_rep(rng, 6)
    flags = rng.choice([2, 3])

    def run(t):
        t.call(BN_REP_DTOR, r.put(t), flags, ret="void")
    return Case("~BigNumRep(%r, flags %d)" % (r, flags), run)


def gen_bn_gcd(rng, i, n):
    g = rand_big(rng, 3) or 1
    a = rand_big(rng, 6) * (g if rng.random() < 0.5 else 1)
    b = rand_big(rng, 6) * (g if rng.random() < 0.5 else 1)
    A = None if a == 0 and rng.random() < 0.5 else rand_rep(rng, 12, False, value=a)
    B = None if b == 0 and rng.random() < 0.5 else rand_rep(rng, 12, False, value=b)

    def run(t):
        slot = put_handle(t, 0x5A5A5A5A)
        ha = put_handle(t, A.put(t) if A else 0)
        hb = put_handle(t, B.put(t) if B else 0)
        t.call(BN_GCD, slot, ha, hb, ret="ptr")
        res = read_handle(t, slot)
        t.obs("gcd", res)
        t.check((res[1] if res else 0) == math.gcd(a, b), "gcd")
    return Case("BigNum::gcd(0x%X, 0x%X)" % (a, b), run)


def gen_bn_modinv(rng, i, n):
    nl = rng.choice([1, 1, 2, 3, 4, rng.randrange(1, 9)])
    m = rng.getrandbits(32 * nl) | 2
    if rng.random() < 0.6:
        m |= 1
    a = rng.randrange(1, m)
    A, Mo = rand_rep(rng, 8, False, value=a), rand_rep(rng, 8, False, value=m)

    def run(t):
        slot = put_handle(t, 0x5A5A5A5A)
        ha, hm = put_handle(t, A.put(t)), put_handle(t, Mo.put(t))
        t.call(BN_MODINV, slot, ha, hm, ret="ptr")
        res = read_handle(t, slot)
        t.obs("inverse", res)
        if math.gcd(a, m) == 1:
            t.check(res[1] == pow(a, -1, m), "modInverse")
    return Case("BigNum::modInverse(0x%X mod 0x%X)" % (a, m), run)


def gen_bn_multrunc_alias(rng, i, n):
    """BigNumRep::mulTrunc with `this` aliasing a and/or b, and a == b.  The
    code clears this first, so an aliased operand reads as zero limbs; the
    signs and a.words are read after the clear (egcs's PRE hoists those loads
    to the loop entry, the C reads them per row / at the end: the same values
    as nothing in the loop writes them).  No oracle: retail and rebuilt are
    compared."""
    alias = ("this=a", "this=b", "this=a=b", "a=b")[i % 4]
    a, b = rand_rep(rng, 8), rand_rep(rng, 8)
    bits = rng.choice([0, 1, 31, 32, 33, 64, a.value.bit_length() + b.value.bit_length(), rng.randrange(0, 600)])
    this = rand_rep(rng, 6)

    def run(t):
        pa, pb, pt = a.put(t), b.put(t), this.put(t)
        if alias == "this=a":
            pt = pa
        elif alias == "this=b":
            pt = pb
        elif alias == "this=a=b":
            pt = pb = pa
        else:
            pb = pa
        t.call(BN_REP_MULTRUNC, pt, pa, pb, bits, ret="ptr")
        for nm, p in (("this", pt), ("a", pa), ("b", pb)):
            t.obs(nm, read_rep(t, p))
        if alias == "a=b":
            neg, v, nw, cap, rc = read_rep(t, pt)
            t.check(v == (a.value * a.value) % (1 << bits) if bits else v == 0, "square value")
            t.check(neg == 0, "square sign")
    return Case("BigNumRep::mulTrunc(%r, %r mod 2^%d, %s)" % (a, b, bits, alias), run)


def gen_bn_mul_alias(rng, i, n):
    """BigNum::mul / mulTrunc with the result handle aliasing b or both
    operands, shared result reps (refcount 2: the written-out detach swaps
    in a fresh rep) and zero operands.  No oracle where this is an operand."""
    fn = BN_MUL if i % 2 == 0 else BN_MULTRUNC
    alias = ("this=b", "this=a=b", "this=a", None)[(i // 2) % 4]
    A, B, C = rand_handle_rep(rng, 6, False), rand_handle_rep(rng, 6, False), rand_handle_rep(rng, 4, False)
    for R in (A, B, C):
        if R is not None and rng.random() < 0.4:
            R.refcount = 2
    bits = rng.randrange(0, 400)

    def run(t):
        ha = put_handle(t, A.put(t) if A else 0)
        hb = put_handle(t, B.put(t) if B else 0)
        hc = put_handle(t, C.put(t) if C else 0)
        if alias == "this=b":
            hc = hb
        elif alias == "this=a=b":
            hb = hc = ha
        elif alias == "this=a":
            hc = ha
        old = t.r32(hc + 4)
        t.call(fn, *([hc, ha, hb] + ([bits] if fn == BN_MULTRUNC else [])), ret="ptr")
        for nm, h in (("result", hc), ("a", ha), ("b", hb)):
            t.obs(nm, read_handle(t, h))
        t.obs("old result rep", read_rep(t, old))                      # kept alive by its other reference
    return Case("BigNum::%s(%r, %r, this %r%s, %s)" % ("mul" if fn == BN_MUL else "mulTrunc", A, B, C,
                                                    ", %d bits" % bits if fn == BN_MULTRUNC else "", alias), run)


def gen_bn_modinv_edges(rng, i, n):
    """modInverse outside the Montgomery use: a >= m (the first quotient is 0,
    so x1 * q is a null handle and operator-='s early return runs), a == m or a
    multiple of it, a == 0 (null handle or an nwords-0 rep: the loop never
    runs), tiny moduli (1, 2, 3) and shared input reps."""
    kind = i % 6
    m = rng.choice([1, 2, 3, 5, rng.getrandbits(32) | 1, rng.getrandbits(64) | 1, rng.getrandbits(96)]) or 7
    if kind == 0:
        a = m + rng.randrange(1, 1 << 40)                              # a > m
    elif kind == 1:
        a = m * rng.randrange(1, 5)                                    # a == k * m
    elif kind == 2:
        a = 0
    elif kind == 3:
        a = rng.randrange(1, m + 1)
    else:
        a = rng.getrandbits(rng.choice([8, 33, 70, 130])) or 1
    null_a = a == 0 and rng.random() < 0.5
    rc = (rng.choice([1, 2]), rng.choice([1, 2]))

    def run(t):
        slot = put_handle(t, 0x5A5A5A5A)
        ha = put_handle(t, 0 if null_a else Rep(a, refcount=rc[0]).put(t))
        hm = put_handle(t, Rep(m, refcount=rc[1]).put(t))
        t.call(BN_MODINV, slot, ha, hm, ret="ptr", maxsteps=60000000)
        res = read_handle(t, slot)
        t.obs("inverse", res)
        t.obs("a", read_handle(t, ha))
        t.obs("m", read_handle(t, hm))
        if math.gcd(a, m) == 1 and m > 1:
            t.check(res is not None and res[1] == pow(a, -1, m), "modInverse")
    return Case("BigNum::modInverse(0x%X%s mod 0x%X, refcounts %s)" % (a, " (null)" if null_a else "", m, rc), run)


# ================================================================== Montgomery

def mont_setup(t, N):
    ctx = t.alloc(0x34, 16)
    hn = put_handle(t, Rep(N, 0, refcount=1).put(t))
    t.call(MONT_CTOR, ctx, hn, ret="ptr")
    return ctx, hn


def read_mont(t, ctx):
    return tuple(read_handle(t, ctx + 8 * k) for k in range(6)) + (t.r32(ctx + 0x30),)


def gen_mont_ctor(rng, i, n):
    bits = rng.choice([2, 3, 8, 31, 32, 33, 64, 65, rng.randrange(2, 200)])
    N = rand_odd_modulus(rng, bits)

    def run(t):
        ctx, hn = mont_setup(t, N)
        st = read_mont(t, ctx)
        t.obs("context", st)
        k = st[6]
        R = 1 << k
        t.check(R >= N > R >> 1 or N == 1, "R")
        t.check(st[1][1] == pow(R, -1, N) and st[3][1] == (-pow(N, -1, R)) % R, "Rinv / N'")
        t.call(MONT_DTOR, ctx, 2, ret="void")
    return Case("Montgomery(N = 0x%X, %d bits) + ~Montgomery" % (N, bits), run)


def gen_mont_mulmod(rng, i, n):
    bits = rng.choice([3, 32, 33, 64, rng.randrange(3, 160)])
    N = rand_odd_modulus(rng, bits)
    x, y = rng.randrange(N), rng.randrange(N)
    square = rng.random() < 0.25

    def run(t):
        ctx, hn = mont_setup(t, N)
        hx = put_handle(t, Rep(x).put(t) if x else 0)
        hy = hx if square else put_handle(t, Rep(y).put(t) if y else 0)
        t.call(MONT_MULMOD, ctx, hx, hy, ret="void")
        res = read_handle(t, hx)
        t.obs("x", res)
        k = t.r32(ctx + 0x30)
        yy = x if square else y
        t.check((res[1] if res else 0) == x * yy * pow(1 << k, -1, N) % N, "mulmod")
        t.call(MONT_DTOR, ctx, 2, ret="void")
    return Case("Montgomery::mulmod(0x%X * 0x%X mod 0x%X%s)" % (x, x if square else y, N,
                                                                 ", squaring" if square else ""), run)


def gen_mont_powmod(rng, i, n):
    bits = rng.choice([3, 32, 64, rng.randrange(3, 130)])
    N = rand_odd_modulus(rng, bits)
    base = rng.randrange(N)
    e = rng.choice([1, 2, 3, 66001, 65537, rng.randrange(1, 1 << 20)])

    def run(t):
        ctx, hn = mont_setup(t, N)
        slot = put_handle(t, 0x5A5A5A5A)
        hb = put_handle(t, Rep(base).put(t) if base else 0)
        he = put_handle(t, Rep(e).put(t))
        t.call(MONT_POWMOD, slot, ctx, hb, he, ret="ptr")
        res = read_handle(t, slot)
        t.obs("result", res)
        t.check((res[1] if res else 0) == pow(base, e, N), "powmod")
        t.call(MONT_DTOR, ctx, 2, ret="void")
    return Case("Montgomery::powmod(0x%X ^ %d mod 0x%X)" % (base, e, N), run)


# the out-of-line copies of the refcount hook bignum_lock_stub (`jr $ra; nop`:
# the lock is compiled out); a spec checks they touch nothing
BN_LOCK_STUBS = [0x01007140, 0x010081B0, 0x01008B90]


def gen_bn_lock_stubs(rng, i, n):
    f = BN_LOCK_STUBS[i % len(BN_LOCK_STUBS)]
    obj, unlock = rng.getrandbits(32), rng.choice([0, 1, rng.getrandbits(32)])
    data = junk(rng, 0x18)

    def run(t):
        p = t.halloc(0x18, data) if obj & 1 else obj
        t.call(f, p, unlock, ret="void")
        t.obs("object", t.read(p, 0x18) if obj & 1 else None)
    return Case("bignum_lock_stub copy 0x%08X(%s, %d)" % (f, "heap object" if obj & 1 else "0x%X" % obj, unlock), run)


SPECS = [
    Spec("bn_lock_stubs", BN_LOCK_STUBS, gen_bn_lock_stubs, 12, "empty refcount hooks: no effect"),
    Spec("bn_rep_from_bytes", [BN_REP_CTOR_BYTES], gen_bn_rep_bytes, 200),
    Spec("bn_rep_dtor", [BN_REP_DTOR], gen_bn_rep_dtor, 50),
    Spec("bn_assign", [BN_ASSIGN], gen_bn_assign, 200),
    Spec("bn_setint", [BN_SETINT], gen_bn_setint, 200),
    Spec("bn_compare", [BN_CMPMAG, BN_CMP, BN_EQ, BN_NE], gen_bn_cmp, 300),
    Spec("bn_swapmag", [BN_SWAPMAG], gen_bn_swapmag, 50),
    Spec("bn_reserve", [BN_RESERVE], gen_bn_reserve, 200),
    Spec("bn_setword", [BN_SETWORD], gen_bn_setword, 200),
    Spec("bn_bitlength", [BN_BITLEN], gen_bn_bitlen, 200),
    Spec("bn_addmag", [BN_ADDMAG], gen_bn_addmag, 200, "BnModel oracle (stale-limb quirk)"),
    Spec("bn_add", [BN_ADD], gen_bn_add, 200, "BnModel oracle"),
    Spec("bn_submag", [BN_SUBMAG], gen_bn_submag, 200, "BnModel oracle"),
    Spec("bn_sub", [BN_SUB], gen_bn_sub, 200, "BnModel oracle"),
    Spec("bn_shift", [BN_SHR1, BN_SHR, BN_SHL1, BN_SHL], gen_bn_shift, 300, "BnModel oracle"),
    Spec("bn_rep_multrunc", [BN_REP_MULTRUNC], gen_bn_multrunc_rep, 150),
    Spec("bn_mul", [BN_MUL, BN_MULTRUNC], gen_bn_mul, 150),
    Spec("bn_rep_multrunc_alias", [BN_REP_MULTRUNC], gen_bn_multrunc_alias, 80, "this aliasing a / b, a == b"),
    Spec("bn_mul_alias", [BN_MUL, BN_MULTRUNC], gen_bn_mul_alias, 80, "result handle = b / a = b, shared reps"),
    Spec("bn_rep_divmod", [BN_REP_DIVMOD, BN_REP_DIVMOD_IP], gen_bn_divmod_rep, 100, "BnModel oracle"),
    Spec("bn_divmod", [BN_DIVMOD], gen_bn_divmod, 100, "BnModel oracle"),
    Spec("bn_swap", [BN_SWAP], gen_bn_swap, 20),
    Spec("bn_gcd", [BN_GCD], gen_bn_gcd, 60),
    Spec("bn_modinverse", [BN_MODINV], gen_bn_modinv, 60),
    Spec("bn_modinverse_edges", [BN_MODINV], gen_bn_modinv_edges, 36, "a >= m, a == 0 / k*m, tiny m, shared reps"),
    Spec("mont_ctor", [MONT_CTOR, MONT_DTOR], gen_mont_ctor, 40),
    Spec("mont_mulmod", [MONT_MULMOD], gen_mont_mulmod, 40),
    Spec("mont_powmod", [MONT_POWMOD], gen_mont_powmod, 30),
]
