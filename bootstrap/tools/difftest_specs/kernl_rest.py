"""kernl_rest group: units kernl_rpc_fio (sdk/libkernl/fileio.c, sifrpc.c) and kernl_rest
(sdk/libkernl/deci2_tty.c, filedev.c, initsys.c, iopreboot.c, loadfile.c).

Everything that talks to the IOP runs against common.Iop, a model IOP behind the
real SIF command layer: the EE's packets are decoded from its sceSifSetDma
transfers and the answers come back through the EE's own SIF0 interrupt
handler, so sceSifBindRpc / sceSifCallRpc / _request_end and the client
libraries run exactly as on the console.  Servers modelled here:
  * FILEIO (0x80000001): FIO_F_SETUP hands over the completion block and
    answers the IOP version; every other call is accepted (or refused) in the
    receive buffer, then the completion block (sema, fno, result pointer and
    size, the result, per-op data: read head/tail bytes, dirent, stat, output
    buffers) is DMAd to the EE and SIF command 0x80000011 runs _fio_intr;
  * LOADFILE (0x80000006): version (fno 0xFF) and replies to the module /
    ELF / peek / poke calls.
The DECI2 manager (Deci2Call, syscall 0x7C) is a model too (Deci2): sockets,
ReqSend / Poll driving the TTY handler with WRITE / WRITEDONE / READ events,
ExSend / ExRecv moving bytes, kPuts lines.  What is compared: the transfers
(sceSifSetDma words), every handler and kernel call, the memory the code and
the model IOP write, return values.
"""
from .common import *

UNITS = ("kernl_rpc_fio", "kernl_rest")

R = dict(
    sceSifBindRpc=0x0101E0C0, _request_call=0x0101E200, sceSifCallRpc=0x0101E290, sceSifCheckStatRpc=0x0101E480,
    sceSifSetRpcQueue=0x0101E4C0, sceSifRegisterRpc=0x0101E558, sceSifRemoveRpc=0x0101E628,
    sceSifRemoveRpcQueue=0x0101E6C0, sceSifGetNextRequest=0x0101E750, sceSifExecRequest=0x0101E7A8,
    sceSifRpcLoop=0x0101E970, _fio_fd_sema_init=0x0101E9B0, _fio_fd_alloc=0x0101EA10, _fio_fd_to_slot=0x0101EA98,
    _fio_intr=0x0101EB08, _fio_lock_sema_init=0x0101EEC0, _fio_lock=0x0101EF10, _fio_unlock=0x0101EF40,
    _fio_init=0x0101EF50, _fio_version_check=0x0101F0F0, sceFsReset=0x0101F180, sceOpen=0x0101F1B8,
    sceClose=0x0101F440, sceLseek=0x0101F5C0, sceRead=0x0101F7F8, sceWrite=0x0101FA68, sceIoctl=0x0101FD28,
    sceIoctl2=0x01020078, _fio_pathcmd=0x01020250, sceRemove=0x010203F8, sceMkdir=0x01020418, sceRmdir=0x010205C8,
    sceFormat=0x010205E8, sceAddDrv=0x01020858, sceDelDrv=0x01020978, sceDopen=0x01020998, sceDclose=0x01020A38,
    sceDread=0x01020BA0, sceGetstat=0x01020CF8, sceChstat=0x01020E98, sceRename=0x010210D8, sceChdir=0x010212C8,
    sceSync=0x010212E8, sceMount=0x01021480, sceUmount=0x010216F0, sceLseek64=0x01021710, sceDevctl=0x01021948,
    sceSymlink=0x01021B78, sceReadlink=0x01021D58, _sceSifLoadFileInit=0x01021F20,
    _sceSifCheckLoadFileVersion=0x01022020, sceSifLoadFileReset=0x010220B0, _sceSifLoadModuleBuffer=0x010220E8,
    sceSifLoadModuleBuffer=0x010222F0, sceSifLoadStartModuleBuffer=0x01022310, _sceSifLoadModule=0x01022330,
    sceSifLoadModule=0x01022558, sceSifLoadStartModule=0x01022578, _sceSifLoadElfPart=0x01022598,
    sceSifLoadElfPart=0x010226A0, sceSifLoadElf=0x010226C0, sceSifGetIopAddr=0x010226E8,
    sceSifSetIopAddr=0x010227D8, sceSifResetIop=0x010228B8, sceSifIsAliveIop=0x010229E8, sceSifSyncIop=0x01022A10,
    sceSifRebootIop=0x01022A58, DIntr=0x01022B68, EIntr=0x01022BB0, clear_user_syscalls=0x01022BD8,
    _InitSys=0x01022C18, tty_reinit_after_iop_reset=0x01022C40, wait_vblank_start=0x01022C50,
    wait_vblank_get_csr=0x01022C90, write=0x01022CF0, read=0x01022D70, open=0x01022DE8, close=0x01022E10,
    lseek=0x01022E18, stub_ret_minus1_c=0x01022E20, sbrk=0x01022E28, getpid=0x01022ED8, fstat=0x01022EE0,
    isatty=0x01022EF8, kill=0x01022F00, link=0x01022F28, unlink=0x01022F50, sceDeci2Open=0x01022F78,
    sceDeci2Close=0x01022FC0, sceDeci2ReqSend=0x01022FE8, sceDeci2Poll=0x01023018, sceDeci2ExRecv=0x01023040,
    sceDeci2ExSend=0x01023078, sceDeci2ExReqSend=0x010230B0, sceDeci2ExLock=0x010230E0,
    sceDeci2ExUnLock=0x01023108, kPuts=0x01023130, tty_ring_init=0x01023158, tty_ring_put=0x01023180,
    tty_ring_get=0x010231C0, deci2_tty_handler=0x01023200, _deci2_tty_write=0x01023398,
    _deci2_tty_read=0x010234E8, _deci2_tty_init=0x010235B8)
PRINTF = 0x01011678                   # pdistd printf (used by sceSifRebootIop): stubbed
INTC_STAT = 0x1000F000


def blocked(t, a, st):
    raise E.EmuError("fake kernel: the thread sleeps forever", "blocked")


