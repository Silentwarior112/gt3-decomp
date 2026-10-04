"""gameflow group: units main (src/main.cpp, main_iop.cpp, crt0.S, crtbegin.c), startup
(src/startup_screen.cpp, tex1.cpp) and imageloader (src/imageloader.cpp,
imageloader_source.cpp).

  * imageloader: sha512_digest_to_bignum (hashlib oracle); Buffer; the Source
    classes (accessors, constructors, virtual destructors, typeinfo functions);
    ImageLoader::decompress / verify / load on generated signed core.gt3-style
    images (n = digest + 1, so the 513-bit modpow accepts them); the boot
    sources' open / getLoadFlags / close and the host_* helpers against the model
    IOP (common.Iop with the FILEIO and mcserv server models of kernl_rest /
    sonyio);
  * startup: the plain-C Tex1 / CLUT animation library of tex1.cpp (oracles for
    the TBP / DBP rebase, the MMI CLUT blend, the float timeline),
    tex1_upload_to_vram on a gs World (GIF DMA snapshots), and the warning
    screen: startup_screen_start (the real or a stubbed inflater), the render
    thread's 151 frames with its GS calls stubbed (float arguments compared as
    bit patterns), draw_frame on stubs and on the real driver, the semaphore
    helpers;
  * main: _start (crt0: .bss clear, InitMainThread / InitHeap with the linker
    symbols, Exit ends the test), the crt glue (__register_frame_info / _exit /
    _root), main and select_boot_source with their callees stubbed (scripted
    probes and opens; entry 0 spins: a runaway on purpose), and main_iop's boot
    helpers (pad, IRX loading, IOP reboot, cdvd) against the model IOP.
Not testable: nop_padding 0x01000000 (two nops in front of the entry point,
never executed)."""
from .common import *
from difftest import HEAP_LO, HEAP_HI

UNITS = ("main", "startup", "imageloader")

SHA_TO_BIGNUM = 0x010043A8
TEX1_SET_VRAM, TEX1_RELOCATE, CLUT_RESET, CLUT_APPLY = 0x0100B2E8, 0x0100B560, 0x0100B680, 0x0100B6D0
CLUT_APPLY_ALL, CLUT_STEP, CLUT_RELEASE, CLUT_RESET_ALL = 0x0100B798, 0x0100B808, 0x0100BB28, 0x0100BBA0
CLUT_TABLE_RELOC = 0x0100BC40


def fbits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


# ================================================================== imageloader

def gen_sha_bignum(rng, i, n):
    L = rng.choice([0, 1, 128, rng.randrange(0, 700)])
    data = junk(rng, L, 0.05)
    rng_off = rng.randrange(16)

    def run(t):
        slot = put_handle(t, 0x5A5A5A5A)
        p = t.put(data, 16, rng_off)
        t.call(SHA_TO_BIGNUM, slot, p, L, ret="ptr")
        res = read_handle(t, slot)
        t.obs("digest", res)
        t.check(res[1] == int.from_bytes(hashlib.sha512(data).digest(), "big"), "digest as bignum")
    return Case("sha512_digest_to_bignum(%d bytes)" % L, run)


# ================================================================== Tex1 (src/tex1.cpp, include/startup_screen.h)
# Tex1Image 0x30: magic, self, ptr08, size, u16 vramBase, vramSize, nTex, nXfer,
#   tex (GsTexRegs[nTex], 0x28 each: TEX0 TEX1 MIPTBP1 MIPTBP2 CLAMP), xfer
#   (Tex1Xfer[nXfer], 0xC: data, u16 dbp, u8 dbw, u8 dpsm, u16 w, u16 h),
#   ptrTable ({count, ptr[count]}), clutAnim (ClutAnimTable {u16 count, pad, ch}).
# ClutAnimChannel 0x20: u16 id, mode, nColors, nKeys; dest; keys; u16 cur, next;
#   f32 time, lerp; s16 hold; s8 dir; u8 dirty.   ClutKey 8: f32 time; palette.

IMG_FMT = "<IIIIHHHHIIII8x"
CH_FMT = "<HHHHIIHHIIhbB"


def rebase14(reg, shift, delta):
    f = ((reg >> shift) & 0x3FFF) + delta
    return (reg & ~(0x3FFF << shift)) | ((f & 0x3FFF) << shift)


def gen_tex1_set_vram(rng, i, n):
    ntex = rng.choice([0, 1, 1, 2, 4])
    regs = []
    for _ in range(ntex):
        r = [rng.getrandbits(64) for _ in range(5)]
        mxl = rng.choice([0, 0, 1, 3, 4, 6, 7])
        r[1] = (r[1] & ~(7 << 2)) | (mxl << 2)                 # TEX1.MXL
        regs.append(r)
    nx = rng.choice([0, 1, 3, 6])
    xfer_null = rng.random() < 0.2
    xf = [(rng.getrandbits(32), rng.getrandbits(16), rng.getrandbits(8), rng.getrandbits(8),
           rng.getrandbits(16), rng.getrandbits(16)) for _ in range(nx)]
    vbase = rng.getrandbits(16)
    addr = (vbase << 6) | rng.randrange(64) if rng.random() < 0.2 else rng.choice(
        [0xA0000, rng.getrandbits(22), rng.getrandbits(32)])

    def run(t):
        tex = t.put(b"".join(struct.pack("<5Q", *r) for r in regs), 16) if ntex else 0
        xfer = 0 if xfer_null else t.put(b"".join(struct.pack("<IHBBHH", *x) for x in xf) or b"\0" * 4, 16)
        img = t.put(struct.pack(IMG_FMT, 0x31786554, 0, 0, 0x38020, vbase, 0x37F, ntex, nx, tex, xfer, 0, 0), 16)
        r = t.call(TEX1_SET_VRAM, img, addr, ret="ptr")
        t.check(r == img, "returns img")
        # oracle: the 14-bit TBP fields and the 16-bit DBPs move by delta
        base = addr >> 6
        delta = (base - vbase) & M32
        delta = delta - (1 << 32) if delta >> 31 else delta
        want = []
        for rr in regs:
            rr = list(rr)
            if delta:
                rr[0] = rebase14(rebase14(rr[0], 0, delta), 37, delta)
                mxl = (rr[1] >> 2) & 7
                if mxl > 0:
                    rr[2] = rebase14(rebase14(rebase14(rr[2], 0, delta), 20, delta), 40, delta)
                    if mxl >= 4:
                        rr[3] = rebase14(rebase14(rebase14(rr[3], 0, delta), 20, delta), 40, delta)
            want += rr
        if ntex:
            t.check(list(struct.unpack("<%dQ" % (5 * ntex), t.read(tex, 0x28 * ntex))) == want, "GsTexRegs")
        if xfer and nx:
            got = [struct.unpack_from("<IHBBHH", t.read(xfer, 12 * nx), 12 * k)[1] for k in range(nx)]
            t.check(got == [(x[1] + (delta if delta else 0)) & 0xFFFF for x in xf], "transfer DBPs")
        t.check(t.r16(img + 0x10) == (base & 0xFFFF if delta else vbase), "vramBase")
    return Case("tex1_set_vram_base(%d tex, %d xfer%s, base 0x%X -> addr 0x%X)" % (
        ntex, nx, " (NULL)" if xfer_null else "", vbase, addr), run)


def rand_channel_parts(rng):
    """(nColors, keys [(time, palette words)], cur, next, mode, fields) for a valid channel."""
    nkeys = rng.choice([1, 2, 2, 3, 4])
    ncol = rng.choice([0, 1, 3, 16, rng.randrange(1, 40)])
    keys = []
    for _ in range(nkeys):
        tm = rng.choice([0.25, 0.5, 1.0, 2.0, 10.0, rng.uniform(0.05, 4.0)])
        if rng.random() < 0.2:
            tm = -tm                                             # a hold key
        keys.append((tm, [rng.getrandbits(32) for _ in range(ncol)]))
    mode = rng.choice([0, 0, 1, 2, 3]) if nkeys >= 2 else rng.choice([0, 2, 3])
    cur = rng.randrange(nkeys)
    nxt = rng.randrange(nkeys)
    return ncol, keys, cur, nxt, mode


