/* kprintf.c - Sony libkernl 2240 SIO putc / kernel printf
 * object, core 0x00287268..0x00287BDF (sifcmd.o's first function, set_sreg
 * at 0x00287BE0, is in sifcmd.c).  Same machine code as
 * the printf part of decomp/bootstrap/sdk/libkernl/kernel.c; the names are
 * this binary's (index.txt): sio_putchar = sio_putc, deci2_sink =
 * _kputs_putc, sio_putchar_crlf = _sio_putchar_crlf, sce_doprnt =
 * _kernel_vprintf, sce_printf_sio = _printf, sce_printf_deci2 = scePrintf.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* The object's data, file statics (the bootstrap's kernel.c has them static
 * too): .data _kputs_count, _putc_hook in this order; .bss _kputs_buf
 * (between tty.o's and sifcmd.o's statics in the original). */
static int  _kputs_count = 0;                           /* 0x003081BC chars buffered */
/* Output hook of the kernel printf engine (symbols: stdout_sink_ptr),
 * initially sio_putchar_crlf (0x00287350); sce_printf_deci2 swaps in
 * deci2_sink while it formats.                                               */
static void (*_putc_hook)(int c) = sio_putchar_crlf;    /* 0x003081C0 */
static char _kputs_buf[KPUTS_BUF_SIZE];                 /* 0x004ADE40 DECI2 kputs line buffer */

// FUNC 0x00287268 sio_putchar
//   Busy-waits while SIO_ISR_TXFULL (bit 15) is set, then writes the byte
//   to SIO_TXFIFO; returns c.  The three nops in the poll loop are the SCE
//   compiler's short-loop padding (R5900 erratum); GCC 14 does not pad, so
//   they are written out here.
#ifdef BOOT_OLD_GCC
#define SIO_POLL_PAD() ((void)0)
#else
#define SIO_POLL_PAD() __asm__ __volatile__("nop\n\tnop\n\tnop")
#endif
int sio_putchar(int c)
{
    while (REG32(SIO_ISR) & SIO_ISR_TXFULL)
        SIO_POLL_PAD();
    *(volatile u8 *)SIO_TXFIFO = (u8)c;
    return c;
}
#undef SIO_POLL_PAD

// FUNC 0x002872A0 deci2_sink
//   Line-buffered putc for sce_printf_deci2: when 0x7E chars are pending
//   (no room left for a char and its NUL), NUL-terminate at [0x7F] (sic,
//   not at the count) and flush the buffer with deci2_write (kPuts) first;
//   '\n' is stored, terminated and the line flushed (tail call); any other
//   char is just appended.
static void deci2_sink(int c)
{
    int count = _kputs_count;

    if (!(count < KPUTS_BUF_SIZE - 2)) {
        _kputs_count = 0;
        _kputs_buf[KPUTS_BUF_SIZE - 1] = '\0';
        deci2_write(_kputs_buf);
        count = _kputs_count;
    }
    if (c == '\n') {
        _kputs_count = 0;
        _kputs_buf[count] = (char)c;
        _kputs_buf[count + 1] = '\0';
        deci2_write(_kputs_buf);
    } else {
        _kputs_count = count + 1;
        _kputs_buf[count] = (char)c;
    }
}

// FUNC 0x00287350 sio_putchar_crlf
//   Default putc hook (initial value of _putc_hook @0x003081C0): '\n' ->
//   sio_putchar('\r') then sio_putchar('\n'); otherwise sio_putchar(c).
void sio_putchar_crlf(int c)
{
    if (c == '\n') {
        sio_putchar('\r');
        sio_putchar('\n');
        return;
    }
    sio_putchar(c);
}

// FUNC 0x00287388 _dtoi
//   Treats the 64-bit argument as the bit pattern of an IEEE double and
//   converts it to int: exp = biased exponent - (1023 + 52); exp < -53 -> 0;
//   exp >= 13 -> 9999; else m = fraction | 1<<52, shifted left by exp, or
//   (exp < 0) right by (-exp - 2) (shift count taken mod 64) and then by 2
//   more, adding 1 only when both dropped bits are 1.  Returns the low 32
//   bits.  NB: its only caller passes an INTEGER (see _print_double), not a
//   double bit pattern.  dsrlv/dsllv use the count's low 6 bits; the
//   explicit & 63 gives GCC 14 the same defined behaviour.
#ifdef BOOT_OLD_GCC
#define DTOI_SHIFT(n) (n)
#else
#define DTOI_SHIFT(n) ((int)(n) & 63)
#endif
static int _dtoi(u64 bits)
{
    s64 exp;

    exp = (bits << 1) >> 53;                /* biased exponent */
    exp -= 1023 + 52;                       /* 0x433: value = mantissa << exp */
    if (exp < -53)
        return 0;
    if (exp >= 13)
        return 9999;
    bits = (bits << 12) >> 12;              /* the 52 fraction bits */
    bits |= (u64)1 << 52;                   /* + the implicit 1 */
    if (exp < 0) {
        exp = -exp;
        bits >>= DTOI_SHIFT(exp - 2);
        if ((bits & 3) == 3)
            bits = (bits >> 2) + 1;
        else
            bits >>= 2;
    } else {
        bits <<= DTOI_SHIFT(exp);
    }
    return (int)bits;
}
#undef DTOI_SHIFT

