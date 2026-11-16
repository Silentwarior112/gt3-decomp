/* T113_shortmenu_tables.h - the two tables of the short menu's qm_* page
 * actions and hooks and their entry counts (translation unit T113, unit
 * u015; ShortMenuText and the functions are in T113_shortmenu_items.h, which
 * includes this header).
 *
 * In its own header since u015's data pass (2026-10-17): the ShortmenuControls
 * of T111 and T103 give the tables to their PageManager, but of the
 * originals only T113 has ShortMenuText's type name (egcs writes the type
 * name of every class it sees whose virtual functions are all inline:
 * ShortMenuText has no key function). */
#ifndef GAME_T113_SHORTMENU_TABLES_H
#define GAME_T113_SHORTMENU_TABLES_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ExecuteFuncData (g_0029C6B0), MenuHook (T229) */

/* The tables (T113 .data, zeros in the image: filled by T113_static_init). */
extern gt2k_menu::ExecuteFuncData g_0029C6B0[21];  /* 0x0029C6B0 the qm_* actions */
extern gt2k_menu::MenuHook        g_0029C758[45];  /* 0x0029C758 the qm_* hooks */
/* Their entry counts (T113 .sdata; the ShortmenuControls of T111 / T103
 * pass them to PageManager / PageLayout). */
extern int g_00351454;                      /* 0x00351454 gp-0x659C = 0x15: g_0029C6B0 */
extern int g_00351480;                      /* 0x00351480 gp-0x6570 = 0x2D: g_0029C758 */

#endif /* GAME_T113_SHORTMENU_TABLES_H */
