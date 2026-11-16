/* vfprintf.c - newlib vfprintf.o (BSD 4.4 vfprintf) in the no-FDE
 * stretch G007 in u137:
 *
 *   0x0027F340  __sprint      (static) flush the I/O vectors through __sfvwrite
 *   0x0027F388  __sbprintf    (static) unbuffered stream: format into a 1 KiB
 *                             stack buffer through a fake FILE, then fflush
 *   0x0027F440  vfprintf      CHECK_INIT, then _vfprintf_r(fp->_data, ...)
 *   0x0027F4B8  _vfprintf_r
 *
 * Built without FLOATING_POINT (no %e/%f/%g: they print as the plain character)
 * and with _NO_LONGLONG: 'q' and "ll" set QUADINT, which nothing reads, so
 * %lld / %qd fetch an int.  LONGINT (%ld, %D, %O, %U) fetches the EE's 64-bit
 * long; the conversions run on a 64-bit u_long (__udivdi3 / __umoddi3 for %d
 * and %u).  The format is scanned with _mbtowc_r(_impure_ptr, .., MB_CUR_MAX
 * = __mb_cur_max).
 * The game's own printf/sprintf (pdistd, 0x0026C690..) do not come here: the
 * only user is fprintf (0x0027D8C8) from __assert.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part1_internal.h"

/* flags */
#define ALT       0x001         /* alternate form                            */
#define HEXPREFIX 0x002         /* add 0x or 0X prefix                       */
#define LADJUST   0x004         /* left adjustment                           */
#define LONGDBL   0x008         /* long double; unimplemented                */
#define LONGINT   0x010         /* long integer                              */
#define QUADINT   0x020         /* quad integer (ignored: _NO_LONGLONG)      */
#define SHORTINT  0x040         /* short integer                             */
#define ZEROPAD   0x080         /* zero (as opposed to blank) pad            */

#define BUF       40            /* buf[] for %c and the digits               */
#define NIOV      8
#define PADSIZE   16            /* pad chunk size                            */

#define to_digit(c)  ((c) - '0')
#define is_digit(c)  ((unsigned)to_digit(c) <= 9)
#define to_char(n)   ((n) + '0')

/* Fetch an argument (8-byte va slots; `long' is 64-bit). */
#define SARG() \
    (flags & LONGINT ? va_arg(ap, s64) : \
     flags & SHORTINT ? (s64)(short)va_arg(ap, int) : \
     (s64)va_arg(ap, int))
#define UARG() \
    (flags & LONGINT ? va_arg(ap, u64) : \
     flags & SHORTINT ? (u64)(unsigned short)va_arg(ap, int) : \
     (u64)va_arg(ap, unsigned int))

// FUNC 0x0027F340 __sprint
/* Flush out all the vectors defined by the given uio, then reset it so that it
 * can be reused. */
int __sprint(FILE *fp, struct __suio *uio)
{
    int err;

    if (uio->uio_resid == 0) {
        uio->uio_iovcnt = 0;
        return 0;
    }
    err = __sfvwrite(fp, uio);
    uio->uio_resid = 0;
    uio->uio_iovcnt = 0;
    return err;
}

// FUNC 0x0027F388 __sbprintf
/* Helper for unbuffered streams: copy the important variables into a fake
 * FILE with a BUFSIZ (0x400) stack buffer, print through vfprintf (the
 * CHECK_INIT front end, not _vfprintf_r), fflush it, and copy back the error
 * flag. */
int __sbprintf(FILE *fp, const char *fmt, va_list ap)
{
    int           ret;
    FILE          fake;
    unsigned char buf[BUFSIZ];

    /* copy the important variables */
    fake._data = fp->_data;
    fake._flags = fp->_flags & ~__SNBF;
    fake._file = fp->_file;
    fake._cookie = fp->_cookie;
    fake._write = fp->_write;

    /* set up the buffer */
    fake._bf._base = fake._p = buf;
    fake._bf._size = fake._w = sizeof (buf);
    fake._lbfsize = 0;          /* not actually used, but Just In Case */

    /* do the work, then copy any error status */
    ret = vfprintf(&fake, fmt, ap);
    if (ret >= 0 && fflush(&fake))
        ret = EOF;
    if (fake._flags & __SERR)
        fp->_flags |= __SERR;
    return ret;
}

