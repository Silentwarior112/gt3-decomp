"""gs group: unit gs (src/gs_driver.cpp): the bootstrap's GS / VIF1 / GIF / DMA driver.

Every function of the unit is called on a random but consistent driver state:
  * the GsContext (0xBC0 bytes) in scratchpad at 0x70003000, with its packet
    pointers (cur / end / tail0 / tail1 / frameHdr / subHdr, optionally carrying
    the uncached / UCAB segment bits in addrBits) inside a 32 KB packet buffer;
  * g_gfx (GfxDriverState, .bss) with waiter lists, a readback record and an
    optional vsync callback; the statics s_gfxInitialized, s_vu1CodeUnscrambled,
    s_lcgState and s_gsTexOverride.
What is compared (besides the scratchpad, the buffer and every global the code
writes): every hardware access in order - GS privileged registers, DMAC / VIF /
GIF registers, the packet a DMA start sends (eeemu.IoModel snapshots the normal
transfer or the whole source chain, tags and payload), COP0 (di / ei, the
performance counters), sync.l / sync.p, VU0 (ctc2 / cfc2 FBRST and VPU-STAT, vsqi
to VU0 memory) - and every kernel call (common.Kernel: semaphores, threads,
INTC / DMAC handlers, FlushCache, SleepThread with what the interrupt handlers do
meanwhile).  sceGsResetGraph (libgraph, libc unit) is stubbed.

Not reachable: gfx_sync_pipeline's over-budget recovery (0x1000000 polls)."""
from .common import *

UNITS = ("gs",)

F = dict(
    set_texture_regs=0x0100BE10, wait_vsync_poll=0x0100BF98, reset_paths=0x0100BFE8,
    init_vu0_constants=0x0100C050, init_lights=0x0100C158, init_material=0x0100C230,
    reset_texture_state=0x0100C300, context_init=0x0100C378, init_default=0x0100C478, init=0x0100C490,
    context_init_spr=0x0100C6A0, init_vu0_constants_spr=0x0100C6C8, init_lights_material_spr=0x0100C6F0,
    reset_texture_state_spr=0x0100C728, shutdown=0x0100C750, change_display_mode=0x0100C7E0,
    store_display_mode=0x0100C848, reset_graph_stored_mode=0x0100C890, update_display_regs=0x0100C8B0,
    set_display_area=0x0100C998, set_screen_size=0x0100C9B8, set_display_fb=0x0100C9D0,
    dma_channel_init=0x0100CA78, dma_init=0x0100CB08, dma_shutdown=0x0100CC00, dma_lock=0x0100CC78,
    dma_unlock=0x0100CC98, dma_sync=0x0100CCB8, dma_send=0x0100CE68, kick_frame_lists=0x0100CF28,
    vu_upload_microcode=0x0100CFE8, disable=0x0100D0F8, mskpath3=0x0100D330, set_packet_buffer=0x0100D3A0,
    get_packet_ptr=0x0100D410, get_packet_space=0x0100D420, begin_packet=0x0100D438, end_packet=0x0100D530,
    close_subpacket=0x0100D5E0, select_list=0x0100D6E0, sync_pipeline=0x0100D7B0,
    wait_vblank_event=0x0100D8D0, vsync=0x0100D9A8, set_draw_buffer=0x0100DAD0, set_draw_area=0x0100DB88,
    update_scissor=0x0100DBB8, set_clear_color=0x0100DD00, build_clear_packet=0x0100DD10,
    clear_buffers=0x0100DE80, emit_render_target_regs=0x0100DF40, image_transfer_packet=0x0100E0A0,
    param_encode=0x0100E198, pow_encoded=0x0100E228, draw_textured_sprite=0x0100E2A0,
    vu0_load_identity_regs=0x0100E404, get_display_state=0x0100E430, set_bg_color_rgb=0x0100E478,
    lcg_rand=0x0100E4A8, xor_unscramble=0x0100E4E8, flush_all_regs=0x0100E548, flush_dirty_regs=0x0100E570,
    intc_vblank=0x0100E630, intc_vif1=0x0100E770, intc_gs_finish=0x0100E888, dmac_handler=0x0100E8E8,
    readback_begin=0x0100EB00, readback_end=0x0100EB78, start_source_chain=0x0100EC20,
    compute_display_period=0x0100ECD0, set_bg_color=0x0100ED58, pack_rgba=0x0100EDB0,
    emit_masked_regs=0x0100EE48, collect_dirty_regs=0x0100EE80, store_material_vec=0x0100EF50,
    set_material_param=0x0100EFF0)
SCE_GS_RESET_GRAPH = 0x010269F8           # libgraph (libc unit): stubbed

CTX = 0x70003000                          # GsContext in scratchpad
BUF = 0x8000                              # packet buffer per case
D_CHCR = {0: 0x10008000, 1: 0x10009000, 2: 0x1000A000}
VIF1_STAT, VIF1_CODE, GIF_STAT, GS_CSR = 0x10003C00, 0x10003C80, 0x10003020, 0x12001000
GFX_SIZE = 0xC0


def fbits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def rfloat(rng, lo=-4.0, hi=4.0):
    k = rng.random()
    if k < 0.15:
        return rng.choice([0.0, 1.0, -1.0, 0.5, 255.0, 128.0])
    return rng.uniform(lo, hi)


# ================================================================== the driver state

def rand_target(rng):
    """a GsRenderTarget: realistic sizes (offsets as gfx_set_draw_buffer makes them)."""
    w = rng.choice([640, 512, 320, 256, 64, rng.randrange(1, 1025)])
    h = rng.choice([448, 224, 256, 512, rng.randrange(1, 1025)])
    if rng.random() < 0.05:
        w = 0                                   # division by zero in render_target_regs: on purpose
    xoff = ((0x800 - (w >> 1)) << 4) & 0xFFFF
    yoff = ((0x800 - (h >> 1)) << 4) & 0xFFFF
    return dict(base=rng.getrandbits(16), fbw=rng.getrandbits(8), psm=rng.choice([0, 1, 2, 0x30, rng.getrandbits(8)]),
                w=w, h=h, xoff=xoff, yoff=yoff, xend=(xoff + w * 16) & 0xFFFF, yend=(yoff + h * 16) & 0xFFFF)


def pack_target(d):
    return struct.pack("<HBBiiHHHH", d["base"], d["fbw"], d["psm"], d["w"], d["h"], d["xoff"], d["yoff"],
                       d["xend"], d["yend"])


def rand_mask(rng):
    k = rng.random()
    if k < 0.2:
        return 0x000003FF1FFF1FFF
    if k < 0.35:
        return 0x000003FF00001FFF
    if k < 0.45:
        return 0
    if k < 0.75:
        return rng.getrandbits(64) & 0x000003FF1FFF1FFF
    return rng.getrandbits(64)


