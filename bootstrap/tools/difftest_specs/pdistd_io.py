"""pdistd_io group: units printf (src/pdistd/printf.cpp), allocator
(src/pdistd/allocator.cpp, pdistd_misc.cpp), filename (src/filename.cpp) and io
(src/hostfs_io.cpp, mcard_io.cpp, thread_util.cpp)."""
from .common import *

UNITS = ("printf", "allocator", "filename", "io")

# ================================================================== pdistd printf (src/pdistd/printf.cpp)

PF_BASE_CTOR, PF_BASE_DTOR, PF_BASE_PUTC, PRINTFORMAT = 0x01010318, 0x01010330, 0x01010360, 0x01010368
PT_CTOR, PT_DTOR, PT_PUTC, PT_FLUSH = 0x010114A8, 0x01011508, 0x01011530, 0x01011590
VPRINTF, PRINTF, TTY_PUTS = 0x010115D8, 0x01011678, 0x01012FC8

DOUBLES = [0.0, -0.0, 1.0, -1.0, 0.5, 1.5, 2.0, 10.0, 100.0, 123.456, -987.654321, 1e-5, 1.5e-5, 3.14159265358979,
           1e10, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 9.999999, 0.1, 0.01, 0.001, 1e-10, 1e-300, 1e300, 1.7976931348623157e308,
           5e-324, 2.2250738585072014e-308, 65536.0, 4294967296.0, 0.3333333333333333, 2.5, 99.5, 1e-4, 12345678.9]


def dbits(x):
    return struct.unpack("<Q", struct.pack("<d", x))[0]


def rand_directive(rng):
    """(format text, [arg slots as ('int', v) / ('str', bytes|None) / ('dbl', bits)])"""
    k = rng.random()
    if k < 0.15:
        return rstr(rng, rng.randrange(1, 8)).replace(b"%", b"%%"), []
    if k < 0.18:
        return b"%%", []
    flags = b"".join(rng.choice([b"-", b"+", b"0", b" ", b"#"]) for _ in range(rng.choice([0, 0, 1, 1, 2, 3])))
    args = []
    width = b""
    wk = rng.random()
    if wk < 0.3:
        width = str(rng.randrange(0, 25)).encode()
    elif wk < 0.4:
        width = b"*"
        args.append(("int", rng.choice([0, 5, 12, -7, -1, rng.randrange(-30, 30)])))
    prec = b""
    pk = rng.random()
    conv = rng.choice(b"diuoxXpcsfFeEgGtdddxxss")
    conv = bytes([conv])
    if pk < 0.3:
        prec = b"." + str(rng.randrange(0, 12)).encode()
    elif pk < 0.36:
        prec = b"."
    elif pk < 0.43:
        prec = b".*"
        args.append(("int", rng.choice([0, 3, 8, -2, rng.randrange(-5, 15)])))
    if conv == b"t":
        # %t with precision <= 0 or >= 20 is excluded (printf.cpp Q6: an uninitialised
        # stack byte / a read past s_pow10_u64, both layout-dependent)
        if prec == b".*":
            args.pop()                  # its '*' argument, pushed above
        prec = b"." + str(rng.randrange(1, 19)).encode()
    star_args = list(args)              # what the '*'s consume while parsing
    length = rng.choice([b"", b"", b"", b"h", b"l", b"ll", b"L"]) if conv in b"diuoxXt" else b""
    if conv in b"diut":
        v = rng.choice([0, 1, -1, 9, 10, 99, 100, 0x7FFFFFFF, -0x80000000, rng.getrandbits(32),
                        rng.getrandbits(64), 10 ** 18, 10 ** 19 + 7, (1 << 64) - 1, rng.randrange(-10 ** 6, 10 ** 6)])
        args.append(("int", v))
    elif conv in b"oxXp":
        args.append(("int", rng.choice([0, 1, 0xDEADBEEF, rng.getrandbits(32), rng.getrandbits(64)])))
    elif conv == b"c":
        args.append(("int", rng.choice([65, 0x20, 0x7E, 0xC1, rng.randrange(256), rng.getrandbits(32)])))
    elif conv == b"s":
        args.append(("str", None if rng.random() < 0.1 else rstr(rng, rng.randrange(0, 30))))
    elif conv == b"F":
        f = rng.choice(DOUBLES[:20]) if rng.random() < 0.7 else struct.unpack("<f", struct.pack("<I", rng.getrandbits(32)))[0]
        try:
            fb = struct.unpack("<I", struct.pack("<f", f))[0]
        except OverflowError:
            fb = rng.getrandbits(32)
        args.append(("int", fb))
    else:
        x = rng.choice(DOUBLES) if rng.random() < 0.75 else None
        args.append(("dbl", dbits(x) if x is not None else rng.getrandbits(64)))
    if b"#" in flags and conv in b"fFgG":
        # '#' keeps a '.' the digit loop never stored when the precision is 0 (f/F)
        # or only one digit is significant (g/G): the original then prints one
        # uninitialised byte of its stack buffer (as with %t, printf.cpp Q6).
        # Layout-dependent, so '#' is only kept for f/F with a precision > 0.
        p0 = (prec in (b".", b".0") or (prec == b".*" and star_args[-1][1] <= 0) or
              (prec[:1] == b"." and prec[1:].isdigit() and int(prec[1:]) == 0))
        if conv in b"gG" or p0:
            flags = flags.replace(b"#", b"")
    if rng.random() < 0.03:
        conv = rng.choice([b"q", b"y", b"n", b"a", b"Z"])          # unknown conversions
        args = star_args                # the '*'s were still read
    return b"%" + flags + width + prec + length + conv, args


def rand_format(rng, maxdir=6):
    fmt, args = b"", []
    for _ in range(rng.randrange(1, maxdir + 1)):
        f, a = rand_directive(rng)
        fmt += f
        args += a
    return fmt, args


def put_args(t, args, strs):
    """8-byte argument slots (the EE va_list)."""
    slots = []
    for kind, v in args:
        if kind == "str":
            slots.append(strs[v] if v is not None else 0)
        else:
            slots.append(v & M64)
    return t.put(struct.pack("<%dQ" % len(slots), *slots) if slots else b"\0" * 8, 16)


def gen_printformat(rng, i, n):
    fmt, args = rand_format(rng)
    fail_at = rng.randrange(0, 40) if rng.random() < 0.1 else None
    delta = rng.choice([0, 0, 0, 8, -4])
    real_putc = rng.random() < 0.1

    def run(t):
        strs = {v: t.cstr(v) for k, v in args if k == "str" and v is not None}
        ap = put_args(t, args, strs)
        f = t.cstr(fmt)
        count = [0]

        def putc(tt, a):
            count[0] += 1
            return M32 if fail_at is not None and count[0] > fail_at else 0
        pfn = t.fn(PF_BASE_PUTC) if real_putc else t.callback(putc, 2, "putc")
        vt = t.vtable([0, (pfn, delta)])               # slot 2 = putc, called with this + delta
        target = t.put(struct.pack("<I", vt) + b"\0" * 12, 16, 4 if delta == -4 else 0)
        t.call(PRINTFORMAT, target, f, ap)
    return Case("PrintFormat(%r, %d args%s%s)" % (fmt[:120], len(args), ", putc fails after %d" % fail_at
                                                    if fail_at is not None else "", ", base putc" if real_putc else ""),
                run)


