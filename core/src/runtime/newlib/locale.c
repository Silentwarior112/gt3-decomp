/* u138 / G007: newlib locale/locale.c (only the "C" locale) and
 * stdlib/mbtowc_r.c (single-byte only). */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

/* ------------------------------------------------------------------------ */
/* locale.c                                                                 */
/* ------------------------------------------------------------------------ */

/* locale.o's .data (0x00307590..0x003075A8, between impure.o's _impure_ptr
 * and mallocr.o's __malloc_av_): MB_CUR_MAX, then two 8-byte "C" arrays that
 * no code references (locale.c's lc_ctype names; the "C"-locale
 * _setlocale_r below does not use them; egcs still emits an unused
 * initialised static, GCC 14 needs `used').  Only vfprintf (u137) reads
 * __mb_cur_max, which is why the range tool counted the block to it: the
 * anchor gives it back to this file. */
// DATA_START 0x00307590
int __mb_cur_max = 1;                           /* 0x00307590 */
static BOOT_USED char lc_ctype[8] = "C";        /* 0x00307598 ? name */
static BOOT_USED char last_lc_ctype[8] = "C";   /* 0x003075A0 ? name */

/* locale.c's static `lconv' (0x30 bytes at the start of locale.o's .rodata).
 * Defined here, not declared extern: its initializer's strings come out after
 * it in reverse order ("" at 0x00348ED0, "." at 0x00348ED8: egcs defers the
 * addressed constants of an initializer and writes them last first), and
 * _setlocale_r's "C" after them (0x00348EE0); _setlocale_r's "" is the same
 * string as the table's. */
static const struct lconv lconv =   /* 0x00348EA0 */
{
    ".", "", "", "", "", "", "", "", "", "",
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
};

// FUNC 0x00281B48 _setlocale_r
char *_setlocale_r(struct _reent *p, int category, const char *locale)
{
    if (locale) {
        if (strcmp(locale, "C") && strcmp(locale, ""))
            return NULL;
        p->_current_category = category;
        p->_current_locale = locale;
    }
    return "C";
}

// FUNC 0x00281BD0 _localeconv_r
struct lconv *_localeconv_r(struct _reent *data)
{
    return (struct lconv *)&lconv;
}

// FUNC 0x00281BE0 setlocale
char *setlocale(int category, const char *locale)
{
    return _setlocale_r(_impure_ptr, category, locale);
}

// FUNC 0x00281C10 localeconv
struct lconv *localeconv(void)
{
    return _localeconv_r(_impure_ptr);
}

/* ------------------------------------------------------------------------ */
/* mbtowc_r.c: one byte per character, no shift state (state unused)        */
/* ------------------------------------------------------------------------ */

// FUNC 0x00282758 _mbtowc_r
int _mbtowc_r(struct _reent *r, int *pwc, const char *s, size_t n, int *state)
{
    int dummy;      /* wchar_t */
    const unsigned char *t = (const unsigned char *)s;

    if (pwc == NULL)
        pwc = &dummy;

    if (s != NULL && n == 0)
        return -1;

    if (s == NULL)
        return 0;   /* not state-dependent */

    *pwc = (int)*t;
    return *t != '\0';
}
