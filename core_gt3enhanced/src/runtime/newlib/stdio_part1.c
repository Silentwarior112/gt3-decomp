/* stdio_part1.c - newlib stdio members of the no-FDE stretch G007 in u137
 * (other than vfprintf.o / vfscanf.o):
 *
 *   fprintf.o  0x0027D8C8  fprintf
 *   (sscanf.o  0x0027E650  eofread, sscanf: sscanf.c)
 *   wsetup.o   0x00280C98  __swsetup
 *   fflush.o   0x00280DC8  fflush
 *   findfp.o   0x00280ED0  std, __sfmoreglue, __sfp, _cleanup_r
 *                          (__sinit 0x00281098 follows, in u138)
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part1_internal.h"

// FUNC 0x0027D8C8 fprintf
/* va_start (a2..a7 spilled next to the stack arguments), vfprintf.  Its one
 * caller is __assert. */
int fprintf(FILE *fp, const char *fmt, ...)
{
    int     ret;
    va_list ap;

    va_start(ap, fmt);
    ret = vfprintf(fp, fmt, ap);
    va_end(ap);
    return ret;
}

/* (sscanf.o, 0x0027E650 eofread / 0x0027E658 sscanf: sscanf.c.) */

/* ------------------------------------------------------------------------ */
/* wsetup.c                                                                 */
/* ------------------------------------------------------------------------ */

#define HASUB(fp)  ((fp)->_ub._base != NULL)
#define FREEUB(fp) { \
        if ((fp)->_ub._base != (fp)->_ubuf) \
            _free_r((fp)->_data, (fp)->_ub._base); \
        (fp)->_ub._base = NULL; \
    }

// FUNC 0x00280C98 __swsetup
/* Various output routines call wsetup to be sure it is safe to write, because
 * either _flags does not include __SWR, or _buf is NULL.  _wsetup returns 0 if
 * OK to write, nonzero otherwise.  Callers: _vfprintf_r (cantwrite) and
 * __sfvwrite. */
int __swsetup(FILE *fp)
{
    /* Make sure stdio is set up. */
    CHECK_INIT(fp);                     /* implicit: __sinit */

    /*
     * If we are not writing, we had better be reading and writing.
     */
    if ((fp->_flags & __SWR) == 0) {
        if ((fp->_flags & __SRW) == 0)
            return EOF;
        if (fp->_flags & __SRD) {
            /* clobber any ungetc data */
            if (HASUB(fp))
                FREEUB(fp);             /* implicit: _free_r unless it is _ubuf */
            fp->_flags &= ~(__SRD | __SEOF);
            fp->_r = 0;
            fp->_p = fp->_bf._base;
        }
        fp->_flags |= __SWR;
    }

    /*
     * Make a buffer if necessary, then set _w.
     */
    if (fp->_bf._base == NULL)
        __smakebuf(fp);
    if (fp->_flags & __SLBF) {
        /*
         * It is line buffered, so make _lbfsize be -_bufsize
         * for the putc() macro.  We will change _lbfsize back
         * to 0 whenever we turn off __SWR.
         */
        fp->_w = 0;
        fp->_lbfsize = -fp->_bf._size;
    } else
        fp->_w = fp->_flags & __SNBF ? 0 : fp->_bf._size;
    return 0;
}

/* ------------------------------------------------------------------------ */
/* fflush.c                                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x00280DC8 fflush
/* Write out the pending bytes of a write stream through fp->_write; NULL
 * flushes every stream of _impure_ptr (_fwalk).  Callers: __sbprintf,
 * __sfvwrite, refill.c's static lflush (0x00282888, u138), __srefill and
 * ungetc. */
