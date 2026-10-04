"""kernl_sys group: unit kernl_sys (sdk/libkernl/kernel.c, sifcmd.c, syscalls.S).

  * kernl_syscall_stubs: every `addiu $v1, N; syscall; jr $ra` stub of the unit
    (common.syscall_stub_spec), under a syscall hook;
  * cache.o: the D-cache walks (SyncDCache / InvalidDCache and their i- and
    _-forms, sceSifWriteBackDCache) against a model of the cache tags: every
    CACHE op and its address, the DIntr / EIntr bracket (di / ei / sync.p);
  * intr.o: the INTC / DMAC mask wrappers (Status.EIE, sync.l, kernel result);
  * alarm.o: InitAlarm (T3_MODE, the copied kernel patch, the syscall table);
  * thread.o: InitThread, topThread (until its WaitSema blocks), the
    interrupt-context thread calls through the top-thread request ring;
  * the SIO putc and the kernel printf engine (_printf, scePrintf with kPuts
    stubbed, _print_double through libgcc's soft-float, _dtoi);
  * sifcmd.o / sifrpc.o: command handler tables, packet sends (the sceSifSetDma
    transfer lists and the words they carry), the SIF0 interrupt dispatcher,
    sceSifInitCmd / sceSifInitRpc handshakes with a model IOP (common.Kernel
    sif_regs / on_dma), the RPC packet pools and the END / RDATA / BIND servers.
"""
from .common import *
from difftest import HOOK_LO
import eeemu as E

UNITS = ("kernl_sys",)

K = dict(
    _SyncDCache=0x0101C050, SyncDCache=0x0101C0F8, iSyncDCache=0x0101C178, _InvalidDCache=0x0101C190,
    InvalidDCache=0x0101C238, iInvalidDCache=0x0101C2B8, DisableIntc=0x0101C2D0, EnableIntc=0x0101C338,
    DisableDmac=0x0101C3A0, EnableDmac=0x0101C408, iEnableIntc=0x0101C470, iDisableIntc=0x0101C490,
    iEnableDmac=0x0101C4B0, iDisableDmac=0x0101C4D0, _kCopy_handler=0x0101C510, InitAlarm=0x0101C558,
    topThread=0x0101C630, InitThread=0x0101C708, iWakeupThread=0x0101C7E0, iRotateThreadReadyQueue=0x0101C878,
    iSuspendThread=0x0101C8F8, sio_putc=0x0101C990, _kputs_putc=0x0101C9C8, _sio_putchar_crlf=0x0101CA78,
    _dtoi=0x0101CAB0, _print_double=0x0101CB40, _kernel_vprintf=0x0101CCA8, _printf=0x0101D270,
    scePrintf=0x0101D2A8, set_sreg=0x0101D308, change_addr=0x0101D328, sceSifGetSreg=0x0101D338,
    sceSifSetSreg=0x0101D350, sceSifGetDataTable=0x0101D370, sceSifInitCmd=0x0101D380, sceSifExitCmd=0x0101D600,
    sceSifSetCmdBuffer=0x0101D638, sceSifSetSysCmdBuffer=0x0101D650, sceSifAddCmdHandler=0x0101D668,
    sceSifRemoveCmdHandler=0x0101D698, _sif_cmd_send=0x0101D6C0, sceSifSendCmd=0x0101D7F8,
    isceSifSendCmd=0x0101D838, _sif_cmd_intr_handler=0x0101D878, sceSifWriteBackDCache=0x0101D9A0,
    sceSifInitRpc=0x0101DA50, sceSifExitRpc=0x0101DBF0, _rpc_get_packet=0x0101DC18, _rpc_packet_free=0x0101DCC0,
    _rpc_get_fpacket=0x0101DCE0, _rpc_get_fpacket2=0x0101DD10, _request_end=0x0101DD50, _request_rdata=0x0101DE08,
    sceSifGetOtherData=0x0101DE68, _search_svdata=0x0101DFC0, _request_bind=0x0101E010)
KPUTS = 0x01023130                       # kernl_rest (deci2_tty.c): stubbed
T3_MODE, SIO_ISR, D_STAT, D5_CHCR = 0x10001810, 0x1000F130, 0x1000E010, 0x1000C000


def blocked(t, a, st):
    raise E.EmuError("fake kernel: the thread blocks in WaitSema forever", "blocked")


# ================================================================== cache.o

def rand_tags(rng, lo, hi):
    """{(index, way): TagLo} with lines inside / just outside [lo, hi] and junk."""
    tags = {}
    for idx in range(0, 0x1000, 0x40):
        for way in (0, 1):
            k = rng.random()
            if k < 0.3:
                base = rng.randrange(max(lo - 0x2000, 0), hi + 0x2000) & ~0xFFF
            elif k < 0.4:
                base = 0
            else:
                base = rng.getrandbits(32) & ~0xFFF
            tags[(idx, way)] = base | rng.getrandbits(12) if rng.random() < 0.3 else base
    return tags