# ================================================================== RPC client / server

def gen_rpc_client(rng, i, n):
    sid = rng.choice([0x80000001, 0x80000003, 0x80000006, rng.getrandbits(32)])
    calls = []
    for _ in range(rng.randrange(1, 4)):
        mode = rng.choice([0, 0, 1, 2, 3])
        ssize = rng.choice([0, 4, 0x10, 0x40, 0x80, rng.randrange(0, 0x200)])
        rsize = rng.choice([0, 4, 0x10, 0x40, rng.randrange(0, 0x100)])
        calls.append((rng.getrandbits(8), mode, ssize, rsize, rng.random() < 0.3, rng.random() < 0.5,
                      rng.getrandbits(32)))
    bind_mode = rng.choice([0, 0, 1])
    unbound = rng.random() < 0.15
    data = junk(rng, 0x200)

    def run(t):
        iop = Iop(bind_unknown=True)
        sif_up(t, iop)
        cd = t.alloc(0x28, 16)
        t.write(cd, b"\0" * 0x28)
        if not unbound:
            t.call(R["sceSifBindRpc"], cd, sid, bind_mode, ret="int")
            t.obs("client after bind", Kernel.words(t, cd, 0x28), "ptrs")
        for fno, mode, ssize, rsize, shared, has_end, para in calls:
            send = t.put(data[:max(ssize, 1)], 64)
            recv = send if shared else t.alloc(max(rsize, 16), 64)
            endf = t.callback(lambda tt, a: 0, 1, "end func") if has_end else 0
            t.call(R["sceSifCallRpc"], cd, fno, mode, send, ssize, recv, rsize, endf, para, ret="int")
            t.call(R["sceSifCheckStatRpc"], cd, ret="int")
            t.obs("receive", t.read(recv, max(rsize, 0)))
    return Case("bind 0x%X (mode %d%s) + calls %s" % (sid, bind_mode, ", never bound" if unbound else "",
                                                      [(c[0], c[1], c[2], c[3]) for c in calls]), run)


def gen_rpc_server(rng, i, n, force=None):
    which = ["queues", "requests", "exec", "loop", "_request_call"][i % 5]
    nq = rng.randrange(1, 4)
    nsd = rng.randrange(1, 5)
    rm = rng.randrange(0, 5)
    rid = rng.choice([0, 4, 5 << 16 | 4, 40 << 16 | 4, rng.getrandbits(32)])
    rmode = rng.choice([0, 1])
    rsize = rng.choice([0, 4, 0x20, 0x100])
    size = rng.choice([0, 0x10, 0x40])
    has_rec = rng.random() < 0.8
    key = rng.choice([0x21, -1])
    active = rng.choice([0, 1])
    dma_fail = rng.choice([0, 0, 1, 2]) if rmode else 0      # rmode 0 retries after a 4M-instruction delay

    def run(t):
        iop = Iop()
        armed = [False, 0]

        def dma_ret(tt, kk):
            if armed[0] and armed[1] < dma_fail:
                armed[1] += 1
                return 0
            return 0x101 + kk
        k = Kernel(sleep=lambda tt, kk: blocked(tt, None, None), dma_ret=dma_ret)
        sif_up(t, iop, k)
        armed[0] = True
        qs = [t.alloc(0x18, 16) for _ in range(nq)]
        sds = [t.alloc(0x44, 16) for _ in range(nsd)]
        for s in sds:
            t.write(s, junk_sd)
        for q in qs:
            t.call(R["sceSifSetRpcQueue"], q, key & M64, ret="void")
        for k_, s in enumerate(sds):
            fn = t.callback(lambda tt, a, k_=k_: (tt.obs("server func", (a[0], a[2])), (rec if has_rec else 0))[1],
                            3, "rpc func %d" % k_)
            t.call(R["sceSifRegisterRpc"], s, 0x80000100 + k_, fn, buf, 0, 0, qs[k_ % nq], ret="void")
        if which == "queues":
            t.call(R["sceSifRemoveRpc"], sds[rm % nsd], qs[rm % nq], ret="ptr")
            t.call(R["sceSifRemoveRpcQueue"], qs[rm % nq], ret="ptr")
            t.call(R["sceSifRemoveRpc"], sds[0], qs[0], ret="ptr")
        else:
            # requests arrive as RPC_CALL packets from the IOP (the real _request_call path)
            for k_ in range(min(rm + 1, nsd)):
                s = sds[k_]
                pkt = t.put(struct.pack("<IIIIIIIIIIIIII", 0x40, 0, 0x8000000A, 0, rid, 0x1E000 + 0x40 * k_, 9,
                                        0x1F000, k_ + 1, size, 0x1D000, rsize, rmode, s) + b"\0" * 8, 16)
                t.write(s + 0x0C, struct.pack("<i", size))
                t.write(s + 0x20, struct.pack("<I", 0))
                t.call(R["_request_call"], pkt, 0, ret="void")
            if which == "requests":
                t.call(R["sceSifGetNextRequest"], qs[0], ret="ptr")
            elif which == "exec":
                t.cache_tags(lambda tt, op, a: 0)
                for s in sds[:1]:
                    t.write(s + 0x24, struct.pack("<IIii", 3, 0x1D000, rsize, rmode))
                    t.write(s + 0x34, struct.pack("<I", rid))
                    t.call(R["sceSifExecRequest"], s, ret="void", maxsteps=3000000 if force is None else 30000000)
            elif which == "loop":
                t.cache_tags(lambda tt, op, a: 0)
                t.call(R["sceSifRpcLoop"], qs[0], ret="void", maxsteps=3000000)
        for q in qs:
            t.obs("queue", Kernel.words(t, q, 0x18), "ptrs")
        for s in sds:
            t.obs("serve", Kernel.words(t, s, 0x44), "ptrs")
    junk_sd = junk(rng, 0x44)
    buf_off = rng.randrange(0, 64, 4)
    rec = 0x01200000 + 0x100 * rng.randrange(16)
    buf = 0x01210000 + buf_off
    if force is not None:                       # drawn as above, then overridden (kr_retry_delay)
        which, rmode, rsize, dma_fail = force
    return Case("rpc server: %s (%d queues, %d servers, rid 0x%X, rmode %d, rsize 0x%X, key %d, dma fails %d)" % (
        which, nq, nsd, rid, rmode, rsize, key, dma_fail), run)


