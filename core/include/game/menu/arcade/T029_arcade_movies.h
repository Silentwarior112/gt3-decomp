/* T029_arcade_movies.h - the background movies of the arcade menu
 * (translation unit T029, unit u005), 0x0010FE58..0x0010FF90.
 *
 * Only data: the MovieParam table g_00292930 and its static initialiser.
 *   [0] "stream/arcade_menu_full.pss"  640 x 448 (the arcade top, T031)
 *   [1] "stream/level_menu_full.pss"   640 x 448
 *   [2] "stream/laguna_full.pss"       640 x 448
 *   [3] "stream/menu_bg2.pss"          320 x 112 (T010 setup, T014, T023)
 *   [4] "stream/laguna_full.pss"       640 x 448 (the course select, T025)
 *   [5] "stream/menu_bg.pss"           640 x 224
 *   [6] "stream/arcade_top.pss"        320 x 448
 *   [7] "stream/arcade_level.pss"      320 x 448
 * field is 1 for the 640 x 448 ones; withSound / oneShot are 0 (the menu
 * backgrounds).
 *
 * Other units name single entries by their address: g_00292948 = [1],
 * g_00292960 = [2], g_00292978 = [3], g_002929A8 = [5] (u003, u004, u007;
 * u003's MovieDesc is this MovieParam): they are g_00292930[n], not
 * symbols of their own. */
#ifndef GAME_T029_ARCADE_MOVIES_H
#define GAME_T029_ARCADE_MOVIES_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */
#include "game/app/T000_main.h"                 /* MovieParam */

extern MovieParam g_00292930[8];            /* 0x00292930 (.data; [3] is 0x00292978) */
void T029_static_init(void);                /* 0x0010FE58 (.ctors 0x0028F040) */
#endif /* __cplusplus */

#endif /* GAME_T029_ARCADE_MOVIES_H */