def gen_dcache(rng, i, n):
    which = ["SyncDCache", "iSyncDCache", "InvalidDCache", "iInvalidDCache", "_SyncDCache", "_InvalidDCache",
             "sceSifWriteBackDCache"][i % 7]
    lo = rng.choice([0x01100000, 0x01234567, 0x00100040, rng.getrandbits(25)])
    hi = lo + rng.choice([0, 1, 0x3F, 0x40, 0x41, 0x1FF, 0x200, 0x201, 0x1000, rng.randrange(0, 0x8000)])
    tags = rand_tags(rng, lo, hi)
    if rng.random() < 0.1:
        lo, hi = hi, lo                                         # empty range on purpose
    eie = rng.random() < 0.7
    size = rng.choice([0, -1, 1, 0x40, 0x41, 0x200, 0x201, 0x1C0, rng.randrange(-10, 0x2000)])

    def run(t):
        t.cache_tags(lambda tt, op, addr: tags[(addr & 0xFC0, addr & 1)])
        if not eie:
            t.cop0("Status", t.cop0("Status") & ~0x10000)
        if which == "sceSifWriteBackDCache":
            t.call(K[which], lo, size & M64, ret="void")
        else:
            t.call(K[which], lo, hi, ret="void")
    return Case("%s(0x%08X, 0x%08X%s; EIE %d)" % (which, lo, hi, ", size %d" % size if "Sif" in which else "",
                                                 eie), run)


# ================================================================== intr.o

def gen_intr(rng, i, n):
    which = ["DisableIntc", "EnableIntc", "DisableDmac", "EnableDmac", "iEnableIntc", "iDisableIntc",
             "iEnableDmac", "iDisableDmac"][i % 8]
    arg = rng.choice([0, 1, 2, 5, 13, 15, rng.getrandbits(32)])
    res = rng.choice([0, 1, -1, rng.getrandbits(31)])
    eie = rng.random() < 0.7

    def run(t):
        Kernel(intr=res).install(t)
        if not eie:
            t.cop0("Status", t.cop0("Status") & ~0x10000)
        t.call(K[which], arg, ret="int")
    return Case("%s(%d) -> kernel %d, EIE %d" % (which, arg, res, eie), run)


# ================================================================== alarm.o

def gen_alarm(rng, i, n):
    which = "InitAlarm" if i % 3 else "_kCopy_handler"
    mode = rng.choice([0, 0, 0x583, 0x100, rng.getrandbits(32)])
    nbytes = rng.choice([0, 3, 4, 8, 0x20, 0x740, rng.randrange(0, 0x400)])
    data = junk(rng, 0x800)

    def copy(t, a, st):
        t.obs("Copy", (a[0], a[2], Kernel.words(t, a[1], min(a[2], 0x800))), "ptrs")
        return 0

    def run(t):
        if which == "InitAlarm":
            Kernel(ret={0x5A: copy, 0x5B: lambda tt, a, st: 0x80076000 + 4 * a[0]}).install(t)
            t.reg(T3_MODE, read=mode)
            t.call(K[which], ret="void")
        else:
            src = t.put(data, 16)
            dst = t.alloc(0x800, 16)
            t.call(K[which], dst, src, nbytes, ret="int")
    return Case("%s(T3_MODE 0x%X, %d bytes)" % (which, mode, nbytes), run)


# ================================================================== thread.o

def gen_threads(rng, i, n):
    which = ["topThread", "InitThread", "iWakeupThread", "iRotateThreadReadyQueue", "iSuspendThread"][i % 5]
    top_id = rng.choice([0, 0, 3, -2, 0x55])
    sema = rng.getrandbits(8)
    req_in = rng.choice([0, 1, 0x1FF, 0x200, rng.getrandbits(32)])
    req_out = rng.choice([0, 0x1FE, rng.getrandbits(32)])
    reqs = [(rng.choice([0, 1, 2, 2, 3, 0xFF]), rng.getrandbits(8)) for _ in range(0x200)]
    nwait = rng.choice([0, 1, 2, 4])
    self_tid = rng.choice([0x11, 0x3, 0x100, 0x7F, 0x80, 0xFF, 0x101, 0x90])
    arg = rng.choice([self_tid, self_tid, 0x11, 0x12, 0x7F, 0x80, rng.getrandbits(32)])
    c_sema = rng.choice([0x21, 0x21, -1])
    c_thr = rng.choice([0x22, 0x22, -3])

    def run(t):
        waits = [0]

        def wait(tt, a, st):
            waits[0] += 1
            if waits[0] > nwait:
                blocked(tt, a, st)
            return a[0]
        rets = {0x44: wait}
        if which == "InitThread":
            rets[0x40] = lambda tt, a, st: (t.obs("CreateSema max/init", t.peek32(a[0] + 4, 2)), c_sema)[1]
            rets[0x20] = lambda tt, a, st: (t.obs("CreateThread entry/stack/size/gp/prio", t.peek32(a[0] + 4, 5),
                                                  "ptrs"), c_thr)[1]
        Kernel(tid=self_tid, ret=rets).install(t)
        t.w32(t.data("topId"), top_id)
        t.w32(t.data("topSema"), sema)
        ta = t.data("topArg")
        t.write(ta, struct.pack("<II", req_out, req_in) + b"".join(struct.pack("<BB", *r) for r in reqs))
        if which == "topThread":
            t.call(K[which], ta, ret="void", maxsteps=200000)
        elif which == "InitThread":
            t.call(K[which], ret="int")
        else:
            t.call(K[which], arg, ret="int")
    return Case("%s(0x%X; topId %d, ring in 0x%X out 0x%X, self %d, %d WaitSema)" % (
        which, arg, top_id, req_in, req_out, self_tid, nwait), run)


