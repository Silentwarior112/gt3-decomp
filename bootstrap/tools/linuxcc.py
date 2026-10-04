"""Run Sony's Linux ee-gcc 2.9-ee-991111-01 cc1 / cc1plus on Windows.

    python tools/linuxcc.py [--root DIR] [--trace] PROGRAM [ARGS...]

The reference compiler (sdk/ee-gcc-2.9-991111-01, see MATCHING.md) is a
32-bit Linux build, and this PC has no WSL.  This is a small i386 Linux
user-mode emulator: Unicorn (`pip install unicorn`, the only dependency)
executes the code, and this file loads the ELF and its dynamic loader and
answers the Linux system calls that cc1 and glibc make (files, memory, TLS,
time, limits).  Anything else returns -ENOSYS (listed with --trace).

PROGRAM and the file arguments are host paths.  Inside the program, an
absolute path beginning with '/' is looked up under the root directory
(default: the compiler's rootfs/, which holds the glibc 2.3.6 that Sony's
Linux toolchain package shipped); any other path (C:/..., relative) is a
host path.  The exit status is the program's own.

tools/mcc.py runs it for `--cc 991111-01`; it is rarely needed by hand.
"""
import os, struct, sys, time

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_PROT_ALL, UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                               UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
                               UC_X86_REG_EIP, UC_X86_REG_GDTR, UC_X86_REG_SS, UC_X86_REG_DS,
                               UC_X86_REG_ES)

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import paths
DEFAULT_ROOT = paths.sdk("ee-gcc-2.9-991111-01", "rootfs")

PAGE = 0x1000
STACK_TOP = 0xC0000000
STACK_SIZE = 0x2000000          # 32 MB: cc1 recurses deeply on big functions
GDT_BASE = 0xC1000000
INTERP_BASE = 0x40000000
MMAP_BASE = 0x50000000

ENOENT, EBADF, ENOMEM, ENOTDIR, ENOTTY, ERANGE, ENOSYS = 2, 9, 12, 20, 25, 34, 38


def seg_desc(base, limit, access, flags):
    """An 8-byte GDT descriptor (limit: 20 bits; flags: the G/D nibble)."""
    return ((limit & 0xFFFF) | ((base & 0xFFFFFF) << 16) | (access << 40) | (((limit >> 16) & 0xF) << 48)
            | (flags << 52) | (((base >> 24) & 0xFF) << 56))


def up(x):
    return (x + PAGE - 1) & ~(PAGE - 1)


def down(x):
    return x & ~(PAGE - 1)


class _Exit(Exception):
    pass