def gen_vprintf_runaway(rng, i, n):
    """deliberately invalid: a '*' width / precision of ~2^28..2^31 pads for longer
    than the step budget.  Both sides must run away after the same output."""
    fmt = rng.choice([b"%.*d", b"%*d", b"%*s", b"%0*x", b"ab%.*u"])
    big = rng.choice([0x7FFFFFFF, 0x10000000 + rng.randrange(1 << 20)])
    args = [("int", big), ("str", b"xyz") if fmt.endswith(b"s") else ("int", rng.getrandbits(32))]

    def run(t):
        strs = {v: t.cstr(v) for k, v in args if k == "str"}
        f = t.cstr(fmt)

        def tty(tt, a):
            tt.obs("tty_puts", tt.cstr_at(a[0]))
            return 0
        t.stub(TTY_PUTS, tty, 1)
        t.call(VPRINTF, f, put_args(t, args, strs), maxsteps=300000)
    return Case("vprintf(%r, 0x%X) - runs away on purpose" % (fmt, big), run)


def gen_vprintf(rng, i, n):
    if i % 50 == 49:
        return gen_vprintf_runaway(rng, i, n)
    fmt, args = rand_format(rng, 8)
    use_printf = rng.random() < 0.4 and len(args) <= 7

    def run(t):
        strs = {v: t.cstr(v) for k, v in args if k == "str" and v is not None}
        f = t.cstr(fmt)

        def tty(tt, a):
            tt.obs("tty_puts", tt.cstr_at(a[0]))
            return 0
        t.stub(TTY_PUTS, tty, 1)
        if use_printf:
            regs = [strs[v] if k == "str" and v is not None else (0 if k == "str" else v) for k, v in args]
            t.call(PRINTF, f, *regs)
        else:
            ap = put_args(t, args, strs)
            t.call(VPRINTF, f, ap)
    return Case("%s(%r, %d args)" % ("printf" if use_printf else "vprintf", fmt[:120], len(args)), run)


def gen_printtarget(rng, i, n):
    size = rng.choice([1, 2, 3, 8, 16, 256])
    chars = [rng.choice([rng.randrange(256), rng.getrandbits(32), 0x7830]) for _ in range(rng.randrange(0, 40))]
    flags = rng.choice([2, 3])

    def run(t):
        def tty(tt, a):
            tt.obs("tty_puts", tt.cstr_at(a[0]))
            return 0
        t.stub(TTY_PUTS, tty, 1)
        obj = t.halloc(16, b"\xCD" * 16)
        buf = t.alloc(size + 8, 16)
        t.call(PT_CTOR, obj, buf, size, ret="ptr")
        for c in chars:
            t.call(PT_PUTC, obj, c)
        t.call(PT_FLUSH, obj, ret="void")
        t.call(PT_DTOR, obj, flags, ret="void")
    return Case("PrintTarget(buffer %d) + %d putc + flush + dtor(%d)" % (size, len(chars), flags), run)


def gen_pf_base(rng, i, n):
    flags = rng.choice([2, 3])
    c = rng.getrandbits(32)

    def run(t):
        obj = t.halloc(8, b"\xCD" * 8)
        t.call(PF_BASE_CTOR, obj, ret="ptr")
        t.call(PF_BASE_PUTC, obj, c)
        t.call(PF_BASE_DTOR, obj, flags, ret="void")
    return Case("PrintFormatTargetBase ctor + putc + dtor(%d)" % flags, run)


def gen_tty_puts(rng, i, n):
    s = rstr(rng, rng.randrange(0, 80))

    def run(t):
        t.call(TTY_PUTS, t.cstr(s), ret="void")
    return Case("tty_puts(%r)" % s[:40], run)


# ================================================================== FileName (src/filename.cpp)

FN_BASE_DTOR, STRDUP, FN_ASSIGN_STR, FN_ASSIGN_FN = 0x01003570, 0x010035E0, 0x01003648, 0x010036A8
FN_GETPATH, DISK_GETPATH, MCBASE_DTOR, MCBASE_CTOR = 0x01003708, 0x01003710, 0x01003830, 0x01003880
MC_GETPATH, DIRP_DTOR, DIRP_CTOR0, DIRP_CTOR_STR = 0x01003960, 0x01003A10, 0x01003A60, 0x01003A70
DIRP_CTOR_COPY, DIRP_ASSIGN_STR, DIRP_ASSIGN, STRDUP_SLASH = 0x01003AA0, 0x01003AD0, 0x01003B20, 0x01003B70
HOST_GETPATH, GLOBAL_DTOR, GLOBAL_CTOR = 0x01003BF0, 0x01003C90, 0x01003CC0
FN_GETFULLPATH, FN_GETNAME, FN_ASSIGN2 = 0x01004090, 0x010040B8, 0x010040C0
CLASSES = {  # name: (ctor(copy), ctor(const char*), ctor(), dtor, GetPath)
    "HostFileName": (0x01003D00, 0x01003D50, 0x01003DA0, 0x01003E10, HOST_GETPATH),
    "MCardFileName": (0x01003E30, 0x01003E80, 0x01003ED0, 0x01003F40, MC_GETPATH),
    "DiskFileName": (0x01003F60, 0x01003FB0, 0x01004000, 0x01004070, DISK_GETPATH),
    "FileName_Base": (0x010040E0, 0x01004120, 0x01004160, FN_BASE_DTOR, FN_GETPATH),
}


def fname(rng):
    k = rng.random()
    if k < 0.3:
        return rng.choice([b"core.gt3", b"CORE.GT3", b"IOPRP224.IMG", b"a", b"", b"sio2man.irx", b"Core.Gt3;1"])
    return rstr(rng, rng.randrange(0, 40))


def gen_strdup(rng, i, n):
    fn, nm = (STRDUP, "strdup_alloc") if i % 2 == 0 else (STRDUP_SLASH, "strdup_with_trailing_slash")
    s = None if rng.random() < 0.1 else fname(rng)
    if s is not None and rng.random() < 0.3:
        s += b"/"

    def run(t):
        p = t.cstr(s) if s is not None else 0
        r = t.call(fn, p, ret="ptr")
        if r:
            t.obs("copy", t.cstr_at(r))
    return Case("%s(%r)" % (nm, s), run)


def gen_filename(rng, i, n):
    """global ctor, then a random life of FileName objects, then the global dtor."""
    ops = []
    for _ in range(rng.randrange(1, 7)):
        ops.append((rng.choice(list(CLASSES)), rng.choice(["str", "str", "copy", "default"]), fname(rng),
                    rng.choice(["getpath", "getpath", "fullpath", "name", "assign_str", "assign", "assign2",
                                "twice"]), fname(rng), rng.choice([2, 3])))

    def run(t):
        t.call(GLOBAL_CTOR, ret="void")
        objs = []
        for cls, how, s, action, s2, flags in ops:
            ctor_copy, ctor_str, ctor0, dtor, getpath = CLASSES[cls]
            obj = t.halloc(0xC, b"\xCD" * 0xC)
            if how == "str":
                t.call(ctor_str, obj, t.cstr(s), ret="ptr")
            elif how == "copy" and objs:
                t.call(ctor_copy, obj, objs[-1][0], ret="ptr")
            else:
                t.call(ctor0, obj, ret="ptr")
                if action in ("getpath", "fullpath", "twice"):
                    t.call(FN_ASSIGN_STR, obj, t.cstr(s), ret="ptr")       # a name to build a path from
            if action in ("getpath", "twice"):
                r = t.call(getpath, obj, ret="ptr")
                t.obs("path", t.cstr_at(r) if r else None)
                if action == "twice":
                    r = t.call(getpath, obj, ret="ptr")
                    t.obs("cached path", t.cstr_at(r) if r else None)
            elif action == "fullpath":
                r = t.call(FN_GETFULLPATH, obj, ret="ptr")
                t.obs("full path", t.cstr_at(r) if r else None)
            elif action == "name":
                r = t.call(FN_GETNAME, obj, ret="ptr")
                t.obs("name", t.cstr_at(r) if r else None)
            elif action == "assign_str":
                t.call(FN_ASSIGN_STR, obj, t.cstr(s2), ret="ptr")
            elif action in ("assign", "assign2") and objs:
                t.call(FN_ASSIGN_FN if action == "assign" else FN_ASSIGN2, obj, objs[-1][0], ret="ptr")
            objs.append((obj, dtor, flags))
        for obj, dtor, flags in reversed(objs):
            t.call(dtor, obj, flags, ret="void")
        t.call(GLOBAL_DTOR, ret="void")
    return Case("FileName life: %s" % "; ".join("%s(%s %r).%s" % (c, h, s[:16], a) for c, h, s, a, _, _ in ops),
                run)