# ================================================================== SIO / printf

def gen_sio(rng, i, n):
    c = rng.getrandbits(32)
    busy = rng.choice([0, 0, 1, 3])

    def run(t):
        t.reg(SIO_ISR, read=[0x8000 | rng_extra] * busy + [rng_extra & ~0x8000])
        t.call(K["sio_putc"], c, ret="int")
    rng_extra = rng.getrandbits(32)
    return Case("sio_putc(0x%X), TX FIFO full for %d polls" % (c, busy), run)


CONVS = ["d", "u", "x", "o", "s", "c", "e", "f", "ld", "lx", "lu", "lo", "hd", "hx", "hu", "ho", "05d", "08x",
         "012o", "040d", "0u", "%", "5d", "g", "p", "X", "l", "h"]


def rand_fmt(rng):
    parts, kinds = [], []
    for _ in range(rng.randrange(0, 7)):
        if rng.random() < 0.4:
            parts.append(rstr(rng, rng.randrange(0, 6)).replace(b"%", b"&").decode("latin1"))
        else:
            c = rng.choice(CONVS)
            parts.append("%" + c)
            if c in ("%", "5d", "g", "p", "X", "l", "h"):
                if c == "5d":
                    parts[-1] = "%5d"
                continue
            kinds.append(c[-1])
    if rng.random() < 0.3:
        parts.append("\n")
    return "".join(parts)[:120], kinds


def rand_arg(rng, kind):
    if kind == "s":
        return ("s", rstr(rng, rng.choice([0, 1, 5, 20])))
    if kind in ("e", "f"):
        return ("f", struct.unpack("<I", struct.pack("<f", rng.choice(
            [0.0, 1.0, -1.0, 0.1, 0.09999, 123456.0, -0.00125, rng.uniform(-1e6, 1e6), rng.uniform(-1, 1)])))[0])
    return ("i", rng.choice([0, 1, -1, 7, 0x80000000, 0x7FFFFFFF, rng.getrandbits(32), rng.getrandbits(64),
                             -rng.getrandbits(40)]) & M64)


def gen_printf(rng, i, n):
    which = ["_printf", "scePrintf", "scePrintf"][i % 3]
    fmt, kinds = rand_fmt(rng)
    kinds = kinds[:7]
    args = [rand_arg(rng, k) for k in kinds]
    count = rng.choice([0, 0, 0x50, 0x7C, 0x7D, 0x7E, 0x7F])
    stub_puts = rng.random() < 0.95

    def run(t):
        t.native(K["scePrintf"])
        t.stub(KPUTS, lambda tt, a: (tt.obs("kPuts text", tt.cstr_at(a[0])), 0)[1], 1, "kPuts")
        t.w32(t.data("_kputs_count"), count)
        vals = []
        for kind, v in args:
            vals.append(t.cstr(v) if kind == "s" else v)
        t.call(K[which], t.cstr(fmt), *vals, ret="void" if which == "_printf" else "int", maxsteps=3000000)
        t.obs("_kputs_count", t.r32(t.data("_kputs_count")))
    return Case("%s(%r, %s; buffered %d)" % (which, fmt, args, count), run)


def gen_doubles(rng, i, n):
    which = ["_dtoi", "_print_double"][i % 2]
    d = rng.choice([0.0, 1.0, -1.0, 0.5, 0.1, 0.0999999, 1e-300, -2.5e10, 123.456, 1e15, rng.uniform(-1e9, 1e9),
                    rng.uniform(-1, 1), 10.0 ** rng.randrange(-30, 30)])
    bits = struct.unpack("<Q", struct.pack("<d", d))[0]
    if which == "_dtoi" and rng.random() < 0.5:
        bits = rng.choice([rng.getrandbits(64), rng.getrandbits(20), (rng.randrange(0x3C0, 0x440) << 52) |
                           rng.getrandbits(52)])

    def run(t):
        t.call(K[which], bits, ret="int" if which == "_dtoi" else "void", maxsteps=3000000)
    return Case("%s(0x%016X = %r)" % (which, bits, d), run)


# ================================================================== sifcmd.o

def put_cmd_data(t, usr, nsys=32, nusr=None, iopbuf=0x1F000):
    """_sif_cmd_data as sceSifInitCmd leaves it, with a user table `usr`
    (address or 0); returns its address."""
    cd = t.data("_sif_cmd_data")
    sys_tab = t.data("_sif_sys_cmd_handlers")
    t.write(cd, struct.pack("<8I", t.data("_sif_pktbuf") | 0x20000000, t.data("_sif_cmdbuf") | 0x20000000, iopbuf,
                            sys_tab, nsys, usr, nusr if nusr is not None else (8 if usr else 0),
                            t.data("_sif_sregs")))
    return cd


def handler_table(t, n, cb_every=2, real=False):
    """n handler entries: callbacks (recording what they get), NULLs, or the
    real system handlers; returns the table address."""
    tab = t.alloc(8 * n, 16)
    for k in range(n):
        if real and k < 2:
            fn = t.fn(K["change_addr"] if k == 0 else K["set_sreg"])
            data = t.data("_sif_cmd_data")
        elif k % cb_every == 0:
            fn = t.callback(lambda tt, a, k=k: (tt.obs("handler %d packet" % k, Kernel.words(tt, a[0], 0x70), "ptrs"),
                                                0)[1], 2, "cmd handler %d" % k)
            data = 0x5000 + k
        else:
            fn, data = 0, 0x6000 + k
        t.write(tab + 8 * k, struct.pack("<II", fn, data))
    return tab