class Linux32:
    def __init__(self, root, argv, trace=False):
        self.root = root
        self.trace = trace
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.pages = set()
        self.fds = {0: 0, 1: 1, 2: 2}
        self.fd_paths = {}
        self.exit_code = None
        self.mmap_next = MMAP_BASE
        self.tls = {}
        self._setup(argv)

    # ---------------------------------------------------------------- memory
    def map(self, addr, size):
        """Map every unmapped page of [addr, addr+size) (RWX)."""
        a, e = down(addr), up(addr + size)
        p = a
        while p < e:
            if p >> 12 in self.pages:
                p += PAGE
                continue
            q = p
            while q < e and q >> 12 not in self.pages:
                q += PAGE
            self.uc.mem_map(p, q - p, UC_PROT_ALL)
            self.pages.update(range(p >> 12, q >> 12))
            p = q

    def unmap(self, addr, size):
        a, e = down(addr), up(addr + size)
        p = a
        while p < e:
            if p >> 12 not in self.pages:
                p += PAGE
                continue
            q = p
            while q < e and q >> 12 in self.pages:
                q += PAGE
            self.uc.mem_unmap(p, q - p)
            self.pages.difference_update(range(p >> 12, q >> 12))
            p = q

    def rd(self, addr, n):
        return bytes(self.uc.mem_read(addr, n))

    def wr(self, addr, data):
        self.uc.mem_write(addr, bytes(data))

    def w32(self, addr, v):
        self.wr(addr, struct.pack("<I", v & 0xFFFFFFFF))

    def cstr(self, addr):
        out = b""
        while True:
            chunk = self.rd(addr + len(out), 64)
            i = chunk.find(b"\0")
            if i >= 0:
                return (out + chunk[:i]).decode("latin1")
            out += chunk

    # ------------------------------------------------------------------- ELF
    def load_elf(self, path, base):
        data = open(path, "rb").read()
        if data[:4] != b"\x7fELF" or data[4] != 1:
            raise SystemExit("linuxcc: not a 32-bit ELF: %s" % path)
        e_type, _, _, e_entry, e_phoff, _, _, _, e_phentsize, e_phnum = struct.unpack_from("<HHIIIIIHHH", data, 16)
        if e_type == 2:                                       # ET_EXEC: absolute addresses
            base = 0
        interp, phdr, end, first = None, None, 0, None
        for i in range(e_phnum):
            p_type, p_offset, p_vaddr, _, p_filesz, p_memsz, _, _ = \
                struct.unpack_from("<IIIIIIII", data, e_phoff + i * e_phentsize)
            if p_type == 1:                                   # PT_LOAD
                va = base + p_vaddr
                self.map(va, p_memsz)
                self.wr(va, data[p_offset:p_offset + p_filesz])
                end = max(end, va + p_memsz)
                if first is None:
                    first = va - p_offset
            elif p_type == 3:                                 # PT_INTERP
                interp = data[p_offset:p_offset + p_filesz].rstrip(b"\0").decode()
            elif p_type == 6:                                 # PT_PHDR
                phdr = base + p_vaddr
        return dict(entry=base + e_entry, phdr=phdr if phdr is not None else first + e_phoff,
                    phnum=e_phnum, interp=interp, end=end, base=base)

    def host_path(self, p):
        return os.path.join(self.root, p.lstrip("/")) if p.startswith("/") else p

    def _setup(self, argv):
        exe = self.load_elf(argv[0], 0)
        interp = self.load_elf(self.host_path(exe["interp"]), INTERP_BASE) if exe["interp"] else None
        self.brk = self.brk0 = up(exe["end"])
        self.map(STACK_TOP - STACK_SIZE, STACK_SIZE)
        self.map(GDT_BASE, PAGE)
        self.uc.reg_write(UC_X86_REG_GDTR, (0, GDT_BASE, PAGE - 1, 0))
        # A flat 32-bit data segment for SS/DS/ES.  Loading any segment register
        # (glibc loads %gs for its TLS) makes the CPU re-derive the stack size
        # from SS's descriptor; without a real one the stack turns 16-bit.
        self.wr(GDT_BASE + 8 * 2, struct.pack("<Q", seg_desc(0, 0xFFFFF, 0x93, 0xC)))
        for r in (UC_X86_REG_SS, UC_X86_REG_DS, UC_X86_REG_ES):
            self.uc.reg_write(r, 2 << 3)

        sp = STACK_TOP

        def push(b):
            nonlocal sp
            sp -= len(b)
            self.wr(sp, b)
            return sp
        platform = push(b"i686\0")
        rand = push(bytes(range(1, 17)))          # AT_RANDOM, fixed: runs are reproducible
        args = [push(a.encode("latin1") + b"\0") for a in argv]
        envs = [push(e + b"\0") for e in (b"LC_ALL=C", b"PATH=/bin")]
        # Initial stack: argc, argv[], NULL, envp[], NULL, auxv (no AT_SYSINFO,
        # so glibc makes its system calls with int $0x80).
        auxv = [3, exe["phdr"], 4, 32, 5, exe["phnum"], 6, PAGE, 7, interp["base"] if interp else 0,
                8, 0, 9, exe["entry"], 11, 1000, 12, 1000, 13, 1000, 14, 1000, 23, 0,
                25, rand, 15, platform, 16, 0, 17, 100, 0, 0]
        words = [len(args)] + args + [0] + envs + [0] + auxv
        sp = (sp - 4 * len(words)) & ~0xF
        self.wr(sp, struct.pack("<%dI" % len(words), *words))
        self.uc.reg_write(UC_X86_REG_ESP, sp)
        self.uc.reg_write(UC_X86_REG_EDX, 0)
        self.start = interp["entry"] if interp else exe["entry"]
        self.uc.hook_add(UC_HOOK_INTR, self._intr)

    # -------------------------------------------------------------- syscalls
    def _intr(self, uc, intno, user):
        if intno != 0x80:
            sys.stderr.write("linuxcc: interrupt %#x at %#x\n" % (intno, uc.reg_read(UC_X86_REG_EIP)))
            self.exit_code = 134
            uc.emu_stop()
            return
        nr = uc.reg_read(UC_X86_REG_EAX)
        a = [uc.reg_read(r) for r in (UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                                      UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP)]
        h = SYSCALLS.get(nr)
        try:
            ret = h(self, *a) if h else -ENOSYS
        except _Exit:
            uc.emu_stop()
            return
        if self.trace:
            sys.stderr.write("[linuxcc] %s(%s) = %d\n" % (h.__name__ if h else "syscall_%d" % nr,
                                                          ", ".join("%#x" % x for x in a[:4]), ret))
        uc.reg_write(UC_X86_REG_EAX, ret & 0xFFFFFFFF)

    def new_fd(self, host_fd, path):
        fd = 3
        while fd in self.fds:
            fd += 1
        self.fds[fd] = host_fd
        self.fd_paths[fd] = path
        return fd

    def run(self):
        try:
            self.uc.emu_start(self.start, 0)
        except UcError as e:
            if self.exit_code is None:
                sys.stderr.write("linuxcc: %s at %#x\n" % (e, self.uc.reg_read(UC_X86_REG_EIP)))
                return 139
        for fd, hfd in list(self.fds.items()):
            if fd > 2:
                os.close(hfd)
        return self.exit_code if self.exit_code is not None else 1


