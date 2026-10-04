"""gzip group: unit gzip - src/gzip_inflate.cpp (GZIP::InflateBase, crc32) and
src/gzip_montgomery_tail.cpp (modpow 0x0100B238, which units.json puts in this unit).

Inputs and the original's quirks (src/gzip_inflate.cpp):
  * a gzip header is mis-parsed when in % 4 == 3 (GZ_DUMPBITS shifts the first
    refill word by 56): the valid header cases use in % 4 != 3;
  * huft_build stores its local Huft r into the tables, but sets r.t only for link
    entries: the t field of every other entry is an uninitialised stack word, so it
    is excluded from the memory comparison (mask_huft_tables);
  * garbage streams (bit flips, a misaligned header) end up reading stale stack
    words (huft_build's v[] past the filled values, r.t followed as a link): their
    results depend on the frame layout, so they are not generated.  The invalid
    streams generated on purpose are the ones the inflater handles the same way
    every time: stored blocks whose NLEN is not ~LEN (never checked) and reserved
    block type 3 (silently skipped).

Beyond zlib's streams, a small DEFLATE writer (Crafted) builds streams with what
zlib never emits but the inflater decodes deterministically (nl/nd past the RFC
limits, distance codes 30/31, a code-length repeat running past nl + nd, a 16
code first, length 258 as 284 + 31, copies reaching before the output start,
65535-byte stored blocks, a type-3 final block, long codes with sub-tables).
Its output model and HuftModel (huft_build's tables entry by entry, gzip-1.2.4's
missing `if (j < z)` included, and inflate_codes' walk over them) are checked on
the retail side.  inflate_stream / inflate_codes can serve the input through a
vtable built by the test whose fetchInput is a streaming loader (install_loader),
so every refill site runs the hook and the written-back state is compared at
each call."""
from .common import *
from difftest import INPUT_HI

UNITS = ("gzip",)

MODPOW = 0x0100B238

# ================================================================== GZIP (src/gzip_inflate.cpp)

GZ_CRC32, GZ_CTOR0, GZ_CTOR2, GZ_DTOR = 0x01011700, 0x01011750, 0x01011768, 0x010117C8
GZ_INITBITS, GZ_HUFT, GZ_FETCH, GZ_HEAPCLEAR = 0x010117F8, 0x01011838, 0x01012E78, 0x01012E80
GZ_SETOUT, GZ_INFLATE = 0x01012E88, 0x01012E98
GZ_OBJ = 0x3040
GZ_LAYOUT = LAYOUT["InflateBase"]
ARENA = 0x3000                   # the Huft arena at +0; heapPtr at +0x3018
_STARTUPA = None


def startupa():
    global _STARTUPA
    if _STARTUPA is None:
        _STARTUPA = open(os.path.join(ROOT, "data", "startupa_img.gz"), "rb").read()
    return _STARTUPA


def sample_data(rng, n):
    k = rng.random()
    if k < 0.25:
        return bytes(rng.randrange(256) for _ in range(n))
    if k < 0.5:
        words = [b"the ", b"core", b".gt3 ", b"GT3 ", b"bootstrap ", b"\n", b"cdrom0:\\", b"0000", b"\xff\xfe"]
        out = b""
        while len(out) < n:
            out += rng.choice(words)
        return out[:n]
    if k < 0.7:
        out = bytearray()
        while len(out) < n:
            out += bytes([rng.randrange(256)]) * rng.randrange(1, 300)
        return bytes(out[:n])
    if k < 0.8:
        return b"\0" * n
    base = bytes(rng.randrange(256) for _ in range(rng.randrange(1, 64)))
    out = bytearray()
    while len(out) < n:
        out += base if rng.random() < 0.7 else bytes([rng.randrange(256)])
    return bytes(out[:n])


def deflate_raw(rng, data):
    level = rng.choice([0, 1, 6, 9, rng.randrange(10)])
    strategy = rng.choice([zlib.Z_DEFAULT_STRATEGY, zlib.Z_DEFAULT_STRATEGY, zlib.Z_FILTERED,
                           zlib.Z_HUFFMAN_ONLY, zlib.Z_RLE, zlib.Z_FIXED])
    c = zlib.compressobj(level, zlib.DEFLATED, -15, rng.choice([1, 8, 9]), strategy)
    out = b""
    pos = 0
    while pos < len(data):
        step = rng.randrange(1, len(data) - pos + 1)
        out += c.compress(data[pos:pos + step])
        pos += step
        if rng.random() < 0.15:
            out += c.flush(rng.choice([zlib.Z_SYNC_FLUSH, zlib.Z_FULL_FLUSH]))
    out += c.flush(zlib.Z_FINISH)
    return out, "level %d strategy %d" % (level, strategy)


def deflate_quirky(rng, data):
    """a raw DEFLATE stream the format forbids but this inflater decodes the same
    way every time: stored blocks whose NLEN is not ~LEN (NLEN is never checked),
    some preceded by a reserved type-3 block header (silently skipped).  Every
    block starts on a byte boundary, so a block header is one byte."""
    chunks, pos = [], 0
    while True:
        k = min(len(data) - pos, rng.choice([0, 1, 7, 100, rng.randrange(0, 2000), 65535]))
        chunks.append(data[pos:pos + k])
        pos += k
        if pos >= len(data):
            break
    out = bytearray()
    for idx, ch in enumerate(chunks):
        final = 1 if idx == len(chunks) - 1 else 0
        if rng.random() < 0.4:
            out.append(0x06 | (final << 3))     # BFINAL 0, BTYPE 3 (skipped), then BFINAL, BTYPE 0 (stored)
        else:
            out.append(final)                   # BFINAL, BTYPE 0 (stored), 5 padding bits
        nlen = rng.choice([len(ch) ^ 0xFFFF, 0, len(ch), rng.getrandbits(16)])
        out += struct.pack("<HH", len(ch), nlen) + ch
    return bytes(out), "stored blocks with bad NLEN / type-3 headers, invalid on purpose"


def gzip_file(rng, data, quirky=False):
    body, how = (deflate_quirky if quirky else deflate_raw)(rng, data)
    flg = 8 if rng.random() < 0.5 else 0
    hdr = b"\x1f\x8b\x08" + bytes([flg]) + struct.pack("<I", rng.getrandbits(32)) + bytes([rng.choice([0, 2, 4]), 3])
    if flg:
        hdr += rstr(rng, rng.randrange(0, 30)) + b"\0"
    return hdr + body + struct.pack("<II", zlib.crc32(data), len(data) & M32), how + (", FNAME" if flg else "")


def mask_huft_tables(t, lo, hi):
    """huft_build fills table entries from its local Huft r {t, n, b, e}, whose t
    field it only sets for link entries (e = 16 + j, j = 1..15).  In every other
    entry (literal, length, distance, end of block, invalid code) t is whatever
    that stack slot held - indeterminate, and different in a build whose frame
    differs - so those four bytes of each 8-byte entry in [lo, hi) are left out of
    the memory comparison.  Everything else is compared."""
    data = t.read(lo, hi - lo)
    for k in range(0, hi - lo - 7, 8):
        if not 17 <= data[k + 7] <= 31:
            t.unchecked(lo + k, 4)


def gz_obj(t):
    """an InflateBase in test memory: (address of its members, this).  The arena
    and the state sit at the same address on both sides, so the table and
    cursor pointers compare as they are; `this` is that address on a GNU v2
    image and 8 below it on an Itanium one (vptr at +0, members at +8)."""
    this = t.alloc_place(GZ_OBJ, GZ_LAYOUT)
    return t.m(this, 0), this