def gen_sif_cmd(rng, i, n):
    ops = ["sceSifInitCmd", "sceSifExitCmd", "sregs", "tables", "sceSifSendCmd", "isceSifSendCmd",
           "_sif_cmd_send", "set_sreg", "change_addr"]
    which = ops[i % len(ops)]
    inited = rng.choice([0, 0, 1])
    subaddr = rng.choice([0, 0, 0x1F000, rng.getrandbits(32)])
    smflag_polls = rng.randrange(0, 4)
    dstat = rng.choice([0, 0x20, rng.getrandbits(32)])
    chcr5 = rng.choice([0, 0x100, rng.getrandbits(32)])
    rno = rng.choice([0, 1, 31, rng.randrange(32)])
    val = rng.getrandbits(32)
    fcode = rng.choice([0x80000000, 0x80000001, 0x80000008, 0x8000001F, 0, 1, 7, rng.getrandbits(3),
                        0x80000000 | rng.getrandbits(5)])
    pktsize = rng.choice([0x0F, 0x10, 0x14, 0x40, 0x70, 0x71, 0, rng.randrange(0, 0x80)])
    size = rng.choice([0, -4, 0x10, 0x40, rng.randrange(0, 0x200)])
    mode = rng.choice([0, 1, 4, 5, rng.getrandbits(3)])
    pkt = junk(rng, 0x80)
    src = junk(rng, 0x200)
    dest = rng.getrandbits(21)
    iopbuf = rng.choice([0x1F000, rng.getrandbits(21)])

    def run(t):
        k = Kernel(sif_regs={0x80000000: subaddr, 4: [0] * smflag_polls + [0x20000], 2: 0x1E800})
        k.install(t)
        t.w32(t.data("_sif_cmd_inited"), inited)
        if which == "sceSifInitCmd":
            t.reg(D_STAT, read=dstat)
            t.reg(D5_CHCR, read=chcr5)
            t.call(K[which], ret="void", maxsteps=500000)
            return
        usr = handler_table(t, 8)
        cd = put_cmd_data(t, usr, iopbuf=iopbuf)
        if which == "sceSifExitCmd":
            t.w32(t.data("_sif0_handler_id"), val)
            t.call(K[which], ret="void")
        elif which == "sregs":
            t.call(K["sceSifSetSreg"], rno, val, ret="int")
            t.call(K["sceSifGetSreg"], rno, ret="int")
            t.call(K["sceSifGetDataTable"], ret="ptr")
        elif which == "tables":
            t.call(K["sceSifSetCmdBuffer"], usr + 0x10, 4, ret="ptr")
            t.call(K["sceSifAddCmdHandler"], fcode & 0x80000007, t.fn(K["set_sreg"]), 0x1234, ret="void")
            t.call(K["sceSifRemoveCmdHandler"], (fcode ^ 1) & 0x80000007, ret="void")
            t.call(K["sceSifSetSysCmdBuffer"], usr, 8, ret="ptr")
            t.call(K["sceSifAddCmdHandler"], 0x80000003, 0, 0x99, ret="void")
        elif which in ("sceSifSendCmd", "isceSifSendCmd"):
            p = t.put(pkt, 16)
            s = t.put(src, 64)
            t.call(K[which], fcode, p, pktsize, s, dest, size & M64, ret="u32")
        elif which == "_sif_cmd_send":
            p = t.put(pkt, 16)
            s = t.put(src, 64, rng_off)
            t.cache_tags(lambda tt, op, a: 0)
            t.call(K[which], fcode, mode, p, pktsize, s, dest, size & M64, ret="u32")
        else:
            p = t.put(pkt, 16)
            t.w32(p + 0x10, rno)
            t.call(K[which], p, cd, ret="void")
    rng_off = rng.randrange(0, 64, 4)
    return Case("%s(fcode 0x%X, pktsize 0x%X, size %d, mode %d; inited %d, SUBADDR 0x%X)" % (
        which, fcode, pktsize, size, mode, inited, subaddr), run)


def gen_sif_intr(rng, i, n):
    psize = rng.choice([0, 0x10, 0x14, 0x18, 0x40, 0x70, rng.randrange(0, 0x71)])
    fcode = rng.choice([0x80000000, 0x80000001, 0x80000002, 0x80000004, 0x8000001F, 0x80000020, 0x80000100,
                        0, 1, 2, 3, 7, 8, 0x7FFFFFFF, rng.getrandbits(32)])
    pkt = bytearray(junk(rng, 0x80))
    pkt[0] = psize
    struct.pack_into("<I", pkt, 8, fcode)
    struct.pack_into("<I", pkt, 0x10, rng.choice([0, 1, 5, 31]))
    nusr = rng.choice([0, 4, 8, -1])
    real = rng.random() < 0.5
    if real and fcode in (0x80000000, 0x80000001) and 0 < psize < 0x18:
        # set_sreg / change_addr read +0x10..+0x17: a shorter packet leaves them
        # reading never-written stack (indeterminate), so they get a full one
        psize = 0x18
        pkt[0] = psize

    def run(t):
        Kernel().install(t)
        usr = handler_table(t, 8)
        cd = put_cmd_data(t, usr, nusr=nusr & M32)
        sys_tab = t.data("_sif_sys_cmd_handlers")
        tab = handler_table(t, 32, 3, real)
        t.write(sys_tab, t.read(tab, 0x100))
        t.write(t.data("_sif_pktbuf"), bytes(pkt))
        t.call(K["_sif_cmd_intr_handler"], 5, ret="int")
        t.obs("SIF sregs", t.read(t.data("_sif_sregs"), 0x80))
    return Case("_sif_cmd_intr_handler(psize 0x%X, fcode 0x%X, %d user handlers, real sys handlers %d)" % (
        psize, fcode, nusr, real), run)


