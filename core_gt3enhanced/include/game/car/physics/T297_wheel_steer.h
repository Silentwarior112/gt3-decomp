/* T297_wheel_steer.h - the steering angle of each wheel (translation unit
 * T297, unit u081), 0x001E8910..0x001E8A48.
 *
 * On the front axle the inner / outer wheel's share of the steering
 * (CarDynamics.steerGain), plus the axle's toe (negative on the even, left
 * wheels).  car_wheel_steer_angles puts the four angles into DynWheel.steer
 * and their sine / cosine into DynCarState.tyreForce[]. */
#ifndef GAME_T297_WHEEL_STEER_H
#define GAME_T297_WHEEL_STEER_H

#include "game/game.h"
#include "game/car/physics/T298_suspension_body.h"      /* CarParams (a typedef) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

/* `unused' is passed (0) but not read. */
f32  wheel_steer_angle(CarParams *p, int wheel, f32 steer, f32 unused);    /* 0x001E8910 */
void car_wheel_steer_angles(RaceCarDynamics *car);                         /* 0x001E8978 */

#endif /* GAME_T297_WHEEL_STEER_H */
