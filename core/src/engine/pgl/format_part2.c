/* format_part2.c - the tail of the printf engine of the graphics
 * library (G001), 0x0024D8C0..0x0024DF28 (it starts in u114 with the field
 * writers fmt_put_string / fmt_put_number).  Output goes through a
 * character callback (font_putc, font_measure_char, sprintf_putc); the
 * arguments are a block of 8-byte slots (the EE va_list layout: an int is
 * the low word of its slot).  Callers: font_vprintf, font_apply_align,
 * fmt_vsprintf (u114).
 *
 * Conversions: %d %u (decimal), %t (decimal with a decimal point: the
 * precision is the number of fractional digits), %o and - as written - %i
 * (octal), %x %X, %c, %s; %e %f %g %E %G consume their slot and print
 * nothing, as do %n and %p.  Flags - + 0 space # (FMT_*), width and
 * precision (digits or *), length h, l (same as none), L (64-bit).  A byte
 * with bit 7 set starts a two-byte (Shift-JIS) character that is passed to
 * the callback as one value.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_vu1_internal.h"

/* The powers of ten fmt_put_decimal counts the digits with, 10^18 down to
 * 10^0.  The file's only .data (0x002F73A8..0x002F7440, right after the
 * font file's GsLibState): not const in the original (a const table would
 * be .rodata). */
u64 g_fmtPow10[19] = {                          /* 0x002F73A8 */
    1000000000000000000ull, 100000000000000000ull, 10000000000000000ull,
    1000000000000000ull, 100000000000000ull, 10000000000000ull,
    1000000000000ull, 100000000000ull, 10000000000ull,
    1000000000ull, 100000000ull, 10000000ull,
    1000000ull, 100000ull, 10000ull,
    1000ull, 100ull, 10ull,
    1ull
};

// FUNC 0x0024D8C0 fmt_put_decimal
/* Digits of v from 10^18 down (no leading zeros).  fixed (%t) with
 * prec >= 0: a '.' follows the digit of 10^prec, and that digit is printed
 * even if it is a leading zero ("0.xx").  Zero prints no digit at all. */
int fmt_put_decimal(PglPutcFunc put, int flags, int width, int prec, u64 v,
                    int sign, int fixed)
{
    char digits[0x20];
    int pointAt;                /* index of the power whose digit the '.' follows; 19 = none */
    int started;
    int len;
    int i;
    int d;
    int c;
    u64 p;

    if (fixed == 0 || prec < 0) {
        pointAt = 19;
    } else {
        pointAt = 18 - prec;
        if (!(-1 < pointAt))
            pointAt = 19;
    }
    started = 0;
    len = 0;
    for (i = 0; i < 19; i++) {
        p = g_fmtPow10[i];
        d = 0;
        while (v >= p) {
            v -= p;
            d++;
        }
        c = d + '0';
        if (started != 0 || d != 0 || i == pointAt) {
            if (i == pointAt) {
                digits[len++] = c;
                c = '.';
            }
            started = 1;
            digits[len++] = c;
        }
    }
    return fmt_put_number(put, flags, width, prec, (s8)sign, digits, len, FMT_BASE_DEC);
}

// FUNC 0x0024D9C0 fmt_put_octal
/* 22 octal digits from bit 63 down, leading zeros dropped.  The sign is
 * kept in buf[0] and read back (lb) for the call. */
int fmt_put_octal(PglPutcFunc put, int flags, int width, int prec, u64 v,
                  int sign)
{
    char buf[0x20];
    int n;
    int started;
    int shift;
    int d;

    n = 1;
    buf[0] = sign;
    started = 0;
    for (shift = 63; shift >= 0; shift -= 3) {
        d = (int)((v >> shift) & 7);
        if (started != 0 || d != 0) {
            buf[n++] = d + '0';
            started = 1;
        }
    }
    return fmt_put_number(put, flags, width, prec, (s8)buf[0], buf + 1, n - 1, FMT_BASE_OCT);
}

// FUNC 0x0024DA38 fmt_put_hex
/* 16 hex digits from bit 60 down, leading zeros dropped; lower case for
 * 'x', upper case otherwise.  Base code 0x10 for 'x', 0x11 for 'X'. */
int fmt_put_hex(PglPutcFunc put, int flags, int width, int prec, u64 v,
                int conv)
{
    char digits[0x20];
    const char *set;
    int n;
    int started;
    int shift;
    int d;

    n = 0;
    if (conv == 'x')
        set = "0123456789abcdef";               /* 0x00344C40 */
    else
        set = "0123456789ABCDEF";               /* 0x00344C58 */
    started = 0;
    for (shift = 60; shift >= 0; shift -= 4) {
        d = (int)((v >> shift) & 0xF);
        if (started != 0 || d != 0) {
            started = 1;
            digits[n++] = set[d];
        }
    }
    return fmt_put_number(put, flags, width, prec, 0, digits, n,
                          (conv == 'X') ? FMT_BASE_HEX_UPPER : FMT_BASE_HEX);
}

// FUNC 0x0024DAE0 fmt_parse_uint
/* Decimal digits at *pp (read as signed chars); *pp is left after them. */
int fmt_parse_uint(const u8 **pp)
{
    const u8 *p = *pp;
    int c = (s8)*p;
    int v = 0;

    while (c >= '0' && c <= '9') {
        p++;
        v = v * 10 + (c - '0');
        c = (s8)*p;
    }
    *pp = p;
    return v;
}