# ================================================================== sifrpc.o (first half)

def put_rpc_state(t, rd, pid, idx, lens, pools, queue=0):
    """a struct _sif_rpc_state at rd; pools = (pkt, rdata, client) addresses."""
    t.write(rd, struct.pack("<11I", *[x & M32 for x in (pid, pools[0], lens[0], 0, 0, pools[1], lens[1], pools[2],
                                                         lens[2], idx, queue)]))


def gen_rpc_pool(rng, i, n):
    which = ["_rpc_get_packet", "_rpc_packet_free", "_rpc_get_fpacket", "_rpc_get_fpacket2", "_rpc_get_packet"][i % 5]
    pid = rng.choice([0, 1, -1, 0x7FFFFFFF, rng.getrandbits(32)])
    busy = [rng.random() < 0.6 for _ in range(32)]
    if rng.random() < 0.15:
        busy = [True] * 32
    lens = [rng.choice([32, 32, 1, 0, 40]), rng.choice([32, 32, 7, -1]), rng.choice([32, 32, 3, 0])]
    if rng.random() < 0.05:
        lens[1] = 0                                     # rdata_table_len 0: break 7 on purpose
    idx = rng.choice([0, 31, 32, 33, -1, rng.getrandbits(32)])
    rid = rng.choice([0, 1, 31, 32, -1, rng.getrandbits(32)])
    slot = rng.randrange(32)
    uncached = rng.random() < 0.5

    def run(t):
        Kernel().install(t)
        pools = []
        for k in range(3):
            p = t.alloc(32 * 0x40, 64)
            t.write(p, junk_pool[k])
            pools.append(p | (0x20000000 if uncached else 0))
        for k, b in enumerate(busy):
            t.w32(pools[0] + 0x40 * k + 0x10, (junk_ids[k] | 1) if b else (junk_ids[k] & ~1))
        rd = t.alloc(0x40, 16)
        put_rpc_state(t, rd, pid, idx, [l & M32 for l in lens], pools)
        if which == "_rpc_get_packet":
            t.call(K[which], rd, ret="ptr")
        elif which == "_rpc_packet_free":
            t.call(K[which], pools[0] + 0x40 * slot, ret="void")
        elif which == "_rpc_get_fpacket":
            t.call(K[which], rd, ret="ptr")
        else:
            t.call(K[which], rd, rid & M64, ret="ptr")
    junk_pool = [junk(rng, 32 * 0x40) for _ in range(3)]
    junk_ids = [rng.getrandbits(32) for _ in range(32)]
    return Case("%s(pid %d, lens %s, idx %d, rid %d, %d busy)" % (which, pid, lens, idx, rid, sum(busy)), run)