def put_channels(t, chans):
    """lay out ClutAnimChannel[len(chans)] with their keys, palettes and dests;
    chans = [(id, ncol, keys, cur, next, mode, time, lerp, hold, dir, dirty, dest_null)].
    Returns (channel array address, [dest addresses])."""
    blobs, dests = [], []
    for (cid, ncol, keys, cur, nxt, mode, tm, lerp, hold, dr, dirty, dest_null) in chans:
        pals = [t.put(struct.pack("<%dI" % ncol, *p) if ncol else b"\0" * 4, 16) for _, p in keys]
        ka = t.put(b"".join(struct.pack("<fI", k[0], pa) for k, pa in zip(keys, pals)), 8)
        dest = 0 if dest_null else t.put(b"\xEE" * (4 * ncol + 8), 16)
        dests.append(dest)
        blobs.append(struct.pack(CH_FMT, cid, mode, ncol, len(keys), dest, ka, cur, nxt, fbits(tm), fbits(lerp),
                                 hold, dr, dirty))
    return t.put(b"".join(blobs), 16), dests


def rand_chan(rng, ids=(0, 1, 2, 7)):
    ncol, keys, cur, nxt, mode = rand_channel_parts(rng)
    tm = rng.choice([0.0, rng.uniform(0.0, 1.0), rng.uniform(0.0, abs(keys[cur][0]))])
    lerp = rng.choice([1.0, 0.0, 0.5, rng.random(), rng.uniform(-9.0, 9.0)])
    hold = rng.choice([0, 0, 0, 1, -1, 2])
    dr = rng.choice([1, 1, -1])
    dirty = rng.choice([0, 1, 1])
    return (rng.choice(ids), ncol, keys, cur, nxt, mode, tm, lerp, hold, dr, dirty, False)


def clut_blend(a, b, w):
    s16 = lambda x: ((x + 0x8000) & 0xFFFF) - 0x8000
    out = 0
    for k in range(4):
        s = ((a >> 8 * k & 0xFF) * s16(w) + (b >> 8 * k & 0xFF) * s16(0x1000 - w)) & M32
        out |= ((s >> 12) & 0xFF) << 8 * k
    return out


def gen_clut_apply(rng, i, n):
    """clut_anim_channel_apply: the per-byte 4.12 blend done with MMI (pextlw /
    pextlb / pextlh / pmulth / pmaddh / psrlw / ppach / ppacb) in the original.
    lerp = k / 4096 is exact; |k| up to 40000 exercises the s16 truncation."""
    ch = rand_chan(rng)
    kk = rng.choice([0, 4096, 2048, rng.randrange(4097), rng.randrange(-40000, 40000)])
    ch = ch[:7] + (kk / 4096.0,) + ch[8:]

    def run(t):
        ca, dests = put_channels(t, [ch])
        t.call(CLUT_APPLY, ca, ret="void")
        cid, ncol, keys, cur, nxt, mode, tm, lerp, hold, dr, dirty, _ = ch
        want = [clut_blend(a, b, kk) for a, b in zip(keys[cur][1], keys[nxt][1])] if dirty else [0xEEEEEEEE] * ncol
        got = list(struct.unpack("<%dI" % ncol, t.read(dests[0], 4 * ncol)))
        t.check(got == want and t.read(dests[0] + 4 * ncol, 8) == b"\xEE" * 8 and t.r8(ca + 0x1F) == 0,
                "blend (lerp %d/4096)" % kk)
    return Case("clut_anim_channel_apply(lerp %d/4096, %d colours, dirty %d)" % (kk, ch[1], ch[10]), run)


def table_case(rng, shape):
    """an image whose clutAnim table is NULL / has a NULL channel array / count 0
    / count 1..4 random channels."""
    if shape == "null_table":
        return None
    if shape == "null_ch":
        return ("null_ch", rng.randrange(0, 4), [])
    if shape == "empty":
        return ("empty", 0, [rand_chan(rng)])
    chans = [rand_chan(rng) for _ in range(rng.randrange(1, 5))]
    return ("chans", len(chans), chans)


def put_table_image(t, tab):
    if tab is None:
        tbl = 0
    else:
        kind, count, chans = tab
        ca = put_channels(t, chans)[0] if chans and kind != "null_ch" else 0
        tbl = t.put(struct.pack("<HHI", count, 0x5A5A, ca), 8)
    return t.put(struct.pack(IMG_FMT, 0x31786554, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, tbl), 16)


def clut_table_run(t, op, tab, cid, dt):
    img = put_table_image(t, tab)
    if op == "step":
        t.call(CLUT_STEP, img, cid & M64, ret="void", floats=(fbits(dt),), maxsteps=2000000)
    elif op == "release":
        t.call(CLUT_RELEASE, img, cid & M64, ret="void")
    elif op == "apply_all":
        t.call(CLUT_APPLY_ALL, img, ret="void")
    else:
        t.call(CLUT_RESET_ALL, img, ret="void")


def clut_table_desc(op, shape, tab, cid, dt):
    desc = "%s(%s%s)" % (op, shape if tab is None or tab[0] != "chans" else "%d channels" % tab[1],
                         ", id %d" % cid if op in ("step", "release") else "")
    if op == "step":
        desc += ", dt %r" % dt
    return "clut_anim_" + desc


def gen_clut_table_ops(rng, i, n):
    """clut_anim_step / clut_anim_release / clut_anim_apply_all / clut_anim_reset_all
    on an image's channel table (dead code in the bootstrap, but linked)."""
    op = ["step", "step", "step", "release", "apply_all", "reset_all"][i % 6]
    shape = rng.choice(["chans"] * 7 + ["null_table", "null_ch", "empty"])
    tab = table_case(rng, shape)
    cid = rng.choice([-1, -1, 0, 1, 2, 7, 3])
    dt = rng.choice([0.0, 0.016, 0.1, 0.5, 1.0, 3.0, -0.1, -0.5, -2.0, rng.uniform(-5.0, 5.0)])
    return Case(clut_table_desc(op, shape, tab, cid, dt), lambda t: clut_table_run(t, op, tab, cid, dt))


def edge_chan(rng):
    """a valid channel with edge field values: u16 ids and modes with bit 15 set
    (zero-extended in the id compare and the mode switch; any mode above 2 is
    the default branch), holds at the s16 limits (release wraps -0x8000 to
    0x7FFF, step increments -0x8000), times of +0.0 / -0.0."""
    ncol, keys, cur, nxt, mode = rand_channel_parts(rng)
    if rng.random() < 0.4:
        mode = rng.choice([3, 4, 0x7FFF, 0x8000, 0xFFFF])
    tm = rng.choice([0.0, -0.0, rng.uniform(0.0, abs(keys[cur][0]))])
    lerp = rng.choice([1.0, 0.5, rng.uniform(-2.0, 2.0)])
    hold = rng.choice([-0x8000, -0x7FFF, -2, -1, 0, 0, 1, 0x7FFE, 0x7FFF])
    return (rng.choice([0, 1, 0x7FFF, 0x8000, 0xFFFF]), ncol, keys, cur, nxt, mode, tm, lerp, hold,
            rng.choice([1, -1]), rng.choice([0, 1]), False)


def gen_clut_table_edges(rng, i, n):
    """the CLUT table entry points on edge values (see edge_chan) with call ids
    that only match a bit-15 channel id when it is zero-extended (0x8000,
    0xFFFF), match none (0x10000, 0x18000) or select every channel (any
    negative id), and dt / channel time of -0.0."""
    op = ["step", "step", "step", "release", "release", "apply_all", "reset_all"][i % 7]
    chans = [edge_chan(rng) for _ in range(rng.randrange(1, 7))]
    tab = ("chans", len(chans), chans)
    cid = rng.choice([-1, -0x8000, -0x10000, 0, 1, 0x7FFF, 0x8000, 0xFFFF, 0x10000, 0x18000, 0x7FFFFFFF])
    dt = rng.choice([0.0, -0.0, 0.016, 0.5, -0.016, -0.5, rng.uniform(-3.0, 3.0)])
    return Case(clut_table_desc(op, "chans", tab, cid, dt) + " [edges]", lambda t: clut_table_run(t, op, tab, cid, dt))


def gen_clut_reset(rng, i, n):
    ch = rand_chan(rng)
    keys_null = rng.random() < 0.1

    def run(t):
        ca, _ = put_channels(t, [ch])
        if keys_null:
            t.w32(ca + 0xC, 0)                       # keys[0] is read anyway: address 0 (RAM in the model)
        t.call(CLUT_RESET, ca, ret="void")
        cid, ncol, keys, cur, nxt, mode = ch[:6]
        tm, lerp, hold, dr, dirty = struct.unpack_from("<IIhbB", t.read(ca + 0x14, 0xC))
        t.check((t.r16(ca + 0x10), t.r16(ca + 0x12), tm, lerp, dr, dirty) ==
                (0, 1 if len(keys) else 0, 0, fbits(1.0), 1, ch[10]), "reset fields")
        if not keys_null:
            t.check(hold == (1 if keys[0][0] < 0 else 0), "hold from keys[0].time")
    return Case("clut_anim_channel_reset(%d keys%s)" % (len(ch[2]), ", keys NULL" if keys_null else ""), run)


