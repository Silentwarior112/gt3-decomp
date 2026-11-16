/* T262_menu_bgm.h - the menu BGM starters (translation unit T262, unit u073),
 * 0x001D27F0..0x001D28C0.
 *
 * Both start a BGM track at volume 0.8 through T464, only when the "menu BGM"
 * game option (GameStatus+0x2FF3C, T429) is on.  C++ linkage (a C++
 * translation unit). */
#ifndef GAME_T262_MENU_BGM_H
#define GAME_T262_MENU_BGM_H

#include "game/game.h"

void menu_bgm_play_if_changed(int group, int track, int arg); /* 0x001D27F0 (F_001D27F0)
                                                   bgm_play_if_changed(group, track, 0.8, arg) (12 callers:
                                                   the menu pages) */
void menu_bgm_play(int group, int track, int arg);            /* 0x001D2858 (F_001D2858)
                                                   bgm_play(group, track, 0.8, arg) */

#endif /* GAME_T262_MENU_BGM_H */
