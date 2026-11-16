/* stdlib.c - newlib stdlib members of the no-FDE stretch G007 in u137:
 *
 *   abort.o   0x0027D7A0  abort          (same code as the bootstrap's)
 *   assert.o  0x0027D7C0  __assert
 *   atexit.o  0x0027D7F8  atexit
 *   atoi.o    0x0027D890  atoi
 *   ctype_.o  (data only)  _ctype_        (.rodata 0x00348730)
 *   errno.o   0x0027D8B8  __errno
 *   impure.o  (data only)  impure_data, _impure_ptr (.data 0x003072A0 / 0x0030758C,
 *                          "C" .rodata 0x00348838)
 *   qsort.o   0x0027DBE8  qsort          (BSD qsort with med3 / swapcode)
 *   rand.o    0x0027E610  srand, rand
 *   strtol.o  0x0027F078  _strtol_r, strtol
 *   atof.o    0x00280DA8  atof
 *
 * `long' is 64 bits in the EE compiler: qsort swaps doublewords (ld/sd),
 * strtol works in 64 bits (__muldi3 / __udivdi3 / __umoddi3).
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part1_internal.h"

/* Doubleword view of byte buffers (may_alias for GCC 14). */
typedef s64 s64_a BOOT_MAY_ALIAS;

// FUNC 0x0027D7A0 abort
/* newlib abort: for (;;) { raise(SIGABRT); _exit(1); }.  Here "raise" is the
 * 16-byte `break; return 0` hook at 0x0026E3C0 (break_stub_1) and _exit is crt0's
 * 0x001000B8, which ignores its argument and jumps to Exit(0).  Callers: the EH /
 * frame.c runtime (__default_terminate, get_reg_addr, end_fde_sort,
 * execute_cfa_insn).  Exact: the index's size (0x1C) leaves out the final
 * b's delay-slot nop at 0x0027D7BC, so the scorer shows 8/7. */
void abort(void)
{
    for (;;) {
        break_stub_1(SIGABRT);
        _exit(1);
    }
}

// FUNC 0x0027D7C0 __assert
/* newlib assert.c: print to stderr, then abort.  11 callers: the assert()s
 * of libscf (scf_bin_to_bcd, scf_bcd_to_bin, .. scf_clock_prev_hour). */
void __assert(const char *file, int line, const char *failedexpr)
{
    fprintf(_impure_ptr->_stderr, "assertion \"%s\" failed: file \"%s\", line %d\n",
            failedexpr, file, line);
    abort();
}

// FUNC 0x0027D7F8 atexit
/* newlib atexit.c: append fn to _impure_ptr's atexit chain (the static
 * _atexit0 block first, then 0x88-byte blocks from malloc).  Its one caller is
 * RaceBase::virtual_25 (0x0017C508). */
int atexit(void (*fn)(void))
{
    struct _atexit *p;

    if ((p = _impure_ptr->_atexit) == NULL)
        _impure_ptr->_atexit = p = &_impure_ptr->_atexit0;
    if (p->_ind >= _ATEXIT_SIZE) {
        if ((p = (struct _atexit *)malloc(sizeof *p)) == NULL)
            return -1;
        p->_ind = 0;
        p->_next = _impure_ptr->_atexit;
        _impure_ptr->_atexit = p;
    }
    p->_fns[p->_ind++] = fn;
    return 0;
}

// FUNC 0x0027D890 atoi
int atoi(const char *s)
{
    return (int)strtol(s, NULL, 10);    /* 64-bit long truncated (dsll32/dsra32) */
}

/* ------------------------------------------------------------------------ */
/* ctype_.c (data only: ctype_.o, linked between atoi.o and errno.o)         */
/* ------------------------------------------------------------------------ */

/* The newlib ctype table, indexed as (_ctype_ + 1)[c] (newlib_part1_internal.h's CTYPE).
 * .rodata 0x00348730, right after assert.o's string (8-aligned: 0x0034872B
 * rounds up to 0x00348730); characters 128..255 are 0.  Readers: strtol,
 * strcasecmp / strupr, vfscanf, u138's strtoul, u005's T025. */
const char _ctype_[1 + 256] = {     /* 0x00348730 */
    0,
    _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
    _C,     _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C,     _C,
    _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
    _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
    _S|_B,  _P,     _P,     _P,     _P,     _P,     _P,     _P,
    _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P,
    _N,     _N,     _N,     _N,     _N,     _N,     _N,     _N,
    _N,     _N,     _P,     _P,     _P,     _P,     _P,     _P,
    _P,     _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U,
    _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
    _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
    _U,     _U,     _U,     _P,     _P,     _P,     _P,     _P,
    _P,     _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L,
    _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
    _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
    _L,     _L,     _L,     _P,     _P,     _P,     _P,     _C
};