def gen_tex1_relocate(rng, i, n, kinds=("zero", "zero", "other", "same")):
    """tex1_relocate (and through it clut_anim_table_relocate and
    clut_anim_channel_reset): a set laid out with file offsets (self = 0, or
    another base), some pointers NULL, then relocated to where it sits.
    kinds "high" (self above the set: a negative delta) and "wrap" (self
    0x34..0x3FC below 2^32, so nearly every stored pointer has wrapped past 0;
    4 mod 8, so none of the 8-aligned offsets becomes NULL) are
    tex1_relocate_edges'."""
    ntex = rng.choice([0, 1, 4])
    nx = rng.choice([0, 1, 6])
    nptr = rng.choice([0, 1, 3])
    has = {k: rng.random() < 0.8 for k in ("ptr08", "tex", "xfer", "ptrTable", "clutAnim")}
    nch = rng.choice([0, 1, 3])
    ch_null = rng.random() < 0.15
    chans = []
    for _ in range(nch):
        ncol, keys, cur, nxt, mode = rand_channel_parts(rng)
        chans.append((rng.getrandbits(16), mode, ncol, keys, rng.random() < 0.15, rng.random() < 0.15,
                      [rng.random() < 0.15 for _ in keys]))
    self_kind = rng.choice(list(kinds))
    other = rng.getrandbits(24) & ~3
    xdata = [rng.getrandbits(20) for _ in range(nx)]
    ptrs = [0 if rng.random() < 0.3 else rng.getrandbits(20) for _ in range(nptr)]
    junkregs = [rng.getrandbits(64) for _ in range(5 * ntex)]

    def run(t):
        base = t.alloc(0x4000, 16)
        selfv = {"zero": 0, "other": other, "same": base, "high": 0x40000000 + other,
                 "wrap": 0xFFFFFFCC - (other & 0x3C8)}[self_kind]
        blob = bytearray(0x4000)
        pos = [0x30]

        def place(data, align=16):
            p = (pos[0] + align - 1) & -align
            blob[p:p + len(data)] = data
            pos[0] = p + len(data)
            return p

        def ptr(off, present=True):
            return (selfv + off) & M32 if present else 0
        tex = place(struct.pack("<%dQ" % len(junkregs), *junkregs) if junkregs else b"\0" * 8)
        xfer = place(b"".join(struct.pack("<IHBBHH", d, 0, 0, 0, 1, 1) for d in xdata) or b"\0" * 4)
        ptab = place(struct.pack("<I%dI" % nptr, nptr, *ptrs))
        chblob = []
        for (cid, mode, ncol, keys, dnull, knull, pnull) in chans:
            pal_offs = [place(struct.pack("<%dI" % ncol, *p) if ncol else b"\0" * 4) for _, p in keys]
            koff = place(b"".join(struct.pack("<fI", k[0], ptr(po, not pn)) for k, po, pn in
                                  zip(keys, pal_offs, pnull)), 8)
            doff = place(b"\0" * (4 * ncol + 4))
            chblob.append(struct.pack(CH_FMT, cid, mode, ncol, len(keys), ptr(doff, not dnull), ptr(koff, not knull),
                                      1, 1, fbits(0.5), fbits(0.25), 5, -1, 1))
        choff = place(b"".join(chblob) or b"\0" * 4)
        tbl = place(struct.pack("<HHI", nch, 0, ptr(choff, not ch_null)), 8)
        struct.pack_into(IMG_FMT, blob, 0, 0x31786554, selfv, ptr(0x1000, has["ptr08"]), 0x4000, 0, 0, ntex, nx,
                         ptr(tex, has["tex"]), ptr(xfer, has["xfer"]), ptr(ptab, has["ptrTable"]),
                         ptr(tbl, has["clutAnim"]))
        if pos[0] > 0x4000:
            raise HarnessError("tex1_relocate spec: blob too large")
        t.write(base, bytes(blob))
        r = t.call(TEX1_RELOCATE, base, ret="ptr")
        t.check(r == base and t.r32(base + 4) == base, "self relocated")
        for k, (fld, off) in enumerate((("ptr08", 0x1000), ("tex", tex), ("xfer", xfer), ("ptrTable", ptab),
                                        ("clutAnim", tbl))):
            want = (base + off) if has[fld] else 0
            if self_kind == "same":
                want = ptr(off, has[fld])
            t.check(t.r32(base + [0x08, 0x18, 0x1C, 0x20, 0x24][k]) == want, "%s relocated" % fld)
    return Case("tex1_relocate(self %s, %d tex, %d xfer, %d ptrs, %d channels%s, NULL: %s)" % (
        self_kind, ntex, nx, nptr, nch, " (ch NULL)" if ch_null else "",
        ",".join(k for k, v in has.items() if not v) or "-"), run)


def gen_tex1_relocate_edges(rng, i, n):
    return gen_tex1_relocate(rng, i, n, ("high", "wrap"))


SPECS = [
    Spec("sha512_to_bignum", [SHA_TO_BIGNUM], gen_sha_bignum, 60, "hashlib oracle"),
    Spec("tex1_set_vram_base", [TEX1_SET_VRAM], gen_tex1_set_vram, 150, "oracle: 14-bit TBP / 16-bit DBP rebase"),
    Spec("tex1_relocate", [TEX1_RELOCATE, CLUT_TABLE_RELOC, CLUT_RESET], gen_tex1_relocate, 150),
    Spec("clut_channel_reset", [CLUT_RESET], gen_clut_reset, 60),
    Spec("clut_channel_apply", [CLUT_APPLY], gen_clut_apply, 150, "MMI blend oracle"),
    Spec("clut_table_ops", [CLUT_STEP, CLUT_RELEASE, CLUT_APPLY_ALL, CLUT_RESET_ALL, CLUT_APPLY, CLUT_RESET],
         gen_clut_table_ops, 300, "float timeline (EE float model)"),
    Spec("clut_table_edges", [CLUT_STEP, CLUT_RELEASE, CLUT_APPLY_ALL, CLUT_RESET_ALL, CLUT_APPLY, CLUT_RESET],
         gen_clut_table_edges, 140, "bit-15 ids / modes, s16 hold limits, -0.0 times"),
    Spec("tex1_relocate_edges", [TEX1_RELOCATE, CLUT_TABLE_RELOC, CLUT_RESET], gen_tex1_relocate_edges, 60,
         "negative delta, pointers wrapped past 2^32"),
]


# ================================================================== hardware / IOP-facing gameflow
# Buffer, the ImageLoader and its boot sources, main's boot helpers, crt glue and
# the Tex1 upload - run against the fake kernel and the model IOP of common.py.

from .kernl_rest import FileioServer, LoadfileServer
from .sonyio import McServer, PadServer, CdServers
from . import gs as _gs

GF = dict(
    SourceRecord_dtor=0x01000140, SourceRecord_ctor=0x010001A0, SourceRecord_release=0x010001B8,
    load_irx_module=0x01000790, boot_iop=0x01000880, shutdown_iop=0x010009D8, read_pad_buttons=0x01000A08,
    tf_mc10=0x010006B0, dt_mc10=0x01000700, tf_mc00=0x01000720, dt_mc00=0x01000770,
    Buffer_dtor=0x01000ED8, Buffer_ctor_n=0x01000F20, Buffer_copy=0x01000F58, Buffer_assign=0x01000FB8,
    Buffer_adopt=0x01001020, Buffer_resize=0x01001070, Source_dtor=0x010041D0, decompress=0x01004248,
    verify=0x010044A8, load=0x010049C8, GetPath=0x01004B08, GetName=0x01004B38, GetDataEnd=0x01004B48,
    GetDataEnd_c=0x01004B58, GetDataEndAddr=0x01004B68, GetData=0x01004B78, GetData_c=0x01004B80,
    GetDataAddr=0x01004B88, GetSize=0x01004B90, Source_ctor=0x01004B98, tf_Source=0x01004BB8,
    Host_ctor=0x01004BF8, Host_close=0x01004C18, Host_open=0x01004C60, Host_flags=0x01004DB0, tf_Host=0x01004E80,
    dt_Host=0x01004ED0, Disk_ctor=0x01004EF0, Disk_close=0x01004F10, Disk_open=0x01004F58, Disk_flags=0x010051F8,
    tf_Disk=0x01005358, dt_Disk=0x010053A8, mcard_is_usable=0x010053C8, MC_ctor=0x01005428, MC_close=0x01005450,
    MC_open=0x01005458, MC_flags=0x01005748, tf_MC=0x01005900, dt_MC=0x01005950, host_read_file=0x01005970,
    host_write_file=0x01005A78, host_file_size=0x01005B08, reg_frame=0x010000C8, dereg_frame=0x01000108,
    exit_=0x010000B8, root=0x010000C0, tex1_upload=0x0100BD38)


