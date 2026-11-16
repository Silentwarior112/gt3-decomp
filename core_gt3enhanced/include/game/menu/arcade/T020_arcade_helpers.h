/* T020_arcade_helpers.h - the helpers of the arcade pages (translation unit
 * T020, unit u002), 0x00104F30..0x001060B8.
 *
 * Most of T020 is member functions of gt2k_menu::ArcadeControl (declared
 * with the class in T010_arcade_control.h: transitionIn / transitionOut,
 * the arrows, attachToBar, the history banners, createMiniCursor,
 * createLabel), called by the arcade pages T018, T021..T025.  The free
 * functions are here.
 *
 * gt2k_menu::MenuColor is the first-pass name (u002, u028, u062, u065,
 * u069) of the four-float colour Color4f (T218): createLabel unpacks the
 * page text colours with the inline Color4f(u32) into a String's vertex
 * colours (Drawable::color, Color4f[4]). */
#ifndef GAME_T020_ARCADE_HELPERS_H
#define GAME_T020_ARCADE_HELPERS_H

#include "game/game.h"
#include "game/menu/framework/T218_color.h"                /* Color4f (gt2k_menu::MenuColor) */

namespace gt2k_menu {
class Transitive;                           /* T216 (pointers only) */

typedef ::Color4f MenuColor;                /* 0x10 bytes: r, g, b, a */
}

/* The end callback of the page-out script g_00309A38 (stored in it at
 * 0x00309ACC): clears g_00350C18, which ArcadeControl::transitionOut waits
 * on. */
void transitionOutDone(void);               /* 0x00104F58 */
extern int g_00350C18;                      /* 0x00350C18 gp-0x6DD8 1 while the page-out script runs */

/* Starts the script g_00309B28 (T020 .rodata) on t: alpha -4 / s for 0.25 s,
 * alpha 0, then opRelease (kill()): the item fades out and dies.  The pages
 * call it on an item right before they forget or replace it. */
void arcade_fade_out(gt2k_menu::Transitive *t);     /* 0x00105150 (first pass: F_00105150) */

/* (g_00352704 + 448) * n, g_00352704 = 40.0 (gp-0x52EC, a variable): the
 * y offset of screen row n (488 per row, the Cross's vertical wrap), i.e. of
 * option row n of the car selects (1 transmission, 2 racing; T018
 * compulsoryOpen, T023 ArcadeControl::scrollToRow / buildCarSelect). */
f32 F_001051B0(int n);                      /* 0x001051B0 */

#endif /* GAME_T020_ARCADE_HELPERS_H */
