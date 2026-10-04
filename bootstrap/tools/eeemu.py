"""eeemu.py - EE (R5900) interpreter for tools/difftest.py.

Copied from docs/gearbox/harness/PDIFF_emu.py (the gearbox plugin's differential
harness), which keeps:
  * the EE float model: round toward zero on every operation, denormal inputs read
    as 0, results below the smallest normal flush to (signed) zero, no Inf/NaN
    (exponent 255 is an ordinary number; overflow clamps to +-FMAX, x/0 -> +-FMAX),
    c.eq/c.lt/c.le compare values.  The float ops are exact integer arithmetic;
  * the base MIPS III / R5900 integer ISA, a decode cache and a write journal.

Changes for the bootstrap (see tools/eeemu_selftest.py for the checks):
  * GPRs are 128 bits: g[r] holds bits 63..0, gh[r] bits 127..64.  The ordinary
    (64-bit) instructions write g[] only and leave gh[] unchanged; LQ/SQ and the
    128-bit MMI instructions use both.  HI/LO are 128 bits as on the EE: hi/lo are
    bits 63..0 (pipeline 0: MULT/DIV/MADD...), hi1/lo1 bits 127..64 (pipeline 1:
    MULT1/DIV1/MADD1...); the parallel MMI multiplies and PMFHI/PMTHI see all 128.
  * the whole MMI instruction set: MADD/MADDU/MADD1/MADDU1, PLZCW, MFHI1/MTHI1/
    MFLO1/MTLO1, MULT1/MULTU1/DIV1/DIVU1, PMFHL.{LW,UW,SLW,LH,SH}, PMTHL.LW,
    PSLLH/PSRLH/PSRAH/PSLLW/PSRLW/PSRAW, and every MMI0/MMI1/MMI2/MMI3 sub-op;
  * the SA register: MFSA/MTSA (SPECIAL), MTSAB/MTSAH (REGIMM) and QFSRV.  SA is
    kept as a byte count 0..15 (MTSAB: rs^imm low 4 bits, MTSAH: 2 * (low 3 bits)),
    QFSRV shifts rs:rt right by 8*SA bits.  MTSA/MFSA move that value unchanged;
  * the trap-immediate REGIMM instructions (TGEI..TNEI); DADD/DADDI/DSUB raise on
    signed 64-bit overflow like ADD/ADDI/SUB do on 32-bit overflow;
  * LWR only sign-extends into bits 63..32 when it loads the whole word (k == 0);
    otherwise bits 63..32 are unchanged (as on the R5900 / PCSX2);
  * DIV/DIVU/DIV1/DIVU1/PDIVW/PDIVUW/PDIVBW by zero raise EmuError: the EE leaves
    the result undefined, and a differential test must not compare it;
  * Mem loads any ELF32 (splat's .elf_header/.elf_trailer pseudo segments are
    skipped), records every written word (journal) and resets from it;
  * the EE address map (Mem): RAM with its kseg0/kseg1/uncached/uncached-
    accelerated mirrors, the 16 KB scratchpad at 0x70000000, and MMIO windows (EE
    registers, VU memory, GS privileged registers, BIOS ROM) whose loads and stores
    go to an IoModel: ordered hardware events answered by a register model, with a
    snapshot of every DMA transfer started (Dn_CHCR.STR);
  * COP0: MFC0/MTC0 (incl. MFPS/MTPS/MFPC/MTPC), DI, EI, ERET, SYNC.L/SYNC.P and
    CACHE as IoModel state + events (Count advances per read, never per instruction);
  * COP2 / VU0 macro mode: QMFC2/QMTC2/CFC2/CTC2, LQC2/SQC2 and the vector
    instructions (vu0_macro) on a VU0 register file in the IoModel;
  * hooks: a hook is called when execution reaches its pc outside a delay slot.
    If it returns True it has emulated the function and execution continues at
    $ra (a stub); otherwise the instruction at pc runs normally (a probe).
    CPU.syscall_hook, when set, emulates SYSCALL (the kernel call number is in $v1);
Anything else (TLB / BC0x / BC2x, VCLIP and the VU random-number ops, RSQRT.S,
BREAK, SYSCALL without a hook, reserved encodings) raises EmuError, so a test never
silently runs past an instruction it does not model.
"""
import math
import struct

M64 = (1 << 64) - 1
M128 = (1 << 128) - 1
M32 = 0xFFFFFFFF
FMAX = 0x7F7FFFFF
MEMSIZE = 0x2000000             # 32 MB of EE main RAM
SPR_BASE = 0x70000000           # scratchpad RAM (16 KB, virtual address)
SPR_SIZE = 0x4000
SPR_OFF = MEMSIZE               # where Mem.m keeps the scratchpad bytes
# physical hardware windows: loads/stores there are MMIO events (IoModel), not memory
MMIO_WINDOWS = (
    (0x10000000, 0x10010000, 'EE registers'),
    (0x11000000, 0x11010000, 'VU memory'),
    (0x12000000, 0x12002000, 'GS privileged registers'),
    (0x1FC00000, 0x20000000, 'BIOS ROM'),
)

# ------------------------------------------------------------------ EE float model (exact ints)
# (unchanged from PDIFF_emu.py)


def _dec(b):
    """bits -> (signed mantissa, exponent): value = m * 2**x.  Denormals/zero -> (0, 0)."""
    e = (b >> 23) & 0xFF
    if e == 0:
        return 0, 0
    m = (b & 0x7FFFFF) | 0x800000
    return (-m if b & 0x80000000 else m), e - 150


def _enc(sign, mag, exp):
    """mag * 2**exp (mag > 0 int), truncated toward zero, flushed, clamped."""
    L = mag.bit_length()
    e = L - 1 + exp
    if e < -126:
        return sign
    if e >= 128:
        return sign | FMAX
    sh = L - 24
    m = (mag >> sh) if sh > 0 else (mag << -sh)
    return sign | ((e + 127) << 23) | (m & 0x7FFFFF)


def eadd(a, b):
    ma, xa = _dec(a)
    mb, xb = _dec(b)
    if ma == 0:
        if mb == 0:
            return 0x80000000 if (a & b) >> 31 else 0
        s, x = mb, xb
    elif mb == 0:
        s, x = ma, xa
    elif xa >= xb:
        s, x = (ma << (xa - xb)) + mb, xb
    else:
        s, x = ma + (mb << (xb - xa)), xa
    if s == 0:
        return 0x80000000 if (a & b) >> 31 else 0
    if s < 0:
        return _enc(0x80000000, -s, x)
    return _enc(0, s, x)


def esub(a, b):
    return eadd(a, b ^ 0x80000000)


def emul(a, b):
    ma, xa = _dec(a)
    mb, xb = _dec(b)
    p = ma * mb
    if p == 0:
        return 0x80000000 if (a ^ b) >> 31 & 1 else 0
    if p < 0:
        return _enc(0x80000000, -p, xa + xb)
    return _enc(0, p, xa + xb)


def ediv(a, b):
    ma, xa = _dec(a)
    mb, xb = _dec(b)
    if mb == 0:
        return ((a ^ b) & 0x80000000) | FMAX
    if ma == 0:
        return 0x80000000 if (a ^ b) >> 31 & 1 else 0
    sign = 0x80000000 if (ma < 0) != (mb < 0) else 0
    q = (abs(ma) << 26) // abs(mb)
    return _enc(sign, q, xa - xb - 26)


