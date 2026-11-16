/* T538_fmod_wrap.h - a float modulo helper (translation unit T538, unit u126; one function). */
#ifndef GAME_T538_FMOD_WRAP_H
#define GAME_T538_FMOD_WRAP_H

#include "game/game.h"

/* x - (f32)(s32)(x / y) * y (truncating); if wrap != 0 and x (not the
 * remainder) is negative, y is added, so a negative exact multiple of y gives
 * y, not 0.  C++ linkage (no symbol survives).  Callers (all with wrap = 1):
 * gt2k_menu::Cross (T255, screen-space wrap of the cross-hair, y = 680 / 488),
 * MenuCarCamera::virtual_8 (T411, y = 360: degrees). */
f32 fmod_wrap(int wrap, f32 x, f32 y);          /* 0x0026E2C8 */

#endif /* GAME_T538_FMOD_WRAP_H */
