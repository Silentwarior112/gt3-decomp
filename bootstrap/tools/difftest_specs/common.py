"""Shared helpers for the difftest spec modules (`from .common import *`).

Re-exports Spec / Case / HarnessError / ROOT from difftest, the usual standard
modules, random-data generators, and the in-memory model of the bignum classes
(include/bignum.h) that several groups pass to the code under test.
"""
import gzip, hashlib, math, os, random, struct, zlib     # noqa: F401  (re-exported)

from difftest import Spec, Case, HarnessError, ROOT, DEFAULT_N   # noqa: F401
import eeemu as E


def E32s(v):
    """a 32-bit value as a signed int."""
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v >> 31 else v

M32, M64 = 0xFFFFFFFF, (1 << 64) - 1
FRESH_WORD = 0xCDCDCDCD          # what the stub malloc / operator new fill a new block with


def s64(v):
    v &= M64
    return v - (1 << 64) if v >> 63 else v


# ================================================================== random data

def rstr(rng, n):
    """n bytes without NUL: mostly printable, some high bytes."""
    out = bytearray()
    for _ in range(n):
        k = rng.random()
        if k < 0.75:
            out.append(rng.randrange(0x20, 0x7F))
        elif k < 0.9:
            out.append(rng.randrange(0x80, 0x100))
        else:
            out.append(rng.randrange(1, 0x20))
    return bytes(out)


def junk(rng, n, zeros=0.15):
    return bytes(0 if rng.random() < zeros else rng.randrange(1, 256) for _ in range(n))


def pick_len(rng, big=300):
    k = rng.random()
    if k < 0.45:
        return rng.randrange(0, 24)
    if k < 0.75:
        return rng.choice([7, 8, 9, 15, 16, 17, 31, 32, 33, 47, 48, 63, 64, 65])
    return rng.randrange(0, big)


def pick_align(rng):
    """(offset a, offset b) modulo 16, biased toward the fast paths."""
    k = rng.random()
    if k < 0.35:
        return 0, 0
    if k < 0.55:
        return rng.choice([0, 8]), rng.choice([0, 8])
    return rng.randrange(16), rng.randrange(16)


def c_strcmp(a, b):
    for x, y in zip(a + b"\0", b + b"\0"):
        if x != y:
            return x - y
        if x == 0:
            return 0
    return 0


# ================================================================== BigNum / BigNumRep in memory
# BigNumRep (0x18 bytes): +0 unused, +4 refcount, +8 nwords, +0xC capacity,
# +0x10 negative, +0x14 u32 *words (capacity limbs).  BigNum is a handle whose
# word at +4 is the BigNumRep pointer (null = 0).

def limbs(v, n=None):
    out = []
    while v:
        out.append(v & M32)
        v >>= 32
    if n is not None:
        out += [0] * (n - len(out))
    return out


class Rep:
    """a BigNumRep to build in memory: |value|, sign, capacity, refcount and
    what the limbs above nwords hold (garbage, default 0)."""

    def __init__(self, value, neg=0, cap=None, refcount=1, garbage=None, nw=None):
        self.value, self.neg, self.refcount = value, neg, refcount
        self.nw = len(limbs(value)) if nw is None else nw
        if cap is None:
            cap = 0 if self.nw == 0 else 1 << (self.nw - 1).bit_length()
        self.cap = cap
        self.garbage = garbage or []

    def words(self):
        w = limbs(self.value, self.nw)
        extra = list(self.garbage) + [0] * self.cap
        return (w + extra)[:self.cap]

    def put(self, t):
        words = t.halloc(4 * self.cap, struct.pack("<%dI" % self.cap, *self.words())) if self.cap else 0
        return t.halloc(0x18, struct.pack("<IiIIII", 0xDEADBEEF, self.refcount, self.nw, self.cap, self.neg, words))

    def __repr__(self):
        return "%s0x%X/nw%d/cap%d" % ("-" if self.neg else "", self.value, self.nw, self.cap)


def read_rep(t, p):
    """(negative, |value| of words[0..nwords), nwords, capacity, refcount) or None."""
    if p == 0:
        return None
    unused, rc, nw, cap, neg, words = struct.unpack("<IiIIII", t.read(p, 0x18))
    v = int.from_bytes(t.read(words, 4 * nw), "little") if nw and words else 0
    return (neg, v, nw, cap, rc)


def read_handle(t, h):
    return read_rep(t, t.r32(h + 4))