def gen_dirprefix(rng, i, n):
    s1, s2 = fname(rng), fname(rng)
    if rng.random() < 0.4:
        s1 += b"/"
    how = rng.choice(["str", "default", "copy"])

    def run(t):
        a = t.halloc(4, b"\xCD" * 4)
        b = t.halloc(4, b"\xCD" * 4)
        t.call(DIRP_CTOR_STR, a, t.cstr(s1), ret="ptr")
        if how == "str":
            t.call(DIRP_CTOR_STR, b, t.cstr(s2), ret="ptr")
        elif how == "default":
            t.call(DIRP_CTOR0, b, ret="ptr")
            t.call(DIRP_ASSIGN_STR, b, t.cstr(s2), ret="ptr")
        else:
            t.call(DIRP_CTOR_COPY, b, a, ret="ptr")
        t.obs("a", t.cstr_at(t.r32(a)))
        t.call(DIRP_ASSIGN, a, b, ret="ptr")
        t.obs("a after =", t.cstr_at(t.r32(a)))
        t.call(DIRP_DTOR, a, 2, ret="void")
        t.call(DIRP_DTOR, b, 3, ret="void")
        m = t.halloc(4, b"\xCD" * 4)
        t.call(MCBASE_CTOR, m, ret="ptr")
        t.obs("McBaseDir", t.cstr_at(t.r32(m)))
        t.call(MCBASE_DTOR, m, rng_flags, ret="void")
    rng_flags = rng.choice([2, 3])
    return Case("DirPrefix(%r) / (%s %r) / McBaseDir" % (s1, how, s2), run)


# ---- PrintFormat edge cases (the matching rounds rewrote its loop head, the
# integer fetch, the digit loops and the float tail: what the random formats
# above rarely reach)

EDGE_FORMATS = [
    # a format ending inside a directive: the '%' is dropped, the rest printed
    b"abc%", b"%", b"x%-", b"%5", b"%.", b"%.3", b"%l", b"%ll", b"%h", b"%L", b"%#", b"%+ 0-#",
    b"%%%", b"%%", b"a%%b%%", b"%-%", b"%5%", b"%q%d",
]


def rand_edge_directive(rng):
    """(format, args): long precisions / widths, every length on every integer
    conversion, the float specials, %F single patterns, %t with 1..19 digits."""
    k = rng.randrange(8)
    if k == 0:                                              # long float precisions (no '#': Q6-like bytes)
        conv = rng.choice(b"feEgG")
        prec = rng.choice([12, 15, 17, 20, 30, 40, 60, 100, 200, 330, 400, 700])
        x = rng.choice(DOUBLES + [1e-310, 4.9e-324, 1e-320, 123456789012345678.0, 2.0 ** 60, 2.0 ** -1022])
        flags = rng.choice([b"", b"-", b"+", b" ", b"0"])
        width = rng.choice([b"", b"5", b"40", b"400"])
        return b"%" + flags + width + b"." + str(prec).encode() + bytes([conv]), [("dbl", dbits(x))]
    if k == 1:                                              # Inf / NaN / denormal / extreme bit patterns
        conv = rng.choice(b"feEgG")
        bits = rng.choice([0x7FF0000000000000, 0xFFF0000000000000, 0x7FF8000000000000, 0x7FFFFFFFFFFFFFFF,
                           0x0000000000000001, 0x000FFFFFFFFFFFFF, 0x0010000000000000, 0x8000000000000001,
                           0x7FEFFFFFFFFFFFFF, 0x3FF0000000000001, 0x3FEFFFFFFFFFFFFF,
                           (rng.getrandbits(11) << 52) | rng.getrandbits(52)])
        prec = rng.choice([b"", b".0", b".1", b".3", b".9"])
        if conv in b"fF" and prec == b".0":
            prec = b".1"
        return b"%" + rng.choice([b"", b"-12", b"+", b"025"]) + prec + bytes([conv]), [("dbl", bits)]
    if k == 2:                                              # %F: hand conversion of a raw single
        fb = rng.choice([0x00000000, 0x80000000, 0x00000001, 0x00200000, 0x00400000, 0x00600000, 0x007FFFFF,
                         0x00800000, 0x3F800000, 0xBF800000, 0x7F7FFFFF, 0xFF7FFFFF, 0x7F800000, 0xFF800000,
                         0x7FC00000, 0x80600000, 0x3EAAAAAB, rng.getrandbits(32)])
        prec = rng.choice([b"", b".1", b".5", b".12", b".40"])
        return b"%" + rng.choice([b"", b"+", b"-15", b"015", b" "]) + prec + b"F", [("int", fb)]
    if k == 3:                                              # every length modifier on every integer conversion
        conv = rng.choice(b"diuoxXp")
        length = rng.choice([b"", b"h", b"l", b"ll", b"L"]) if conv != ord("p") else b""
        v = rng.choice([0, 1, -1, 0x7FFF, 0x8000, 0xFFFF, -0x8000, 0x7FFFFFFF, -0x80000000, 0xFFFFFFFF,
                        0x80000000, (1 << 63), (1 << 64) - 1, 10 ** 19 - 1, 10 ** 19, rng.getrandbits(64),
                        rng.getrandbits(32) | (rng.getrandbits(32) << 32)])
        flags = b"".join(rng.choice([b"-", b"+", b"0", b" ", b"#"]) for _ in range(rng.randrange(0, 4)))
        width = rng.choice([b"", b"1", b"8", b"22", b"*"])
        prec = rng.choice([b"", b".0", b".1", b".10", b".25", b".*"])
        args = []
        if width == b"*":
            args.append(("int", rng.choice([-30, -1, 0, 3, 26])))
        if prec == b".*":
            args.append(("int", rng.choice([-4, 0, 2, 21])))
        args.append(("int", v))
        return b"%" + flags + width + prec + length + bytes([conv]), args
    if k == 4:                                              # %t: 1..19 digits after the point
        prec = rng.randrange(1, 20)
        length = rng.choice([b"", b"h", b"l", b"ll"])
        v = rng.choice([0, 1, -1, 5, 12345, -12345, 10 ** prec, 10 ** prec - 1, -(10 ** prec), 2 ** 31,
                        rng.getrandbits(64), (1 << 63) - 1, -(1 << 63)])
        return (b"%" + rng.choice([b"", b"+", b" ", b"-30", b"030"]) + b"." + str(prec).encode() + length + b"t",
                [("int", v)])
    if k == 5:                                              # strings: precision / width / '0' flag
        s = rstr(rng, rng.randrange(0, 70))
        return (b"%" + rng.choice([b"", b"-", b"0", b"-0"]) + rng.choice([b"", b"3", b"64", b"*"]).replace(b"*", b"80")
                + rng.choice([b"", b".0", b".1", b".5", b".100"]) + b"s", [("str", None if rng.random() < 0.1 else s)])
    if k == 6:                                              # %c with odd values
        return b"%" + rng.choice([b"", b"5", b"-3"]) + b"c", [("int", rng.choice([0, 0x80, 0xFF, 0x1FF,
                                                                                 rng.getrandbits(64)]))]
    if k == 7 and rng.random() < 0.15:
        # on purpose: %t with precision 20 divides by s_pow10_u64[19], the
        # table's trailing zero word (0x0103FB18), and never ends (quirk Q6)
        return b"%" + rng.choice([b"", b"+", b"-30"]) + b".20t", [("int", rng.getrandbits(64))]
    return rng.choice(EDGE_FORMATS), []