class Ctx:
    """random decisions for a GsContext and its packet buffer (made in gen)."""

    def __init__(self, rng, packet=None):
        self.junk = junk(rng, 0xBC0, 0.3)
        r = rng
        self.addr_bits = r.choice([0, 0, 0x20000000, 0x30000000])
        self.cur_off = 0x400 + 16 * r.randrange(0, 0x40)
        self.end_off = r.choice([self.cur_off + 0x2000, BUF, self.cur_off])
        self.tail0 = r.choice([None, 0x100, 0x110, 0x200])
        self.tail1 = r.choice([None, 0x120, 0x130, 0x210])
        self.tail_bits = r.random() < 0.3
        self.frame_hdr = r.choice([None, 0x0, 0x0, 0x40])
        self.hdr_ptrs = [r.choice([None, 0x300, 0x310, 0x380]) for _ in range(4)]
        self.sub_hdr = r.choice([None, 0x80])
        self.packet_open = packet if packet is not None else r.choice([0, 1, 1])
        self.sub_open = 1 if r.random() < 0.15 else 0
        self.cur_list = r.choice([0, 1])
        self.path3 = r.choice([0, 0, 1])
        self.u8s = {o: r.choice([0, 1, r.getrandbits(8)]) for o in
                    (0x0, 0x1, 0x2, 0x6, 0x7, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
                     0x1BC, 0x1BD, 0x1D0, 0x1D1)}
        self.u8s[0x22] = r.choice([0, 0, 0, 1])                   # bgImmediate
        self.u8s[0x23] = r.choice([0, 1])                         # halfHeight
        self.prim = r.getrandbits(16)
        self.u32s = {0x008: r.getrandbits(32), 0x018: r.getrandbits(32),
                     0x01C: r.choice([0, 1, -1, 16, -16, r.randrange(-200, 200), r.getrandbits(32)]) & M32,
                     0x120: r.choice([0, 0, 1]), 0x14C: r.getrandbits(32), 0x150: r.getrandbits(32),
                     0x16C: r.getrandbits(32), 0x1C4: r.choice([0, 1, 2, 3, 4]), 0x1C8: r.choice([0, 1]),
                     0xB60: r.getrandbits(32), 0xB64: r.getrandbits(32),
                     0xB68: r.choice([0, 1, 2, 3, 4, 5, 11, r.getrandbits(32)]), 0xB6C: r.randrange(4)}
        self.floats = {0x154: rfloat(r, 0, 2), 0x160: r.choice([0.0, 1.0, 65535.0, 16777215.0, rfloat(r, 0, 1e6)])}
        self.regs = [r.getrandbits(64) for _ in range(23)]
        self.masks = [rand_mask(r) for _ in range(5)]
        self.tgtB = rand_target(r)
        self.tgtA = rand_target(r)
        self.win = [r.randrange(0, 64), r.randrange(0, 64), r.randrange(1, 1024), r.randrange(1, 1024)]

    def put(self, t, buf):
        """write the context; returns the buffer layout."""
        bits = self.addr_bits

        def seg(off, with_bits=True):
            return ((buf + off) & 0x0FFFFFFF) | (bits if with_bits else 0)
        c = bytearray(self.junk)
        for o, v in self.u8s.items():
            c[o] = v
        c[3] = self.cur_list
        struct.pack_into("<H", c, 4, self.prim)
        for o, v in self.u32s.items():
            struct.pack_into("<I", c, o, v)
        for o, v in self.floats.items():
            struct.pack_into("<f", c, o, v)
        struct.pack_into("<23Q", c, 0x30, *self.regs)
        struct.pack_into("<5Q", c, 0xE8, *self.masks)
        struct.pack_into("<IIIIII", c, 0x110, buf, BUF, seg(self.cur_off), seg(self.end_off), self.u32s[0x120], bits)
        tb = self.tail_bits
        struct.pack_into("<IIII", c, 0x128, seg(self.tail0, tb) if self.tail0 is not None else 0,
                         seg(self.tail1, tb) if self.tail1 is not None else 0,
                         buf + self.frame_hdr if self.frame_hdr is not None else 0,
                         buf + self.sub_hdr if self.sub_hdr is not None else 0)
        c[0x170:0x184] = pack_target(self.tgtB)
        c[0x184:0x198] = pack_target(self.tgtA)
        struct.pack_into("<iiii", c, 0x1AC, *self.win)
        c[0x1BE] = self.packet_open
        c[0x1BF] = self.sub_open
        c[0x1C0] = self.path3
        t.write(CTX, bytes(c))
        if self.frame_hdr is not None:
            t.write(buf + self.frame_hdr, struct.pack("<4I", *[buf + p if p is not None else 0 for p in self.hdr_ptrs]))


class Gfx:
    """random decisions for g_gfx (GfxDriverState) and the gs statics."""

    def __init__(self, rng):
        r = rng
        self.junk = junk(r, GFX_SIZE, 0.3)
        self.vblank = r.choice([0, 5, 0xFFFFFFFF, 0x100000000, r.getrandbits(64), r.getrandbits(20)])
        self.last = (self.vblank - r.choice([0, 1, 2, 3, 7, r.getrandbits(8)])) & M64
        self.cb = r.random() < 0.3
        self.s8s = [r.choice([0, 1, 2, -1, -2, 0x7F, -0x80, r.randrange(-128, 128)]) for _ in range(4)]
        self.chans = [(r.choice([0, 1]), r.choice([0, 1, 1, 2]), r.choice([0, 0, 1, 2, 3]), r.getrandbits(8))
                      for _ in range(3)]
        self.vwaiters = r.choice([0, 0, 1, 2])
        self.rb = r.choice([None, "rec", "rec_cb"])
        self.rb_rec = (r.getrandbits(16), r.getrandbits(32), r.getrandbits(32))
        self.disp = [r.getrandbits(16) for _ in range(2)] + [r.randrange(-32, 32), r.randrange(-32, 32)] + [
            r.choice([640, 512, 320, r.randrange(1, 2048)]), r.choice([448, 224, 512, r.randrange(1, 2048)]),
            r.choice([640, 512, 320, r.randrange(1, 4096)]), r.choice([448, 224, 512, r.randrange(1, 4096)])]
        if r.random() < 0.03:
            self.disp[6 + r.randrange(2)] = 0          # zero screen size: a division by zero on purpose
        self.mode = [r.getrandbits(8), r.choice([0, 1, 1, 2]), r.choice([2, 2, 3, r.getrandbits(8)]),
                     r.choice([0, 1, 1, 2])]
        self.initialized = r.choice([0, 0, 0, 1])
        self.unscrambled = r.choice([0, 1, 1, 1])
        self.lcg = r.getrandbits(32)
        self.override = r.random() < 0.3
        self.override_masks = [r.getrandbits(64) for _ in range(3)]

    def put(self, t, buf):
        g = t.data("g_gfx")
        b = bytearray(self.junk)
        struct.pack_into("<QQ", b, 0, self.vblank, self.last)
        cb = t.callback(lambda tt, a: 0, 1, "vsyncCallback") if self.cb else 0
        struct.pack_into("<I", b, 0x10, cb)
        struct.pack_into("<4b", b, 0x15, *self.s8s)
        node = buf + 0x7000
        for i, (tag, busy, nw, sema) in enumerate(self.chans):
            head = 0
            for k in range(nw):
                t.write(node, struct.pack("<II", head, 0x40 + 16 * i + k))
                head = node
                node += 16
            struct.pack_into("<BBxxIi", b, 0x1C + 12 * i, tag, busy, head, sema)
        head = 0
        for k in range(self.vwaiters):
            t.write(node, struct.pack("<II", head, 0x60 + k))
            head = node
            node += 16
        struct.pack_into("<I", b, 0x40, head)
        rec = 0
        if self.rb:
            rec = buf + 0x7800
            rcb = t.callback(lambda tt, a: 0, 2, "readback callback") if self.rb == "rec_cb" else 0
            t.write(rec, struct.pack("<HHIII", self.rb_rec[0], 0, self.rb_rec[1], rcb, self.rb_rec[2]))
        struct.pack_into("<I", b, 0x44, rec)
        struct.pack_into("<HHhhHHHH", b, 0x88, *self.disp)
        struct.pack_into("<BBBB", b, 0x98, *self.mode)
        t.write(g, bytes(b))
        t.w8(t.data("s_gfxInitialized"), self.initialized)
        t.w8(t.data("s_vu1CodeUnscrambled"), self.unscrambled)
        t.w32(t.data("s_lcgState"), self.lcg)
        if self.override:
            o = t.data("s_gsTexOverride")
            t.write(o, struct.pack("<3Q", *self.override_masks))
            t.w8(o + 0x1C, 1)
        return g