// FUNC 0x0027F440 vfprintf
int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
    CHECK_INIT(fp);                     /* implicit: __sinit */
    return _vfprintf_r(fp->_data, fp, fmt, ap);
}

/*
 * Queue an I/O vector (PRINT), pad with a 16-byte pattern (PAD), write out the
 * queued vectors (FLUSH).
 */
#define PRINT(ptr, len) {                       \
        iovp->iov_base = (ptr);                 \
        iovp->iov_len = (len);                  \
        uio.uio_resid += (len);                 \
        iovp++;                                 \
        if (++uio.uio_iovcnt >= NIOV) {         \
            if (__sprint(fp, &uio))             \
                goto error;                     \
            iovp = iov;                         \
        }                                       \
    }
#define PAD(howmany, with) {                    \
        if ((n = (howmany)) > 0) {              \
            while (n > PADSIZE) {               \
                PRINT(with, PADSIZE);           \
                n -= PADSIZE;                   \
            }                                   \
            PRINT(with, n);                     \
        }                                       \
    }
#define FLUSH() {                               \
        if (uio.uio_resid && __sprint(fp, &uio)) \
            goto error;                         \
        uio.uio_iovcnt = 0;                     \
        iovp = iov;                             \
    }

/* cantwrite (local.h): not writable yet and __swsetup fails. */
#define cantwrite(fp) \
    ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && __swsetup(fp))

// FUNC 0x0027F4B8 _vfprintf_r
/* The BSD formatter: literal runs and conversions are queued as I/O vectors
 * (8 at a time, PRINT) and written by __sprint; padding comes from the static
 * 16-byte blanks / zeroes arrays.  Flags ' ', '#', '*', '-', '+', '.', '0',
 * width digits, 'h', 'l' ("ll" -> QUADINT), 'q'; conversions c D d i n O o p s U
 * u X x; anything else prints itself (NUL ends the format).  Returns the
 * number of characters, or EOF when the stream has its error flag. */