def gen_printformat_edge(rng, i, n):
    fmt, args = b"", []
    for _ in range(rng.randrange(1, 4)):
        f, a = rand_edge_directive(rng)
        if f in EDGE_FORMATS and not f.endswith(b"%%"):
            fmt += f
            break                                           # ends the format (an unterminated directive)
        fmt += f
        args += a

    def run(t):
        strs = {v: t.cstr(v) for k, v in args if k == "str" and v is not None}
        ap = put_args(t, args, strs)
        f = t.cstr(fmt)
        out = []

        def putc(tt, a):
            out.append(a[1] & 0xFF)
            return 0
        vt = t.vtable([0, t.callback(putc, 2, "putc")])
        target = t.put(struct.pack("<I", vt) + b"\0" * 12, 16)
        t.call(PRINTFORMAT, target, f, ap, maxsteps=300000 if runaway else 40000000)
        t.obs("text", bytes(out))
    runaway = b".20t" in fmt
    return Case("PrintFormat(%r, %d args)%s" % (fmt[:120], len(args), " - runs away on purpose" if runaway else ""),
                run)


# ================================================================== io (src/mcard_io.cpp, thread_util.cpp,
# hostfs_io.cpp) and the pdistd lock helpers (src/pdistd/pdistd_misc.cpp)

MC_CLOSE_SYNC, MC_CLOSE_UNLOCK, MC_SHUTDOWN = 0x0100F130, 0x0100F198, 0x0100F1A0
MC_GETDIR_SYNC, MC_GETDIR_UNLOCK, MC_GETINFO_SYNC, MC_GETINFO_UNLOCK = 0x0100F1D0, 0x0100F288, 0x0100F290, 0x0100F338
MC_INIT_ONCE, MC_OPEN_SYNC, MC_OPEN_UNLOCK = 0x0100F340, 0x0100F3A8, 0x0100F440
MC_READ_SYNC, MC_READ_UNLOCK, MC_SYNC_WAIT = 0x0100F448, 0x0100F4D0, 0x0100F4D8
CREATE_MUTEX_SEMA, YIELD_LOW, SIGNAL_SEMA_HELPER, BUSY_WAIT = 0x0100F5D0, 0x0100F640, 0x0100F6B0, 0x0100F8D8
HOST_OPEN, HOST_LSEEK, HOST_READ, HOST_WRITE, HOST_CLOSE = 0x0100F6D0, 0x0100F718, 0x0100F770, 0x0100F7C8, 0x0100F820
HOST_GET_FILE_SIZE, HOST_FILE_EXISTS, EMPTY_FN0, EMPTY_FN1 = 0x0100F858, 0x01005B68, 0x010041C0, 0x010041C8
INTR_SUSPEND, GLOCK_ACQUIRE, GLOCK_RELEASE, BREAK_STUB0, BREAK_STUB1 = (0x01012F38, 0x01012F68, 0x01012FA0,
                                                                        0x01012FA8, 0x01012FB8)
LIBMC = dict(init=0x010157B8, open=0x01015B18, close=0x01015C88, read=0x01015EA8, sync=0x010161B8,
             getinfo=0x010162F0, getdir=0x01016478)
FIO = dict(open=0x0101F1B8, close=0x0101F440, read=0x0101F7F8, write=0x0101FA68, lseek=0x0101F5C0)

# wrapper, its unlock stub, the libmc call, the libmc call's argument count
MC_WRAPPERS = {
    "close": (MC_CLOSE_SYNC, MC_CLOSE_UNLOCK, LIBMC["close"], 1),
    "getdir": (MC_GETDIR_SYNC, MC_GETDIR_UNLOCK, LIBMC["getdir"], 6),
    "getinfo": (MC_GETINFO_SYNC, MC_GETINFO_UNLOCK, LIBMC["getinfo"], 5),
    "open": (MC_OPEN_SYNC, MC_OPEN_UNLOCK, LIBMC["open"], 4),
    "read": (MC_READ_SYNC, MC_READ_UNLOCK, LIBMC["read"], 3),
}


def sema_value(rng):
    """a lock semaphore id as the image may hold it: -1 (not created) or an id."""
    return rng.choice([0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x21, rng.randrange(2, 0x80), 0x80000000])


def int_arg(rng):
    return E32s(rng.choice([0, 1, 2, -1, 0x40, rng.getrandbits(32), rng.getrandbits(8)]))


def scripted(seq, then):
    """a stub that returns seq[0], seq[1], ... and then `then` forever."""
    k = [0]

    def f(tt, a):
        j = k[0]
        k[0] += 1
        return seq[j] if j < len(seq) else then
    return f


def yield_stub(t):
    t.stub(YIELD_LOW, lambda tt, a: 0, 0, "yield_low_priority")


def gen_mc_sync(rng, i, n):
    """a blocking libmc wrapper: WaitSema, the libmc call re-issued (with a
    yield) while busy, mc_sync_wait until sceMcSync reports 1, its own unlock
    stub (iSignalSema through signal_sema_helper), the mcserv result."""
    which = list(MC_WRAPPERS)[i % 5]
    wrap, unlock, libfn, nargs = MC_WRAPPERS[which]
    sema = sema_value(rng)
    busy = [E32s(rng.choice([-200, 1, -1, -210, 0x7FFFFFFF, rng.getrandbits(32)]))
            for _ in range(rng.choice([0, 0, 1, 2, 5]))]
    pend = [rng.choice([0, -1, 0, 2, -5]) for _ in range(rng.choice([0, 0, 1, 3]))]
    cmd = rng.randrange(1, 0x10)
    result = E32s(rng.choice([0, 1, -1, -2, -4, -7, 0x200, rng.getrandbits(32)]))
    raw = [int_arg(rng) for _ in range(nargs)]
    status = rng.choice([0x70030C13, 0x70030C12, 0x70000C01, rng.getrandbits(32)])
    sig = rng.choice([None, None, 0, -420, 7])

    def run(t):
        ret = {} if sig is None else {-0x43: sig}
        Kernel(ret=ret).install(t)
        t.cop0("Status", status)
        t.w32(t.data("s_mc_lock_sema"), sema)
        t.stub(libfn, scripted(busy, 0), nargs, "libmc %s" % which)
        yield_stub(t)

        def sync(tt, a):
            if sync.k < len(pend):
                sync.k += 1
                return pend[sync.k - 1]
            tt.w32(a[1], cmd)
            tt.w32(a[2], result)
            return 1
        sync.k = 0
        t.stub(LIBMC["sync"], sync, 3, "sceMcSync")
        args = list(raw)
        if which == "getdir":                       # (port, slot, name, mode, maxent, table)
            args[2] = t.cstr(dirname)
            args[5] = t.alloc(0x40, 64)
        elif which == "getinfo":                    # (port, slot, &type, &free, &format)
            args[2:5] = [t.alloc(4, 4), t.alloc(4, 4), t.alloc(4, 4)]
        elif which == "open":                       # (port, slot, name, mode)
            args[2] = t.cstr(b"/BASCUS-97102GAMEDATA/core.gt3")
        elif which == "read":                       # (fd, buf, size)
            args[1] = t.alloc(0x100, 64)
        t.call(wrap, *args, ret="int")
        t.obs("s_mc_lock_sema", t.r32(t.data("s_mc_lock_sema")))
        t.call(unlock, ret="void", label="%s_sync_unlock (alone)" % which)
    dirname = rstr(rng, 20)
    return Case("mc_%s_sync(sema %d, %d busy, %d pending, result %d)" % (which, E32s(sema), len(busy), len(pend),
                                                                         result), run)


