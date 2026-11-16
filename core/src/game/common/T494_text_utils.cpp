/* T494 - string and number formatting helpers (the dash glyph of the menu
 * font is 0x7F: the formatters' "no value"), and two quad drawers.
 * 0x0023B788..0x0023C0B8.
 *
 * Data: the functions' literals only (.rodata 0x00343A28..0x00343AAA, the
 * ones of 8 bytes or less in .sdata 0x00352F30..0x00352F4D, -G8); the
 * headers write no type name. */
#include "libc.h"                               /* strcat, strcpy, strlen */
#include "engine/pgl/G001_flare.h"                    /* PrimVertex, gfx_alloc_tail, pgl_draw_strip */
#include "game/race/core/T128_race_lap_record.h"          /* RACE_TIME_INVALID */
#include "game/data/T516_country.h"                  /* Country g_00352FD8, COUNTRY_JP */
#include "game/data/T518_unit_settings.h"            /* UnitSettings g_002F59D8 */
#include "engine/pdistd/T534_sprintf.h"                  /* sprintf */
#include "game/data/car_database_internal.h"
#include "game/common/T494_text_utils.h"

/* 1 if a and b are both non-NULL and equal strings. */
// FUNC 0x0023B788 str_equal
int str_equal(const char *a, const char *b)
{
    if (a == 0 || b == 0)
        return 0;
    if (a == b)
        return 1;
    for (;;) {
        char c = *a++;

        if (c != *b++)
            return 0;
        if (c == 0)
            return 1;
    }
}

/* 1 if a sorts before b (signed chars; a proper prefix sorts first).  No
 * callers. */
// FUNC 0x0023B7D0 str_less
int str_less(const char *a, const char *b)
{
    int lenA = strlen(a);
    int lenB = strlen(b);
    int shorter = lenA < lenB;
    int i;

    for (i = 0;; i++) {
        /* the test inside: retail's loop is not rotated */
        if (i >= lenA || i >= lenB)
            return shorter;
        if (a[i] < b[i])
            return 1;
        if (b[i] < a[i])
            return 0;
    }
}

/* value in decimal with a ',' every three digits, built backwards in a
 * 0x40-byte buffer.  "%ld" with the 64-bit register the callers pass (the
 * game's printf takes it whole; GCC 14 warns about the format). */
// FUNC 0x0023B868 format_number
void format_number(char *buf, s64 value)
{
    char grouped[0x80];                 /* retail's frame has 0x80 bytes of locals;
                                           the text ends at [0x3F] */
    int len;
    int out;                            /* the next free byte of grouped, from the end */
    int digits;
    int i;                              /* the next digit of buf, from the end */

    sprintf(buf, "%ld", value);
    len = strlen(buf);
    i = len - 1;
    grouped[0x3F] = 0;
    out = 0x3E;
    digits = 0;
    while (digits < len) {
        digits++;
        char c = buf[i--];              /* a char value: retail's lb */

        grouped[out--] = c;
        if (digits % 3 == 0 && digits < len)
            grouped[out--] = ',';
    }
    strcpy(buf, &grouped[out + 1]);
}

/* A time in ms: "h:mm'ss.mmm", or "m'ss.mmm" under an hour (unsigned
 * divisions); RACE_TIME_INVALID (0x157529FF = 359999999 ms, "no time")
 * gives the dash glyphs. */
// FUNC 0x0023B930 format_time
void format_time(char *buf, u32 ms)
{
    u32 msec, sec, min, hours, rest;

    if (ms == RACE_TIME_INVALID) {
        strcpy(buf, "\x7f\x7f'\x7f\x7f.\x7f\x7f\x7f");     /* 0x00343A28 (copied inline) */
        return;
    }
    msec = ms % 1000;
    rest = ms / 1000;
    sec = rest % 60;
    rest = rest / 60;
    min = rest % 60;
    hours = rest / 60;
    if (hours != 0)
        sprintf(buf, "%d:%02d'%02d.%03d", hours, min, sec, msec);
    else
        sprintf(buf, "%d'%02d.%03d", min, sec, msec);
}

/* A speed in m/s, in the user's unit (UnitSettings::convSpeed: km/h or
 * mph), one decimal (truncated), then the unit name; dashes and the unit
 * name for speed <= 0. */
// FUNC 0x0023BA08 format_speed
void format_speed(char *buf, f32 mps)
{
    const char *unit = g_002F59D8.speedName();
    f32 speed;

    if (mps <= 0.0f) {
        strcpy(buf, "\x7f\x7f\x7f.\x7f");                  /* 0x00352F38 (copied inline) */
        strcat(buf, unit);
        return;
    }
    speed = g_002F59D8.convSpeed(mps);
    sprintf(buf, "%d.%1d%s", (int)speed, (int)(speed * 10.0f) % 10, unit);
}

/* A date: "yy/mm/dd" in Japan (COUNTRY_JP), else "dd/mm/yy" (each "%02d").
 * With fullYear, a year before 2000 gives dashes for a 4-digit year;
 * without, a negative year gives dashes. */
// FUNC 0x0023BAF8 format_date
void format_date(char *buf, int day, int month, int year, int fullYear)
{
    int country = g_00352FD8.value;

    /* one test for the printable cases: retail lays out the sprintf
     * paths first, then the 4-digit dashes, then the 2-digit ones */
    if ((fullYear == 0 && year >= 0) || (fullYear != 0 && year >= 2000)) {
        if (country == COUNTRY_JP)
            sprintf(buf, "%02d/%02d/%02d", year, month, day);
        else
            sprintf(buf, "%02d/%02d/%02d", day, month, year);
    } else if (fullYear != 0) {
        if (country == COUNTRY_JP)
            strcpy(buf, "\x7f\x7f\x7f\x7f/\x7f\x7f/\x7f\x7f");  /* 0x00343A80 */
        else
            strcpy(buf, "\x7f\x7f/\x7f\x7f/\x7f\x7f\x7f\x7f");  /* 0x00343A90 */
    } else {
        strcpy(buf, "\x7f\x7f/\x7f\x7f/\x7f\x7f");     /* 0x00343AA0 */
    }
}

