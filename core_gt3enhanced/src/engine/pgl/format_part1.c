/* format_part1.c - unit u114, 0x0024D4F8..0x0024D8C0 (G001, no FDE: C).
 *
 * The start of the graphics library's printf (its own object: the font
 * code before it saves $s1 as a qword once the kerning has used it, these
 * functions save it as a doubleword in retail): the field writers of %s
 * and of the numbers, used by the format engine fmt_vprintf (u115,
 * format_part2.c continues this file) directly and through
 * fmt_put_decimal / fmt_put_octal / fmt_put_hex.  Output goes through a put
 * function (font_putc, font_measure_char, sprintf_putc) that returns < 0 on
 * failure.  Flags: 1 '-' (left-justify), 2 '+', 4 '0' (zero pad), 0x10 '#'.
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_font_internal.h"

/* fmt_put_string / fmt_put_number flags */
enum {
    FMT_LEFT  = 0x01,           /* '-' */
    FMT_PLUS  = 0x02,           /* '+' */
    FMT_ZERO  = 0x04,           /* '0' */
    FMT_ALT   = 0x10            /* '#' */
};

// FUNC 0x0024D4F8 fmt_put_string
// %s: at most prec bytes of s (all if prec < 0) padded to width with ' '
// (or '0' with flag 4) on the left, or with ' ' on the right with flag 1.
// A byte with bit 7 set and the next one are put as one two-byte code
// (both count).  Returns the characters put, -1 if put failed.  Caller:
// fmt_vprintf.
int fmt_put_string(PglPutcFunc put, u32 flags, s32 width, s32 prec, const u8 *s)
{
    s32 padRight = 0;
    s32 padLeft = 0;
    s32 count = 0;
    s32 len, i;
    int c;

    len = (s32)strlen((const char *)s);
    if (prec >= 0)
        len = (prec < len) ? prec : len;
    width -= len;
    if (width < 0)
        width = 0;
    if (!(flags & FMT_LEFT))
        padLeft = width;
    else
        padRight = width;
    c = (flags & FMT_ZERO) ? '0' : ' ';
    for (i = 0; i < padLeft; i++) {
        if (put(c) < 0)
            return -1;
        count++;
    }
    for (i = 0; i < len; ) {
        c = *s++;
        if (c & 0x80) {
            c = (c << 8) + *s++;
            count++;
            i++;
        }
        if (put(c) < 0)
            return -1;
        i++;
        count++;
    }
    for (i = 0; i < padRight; i++) {
        if (put(' ') < 0)
            return -1;
        count++;
    }
    return count;
}

// FUNC 0x0024D670 fmt_put_number
// A converted number: digits[0..ndigits) with the sign (base <= 10: '-'
// as given, '+' with flag 2), the '#' prefix ("0" from base 8, "0x" from
// base 16, "0X" for an odd base >= 16), at least prec digits (default 1;
// zero-filled), padded to width: ' ' on the left, ' ' on the right with
// flag 1, '0' after the prefix with flag 4.  Nothing to print (0 digits,
// precision 0) leaves only the width in spaces.  Returns the characters put,
// -1 if put failed.  Callers: fmt_put_decimal, fmt_put_octal, fmt_put_hex.
// Not exact (91%): retail computes minDigits as li 1 / movn prec with the
// test against a -1 register (shared with the width test), here slti /
// movz; tried 16 if / ternary / negated forms of both tests in both orders.
int fmt_put_number(PglPutcFunc put, u32 flags, s32 width, s32 prec, s8 sign,
                   const char *digits, s32 ndigits, s32 base)
{
    char prefix[4];                             /* sp+0x00 */
    s32 minDigits;
    s32 padRight = 0;
    s32 nprefix = 0;
    s32 padLeft = 0;
    s32 zeros, count, i;

    minDigits = 1;
    if (prec >= 0)
        minDigits = prec;
    if (width < 0)
        width = 0;
    if (base < 11) {
        if (sign != '-' && (flags & FMT_PLUS))
            sign = '+';
        if (sign != 0) {
            prefix[0] = sign;
            nprefix = 1;
        }
    }
    if (flags & FMT_ALT) {
        if (base >= 8)
            prefix[nprefix++] = '0';
        if (base >= 16)
            prefix[nprefix++] = (base & 1) ? 'X' : 'x';
    }
    zeros = minDigits - ndigits;
    if (zeros < 0)
        zeros = 0;
    if ((ndigits | zeros) == 0) {
        padLeft = width;
        nprefix = 0;
    } else {
        i = width - (nprefix + ndigits) - zeros;
        if (i < 0) {
            zeros += i;
            if (!(zeros > 0))
                zeros = (ndigits == 0);
        } else if (!(flags & FMT_LEFT)) {
            if (flags & FMT_ZERO)
                zeros += i;
            else
                padLeft = i;
        } else {
            padRight = i;
        }
    }
    count = 0;
    for (i = 0; i < padLeft; i++, count++)
        if (put(' ') < 0)
            return -1;
    for (i = 0; i < nprefix; i++, count++)
        if (put(prefix[i]) < 0)
            return -1;
    for (i = 0; i < zeros; i++, count++)
        if (put('0') < 0)
            return -1;
    for (i = 0; i < ndigits; i++, count++)
        if (put(digits[i]) < 0)
            return -1;
    for (i = 0; i < padRight; i++, count++)
        if (put(' ') < 0)
            return -1;
    return count;
}