def complete_dma(t, k):
    """SleepThread default: the DMAC completion interrupt clears every busy flag."""
    g = t.data("g_gfx")
    for c in range(3):
        t.w8(g + 0x1C + 12 * c + 1, 0)


class World:
    """a whole driver state + kernel model for one case."""

    def __init__(self, rng, packet=None, sleep=complete_dma, kernel=None):
        self.ctx = Ctx(rng, packet)
        self.gfx = Gfx(rng)
        self.kernel = kernel or Kernel(sleep=sleep)
        if self.kernel.sleep is None:
            self.kernel.sleep = complete_dma

    def put(self, t):
        buf = t.alloc(BUF, 16)
        self.ctx.put(t, buf)
        g = self.gfx.put(t, buf)
        self.kernel.install(t)
        t.stub(SCE_GS_RESET_GRAPH, lambda tt, a: 0, 4)
        return buf, g


def gs_case(desc, world, body):
    def run(t):
        buf, g = world.put(t)
        body(t, buf, g)
    return Case(desc, run)


# ================================================================== specs

def gen_tex_regs(rng, i, n):
    w = World(rng)
    tex0 = rng.getrandbits(64)
    tex1 = rng.getrandbits(64)
    if rng.random() < 0.5:
        tex1 = (tex1 & ~0x1C) | (rng.choice([0, 1, 3, 4, 6]) << 2)
    if rng.random() < 0.3:
        tex0 &= (1 << 61) - 1                                        # CLD 0
    regs = [tex0, tex1] + [rng.getrandbits(64) for _ in range(3)]

    def body(t, buf, g):
        p = t.put(struct.pack("<5Q", *regs), 16)
        t.call(F["set_texture_regs"], p, ret="void")
    return gs_case("gfx_set_texture_regs(MXL %d, CLD %d, override %d)" % (
        (tex1 >> 2) & 7, tex0 >> 61, w.gfx.override), w, body)


def gen_vsync_poll(rng, i, n):
    w = World(rng)
    k = rng.choice([0, 0, 1, 2, 5, rng.randrange(1, 30)])
    never = i % 40 == 39
    seq = [rng.choice([0, 0x10, 0x2000, 0x551B0000]) for _ in range(k)] + [8 | rng.getrandbits(3) << 4]

    def body(t, buf, g):
        t.reg(GS_CSR, read=0 if never else seq)
        t.call(F["wait_vsync_poll"], ret="void", maxsteps=3000000)
    return gs_case("gfx_wait_vsync_poll(%s)" % ("VSINT never set: 0x3A980 polls" if never else
                                                "VSINT after %d polls" % k), w, body)


def gen_reset_paths(rng, i, n):
    w = World(rng)
    fb = rng.getrandbits(32)

    def body(t, buf, g):
        t.vi(28, fb)
        t.call(F["reset_paths"], ret="void")
    return gs_case("gfx_reset_paths(FBRST 0x%X)" % fb, w, body)


def obs_vu(t):
    t.obs("VU0 vf28..31", [t.vf(k) for k in (28, 29, 30, 31)])
    t.obs("VU0 vi10..13", [t.vi(k) for k in (10, 11, 12, 13)])


def gen_vu0_constants(rng, i, n):
    w = World(rng)
    which = ["init_vu0_constants", "init_vu0_constants_spr", "vu0_load_identity_regs"][i % 3]
    in_ram = rng.random() < 0.4
    vf = {k: [rng.getrandbits(32) for _ in range(4)] for k in (28, 29, 30, 31)}
    vi = {k: rng.getrandbits(16) for k in (10, 11, 12, 13)}

    def body(t, buf, g):
        for k, v in vf.items():
            t.vf(k, v)
        for k, v in vi.items():
            t.vi(k, v)
        if which == "init_vu0_constants":
            t.call(F[which], (buf + 0x1000) if in_ram else CTX, ret="void")
        else:
            t.call(F[which], ret="void")
        obs_vu(t)
    return gs_case("%s(%s)" % (which, "RAM ctx" if in_ram and which == "init_vu0_constants" else ""), w, body)


def gen_ctx_init(rng, i, n):
    w = World(rng)
    which = ["init_lights", "init_material", "reset_texture_state", "context_init", "context_init_spr",
             "init_lights_material_spr", "reset_texture_state_spr"][i % 7]
    in_ram = rng.random() < 0.3

    def body(t, buf, g):
        if which in ("init_lights", "init_material", "reset_texture_state", "context_init"):
            t.call(F[which], (buf + 0x1000) if in_ram else CTX, ret="void")
        else:
            t.call(F[which], ret="void")
        obs_vu(t)
    return gs_case("gfx_%s(%s)" % (which, "RAM ctx" if in_ram else "SPR ctx"), w, body)


def gen_init(rng, i, n):
    w = World(rng, kernel=Kernel(intr=rng.choice([0, 1])))
    which = "init_default" if i % 4 == 3 else "init"
    args = [rng.choice([0, 1, 0x100, rng.getrandbits(32)]), rng.choice([0, 1, 2]), rng.choice([2, 3, 0x51]),
            rng.choice([0, 1])]
    csr = rng.choice([None, [0, 0, 8], 0x551B0008, 0x12345678 | 8])

    def body(t, buf, g):
        if csr is not None:
            t.reg(GS_CSR, read=csr)
        if which == "init":
            t.call(F["init"], *args, ret="void", maxsteps=5000000)
        else:
            t.call(F["init_default"], args[2], ret="void", maxsteps=5000000)
        obs_vu(t)
        t.obs("COP0 PCCR/PCR0/PCR1", [t.cop0(r) for r in ("PCCR", "PCR0", "PCR1")])
    return gs_case("gfx_%s(%s; initialized %d, unscrambled %d)" % (
        which, ", ".join("0x%X" % a for a in (args if which == "init" else args[2:3])), w.gfx.initialized,
        w.gfx.unscrambled), w, body)


def gen_shutdown(rng, i, n):
    w = World(rng, kernel=Kernel(intr=rng.choice([0, 1])))
    which = "shutdown" if i % 3 else "dma_shutdown"

    def body(t, buf, g):
        t.call(F[which], ret="void")
        t.obs("COP0 PCCR", t.cop0("PCCR"))
    return gs_case("gfx_%s(initialized %d)" % (which, w.gfx.initialized), w, body)


