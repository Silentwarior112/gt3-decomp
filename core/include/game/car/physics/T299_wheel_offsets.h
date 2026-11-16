/* T299_wheel_offsets.h - the wheel positions relative to the body
 * (translation unit T299, unit u081), 0x001E9CE8..0x001E9E18.
 *
 * From the body matrix: wheel 0 at (+frontDist, -halfTrack), 1 at
 * (+frontDist, +halfTrack), 2 at (-rearDist, -halfTrack), 3 at (-rearDist,
 * +halfTrack), into DynWheel.offX / offY (car_wheel_planar_offsets) and
 * offZ (car_wheel_heights, 0x001E9CE8, inside u081). */
#ifndef GAME_T299_WHEEL_OFFSETS_H
#define GAME_T299_WHEEL_OFFSETS_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

void car_wheel_planar_offsets(RaceCarDynamics *car);   /* 0x001E9D48 offX / offY */
void car_wheel_offsets(RaceCarDynamics *car);          /* 0x001E9DE8 car_wheel_heights, then car_wheel_planar_offsets */

#endif /* GAME_T299_WHEEL_OFFSETS_H */