def gen_mc_misc(rng, i, n):
    """mc_sync_wait alone, mc_shutdown, mc_init_once (CreateSema retries, the id 0
    rejection, sceMcInit busy loop)."""
    which = ["sync_wait", "shutdown", "init_once", "init_once"][i % 4]
    sema = sema_value(rng) if which != "init_once" or rng.random() < 0.25 else 0xFFFFFFFF
    pend = [rng.choice([0, -1, 2, -5, 0x7FFFFFFF]) for _ in range(rng.choice([0, 1, 2, 4]))]
    creates = [rng.choice([-1, -1, 0, 0]) for _ in range(rng.choice([0, 0, 1, 2, 3]))] + [rng.randrange(1, 0x40)]
    busy = [E32s(rng.choice([-200, 1, -1, rng.getrandbits(32)])) for _ in range(rng.choice([0, 1, 3]))]
    cmd, result = rng.randrange(0x10), E32s(rng.getrandbits(32))

    def run(t):
        def create(tt, a, st):
            p = tt.peek32(a[0], 6)
            tt.obs("CreateSema max/init/option", (p[1], p[2], p[5]))
            create.k += 1
            return creates[min(create.k - 1, len(creates) - 1)]
        create.k = 0
        Kernel(ret={0x40: create}).install(t)
        t.w32(t.data("s_mc_lock_sema"), sema)
        yield_stub(t)
        if which == "sync_wait":
            def sync(tt, a):
                if sync.k < len(pend):
                    sync.k += 1
                    return pend[sync.k - 1]
                tt.w32(a[1], cmd)
                tt.w32(a[2], result)
                return 1
            sync.k = 0
            t.stub(LIBMC["sync"], sync, 3, "sceMcSync")
            c, r = t.alloc(4, 4), t.alloc(4, 4)
            t.call(MC_SYNC_WAIT, c, r, ret="void")
        elif which == "shutdown":
            t.call(MC_SHUTDOWN, ret="void")
        else:
            t.stub(LIBMC["init"], scripted(busy, 0), 0, "sceMcInit")
            t.call(MC_INIT_ONCE, ret="void")
        t.obs("s_mc_lock_sema", t.r32(t.data("s_mc_lock_sema")))
    return Case("mc %s(sema %d, CreateSema %s, sceMcInit busy %d)" % (which, E32s(sema), creates, len(busy)), run)


def gen_thread_util(rng, i, n):
    """create_mutex_sema (retry on -1, recursion on id 0), signal_sema_helper,
    yield_low_priority (busy_wait_delay stubbed or real), busy_wait_delay."""
    which = ["create", "signal", "yield", "busy"][i % 4]
    init = rng.choice([0, 1, 1, 2, -1])
    creates = [rng.choice([-1, -1, 0]) for _ in range(rng.choice([0, 0, 1, 2, 4]))] + [rng.randrange(1, 0x100)]
    sid = E32s(rng.choice([0, 1, 0x21, -1, rng.getrandbits(32)]))
    sig = rng.choice([None, 0, -420, 0x21])
    status = rng.choice([0x70030C13, 0x70000C01, 0, rng.getrandbits(32)])
    tid, prio = rng.choice([0x11, 1, 0x50]), rng.choice([1, 0x40, 0x7E, 0x7F, rng.randrange(128)])
    real_busy = i % 16 == 2
    nbusy = rng.choice([0, 1, 2, 10, 33, -1, -75, 0xFFFFFFFF00000002, 0x8000000000000001, 0x00000000FFFFFFFF
                        if rng.random() < 0.05 else 3])
    if nbusy == 0x00000000FFFFFFFF:
        nbusy = 4                                   # 75 * 2^32 passes: far beyond any step budget

    def run(t):
        def create(tt, a, st):
            p = tt.peek32(a[0], 6)
            tt.obs("CreateSema max/init/option", (p[1], p[2], p[5]))
            create.k += 1
            return creates[min(create.k - 1, len(creates) - 1)]
        create.k = 0

        def refer(tt, a, st):
            tt.write(a[1], struct.pack("<12i", 0x10, 0x01000000, 0x01F00000, 0x4000, 0x01049CF0, 1, prio, 0, 0,
                                       0, 0, 0))
            return 0
        ret = {0x40: create, 0x30: refer, 0x2F: tid}
        if sig is not None:
            ret[-0x43] = sig
        Kernel(ret=ret).install(t)
        t.cop0("Status", status)
        if which == "create":
            yield_stub(t)
            t.call(CREATE_MUTEX_SEMA, init, ret="int")
        elif which == "signal":
            t.call(SIGNAL_SEMA_HELPER, sid, ret="int")
        elif which == "yield":
            # (the GCC 14 build inlines busy_wait_delay into yield_low_priority:
            # a stub would not intercept it there, so it runs for real)
            if not real_busy and t.stub_reaches(YIELD_LOW, BUSY_WAIT):
                t.stub(BUSY_WAIT, lambda tt, a: 0, 1, "busy_wait_delay")
            t.call(YIELD_LOW, ret="void", maxsteps=2000000)
        else:
            # the pass count is the whole behaviour of busy_wait_delay: compare the
            # instructions it executes (75 n + 2 passes of an 8-instruction loop)
            s0 = t.side.cpu.steps
            t.call(BUSY_WAIT, nbusy, ret="void", maxsteps=200000)
            steps = t.side.cpu.steps - s0
            t.obs("busy_wait_delay instructions", steps)
            cnt = s64(nbusy) * 75
            passes = cnt + 2 if cnt >= 0 else 1
            t.check(steps in (8 * passes + 7, 8 * passes + 8), "busy_wait_delay(%d): %d steps" % (s64(nbusy),
                                                                                                    steps))
    return Case("%s(init %d, sid %d, CreateSema %s, n %d%s)" % (which, init, sid, creates, s64(nbusy),
                                                               ", real busy_wait_delay" if real_busy else ""), run)


def gen_host_io(rng, i, n):
    """host_open/lseek/read/write/close and host_get_file_size: the pdistd global
    lock (lazily created with create_mutex_sema), the fileio call, the unlock;
    host_file_exists builds "host:" + name and probes with host_get_file_size."""
    which = ["open", "lseek", "read", "write", "close", "size", "size", "exists"][i % 8]
    sema = rng.choice([0xFFFFFFFF, 0xFFFFFFFF, 0x21, rng.randrange(1, 0x40)])
    creates = [rng.choice([-1, 0]) for _ in range(rng.choice([0, 0, 1, 2]))] + [rng.randrange(1, 0x40)]
    fd = E32s(rng.choice([0, 1, 3, -1, -2, -19, rng.getrandbits(32)]))
    pos = E32s(rng.choice([0, 2, 0x1000, 0x7FFFFFFF, -1, -22, rng.getrandbits(32)]))
    rv = E32s(rng.choice([0, 1, 0x100, -1, -5, rng.getrandbits(32)]))
    a1, a2 = int_arg(rng), int_arg(rng)
    name = rng.choice([b"/tmp/core.gt3", b"/tmp/", b"", rstr(rng, rng.randrange(1, 100))])
    data = junk(rng, 0x40)

    def run(t):
        def create(tt, a, st):
            p = tt.peek32(a[0], 6)
            tt.obs("CreateSema max/init/option", (p[1], p[2], p[5]))
            create.k += 1
            return creates[min(create.k - 1, len(creates) - 1)]
        create.k = 0
        Kernel(ret={0x40: create}).install(t)
        t.w32(t.data("s_globalLockSema"), sema)
        yield_stub(t)

        def op(tt, a):
            tt.obs("sceOpen", (tt.cstr_at(a[0]), E32s(a[1])))
            return fd
        t.stub(FIO["open"], op, 2, "sceOpen", log=0)
        t.stub(FIO["lseek"], lambda tt, a: pos, 3, "sceLseek")
        t.stub(FIO["read"], lambda tt, a: rv, 3, "sceRead")
        t.stub(FIO["write"], lambda tt, a: rv, 3, "sceWrite")
        t.stub(FIO["close"], lambda tt, a: rv, 1, "sceClose")
        if which == "open":
            t.call(HOST_OPEN, t.cstr(name), a1, ret="int")
        elif which == "lseek":
            t.call(HOST_LSEEK, fd, a1, a2, ret="int")
        elif which == "read":
            t.call(HOST_READ, fd, t.alloc(0x40, 16), a1, ret="int")
        elif which == "write":
            t.call(HOST_WRITE, fd, t.put(data, 16), a1, ret="int")
        elif which == "close":
            t.call(HOST_CLOSE, fd, ret="int")
        elif which == "size":
            out = t.put(b"\xA5" * 16, 16)
            t.call(HOST_GET_FILE_SIZE, t.cstr(name), out, ret="int")
        else:
            t.call(HOST_FILE_EXISTS, t.cstr(name), ret="u8")
        t.obs("s_globalLockSema", t.r32(t.data("s_globalLockSema")))
    return Case("host %s(%r, sema %d, fd %d, pos %d, rv %d)" % (which, name[:24], E32s(sema), fd, pos, rv), run)