// FUNC 0x0027D8B8 __errno
/* &_impure_ptr->_errno (_errno is the first member).  Callers: the syscall
 * glue open / sbrk / link / unlink and libm's sqrt. */
int *__errno(void)
{
    return &_impure_ptr->_errno;
}

/* ------------------------------------------------------------------------ */
/* impure.c (data only: impure.o, linked between fprintf.o and memcmp.o)     */
/* ------------------------------------------------------------------------ */

/* The default reent, newlib's _REENT_INIT (impure_data): stdin / stdout /
 * stderr point at its own __sf[3] (all zero until __sinit), the locale is
 * "C" (.rodata 0x00348838, after _ctype_), _new._reent._rand_next is 1;
 * every other byte is 0.  .data 0x003072A0..0x00307590, _impure_ptr last.
 * (The bootstrap's runtime/libc.c defines the same pair next to __errno.) */
// DATA_START 0x003072A0
static struct _reent impure_data = {    /* 0x003072A0 */
    0,                                  /* _errno */
    &impure_data.__sf[0],               /* _stdin */
    &impure_data.__sf[1],               /* _stdout */
    &impure_data.__sf[2],               /* _stderr */
    0,                                  /* _inc */
    "",                                 /* _emergency */
    0,                                  /* _current_category */
    "C",                                /* _current_locale */
    0,                                  /* __sdidinit */
    NULL,                               /* __cleanup */
    NULL,                               /* _result */
    0,                                  /* _result_k */
    NULL,                               /* _p5s */
    NULL,                               /* _freelist */
    0,                                  /* _cvtlen */
    NULL,                               /* _cvtbuf */
    1,                                  /* _new._reent._rand_next */
};
struct _reent *_impure_ptr = &impure_data;  /* 0x0030758C */

/* ------------------------------------------------------------------------ */
/* qsort.c (BSD 4.4 via newlib)                                              */
/* ------------------------------------------------------------------------ */

#define qmin(a, b) ((a) < (b) ? (a) : (b))

/* `long i = n / sizeof (TYPE)': n converted to the 32-bit size_t, then zero-
 * extended into the 64-bit long counter (srl / dsll32 / dsrl32). */
#define swapcode(TYPE, parmi, parmj, n) {           \
        s64 i = (u32)(n) / sizeof (TYPE);           \
        TYPE *pi = (TYPE *)(parmi);                 \
        TYPE *pj = (TYPE *)(parmj);                 \
        do {                                        \
            TYPE t = *pi;                           \
            *pi++ = *pj;                            \
            *pj++ = t;                              \
        } while (--i > 0);                          \
    }

/* swaptype 0: one long; 1: longs; 2: bytes (a or es not 8-aligned). */
#define SWAPINIT(a, es) \
    swaptype = ((u32)(char *)(a) % sizeof (s64) || (es) % sizeof (s64)) ? 2 : \
               (es) == sizeof (s64) ? 0 : 1;

static inline void swapfunc(char *a, char *b, int n, int swaptype)
{
    if (swaptype <= 1)
        swapcode(s64_a, a, b, n)
    else
        swapcode(char, a, b, n)
}

#define swap(a, b)                                  \
    if (swaptype == 0) {                            \
        s64 t = *(s64_a *)(a);                      \
        *(s64_a *)(a) = *(s64_a *)(b);              \
        *(s64_a *)(b) = t;                          \
    } else                                          \
        swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline char *med3(char *a, char *b, char *c, int (*cmp)(const void *, const void *))
{
    return cmp(a, b) < 0 ?
           (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
          : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

// FUNC 0x0027DBE8 qsort
/* Bentley-McIlroy quicksort: insertion sort below 7 elements, median of 3
 * (ninther above 40), 3-way partitioning, recursion on the left part and
 * iteration on the right one.  All size/pointer compares are unsigned, the
 * min() of the vecswaps signed.
 * Matching: exact with the reference compiler 991111-01; with 991111b (99.6%)
 * two spill slots are swapped (sp+0x38 / 0x3C: GCSE's temporaries for n - 1
 * and a + es, numbered by hash order, which depends on the pseudo base). */
void qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *))
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int   d, r, swaptype, swap_cnt;