int fflush(FILE *fp)
{
    unsigned char *p;
    int n, t;

    if (fp == NULL)
        return _fwalk(_impure_ptr, fflush);

    CHECK_INIT(fp);                     /* implicit: __sinit */

    t = fp->_flags;                     /* lh: sign-extended short */
    if ((t & __SWR) == 0 || (p = fp->_bf._base) == NULL)
        return 0;
    n = fp->_p - p;                     /* write this much */

    /*
     * Set these immediately to avoid problems with longjmp
     * and to allow exchange buffering (via setvbuf) in user
     * write function.
     */
    fp->_p = p;
    fp->_w = t & (__SLBF | __SNBF) ? 0 : fp->_bf._size;

    while (n > 0) {
        t = (*fp->_write)(fp->_cookie, (char *)p, n);
        if (t <= 0) {
            fp->_flags |= __SERR;
            return EOF;
        }
        p += t;
        n -= t;
    }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* findfp.c (first part)                                                     */
/* ------------------------------------------------------------------------ */

#define NIOBS 4                         /* FILEs per extra _glue block */

// FUNC 0x00280ED0 std
/* Static in findfp.c: set up stdin/stdout/stderr for __sinit (u138): the
 * cookie is the FILE itself, the callbacks are stdio.c's __sread / __swrite /
 * __sseek / __sclose.  _bf._size is left as it is. */
void std(FILE *fp, int flags, int file, struct _reent *ptr)
{
    fp->_p = NULL;
    fp->_r = 0;
    fp->_w = 0;
    fp->_flags = flags;
    fp->_file = file;
    fp->_bf._base = NULL;
    fp->_lbfsize = 0;
    fp->_cookie = fp;
    fp->_read = __sread;
    fp->_write = __swrite;
    fp->_seek = __sseek;
    fp->_close = __sclose;
    fp->_data = ptr;
}

// FUNC 0x00280F28 __sfmoreglue
/* Static in findfp.c: one _malloc_r block holding a _glue header and n
 * zeroed FILEs. */
struct _glue *__sfmoreglue(struct _reent *d, int n)
{
    struct _glue *g;
    FILE *p;

    g = (struct _glue *)_malloc_r(d, sizeof (*g) + n * sizeof (FILE));
    if (g == NULL)
        return NULL;
    p = (FILE *)(g + 1);
    g->_next = NULL;
    g->_niobs = n;
    g->_iobs = p;
    memset(p, 0, n * sizeof (FILE));
    return g;
}

// FUNC 0x00280F98 __sfp
/* Find a free FILE (_flags == 0) in d's glue chain, adding blocks of NIOBS (4)
 * as needed; ENOMEM (12) in d->_errno when that fails.  The slot is reserved
 * (_flags = 1, _file = -1) and its pointers / buffers cleared.  No caller
 * found in the binary. */
FILE *__sfp(struct _reent *d)
{
    FILE *fp;
    int n;
    struct _glue *g;

    if (!d->__sdidinit)
        __sinit(d);

    for (g = &d->__sglue;; g = g->_next) {
        for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
            if (fp->_flags == 0)
                goto found;
        if (g->_next == NULL &&
            (g->_next = __sfmoreglue(d, NIOBS)) == NULL)
            break;
    }
    d->_errno = ENOMEM;
    return NULL;

found:
    fp->_flags = 1;             /* reserve this slot; caller sets real flags */
    fp->_file = -1;
    fp->_data = d;
    fp->_p = NULL;              /* no current pointer */
    fp->_w = 0;                 /* nothing to read or write */
    fp->_r = 0;
    fp->_bf._base = NULL;       /* no buffer */
    fp->_bf._size = 0;
    fp->_lbfsize = 0;           /* not line buffered */
    fp->_ub._base = NULL;       /* no ungetc buffer */
    fp->_ub._size = 0;
    fp->_lb._base = NULL;       /* no line buffer */
    fp->_lb._size = 0;
    return fp;
}

// FUNC 0x00281078 _cleanup_r
/* _fwalk(ptr, fflush) (a tail jump).  Reached through _reent.__cleanup. */
void _cleanup_r(struct _reent *ptr)
{
    (void)_fwalk(ptr, fflush);
}
