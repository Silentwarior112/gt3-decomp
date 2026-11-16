/* T015_arcade_menu.h - the entry points of the arcade menu (translation unit
 * T015, unit u007), 0x00116508..0x001168B8.
 *
 * The arcade menu runs on a heap of its own (memalign'ed, pushed as the menu
 * heap, T204) with the ArcadeControl (T010, 0x3BE0 bytes) on the stack; the
 * choices go to the caller's MenuArgs (ArcadeControl::finish). */
#ifndef GAME_T015_ARCADE_MENU_H
#define GAME_T015_ARCADE_MENU_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

struct MenuArgs;                            /* T000: the results of a menu (pointers only) */

/* One run with a 3.5 MB menu heap: ArcadeControl(args).run(). */
int run_arcade_menu(MenuArgs *args);        /* 0x00116508 */
/* The arcade menu (T000 main): run_arcade_menu again after loading a guest
 * garage from the memory card (GameStatus menu.nextMode 0x1C, T068
 * F_0014AE00) and after the ending movies (nextMode 0xE / 0xF:
 * "stream/ending_short_full.pss" / "stream/ending_long_full.pss", T051
 * F_001287A8); any other nextMode returns run_arcade_menu's result. */
int arcade_menu(MenuArgs *args);            /* 0x001165D8 */
/* No callers (a test): ArcadeControl::runPreview on a 4 MB heap filled with
 * 0xA5. */
int arcade_preview_test(void);              /* 0x00116748 */

#endif /* __cplusplus */

#endif /* GAME_T015_ARCADE_MENU_H */
