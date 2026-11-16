/* sscanf.c - newlib sscanf.o in the no-FDE stretch G007 in u137:
 *
 *   0x0027E650  eofread  (static; the index counts it into rand's entry)
 *   0x0027E658  sscanf
 *
 * Its own file because it is its own object: sscanf passes eofread's address
 * (a .text-relative reference), which must imply the same .text base as the
 * other code references of the file; next to fflush.o's (fflush passes
 * itself to _fwalk) in one file the two disagreed.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part1_internal.h"

// FUNC 0x0027E650 eofread
/* The string stream's read callback, `return 0' (always EOF).  (A function
 * of its own in the list since the 2026-10-16 regen; it was counted into
 * rand's entry.) */
static int eofread(void *cookie, char *buf, int len)
{
    return 0;
}

// FUNC 0x0027E658 sscanf
/* A read-only string FILE on the stack (_flags __SRD, _p/_bf._base = str,
 * _r/_bf._size = strlen(str), _read = eofread, no ungetc / line buffers,
 * _data = _impure_ptr), then __svfscanf.  newlib's statement order (_data
 * last).  One caller, gt2k_menu::EventStatusObject::EventStatusObject. */
int sscanf(const char *str, const char *fmt, ...)
{
    int     ret;
    va_list ap;
    FILE    f;

    f._flags = __SRD;
    f._bf._base = f._p = (unsigned char *)str;
    f._bf._size = f._r = strlen(str);       /* MMI strlen, core 0x00269A68 */
    f._read = eofread;
    f._ub._base = NULL;
    f._lb._base = NULL;
    f._data = _impure_ptr;
    va_start(ap, fmt);
    ret = __svfscanf(&f, fmt, ap);
    va_end(ap);
    return ret;
}
