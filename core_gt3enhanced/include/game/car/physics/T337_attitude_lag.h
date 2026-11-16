/* T337_attitude_lag.h - the lagged attitude (translation unit T337, unit
 * u078), 0x001E0740..0x001E0888.
 *
 * A lagged copy of the car's yaw (a spring-damper, 0x00352958 / 0x00352960)
 * and pitch (spring 20, damper 10), per 1/60 s, in the dynamics state's
 * lagYaw / lagPitch (+0x4F8..+0x510), and the differences to the car's
 * angles (lagYawDiff / lagPitchDiff, read by car_lag_yaw / car_lag_pitch,
 * T302).  Both take the dynamics state
 * (car +0xF84): DynCarState (T288) as u078 calls it, the record
 * CarDynState (T296) of the other dynamics units. */
#ifndef GAME_T337_ATTITUDE_LAG_H
#define GAME_T337_ATTITUDE_LAG_H

#include "game/game.h"

class DynCarState;                          /* T288_car_sound.h (pointers only) */

void car_attitude_lag_reset(DynCarState *s);    /* 0x001E0740 lagYaw / lagPitch = yaw / pitch, the rest 0 */
void car_attitude_lag_update(DynCarState *s);   /* 0x001E0768 */

#endif /* GAME_T337_ATTITUDE_LAG_H */