def gen_buffer(rng, i, n):
    which = ["dtor", "ctor_n", "copy", "assign", "adopt", "resize", "resize"][i % 7]
    size = rng.choice([0, 1, 8, 0x40, 0x101, rng.randrange(0, 0x400)])
    nsize = rng.choice([0, 1, 7, 0x40, 0x200, rng.randrange(0, 0x400), size])
    null = rng.random() < 0.2
    flags = rng.choice([0, 1, 2, 3])
    data = junk(rng, 0x400)

    def run(t):
        b = t.halloc(8)
        blk = 0 if null else t.halloc(max(size, 1), data[:max(size, 1)])
        t.write(b, struct.pack("<II", blk, size))
        if which == "dtor":
            t.call(GF["Buffer_dtor"], b, flags, ret="void")
            return
        if which == "ctor_n":
            t.call(GF["Buffer_ctor_n"], b, nsize, ret="ptr")
        elif which in ("copy", "assign"):
            o = t.halloc(8)
            oblk = 0 if (null and nsize == 0) else t.halloc(max(nsize, 1), data[::-1][:max(nsize, 1)])
            t.write(o, struct.pack("<II", oblk, nsize))
            t.call(GF["Buffer_copy" if which == "copy" else "Buffer_assign"], b, o, ret="ptr")
        elif which == "adopt":
            p = t.halloc(max(nsize, 1))
            t.call(GF["Buffer_adopt"], b, p, nsize, ret="ptr")
        else:
            t.call(GF["Buffer_resize"], b, nsize, ret="void")
        d, s = struct.unpack("<II", t.read(b, 8))
        t.obs("buffer", (d, s), "ptrs")
        if d:
            t.obs("contents", t.read(d, min(s, 0x400)))
    return Case("Buffer::%s(size %d -> %d, %s, flags %d)" % (which, size, nsize, "NULL" if null else "data", flags),
                run)


def gen_source_misc(rng, i, n):
    which = ["accessors", "accessors", "ctors", "dtors", "typeinfo", "records"][i % 6]
    size = rng.choice([0, 1, 0x40, rng.getrandbits(32)])
    flags = rng.choice([0, 1, 2, 3])
    kind = rng.choice(["Host", "Disk", "MC", "mc10", "mc00", "Source"])
    order = rng.sample(["tf_Source", "tf_Host", "tf_Disk", "tf_MC", "tf_mc10", "tf_mc00"], 6)
    port, slot = rng.randrange(2), rng.randrange(2)

    def run(t):
        name = t.cstr(b"CORE.GT3")
        path = t.cstr(b"cdrom0:/CORE.GT3;1")
        gp = t.callback(lambda tt, a: path, 1, "FileName::GetPath")
        fdt = t.callback(lambda tt, a: 0, 2, "FileName::~FileName")
        vt = t.vtable([fdt, gp])
        fname = t.objput(struct.pack("<III", name, 0, vt), LAYOUT["FileName"])
        data = t.halloc(0x40)
        src = t.halloc_obj(0x18, LAYOUT["MCardSource"], struct.pack("<IIII", data, size, fname, 0))
        if which == "accessors":
            for f in ("GetPath", "GetName", "GetDataEnd", "GetDataEnd_c", "GetDataEndAddr", "GetData", "GetData_c",
                      "GetDataAddr", "GetSize"):
                t.call(GF[f], src, ret="ptr")
        elif which == "ctors":
            for f, args in (("Source_ctor", ()), ("Host_ctor", ()), ("Disk_ctor", ()), ("MC_ctor", (port, slot))):
                o = t.halloc_obj(0x18, LAYOUT["MCardSource"])
                t.call(GF[f], o, *args, ret="ptr")
                t.obs(f, t.objwords(o, 0x18), "ptrs")
        elif which == "dtors":
            f = {"Host": "dt_Host", "Disk": "dt_Disk", "MC": "dt_MC", "mc10": "dt_mc10", "mc00": "dt_mc00",
                 "Source": "Source_dtor"}[kind]
            t.call(GF[f], src, flags, ret="void")
        elif which == "typeinfo":
            for f in order:
                t.call(GF[f], ret="ptr")
            t.call(GF[order[0]], ret="ptr")
        else:
            rec = t.halloc(0xC)
            # +2..+3 is padding after the u16 loadFlags: retail's `sh` leaves
            # it alone, GCC 14 may merge the stores into an `sw` over it
            t.unchecked(rec + 2, 2)
            t.call(GF["SourceRecord_ctor"], rec, ret="ptr")
            t.w32(rec + 8, src)
            vtab = t.vtable([t.fn(GF["Source_dtor"])])
            t.w32(t.m(src, 0xC), vtab)
            if flags & 2:
                t.call(GF["SourceRecord_release"], rec, ret="ptr")
            t.call(GF["SourceRecord_dtor"], rec, flags & 1, ret="void")
    return Case("Source %s (%s, flags %d, size 0x%X)" % (which, kind, flags, size), run)


# ---- ImageLoader: signed containers -------------------------------------------------

