"""libc group: unit libc (runtime/libc.c, runtime/libc_strlen.S; sdk/libgraph/libgraph.c
is in the unit too but not covered: sceGsGetGParam / sceGsResetGraph touch GS registers).
The four syscall stubs at 0x0101B7C0 (runtime/libc_syscall_stubs.S) are tested with a
syscall hook."""
from .common import *

UNITS = ("libc",)

STRLEN, MEMCPY, MEMMOVE, MEMSET = 0x0100F538, 0x0101B010, 0x0101B0BC, 0x0101B1B8
STRCAT, STRCMP, STRCPY, STRNCPY = 0x0101B264, 0x0101B380, 0x0101B4C4, 0x0101B5D8
ERRNO, MEMCMP = 0x01026940, 0x0102694C


def gen_strlen(rng, i, n):
    L = pick_len(rng)
    off = i % 16 if i < 32 else rng.randrange(16)
    s = rstr(rng, L)
    pre, post = junk(rng, off, 0.3), junk(rng, 40, 0.3)

    def run(t):
        p = t.put(pre + s + b"\0" + post, 16)
        r = t.call(STRLEN, p + off)
        t.check(r == L, "strlen = %d, want %d" % (r, L))
    return Case("strlen(len %d, s %% 16 = %d)" % (L, off), run)


def gen_strcmp(rng, i, n):
    L = pick_len(rng, 200)
    a = rstr(rng, L)
    kind = rng.choice(["equal", "equal", "differ", "differ", "prefix_a", "prefix_b", "high"])
    b = bytearray(a)
    if kind == "differ" and L:
        p = rng.randrange(L)
        b[p] = rng.randrange(1, 256)
    elif kind == "high" and L:
        p = rng.randrange(L)
        b[p] = a[p] ^ 0x80 or 1
    elif kind == "prefix_a":
        b += rstr(rng, rng.randrange(1, 20))
    elif kind == "prefix_b" and L:
        b = b[:rng.randrange(L)]
    b = bytes(b)
    oa, ob = pick_align(rng)
    ta, tb = junk(rng, 24, 0.2), junk(rng, 24, 0.2)
    want = c_strcmp(a, b)

    def run(t):
        pa = t.put(a + b"\0" + ta, 16, oa)
        pb = t.put(b + b"\0" + tb, 16, ob)
        r = t.call(STRCMP, pa, pb)
        t.check(s64(r) == want, "strcmp = %d, want %d" % (s64(r), want))
    return Case("strcmp(%s, len %d/%d, align %d/%d)" % (kind, len(a), len(b), oa, ob), run,
                lambda: "a=%r b=%r" % (a, b))


def gen_strcpy(rng, i, n):
    L = pick_len(rng)
    s = rstr(rng, L)
    od, os_ = pick_align(rng)
    fill = junk(rng, L + 40, 0)
    tail = junk(rng, 24, 0.2)

    def run(t):
        d = t.put(fill, 16, od)
        p = t.put(s + b"\0" + tail, 16, os_)
        r = t.call(STRCPY, d, p, ret="ptr")
        t.check(r == d and t.read(d, L + 1) == s + b"\0", "strcpy result")
    return Case("strcpy(len %d, dst %% 16 = %d, src %% 16 = %d)" % (L, od, os_), run)


def gen_strcat(rng, i, n):
    la, lb = pick_len(rng, 120), pick_len(rng, 120)
    a, b = rstr(rng, la), rstr(rng, lb)
    od, os_ = pick_align(rng)
    after = junk(rng, lb + 40, 0.1)
    tail = junk(rng, 24, 0.2)

    def run(t):
        d = t.put(a + b"\0" + after, 16, od)
        p = t.put(b + b"\0" + tail, 16, os_)
        r = t.call(STRCAT, d, p, ret="ptr")
        t.check(r == d and t.read(d, la + lb + 1) == a + b + b"\0", "strcat result")
    return Case("strcat(len %d + %d, dst %% 16 = %d, src %% 16 = %d)" % (la, lb, od, os_), run)


def gen_strncpy(rng, i, n):
    L = pick_len(rng, 120)
    s = rstr(rng, L)
    k = rng.choice([0, 1, L, L + 1, max(0, L - 1), rng.randrange(0, L + 40), rng.randrange(0, 200)])
    if rng.random() < 0.15:           # all bytes >= 0x7F: the only case the broken first quadword test copies
        s = bytes(rng.randrange(0x7F, 0x100) for _ in range(L))
    od, os_ = pick_align(rng)
    fill = junk(rng, k + 40, 0)
    tail = junk(rng, 40, 0.2)

    def run(t):
        d = t.put(fill, 16, od)
        p = t.put(s + b"\0" + tail, 16, os_)
        r = t.call(STRNCPY, d, p, k, ret="ptr")
        t.check(r == d and t.read(d, k) == (s + b"\0" * (k + 1))[:k], "strncpy result")
    return Case("strncpy(len %d, n %d, dst %% 16 = %d, src %% 16 = %d)" % (L, k, od, os_), run)