/* The next argument slot (8 bytes) as type t. */
#define FMT_NEXT(args)  ((args) += 8)
#define FMT_ARG(args, t) (*(t *)((args) - 8))

// FUNC 0x0024DB30 fmt_vprintf
/* Returns the number of characters counted, or -1 when the callback fails
 * (non-zero for a plain character, negative for %c, or a negative result
 * of a conversion).  Quirks kept: %c and %s are not counted; a '*'
 * precision is stored into the width; an unknown conversion prints the
 * text after the '%' as plain characters.  (The plain-character path comes
 * first and the conversion after it, reached by goto: the retail block
 * order; the flag loop is a for loop whose cases continue.) */
int fmt_vprintf(PglPutcFunc put, const u8 *fmt, const u8 *args)
{
    const u8 *p;
    const u8 *spec;             /* the text after the '%' */
    u32 c;
    int count;
    int flags;
    int width;
    int prec;
    int size;
    int r;

    count = 0;
    c = fmt[0];
    p = fmt + 1;
    while (c != 0) {
        if (c == '%') {
            spec = p;
            if (*spec != c)
                goto conversion;
            p = spec + 1;                       /* "%%" */
        } else if (c & 0x80) {                  /* two-byte character */
            count++;
            c = (c << 8) + *p++;
        }
        if (put(c) != 0)
            return -1;
        count++;
        goto next;

conversion:
        for (flags = 0; ; p++) {
            c = *p;
            switch (c) {
            case '-': flags |= FMT_LEFT;  continue;
            case '+': flags |= FMT_PLUS;  continue;
            case '0': flags |= FMT_ZERO;  continue;
            case ' ': flags |= FMT_SPACE; continue;
            case '#': flags |= FMT_ALT;   continue;
            }
            break;
        }
        width = -1;
        if (c >= '0' && c <= '9') {
            width = fmt_parse_uint(&p);
            c = *p;
        } else if (c == '*') {
            p++;
            FMT_NEXT(args);
            width = FMT_ARG(args, s32);
            c = *p;
        }
        prec = -1;
        if (c == '.') {
            p++;
            c = *p;
            if (c >= '0' && c <= '9') {
                prec = fmt_parse_uint(&p);
            } else if (c == '*') {
                FMT_NEXT(args);
                width = FMT_ARG(args, s32);     /* (sic) into the width */
                p++;
            }
        }
        switch (*p++) {                         /* length modifier */
        case 'h': size = FMT_SIZE_SHORT;    break;
        case 'l': size = FMT_SIZE_LONG;     break;
        case 'L': size = FMT_SIZE_LONGLONG; break;
        default:  size = FMT_SIZE_INT; p--; break;
        }
        c = *p++;
        switch (c) {
        case 'X': case 'd': case 'i': case 'o': case 't': case 'u': case 'x': {
            s64 sv;                 /* the argument, sign-extended */
            u64 v;                  /* the argument as an unsigned magnitude */
            int sign;

            switch (size) {
            default:
                FMT_NEXT(args);
                sv = FMT_ARG(args, s32);
                v = FMT_ARG(args, u32);
                break;
            case FMT_SIZE_SHORT:
                FMT_NEXT(args);
                sv = FMT_ARG(args, s16);
                v = sv & 0xFFFF;
                break;
            case FMT_SIZE_LONGLONG:
                FMT_NEXT(args);
                v = FMT_ARG(args, u64);
                sv = (s64)v;
                break;
            }
            sign = 0;
            if (sv < 0 && (c == 'd' || c == 'i' || c == 't')) {
                v = (u64)(0 - sv);
                sign = '-';
            }
            switch (c) {
            case 'd':
            case 'u':
                r = fmt_put_decimal(put, flags, width, prec, v, sign, 0);
                break;
            case 't':
                r = fmt_put_decimal(put, flags, width, prec, v, sign, 1);
                break;
            case 'i':                           /* (sic) octal */
            case 'o':
                r = fmt_put_octal(put, flags, width, prec, v, sign);
                break;
            case 'X':
            case 'x':
                r = fmt_put_hex(put, flags, width, prec, v, c);
                break;
            default:
                r = 0;
                break;
            }
            if (r < 0)
                return -1;
            count += r;
            break;
        }
        case 'E': case 'G': case 'e': case 'f': case 'g':
            /* No float output: the slot is skipped (float / double /
             * long double, all 8 bytes here; the binary keeps the dead
             * size < FMT_SIZE_LONGLONG test). */
            if (size < FMT_SIZE_LONG)
                FMT_NEXT(args);
            else if (size < FMT_SIZE_LONGLONG)
                FMT_NEXT(args);
            else
                FMT_NEXT(args);
            break;
        case 'c':
            FMT_NEXT(args);
            r = put(FMT_ARG(args, s8));
            if (r < 0)
                return -1;
            break;
        case 's':
            /* (the binary still holds c in $a5 here: not an argument) */
            FMT_NEXT(args);
            r = fmt_put_string(put, flags, width, prec, FMT_ARG(args, const u8 *));
            if (r < 0)
                return -1;
            break;
        case 'n': case 'p':
            FMT_NEXT(args);
            break;
        default:
            p = spec;
            break;
        }
next:
        c = *p;
        p++;
    }
    return count;
}