def esqrt(b):
    """EE SQRT.S: sqrt(|x|), truncated."""
    m, x = _dec(b)
    m = abs(m)
    if m == 0:
        return 0
    if x & 1:
        m <<= 1
        x -= 1
    r = math.isqrt(m << 50)
    return _enc(0, r, x // 2 - 25)


def ecvt_s_w(i):
    i &= 0xFFFFFFFF
    if i & 0x80000000:
        i -= 1 << 32
    if i == 0:
        return 0
    if i < 0:
        return _enc(0x80000000, -i, 0)
    return _enc(0, i, 0)


def ecvt_w_s(b):
    m, x = _dec(b)
    if m == 0:
        return 0
    if x >= 0:
        t = m << x
    else:
        t = -((-m) >> -x) if m < 0 else m >> -x
    if t > 0x7FFFFFFF:
        t = 0x7FFFFFFF
    if t < -0x80000000:
        t = -0x80000000
    return t & 0xFFFFFFFF


def fkey(b):
    """order key: value comparisons on EE floats (denormals and -0 are 0)."""
    if (b >> 23) & 0xFF == 0:
        return 0
    return -(b & 0x7FFFFFFF) if b & 0x80000000 else (b & 0x7FFFFFFF)


def fbits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def bitsf(b):
    return struct.unpack('<f', struct.pack('<I', b & 0xFFFFFFFF))[0]


# ------------------------------------------------------------------ integer helpers

def sx32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


def sx16(x):
    x &= 0xFFFF
    return x - (1 << 16) if x & 0x8000 else x


def sx8(x):
    x &= 0xFF
    return x - 0x100 if x & 0x80 else x


def u64(x):
    return x & M64


def s64(x):
    x &= M64
    return x - (1 << 64) if x >> 63 else x


def sext32(x):
    return sx32(x) & M64


# ------------------------------------------------------------------ 128-bit lane helpers (MMI)

def lanes(v, w):
    """128-bit value -> list of 128/w unsigned lanes, lane 0 = least significant."""
    m = (1 << w) - 1
    return [(v >> s) & m for s in range(0, 128, w)]


def slanes(v, w):
    """128-bit value -> list of signed lanes."""
    h = 1 << (w - 1)
    m = (1 << w) - 1
    out = []
    for s in range(0, 128, w):
        x = (v >> s) & m
        out.append(x - (1 << w) if x & h else x)
    return out


def pack(ls, w):
    """list of lanes (any ints; masked to w bits) -> 128-bit value."""
    m = (1 << w) - 1
    v = 0
    s = 0
    for x in ls:
        v |= (x & m) << s
        s += w
    return v


def _sat(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


def _sats(w):
    return -(1 << (w - 1)), (1 << (w - 1)) - 1


def _mk_lane(w, fn, signed):
    """rd = pack(fn(a_i, b_i)) over w-bit lanes."""
    get = slanes if signed else lanes

    def op(a, b):
        return pack([fn(x, y) for x, y in zip(get(a, w), get(b, w))], w)
    return op


def _add_sat_s(w):
    lo, hi = _sats(w)
    return _mk_lane(w, lambda x, y: _sat(x + y, lo, hi), True)


def _sub_sat_s(w):
    lo, hi = _sats(w)
    return _mk_lane(w, lambda x, y: _sat(x - y, lo, hi), True)


def _add_sat_u(w):
    hi = (1 << w) - 1
    return _mk_lane(w, lambda x, y: min(x + y, hi), False)


def _sub_sat_u(w):
    return _mk_lane(w, lambda x, y: max(x - y, 0), False)


def _abs_sat(w):
    hi = (1 << (w - 1)) - 1

    def op(a, b):
        return pack([min(abs(x), hi) for x in slanes(b, w)], w)
    return op


def _ext_low(w):
    """PEXTL*: interleave the low halves, rt lanes at even positions."""
    n = 64 // w

    def op(a, b):
        la, lb = lanes(a, w), lanes(b, w)
        out = []
        for i in range(n):
            out += [lb[i], la[i]]
        return pack(out, w)
    return op


def _ext_up(w):
    """PEXTU*: interleave the high halves, rt lanes at even positions."""
    n = 64 // w

    def op(a, b):
        la, lb = lanes(a, w), lanes(b, w)
        out = []
        for i in range(n):
            out += [lb[n + i], la[n + i]]
        return pack(out, w)
    return op


def _pac(w):
    """PPAC*: even lanes of rt then even lanes of rs."""
    def op(a, b):
        la, lb = lanes(a, w), lanes(b, w)
        return pack(lb[0::2] + la[0::2], w)
    return op


def _ppac5(b):
    out = []
    for x in lanes(b, 32):
        out.append(((x >> 3) & 0x1F) | (((x >> 11) & 0x1F) << 5) | (((x >> 19) & 0x1F) << 10) | ((x >> 31) << 15))
    return pack(out, 32)


def _pext5(b):
    out = []
    for x in lanes(b, 32):
        out.append(((x & 0x1F) << 3) | (((x >> 5) & 0x1F) << 11) | (((x >> 10) & 0x1F) << 19) | (((x >> 15) & 1) << 31))
    return pack(out, 32)


def _plzcw_word(x):
    """leading bits equal to bit 31, not counting bit 31 itself (0..31)."""
    if x & 0x80000000:
        x = ~x & 0xFFFFFFFF
    return 31 if x == 0 else 31 - x.bit_length()


def _pceqb(a, b):
    r = 0
    x = a ^ b
    for s in range(0, 128, 8):
        if not (x >> s) & 0xFF:
            r |= 0xFF << s
    return r


# ------------------------------------------------------------------ memory

class EmuError(RuntimeError):
    """An emulated program did something the model refuses to continue past.

    kind  the failure class, compared by tools/difftest.py when both sides fail:
          'range' (address outside RAM), 'misaligned u32' / 'misaligned w16' ...
          (u = load, w = store, with the width), 'runaway', 'div0', 'overflow',
          'trap', 'syscall N' (N = $v1, the kernel call number), 'break N',
          'unimplemented', 'delay-slot', 'abi'
          (a callee-saved register not restored), 'cstr' (unterminated string read
          by a test), 'heap' (stub allocator misuse), 'setup' (ELF / call setup);
    addr  the faulting data address for 'range' / 'misaligned *' / 'cstr' /
          'heap' errors, else None.
    The message is for people; e.pc (set by CPU.run) is where it happened."""

    def __init__(self, msg, kind='other', addr=None):
        RuntimeError.__init__(self, msg)
        self.kind = kind
        self.addr = addr


def mmio_window(p, n=1):
    """name of the MMIO window holding physical [p, p+n), or None."""
    for lo, hi, name in MMIO_WINDOWS:
        if lo <= p and p + n <= hi:
            return name
    return None


class Mem:
    """The EE address space of a test: 32 MB of RAM, the 16 KB scratchpad and the
    hardware (MMIO) windows.

    Virtual address map (what the EE kernel's default TLB gives a program):
      0x00000000..0x01FFFFFF  RAM (kuseg, cached)              -> index = address
      0x10000000..0x1FFFFFFF  hardware, identity-mapped         -> MMIO (see below)
      0x20000000..0x21FFFFFF  RAM, uncached mirror              -> index = address - 0x20000000
      0x30000000..0x31FFFFFF  RAM, uncached-accelerated mirror  -> index = address - 0x30000000
      0x70000000..0x70003FFF  scratchpad (its own 16 KB)        -> index = SPR_OFF + offset
      0x80000000..0xBFFFFFFF  kseg0 / kseg1: physical = address & 0x1FFFFFFF
                              (RAM below 32 MB, MMIO in the hardware windows)
    Anything else (0x02000000..0x0FFFFFFF, 0x40000000..0x6FFFFFFF, past the end of a
    mirror or of the scratchpad, 0xC0000000..) is a 'range' EmuError.

    Physical MMIO windows (MMIO_WINDOWS): EE registers 0x10000000..0x1000FFFF
    (timers, IPU, GIF, VIF0/1, FIFOs, DMAC, INTC, SIO, SIF ...), VU0/VU1 memory
    0x11000000..0x1100FFFF, GS privileged registers 0x12000000..0x12001FFF and the
    BIOS ROM 0x1FC00000..0x1FFFFFFF.  An MMIO load or store is not memory: it goes
    to self.io (an IoModel), which records it as an ordered event and answers loads
    from its register model.  Byte-range helpers (rd / wr / cstr) and the partial-
    word stores (swl/swr/sdl/sdr) refuse MMIO ('mmio' EmuError).

    m holds RAM at [0, MEMSIZE) and the scratchpad at [SPR_OFF, SPR_OFF + SPR_SIZE),
    so the write journal, reset() and written() work on both: written() lists
    indices, and index_addr() turns an index back into an address for messages.
    Every write journals the original contents of each word it touches (orig), so
    reset() restores the pristine state and written() lists the words changed since
    then."""

    # class-level defaults so instances built with Mem.__new__ (the self-test's
    # synthetic memories) behave like a fresh Mem with no watch window
    watch = None
    _wmask = None
    unwritten_reads = ()
    io = None

    def __init__(self, path, skip_sections=('.elf_header', '.elf_trailer')):
        self.path = path
        d = open(path, 'rb').read()
        if d[:4] != b'\x7fELF' or d[4] != 1 or d[5] != 1:
            raise EmuError('%s: not a little-endian ELF32 file' % path, 'setup')
        self.m = bytearray(MEMSIZE + SPR_SIZE)
        self.io = IoModel(self)
        e_phoff, e_shoff = struct.unpack_from('<II', d, 0x1C)
        e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHHHH', d, 0x2A)
        # file ranges of sections that are not part of the program image (splat's
        # ELF header / trailer pseudo sections in the matching build)
        skip = []
        if e_shoff and e_shnum:
            secs = [struct.unpack_from('<IIIIIIIIII', d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
            shstr = secs[e_shstrndx][4] if e_shstrndx < len(secs) else 0
            for s in secs:
                nm = d[shstr + s[0]:d.index(b'\0', shstr + s[0])].decode('latin1') if shstr else ''
                if nm in skip_sections and s[5]:
                    skip.append((s[4], s[4] + s[5]))
        self.segs = []
        for i in range(e_phnum):
            o = e_phoff + i * e_phentsize
            t, off, va, pa, fsz, msz, fl, al = struct.unpack_from('<IIIIIIII', d, o)
            if t != 1 or not msz:
                continue
            if any(lo <= off < hi for lo, hi in skip):
                continue
            va &= 0x1FFFFFFF
            if va + msz > MEMSIZE:
                raise EmuError('%s: segment %08X+%X outside RAM' % (path, va, msz), 'setup')
            self.m[va:va + fsz] = d[off:off + fsz]
            self.segs.append((va, fsz, msz))
        self.orig = {}          # word address -> the 4 bytes it held before the first write
        # set_watch(lo, hi): loads (u8..u64) of bytes in [lo, hi) that nothing wrote
        # since reset() are noted in unwritten_reads (a read of uninitialised memory,
        # e.g. a stack slot no code stored to)
        self.watch = None
        self.unwritten_reads = []
        self._wmask = None

    def set_watch(self, lo, hi):
        self.watch = (lo, hi)
        self.unwritten_reads = []
        self._wmask = bytearray(hi - lo)     # 1 = written since reset()
        self._wext = [hi - lo, 0]            # dirty extent of the mask

    def _mark(self, a, n):
        lo, hi = self.watch
        if a < hi and a + n > lo:
            b, e = max(a, lo) - lo, min(a + n, hi) - lo
            self._wmask[b:e] = b'\x01' * (e - b)
            x = self._wext
            if b < x[0]:
                x[0] = b
            if e > x[1]:
                x[1] = e

    def image_range(self):
        lo = min(va for va, fsz, msz in self.segs)
        hi = max(va + msz for va, fsz, msz in self.segs)
        return lo, hi

    # journal ------------------------------------------------------------
    def reset(self):
        m = self.m
        for k, v in self.orig.items():
            m[k:k + 4] = v
        self.orig = {}
        self.unwritten_reads = []
        if self._wmask is not None:
            b, e = self._wext
            if e > b:
                self._wmask[b:e] = bytes(e - b)
            self._wext = [len(self._wmask), 0]

    def _note(self, a, n):
        lo = self.watch[0]
        u = self.unwritten_reads
        for k in range(a, a + n):
            if k - lo < len(self._wmask) and not self._wmask[k - lo]:
                if len(u) < 16 and k not in u:
                    u.append(k)

    def commit(self):
        """make the current contents the pristine state (drop the journal)."""
        self.orig = {}

    def written(self):
        return sorted(self.orig)

    def _j(self, a, n):
        o = self.orig
        k = a & ~3
        end = a + n
        while k < end:
            if k not in o:
                o[k] = bytes(self.m[k:k + 4])
            k += 4

    # address translation ---------------------------------------------------
    @staticmethod
    def _slow(a, n):
        """index into m of an n-byte access at virtual address a outside the
        plain-RAM fast path, or -1 when it is an MMIO access (raises 'range' for
        an unmapped address)."""
        a &= 0xFFFFFFFF
        seg = a >> 28
        if seg == 0:
            p = a                                           # past the end of RAM
        elif seg == 1:
            p = a                                           # hardware, identity-mapped
        elif seg in (2, 3):
            p = a & 0x0FFFFFFF                              # uncached / UCAB RAM mirror
            if p + n <= MEMSIZE:
                return p
            raise EmuError('address %08X out of range' % a, 'range', a)
        elif seg == 7:
            o = a - SPR_BASE
            if 0 <= o and o + n <= SPR_SIZE:
                return SPR_OFF + o
            raise EmuError('address %08X out of range' % a, 'range', a)
        elif 8 <= seg <= 0xB:
            p = a & 0x1FFFFFFF                              # kseg0 / kseg1
        else:
            raise EmuError('address %08X out of range' % a, 'range', a)
        if p + n <= MEMSIZE:
            return p
        if p >= 0x10000000 and mmio_window(p, n):
            return -1
        raise EmuError('address %08X out of range' % a, 'range', a)

    @staticmethod
    def phys(a):
        """physical address of a hardware (MMIO) virtual address."""
        a &= 0xFFFFFFFF
        return a & 0x1FFFFFFF if a >> 31 else a

    def _chk(self, a, n):
        """index of an n-byte RAM / scratchpad range (no MMIO)."""
        if 0 <= a and a + n <= MEMSIZE:
            return a
        k = self._slow(a, n)
        if k < 0:
            raise EmuError('%d-byte access to hardware at %08X (byte-range access to MMIO)' % (n, a), 'mmio', a)
        return k

    @staticmethod
    def index_addr(k):
        """the virtual address of index k of m (for messages)."""
        return SPR_BASE + (k - SPR_OFF) if k >= SPR_OFF else k

    def _io(self):
        io = self.io
        if io is None:
            io = self.io = IoModel(self)
        return io

    # reads ----------------------------------------------------------------
    def rd(self, a, n):
        a = self._chk(a, n)
        return bytes(self.m[a:a + n])

    def u8(self, a):
        if a + 1 > MEMSIZE:
            k = self._slow(a, 1)
            if k < 0:
                return self._io().read(self.phys(a), 1)
            a = k
        w = self.watch
        if w is not None and w[0] <= a < w[1]:
            self._note(a, 1)
        return self.m[a]

    def u16(self, a):
        if a & 1:
            raise EmuError('misaligned u16 %08X' % a, 'misaligned u16', a)
        if a + 2 > MEMSIZE:
            k = self._slow(a, 2)
            if k < 0:
                return self._io().read(self.phys(a), 2)
            a = k
        w = self.watch
        if w is not None and w[0] <= a < w[1]:
            self._note(a, 2)
        return struct.unpack_from('<H', self.m, a)[0]

    def u32(self, a):
        if a & 3:
            raise EmuError('misaligned u32 %08X' % a, 'misaligned u32', a)
        if a + 4 > MEMSIZE:
            k = self._slow(a, 4)
            if k < 0:
                return self._io().read(self.phys(a), 4)
            a = k
        w = self.watch
        if w is not None and w[0] <= a < w[1]:
            self._note(a, 4)
        return struct.unpack_from('<I', self.m, a)[0]

    def u64(self, a):
        if a & 7:
            raise EmuError('misaligned u64 %08X' % a, 'misaligned u64', a)
        if a + 8 > MEMSIZE:
            k = self._slow(a, 8)
            if k < 0:
                return self._io().read(self.phys(a), 8)
            a = k
        w = self.watch
        if w is not None and w[0] <= a < w[1]:
            self._note(a, 8)
        return struct.unpack_from('<Q', self.m, a)[0]

    def u128(self, a):
        """16-byte load (LQ / LQC2: the caller has cleared the low 4 bits)."""
        if a + 16 > MEMSIZE:
            k = self._slow(a, 16)
            if k < 0:
                return self._io().read(self.phys(a), 16)
            a = k
        w = self.watch
        if w is not None and w[0] <= a < w[1]:
            self._note(a, 16)
        lo_, hi_ = struct.unpack_from('<QQ', self.m, a)
        return lo_ | (hi_ << 64)

    def cstr(self, a, limit=0x10000):
        k = self._chk(a, 1)
        end = MEMSIZE if k < MEMSIZE else SPR_OFF + SPR_SIZE
        e = self.m.find(b'\0', k, min(k + limit, end))
        if e < 0:
            raise EmuError('unterminated string at %08X' % a, 'cstr', a)
        return bytes(self.m[k:e])

    # writes ---------------------------------------------------------------
    def wr(self, a, bs):
        a = self._chk(a, len(bs))
        self._j(a, len(bs))
        if self.watch is not None:
            self._mark(a, len(bs))
        self.m[a:a + len(bs)] = bs

    def w8(self, a, v):
        if a + 1 > MEMSIZE:
            k = self._slow(a, 1)
            if k < 0:
                return self._io().write(self.phys(a), 1, v)
            a = k
        self._j(a, 1)
        if self.watch is not None:
            self._mark(a, 1)
        self.m[a] = v & 0xFF

    def w16(self, a, v):
        if a & 1:
            raise EmuError('misaligned w16 %08X' % a, 'misaligned w16', a)
        if a + 2 > MEMSIZE:
            k = self._slow(a, 2)
            if k < 0:
                return self._io().write(self.phys(a), 2, v)
            a = k
        self._j(a, 2)
        if self.watch is not None:
            self._mark(a, 2)
        struct.pack_into('<H', self.m, a, v & 0xFFFF)

    def w32(self, a, v):
        if a & 3:
            raise EmuError('misaligned w32 %08X' % a, 'misaligned w32', a)
        if a + 4 > MEMSIZE:
            k = self._slow(a, 4)
            if k < 0:
                return self._io().write(self.phys(a), 4, v)
            a = k
        self._j(a, 4)
        if self.watch is not None:
            self._mark(a, 4)
        struct.pack_into('<I', self.m, a, v & 0xFFFFFFFF)

    def w64(self, a, v):
        if a & 7:
            raise EmuError('misaligned w64 %08X' % a, 'misaligned w64', a)
        if a + 8 > MEMSIZE:
            k = self._slow(a, 8)
            if k < 0:
                return self._io().write(self.phys(a), 8, v)
            a = k
        self._j(a, 8)
        if self.watch is not None:
            self._mark(a, 8)
        struct.pack_into('<Q', self.m, a, v & M64)

    def w128(self, a, v):
        """16-byte store (SQ / SQC2: the caller has cleared the low 4 bits)."""
        if a + 16 > MEMSIZE:
            k = self._slow(a, 16)
            if k < 0:
                return self._io().write(self.phys(a), 16, v)
            a = k
        self._j(a, 16)
        if self.watch is not None:
            self._mark(a, 16)
        struct.pack_into('<QQ', self.m, a, v & M64, (v >> 64) & M64)


# ------------------------------------------------------------------ hardware model (MMIO, COP0, VU0)

HARNESS_RET = 0x00FFFFF0        # return address of a harness call (never code); CPU.RET

# DMAC channels: CHCR address -> channel; the other registers are at +0x10 MADR,
# +0x20 QWC, +0x30 TADR, +0x40 ASR0, +0x50 ASR1, +0x80 SADR
DMA_CHCR = {0x10008000: 0, 0x10009000: 1, 0x1000A000: 2, 0x1000B000: 3, 0x1000B400: 4,
            0x1000C000: 5, 0x1000C400: 6, 0x1000C800: 7, 0x1000D000: 8, 0x1000D400: 9}
DMA_BASE = {ch: a for a, ch in DMA_CHCR.items()}
DMA_NAMES = ['VIF0', 'VIF1', 'GIF', 'fromIPU', 'toIPU', 'SIF0', 'SIF1', 'SIF2', 'fromSPR', 'toSPR']
DMA_TO_MEMORY = (3, 5, 8)       # always peripheral -> memory; 2 4 6 9 always memory -> peripheral,
DMA_FROM_MEMORY = (2, 4, 6, 9)  # 0 1 7 by CHCR.DIR (bit 0, 1 = from memory)
# registers holding a DMA memory address (physical, bit 31 = scratchpad): a pointer
DMA_ADDR_REGS = frozenset(b + o for b in DMA_BASE.values() for o in (0x10, 0x30, 0x40, 0x50))
DMA_MAX_TAGS = 4096             # a chain walk stops after this many tags ...
DMA_MAX_QW = 0x10000            # ... or this many qwords (1 MB)

VU0_DATA = 0x11004000           # VU0 data memory as the EE sees it (4 KB)
GS_CSR = 0x12001000

# reset / idle values of hardware registers (32-bit words; every other byte reads
# 0 until written).  GS CSR: VSINT set (a vsync poll ends at once), FIFO empty,
# REV 0x1B, ID 0x55; it and INTC_STAT are not latched (writing 1 clears a bit).
RESET_VALUES = {GS_CSR: 0x551B4008}
NO_LATCH = frozenset([GS_CSR, GS_CSR + 4, 0x1000F000])

_REG_NAMES = {
    0x10003000: 'GIF_CTRL', 0x10003010: 'GIF_MODE', 0x10003020: 'GIF_STAT', 0x10003080: 'GIF_CNT',
    0x10003800: 'VIF0_STAT', 0x10003810: 'VIF0_FBRST', 0x10003820: 'VIF0_ERR', 0x10003830: 'VIF0_MARK',
    0x10003880: 'VIF0_CODE', 0x10003C00: 'VIF1_STAT', 0x10003C10: 'VIF1_FBRST', 0x10003C20: 'VIF1_ERR',
    0x10003C30: 'VIF1_MARK', 0x10003C80: 'VIF1_CODE', 0x10004000: 'VIF0_FIFO', 0x10005000: 'VIF1_FIFO',
    0x10006000: 'GIF_FIFO', 0x10007000: 'IPU_OUT_FIFO', 0x10007010: 'IPU_IN_FIFO',
    0x1000E000: 'D_CTRL', 0x1000E010: 'D_STAT', 0x1000E020: 'D_PCR', 0x1000E030: 'D_SQWC',
    0x1000E040: 'D_RBSR', 0x1000E050: 'D_RBOR', 0x1000E060: 'D_STADR',
    0x1000F000: 'INTC_STAT', 0x1000F010: 'INTC_MASK', 0x1000F100: 'SIO_LCR', 0x1000F110: 'SIO_LSR',
    0x1000F120: 'SIO_IER', 0x1000F130: 'SIO_ISR', 0x1000F140: 'SIO_FCR', 0x1000F150: 'SIO_BGR',
    0x1000F180: 'SIO_TXFIFO', 0x1000F1C0: 'SIO_RXFIFO', 0x1000F200: 'SIF_MSCOM', 0x1000F210: 'SIF_SMCOM',
    0x1000F220: 'SIF_MSFLG', 0x1000F230: 'SIF_SMFLG', 0x1000F240: 'SIF_CTRL', 0x1000F260: 'SIF_BD6',
    0x1000F520: 'D_ENABLER', 0x1000F590: 'D_ENABLEW',
    0x12000000: 'GS_PMODE', 0x12000010: 'GS_SMODE1', 0x12000020: 'GS_SMODE2', 0x12000030: 'GS_SRFSH',
    0x12000040: 'GS_SYNCH1', 0x12000050: 'GS_SYNCH2', 0x12000060: 'GS_SYNCV', 0x12000070: 'GS_DISPFB1',
    0x12000080: 'GS_DISPLAY1', 0x12000090: 'GS_DISPFB2', 0x120000A0: 'GS_DISPLAY2', 0x120000B0: 'GS_EXTBUF',
    0x120000C0: 'GS_EXTDATA', 0x120000D0: 'GS_EXTWRITE', 0x120000E0: 'GS_BGCOLOR', 0x12001000: 'GS_CSR',
    0x12001010: 'GS_IMR', 0x12001040: 'GS_BUSDIR', 0x12001080: 'GS_SIGLBLID',
}
for _ch, _b in DMA_BASE.items():
    for _o, _n in ((0, 'CHCR'), (0x10, 'MADR'), (0x20, 'QWC'), (0x30, 'TADR'), (0x40, 'ASR0'), (0x50, 'ASR1'),
                   (0x80, 'SADR')):
        _REG_NAMES[_b + _o] = 'D%d_%s' % (_ch, _n)
for _k, _b in enumerate((0x10000000, 0x10000800, 0x10001000, 0x10001800)):
    for _o, _n in ((0, 'COUNT'), (0x10, 'MODE'), (0x20, 'COMP'), (0x30, 'HOLD')):
        _REG_NAMES[_b + _o] = 'T%d_%s' % (_k, _n)


def io_name(p):
    """a readable name for a physical hardware address (or a register-model key)."""
    if not isinstance(p, int):
        return str(p)
    n = _REG_NAMES.get(p)
    if n:
        return n
    n = _REG_NAMES.get(p & ~7)
    if n:
        return '%s+%d' % (n, p & 7)
    for lo, nm in ((0x11000000, 'VU0_MICRO'), (0x11004000, 'VU0_DATA'), (0x11008000, 'VU1_MICRO'),
                   (0x1100C000, 'VU1_DATA')):
        if lo <= p < lo + 0x4000:
            return '%s+0x%X' % (nm, p - lo)
    if 0x1FC00000 <= p < 0x20000000:
        return 'ROM+0x%X' % (p - 0x1FC00000)
    return 'HW_%08X' % p


COP0_NAMES = {0: 'Index', 1: 'Random', 2: 'EntryLo0', 3: 'EntryLo1', 4: 'Context', 5: 'PageMask', 6: 'Wired',
              8: 'BadVAddr', 9: 'Count', 10: 'EntryHi', 11: 'Compare', 12: 'Status', 13: 'Cause', 14: 'EPC',
              15: 'PRId', 16: 'Config', 23: 'BadPAddr', 28: 'TagLo', 29: 'TagHi', 30: 'ErrorEPC'}
# COP0 state at the start of a case: a user thread of the EE kernel (CU0-2, EDI,
# EIE, IM2/IM3, KSU user, EXL, IE); EPC / ErrorEPC = the harness return address,
# so an ERET out of a handler the test called returns to the test.
COP0_RESET = {'Status': 0x70030C13, 'PRId': 0x2E20, 'Config': 0x440, 'EPC': HARNESS_RET, 'ErrorEPC': HARNESS_RET}
COUNT_STEP = 0x1000             # each read of Count returns the previous value + this
STATUS_EIE = 0x10000


def cop0_name(rd, sel):
    if rd == 25:                                # MFPS/MTPS/MFPC/MTPC
        return {0: 'PCCR', 1: 'PCR0', 3: 'PCR1'}.get(sel, 'Perf.%d' % sel)
    if rd == 24:                                # MFBPC/MTBPC/... (debug breakpoint registers)
        return 'Debug.%d' % sel
    return COP0_NAMES.get(rd, 'cop0.%d' % rd)


# VU0 control registers (cfc2 / ctc2 vi16..31): 16 Status flag, 17 MAC flag,
# 18 clipping flag, 20 R, 21 I, 22 Q are computation state; 26 TPC, 27 CMSAR0,
# 28 FBRST, 29 VPU-STAT, 31 CMSAR1 (and the reserved 19, 23-25, 30) are hardware:
# their reads and writes are events answered by the register model (key 'vi29' ...).
VU0_HW_REGS = frozenset([19, 23, 24, 25, 26, 27, 28, 29, 30, 31])


class Reg:
    """A spec's model of one hardware register (or COP0 / VU0 register).

    read   None    the default: the last value written (latched), or the reset value
           int     every read returns it
           list    successive reads return successive items; the last one repeats
           fn(n, latched) -> int   n = how many reads came before
    write  None or fn(value, width): called after a write (which is still latched)."""

    def __init__(self, read=None, write=None):
        self.read = read
        self.write = write
        self.n = 0

    def value(self, latched):
        r = self.read
        n = self.n
        self.n = n + 1
        if isinstance(r, int):
            return r
        if isinstance(r, (list, tuple)):
            return r[min(n, len(r) - 1)] if r else latched
        return r(n, latched)


class IoModel:
    """The hardware a test can see, identical on both sides of a differential test.

    Every hardware access becomes an event appended to self.events, in program order
    with whatever else the harness logs there:
        ('hw', key, value, count)
    key                            value
    ('r', phys, width)             the value the load got      MMIO load (width 1/2/4/8/16)
    ('w', phys, width)             the value stored            MMIO store
    ('dma', ch, mode)              snapshot of the transfer    a store setting Dn_CHCR.STR
    ('mfc0', name) / ('mtc0', name)  the value                 COP0 (not mfc0 Status)
    ('di',) ('ei',) ('eret',) ('sync.l',) ('sync.p',)  None
    ('cache', op)                  the effective address       CACHE
    ('cfc2', 'viN') / ('ctc2', 'viN')  the value               VU0 hardware control regs
    ('vcallms', addr)              None                        VCALLMS / VCALLMSR (not run)
    count is 1, except that identical consecutive reads ('r', 'mfc0', 'cfc2' with the
    same value) are merged into one event with the number of reads, so a polling loop
    stays one event while the number of polls is still compared.

    Loads are answered by the register model: a spec's Reg for that key (physical
    address, COP0 register name or 'viN'), else the default - the last value stored
    (latched per byte; never-written bytes read their RESET_VALUES word or 0), with
    the 'idle' rules: Dn_CHCR reads with STR (0x100) clear (every DMA has finished),
    GS CSR reads 0x551B4008 (VSINT set), INTC_STAT reads 0; so VIF/GIF STAT read 0
    (idle) and VPU-STAT (vi29) reads 0 (VU0/VU1 stopped).

    A store that sets Dn_CHCR.STR snapshots the transfer from memory at that moment:
    normal mode MADR/QWC, or the source chain from TADR (cnt next ref refs refe call
    ret end; an IRQ tag with CHCR.TIE stops it), as ('tag', w0, w1, w2, w3) and
    ('data', bytes) items ending with ('end', why); a peripheral -> memory transfer
    is just (MADR, QWC).  DMA addresses are physical, bit 31 = scratchpad.

    COP0 state (cop0 dict): Status (di/ei clear/set EIE, bit 16), Count (each read
    returns the previous value + COUNT_STEP: deterministic, never instruction-count
    based), EPC/ErrorEPC (ERET target), TagLo (set by the D/I-cache load-tag ops
    from self.cache_tag(op, addr), default 0 = no valid line), the performance
    counters and the rest (latched).  VU0 state: vf[32] (4 x 32-bit float bits,
    vf0 = 0,0,0,1.0), vi[32] (integer and control registers), acc (4 floats)."""

    MERGE = frozenset(['r', 'mfc0', 'cfc2'])

    def __init__(self, mem=None):
        self.mem = mem
        self.reset()

    def reset(self):
        self.events = []
        self.latch = {}               # physical byte address -> last byte stored
        self.models = {}              # key -> Reg
        self.cop0 = dict(COP0_RESET)
        self.cache_tag = None         # fn(op, addr) -> TagLo for CACHE 0x00 / 0x10 (load tag)
        self.vf = [[0, 0, 0, 0] for _ in range(32)]
        self.vf[0] = [0, 0, 0, 0x3F800000]
        self.vi = [0] * 32
        self.acc = [0, 0, 0, 0]

    def model(self, key, read=None, write=None):
        r = self.models[key] = Reg(read, write)
        return r

    def emit(self, key, value):
        ev = self.events
        if ev and key[0] in self.MERGE:
            last = ev[-1]
            if last[0] == 'hw' and last[1] == key and last[2] == value:
                ev[-1] = ('hw', key, value, last[3] + 1)
                return
        ev.append(('hw', key, value, 1))

    # MMIO ------------------------------------------------------------------
    def latched(self, phys, width):
        lt = self.latch
        v = 0
        for i in range(width):
            b = lt.get(phys + i)
            if b is None:
                a = phys + i
                b = (RESET_VALUES.get(a & ~3, 0) >> (8 * (a & 3))) & 0xFF
            v |= b << (8 * i)
        return v

    def _reset_value(self, phys, width):
        v = 0
        for i in range(width):
            a = phys + i
            v |= ((RESET_VALUES.get(a & ~3, 0) >> (8 * (a & 3))) & 0xFF) << (8 * i)
        return v

    def read(self, phys, width):
        m = self.models.get(phys)
        if (phys & ~7) in NO_LATCH or (phys & ~3) in NO_LATCH:
            latched = self._reset_value(phys, width)
        else:
            latched = self.latched(phys, width)
        if m is not None and m.read is not None:
            v = m.value(latched)
        elif phys in DMA_CHCR:
            v = latched & ~0x100                     # STR clear: the transfer has finished
        else:
            v = latched
        v &= (1 << (8 * width)) - 1
        self.emit(('r', phys, width), v)
        return v

    def write(self, phys, width, value):
        value &= (1 << (8 * width)) - 1
        self.emit(('w', phys, width), value)
        lt = self.latch
        for i in range(width):
            lt[phys + i] = (value >> (8 * i)) & 0xFF
        m = self.models.get(phys)
        if m is not None and m.write is not None:
            m.write(value, width)
        ch = DMA_CHCR.get(phys)
        if ch is not None and value & 0x100:
            self.dma_kick(ch, value)

    # DMA -------------------------------------------------------------------
    def _dma_rd(self, addr, nqw):
        """nqw qwords at a DMA address (bit 31 = scratchpad), or None if unmapped."""
        n = 16 * nqw
        m = self.mem.m
        if addr & 0x80000000:
            o = addr & 0x3FF0
            if o + n <= SPR_SIZE:
                return bytes(m[SPR_OFF + o:SPR_OFF + o + n])
            out = bytearray()
            while len(out) < n:                     # the scratchpad address wraps
                k = min(n - len(out), SPR_SIZE - o)
                out += m[SPR_OFF + o:SPR_OFF + o + k]
                o = 0
            return bytes(out)
        p = addr & 0x7FFFFFF0
        if p + n > MEMSIZE:
            return None
        return bytes(m[p:p + n])

    def dma_kick(self, ch, chcr):
        base = DMA_BASE[ch]
        madr = self.latched(base + 0x10, 4)
        qwc = self.latched(base + 0x20, 4) & 0xFFFF
        tadr = self.latched(base + 0x30, 4)
        mode = (chcr >> 2) & 3
        src = ch in DMA_FROM_MEMORY or (ch not in DMA_TO_MEMORY and chcr & 1)
        if not src:
            self.emit(('dma', ch, 'to-memory'), (madr, qwc))
            return
        rec = []
        if mode == 0:
            d = self._dma_rd(madr, qwc)
            rec.append(('data', d) if d is not None else ('end', 'bad MADR %08X' % madr))
            self.emit(('dma', ch, 'normal'), tuple(rec))
            return
        if mode != 1:
            self.emit(('dma', ch, 'mode %d' % mode), (madr, qwc, tadr))
            return
        asr = []
        asp = (chcr >> 4) & 3
        for k in range(min(asp, 2)):
            asr.append(self.latched(base + 0x40 + 0x10 * k, 4))
        total = qwc
        if qwc:
            d = self._dma_rd(madr, qwc)
            if d is None:
                rec.append(('end', 'bad MADR %08X' % madr))
                self.emit(('dma', ch, 'chain'), tuple(rec))
                return
            rec.append(('data', d))
        tag = tadr
        for _ in range(DMA_MAX_TAGS):
            raw = self._dma_rd(tag, 1)
            if raw is None:
                rec.append(('end', 'bad tag address %08X' % tag))
                break
            w0, w1, w2, w3 = struct.unpack('<4I', raw)
            rec.append(('tag', w0, w1, w2, w3))
            n = w0 & 0xFFFF
            tid = (w0 >> 28) & 7
            if tid in (0, 3, 4):                    # refe / ref / refs: data at ADDR
                at, after = w1, tag + 16
            else:
                at = tag + 16
                after = at + 16 * n
            if n:
                d = self._dma_rd(at, n)
                if d is None:
                    rec.append(('end', 'bad data address %08X' % at))
                    break
                rec.append(('data', d))
            total += n
            if total > DMA_MAX_QW:
                rec.append(('end', 'qword limit'))
                break
            if w0 >> 31 and chcr & 0x80:
                rec.append(('end', 'irq'))
                break
            if tid in (0, 7):
                rec.append(('end', 'refe' if tid == 0 else 'end'))
                break
            if tid == 1:
                tag = after
            elif tid == 2:
                tag = w1
            elif tid in (3, 4):
                tag = tag + 16
            elif tid == 5:
                if len(asr) >= 2:
                    rec.append(('end', 'call with ASR full'))
                    break
                asr.append(after)
                tag = w1
            else:                                   # 6: ret
                if not asr:
                    rec.append(('end', 'ret'))
                    break
                tag = asr.pop()
        else:
            rec.append(('end', 'tag limit'))
        self.emit(('dma', ch, 'chain'), tuple(rec))

    # COP0 ------------------------------------------------------------------
    def cop0_read(self, name):
        c = self.cop0
        m = self.models.get(name)
        if m is not None and m.read is not None:
            v = m.value(c.get(name, 0))
        elif name == 'Count':
            v = c.get('Count', 0)
            c['Count'] = (v + COUNT_STEP) & M32
        else:
            v = c.get(name, 0)
        v &= M32
        if name != 'Status':                        # a Status read has no side effect
            self.emit(('mfc0', name), v)
        return v

    def cop0_write(self, name, value):
        value &= M32
        self.emit(('mtc0', name), value)
        self.cop0[name] = value
        m = self.models.get(name)
        if m is not None and m.write is not None:
            m.write(value, 4)

    def op(self, name):
        """di / ei / sync.l / sync.p: an event (di and ei also clear / set Status.EIE)."""
        self.emit((name,), None)
        if name == 'di':
            self.cop0['Status'] &= ~STATUS_EIE
        elif name == 'ei':
            self.cop0['Status'] |= STATUS_EIE

    def eret(self):
        """ERET: the target (ErrorEPC when Status.ERL, else EPC); clears ERL / EXL."""
        self.emit(('eret',), None)
        c = self.cop0
        if c['Status'] & 4:
            c['Status'] &= ~4
            return c.get('ErrorEPC', 0) & M32
        c['Status'] &= ~2
        return c.get('EPC', 0) & M32

    def cache(self, op, addr):
        self.emit(('cache', op), addr)
        if op in (0x00, 0x10):                      # IXLTG / DXLTG: index load tag -> TagLo
            f = self.cache_tag
            self.cop0['TagLo'] = (f(op, addr) if f is not None else 0) & M32
            self.cop0['TagHi'] = 0

    # VU0 control registers --------------------------------------------------
    def vi_read(self, r):
        if r in VU0_HW_REGS:
            key = 'vi%d' % r
            m = self.models.get(key)
            v = m.value(self.vi[r]) if m is not None and m.read is not None else self.vi[r]
            v &= M32
            self.emit(('cfc2', key), v)
            return v
        return self.vi[r]

    def vi_write(self, r, value):
        if r == 0:
            return
        if r in VU0_HW_REGS:
            key = 'vi%d' % r
            value &= M32
            self.emit(('ctc2', key), value)
            self.vi[r] = value
            m = self.models.get(key)
            if m is not None and m.write is not None:
                m.write(value, 4)
            return
        self.vi[r] = value & (0xFFFF if r < 16 else M32)


# ------------------------------------------------------------------ CPU

LIKELY_SKIP = -1
ERET_JUMP = -2

# register sentinels loaded before every call (identical on both sides of a test)
SENTINEL = 0x5EED000000000000


class CPU:
    RET = HARNESS_RET           # return address of a harness call (never code)
    SP0 = 0x01FFE000            # $sp at the start of a harness call
    STACK_LO = 0x01F00000       # [STACK_LO, SP0) is the stack
    STACK_HI = 0x02000000

    def __init__(self, mem, gp=0):
        self.mem = mem
        if getattr(mem, 'io', None) is None:
            mem.io = IoModel(mem)
        self.io = mem.io            # hardware state + events (COP0, VU0, MMIO); reset by the user
        self._eret_pc = 0
        self.g = [0] * 32
        self.gh = [0] * 32          # bits 127..64 of each GPR
        self.f = [0] * 32
        self.acc = 0
        self.cond = False
        self.hi = self.lo = 0
        self.hi1 = self.lo1 = 0
        self.sa = 0
        self.hooks = {}
        self.syscall_hook = None    # fn(cpu, code): emulates SYSCALL (number in $v1); None = EmuError
        self.cache = {}
        self.steps = 0
        self.gp = gp

    # -------------------------------------------------------------- calls
    def reset_regs(self):
        """deterministic values in every register (identical on both sides)."""
        g, gh, f = self.g, self.gh, self.f
        for i in range(1, 32):
            g[i] = SENTINEL | (i << 8)
            gh[i] = SENTINEL | (i << 8) | 0x80
            f[i] = 0x3F800000 + i       # 1.0 + a few ulps
        g[0] = gh[0] = 0
        f[0] = 0
        self.hi, self.lo, self.hi1, self.lo1 = SENTINEL | 0x11, SENTINEL | 0x22, SENTINEL | 0x33, SENTINEL | 0x44
        self.sa = 0
        self.acc = 0
        self.cond = False

    def call(self, entry, ints=(), floats=(), maxsteps=50000000, sp=None):
        """EABI call: ints in $a0..$a3,$t0..$t3 ($4..$11), floats in $f12..$f19.
        A None entry leaves that register at its sentinel (an n32 caller puts a
        float argument's slot in $f12+k, not in $4+k, and vice versa).
        Returns ($v0, $v1, $f0).  Raises EmuError if a callee-saved register
        ($s0..$s7, $gp, $sp, $fp: bits 63..0, $f20..$f31) is not preserved."""
        self.reset_regs()
        g = self.g
        if len(ints) > 8 or len(floats) > 8:
            raise EmuError('more than 8 register arguments', 'setup')
        for i, v in enumerate(ints):
            if v is None:
                continue
            if isinstance(v, tuple):                    # (low, high): all 128 bits of the register
                g[4 + i], self.gh[4 + i] = v[0] & M64, v[1] & M64
                continue
            g[4 + i] = v & M64
            if v > M64:                                 # a 128-bit argument (e.g. a qword struct in $a3)
                self.gh[4 + i] = (v >> 64) & M64
        for i, v in enumerate(floats):
            if v is None:
                continue
            self.f[12 + i] = v & 0xFFFFFFFF
        g[31] = self.RET
        g[29] = self.SP0 if sp is None else sp
        g[28] = self.gp
        saved = [g[r] for r in (16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30)]
        fsaved = self.f[20:32]
        self.run(entry, maxsteps)
        now = [g[r] for r in (16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30)]
        if now != saved:
            bad = [r for r, a, b in zip((16, 17, 18, 19, 20, 21, 22, 23, 28, 29, 30), saved, now) if a != b]
            raise EmuError('callee-saved register(s) %s not preserved' % ','.join('$%d' % r for r in bad), 'abi')
        if self.f[20:32] != fsaved:
            raise EmuError('callee-saved FPR(s) $f20..$f31 not preserved', 'abi')
        return g[2], g[3], self.f[0]

    def run(self, pc, maxsteps):
        g = self.g
        cache = self.cache
        hooks = self.hooks
        RET = self.RET
        decode = self.decode
        rd32 = self.mem.u32
        steps = 0
        delay = None
        try:
            while True:
                if delay is None:
                    if pc == RET:
                        return
                    h = hooks.get(pc)
                    if h is not None:
                        r = h(self)
                        if r:
                            # True: return to $ra; an int: continue there (a thunk)
                            pc = g[31] & 0xFFFFFFFF if r is True else r
                            continue
                steps += 1
                if steps > maxsteps:
                    raise EmuError('runaway (more than %d instructions) at %08X' % (maxsteps, pc), 'runaway')
                fn = cache.get(pc)
                if fn is None:
                    fn = decode(pc, rd32(pc))
                    cache[pc] = fn
                t = fn()
                if delay is not None:
                    if t is not None:
                        raise EmuError('branch in delay slot at %08X' % pc, 'delay-slot')
                    pc = delay
                    delay = None
                    continue
                if t is None:
                    pc += 4
                elif t == LIKELY_SKIP:
                    pc += 8
                elif t == ERET_JUMP:                # no delay slot
                    pc = self._eret_pc
                else:
                    delay = t
                    pc += 4
        except EmuError as e:
            if not getattr(e, 'pc', None):
                e.pc = pc
            raise
        finally:
            self.steps += steps
            self.last_pc = pc

    # -------------------------------------------------------------- 128-bit views
    def q(self, r):
        return self.g[r] | (self.gh[r] << 64)

    def setq(self, r, v):
        if r:
            self.g[r] = v & M64
            self.gh[r] = (v >> 64) & M64

    def lo128(self):
        return (self.lo & M64) | ((self.lo1 & M64) << 64)

    def hi128(self):
        return (self.hi & M64) | ((self.hi1 & M64) << 64)

    def set_lo128(self, v):
        self.lo = v & M64
        self.lo1 = (v >> 64) & M64

    def set_hi128(self, v):
        self.hi = v & M64
        self.hi1 = (v >> 64) & M64

    # -------------------------------------------------------------- decoder
    def decode(self, pc, w):
        g, gh, f, mem = self.g, self.gh, self.f, self.mem
        cpu = self
        op = w >> 26
        rs = (w >> 21) & 31
        rt = (w >> 16) & 31
        rd = (w >> 11) & 31
        sa = (w >> 6) & 31
        fn = w & 63
        imm = w & 0xFFFF
        simm = sx16(imm)
        br = (pc + 4 + (simm << 2)) & 0xFFFFFFFF
        bad = EmuError('unimplemented %08X at %08X' % (w, pc), 'unimplemented')

        def nop():
            return None

        def W(dst, expr):
            """a closure writing g[dst] = expr() (bits 63..0; discarded for r0)."""
            if dst == 0:
                def f0():
                    expr()
                    return None
                return f0

            def f1():
                g[dst] = expr()
                return None
            return f1

        if w == 0:
            return nop

        # ------------------------------------------------ SPECIAL
        if op == 0:
            if fn == 0x00: return W(rd, lambda: sext32((g[rt] << sa) & 0xFFFFFFFF))
            if fn == 0x02: return W(rd, lambda: sext32((g[rt] & 0xFFFFFFFF) >> sa))
            if fn == 0x03: return W(rd, lambda: sext32(sx32(g[rt]) >> sa))
            if fn == 0x04: return W(rd, lambda: sext32((g[rt] << (g[rs] & 31)) & 0xFFFFFFFF))
            if fn == 0x06: return W(rd, lambda: sext32((g[rt] & 0xFFFFFFFF) >> (g[rs] & 31)))
            if fn == 0x07: return W(rd, lambda: sext32(sx32(g[rt]) >> (g[rs] & 31)))
            if fn == 0x08:
                def jr():
                    return g[rs] & 0xFFFFFFFF
                return jr
            if fn == 0x09:
                def jalr():
                    t = g[rs] & 0xFFFFFFFF
                    if rd:
                        g[rd] = pc + 8
                    return t
                return jalr
            if fn == 0x0A:
                def movz():
                    if g[rt] == 0 and rd:
                        g[rd] = g[rs]
                return movz
            if fn == 0x0B:
                def movn():
                    if g[rt] != 0 and rd:
                        g[rd] = g[rs]
                return movn
            if fn == 0x0C:
                code = (w >> 6) & 0xFFFFF

                def syscall():
                    h = cpu.syscall_hook
                    if h is not None:
                        cpu.syscall_pc = pc
                        h(cpu, code)            # the hook emulates the kernel call; go on after it
                        return None
                    num = sx32(g[3])
                    raise EmuError('syscall %d ($v1 = %d) at %08X' % (code, num, pc), 'syscall %d' % num)
                return syscall
            if fn == 0x0D:
                code = (w >> 6) & 0xFFFFF

                def brk():
                    raise EmuError('break %d at %08X' % (code, pc), 'break %d' % code)
                return brk
            if fn == 0x0F:                                                    # sync.l / sync.p: events
                sname = 'sync.p' if (w >> 6) & 0x10 else 'sync.l'
                io = self.io

                def sync():
                    io.op(sname)
                return sync
            if fn == 0x10: return W(rd, lambda: cpu.hi)
            if fn == 0x12: return W(rd, lambda: cpu.lo)
            if fn == 0x11:
                def mthi():
                    cpu.hi = g[rs]
                return mthi
            if fn == 0x13:
                def mtlo():
                    cpu.lo = g[rs]
                return mtlo
            if fn == 0x14: return W(rd, lambda: u64(g[rt] << (g[rs] & 63)))   # dsllv
            if fn == 0x16: return W(rd, lambda: u64(g[rt] >> (g[rs] & 63)))   # dsrlv
            if fn == 0x17: return W(rd, lambda: u64(s64(g[rt]) >> (g[rs] & 63)))
            if fn in (0x18, 0x19):                                            # mult / multu (EE: rd = lo)
                def mult():
                    if fn == 0x18:
                        p = sx32(g[rs]) * sx32(g[rt])
                    else:
                        p = (g[rs] & 0xFFFFFFFF) * (g[rt] & 0xFFFFFFFF)
                    cpu.lo = sext32(p)
                    cpu.hi = sext32(p >> 32)
                    if rd:
                        g[rd] = cpu.lo
                return mult
            if fn in (0x1A, 0x1B):
                def div():
                    if fn == 0x1A:
                        a, b = sx32(g[rs]), sx32(g[rt])
                    else:
                        a, b = g[rs] & 0xFFFFFFFF, g[rt] & 0xFFFFFFFF
                    if b == 0:
                        raise EmuError('division by zero (result undefined) at %08X' % pc, 'div0')
                    q = abs(a) // abs(b)
                    if (a < 0) != (b < 0):
                        q = -q
                    cpu.lo = sext32(q)
                    cpu.hi = sext32(a - q * b)
                return div
            if fn == 0x20:
                def add():
                    r = sx32(g[rs]) + sx32(g[rt])
                    if not -0x80000000 <= r <= 0x7FFFFFFF:
                        raise EmuError('add overflow at %08X' % pc, 'overflow')
                    if rd:
                        g[rd] = sext32(r)
                return add
            if fn == 0x21: return W(rd, lambda: sext32(g[rs] + g[rt]))
            if fn == 0x22:
                def sub():
                    r = sx32(g[rs]) - sx32(g[rt])
                    if not -0x80000000 <= r <= 0x7FFFFFFF:
                        raise EmuError('sub overflow at %08X' % pc, 'overflow')
                    if rd:
                        g[rd] = sext32(r)
                return sub
            if fn == 0x23: return W(rd, lambda: sext32(g[rs] - g[rt]))
            if fn == 0x24: return W(rd, lambda: g[rs] & g[rt])
            if fn == 0x25: return W(rd, lambda: g[rs] | g[rt])
            if fn == 0x26: return W(rd, lambda: g[rs] ^ g[rt])
            if fn == 0x27: return W(rd, lambda: u64(~(g[rs] | g[rt])))
            if fn == 0x28: return W(rd, lambda: cpu.sa)                       # mfsa
            if fn == 0x29:                                                    # mtsa
                def mtsa():
                    cpu.sa = g[rs] & 0xF
                return mtsa
            if fn == 0x2A: return W(rd, lambda: 1 if s64(g[rs]) < s64(g[rt]) else 0)
            if fn == 0x2B: return W(rd, lambda: 1 if g[rs] < g[rt] else 0)
            if fn in (0x2C, 0x2E):                                            # dadd / dsub (trap)
                def daddsub():
                    if fn == 0x2C:
                        r = s64(g[rs]) + s64(g[rt])
                    else:
                        r = s64(g[rs]) - s64(g[rt])
                    if not -(1 << 63) <= r < (1 << 63):
                        raise EmuError('%s overflow at %08X' % ('dadd' if fn == 0x2C else 'dsub', pc), 'overflow')
                    if rd:
                        g[rd] = r & M64
                return daddsub
            if fn == 0x2D: return W(rd, lambda: u64(g[rs] + g[rt]))           # daddu
            if fn == 0x2F: return W(rd, lambda: u64(g[rs] - g[rt]))           # dsubu
            if 0x30 <= fn <= 0x36 and fn != 0x35:                             # tge tgeu tlt tltu teq tne
                def tr():
                    a, b = s64(g[rs]), s64(g[rt])
                    ua, ub = g[rs], g[rt]
                    c = {0x30: a >= b, 0x31: ua >= ub, 0x32: a < b, 0x33: ua < ub, 0x34: a == b, 0x36: a != b}[fn]
                    if c:
                        raise EmuError('trap at %08X' % pc, 'trap')
                return tr
            if fn == 0x38: return W(rd, lambda: u64(g[rt] << sa))
            if fn == 0x3A: return W(rd, lambda: u64(g[rt] >> sa))
            if fn == 0x3B: return W(rd, lambda: u64(s64(g[rt]) >> sa))
            if fn == 0x3C: return W(rd, lambda: u64(g[rt] << (sa + 32)))
            if fn == 0x3E: return W(rd, lambda: u64(g[rt] >> (sa + 32)))
            if fn == 0x3F: return W(rd, lambda: u64(s64(g[rt]) >> (sa + 32)))
            raise bad

        # ------------------------------------------------ REGIMM
        if op == 1:
            if rt in (0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13):
                lt = rt & 1 == 0
                lk = rt in (2, 3, 0x12, 0x13)
                link = rt >= 0x10

                def regimm():
                    v = s64(g[rs])
                    if link:
                        g[31] = pc + 8
                    c = v < 0 if lt else v >= 0
                    if c:
                        return br
                    return LIKELY_SKIP if lk else None
                return regimm
            if 8 <= rt <= 14 and rt != 13:                                    # tgei tgeiu tlti tltiu teqi tnei
                def tri():
                    a = s64(g[rs])
                    ua = g[rs]
                    ub = simm & M64
                    c = {8: a >= simm, 9: ua >= ub, 10: a < simm, 11: ua < ub, 12: a == simm, 14: a != simm}[rt]
                    if c:
                        raise EmuError('trap at %08X' % pc, 'trap')
                return tri
            if rt == 0x18:                                                    # mtsab
                def mtsab():
                    cpu.sa = (g[rs] ^ imm) & 0xF
                return mtsab
            if rt == 0x19:                                                    # mtsah
                def mtsah():
                    cpu.sa = ((g[rs] ^ imm) & 0x7) << 1
                return mtsah
            raise bad

        if op == 2:
            t = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
            return lambda: t
        if op == 3:
            t = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)

            def jal():
                g[31] = pc + 8
                return t
            return jal
        if op in (4, 5, 6, 7, 0x14, 0x15, 0x16, 0x17):
            base = op & 3
            lk = op >= 0x14
            if base >= 2 and rt != 0:
                raise bad

            def bra():
                a = s64(g[rs])
                if base == 0:
                    c = a == s64(g[rt])
                elif base == 1:
                    c = a != s64(g[rt])
                elif base == 2:
                    c = a <= 0
                else:
                    c = a > 0
                if c:
                    return br
                return LIKELY_SKIP if lk else None
            return bra
        if op == 0x08:
            def addi():
                r = sx32(g[rs]) + simm
                if not -0x80000000 <= r <= 0x7FFFFFFF:
                    raise EmuError('addi overflow at %08X' % pc, 'overflow')
                if rt:
                    g[rt] = sext32(r)
            return addi
        if op == 0x09: return W(rt, lambda: sext32(g[rs] + simm))
        if op == 0x0A: return W(rt, lambda: 1 if s64(g[rs]) < simm else 0)
        if op == 0x0B: return W(rt, lambda: 1 if g[rs] < u64(simm) else 0)
        if op == 0x0C: return W(rt, lambda: g[rs] & imm)
        if op == 0x0D: return W(rt, lambda: g[rs] | imm)
        if op == 0x0E: return W(rt, lambda: g[rs] ^ imm)
        if op == 0x0F:
            v = sext32(imm << 16)
            return W(rt, lambda: v)
        if op == 0x18:                                                        # daddi (trap)
            def daddi():
                r = s64(g[rs]) + simm
                if not -(1 << 63) <= r < (1 << 63):
                    raise EmuError('daddi overflow at %08X' % pc, 'overflow')
                if rt:
                    g[rt] = r & M64
            return daddi
        if op == 0x19: return W(rt, lambda: u64(g[rs] + simm))               # daddiu

        def ea():
            return (g[rs] + simm) & 0xFFFFFFFF

        if op == 0x20: return W(rt, lambda: u64(sx8(mem.u8(ea()))))
        if op == 0x24: return W(rt, lambda: mem.u8(ea()))
        if op == 0x21: return W(rt, lambda: u64(sx16(mem.u16(ea()))))
        if op == 0x25: return W(rt, lambda: mem.u16(ea()))
        if op == 0x23: return W(rt, lambda: sext32(mem.u32(ea())))
        if op == 0x27: return W(rt, lambda: mem.u32(ea()))
        if op == 0x37: return W(rt, lambda: mem.u64(ea()))
        if op == 0x28:
            def sb():
                mem.w8(ea(), g[rt])
            return sb
        if op == 0x29:
            def sh():
                mem.w16(ea(), g[rt])
            return sh
        if op == 0x2B:
            def sw():
                mem.w32(ea(), g[rt])
            return sw
        if op == 0x3F:
            def sd():
                mem.w64(ea(), g[rt])
            return sd
        if op in (0x22, 0x26):            # lwl / lwr (little endian)
            left = op == 0x22

            def lwlr():
                a = ea()
                word = mem.u32(a & ~3)
                k = a & 3
                old = g[rt] & 0xFFFFFFFF
                if left:
                    sh_ = 8 * (3 - k)
                    v = ((word << sh_) & 0xFFFFFFFF) | (old & ((1 << sh_) - 1))
                    if rt:
                        g[rt] = sext32(v)          # bit 31 always comes from memory
                else:
                    sh_ = 8 * k
                    mask = 0xFFFFFFFF >> sh_
                    v = (old & ~mask & 0xFFFFFFFF) | (word >> sh_)
                    if rt:
                        if k == 0:
                            g[rt] = sext32(v)      # whole word loaded
                        else:
                            g[rt] = (g[rt] & 0xFFFFFFFF00000000) | v
            return lwlr
        if op in (0x1A, 0x1B):            # ldl / ldr
            left = op == 0x1A

            def ldlr():
                a = ea()
                dw = mem.u64(a & ~7)
                k = a & 7
                old = g[rt]
                if left:
                    sh_ = 8 * (7 - k)
                    v = ((dw << sh_) & M64) | (old & ((1 << sh_) - 1))
                else:
                    sh_ = 8 * k
                    mask = (M64 >> sh_)
                    v = (old & ~mask & M64) | (dw >> sh_)
                if rt:
                    g[rt] = v
            return ldlr
        if op in (0x2A, 0x2E, 0x2C, 0x2D):   # swl / swr / sdl / sdr
            def sxlr():
                a = ea()
                n = 4 if op in (0x2A, 0x2E) else 8
                base_ = a & ~(n - 1)
                k = a & (n - 1)
                v = g[rt]
                cur = bytearray(mem.rd(base_, n))
                vb = v.to_bytes(8, 'little')[:n]
                if op in (0x2A, 0x2C):   # left: the high k+1 bytes of rt at base..a
                    for i in range(k + 1):
                        cur[i] = vb[n - 1 - k + i]
                else:                    # right: the low n-k bytes of rt at a..end
                    for i in range(n - k):
                        cur[k + i] = vb[i]
                mem.wr(base_, bytes(cur))
            return sxlr
        if op == 0x1E:                    # lq (low 4 address bits ignored)
            def lq():
                v = mem.u128(ea() & ~15)
                if rt:
                    g[rt] = v & M64
                    gh[rt] = v >> 64
            return lq
        if op == 0x1F:                    # sq
            def sq():
                mem.w128(ea() & ~15, g[rt] | (gh[rt] << 64))
            return sq
        if op == 0x31:
            def lwc1():
                f[rt] = mem.u32(ea())
            return lwc1
        if op == 0x39:
            def swc1():
                mem.w32(ea(), f[rt])
            return swc1
        if op == 0x2F:                    # cache: an event (op, effective address)
            io = self.io

            def cache():
                io.cache(rt, ea())
            return cache
        if op == 0x33: return nop          # pref
        if op == 0x36:                    # lqc2 (VU0 vf <- memory)
            io = self.io

            def lqc2():
                v = mem.u128(ea() & ~15)
                if rt:
                    io.vf[rt] = [(v >> s) & M32 for s in (0, 32, 64, 96)]
            return lqc2
        if op == 0x3E:                    # sqc2
            io = self.io

            def sqc2():
                x = io.vf[rt]
                mem.w128(ea() & ~15, x[0] | (x[1] << 32) | (x[2] << 64) | (x[3] << 96))
            return sqc2

        # ------------------------------------------------ COP0 / COP1 / COP2
        if op == 0x10:
            return self._decode_cop0(pc, w, bad, W)
        if op == 0x11:
            return self._decode_cop1(pc, w, bad, W)
        if op == 0x12:
            return self._decode_cop2(pc, w, bad, W)

        # ------------------------------------------------ MMI
        if op == 0x1C:
            return self._decode_mmi(pc, w, bad, W)
        raise bad

    def _decode_cop0(self, pc, w, bad, W):
        """MFC0 / MTC0 (with the R5900 MFPS/MFPC/MTPS/MTPC/BPC forms of $24/$25), DI,
        EI, ERET; the state and events are in the IoModel."""
        g, io, cpu = self.g, self.io, self
        rs = (w >> 21) & 31
        rt = (w >> 16) & 31
        rd = (w >> 11) & 31
        if rs == 0x00:                                                    # mfc0
            name = cop0_name(rd, w & 0x7FF)
            return W(rt, lambda: sext32(io.cop0_read(name)))
        if rs == 0x04:                                                    # mtc0
            name = cop0_name(rd, w & 0x7FF)

            def mtc0():
                io.cop0_write(name, g[rt])
            return mtc0
        if rs == 0x10:
            fn = w & 63
            if fn in (0x38, 0x39):                                        # ei / di
                nm = 'ei' if fn == 0x38 else 'di'

                def eidi():
                    io.op(nm)
                return eidi
            if fn == 0x18:                                                # eret (no delay slot)
                def eret():
                    cpu._eret_pc = io.eret()
                    return ERET_JUMP
                return eret
        raise bad

    def _decode_cop2(self, pc, w, bad, W):
        """VU0 in macro mode: QMFC2 / CFC2 / QMTC2 / CTC2 and the COP2 vector
        instructions (see vu0_macro).  The register file is io.vf / io.vi / io.acc."""
        g, gh, io = self.g, self.gh, self.io
        rs = (w >> 21) & 31
        rt = (w >> 16) & 31
        rd = (w >> 11) & 31
        if rs < 0x10:
            if rs == 0x01:                                                # qmfc2
                def qmfc2():
                    x = io.vf[rd]
                    if rt:
                        g[rt] = x[0] | (x[1] << 32)
                        gh[rt] = x[2] | (x[3] << 32)
                return qmfc2
            if rs == 0x05:                                                # qmtc2
                def qmtc2():
                    if rd:
                        io.vf[rd] = [g[rt] & M32, g[rt] >> 32, gh[rt] & M32, gh[rt] >> 32]
                return qmtc2
            if rs == 0x02:                                                # cfc2
                return W(rt, lambda: sext32(io.vi_read(rd)))
            if rs == 0x06:                                                # ctc2
                def ctc2():
                    io.vi_write(rd, g[rt] & M32)
                return ctc2
            raise bad
        f = vu0_macro(io, w)
        if f is None:
            raise bad
        return f

    def _decode_cop1(self, pc, w, bad, W):
        g, f, cpu = self.g, self.f, self
        rs = (w >> 21) & 31
        rt = (w >> 16) & 31
        rd = (w >> 11) & 31
        sa = (w >> 6) & 31
        fn = w & 63
        simm = sx16(w & 0xFFFF)
        br = (pc + 4 + (simm << 2)) & 0xFFFFFFFF
        fmt = rs
        ft, fs, fd = rt, rd, sa
        if fmt == 0x00:
            return W(rt, lambda: sext32(f[fs]))                           # mfc1
        if fmt == 0x04:
            def mtc1():
                f[fs] = g[rt] & 0xFFFFFFFF
            return mtc1
        if fmt == 0x02:                                                   # cfc1
            if fs != 31:
                raise bad

            def cfc1():
                if rt:
                    g[rt] = 0x00800000 if cpu.cond else 0
            return cfc1
        if fmt == 0x06:                                                   # ctc1
            if fs != 31:
                raise bad

            def ctc1():
                cpu.cond = bool(g[rt] & 0x00800000)
            return ctc1
        if fmt == 0x08:
            tf = rt & 1
            lk = (rt >> 1) & 1

            def bc1():
                if (1 if cpu.cond else 0) == tf:
                    return br
                return LIKELY_SKIP if lk else None
            return bc1
        if fmt == 0x10:
            if fn == 0x00:
                def add_s():
                    f[fd] = eadd(f[fs], f[ft])
                return add_s
            if fn == 0x01:
                def sub_s():
                    f[fd] = esub(f[fs], f[ft])
                return sub_s
            if fn == 0x02:
                def mul_s():
                    f[fd] = emul(f[fs], f[ft])
                return mul_s
            if fn == 0x03:
                def div_s():
                    f[fd] = ediv(f[fs], f[ft])
                return div_s
            if fn == 0x04:
                def sqrt_s():
                    f[fd] = esqrt(f[ft])   # EE encodes the source in ft
                return sqrt_s
            if fn == 0x05:
                def abs_s():
                    f[fd] = f[fs] & 0x7FFFFFFF
                return abs_s
            if fn == 0x06:
                def mov_s():
                    f[fd] = f[fs]
                return mov_s
            if fn == 0x07:
                def neg_s():
                    f[fd] = f[fs] ^ 0x80000000
                return neg_s
            if fn == 0x24:
                def cvt_w_s():
                    f[fd] = ecvt_w_s(f[fs])
                return cvt_w_s
            if fn == 0x18:
                def adda():
                    cpu.acc = eadd(f[fs], f[ft])
                return adda
            if fn == 0x19:
                def suba():
                    cpu.acc = esub(f[fs], f[ft])
                return suba
            if fn == 0x1A:
                def mula():
                    cpu.acc = emul(f[fs], f[ft])
                return mula
            if fn == 0x1C:
                def madd():
                    f[fd] = eadd(cpu.acc, emul(f[fs], f[ft]))
                return madd
            if fn == 0x1D:
                def msub():
                    f[fd] = esub(cpu.acc, emul(f[fs], f[ft]))
                return msub
            if fn == 0x1E:
                def madda():
                    cpu.acc = eadd(cpu.acc, emul(f[fs], f[ft]))
                return madda
            if fn == 0x1F:
                def msuba():
                    cpu.acc = esub(cpu.acc, emul(f[fs], f[ft]))
                return msuba
            if fn == 0x28:
                def max_s():
                    f[fd] = f[fs] if fkey(f[fs]) >= fkey(f[ft]) else f[ft]
                return max_s
            if fn == 0x29:
                def min_s():
                    f[fd] = f[fs] if fkey(f[fs]) <= fkey(f[ft]) else f[ft]
                return min_s
            if fn == 0x30:
                def c_f():
                    cpu.cond = False
                return c_f
            if fn == 0x32:
                def c_eq():
                    cpu.cond = fkey(f[fs]) == fkey(f[ft])
                return c_eq
            if fn == 0x34:
                def c_lt():
                    cpu.cond = fkey(f[fs]) < fkey(f[ft])
                return c_lt
            if fn == 0x36:
                def c_le():
                    cpu.cond = fkey(f[fs]) <= fkey(f[ft])
                return c_le
            raise bad
        if fmt == 0x14 and fn == 0x20:
            def cvt_s_w():
                f[fd] = ecvt_s_w(f[fs])
            return cvt_s_w
        raise bad

    # -------------------------------------------------------------- MMI
    def _decode_mmi(self, pc, w, bad, W):
        g, gh, cpu = self.g, self.gh, self
        rs = (w >> 21) & 31
        rt = (w >> 16) & 31
        rd = (w >> 11) & 31
        sa = (w >> 6) & 31
        fn = w & 63

        def Q(fnc):
            """rd (128) = fnc(rs128, rt128)."""
            def f():
                r = fnc(g[rs] | (gh[rs] << 64), g[rt] | (gh[rt] << 64))
                if rd:
                    g[rd] = r & M64
                    gh[rd] = r >> 64
            return f

        def QH(fnc):
            """rd (128, or None = no write) = fnc(rs128, rt128) with access to HI/LO."""
            def f():
                r = fnc(g[rs] | (gh[rs] << 64), g[rt] | (gh[rt] << 64))
                if rd and r is not None:
                    g[rd] = r & M64
                    gh[rd] = r >> 64
            return f

        # ---- base MMI functions
        if fn in (0x00, 0x01, 0x20, 0x21):                    # madd maddu madd1 maddu1
            signed = fn in (0x00, 0x20)
            pipe1 = fn >= 0x20

            def madd():
                if pipe1:
                    acc = ((cpu.hi1 & M32) << 32) | (cpu.lo1 & M32)
                else:
                    acc = ((cpu.hi & M32) << 32) | (cpu.lo & M32)
                if signed:
                    acc = s64(acc) + sx32(g[rs]) * sx32(g[rt])
                else:
                    acc = acc + (g[rs] & M32) * (g[rt] & M32)
                acc &= M64
                lo_, hi_ = sext32(acc), sext32(acc >> 32)
                if pipe1:
                    cpu.lo1, cpu.hi1 = lo_, hi_
                else:
                    cpu.lo, cpu.hi = lo_, hi_
                if rd:
                    g[rd] = lo_
            return madd
        if fn == 0x04:                                        # plzcw
            def plzcw():
                v = g[rs]
                r = _plzcw_word(v & M32) | (_plzcw_word(v >> 32) << 32)
                if rd:
                    g[rd] = r
            return plzcw
        if fn == 0x10: return W(rd, lambda: cpu.hi1)          # mfhi1
        if fn == 0x12: return W(rd, lambda: cpu.lo1)          # mflo1
        if fn == 0x11:
            def mthi1():
                cpu.hi1 = g[rs]
            return mthi1
        if fn == 0x13:
            def mtlo1():
                cpu.lo1 = g[rs]
            return mtlo1
        if fn in (0x18, 0x19):                                # mult1 / multu1
            def mult1():
                if fn == 0x18:
                    p = sx32(g[rs]) * sx32(g[rt])
                else:
                    p = (g[rs] & M32) * (g[rt] & M32)
                cpu.lo1 = sext32(p)
                cpu.hi1 = sext32(p >> 32)
                if rd:
                    g[rd] = cpu.lo1
            return mult1
        if fn in (0x1A, 0x1B):                                # div1 / divu1
            def div1():
                if fn == 0x1A:
                    a, b = sx32(g[rs]), sx32(g[rt])
                else:
                    a, b = g[rs] & M32, g[rt] & M32
                if b == 0:
                    raise EmuError('division by zero (result undefined) at %08X' % pc, 'div0')
                q = abs(a) // abs(b)
                if (a < 0) != (b < 0):
                    q = -q
                cpu.lo1 = sext32(q)
                cpu.hi1 = sext32(a - q * b)
            return div1
        if fn == 0x30:                                        # pmfhl.fmt
            if sa > 4:
                raise bad

            def pmfhl(a, b):
                LO, HI = lanes(cpu.lo128(), 32), lanes(cpu.hi128(), 32)
                if sa == 0:                                   # .lw
                    return pack([LO[0], HI[0], LO[2], HI[2]], 32)
                if sa == 1:                                   # .uw
                    return pack([LO[1], HI[1], LO[3], HI[3]], 32)
                if sa == 2:                                   # .slw
                    out = []
                    for i in (0, 2):
                        x = s64((HI[i] << 32) | LO[i])
                        out.append(_sat(x, -0x80000000, 0x7FFFFFFF) & M64)
                    return pack(out, 64)
                if sa == 3:                                   # .lh
                    L, H = lanes(cpu.lo128(), 16), lanes(cpu.hi128(), 16)
                    return pack([L[0], L[2], H[0], H[2], L[4], L[6], H[4], H[6]], 16)
                LS, HS = slanes(cpu.lo128(), 32), slanes(cpu.hi128(), 32)   # .sh
                return pack([_sat(x, -0x8000, 0x7FFF) for x in
                             (LS[0], LS[1], HS[0], HS[1], LS[2], LS[3], HS[2], HS[3])], 16)
            return QH(pmfhl)
        if fn == 0x31:                                        # pmthl.lw
            if sa != 0:
                raise bad

            def pmthl():
                x = lanes(g[rs] | (gh[rs] << 64), 32)
                LO, HI = lanes(cpu.lo128(), 32), lanes(cpu.hi128(), 32)
                LO[0], HI[0], LO[2], HI[2] = x[0], x[1], x[2], x[3]
                cpu.set_lo128(pack(LO, 32))
                cpu.set_hi128(pack(HI, 32))
            return pmthl
        if fn in (0x34, 0x36, 0x37):                          # psllh psrlh psrah
            s = sa & 0xF
            if fn == 0x34:
                return Q(lambda a, b: pack([x << s for x in lanes(b, 16)], 16))
            if fn == 0x36:
                return Q(lambda a, b: pack([x >> s for x in lanes(b, 16)], 16))
            return Q(lambda a, b: pack([x >> s for x in slanes(b, 16)], 16))
        if fn in (0x3C, 0x3E, 0x3F):                          # psllw psrlw psraw
            s = sa
            if fn == 0x3C:
                return Q(lambda a, b: pack([x << s for x in lanes(b, 32)], 32))
            if fn == 0x3E:
                return Q(lambda a, b: pack([x >> s for x in lanes(b, 32)], 32))
            return Q(lambda a, b: pack([x >> s for x in slanes(b, 32)], 32))

        # ---- MMI0
        if fn == 0x08:
            op = MMI0.get(sa)
            if op is None:
                raise bad
            return Q(op)
        # ---- MMI1
        if fn == 0x28:
            if sa == 0x1B:                                    # qfsrv
                return Q(lambda a, b: (((a << 128) | b) >> (8 * cpu.sa)) & M128)
            op = MMI1.get(sa)
            if op is None:
                raise bad
            return Q(op)
        # ---- MMI2
        if fn == 0x09:
            op = MMI2.get(sa)
            if op is not None:
                return Q(op)
            h = MMI2_HL.get(sa)
            if h is None:
                raise bad
            return QH(lambda a, b: h(cpu, a, b, pc))
        # ---- MMI3
        if fn == 0x29:
            op = MMI3.get(sa)
            if op is not None:
                return Q(op)
            h = MMI3_HL.get(sa)
            if h is None:
                raise bad
            return QH(lambda a, b: h(cpu, a, b, pc))
        raise bad


# ------------------------------------------------------------------ MMI sub-op tables
# Pure lane operations: fn(rs128, rt128) -> rd128.

def _cmp_mask(w, pred, signed):
    m = (1 << w) - 1
    return _mk_lane(w, lambda x, y: m if pred(x, y) else 0, signed)


MMI0 = {
    0x00: _mk_lane(32, lambda x, y: x + y, False),         # paddw
    0x01: _mk_lane(32, lambda x, y: x - y, False),         # psubw
    0x02: _cmp_mask(32, lambda x, y: x > y, True),         # pcgtw
    0x03: _mk_lane(32, max, True),                         # pmaxw
    0x04: _mk_lane(16, lambda x, y: x + y, False),         # paddh
    0x05: _mk_lane(16, lambda x, y: x - y, False),         # psubh
    0x06: _cmp_mask(16, lambda x, y: x > y, True),         # pcgth
    0x07: _mk_lane(16, max, True),                         # pmaxh
    0x08: _mk_lane(8, lambda x, y: x + y, False),          # paddb
    0x09: _mk_lane(8, lambda x, y: x - y, False),          # psubb
    0x0A: _cmp_mask(8, lambda x, y: x > y, True),          # pcgtb
    0x10: _add_sat_s(32),                                  # paddsw
    0x11: _sub_sat_s(32),                                  # psubsw
    0x12: _ext_low(32),                                    # pextlw
    0x13: _pac(32),                                        # ppacw
    0x14: _add_sat_s(16),                                  # paddsh
    0x15: _sub_sat_s(16),                                  # psubsh
    0x16: _ext_low(16),                                    # pextlh
    0x17: _pac(16),                                        # ppach
    0x18: _add_sat_s(8),                                   # paddsb
    0x19: _sub_sat_s(8),                                   # psubsb
    0x1A: _ext_low(8),                                     # pextlb
    0x1B: _pac(8),                                         # ppacb
    0x1E: lambda a, b: _pext5(b),                          # pext5
    0x1F: lambda a, b: _ppac5(b),                          # ppac5
}

MMI1 = {
    0x01: _abs_sat(32),                                    # pabsw
    0x02: _cmp_mask(32, lambda x, y: x == y, False),       # pceqw
    0x03: _mk_lane(32, min, True),                         # pminw
    0x04: lambda a, b: pack([x - y for x, y in zip(lanes(a, 16)[:4], lanes(b, 16)[:4])] +
                            [x + y for x, y in zip(lanes(a, 16)[4:], lanes(b, 16)[4:])], 16),   # padsbh
    0x05: _abs_sat(16),                                    # pabsh
    0x06: _cmp_mask(16, lambda x, y: x == y, False),       # pceqh
    0x07: _mk_lane(16, min, True),                         # pminh
    0x0A: _pceqb,                                          # pceqb
    0x10: _add_sat_u(32),                                  # padduw
    0x11: _sub_sat_u(32),                                  # psubuw
    0x12: _ext_up(32),                                     # pextuw
    0x14: _add_sat_u(16),                                  # padduh
    0x15: _sub_sat_u(16),                                  # psubuh
    0x16: _ext_up(16),                                     # pextuh
    0x18: _add_sat_u(8),                                   # paddub
    0x19: _sub_sat_u(8),                                   # psubub
    0x1A: _ext_up(8),                                      # pextub
    # 0x1B qfsrv needs SA: decoded directly
}


def _perm(w, idx):
    """rd lane i = rt lane idx[i]."""
    def op(a, b):
        lb = lanes(b, w)
        return pack([lb[i] for i in idx], w)
    return op


def _vshift(kind):
    """psllvw / psrlvw / psravw: words 0 and 2 of rt shifted by rs words 0 and 2,
    each 32-bit result sign-extended into a 64-bit lane."""
    def op(a, b):
        la, lb = lanes(a, 32), lanes(b, 32)
        out = []
        for i in (0, 2):
            s = la[i] & 31
            if kind == 'l':
                r = (lb[i] << s) & M32
            elif kind == 'r':
                r = lb[i] >> s
            else:
                r = (sx32(lb[i]) >> s) & M32
            out.append(sext32(r))
        return pack(out, 64)
    return op


MMI2 = {
    0x02: _vshift('l'),                                    # psllvw
    0x03: _vshift('r'),                                    # psrlvw
    0x0A: lambda a, b: pack([v for i in range(4) for v in (lanes(b, 16)[i], lanes(a, 16)[4 + i])], 16),  # pinth
    0x0E: lambda a, b: (b & M64) | ((a & M64) << 64),      # pcpyld
    0x12: lambda a, b: a & b,                              # pand
    0x13: lambda a, b: a ^ b,                              # pxor
    0x1A: _perm(16, [2, 1, 0, 3, 6, 5, 4, 7]),             # pexeh
    0x1B: _perm(16, [3, 2, 1, 0, 7, 6, 5, 4]),             # prevh
    0x1E: _perm(32, [2, 1, 0, 3]),                         # pexew
    0x1F: _perm(32, [1, 2, 0, 3]),                         # prot3w
}

MMI3 = {
    0x03: _vshift('a'),                                    # psravw
    0x0A: lambda a, b: pack([v for i in range(4) for v in (lanes(b, 16)[2 * i], lanes(a, 16)[2 * i])], 16),  # pinteh
    0x0E: lambda a, b: (a >> 64) | ((b >> 64) << 64),      # pcpyud
    0x12: lambda a, b: a | b,                              # por
    0x13: lambda a, b: ~(a | b) & M128,                    # pnor
    0x1A: _perm(16, [0, 2, 1, 3, 4, 6, 5, 7]),             # pexch
    0x1B: _perm(16, [0, 0, 0, 0, 4, 4, 4, 4]),             # pcpyh
    0x1E: _perm(32, [0, 2, 1, 3]),                         # pexcw
}


# Operations that read/write HI/LO: fn(cpu, rs128, rt128, pc) -> rd128 or None.

def _wmul(cpu, a, b, pc, signed, mode):
    """pmultw / pmultuw / pmaddw / pmadduw / pmsubw on words 0 and 2."""
    la, lb = lanes(a, 32), lanes(b, 32)
    LO, HI = lanes(cpu.lo128(), 64), lanes(cpu.hi128(), 64)
    out = []
    for k, i in enumerate((0, 2)):
        if signed:
            p = sx32(la[i]) * sx32(lb[i])
        else:
            p = la[i] * lb[i]
        if mode:
            acc = ((HI[k] & M32) << 32) | (LO[k] & M32)
            if signed:
                acc = s64(acc)
            p = acc + p if mode > 0 else acc - p
        p &= M64
        LO[k] = sext32(p)
        HI[k] = sext32(p >> 32)
        out.append(p)
    cpu.set_lo128(pack(LO, 64))
    cpu.set_hi128(pack(HI, 64))
    return pack(out, 64)


def _wdiv(cpu, a, b, pc, signed):
    """pdivw / pdivuw on words 0 and 2 (rd is not written)."""
    la, lb = lanes(a, 32), lanes(b, 32)
    LO, HI = lanes(cpu.lo128(), 64), lanes(cpu.hi128(), 64)
    for k, i in enumerate((0, 2)):
        if signed:
            x, y = sx32(la[i]), sx32(lb[i])
        else:
            x, y = la[i], lb[i]
        if y == 0:
            raise EmuError('division by zero (result undefined) at %08X' % pc, 'div0')
        q = abs(x) // abs(y)
        if (x < 0) != (y < 0):
            q = -q
        LO[k] = sext32(q)
        HI[k] = sext32(x - q * y)
    cpu.set_lo128(pack(LO, 64))
    cpu.set_hi128(pack(HI, 64))
    return None


def _pdivbw(cpu, a, b, pc):
    """each word of rs / halfword 0 of rt (signed); remainders sign-extended."""
    d = sx16(b & 0xFFFF)
    if d == 0:
        raise EmuError('division by zero (result undefined) at %08X' % pc, 'div0')
    L, H = [], []
    for x in slanes(a, 32):
        q = abs(x) // abs(d)
        if (x < 0) != (d < 0):
            q = -q
        L.append(q)
        H.append(sx16(x - q * d))
    cpu.set_lo128(pack(L, 32))
    cpu.set_hi128(pack(H, 32))
    return None


def _hmul(cpu, a, b, pc, mode):
    """pmulth (mode 0) / pmaddh (+1) / pmsubh (-1): 8 halfword products into
    LO.w0 LO.w1 HI.w0 HI.w1 LO.w2 LO.w3 HI.w2 HI.w3; rd = LO.w0 HI.w0 LO.w2 HI.w2."""
    pa, pb = slanes(a, 16), slanes(b, 16)
    LO, HI = lanes(cpu.lo128(), 32), lanes(cpu.hi128(), 32)
    dest = [(LO, 0), (LO, 1), (HI, 0), (HI, 1), (LO, 2), (LO, 3), (HI, 2), (HI, 3)]
    for i, (reg, j) in enumerate(dest):
        p = pa[i] * pb[i]
        if mode > 0:
            p = sx32(reg[j]) + p
        elif mode < 0:
            p = sx32(reg[j]) - p
        reg[j] = p & M32
    cpu.set_lo128(pack(LO, 32))
    cpu.set_hi128(pack(HI, 32))
    return pack([LO[0], HI[0], LO[2], HI[2]], 32)


def _hhmul(cpu, a, b, pc, sign):
    """phmadh (+1) / phmsbh (-1): rd.w[i] = rs.h[2i+1]*rt.h[2i+1] +- rs.h[2i]*rt.h[2i];
    LO.w0 = rd.w0, HI.w0 = rd.w1, LO.w2 = rd.w2, HI.w2 = rd.w3.  LO/HI words 1 and 3
    are undefined in the EE manual; they are left unchanged here."""
    pa, pb = slanes(a, 16), slanes(b, 16)
    r = []
    for i in range(4):
        hi_p = pa[2 * i + 1] * pb[2 * i + 1]
        lo_p = pa[2 * i] * pb[2 * i]
        r.append((hi_p + lo_p if sign > 0 else hi_p - lo_p) & M32)
    LO, HI = lanes(cpu.lo128(), 32), lanes(cpu.hi128(), 32)
    LO[0], HI[0], LO[2], HI[2] = r[0], r[1], r[2], r[3]
    cpu.set_lo128(pack(LO, 32))
    cpu.set_hi128(pack(HI, 32))
    return pack(r, 32)


def _pmfhi(cpu, a, b, pc):
    return cpu.hi128()


def _pmflo(cpu, a, b, pc):
    return cpu.lo128()


def _pmthi(cpu, a, b, pc):
    cpu.set_hi128(a)
    return None


def _pmtlo(cpu, a, b, pc):
    cpu.set_lo128(a)
    return None


MMI2_HL = {
    0x00: lambda c, a, b, pc: _wmul(c, a, b, pc, True, +1),   # pmaddw
    0x04: lambda c, a, b, pc: _wmul(c, a, b, pc, True, -1),   # pmsubw
    0x08: _pmfhi,                                             # pmfhi
    0x09: _pmflo,                                             # pmflo
    0x0C: lambda c, a, b, pc: _wmul(c, a, b, pc, True, 0),    # pmultw
    0x0D: lambda c, a, b, pc: _wdiv(c, a, b, pc, True),       # pdivw
    0x10: lambda c, a, b, pc: _hmul(c, a, b, pc, +1),         # pmaddh
    0x11: lambda c, a, b, pc: _hhmul(c, a, b, pc, +1),        # phmadh
    0x14: lambda c, a, b, pc: _hmul(c, a, b, pc, -1),         # pmsubh
    0x15: lambda c, a, b, pc: _hhmul(c, a, b, pc, -1),        # phmsbh
    0x1C: lambda c, a, b, pc: _hmul(c, a, b, pc, 0),          # pmulth
    0x1D: _pdivbw,                                            # pdivbw
}

MMI3_HL = {
    0x00: lambda c, a, b, pc: _wmul(c, a, b, pc, False, +1),  # pmadduw
    0x08: _pmthi,                                             # pmthi
    0x09: _pmtlo,                                             # pmtlo
    0x0C: lambda c, a, b, pc: _wmul(c, a, b, pc, False, 0),   # pmultuw
    0x0D: lambda c, a, b, pc: _wdiv(c, a, b, pc, False),      # pdivuw
}

# mnemonic tables (for the self-test's decode cross-check and for messages)
MMI_BASE_NAMES = {0x00: 'madd', 0x01: 'maddu', 0x04: 'plzcw', 0x10: 'mfhi1', 0x11: 'mthi1', 0x12: 'mflo1',
                  0x13: 'mtlo1', 0x18: 'mult1', 0x19: 'multu1', 0x1A: 'div1', 0x1B: 'divu1', 0x20: 'madd1',
                  0x21: 'maddu1', 0x30: 'pmfhl', 0x31: 'pmthl', 0x34: 'psllh', 0x36: 'psrlh', 0x37: 'psrah',
                  0x3C: 'psllw', 0x3E: 'psrlw', 0x3F: 'psraw'}
MMI0_NAMES = {0x00: 'paddw', 0x01: 'psubw', 0x02: 'pcgtw', 0x03: 'pmaxw', 0x04: 'paddh', 0x05: 'psubh',
              0x06: 'pcgth', 0x07: 'pmaxh', 0x08: 'paddb', 0x09: 'psubb', 0x0A: 'pcgtb', 0x10: 'paddsw',
              0x11: 'psubsw', 0x12: 'pextlw', 0x13: 'ppacw', 0x14: 'paddsh', 0x15: 'psubsh', 0x16: 'pextlh',
              0x17: 'ppach', 0x18: 'paddsb', 0x19: 'psubsb', 0x1A: 'pextlb', 0x1B: 'ppacb', 0x1E: 'pext5',
              0x1F: 'ppac5'}
MMI1_NAMES = {0x01: 'pabsw', 0x02: 'pceqw', 0x03: 'pminw', 0x04: 'padsbh', 0x05: 'pabsh', 0x06: 'pceqh',
              0x07: 'pminh', 0x0A: 'pceqb', 0x10: 'padduw', 0x11: 'psubuw', 0x12: 'pextuw', 0x14: 'padduh',
              0x15: 'psubuh', 0x16: 'pextuh', 0x18: 'paddub', 0x19: 'psubub', 0x1A: 'pextub', 0x1B: 'qfsrv'}
MMI2_NAMES = {0x00: 'pmaddw', 0x02: 'psllvw', 0x03: 'psrlvw', 0x04: 'pmsubw', 0x08: 'pmfhi', 0x09: 'pmflo',
              0x0A: 'pinth', 0x0C: 'pmultw', 0x0D: 'pdivw', 0x0E: 'pcpyld', 0x10: 'pmaddh', 0x11: 'phmadh',
              0x12: 'pand', 0x13: 'pxor', 0x14: 'pmsubh', 0x15: 'phmsbh', 0x1A: 'pexeh', 0x1B: 'prevh',
              0x1C: 'pmulth', 0x1D: 'pdivbw', 0x1E: 'pexew', 0x1F: 'prot3w'}
MMI3_NAMES = {0x00: 'pmadduw', 0x03: 'psravw', 0x08: 'pmthi', 0x09: 'pmtlo', 0x0A: 'pinteh', 0x0C: 'pmultuw',
              0x0D: 'pdivuw', 0x0E: 'pcpyud', 0x12: 'por', 0x13: 'pnor', 0x1A: 'pexch', 0x1B: 'pcpyh',
              0x1E: 'pexcw'}


# ------------------------------------------------------------------ VU0 macro mode (COP2)
# The vector instructions run on io.vf / io.vi / io.acc with the EE float model
# (eadd / esub / emul / ediv / esqrt: round toward zero, no denormals, no Inf/NaN,
# overflow clamped).  The MAC / status / clipping flags are not computed (cfc2 of
# vi16..18 reads what was stored there, 0 at first).  VU0 data memory (vlqi / vsqi /
# vlqd / vsqd / vilwr / viswr) is the MMIO window at VU0_DATA, so those accesses are
# hardware events.  VCALLMS / VCALLMSR are events; the micro program does not run.

_VU_ARITH = {
    'add': lambda a, b, acc: eadd(a, b),
    'sub': lambda a, b, acc: esub(a, b),
    'mul': lambda a, b, acc: emul(a, b),
    'madd': lambda a, b, acc: eadd(acc, emul(a, b)),
    'msub': lambda a, b, acc: esub(acc, emul(a, b)),
    'max': lambda a, b, acc: a if fkey(a) >= fkey(b) else b,
    'mini': lambda a, b, acc: a if fkey(a) <= fkey(b) else b,
}
_VU_GROUP = ['add', 'sub', 'madd', 'msub', 'max', 'mini', 'mul']      # special1 0x00..0x1B by 4
# special1 0x1C..0x2F: (op, second operand: 'q' / 'i' / 'v')
_VU_S1 = {0x1C: ('mul', 'q'), 0x1D: ('max', 'i'), 0x1E: ('mul', 'i'), 0x1F: ('mini', 'i'),
          0x20: ('add', 'q'), 0x21: ('madd', 'q'), 0x22: ('add', 'i'), 0x23: ('madd', 'i'),
          0x24: ('sub', 'q'), 0x25: ('msub', 'q'), 0x26: ('sub', 'i'), 0x27: ('msub', 'i'),
          0x28: ('add', 'v'), 0x29: ('madd', 'v'), 0x2A: ('mul', 'v'), 0x2B: ('max', 'v'),
          0x2C: ('sub', 'v'), 0x2D: ('msub', 'v'), 0x2F: ('mini', 'v')}
# special2 accumulator forms: code -> (op, operand)
_VU_S2A = {0x1C: ('mul', 'q'), 0x1E: ('mul', 'i'), 0x20: ('add', 'q'), 0x21: ('madd', 'q'),
           0x22: ('add', 'i'), 0x23: ('madd', 'i'), 0x24: ('sub', 'q'), 0x25: ('msub', 'q'),
           0x26: ('sub', 'i'), 0x27: ('msub', 'i'), 0x28: ('add', 'v'), 0x29: ('madd', 'v'),
           0x2A: ('mul', 'v'), 0x2C: ('sub', 'v'), 0x2D: ('msub', 'v')}


def vu0_macro_name(w):
    """the mnemonic vu0_macro implements for COP2 vector instruction w (None: not
    modelled) - for the self-test's decode cross-check against rabbitizer."""
    lo6 = w & 63
    xyzw = 'xyzw'
    if lo6 < 0x3C:
        if lo6 < 0x1C:
            return 'v' + _VU_GROUP[lo6 >> 2] + xyzw[lo6 & 3]
        n = {0x1C: 'vmulq', 0x1D: 'vmaxi', 0x1E: 'vmuli', 0x1F: 'vminii', 0x20: 'vaddq', 0x21: 'vmaddq',
             0x22: 'vaddi', 0x23: 'vmaddi', 0x24: 'vsubq', 0x25: 'vmsubq', 0x26: 'vsubi', 0x27: 'vmsubi',
             0x28: 'vadd', 0x29: 'vmadd', 0x2A: 'vmul', 0x2B: 'vmax', 0x2C: 'vsub', 0x2D: 'vmsub',
             0x2E: 'vopmsub', 0x2F: 'vmini', 0x30: 'viadd', 0x31: 'visub', 0x32: 'viaddi', 0x34: 'viand',
             0x35: 'vior', 0x38: 'vcallms', 0x39: 'vcallmsr'}
        return n.get(lo6)
    code = (((w >> 6) & 0x1F) << 2) | (w & 3)
    if code < 0x10:
        return 'v' + ['adda', 'suba', 'madda', 'msuba'][code >> 2] + xyzw[code & 3]
    if 0x18 <= code < 0x1C:
        return 'vmula' + xyzw[code & 3]
    n = {0x10: 'vitof0', 0x11: 'vitof4', 0x12: 'vitof12', 0x13: 'vitof15', 0x14: 'vftoi0', 0x15: 'vftoi4',
         0x16: 'vftoi12', 0x17: 'vftoi15', 0x1C: 'vmulaq', 0x1D: 'vabs', 0x1E: 'vmulai', 0x20: 'vaddaq',
         0x21: 'vmaddaq', 0x22: 'vaddai', 0x23: 'vmaddai', 0x24: 'vsubaq', 0x25: 'vmsubaq', 0x26: 'vsubai',
         0x27: 'vmsubai', 0x28: 'vadda', 0x29: 'vmadda', 0x2A: 'vmula', 0x2C: 'vsuba', 0x2D: 'vmsuba',
         0x2E: 'vopmula', 0x2F: 'vnop', 0x30: 'vmove', 0x31: 'vmr32', 0x34: 'vlqi', 0x35: 'vsqi', 0x36: 'vlqd',
         0x37: 'vsqd', 0x38: 'vdiv', 0x39: 'vsqrt', 0x3A: 'vrsqrt', 0x3B: 'vwaitq', 0x3C: 'vmtir',
         0x3D: 'vmfir', 0x3E: 'vilwr', 0x3F: 'viswr'}
    return n.get(code)


def vu0_macro(io, w):
    """a closure running the COP2 vector instruction w, or None (not modelled)."""
    dest = (w >> 21) & 0xF
    ft = (w >> 16) & 31
    fs = (w >> 11) & 31
    fd = (w >> 6) & 31
    bc = w & 3
    lo6 = w & 63
    sel = [i for i in range(4) if dest & (8 >> i)]

    def put(reg, vals):
        if reg:
            dst = io.vf[reg]
            for i in sel:
                dst[i] = vals[i]

    def arith(opname, kind, to_acc, out):
        fop = _VU_ARITH[opname]

        def f():
            a = io.vf[fs]
            if kind == 'bc':
                bs = [io.vf[ft][bc]] * 4
            elif kind == 'q':
                bs = [io.vi[22] & M32] * 4
            elif kind == 'i':
                bs = [io.vi[21] & M32] * 4
            else:
                bs = list(io.vf[ft])
            acc = io.acc
            res = [fop(a[i], bs[i], acc[i]) for i in range(4)]
            if to_acc:
                for i in sel:
                    acc[i] = res[i]
            else:
                put(out, res)
        return f

    def mem_addr(r):
        return VU0_DATA + ((io.vi[r] << 4) & 0xFF0)

    def vstore(r, src):
        a = mem_addr(r)
        x = io.vf[src]
        if dest == 0xF:
            io.write(a, 16, x[0] | (x[1] << 32) | (x[2] << 64) | (x[3] << 96))
        else:
            for i in sel:
                io.write(a + 4 * i, 4, x[i])

    def vload(r, dst):
        a = mem_addr(r)
        if dest == 0xF:
            v = io.read(a, 16)
            vals = [(v >> s) & M32 for s in (0, 32, 64, 96)]
        else:
            vals = [0, 0, 0, 0]
            for i in sel:
                vals[i] = io.read(a + 4 * i, 4)
        put(dst, vals)

    if lo6 < 0x3C:                                      # special1
        if lo6 < 0x1C:
            return arith(_VU_GROUP[lo6 >> 2], 'bc', False, fd)
        if lo6 in _VU_S1:
            o, k = _VU_S1[lo6]
            return arith(o, k, False, fd)
        if lo6 == 0x2E:                                 # vopmsub: fd.xyz = ACC.xyz - fs.yzx * ft.zxy
            def vopmsub():
                a, b, acc = io.vf[fs], io.vf[ft], io.acc
                res = [esub(acc[0], emul(a[1], b[2])), esub(acc[1], emul(a[2], b[0])),
                       esub(acc[2], emul(a[0], b[1])), 0]
                put(fd, res)
            return vopmsub
        if lo6 in (0x30, 0x31, 0x34, 0x35):             # viadd visub viand vior: id, is, it
            fop = {0x30: lambda x, y: x + y, 0x31: lambda x, y: x - y, 0x34: lambda x, y: x & y,
                   0x35: lambda x, y: x | y}[lo6]

            def viop():
                if fd:
                    io.vi[fd] = fop(io.vi[fs], io.vi[ft]) & 0xFFFF
            return viop
        if lo6 == 0x32:                                 # viaddi it, is, imm5
            imm = fd - 32 if fd & 0x10 else fd

            def viaddi():
                if ft:
                    io.vi[ft] = (io.vi[fs] + imm) & 0xFFFF
            return viaddi
        if lo6 in (0x38, 0x39):                         # vcallms imm15 / vcallmsr (CMSAR0)
            imm = ((w >> 6) & 0x7FFF) << 3

            def vcallms():
                io.emit(('vcallms', imm if lo6 == 0x38 else 'vi27'),
                        None if lo6 == 0x38 else io.vi[27])
            return vcallms
        return None
    code = (((w >> 6) & 0x1F) << 2) | (w & 3)           # special2
    if code < 0x10:
        return arith(['add', 'sub', 'madd', 'msub'][code >> 2], 'bc', True, None)
    if 0x18 <= code < 0x1C:
        return arith('mul', 'bc', True, None)
    if code in _VU_S2A:
        o, k = _VU_S2A[code]
        return arith(o, k, True, None)
    if 0x10 <= code < 0x18:                             # vitof0/4/12/15, vftoi0/4/12/15: ft <- fs
        n = (0, 4, 12, 15)[code & 3]
        if code < 0x14:
            def conv(x):
                v = ecvt_s_w(x)
                return emul(v, (127 - n) << 23) if n else v
        else:
            def conv(x):
                return ecvt_w_s(emul(x, (127 + n) << 23) if n else x)

        def vconv():
            put(ft, [conv(x) for x in io.vf[fs]])
        return vconv
    if code == 0x1D:                                    # vabs
        def vabs():
            put(ft, [x & 0x7FFFFFFF for x in io.vf[fs]])
        return vabs
    if code == 0x2E:                                    # vopmula: ACC.xyz = fs.yzx * ft.zxy
        def vopmula():
            a, b = io.vf[fs], io.vf[ft]
            res = [emul(a[1], b[2]), emul(a[2], b[0]), emul(a[0], b[1]), 0]
            for i in sel:
                io.acc[i] = res[i]
        return vopmula
    if code in (0x2F, 0x3B):                            # vnop / vwaitq
        def vnop():
            return None
        return vnop
    if code == 0x30:                                    # vmove ft, fs
        def vmove():
            put(ft, list(io.vf[fs]))
        return vmove
    if code == 0x31:                                    # vmr32 ft, fs
        def vmr32():
            x = io.vf[fs]
            put(ft, [x[1], x[2], x[3], x[0]])
        return vmr32
    if code in (0x34, 0x36):                            # vlqi / vlqd ft, (is++ / --is)
        def vlq():
            if code == 0x36 and fs:
                io.vi[fs] = (io.vi[fs] - 1) & 0xFFFF
            vload(fs, ft)
            if code == 0x34 and fs:
                io.vi[fs] = (io.vi[fs] + 1) & 0xFFFF
        return vlq
    if code in (0x35, 0x37):                            # vsqi / vsqd fs, (it++ / --it)
        def vsq():
            if code == 0x37 and ft:
                io.vi[ft] = (io.vi[ft] - 1) & 0xFFFF
            vstore(ft, fs)
            if code == 0x35 and ft:
                io.vi[ft] = (io.vi[ft] + 1) & 0xFFFF
        return vsq
    fsf = (w >> 21) & 3
    ftf = (w >> 23) & 3
    if code == 0x38:                                    # vdiv Q, fs.fsf, ft.ftf
        def vdiv():
            io.vi[22] = ediv(io.vf[fs][fsf], io.vf[ft][ftf])
        return vdiv
    if code == 0x39:                                    # vsqrt Q, ft.ftf
        def vsqrt():
            io.vi[22] = esqrt(io.vf[ft][ftf])
        return vsqrt
    if code == 0x3A:                                    # vrsqrt Q, fs.fsf, ft.ftf
        def vrsqrt():
            io.vi[22] = ediv(io.vf[fs][fsf], esqrt(io.vf[ft][ftf]))
        return vrsqrt
    if code == 0x3C:                                    # vmtir it, fs.fsf
        def vmtir():
            if ft:
                io.vi[ft] = io.vf[fs][fsf] & 0xFFFF
        return vmtir
    if code == 0x3D:                                    # vmfir ft, is (sign-extended)
        def vmfir():
            v = io.vi[fs] & 0xFFFF
            v = (v - 0x10000 if v & 0x8000 else v) & M32
            put(ft, [v] * 4)
        return vmfir
    if code in (0x3E, 0x3F):                            # vilwr / viswr it, (is): one field
        def vilw():
            a = mem_addr(fs)
            for i in sel:
                if code == 0x3E:
                    v = io.read(a + 4 * i, 4)
                    if ft:
                        io.vi[ft] = v & 0xFFFF
                else:
                    io.write(a + 4 * i, 4, io.vi[ft] & 0xFFFF)
        return vilw
    return None


# ------------------------------------------------------------------ ELF symbols

def elf_symbols(path):
    """[(name, value, size, type, bind, shndx)] from an ELF32 .symtab."""
    d = open(path, 'rb').read()
    e_shoff, = struct.unpack_from('<I', d, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', d, 0x2E)
    secs = [struct.unpack_from('<IIIIIIIIII', d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
    out = []
    for s in secs:
        if s[1] == 2:  # SHT_SYMTAB
            so = secs[s[6]][4]
            for k in range(s[5] // 16):
                nm, val, size, info, oth, shn = struct.unpack_from('<IIIBBH', d, s[4] + 16 * k)
                e = d.index(b'\0', so + nm)
                name = d[so + nm:e].decode('latin1')
                if name:
                    out.append((name, val, size, info & 15, info >> 4, shn))
    return out
