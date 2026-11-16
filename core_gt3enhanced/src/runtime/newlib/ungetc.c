/* ungetc.c - newlib ungetc.o: __submore (0x00283D88) and ungetc
 * (0x00283E78).  Standard newlib code of the late 1990s; the only caller of
 * ungetc is __svfscanf (0x00280088, u137).
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

/* fp has an ungetc buffer in use. */
#define HASUB(fp) ((fp)->_ub._base != NULL)

/* newlib's CHECK_INIT (local.h), as newlib_part1_internal.h has it: attach the stream to
 * _impure_ptr, run __sinit once.  The do { } while (0) is retail's: its loop
 * note aligns the __sdidinit test's label (the nop at 0x00283EB4). */
#define CHECK_INIT(fp) \
    do { \
        if ((fp)->_data == NULL) \
            (fp)->_data = _impure_ptr; \
        if (!(fp)->_data->__sdidinit) \
            __sinit((fp)->_data); \
    } while (0)

// FUNC 0x00283D88 __submore
//   Grow the ungetc buffer.  First time (still the 3-byte reserve _ubuf):
//   _malloc_r a BUFSIZ (1024) buffer and move the 3 reserve bytes to its
//   end.  Later: _realloc_r to twice the size and move the old contents to
//   the upper half.  0 on success, EOF if the allocation fails.
static int __submore(FILE *fp)
{
    int i;
    unsigned char *p;

    if (fp->_ub._base == fp->_ubuf) {
        /* Get a new buffer (rather than expanding the old one). */
        if ((p = (unsigned char *)_malloc_r(fp->_data, (size_t)BUFSIZ)) == NULL)
            return EOF;
        fp->_ub._base = p;
        fp->_ub._size = BUFSIZ;
        p += BUFSIZ - sizeof(fp->_ubuf);
        for (i = sizeof(fp->_ubuf); --i >= 0;)
            p[i] = fp->_ubuf[i];
        fp->_p = p;
        return 0;
    }
    i = fp->_ub._size;
    p = (unsigned char *)_realloc_r(fp->_data, (void *)fp->_ub._base, i << 1);
    if (p == NULL)
        return EOF;
    (void)memcpy((void *)(p + i), (void *)p, (size_t)i);
    fp->_p = p + i;
    fp->_ub._base = p;
    fp->_ub._size = i << 1;
    return 0;
}

// FUNC 0x00283E78 ungetc
//   Push c back onto fp.  Initialises stdio for fp's _reent (fp->_data,
//   defaulting to _impure_ptr) if needed, clears __SEOF, switches a
//   read/write stream from writing to reading (fflush), then either extends
//   the current ungetc buffer, backs _p up over an identical byte, or starts
//   the 3-byte reserve buffer.  Returns (unsigned char)c, or EOF.
int ungetc(int c, FILE *fp)
{
    if (c == EOF)
        return EOF;

    CHECK_INIT(fp);                     /* implicit: __sinit */

    /* After ungetc, we won't be at eof anymore */
    fp->_flags &= ~__SEOF;

    if ((fp->_flags & __SRD) == 0) {
        /* Not already reading: no good unless reading-and-writing.
         * Otherwise, flush any current write stuff. */
        if ((fp->_flags & __SRW) == 0)
            return EOF;
        if (fp->_flags & __SWR) {
            if (fflush(fp))
                return EOF;
            fp->_flags &= ~__SWR;
            fp->_w = 0;
            fp->_lbfsize = 0;
        }
        fp->_flags |= __SRD;
    }
    c = (unsigned char)c;

    /* If we are in the middle of ungetc'ing, just continue.  This may
     * require expanding the current ungetc buffer. */
    if (HASUB(fp)) {
        if (fp->_r >= fp->_ub._size && __submore(fp))
            return EOF;
        *--fp->_p = c;
        fp->_r++;
        return c;
    }

    /* If we can handle this by simply backing up, do so, but never replace
     * the original character (sscanf on const data). */
    if (fp->_bf._base != NULL && fp->_p > fp->_bf._base && fp->_p[-1] == c) {
        fp->_p--;
        fp->_r++;
        return c;
    }

    /* Create an ungetc buffer; initially the `reserve' buffer. */
    fp->_ur = fp->_r;
    fp->_up = fp->_p;
    fp->_ub._base = fp->_ubuf;
    fp->_ub._size = sizeof(fp->_ubuf);
    fp->_ubuf[sizeof(fp->_ubuf) - 1] = c;
    fp->_p = &fp->_ubuf[sizeof(fp->_ubuf) - 1];
    fp->_r = 1;
    return c;
}
