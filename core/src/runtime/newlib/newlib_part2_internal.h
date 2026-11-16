/* u138: GT3 main executable 0x00281088..0x00283D50, the middle of the no-FDE
 * stretch G007: Cygnus newlib (the ~1.8 era copy linked by SCE's ee-gcc
 * 2.9-ee-991111):
 *   - stdio      findfp.c (_cleanup, __sinit), fread.c, fvwrite.c, fwalk.c,
 *                makebuf.c, refill.c, stdio.c (__sread/__swrite/__sseek/__sclose)
 *   - freer      freer.o: mallocr.c's DEFINE_FREE object (_free_r,
 *                _malloc_trim_r)
 *   - mallocr    mallocr.o: DEFINE_MALLOC (the bins and tunables in .data,
 *                malloc_extend_top, _malloc_r), mlock.c, reent/sbrkr.c; the
 *                dlmalloc macros both objects compile are mallocr.h
 *                (dlmalloc 2.6.x, REENT build)
 *   - locale     locale.c (.data: __mb_cur_max and two "C" arrays), mbtowc_r.c
 *   - memchr     the PS2 newlib MMI version (16 bytes at a time with lq)
 *   - strtod.c   (David Gay's strtod; mprec.c's Bigint routines are in u139)
 *   - strtoul.c
 *
 * Types: with ee-gcc `long` is 64 bits (int and pointers 32).  The machine code
 * shows it everywhere (ld/sd, dsll32/dsrl32 zero extension, __udivdi3,
 * __fixdfdi).  The sources below say s64 / u64 wherever newlib says `long` /
 * `unsigned long`, and u32 / s32 for size_t / int.  `double` is soft-float
 * (the fp-bit dp* routines of u135); writing the arithmetic as C double
 * operations is the same calls.
 *
 * Names: the functions carry the newlib names (the code is newlib's, line for
 * line).  The shared newlib types, data and the functions other units call
 * are in runtime/newlib/G007_stdio.h (the canonical header of G007); this header keeps
 * what only u138 uses.
 */
#ifndef U138_H
#define U138_H

#include "runtime/newlib/G007_stdio.h"    /* own TU: FILE, _reent, __suio, _Bigint, malloc_chunk,
                                   _impure_ptr, __malloc_av_, _ctype_, sys_errno, ... */