def gen_pdistd_lock(rng, i, n):
    """intr_suspend (the di errata loop; returns the old Status.EIE),
    global_lock_acquire / release, the two break stubs (trap on purpose) and
    imageloader.cxx's two empty functions (filename unit)."""
    which = ["intr", "intr", "acquire", "release", "break0", "break1", "empty"][i % 7]
    status = rng.choice([0x70030C13, 0x70030C12, 0x70020C13, 0x00010001, 0x00010000, 0, 0xFFFFFFFF,
                         rng.getrandbits(32)])
    sema = rng.choice([0xFFFFFFFF, 0xFFFFFFFF, 0, 0x21, rng.randrange(1, 0x40), 0x80000000])
    creates = [rng.choice([-1, 0]) for _ in range(rng.choice([0, 0, 1]))] + [rng.randrange(1, 0x40)]
    wait = rng.choice([None, 0, -1, 0x21])

    def run(t):
        def create(tt, a, st):
            p = tt.peek32(a[0], 6)
            tt.obs("CreateSema max/init/option", (p[1], p[2], p[5]))
            create.k += 1
            return creates[min(create.k - 1, len(creates) - 1)]
        create.k = 0
        ret = {0x40: create}
        if wait is not None:
            ret[0x44] = wait
        Kernel(ret=ret).install(t)
        t.cop0("Status", status)
        t.w32(t.data("s_globalLockSema"), sema)
        yield_stub(t)
        if which == "intr":
            t.call(INTR_SUSPEND, ret="int")
            t.obs("Status after", t.cop0("Status"))
        elif which == "acquire":
            t.call(GLOCK_ACQUIRE, ret="int")
        elif which == "release":
            t.call(GLOCK_RELEASE, ret="void")
        elif which == "break0":
            t.call(BREAK_STUB0, ret="int", maxsteps=100)
        elif which == "break1":
            t.call(BREAK_STUB1, 6, ret="int", maxsteps=100)
        else:
            t.call(EMPTY_FN0, ret="void")
            t.call(EMPTY_FN1, ret="void")
        t.obs("s_globalLockSema", t.r32(t.data("s_globalLockSema")))
    return Case("%s(Status %08X, sema %d)%s" % (which, status, E32s(sema),
                                               " - traps on purpose" if which.startswith("break") else ""), run)


# ================================================================== pdistd::Allocator (src/pdistd/allocator.cpp)

AL = dict(ctor_int=0x0100F918, ctor_arena=0x0100F940, dtor=0x0100F978, align_rep=0x0100F9B8,
          rebuild_head=0x0100FA78, insert_rep=0x0100FAA8, unlink_rep=0x0100FAD8, push_free=0x0100FAF8,
          remove_free=0x0100FB18, assign=0x0100FB40, unassign=0x0100FBA0, allocate=0x0100FBB8,
          reallocate=0x0100FD10, free=0x0100FED8, max_free=0x01010010, total_free=0x01010078, debug=0x010100E0,
          count_free=0x010101D8, rep_valid=0x01010278, reset=0x010102C0)
HEAPF = dict(init_once=0x01011210, max_free=0x010112A0, total_free=0x010112C0, dump=0x010112E0, count=0x01011300,
             calloc=0x01011320, free=0x01011368, malloc=0x01011390, realloc=0x010113C8, memalign=0x01011410,
             gdtor=0x01011458, gctor=0x01011480)
TYPEINFO_FNS = [0x01003DC0, 0x01003EF0, 0x01004020, 0x01004180, 0x010102D8, 0x010111D0, 0x010116B0]


def pick_size(rng, big=2048):
    return rng.choice([0, 1, 15, 16, 17, 32, 48, 100, 0x200, rng.randrange(0, 64), rng.randrange(0, big)])


def pick_align(rng):
    return rng.choice([0, 1, 8, 16, 16, 16, 16, 32, 32, 64, 128, 256])


def heap_ops(rng, nops):
    ops = []
    for _ in range(nops):
        k = rng.random()
        if k < 0.38:
            ops.append(("alloc", pick_size(rng), pick_align(rng)))
        elif k < 0.52:
            ops.append(("realloc", rng.randrange(1 << 16), pick_size(rng), pick_align(rng), rng.random() < 0.1))
        elif k < 0.78:
            ops.append(("free", rng.randrange(1 << 16)))
        elif k < 0.82:
            ops.append(("bad", rng.choice(["free_null", "free_outside", "free_mid", "realloc_mid", "realloc_outside"]),
                        rng.randrange(1 << 16), pick_size(rng)))
        elif k < 0.86:
            ops.append(("max",))
        elif k < 0.90:
            ops.append(("total",))
        elif k < 0.94:
            ops.append(("count",))
        elif k < 0.97:
            ops.append(("valid", rng.randrange(1 << 16), rng.random() < 0.3))
        else:
            ops.append(("debug",))
    return ops


def fill_pattern(seed, n):
    """payload bytes 0x00..0x7F (never four 0xFF bytes: no fake 'allocated' marker)."""
    return bytes(((seed * 37 + k * 13) >> 1) & 0x7F for k in range(n))