def check_arena(t, obj):
    """after a valid stream: the tables stayed inside the arena (heapPtr at +0x3018
    is only written when a Huffman block was decoded); mask their r.t words."""
    if obj + 0x3018 in t.written(obj + 0x3018, obj + 0x301C):
        used = (t.r32(obj + 0x3018) - obj) & M32
        t.check(used <= ARENA, "valid input, but the Huffman tables overran the 0x3000-byte arena (0x%X)" % used)
    mask_huft_tables(t, obj, obj + ARENA)


VALID_OFF = [o for o in range(16) if o % 4 != 3]


def gen_crc32(rng, i, n):
    if i == 0:
        data = startupa()
    else:
        data = junk(rng, pick_len(rng, 2000), 0.1)
    off = rng.randrange(16)

    def run(t):
        p = t.put(data, 16, off)
        r = t.call(GZ_CRC32, p, len(data))
        t.check(r & M32 == zlib.crc32(data), "crc32")
    return Case("crc32(%d bytes%s)" % (len(data), ", startupa.img" if i == 0 else ""), run)


def gen_gzip_file(rng, i, n):
    """the (out, in) constructor - a gzip file with header, FNAME or no flags, at
    in % 4 != 3 - then ~InflateBase(2).  Every 8th case: stored blocks with bad
    NLEN / type-3 headers (invalid on purpose, decoded all the same)."""
    if i == 0:
        blob = startupa()
        data = gzip.decompress(blob)
        how = "the embedded startupa.img"
    else:
        data = sample_data(rng, rng.choice([0, 1, 100, rng.randrange(0, 3000), rng.randrange(0, 40000)]))
        blob, how = gzip_file(rng, data, quirky=i % 8 == 7)
    off = rng.choice(VALID_OFF) if i else 0

    def run(t):
        obj, this = gz_obj(t)
        out = t.alloc(len(data) + 64, 16)
        src = t.put(blob, 16, off, pad=16)
        t.call(GZ_CTOR2, this, out, src, ret="ptr", maxsteps=60000000)
        check_arena(t, obj)
        got = t.read(out, len(data))
        t.check(got == data, "inflated output")
        t.obs("output sha1", hashlib.sha1(got).digest())
        t.call(GZ_DTOR, this, 2, ret="void")
    return Case("InflateBase(out, in): %d -> %d bytes (%s, in %% 16 = %d)" % (len(blob), len(data), how, off), run)


def gen_gzip_raw(rng, i, n):
    """InflateBase() + inflate(out, in, size, raw) + ~InflateBase: raw DEFLATE, or
    (20%) a gzip file with header at in % 4 != 3.  Trailing bytes move inEnd, so a
    4-aligned end makes the refill hook fire.  Every 8th case: stored blocks with
    bad NLEN / type-3 headers (invalid on purpose, decoded all the same)."""
    quirky = i % 8 == 7
    data = sample_data(rng, rng.choice([0, 1, 2, 300, rng.randrange(0, 5000), rng.randrange(0, 30000)]))
    raw = rng.random() >= 0.2
    if raw:
        body, how = (deflate_quirky if quirky else deflate_raw)(rng, data)
    else:
        body, how = gzip_file(rng, data, quirky)
    off = rng.randrange(16) if raw else rng.choice(VALID_OFF)
    extra = rng.choice([0, 0, 1, 2, 3, 4, 7])        # trailing bytes: moves inEnd (4-aligned ends fire the hook)

    def run(t):
        obj, this = gz_obj(t)
        out = t.alloc(len(data) + 64, 16)
        src = t.put(body + b"\xA5" * extra, 16, off, pad=16)
        t.call(GZ_CTOR0, this, ret="ptr")
        t.call(GZ_INFLATE, this, out, src, len(body) + extra, 1 if raw else 0, ret="void", maxsteps=40000000)
        check_arena(t, obj)
        got = t.read(out, len(data))
        t.check(got == data, "inflated output")
        t.obs("output sha1", hashlib.sha1(got).digest())
        t.call(GZ_DTOR, this, 2, ret="void")
    return Case("inflate(%s, %d -> %d bytes, %s, in %% 16 = %d, +%d)" % ("raw" if raw else "gzip", len(body),
                                                                         len(data), how, off, extra), run)


