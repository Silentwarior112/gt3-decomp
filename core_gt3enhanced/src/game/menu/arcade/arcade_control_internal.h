/* arcade_control_internal.h - unit u007 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00113540..0x001168B8, second pass.
 *
 * Translation units (link order):
 *   T031  ArcadeControl pages "ar/top", "ar/level", "ar/goodies"
 *   T010  gt2k_menu::ArcadeControl: the arcade-mode menu controller (mode
 *         flows, setup / teardown, results written to the caller's MenuArgs)
 *   T011  gt2k_menu::ArcadeRender (the controller's RenderBase)
 *   T012  ArcadeControl page "ar/push_start" (attract screen)
 *   T013  ArcadeControl i.LINK pages "ar/connect" and "ar/wait_other"
 *   T014  ArcadeControl i.LINK page "ar/wait_other" (selection exchange)
 *   T032  gt2k_menu::CircleGauge (round gauge of one car statistic)
 *   T015  the arcade menu entry points (menu heap, ArcadeControl on the stack)
 *
 * T031 / T012 / T013 / T014 have no header of their own: their functions are
 * ArcadeControl members, declared with the class in T010_arcade_control.h.
 * Every class is defined in a canonical header.  Each source includes the
 * canonical headers it needs itself, in the order that gives the original's
 * typeinfo names (the data pass, ref/data/u007/notes.md: egcs writes them in
 * the reverse order of the classes' definitions, and a header that defines
 * classes the original did not see adds names retail does not have); this
 * header has only the unit's local constants. */
#ifndef U007_H
#define U007_H

#include "types.h"

/* enum ArcadeClass (ARCADE_CLASS_*): game/menu/arcade/T010_arcade_control.h. */

/* The background movies of the arcade pages (g_00292930[], T029). */
enum {
    ARCADE_MOVIE_TOP   = 0,                 /* "stream/arcade_menu_full.pss" 640 x 448 */
    ARCADE_MOVIE_LEVEL = 1,                 /* "stream/level_menu_full.pss" 640 x 448 */
    ARCADE_MOVIE_BG2   = 3,                 /* "stream/menu_bg2.pss" 320 x 112 */
    ARCADE_MOVIE_BG    = 5                  /* "stream/menu_bg.pss" 640 x 224 */
};

/* GameStatus::menu.nextMode values the arcade menu sets or handles (main's
 * dispatch, T000, takes the others). */
enum {
    NEXT_MODE_ENDING_SHORT = 0xE,           /* the short ending movie (set by "ar/select_ending", T016) */
    NEXT_MODE_ENDING_LONG  = 0xF,           /* the long one */
    NEXT_MODE_GUEST_GARAGE = 0x1C           /* load the guest garage from a memory card (T015) */
};

/* The menu sound effect of a decision: T233's MENU_SE_DECIDE. */

/* The idle frames after which the attract screen's pages time out (60 s). */
#define ARCADE_ATTRACT_TIMEOUT 3600

/* The frames "ar/connect" waits once a link was seen (g_00350FC8, 9 s). */
#define ARCADE_CONNECT_COUNTDOWN 540

#endif /* U007_H */
