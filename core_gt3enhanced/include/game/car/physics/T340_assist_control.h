/* T340_assist_control.h - the slip estimate and the driving aid dispatch
 * (translation unit T340, unit u079), 0x001E3708..0x001E3BF0.
 *
 * car_assists (from the conductor's car step, u085) first estimates the
 * under- / oversteer of the frame (car_slip_estimate: DynCarState
 * understeer / oversteer), then runs the traction control (T357) and the
 * brake assist (T356) with the car's own aid block (CarDynamics.tcs) for the
 * pad driver in control mode 0 (or with pitTimer set), else with the
 * computer driver's copy (AIDriver.params.assist, +0x30, T361 / T362; u079
 * Transmission tcs2); it sets DynCarState
 * tractionAssist / brakeAssist. */
#ifndef GAME_T340_ASSIST_CONTROL_H
#define GAME_T340_ASSIST_CONTROL_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

void car_assists(RaceCarDynamics *car);     /* 0x001E3B18 */

#endif /* GAME_T340_ASSIST_CONTROL_H */