def gen_gzip_parts(rng, i, n):
    """the small members: initBitStream, setOutputData, heap_clear, fetchInput, dtor(3)."""
    off = rng.randrange(16)
    size = rng.choice([0, 1, 4, 0xFFFFFFFF, rng.randrange(0, 5000)])
    data = junk(rng, 32, 0.1)
    fill = junk(rng, 0x40, 0)

    def run(t):
        this = t.halloc_obj(GZ_OBJ, GZ_LAYOUT, (fill * (GZ_OBJ // 0x40))[:GZ_LAYOUT[2]], place=True)
        obj = t.m(this, 0)
        src = t.put(data, 16)
        out = t.alloc(16, 16)
        t.call(GZ_CTOR0, this, ret="ptr")
        t.call(GZ_INITBITS, this, src + off, size, ret="void")
        t.call(GZ_SETOUT, this, out + off, ret="void")
        t.call(GZ_HEAPCLEAR, this, ret="void")
        t.call(GZ_FETCH, this, ret="void")
        t.obs("state", t.read(obj + 0x3000, 0x30))
        t.call(GZ_DTOR, this, 3, ret="void")
    return Case("initBitStream(in %% 4 = %d, size 0x%X) + setOutputData + heap_clear + fetchInput + ~(3)"
                % (off % 4, size), run)


def kraft_complete(rng, n, maxlen=15):
    """a random COMPLETE code-length set (Kraft sum exactly 1) over n >= 2
    symbols, about 15% of them unused (length 0)."""
    lens = [0] * n
    used = [i for i in range(n) if rng.random() < 0.85]
    if len(used) < 2:
        used = sorted(rng.sample(range(n), 2))
    # a random full binary tree: start from the two depth-1 leaves, split leaves
    leaves = [1, 1]
    while len(leaves) < len(used):
        k = rng.randrange(len(leaves))
        if leaves[k] >= maxlen:
            continue
        d = leaves.pop(k) + 1
        leaves += [d, d]
    rng.shuffle(leaves)
    for s, d in zip(used, leaves):
        lens[s] = d
    return lens


def kraft(lens):
    return sum(2.0 ** -x for x in lens if x)


def gen_huft(rng, i, n):
    """huft_build on its own, with the lookup bits the callers use (or 1..10):
      complete code sets, some lengths 0 (~70%);
      incomplete sets without zero lengths (one symbol = the one-distance-code case;
      long codes make the tables outgrow the 0x3000-byte arena, which is not
      checked: the object is the last input block, so they run into free memory);
      all lengths zero (returns 0, *t = NULL);
      every 25th case: an over-subscribed set without zero lengths - invalid on
      purpose: y is unsigned, so c[g] gains ~2^32 dummy codes and the build runs
      away (the comparison is then the error class and the events before it).
    Left out: incomplete sets WITH zero-length codes, which read stale v[] stack
    words in the original (src/gzip_inflate.cpp huft_build) - uninitialised,
    layout-dependent data, not comparable between builds."""
    kind = rng.choice(["lit", "dist", "bl"])
    nsym = {"lit": rng.randrange(257, 289), "dist": rng.randrange(1, 31), "bl": 19}[kind]
    maxlen = 7 if kind == "bl" else 15
    k = rng.random()
    if i % 25 == 24:
        what = "over-subscribed, invalid on purpose"
        nsym = max(nsym, 3)
        lens = [rng.randrange(1, max(2, nsym.bit_length())) for _ in range(nsym)]
        lens[0] = lens[1] = lens[2] = 1
    elif k < 0.03:
        what = "all zero"
        lens = [0] * nsym
    elif k < 0.75 and nsym >= 2:
        what = "complete"
        lens = kraft_complete(rng, nsym, maxlen)
    else:
        what = "incomplete, no zero lengths"
        lo = max(1, nsym.bit_length())
        hi = maxlen if rng.random() < 0.15 else min(maxlen, lo + 2)     # long codes: many dummy entries (slow)
        lens = [rng.randrange(lo, hi + 1) for _ in range(nsym)]
    invalid = what.startswith("over")
    assert {"complete": kraft(lens) == 1, "all zero": max(lens) == 0,
            "incomplete, no zero lengths": kraft(lens) < 1 and 0 not in lens}.get(what, kraft(lens) > 1), (what, lens)
    bits = {"lit": 9, "dist": 6, "bl": 7}[kind]
    if rng.random() < 0.3:
        bits = rng.randrange(1, 11)

    def run(t):
        b = t.put(struct.pack("<%dI" % nsym, *lens), 16)
        tp = t.alloc(4, 4)
        mp = t.alloc(4, 4)
        obj, this = gz_obj(t)                 # last: arena overruns land in unused input memory
        t.call(GZ_CTOR0, this, ret="ptr")
        t.call(GZ_HEAPCLEAR, this, ret="void")
        if kind == "lit":
            d, e, s = t.data("cplens"), t.data("cplext"), 257
        elif kind == "dist":
            d, e, s = t.data("cpdist"), t.data("cpdext"), 0
        else:
            d, e, s = 0, 0, 19
        r = t.call(GZ_HUFT, this, b, nsym, s, d, e, tp, mp, bits, maxsteps=500000 if invalid else 20000000)
        top = max([w for w in t.written(obj, INPUT_HI)] or [obj]) + 4
        mask_huft_tables(t, obj, (top + 7) & ~7)
        t.obs("*m", t.r32(mp))
        t.obs("table pointer offset", (t.r32(tp) - obj) & M32)
        if what == "all zero":
            t.check(r == 0 and t.r32(tp) == 0 and t.r32(mp) == 0, "empty code set")
        elif not invalid:
            want = 1 if what != "complete" and max(lens) != 1 else 0
            t.check(r == want, "huft_build returned %d, want %d (incomplete set and max length > 1)" % (r, want))
    return Case("huft_build(%s, %s, %d symbols, bits %d, lengths %s...)" % (kind, what, nsym, bits, lens[:12]), run)


# ================================================================== crafted DEFLATE streams
# A small DEFLATE writer, so that the specs below can choose what zlib never
# emits but this inflater decodes the same way every time: nl up to 288 and nd
# up to 32 (no "nl > 286 || nd > 30" check; distance codes 30/31 have base 0
# and copy the output onto itself), a code-length repeat that runs past nl + nd
# (unchecked; kept below ll[] + dl[] + the 2 padding words after them, 348
# slots, which the original frame never reads; beyond them it overwrites
# spilled locals), a 16 code at the very start (repeats 0), every nb 4..19,
# length 258 as code 284 + 31, copies that reach before the output start,
# stored blocks with any NLEN, reserved type-3 blocks (also as the final block),
# and long codes that need sub-tables.  Every Huffman code is complete, the
# one-distance-code case (nd = 1, one 1-bit code) or all zero, so huft_build
# never reads stale v[] words, and only symbols with a real table entry are
# emitted (never lit/len 286/287 or a fixed-block distance 30/31: e = 99,
# followed as a link through the uninitialised r.t).

GZ_CODES, GZ_MAIN, GZ_TF = 0x01011E40, 0x01012180, 0x01012EF8
LBASE = [3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195,
         227, 258]
LEXT = [0] * 8 + [1] * 4 + [2] * 4 + [3] * 4 + [4] * 4 + [5] * 4 + [0]
DBASE = [1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
         6145, 8193, 12289, 16385, 24577, 0, 0]
DEXT = [0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 0, 0]
BORDER = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]
FIXED_LIT = [8] * 144 + [9] * 112 + [7] * 24 + [8] * 8
FIXED_DIST = [5] * 30
LL_SLOTS = 316 + 30 + 2      # ll[] + dl[] + 2 padding words in the original frame: code-length writes stay below
ARENA_ENTRIES = ARENA // 8
GUARD = 0x400                # bytes in front of the output buffer (copies reaching before the start read them)
INVALID = "invalid on purpose"


class BitW:
    """LSB-first bit writer (DEFLATE order)."""

    def __init__(self):
        self.out = bytearray()
        self.acc = 0
        self.nacc = 0

    def put(self, val, nbits):
        self.acc |= (val & ((1 << nbits) - 1)) << self.nacc
        self.nacc += nbits
        while self.nacc >= 8:
            self.out.append(self.acc & 0xFF)
            self.acc >>= 8
            self.nacc -= 8

    def code(self, rc):
        """a Huffman code as canonical() gives it (bit-reversed, length)."""
        self.put(rc[0], rc[1])

    def align(self):
        if self.nacc:
            self.put(0, 8 - self.nacc)

    def data(self):
        return bytes(self.out) + (bytes([self.acc]) if self.nacc else b"")


def canonical(lens):
    """{symbol: (bit-reversed code, length)}: RFC 1951 3.2.2, the code huft_build decodes."""
    maxb = max(list(lens) + [0])
    count = [0] * (maxb + 1)
    for x in lens:
        if x:
            count[x] += 1
    nxt, code = [0] * (maxb + 2), 0
    for bits in range(1, maxb + 1):
        code = (code + count[bits - 1]) << 1
        nxt[bits] = code
    out = {}
    for s, x in enumerate(lens):
        if x:
            out[s] = (int(format(nxt[x], "0%db" % x)[::-1], 2), x)
            nxt[x] += 1
    return out


def complete_lengths(rng, nsym, must, maxlen, extra=0.1, deep=False):
    """code lengths over nsym >= 2 symbols forming a COMPLETE code in which every
    symbol of `must` has a code (others with probability `extra`); deep: grow
    long codes (sub-tables)."""
    used = set(must)
    for s in range(nsym):
        if s not in used and rng.random() < extra:
            used.add(s)
    while len(used) < 2:
        used.add(rng.randrange(nsym))
    used = sorted(used)
    leaves = [1, 1]
    while len(leaves) < len(used):
        if deep and rng.random() < 0.5:
            k = max((k for k, d in enumerate(leaves) if d < maxlen), key=lambda k: leaves[k])
        else:
            k = rng.randrange(len(leaves))
            if leaves[k] >= maxlen:
                continue
        d = leaves.pop(k) + 1
        leaves += [d, d]
    rng.shuffle(leaves)
    lens = [0] * nsym
    for s, d in zip(used, leaves):
        lens[s] = d
    return lens


CPLENS, CPLEXT = LBASE + [0, 0], LEXT + [99, 99]          # the .data tables as huft_build reads them


class Unmodelled(Exception):
    """the original would read data the models below do not know (an entry
    huft_build never wrote, the uninitialised r.t, memory past the stream)."""


class HuftModel:
    """huft_build's tables exactly as the original builds them - a port of
    src/gzip_inflate.cpp huft_build over a flat arena of entries [t, n, b, e]
    (t: arena index of a table; None: a field nothing wrote, e.g. the
    uninitialised r.t; the arena can be shared by several builds, from entry
    `hp` on, as heapPtr is), gzip-1.2.4's missing `if (j < z)` included.
    quirk: that missing guard made a table one bit wider than l while longer
    codes still need a level below it - the tables then no longer decode as RFC
    1951 says (e.g. lookup bits 6 with distance codes of 13+ bits).  (A table
    widened where no level follows, z = g - w, still decodes right.)  For
    complete codes and the one-code case: incomplete sets with zero lengths
    read stale v[] words, which this does not model."""

    def __init__(self, lens, bits, s=None, d=(), e=(), arena=None, hp=0):
        n = len(lens)
        s = n if s is None else s
        self.arena = {} if arena is None else arena
        self.total, self.l, self.root, self.quirk, self.end = 0, 0, None, False, hp
        start = hp
        c = [0] * 17
        for v in lens:
            c[v] += 1
        if c[0] == n:
            return                                  # *t = NULL, *m = 0
        l = bits
        j = 1
        while j <= 16 and not c[j]:
            j += 1
        k = j
        l = max(l, j)
        i = 16
        while i and not c[i]:
            i -= 1
        g = i
        l = min(l, i)
        self.l = l
        y = 1 << j
        while j != i:
            y -= c[j]
            j += 1
            y <<= 1
        y -= c[i]
        c[i] += y
        v = [sym for ln in range(1, 17) for sym in range(n) if lens[sym] == ln]
        arena = self.arena
        x = [0] * 17
        u = [None] * 16
        r = [None, None, None, None]                # t, n, b, e
        code, h, w, z, p, q, tp = 0, -1, -l, 0, 0, None, None
        while k <= g:
            a = c[k]
            while a:
                a -= 1
                while k > w + l:
                    h += 1
                    w += l
                    z = min(g - w, l)
                    j = k - w
                    f = 1 << j
                    if f > a + 1:
                        f -= a + 1
                        xp = k
                        while True:
                            j += 1
                            if j >= z:
                                break
                            xp += 1
                            f <<= 1
                            if f <= c[xp]:
                                break
                            f -= c[xp]
                    if j > z and z < g - w:
                        self.quirk = True
                    z = 1 << j
                    q = hp                          # bump-allocate: link word, then the table
                    if tp is None:
                        self.root = q + 1           # *t (the caller's tl / td)
                    else:
                        arena.setdefault(tp, [None] * 4)[0] = q + 1
                    hp = q + z + 1
                    tp = q
                    q += 1
                    u[h] = q
                    arena.setdefault(tp, [None] * 4)[0] = 0     # *tp = NULL (the link word's t)
                    if h:
                        r[2], r[3] = l, 16 + j
                        x[h] = code
                        r[0], r[1] = q, 0
                        arena[u[h - 1] + (code >> (w - l))] = list(r)
                r[2] = k - w
                if p >= n:
                    r[3] = 99
                elif p >= len(v):
                    raise Unmodelled("huft_build reads a stale v[] word")
                elif v[p] < s:
                    r[3] = 16 if v[p] < 256 else 15
                    r[1] = v[p]
                    p += 1
                else:
                    r[3] = e[v[p] - s]
                    r[1] = d[v[p] - s]
                    p += 1
                f = 1 << (k - w)
                jj = code >> w
                while jj < z:
                    arena[q + jj] = list(r)
                    jj += f
                j = 1 << (k - 1)
                while code & j:
                    code ^= j
                    j >>= 1
                code ^= j
                while (code & ((1 << w) - 1)) != x[h]:
                    h -= 1
                    w -= l
            k += 1
        self.end = hp
        self.total = hp - start
        if max(arena) >= ARENA_ENTRIES:
            raise Unmodelled("tables past the 0x3000-byte arena")

    def lookup(self, S, pos, limit):
        """inflate_codes' table walk at bit pos of the stream S (known bits [0,
        limit)): (e, n, new pos)."""
        if self.root is None:
            raise Unmodelled("a NULL table")
        nb = self.l
        t = self.root
        for _ in range(32):
            if pos + nb > limit:
                raise Unmodelled("decoding looks past the known stream")
            ent = self.arena.get(t + ((S >> pos) & ((1 << nb) - 1)))
            if ent is None or None in ent[1:]:
                raise Unmodelled("an entry huft_build did not write")
            pos += ent[2]
            if ent[3] <= 16:
                return ent[3], ent[1], pos
            if ent[0] is None:
                raise Unmodelled("the uninitialised r.t followed as a link")
            t, nb = ent[0], (ent[3] - 16) & 31
        raise Unmodelled("a link cycle")


def huft_plan(lens, bits):
    """(Huft entries huft_build allocates - tables plus their link words, the
    lookup bits l it stores in *m, the harmful gzip-1.2.4 table quirk)."""
    m = HuftModel(lens, bits, None, [0] * 32, [0] * 32)
    return m.total, m.l, m.quirk


def build_tables(lit, lb, dist, db):
    """the original's lit/len and distance tables in one arena, lit first as
    inflate_main builds them: (lm, dm), or None if they leave the arena (or
    need stale v[] words)."""
    try:
        lm = HuftModel(lit, lb, 257, CPLENS, CPLEXT)
        dm = HuftModel(dist, db, 0, DBASE, DEXT, lm.arena, lm.end)
    except Unmodelled:
        return None
    return (lm, dm) if dm.end <= ARENA_ENTRIES else None


def ops_size(ops):
    """bytes the ops output; Unmodelled if a copy reaches before the guard bytes."""
    pos = 0
    for op in ops:
        if op[0] == "copy" and pos - op[2] < -GUARD:
            raise Unmodelled("a copy reaching before the guard bytes")
        pos += 1 if op[0] == "lit" else op[1] if op[0] == "copy" else len(op[1])
    return pos


def model_codes(S, limit, lm, dm, cap):
    """inflate_codes on the stream S (known bits [0, limit)) with the tables
    lm / dm: the output ops up to the end-of-block code."""
    ops, pos, out = [], 0, 0
    while True:
        e, n, pos = lm.lookup(S, pos, limit)
        if e == 16:
            ops.append(("lit", n & 0xFF))
            out += 1
        elif e == 15:
            return ops
        else:
            if pos + e > limit:
                raise Unmodelled("decoding looks past the known stream")
            n += (S >> pos) & ((1 << (e & 31)) - 1)
            pos += e
            e2, base, pos = dm.lookup(S, pos, limit)
            if pos + e2 > limit:
                raise Unmodelled("decoding looks past the known stream")
            dist = base + ((S >> pos) & ((1 << (e2 & 31)) - 1))
            pos += e2
            if n == 0:
                raise Unmodelled("a zero-length copy (the byte loop runs away)")
            ops.append(("copy", n, dist))
            out += n
        if out > cap:
            raise Unmodelled("more output than the buffer holds")


def len_code(rng, L):
    if L == 258 and rng.random() < 0.4:
        return 27, 31                       # code 284 + 31 extra bits = 258
    for c in range(28, -1, -1):
        if LBASE[c] <= L:
            return c, L - LBASE[c]


def dist_code(D):
    for c in range(29, -1, -1):
        if DBASE[c] <= D:
            return c, D - DBASE[c]


def model_output(init, ops):
    """the output a stream's ops produce, from the memory `init` = GUARD bytes in
    front of the output buffer + the buffer as it was (LZ copies are byte-wise
    forward copies from op - dist, which may reach into the guard or, with
    distance 0, onto themselves)."""
    m = bytearray(init)
    pos = GUARD
    for op in ops:
        if op[0] == "lit":
            m[pos] = op[1]
            pos += 1
        elif op[0] == "copy":
            for _ in range(op[1]):
                m[pos] = m[pos - op[2]]
                pos += 1
        else:
            m[pos:pos + len(op[1])] = op[1]
            pos += len(op[1])
    return bytes(m[GUARD:pos])


def rle_lengths(rng, seq, over_max):
    """[(symbol, extra value, extra bits)] coding seq as the inflater reads it: 16
    repeats the last 0..15 length (0 at the start or after a 17/18), 17/18 zeros.
    over_max > 0: a final run of >= 3 zeros is coded by one 17/18 that runs 1 to
    over_max past the end.  Returns (items, how far it ran past the end, a 16 at
    the start)."""
    out = []
    i, prev, n = 0, 0, len(seq)
    lead16 = False
    while i < n:
        v = seq[i]
        r = 1
        while i + r < n and seq[i + r] == v:
            r += 1
        if over_max and i + r == n and v == 0 and 3 <= r < 138:
            c = rng.randint(r + 1, min(138, r + over_max))
            out.append((17, c - 3, 3) if c <= 10 else (18, c - 11, 7))
            i += c
            break
        opts = ["lit"]
        if v == 0 and r >= 3:
            opts += ["zero"] * 3
        if v == prev and r >= 3:
            opts += ["rep"] * 2
        o = rng.choice(opts)
        if o == "lit":
            out.append((v, 0, 0))
            prev = v
            i += 1
            continue
        hi = 138 if o == "zero" else 6
        c = rng.randint(3, min(r, hi))
        if i + r == n and over_max and c == r and rng.random() < 0.6:
            c = rng.randint(c, min(hi, r + over_max))
        if o == "zero":
            out.append((17, c - 3, 3) if c <= 10 else (18, c - 11, 7))
            prev = 0
        else:
            lead16 = lead16 or i == 0
            out.append((16, c - 3, 2))
        i += c
    return out, i - n, lead16


class Crafted:
    """a raw DEFLATE stream being built, with the ops of its output model."""

    def __init__(self, rng):
        self.rng = rng
        self.w = BitW()
        self.ops = []
        self.produced = 0
        self.notes = []
        self.big_stored = False          # the next stored block is 65535 bytes

    def note(self, s):
        if s not in self.notes:
            self.notes.append(s)

    def tokens(self, ntok, nd=30, far=True, rle=False):
        """literal / copy tokens; copy = (L, D, len code, len extra, dist code, dist extra).
        rle: every distance is 1 (one distance code, as zlib's Z_RLE)."""
        rng = self.rng
        toks = []
        alpha, base = rng.choice([2, 4, 16, 256]), rng.randrange(256)
        for _ in range(ntok):
            if self.produced == 0 or rng.random() < 0.45:
                toks.append(("lit", (base + rng.randrange(alpha)) & 0xFF))
                self.produced += 1
                continue
            k = rng.random()
            if k < 0.5:
                L = rng.randint(3, 12)
            elif k < 0.7:
                c = rng.randrange(29)
                L = LBASE[c] + rng.choice([0, (1 << LEXT[c]) - 1])
            elif k < 0.8:
                L = 258
            else:
                L = rng.randint(3, 258)
            lc, lx = len_code(rng, L)
            if rle:
                dc, D, dx = 0, 1, 0
            elif nd > 30 and rng.random() < 0.1:
                dc, D, dx = rng.randrange(30, nd), 0, 0
                self.note("distance codes 30/31 (base 0: the copy reads its own output bytes)")
            else:
                maxd = min(32768, self.produced)
                if far and rng.random() < 0.04:
                    maxd = min(32768, self.produced + GUARD)
                k = rng.random()
                if k < 0.4:
                    D = rng.randint(1, min(maxd, 64))
                elif k < 0.6:
                    c = rng.randrange(30)
                    D = min(maxd, DBASE[c] + rng.choice([0, (1 << DEXT[c]) - 1]))
                else:
                    D = rng.randint(1, maxd)
                if D > self.produced:
                    self.note("copies reaching before the output start (into the guard bytes), " + INVALID)
                dc, dx = dist_code(D)
            toks.append(("copy", L, D, lc, lx, dc, dx))
            self.produced += L
        return toks

    def emit(self, toks, litc, distc):
        w = self.w
        for tk in toks:
            if tk[0] == "lit":
                w.code(litc[tk[1]])
                self.ops.append(tk)
            else:
                _, L, D, lc, lx, dc, dx = tk
                w.code(litc[257 + lc])
                w.put(lx, LEXT[lc])
                w.code(distc[dc])
                w.put(dx, DEXT[dc])
                self.ops.append(("copy", L, D))
        w.code(litc[256])

    def dyn_codes(self, ntok, tail_zeros=0):
        """tokens and complete lit/len and distance codes for them:
        (toks, nl, nd, lit lengths, dist lengths); tail_zeros: the distance
        lengths end with that many unused (0) codes."""
        rng = self.rng
        nd_cap = rng.choice([31, 32]) if rng.random() < 0.25 and not tail_zeros else 30
        rle = rng.random() < 0.12
        toks = self.tokens(ntok, nd_cap, rle=rle)
        lit_used = {256} | {tk[1] for tk in toks if tk[0] == "lit"} | {257 + tk[3] for tk in toks if tk[0] == "copy"}
        dist_used = {tk[5] for tk in toks if tk[0] == "copy"}
        nl0 = max(lit_used) + 1
        nl = max(nl0, rng.choice([nl0, nl0, 286, 287, 288, rng.randint(nl0, 288)]))
        nd0 = max(dist_used) + 1 if dist_used else 1
        nd = max(nd0, rng.choice([nd0, nd0, 30, 31, 32, rng.randint(nd0, 32)]))
        if tail_zeros:
            # (nd >= 2: [1] + zeros would be incomplete WITH zero lengths, i.e. stale v[])
            tail_zeros = min(tail_zeros, 32 - max(nd0, 2))
            nd = max(2, min(nd, 32 - tail_zeros))
        for _ in range(20):
            deep = rng.random() < 0.4
            extra = rng.choice([0, 0.05, 0.3, 1.0])
            lit = complete_lengths(rng, nl, lit_used, 15, extra, deep)
            if not dist_used:
                k = rng.random()
                dist = [0] * nd if k < 0.5 or nd == 1 else complete_lengths(rng, nd, set(), 15, extra, deep)
            elif nd == 1:
                dist = [1]                      # one distance code: incomplete, no zero lengths
                self.note("one distance code")
            else:
                dist = complete_lengths(rng, nd, dist_used, 15, extra, deep)
            # inflate_main's lookup bits 9 / 6; the output model is RFC 1951's, so
            # codes for which the original's tables decode otherwise are redrawn
            tb = build_tables(lit, 9, dist + [0] * tail_zeros, 6)
            if tb and not (tb[0].quirk or tb[1].quirk):
                break
        else:
            raise HarnessError("could not draw codes that fit the arena and decode as written")
        dist += [0] * tail_zeros
        nd += tail_zeros
        if nl > 286 or nd > 30:
            self.note("nl %d / nd %d past the RFC 1951 limits (unchecked)" % (nl, nd))
        return toks, nl, nd, lit, dist

    def dynamic(self, final):
        rng = self.rng
        w = self.w
        over = rng.random() < 0.3
        toks, nl, nd, lit, dist = self.dyn_codes(rng.choice([0, 1, 5, rng.randrange(0, 300), rng.randrange(0, 1200)]),
                                                 rng.randint(3, 8) if over else 0)
        rle, over, lead16 = rle_lengths(rng, lit + dist, LL_SLOTS - (nl + nd) if over else 0)
        if over:
            self.note("a code-length repeat runs %d past nl + nd (unchecked), %s" % (over, INVALID))
        if lead16:
            self.note("a 16 code first (repeats length 0), " + INVALID)
        bl = complete_lengths(rng, 19, {s for s, _, _ in rle}, 7, rng.choice([0, 0.2]), rng.random() < 0.3)
        nb = max(4, max(j for j in range(19) if bl[BORDER[j]]) + 1)
        if rng.random() < 0.3:
            nb = rng.randint(nb, 19)
        w.put(final, 1)
        w.put(2, 2)
        w.put(nl - 257, 5)
        w.put(nd - 1, 5)
        w.put(nb - 4, 4)
        for j in range(nb):
            w.put(bl[BORDER[j]], 3)
        blc = canonical(bl)
        for s, xv, xb in rle:
            w.code(blc[s])
            w.put(xv, xb)
        self.emit(toks, canonical(lit), canonical(dist))
        return "dynamic(nl %d, nd %d, nb %d, %d tokens)" % (nl, nd, nb, len(toks))

    def fixed(self, final):
        rng = self.rng
        toks = self.tokens(rng.choice([0, 1, rng.randrange(0, 300), rng.randrange(0, 1000)]), 30)
        self.w.put(final, 1)
        self.w.put(1, 2)
        self.emit(toks, canonical(FIXED_LIT), canonical(FIXED_DIST))
        return "fixed(%d tokens)" % len(toks)

    def stored(self, final):
        rng = self.rng
        w = self.w
        L = rng.choice([0, 0, 1, 2, 3, 4, 5, rng.randrange(0, 300), rng.randrange(0, 3000),
                        65535 if rng.random() < 0.15 else 6])
        if self.big_stored:
            L, self.big_stored = 65535, False
        data = junk(rng, L, 0.1)
        nlen = rng.choice([L ^ 0xFFFF, L ^ 0xFFFF, 0, L, rng.getrandbits(16)])
        if nlen != L ^ 0xFFFF:
            self.note("stored NLEN != ~LEN, " + INVALID)
        w.put(final, 1)
        w.put(0, 2)
        w.align()
        w.put(L, 16)
        w.put(nlen, 16)
        w.out += data
        self.ops.append(("bytes", data))
        self.produced += L
        return "stored(%d)" % L

    def type3(self, final):
        self.w.put(final, 1)
        self.w.put(3, 2)
        self.note("reserved type-3 block%s (skipped), %s" % (" as the final block" if final else "", INVALID))
        return "type3%s" % (" final" if final else "")


def crafted_stream(rng, big_stored=False):
    """a crafted stream of 1..5 blocks; big_stored: one of them is a 65535-byte stored block."""
    c = Crafted(rng)
    c.big_stored = big_stored
    nblocks = rng.choice([1, 1, 2, 3, rng.randrange(1, 6)])
    kinds = [rng.choice(["dynamic"] * 5 + ["fixed"] * 2 + ["stored"] * 2 + ["type3"]) for _ in range(nblocks)]
    if big_stored and "stored" not in kinds:
        kinds[rng.randrange(nblocks)] = "stored"
    return c, ", ".join(getattr(c, kd)(1 if bi == nblocks - 1 else 0) for bi, kd in enumerate(kinds))


def gzip_header(rng):
    flg = 8 if rng.random() < 0.5 else 0
    hdr = b"\x1f\x8b\x08" + bytes([flg]) + struct.pack("<I", rng.getrandbits(32)) + bytes([rng.choice([0, 2, 4]), 3])
    if flg:
        hdr += rstr(rng, rng.randrange(0, 30)) + b"\0"
    return hdr


def out_region(t, produced, slack=64):
    """GUARD bytes + the output buffer; returns (region, out, init) with init the
    region's bytes before the call (the output model starts from them)."""
    region = t.alloc(GUARD + produced + slack, 16)
    return region, region + GUARD, t.read(region, GUARD + produced)


def gen_inflate_crafted(rng, i, n):
    """InflateBase() + initBitStream + setOutputData + inflate_main(raw) on a
    crafted stream (see above); 20% as a gzip file (in % 4 != 3).  Oracle: the
    output model of the stream's ops.  Every 12th case has a 65535-byte stored block."""
    c, how = crafted_stream(rng, i % 12 == 5)
    raw = rng.random() >= 0.2
    body = c.w.data()
    if not raw:
        body = gzip_header(rng) + body
    off = rng.randrange(16) if raw else rng.choice(VALID_OFF)
    extra = rng.choice([0, 0, 1, 2, 3, 4, 7])

    def run(t):
        obj, this = gz_obj(t)
        region, out, init = out_region(t, c.produced)
        src = t.put(body + b"\xA5" * extra, 16, off, pad=16)
        t.call(GZ_CTOR0, this, ret="ptr")
        t.call(GZ_INITBITS, this, src, len(body) + extra, ret="void")
        t.call(GZ_SETOUT, this, out, ret="void")
        t.call(GZ_MAIN, this, 1 if raw else 0, ret="void", maxsteps=40000000)
        check_arena(t, obj)
        got = t.read(out, c.produced)
        t.check(got == model_output(init, c.ops), "inflated output (stream model)")
        t.obs("output sha1", hashlib.sha1(got).digest())
        t.call(GZ_DTOR, this, 2, ret="void")
    return Case("inflate_main(%s: %s; in %% 16 = %d, +%d)%s" % (
        "raw" if raw else "gzip", how, off, extra, "; " + "; ".join(c.notes) if c.notes else ""), run)


# ---- a streaming loader: fetchInput through a vtable built by the test

def split_chunks(rng, blob, off, small=False):
    """blob (starting at in % 4 == off) in chunks for a streaming fetchInput: the
    first one ends 4-aligned, every other is whole words (the last zero-padded).
    small: 1-3 words each, so that nearly every refill runs the hook."""
    sizes, pos = [], 0
    while pos < len(blob) or not sizes:
        if not sizes:
            s = (4 if small else rng.choice([4, 4, 8, 16, 64, 256, 1024])) - off
            s = s if s > 0 else s + 4
        elif small:
            s = 4 * rng.choice([1, 1, 2, 3])
        else:
            s = 4 * rng.choice([1, 1, 2, 3, 16, rng.randrange(1, 64), rng.randrange(1, 600)])
        sizes.append(s)
        pos += s
    chunks, pos = [], 0
    for s in sizes:
        ch = blob[pos:pos + s]
        chunks.append(ch + b"\0" * (s - len(ch)))
        pos += s
    return chunks


TAIL = 64                    # zero bytes served after the last chunk (the reader looks ahead)


def install_loader(t, this, chunk_addrs, chunk_sizes, moves):
    """point the object's vptr (GNU v2 +0x3030) at a vtable (T.vtable) whose
    slot 2 (fetchInput) is a callback acting as a streaming subclass: it records
    the state the bit reader wrote back (bitbuf, bitcount, out - and inNext /
    inEnd as they stand), serves the next chunk in inNext / inEnd and, at the
    call numbers in `moves`, moves the output cursor."""
    tail = t.put(b"\0" * TAIL, 16)
    st = {"k": 0}

    def fetch(t, args):
        o = t.m(args[0], 0)                     # the members: this + 8 on an Itanium image
        k = st["k"]
        st["k"] = k + 1
        t.obs("fetchInput #%d: bitbuf, bitcount, inNext, inEnd, out" % k,
              (t.r64(o + 0x3000), t.r32(o + 0x3008), t.r32(o + 0x300C), t.r32(o + 0x3010),
               t.r32(o + 0x3014)))
        nxt, size = (chunk_addrs[k + 1], chunk_sizes[k + 1]) if k + 1 < len(chunk_addrs) else (tail, TAIL)
        t.w32(o + 0x300C, nxt)
        t.w32(o + 0x3010, nxt + size)
        if k in moves:
            t.w32(o + 0x3014, (t.r32(o + 0x3014) + moves[k]) & M32)
        return None

    vt = t.vtable([0, t.callback(fetch, 1, "fetchInput (streaming loader)")])
    t.w32(t.m(this, 0x3030), vt)
    return st


def put_chunks(word_offsets, t, chunks, off):
    """the chunks in input memory: the first at in % 16 == off, the others word
    aligned at the offsets (mod 16) drawn in gen."""
    return [t.put(ch, 16, off if k == 0 else word_offsets[k % len(word_offsets)]) for k, ch in enumerate(chunks)]


def gen_inflate_stream(rng, i, n):
    """inflate_main with the input served in chunks by a streaming fetchInput
    (install_loader): every refill site of inflate_main and inflate_codes goes
    through the hook, which records the written-back state each time.  Streams:
    zlib (raw or gzip) or crafted.  Every 4th case the hook also moves the output
    cursor a few times (the stored-block copy stores its byte at the cursor taken
    before the refill); those have no output oracle."""
    if rng.random() < 0.5:
        data = sample_data(rng, rng.choice([0, 1, 300, rng.randrange(0, 4000), rng.randrange(0, 20000)]))
        raw = rng.random() >= 0.3
        if raw:
            body, how = (deflate_quirky if i % 8 == 7 else deflate_raw)(rng, data)
        else:
            body, how = gzip_file(rng, data, i % 8 == 7)
        ops, produced, notes = [("bytes", data)], len(data), []
    else:
        c, how = crafted_stream(rng)
        raw = rng.random() >= 0.2
        body = c.w.data() if raw else gzip_header(rng) + c.w.data()
        ops, produced, notes = c.ops, c.produced, c.notes
    off = rng.randrange(4) if raw else rng.choice([0, 1, 2])
    chunks = split_chunks(rng, body, off, i % 3 == 1)
    woffs = [4 * rng.randrange(4) for _ in range(8)]
    moves = {}
    if i % 4 == 3:
        for _ in range(rng.randrange(1, 6)):
            moves[rng.randrange(0, max(1, len(chunks) + 2))] = rng.randrange(1, 40)
    slack = 64 + sum(moves.values())

    def run(t):
        obj, this = gz_obj(t)
        region, out, init = out_region(t, produced, slack)
        addrs = put_chunks(woffs, t, chunks, off)
        t.call(GZ_CTOR0, this, ret="ptr")
        st = install_loader(t, this, addrs, [len(ch) for ch in chunks], moves)
        t.call(GZ_INITBITS, this, addrs[0], len(chunks[0]), ret="void")
        t.call(GZ_SETOUT, this, out, ret="void")
        t.call(GZ_MAIN, this, 1 if raw else 0, ret="void", maxsteps=60000000)
        check_arena(t, obj)
        t.obs("fetchInput calls", st["k"])
        if not moves:
            got = t.read(out, produced)
            t.check(got == model_output(init, ops), "inflated output")
        t.call(GZ_DTOR, this, 2, ret="void")
    return Case("inflate_main(%s, %d bytes in %d chunks, in %% 4 = %d: %s%s%s)" % (
        "raw" if raw else "gzip", len(body), len(chunks), off, how,
        "; output cursor moved at fetchInput %s" % sorted(moves) if moves else "",
        "; " + "; ".join(notes) if notes else ""), run)


def gen_inflate_codes(rng, i, n):
    """inflate_codes on its own: tables from huft_build (fixed codes, or crafted
    complete codes with lookup bits 9 / 6 or 1..10), a bit buffer holding
    32 + k0 bits (k0 = bitcount 0..31) with the rest of the block word-aligned at
    inNext, contiguous (inEnd never reached) or served by the streaming loader.
    Checks the output, bl / bd overwritten with the masks, and the state written
    back at the end-of-block code.  The output oracle is the model of the
    original's tables and table walk (HuftModel, model_codes): with small lookup
    bits and long codes gzip-1.2.4's table quirk makes the original decode a
    stream otherwise than it was written; such cases are kept and say so (the
    ones whose decode the models cannot follow are redrawn)."""
    for _ in range(50):
        c = Crafted(rng)
        ntok = rng.choice([0, 1, 4, rng.randrange(0, 200), rng.randrange(0, 1500)])
        if rng.random() < 0.3:
            toks = c.tokens(ntok, 30)
            nl, nd, lit, dist = 288, 30, FIXED_LIT, FIXED_DIST
            lb, db = rng.choice([(7, 5), (7, 5), (rng.randint(1, 10), rng.randint(1, 10))])
            kind = "fixed codes"
        else:
            toks, nl, nd, lit, dist = c.dyn_codes(ntok)
            lb, db = rng.choice([(9, 6), (9, 6), (7, 5), (rng.randint(1, 10), rng.randint(1, 10))])
            kind = "crafted codes nl %d nd %d" % (nl, nd)
        tb = build_tables(lit, lb, dist, db)
        if tb is None:
            continue
        lm, dm = tb
        c.emit(toks, canonical(lit), canonical(dist))
        stream = c.w.data() + junk(rng, 8, 0.2)
        s = int.from_bytes(stream, "little")
        try:
            ops = model_codes(s, 8 * len(stream) + 64, lm, dm, 1 << 18)     # + the 8 zero bytes of rest
            produced = ops_size(ops)
        except Unmodelled:
            continue
        break
    else:
        raise HarnessError("inflate_codes: no case the table model can follow")
    if ops != c.ops:
        c.note("gzip-1.2.4's table quirk (no `if (j < z)`): decoded otherwise than written")
    k0 = rng.randrange(32)
    bitbuf = s & ((1 << (32 + k0)) - 1)
    rest = (s >> (32 + k0)).to_bytes(len(stream) + 8, "little")
    stream_in = rng.random() < 0.5
    chunks = split_chunks(rng, rest, 0, rng.random() < 0.6) if stream_in else [rest]
    woffs = [4 * rng.randrange(4) for _ in range(8)]

    def run(t):
        obj, this = gz_obj(t)
        region, out, init = out_region(t, produced)
        lp = t.put(struct.pack("<%dI" % nl, *lit), 16)
        dp = t.put(struct.pack("<%dI" % nd, *dist), 16)
        addrs = put_chunks(woffs, t, chunks, 0)
        t.call(GZ_CTOR0, this, ret="ptr")
        t.call(GZ_HEAPCLEAR, this, ret="void")
        t.call(GZ_HUFT, this, lp, nl, 257, t.data("cplens"), t.data("cplext"), obj + 0x301C, obj + 0x3024, lb)
        t.call(GZ_HUFT, this, dp, nd, 0, t.data("cpdist"), t.data("cpdext"), obj + 0x3020, obj + 0x3028, db)
        t.check((t.r32(obj + 0x3018) - obj) & M32 == 8 * dm.end, "HuftModel: arena use")
        t.check((t.r32(obj + 0x3024), t.r32(obj + 0x3028)) == (lm.l, dm.l), "HuftModel: lookup bits")
        for m, tl in ((lm, 0x301C), (dm, 0x3020)):
            root = t.r32(obj + tl)
            t.check(root == (0 if m.root is None else obj + 8 * m.root), "HuftModel: table pointer")
            for idx, ent in m.arena.items():
                if None not in ent[1:] and idx < m.end:
                    got = struct.unpack("<IHBB", t.read(obj + 8 * idx, 8))
                    want_t = None if ent[0] is None or ent[3] <= 16 else obj + 8 * ent[0]
                    t.check(got[1:] == tuple(ent[1:]) and want_t in (None, got[0]),
                            "HuftModel: entry %d is %s, model %s" % (idx, got, ent))
        t.w64(obj + 0x3000, bitbuf)
        t.w32(obj + 0x3008, k0)
        t.w32(obj + 0x300C, addrs[0])
        # streamed: the chunk end; contiguous: an odd address the word pointer never reaches
        t.w32(obj + 0x3010, addrs[0] + len(chunks[0]) if stream_in else (addrs[0] + len(rest) + 64) | 1)
        t.w32(obj + 0x3014, out)
        if stream_in:
            install_loader(t, this, addrs, [len(ch) for ch in chunks], {})
        t.call(GZ_CODES, this, ret="void", maxsteps=30000000)
        mask_huft_tables(t, obj, obj + ARENA)
        got = t.read(out, produced)
        t.check(got == model_output(init, ops), "inflated output (table model)")
        t.check((t.r32(obj + 0x3024), t.r32(obj + 0x3028)) == ((1 << lm.l) - 1, (1 << dm.l) - 1),
                "bl / bd hold the masks")
        t.check(t.r32(obj + 0x3014) == out + produced, "out written back")
        t.obs("state", t.read(obj + 0x3000, 0x2C))
        t.call(GZ_DTOR, this, 2, ret="void")
    return Case("inflate_codes(%s, lookup %d/%d, %d tokens, bitcount %d, %s)%s" % (
        kind, lb, db, len(toks), k0, "%d chunks" % len(chunks) if stream_in else "contiguous",
        "; " + "; ".join(c.notes) if c.notes else ""), run)


# ---- the compiler-generated typeinfo function and the Montgomery TU's tail copies

def gen_gz_typeinfo(rng, i, n):
    """__tfQ24GZIP11InflateBase: constructs the __user_type_info node (.sbss) on
    the first call only, returns its address (compared as the type_info token)."""
    calls = 1 + i % 3

    def run(t):
        for _ in range(calls):
            t.call(GZ_TF, ret="ptr")
    return Case("__tfQ24GZIP11InflateBase x%d" % calls, run)


GZ_SWAP_COPY, GZ_LOCK_DUP = 0x0100B2C8, 0x0100B2E0
BN_SWAP_FIRST = 0x01007128


def gen_gz_tail(rng, i, n):
    """the Montgomery TU's out-of-line copies (src/gzip_montgomery_tail.cpp):
    BigNum::swap (exchanges the rep pointers, no refcount traffic) and the empty
    refcount hook bignum_lock_stub (no effect)."""
    if i % 2 == 0:
        a, b = rand_handle_rep(rng, 3), rand_handle_rep(rng, 3)
        same = rng.random() < 0.15

        def run(t):
            if not t.retail and t.fn(GZ_SWAP_COPY) == t.fn(BN_SWAP_FIRST):
                raise HarnessError("0x0100B2C8 resolved to BigNum::swap 0x01007128's rebuilt copy "
                                   "(build/score/symmap.json stale?)")
            ha = put_handle(t, a.put(t) if a else 0)
            hb = ha if same else put_handle(t, b.put(t) if b else 0)
            t.call(GZ_SWAP_COPY, ha, hb, ret="void")
            t.obs("handles", (read_handle(t, ha), read_handle(t, hb)))
        return Case("BigNum::swap copy(%r, %s)" % (a, "itself" if same else repr(b)), run)
    obj, unlock = rng.getrandbits(32), rng.choice([0, 1, rng.getrandbits(32)])
    data = junk(rng, 0x18)

    def run(t):
        p = t.halloc(0x18, data) if obj & 1 else obj
        t.call(GZ_LOCK_DUP, p, unlock, ret="void")
        t.obs("object", t.read(p, 0x18) if obj & 1 else None)
    return Case("bignum_lock_stub copy 0x0100B2E0(%s, %d)" % ("heap object" if obj & 1 else "0x%X" % obj, unlock),
                run)


# ================================================================== modpow (src/gzip_montgomery_tail.cpp)

def gen_modpow(rng, i, n, bits=None):
    bits = bits or rng.choice([3, 32, 64, 96, rng.randrange(3, 160)])
    N = rand_odd_modulus(rng, bits)
    base = rng.randrange(N)
    e = rng.choice([66001, 66001, 65537, 3, rng.randrange(1, 1 << 17)])

    def run(t):
        slot = put_handle(t, 0x5A5A5A5A)
        hb = put_handle(t, Rep(base).put(t) if base else 0)
        he = put_handle(t, Rep(e).put(t))
        hn = put_handle(t, Rep(N).put(t))
        t.call(MODPOW, slot, hb, he, hn, ret="ptr")
        res = read_handle(t, slot)
        t.obs("result", res)
        t.check((res[1] if res else 0) == pow(base, e, N), "modpow")
    return Case("modpow(%d-bit base ^ %d mod %d-bit N)" % (base.bit_length(), e, bits), run)


def gen_modpow_1024(rng, i, n):
    return gen_modpow(rng, i, n, 1024)


SPECS = [
    Spec("crc32", [GZ_CRC32], gen_crc32, 100, "zlib oracle; case 1 = startupa.img"),
    Spec("gzip_file", [GZ_CTOR2, GZ_DTOR], gen_gzip_file, 40, "case 1 = startupa.img; every 8th invalid on purpose"),
    Spec("gzip_raw", [GZ_CTOR0, GZ_INFLATE, GZ_DTOR], gen_gzip_raw, 100, "every 8th invalid on purpose"),
    Spec("gzip_parts", [GZ_CTOR0, GZ_INITBITS, GZ_SETOUT, GZ_HEAPCLEAR, GZ_FETCH, GZ_DTOR], gen_gzip_parts, 40),
    Spec("huft_build", [GZ_HUFT], gen_huft, 150, "every 25th runs away on purpose"),
    Spec("inflate_crafted", [GZ_CTOR0, GZ_INITBITS, GZ_SETOUT, GZ_MAIN, GZ_DTOR], gen_inflate_crafted, 60,
         "crafted streams: nl/nd past the limits, repeat overrun, type-3 final, stored NLEN, 284+31"),
    Spec("inflate_stream", [GZ_CTOR0, GZ_INITBITS, GZ_SETOUT, GZ_MAIN, GZ_DTOR], gen_inflate_stream, 40,
         "input served by a streaming fetchInput subclass; every 4th moves the output cursor"),
    Spec("inflate_codes", [GZ_CTOR0, GZ_HEAPCLEAR, GZ_HUFT, GZ_CODES, GZ_DTOR], gen_inflate_codes, 60,
         "direct, any bitcount, contiguous or streamed input"),
    Spec("gz_typeinfo", [GZ_TF], gen_gz_typeinfo, 6),
    Spec("gz_tail_copies", [GZ_SWAP_COPY, GZ_LOCK_DUP], gen_gz_tail, 20, "Montgomery TU tail: swap / lock hook"),
    Spec("modpow", [MODPOW], gen_modpow, 30),
    Spec("modpow_1024", [MODPOW], gen_modpow_1024, 2, "1024-bit modulus, the RSA path"),
]