def gen_display(rng, i, n):
    w = World(rng)
    which = ["change_display_mode", "store_display_mode", "reset_graph_stored_mode", "update_display_regs",
             "set_display_area", "set_screen_size", "set_display_fb"][i % 7]
    m = w.gfx.mode
    if which == "change_display_mode" and rng.random() < 0.3:
        args = [m[1], m[2], m[3]]                                    # unchanged: early return
    elif which in ("change_display_mode", "store_display_mode"):
        args = [rng.choice([0, 1, 2, 0x101]), rng.choice([2, 3, 0x102]), rng.choice([0, 1, 2, 0x101])]
    elif which == "set_display_area":
        args = [rng.getrandbits(16), rng.getrandbits(16), rng.choice([640, rng.randrange(1, 2048)]),
                rng.choice([448, rng.randrange(1, 2048)])]
    elif which == "set_screen_size":
        args = [rng.choice([640, 512, rng.randrange(1, 4096)]), rng.choice([448, 224, rng.randrange(1, 4096)])]
    elif which == "set_display_fb":
        args = [rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32), 0x1C0]
    else:
        args = []
    own = which == "update_display_regs" and rng.random() < 0.3

    def body(t, buf, g):
        if which == "update_display_regs":
            if own:
                d = buf + 0x2000
                t.write(d, t.read(g, GFX_SIZE))                       # another state block as the argument
                t.call(F[which], d, ret="void")
            else:
                t.call(F[which], g, ret="void")
        else:
            t.call(F[which], *args, ret="void")
    return gs_case("gfx_%s(%s)%s" % (which, ", ".join("0x%X" % a for a in args), " (own block)" if own else ""),
                   w, body)


def gen_dma_channels(rng, i, n):
    w = World(rng, kernel=Kernel(intr=rng.choice([0, 1])))
    which = ["dma_channel_init", "dma_init", "dma_lock", "dma_unlock"][i % 4]
    ch = rng.randrange(3)
    chcr = rng.choice([0x40, 0xC0, 0x80, rng.getrandbits(32)])

    def body(t, buf, g):
        if which == "dma_channel_init":
            t.call(F[which], ch, chcr, ret="void")
        elif which == "dma_init":
            for c in range(3):
                t.reg(D_CHCR[c], read=rng_regs[c])
            t.reg(0x1000E010, read=dstat)
            t.reg(0x1000E020, read=dpcr)
            t.reg(0x1000E000, read=dctrl)
            t.call(F[which], ret="void")
        else:
            t.call(F[which], ch, ret="int")
    rng_regs = [rng.getrandbits(32) for _ in range(3)]
    dstat, dpcr, dctrl = rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32)
    return gs_case("gfx_%s(ch %d, chcr 0x%X)" % (which, ch, chcr), w, body)


def busy_sleeper(ch, after, also_vblank=False):
    """SleepThread: the DMAC handler completes channel ch after `after` sleeps
    (never if None); the vblank handler counts a vblank every sleep if asked."""
    def sleep(t, k):
        g = t.data("g_gfx")
        if after is not None and k + 1 >= after:
            t.w8(g + 0x1C + 12 * ch + 1, 0)
        if also_vblank:
            t.w64(g, (t.r64(g) + 1) & M64)
    return sleep


def gen_dma_sync(rng, i, n):
    ch = rng.randrange(3)
    after = rng.choice([1, 1, 2, 3, 5, None]) if i % 10 else None
    w = World(rng, sleep=busy_sleeper(ch, after))
    strs = rng.choice([0, 0, 1, 3])
    pre_busy = rng.choice([0, 1, 1, 1])

    def body(t, buf, g):
        t.w8(g + 0x1C + 12 * ch + 1, pre_busy)
        t.reg(D_CHCR[ch], read=lambda tt, k, latched: (latched | 0x100) if k < strs else (latched & ~0x100))
        t.call(F["dma_sync"], ch, ret="int", maxsteps=2000000)
    return gs_case("gfx_dma_sync(ch %d, busy %d, completes after %s sleeps, STR reads %d)" % (
        ch, pre_busy, after, strs), w, body)


def rand_chain(rng):
    """a DMA source chain description: [(tag id, qwc, irq, target)]"""
    out = []
    for _ in range(rng.randrange(0, 5)):
        out.append((rng.choice([1, 1, 2, 3, 3, 4, 5]), rng.randrange(0, 4), rng.random() < 0.15))
    out.append((rng.choice([7, 7, 0, 6]), rng.randrange(0, 3), False))
    return out


def dma_addr(a):
    """the DMA (physical) form of a virtual address: scratchpad = bit 31 + offset."""
    return 0x80000000 | (a & 0x3FF0) if a >> 28 == 7 else a & 0x0FFFFFFF


def put_chain(t, base, desc, payload_base):
    """lay the chain out at base (qword aligned); data for ref tags at payload_base."""
    p = base
    for k, (tid, qwc, irq) in enumerate(desc):
        nxt = p + 16 + 16 * (qwc if tid in (1, 2, 5, 6, 7) else 0)
        addr = 0
        if tid in (0, 3, 4):
            addr = payload_base + 0x100 * k
            t.write(addr, bytes((k * 31 + j) & 0xFF for j in range(16 * qwc)))
        elif tid in (2, 5):
            addr = nxt                                           # next / call: the next tag follows anyway
        w0 = (tid << 28) | qwc | ((1 << 31) if irq else 0) | (k << 16)
        t.write(p, struct.pack("<4I", w0, dma_addr(addr) if addr else 0, 0x1100 + k, 0x5000 + qwc))
        if tid in (1, 2, 5, 6, 7):
            t.write(p + 16, bytes((k * 17 + j) & 0xFF for j in range(16 * qwc)))
        p = nxt
    return base


def gen_dma_send(rng, i, n):
    w = World(rng)
    which = ["dma_send", "start_source_chain", "kick_frame_lists"][i % 3]
    ch = rng.randrange(3)
    where = rng.choice(["ram", "ram", "spr", "uncached", "kseg0"])
    qwc = rng.choice([0, 1, 4, rng.randrange(0, 64)])
    chains = [rand_chain(rng), rand_chain(rng)]
    hdr_null = rng.choice([False, False, True])
    chain_null = [rng.random() < 0.2, rng.random() < 0.2]
    pre_chcr = [rng.getrandbits(32) for _ in range(3)]

    def addr(t, off, buf):
        if where == "spr":
            return 0x70000000 + off
        a = buf + 0x4000 + off
        return {"ram": a, "uncached": a | 0x20000000, "kseg0": a | 0x80000000}[where]

    def body(t, buf, g):
        for c in range(3):
            t.w8(g + 0x1C + 12 * c + 1, 0)                          # idle channels (no waiting)
            t.reg(D_CHCR[c], read=lambda tt, k, latched, v=pre_chcr[c]: v & ~0x100)
        if which == "dma_send":
            src = addr(t, 0, buf)
            t.write(src, bytes((j * 7) & 0xFF for j in range(16 * qwc)) or b"\0")
            t.call(F[which], ch, src, qwc, ret="void")
        elif which == "start_source_chain":
            c = addr(t, 0, buf)
            put_chain(t, c, chains[0], buf + 0x6000)
            t.call(F[which], ch, c, ret="void")
        else:
            hdr = buf + 0x3000
            c0 = put_chain(t, buf + 0x4000, chains[0], buf + 0x6000)
            c1 = put_chain(t, buf + 0x5000, chains[1], buf + 0x6800)
            t.write(hdr, struct.pack("<4I", 0 if chain_null[0] else c0, 0 if chain_null[1] else c1, 0, 0))
            if hdr_null:
                t.w32(CTX + 0x130, hdr)
            t.call(F[which], 0 if hdr_null else hdr, ret="void")
    return gs_case("gfx_%s(ch %d, %s, qwc %d)" % (which, ch, where, qwc), w, body)