// FUNC 0x00287418 _print_double
//   %e/%f back end of the kernel printf.  Negative: d = 0.0 - d (dpsub) and
//   '-' through the putc hook.  Then scales d into [0.1, 1): while d < 0.1
//   d *= 10 (dpmul, exp--), else while !(d < 1.0) d /= 10 (dpdiv, exp++).
//   Prints "0.%d" of _dtoi((u64)(d * 1e6)) and then "e+%d" (exp >= 0) or
//   "e%d" (tail call).
//   BUG (faithful): __fixunsdfdi yields an integer < 1e6 that _dtoi decodes
//   as a double bit pattern (exponent field 0 -> returns 0), so every
//   non-zero value prints as "0.0e<exp>".  All compares go through dpcmp
//   (NaN compares as "greater": a NaN/Inf argument loops forever in the /10
//   loop, hence the !(d < x) forms).  Constants: 0.1 @0x003491A8 and
//   @0x003491B0, 1e6 @0x003491B8; 10.0 and 1.0 are built inline.
static void _print_double(double d)
{
    int exp = 0;
    int digits;

    if (d < 0.0) {                          /* implicit: dpcmp */
        d = 0.0 - d;                        /* implicit: dpsub */
        _putc_hook('-');
    }
    if (d < 0.1) {
        while (d < 0.1) {
            d = d * 10.0;                   /* implicit: dpmul */
            exp--;
        }
    } else if (!(d < 1.0)) {
        while (!(d < 1.0)) {
            d = d / 10.0;                   /* implicit: dpdiv */
            exp++;
        }
    }
    digits = _dtoi((u64)(d * 1000000.0));   /* implicit: dpmul, __fixunsdfdi */
    sce_printf_sio("0.%d", digits);
    if (exp >= 0)
        sce_printf_sio("e+%d", exp);
    else
        sce_printf_sio("e%d", exp);
}

