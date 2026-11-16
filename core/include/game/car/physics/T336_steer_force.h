/* T336_steer_force.h - the steering force (translation unit T336, unit
 * u078), 0x001DFF40..0x001E0668.
 *
 * The force-feedback wheel: steer_force_init (called by input_tuning_init,
 * T342) sets the gains (.sdata 0x00352934..0x0035294C) and the response
 * curve 0x0049D050 from the input tuning bytes 0x002D4628 (+0x34..+0x3E).
 * steer_force_update (per car, from car_step, T302) sums the front wheels'
 * lateral forces / sprungWeight[0] through the curve, the steering rate, the
 * clamped steering angle term (DynCarState.steer; InputTuning's
 * steerAngleGain / steerAngleLimit) and the clamped front slip angle terms,
 * filters it and adds the
 * front tyres' lateral term (the ground normal along the wheel's lateral
 * axis x the load); below 2 km/h only the steering angle (at most 5
 * degrees) / 10 degrees.  Clamped to +-1, x 127, it goes to RaceCarState's
 * PadVibration.steer as 0x80 - force.  pad_vibration_send hands the
 * vibration block to the pad (RaceInput::sendVibration, T413).  The gains
 * (.sdata 0x00352934..0x0035294C) and the curve (ResponseCurve 0x0049D050)
 * are T336's own data. */
#ifndef GAME_T336_STEER_FORCE_H
#define GAME_T336_STEER_FORCE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

void steer_force_init(void);                    /* 0x001DFF40 */
void steer_force_update(RaceCarState *car);     /* 0x001E0068 */
void pad_vibration_send(RaceCarState *car);     /* 0x001E0618 not when the driver is 1 / 2 or ctrlMode != 0 */

#endif /* GAME_T336_STEER_FORCE_H */