def gen_checkstat(rng, i, n):
    """sceSifCheckStatRpc on hand-made client data: no packet, a packet whose id
    no longer matches (also only in bit 31), a matching packet with rec_id bit 0
    clear or set."""
    k = i % 5
    rpc_id = rng.choice([0, 1, 0x80000000, rng.getrandbits(32)])
    pid = rpc_id if k in (3, 4) else rng.choice([rpc_id ^ 0x80000000, rpc_id ^ 1, rng.getrandbits(32)])
    if k not in (3, 4) and pid == rpc_id:
        pid ^= 2
    rec_id = (rng.getrandbits(31) << 1) | (1 if k in (0, 1, 4) else 0)
    tid, mode = rng.getrandbits(32), rng.getrandbits(32)
    junk_pkt = junk(rng, 0x40)

    def run(t):
        pkt = t.put(junk_pkt[:0x10] + struct.pack("<IIi", rec_id, 0, E32s(rpc_id)) + junk_pkt[0x1C:], 64)
        t.w32(pkt + 0x14, pkt)
        cd = t.put(struct.pack("<IIII", 0 if k == 0 else pkt, pid, tid, mode), 16)
        t.call(R["sceSifCheckStatRpc"], cd, ret="int")
    return Case("sceSifCheckStatRpc(%s, pid 0x%X, rpc_id 0x%X, rec_id 0x%X)" % (
        ["no packet", "id differs, bit 0 set", "id differs, bit 0 clear", "same id, bit 0 clear",
         "same id, bit 0 set"][k], pid, rpc_id, rec_id), run)


class SlowBindIop(Iop):
    """an IOP whose server `sid` is not registered yet for the first `fails`
    binds: they are answered with serve NULL, the way the real IOP answers a
    bind of an unknown server, so the client busy-waits and binds again."""

    def __init__(self, sid, fails, **kw):
        Iop.__init__(self, **kw)
        self.slow_sid, self.fails0 = sid, fails

    def install(self, t, kernel=None):
        self.fails = self.fails0                                     # per side
        return Iop.install(self, t, kernel)

    def on_dma(self, t, items):
        words = items[-1][3]
        if (self.fails and isinstance(words, tuple) and len(words) >= 9 and None not in words[:9] and
                words[2] == 0x80000009 and words[8] == self.slow_sid):
            self.fails -= 1
            self.send(0x80000008, [words[4], words[5], words[6], words[7], 0x80000009, 0, 0, 0])
            self.deliver(t)
            return
        Iop.on_dma(self, t, items)


def gen_retry_delay(rng, i, n):
    """the busy-wait retry loops (0x100001 passes of Sony's nopdelay, about
    7.3M instructions each): sceSifExecRequest's raw SIF DMA retry (rmode 0),
    _fio_init's and _sceSifLoadFileInit's re-bind after a bind that found no
    server.  One retry per case; what is compared is that the delay ends and
    the retry happens with the same calls, DMA and memory."""
    k = i % 3
    if k == 0:
        return gen_rpc_server(rng, 2, n, force=("exec", 0, rng.choice([0, 0x20]), 1))
    if k == 1:
        server = FileioServer(rng)
        path = rand_path(rng)[:0x40]
        flag = rng.choice([1, 2, 0x8001])

        def run(t):
            sif_up(t, SlowBindIop(0x80000001, 1, servers={0x80000001: server}))
            for nm, v in (("_fio_initialized", 0), ("_fio_lock_sema", 0xFFFFFFFF), ("_fio_fd_sema", 0xFFFFFFFF),
                          ("_fio_pending_sema", 0xFFFFFFFF)):
                t.w32(t.data(nm), v)
            t.call(R["sceOpen"], t.cstr(path), flag, 0x1FF, ret="int", maxsteps=30000000)
            t.obs("_fio_slots", Kernel.words(t, t.data("_fio_slots"), 0x200), "ptrs")
        return Case("FILEIO bind finds no server once, then sceOpen(%r, 0x%X) (version %r)" % (
            path[:30], flag, server.version), run)
    server = LoadfileServer(rng)
    path = rand_path(rng)[:0x40]
    alen = rng.choice([0, 8, 0xFC])

    def run(t):
        sif_up(t, SlowBindIop(0x80000006, 1, servers={0x80000006: server}))
        t.w32(t.data("_lf_bind_state"), 0xFFFFFFFF)
        args = t.put(junk_args, 16)
        res = t.alloc(16, 16)
        t.call(R["sceSifLoadStartModule"], t.cstr(path), alen, args, res, ret="int", maxsteps=30000000)
        t.obs("result", t.read(res, 8))
    junk_args = junk(rng, 0x100)
    return Case("LOADFILE bind finds no server once, then sceSifLoadStartModule(%r, %d) (version %r)" % (
        path[:30], alen, server.version), run)


# ================================================================== FILEIO