// FUNC 0x00287580 sce_doprnt
//   The kernel printf engine.  `ap` points at 64-bit argument slots (the
//   callers spill $a1..$t3 with sd; each argument occupies one slot and is
//   read from its low bytes - see KARG).  Everything goes out through
//   _putc_hook, one (signed) char at a time.  Conversion switch = 0x49-entry
//   jump table @0x003491C0 indexed by (c - '0'):
//     '0'  "%0N" / "%0NN" (clamped to 31) pre-fills N '0's to the left of the
//          digit area; a '0' not followed by a digit is skipped.  fmt[2] is
//          read before fmt[1] is tested.  Only o/x/d/u use the padding.
//          (Width digits without a leading '0', e.g. "%5d", are NOT
//          understood: '5' hits the default case and "d" prints literally.)
//     'l'  64-bit argument (ld), 'h' 16-bit (lhu / lh); default 32-bit.
//     'o'/'x'/'u'  unsigned octal / lower-case hex / decimal (__umoddi3,
//          __udivdi3); 'd' signed decimal (__moddi3, __divdi3) with '-'
//          emitted immediately, before any padding.
//     'e','f'  float slot (lwc1): 0.0f prints "0", else _print_double.
//     's'  prints "(null)" when the FIRST CHAR is NUL (the pointer is
//          dereferenced first; NULL is not special-cased); no padding.
//     'c'  low byte of the slot, sign-extended.
//     anything else (incl. '%', '1'..'9', 'g', 'p', 'X') is swallowed - also a
//          NUL right after '%', 'l', 'h' or "%0": the pointer then steps PAST
//          the terminator and formatting continues in the memory after it.
//   Returns 0: the original has no return statement and leaves $v0 holding
//   the loop test's load of the terminating NUL.
#define KARG(ap, T) (*(T *)(void *)((ap += 1) - 1))  /* EE va_arg: 8-byte slots */
static int sce_doprnt(const char *fmt, u64 *ap)
{
    char  buf[32];      /* sp+0x00; digits built backwards from buf[30], buf[31] = NUL */
    char *p;            /* $s0 while emitting a field                              */
    char *pad;          /* $s4: first '0' of the "%0N" fill, NULL = none           */
    int   mod;          /* $a3: 0, 'l' or 'h'                                      */
    int   width;
    u64   uval;
    s64   sval;
    char  c;

    for (; *fmt != '\0'; fmt++) {
        c = *fmt;
        pad = NULL;
        mod = 0;
        if (c == '%') {
        again:
            switch (*++fmt) {
            case '0': {
                int second;     /* fmt[2]: a possible second width digit */

                width = fmt[1] - '0';
                second = fmt[2];
                if ((u8)width < 10) {
                    if ((unsigned int)(second - '0') < 10) {
                        width = width * 10 + (second - '0');
                        fmt += 2;
                        if (!(width < 32))
                            width = 31;
                    } else {
                        fmt += 1;
                    }
                    pad = buf + (31 - width);
                    for (; width > 0; width--)
                        buf[31 - width] = '0';
                }
                goto again;
            }

            case 'l':
                mod = 'l';
                goto again;

            case 'h':
                mod = 'h';
                goto again;

            case 'o':
                if (mod == 'l')
                    uval = KARG(ap, u64);
                else if (mod == 'h')
                    uval = KARG(ap, u16);
                else
                    uval = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uval == 0) {
                    *--p = '0';
                } else {
                    while (uval != 0) {
                        *--p = (char)((uval & 7) + '0');
                        uval >>= 3;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'x':
                if (mod == 'l')
                    uval = KARG(ap, u64);
                else if (mod == 'h')
                    uval = KARG(ap, u16);
                else
                    uval = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uval == 0) {
                    *--p = '0';
                } else {
                    while (uval != 0) {
                        if ((uval & 0xF) < 10)
                            *--p = (char)((uval & 0xF) + '0');
                        else
                            *--p = (char)((uval & 0xF) + 'a' - 10);
                        uval >>= 4;
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'd':
                if (mod == 'l')
                    sval = KARG(ap, s64);
                else if (mod == 'h')
                    sval = KARG(ap, s16);
                else
                    sval = KARG(ap, s32);
                p = &buf[31];
                *p = '\0';
                if (sval == 0) {
                    *--p = '0';
                } else {
                    if (sval < 0) {
                        sval = -sval;
                        _putc_hook('-');
                    }
                    while (sval != 0) {
                        *--p = (char)(sval % 10 + '0');     /* implicit: __moddi3 */
                        sval = sval / 10;                   /* implicit: __divdi3 */
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'u':
                if (mod == 'l')
                    uval = KARG(ap, u64);
                else if (mod == 'h')
                    uval = KARG(ap, u16);
                else
                    uval = KARG(ap, u32);
                p = &buf[31];
                *p = '\0';
                if (uval == 0) {
                    *--p = '0';
                } else {
                    while (uval != 0) {
                        *--p = (char)(uval % 10 + '0');     /* implicit: __umoddi3 */
                        uval = uval / 10;                   /* implicit: __udivdi3 */
                    }
                }
                if (pad != NULL && pad < p)
                    p = pad;
                while (*p != '\0')
                    _putc_hook((s8)*p++);
                break;

            case 'e':
            case 'f': {
                float f = KARG(ap, float);

                if (f == 0.0f)
                    _putc_hook('0');
                else
                    _print_double((double)f);   /* implicit: fptodp */
                break;
            }

            case 's': {
                const char *s = KARG(ap, const char *);

                if (*s == '\0') {
                    _putc_hook('(');
                    _putc_hook('n');
                    _putc_hook('u');
                    _putc_hook('l');
                    _putc_hook('l');
                    _putc_hook(')');
                } else {
                    p = (char *)s;
                    do {
                        _putc_hook((s8)*p);
                        p++;
                    } while (*p != '\0');
                }
                break;
            }

            case 'c':
                sval = KARG(ap, s8);
                _putc_hook((int)sval);
                break;

            default:
                break;
            }
        } else {
            _putc_hook(c);
        }
    }
#ifndef BOOT_OLD_GCC
    return 0;   /* see above: egcs leaves the loop test's 0 in $v0 (no `move') */
#endif
}
#undef KARG

// FUNC 0x00287B48 sce_printf_sio
//   Varargs shim: spills $a1..$t3 as 64-bit slots and runs sce_doprnt with
//   the CURRENT putc hook (SIO CR/LF unless sce_printf_deci2 is active).
//   Callers: topThread, deci2_tty_handler, _print_double.  $v0 is left as
//   sce_doprnt's result (0); no caller reads it.
void sce_printf_sio(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    sce_doprnt(fmt, (u64 *)ap);   /* EE: va_list = pointer to the 8-byte slots */
    va_end(ap);
}

// FUNC 0x00287B80 sce_printf_deci2
//   Kernel/TTY printf (the bootstrap's scePrintf): saves the putc hook,
//   installs deci2_sink (DECI2 kPuts line buffer), formats, restores the
//   hook and returns sce_doprnt's result (0).  A trailing partial line stays
//   in the buffer until the next '\n'.  9 callers outside the unit (libcdvd
//   debug output and others).
int sce_printf_deci2(const char *fmt, ...)
{
    va_list ap;
    void (*saved_hook)(int c);
    int ret;

    saved_hook = _putc_hook;
    _putc_hook = deci2_sink;
    va_start(ap, fmt);
    ret = sce_doprnt(fmt, (u64 *)ap);
    va_end(ap);
    _putc_hook = saved_hook;
    return ret;
}