def sys_exit(k, code, *_):
    k.exit_code = code & 0xFF
    raise _Exit()


def sys_read(k, fd, buf, n, *_):
    if fd not in k.fds:
        return -EBADF
    data = os.read(k.fds[fd], n) if n else b""
    k.wr(buf, data)
    return len(data)


def sys_write(k, fd, buf, n, *_):
    if fd not in k.fds:
        return -EBADF
    return os.write(k.fds[fd], k.rd(buf, n)) if n else 0


def sys_writev(k, fd, iov, cnt, *_):
    total = 0
    for i in range(cnt):
        base, ln = struct.unpack("<II", k.rd(iov + 8 * i, 8))
        if ln:
            total += sys_write(k, fd, base, ln)
    return total


def _open(k, path, flags, mode):
    hp = k.host_path(path)
    if flags & 0x10000 or os.path.isdir(hp):                  # O_DIRECTORY: none needed
        return -ENOTDIR if os.path.isfile(hp) else -ENOENT
    f = (os.O_RDONLY, os.O_WRONLY, os.O_RDWR, os.O_RDWR)[flags & 3] | getattr(os, "O_BINARY", 0)
    for bit, of in ((0x40, os.O_CREAT), (0x80, os.O_EXCL), (0x200, os.O_TRUNC), (0x400, os.O_APPEND)):
        if flags & bit:
            f |= of
    try:
        return k.new_fd(os.open(hp, f, mode or 0o644), hp)
    except OSError as e:
        return -(e.errno or ENOENT)


def sys_open(k, path, flags, mode, *_):
    return _open(k, k.cstr(path), flags, mode)


def sys_openat(k, dirfd, path, flags, mode, *_):
    return _open(k, k.cstr(path), flags, mode)


def sys_close(k, fd, *_):
    if fd not in k.fds:
        return -EBADF
    if fd > 2:
        os.close(k.fds[fd])
    del k.fds[fd]
    k.fd_paths.pop(fd, None)
    return 0


def sys_lseek(k, fd, off, whence, *_):
    if fd not in k.fds:
        return -EBADF
    return os.lseek(k.fds[fd], off - (1 << 32) if off & 0x80000000 else off, whence)


