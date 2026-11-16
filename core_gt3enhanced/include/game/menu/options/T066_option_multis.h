/* T066_option_multis.h - the multi button functions of the options screen
 * (translation unit T066, unit u022), 0x00141F70..0x001425F8.
 *
 * g_002A1CD8 is the MenuMultiFunc table (T229) OptionControl::begin (T062)
 * gives both layouts of its PageManager (PageLayout::setMultiFuncs, count
 * g_0035171C, ctx the OptionControl): "multi_assist_asm" ..
 * "multi_wide_mode" (21 entries; "multi_cfg_steering_1p" is the wide mode's
 * function too).  ButtonList::createMultiButton (T244) calls func(ctx); the
 * functions return the MultiButtonInfo by value (MenuMultiFunc, T229: the
 * caller's buffer is the hidden $a0 and comes back in $v0): index, the
 * choice from the edited GameOptions (on / off options list "on" first:
 * 1 - flag; the wide mode lists "off" first; the assist types and the boost
 * are the stored value), and changed, the function that stores a new one
 * (the wide mode through GameOptions::setWide, T428). */
#ifndef GAME_T066_OPTION_MULTIS_H
#define GAME_T066_OPTION_MULTIS_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuMultiFunc (g_002A1CD8), MultiButtonInfo */

namespace gt2k_menu {
struct MultiButtonInfo;                     /* T229 (defined there) */
}

/* T066 .data, filled by T066_static_init (21 entries, sorted by name). */
extern gt2k_menu::MenuMultiFunc g_002A1CD8[22];    /* 0x002A1CD8 (21 in GT3; the enhanced engine adds
                                                      "multi_assist", menus_a.md 23) */
extern int g_0035171C;                      /* 0x0035171C gp-0x62D4 its entries (21) */
void T066_static_init(void);                /* 0x001423A8 (.ctors 0x0028F08C) */

#endif /* GAME_T066_OPTION_MULTIS_H */
