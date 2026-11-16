/* T248_menu_signal.h - the menu signal flag (translation unit T248, unit
 * u071), 0x001CBE30..0x001CBE80.
 *
 * One .sbss int (g_0035324C, gp-0x47A4): an operator script sets it through
 * menu_signal_set (the callback of the opCall at 0x00333B30, the third
 * instruction of T211's fade-out script g_00333AD0: u064), the PageManager
 * (T211 PageManager::fadeOut) polls and clears it, ControlBase::attach
 * clears it.  (The names are not the original ones.)
 * C++ functions (C++ linkage, no classes). */
#ifndef GAME_T248_MENU_SIGNAL_H
#define GAME_T248_MENU_SIGNAL_H

#include "game/game.h"

void menu_signal_clear(void);               /* 0x001CBE30 */
void menu_signal_nop(void);                 /* 0x001CBE38 empty (ControlBase::detach) */
void menu_signal_set(void);                 /* 0x001CBE40 (operator 0x1E's callback, data 0x00333B34) */
void menu_signal_reset(void);               /* 0x001CBE50 clears it (no callers) */
int  menu_signal_get(void);                 /* 0x001CBE58 (no callers) */
int  menu_signal_poll(void);                /* 0x001CBE60 0 (and cleared) once set, 1 while not */

#endif /* GAME_T248_MENU_SIGNAL_H */