def sys_llseek(k, fd, hi, lo, result, whence, *_):
    if fd not in k.fds:
        return -EBADF
    off = (hi << 32) | lo
    pos = os.lseek(k.fds[fd], off - (1 << 64) if off >> 63 else off, whence)
    k.wr(result, struct.pack("<q", pos))
    return 0


_inodes = {}


def _put_stat64(k, buf, path, mode, size, mtime):
    # Windows file IDs do not fit the old 32-bit st_ino (glibc's stat() would
    # fail with EOVERFLOW), so files get small synthetic inode numbers.
    ino = _inodes.setdefault(path, len(_inodes) + 2)
    t = int(mtime)
    k.wr(buf, struct.pack("<QIIIIIIQIqIQIIIIIIQ", 0x801, 0, ino, mode, 1, 1000, 1000, 0, 0, size, 4096,
                          (size + 511) // 512, t, 0, t, 0, t, 0, ino))
    return 0


def sys_stat64(k, path, buf, *_):
    hp = k.host_path(k.cstr(path))
    try:
        st = os.stat(hp)
    except OSError:
        return -ENOENT
    return _put_stat64(k, buf, hp, st.st_mode, st.st_size, st.st_mtime)


def sys_fstat64(k, fd, buf, *_):
    if fd not in k.fds:
        return -EBADF
    if fd not in k.fd_paths:                                   # stdin/stdout/stderr
        return _put_stat64(k, buf, "<tty%d>" % fd, 0o20620, 0, 0)
    st = os.fstat(k.fds[fd])
    return _put_stat64(k, buf, k.fd_paths[fd], st.st_mode, st.st_size, st.st_mtime)


def sys_fstatat64(k, dirfd, path, buf, flags, *_):
    return sys_stat64(k, path, buf)


def sys_access(k, path, mode, *_):
    return 0 if os.path.exists(k.host_path(k.cstr(path))) else -ENOENT


def sys_unlink(k, path, *_):
    try:
        os.remove(k.host_path(k.cstr(path)))
        return 0
    except OSError:
        return -ENOENT


def sys_brk(k, addr, *_):
    # Shrinking keeps the pages mapped; growing maps them.
    if addr > k.brk:
        k.map(k.brk, addr - k.brk)
    if addr >= k.brk0:
        k.brk = addr
    return k.brk


def sys_mmap2(k, addr, length, prot, flags, fd, pgoff):
    length = up(length)
    if flags & 0x10:                                          # MAP_FIXED
        a = addr
        k.map(a, length)
        k.wr(a, bytes(length))
    else:
        a = k.mmap_next
        k.mmap_next += length + PAGE
        k.map(a, length)
    if not flags & 0x20:                                      # file-backed
        if fd not in k.fds:
            return -EBADF
        hfd = k.fds[fd]
        pos = os.lseek(hfd, 0, 1)
        os.lseek(hfd, pgoff * PAGE, 0)
        k.wr(a, os.read(hfd, length))
        os.lseek(hfd, pos, 0)
    return a


def sys_old_mmap(k, args, *_):
    addr, length, prot, flags, fd, off = struct.unpack("<6I", k.rd(args, 24))
    return sys_mmap2(k, addr, length, prot, flags, fd, off // PAGE)


def sys_munmap(k, addr, length, *_):
    k.unmap(addr, length)
    return 0


def sys_ok(k, *_):
    return 0


def sys_mremap(k, *_):
    return -ENOMEM                     # glibc falls back to malloc + copy + free


def sys_uname(k, buf, *_):
    fields = (b"Linux", b"localhost", b"2.6.32", b"#1 SMP", b"i686", b"(none)")
    k.wr(buf, b"".join(f.ljust(65, b"\0") for f in fields))
    return 0


def sys_set_thread_area(k, uinfo, *_):
    entry, base, limit, flags = struct.unpack("<IIII", k.rd(uinfo, 16))
    if entry == 0xFFFFFFFF:
        entry = 12 + len(k.tls)
    seg32, contents, ro, pages = flags & 1, (flags >> 1) & 3, (flags >> 3) & 1, (flags >> 4) & 1
    access = 0x80 | (3 << 5) | 0x10 | (contents << 2) | (0 if ro else 2) | 1
    k.wr(GDT_BASE + 8 * entry, struct.pack("<Q", seg_desc(base, limit, access, (pages << 3) | (seg32 << 2))))
    k.tls[entry] = base
    k.w32(uinfo, entry)
    return 0


def sys_getrlimit(k, res, rlim, *_):
    k.wr(rlim, struct.pack("<II", STACK_SIZE if res == 3 else 0xFFFFFFFF, 0xFFFFFFFF))
    return 0


def sys_prlimit64(k, pid, res, new, old, *_):
    if old:
        k.wr(old, struct.pack("<QQ", STACK_SIZE if res == 3 else (1 << 64) - 1, (1 << 64) - 1))
    return 0


def sys_rt_sigaction(k, sig, act, oldact, *_):
    if oldact:
        k.wr(oldact, bytes(20))
    return 0


def sys_rt_sigprocmask(k, how, nset, oset, *_):
    if oset:
        k.wr(oset, bytes(8))
    return 0


def sys_getpid(k, *_):
    return 1000


def sys_time(k, tloc, *_):
    t = int(time.time())
    if tloc:
        k.w32(tloc, t)
    return t


def sys_gettimeofday(k, tv, tz, *_):
    t = time.time()
    if tv:
        k.wr(tv, struct.pack("<II", int(t), int((t % 1) * 1e6)))
    return 0


def sys_clock_gettime(k, clk, tp, *_):
    t = time.time()
    k.wr(tp, struct.pack("<II", int(t), int((t % 1) * 1e9)))
    return 0


def sys_times(k, buf, *_):
    if buf:
        k.wr(buf, bytes(16))
    return int(time.time() * 100) & 0x7FFFFFFF


def sys_getrusage(k, who, buf, *_):
    k.wr(buf, bytes(72))
    return 0


def sys_ioctl(k, *_):
    return -ENOTTY


def sys_getcwd(k, buf, size, *_):
    cwd = os.getcwd().replace("\\", "/").encode("latin1") + b"\0"
    if len(cwd) > size:
        return -ERANGE
    k.wr(buf, cwd)
    return len(cwd)


def sys_readlink(k, *_):
    return -ENOENT


SYSCALLS = {
    1: sys_exit, 3: sys_read, 4: sys_write, 5: sys_open, 6: sys_close, 10: sys_unlink,
    13: sys_time, 19: sys_lseek, 20: sys_getpid, 24: sys_getpid, 33: sys_access, 43: sys_times,
    45: sys_brk, 47: sys_getpid, 49: sys_getpid, 50: sys_getpid, 54: sys_ioctl, 75: sys_ok,
    76: sys_getrlimit, 77: sys_getrusage, 78: sys_gettimeofday, 85: sys_readlink, 90: sys_old_mmap,
    91: sys_munmap, 122: sys_uname, 125: sys_ok, 140: sys_llseek, 146: sys_writev, 163: sys_mremap,
    174: sys_rt_sigaction, 175: sys_rt_sigprocmask, 183: sys_getcwd, 186: sys_ok, 191: sys_getrlimit,
    192: sys_mmap2, 195: sys_stat64, 196: sys_stat64, 197: sys_fstat64, 199: sys_getpid,
    200: sys_getpid, 201: sys_getpid, 202: sys_getpid, 219: sys_ok, 221: sys_ok, 224: sys_getpid,
    240: sys_ok, 243: sys_set_thread_area, 252: sys_exit, 258: sys_getpid, 265: sys_clock_gettime,
    295: sys_openat, 300: sys_fstatat64, 311: sys_ok, 340: sys_prlimit64,
}


def run(argv, root=DEFAULT_ROOT, trace=False):
    """Run argv[0] (a host path to an i386 Linux ELF) with argv[1:]; return its exit status."""
    return Linux32(root, list(argv), trace).run()


def main(argv):
    root, trace = DEFAULT_ROOT, False
    while argv and argv[0] in ("--root", "--trace"):
        if argv[0] == "--root":
            root, argv = argv[1], argv[2:]
        else:
            trace, argv = True, argv[1:]
    if not argv:
        sys.exit(__doc__)
    return run(argv, root, trace)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
