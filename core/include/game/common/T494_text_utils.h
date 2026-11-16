/* T494_text_utils.h - string and number formatting helpers and two quad
 * drawers (translation unit T494, unit u109), 0x0023B788..0x0023C0B8.
 *
 * C++ translation unit without classes: the functions have C++ linkage (every
 * caller is a C++ unit); the header also compiles as C.  The dash glyph of
 * the menu font is 0x7F (the "no value" output of the formatters). */
#ifndef GAME_T494_TEXT_UTILS_H
#define GAME_T494_TEXT_UTILS_H

#include "game/game.h"

int  str_equal(const char *a, const char *b);               /* 0x0023B788 1 if both non-NULL and equal */
int  str_less(const char *a, const char *b);                /* 0x0023B7D0 1 if a sorts before b (no callers) */
void format_number(char *buf, s64 value);                   /* 0x0023B868 decimal, ',' every three digits */
void format_time(char *buf, u32 ms);                        /* 0x0023B930 "h:mm'ss.mmm" / "m'ss.mmm";
                                                               359999999 ("no time") gives dashes */
void format_speed(char *buf, f32 mps);                      /* 0x0023BA08 a speed in m/s, in the user's unit
                                                               (UnitSettings::convSpeed), one decimal, then
                                                               the unit name; dashes for <= 0 */
void format_date(char *buf, int day, int month, int year, int fullYear); /* 0x0023BAF8 "yy/mm/dd" in
                                                               region 0, else "dd/mm/yy" */
int  str_has_prefix(const char *prefix, const char *s);     /* 0x0023BC08 */
int  str_has_suffix(const char *suffix, const char *s);     /* 0x0023BC48 (s is not checked to be long enough) */
void zero_memory(void *p, int size);                        /* 0x0023BCC0 as words if size % 4 == 0, else bytes */
void wide_to_ascii(const u16 *src, char *dst);              /* 0x0023BD40 low byte & 0x7F of each unit (no callers) */
void str_cut_last_underscore(char *dst, const char *src);   /* 0x0023BD98 strcpy, then cut at the last '_' */
void path_insert_prefix(char *out, const char *path, const char *prefix); /* 0x0023BE10 dir + prefix + file
                                                               (no callers) */
void draw_line_quad(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 width); /* 0x0023BEF0 a line as a
                                                               4-vertex strip */
void draw_rect(f32 x, f32 y, f32 w, f32 h);                 /* 0x0023C008 a w x h rectangle centred on (x, y) */

#endif /* GAME_T494_TEXT_UTILS_H */