def gen_rpc_servers(rng, i, n):
    which = ["_request_end", "_request_rdata", "_search_svdata", "_request_bind", "sceSifGetOtherData"][i % 5]
    cid = rng.choice([0x8000000A, 0x80000009, 0x8000000C, 0x80000008, rng.getrandbits(32)])
    has_func = rng.random() < 0.6
    tid = rng.choice([0x21, 0, -1, 5])
    nq = [rng.randrange(0, 4) for _ in range(rng.randrange(0, 4))]
    sids = [[rng.choice([0x80000001, 0x80000003, 0x8000000F, rng.getrandbits(32)]) for _ in range(q)] for q in nq]
    sid = rng.choice([0x80000001, 0x80000003, 0x80000099])
    idx = rng.choice([0, 31, 5])
    mode = rng.choice([0, 1, 2, 3])
    size = rng.choice([0, 0x10, 0x100, -1])
    sema = rng.choice([0x21, -1])
    dma_ok = rng.random() < 0.8
    busy = [rng.random() < 0.5 for _ in range(32)]

    def run(t):
        Kernel(ret={0x40: lambda tt, a, st: (tt.obs("CreateSema max/init", tt.peek32(a[0] + 4, 2)), sema)[1]},
               dma_ret=None if dma_ok else 0).install(t)
        pools = [t.alloc(32 * 0x40, 64) | 0x20000000 for _ in range(3)]
        for k, b in enumerate(busy):
            t.w32(pools[0] + 0x40 * k + 0x10, 5 if b else 4)
        # server queues and their registered servers
        qhead = 0
        for qk in reversed(range(len(nq))):
            q = t.alloc(0x18, 16)
            shead = 0
            for sk in reversed(range(nq[qk])):
                s = t.alloc(0x44, 16)
                t.write(s, struct.pack("<I", sids[qk][sk]) + junk_srv[:0x34] + struct.pack("<III", shead, 0, q))
                shead = s
            t.write(q, struct.pack("<iiIIII", 7, 0, shead, 0, 0, qhead))
            qhead = q
        rd = t.data("sif_rpc_state") if which == "sceSifGetOtherData" else t.alloc(0x40, 16)
        put_rpc_state(t, rd, 0x40, idx, [32, 32, 32], pools, qhead)
        put_cmd_data(t, 0, iopbuf=0x1F000)
        client = t.alloc(0x28, 16)
        endf = t.callback(lambda tt, a: 0, 1, "end func") if has_func else 0
        t.write(client, struct.pack("<IIiIIIIIII", pools[0] + 0x80, 3, tid, 0, 0x80000001, 0, 0, endf, 0x4444, 0))
        pkt = t.put(junk_pkt, 16)
        t.write(pkt + 0x1C, struct.pack("<IIIII", client, cid if which == "_request_end" else sid, 0x7000,
                                        size & M32, 0x8000))
        if which in ("_request_end", "_request_rdata", "_request_bind"):
            if which == "_request_rdata":
                t.w32(pkt + 0x20, t.put(junk_pkt, 16))       # src: EE data the IOP asked for
            t.call(K[which], pkt, rd, ret="void")
            t.obs("client", Kernel.words(t, client, 0x28), "ptrs")
        elif which == "_search_svdata":
            t.call(K[which], sid, rd, ret="ptr")
        else:
            src = t.put(junk_pkt, 16)
            t.call(K[which], client, src, 0x1E000, size & M64, mode, ret="int")
            t.obs("receive data", Kernel.words(t, client, 0x1C), "ptrs")
    junk_srv = junk(rng, 0x40)
    junk_pkt = junk(rng, 0x40)
    return Case("%s(cid 0x%X, func %d, tid %d, queues %s, sid 0x%X, mode %d, size %d, sema %d, dma %d)" % (
        which, cid, has_func, tid, nq, sid, mode, size, sema, dma_ok), run)


def gen_rpc_init(rng, i, n):
    which = "sceSifInitRpc" if i % 4 else "sceSifExitRpc"
    rpc_inited = rng.choice([0, 0, 1])
    cmd_inited = rng.choice([0, 1])
    rpcinit = rng.choice([0, 0, 1])
    answer = i % 10 != 9
    subaddr = rng.choice([0, 0x1F000])

    def on_dma(t, k, items):
        # the IOP answers the RPC layer's INIT_CMD (opt 1) with SET_SREG(0, 1):
        # sreg 0 becomes non-zero.  Case 10, 20 ...: it never answers (a runaway
        # on purpose).
        w = items[-1][3]
        if answer and isinstance(w, tuple) and len(w) >= 4 and w[2] == 0x80000002 and w[3] == 1:
            t.w32(t.data("_sif_sregs"), 1)

    def run(t):
        Kernel(sif_regs={0x80000000: subaddr, 0x80000002: rpcinit, 4: 0x20000, 2: 0x1E800},
               on_dma=on_dma).install(t)
        t.w32(t.data("_sif_rpc_inited"), rpc_inited)
        t.w32(t.data("_sif_cmd_inited"), cmd_inited)
        if cmd_inited:
            put_cmd_data(t, 0)
        t.call(K[which], rng_mode, ret="void", maxsteps=500000 if answer else 30000)
        t.obs("sif_rpc_state", Kernel.words(t, t.data("sif_rpc_state"), 0x2C), "ptrs")
    rng_mode = rng.getrandbits(32)
    return Case("%s(rpc inited %d, cmd inited %d, RPCINIT %d, SUBADDR 0x%X%s)" % (
        which, rpc_inited, cmd_inited, rpcinit, subaddr, "" if answer else "; the IOP never answers: runaway on purpose"),
        run)


# ================================================================== edge cases of the matching edits
# (verifier round: _dtoi's shift counts, _kernel_vprintf's parser, the handler
# table offsets, _request_end's re-read of pkt->client)