loop:
    SWAPINIT(a, es);
    swap_cnt = 0;
    if (n < 7) {
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = (char *)a + (n / 2) * es;
    if (n > 7) {
        pl = a;
        pn = (char *)a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = med3(pl, pl + d, pl + 2 * d, cmp);
            pm = med3(pm - d, pm, pm + d, cmp);
            pn = med3(pn - 2 * d, pn - d, pn, cmp);
        }
        pm = med3(pl, pm, pn, cmp);
    }
    swap((char *)a, pm);
    pa = pb = (char *)a + es;

    pc = pd = (char *)a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = cmp(pb, a)) <= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = cmp(pc, a)) >= 0) {
            if (r == 0) {
                swap_cnt = 1;
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc)
            break;
        swap(pb, pc);
        swap_cnt = 1;
        pb += es;
        pc -= es;
    }
    if (swap_cnt == 0) {    /* Switch to insertion sort */
        for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
            for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }

    pn = (char *)a + n * es;
    r = qmin(pa - (char *)a, pb - pa);
    vecswap((char *)a, pb - r, r);
    /* The listing compares the two in 32 bits and signed (slt; es is not
     * widened): with an unsigned size_t es this C would compare unsigned,
     * hence the (int).  Both values are >= 0 here, so the two agree. */
    r = qmin(pd - pc, pn - pd - (int)es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > es)
        qsort(a, r / es, es, cmp);
    if ((r = pd - pc) > es) {
        /* Iterate rather than recurse to save stack space */
        a = pn - r;
        n = r / es;
        goto loop;
    }
}

/* ------------------------------------------------------------------------ */
/* rand.c                                                                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x0027E610 srand
void srand(unsigned int seed)
{
    _impure_ptr->_rand_next = seed;
}

// FUNC 0x0027E620 rand
/* LCG x = x * 1103515245 + 12345, result & RAND_MAX.  Callers:
 * TirePacket::TirePacket, ads_play, music_play.  NOTE: the last 8 bytes of this range
 * (0x0027E650, `return 0') are sscanf.o's static eofread, in sscanf.c.
 * Exact against 0x0027E620..0x0027E650; the scorer compares the whole index entry
 * (14 words), hence 12/14 (ref/matching/u137/notes.md). */
int rand(void)
{
    struct _reent *reent = _impure_ptr;

    reent->_rand_next = reent->_rand_next * 1103515245 + 12345;
    return reent->_rand_next & RAND_MAX;
}

/* ------------------------------------------------------------------------ */
/* strtol.c (64-bit long)                                                    */
/* ------------------------------------------------------------------------ */

#define S64_MIN ((s64)0x8000000000000000ULL)
#define S64_MAX ((s64)0x7FFFFFFFFFFFFFFFULL)

// FUNC 0x0027F078 _strtol_r
/* BSD strtol: skip spaces, optional sign, 0x prefix for base 0/16, base 0 ->
 * 8 or 10; accumulate in an unsigned 64-bit long with the cutoff / cutlim
 * overflow test (__umoddi3 / __udivdi3 / __muldi3); overflow gives LONG_MIN /
 * LONG_MAX and rptr->_errno = ERANGE (34). */
s64 _strtol_r(struct _reent *rptr, const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    u64 acc;
    int c;
    u64 cutoff;
    int neg = 0, any, cutlim;

    do {
        c = *s++;                       /* lb: signed char */
    } while (ISSPACE(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else if (c == '+')
        c = *s++;
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    cutoff = neg ? (u64)S64_MIN : (u64)S64_MAX;
    cutlim = (int)(cutoff % (u64)(s64)base);  /* implicit: __umoddi3 */
    cutoff /= (u64)(s64)base;  /* implicit: __udivdi3 */
    for (acc = 0, any = 0;; c = *s++) {
        if (ISDIGIT(c))
            c -= '0';
        else if (ISALPHA(c))
            c -= ISUPPER(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= (u64)(s64)base;  /* implicit: __muldi3 */
            acc += (s64)c;
        }
    }
    if (any < 0) {
        acc = neg ? (u64)S64_MIN : (u64)S64_MAX;
        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *)(any ? s - 1 : nptr);
    return (s64)acc;
}

// FUNC 0x0027F2B0 strtol
s64 strtol(const char *nptr, char **endptr, int base)
{
    return _strtol_r(_impure_ptr, nptr, endptr, base);
}

/* ------------------------------------------------------------------------ */
/* atof.c                                                                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x00280DA8 atof
/* strtod(s, NULL); called by __svfscanf for %e/%f/%g. */
double atof(const char *s)
{
    return strtod(s, NULL);
}
