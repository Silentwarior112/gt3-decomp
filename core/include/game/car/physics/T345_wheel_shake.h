/* T345_wheel_shake.h - the wheel shake (translation unit T345, unit u080),
 * 0x001E6A38..0x001E6C28.
 *
 * A per-wheel damped oscillation in -1..1 (DynWheel.shake / shakeVel, ? the
 * wheel's shake or judder): a spring of stiffness 100 and damping 5, driven
 * by DynCarState +0x4C4 (accelX) x 5 and by random kicks (F_00245D5C, G001):
 * with the wheel's velX, and on a surface whose tyre index is non-zero with
 * the spin.  Called by the conductor's car step (F_001F39D8, T317). */
#ifndef GAME_T345_WHEEL_SHAKE_H
#define GAME_T345_WHEEL_SHAKE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

void car_wheel_shake(RaceCarState *car);    /* 0x001E6A38 */

#endif /* GAME_T345_WHEEL_SHAKE_H */