#ifdef __cplusplus
extern "C" {
#endif

/* newlib constants (stdio.h, errno.h, unistd.h) */
#define EOF       (-1)
#define BUFSIZ    1024
#define ERANGE    34
#define SEEK_END  2

/* newlib's CHECK_INIT (local.h), as newlib_part1_internal.h has it: attach the stream to
 * _impure_ptr, run __sinit once.  The do { } while (0) is retail's: its loop
 * note aligns the __sdidinit test's label (the nop at 0x002828D4). */
#define CHECK_INIT(fp) \
    do { \
        if ((fp)->_data == NULL) \
            (fp)->_data = _impure_ptr; \
        if (!(fp)->_data->__sdidinit) \
            __sinit((fp)->_data); \
    } while (0)

/* struct stat: runtime/newlib/G007_stdio.h. */
#define S_IFMT   0xF000     /* type of file */
#define S_IFCHR  0x2000     /* character special */
#define S_IFREG  0x8000     /* regular */

/* ======================================================================== */
/* mallocr.c                                                                 */
/* ======================================================================== */

struct mallinfo {
    int arena;      /* 0x00 total space allocated from system (sbrked_mem) */
    int ordblks;    /* 0x04 */
    int smblks;     /* 0x08 */
    int hblks;      /* 0x0C */
    int hblkhd;     /* 0x10 */
    int usmblks;    /* 0x14 */
    int fsmblks;    /* 0x18 */
    int uordblks;   /* 0x1C */
    int fordblks;   /* 0x20 */
    int keepcost;   /* 0x24 */
};
EGCS_SIZE(struct mallinfo, 0x28);   /* current_mallinfo ends at _libkernl_version (0x00307A00) */

/* newlib's locale.h (only locale.c uses it); CHAR_MAX as limits.h has it */
#ifndef CHAR_MAX
#define CHAR_MAX 127
#endif
struct lconv {
    char *decimal_point;        /* 0x00 */
    char *thousands_sep;        /* 0x04 */
    char *grouping;             /* 0x08 */
    char *int_curr_symbol;      /* 0x0C */
    char *currency_symbol;      /* 0x10 */
    char *mon_decimal_point;    /* 0x14 */
    char *mon_thousands_sep;    /* 0x18 */
    char *mon_grouping;         /* 0x1C */
    char *positive_sign;        /* 0x20 */
    char *negative_sign;        /* 0x24 */
    char int_frac_digits;       /* 0x28 */
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};
EGCS_SIZE(struct lconv, 0x30);

/* ======================================================================== */
/* Data only u138 uses                                                      */
/* ======================================================================== */

/* mallocr.c statics, under newlib's INTERNAL_NEWLIB link names; they follow
 * __malloc_av_ (0x003075A8 + 258 * 4 = 0x003079B0).  Defined (with
 * __malloc_av_) in mallocr_part1.c, mallocr.o's .data; freer.o
 * (freer.c) uses them through these externs. */
extern u64             __malloc_trim_threshold;    /* 0x003079B0 trim_threshold (0x20000) */
extern u64             __malloc_top_pad;           /* 0x003079B8 top_pad (0) */
extern char           *__malloc_sbrk_base;         /* 0x003079C0 sbrk_base ((char *)-1) */
extern u64             __malloc_max_sbrked_mem;    /* 0x003079C8 max_sbrked_mem */
extern u64             __malloc_max_total_mem;     /* 0x003079D0 max_total_mem */
extern struct mallinfo __malloc_current_mallinfo;  /* 0x003079D8 current_mallinfo */
/* mprec.c's tables (mprec.h: bigtens / tinytens), right after __mprec_tens:
 * mprec.o's .rodata, defined in u139's mprec.c */
extern const double    __mprec_bigtens[5];         /* 0x00349098 1e16 .. 1e256 */
extern const double    __mprec_tinytens[5];        /* 0x003490C0 1e-16 .. 1e-256 */
/* strtod's HUGE_VAL: newlib math.h's `__infinity[0].d', libm's s_infconst.o
 * (+inf, 8 bytes right after __mprec_tinytens; defined by u139's data-only
 * infconst.c).  Not strtod.o's data: its .rodata ends at
 * 0x00348FC0, where mprec.o's p05 follows.  The union as math.h has it (an
 * array "to avoid gcc's problem with initialized union constants"). */
union __dmath {
    u32    i[2];
    double d;
};
extern const union __dmath __infinity[];   /* 0x003490E8 +inf (was g_003490E8) */

/* ======================================================================== */
/* This unit: functions no other unit calls                                 */
/* ======================================================================== */

/* stdio_part2.c */
void _cleanup(void);                                                    /* 0x00281088 */

/* freer.c */
int _malloc_trim_r(struct _reent *reent_ptr, size_t pad);               /* 0x00281568 */

/* mallocr_part1.c */
void *_sbrk_r(struct _reent *ptr, int incr);                            /* 0x00282A20 */

/* locale.c */
char *_setlocale_r(struct _reent *p, int category, const char *locale); /* 0x00281B48 */
struct lconv *_localeconv_r(struct _reent *data);                       /* 0x00281BD0 */
char *setlocale(int category, const char *locale);                      /* 0x00281BE0 */
struct lconv *localeconv(void);                                         /* 0x00281C10 */

/* strtod.c */
double _strtod_r(struct _reent *ptr, const char *s00, char **se);       /* 0x00282BF0 */
float strtodf(const char *s00, char **se);                              /* 0x00283B08 */

#ifdef __cplusplus
}
#endif

#endif /* U138_H */
