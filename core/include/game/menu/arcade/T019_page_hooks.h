/* T019_page_hooks.h - the FocusFuncData table of the arcade menu
 * (translation unit T019, unit u002), 0x00104EE0..0x00104F30.
 *
 * ArcadeControl::setup (T010) gives g_00290940 to its PageManager (T211)
 * as the FocusFuncData table (hooks[4], ctx the ArcadeControl): the initial
 * cursor of "ar/connect" is button 1, that of "ar/top" the current mode
 * (ArcadeControl::mode).  The static initialiser fills the table in. */
#ifndef GAME_T019_PAGE_HOOKS_H
#define GAME_T019_PAGE_HOOKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::FocusFuncData */

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
}

/* {"ar/connect", arcade_connect_focus}, {"ar/top", arcade_top_focus}. */
extern gt2k_menu::FocusFuncData g_00290940[2];     /* 0x00290940 */
extern const int g_00350C08;                /* 0x00350C08 gp-0x6DE8 its entries (2; const: see T016_ending_menu.h) */

/* ctx is the ArcadeControl; pm is not used. */
int  arcade_top_focus(void *ctx, gt2k_menu::PageManager *pm);       /* 0x00104EE0 ((ArcadeControl *)ctx)->mode */
int  arcade_connect_focus(void *ctx, gt2k_menu::PageManager *pm);   /* 0x00104EE8 1 */
void T019_static_init(void);                /* 0x00104EF0 (.ctors 0x0028F034) */

#endif /* GAME_T019_PAGE_HOOKS_H */
