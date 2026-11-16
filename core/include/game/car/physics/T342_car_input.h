/* T342_car_input.h - CarInput, CarCtrlOut and the driver input processing
 * (translation unit T342, unit u079), 0x001E3EA0..0x001E4E78.
 *
 * input_tuning_init turns the input tuning bytes 0x002D4628 into the pedal
 * ramps, the steering speeds and springs, the minimum locks of the steering
 * assists and the wheel controller's throttle / brake curves (then
 * steer_force_init, T336).  car_input_update reads the driver's CarInput
 * once per frame (RaceInput::read, T413; the conductor's slot 19 sees it)
 * and runs the pad or the automatic path (T358) by DynCarState.driver: the
 * steering with a speed-dependent lock, the throttle / brake / handbrake
 * ramps or analog curves, and the CarCtrlOut of the frame.
 *
 * CarInput is also the frame of the control record (T318 / T353: u085 calls
 * it ControlFrame, whose clear() is CarInput::clear). */
#ifndef GAME_T342_CAR_INPUT_H
#define GAME_T342_CAR_INPUT_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

/* The output of the input processing (DynCarState +0x380 = RaceCarState
 * +0x1304; u080's GearInput is its first two bytes).  Not polymorphic.
 * Size 4 (axleTorque follows at DynCarState +0x384). */
class CarCtrlOut {
public:
    s8 reverse;                             /* 0x0 non-zero: shift to the reverse (the second throttle is held; lb) */
    s8 shift;                               /* 0x1 CarInput.shift: > 0 up, < 0 down (lb) */
    u8 throttleKey;                         /* 0x2 0, 1 the throttle key held, 2 both */
    u8 steerKind;                           /* 0x3 0 wheel, 1 digital, 2 analog */
};
EGCS_SIZE(CarCtrlOut, 4);

/* The driver's input of a frame (RaceInput::read fills it; 0x10 bytes on
 * the stack of car_input_update).  Not polymorphic.  Size 0x10 (u085's
 * ControlFrame, the record's stride). */
class CarInput {
public:
    void clear();                           /* 0x001F4330 (T353, u085 ControlFrame::clear) everything 0 */

    u8  flags;                              /* 0x0 analog: bit 0 steer, 1 throttle, 2 brake, 3 throttle2, 4 handbrake */
    u8  device;                             /* 0x1 the steering source: 0 digital, 2 analog stick, 3 pressure, 4 wheel */
    s8  shift;                              /* 0x2 +1 up, -1 down */
    u8  unk_3;                              /* 0x3 (not recorded by T318) */
    s16 steer;                              /* 0x4 -0x1000..0x1000 (positive: left) */
    s16 steerMag;                           /* 0x6 (pressure steer: the larger of the two buttons) */
    s16 throttle;                           /* 0x8 0..0x1000 (digital: 0 / 1) */
    s16 brake;                              /* 0xA */
    s16 throttle2;                          /* 0xC the second throttle: the reverse (u095 unkC) */
    s16 handbrake;                          /* 0xE (u095 unkE) */
};
EGCS_SIZE(CarInput, 0x10);

/* DynamicsConductor::virtual_20 (T317) passes its argument in $a0 (kept in
 * $s5 for it and WearCurve::setup); it is not read. */
void input_tuning_init(int unused);                             /* 0x001E3EA0 */
void car_set_steer_assist(RaceCarDynamics *car, int v);         /* 0x001E4468 DynCarState.steerAssist = v (int: DynCar_initialize passes it unextended) */
void car_input_update(RaceCarDynamics *car);                    /* 0x001E4D38 */

#endif /* GAME_T342_CAR_INPUT_H */