int _vfprintf_r(struct _reent *data, FILE *fp, const char *fmt0, va_list ap)
{
    char *fmt;                  /* format string */
    int ch;                     /* character from fmt */
    int n, m;                   /* handy integers (short term usage) */
    char *cp;                   /* handy char pointer (short term usage) */
    struct __siov *iovp;        /* for PRINT macro */
    int flags;                  /* flags as above */
    int ret;                    /* return value accumulator */
    int width;                  /* width from format (%8d), or 0 */
    int prec;                   /* precision from format (%.3d), or -1 */
    char sign;                  /* sign prefix (' ', '+', '-', or \0) */
    int wc;                     /* wchar_t */
    u64 _uquad;                 /* integer arguments %[diouxX] (u_long) */
    enum { OCT, DEC, HEX } base; /* base for [diouxX] conversion */
    int dprec;                  /* a copy of prec if [diouxX], 0 otherwise */
    int realsz;                 /* field size expanded by dprec */
    int size;                   /* size of converted field or string */
    char *xdigs;                /* digits for [xX] conversion */
    struct __suio uio;          /* output information: summary */
    struct __siov iov[NIOV];    /* ... and individual io vectors */
    char buf[BUF];              /* space for %c, %[diouxX] */
    char ox[2];                 /* space for 0x hex-prefix */
    int state = 0;              /* mbtowc calls from library must not change state */

    /*
     * Choose PADSIZE to trade efficiency vs. size.  If larger printf
     * fields occur frequently, increase PADSIZE and make the initialisers
     * below longer.  (.rodata 0x00348840 / 0x00348850: the first data of
     * vfprintf.o; the PAD loops address them directly, `lui; addiu'.)
     */
    static const char blanks[PADSIZE] =
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    static const char zeroes[PADSIZE] =
        {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

    /* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
    if (cantwrite(fp))                  /* implicit: __swsetup */
        return EOF;

    /* optimise fprintf(stderr) (and other unbuffered Unix files) */
    if ((fp->_flags & (__SNBF | __SWR | __SRW)) == (__SNBF | __SWR) &&
        fp->_file >= 0)
        return __sbprintf(fp, fmt0, ap);

    fmt = (char *)fmt0;
    uio.uio_iov = iovp = iov;
    uio.uio_resid = 0;
    uio.uio_iovcnt = 0;
    ret = 0;

    /*
     * Scan the format for conversions (`%' character).
     */
    for (;;) {
        cp = fmt;
        while ((n = _mbtowc_r(_impure_ptr, &wc, fmt, __mb_cur_max, &state)) > 0) {
            fmt += n;
            if (wc == '%') {
                fmt--;
                break;
            }
        }
        if ((m = fmt - cp) != 0) {
            PRINT(cp, m);
            ret += m;
        }
        if (n <= 0)
            goto done;
        fmt++;                  /* skip over '%' */

        flags = 0;
        dprec = 0;
        width = 0;
        prec = -1;
        sign = '\0';

rflag:  ch = *fmt++;
reswitch:
        switch (ch) {
        case ' ':
            /*
             * ``If the space and + flags both appear, the space
             * flag will be ignored.''
             *  -- ANSI X3J11
             */
            if (!sign)
                sign = ' ';
            goto rflag;
        case '#':
            flags |= ALT;
            goto rflag;
        case '*':
            /*
             * ``A negative field width argument is taken as a
             * - flag followed by a positive field width.''
             *  -- ANSI X3J11
             * They don't exclude field widths read from args.
             */
            if ((width = va_arg(ap, int)) >= 0)
                goto rflag;
            width = -width;
            /* FALLTHROUGH */
        case '-':
            flags |= LADJUST;
            goto rflag;
        case '+':
            sign = '+';
            goto rflag;
        case '.':
            if ((ch = *fmt++) == '*') {
                n = va_arg(ap, int);
                prec = n < 0 ? -1 : n;
                goto rflag;
            }
            n = 0;
            while (is_digit(ch)) {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            }
            prec = n < 0 ? -1 : n;
            goto reswitch;
        case '0':
            /*
             * ``Note that 0 is taken as a flag, not as the
             * beginning of a field width.''
             *  -- ANSI X3J11
             */
            flags |= ZEROPAD;
            goto rflag;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = 0;
            do {
                n = 10 * n + to_digit(ch);
                ch = *fmt++;
            } while (is_digit(ch));
            width = n;
            goto reswitch;
        case 'h':
            flags |= SHORTINT;
            goto rflag;
        case 'l':
            if (*fmt == 'l') {
                fmt++;
                flags |= QUADINT;
            } else {
                flags |= LONGINT;
            }
            goto rflag;
        case 'q':
            flags |= QUADINT;
            goto rflag;
        case 'c':
            *(cp = buf) = va_arg(ap, int);
            size = 1;
            sign = '\0';
            break;
        case 'D':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'd':
        case 'i':
            _uquad = SARG();
            if ((s64)_uquad < 0) {
                _uquad = -_uquad;
                sign = '-';
            }
            base = DEC;
            goto number;
        case 'n':
            if (flags & LONGINT)
                *va_arg(ap, s64 *) = ret;
            else if (flags & SHORTINT)
                *va_arg(ap, short *) = ret;
            else
                *va_arg(ap, int *) = ret;
            continue;   /* no output */
        case 'O':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'o':
            _uquad = UARG();
            base = OCT;
            goto nosign;
        case 'p':
            /*
             * ``The argument shall be a pointer to void.  The
             * value of the pointer is converted to a sequence
             * of printable characters, in an implementation-
             * defined manner.''
             *  -- ANSI X3J11
             */
            /* (u_long)(unsigned _POINTER_INT)ptr: lw, i.e. sign-extended to 64 bits */
            _uquad = (u64)(s64)(s32)(u32)va_arg(ap, void *);
            base = HEX;
            xdigs = "0123456789abcdef";
            flags |= HEXPREFIX;
            ch = 'x';
            goto nosign;
        case 's':
            if ((cp = va_arg(ap, char *)) == NULL)
                cp = "(null)";
            if (prec >= 0) {
                /*
                 * can't use strlen; can only look for the
                 * NUL in the first `prec' characters, and
                 * strlen() will go further.
                 */
                char *p = memchr(cp, 0, prec);

                if (p != NULL) {
                    size = p - cp;
                    if (size > prec)
                        size = prec;
                } else
                    size = prec;
            } else
                size = strlen(cp);
            sign = '\0';
            break;
        case 'U':
            flags |= LONGINT;
            /*FALLTHROUGH*/
        case 'u':
            _uquad = UARG();
            base = DEC;
            goto nosign;
        case 'X':
            xdigs = "0123456789ABCDEF";
            goto hex;
        case 'x':
            xdigs = "0123456789abcdef";
hex:        _uquad = UARG();
            base = HEX;
            /* leading 0x/X only if non-zero */
            if (flags & ALT && _uquad != 0)
                flags |= HEXPREFIX;

            /* unsigned conversions */
nosign:     sign = '\0';
            /*
             * ``... diouXx conversions ... if a precision is
             * specified, the 0 flag will be ignored.''
             *  -- ANSI X3J11
             */
number:     if ((dprec = prec) >= 0)
                flags &= ~ZEROPAD;

            /*
             * ``The result of converting a zero value with an
             * explicit precision of zero is no characters.''
             *  -- ANSI X3J11
             */
            cp = buf + BUF;
            if (_uquad != 0 || prec != 0) {
                /*
                 * Unsigned mod is hard, and unsigned mod
                 * by a constant is easier than that by
                 * a variable; hence this switch.
                 */
                switch (base) {
                case OCT:
                    do {
                        *--cp = to_char(_uquad & 7);
                        _uquad >>= 3;
                    } while (_uquad);
                    /* handle octal leading 0 */
                    if (flags & ALT && *cp != '0')
                        *--cp = '0';
                    break;

                case DEC:
                    /* many numbers are 1 digit */
                    while (_uquad >= 10) {
                        *--cp = to_char(_uquad % 10);   /* implicit: __umoddi3 */
                        _uquad /= 10;                   /* implicit: __udivdi3 */
                    }
                    *--cp = to_char(_uquad);
                    break;

                case HEX:
                    do {
                        *--cp = xdigs[_uquad & 15];
                        _uquad >>= 4;
                    } while (_uquad);
                    break;

                default:
                    cp = "bug in vfprintf: bad base";
                    size = strlen(cp);
                    goto skipsize;
                }
            }
            size = buf + BUF - cp;
        skipsize:
            break;
        default:    /* "%?" prints ?, unless ? is NUL */
            if (ch == '\0')
                goto done;
            /* pretend it was %c with argument ch */
            cp = buf;
            *cp = ch;
            size = 1;
            sign = '\0';
            break;
        }

        /*
         * All reasonable formats wind up here.  At this point, `cp'
         * points to a string which (if not flags&LADJUST) should be
         * padded out to `width' places.  If flags&ZEROPAD, it should
         * first be prefixed by any sign or other prefix; otherwise,
         * it should be blank padded before the prefix is emitted.
         * After any left-hand padding and prefixing, emit zeroes
         * required by a decimal [diouxX] precision, then print the
         * string proper; finally, if LADJUST, pad with blanks.
         *
         * Compute actual size, so we know how much to pad.
         * size excludes decimal prec; realsz includes it.
         */
        realsz = dprec > size ? dprec : size;
        if (sign)
            realsz++;
        else if (flags & HEXPREFIX)
            realsz += 2;

        /* right-adjusting blank padding */
        if ((flags & (LADJUST | ZEROPAD)) == 0)
            PAD(width - realsz, blanks);

        /* prefix */
        if (sign) {
            PRINT(&sign, 1);
        } else if (flags & HEXPREFIX) {
            ox[0] = '0';
            ox[1] = ch;
            PRINT(ox, 2);
        }

        /* right-adjusting zero padding */
        if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
            PAD(width - realsz, zeroes);

        /* leading zeroes from decimal precision */
        PAD(dprec - size, zeroes);

        /* the string or number proper */
        PRINT(cp, size);

        /* left-adjusting padding (always blank) */
        if (flags & LADJUST)
            PAD(width - realsz, blanks);

        /* finally, adjust ret */
        ret += width > realsz ? width : realsz;

        FLUSH();    /* implicit: __sprint (FLUSH); copy out the I/O vectors */
    }
done:
    FLUSH();
error:
    return (fp->_flags & __SERR) ? EOF : ret;   /* __sferror(fp) */
    /* NOTREACHED */
}
