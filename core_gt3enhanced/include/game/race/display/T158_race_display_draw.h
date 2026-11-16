/* T158_race_display_draw.h - helpers of the race display (translation unit
 * T158, unit u055), 0x0019A148..0x0019AB38.
 *
 * Textured and plain rectangles of the 2D prim buffer (F_00255760, drawn as
 * strips by F_002501A8), the alignment of a position, the time / speed
 * formatters and the rank names.  Used by the T157 displays, the meters
 * (T160, T161, T164), the race display (T154) and the license tests.
 *
 * Alignment (align & 3): 0 left / top, 1 right / bottom, else centred;
 * align & 4 shifts left / right by 106.667 (0x6A as int) on the wide /
 * split screen (g_002F424C).  A texture's size is read from its CLAMP
 * register (REGION_CLAMP MAXU bits 14..23, MAXV bits 34..43, + 1); the UVs
 * cover it inside its power-of-two page.
 *
 * C++ functions (C++ linkage); the header has no classes and also reads as C.
 */
#ifndef GAME_T158_RACE_DISPLAY_DRAW_H
#define GAME_T158_RACE_DISPLAY_DRAW_H

#include "game/game.h"

struct Tex1Image;                               /* G001 (pointers only) */

/* The alignment values of race_align_pos / posf and the draw helpers (and
 * RaceDisplayObjectBase::alignX / alignY, RACE_ALIGN_CENTER by default):
 * align & 3 the anchor, align & 4 shifted outwards by RACE_WIDE_SHIFT
 * (RACE_WIDE_SHIFT_INT for race_align_pos) when the wide screen mode
 * (g_002F424C) is on.  (The sweep moved u053 / u054 / u055 / u056's copies
 * here.) */
enum {
    RACE_ALIGN_LEFT   = 0,                  /* left / top: pos - shift */
    RACE_ALIGN_RIGHT  = 1,                  /* right / bottom: pos + shift - size */
    RACE_ALIGN_CENTER = 2,                  /* centred (also 3): pos - size / 2, no shift */
    RACE_ALIGN_WIDE   = 4,                  /* + the wide screen shift (left: -, right: +) */
    RACE_ALIGN_TOP    = RACE_ALIGN_LEFT,    /* the same values for y */
    RACE_ALIGN_BOTTOM = RACE_ALIGN_RIGHT
};
/* The shift: 106.66667 (0x42D55556; T156 gp-0x7C6C = 0x0034FD84, T158 its
 * own copy gp-0x7C38 = 0x0034FDB8, T157 more copies); the integer one 0x6A. */
#define RACE_WIDE_SHIFT     106.66667175f
#define RACE_WIDE_SHIFT_INT 0x6A            /* 106 */

/* Draws tex at its own size, aligned at (x, y); nothing for NULL.  (u054:
 * F_0019A148.) */
void race_draw_image(struct Tex1Image *tex, int alignX, int alignY, f32 x, f32 y);  /* 0x0019A148 */
/* The same stretched over the w x h rectangle aligned at (x, y).  The
 * alignments come last (u055's matching: retail's parameter copies are
 * ordered so, as in race_align_posf below). */
void race_draw_image_rect(struct Tex1Image *tex, f32 x, f32 y, f32 w, f32 h,
                          int alignX, int alignY);  /* 0x0019A348 */
/* An untextured w x h rectangle in the current colour (pgl_color). */
void race_fill_rect(int x, int y, int w, int h);                        /* 0x0019A548 */
/* The same with its corners cut by `corner' pixels (an 8-vertex strip). */
void race_fill_octagon(int x, int y, int w, int h, int corner);         /* 0x0019A608 */
/* The float alignment: left pos - shift, right pos + shift - size, centred
 * pos - size / 2 (no shift).  The same parameter order as race_align_pos
 * (the registers are a0 / f12 / f13 either way; u055's matching: callers
 * set pos up before align). */
f32  race_align_posf(f32 pos, f32 size, int align);                     /* 0x0019A730 */
/* The integer one: centred is pos - (size >> 1). */
int  race_align_pos(int pos, int size, int align);                      /* 0x0019A790 */
/* A time in ms as M'SS.mmm (1 to 3 minute digits, clamped to 999'59.999);
 * < 0 or 359999999 (no time): "-'--.---" in the font's dash glyph 0x7F. */
void race_format_time(char *buf, int ms);                               /* 0x0019A7F0 */
/* "%.1f" of the speed clamped to 999.9; <= 0 or FLT_MAX: "---.-". */
void race_format_speed(char *buf, f32 speed);                           /* 0x0019A968 */
/* "1st".."12th" for 1..12 (g_0049C3B0, set by T158_static_init from the
 * race message table g_002C43E8[37..48]), else "". */
const char *race_rank_name(int rank);                                   /* 0x0019AA08 */
/* Copies src (NULL: "") to dst; size < 0: unbounded, else at most size - 1
 * characters and dst[size - 1] = 0. */
void race_copy_string(char *dst, const char *src, int size);            /* 0x0019AA38 */
/* (The rank names g_0049C3B0 are a file static of T158, with their dynamic
 * initialiser: retail has them in .bss, which egcs gives only a static;
 * u055's data pass.  No other file uses them.) */

/* (__ti18RaceRoundMeterBase, which the plan put here, is declared with its
 * class in T160_race_meters.h.) */

#endif /* GAME_T158_RACE_DISPLAY_DRAW_H */