/* 1 if s starts with prefix. */
// FUNC 0x0023BC08 str_has_prefix
int str_has_prefix(const char *prefix, const char *s)
{
    /* 70.3%: the loop is retail's; retail keeps the copy of the exit
     * test in front of the loop (lb, beqz, then the second load and the
     * increments), ours is merged back into `b <test>' by jump2's
     * cross-jumping (both copies identical).  Tried: while / for / do
     * shapes, the test before or after the loads and increments, the chars
     * declared outside, the reference compiler (the same). */
    for (;;) {
        char c = *prefix++;
        char d = *s++;

        if (c == 0)
            break;
        if (c != d)
            return 0;
    }
    return 1;
}

/* 1 if s ends with suffix: compares backwards from both terminators,
 * strlen(suffix) + 1 characters (no check that s is long enough). */
// FUNC 0x0023BC48 str_has_suffix
int str_has_suffix(const char *suffix, const char *s)
{
    int suffixLen = strlen(suffix);
    int sLen = strlen(s);
    int i = 0;

    do {
        if (suffix[suffixLen - i] != s[sLen - i])
            return 0;
        i++;
    } while (i <= suffixLen);
    return 1;
}

/* Zeroes size bytes: as words (from the last) if size is a multiple of 4,
 * else as bytes (from the last). */
// FUNC 0x0023BCC0 zero_memory
void zero_memory(void *p, int size)
{
    if ((size & 3) == 0) {
        int i;

        for (i = size / 4 - 1; i >= 0; i--)
            ((int *)p)[i] = 0;
        return;
    }
    while (--size >= 0)
        ((char *)p)[size] = 0;
}

/* A 16-bit string to 7-bit ASCII (the low byte & 0x7F of each unit).  No
 * callers. */
// FUNC 0x0023BD40 wide_to_ascii
void wide_to_ascii(const u16 *src, char *dst)
{
    int i = 0;

    while (src[i] != 0) {
        dst[i] = src[i] & 0x7F;
        i++;
    }
    dst[i] = 0;
}

/* strcpy(dst, src), then cuts dst at its last '_' (a wheel or car item name
 * without its suffix). */
// FUNC 0x0023BD98 str_cut_last_underscore
void str_cut_last_underscore(char *dst, const char *src)
{
    int i;

    strcpy(dst, src);
    for (i = strlen(dst); i >= 0; i--) {
        if (dst[i] == '_')
            break;
    }
    if (i >= 0)
        dst[i] = 0;
}

/* out = path's directory (up to its last '/') + prefix + its file name;
 * prefix + path if there is no '/'.  0x80-byte buffers.  No callers. */
// FUNC 0x0023BE10 path_insert_prefix
void path_insert_prefix(char *out, const char *path, const char *prefix)
{
    char dir[0x80];
    char file[0x80];
    int len = strlen(path);
    int i;

    strcpy(dir, path);
    for (i = len - 1; i >= 0; i--) {
        if (dir[i] == '/')
            break;
    }
    if (i >= 0) {
        strcpy(file, path + i);         /* from the '/' */
        dir[i + 1] = 0;
        sprintf(out, "%s%s%s", dir, prefix, file + 1);
    } else {
        sprintf(out, "%s%s", prefix, path);
    }
}

/* A line (x0,y0,z0)-(x1,y1,z1) as a 4-vertex strip.  Quirk: the half-width
 * offset (width/2 * dy / length) is applied to x only, and y gets a fixed
 * +-0.5. */
// FUNC 0x0023BEF0 draw_line_quad
void draw_line_quad(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 width)
{
    f32 dx = x1 - x0;
    f32 dy = y1 - y0;
    f32 len = boot_sqrtf_v(dx * dx + dy * dy);  /* sqrt.s, then div.s (no rsqrt.s) */
    f32 offset = width * 0.5f / len * dy;
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);   /* 3 quadwords: 4 vertices */

    v[0].x = x0 + offset;
    v[1].x = x0 - offset;
    v[2].x = x1 + offset;
    v[3].x = x1 - offset;
    v[0].y = y0 - 0.5f;
    v[1].y = y0 + 0.5f;
    v[2].y = y1 - 0.5f;
    v[3].y = y1 + 0.5f;
    v[0].z = z0;
    v[1].z = z0;
    v[2].z = z1;
    v[3].z = z1;
    pgl_draw_strip(v, 0, 0, 0, 4);
}

/* A w x h rectangle centred on (x, y), z 0 (stored as the integer 0), as a
 * 4-vertex strip. */
// FUNC 0x0023C008 draw_rect
void draw_rect(f32 x, f32 y, f32 w, f32 h)
{
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
    f32 halfW, halfH;

    v[0].z = 0;
    v[1].z = 0;
    v[2].z = 0;
    v[3].z = 0;
    halfW = w * 0.5f;
    halfH = h * 0.5f;
    /* each corner value goes to both vertices that share it */
    v[2].x = v[0].x = x - halfW;
    v[1].y = v[0].y = y - halfH;
    v[3].x = v[1].x = x + halfW;
    v[3].y = v[2].y = y + halfH;
    pgl_draw_strip(v, 0, 0, 0, 4);
}