def gen_memcpy(rng, i, n):
    k = pick_len(rng, 600)
    od, os_ = pick_align(rng)
    src = junk(rng, k, 0.1)
    fill = junk(rng, k + 32, 0)

    def run(t):
        d = t.put(fill, 16, od)
        p = t.put(src, 16, os_)
        r = t.call(MEMCPY, d, p, k, ret="ptr")
        t.check(r == d and t.read(d, k) == src, "memcpy result")
    return Case("memcpy(n %d, dst %% 16 = %d, src %% 16 = %d)" % (k, od, os_), run)


def gen_memmove(rng, i, n):
    """overlapping and disjoint moves inside one region; both ends of the
    destination stay inside it (margin = k + 48 bytes on either side of src)."""
    k = pick_len(rng, 400)
    delta = rng.choice([0, 1, -1, 8, -8, 16, -16, 32, -32, rng.randrange(-80, 81), rng.randrange(-k - 20, k + 21)])
    base_off = rng.choice([0, 0, 8, rng.randrange(16)])
    margin = k + 96
    region = junk(rng, 2 * margin + k + 16, 0.1)

    def run(t):
        r0 = t.put(region, 16)
        s = r0 + margin + base_off
        d = s + delta
        r = t.call(MEMMOVE, d, s, k, ret="ptr")
        exp = bytearray(region)
        exp[d - r0:d - r0 + k] = region[s - r0:s - r0 + k]
        t.check(r == d and t.read(r0, len(region)) == bytes(exp), "memmove result")
    return Case("memmove(n %d, delta %d, src %% 16 = %d)" % (k, delta, (margin + base_off) % 16), run)


def gen_memset(rng, i, n):
    k = pick_len(rng, 600)
    c = rng.choice([0, 0xFF, 0x80, rng.randrange(256), rng.randrange(-1000, 70000), rng.getrandbits(32)])
    od = rng.choice([0, 0, 8, rng.randrange(16)])
    fill = junk(rng, k + 32, 0)

    def run(t):
        d = t.put(fill, 16, od)
        r = t.call(MEMSET, d, c, k, ret="ptr")
        t.check(r == d and t.read(d, k) == bytes([c & 0xFF]) * k and t.read(d + k, 32) == fill[k:k + 32],
                "memset result")
    return Case("memset(n %d, c 0x%X, dst %% 16 = %d)" % (k, c & M32, od), run)


def gen_memcmp(rng, i, n):
    k = pick_len(rng, 300)
    a = junk(rng, k, 0.1)
    b = bytearray(a)
    if k and rng.random() < 0.65:
        p = rng.randrange(k)
        b[p] = rng.randrange(256)
        if rng.random() < 0.3 and p + 1 < k:
            b[rng.randrange(p + 1, k)] = rng.randrange(256)
    b = bytes(b)
    oa, ob = pick_align(rng)
    want = next((x - y for x, y in zip(a, b) if x != y), 0)
    junk_fixed = junk(rng, 20, 0.2)

    def run(t):
        pa = t.put(a + junk_fixed, 16, oa)
        pb = t.put(b + junk_fixed[::-1], 16, ob)
        r = t.call(MEMCMP, pa, pb, k)
        t.check(s64(r) == want, "memcmp = %d, want %d" % (s64(r), want))
    return Case("memcmp(n %d, align %d/%d)" % (k, oa, ob), run)


def gen_errno(rng, i, n):
    def run(t):
        t.call(ERRNO, ret="ptr")
    return Case("__errno()", run)


SPECS = [
    Spec("strlen", [STRLEN], gen_strlen, 300),
    Spec("strcmp", [STRCMP], gen_strcmp, 300),
    Spec("strcpy", [STRCPY], gen_strcpy, 200),
    Spec("strcat", [STRCAT], gen_strcat, 200),
    Spec("strncpy", [STRNCPY], gen_strncpy, 250),
    Spec("memcpy", [MEMCPY], gen_memcpy, 250),
    Spec("memmove", [MEMMOVE], gen_memmove, 250),
    Spec("memset", [MEMSET], gen_memset, 250),
    Spec("memcmp", [MEMCMP], gen_memcmp, 250),
    Spec("errno", [ERRNO], gen_errno, 3, "pointer into .data (impure_data)"),
]
SPECS += [s for s in [syscall_stub_spec("libc_syscall_stubs", UNITS, 4)] if s]