def gen_vu_upload(rng, i, n):
    w = World(rng)
    ch = rng.choice([0, 1])
    size = rng.choice([0, 16, 0x398, 0x800, 0x3DA8, rng.randrange(0, 0x7800)])
    vu = rng.getrandbits(rng.choice([8, 16]))
    where = rng.choice(["ram", "ram", "spr", "image0", "image1"])

    def body(t, buf, g):
        t.w8(g + 0x1C + 12 * ch + 1, 0)
        if where == "spr":
            code = 0x70000000
            sz = min(size, 0x2000)
        elif where == "image0":
            code, sz = t.data("g_gsVu0Microcode"), min(size, 0x398)
        elif where == "image1":
            code, sz = t.data("g_gsVu1Microcode"), min(size, 0x3DA8)
        else:
            code, sz = buf + 0x10, size
            t.write(code, bytes((j * 13 + 5) & 0xFF for j in range(min(sz + 16, 0x7800))))
        t.call(F["vu_upload_microcode"], ch, vu, sz, code, ret="void")
    return gs_case("vu_upload_microcode(ch %d, vu 0x%X, size 0x%X, %s)" % (ch, vu, size, where), w, body)


def gen_disable(rng, i, n):
    w = World(rng)
    what = rng.choice(list(range(0x16)) + list(range(0x63, 0x6E)) + [0x40, rng.getrandbits(32)])

    def body(t, buf, g):
        t.call(F["disable"], what, ret="void")
    return gs_case("gfx_disable(0x%X, packet %d/%d)" % (what, w.ctx.packet_open, w.ctx.sub_open), w, body)


PKT_OPS = ["set_packet_buffer", "get_packet_ptr", "get_packet_space", "begin_packet", "end_packet",
           "close_subpacket", "select_list", "mskpath3", "flush_dirty_regs", "flush_all_regs",
           "collect_dirty_regs", "emit_masked_regs"]


def gen_packets(rng, i, n):
    """a short random sequence of packet-writer operations on one context."""
    w = World(rng)
    ops = []
    for k in range(rng.randrange(1, 7)):
        op = PKT_OPS[(i + k * 5) % len(PKT_OPS)] if k == 0 else rng.choice(PKT_OPS)
        if op == "set_packet_buffer":
            ops.append((op, rng.choice([None, 0x400, 0x800]), rng.choice([0, BUF - 0x400, rng.getrandbits(32)])))
        elif op == "select_list":
            ops.append((op, rng.choice([0, 1, 1, 2, rng.getrandbits(32)])))
        elif op == "mskpath3":
            ops.append((op, rng.choice([0, 1, 0x100, rng.getrandbits(32)])))
        elif op == "emit_masked_regs":
            ops.append((op, rng.choice([0, 1, rng.getrandbits(13), rng.getrandbits(10), rng.getrandbits(64)])))
        else:
            ops.append((op,))

    def body(t, buf, g):
        for op in ops:
            if op[0] == "set_packet_buffer":
                b = 0 if op[1] is None else buf + op[1]
                t.call(F[op[0]], b, op[2], ret="void")
            elif op[0] in ("select_list", "mskpath3"):
                t.call(F[op[0]], op[1], ret="int" if op[0] == "select_list" else "void")
            elif op[0] in ("get_packet_ptr", "begin_packet"):
                t.call(F[op[0]], ret="ptr")
            elif op[0] == "get_packet_space":
                t.call(F[op[0]], ret="int")
            elif op[0] == "collect_dirty_regs":
                t.call(F[op[0]], CTX, buf + 0x6000, ret="int")
            elif op[0] == "emit_masked_regs":
                regs = t.data("s_gsRegsGeneral")                      # a 10-entry table (the mask is cut to it)
                m = op[1] & 0x3FF
                t.call(F[op[0]], buf + 0x6000, m, regs, CTX + 0x30, ret="ptr")
            else:
                t.call(F[op[0]], ret="void")
    return gs_case("packet ops: %s" % " ".join(o[0] for o in ops), w, body)


def gen_sync_pipeline(rng, i, n):
    w = World(rng)
    after = [rng.choice([None, 1, 1, 2, 3]) for _ in range(3)]
    busy = [rng.choice([0, 0, 1]), rng.choice([0, 0, 1])]
    vif = rng.choice([0x1F000003, 0x01000000, 0x2, rng.getrandbits(32) | 3])
    gif = rng.choice([0xC00, 0x400, 0x800])

    def body(t, buf, g):
        t.w8(g + 0x1C + 13, busy[0])
        t.w8(g + 0x1C + 25, busy[1])
        if after[0]:
            t.reg(VIF1_STAT, read=[vif] * after[0] + [vif & ~0x1F000003])
        if after[1]:
            t.reg("vi29", read=[0x100] * after[1] + [0x2])
        if after[2]:
            t.reg(GIF_STAT, read=[gif] * after[2] + [0x3])
        t.call(F["sync_pipeline"], ret="void", maxsteps=2000000)
    w.kernel.sleep = lambda t, k: [t.w8(t.data("g_gfx") + 0x1C + 12 * c + 1, 0) for c in (1, 2)]
    return gs_case("gfx_sync_pipeline(VIF1 %s, VU1 %s, GIF %s polls busy)" % tuple(after), w, body)


def gen_vblank(rng, i, n):
    which = "wait_vblank_event" if i % 3 == 0 else "vsync"
    step = rng.choice([1, 1, 1, 2, 3])
    lag = rng.choice([1, 1, 2, 4])

    def sleep(t, k):
        g = t.data("g_gfx")
        if (k + 1) % lag == 0:
            t.w64(g, (t.r64(g) + step) & M64)
    w = World(rng, sleep=sleep)
    nv = rng.choice([0, 1, 2, 3, -1, -2, -3, -5, 6, rng.randrange(-8, 9)])

    def body(t, buf, g):
        if which == "vsync":
            t.call(F["vsync"], nv, ret="int", maxsteps=3000000)
        else:
            t.call(F["wait_vblank_event"], ret="void", maxsteps=3000000)
    return gs_case("gfx_%s(%d), vblank +%d every %d sleeps (count 0x%X, last 0x%X, inter %d)" % (
        which, nv, step, lag, w.gfx.vblank, w.gfx.last, w.gfx.mode[1]), w, body)


def gen_draw_target(rng, i, n):
    w = World(rng)
    which = ["set_draw_buffer", "set_draw_area", "update_scissor"][i % 3]
    args = {"set_draw_buffer": [rng.getrandbits(32), rng.choice([0, 1, 2, rng.getrandbits(8)]),
                                rng.choice([640, 512, rng.randrange(0, 2048)]), rng.choice([448, rng.randrange(0, 2048)])],
            "set_draw_area": [rng.randrange(-64, 640), rng.randrange(-64, 640), rng.randrange(0, 2048),
                              rng.randrange(-5, 2048)],
            "update_scissor": []}[which]

    def body(t, buf, g):
        if which == "update_scissor":
            t.call(F[which], CTX, ret="void")
        else:
            t.call(F[which], *[a & M64 for a in args], ret="void")
    return gs_case("gfx_%s(%s)" % (which, ", ".join(str(a) for a in args)), w, body)


def gen_clear(rng, i, n):
    w = World(rng, packet=1)
    which = ["set_clear_color", "build_clear_packet", "clear_buffers", "clear_buffers", "emit_render_target_regs"][i % 5]
    rgba, test, z = rng.getrandbits(32), rng.getrandbits(64), rng.getrandbits(32)
    mask = rng.choice([0, 1, 2, 3, 4, 5, 7, rng.getrandbits(32)])
    tgt = rand_target(rng)
    if tgt["w"] == 0:
        tgt["w"] = 1
        tgt["xend"] = (tgt["xoff"] + 16) & 0xFFFF

    def body(t, buf, g):
        if which == "set_clear_color":
            t.call(F[which], rgba, ret="void")
        elif which == "build_clear_packet":
            p = t.put(pack_target(tgt), 16)
            t.call(F[which], CTX, test, rgba, z, p, ret="void")
        elif which == "clear_buffers":
            t.call(F[which], mask, ret="void")
        else:
            t.call(F[which], CTX, ret="void")
    return gs_case("gs_%s(mask 0x%X; tgtA %dx%d tgtB %dx%d)" % (which, mask, w.ctx.tgtA["w"], w.ctx.tgtA["h"],
                                                              w.ctx.tgtB["w"], w.ctx.tgtB["h"]), w, body)


