/* T344_drivetrain.h - the drivetrain torques of a frame (translation unit
 * T344, unit u080), 0x001E5768..0x001E6878.
 *
 * car_drivetrain_step (T296) calls car_wheel_torques, car_active_yaw_control,
 * car_drive_torque and car_axle_differentials in this order: the wheels'
 * torques, the active yaw control of differential types 6 / 7, the clutch,
 * the engine torque (engine_update, T338) and the engine speed, the torque
 * split between the axles by GearCalc.driveType (a coupling, a centre
 * differential or both axles locked), the axle speeds, and the
 * differential of each axle (GearCalc.diffType: open, locked, LSD,
 * viscous).  98000 is the code's "locked" marker. */
#ifndef GAME_T344_DRIVETRAIN_H
#define GAME_T344_DRIVETRAIN_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* Each wheel's torque: -tyreRadius[axle] x (driveOut + brakeOut), less the
 * brake torque against the spin (TyreForce.torque). */
void car_wheel_torques(RaceCarState *car);          /* 0x001E5768 */
/* diffType 6 / 7 between 10 and 180 km/h: the steering moves torque between
 * the axle's wheels towards their mean speed +- 1/8 by the steering's side. */
void car_active_yaw_control(RaceCarState *car);     /* 0x001E57F8 */
void car_drive_torque(RaceCarState *car);           /* 0x001E5B98 */
void car_axle_differentials(RaceCarState *car);     /* 0x001E6488 */

/* ---- The enhanced engine (ref/enhanced/ported/physics.md) ------------- */

class GearCalc;                             /* T292_gear_calc.h */
class EngineSpecSheet;                      /* T291_engine.h */

/* Lupo 2's gearbox types (docs/gearbox_dualnote.md; GEARBOX_*, T292):
 * the CVT's rpm limiter (L2 0x001DF910: holds the engine at its target rpm,
 * GearCalc.shiftRpm[0], by the ratio factor; types other than 1 untouched),
 * the CVT / clutchless controller (L2 0x001DFA38: the engine torque through
 * RaceCarState.cvtRatio) and the electric single speed's motor law (L2
 * 0x001DFCA0).  From car_drive_torque, the top speed (T284) and the rolling
 * start (T330). */
void gearbox_cvt_limit(GearCalc *g, f32 *engineSpeed, f32 *factor);
f32  gearbox_cvt_torque(RaceCarState *car);
f32  gearbox_electric_torque(RaceCarState *car);

/* The Honda Dualnote (docs/gearbox_dualnote.md; RaceCarState.hybrid,
 * batteryCharge, batteryCapacity, batteryFlow; the front motors' load in
 * DynCarState.clutchTorqueAbs, the whine's input as in Lupo 2).  Lupo 2
 * L2 0x001DFFF0..0x001E1238. */
void hybrid_params_init();                          /* L2 0x001DFFF0: the 27 bytes -> the constants */
void hybrid_setup(RaceCarState *car);               /* L2 0x001E0300: flag, full battery (DynCar_initialize) */
void hybrid_flow_reset(RaceCarState *car);          /* L2 0x001E0348: each step (DynamicsConductor::update) */
void hybrid_front_motors(RaceCarState *car);        /* L2 0x001E0900 (car_active_yaw_control) */
f32  hybrid_free_rev_torque(RaceCarState *car);     /* L2 0x001E04B0: clutch open (car_engine_speed) */
f32  hybrid_battery_level(RaceCarState *car);       /* L2 0x001E02C0: charge / capacity (the HUD gauge) */
/* The spec sheet's power + the motors' 75 kW (L2 0x001E1238; menus_b.md 18,
 * rendering's ShowRoomCar and gear_chart_with_sheet, T292). */
void hybrid_power_bonus(EngineSpecSheet *sheet);

#endif /* GAME_T344_DRIVETRAIN_H */
