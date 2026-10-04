"""sonyio group: units libcdvd (sdk/libcdvd/libcdvd.c), libpad (sdk/libpad/libpad.c) and libmc
(sdk/libmc/libmc.c, src/execps2.c).

The three libraries are SIF RPC clients; every case brings SIF up
(common.sif_up) and runs a short random sequence of library calls against a
model IOP (common.Iop) with a server model per library:
  * libcdvd: init (0x80000592, reply {result, version, version, flag}),
    S / N commands, search file (the IOP writes the sceCdlFILE over the send
    buffer through its .addr), disk ready, power-off server; the IOP can send
    the power-off notification (SIF command 0x80000012).  With `defer` the
    RPC completions arrive only when the EE waits (sceCdSync / _sceCdSyncS
    poll, _sceCdDelay: SetAlarm + WaitSema, the alarm fires);
  * libpad: one shared 0x80-byte buffer, replies {command echo, result word 3,
    IOP actuator buffer word 5, module version}; the pad DMA areas (two
    ping-pong halves) are filled as padman would; actuator blocks go out by
    sceSifSetDma;
  * libmc: mcserv / mcman versions, results, the end-parameter block the IOP
    writes for Read / GetInfo / Chdir (head / tail fragments, card info, cwd)
    and the asynchronous calls finished through sceMcSync (deferred: alarm +
    SleepThread in mc_delay_thread).
Debug output: scePrintf is the harness's base stub, the pdistd printf is
stubbed with its formatted text compared (common.printf_stub)."""
from .common import *

UNITS = ("libcdvd", "libpad", "libmc")

CD = dict(_sceCdAlarmHandler=0x01012FD8, _sceCdDelay=0x01013000, sceCdCallback=0x01013068,
          _sceCdCbDispatch=0x010130C0, _sceCdCallbackThread=0x01013160, sceCdInitEeCB=0x01013220,
          _sceCdAlignReadCb=0x010132F8, _sceCdSemaInit=0x01013398, _sceCdEeCbExit=0x01013430,
          sceCdPOffCallback=0x010134B0, _sceCdPOffCmdHandler=0x01013520, _sceCdPOffCbBind=0x01013560,
          sceCdSearchFile=0x010136E8, _sceCdCheckNCmd=0x010139E8, sceCdNCmdDiskReady=0x01013B58,
          sceCdSync=0x01013BF0, _sceCdSyncS=0x01013C90, _sceCdCheckSCmd=0x01013D00, sceCdInit=0x01013E70,
          sceCdDiskReady=0x01014150, sceCdMmode=0x01014348)
PAD = dict(pad_act_dma_transfer=0x01014410, scePadInit=0x01014540, pad_init_rpc=0x01014680, scePadEnd=0x01014720,
           scePadPortOpen=0x010147A0, scePadPortClose=0x01014988, pad_get_recv_buf=0x01014A40,
           scePadGetFrameCount=0x01014AA0, scePadRead=0x01014AF0, scePadGetState=0x01014B70,
           scePadStateIntToStr=0x01014BE8, scePadSetReqState=0x01014C20, scePadGetReqState=0x01014C88,
           scePadReqIntToStr=0x01014CD8, scePadInfoAct=0x01014D10, scePadInfoComb=0x01014E30,
           scePadInfoMode=0x01014F50, scePadSetMainMode=0x01015088, scePadSetActDirect=0x01015140,
           scePadSetActAlign=0x010151F8, scePadGetButtonMask=0x010152D0, scePadSetButtonInfo=0x01015388,
           scePadInfoPressMode=0x01015438, scePadEnterPressMode=0x01015498, scePadExitPressMode=0x010154F0,
           scePadSetVrefParam=0x01015548, scePadGetPortMax=0x01015618, scePadGetSlotMax=0x01015680,
           scePadGetModVersion=0x010156E8, pad_rpc_cmd_0x14=0x01015750)
PAD_PADDING = 0x01014984                 # F_01014984: one nop of alignment padding, never executed
MC = dict(sceMcInit=0x010157B8, mc_get_rpc_state=0x01015970, sceMcChangeThreadPriority=0x010159A0,
          sceMcGetSlotMax=0x01015A58, sceMcOpen=0x01015B18, sceMcMkdir=0x01015C50, sceMcClose=0x01015C88,
          sceMcSeek=0x01015D40, mc_read_end_callback=0x01015E18, sceMcRead=0x01015EA8, sceMcWrite=0x01015FC8,
          mc_alarm_handler=0x01016148, mc_delay_thread=0x01016170, sceMcSync=0x010161B8,
          mc_getinfo_end_callback=0x01016298, sceMcGetInfo=0x010162F0, sceMcGetDir=0x01016478,
          mc_chdir_end_callback=0x010165D0, sceMcChdir=0x01016658, sceMcFormat=0x010167A8,
          sceMcDelete=0x01016870, sceMcFlush=0x01016998, sceMcSetFileInfo=0x01016A50, sceMcRename=0x01016C28,
          sceMcUnformat=0x01016D90, sceMcGetEntSpace=0x01016E58)
