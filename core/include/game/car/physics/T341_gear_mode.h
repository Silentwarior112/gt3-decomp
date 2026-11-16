/* T341_gear_mode.h - the control mode requests (translation unit T341, unit
 * u079), 0x001E3BF0..0x001E3EA0.
 *
 * Each sets DynCarState.ctrlMode (CTRL_*, T288: who drives the car; u079
 * gearMode, u086 mode, u083 pitState) after moving the computer driver
 * (AIDriver, T361; u079 Transmission) to the mode's driving line
 * (ai_change_line 0x001F8980, T367; lines 3, 4, 6, 7; "now": without the
 * blend): a refusal leaves the mode and returns 0; 1 when it is set.  The
 * names are u079's, by the mode each sets (first pass F_<address>; u083
 * proposed ctrl_*); car_mode_stop_roll (0x001E3DF0, mode 5 by line 6) is used
 * inside u079 only.  The file name ("gear_mode") is the first pass's
 * misreading of the byte as a gear mode. */
#ifndef GAME_T341_GEAR_MODE_H
#define GAME_T341_GEAR_MODE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

int car_mode_driver(RaceCarDynamics *car);     /* 0x001E3BF0 CTRL_DRIVER (line 7 for the pad driver, else 6 out of modes 2..4); recover = 0 */
int car_mode_computer(RaceCarDynamics *car);   /* 0x001E3C78 CTRL_COMPUTER (line 6 out of modes 2..4, or out of 0 for the pad driver) */
int car_mode_pit_in(RaceCarDynamics *car);     /* 0x001E3D08 CTRL_PIT_IN (line 3, now), flags |= DYN_FLAG_PIT_CALL */
int car_mode_pit_box(RaceCarDynamics *car);    /* 0x001E3D60 CTRL_PIT_BOX (line 4, now) */
int car_mode_pit_out(RaceCarDynamics *car);    /* 0x001E3DA8 CTRL_PIT_OUT (line 3, now) */
int car_mode_stop_brake(RaceCarDynamics *car); /* 0x001E3E38 CTRL_STOP_BRAKE (line 6) */
int car_mode_stop(RaceCarDynamics *car);       /* 0x001E3E80 CTRL_STOP, always */
int car_mode_still(RaceCarDynamics *car);      /* 0x001E3E90 CTRL_STILL, always */

#endif /* GAME_T341_GEAR_MODE_H */
