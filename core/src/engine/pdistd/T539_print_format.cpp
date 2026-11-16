/* T539_print_format.cpp - pdistd::PrintFormatTargetBase and pdistd::PrintFormat,
 * the printf engine of Polyphony's pdistd.
 *
 * Same machine code as the bootstrap's src/pdistd/printformat.cpp
 * (0x01010318..0x01011210); the source below is that file's, with the core
 * addresses.  Class: include/pdistd/printf.h (core addresses in
 * include/engine/pdistd/T539_print_format.h).
 *
 * Core data: s_pow10_u64 @0x003474C0 and s_digits @0x00347560 (defined
 * below), jump tables 0x00347580 (flags ' '..'0') and 0x003475D0
 * (conversions 'E'..'x'); s_pow10_double @0x003460C0 (T530a_printf_tables.cpp;
 * all three declared in pdistd/printf.h); "(null)" @0x003531B8.
 * vtable _vt$Q26pdistd21PrintFormatTargetBase @0x003476A0:
 *   [0] +0xC   ~PrintFormatTargetBase  0x0026B1E0
 *   [1] +0x14  putc                    0x0026B210
 * RTTI name "Q26pdistd21PrintFormatTargetBase" @0x003476B8, __user_type_info
 * @0x003534F8.
 *
 * The format dialect: flags - + 0 space #, width digits | *, precision
 * .digits | .*, length h, l, L / ll (each argument is one 8-byte slot),
 * conversions c s d i u o x X p e E f F g G plus Polyphony's %t (decimal with
 * an implied point) and %F (the slot holds a raw IEEE single).  Floats are
 * formatted by long division against a table of doubles and TRUNCATED.
 * Quirks Q1..Q9: see the bootstrap file's header (all reproduced here).
 */
#include "engine/pdistd/pdistd_internal.h"
#include "engine/pdistd/T539_print_format.h"

/* "(null)", printed for a NULL %s argument, is a string literal (.sdata
 * 0x003531B8, addressed with lui/addiu). */

/* ========================================================================== */
/* The integer tables (the double table is T530a_printf_tables.cpp's)         */
/* ========================================================================== */

/* They open this file's .rodata (0x003474C0..0x00347580, before the jump
 * tables), and the integer table is 64-byte aligned: in every build it sits
 * on a 64-byte boundary after the zeros that follow the double table (7, 1
 * and 2 words in the US core, the EU core and the bootstraps), which the
 * double table's own object cannot give (its start is not 64-aligned in the
 * EU core and the bootstraps).  Lupo 2 links this object before the double
 * table's and shows the same .rodata: the integer table (0x0034F2C0), the
 * digits, the two jump tables, the type name, then the doubles (0x0034F4C8). */

/* 0x003474C0 .rodata: exact integer powers 10^0 .. 10^18 (decimal / %t), then
 * a zero word (0x00347558) that PrintFormat reads as [19] (quirk Q6). */
extern const u64 s_pow10_u64[20] __attribute__((aligned(64))) = {      /* 0x003474C0 */
    1ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    0,
};

/* 0x00347560 .rodata: lower-case digits, immediately followed (+0x10) by the
 * upper-case set used for %X.  32 bytes, no terminators in the original. */
