/* T233_menu_sound.h - the menu music of a page, the menu sound effects
 * (translation unit T233, unit u068).
 *
 * MenuSound (? name; u064 has it as gt2k_menu::MenuSound) is held by the
 * PageManager (T211, +0xAF8). */
#ifndef GAME_T233_MENU_SOUND_H
#define GAME_T233_MENU_SOUND_H

#include "game/game.h"

/* Not polymorphic.  Size 4 (PageManager +0xAF8 .. +0xAFC). */
class MenuSound {
public:
    MenuSound();                            /* 0x001C4D78 bgm 0x23 (none) */
    int selectMusic(const char *page);      /* 0x001C4D88 the music of a page by its name (mostly the
                                               folder prefix: "cd/" 2, "at/" 0x10, "hm/" 6, "lc/" 7,
                                               "r/" 4, "ts/" 0xF, "map" 5, "mt/" 9, "ar/" 0, ...;
                                               others 0x23: the music stops); changed only when it
                                               differs; returns 1 */

    int bgm;                                /* 0x0 the music playing (T262 F_001D27F0), 0x23: none */
};
EGCS_SIZE(MenuSound, 0x4);

/* The menu sound effects: menu_play_se's (and T202 menu_sound_after's) se.
 * One set for every unit (the sweep merged u002 / u003 / u004 / u005 / u007 /
 * u008 / u009 / u013 / u014 / u022 / u028..u031 / u036..u040 / u061 / u062 /
 * u068 / u069 / u070 / u074's local copies: ARCADE_SE_*, MC_SE_*,
 * LOGGER_SE_*, RACE_MENU_SE_*, RECORDS_SE_PANEL, OIL_SE). */
enum MenuSe {
    MENU_SE_BOUGHT    = 0,                  /* (? name) a car bought (u013, after a delay) */
    MENU_SE_CANCEL    = 1,                  /* cancel: back to the previous page (PageManager::back) */
    MENU_SE_CAR_WASH  = 2,                  /* (? name) the car wash starts spinning (u013) */
    MENU_SE_CURSOR    = 3,                  /* the cursor / a choice moved */
    MENU_SE_REFUSED   = 4,                  /* a move or decide that is not possible */
    MENU_SE_DECIDE    = 5,                  /* decide, with the button's decide effect
                                               (PageManager::playDecideEffect, the arcade pages) */
    MENU_SE_OIL       = 6,                  /* (? name) the oil drains / is refilled (u008) */
    MENU_SE_CONFIRM   = 7,                  /* decide without an effect (playDecideEffect on a page
                                               without one; the memory card, logger and race menus) */
    MENU_SE_VALUE     = 8,                  /* a slider's value changed */
    MENU_SE_CLOSE     = 9,                  /* a popup closed / the screen left without a back page
                                               (PageManager::back) */
    MENU_SE_LIST_MOVE = 10,                 /* a ButtonList's cursor moved, the main selection
                                               scrolled, a logger panel opened */
    MENU_SE_WHEEL_FIT = 0xC                 /* (? name) wheels changed (u013, after a delay) */
};

/* Plays menu sound effect se (MENU_SE_*; T469, volume 1.0) if the sound
 * effects option is on (GameStatus +0x2FF40). */
void menu_play_se(int se);                  /* 0x001C4F80 */

#endif /* GAME_T233_MENU_SOUND_H */