def put_handle(t, rep_addr):
    h = t.alloc(8, 8)
    t.write(h, struct.pack("<II", 0xFEEDF00D, rep_addr))
    return h


def rand_big(rng, maxlimbs=12):
    k = rng.random()
    if k < 0.08:
        return 0
    if k < 0.25:
        nl = rng.randrange(1, 3)
    elif k < 0.8:
        nl = rng.randrange(1, max(2, maxlimbs // 2))
    else:
        nl = rng.randrange(1, maxlimbs + 1)
    v = rng.getrandbits(32 * nl)
    style = rng.random()
    if style < 0.1:
        v = (1 << (32 * nl)) - 1                                   # all ones
    elif style < 0.2:
        v = 1 << rng.randrange(32 * nl)                            # power of two
    elif style < 0.3:
        v |= 1 << (32 * nl - 1)                                    # top bit set
    elif style < 0.4:
        v &= ~((1 << (32 * max(0, nl - 1))) - 1)                   # low limbs zero
    return v


def rand_rep(rng, maxlimbs=12, allow_neg=True, value=None):
    v = rand_big(rng, maxlimbs) if value is None else value
    neg = 1 if allow_neg and rng.random() < 0.3 else 0
    nw = len(limbs(v))
    k = rng.random()
    if nw == 0:
        cap = rng.choice([0, 0, 1, 2, 4])
    else:
        cap = 1 << (nw - 1).bit_length()
        if k < 0.25:
            cap *= rng.choice([2, 4])
    gb = [rng.getrandbits(32) for _ in range(cap)] if rng.random() < 0.4 else None
    if neg and nw == 0 and rng.random() < 0.5:
        neg = 0
    return Rep(v, neg, cap, rng.choice([1, 1, 1, 2]), gb)


def rand_handle_rep(rng, maxlimbs, pos=True):
    """None (null rep) or a Rep; shared reps get refcount 2."""
    if rng.random() < 0.1:
        return None
    return rand_rep(rng, maxlimbs, not pos)


def rand_odd_modulus(rng, bits):
    return rng.getrandbits(bits) | (1 << (bits - 1)) | 1


def sval(r):
    return -r.value if r.neg else r.value


# ================================================================== C++ object layouts (T.obj, T.m, T.vtable)

# (GNU v2 offset of the vptr = the members before it, the Itanium offset of those
# members (vptr at +0), sizeof).  GCC 2.9x places the vptr after the data members
# of the class that introduces virtual functions; a derived class's members keep
# their offsets in both ABIs.  On a GNU v2 image (retail, SCE) every T.m / T.objwrite
# / T.objread is the identity; against the GCC 14 build they rotate the offsets.
LAYOUT = dict(
    FileName=(0x08, 4, 0x0C),          # FileName_Base + subclasses: name +0, path +4 (include/filename.h)
    Source=(0x0C, 4, 0x10),            # ImageLoader::Source, Host/DiskSource: buffer +0, fileName +8
    MCardSource=(0x0C, 4, 0x18),       # + port +0x10, slot +0x14
    Allocator=(0x14, 4, 0x18),         # pdistd::Allocator: m_first .. m_firstFit +0x10
    InflateBase=(0x3030, 8, 0x3038),   # GZIP::InflateBase: arena, state (u64 bitbuf: 8-aligned)
    PrintTarget=(0x00, 0, 0x10),       # vptr at +0 in both (no members before it)
)


# ================================================================== a fake EE kernel (t.syscalls)

# EE kernel call numbers ($v1 of the syscall stubs, include/eekernel.h)
SYS = dict(SetGsCrt=0x02, AddIntcHandler=0x10, RemoveIntcHandler=0x11, AddDmacHandler=0x12,
           RemoveDmacHandler=0x13, EnableIntc=0x14, DisableIntc=0x15, EnableDmac=0x16, DisableDmac=0x17,
           iEnableIntc=-0x1A, iDisableIntc=-0x1B, iEnableDmac=-0x1C, iDisableDmac=-0x1D,
           CreateThread=0x20, DeleteThread=0x21, StartThread=0x22, ExitThread=0x23, ExitDeleteThread=0x24,
           TerminateThread=0x25, ChangeThreadPriority=0x29, iGetThreadId=-0x2F, iChangeThreadPriority=-0x2A,
           RotateThreadReadyQueue=0x2B, iRotateThreadReadyQueue=-0x2C, GetThreadId=0x2F,
           ReferThreadStatus=0x30, SleepThread=0x32, WakeupThread=0x33, iWakeupThread=-0x34,
           CancelWakeupThread=0x35, iCancelWakeupThread=-0x36, SuspendThread=0x37, iSuspendThread=-0x38,
           ResumeThread=0x39, CreateSema=0x40, DeleteSema=0x41, SignalSema=0x42, iSignalSema=-0x43,
           WaitSema=0x44, PollSema=0x45, iPollSema=-0x46, ReferSemaStatus=0x47, FlushCache=0x64,
           iFlushCache=-0x68, GsGetIMR=0x70, GsPutIMR=0x71, SetSyscall=0x74, SifDmaStat=0x76,
           iSifDmaStat=-0x76, SifSetDma=0x77, iSifSetDma=-0x77, SifSetDChain=0x78, iSifSetDChain=-0x78,
           SifSetReg=0x79, SifGetReg=0x7A, Deci2Call=0x7C, MachineType=0x7E, GetMemorySize=0x7F)


# how many argument registers each kernel call takes (only those are compared;
# the others hold whatever the caller left there).  Default 4.
SYS_ARGC = {0x01: 1, 0x02: 3, 0x04: 1, 0x06: 3, 0x07: 4, 0x0A: 2, 0x0B: 1, 0x0C: 1, 0x0D: 2, 0x0E: 2, 0x0F: 2,
            0x10: 3, 0x11: 2, 0x12: 3, 0x13: 2, 0x14: 1, 0x15: 1, 0x16: 1, 0x17: 1, -0x1A: 1, -0x1B: 1, -0x1C: 1,
            -0x1D: 1, 0xFC: 3, 0xFD: 1, -0xFE: 3, -0xFF: 1, 0x20: 1, 0x21: 1, 0x22: 2, 0x23: 0, 0x24: 0, 0x25: 1,
            -0x26: 1, 0x27: 0, 0x28: 0, 0x29: 2, -0x2A: 2, 0x2B: 1, -0x2C: 1, 0x2D: 1, -0x2E: 1, 0x2F: 0, -0x2F: 0, 0x30: 2,
            -0x31: 2, 0x32: 0, 0x33: 1, -0x34: 1, 0x35: 1, -0x36: 1, 0x37: 1, -0x38: 1, 0x39: 1, -0x3A: 1,
            0x3C: 5, 0x3D: 2, 0x3E: 0, 0x40: 1, 0x41: 1, 0x42: 1, -0x43: 1, 0x44: 1, 0x45: 1, -0x46: 1, 0x47: 2,
            -0x48: 2, 0x4A: 1, 0x4B: 1, 0x5A: 3, 0x5B: 1, 0x5C: 1, -0x5C: 1, 0x5D: 1, -0x5D: 1, 0x5E: 1, -0x5E: 1,
            0x5F: 1, -0x5F: 1, 0x60: 1, 0x61: 1, 0x62: 1, 0x63: 1, 0x64: 1, 0x66: 1, -0x67: 1, -0x68: 1, -0x6A: 1,
            0x6B: 0, 0x6C: 1, 0x6D: 1, 0x6E: 3, 0x6F: 3, 0x70: 0, -0x70: 0, 0x71: 1, -0x71: 1, 0x72: 1, 0x73: 2,
            0x74: 2, 0x76: 1, -0x76: 1, 0x77: 2, -0x77: 2, 0x78: 0, -0x78: 0, 0x79: 2, 0x7A: 1, 0x7B: 2, 0x7C: 2,
            0x7D: 0, 0x7E: 0, 0x7F: 0}
# stubs of the same call number with one more argument
SYS_ARGC_STUB = {0x0101B8D0: 4, 0x0101B900: 4}        # AddIntcHandler2, AddDmacHandler2


def syscall_argc(t, num, pc):
    """t.syscalls nargs: the argument count of kernel call num made at pc."""
    for stub, n in SYS_ARGC_STUB.items():
        try:
            if t.fn(stub) + 4 == pc:
                return n
        except HarnessError:
            pass
    return SYS_ARGC.get(num, 4)


class Kernel:
    """A deterministic fake EE kernel for t.syscalls: the harness logs every call
    (number, $a0..$t3, $v0) and compares the logs; this decides the results and
    records what the kernel would read through pointer arguments (t.obs, with
    never-written stack bytes as None - see t.peek):

      ids          CreateSema / CreateThread / Add*Handler / SifSetDma return
                   increasing ids (per case); the Sema calls return their id
      tid          GetThreadId, SleepThread
      sleep        fn(t, k) run at the k-th SleepThread (0-based): what interrupt
                   handlers would do meanwhile (clear a busy flag, count a vblank)
      intr         result of (_)(i)Enable/Disable Intc/Dmac (1 = state changed)
      dma_stat     sceSifDmaStat result (fn(t, id) or int; default -1 = finished)
      sif_regs     sceSifGetReg values {reg: int, or a list read in turn (the last
                   repeats)}; sceSifSetReg stores an int, except SMFLAG (4), where
                   the written bits are cleared (acknowledged)
      on_getreg    fn(t, reg, state) before each sceSifGetReg (the IOP's progress)
      dma_ret      sceSifSetDma result (int, or fn(t, k) for the k-th call; default
                   increasing ids, never 0)
      on_dma       fn(t, k, items) after the k-th sceSifSetDma: what the IOP does
                   with the transfer (answer a command, set a SIF register ...)
      on_block     fn(t, sema) when a WaitSema would block: deliver what would wake
                   it (e.g. Iop.deliver); still zero afterwards = a deadlock, an
                   EmuError 'blocked' (both sides alike)
      on_sleep     fn(t) at every SleepThread, before `sleep` (e.g. Iop.deliver)
    Alarms: SetAlarm / iSetAlarm record (handler, arg) and return an id,
    Release(i)Alarm drops it; pending alarms fire (t.interrupt(handler, id,
    time, arg)) when the thread sleeps or would block in WaitSema - time passes
    only while the thread waits.
      ret          {number: int or fn(t, args, state)} overrides anything above
    Semaphores this kernel created count like the EE kernel's (initCount from the
    parameter block, Signal / iSignal +1 up to maxCount (else -420), Wait -1,
    Poll -1 or -419, ReferSemaStatus fills the block); a sema id it did not create
    (e.g. a random id in a test's state) is accepted by every call.
    CreateSema / CreateThread record the parameter block, sceSifSetDma the
    transfer list and the words each element sends (that is what the IOP gets),
    as 'ptrs' observations: EE addresses inside them compare like pointers."""

    def __init__(self, tid=0x11, sleep=None, intr=1, dma_stat=-1, sif_regs=None, ret=None, max_sleeps=100000,
                 dma_ret=None, on_dma=None, on_block=None, on_sleep=None, on_getreg=None):
        self.tid, self.sleep, self.intr, self.dma_stat = tid, sleep, intr, dma_stat
        self.sif_regs = dict(sif_regs or {})
        self.ret = dict(ret or {})
        self.max_sleeps = max_sleeps
        self.dma_ret, self.on_dma, self.on_block, self.on_sleep = dma_ret, on_dma, on_block, on_sleep
        self.on_getreg = on_getreg
        # every caller of sceOpen in the bootstrap passes no `mode`: the mode
        # word sceOpen reads with va_arg (+0x10 of the 0x418-byte FIO_F_OPEN
        # argument) is whatever the caller left in $a2 - indeterminate, and
        # different in a build whose caller differs (set by the specs that run
        # those callers; not compared then)
        self.open_mode_unknown = False

    def install(self, t):
        st = dict(id=0x20, sleeps=0, imr=0xFF00, regs=dict(self.sif_regs), dma=0x100, ndma=0, nreads={}, semas={},
                  alarms={}, alarm_id=0x40)
        self.st = st
        t.syscalls(lambda tt, num, args: self.call(tt, num, args, st), nargs=syscall_argc)
        return st

    def fire_alarms(self, t, st):
        """run every pending alarm handler (the alarm expires while the thread waits)."""
        for aid in sorted(st["alarms"]):
            handler, arg, time = st["alarms"].pop(aid)
            t.interrupt(handler, aid, time, arg, raw=True, label="alarm %d handler" % aid)

    @staticmethod
    def words(t, addr, size):
        """what a transfer of `size` bytes at addr sends: 32-bit words (None for a
        never-written stack word) and the trailing bytes."""
        n = size // 4
        out = list(t.peek32(addr, n)) if n else []
        if size % 4:
            out.append(t.peek(addr + 4 * n, size % 4))
        return tuple(out)

    def call(self, t, num, a, st):
        r = self.ret.get(num)
        if r is not None:
            return r(t, a, st) if callable(r) else r
        if num in (0x10, 0x12, 0x20, 0x40):                          # new handler / thread / sema id
            st["id"] += 1
            # only the fields the kernel takes in are compared: the others are
            # often stale stack (Sony's code leaves them unset)
            if num == 0x40:
                p = t.peek32(a[0], 6)
                t.obs("CreateSema max/init", (p[1], p[2]))
                st["semas"][st["id"]] = [p[2] or 0, p[1], p[2], p[4], p[5]]     # count max init attr option
            elif num == 0x20:
                t.obs("CreateThread entry/stack/size/gp/prio", t.peek32(a[0] + 4, 5), "ptrs")
            return st["id"]
        if num in (0x14, 0x15, 0x16, 0x17, -0x1A, -0x1B, -0x1C, -0x1D):
            return self.intr
        if num in (0x2F, -0x2F):                                     # GetThreadId / _iGetThreadId
            return self.tid
        if num == 0x32:                                              # SleepThread
            k = st["sleeps"]
            st["sleeps"] = k + 1
            if k >= self.max_sleeps:
                raise HarnessError("fake kernel: more than %d SleepThread calls" % self.max_sleeps)
            if self.on_sleep is not None:
                self.on_sleep(t)
            self.fire_alarms(t, st)
            if self.sleep is not None:
                self.sleep(t, k)
            return self.tid
        if num in (0xFC, -0xFE):                                     # (i)SetAlarm(time, handler, arg)
            st["alarm_id"] += 1
            st["alarms"][st["alarm_id"]] = (a[1], a[2], a[0] & 0xFFFF)
            return st["alarm_id"]
        if num in (0xFD, -0xFF):                                     # (i)ReleaseAlarm(id)
            return 0 if st["alarms"].pop(a[0], None) is not None else 0xFFFFFFFF
        if num in (0x41, 0x42, -0x43, 0x44, 0x45, -0x46, 0x47):
            sem = st["semas"].get(E32s(a[0]))
            if sem is None:
                return a[0]                                          # not ours: accept
            if num == 0x41:
                del st["semas"][E32s(a[0])]
            elif num in (0x42, -0x43):
                if sem[1] is not None and sem[0] >= E32s(sem[1]):
                    return -420                                      # KE_SEMA_OVF
                sem[0] += 1
            elif num == 0x44:
                if sem[0] <= 0 and self.on_block is not None:
                    self.on_block(t, a[0])
                if sem[0] <= 0:
                    self.fire_alarms(t, st)
                if sem[0] <= 0:
                    raise E.EmuError("fake kernel: WaitSema(%d) blocks forever" % a[0], "blocked")
                sem[0] -= 1
            elif num in (0x45, -0x46):
                if sem[0] <= 0:
                    return -419                                      # KE_SEMA_ZERO
                sem[0] -= 1
            else:
                t.write(a[1], struct.pack("<6I", *[(x or 0) & M32 for x in (sem[0], sem[1], sem[2], 0, sem[3],
                                                                                 sem[4])]))
            return a[0]
        if num == 0x70:
            return st["imr"]
        if num == 0x71:
            old, st["imr"] = st["imr"], a[0]
            return old
        if num in (0x76, -0x76):
            d = self.dma_stat
            return d(t, a[0]) if callable(d) else d
        if num in (0x77, -0x77):                                     # sceSifSetDma(sdd, n)
            items = []
            for k in range(min(a[1], 32)):
                src, dst, size, mode = t.peek32(a[0] + 16 * k, 4)
                data = None
                if src is not None and size is not None and size <= 0x10000:
                    try:
                        data = self.words(t, src, size)
                    except Exception:
                        data = "unmapped"
                if self.open_mode_unknown and size == 0x418 and isinstance(data, tuple) and len(data) > 4:
                    data = data[:4] + (None,) + data[5:]
                items.append((dst, size, mode, data))
            t.obs("sceSifSetDma", items, "ptrs")
            k = st["ndma"]
            st["ndma"] = k + 1
            if self.dma_ret is not None:
                r = self.dma_ret(t, k) if callable(self.dma_ret) else self.dma_ret
            else:
                st["dma"] += 1
                r = st["dma"]
            if r and self.on_dma is not None:                        # a refused transfer reaches nobody
                self.on_dma(t, k, items)
            return r
        if num == 0x79:
            old = st["regs"].get(a[0], 0)
            if a[0] == 4 and isinstance(old, int):
                st["regs"][4] = old & ~a[1]                          # SMFLAG: the EE writes 1s to acknowledge
            else:
                st["regs"][a[0]] = a[1]
            return old if isinstance(old, int) else 0
        if num == 0x7A:
            if self.on_getreg is not None:
                self.on_getreg(t, a[0], st)
            v = st["regs"].get(a[0], 0)
            if isinstance(v, (list, tuple)):
                n = st["nreads"].get(a[0], 0)
                st["nreads"][a[0]] = n + 1
                return v[min(n, len(v) - 1)]
            return v
        if num == 0x7F:
            return 0x2000000
        return 0


# ================================================================== printf as a stub

PDISTD_PRINTF = 0x01011678                # pdistd printf (printf unit), called by the SDK libraries


def c_format(t, fmt, args):
    """a C printf of fmt with 32-bit register arguments (enough for the
    libraries' debug messages: d i u x X o c s p, flags, width, precision, l/h)."""
    import re as _re
    out, k = [], 0
    for m in _re.finditer(rb"%([-+ #0]*)(\d*)(?:\.(\d+))?([lh]*)([diuxXocsp%])|[^%]+|%", fmt):
        tok = m.group(0)
        if not tok.startswith(b"%") or m.group(5) is None:
            out.append(tok)
            continue
        conv = m.group(5)
        if conv == b"%":
            out.append(b"%")
            continue
        v = args[k] if k < len(args) else 0
        k += 1
        if conv == b"s":
            try:
                s = t.cstr_at(v)
            except Exception:
                s = b"<bad %08X>" % v
            out.append((b"%" + m.group(1) + m.group(2) + b"s") % s if m.group(2) else s)
            continue
        if conv == b"c":
            out.append(bytes([v & 0xFF]))
            continue
        if conv in (b"d", b"i"):
            v = E32s(v)
        pyconv = {b"u": b"d", b"p": b"x", b"i": b"d"}.get(conv, conv)
        spec = b"%" + m.group(1) + m.group(2) + ((b"." + m.group(3)) if m.group(3) else b"") + pyconv
        out.append(spec % v)
    return b"".join(out)


def printf_stub(t, func=PDISTD_PRINTF, name="printf"):
    """stub a printf-like callee: the formatted text is compared (t.obs), not
    the argument registers it does not use."""
    def f(tt, a):
        try:
            fmt = tt.cstr_at(a[0])
        except Exception:
            fmt = b"<bad format %08X>" % a[0]
        tt.obs(name, c_format(tt, fmt, a[1:]))
        return 0
    t.stub(func, f, 8, name, log=0)


# ================================================================== a model IOP (SIF commands / RPC)

SIF_CMD_INTR_HANDLER = 0x0101D878         # _sif_cmd_intr_handler (kernl_sys): the SIF0 interrupt
IOP_SERVE_BASE, IOP_BUFF_BASE = 0x00080000, 0x00090000


def pattern_bytes(n, seed):
    """n deterministic bytes (a server's default reply)."""
    return bytes(((seed * 0x9E3779B1 + k * 0x45D9F3B) >> 13) & 0xFF for k in range(n))


class Iop:
    """The IOP side of the SIF command layer, for code that talks to IOP RPC
    servers (fileio, loadfile, libcdvd, libpad, libmc ...).  It sits on a Kernel
    (its on_dma and on_block): every command packet the EE sends with
    sceSifSetDma is decoded, and the IOP's answers are delivered the way the
    hardware delivers them - the packet is written into the EE's SIF0 receive
    buffer (_sif_cmd_data.pktbuf) and the EE's own SIF0 interrupt handler
    _sif_cmd_intr_handler runs (t.interrupt), right after the DMA that caused
    it (the earliest moment the real IOP could answer):

      INIT_CMD opt 1 (sceSifInitRpc)  SET_SREG(0, 1): the RPC-ready flag
      RESET_CMD                       the IOP reboots: two SMFLAG polls later
                                      SIFINIT | CMDINIT | BOOTEND are set
      RPC_BIND sid                    RPC_END {cid BIND, serve, buff, cbuff}: every
                                      sid in `servers` is bound (others too unless
                                      bind_unknown is False: serve NULL)
      RPC_CALL fno                    the server's reply bytes are written to the
                                      EE receive buffer (rsize bytes, as the IOP's
                                      DMA does); rmode 1: RPC_END {cid CALL};
                                      rmode 0: the packet's in-use bit is cleared
      anything else                   servers_cmd.get(fcode)(t, words, iop) if any

    servers: {sid: fn(t, fno, send_words, rsize, iop) -> bytes or None}; the
    default reply is pattern_bytes.  A server object (or the object of a bound
    method) with a reset() method is reset at install, i.e. once per side.  send_words = what the EE DMAd to the server
    buffer (Kernel.words).  A server can queue extra packets with iop.send(fcode,
    body words) (e.g. FILEIO's completion command).  The IOP also logs nothing
    itself: the EE side's sceSifSetDma, handler calls and memory are compared."""

    def __init__(self, servers=None, servers_cmd=None, bind_unknown=True, defer=False):
        self.servers = dict(servers or {})
        self.servers_cmd = dict(servers_cmd or {})
        self.bind_unknown = bind_unknown
        self.defer = defer

    def install(self, t, kernel=None):
        k = kernel or Kernel()
        for reg, v in ((0x80000000, 0), (4, 0x70000), (2, 0x1E800)):      # SUBADDR, SMFLAG, SMCOM
            k.sif_regs.setdefault(reg, v)
        self.t = t
        for srv in list(self.servers.values()) + list(self.servers_cmd.values()):
            r = getattr(srv, "reset", None) or getattr(getattr(srv, "__self__", None), "reset", None)
            if callable(r):
                r()                                                   # per-side server state (call counters)
        self.queue = []
        self.deferred = []
        self.delivering = False
        self.sids = {}
        self.calls = 0
        k.on_dma = lambda tt, n, items: self.on_dma(tt, items)
        k.on_block = lambda tt, sema: self.deliver(tt, True)
        k.on_sleep = lambda tt: self.deliver(tt, True)
        k.on_getreg = self.on_getreg
        self.rebooting = None
        k.install(t)
        return k

    def serve_of(self, sid):
        if sid not in self.sids:
            self.sids[sid] = len(self.sids)
        return IOP_SERVE_BASE + 0x100 * self.sids[sid], IOP_BUFF_BASE + 0x1000 * self.sids[sid]

    def sid_of(self, serve):
        for sid, k in self.sids.items():
            if IOP_SERVE_BASE + 0x100 * k == serve:
                return sid
        return None

    def send(self, fcode, body, opt=0):
        """queue an IOP -> EE command packet (header + body words)."""
        psize = 0x10 + 4 * len(body)
        hdr = struct.pack("<IIII", psize & 0xFF, 0, fcode, opt)
        self.queue.append(hdr + struct.pack("<%dI" % len(body), *[(w or 0) & M32 for w in body]))

    def on_dma(self, t, items):
        dst, size, mode, words = items[-1]
        if isinstance(words, tuple) and len(words) >= 4 and None not in words[:4]:
            fcode = words[2]
            extra = items[0][3] if len(items) > 1 else ()
            w = list(words) + [0] * 16
            if fcode == 0x80000002 and words[3] == 1:                 # INIT_CMD from sceSifInitRpc
                self.send(0x80000001, [0, 1])
            elif fcode == 0x80000003:                                 # RESET_CMD: the IOP reboots
                self.rebooting = 0
            elif fcode == 0x80000009:                                 # RPC_BIND
                sid = w[8]
                if sid in self.servers or self.bind_unknown:
                    serve, buff = self.serve_of(sid)
                else:
                    serve = buff = 0
                self.send(0x80000008, [w[4], w[5], w[6], w[7], 0x80000009, serve, buff, 0])
            elif fcode == 0x8000000A:                                 # RPC_CALL
                def complete(t, w=w, extra=extra):
                    rec_id, pkt_addr, rpc_id, client, fno, ssize, receive, rsize, rmode, serve = w[4:14]
                    sid = self.sid_of(serve)
                    fn = self.servers.get(sid)
                    self.calls += 1
                    rsz = E32s(rsize)
                    reply = fn(t, fno, extra, rsz, self) if fn else None
                    if reply is None and rsz > 0:
                        reply = pattern_bytes(rsz, (sid or 0) ^ (fno << 8) ^ self.calls)
                    if reply and receive and rsz > 0:
                        t.write(receive, bytes(reply)[:rsz])
                    if rmode:
                        self.send(0x80000008, [rec_id, pkt_addr, rpc_id, client, 0x8000000A, 0, 0, 0])
                    else:
                        # no END: the server DMAs its reply packet {rec_id 0, rpc_id 0,
                        # client, cid CALL} over the client's packet (sceSifExecRequest's
                        # rmode-0 path is the EE twin), which frees it
                        t.write(pkt_addr + 0x10, struct.pack("<IIIII", 0, pkt_addr, 0, client, 0x8000000A))
                if self.defer:
                    self.deferred.append(complete)                    # answered when the EE waits
                else:
                    complete(t)
            else:
                h = self.servers_cmd.get(fcode)
                if h is not None:
                    h(t, w, self)
        self.deliver(t)

    def on_getreg(self, t, reg, st):
        """after a RESET_CMD the IOP comes back two SMFLAG polls later:
        SIFINIT | CMDINIT | BOOTEND set, a new command buffer address."""
        if reg == 4 and self.rebooting is not None:
            self.rebooting += 1
            if self.rebooting >= 2:
                st["regs"][4] = (st["regs"].get(4, 0) if isinstance(st["regs"].get(4, 0), int) else 0) | 0x70000
                st["regs"][2] = 0x1E800
                self.rebooting = None

    def deliver(self, t, waiting=False):
        """run the SIF0 interrupt for every queued packet (and, when the EE
        waits, first complete the deferred calls)."""
        if self.delivering:
            return
        self.delivering = True
        try:
            if waiting:
                while self.deferred:
                    self.deferred.pop(0)(t)
            while self.queue:
                pkt = self.queue.pop(0)
                buf = t.r32(t.data("_sif_cmd_data"))                  # _sif_cmd_data.pktbuf
                if not buf:
                    t.obs("IOP packet with no EE receive buffer", pkt.hex())
                    continue
                t.write(buf, pkt)
                t.interrupt(SIF_CMD_INTR_HANDLER, 5, label="SIF0 interrupt (fcode 0x%08X)" %
                            struct.unpack_from("<I", pkt, 8)[0])
        finally:
            self.delivering = False


def sif_up(t, iop, kernel=None):
    """bring the EE side of SIF up as the game does (sceSifInitRpc with a model
    IOP answering); returns the Kernel."""
    k = iop.install(t, kernel)
    t.w32(t.data("_sif_rpc_inited"), 0)
    t.w32(t.data("_sif_cmd_inited"), 0)
    t.call(0x0101DA50, 0, ret="void", label="sceSifInitRpc (setup)", maxsteps=300000)
    return k


# ================================================================== EE kernel syscall stubs

_STUBS = None


def retail_syscall_stubs(units):
    """[(retail address, kernel call number)] of the 16-byte syscall stubs
    (addiu $v1, $zero, N; syscall; jr $ra; nop) in the retail .text of `units`,
    found in the retail file itself (single PT_LOAD: file offset = va - 0x01000000
    + 0x1000)."""
    global _STUBS
    if _STUBS is None:
        import json
        from difftest import RETAIL_PATH
        d = open(RETAIL_PATH, "rb").read()
        ranges = [(u, int(lo, 16), int(hi, 16)) for u, s in
                  json.load(open(os.path.join(ROOT, "ref", "units.json"))).items() for lo, hi in s["ranges"]]
        _STUBS = []
        for va in range(0x01000000, 0x01026C00, 4):
            o = va - 0x01000000 + 0x1000
            w = struct.unpack_from("<4I", d, o)
            if w[0] >> 16 == 0x2403 and w[1] == 0x0000000C and w[2] == 0x03E00008 and w[3] == 0:
                unit = next((u for u, lo, hi in ranges if lo <= va < hi), "?")
                num = w[0] & 0xFFFF
                _STUBS.append((va, num - 0x10000 if num & 0x8000 else num, unit))
    return [(a, num) for a, num, u in _STUBS if u in units]


def syscall_stub_spec(name, units, per_stub=2):
    """a Spec calling every syscall stub of `units` (case i -> stub i mod count)
    with random $a0..$t3 under a syscall hook: the kernel call number, the eight
    argument registers and the returned $v0 are compared."""
    stubs = retail_syscall_stubs(units)
    if not stubs:
        return None

    def gen(rng, i, n):
        addr, num = stubs[i % len(stubs)]
        args = [rng.choice([0, 1, M32, rng.getrandbits(32), rng.getrandbits(64)]) for _ in range(8)]
        ret = rng.choice([0, 1, -1, rng.getrandbits(31), -rng.getrandbits(31)])

        def run(t):
            t.syscalls(lambda tt, k, a: ret, nargs=8)
            t.call(addr, *args, ret="int")
        return Case("stub 0x%08X (kernel call %d), $v0 = %d" % (addr, num, ret), run)
    return Spec(name, [a for a, _ in stubs], gen, per_stub * len(stubs),
                "%d stubs; syscall hook logs number + $a0..$t3" % len(stubs))
