/* T028_arcade_menu_hooks.h - the ExecuteFuncData table of the arcade menu
 * (translation unit T028, unit u005), 0x0010FB40..0x0010FE58.
 *
 * g_002927B8 maps the exec names of the arcade pages' buttons (sorted for
 * the binary search) to their functions; ArcadeControl::setup (T010) gives
 * it to its PageManager (T211 +0xB18, count g_00350EA0, ctx the
 * ArcadeControl).  The PageManager calls func(ctx, rec, index, layout) for a
 * decided button whose ButtonData::exec matches (rec->exec is at +0xC0).
 *
 * The hooks take the ExecuteFuncData signature (the first pass had
 * (ControlBase *owner, void *arg)); only ctx and rec are used.  Their
 * member function ArcadeControl::setModeByName (0x0010FB60) is declared with
 * the class (T010_arcade_control.h). */
#ifndef GAME_T028_ARCADE_MENU_HOOKS_H
#define GAME_T028_ARCADE_MENU_HOOKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_002927B8) */

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only) */
class PageLayout;                           /* T229 */
}

/* "compulsory", "goodies", "i_link_battle", "load_guest_garage",
 * "single_race", "time_attack", "two_player_battle":
 * ArcadeControl::setModeByName(rec->exec). */
void select_game_hook(void *ctx, gt2k_menu::ButtonData *rec, int index,
                      gt2k_menu::PageLayout *layout);          /* 0x0010FB40 */
/* "i_link_live", "i_link_play": g_00350FC8 = 0 (T010), then
 * pdi1394::sendByte(1) on the link (F_0022D3B8). */
void i_link_start_hook(void *ctx, gt2k_menu::ButtonData *rec, int index,
                       gt2k_menu::PageLayout *layout);         /* 0x0010FC78 */
/* "cancel_select_game": g_00352750 = g_0035274C = 0 (T261_menu_draw_state.h:
 * no screen clear before the frames). */
void cancel_select_game_hook(void *ctx, gt2k_menu::ButtonData *rec, int index,
                             gt2k_menu::PageLayout *layout);   /* 0x0010FCA8 */
/* "go_ending_easy" / "go_ending_normal": the GameStatus menu.nextMode 14 /
 * 15 (the ending movies, T015 arcade_menu). */
void go_ending_easy_hook(void *ctx, gt2k_menu::ButtonData *rec, int index,
                         gt2k_menu::PageLayout *layout);       /* 0x0010FCB8 */
void go_ending_normal_hook(void *ctx, gt2k_menu::ButtonData *rec, int index,
                           gt2k_menu::PageLayout *layout);     /* 0x0010FCE8 */

/* T028 .data, filled by T028_static_init (13 entries, sorted by name;
 * [9] "set_garage_car" is T037's action_set_garage_car 0x00125D60). */
extern gt2k_menu::ExecuteFuncData g_002927B8[13];  /* 0x002927B8 */
extern const int g_00350EA0;                /* 0x00350EA0 gp-0x6B50 its entries (13; const: see T016_ending_menu.h) */
void T028_static_init(void);                /* 0x0010FD18 (.ctors 0x0028F03C) */

#endif /* GAME_T028_ARCADE_MENU_HOOKS_H */
