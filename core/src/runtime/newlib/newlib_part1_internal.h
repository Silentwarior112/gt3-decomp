/* newlib_part1_internal.h - Sony EE libc / newlib 1.8-era C library, no-FDE stretch G007, part
 * 0x0027D7A0..0x00281088 of the GT3 US main executable.
 *
 * The members are newlib objects in archive (alphabetical) order, compiled with
 * the EE's 64-bit `long' (written s64/u64 here: GCC 14's n32 `long' is 32 bits):
 *
 *   stdlib.c   abort assert atexit atoi (ctype_) errno (impure) | qsort rand |
 *                        strtol | atof  (ctype_.o / impure.o: data only)
 *   string.c   memcmp memcpy memmove memset | strcasecmp strcat strchr strcmp
 *                        strcpy strncmp strncpy strstr | strupr
 *   stdio_part1.c    fprintf | wsetup | fflush | findfp (std .. _cleanup_r)
 *   sscanf.c   sscanf.o (eofread, sscanf)
 *   vfprintf.c vfprintf.o (__sprint, __sbprintf, vfprintf, _vfprintf_r),
 *                        integer-only (no FLOATING_POINT), _NO_LONGLONG
 *   vfscanf.c  vfscanf.o (__svfscanf, __sccl), with FLOATING_POINT (atof)
 *
 * Eight string/memory routines and abort are the same machine code as the
 * bootstrap's runtime/libc.c and are copied from it (boot.txt).
 *
 * The newlib structures (FILE, struct _reent, __suio, _glue, _atexit), the
 * __S* flags, the functions of the rest of G007 (__sinit .. ungetc) and
 * _impure_ptr come from the translation unit's canonical header.  What stays
 * here is local to the unit: stdio constants, the ctype macros, the unit's own
 * data and the functions no other unit calls.
 */
#ifndef U137_H
#define U137_H

#include "types.h"
#include "libc.h"               /* strlen (core 0x00269A68), malloc (core 0x0026C3A8),
                                   _exit (core 0x001000B8), break_stub_1 (core 0x0026E3C0),
                                   ENOMEM; <stdarg.h>'s va_list */
#include "runtime/newlib/G007_stdio.h"    /* own TU: FILE, struct _reent, __sinit .. ungetc, _impure_ptr */

#ifndef BOOT_OLD_GCC
/* strtol returns the EE's 64-bit long; GCC 14 (n32) would expect a 32-bit long. */
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

/* ======================================================================== */
/* newlib constants                                                         */
/* ======================================================================== */

#define EOF      (-1)
#define BUFSIZ   1024
#ifndef ERANGE
#define ERANGE   34             /* strtol overflow */
#endif
#define SIGABRT  6              /* abort's "raise" argument */
#define RAND_MAX 0x7FFFFFFF

/* newlib ctype bits: (_ctype_ + 1)[c].  The index is the int value of c: a
 * (signed) char >= 0x80 indexes before the table, as in the original (lb). */
#define _U      0x01            /* upper                                     */
#define _L      0x02            /* lower                                     */
#define _N      0x04            /* digit                                     */
#define _S      0x08            /* space                                     */
#define _P      0x10            /* punctuation                               */
#define _C      0x20            /* control                                   */
#define _X      0x40            /* hex digit                                 */
#define _B      0x80            /* blank                                     */
#define CTYPE(c)   ((_ctype_ + 1)[(int)(c)])
#define ISSPACE(c) (CTYPE(c) & _S)
#define ISDIGIT(c) (CTYPE(c) & _N)
#define ISUPPER(c) (CTYPE(c) & _U)
#define ISLOWER(c) (CTYPE(c) & _L)
#define ISALPHA(c) (CTYPE(c) & (_U | _L))
/* newlib ctype.h's GNU toupper: a statement expression, not a call. */
#define TOUPPER(c) __extension__ ({ int __x = (c); ISLOWER(__x) ? (__x - 'a' + 'A') : __x; })

/* ======================================================================== */
/* Globals (data owned elsewhere)                                           */
/* ======================================================================== */

/* (_ctype_, 0x00348730, the newlib ctype table, and _impure_ptr / impure_data:
 * declared in runtime/newlib/G007_stdio.h, defined by stdlib.c (ctype_.o and
 * impure.o, data-only objects).)
 * vfprintf.c's `blanks' / `zeroes' (0x00348840 / 0x00348850) and vfscanf.c's
 * `basefix' (0x00348A30) are the functions' own static const arrays.
 * __mb_cur_max opens locale.o's .data (0x00307590..0x003075A8 with two 8-byte
 * "C" buffers): u138's locale.c's object, not vfprintf.o's. */
extern int         __mb_cur_max;        /* 0x00307590 locale.c's MB_CUR_MAX (= 1) */

/* The 64-bit helpers __muldi3 0x00278F48 (runtime/libgcc/G005_dp_bit.h), __udivdi3
 * 0x0028DAA8 and __umoddi3 0x0028E078 (runtime/newlib/G007_stdio.h) and dp-bit's
 * double -> float dptofp 0x00279C50 (runtime/libgcc/G005_dp_bit.h) are the compiler's
 * code for the 64-bit / double C arithmetic written in this unit. */

/* ======================================================================== */
/* This unit's functions that no other unit calls                           */
/* ======================================================================== */

int    fprintf(FILE *fp, const char *fmt, ...);                          /* 0x0027D8C8 */
void   srand(unsigned int seed);                                         /* 0x0027E610 */
s64    _strtol_r(struct _reent *rptr, const char *nptr, char **endptr, int base); /* 0x0027F078 */
s64    strtol(const char *nptr, char **endptr, int base);                /* 0x0027F2B0 */
int    __sprint(FILE *fp, struct __suio *uio);                           /* 0x0027F340 (static in vfprintf.c) */
int    __sbprintf(FILE *fp, const char *fmt, va_list ap);                /* 0x0027F388 (static in vfprintf.c) */
int    vfprintf(FILE *fp, const char *fmt, va_list ap);                  /* 0x0027F440 */
int    _vfprintf_r(struct _reent *data, FILE *fp, const char *fmt0, va_list ap); /* 0x0027F4B8 */
int    __svfscanf(FILE *fp, const char *fmt0, va_list ap);               /* 0x00280088 */
const s8 *__sccl(char *tab, const s8 *fmt);                              /* 0x00280B90 (static in vfscanf.c) */
double atof(const char *s);                                              /* 0x00280DA8 */
struct _glue *__sfmoreglue(struct _reent *d, int n);                     /* 0x00280F28 (static in findfp.c) */
FILE  *__sfp(struct _reent *d);                                          /* 0x00280F98 */

/* newlib's CHECK_INIT (local.h): attach the stream to _impure_ptr, run __sinit once. */
#define CHECK_INIT(fp) \
    do { \
        if ((fp)->_data == NULL) \
            (fp)->_data = _impure_ptr; \
        if (!(fp)->_data->__sdidinit) \
            __sinit((fp)->_data); \
    } while (0)

#endif /* U137_H */
