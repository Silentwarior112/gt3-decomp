/* u138 / G007: newlib stdio/refill.c (lflush, __srefill).
 *
 * Its own file, as retail's refill.o: __srefill passes lflush's address
 * (a %hi/%lo pair the assembler writes against .text), and retail has
 * sbrkr.o's _sbrk_r (0x00282A20) between refill.o and stdio.o, so lflush and
 * __sseek (makebuf.c's reference) cannot share one .text in C. */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

/* ------------------------------------------------------------------------ */
/* refill.c                                                                 */
/* ------------------------------------------------------------------------ */

/* newlib bug, reproduced: the test reads
 *     if ((fp->_flags & (__SLBF | __SWR)) == __SLBF | __SWR)
 * which parses as ((... == __SLBF) | __SWR), always true, so the compiler
 * folded the function to a plain fflush(fp) - which is all the code does. */
// FUNC 0x00282888 lflush
static int lflush(FILE *fp)
{
    if (((fp->_flags & (__SLBF | __SWR)) == __SLBF) | __SWR)
        return fflush(fp);
    return 0;
}

// FUNC 0x002828A8 __srefill
int __srefill(FILE *fp)
{
    /* make sure stdio is set up */
    CHECK_INIT(fp);                     /* implicit: __sinit */

    fp->_r = 0;     /* largely a convenience for callers */

    /* SysV does not make this test; take it out for compatibility */
    if (fp->_flags & __SEOF)
        return EOF;

    /* if not already reading, have to be reading and writing */
    if ((fp->_flags & __SRD) == 0) {
        if ((fp->_flags & __SRW) == 0)
            return EOF;
        /* switch to reading */
        if (fp->_flags & __SWR) {
            if (fflush(fp))
                return EOF;
            fp->_flags &= ~__SWR;
            fp->_w = 0;
            fp->_lbfsize = 0;
        }
        fp->_flags |= __SRD;
    } else {
        /* We were reading.  If there is an ungetc buffer, we must have been
         * reading from that.  Drop it, restoring the previous buffer (if
         * any).  If there is anything in that buffer, return. */
        if (fp->_ub._base != NULL) {                /* HASUB */
            if (fp->_ub._base != fp->_ubuf)         /* FREEUB */
                _free_r(fp->_data, fp->_ub._base);
            fp->_ub._base = NULL;
            if ((fp->_r = fp->_ur) != 0) {
                fp->_p = fp->_up;
                return 0;
            }
        }
    }

    if (fp->_bf._base == NULL)
        __smakebuf(fp);

    /* Before reading from a line buffered or unbuffered file, flush all
     * line buffered output files, per the ANSI C standard. */
    if (fp->_flags & (__SLBF | __SNBF))
        (void)_fwalk(fp->_data, lflush);
    fp->_p = fp->_bf._base;
    fp->_r = (*fp->_read)(fp->_cookie, (char *)fp->_p, fp->_bf._size);
    fp->_flags &= ~__SMOD;      /* buffer contents are again pristine */
    if (fp->_r <= 0) {
        if (fp->_r == 0)
            fp->_flags |= __SEOF;
        else {
            fp->_r = 0;
            fp->_flags |= __SERR;
        }
        return EOF;
    }
    return 0;
}