def gen_image_transfer(rng, i, n):
    w = World(rng)
    psm = rng.choice([0, 1, 2, 0x0A, 0x13, 0x14, 0x1B, 0x24, 0x2C, 0x2D, 0x30, rng.getrandbits(8)])
    bb = (rng.getrandbits(24)) | (psm << 24)
    dx, dy = rng.getrandbits(rng.choice([11, 16, 32])), rng.getrandbits(rng.choice([11, 16, 32]))
    wd, ht = rng.choice([0, 1, 64, 640, rng.getrandbits(12)]), rng.choice([0, 1, 448, rng.getrandbits(12)])

    def body(t, buf, g):
        b = t.put(struct.pack("<I", bb), 16)
        t.call(F["image_transfer_packet"], buf + 0x2000, b, dx, dy, wd, ht, ret="int")
    return gs_case("gs_build_image_transfer_packet(psm 0x%X, %dx%d at %d,%d)" % (psm, wd, ht, dx, dy), w, body)


def gen_param_encode(rng, i, n):
    which = "param_encode" if i % 2 == 0 else "pow_encoded"
    v = rng.choice([0.0, 1.0, 0.5, 0.0625, 0.03, 127.9375, 128.0, 200.0, -1.0, rng.uniform(0, 130),
                    rng.uniform(0, 2), rng.uniform(-10, 300)])
    enc = rng.choice([0, 0x10, 0x40000, 0x70000 | rng.getrandbits(10), 0xA0000 | rng.getrandbits(16),
                      rng.getrandbits(20), rng.getrandbits(32)])

    def run(t):
        if which == "param_encode":
            t.call(F[which], ret="u32", floats=(fbits(v),))
        else:
            t.call(F[which], enc, ret="f0", floats=(fbits(v),))
    return Case("%s(%r%s)" % (which, v, ", 0x%X" % enc if which == "pow_encoded" else ""), run)


def gen_sprite(rng, i, n):
    w = World(rng, packet=rng.choice([0, 1, 1]))
    args = [rng.randrange(-100, 700) for _ in range(4)] + [rng.getrandbits(rng.choice([8, 16, 32])) for _ in range(4)]

    def body(t, buf, g):
        t.call(F["draw_textured_sprite"], *[a & M64 for a in args], ret="void")
    return gs_case("gs_draw_textured_sprite(%s)" % ", ".join(str(a) for a in args), w, body)


def gen_small(rng, i, n):
    w = World(rng)
    which = ["get_display_state", "compute_display_period", "set_bg_color", "lcg_rand"][i % 4]
    rgba = rng.getrandbits(32)

    def body(t, buf, g):
        if which == "compute_display_period":
            t.call(F[which], CTX, ret="void")
        elif which == "set_bg_color":
            t.call(F[which], rgba, ret="void")
        else:
            t.call(F[which], ret="int")
            if which == "lcg_rand":
                t.call(F[which], ret="int")
    return gs_case("gs_%s(0x%X; ffmd %d, f1BD %d, field %d, bgImmediate %d kind %d)" % (
        which, rgba, w.gfx.mode[3], w.ctx.u8s[0x1BD], w.ctx.u8s[0x1BC], w.ctx.u8s[0x22], w.ctx.u32s[0xB68]),
        w, body)


def gen_colors(rng, i, n):
    w = World(rng)
    which = ["set_bg_color_rgb", "pack_rgba", "set_material_param", "set_material_param", "store_material_vec"][i % 5]
    fl = [rfloat(rng, -0.5, 1.5) for _ in range(5)]
    mask, kind = rng.randrange(4), rng.choice([0, 1, 2, 3, 4, 5, 11, 11, rng.getrandbits(32)])
    value = rng.choice([0, 1, 64, 128, 129, 1000, rng.getrandbits(8), rng.getrandbits(32)])
    face = rng.choice([0, 1])
    vec = [fbits(rfloat(rng)) for _ in range(4)]
    v128 = vec[0] | (vec[1] << 32) | (vec[2] << 64) | (vec[3] << 96)

    def body(t, buf, g):
        if which == "set_bg_color_rgb":
            t.call(F[which], ret="void", floats=tuple(fbits(x) for x in fl[:4]))
        elif which == "pack_rgba":
            t.call(F[which], ret="u32", floats=tuple(fbits(x) for x in fl))
        elif which == "set_material_param":
            t.call(F[which], mask, kind, value, ret="void")
        else:
            t.call(F[which], CTX, face, kind, v128, ret="void")
    return gs_case("%s(floats %s, mask %d kind 0x%X value 0x%X face %d)" % (
        which, ["%.3f" % x for x in fl], mask, kind, value, face), w, body)


def gen_unscramble(rng, i, n):
    w = World(rng)
    ln = rng.choice([0, 1, 16, rng.randrange(0, 600), -5])
    data = junk(rng, max(ln, 0) + 8, 0.1)

    def body(t, buf, g):
        p = t.put(data, 16)
        t.call(F["xor_unscramble"], p, ln & M64, ret="void")
    return gs_case("xor_unscramble_with_lcg(%d bytes, done %d)" % (ln, w.gfx.unscrambled), w, body)


def gen_handlers(rng, i, n):
    w = World(rng)
    which = ["intc_vblank", "intc_vif1", "intc_gs_finish", "dmac_handler", "dmac_handler", "readback_begin",
             "readback_end"][i % 7]
    pc = [rng.getrandbits(32) for _ in range(2)]
    csr = rng.getrandbits(32)
    stat, code = rng.choice([0, 0x800, rng.getrandbits(32)]), rng.choice([0, 0x10000, 0x20000, 0x30000, 0x40000,
                                                                          rng.getrandbits(32)])
    ch = rng.randrange(3)
    chcr = rng.choice([0x04, 0x84 | (rng.getrandbits(3) << 28), 0x80000004 | rng.getrandbits(3) << 28,
                       0x80070004, 0x80020004, 0x80010004, 0x80040004, 0x60000014, 0x70000004, rng.getrandbits(32)])
    tadr_off = rng.choice([0x7800, 0x7800, 0x7810])
    if which in ("readback_end", "intc_gs_finish") and w.gfx.rb is None:
        w.gfx.rb = "rec"

    def body(t, buf, g):
        t.reg("PCR0", read=pc[0])
        t.reg("PCR1", read=pc[1])
        t.reg(GS_CSR, read=csr)
        t.reg(VIF1_STAT, read=stat)
        t.reg(VIF1_CODE, read=code)
        t.reg(0x10009030, read=buf + tadr_off)                         # D1_TADR -> the readback record
        t.reg(D_CHCR[ch], read=chcr)
        if which == "dmac_handler":
            t.call(F[which], ch, ret="int")
        elif which.startswith("readback"):
            t.call(F[which], g, ret="void")
        else:
            t.call(F[which], rng_cause, ret="int")
        t.obs("COP0 PCR0/PCR1", [t.cop0("PCR0"), t.cop0("PCR1")])
    rng_cause = rng.getrandbits(4)
    return gs_case("%s(ch %d, CHCR 0x%08X, VIF1_STAT 0x%X CODE 0x%X, finish %d, sync %s)" % (
        which, ch, chcr, stat, code, w.gfx.s8s[0], w.gfx.s8s[1:]), w, body)


