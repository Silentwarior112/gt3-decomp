/* u138 / G007: newlib stdlib/strtoul.c (the BSD one).  `unsigned long` is
 * 64-bit here: cutoff / cutlim come from __udivdi3 / __umoddi3 of
 * ULONG_MAX, acc * base is __muldi3.  The strtoul wrapper (0x00283D50) is in
 * u139.  The ctype tests read _ctype_ + 1 (0x00348731) indexed by the
 * (signed) char, as newlib's macros do. */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

/* newlib ctype.h */
#define _U  0x01    /* upper */
#define _L  0x02    /* lower */
#define _N  0x04    /* digit */
#define _S  0x08    /* space */
#define isspace(c)  ((_ctype_ + 1)[(int)(c)] & _S)
#define isdigit(c)  ((_ctype_ + 1)[(int)(c)] & _N)
#define isalpha(c)  ((_ctype_ + 1)[(int)(c)] & (_U | _L))
#define isupper(c)  ((_ctype_ + 1)[(int)(c)] & _U)
#define ULONG_MAX   0xFFFFFFFFFFFFFFFFULL   /* 64-bit unsigned long */

// FUNC 0x00283B30 _strtoul_r
u64 _strtoul_r(struct _reent *rptr, const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    u64 acc;        /* unsigned long */
    int c;
    u64 cutoff;     /* unsigned long */
    int neg = 0, any, cutlim;

    /* See strtol for comments as to the logic used. */
    do {
        c = *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    cutoff = (u64)ULONG_MAX / (u64)(s64)base;               /* implicit: __udivdi3 */
    cutlim = (int)((u64)ULONG_MAX % (u64)(s64)base);        /* implicit: __umoddi3 */
    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= (u64)(s64)base;  /* implicit: __muldi3 */
            acc += c;
        }
    }
    if (any < 0) {
        acc = ULONG_MAX;
        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *)(any ? s - 1 : nptr);
    return acc;
}