EXECPS2 = 0x01016F70


def rand_name(rng, n=60):
    k = rng.random()
    if k < 0.1:
        return b""
    if k < 0.15:
        return bytes(rng.randrange(0x41, 0x5B) for _ in range(rng.choice([0xFF, 0x100, 0x3FF, 0x400, 0x410])))
    return rng.choice([b"\\SYSTEM.CNF;1", b"/BASCUS-97102/", b"*", b"GT3.DAT"]) + rstr(rng, rng.randrange(0, n))


# ================================================================== libcdvd

class CdServers:
    def __init__(self, rng):
        self.flag = rng.choice([0, 0xFE, 0xFF, 0x12, 0])
        self.vers = (rng.choice([0x100, 0x200, 0x300, 0x1FF]), rng.choice([0x200, 0x100, 0x300]))
        self.res = [rng.choice([0, 1, 2, 6, -1, rng.getrandbits(32)]) for _ in range(16)]
        self.file = junk(rng, 0x24)
        self.k = 0

    def reset(self):
        self.k = 0

    def servers(self):
        def init(t, fno, send, rsize, iop):
            return struct.pack("<iIII", 1, self.vers[0], self.vers[1], self.flag)[:rsize]

        def generic(t, fno, send, rsize, iop):
            self.k += 1
            return struct.pack("<I", self.res[self.k % 16] & M32).ljust(max(rsize, 0), b"\0")[:rsize]

        def search(t, fno, send, rsize, iop):
            if send and len(send) > 0x124 // 4 and send[0x124 // 4]:
                t.write(send[0x124 // 4], self.file)                  # the sceCdlFILE over the buffer head
            return generic(t, fno, send, rsize, iop)
        generic.reset = self.reset                                  # (Iop.install resets per side)
        return {0x80000592: init, 0x80000593: generic, 0x80000595: generic, 0x80000596: generic,
                0x80000597: search, 0x8000059A: generic}


CD_OPS = ["sceCdInit", "sceCdMmode", "sceCdDiskReady", "sceCdSync", "sceCdSearchFile", "sceCdNCmdDiskReady",
          "sceCdCallback", "sceCdInitEeCB", "sceCdPOffCallback", "poff_notify", "_sceCdCbDispatch",
          "_sceCdAlignReadCb", "_sceCdCallbackThread", "_sceCdSyncS", "sceCdInit_exit"]


def gen_cdvd(rng, i, n):
    srv = CdServers(rng)
    defer = rng.random() < 0.3
    pre_init = rng.random() < 0.7
    ops = []
    for k in range(rng.randrange(1, 4)):
        op = CD_OPS[(i + 4 * k) % len(CD_OPS)] if k == 0 else rng.choice(CD_OPS)
        ops.append((op, rng.choice([0, 1, 2, 5, 8, rng.getrandbits(32)]), rand_name(rng), rng.getrandbits(32),
                    rng.choice([0, 1, 0xB, -1, 5])))
    debug = rng.choice([0, 0, 1])
    busy = rng.choice([0, 0, 1])
    cbnum = rng.choice([0, 0xB, 3, -1])
    has_cb = rng.random() < 0.5
    cb_thread = rng.choice([0, 0, 0x30])
    align = [rng.choice([0, 3, 0x40, -1]), rng.choice([0, 5, 0x40])]

    def run(t):
        iop = Iop(servers=srv.servers(), defer=defer)
        sif_up(t, iop)
        srv.k = 0
        printf_stub(t)
        t.w32(t.data("s_cd_debug"), debug)
        if pre_init:
            t.call(CD["sceCdInit"], 1, ret="int", label="sceCdInit (setup)", maxsteps=2000000)
        cb = t.callback(lambda tt, a: 0, 1, "user callback")
        for op, mode, name, x, num in ops:
            if op == "sceCdInit":
                t.call(CD[op], mode & M32, ret="int", maxsteps=2000000)
            elif op == "sceCdInit_exit":
                t.w32(t.data("s_cd_cb_thread_id"), cb_thread)
                t.call(CD["sceCdInit"], 5, ret="int", maxsteps=2000000)
            elif op == "sceCdMmode":
                t.call(CD[op], mode & M32, ret="int", maxsteps=2000000)
            elif op == "sceCdDiskReady":
                t.call(CD[op], mode & M32, ret="int", maxsteps=2000000)
            elif op in ("sceCdSync", "_sceCdSyncS"):
                t.w32(t.data("s_cd_cb_busy"), busy)
                if busy and mode in (0, 2):
                    # a completion pending: its end function (which clears the flag)
                    # runs when the EE waits - sceCdSync(0) goes through _sceCdDelay
                    # (SetAlarm + WaitSema) until then
                    iop.deferred.append(lambda tt: tt.w32(tt.data("s_cd_cb_busy"), 0))
                t.call(CD[op], 0 if mode in (0, 2) else 1, ret="int", maxsteps=2000000)
            elif op == "sceCdSearchFile":
                fp = t.alloc(0x30, 16)
                t.call(CD[op], fp, t.cstr(name), ret="int", maxsteps=2000000)
                t.obs("sceCdlFILE", t.read(fp, 0x30))
            elif op == "sceCdNCmdDiskReady":
                t.call(CD[op], ret="int", maxsteps=2000000)
            elif op == "sceCdCallback":
                t.call(CD[op], cb if has_cb else 0, ret="ptr")
            elif op == "sceCdInitEeCB":
                t.w32(t.data("s_cd_cb_thread_id"), cb_thread)
                t.call(CD[op], mode & 0x7F, t.alloc(0x100, 16), 0x100, ret="int")
            elif op == "sceCdPOffCallback":
                t.call(CD[op], cb if has_cb else 0, x, ret="ptr", maxsteps=2000000)
            elif op == "poff_notify":
                # two separate statics: their order differs between builds
                # (GCC 14 places s_poff_arg first), so never one 8-byte write
                t.w32(t.data("s_poff_func"), cb if has_cb else 0)
                t.w32(t.data("s_poff_arg"), x)
                t.w32(t.data("s_cd_init_busy"), num & 1)
                iop.send(0x80000012, [x])
                iop.deliver(t)
            elif op == "_sceCdCbDispatch":
                p = t.put(struct.pack("<i", num), 16)
                t.w32(t.data("s_cd_cb_thread_id"), cb_thread)
                t.w32(t.data("s_cd_user_cb"), cb if has_cb else 0)
                t.call(CD[op], p, ret="void")
            elif op == "_sceCdAlignReadCb":
                d = t.put(struct.pack("<iiII", align[0], align[1], 0, 0) + junk_align, 64)
                dst = t.alloc(0x100, 16)
                t.write(d + 8, struct.pack("<II", dst, dst + 0x80))
                t.call(CD[op], d, ret="void")
                t.obs("aligned read", t.read(dst, 0x100))
            elif op == "_sceCdCallbackThread":
                t.w32(t.data("s_cd_cb_num"), num & M32)
                t.w32(t.data("s_cd_user_cb"), cb if has_cb else 0)
                t.w32(t.data("s_cd_cb_num_user"), x)
                t.call(CD["_sceCdSemaInit"], ret="void")
                t.call(CD[op], 0, ret="void", maxsteps=200000)
        for nm in ("s_cd_cb_busy", "s_cd_cb_num", "s_ncmd_bind", "s_scmd_bind", "s_init_count", "s_cd_init_busy"):
            t.obs(nm, t.r32(t.data(nm)))
    junk_align = junk(rng, 0x80)
    return Case("libcdvd %s (defer %d, pre-init %d, flag 0x%X, vers %s, debug %d)" % (
        " ".join(o[0] for o in ops), defer, pre_init, srv.flag, srv.vers, debug), run)


class FlakyIop(Iop):
    """an Iop whose RPC_BIND answers for some servers come back with serve NULL
    the first few times once `armed` is set (the IOP module has not registered
    the server yet): nobind = {sid: how many binds fail that way}."""

    def __init__(self, nobind=None, **kw):
        Iop.__init__(self, **kw)
        self.nobind0 = dict(nobind or {})

    def install(self, t, kernel=None):
        self.nobind = dict(self.nobind0)                              # per side
        self.armed = False
        return Iop.install(self, t, kernel)

    def on_dma(self, t, items):
        words = items[-1][3]
        if self.armed and isinstance(words, tuple) and len(words) >= 9 and None not in words[:4] \
                and words[2] == 0x80000009 and self.nobind.get(words[8], 0) > 0:
            self.nobind[words[8]] -= 1
            self.send(0x80000008, [words[4], words[5], words[6], words[7], 0x80000009, 0, 0, 0])
            self.deliver(t)
            return
        Iop.on_dma(self, t, items)


# sceCdInit's retry and failure paths (and the power-off bind it ends with).
# kind -> (refused sceSifSetDma numbers counted from the sceCdInit call, binds
# answered with serve NULL).  The DMAs of sceCdInit(mode != 5) in order: bind
# 0x80000592, the init call, bind 0x80000596 (_sceCdPOffCbBind), the power-off
# call; a refused DMA makes sceSifBindRpc / sceSifCallRpc return -2.  A bind
# error or a missing server costs one _sceCdNopDelay (0x100001 passes, about
# 7 M emulated instructions), so each case has at most one.
CD_INIT_PATHS = {
    "init bind error":        ({0}, {}),
    "init no server":         (set(), {0x80000592: 1}),
    "init RPC refused":       ({1}, {}),
    "POff bind error":        ({2}, {}),
    "POff no server":         (set(), {0x80000596: 1}),
    "POff RPC refused":       ({3}, {}),
    "exit, init bind error":  ({0}, {}),
    "sema re-create":         (set(), {}),
}


def gen_cdvd_init_paths(rng, i, n):
    kind = sorted(CD_INIT_PATHS)[i % len(CD_INIT_PATHS)]
    refuse, nobind = CD_INIT_PATHS[kind]
    srv = CdServers(rng)
    debug = (i // len(CD_INIT_PATHS)) % 2 if i < 2 * len(CD_INIT_PATHS) else rng.choice([0, 1])
    mode = 5 if kind.startswith("exit") else rng.choice([0, 1, 2, rng.getrandbits(32)])
    sema = rng.choice([(-1, 0x21), (0x21, -1), (-1, -1), (0x21, 0x22)])     # (s_ncmd_sema, s_scmd_sema)
    pre_init = rng.random() < 0.5

    def run(t):
        plan = dict(armed=False, k=0)

        def dma_ret(tt, k):
            if plan["armed"]:
                j = plan["k"]
                plan["k"] = j + 1
                if j in refuse:
                    return 0
            return 0x101 + k
        iop = FlakyIop(nobind=nobind, servers=srv.servers())
        sif_up(t, iop, Kernel(dma_ret=dma_ret))
        srv.k = 0
        t.w32(t.data("s_cd_debug"), debug)
        if pre_init:
            t.call(CD["sceCdInit"], 1, ret="int", label="sceCdInit (setup)", maxsteps=2000000)
        if kind == "sema re-create":
            # _sceCdSemaInit's quirk: one surviving mutex id and all three are created again
            t.w32(t.data("s_ncmd_sema"), sema[0] & M32)
            t.w32(t.data("s_scmd_sema"), sema[1] & M32)
            t.call(CD["_sceCdSemaInit"], ret="void")
        else:
            plan["armed"] = iop.armed = True
            t.call(CD["sceCdInit"], mode & M32, ret="int", maxsteps=20000000)
        # (s_init_bind and s_poff_sbuf are only ever written: compared as written
        # memory, not looked up here - GCC 14 drops them)
        for nm in ("s_cd_init_busy", "s_poff_bind", "s_init_count", "s_cd_debug", "s_ncmd_sema",
                   "s_scmd_sema", "s_cd_cb_sema", "s_cd_cb_busy", "s_init_sbuf"):
            t.obs(nm, t.r32(t.data(nm)))
    return Case("libcdvd %s (mode 0x%X, debug %d, pre-init %d, flag 0x%X, vers %s%s)" % (
        kind, mode & M32, debug, pre_init, srv.flag, srv.vers,
        ", semas %d/%d" % sema if kind == "sema re-create" else ""), run)


# ================================================================== libpad

class PadServer:
    def __init__(self, rng):
        self.ver = rng.choice([0x400, 0x401, 0x412, 0x300, 0x500])
        self.res = [rng.choice([1, 1, 0, 2, 4, rng.getrandbits(32)]) for _ in range(16)]
        self.k = 0

    def reset(self):
        self.k = 0

    def __call__(self, t, fno, send, rsize, iop):
        self.k += 1
        w = list(send) + [0] * 32
        w = [x or 0 for x in w[:32]]
        cmd = w[0]
        w[3] = self.ver if cmd == 0x12 else self.res[self.k % 16]
        if cmd == 1:
            w[5] = 0x0001D000 + 0x40 * (w[1] * 4 + w[2])          # IOP actuator buffer
        return struct.pack("<32I", *[x & M32 for x in w])[:rsize]


PAD_OPS = ["scePadPortOpen", "scePadPortClose", "scePadGetFrameCount", "scePadRead", "scePadGetState",
           "scePadStateIntToStr", "scePadSetReqState", "scePadGetReqState", "scePadReqIntToStr", "scePadInfoAct",
           "scePadInfoComb", "scePadInfoMode", "scePadSetMainMode", "scePadSetActDirect", "scePadSetActAlign",
           "scePadGetButtonMask", "scePadSetButtonInfo", "scePadInfoPressMode", "scePadEnterPressMode",
           "scePadExitPressMode", "scePadSetVrefParam", "scePadGetPortMax", "scePadGetSlotMax",
           "scePadGetModVersion", "pad_rpc_cmd_0x14", "scePadEnd", "pad_act_dma_transfer"]


def rand_half(rng):
    """one PadDmaHalf as padman would DMA it."""
    h = bytearray(junk(rng, 0x80, 0.3))
    struct.pack_into("<i", h, 0x58, rng.choice([0, 1, 2, 100, -5, rng.getrandbits(31)]))
    struct.pack_into("<I", h, 0x60, rng.choice([0, 2, 6, 0x12, 0x20, 0x21]))   # padman sends <= 0x20
    h[0x64] = rng.choice([0, 1, 2, 3])
    h[0x65] = rng.choice([0x41, 0x73, 0x79, 0xF3, rng.getrandbits(8)])
    h[0x66] = rng.choice([0, 1, 2, 3])
    h[0x68] = rng.choice([0, 1, 2, 4])
    h[0x69] = rng.choice([0, 1, 3])
    h[0x6A] = rng.choice([0, 1, 2, 4])
    h[0x6B] = rng.choice([0, 1, 4])
    h[0x70] = rng.choice([0, 1, 2, 5, 6, 7, rng.getrandbits(8)])
    h[0x71] = rng.choice([0, 1, 2, 3])
    h[0x72] = rng.choice([0, 1, 1])
    return bytes(h)


def gen_pad(rng, i, n):
    srv = PadServer(rng)
    init = rng.random() < 0.85
    ops = []
    for k in range(rng.randrange(1, 5)):
        op = PAD_OPS[(i + 5 * k) % len(PAD_OPS)] if k == 0 else rng.choice(PAD_OPS)
        ops.append((op, rng.randrange(2), rng.randrange(4), rng.choice([0, 1, 2, 3, 4, rng.getrandbits(8)]),
                    rng.choice([-1, 0, 1, 2, 3, 4]), junk(rng, 8), rng.random() < 0.15))
    halves = [[rand_half(rng), rand_half(rng)] for _ in range(8)]
    opened = [rng.random() < 0.5 for _ in range(8)]
    dma_fail = rng.random() < 0.1
    act = [junk(rng, 0x40) for _ in range(8)]

    def run(t):
        iop = Iop(servers={0x80000100: srv, 0x80000101: srv})
        sif_up(t, iop, Kernel(dma_ret=(lambda tt, kk: 0 if dma_fail and kk > 3 else 0x101 + kk)))
        srv.k = 0
        printf_stub(t)
        t.cache_tags(lambda tt, op, a: 0)
        if init:
            t.call(PAD["scePadInit"], 0, ret="int", label="scePadInit (setup)", maxsteps=2000000)
        areas = []
        for k in range(8):                                         # port k >> 2, slot k & 3
            a = t.alloc(0x100, 64)
            areas.append(a)
            if opened[k]:
                t.call(PAD["scePadPortOpen"], k >> 2, k & 3, a, ret="int", label="scePadPortOpen (setup)")
            t.write(a, halves[k][0] + halves[k][1])              # padman's DMA
            t.write(t.data("s_padActDma") + 0x40 * k, act[k])
        for op, port, slot, x, y, data, unaligned in ops:
            k = port * 4 + slot
            if op == "scePadPortOpen":
                t.call(PAD[op], port, slot, areas[k] + (8 if unaligned else 0), ret="int")
            elif op in ("scePadPortClose", "scePadGetFrameCount", "scePadGetState", "scePadGetReqState",
                        "scePadGetButtonMask", "scePadInfoPressMode", "scePadEnterPressMode",
                        "scePadExitPressMode", "pad_act_dma_transfer"):
                t.call(PAD[op], port, slot, ret="void" if op == "pad_act_dma_transfer" else "int")
            elif op == "scePadRead":
                b = t.alloc(0x40, 16)
                t.call(PAD[op], port, slot, b, ret="int")
                t.obs("read", t.read(b, 0x40))
            elif op in ("scePadStateIntToStr", "scePadReqIntToStr"):
                b = t.alloc(0x20, 16)
                st = x & 7 if op == "scePadStateIntToStr" else x & 3
                t.call(PAD[op], st, b, ret="void")
                t.obs("str", t.read(b, 0x20))
            elif op == "scePadSetReqState":
                t.call(PAD[op], port, slot, x, ret="int")
            elif op in ("scePadInfoAct", "scePadInfoComb", "scePadInfoMode"):
                t.call(PAD[op], port, slot, y & M64, x & 3, ret="int")
            elif op == "scePadSetMainMode":
                t.call(PAD[op], port, slot, x & 1, y & 3, ret="int")
            elif op in ("scePadSetActDirect", "scePadSetActAlign", "scePadSetVrefParam"):
                t.call(PAD[op], port, slot, t.put(data, 16), ret="int")
            elif op == "scePadSetButtonInfo":
                t.call(PAD[op], port, slot, x, ret="int")
            elif op in ("scePadGetPortMax", "scePadGetModVersion", "scePadEnd"):
                t.call(PAD[op], ret="int")
            elif op == "scePadGetSlotMax":
                t.call(PAD[op], port, ret="int")
            elif op == "pad_rpc_cmd_0x14":
                t.call(PAD[op], x, ret="int")
        t.obs("pad ctl", Kernel.words(t, t.data("s_padCtl"), 0xE0), "ptrs")
    return Case("libpad %s (init %d, ver 0x%X, dma fail %d)" % (
        " ".join("%s(%d,%d)" % (o[0], o[1], o[2]) for o in ops), init, srv.ver, dma_fail), run)


def gen_pad_edges(rng, i, n):
    """the info getters at the edges of their tables: actuator / list numbers
    below -1 and at or past the counts (the counts up to 0xFF: the code indexes
    4-byte entries from +0x30 / +0x40 of the half with no other bound), every
    term / offs, button masks with the top bits set, and the newest-half pick
    (pad_get_recv_buf) with negative and equal frame counters.  The pad area
    is followed by more input memory, so far entries read defined bytes."""
    srv = PadServer(rng)
    port, slot = rng.randrange(2), rng.randrange(4)
    halves = [bytearray(rand_half(rng)), bytearray(rand_half(rng))]
    frames = rng.choice([(0, 0), (-1, 0), (0, -1), (5, 5), (-0x80000000, 0x7FFFFFFF), (0x7FFFFFFF, -0x80000000),
                         (1, 2), (2, 1)])
    for h, f in zip(halves, frames):
        struct.pack_into("<i", h, 0x58, f)
        h[0x72] = rng.choice([1, 1, 1, 0])
        h[0x64] = rng.choice([2, 3, 2, 1])
        h[0x66] = rng.choice([2, 3, 1])
        h[0x6A] = rng.choice([0, 1, 4, 5, 8, 0x40, 0x7F, 0x80, 0xFF])
        h[0x6B] = rng.choice([0, 1, 4, 5, 8, 0x40, 0x7F, 0x80, 0xFF])
        h[0x79:0x7D] = rng.choice([b"\xFF\xFF\xFF\xFF", b"\x00\x00\x00\x80", b"\xFF\xFF\x03\x00", b"\x80\x80\x80\x80",
                                   bytes(rng.randrange(256) for _ in range(4))])
    after = junk(rng, 0x400)
    calls = []
    for k in range(6):
        op = rng.choice(["scePadInfoAct", "scePadInfoComb", "scePadGetButtonMask", "scePadInfoPressMode",
                         "scePadGetFrameCount", "scePadInfoMode"])
        x = rng.choice([-0x80000000, -9, -5, -2, -1, 0, 1, 3, 4, 5, 7, 8, 0x20, 0x3F, 0x7E, 0xFE, 0xFF, 0x100])
        y = rng.choice([-2, -1, 0, 1, 2, 3, 4, 5])
        calls.append((op, x, y))

    def run(t):
        iop = Iop(servers={0x80000100: srv, 0x80000101: srv})
        sif_up(t, iop)
        srv.k = 0
        printf_stub(t)
        t.cache_tags(lambda tt, op, a: 0)
        t.call(PAD["scePadInit"], 0, ret="int", label="scePadInit (setup)", maxsteps=2000000)
        a = t.alloc(0x500, 64)
        t.call(PAD["scePadPortOpen"], port, slot, a, ret="int", label="scePadPortOpen (setup)")
        t.write(a, bytes(halves[0]) + bytes(halves[1]) + after)
        for op, x, y in calls:
            if op in ("scePadInfoAct", "scePadInfoComb"):
                t.call(PAD[op], port, slot, x & M64, y & M64, ret="int")
            elif op == "scePadInfoMode":
                t.call(PAD[op], port, slot, y & 7, x & M64, ret="int")
            else:
                t.call(PAD[op], port, slot, ret="int")
    return Case("libpad info edges port %d slot %d frames %s: %s" % (
        port, slot, frames, " ".join("%s(%d,%d)" % c for c in calls)), run)


# ================================================================== libmc

class McServer:
    def __init__(self, rng):
        self.vers = (rng.choice([0x20A, 0x20A, 0x209, 0x300]), rng.choice([0x20E, 0x20E, 0x20D, 0x300]))
        self.res = [rng.choice([0, 1, 0, -1, -2, -4, 3, 0x200, rng.getrandbits(32)]) for _ in range(16)]
        self.data = junk(rng, 0x800)
        self.endp = junk(rng, 0xC0)
        self.k = 0

    def reset(self):
        self.k = 0

    def __call__(self, t, fno, send, rsize, iop):
        self.k += 1
        res = self.res[self.k % 16]
        w = [x or 0 for x in (list(send) + [0] * 16)[:16]]
        if fno == 0xFE:
            return struct.pack("<iII", 0, self.vers[0], self.vers[1])[:rsize]
        if fno in (5, 1, 0xC) and w[7]:                              # read / card info / chdir: end parameters
            if fno == 5:
                buf, size = w[6], E32s(w[3])
                cnt = max(0, min(size, 0x400, res if res >= 0 else 0))
                head = min((-buf) & 0x3F, cnt)
                tail = (cnt - head) & 0x3F if cnt > head else 0
                mid = cnt - head - tail
                if mid > 0:
                    t.write(buf + head, self.data[:mid])
                ep = struct.pack("<iiII", head, tail, buf, buf + head + mid) + self.endp[0x10:0xC0]
            else:
                ep = self.endp
            t.write(w[7], ep)
        return struct.pack("<I", res & M32).ljust(max(rsize, 0), b"\0")[:rsize]


MC_OPS = ["sceMcOpen", "sceMcMkdir", "sceMcClose", "sceMcSeek", "sceMcRead", "sceMcWrite", "sceMcSync", "sceMcGetInfo",
          "sceMcGetDir", "sceMcChdir", "sceMcFormat", "sceMcDelete", "sceMcFlush", "sceMcSetFileInfo", "sceMcRename",
          "sceMcUnformat", "sceMcGetEntSpace", "sceMcChangeThreadPriority", "sceMcGetSlotMax", "mc_get_rpc_state",
          "sceMcInit"]


def gen_mc(rng, i, n):
    srv = McServer(rng)
    defer = rng.random() < 0.4
    init = rng.random() < 0.85
    ops = []
    for k in range(rng.randrange(1, 5)):
        op = MC_OPS[(i + 7 * k) % len(MC_OPS)] if k == 0 else rng.choice(MC_OPS)
        ops.append((op, rng.choice([0, 1, 2]), rng.choice([0, 1]), rand_name(rng), rng.choice([0, 1, 2, 3, 0x40,
                                                                                                0x200, 0x8000]),
                    rng.choice([0, 1, 0x10, 0x3F, 0x40, 0x41, 0x100, 0x3FF, rng.randrange(0, 0x400)]),
                    rng.randrange(0, 64), rng.random() < 0.5, junk(rng, 0x40)))

    def run(t):
        iop = Iop(servers={0x80000400: srv}, defer=defer)
        sif_up(t, iop)
        srv.k = 0
        printf_stub(t)
        t.cache_tags(lambda tt, op, a: 0)
        t.w32(t.data("s_mcSema"), 0xFFFFFFFF)
        t.w32(t.data("s_mcFuncNo"), 0)
        if init:
            t.call(MC["sceMcInit"], ret="int", label="sceMcInit (setup)", maxsteps=3000000)
        for op, port, slot, name, mode, size, off, null, data in ops:
            nm = t.cstr(name)
            buf = t.alloc(0x500, 64, off)
            if op == "sceMcOpen":
                t.call(MC[op], port, slot, 0 if null and not name else nm, mode, ret="int")
            elif op in ("sceMcMkdir", "sceMcDelete", "sceMcGetEntSpace"):
                t.call(MC[op], port, slot, nm, ret="int")
            elif op in ("sceMcClose", "sceMcFlush"):
                t.call(MC[op], mode & 7, ret="int")
            elif op == "sceMcSeek":
                t.call(MC[op], mode & 7, size, port, ret="int")
            elif op == "sceMcRead":
                t.call(MC[op], mode & 7, buf, size, ret="int")
            elif op == "sceMcWrite":
                t.write(buf, data)
                t.call(MC[op], mode & 7, buf, size, ret="int")
            elif op == "sceMcSync":
                r = t.alloc(16, 16)
                t.call(MC[op], 0 if null else 1, r, r + 4, ret="int", maxsteps=3000000)
                t.obs("sync out", t.read(r, 8))
            elif op == "sceMcGetInfo":
                r = t.alloc(16, 16)
                t.call(MC[op], port, slot, r, r + 4, 0 if null else r + 8, ret="int")
            elif op == "sceMcGetDir":
                t.call(MC[op], port, slot, nm, mode & 1, size & 0x3F, buf, ret="int")
            elif op == "sceMcChdir":
                t.call(MC[op], port, slot, nm, 0 if null else buf, ret="int")
            elif op in ("sceMcFormat", "sceMcUnformat"):
                t.call(MC[op], port, slot, ret="int")
            elif op == "sceMcSetFileInfo":
                t.call(MC[op], port, slot, nm, t.put(data, 16), mode, ret="int")
            elif op == "sceMcRename":
                t.call(MC[op], port, slot, nm, t.cstr(name[::-1]), ret="int")
            elif op == "sceMcChangeThreadPriority":
                t.call(MC[op], size, ret="int")
            elif op == "sceMcGetSlotMax":
                t.call(MC[op], port, ret="int", maxsteps=3000000)
            elif op == "mc_get_rpc_state":
                r = t.alloc(16, 16)
                t.call(MC[op], r, r + 4, ret="ptr")
                t.obs("rpc state", (t.r32(r), t.r32(r + 4)), "ptrs")
            else:
                t.call(MC[op], ret="int", maxsteps=3000000)
            t.obs("buffer", t.read(buf, 0x500))
        if defer:
            r = t.alloc(16, 16)
            t.call(MC["sceMcSync"], 0, r, r + 4, ret="int", label="sceMcSync (final)", maxsteps=3000000)
        t.obs("mc state", (t.r32(t.data("s_mcFuncNo")), Kernel.words(t, t.data("s_mcRdata"), 16)))
    return Case("libmc %s (defer %d, init %d, versions %s)" % (" ".join(o[0] for o in ops), defer, init, srv.vers),
                run)


def gen_mc_edges(rng, i, n):
    """the corners of sceMcWrite (every buffer alignment around the 0x10 / 0x11
    size limit: head bytes copied into the descriptor, the rest DMAd from the
    next 16-byte boundary), sceMcSync (NULL cmd / result, poll vs wait, nothing
    pending) and sceMcGetInfo (each of the three result pointers NULL or not,
    then the end function through sceMcSync)."""
    srv = McServer(rng)
    srv.vers = (0x20A, 0x20E)                                     # sceMcInit succeeds
    kind = ("write", "sync", "getinfo")[i % 3]
    defer = rng.random() < 0.5
    off = (i // 3) % 16 if i < 48 else rng.randrange(64)
    size = rng.choice([0, 1, 0xF, 0x10, 0x11, 0x12, 0x1F, 0x20, 0x21, 0x2F, 0x30, 0x40, 0x41, rng.randrange(0x400)])
    fd = rng.choice([0, 1, 3, 7])
    data = junk(rng, 0x480)
    mode = rng.choice([0, 1, 2, -1])
    nulls = (i // 3) % 8 if i < 48 else rng.randrange(8)            # bit 0: cmd/type, 1: result/free, 2: format
    pending = rng.choice(["sceMcOpen", "sceMcGetInfo", "sceMcRead", None])

    def run(t):
        iop = Iop(servers={0x80000400: srv}, defer=defer)
        sif_up(t, iop)
        srv.k = 0
        printf_stub(t)
        t.cache_tags(lambda tt, op, a: 0)
        t.w32(t.data("s_mcSema"), 0xFFFFFFFF)
        t.w32(t.data("s_mcFuncNo"), 0)
        t.call(MC["sceMcInit"], ret="int", label="sceMcInit (setup)", maxsteps=3000000)
        buf = t.alloc(0x500, 64, off)
        t.write(buf, data)
        r = t.alloc(16, 16)
        t.write(r, b"\xA5" * 16)
        if kind == "write":
            t.call(MC["sceMcWrite"], fd, buf, size, ret="int")
        elif kind == "sync":
            if pending == "sceMcOpen":
                t.call(MC[pending], 0, 0, t.cstr(b"/BASCUS-97102/GT3"), 1, ret="int")
            elif pending == "sceMcGetInfo":
                t.call(MC[pending], 0, 0, r + 8, 0, 0, ret="int")
            elif pending == "sceMcRead":
                t.call(MC[pending], fd, buf, size, ret="int")
            t.call(MC["sceMcSync"], mode & M32, 0 if nulls & 1 else r, 0 if nulls & 2 else r + 4, ret="int",
                   maxsteps=3000000)
        else:
            t.call(MC["sceMcGetInfo"], fd & 1, fd >> 1, 0 if nulls & 1 else r, 0 if nulls & 2 else r + 4,
                   0 if nulls & 4 else r + 8, ret="int")
        t.call(MC["sceMcSync"], 0, r + 12, 0, ret="int", label="sceMcSync (final)", maxsteps=3000000)
        t.obs("out", t.read(r, 16))
        t.obs("buffer", t.read(buf, 0x500))
        t.obs("mc state", (t.r32(t.data("s_mcFuncNo")), Kernel.words(t, t.data("s_mcRdata"), 16)))
    return Case("libmc %s (buf +%d, size 0x%X, mode %d, nulls %d, pending %s, defer %d)" % (
        kind, off, size, mode, nulls, pending, defer), run)


def gen_execps2(rng, i, n):
    entry = rng.choice([0x00100008, 0x01000008, rng.getrandbits(32)])
    argc = rng.choice([0, 1, 2, 3])

    def run(t):
        Kernel(ret={0x07: rng_ret}).install(t)
        argv = t.put(struct.pack("<4I", t.cstr(b"cdrom0:\\GT3.ELF;1"), t.cstr(b"hot"), 0, 0), 16)
        t.call(EXECPS2, entry, argc, argv, ret="int")
    rng_ret = rng.choice([0, -1, 5])
    return Case("exec_ps2_with_sbus_cleanup(0x%X, %d)" % (entry, argc), run)


SPECS = [
    Spec("cdvd", list(CD.values()), gen_cdvd, 240, "model IOP: cdvd servers, power-off command, deferred replies"),
    Spec("cdvd_init_paths", [CD["sceCdInit"], CD["_sceCdSemaInit"]], gen_cdvd_init_paths, 16,
         "sceCdInit / power-off bind: bind errors, missing servers, refused RPCs; _sceCdSemaInit re-create"),
    Spec("pad", [v for k, v in PAD.items()], gen_pad, 300, "model IOP: padman replies, DMA areas"),
    Spec("pad_edges", [PAD[k] for k in ("scePadInfoAct", "scePadInfoComb", "scePadGetButtonMask",
                                        "scePadInfoPressMode", "scePadGetFrameCount", "scePadInfoMode")],
         gen_pad_edges, 100, "info getters past their tables, button masks, newest-half pick"),
    Spec("mc", list(MC.values()), gen_mc, 300, "model IOP: mcserv replies, end parameters, deferred calls"),
    Spec("mc_edges", [MC["sceMcWrite"], MC["sceMcSync"], MC["sceMcGetInfo"], MC["sceMcInit"]], gen_mc_edges, 144,
         "sceMcWrite alignments x sizes, sceMcSync NULL outputs / modes, sceMcGetInfo NULL combinations"),
    Spec("execps2", [EXECPS2], gen_execps2, 12),
]