# ------------------------------------------------- edge cases of the matching rewrites

WAIT_NODES = 0x7400                       # buffer offset of the other threads' waiter nodes


def gen_waiters(rng, i, n):
    """gfx_dma_sync / gfx_wait_vblank_event while other threads change the waiter
    list during the sleep: pushed in front of the stack node (the unlink walks to
    it and patches the previous node), the list dropped or replaced, or the node
    taken off the head (nothing to unlink).  Extra nodes are in the buffer."""
    which = "dma_sync" if i % 2 == 0 else "wait_vblank_event"
    ch = rng.randrange(3)
    k0 = rng.choice([0, 0, 1, 2])
    timeout = which == "dma_sync" and rng.random() < 0.15
    # a timeout drops the list itself: only the modes that never leave a
    # buffer node pointing at the stack node
    mode = rng.choice(["drop", "drop_push"]) if timeout else \
        rng.choice(["push", "push", "push", "drop", "drop_push", "pop_head"])
    m = rng.randrange(1, 4)
    done = None if timeout else rng.choice([k0, k0 + 1, k0 + 3])
    head_off = 0x1C + 12 * ch + 4 if which == "dma_sync" else 0x40
    cell = {}

    def sleep(t, k):
        g = t.data("g_gfx")
        a = g + head_off
        if k == k0:
            head = t.r32(a)
            if mode in ("push", "drop_push"):
                nxt = head if mode == "push" else 0
                for j in range(m):
                    nd = cell["buf"] + WAIT_NODES + 16 * j
                    t.write(nd, struct.pack("<II", nxt, 0x70 + j))
                    nxt = nd
                t.w32(a, nxt)
            elif mode == "drop":
                t.w32(a, 0)
            else:                                                     # pop_head: the stack node
                t.w32(a, t.r32(head) if head else 0)
        if done is not None and k >= done:
            if which == "dma_sync":
                t.w8(g + 0x1C + 12 * ch + 1, 0)
            elif k == done:
                t.w64(g, (t.r64(g) + 1) & M64)
    w = World(rng, sleep=sleep)

    def body(t, buf, g):
        cell["buf"] = buf
        if which == "dma_sync":
            t.w8(g + 0x1C + 12 * ch + 1, 1)                           # busy: it waits
            t.call(F["dma_sync"], ch, ret="int", maxsteps=2000000)
        else:
            t.call(F["wait_vblank_event"], ret="void", maxsteps=2000000)
    return gs_case("gfx_%s(%s): at sleep %d %s %d node(s), %s" % (
        which, "ch %d" % ch if which == "dma_sync" else "", k0, mode, m,
        "never completes (timeout)" if timeout else "done at sleep %d" % done), w, body)


def gen_image_alias(rng, i, n):
    """gs_build_image_transfer_packet with dstBitblt pointing INTO the packet it
    writes (on purpose): the PSM byte is read before any store, the BITBLTBUF
    word after the store of qword 1's low word and before its register-address
    store - the retail order, which the union packet stores must keep."""
    w = World(rng)
    k = rng.choice([4, 4, 5, 6, 6, 7, 0, 1, 2, 3, 8, 9, 12, 13, 16, 20, 21, 22, 23])
    psm = rng.choice([0, 1, 2, 0x0A, 0x13, 0x14, 0x1B, 0x24, 0x2C, 0x2D, rng.getrandbits(8)])
    pre = bytearray(junk(rng, 0x70, 0.2))
    pre[4 * k + 3] = psm
    dx, dy = rng.getrandbits(rng.choice([11, 16, 32])), rng.getrandbits(rng.choice([11, 16, 32]))
    wd, ht = rng.choice([1, 64, 640, rng.getrandbits(12)]), rng.choice([1, 448, rng.getrandbits(12)])

    def body(t, buf, g):
        pkt = buf + 0x2000
        t.write(pkt, bytes(pre))
        t.call(F["image_transfer_packet"], pkt, pkt + 4 * k, dx, dy, wd, ht, ret="int")
    return gs_case("gs_build_image_transfer_packet(dstBitblt = pkt word %d, psm 0x%X, %dx%d)" % (k, psm, wd, ht),
                   w, body)


def gen_clear_edges(rng, i, n):
    """gs_build_clear_packet's strip loop at its edges: xend at or below xoff (one
    strip), exactly k strips past it, one past that, xoff with low bits set; the
    loop must end, so xend <= 0xFC00 + (xoff & 0x3FF)."""
    w = World(rng, packet=1)
    xoff = rng.choice([0, 0x6C00, 0x7000 | rng.getrandbits(10), rng.getrandbits(15), 0xF000 | rng.getrandbits(10)])
    k = rng.randrange(0, 12)
    kind = rng.choice(["below", "equal", "exact", "exact+1", "random"])
    hi = 0xFC00 + (xoff & 0x3FF)
    xend = {"below": xoff - rng.randrange(0, xoff + 1), "equal": xoff, "exact": xoff + 0x400 * k,
            "exact+1": xoff + 0x400 * k + 1, "random": rng.randrange(0, hi + 1)}[kind]
    xend = max(0, min(xend, hi))
    tgt = dict(base=rng.getrandbits(16), fbw=rng.getrandbits(8), psm=rng.getrandbits(8), w=rng.randrange(1, 1024),
               h=rng.randrange(1, 1024), xoff=xoff, yoff=rng.getrandbits(16), xend=xend, yend=rng.getrandbits(16))
    test, rgba, z = rng.getrandbits(64), rng.getrandbits(32), rng.getrandbits(32)
    zmask = rng.choice([0, 1, 0xFF])

    def body(t, buf, g):
        t.w8(CTX + 0x1D0, zmask)
        p = t.put(pack_target(tgt), 16)
        t.call(F["build_clear_packet"], CTX, test, rgba, z, p, ret="void")
    return gs_case("gs_build_clear_packet(xoff 0x%X, xend 0x%X: %s, k %d)" % (xoff, xend, kind, k), w, body)


def gen_scissor_edges(rng, i, n):
    """gs_update_scissor_and_viewport with window values outside the 11-bit SCISSOR
    fields (negative, >= 0x800, up to 2^26: truncated, the other bits of the
    shadow kept) and the viewport floats; gfx_set_draw_area with odd / negative
    y and h under halfHeight (arithmetic shifts)."""
    w = World(rng)
    which = "update_scissor" if i % 3 else "set_draw_area"

    def val():
        return rng.choice([0, 1, -1, 0x7FF, 0x800, 0xFFF, -0x800, -0x801, rng.randrange(-0x4000000, 0x4000000),
                           rng.randrange(-4096, 4096)])
    win = [val() for _ in range(4)]
    sc = rng.choice([0, M64, rng.getrandbits(64)])
    half = rng.choice([0, 1])

    def body(t, buf, g):
        t.w64(CTX + 0x80 + 8 * 4, sc)
        t.w8(CTX + 0x23, half)
        if which == "update_scissor":
            t.write(CTX + 0x1AC, struct.pack("<iiii", *win))
            t.call(F[which], CTX, ret="void")
        else:
            t.call(F[which], *[a & M64 for a in win], ret="void")
    return gs_case("%s(%s; SCISSOR 0x%X, halfHeight %d)" % (which, ", ".join(str(a) for a in win), sc, half),
                   w, body)


