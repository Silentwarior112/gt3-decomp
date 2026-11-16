/* T065_option_sliders.h - the slider functions of the options screen
 * (translation unit T065, unit u022), 0x00141E18..0x00141F70.
 *
 * g_002A1C38 is the MenuSliderFunc table (T229) OptionControl::begin (T062)
 * gives both layouts of its PageManager (PageLayout::setSliderFuncs, count
 * g_00351718, ctx the OptionControl): "slider_race_laps" and
 * "slider_split_race_laps" [1, 100) by 1 (1..99 laps),
 * "slider_split_handicap" [-150, 151) by 5 (-150..150 m), each with a set
 * function that stores the value in the edited GameOptions and returns 1;
 * mode 1 in every record, no inner range (limitMin == limitMax == 0).
 * ButtonList::createSlider (T244) calls func(ctx); the functions return the
 * SliderDesc by value (MenuSliderFunc, T229: the caller's buffer is the
 * hidden $a0 and comes back in $v0).  The set functions take T238's
 * SliderSetFn arguments (ctx, value, commit; commit not used).
 *
 * ::SliderDesc (u022, u035) is T238's gt2k_menu::SliderDesc: min, max
 * (exclusive), value, step, set, mode (u022 / u035 unk14), limitMin /
 * limitMax (u022 unk18 / unk1C).  There is no global alias: with `using
 * namespace gt2k_menu' egcs finds a global typedef and the class ambiguous,
 * so the units write SliderDesc under the using-directive (or
 * gt2k_menu::SliderDesc). */
#ifndef GAME_T065_OPTION_SLIDERS_H
#define GAME_T065_OPTION_SLIDERS_H

#include "game/game.h"
#include "game/menu/framework/T238_slider.h"               /* gt2k_menu::SliderDesc */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuSliderFunc (g_002A1C38) */

namespace gt2k_menu {
struct SliderDesc;                          /* T238 (defined there): 0x20 bytes */
}

/* T065 .data, filled by T065_static_init (3 entries, sorted by name). */
extern gt2k_menu::MenuSliderFunc g_002A1C38[3];    /* 0x002A1C38 */
extern int g_00351718;                      /* 0x00351718 gp-0x62D8 its entries (3) */
void T065_static_init(void);                /* 0x00141F10 (.ctors 0x0028F088) */

#endif /* GAME_T065_OPTION_SLIDERS_H */
