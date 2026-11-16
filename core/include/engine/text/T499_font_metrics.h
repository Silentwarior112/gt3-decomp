/* T499_font_metrics.h - font metrics of a Font (translation unit T499, unit u110),
 * 0x0023D690..0x0023D7E0.
 *
 * Small accessors over the face a Font draws with (Font +0x244, T496): the
 * character width / height bytes of the face (+0x0C / +0x0D), and the width /
 * height of a string, measured by selecting the face (G001 font_set) and asking
 * the G001 text routines.  A Font without a face gives 0.  Callers: the race
 * displays (RaceGearDisplay, RaceLapTimesDisplay, ...), gt2k_menu::String.
 * u110 called the parameter's class FontText ("? name"); the callers' Font is
 * the class: its +0x244 is the face. */
#ifndef GAME_T499_FONT_METRICS_H
#define GAME_T499_FONT_METRICS_H

#include "game/game.h"

class Font;                                     /* T496 (pointers only) */

u8  text_char_width(Font *f);                   /* 0x0023D690 face->width (u8 +0x0C), 0 without a face */
u8  text_char_height(Font *f);                  /* 0x0023D6B0 face->height (u8 +0x0D), 0 without a face */
f32 text_string_width(Font *f, const char *s);  /* 0x0023D710 = font_measure_width(f, s) */
/* The width / height of s in f's face (0 without a face): font_set(face, NULL),
 * then the G001 measure (font_text_width 0x0024B440, font_text_height
 * 0x0024B538: lines x line height x scale).  text_string_width / the unreferenced text_string_height
 * (0x0023D730) are pass-throughs to them. */
f32 font_measure_width(Font *f, const char *s);  /* 0x0023D750 */
f32 font_measure_height(Font *f, const char *s); /* 0x0023D798 */

#endif /* GAME_T499_FONT_METRICS_H */