def gen_rt_edges(rng, i, n):
    """gs_emit_render_target_regs with signed target sizes (negative widths and
    heights: signed divides, truncation toward zero), the SCISSOR fields at 0 /
    0x7FF, and FRAME with PSM >= 0x80 (the 32-bit frame word sign-extends into
    the A+D data).  No zero divisor (that is gs_clear's on-purpose case)."""
    w = World(rng, packet=1)

    def size():
        v = rng.choice([1, -1, 2, -2, 640, -640, 448, rng.randrange(-2048, 2049), rng.randrange(-0x8000, 0x8000)])
        return v or 3
    sizes = [size() for _ in range(4)]                                  # wB, hB, wA, hA
    sc = rng.choice([0, M64, 0x07FF07FF07FF07FF, rng.getrandbits(64)])
    psm = rng.choice([0x80, 0xFF, rng.getrandbits(8)])

    def body(t, buf, g):
        t.w64(CTX + 0x80 + 8 * 4, sc)
        t.w8(CTX + 0x173, psm)
        t.write(CTX + 0x174, struct.pack("<ii", sizes[0], sizes[1]))
        t.write(CTX + 0x188, struct.pack("<ii", sizes[2], sizes[3]))
        t.call(F["emit_render_target_regs"], CTX, ret="void")
    return gs_case("gs_emit_render_target_regs(tgtB %dx%d, tgtA %dx%d, SCISSOR 0x%X, psm 0x%X)" % (
        sizes[0], sizes[1], sizes[2], sizes[3], sc, psm), w, body)


def gen_masked_alias(rng, i, n):
    """gs_emit_masked_regs with the output overlapping the register-number bytes
    and / or the shadow dwords (on purpose): the binary loads both values of a
    qword before storing either, so it copies the old bytes."""
    w = World(rng)
    mask = rng.choice([1, 3, 5, 0xF, 0x3FF, rng.getrandbits(10) | 1, rng.getrandbits(16) | 1])
    roff = rng.choice([0, 1, 7, 8, 9, 15, 16, 17, 24, rng.randrange(0, 64)])     # regNums - out
    soff = rng.choice([None, -8, 0, 8, 16, rng.randrange(-4, 8) * 8])             # shadow - out
    data = junk(rng, 0x200, 0.1)

    def body(t, buf, g):
        base = buf + 0x6000
        t.write(base, data)
        out = base + 0x80
        shadow = out + soff if soff is not None else CTX + 0x30
        t.call(F["emit_masked_regs"], out, mask, out + roff, shadow, ret="ptr")
    return gs_case("gs_emit_masked_regs(mask 0x%X, regNums = out + %d, shadow = %s)" % (
        mask, roff, "ctx->gen" if soff is None else "out %+d" % soff), w, body)


def fns(*names):
    return [F[x] for x in names]


SPECS = [
    Spec("gs_texture_regs", fns("set_texture_regs"), gen_tex_regs, 150),
    Spec("gs_vsync_poll", fns("wait_vsync_poll"), gen_vsync_poll, 40, "GS CSR model; case 40 polls 0x3A980 times"),
    Spec("gs_reset_paths", fns("reset_paths"), gen_reset_paths, 20, "VIF/GIF MMIO + COP2 FBRST"),
    Spec("gs_vu0_constants", fns("init_vu0_constants", "init_vu0_constants_spr", "vu0_load_identity_regs"),
         gen_vu0_constants, 45, "COP2 macro code, vsqi to VU0 memory"),
    Spec("gs_context_init", fns("init_lights", "init_material", "reset_texture_state", "context_init",
                                "context_init_spr", "init_lights_material_spr", "reset_texture_state_spr"),
         gen_ctx_init, 70),
    Spec("gs_init", fns("init", "init_default"), gen_init, 24, "the whole driver init: MMIO, DMA, kernel, COP0"),
    Spec("gs_shutdown", fns("shutdown", "dma_shutdown"), gen_shutdown, 30),
    Spec("gs_display", fns("change_display_mode", "store_display_mode", "reset_graph_stored_mode",
                           "update_display_regs", "set_display_area", "set_screen_size", "set_display_fb"),
         gen_display, 140, "GS privileged registers"),
    Spec("gs_dma_channels", fns("dma_channel_init", "dma_init", "dma_lock", "dma_unlock"), gen_dma_channels, 60),
    Spec("gs_dma_sync", fns("dma_sync"), gen_dma_sync, 60, "SleepThread model, timeout path"),
    Spec("gs_dma_send", fns("dma_send", "start_source_chain", "kick_frame_lists"), gen_dma_send, 90,
         "DMA start snapshots (normal / source chain)"),
    Spec("gs_vu_upload", fns("vu_upload_microcode"), gen_vu_upload, 30, "chain built on the stack"),
    Spec("gs_disable", fns("disable"), gen_disable, 150),
    Spec("gs_packets", fns("set_packet_buffer", "get_packet_ptr", "get_packet_space", "begin_packet", "end_packet",
                           "close_subpacket", "select_list", "mskpath3", "flush_dirty_regs", "flush_all_regs",
                           "collect_dirty_regs", "emit_masked_regs"), gen_packets, 200),
    Spec("gs_sync_pipeline", fns("sync_pipeline"), gen_sync_pipeline, 40, "VIF1/VU1/GIF busy models"),
    Spec("gs_vblank", fns("wait_vblank_event", "vsync"), gen_vblank, 90, "vblank counting through SleepThread"),
    Spec("gs_draw_target", fns("set_draw_buffer", "set_draw_area", "update_scissor"), gen_draw_target, 90),
    Spec("gs_clear", fns("set_clear_color", "build_clear_packet", "clear_buffers", "emit_render_target_regs"),
         gen_clear, 150),
    Spec("gs_image_transfer", fns("image_transfer_packet"), gen_image_transfer, 100),
    Spec("gs_param_encode", fns("param_encode", "pow_encoded"), gen_param_encode, 150, "EE float model"),
    Spec("gs_sprite", fns("draw_textured_sprite"), gen_sprite, 80),
    Spec("gs_small", fns("get_display_state", "compute_display_period", "set_bg_color", "lcg_rand"), gen_small, 80),
    Spec("gs_colors", fns("set_bg_color_rgb", "pack_rgba", "set_material_param", "store_material_vec"),
         gen_colors, 150, "retail set_material_param uses VU0 (vitof0 / vmulx)"),
    Spec("gs_unscramble", fns("xor_unscramble"), gen_unscramble, 30),
    Spec("gs_handlers", fns("intc_vblank", "intc_vif1", "intc_gs_finish", "dmac_handler", "readback_begin",
                            "readback_end"), gen_handlers, 210, "INTC / DMAC handlers, readback"),
    Spec("gs_waiters", fns("dma_sync", "wait_vblank_event"), gen_waiters, 80,
         "waiter lists changed by other threads during the sleep (unlink behind the head)"),
    Spec("gs_image_alias", fns("image_transfer_packet"), gen_image_alias, 60,
         "dstBitblt inside the packet: read order against the packet stores"),
    Spec("gs_clear_edges", fns("build_clear_packet"), gen_clear_edges, 80, "strip loop boundaries"),
    Spec("gs_scissor_edges", fns("update_scissor", "set_draw_area"), gen_scissor_edges, 90,
         "11-bit SCISSOR bitfields, viewport floats, halfHeight shifts"),
    Spec("gs_rt_edges", fns("emit_render_target_regs"), gen_rt_edges, 80,
         "signed scissor rescale, sign-extended FRAME word"),
    Spec("gs_masked_alias", fns("emit_masked_regs"), gen_masked_alias, 60,
         "output overlapping regNums / shadow: both loads before the stores"),
]