class HeapDriver:
    """runs heap_ops against one allocator interface; keeps the live blocks (as
    the code returned them) and a size budget so that no allocation can fail
    in a realloc (whose allocate() result the original does not check)."""

    def __init__(self, t, arena, size, api):
        self.t, self.arena, self.size, self.api = t, arena, size, api
        self.live = []                              # (ptr, size, align)
        self.seed = 1

    def cost(self, size, align):
        return ((size + 15) & ~15) + 0x40 + max(align, 16)

    def used(self):
        return sum(self.cost(s, a) for _, s, a in self.live)

    def fits(self, size, align):
        return len(self.live) < 24 and self.used() + self.cost(size, align) <= self.size // 3

    def fill(self, p, n):
        self.seed += 1
        if n:
            self.t.write(p, fill_pattern(self.seed, n))

    def run(self, ops):
        t, api = self.t, self.api
        for op in ops:
            kind = op[0]
            if kind == "alloc":
                _, size, align = op
                if not self.fits(size, align):
                    kind, op = "free", ("free", 0)
                else:
                    p = api.alloc(size, align)
                    if p:
                        self.live.append((p, size, align))
                        self.fill(p, size)
                    continue
            if kind == "realloc":
                _, k, size, align, null = op
                if null or not self.live:
                    if not self.fits(size, align):
                        continue
                    p = api.realloc(0, size, align)
                    if p:
                        self.live.append((p, size, align))
                        self.fill(p, size)
                    continue
                j = k % len(self.live)
                old, osize, oalign = self.live[j]
                if self.used() - self.cost(osize, oalign) + self.cost(size + 0x20, max(align, oalign)) > \
                        self.size // 3:
                    continue
                p = api.realloc(old, size, align)
                if p:
                    self.live[j] = (p, size, max(align, oalign))
                continue
            if kind == "free":
                if self.live:
                    p, _, _ = self.live.pop(op[1] % len(self.live))
                    api.free(p)
                continue
            if kind == "bad":
                _, how, k, size = op
                if how == "free_null":
                    api.free(0)
                elif how == "free_outside":
                    api.free(self.arena - 0x100 + (k & 0xF0))
                elif how == "realloc_outside":
                    api.realloc(self.arena + self.size + 0x40 + (k & 0xF0), size, 16)
                elif self.live:
                    p, s, _ = self.live[k % len(self.live)]
                    q = p + 16 * (1 + (k >> 4) % max(1, (s + 15) // 16))
                    if how == "free_mid":
                        api.free(q)
                    else:
                        api.realloc(q, size, 16)
                continue
            if kind == "valid":
                _, k, mid = op
                if self.live:
                    p, s, _ = self.live[k % len(self.live)]
                    api.valid(p - 16 + (16 if mid else 0))
                continue
            getattr(api, kind)()


class ObjApi:
    """an Allocator object of the test's own over an arena in test memory."""

    def __init__(self, t, obj):
        self.t, self.obj = t, obj

    def alloc(self, size, align):
        return self.t.call(AL["allocate"], self.obj, size, align, ret="ptr")

    def realloc(self, p, size, align):
        return self.t.call(AL["reallocate"], self.obj, p, size, align, ret="ptr")

    def free(self, p):
        self.t.call(AL["free"], self.obj, p, ret="void")

    def valid(self, rep):
        self.t.call(AL["rep_valid"], self.obj, rep, ret="u8")

    def max(self):
        self.t.call(AL["max_free"], self.obj, ret="u32")

    def total(self):
        self.t.call(AL["total_free"], self.obj, ret="u32")

    def count(self):
        self.t.call(AL["count_free"], self.obj, ret="int")

    def debug(self):
        self.t.call(AL["debug"], self.obj, ret="void")


class FrontApi:
    """the C heap front-ends over the global Allocator (the align argument goes
    to memalign; malloc / calloc / realloc always use 16)."""

    def __init__(self, t, mode):
        self.t, self.mode = t, mode

    def alloc(self, size, align):
        t = self.t
        if align > 16 or self.mode == 1:
            return t.call(HEAPF["memalign"], align, size, ret="ptr")
        if self.mode == 2 and size:
            return t.call(HEAPF["calloc"], 1 + size % 3, size // (1 + size % 3), ret="ptr")
        return t.call(HEAPF["malloc"], size, ret="ptr")

    def realloc(self, p, size, align):
        return self.t.call(HEAPF["realloc"], p, size, ret="ptr")

    def free(self, p):
        self.t.call(HEAPF["free"], p, ret="void")

    def valid(self, rep):
        self.t.call(AL["rep_valid"], self.t.data("s_allocator"), rep, ret="u8")

    def max(self):
        self.t.call(HEAPF["max_free"], ret="u32")

    def total(self):
        self.t.call(HEAPF["total_free"], ret="u32")

    def count(self):
        self.t.call(HEAPF["count"], ret="int")

    def debug(self):
        self.t.call(HEAPF["dump"], ret="void")


def gen_allocator(rng, i, n):
    """pdistd::Allocator on an object and an arena of the test: a constructor,
    a random life of allocate / reallocate / free (best and first fit, aligned
    blocks, invalid pointers on purpose), the queries and the debug dump, then
    unassign / reset and the destructor."""
    ctor = rng.choice(["int+assign", "arena", "arena"])
    first_fit = rng.choice([0, 0, 1, 7])
    size = rng.choice([0x8000, 0x10000, 0x30000]) + rng.randrange(0, 0x40)
    off = rng.choice([0, 0, 4, 8, 12, rng.randrange(16)])
    ops = heap_ops(rng, rng.randrange(4, 40))
    end = rng.choice(["dtor", "unassign", "reset", "none"])
    flags = rng.choice([2, 3])

    def run(t):
        printf_stub(t)
        obj = t.halloc_obj(0x18, LAYOUT["Allocator"], b"\xCD" * 0x18)
        arena = t.alloc(size + 0x40, 16)
        base = arena + off
        if ctor == "arena":
            t.call(AL["ctor_arena"], obj, base, size, first_fit, ret="ptr")
        else:
            t.call(AL["ctor_int"], obj, first_fit, ret="ptr")
            t.call(AL["assign"], obj, base, size, ret="ptr")
        HeapDriver(t, base, size, ObjApi(t, obj)).run(ops)
        t.call(AL["count_free"], obj, ret="int")
        t.call(AL["total_free"], obj, ret="u32")
        t.obs("allocator", t.objwords(obj, 0x18), "ptrs")
        if end == "unassign":
            t.call(AL["unassign"], obj, ret="ptr")
        elif end == "reset":
            t.call(AL["reset"], obj, ret="void")
        if end != "none":
            t.call(AL["dtor"], obj, flags, ret="void")
    return Case("Allocator(%s, firstFit %d, arena %#x+%d): %d ops, %s" % (ctor, first_fit, size, off, len(ops), end),
                run)


def gen_allocator_prims(rng, i, n):
    """the private chain / free-list primitives called directly on a small heap
    built by allocate / free (alignRepStart with every gap class,
    rebuildHeadRep, insertRepAt, unlinkRep, pushFreeRep, removeFreeRep)."""
    which = ["align_rep", "align_rep", "rebuild_head", "insert_rep", "unlink_rep", "push_free", "remove_free"][i % 7]
    sizes = [pick_size(rng, 512) for _ in range(rng.randrange(2, 8))]
    frees = [rng.random() < 0.4 for _ in sizes]
    pick, pick2 = rng.randrange(1 << 16), rng.randrange(1 << 16)
    align = rng.choice([16, 32, 64, 128, 256, 0x1000])
    offset = rng.choice([0x10, 0x20, 0x30, 0x100])
    shift = rng.choice([0, 0x10, 0x20, 0x30])

    def run(t):
        obj = t.halloc_obj(0x18, LAYOUT["Allocator"], b"\xCD" * 0x18)
        arena = t.alloc(0x4000, 16)
        t.call(AL["ctor_arena"], obj, arena + shift, 0x3F00, 0, ret="ptr")
        ptrs = [t.call(AL["allocate"], obj, s, 16, ret="ptr") for s in sizes]
        for p, f in zip(ptrs, frees):
            if f and p:
                t.call(AL["free"], obj, p, ret="void")
        reps = []
        r = t.r32(t.m(obj, 8))                      # m_head, then the address chain
        while arena <= r < arena + 0x4000 and len(reps) < 64:
            reps.append(r)
            r = t.r32(r + 4)
        rep = reps[pick % len(reps)]
        rep2 = reps[pick2 % len(reps)]
        if which == "align_rep":
            t.call(AL["align_rep"], obj, rep, align, ret="ptr")
        elif which == "rebuild_head":
            t.call(AL["rebuild_head"], obj, rep, ret="void")
        elif which == "insert_rep":
            t.call(AL["insert_rep"], obj, rep, offset, ret="ptr")
        elif which == "unlink_rep":
            t.call(AL["unlink_rep"], obj, rep, t.r32(rep + 4), ret="void")
        elif which == "push_free":
            t.call(AL["push_free"], obj, rep, ret="void")
        else:
            free_reps = []
            f = t.r32(t.m(obj, 0xC))
            while f and len(free_reps) < 64:
                free_reps.append(f)
                f = t.r32(f + 0xC)
            if free_reps:
                t.call(AL["remove_free"], obj, free_reps[pick2 % len(free_reps)], ret="void")
        t.obs("allocator", t.objwords(obj, 0x18), "ptrs")
    return Case("Allocator::%s(%d blocks, align %#x, offset %#x, arena +%#x)" % (which, len(sizes), align, offset,
                                                                                 shift), run)


def gen_heap_frontend(rng, i, n):
    """the C heap: the global allocator's static ctor / dtor, heap_init_once over
    an arena of the test (s_heapStart / s_heapLimit pointed at it, so the
    one-time memset covers test memory), malloc / calloc / memalign / realloc /
    free and the heap_* query wrappers."""
    size = rng.choice([0x6000, 0x10000, 0x20000]) + rng.randrange(0, 0x40)
    off = rng.randrange(16)
    mode = rng.choice([0, 0, 1, 2])
    ctor = rng.random() < 0.7
    ops = heap_ops(rng, rng.randrange(3, 30))
    twice = rng.random() < 0.3

    def run(t):
        for f in ("malloc", "free", "calloc", "realloc", "memalign"):
            t.native(HEAPF[f])
        printf_stub(t)
        arena = t.alloc(size + 0x40, 16)
        t.w32(t.data("s_heapStart"), arena + off)
        t.w32(t.data("s_heapLimit"), arena + off + size)
        if ctor:
            t.call(HEAPF["gctor"], ret="void")
        if twice:
            t.call(HEAPF["init_once"], ret="void", maxsteps=5000000)
        base = (arena + off + 15) & ~15
        HeapDriver(t, base, size - 16, FrontApi(t, mode)).run(ops)
        t.call(HEAPF["count"], ret="int")
        t.call(HEAPF["max_free"], ret="u32")
        t.obs("heap base/size", (t.r32(t.data("s_heapBase")), t.r32(t.data("s_heapSize"))), "ptrs")
        if ctor:
            t.call(HEAPF["gdtor"], ret="void")
    return Case("heap(arena %#x+%d, mode %d, ctor %d): %d ops" % (size, off, mode, ctor, len(ops)), run)


SI_TYPE_INFO_CTOR = 0x01019C38          # __si_type_info::__si_type_info(this, name, base) (cxxrt)
TF_PRINTTARGET = 0x010116B0


def gen_typeinfo(rng, i, n):
    """the group's typeinfo functions: built once, the same node on a second call.

    Known difference, not compared: the RTTI name of the anonymous-namespace
    PrintTarget.  Retail's is "Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget":
    egcs named printf.cxx's anonymous namespace after the file name plus six
    random characters (append_random_chars: time and pid), because nothing
    global was defined in that object before it.  A rebuild cannot reproduce
    the random part (here the namespace is named after the first global symbol
    of printf.cpp instead), and nothing in the program reads the name.  So for
    0x010116B0 the __si_type_info constructor is stubbed: its this / base
    arguments, the base typeinfo call and the returned node are compared, the
    name string is not (the node is not written either, so the second call
    builds it again on both sides)."""
    order = list(TYPEINFO_FNS)
    rng.shuffle(order)

    def run(t):
        for f in order:
            if f == TF_PRINTTARGET:
                t.stub(SI_TYPE_INFO_CTOR, lambda tt, a: a[0], 3, "__si_type_info ctor (name not compared)",
                       log=1)
                t.call(f, ret="ptr")
                t.call(f, ret="ptr", label="%08X (again)" % f)
                t.native(SI_TYPE_INFO_CTOR)
                continue
            r = t.call(f, ret="ptr")
            t.obs("name", t.cstr_at(t.r32(r)))
            t.call(f, ret="ptr", label="%08X (again)" % f)
    return Case("typeinfo functions in order %s" % ", ".join("%08X" % f for f in order), run)



SPECS = [
    # printf
    Spec("printformat", [PRINTFORMAT, PF_BASE_PUTC], gen_printformat, 400),
    Spec("vprintf", [VPRINTF, PRINTF, PT_CTOR, PT_PUTC, PT_FLUSH, PT_DTOR], gen_vprintf, 200,
         "every 50th case runs away on purpose"),
    Spec("printtarget", [PT_CTOR, PT_PUTC, PT_FLUSH, PT_DTOR], gen_printtarget, 100),
    Spec("printformat_base", [PF_BASE_CTOR, PF_BASE_PUTC, PF_BASE_DTOR], gen_pf_base, 10),
    Spec("tty_puts", [TTY_PUTS], gen_tty_puts, 20),
    # FileName
    Spec("strdup", [STRDUP, STRDUP_SLASH], gen_strdup, 150),
    Spec("filename", [GLOBAL_CTOR, GLOBAL_DTOR, FN_ASSIGN_STR, FN_ASSIGN_FN, FN_ASSIGN2, FN_GETFULLPATH, FN_GETNAME]
         + [a for v in CLASSES.values() for a in v], gen_filename, 150),
    Spec("dirprefix", [DIRP_CTOR_STR, DIRP_CTOR0, DIRP_CTOR_COPY, DIRP_ASSIGN_STR, DIRP_ASSIGN, DIRP_DTOR,
                       MCBASE_CTOR, MCBASE_DTOR], gen_dirprefix, 60),
    Spec("printformat_edge", [PRINTFORMAT], gen_printformat_edge, 300,
         "long precisions, float specials, %F singles, every length, %t, unterminated directives"),
    # io
    Spec("mc_sync", [w for v in MC_WRAPPERS.values() for w in v[:2]], gen_mc_sync, 100,
         "libmc stubbed: busy re-issue, pending sceMcSync, the unlock stubs"),
    Spec("mc_misc", [MC_SYNC_WAIT, MC_SHUTDOWN, MC_INIT_ONCE], gen_mc_misc, 60),
    Spec("thread_util", [CREATE_MUTEX_SEMA, SIGNAL_SEMA_HELPER, YIELD_LOW, BUSY_WAIT], gen_thread_util, 64,
         "busy_wait_delay: the executed instruction count is compared"),
    Spec("host_io", [HOST_OPEN, HOST_LSEEK, HOST_READ, HOST_WRITE, HOST_CLOSE, HOST_GET_FILE_SIZE, HOST_FILE_EXISTS],
         gen_host_io, 80, "fileio stubbed"),
    Spec("pdistd_lock", [INTR_SUSPEND, GLOCK_ACQUIRE, GLOCK_RELEASE, BREAK_STUB0, BREAK_STUB1, EMPTY_FN0, EMPTY_FN1],
         gen_pdistd_lock, 42, "the break stubs trap on purpose"),
    # allocator
    Spec("allocator", [AL[k] for k in ("ctor_int", "ctor_arena", "dtor", "assign", "unassign", "allocate",
                                       "reallocate", "free", "max_free", "total_free", "debug", "count_free",
                                       "rep_valid", "reset")], gen_allocator, 120),
    Spec("allocator_prims", [AL[k] for k in ("ctor_arena", "allocate", "free", "align_rep", "rebuild_head",
                                             "insert_rep", "unlink_rep", "push_free", "remove_free")],
         gen_allocator_prims, 70),
    Spec("heap_frontend", list(HEAPF.values()) + [AL["rep_valid"]], gen_heap_frontend, 60,
         "the arena is test memory (s_heapStart / s_heapLimit)"),
    Spec("pdistd_typeinfo", TYPEINFO_FNS, gen_typeinfo, 6),
]