class FileioServer:
    """the IOP FILEIO server (0x80000001), decided in gen (rng), replayed per side."""

    def __init__(self, rng):
        self.version = rng.choice([b"2240", b"2240", b"....", b"2250"])
        self.refuse = [rng.random() < 0.08 for _ in range(16)]
        self.results = [rng.choice([0, 0, 1, 3, 7, 0x40, 0x200, -2, -5, -19, -22, rng.randrange(-30, 0x1000)])
                        for _ in range(16)]
        self.data = junk(rng, 0x1400, 0.05)
        self.extra = junk(rng, 0x440, 0.05)
        # output length the IOP reports for devctl / readlink / ioctl2: the
        # requested size, or more (the EE clamps it to 0x400)
        self.lens = [rng.choice([None, None, 0x3FF, 0x400, 0x401, 0x800]) for _ in range(16)]

    def __call__(self, t, fno, send, rsize, iop):
        k = iop.fio_calls = getattr(iop, "fio_calls", -1) + 1
        if fno == 0xFF:                                          # setup: completion block address
            iop.fio_block = send[0] if send else None
            return self.version
        if self.refuse[k % 16]:
            return struct.pack("<I", 0) + b"\0" * 12
        w = list(send) + [0] * 0x210
        sema, result, rsz = w[0], w[1], w[2]
        res = self.results[k % 16]
        blk = iop.fio_block
        if blk:
            extra = b""
            if fno == 2:                                         # read: head / tail bytes + aligned DMA
                buf, nbyte = w[4], E32s(w[5])
                cnt = max(0, min(res, nbyte, 0x1000)) if res >= 0 else 0
                head = min((-buf) & 0xF, cnt)
                tail = (cnt - head) & 0xF if cnt > head else 0
                mid = cnt - head - tail
                if mid > 0:
                    t.write(buf + head, self.data[:mid])
                hd, td = self.data[0x1000:0x1000 + head], self.data[0x1100:0x1100 + tail]
                extra = struct.pack("<iiII", head, tail, buf, buf + head + mid) + hd.ljust(0x40, b"\0") + \
                    td.ljust(0x40, b"\0")
            elif fno == 11:                                      # dread: dirent to the caller's buffer
                extra = struct.pack("<I", w[4]) + self.extra[:0x144]
            elif fno == 12:                                      # getstat
                extra = struct.pack("<I", w[3]) + self.extra[:0x40]
            elif fno in (23, 25, 26):                            # devctl / readlink / ioctl2 output
                dst, ln = {23: (w[0x814 // 4], w[0x818 // 4]), 25: (w[4], w[3]),
                           26: (w[0x414 // 4], w[0x418 // 4])}[fno]
                if self.lens[k % 16] is not None:
                    ln = self.lens[k % 16]
                extra = struct.pack("<II", dst, ln) + self.extra[:0x424]
            out = struct.pack("<iiII", E32s(sema), fno, result, rsz) + struct.pack("<q", res)[:8]
            t.write(blk, out[:0x10] + struct.pack("<i", res) + extra if fno != 22 else
                    out[:0x10] + struct.pack("<q", res))
            iop.send(0x80000011, [])
        return struct.pack("<I", 1) + b"\0" * 12


FIO_OPS = ["sceOpen", "sceClose", "sceRead", "sceWrite", "sceLseek", "sceIoctl", "sceIoctl2", "sceRemove", "sceMkdir",
           "sceRmdir", "sceFormat", "sceAddDrv", "sceDelDrv", "sceDopen", "sceDclose", "sceDread", "sceGetstat",
           "sceChstat", "sceRename", "sceChdir", "sceSync", "sceMount", "sceUmount", "sceLseek64", "sceDevctl",
           "sceSymlink", "sceReadlink", "sceFsReset"]


def rand_path(rng):
    k = rng.random()
    if k < 0.1:
        return b""
    if k < 0.15:
        return b"host0:" + bytes(rng.randrange(0x41, 0x5B) for _ in range(rng.choice([0x3F9, 0x400, 0x410])))
    return rng.choice([b"host0:", b"cdrom0:\\", b"mc0:/BASCUS-97102/", b"rom0:"]) + rstr(rng, rng.randrange(0, 40))


def gen_fileio(rng, i, n):
    server = FileioServer(rng)
    inited = rng.random() < 0.2
    ops = []
    for k in range(rng.randrange(1, 4)):
        op = FIO_OPS[(i + 3 * k) % len(FIO_OPS)] if k == 0 else rng.choice(FIO_OPS)
        fd = rng.choice([0, 1, 2, 5, 31, 32, -1, rng.getrandbits(32)])
        ops.append((op, fd, rand_path(rng), rand_path(rng), rng.choice([0, 1, 3, 0x200, 0x8000, 0x8001,
                                                                        rng.getrandbits(32)]),
                    rng.choice([0, 1, 7, 15, 16, 17, 0x40, 0x123, 0x3FF, 0x400, 0x401, rng.randrange(0, 0x1000)]),
                    rng.randrange(0, 16),
                    rng.getrandbits(64)))
    slots = [(rng.getrandbits(8), rng.choice([0, 0, 1, 0x10000000, 0x8000, 0x101, rng.getrandbits(32)]))
             for _ in range(32)]
    pending = [rng.choice([-1, -1, -1, rng.getrandbits(8)]) for _ in range(32)]

    def run(t):
        iop = Iop(servers={0x80000001: server})
        sif_up(t, iop)
        for nm, v in (("_fio_initialized", 0), ("_fio_lock_sema", 0xFFFFFFFF), ("_fio_fd_sema", 0xFFFFFFFF),
                      ("_fio_pending_sema", 0xFFFFFFFF)):
            t.w32(t.data(nm), v)
        if inited:
            t.call(R["_fio_init"], ret="int", label="_fio_init (setup)", maxsteps=3000000)
            sl = t.data("_fio_slots")
            for k, (fd, mode) in enumerate(slots):
                t.write(sl + 16 * k, struct.pack("<iI", fd, mode))
            t.write(t.data("_fio_pending"), struct.pack("<32i", *[p if p < 128 else p - 256 for p in pending]))
        for (op, fd, p1, p2, flag, nbyte, req, x), big in zip(ops, bigs):
            buf = t.alloc(0x1100, 64, nbyte & 0xF)
            path1, path2 = t.cstr(p1), t.cstr(p2)
            argb = t.put(junk_arg, 16)
            alen = nbyte & 0x7FF if big is None else big     # a size argument at its compare boundaries
            blen = nbyte if big is None else big
            args = {"sceOpen": (path1, flag, 0x1FF), "sceClose": (fd,), "sceRead": (fd, buf, nbyte),
                    "sceWrite": (fd, argb + (x & 0xF), nbyte), "sceLseek": (fd, x & M32, req % 3),
                    "sceIoctl": (fd, req % 6, argb), "sceIoctl2": (fd, req, argb, nbyte & 0x7FF, buf, blen),
                    "sceRemove": (path1,), "sceMkdir": (path1, flag), "sceRmdir": (path1,),
                    "sceFormat": (path1, path2, argb, alen), "sceAddDrv": (flag,), "sceDelDrv": (path1,),
                    "sceDopen": (path1,), "sceDclose": (fd,), "sceDread": (fd, buf), "sceGetstat": (path1, buf),
                    "sceChstat": (path1, argb, flag), "sceRename": (path1, path2), "sceChdir": (path1,),
                    "sceSync": (path1, flag), "sceMount": (path1, path2, flag, argb, alen),
                    "sceUmount": (path1,), "sceLseek64": (fd, x, req % 3),
                    "sceDevctl": (path1, flag, argb, alen, buf, nbyte),
                    "sceSymlink": (path1, path2), "sceReadlink": (path1, buf, blen), "sceFsReset": ()}[op]
            t.call(R[op], *args, ret="u64" if op == "sceLseek64" else "int", maxsteps=3000000)
            t.obs("buffer", t.read(buf, 0x1100))
        for nm, sz in (("_fio_slots", 0x200), ("_fio_pending", 0x80)):
            t.obs(nm, Kernel.words(t, t.data(nm), sz), "ptrs")
    junk_arg = junk(rng, 0x800)
    # drawn last (the inputs above stay as they were): sizes at the edges of the
    # unsigned (devctl arglen, ioctl2 buflen, readlink bufsize) and signed
    # (mount / format arglen) bound checks
    bigs = [rng.choice([None, None, None, 0x3FF, 0x400, 0x401, 0x80000000, 0xFFFFFFFF]) for _ in ops]
    return Case("fileio %s (version %r, %s)" % (
        " ".join("%s(fd %d%s)" % (o[0], E32s(o[1]), "" if b is None else ", size 0x%X" % b) for o, b in zip(ops, bigs)),
        server.version, "inited" if inited else "lazy init"), run)


# ================================================================== LOADFILE

class LoadfileServer:
    def __init__(self, rng):
        self.version = rng.choice([b"2240", b"2240", b"....", b"2010"])
        self.replies = [junk(rng, 0x20, 0.3) for _ in range(8)]
        for k in range(8):
            if rng.random() < 0.3:
                self.replies[k] = b"\0" * 4 + self.replies[k][4:]          # ELF load: epc 0

    def __call__(self, t, fno, send, rsize, iop):
        k = iop.lf_calls = getattr(iop, "lf_calls", -1) + 1
        if fno == 0xFF:
            return self.version.ljust(rsize, b"\0")
        return self.replies[k % 8][:max(rsize, 0)]


LF_OPS = ["sceSifLoadModule", "sceSifLoadStartModule", "sceSifLoadModuleBuffer", "sceSifLoadStartModuleBuffer",
          "sceSifLoadElf", "sceSifLoadElfPart", "sceSifGetIopAddr", "sceSifSetIopAddr", "sceSifLoadFileReset"]


def gen_loadfile(rng, i, n):
    server = LoadfileServer(rng)
    bound = rng.random() < 0.3
    bind_fail = rng.random() < 0.05
    ops = []
    for k in range(rng.randrange(1, 3)):
        op = LF_OPS[(i + k) % len(LF_OPS)]
        ops.append((op, rand_path(rng)[:0x120], rand_path(rng)[:0x120], rng.choice([0, 1, 8, 0xFC, 0xFD, 0x200, -1]),
                    rng.random() < 0.7, rng.getrandbits(32), rng.choice([0, 1, 2, 3, -1]), rng.getrandbits(32)))

    def run(t):
        iop = Iop(servers={0x80000006: server})
        sif_up(t, iop)
        t.w32(t.data("_lf_bind_state"), 0 if bound else 0xFFFFFFFF)
        if bound:
            t.write(t.data("_lf_iop_version"), server.version)
            t.write(t.data("_lf_client"), struct.pack("<10I", *([0] * 9 + [IOP_SERVE_BASE])))
        if bind_fail:
            t.w32(t.data("sif_rpc_state") + 8, 0)                    # no packet: sceSifBindRpc -1
        args = t.put(junk_args, 16)
        res = t.alloc(16, 16)
        data = t.alloc(16, 16)
        for op, p1, p2, alen, has_arg, addr, width, val in ops:
            path1, path2 = t.cstr(p1), t.cstr(p2)
            a = args if has_arg else 0
            argl = {"sceSifLoadModule": (path1, alen & M32, a), "sceSifLoadStartModule": (path1, alen & M32, a, res),
                    "sceSifLoadModuleBuffer": (addr, alen & M32, a),
                    "sceSifLoadStartModuleBuffer": (addr, alen & M32, a, res), "sceSifLoadElf": (path1, data),
                    "sceSifLoadElfPart": (path1, path2, data), "sceSifGetIopAddr": (addr, data, width & M32),
                    "sceSifSetIopAddr": (addr, args, width & M32), "sceSifLoadFileReset": ()}[op]
            t.call(R[op], *argl, ret="int", maxsteps=3000000)
            t.obs("results", (t.read(res, 8), t.read(data, 16)))
    junk_args = junk(rng, 0x200)
    return Case("loadfile %s (version %r, bound %d, bind fails %d)" % (
        " ".join(o[0] for o in ops), server.version, bound, bind_fail), run)


# ================================================================== IOP reboot

def gen_iopreboot(rng, i, n):
    which = ["sceSifResetIop", "sceSifIsAliveIop", "sceSifSyncIop", "sceSifRebootIop"][i % 4]
    arg = rng.choice([b"", b"rom0:UDNL", b"rom0:UDNL cdrom0:\\IOPRP224.IMG;1", rstr(rng, rng.randrange(0, 79))])
    img = rng.choice([b"cdrom0:\\IOPRP224.IMG;1", b"", rstr(rng, rng.randrange(60, 75)), rstr(rng, 69),
                      rstr(rng, 70)])
    mode = rng.getrandbits(32)
    smflag = rng.choice([0, 0x10000, 0x40000, 0x70000, rng.getrandbits(32)])
    dma_ok = rng.random() < 0.85
    tty_ok = rng.random() < 0.8
    old_pkt = junk(rng, 0x68)

    def run(t):
        dec = Deci2(open_ok=tty_ok)
        armed = [False]
        k = Kernel(sif_regs={0x80000000: 0x1F000, 4: smflag, 2: 0x1E800},
                   ret={0x7C: dec.call}, dma_ret=lambda tt, kk: 0x101 + kk if dma_ok or not armed[0] else 0)
        dec.reset()
        if which == "sceSifRebootIop":
            sif_up(t, Iop(), k)
            t.stub(PRINTF, lambda tt, a: (tt.obs("printf", (tt.cstr_at(a[0]), tt.cstr_at(a[1]))), 0)[1], 2, "printf")
        else:
            k.install(t)
        armed[0] = True
        t.write(t.data("_sif_reset_pkt"), old_pkt)
        t.cache_tags(lambda tt, op, a: 0)
        if which == "sceSifResetIop":
            t.call(R[which], t.cstr(arg), mode, ret="int")
        elif which == "sceSifRebootIop":
            t.call(R[which], t.cstr(img), ret="int", maxsteps=3000000)
        else:
            t.call(R[which], ret="int")
        t.obs("reset packet", t.read(t.data("_sif_reset_pkt"), 0x68))
    return Case("%s(arg %r, img %r; SMFLAG 0x%X, dma %d)" % (which, arg[:40], img[:40], smflag, dma_ok), run)


# ================================================================== DECI2 model

DECI2_ARGC = {1: 4, 2: 1, 3: 2, 4: 1, -5: 3, -6: 3, -7: 2, -8: 1, -9: 1, 0x10: 1}


class Deci2:
    """the DECI2 manager behind Deci2Call (syscall 0x7C), per side.
    Open {protocol, opt, handler, kstatus} -> socket 1 (or -1); ReqSend marks a
    send pending; Poll runs the registered handler (t.interrupt) with READ /
    READDONE for queued host input, then WRITE (the handler ExSends) until the
    requested bytes are out, then WRITEDONE; ExSend accepts up to `chunk` bytes
    (or fails); ExRecv copies queued input.  Every call's parameter block (the
    words that call takes: DECI2_ARGC; the stack words after it are leftovers
    of earlier frames) and the bytes moved are recorded.  Errors come back as
    negative ints, i.e. sign-extended in $v0 as the kernel returns them (a
    zero-extended 0xFFFFFFFB is a positive 64-bit value to `bltz`, and the
    callers' error paths would never run)."""

    def __init__(self, open_ok=True, chunk=0x200, send_err=False, reqsend_err=False, host_input=b""):
        self.open_ok, self.chunk, self.send_err, self.reqsend_err = open_ok, chunk, send_err, reqsend_err
        self.host_input = host_input
        self.reset()

    def reset(self):
        self.sock = None
        self.pending = False
        self.sent_req = 0
        self.inbox = bytearray(self.host_input)
        self.polls = 0

    def call(self, t, a, st):
        func = E32s(a[0])
        p = t.peek32(a[1], DECI2_ARGC.get(func, 4))
        t.obs("Deci2Call %d" % func, p, "ptrs")
        if func == 1:
            if not self.open_ok:
                return -1
            self.sock = p
            return 1
        if func == 0x10:
            t.obs("kPuts", t.cstr_at(p[0]) if p[0] else None)
            return 0
        if func == 3:
            if self.reqsend_err:
                return -2
            self.pending = True
            return 0
        if func == 4:
            self.polls += 1
            if self.polls > 64:
                raise E.EmuError("DECI2 model: polled more than 64 times", "blocked")
            if self.sock is None:
                return 0
            handler, opt = self.sock[2], self.sock[1]
            if self.inbox:
                n = min(len(self.inbox), 0x80)
                t.interrupt(handler, 1, n, opt, raw=True, label="DECI2 handler READ %d" % n)
                t.interrupt(handler, 2, 0, opt, raw=True, label="DECI2 handler READDONE")
            elif self.pending:
                if self.sent_req:
                    self.pending = False
                    t.interrupt(handler, 4, 0, opt, raw=True, label="DECI2 handler WRITEDONE")
                else:
                    t.interrupt(handler, 3, 0, opt, raw=True, label="DECI2 handler WRITE")
            return 0
        if func == -6:                                               # ExSend {s, buf, len}
            if self.send_err:
                return -5
            n = min(p[2] & 0xFFFF, self.chunk)
            t.obs("DECI2 sent", t.read(p[1], n))
            if n == (p[2] & 0xFFFF):
                self.sent_req = 1
            return n
        if func == -5:                                               # ExRecv {s, buf, len}
            if getattr(self, "recv_err", False):
                return -3
            n = min(p[2] & 0xFFFF, len(self.inbox))
            t.write(p[1], bytes(self.inbox[:n]))
            del self.inbox[:n]
            return n
        return 0


def gen_deci2(rng, i, n):
    which = ["wrappers", "kPuts", "tty_init", "tty_write", "tty_read", "handler", "glue_write", "glue_read",
             "reinit"][i % 9]
    s = rng.getrandbits(32)
    text = rng.choice([b"hello\n", b"\n\n", b"", rstr(rng, rng.randrange(0, 300)), b"a\nb\nc" * 40,
                       b"x" * 0xFE + b"\nyz", b"x" * 0xFF + b"\nyz", b"x" * 0xFD + b"\n\n"])
    ln = rng.choice([len(text), max(len(text) - 1, 0), 0, -1])
    dec = Deci2(open_ok=rng.random() < 0.9, chunk=rng.choice([0x200, 0x200, 7, 0x40]),
                send_err=rng.random() < 0.1, reqsend_err=rng.random() < 0.1,
                host_input=rng.choice([b"", b"", rstr(rng, rng.randrange(1, 30)) + b"\n"]))
    busy = rng.choice([0, 0, 0, 1])
    ring = [rng.randrange(0, 0x100), rng.randrange(0, 0x100), rng.randrange(0, 0x100)]   # count, rp, wp
    ring_data = rstr(rng, 0x100)
    event = rng.choice([1, 2, 3, 4, 0, 5])
    param = rng.choice([0, 0, 4, 0x20, 0x140, 0x141])
    # rlen + the host bytes (< 0x20) stay within the 0x140-byte receive buffer: the
    # handler receives past it unchecked (it only warns), and what an overflow
    # hits depends on the data layout, not on the code
    rlen = rng.choice([0, 0, 0x20, 0x121])
    pkt = struct.pack("<HH", rng.choice([12, 20, 0x40, 0x10C]), 0) + rstr(rng, 0x13C)
    opened = rng.choice([0, 1])
    fd = rng.choice([0, 1, 2, 3, -1])
    # drawn last, so the cases above keep their inputs: an ExRecv error (the
    # handler's "receive error" path), and LF / CR bytes a few bytes after the
    # ring's read pointer (read()'s early return after a line end)
    dec.recv_err = rng.random() < 0.1
    eols = [(rng.randrange(0, 6), rng.choice(b"\n\r")) for _ in range(rng.choice([0, 0, 1, 2]))]
    rdata = bytearray(ring_data)
    for d, ch in eols:
        rdata[(ring[1] + d) % 0x100] = ch
    rdata = bytes(rdata)

    def run(t):
        dec.reset()
        Kernel(ret={0x7C: dec.call}).install(t)
        tty = t.data("s_tty")
        t.w32(t.data("s_tty_opened"), opened)
        if which not in ("tty_init", "reinit", "wrappers", "kPuts"):
            dec.sock = (0x210, tty, t.fn(R["deci2_tty_handler"]), 0)
            # an opened TTY: ring and context as _deci2_tty_init leaves them
            rg = t.data("s_tty_ring")
            t.write(rg, struct.pack("<iiII", 0x100, ring[0], rg + 0x10 + ring[1], rg + 0x10 + ring[2]) + rdata)
            rb = t.data("s_tty_recvbuf") | 0x20000000
            t.write(t.data("s_tty_recvbuf"), pkt)
            t.write(tty, struct.pack("<iiiiIII", 1, 0, rlen, busy, t.data("s_tty_sendbuf") | 0x20000000, rb, rg))
        if which == "wrappers":
            # ExSend's len 0x18010 is an unsigned short argument out of range on
            # purpose: the EABI callee (egcs) masks it to 0x8010.  (GCC 14's n32
            # code leaves the promotion to the caller, as every C caller does,
            # so only this raw register input differs there.)
            for f, args in (("sceDeci2Open", (0x210, 0x1234, t.fn(R["deci2_tty_handler"]))), ("sceDeci2Close", (s,)),
                            ("sceDeci2ReqSend", (s, ord("H"))), ("sceDeci2Poll", (s,)),
                            ("sceDeci2ExRecv", (s, 0x01300000, 0x10)), ("sceDeci2ExSend", (s, 0x01300000, 0x18010)),
                            ("sceDeci2ExReqSend", (s, 0x1F0)), ("sceDeci2ExLock", (s,)),
                            ("sceDeci2ExUnLock", (s,))):
                t.call(R[f], *args, ret="int")
        elif which == "kPuts":
            t.call(R["kPuts"], t.cstr(text), ret="void")
        elif which in ("tty_init", "reinit"):
            t.call(R["_deci2_tty_init" if which == "tty_init" else "tty_reinit_after_iop_reset"], ret="int")
            t.obs("tty", Kernel.words(t, t.data("s_tty"), 0x1C), "ptrs")
        elif which in ("tty_write", "glue_write"):
            b = t.put(text or b"\0", 16)
            if which == "tty_write":
                t.call(R["_deci2_tty_write"], b, ln & M64, ret="int", maxsteps=2000000)
            else:
                t.call(R["write"], fd & M64, b, ln & M64, ret="int", maxsteps=2000000)
        elif which in ("tty_read", "glue_read"):
            b = t.alloc(0x200, 16)
            n_ = min(ln, ring[0]) if ln > 0 else ln              # never wait for more than the ring holds
            if ring[0] == 0:
                n_ = 0
            if which == "tty_read":
                t.call(R["_deci2_tty_read"], b, n_ & M64, ret="int", maxsteps=2000000)
            else:
                t.call(R["read"], fd & M64 if fd != 0 or ring[0] else 1, b, n_ & M64, ret="int", maxsteps=2000000)
            t.obs("read buffer", t.read(b, 0x200))
        else:
            t.call(R["deci2_tty_handler"], event, param, tty, ret="void")
        t.obs("ring", Kernel.words(t, t.data("s_tty_ring"), 0x110), "ptrs")
    return Case("deci2 %s(len %d, busy %d, event %d param 0x%X, ring %s, open %d, chunk 0x%X%s%s%s%s)" % (
        which, ln, busy, event, param, ring, dec.open_ok, dec.chunk, ", send err" if dec.send_err else "",
        ", reqsend err" if dec.reqsend_err else "", ", recv err" if dec.recv_err else "",
        ", line ends at rp+%s" % [d for d, _ in eols] if eols else ""), run)


# ================================================================== initsys / glue

def gen_initsys(rng, i, n):
    which = ["DIntr", "EIntr", "clear_user_syscalls", "_InitSys", "wait_vblank_start", "wait_vblank_get_csr",
             "glue", "sbrk"][i % 8]
    eie = rng.random() < 0.6
    polls = rng.randrange(0, 4)
    flag_after = rng.choice([None, 0, 1, 2])
    incr = rng.choice([0, 0x10, 0x1000, -0x10, 0x100000, rng.getrandbits(32)])
    eoh = rng.choice([0x01FFFFFF, 0x01100000, 0x0104E000, rng.getrandbits(32)])
    brk = rng.choice([0x0104DE94, 0x01100000, rng.getrandbits(32)])
    t3 = rng.choice([0, 0x100])
    topid = rng.choice([0, 3])
    # drawn last (the inputs above stay as they were): the R5900 `di` erratum -
    # Status.EIE still reads set after this many `di; sync.p` passes, so the
    # retry loops of DIntr and sbrk's inline DI() run more than once
    di_lag = rng.choice([0, 0, 1, 2, 5])

    def run(t):
        k = Kernel(ret={0x3E: eoh, 0x5A: 0, 0x5B: lambda tt, a, st: 0x80076000 + 4 * a[0]})
        k.install(t)
        if not eie:
            t.cop0("Status", t.cop0("Status") & ~0x10000)
        if di_lag:
            # read 0 is the entry test; reads 1..di_lag follow a `di` that has not
            # taken effect yet
            t.reg("Status", read=lambda tt, n_, latched: latched | 0x10000 if 1 <= n_ <= di_lag else latched)
        if which in ("DIntr", "EIntr"):
            t.call(R[which], ret="int")
        elif which == "clear_user_syscalls":
            t.call(R[which], ret="void")
        elif which == "_InitSys":
            t.reg(0x10001810, read=t3)
            t.w32(t.data("topId"), topid)
            t.call(R[which], ret="int")
        elif which == "wait_vblank_start":
            t.reg(INTC_STAT, read=[0] * polls + [4])
            t.call(R[which], ret="void")
        elif which == "wait_vblank_get_csr":
            t.reg(INTC_STAT, read=[0] * polls + ([4] if flag_after is None else [0] * 50))

            def setflag(tt, a, st):
                flag_addr.append((a[0], a[1]))
                return 0
            flag_addr = []
            k.ret[0x73] = setflag

            def intc(tt, n_, latched):
                if flag_addr and flag_after is not None and n_ >= flag_after:
                    tt.w32(flag_addr[0][0], 1)
                    tt.w64(flag_addr[0][1], 0x1234567800000000 | n_)
                return 0 if flag_after is not None or n_ < polls else 4
            t.reg(INTC_STAT, read=intc)
            # without the kernel flag the loop ends on VBON and the function
            # returns the csr stack word nothing wrote: indeterminate, not compared
            t.call(R[which], ret="u64" if flag_after is not None else "void")
        elif which == "glue":
            st = t.alloc(0x68, 16)
            for f, args in (("open", (t.cstr(b"x"), 0)), ("close", (3,)), ("lseek", (3, 0, 0)),
                            ("stub_ret_minus1_c", ()), ("getpid", ()), ("fstat", (1, st)), ("isatty", (1,)),
                            ("kill", (2, 9)), ("kill", (1, 3)), ("link", (0, 0)), ("unlink", (0,))):
                t.call(R[f], *args, ret="int")
            t.obs("stat", t.read(st, 0x68))
        else:
            t.w32(t.data("s_heap_break"), brk)
            t.call(R["sbrk"], incr & M64, ret="ptr")
            t.obs("break", t.r32(t.data("s_heap_break")))
    return Case("%s(EIE %d, polls %d, flag %s, incr 0x%X, EndOfHeap 0x%X, break 0x%X, di lag %d)" % (
        which, eie, polls, flag_after, incr, eoh, brk, di_lag), run)


def fns(*names):
    return [R[x] for x in names]


SPECS = [s for s in [syscall_stub_spec('kernl_rest_syscall_stubs', UNITS)] if s] + [
    Spec("kr_rpc_client", fns("sceSifBindRpc", "sceSifCallRpc", "sceSifCheckStatRpc"), gen_rpc_client, 80,
         "model IOP over the real SIF command layer"),
    Spec("kr_rpc_server", fns("sceSifSetRpcQueue", "sceSifRegisterRpc", "sceSifRemoveRpc", "sceSifRemoveRpcQueue",
                              "sceSifGetNextRequest", "sceSifExecRequest", "sceSifRpcLoop", "_request_call"),
         gen_rpc_server, 100, "EE-side RPC server (dead code here)"),
    Spec("kr_checkstat", fns("sceSifCheckStatRpc"), gen_checkstat, 40, "the in-flight test, every outcome"),
    Spec("kr_retry_delay", fns("sceSifSetRpcQueue", "sceSifRegisterRpc", "_request_call", "sceSifExecRequest",
                               "sceOpen", "sceSifLoadStartModule"), gen_retry_delay, 3,
         "busy-wait retry loops (slow: ~7.3M instructions per retry and side)"),
    Spec("kr_fileio", fns(*FIO_OPS) + fns("_fio_fd_sema_init", "_fio_fd_alloc", "_fio_fd_to_slot", "_fio_intr",
                                          "_fio_lock_sema_init", "_fio_lock", "_fio_unlock", "_fio_init",
                                          "_fio_version_check", "_fio_pathcmd"),
         gen_fileio, 280, "FILEIO server model, completion command 0x80000011"),
    Spec("kr_loadfile", fns(*LF_OPS) + fns("_sceSifLoadFileInit", "_sceSifCheckLoadFileVersion",
                                           "_sceSifLoadModuleBuffer", "_sceSifLoadModule", "_sceSifLoadElfPart"),
         gen_loadfile, 90, "LOADFILE server model"),
    Spec("kr_iopreboot", fns("sceSifResetIop", "sceSifIsAliveIop", "sceSifSyncIop", "sceSifRebootIop"),
         gen_iopreboot, 60),
    Spec("kr_deci2", fns("sceDeci2Open", "sceDeci2Close", "sceDeci2ReqSend", "sceDeci2Poll", "sceDeci2ExRecv",
                         "sceDeci2ExSend", "sceDeci2ExReqSend", "sceDeci2ExLock", "sceDeci2ExUnLock", "kPuts",
                         "tty_ring_init", "tty_ring_put", "tty_ring_get", "deci2_tty_handler", "_deci2_tty_write",
                         "_deci2_tty_read", "_deci2_tty_init", "write", "read", "tty_reinit_after_iop_reset"),
         gen_deci2, 180, "DECI2 manager model"),
    Spec("kr_initsys", fns("DIntr", "EIntr", "clear_user_syscalls", "_InitSys", "wait_vblank_start",
                           "wait_vblank_get_csr", "open", "close", "lseek", "stub_ret_minus1_c", "getpid", "fstat",
                           "isatty", "kill", "link", "unlink", "sbrk"), gen_initsys, 80),
]