def make_container(rng, kind=None):
    """(container bytes, entry, sections, kind) - a core.gt3-style image: u16
    flags, u32 rawSize, raw DEFLATE of {u16 modLen, modulus, u16 sigLen,
    signature, payload}.  'good' images use n = digest + 1, sig = digest
    (accepted: the digest is made even, and (-1)^66001 = -1 = digest mod n)."""
    nsec = rng.choice([0, 1, 2, 3])
    secs = []
    for k in range(nsec):
        secs.append((0x01780000 + 0x1000 * k + rng.randrange(0, 0x100), junk(rng, rng.choice([0, 4, 0x33, 0x200]))))
    entry = rng.getrandbits(32)
    for _ in range(64):
        payload = struct.pack("<II", nsec, entry) + b"".join(struct.pack("<II", a, len(d)) + d for a, d in secs) + \
            junk(rng, rng.randrange(0, 16))
        digest = int.from_bytes(hashlib.sha512(payload).digest(), "big")
        if digest % 2 == 0:
            break
    kind = kind or rng.choice(["good", "good", "good", "badsig", "short_mod"])
    n_ = digest + 1
    sig = digest
    if kind == "badsig":
        sig = digest ^ (1 << rng.randrange(0, 500))
    mod = n_.to_bytes((n_.bit_length() + 7) // 8, "big")
    sigb = sig.to_bytes(max(1, (sig.bit_length() + 7) // 8), "big")
    if kind == "short_mod":
        mod = mod[:rng.randrange(1, 8)] or b"\x03"
        if not mod[-1] & 1:
            mod = mod[:-1] + bytes([mod[-1] | 1])                  # an odd modulus (Montgomery)
    body = struct.pack("<H", len(mod)) + mod + struct.pack("<H", len(sigb)) + sigb + payload
    co = zlib.compressobj(9, zlib.DEFLATED, -15)
    raw = co.compress(body) + co.flush()
    raw_size = len(body) + rng.choice([0, 0, 0, 16])
    return struct.pack("<HI", rng.getrandbits(16), raw_size) + raw, entry, secs, kind


def gen_imageloader(rng, i, n):
    # an accepted image costs a 513-bit modpow (~7M instructions): few cases
    which, kind, pre = [("load", "good", False), ("load", "badsig", False), ("verify", "short_mod", False),
                        ("load", "good", True), ("decompress", "good", False), ("verify", "badsig", True)][i % 6]
    cont, entry, secs, kind = make_container(rng, kind)

    def run(t):
        Kernel().install(t)
        src = t.halloc_obj(0x10, LAYOUT["Source"])
        blk = t.halloc(len(cont), cont)
        t.objwrite(src, struct.pack("<IIII", blk, len(cont), 0, 0))
        il = t.alloc(0x24, 16)
        t.write(il, struct.pack("<9I", src, 0, 0, 0, 0, 0, 0, 0, 0))
        if pre:                                                   # image already there: decompress skips
            t.call(GF["decompress"], il, ret="void", label="ImageLoader::decompress (setup)", maxsteps=20000000)
        t.call(GF[which], il, ret="u32" if which != "decompress" else "void", maxsteps=60000000)
        t.obs("loader", Kernel.words(t, il, 0x24), "ptrs")
    return Case("ImageLoader::%s(%s image, %d sections, %d bytes%s)" % (which, kind, len(secs), len(cont),
                                                                       ", pre-decompressed" if pre else ""), run)


# ---- the boot sources and host: helpers (io unit -> fileio / libmc) -----------------

def io_world(t, mc=None, fio=None, defer=False):
    servers = {}
    if fio is not None:
        servers[0x80000001] = fio
    if mc is not None:
        servers[0x80000400] = mc
    iop = Iop(servers=servers, defer=defer)
    kern = sif_up(t, iop)
    kern.open_mode_unknown = True                 # the sources' sceOpen(path, flags) passes no mode
    t.unchecked(t.data("_fio_cmd_buf") + 0x10, 4)  # ... which sceOpen stores there (FIO_F_OPEN.mode)
    printf_stub(t)
    t.cache_tags(lambda tt, op, a: 0)
    for nm, v in (("_fio_initialized", 0), ("_fio_lock_sema", 0xFFFFFFFF), ("_fio_fd_sema", 0xFFFFFFFF),
                  ("_fio_pending_sema", 0xFFFFFFFF)):
        t.w32(t.data(nm), v)
    if mc is not None:
        # libmc as the boot left it (mc_init_once -> sceMcInit): the io unit's
        # wrappers retry an unbound libmc forever
        t.w32(t.data("s_mcSema"), 0xFFFFFFFF)
        t.w32(t.data("s_mcFuncNo"), 0)
        saved = mc.vers
        mc.vers = (0x20A, 0x20E)
        t.call(0x010157B8, ret="int", label="sceMcInit (setup)", maxsteps=3000000)
        mc.vers = saved
        mc.k = 0
    return iop


class ReadLog:
    """a FileioServer that also notes the opens (FIO_F_OPEN) and the bytes each
    sceRead (FIO_F_READ) delivered (None: the request was refused).
    HostSource::getLoadFlags returns its 2-byte stack buffer whatever
    host_read_file did, i.e. stack garbage after a failed open or a short read:
    its result is compared only when no open was made (no such file: 0) or a
    read filled the buffer."""

    def __init__(self, srv):
        self.srv = srv
        self.reads, self.opens = [], 0

    def reset(self):
        self.reads, self.opens = [], 0
        if hasattr(self.srv, "reset"):
            self.srv.reset()

    def __call__(self, t, fno, send, rsize, iop):
        k = getattr(iop, "fio_calls", -1) + 1
        r = self.srv(t, fno, send, rsize, iop)
        if fno == 0:
            self.opens += 1
        if fno == 2 and getattr(iop, "fio_block", None):
            if self.srv.refuse[k % 16]:
                self.reads.append(None)
            else:
                res, nbyte = self.srv.results[k % 16], E32s((list(send) + [0] * 6)[5])
                self.reads.append(max(0, min(res, nbyte, 0x1000)) if res >= 0 else 0)
        return r


def gen_sources(rng, i, n):
    which = ["Host_open", "Host_flags", "Disk_open", "Disk_flags", "MC_open", "MC_flags", "mcard_is_usable",
             "host_read_file", "host_write_file", "host_file_size", "Host_close", "Disk_close", "MC_close"][i % 13]
    fio = ReadLog(FileioServer(rng))
    mc = McServer(rng)
    name = rng.choice([b"CORE.GT3", b"core.gt3", b"", rstr(rng, rng.randrange(1, 30))])
    off, ln = rng.choice([0, 2, 0x100]), rng.choice([0, 2, 0x40, 0x1000])
    data = junk(rng, 0x200)
    port, slot = rng.randrange(2), rng.randrange(2)
    has_fname = rng.random() < 0.5

    def run(t):
        io_world(t, mc, fio)
        nm = t.cstr(name)
        if which in ("Host_open", "Disk_open", "MC_open", "Host_close", "Disk_close", "MC_close"):
            obj = t.halloc_obj(0x18, LAYOUT["MCardSource"])
            kind = which.split("_")[0]
            if kind == "MC":
                t.call(GF["MC_ctor"], obj, port, slot, ret="ptr", label="MCardSource::MCardSource (setup)")
            else:
                t.call(GF[kind + "_ctor"], obj, ret="ptr", label="%sSource ctor (setup)" % kind)
            if has_fname:
                fn = t.halloc_obj(0xC, LAYOUT["FileName"])
                vt = t.vtable([t.callback(lambda tt, a: 0, 2, "FileName dtor")])
                t.objwrite(fn, struct.pack("<III", t.halloc(8, b"OLD\0"), 0, vt))
                t.w32(t.m(obj, 8), fn)
            if which.endswith("open"):
                t.call(GF[which], obj, nm, ret="u8", maxsteps=20000000)
            else:
                t.call(GF[which], obj, ret="void")
            t.obs("source", t.objwords(obj, 0x18), "ptrs")
            fnp = t.r32(t.m(obj, 8))
            if fnp and fnp != (fn if has_fname else 0) and HEAP_LO <= fnp < HEAP_HI:
                t.obj(fnp, LAYOUT["FileName"])           # the FileName open() made (operator new(0xC))
        elif which == "Host_flags":
            fio.reads, fio.opens = [], 0
            v = t.call(GF[which], nm, ret="void", maxsteps=20000000)
            full = not fio.opens or (fio.reads and (fio.reads[-1] or 0) >= 2)
            t.obs("getLoadFlags", (v & 0xFFFF) if full else "stack garbage (short read)")
        elif which == "Disk_flags":
            t.call(GF[which], nm, ret="u16", maxsteps=20000000)
        elif which == "MC_flags":
            t.call(GF[which], nm, port, slot, ret="int", maxsteps=20000000)
        elif which == "mcard_is_usable":
            t.call(GF[which], port, slot, ret="u8", maxsteps=20000000)
        elif which == "host_read_file":
            b = t.alloc(0x1100, 16)
            t.call(GF[which], nm, b, off, ln, ret="ptr", maxsteps=20000000)
            t.obs("read", t.read(b, 0x1100))
        elif which == "host_write_file":
            t.call(GF[which], nm, t.put(data, 16), ln & 0x1FF, ret="u8", maxsteps=20000000)
        else:
            t.call(GF[which], nm, ret="u32", maxsteps=20000000)
    return Case("%s(%r, port %d slot %d, off %d len %d, old fileName %d)" % (which, name[:20], port, slot, off, ln,
                                                                            has_fname), run)


# ---- main's boot helpers ---------------------------------------------------------------

def gen_main_iop(rng, i, n):
    which = ["read_pad_buttons", "shutdown_iop", "load_irx_module", "boot_iop"][i % 4]
    pad = PadServer(rng)
    lf = LoadfileServer(rng)
    lf.replies = [b"\x01\0\0\0" + r[4:] for r in lf.replies]        # module ids >= 0: no endless retry
    lf.version = b"2240"
    cd = CdServers(rng)
    mc = McServer(rng)
    mc.vers = (0x20A, 0x20E)                 # mc_init_once retries sceMcInit until it returns 0
    from .sonyio import rand_half
    half = rand_half(rng)
    name = rng.choice([b"sio2man", b"padman", b"mcman", b"MCSERV", rstr(rng, rng.randrange(1, 12)).lower()])

    def run(t):
        servers = dict(cd.servers())
        servers.update({0x80000100: pad, 0x80000101: pad, 0x80000006: lf, 0x80000400: mc})
        iop = Iop(servers=servers)

        def vblank(tt, k_):                                 # gfx_vsync(1) in the pad poll: a vblank per sleep
            gg = tt.data("g_gfx")
            tt.w64(gg, (tt.r64(gg) + 1) & M64)
        k = Kernel(sif_regs={4: 0x70000}, sleep=vblank)
        sif_up(t, iop, k)
        k.ret[0x7C] = lambda tt, a, st: 0                   # DECI2 (TTY re-init after the IOP reboot)
        printf_stub(t)
        t.cache_tags(lambda tt, op, a: 0)
        if which == "read_pad_buttons":
            area = t.alloc(0x100, 64)
            t.call(0x01014540, 0, ret="int", label="scePadInit (setup)", maxsteps=3000000)
            t.call(0x010147A0, 0, 0, area, ret="int", label="scePadPortOpen (setup)")
            t.write(area, half + half)
            t.call(GF[which], ret="int")
        elif which == "shutdown_iop":
            t.call(GF[which], ret="void", maxsteps=3000000)
        elif which == "load_irx_module":
            t.call(GF[which], t.cstr(name), ret="void", maxsteps=3000000)
        else:
            t.call(GF[which], ret="void", maxsteps=20000000)
    return Case("%s(%r)" % (which, name), run)


SIF_LOAD_MODULE = 0x01022558


def gen_irx_path(rng, i, n):
    """load_irx_module (0x01000790): "cdrom0:\\" + name with only 'a'..'z'
    upper-cased + ";1".  Names made of the bytes around that range ('@' '['
    '`' '{'), bytes >= 0x80 (negative as a signed char), or empty.
    sceSifLoadModule is stubbed: the path text and the other two arguments are
    recorded, and it fails (any negative int: retried) 0..3 times before a
    result >= 0 ends the loop."""
    edge = b"`az{AZ@[09._~\x7f\x80\xc1\xe1\xfa\xfe\xff" + bytes(range(0x61, 0x7B))
    L = rng.choice([0, 1, 2, 12, rng.randrange(1, 64)])
    name = bytes(rng.choice(edge) for _ in range(L))
    rets = [rng.choice([-1, -2, -201, -0x80000000]) for _ in range(rng.choice([0, 0, 1, 3]))]
    rets.append(rng.choice([0, 1, 0x7FFFFFFF, rng.getrandbits(31)]))
    want = b"cdrom0:\\" + bytes(c - 0x20 if 0x61 <= c <= 0x7A else c for c in name) + b";1"

    def run(t):
        calls = []

        def load(tt, a):
            path = bytes(tt.cstr_at(a[0]))
            tt.obs("sceSifLoadModule path", path)
            tt.obs("sceSifLoadModule args 1, 2", (a[1], a[2]))
            tt.check(path == want, "path: 'a'..'z' upper-cased only")
            r = rets[min(len(calls), len(rets) - 1)]
            calls.append(r)
            return r
        t.stub(SIF_LOAD_MODULE, load, 3, log=0)          # the path is a stack address: compared as text
        t.call(GF["load_irx_module"], t.cstr(name), ret="void", maxsteps=200000)
        t.obs("sceSifLoadModule calls", len(calls))
    return Case("load_irx_module(%r, %d failures first)" % (name, len(rets) - 1), run)


def gen_crt(rng, i, n):
    which = ["reg_frame", "dereg_frame", "exit_", "root"][i % 4]
    status = rng.getrandbits(32)

    def run(t):
        Kernel(ret={0x04: 0, 0x23: 0}).install(t)
        if which == "reg_frame":
            t.call(GF[which], ret="void")
            t.call(GF["dereg_frame"], ret="void")
        elif which == "dereg_frame":
            t.call(GF[which], ret="void")
        elif which == "exit_":
            t.call(GF[which], status, ret="void")
        else:
            t.call(GF[which], status, ret="void", maxsteps=50)
    return Case("%s(0x%X)" % (which, status), run)


def gen_tex1_upload(rng, i, n):
    nx = rng.choice([0, 1, 2, 3])
    xf = [(rng.getrandbits(16), rng.getrandbits(8), rng.choice([0, 1, 2, 0x13, 0x14]),
           rng.choice([8, 16, 64, 640]), rng.choice([1, 8, 64])) for _ in range(nx)]
    w = _gs.World(rng)

    def run(t):
        buf, g = w.put(t)
        for c in range(3):
            t.w8(g + 0x1C + 12 * c + 1, 0)
        recs = b""
        for dbp, dbw, psm, wd, ht in xf:
            px = t.alloc(wd * ht * 4 + 16, 16)
            t.write(px, bytes((k * 7) & 0xFF for k in range(min(wd * ht * 4, 0x4000))))
            recs += struct.pack("<IHBBHH", px, dbp, dbw, psm, wd, ht)
        xfer = t.put(recs or b"\0" * 4, 16)
        img = t.put(struct.pack(IMG_FMT, 0x31786554, 0, 0, 0x38020, 0, 0, 0, nx, 0, xfer, 0, 0), 16)
        t.call(GF["tex1_upload"], img, ret="void", maxsteps=3000000)
    return Case("tex1_upload_to_vram(%d transfers %s)" % (nx, [(x[2], x[3], x[4]) for x in xf]), run)


SPECS += [
    Spec("gf_buffer", [GF[x] for x in ("Buffer_dtor", "Buffer_ctor_n", "Buffer_copy", "Buffer_assign",
                                        "Buffer_adopt", "Buffer_resize")], gen_buffer, 140),
    Spec("gf_source_misc", [GF[x] for x in ("GetPath", "GetName", "GetDataEnd", "GetDataEnd_c", "GetDataEndAddr",
                                             "GetData", "GetData_c", "GetDataAddr", "GetSize", "Source_ctor",
                                             "Host_ctor", "Disk_ctor", "MC_ctor", "dt_Host", "dt_Disk", "dt_MC",
                                             "dt_mc10", "dt_mc00", "Source_dtor", "tf_Source", "tf_Host", "tf_Disk",
                                             "tf_MC", "tf_mc10", "tf_mc00", "SourceRecord_ctor",
                                             "SourceRecord_release", "SourceRecord_dtor")], gen_source_misc, 120),
    Spec("gf_imageloader", [GF["decompress"], GF["verify"], GF["load"]], gen_imageloader, 6,
         "generated signed images (n = digest + 1)"),
    Spec("gf_sources", [GF[x] for x in ("Host_open", "Host_flags", "Disk_open", "Disk_flags", "MC_open", "MC_flags",
                                         "mcard_is_usable", "host_read_file", "host_write_file", "host_file_size",
                                         "Host_close", "Disk_close", "MC_close")], gen_sources, 130,
         "fileio / libmc through the model IOP"),
    Spec("gf_main_iop", [GF[x] for x in ("read_pad_buttons", "shutdown_iop", "load_irx_module", "boot_iop")],
         gen_main_iop, 24, "model IOP: pad, loadfile, cdvd, IOP reboot"),
    Spec("gf_irx_path", [GF["load_irx_module"]], gen_irx_path, 60,
         "upper-casing edges, sceSifLoadModule stubbed (retries)"),
    Spec("gf_crt", [GF[x] for x in ("reg_frame", "dereg_frame", "exit_", "root")], gen_crt, 8),
    Spec("gf_tex1_upload", [GF["tex1_upload"]], gen_tex1_upload, 30, "GIF DMA of the transfer header + pixels"),
]


# ================================================================== the startup screen (src/startup_screen.cpp)
# Its GS callees are stubbed (each call compared with its arguments; the float
# arguments of gs_set_bg_color_rgb as bit patterns) or, in some draw cases, run
# for real on a gs World; the kernel is common.Kernel.

SS = dict(start=0x01000A50, draw_frame=0x01000B40, thread=0x01000C88, request_stop=0x01000E70,
          wait_thread=0x01000E80, stop=0x01000EA8)
SS_GS_STUBS = [  # retail address, name, integer arguments
    (0x0100D3A0, "gfx_set_packet_buffer", 2), (0x0100D438, "gfx_begin_packet", 0),
    (0x0100DAD0, "gfx_set_draw_buffer", 4), (0x0100DD00, "gs_set_clear_color", 1),
    (0x0100DE80, "gs_clear_buffers", 1), (0x0100D0F8, "gfx_disable", 1), (0x0100BE10, "gfx_set_texture_regs", 1),
    (0x0100E478, "gs_set_bg_color_rgb", 0), (0x0100E2A0, "gs_draw_textured_sprite", 8),
    (0x0100D530, "gfx_end_packet", 0), (0x0100CF28, "gfx_kick_frame_lists", 1),
    (0x0100D7B0, "gfx_sync_pipeline", 0), (0x0100D9A8, "gfx_vsync", 1), (0x0100C7E0, "gfx_change_display_mode", 3),
    (0x0100C9B8, "gfx_set_screen_size", 2), (0x0100C998, "gfx_set_display_area", 4),
    (0x0100E430, "gs_get_display_state", 0), (0x0100C9D0, "gfx_set_display_fb", 4)]
INFLATE_CTOR, INFLATE_DTOR = 0x01011768, 0x010117C8       # GZIP::InflateBase(out, in) / ~InflateBase (gzip unit)


def ss_stub_gs(t, display_state=0, vsync=None):
    """stub the GS driver calls of the startup screen; vsync(t, k) runs at the
    k-th gfx_vsync (1-based) - the main thread's doings meanwhile."""
    nv = [0]

    def mk(name):
        def f(tt, a):
            if name == "gs_set_bg_color_rgb":
                tt.obs("gs_set_bg_color_rgb floats", tuple(tt.float_args()[:3]))
            elif name == "gs_get_display_state":
                return display_state
            elif name == "gfx_vsync":
                nv[0] += 1
                if vsync is not None:
                    vsync(tt, nv[0])
                return nv[0]
            elif name == "gfx_begin_packet":
                return 0x01234560
            return 0
        return f
    for a, name, nargs in SS_GS_STUBS:
        t.stub(a, mk(name), nargs, name)


def put_startup_tex(t, regs):
    """a Tex1Image with len(regs) GsTexRegs (5 x u64 each), stored as g_startupTex."""
    tex = t.put(b"".join(struct.pack("<5Q", *r) for r in regs), 16)
    img = t.put(struct.pack(IMG_FMT, 0x31786554, 0, 0, 0x40000, 0x2800, 0, len(regs), 0, tex, 0, 0, 0), 16)
    t.w32(t.data("g_startupTex"), img)
    return img


def rand_tex_regs(rng, k=3):
    out = []
    for _ in range(k):
        r = [rng.getrandbits(64) for _ in range(5)]
        if rng.random() < 0.6:                                     # a realistic CLAMP: MAXU / MAXV < 640 / 448
            r[4] = (r[4] & ~((0x3FF << 14) | (0x3FF << 34))) | (rng.randrange(640) << 14) | (rng.randrange(448) << 34)
        out.append(r)
    return out


def gen_ss_draw(rng, i, n):
    """startup_screen_draw_frame(fb, image, alpha): GS calls stubbed, or (every
    third case) the real driver on a gs World, drawing into s_startupPacketBuf."""
    native = i % 3 == 2
    fb = rng.choice([0, 0x50000, rng.getrandbits(32)])
    image = rng.choice([-1, 0, 1, 2, -rng.randrange(1, 100)])
    alpha = rng.choice([0.0, 1.0, 0.5, 1.0 / 15, rng.uniform(0, 1), rng.uniform(-3, 3)])
    regs = rand_tex_regs(rng)
    w = _gs.World(rng) if native else None

    def run(t):
        if native:
            w.put(t)
        else:
            Kernel().install(t)
            ss_stub_gs(t)
        put_startup_tex(t, regs)
        t.call(SS["draw_frame"], fb, image & M64, ret="void", floats=(fbits(alpha),), maxsteps=3000000)
    return Case("startup_screen_draw_frame(0x%X, %d, %r)%s" % (fb, image, alpha, " on the real GS driver" if native
                                                               else ""), run)


def gen_ss_thread(rng, i, n):
    """startup_screen_thread: 151 frames of the image sequence, the exit frames,
    SignalSema + ExitDeleteThread.  The stop flag is set before the start or by
    the main thread at the k-th gfx_vsync; s_startupImageSeq = {first, -1}."""
    first = rng.choice([0, 0, 1, 2, -1])
    stop = rng.choice([None, None, 0, 1, 2, 3, 40, 152, 153, rng.randrange(1, 160)])
    state = rng.choice([4, 4, 0, 1, 3])
    regs = rand_tex_regs(rng)
    sema = rng.getrandbits(31)

    def run(t):
        Kernel().install(t)

        def vs(tt, k):
            if stop is not None and k == stop:
                tt.w32(tt.data("s_startupStopRequested"), 1)
        ss_stub_gs(t, state, vs)
        put_startup_tex(t, regs)
        if first != 0:
            t.unwritten_static("s_startupImageSeq")          # {0, -1} in the program
        t.write(t.data("s_startupImageSeq"), struct.pack("<ii", first, -1))
        t.w32(t.data("s_startupStopRequested"), 1 if stop == 0 else 0)
        t.w32(t.data("s_startupDoneSema"), sema)
        t.call(SS["thread"], 0, ret="void", maxsteps=3000000)
    return Case("startup_screen_thread(sequence {%d, -1}, stop %s, display state %d)" % (
        first, "never" if stop is None else "before start" if stop == 0 else "at vsync %d" % stop, state), run)


def gen_ss_sync(rng, i, n):
    which = ["request_stop", "wait_thread", "stop"][i % 3]
    count = rng.choice([1, 1, 2, 0])
    ours = rng.random() < 0.8                       # a semaphore of the fake kernel (else any id: accepted)
    sid = rng.getrandbits(31)
    flag = rng.choice([0, 1, rng.getrandbits(32)])

    def run(t):
        k = Kernel()
        st = k.install(t)
        if ours:
            sid_ = 0x77
            st["semas"][sid_] = [count, 1, 1, 0, 0]
        else:
            sid_ = sid
        t.w32(t.data("s_startupDoneSema"), sid_)
        t.w32(t.data("s_startupStopRequested"), flag)
        t.call(SS[which], ret="void")
        t.obs("sema", tuple(st["semas"][sid_]) if sid_ in st["semas"] else None)
    return Case("startup_screen_%s(sema %s, count %d, flag 0x%X)" % (which, "ours" if ours else "0x%X" % sid, count,
                                                                     flag), run)


_STARTUPA_BLOB = []


def startupa_blob():
    if not _STARTUPA_BLOB:
        _STARTUPA_BLOB.append(open(os.path.join(ROOT, "data", "startupa_img.gz"), "rb").read())
    return _STARTUPA_BLOB[0]


def gen_ss_start(rng, i, n):
    """startup_screen_start: memalign, inflate startupa.img into it (the real
    GZIP::InflateBase in even cases, a Python stub in odd ones), relocate, link to
    VRAM 0xA0000 (tex1_upload_to_vram stubbed: its own spec), the completion
    semaphore and the render thread."""
    native = i % 2 == 0
    tid = rng.choice([0x11, 0x1F, rng.randrange(1, 256)])

    def run(t):
        Kernel(tid=tid).install(t)
        t.w32(t.data("s_startupStopRequested"), 1)
        t.stub(GF["tex1_upload"], lambda tt, a: 0, 1, "tex1_upload_to_vram")
        if not native:
            def ctor(tt, a):
                this, out, inp = a[:3]
                blob = startupa_blob()
                tt.obs("gzip input is startupa.img", tt.read(inp, len(blob)) == blob)
                tt.write(out, gzip.decompress(blob))
                return this
            t.stub(INFLATE_CTOR, ctor, 3, "GZIP::InflateBase(out, in)")
            t.stub(INFLATE_DTOR, lambda tt, a: 0, 2, "GZIP::~InflateBase")
        t.call(SS["start"], ret="void", maxsteps=20000000)
        t.obs("g_startupTex", t.r32(t.data("g_startupTex")), "ptr")
        t.obs("s_startupDoneSema / stop flag", (t.r32(t.data("s_startupDoneSema")),
                                                t.r32(t.data("s_startupStopRequested"))))
    return Case("startup_screen_start(%s inflater, thread id 0x%X)" % ("real" if native else "stubbed", tid), run)


# ================================================================== main / select_boot_source / _start

MAIN, SELECT, START = 0x01000538, 0x010001C8, 0x01000008
SEL_STUBS = dict(cd_ready=0x01014150, mc_flags=GF["MC_flags"], disk_flags=GF["Disk_flags"], yield_=0x0100F640,
                 mc_open=GF["MC_open"], mc_close=GF["MC_close"], disk_open=GF["Disk_open"], disk_close=GF["Disk_close"])


def gen_select(rng, i, n):
    """select_boot_source: scripted probe results (MCardSource / DiskSource
    getLoadFlags, u16 values) and open() results; after the script every disc
    probe hits and every open succeeds.  The sources are made by the real
    constructors (operator new = the stub heap) and the losing records' sources
    are destroyed by the real destructors."""
    skip = rng.random() < 0.2
    npass = rng.choice([1, 1, 2, 3, 5])

    def flags():
        return rng.choice([0, 0, 0, 1, 2, 3, 0x10, 0x8000, 0xFFFF, rng.getrandbits(16)])
    mc0 = [flags() for _ in range(npass)]
    mc1 = [flags() for _ in range(npass)]
    disk = [flags() for _ in range(npass)]
    opens = [rng.choice([0, 1, 1]) for _ in range(npass)]
    tail_disk = rng.choice([1, 0x10, 0xFFFF])

    def run(t):
        Kernel().install(t)
        t.w32(t.data("g_skipMemoryCardBoot"), 1 if skip else 0)
        cnt = dict(mc0=0, mc1=0, disk=0, open=0)

        def mcf(tt, a):
            key = "mc%d" % (a[1] & 1)
            k = cnt[key]
            cnt[key] = k + 1
            lst = mc0 if key == "mc0" else mc1
            return lst[k] if k < len(lst) else 0

        def dkf(tt, a):
            k = cnt["disk"]
            cnt["disk"] = k + 1
            return disk[k] if k < len(disk) else tail_disk

        def opn(tt, a):
            k = cnt["open"]
            cnt["open"] = k + 1
            tt.obs("open: name", tt.cstr_at(a[1]))
            return opens[k] if k < len(opens) else 1
        t.stub(SEL_STUBS["cd_ready"], lambda tt, a: 2, 1, "sceCdDiskReady")
        t.stub(SEL_STUBS["mc_flags"], mcf, 3, "MCardSource::getLoadFlags")
        t.stub(SEL_STUBS["disk_flags"], dkf, 1, "DiskSource::getLoadFlags")
        t.stub(SEL_STUBS["yield_"], lambda tt, a: 0, 0, "yield_low_priority")
        for nm in ("mc_open", "disk_open"):
            t.stub(SEL_STUBS[nm], opn, 2, nm)
        for nm in ("mc_close", "disk_close"):
            t.stub(SEL_STUBS[nm], lambda tt, a: 0, 1, nm)
        src = t.call(SELECT, ret="ptr", maxsteps=2000000)
        if src:
            t.obj(src & M32, LAYOUT["Source"])         # made by the code: DiskSource or MCardSource
            t.obs("source", t.objwords(src & M32, 0x10), "ptrs")
    return Case("select_boot_source(skip MC %d, MC0 %s, MC1 %s, disk %s, opens %s)" % (
        skip, mc0, mc1, disk, opens), run)


MAIN_STUBS = [  # retail address, name, integer arguments
    (0x010170E0, "__main", 0), (0x0100C478, "gfx_init_default", 1), (0x01000A50, "startup_screen_start", 0),
    (0x01000880, "boot_iop_reboot_and_pad_init", 0), (0x01000E80, "startup_screen_wait_thread", 0),
    (0x01000EA8, "startup_screen_stop", 0), (0x010009D8, "shutdown_iop_services", 0),
    (0x0100C750, "gfx_shutdown", 0), (0x0100BF98, "gfx_wait_vsync_poll", 0)]


def gen_main(rng, i, n):
    """main with every callee stubbed except strcpy and Buffer::~Buffer: the boot
    source's path copied to the stack, the ImageLoader built / loaded / destroyed
    (virtual delete of the source, ~Buffer of its image), the shutdown calls and
    ExecPS2's argv; entry 0 (image rejected) spins forever (a runaway on purpose)."""
    entry = rng.choice([0, 0x00100008, rng.getrandbits(32) | 1])
    path = rng.choice([b"cdrom0:\\CORE.GT3;1", b"mc0:/BASCUS-97102/core.gt3", b"", rstr(rng, rng.randrange(1, 0x80))])
    image = rng.random() < 0.5
    isize = rng.choice([0, 0x10, 0x1234])

    def run(t):
        Kernel().install(t)
        for a, name, nargs in MAIN_STUBS:
            t.stub(a, lambda tt, x: 0, nargs, name)
        p = t.cstr(path)
        gp = t.callback(lambda tt, a: p, 1, "FileName::GetPath")
        fvt = t.vtable([t.callback(lambda tt, a: 0, 2, "FileName dtor"), gp])
        fname = t.objput(struct.pack("<III", t.cstr(b"core.gt3"), 0, fvt), LAYOUT["FileName"])
        svt = t.vtable([t.callback(lambda tt, a: 0, 2, "Source dtor")])
        src = t.halloc_obj(0x10, LAYOUT["Source"], struct.pack("<IIII", 0, 0, fname, svt))
        t.stub(SELECT, lambda tt, a: src, 0, "select_boot_source")

        def load(tt, a):
            tt.obs("ImageLoader", Kernel.words(tt, a[0], 0xC), "ptrs")
            if image:
                tt.write(a[0] + 4, struct.pack("<II", tt.halloc(max(isize, 1)), isize))
            return entry

        def execps2(tt, a):
            tt.obs("argv strings", (tt.cstr_at(tt.r32(a[2])), tt.cstr_at(tt.r32(a[2] + 4)), tt.r32(a[2] + 8)))
            return 0
        t.stub(GF["load"], load, 1, "ImageLoader::load")
        t.stub(0x01016F70, execps2, 3, "exec_ps2_with_sbus_cleanup")
        t.call(MAIN, 1, t.alloc(16), ret="int", maxsteps=20000)
        t.obs("s_execArgv", Kernel.words(t, t.data("s_execArgv"), 12), "ptrs")
    return Case("main(entry 0x%X, path %r, image buffer %s)" % (entry, path[:24], isize if image else "none"), run)


def gen_start(rng, i, n):
    """_start (crt0.S): .bss cleared (prefilled here; the loop also clears the
    quadword holding _end and the next one), InitMainThread / InitHeap with the
    linker symbols, _InitSys, FlushCache(0), ei, main(argc, argv) from the _args
    block the kernel filled in, Exit(main's result).  _InitSys and main are
    stubbed; Exit ends the test (it never returns on the EE)."""
    argc = rng.choice([0, 1, 2, 3])
    args = [rstr(rng, rng.randrange(1, 20)) for _ in range(argc)]
    mret = rng.choice([0, 1, M32, rng.getrandbits(32)])

    def run(t):
        k = Kernel()
        fbss, end, a_args = t.sym("_fbss"), t.sym("_end"), t.sym("_args")

        def init_main_thread(tt, a, st):
            # the kernel: argc / argv / strings into _args; returns the new $sp
            # (the harness's own, so that the callee-saved check holds)
            tt.obs("InitMainThread(gp?, stack, size, args?, root?)",
                   (a[0] == tt.sym("_gp"), a[1], a[2], a[3] == a_args, a[4] == tt.fn(GF["root"])))
            pos = a_args + 4 + 64
            ptrs = []
            for s in args:
                tt.write(pos, s + b"\0")
                ptrs.append(pos)
                pos += len(s) + 1
            tt.write(a_args, struct.pack("<I%dI" % len(ptrs), argc, *ptrs))
            return E.CPU.SP0
        k.ret[0x3C] = init_main_thread
        k.ret[0x3D] = lambda tt, a, st: tt.obs("InitHeap(end?, size)", (a[0] == end, a[1])) or 0
        st = k.install(t)
        # InitMainThread / InitHeap take linker addresses: compared above (is it
        # this side's symbol?) instead of as raw registers
        t.syscalls(lambda tt, num, a: k.call(tt, num, a, st),
                   nargs=lambda tt, num, pc: 0 if num in (0x3C, 0x3D) else syscall_argc(tt, num, pc))
        t.write(fbss, bytes(((0xA5 ^ (j * 7)) & 0xFF) or 1 for j in range(end - fbss)))
        t.write(end, b"\xEE" * 0x40)
        t.stub(0x01022C18, lambda tt, a: 0, 0, "_InitSys")

        def main_(tt, a):
            tt.obs("main(argc, argv)", (a[0], a[1] - a_args) +
                   tuple(tt.cstr_at(tt.r32(a[1] + 4 * j)) for j in range(argc)))
            return mret
        t.stub(MAIN, main_, 2, "main", log=0)

        def exit_(tt, a):
            tt.side.cpu.g[31] = E.CPU.RET                         # Exit does not return: end the test
            return 0
        t.stub(0x0101B800, exit_, 1, "Exit")
        t.call(START, ret="void", maxsteps=200000)
        left = [j for j in range(end - fbss) if t.r8(fbss + j) and not 0 <= fbss + j - a_args < 0x144]
        past = 0
        while past < 0x40 and t.r8(end + past) == 0:
            past += 1
        # the loop stores whole quadwords up to the one after _end's: how far it
        # goes past _end depends on _end's alignment, so it is measured from
        # _end rounded up to 16
        t.obs(".bss bytes not cleared / cleared up to (_end + 15) & ~15 +", (len(left), left[:4],
                                                                            end + past - ((end + 15) & ~15)))
        t.write(end, bytes(0x40))                                 # back to the pristine zeros
    return Case("_start(argc %d, main returns 0x%X)" % (argc, mret), run)


def gen_lock_stub(rng, i, n):
    obj, unlock = rng.getrandbits(32), rng.choice([0, 1, rng.getrandbits(32)])

    def run(t):
        t.call(0x01004B00, obj, unlock, ret="void")
    return Case("bignum_lock_stub(0x%X, %d)" % (obj, unlock), run)


SPECS += [
    Spec("gf_startup_draw", [SS["draw_frame"]], gen_ss_draw, 60, "GS calls stubbed / the real driver"),
    Spec("gf_startup_thread", [SS["thread"]], gen_ss_thread, 16, "151 frames, stop flag, exit frames"),
    Spec("gf_startup_sync", [SS["request_stop"], SS["wait_thread"], SS["stop"]], gen_ss_sync, 30),
    Spec("gf_startup_start", [SS["start"]], gen_ss_start, 4, "inflate (real / stub), semaphore, thread"),
    Spec("gf_select_boot", [SELECT], gen_select, 80, "scripted probes and opens"),
    Spec("gf_main", [MAIN], gen_main, 30, "callees stubbed; entry 0 spins (runaway on purpose)"),
    Spec("gf_start", [START], gen_start, 8, "crt0: .bss clear, InitMainThread / InitHeap, Exit"),
    Spec("gf_lock_stub", [0x01004B00], gen_lock_stub, 6, "empty refcount hook: no effect"),
]
