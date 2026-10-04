"""sha512 group: unit sha512 (src/sha512.cpp).  (sha512_digest_to_bignum 0x010043A8 is
in the imageloader unit: group gameflow.)"""
from .common import *

UNITS = ("sha512",)

SHA_CMP, SHA_STORE_BE, SHA_INIT, SHA_LOAD_BE64 = 0x010010F0, 0x01001140, 0x010011D0, 0x010012B8
SHA_TRANSFORM, SHA_RESET, SHA_UPDATE, SHA_PAD = 0x01001320, 0x01003280, 0x010032B8, 0x010033C8
SHA_BITLEN, SHA_FINAL = 0x01003468, 0x010034E0


def _icbrt(n):
    x = 1 << ((n.bit_length() + 2) // 3)
    while True:
        y = (2 * x + n // (x * x)) // 3
        if y >= x:
            break
        x = y
    while x * x * x > n:
        x -= 1
    while (x + 1) ** 3 <= n:
        x += 1
    return x


def _primes(k):
    out, c = [], 2
    while len(out) < k:
        if all(c % p for p in out if p * p <= c):
            out.append(c)
        c += 1
    return out


# FIPS 180-2 constants derived from first principles (not copied from the binary)
SHA_K = [_icbrt(p << 192) & M64 for p in _primes(80)]
SHA_IV = [math.isqrt(p << 128) & M64 for p in _primes(8)]


def _rotr(x, n):
    return ((x >> n) | (x << (64 - n))) & M64


def sha512_compress(state, block):
    W = list(struct.unpack(">16Q", block))
    for j in range(16, 80):
        s0 = _rotr(W[j - 15], 1) ^ _rotr(W[j - 15], 8) ^ (W[j - 15] >> 7)
        s1 = _rotr(W[j - 2], 19) ^ _rotr(W[j - 2], 61) ^ (W[j - 2] >> 6)
        W.append((W[j - 16] + s0 + W[j - 7] + s1) & M64)
    a, b, c, d, e, f, g, h = state
    for j in range(80):
        S1 = _rotr(e, 14) ^ _rotr(e, 18) ^ _rotr(e, 41)
        ch = (e & f) ^ (~e & g)
        t1 = (h + S1 + ch + SHA_K[j] + W[j]) & M64
        S0 = _rotr(a, 28) ^ _rotr(a, 34) ^ _rotr(a, 39)
        mj = (a & b) ^ (a & c) ^ (b & c)
        t2 = (S0 + mj) & M64
        h, g, f, e, d, c, b, a = g, f, e, (d + t1) & M64, c, b, a, (t1 + t2) & M64
    return [(x + y) & M64 for x, y in zip(state, (a, b, c, d, e, f, g, h))]


class ShaCtx:
    """Python model of SHA512Ctx {u64 total_bits; u64 state[8]; u8 *buf_pos; u8 block[128]}."""
    SIZE = 0xD0

    def __init__(self, total, state, k, block):
        self.total, self.state, self.k, self.block = total, list(state), k, bytearray(block)

    def pack(self, base):
        return struct.pack("<Q8QI", self.total, *self.state, base + 0x4C + self.k) + bytes(self.block) + b"\0" * 4

    @staticmethod
    def unpack(bs, base):
        total, *rest = struct.unpack_from("<Q8QI", bs)
        return ShaCtx(total, rest[:8], rest[8] - (base + 0x4C), bs[0x4C:0xCC])

    def update(self, data):
        self.total = (self.total + (len(data) << 3)) & M64
        if self.k > 0:
            space = 0x80 - self.k
            if len(data) < space:
                self.block[self.k:self.k + len(data)] = data
                self.k += len(data)
                return
            self.block[self.k:0x80] = data[:space]
            data = data[space:]
            self.state = sha512_compress(self.state, bytes(self.block))
            self.k = 0
        while len(data) >= 0x80:
            self.state = sha512_compress(self.state, data[:0x80])
            data = data[0x80:]
        if data:
            self.block[:len(data)] = data
            self.k = len(data)

    def pad(self):
        self.block[self.k] = 0x80
        self.k += 1
        left = 0x80 - self.k
        self.block[self.k:] = b"\0" * left
        if left < 16:
            self.state = sha512_compress(self.state, bytes(self.block))
            self.k = 0
            self.block[:] = b"\0" * 0x80
        self.block[0x78:0x80] = struct.pack(">Q", self.total)

    def final(self):
        self.pad()
        self.state = sha512_compress(self.state, bytes(self.block))
        out = list(self.state)
        self.total, self.state, self.k = 0, list(SHA_IV), 0
        return out


def rand_state(rng):
    return [rng.getrandbits(64) for _ in range(8)]


def rand_ctx(rng):
    k = rng.choice([0, 0, 1, 111, 112, 113, 127, rng.randrange(128)])
    total = rng.choice([rng.getrandbits(64), rng.randrange(1 << 20) * 8 + k * 8])
    return ShaCtx(total, rand_state(rng), k, junk(rng, 0x80, 0.1))


def gen_sha_compare(rng, i, n):
    a = rand_state(rng)
    b = list(a)
    k = rng.randrange(9)
    if k < 8:
        b[k] = rng.choice([a[k] + 1, a[k] - 1, rng.getrandbits(64), a[k] ^ (1 << 63)]) & M64
    want = 0
    for x, y in zip(a, b):
        if x != y:
            want = -1 if x < y else 1
            break

    def run(t):
        pa = t.put(struct.pack("<8Q", *a), 16, 8 * (k & 1))
        pb = t.put(struct.pack("<8Q", *b), 16)
        r = t.call(SHA_CMP, pa, pb)
        t.check(s64(r) == want, "digest_compare")
    return Case("sha512_digest_compare(first difference at word %d)" % k, run)


def gen_sha_store_be(rng, i, n):
    st = rand_state(rng)
    off = rng.randrange(16)
    fill = junk(rng, 80, 0)

    def run(t):
        o = t.put(fill, 16, off)
        s = t.put(struct.pack("<8Q", *st), 16)
        t.call(SHA_STORE_BE, o, s, ret="void")
        t.check(t.read(o, 64) == struct.pack(">8Q", *st), "store_digest_be")
    return Case("sha512_store_digest_be(out %% 16 = %d)" % off, run)


def gen_sha_init(rng, i, n):
    fill = junk(rng, 72, 0)

    def run(t):
        s = t.put(fill, 16, 8 * (i & 1))
        t.call(SHA_INIT, s, ret="void")
        t.check(list(struct.unpack("<8Q", t.read(s, 64))) == SHA_IV, "init_state")
    return Case("sha512_init_state", run)


def gen_sha_load(rng, i, n):
    off = i % 16 if i < 32 else rng.randrange(16)
    data = junk(rng, 24, 0.1)

    def run(t):
        p = t.put(data, 16)
        r = t.call(SHA_LOAD_BE64, p + off, ret="u64")
        t.check(r == struct.unpack(">Q", data[off:off + 8])[0] if off <= 16 else True, "load_be64")
    return Case("sha512_load_be64(p %% 16 = %d)" % off, run)


def gen_sha_transform(rng, i, n):
    st = rand_state(rng) if i % 4 else list(SHA_IV)
    off = rng.randrange(16) if i % 3 else 0
    block = junk(rng, 128, 0.05)

    def run(t):
        s = t.put(struct.pack("<8Q", *st), 16)
        b = t.put(block, 16, off)
        t.call(SHA_TRANSFORM, s, b, ret="void")
        t.check(list(struct.unpack("<8Q", t.read(s, 64))) == sha512_compress(st, block), "transform")
    return Case("sha512_transform(block %% 16 = %d)" % off, run)


def _ctx_case(rng, fn, name, extra=None, c=None):
    if c is None:
        c = rand_ctx(rng)
    data = extra

    def run(t):
        p = t.alloc(ShaCtx.SIZE, 16, 8 * rng_off)
        t.write(p, c.pack(p))
        args = [p]
        if fn == SHA_UPDATE:
            d = t.put(data, 16, d_off)
            args += [d, len(data)]
        if fn == SHA_FINAL:
            o = t.alloc(64, 16)
            args.append(o)
        t.call(fn, *args, ret="void")
        m = ShaCtx(c.total, c.state, c.k, c.block)
        if fn == SHA_UPDATE:
            m.update(data)
        elif fn == SHA_PAD:
            m.pad()
        elif fn == SHA_BITLEN:
            m.block[0x78:0x80] = struct.pack(">Q", m.total)
        elif fn == SHA_RESET:
            m.total, m.state, m.k = 0, list(SHA_IV), 0
        elif fn == SHA_FINAL:
            want = m.final()
            t.check(list(struct.unpack("<8Q", t.read(o, 64))) == want, "final digest")
        got = ShaCtx.unpack(t.read(p, 0xCC), p)
        t.check((got.total, got.state, got.k) == (m.total, m.state, m.k) and
                (fn in (SHA_RESET, SHA_FINAL) or got.block == m.block), "%s context" % name)
    rng_off = rng.randrange(2)
    d_off = rng.randrange(16)
    return Case("%s(buffered %d%s)" % (name, c.k, "" if data is None else ", len %d" % len(data)), run)


def gen_sha_update(rng, i, n):
    L = rng.choice([0, 1, 127, 128, 129, 255, 256, rng.randrange(0, 300), rng.randrange(0, 1200)])
    return _ctx_case(rng, SHA_UPDATE, "sha512_update", junk(rng, L, 0.05))


def gen_sha_pad(rng, i, n):
    return _ctx_case(rng, SHA_PAD, "sha512_pad")


def gen_sha_bitlen(rng, i, n):
    return _ctx_case(rng, SHA_BITLEN, "sha512_store_bitlen")


def gen_sha_reset(rng, i, n):
    return _ctx_case(rng, SHA_RESET, "sha512_reset")


def gen_sha_final(rng, i, n):
    return _ctx_case(rng, SHA_FINAL, "sha512_final")


def gen_sha_stream(rng, i, n):
    L = rng.choice([0, 1, 111, 112, 127, 128, 129, 239, 240, 256, rng.randrange(0, 600), rng.randrange(0, 4000)])
    msg = junk(rng, L, 0.05)
    cuts = sorted(rng.randrange(L + 1) for _ in range(rng.randrange(0, 5)))
    off = rng.randrange(16)

    def run(t):
        ctx = t.alloc(ShaCtx.SIZE, 16)
        m = t.put(msg, 16, off)
        dig = t.alloc(64, 16)
        out = t.alloc(64, 16, off)
        t.call(SHA_RESET, ctx, ret="void")
        last = 0
        for c in cuts + [L]:
            t.call(SHA_UPDATE, ctx, m + last, c - last, ret="void")
            last = c
        t.call(SHA_FINAL, ctx, dig, ret="void")
        t.call(SHA_STORE_BE, out, dig, ret="void")
        d = t.read(out, 64)
        t.obs("digest", d)
        t.check(d == hashlib.sha512(msg).digest(), "SHA-512 digest")
    return Case("SHA-512 of %d bytes in %d update(s)" % (L, len(cuts) + 1), run)


# ---- edge cases and call structure (added by the round-2 behaviour verifier) ----------------------

MEMCPY, MEMSET = 0x0101B010, 0x0101B1B8     # libc (hand-written MMI asm in retail)

_BE64_PATTERNS = [M64, 0x8000000000000000, 0x0000000080000000, 0x8000000080000000, 0x8080808080808080,
                  0x7F7F7F7F7F7F7F7F, 0xFF000000FF000000, 0x00000000FFFFFFFF, 0xFFFFFFFF00000000,
                  0x7FFFFFFF7FFFFFFF, 0x00FF00FF00FF00FF, 0xFF00FF00FF00FF00, 0]


def gen_sha_load_edges(rng, i, n):
    """every single-bit word, then sign-bit patterns of each byte / 32-bit half (the
    halves are built in int arithmetic: a set bit 31 must not sign-extend into the
    other half), then random words with bit 31 / 63 forced on."""
    if i < 64:
        v = 1 << i
    elif i - 64 < len(_BE64_PATTERNS):
        v = _BE64_PATTERNS[i - 64]
    else:
        v = rng.getrandbits(64) | (rng.choice([0, 1]) << 31) | (rng.choice([0, 1]) << 63)
    off = rng.randrange(16)
    pre, post = junk(rng, 16, 0), junk(rng, 16, 0)

    def run(t):
        p = t.put(pre + struct.pack(">Q", v) + post, 16, off)
        r = t.call(SHA_LOAD_BE64, p + 16, ret="u64")
        t.check(r == v, "load_be64")
    return Case("sha512_load_be64(0x%016X, p %% 16 = %d)" % (v, (off + 16) % 16), run)


def gen_sha_transform_alias(rng, i, n):
    """block overlapping state[] (const u8 * may alias u64 *): the original reads the
    whole block (rounds 0..15) before it writes state[] (epilogue), so the result is
    the compression of the bytes present at entry.  Also 8-aligned (not 16) state."""
    d = rng.choice([0, 1, 7, 8, 32, 63, 64, -1, -8, -64, -120, -127, rng.randrange(-127, 64)])
    st = rand_state(rng)
    pre, post = junk(rng, 128, 0.05), junk(rng, 128, 0.05)
    s_off = 8 * rng.randrange(2)

    def run(t):
        base = t.put(pre + struct.pack("<8Q", *st) + post, 16, s_off)
        s = base + 128
        blk = t.read(s + d, 128)
        t.call(SHA_TRANSFORM, s, s + d, ret="void")
        t.check(list(struct.unpack("<8Q", t.read(s, 64))) == sha512_compress(st, blk), "aliased transform")
    return Case("sha512_transform(block = state %+d, state %% 16 = %d)" % (d, s_off), run)


_UPD_K = [0, 1, 2, 64, 110, 111, 112, 113, 126, 127]


def gen_sha_update_edges(rng, i, n):
    """buffered k x length grid around the free space (len < / == / > space, one or
    two whole blocks after the top-up, len 0)."""
    k = _UPD_K[i % len(_UPD_K)]
    space = 0x80 - k
    L = max(0, [0, 1, space - 1, space, space + 1, space + 127, space + 128, space + 129,
                space + 256][(i // len(_UPD_K)) % 9])
    c = ShaCtx(rng.getrandbits(64), rand_state(rng), k, junk(rng, 0x80, 0.1))
    return _ctx_case(rng, SHA_UPDATE, "sha512_update", junk(rng, L, 0.05), c=c)


def gen_sha_pad_final_sweep(rng, i, n):
    """sha512_pad and sha512_final for every buffered length 0..127 (the length field
    fits iff at most 111 bytes are buffered: left >= 16 after the 0x80 byte)."""
    k = (i // 2) % 128
    c = ShaCtx(rng.getrandbits(64), rand_state(rng), k, junk(rng, 0x80, 0.1))
    if i % 2 == 0:
        return _ctx_case(rng, SHA_PAD, "sha512_pad", c=c)
    return _ctx_case(rng, SHA_FINAL, "sha512_final", c=c)


def _stub_transform(t, a):
    """sha512_transform as a model: the block it is handed (and the state) are
    observed, then the compression is applied so the caller continues normally."""
    s, b = a[0], a[1]
    blk = t.read(b, 128)
    st = list(struct.unpack("<8Q", t.read(s, 64)))
    t.obs("transform block", blk)
    t.obs("transform state", tuple(st))
    t.write(s, struct.pack("<8Q", *sha512_compress(st, blk)))
    return 0


def _stub_memcpy(t, a):
    d, s, n = a
    if n:
        t.write(d, t.read(s, n))
    return d


def _stub_memset(t, a):
    p, c, n = a
    if n:
        t.write(p, bytes([c & 0xFF]) * n)
    return p


def _stub_wipe(t, a):
    """memset of a local of sha512_transform: the pointer is a stack address (frame
    layout), so it is not compared; the value, the size and the contents wiped (S =
    a..h after round 79, W = W[64..79]) are.  Returns 0, not p: the return value is
    part of the compared call event, p depends on the frame layout, and
    sha512_transform ignores memset's result."""
    p, c, n = a
    t.obs("wipe", (c & 0xFF, n, t.peek(p, n)))
    if n:
        t.write(p, bytes([c & 0xFF]) * n)
    return 0


def gen_sha_calls(rng, i, n):
    """call structure with the callees stubbed: transform -> memset(S, 0, 0x40) then
    memset(W, 0, 0x80) with the wiped contents; update -> memcpy / transform with
    their arguments and the block contents at each transform; pad -> memset(buf_pos,
    0, left) [, transform(state, block), memset(block, 0, 0x80)]; final -> pad's
    calls, transform(state, block).
    Not stubbed: store_bitlen / reset (the GCC 14 build inlines them) and load_be64.
    GCC 14 (-fipa-ra) keeps values in the caller-saved registers its same-unit
    leaf load_be64 does not touch ($a4 across the first call); the harness no
    longer clobbers registers after a same-unit stub on that image (MATCHING.md
    "The GCC 14 build"), so a stub would work there now.  (The 16 load_be64
    calls of the SCE build are pinned by the scorer's jal-target check.)"""
    kind = ("transform", "update", "pad", "final")[i % 4]
    if kind == "transform":
        st = rand_state(rng)
        block = junk(rng, 128, 0.05)
        off = rng.randrange(16)

        def run(t):
            t.stub(MEMSET, _stub_wipe, 3, "memset", log=0)
            s = t.put(struct.pack("<8Q", *st), 16)
            b = t.put(block, 16, off)
            t.call(SHA_TRANSFORM, s, b, ret="void")
            t.check(list(struct.unpack("<8Q", t.read(s, 64))) == sha512_compress(st, block), "transform")
        return Case("calls of sha512_transform(block %% 16 = %d)" % off, run)

    c = rand_ctx(rng)
    if kind == "update":
        L = rng.choice([0, 1, 0x80 - c.k - 1, 0x80 - c.k, 0x80 - c.k + 1, 255, 256, rng.randrange(0, 700)])
        data = junk(rng, max(0, L), 0.05)
    d_off = rng.randrange(16)

    def run(t):
        t.stub(SHA_TRANSFORM, _stub_transform, 2, "sha512_transform")
        t.stub(MEMCPY, _stub_memcpy, 3, "memcpy")
        t.stub(MEMSET, _stub_memset, 3, "memset")
        p = t.alloc(ShaCtx.SIZE, 16)
        t.write(p, c.pack(p))
        m = ShaCtx(c.total, c.state, c.k, c.block)
        if kind == "update":
            d = t.put(data, 16, d_off)
            t.call(SHA_UPDATE, p, d, len(data), ret="void")
            m.update(data)
        elif kind == "pad":
            t.call(SHA_PAD, p, ret="void")
            m.pad()
        else:
            o = t.alloc(64, 16)
            t.call(SHA_FINAL, p, o, ret="void")
            t.check(list(struct.unpack("<8Q", t.read(o, 64))) == m.final(), "final digest")
        got = ShaCtx.unpack(t.read(p, 0xCC), p)
        t.check((got.total, got.state, got.k) == (m.total, m.state, m.k) and
                (kind == "final" or got.block == m.block), "%s context" % kind)
    return Case("calls of sha512_%s(buffered %d%s)" % (kind, c.k, ", len %d" % len(data) if kind == "update" else ""),
                run)


SPECS = [
    Spec("sha512_compare", [SHA_CMP], gen_sha_compare, 200),
    Spec("sha512_store_be", [SHA_STORE_BE], gen_sha_store_be, 200),
    Spec("sha512_init", [SHA_INIT], gen_sha_init, 20),
    Spec("sha512_load_be64", [SHA_LOAD_BE64], gen_sha_load, 100),
    Spec("sha512_transform", [SHA_TRANSFORM], gen_sha_transform, 200),
    Spec("sha512_reset", [SHA_RESET], gen_sha_reset, 50),
    Spec("sha512_update", [SHA_UPDATE], gen_sha_update, 200),
    Spec("sha512_pad", [SHA_PAD], gen_sha_pad, 200),
    Spec("sha512_bitlen", [SHA_BITLEN], gen_sha_bitlen, 100),
    Spec("sha512_final", [SHA_FINAL], gen_sha_final, 200),
    Spec("sha512_stream", [SHA_RESET, SHA_UPDATE, SHA_FINAL, SHA_STORE_BE], gen_sha_stream, 200,
         "hashlib oracle"),
    Spec("sha512_load_be64_edges", [SHA_LOAD_BE64], gen_sha_load_edges, 100,
         "one-hot words, sign bits of each byte / half"),
    Spec("sha512_transform_alias", [SHA_TRANSFORM], gen_sha_transform_alias, 60,
         "block overlapping state; 8-aligned state"),
    Spec("sha512_update_edges", [SHA_UPDATE], gen_sha_update_edges, 90, "buffered k x len around the free space"),
    Spec("sha512_pad_final_sweep", [SHA_PAD, SHA_FINAL], gen_sha_pad_final_sweep, 256,
         "every buffered length 0..127"),
    Spec("sha512_calls", [SHA_TRANSFORM, SHA_UPDATE, SHA_PAD, SHA_FINAL], gen_sha_calls, 200,
         "callees stubbed: load_be64 / transform / memcpy / memset calls and arguments"),
]
