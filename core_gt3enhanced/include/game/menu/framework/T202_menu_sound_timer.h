/* T202_menu_sound_timer.h - a menu sound played after a delay (translation
 * unit T202, unit u062), 0x001B28D8..0x001B29A8.
 *
 * A gt2k_menu::Timer (T252, its constructor inline) hung under a menu Object
 * counts the frames down, then its callback (0x001B28D8, private to T202)
 * plays the sound through menu_play_se (T233). */
#ifndef GAME_T202_MENU_SOUND_TIMER_H
#define GAME_T202_MENU_SOUND_TIMER_H

#include "game/game.h"

namespace gt2k_menu {
class Object;                                   /* T220 (pointers only) */
}

/* Plays menu sound se after frames frames: operator new(0x2C) Timer(callback,
 * se, frames), attached under parent.  Callers: T037 (u013), se 0 / 0xC after
 * 30 frames. */
void menu_sound_after(gt2k_menu::Object *parent, int se, int frames);  /* 0x001B28F8 */

#endif /* GAME_T202_MENU_SOUND_TIMER_H */