extern const char s_digits[32] = {             /* 0x00347560 */
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/* ========================================================================== */
/* PrintFormat constants                                                      */
/* ========================================================================== */

/* flags ($t5) */
enum {
    FL_LEFT  = 0x01,   /* '-' */
    FL_PLUS  = 0x02,   /* '+' (also set by a negative '*' width - quirk Q1) */
    FL_ZERO  = 0x04,   /* '0' */
    FL_SPACE = 0x08,   /* ' '  ((flags << 2) & 0x20 == ' ' in the original) */
    FL_ALT   = 0x10,   /* '#' */
    FL_G     = 0x40    /* set by %g/%G: suppress trailing zeros in e-style */
};

/* length modifier ($t2) */
enum {
    LEN_INT      = 0,  /* none: lw of the slot (32-bit)          */
    LEN_SHORT    = 1,  /* 'h' : lh of the slot                   */
    LEN_LONG     = 2,  /* 'l' : ld of the slot (EE long = 64-bit) */
    LEN_LONGLONG = 3   /* 'L' or 'll': ld of the slot            */
};

/* integer radix selector ($a1) */
enum {
    BASE_OCT = 0,
    BASE_DEC = 1,
    BASE_HEX = 2
};

#define DBL_MANT_MASK  0x000FFFFFFFFFFFFFULL   /* (u64)-1 >> 12 */
#define DBL_HIDDEN_BIT 0x0010000000000000ULL   /* 0x8000 << 37  */

/* Emit one character through the target's virtual putc (vtable slot 2,
 * lh delta @vt+0x10 / lw pfn @vt+0x14); -1 aborts the whole call with -1.
 * A plain `if`, not do { } while (0): egcs emits loop notes for a do-while,
 * and final aligns the next label after each one to 8 bytes, which the
 * original does not have.  Only ever used as a statement of its own. */
#define PUTC_OR_FAIL(ch)                        \
    if (target->putc(ch) == -1)                 \
        return -1

/* Decimal number parser for width / precision.  INLINED at both call sites in
 * the binary (0x0026B348 and 0x0026B3FC); taking the cursor's address is why
 * PrintFormat keeps its format cursor in memory at sp+0x280.  (egcs has no
 * always_inline and inlines it at -O2 anyway.) */
#ifdef BOOT_OLD_GCC
#define PRINTF_ALWAYS_INLINE inline
#else
#define PRINTF_ALWAYS_INLINE inline __attribute__((always_inline))
#endif
static PRINTF_ALWAYS_INLINE int parse_decimal(const char **pp)
{
    const char *s = *pp;
    int n = 0;
    int d;                  /* int: andi 0xFF + signed slti in the binary */

    for (d = (*s - '0') & 0xFF; d < 10; d = (*s - '0') & 0xFF) {
        n = n * 10 + d;
        s++;
    }
    *pp = s;
    return n;
}


namespace pdistd {

// FUNC 0x0026B1C8 pdistd::PrintFormatTargetBase::PrintFormatTargetBase
// Constructor: only installs the vptr (_vt$Q26pdistd21PrintFormatTargetBase
// @0x003476A0) and returns this.  Called from PrintTarget's constructor.
PrintFormatTargetBase::PrintFormatTargetBase()
{
}

// FUNC 0x0026B1E0 pdistd::PrintFormatTargetBase::~PrintFormatTargetBase
// Destructor (vtable slot 0, +0xC).  GCC 2.9x hidden `int flags`:
// re-installs the base vptr, then `if (flags & 1) operator delete(this)`
// (0x0027C0C0).  Reached from ~PrintTarget, which passes its flags through.
PrintFormatTargetBase::~PrintFormatTargetBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0026B210 pdistd::PrintFormatTargetBase::putc
// Default character sink (vtable slot 1, +0x14): discards the character
// and returns 0 (success).  Overridden by PrintTarget::putc.
int PrintFormatTargetBase::putc(int c)
{
    return 0;
}

// FUNC 0x0026B218 pdistd::PrintFormat (print_format_engine)
// The %-format interpreter (3688 bytes, 0x360-byte frame).  Walks `format`,
// sends literal characters straight to target->putc, and for each conversion
// builds the text in `buf` (integers right-aligned at the end of buf, floats
// left-aligned at its start) and then emits
//   [left spaces][sign][prefix]["0" padding][text][(float) zeros][exponent][right spaces]
// Returns the number of characters counted, or -1 as soon as putc returns -1.
// Jump tables: flags 0x00347580 (' '..'0'), conversions 0x003475D0 ('E'..'x').
// Stack: buf = sp+0x000 (634 bytes), cursor sp+0x280, prefix word sp+0x284,
// prefix length sp+0x288, right-pad count sp+0x28C, 128-bit spills of
// $t2/$t7/$t9 at sp+0x290..0x2BF around the putc calls.  Callers: vprintf,
// vsprintf (in the bootstrap only vprintf).
// The original reuses a handful of scratch variables across the conversions
// (the register allocation shows it): c is also the hex/octal digit mask and
// the float digit count, i every count/index/shift/pad value, len the float
// alignment shift, trailing-zero count and exponent, str the float text
// pointer, fixedPoint the integer field total, uval the double's bits.
int PrintFormat(PrintFormatTargetBase *target, const char *format, va_list args)
{
    char buf[634];              /* sp+0x000 .. sp+0x279                       */
    int written;                /* $fp */
    int c;                      /* $t1: the char (lb, sign-extended); scratch later */
    int next;                   /* $a0: the character after c */

    written = 0;
    for (;;) {
        /* the following character is read before the NUL test (one byte
         * past the terminator at the end, as in the original) */
        c = format[0];
        next = format[1];
        format++;
        if (c == '\0')
            break;
        const char *resume;     /* $t3: where to continue after a bad conversion */
        int flags, width, prec, lenmod;
        int sign;               /* $t9: -1 = "signed conversion" marker, then 0/' '/'+'/'-' */
        s32 prefix;             /* sp+0x284: byte0 = count, bytes 1.. = chars   */
        int prefixLen;          /* sp+0x288 */
        int fixedPoint;         /* $a2: %t flag; later the field total */
        int base;               /* $a1 */
        int expChar;            /* $t7: 'e'/'E', 0 = f-style, -1 = %g undecided */
        int len;                /* $s0: characters of text at str; float scratch */
        const char *str;        /* $s2: the text to print */
        int signLen, zeroPad, leftPad, rightPad;  /* $a1, $s5, $s6, sp+0x28C  */
        int i;                  /* $s1: counts, indices, shift, pad width */
        s64 sval;               /* $a3 */
        u64 uval;               /* $t0 */

        if (c != '%') {
            PUTC_OR_FAIL(c);
            written++;
            continue;
        }
        if (next == '%') {
            PUTC_OR_FAIL('%');
            written++;
            format++;
            continue;
        }

        /* ---- flags (jump table 0x00347580, index c - ' ', 17 entries) ---- */
        resume = format;
        flags  = 0;
        width  = 0;
        prec   = -1;
        lenmod = LEN_INT;
        for (;; format++) {
            c = *format;
            switch (c) {
            case '-': flags |= FL_LEFT;  continue;
            case '+': flags |= FL_PLUS;  continue;
            case '0': flags |= FL_ZERO;  continue;
            case ' ': flags |= FL_SPACE; continue;
            case '#': flags |= FL_ALT;   continue;
            }
            break;
        }

        /* ---- width ---- */
        if ((unsigned)(c - '0') < 10) {
            width = parse_decimal(&format);
            c = *format;
        } else if (c == '*') {
            width = va_arg(args, int);          /* lw of the slot */
            if (width < 0) {
                width = -width;
                flags |= FL_PLUS;               /* sic (Q1): not FL_LEFT */
            }
            c = *++format;
        }

        /* ---- precision ---- */
        if (c == '.') {
            c = *++format;
            if ((unsigned)(c - '0') < 10) {
                prec = parse_decimal(&format);
            } else if (c == '*') {
                format++;
                prec = va_arg(args, int);
            }
            c = *format;
            if (prec < 0)                       /* Q2: "." / negative '*' -> 0 */
                prec = 0;
        }

        /* ---- length modifier ---- */
        if (c == 'L') {
            lenmod = LEN_LONGLONG;
            c = *++format;
        } else if (c == 'h') {
            lenmod = LEN_SHORT;
            c = *++format;
        } else if (c == 'l') {
            c = *++format;
            lenmod = LEN_LONG;
            if (c == 'l') {
                lenmod = LEN_LONGLONG;
                c = *++format;
            }
        }
        format++;                                    /* past the conversion character */

        /* ---- conversion (jump table 0x003475D0, index c - 'E', 52 entries) ---- */
        sign       = 0;
        prefix     = 0;
        fixedPoint = 0;
        switch (c) {
        case 'c':                               /* 0x0026B520 - Q3: no padding */
            PUTC_OR_FAIL((s8)va_arg(args, int));    /* lb of the slot */
            written++;
            continue;

        case 's': {                             /* 0x0026B550 */
            const char *e;

            str = va_arg(args, const char *);
            if (str == NULL)
                str = "(null)";
            for (e = str; *e != '\0'; e++)      /* inlined strlen */
                ;
            len = (int)(e - str);
            if (prec >= 0 && prec < len)
                len = prec;
            leftPad   = 0;
            signLen   = 0;
            prefixLen = 0;
            zeroPad   = 0;
            rightPad  = 0;
            goto output;
        }

        case 'o':                               /* 0x0026B5BC */
            prefix = 0x3001;                    /* 1 char: '0' */
            if (!(flags & FL_ALT))
                prefix = 0;
            base = BASE_OCT;
            goto integer;

        case 'p':                               /* 0x0026B5D8: %format == %#x of an int */
            c = 'x';
            lenmod = LEN_INT;
            flags |= FL_ALT;
            /* fall through */
        case 'X':
        case 'x':                               /* 0x0026B5E4 */
            if (flags & FL_ALT)
                prefix = (c << 16) | 0x3002;    /* 2 chars: '0', c */
            base = BASE_HEX;
            goto integer;

        case 't':                               /* 0x0026B600: Polyphony fixed point */
            fixedPoint = 1;
            /* fall through */
        case 'd':
        case 'i':                               /* 0x0026B604 */
            sign = -1;
            /* fall through */
        case 'u':                               /* 0x0026B608 */
            base = BASE_DEC;
        integer: {
            const char *digits;                 /* $a0 (octal / hex) */

            /* 0x0026B60C: fetch one 8-byte slot */
            if (lenmod == LEN_INT) {
                /* the slot is read twice: lw (signed) and lwu (unsigned),
                 * both after the one va_arg increment */
                sval = va_arg(args, s32);
                uval = ((u32 *)args)[-2];
            } else if ((unsigned)(lenmod - LEN_LONG) < 2) {
                uval = va_arg(args, u64);       /* ld */
                sval = (s64)uval;
            } else if (lenmod == LEN_SHORT) {
                sval = (s16)va_arg(args, int);  /* lh */
                uval = sval & 0xFFFF;
            } else {                            /* unreachable (lenmod is 0..3) */
                sval = 0;
                uval = 0;
            }

            if (sign < 0) {                     /* %d %i %t */
                sign = (flags & FL_SPACE) ? ' ' : 0;
                if (flags & FL_PLUS)
                    sign = '+';
                if (sval < 0) {
                    uval = 0 - (u64)sval;       /* dsubu */
                    sign = '-';
                }
            }

            len = 0;
            switch (base) {
            case BASE_OCT:                      /* 0x0026B6D4 */
                digits = s_digits;
                c = 7;                          /* digit mask */
                i = 3;                          /* bits per digit */
                goto radix;
            case BASE_HEX:                      /* 0x0026B6E8 */
                digits = (c == 'X') ? s_digits + 16 : s_digits;
                c = 15;
                i = 4;
            radix: {                            /* 0x0026B704 */
                char *end = &buf[sizeof(buf) - 1];
                char *q = end;

                do {
                    *q-- = digits[(int)uval & c];
                    uval >>= i;
                } while (uval != 0);
                len = (int)(end - q);
                break;
            }
            case BASE_DEC: {                    /* 0x0026B744 */
                char *end = &buf[sizeof(buf)];  /* $a1 */
                char *dot;                      /* $s2 */
                char *q;                        /* $a3 */

                /* digit count: smallest i in 1..18 with uval < 10^i, else
                 * 19 (Q5) */
                for (i = 1; i < 19 && uval >= s_pow10_u64[i]; i++)
                    ;
                len = i;
                dot = NULL;
                if (fixedPoint) {
                    /* %t: at least `prec` digits, '.' in front of the last
                     * `prec` of them; the precision is used up here.  With
                     * prec <= 0 the '.' slot is never reached (Q6). */
                    len = (i < prec) ? prec : i;
                    dot = end - prec - 1;
                    len += 1;
                    prec = -1;
                }
                q = end - len;
                for (i = len - fixedPoint - 1; i >= 0; i--) {
                    u64 pw = s_pow10_u64[i];    /* i = 19.. for %t with prec >= 20 (Q6) */
                    int d = 0;

                    while (uval >= pw) {
                        uval -= pw;
                        d++;
                    }
                    if (q == dot)
                        *q++ = '.';
                    *q++ = (char)('0' + d);
                }
                break;
            }
            }
            prefixLen = prefix & 0xFF;

            /* 0x0026B870 */
            str      = &buf[sizeof(buf)] - len;
            zeroPad  = (len < prec) ? prec - len : 0;
            leftPad  = 0;
            rightPad = 0;
            if (sign != 0)
                signLen = 1;
            else
                signLen = 0;
        }
        output: {
            /* ---- 0x0026B898: field output for %s and the integer conversions ---- */
            fixedPoint = signLen + prefixLen + zeroPad + len;   /* field total */

            if (fixedPoint < width) {
                i = width - fixedPoint;

                if (flags & FL_ZERO)
                    zeroPad += i;
                else if (flags & FL_LEFT)
                    rightPad = i;
                else
                    leftPad = i;
            }
            for (; leftPad > 0; leftPad--) {
                PUTC_OR_FAIL(' ');
                written++;
            }
            if (sign != 0) {
                PUTC_OR_FAIL(sign);
                written++;
            }
            /* prefix chars are passed unmasked (e.g. 0x7830 for "0x"'s '0');
             * PrintTarget::putc stores only the low byte */
            for (i = 1; i <= prefixLen; i++) {
                PUTC_OR_FAIL(prefix >> (8 * i));
                written++;
            }
            for (; zeroPad > 0; zeroPad--) {
                PUTC_OR_FAIL('0');
                written++;
            }
            for (; len > 0; len--) {
                PUTC_OR_FAIL((s8)*str++);
                written++;
            }
            for (; rightPad > 0; rightPad--) {
                PUTC_OR_FAIL(' ');
                written++;
            }
            continue;
        }

        case 'E':
        case 'e':                               /* 0x0026BA38 */
            expChar = c;
            goto float_default_prec;
        case 'F':
        case 'f':                               /* 0x0026BA40 */
            expChar = 0;
        float_default_prec:
            if (prec < 0)
                prec = 6;
            goto floating;
        case 'G':
        case 'g':                               /* 0x0026BA58 - Q7 */
            if (prec <= 0)
                prec = 1;
            flags |= FL_G;
            expChar = -1;
        floating: {
            int exp10;                          /* $t2: floor(log10(|x|))      */
            int nsig;                           /* $a2 */
            int pointPos;                       /* $t3: index of the '.'       */
            int e2;                             /* $t0: exponent of the mantissa */
            int sigLen;                         /* $s3: chars of buf to print  */
            int expLen;                         /* $a0 */
            int k;                              /* $a2: s_pow10_double index */
            s64 mant;                           /* $a3: signed bits, biased exponent, mantissa */
            int total;                          /* $a2 */

            /* 0x0026BA6C: fetch.  uval = the double's bit pattern, mant the
             * same bits signed (later the biased exponent, then the
             * mantissa being divided down) */
            if (c != 'F') {
                uval = va_arg(args, u64);       /* ld */
                mant = uval;
            } else {
                /* raw IEEE single in the slot's low word -> double bits by hand:
                 * shift into place, rebias the exponent (+0x380) keeping the
                 * sign; |bits| with a zero exponent and top-2 mantissa bits
                 * clear (mask 0x7FE00000) becomes +0.0 */
                s32 fbits = va_arg(args, s32);
                mant = (s64)(((u64)(s64)fbits << 29) +
                             ((fbits < 0) ? 0xC800000000000000ULL : 0x3800000000000000ULL));
                if ((fbits & 0x7FE00000) == 0)
                    mant = 0;
                uval = mant;
            }

            /* 0x0026BABC: sign */
            sign = (flags & FL_SPACE) ? ' ' : 0;
            if (flags & FL_PLUS)
                sign = '+';
            if (mant < 0) {
                uval = ((u64)mant << 1) >> 1;
                sign = '-';
            }

            /* decimal exponent: binary search of the bit pattern in the table */
            exp10 = 0;
            if (uval != 0) {
                int lo = 0, hi = 633, mid = 316;    /* 316 = 10^-8 */
                u64 p = *(s_pow10_double + 316);

                if (uval != p) {
                    for (;;) {
                        if (uval < p)
                            hi = mid;
                        else
                            lo = mid;
                        mid = (lo + hi) >> 1;
                        p = *(s_pow10_double + mid);
                        if (uval == p)
                            break;
                        if (lo == mid)
                            break;
                    }
                }
                exp10 = mid - 324;
            }
            nsig = prec + 1;
            mant = uval >> 52;                  /* biased binary exponent */

            /* 0x0026BB64: %g picks e- or f-style ('g'-2 == 'e', 'G'-2 == 'E') */
            if (expChar < 0) {
                expChar = c - 2;
                if (!(exp10 < -4) && exp10 < prec)
                    expChar = 0;
            }
            c = nsig;
            pointPos = 1;
            if (expChar == 0) {
                if (exp10 < 0)
                    exp10 = 0;
                pointPos = exp10 + 1;
                c += exp10;
            }

            /* 0x0026BBA0: split into mantissa (with hidden bit) and exponent.
             * From here mant is the mantissa, c the number of digits still
             * to generate and k the s_pow10_double index. */
            e2 = (int)(mant & 0x7FF);
            mant = uval & DBL_MANT_MASK;
            if (e2 != 0)
                mant |= DBL_HIDDEN_BIT;
            k = exp10 + 324;
            if (k < c)
                c = k;

            /* 0x0026BBE8: digit generation by repeated subtraction of the
             * exponent-aligned mantissa of 10^k; truncating, no rounding.
             * i = characters in buf, len = the alignment shift, d first the
             * binary exponent of 10^k, then the digit */
            i = 0;
            sigLen = 0;
            while (c > 0) {
                u64 pw = s_pow10_double[k];
                int d;                          /* $a0 */
                int ch;                         /* $a1 */
                u64 pm;

                d = (int)((pw >> 52) & 0x7FF);
                pm = pw & DBL_MANT_MASK;
                if (d != 0)
                    pm |= DBL_HIDDEN_BIT;
                len = e2 - d;
                d = 0;
                if (len >= 0) {
                    mant <<= len;               /* dsllv */
                    e2 -= len;
                    while ((u64)mant >= pm) {
                        mant -= pm;
                        d++;
                    }
                    k--;
                    c--;
                    ch = '0' + d;
                    if (mant != 0) {
                        while ((u64)mant <= DBL_MANT_MASK) {    /* renormalise */
                            mant <<= 1;
                            e2--;
                        }
                    }
                } else {
                    k--;
                    c--;
                    ch = '0';
                }
                if (i == pointPos) {
                    sigLen = i;
                    buf[i++] = '.';
                }
                buf[i++] = ch;
                if (d != 0)
                    sigLen = i;                 /* end of the last nonzero digit */
            }

            /* 0x0026BCF4: how much of buf to print (sigLen) and how many
             * zeros to append (len) */
            leftPad = 0;
            if (sign != 0)
                signLen = 1;
            else
                signLen = 0;
            zeroPad = 0;
            if (expChar == 0) {
                if (prec != 0 || (flags & FL_ALT)) {
                    if (sigLen == pointPos)
                        sigLen++;               /* keep the '.' */
                }
                expLen = 0;
                c = prec + 2 + exp10;           /* Q9: +2 even without '.' */
                len = c - sigLen;
            } else {
                len = 0;
                if (!(flags & FL_G)) {
                    c = prec + 2;               /* Q9: '.' lost if sigLen == 1 */
                    len = c - sigLen;
                } else if (flags & FL_ALT) {
                    if (sigLen == 1)
                        sigLen++;
                }
                expLen = (exp10 <= -100 || exp10 >= 100) ? 5 : 4;  /* "e+ddd" / "e+dd" */
            }

            rightPad = 0;
            total = signLen + sigLen + len + expLen;
            if (total < width) {
                i = width - total;

                if (flags & FL_ZERO)
                    zeroPad = i;
                else if (flags & FL_LEFT)
                    rightPad = i;
                else
                    leftPad = i;
            }

            for (; leftPad > 0; leftPad--) {
                PUTC_OR_FAIL(' ');
                written++;
            }
            if (sign != 0) {
                PUTC_OR_FAIL(sign);
                written++;
            }
            for (; zeroPad > 0; zeroPad--) {
                PUTC_OR_FAIL('0');
                written++;
            }
            for (str = buf; sigLen > 0; sigLen--) {
                PUTC_OR_FAIL((s8)*str++);
                written++;
            }
            for (; len > 0; len--) {
                PUTC_OR_FAIL('0');
                written++;
            }
            if (expChar != 0) {
                /* 0x0026BF10 - Q8: no exponent sign is emitted, yet 4 is added */
                int hundreds;

                PUTC_OR_FAIL(expChar);
                len = __builtin_abs(exp10);     /* abs(): bltzl/negu */
                hundreds = len / 100;
                len = len % 100;
                if (hundreds != 0) {
                    PUTC_OR_FAIL('0' + hundreds);
                    written++;
                }
                PUTC_OR_FAIL('0' + len / 10);
                PUTC_OR_FAIL('0' + len % 10);
                written += 4;
            }
            for (; rightPad > 0; rightPad--) {
                PUTC_OR_FAIL(' ');
                written++;
            }
            continue;
        }

        default:                                /* 0x0026C030: unknown conversion */
            format = resume;                         /* drop the '%', print the rest literally */
            continue;
        }
    }
    return written;
}

} /* namespace pdistd */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0026C080 __tfQ26pdistd21PrintFormatTargetBase
// Callers: the two PrintTargets' typeinfo functions (0x0026C6C8, 0x0026C868).
extern "C" void *__tfQ26pdistd21PrintFormatTargetBase(void)
{
    if (__tiQ26pdistd21PrintFormatTargetBase[0] == 0)
        __rtti_user(__tiQ26pdistd21PrintFormatTargetBase, "Q26pdistd21PrintFormatTargetBase");
    return __tiQ26pdistd21PrintFormatTargetBase;
}
#endif