def gen_dtoi_edges(rng, i, n):
    """Every biased exponent around the decision points (e = -0x36 .. 14, i.e.
    0x3FC..0x441, incl. e == -1 whose shift count is -1 = 63 mod 64), with an
    all-ones, a random and a zero / alternating mantissa, either sign."""
    k = i % 70
    exp = 0x3FC + k
    mant = [0xFFFFFFFFFFFFF, rng.getrandbits(52), [0, 0x5555555555555, 0xAAAAAAAAAAAAA][(i // 70) % 3]][(i // 70) % 3]
    if i >= 210:
        exp = rng.choice([0, 1, 0x3FB, 0x7FF, 0x7FE, 0x400, 0x432, 0x431, 0x433])
        mant = rng.getrandbits(52)
    sign = rng.getrandbits(1)
    bits = (sign << 63) | (exp << 52) | mant

    def run(t):
        t.call(K["_dtoi"], bits, ret="int")
    return Case("_dtoi(0x%016X: exp 0x%03X, e %d)" % (bits, exp, exp - 0x433), run)


def f32(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


# (format, args): the format is followed by a NUL and then "<tail>" + NUL, so a
# conversion that steps past the terminator (a NUL right after '%', 'l', 'h' or
# "%0[N]") formats the tail, deterministically on both sides.  Args: ints are
# 64-bit slot values, ("s", bytes) a string, ("f", bits) a float slot.
PRINTF_EDGES = [
    (b"%", []), (b"ab%", []), (b"%l", []), (b"%h", []), (b"%0", []), (b"%05", []), (b"%012", []),
    (b"%0x", [0x1234]), (b"%00d", [7]), (b"%01d", [0]), (b"%01d", [123]), (b"%031d", [-42 & M64]),
    (b"%032d", [42]), (b"%099x", [0xDEAD]), (b"%0123d", [5]), (b"%05ld", [1 << 63]), (b"%0l5d", [9]),
    (b"%l05d", [-9 & M64]), (b"%h08x", [0x12348765]), (b"%hd|%hu|%hx|%ho", [0x8000, 0xFFFF, 0x1FFFF, 0x7FFF8000]),
    (b"%ld|%lu|%lx|%lo", [1 << 63, M64, M64, M64]), (b"%ld", [(1 << 63) - 1]),
    (b"%d|%u|%x|%o", [0x80000000, 0xFFFFFFFF, 0x80000000, 0xFFFFFFFF]), (b"%d", [0xFFFFFFFF80000000]),
    (b"%d|%u", [0x123456789, 0xFFFFFFFF00000001]), (b"%c%c%c%c", [0x80, 0xFF, 0x100, 0x7F41]),
    (b"%s|%s", [("s", b""), ("s", b"\x80\xff\x01abc")]), (b"%08d", [-5 & M64]), (b"%02d", [12345]),
    (b"%03x%05o%07u", [0xABCDEF, 8, 99]), (b"%%%05%07d", [3]), (b"%e|%f", [("f", 0), ("f", 0x80000000)]),
    (b"%f", [("f", f32(1.5))]), (b"\x80\xfe%c\xff", [0xC3]), (b"%099d", [1]), (b"%lhd|%hld", [0x18000, 0x18000]),
    (b"%0%d", [4]), (b"%05x%x", [0x1F, 0x2F]), (b"%030o", [M64]), (b"%0\xb5d", [6]),
]


def gen_printf_edges(rng, i, n):
    which = ["_printf", "scePrintf"][i % 2]
    fmt, args = PRINTF_EDGES[(i // 2) % len(PRINTF_EDGES)]
    count = [0, 0x7D][(i // (2 * len(PRINTF_EDGES))) % 2]

    def run(t):
        t.native(K["scePrintf"])
        t.stub(KPUTS, lambda tt, a: (tt.obs("kPuts text", tt.cstr_at(a[0])), 0)[1], 1, "kPuts")
        t.w32(t.data("_kputs_count"), count)
        vals = []
        for a in args:
            if isinstance(a, tuple):
                vals.append(t.cstr(a[1]) if a[0] == "s" else a[1])
            else:
                vals.append(a)
        f = t.put(fmt + b"\0<tail>\0", 16)
        t.call(K[which], f, *vals, ret="void" if which == "_printf" else "int", maxsteps=3000000)
        t.obs("_kputs_count", t.r32(t.data("_kputs_count")))
    return Case("%s(%r + NUL + '<tail>', %s; buffered %d)" % (which, fmt, args, count), run)


def gen_sif_handlers(rng, i, n):
    """sceSifAddCmdHandler / sceSifRemoveCmdHandler with fcode bits 29..30 set:
    the entry is at byte offset fcode << 3 (32-bit), so they shift out."""
    hi = [0, 0x20000000, 0x40000000, 0x60000000][i % 4]
    sysb = 0x80000000 if (i // 4) % 2 else 0
    idx = rng.randrange(8)
    fcode = sysb | hi | idx
    rm = (sysb | rng.choice([0, 0x20000000, 0x40000000, 0x60000000])) | rng.randrange(8)
    data = rng.getrandbits(32)

    def run(t):
        Kernel().install(t)
        usr = t.put(junk_tab[0], 16)
        cd = put_cmd_data(t, usr, nusr=8)
        sys_tab = t.put(junk_tab[1], 16)
        t.w32(cd + 0x0C, sys_tab)
        t.call(K["sceSifAddCmdHandler"], fcode, t.fn(K["set_sreg"]), data, ret="void")
        t.call(K["sceSifAddCmdHandler"], fcode ^ 1, 0, data ^ 0xFFFF, ret="void")
        t.call(K["sceSifRemoveCmdHandler"], rm, ret="void")
        t.obs("user table", Kernel.words(t, usr, 0x40), "ptrs")
        t.obs("system table", Kernel.words(t, sys_tab, 0x40), "ptrs")
    junk_tab = [junk(rng, 0x40), junk(rng, 0x40)]
    return Case("sceSifAddCmdHandler(0x%08X) / sceSifRemoveCmdHandler(0x%08X)" % (fcode, rm), run)


def gen_rpc_end_reread(rng, i, n):
    """_request_end re-reads pkt->client (and client->rpcd.tid / paddr) after
    the CALL end function returns.  On purpose, the end function (handed the
    packet as its parameter) retargets pkt->client or changes the first
    client's tid / paddr."""
    act = ["retarget", "tid", "paddr", "none"][i % 4]
    tid1 = rng.choice([0x21, -1, 0])
    tid2 = rng.choice([0x33, -1])
    cid = rng.choice([0x8000000A, 0x8000000A, 0x80000009])

    def run(t):
        Kernel().install(t)
        pool = t.alloc(4 * 0x40, 64) | 0x20000000
        for k in range(4):
            t.w32(pool + 0x40 * k + 0x10, 5)
        pkt = t.put(junk_pkt, 16)
        c1 = t.alloc(0x28, 16)
        c2 = t.alloc(0x28, 16)

        def endf(tt, a):
            if act == "retarget":
                tt.w32(a[0] + 0x1C, c2)
            elif act == "tid":
                tt.w32(c1 + 8, 0x44 if tid1 < 0 else -1 & M32)
            elif act == "paddr":
                tt.w32(c1, pool + 0xC0)
            return 0
        fn = t.callback(endf, 1, "end func")
        t.write(c1, struct.pack("<IIiIIIIIII", pool + 0x40, 3, tid1, 0, 0x80000001, 0, 0, fn, pkt, 0))
        t.write(c2, struct.pack("<IIiIIIIIII", pool + 0x80, 4, tid2, 0, 0x80000001, 0, 0, 0, 0, 0))
        t.write(pkt + 0x1C, struct.pack("<IIIII", c1, cid, 0x7000, 0x7100, 0x7200))
        t.call(K["_request_end"], pkt, t.alloc(0x40, 16), ret="void")
        t.obs("clients", (Kernel.words(t, c1, 0x28), Kernel.words(t, c2, 0x28)), "ptrs")
    junk_pkt = junk(rng, 0x40)
    return Case("_request_end(cid 0x%X; end func: %s; tid %d / %d) - the end func edits the packet on purpose" % (
        cid, act, tid1, tid2), run)


SPECS = [s for s in [syscall_stub_spec('kernl_syscall_stubs', UNITS)] if s] + [
    Spec("ks_dcache", [K[x] for x in ("SyncDCache", "iSyncDCache", "InvalidDCache", "iInvalidDCache", "_SyncDCache",
                                       "_InvalidDCache", "sceSifWriteBackDCache")], gen_dcache, 70,
         "cache-tag model; every CACHE op"),
    Spec("ks_intr", [K[x] for x in ("DisableIntc", "EnableIntc", "DisableDmac", "EnableDmac", "iEnableIntc",
                                     "iDisableIntc", "iEnableDmac", "iDisableDmac")], gen_intr, 80),
    Spec("ks_alarm", [K["InitAlarm"], K["_kCopy_handler"]], gen_alarm, 30, "T3_MODE model, patch copy"),
    Spec("ks_threads", [K[x] for x in ("topThread", "InitThread", "iWakeupThread", "iRotateThreadReadyQueue",
                                        "iSuspendThread")], gen_threads, 150, "topThread runs until WaitSema blocks"),
    Spec("ks_sio", [K["sio_putc"]], gen_sio, 20, "SIO_ISR model"),
    Spec("ks_printf", [K[x] for x in ("_printf", "scePrintf", "_kernel_vprintf", "_kputs_putc", "_sio_putchar_crlf")],
         gen_printf, 240, "SIO output / kPuts lines"),
    Spec("ks_doubles", [K["_dtoi"], K["_print_double"]], gen_doubles, 80, "libgcc soft-float"),
    Spec("ks_sif_cmd", [K[x] for x in ("sceSifInitCmd", "sceSifExitCmd", "sceSifSetSreg", "sceSifGetSreg",
                                        "sceSifGetDataTable", "sceSifSetCmdBuffer", "sceSifSetSysCmdBuffer",
                                        "sceSifAddCmdHandler", "sceSifRemoveCmdHandler", "sceSifSendCmd",
                                        "isceSifSendCmd", "_sif_cmd_send", "set_sreg", "change_addr")],
         gen_sif_cmd, 180, "sceSifSetDma transfer lists"),
    Spec("ks_sif_intr", [K["_sif_cmd_intr_handler"]], gen_sif_intr, 80, "SIF0 packet dispatch"),
    Spec("ks_rpc_pool", [K[x] for x in ("_rpc_get_packet", "_rpc_packet_free", "_rpc_get_fpacket",
                                         "_rpc_get_fpacket2")], gen_rpc_pool, 100),
    Spec("ks_rpc_servers", [K[x] for x in ("_request_end", "_request_rdata", "_search_svdata", "_request_bind",
                                            "sceSifGetOtherData")], gen_rpc_servers, 150),
    Spec("ks_rpc_init", [K["sceSifInitRpc"], K["sceSifExitRpc"]], gen_rpc_init, 40, "model IOP handshake"),
    Spec("ks_dtoi_edges", [K["_dtoi"]], gen_dtoi_edges, 240, "every exponent around e = -0x35 .. 13"),
    Spec("ks_printf_edges", [K[x] for x in ("_printf", "scePrintf", "_kernel_vprintf", "_kputs_putc")],
         gen_printf_edges, 4 * len(PRINTF_EDGES), "parser corners; NUL after '%' / 'l' / 'h' / \"%0N\""),
    Spec("ks_sif_handlers", [K["sceSifAddCmdHandler"], K["sceSifRemoveCmdHandler"]], gen_sif_handlers, 64,
         "fcode bits 29..31"),
    Spec("ks_rpc_end_reread", [K["_request_end"]], gen_rpc_end_reread, 40, "pkt->client re-read after the end func"),
]
