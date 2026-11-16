/* T343_gear_select.h - the gearbox (translation unit T343, unit u080),
 * 0x001E4E78..0x001E5768.
 *
 * The gear a speed calls for, and the shift logic of each transmission
 * mode (DynCarState.transMode, u080; u079 / u081 pedalMode: automatic,
 * semi-automatic, manual, sequential; the shift parameters of the input
 * tuning bytes 0x002D4628 +0x14..+0x17).  car_select_gear returns the gear
 * to shift to, or -1 for no change; car_gear_select (T296) applies it with
 * car_set_gear.  Gear 0 is the reverse, 1..GearCalc.gearCount forward.
 *
 * GearInput is u080's name for the first two bytes of CarCtrlOut (T342,
 * DynCarState +0x380: reverse, shift; read with lb), which car_gear_select
 * passes: a typedef here.  CarRunState is u080's name for the dynamic state
 * (T366; the same 0x538 bytes as DynCarState, T288): T366 is included, not
 * forward-declared, in case it is a typedef there. */
#ifndef GAME_T343_GEAR_SELECT_H
#define GAME_T343_GEAR_SELECT_H

#include "game/game.h"
#include "game/car/physics/T342_car_input.h"            /* CarCtrlOut (GearInput) */
#include "game/race/ai/T366_ai_drive.h"             /* CarRunState (pointers only) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */

typedef CarCtrlOut GearInput;               /* u080 */

void car_set_gear(CarRunState *run, int gear);                 /* 0x001E4E78 run->gear = gear */
/* The lowest forward gear whose speed at revLimit - 500 rpm is above
 * `speed' (m/s): the top gear if none, 0 (the reverse) for a negative one. */
int  car_gear_for_speed(RaceCarState *car, f32 speed);          /* 0x001E4E80 */
/* While pitTimer runs: 1 (-1 in 1st already).  Else by transMode (0 for
 * driver 1 or a control mode): 0 / 2 automatic (car_shift_auto, bySpeed
 * 0 / 1), 1 semi-automatic, 3, 4, 6 manual, 5 sequential. */
int  car_select_gear(RaceCarState *car, GearInput *in);         /* 0x001E5690 */

#endif /* GAME_T343_GEAR_SELECT_H */
