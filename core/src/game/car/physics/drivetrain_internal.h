/* drivetrain_internal.h - unit u080 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001E4E78..0x001E6C28, second pass.
 *
 * Translation units (link order):
 *   T343  the gearbox: the gear of a speed, the automatic / semi-automatic /
 *         manual / sequential shift logic, the gear request of a frame
 *         (include/game/car/physics/T343_gear_select.h)
 *   T344  the drivetrain: the wheels' torques, the active yaw control
 *         (diffType 6 / 7), the clutch and the engine torque, the torque split
 *         between the axles by GearCalc.driveType, the axle differentials
 *         (T344_drivetrain.h)
 *   T295  a per-wheel heat in 0..1 (? the tyre's or the brake's) and its
 *         constants (T295_tyre_heat.h)
 *   T345  a per-wheel damped random oscillation (? the wheel's shake)
 *         (T345_wheel_shake.h)
 *
 * Every function works on a RaceCarState (T302): its parameter block
 * `params' (CarDynamics, T279: the engine, the drivetrain block GearCalc at
 * +0x1F8, the tyres), the frame time `dt' and the run state `st'
 * (DynCarState = CarRunState, T288 / T366: the wheels DynWheel = CarWheelState,
 * the tyre force terms TyreForce, the clutch, the axle speeds).  Every class
 * is defined in its canonical header; this header adds the outside headers
 * and what only this unit uses: the local functions of T343 / T344 and the
 * constants of the drivetrain code.
 */
#ifndef U080_H
#define U080_H

#include "types.h"

/* own translation units */
#include "game/car/physics/T343_gear_select.h"
#include "game/car/physics/T344_drivetrain.h"
#include "game/car/physics/T295_tyre_heat.h"
#include "game/car/physics/T345_wheel_shake.h"

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* vu0_rand */
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics (engine, gear, tyreIndex, brakeTorque) */
#include "game/car/physics/T284_car_param.h"            /* CarDynamics_getTyre */
#include "game/sound/T288_car_sound.h"            /* DynCarState, TyreForce */
#include "game/car/physics/T292_gear_calc.h"            /* GearCalc (u080's Drivetrain) */
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData (TyreGrip) */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (params, dt, st, wheelBrakeHeatPrev) */
#include "game/car/physics/T325_misc.h"                 /* g_002D28B0 (DynParams, T283) */
#include "game/car/physics/T335_pad_vibration.h"        /* DynWheel */
#include "game/car/physics/T338_engine_output.h"        /* engine_update */
#include "game/car/physics/T339_misc.h"                 /* g_002D4628 (InputTuning: u080's ShiftParams) */
#include "game/car/physics/T356_brake_assist.h"         /* CarWheelState (DynWheel) */
#include "game/race/ai/T366_ai_drive.h"             /* CarRunState (DynCarState) */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* "no gear change", the result of the shift logic */
#define GEAR_KEEP           (-1)

/* DynCarState.clutchState: CLUTCH_* (T288); GearCalc.driveType / diffType:
 * DRIVE_* / DIFF_* (T292); DynWheel.flags: WHEEL_* (T335). */

/* The drivetrain code's marker for "locked" (a coupling that holds both
 * axles together), compared with == everywhere. */
#define DRIVE_LOCKED        98000.0f

#define KMH_TO_MS           0.2777778f      /* 1 km/h in m/s */
#define RPM_TO_RADS         0.10471976f     /* 1 rpm in rad/s (2 pi / 60) */

/* ======================================================================== */
/* This unit (the functions the canonical headers leave out: used only here) */
/* ======================================================================== */
/* T343 */
void car_update_gear_offset(CarRunState *run);                       /* 0x001E4F10 */
int  car_shift_auto(RaceCarState *car, GearInput *in, int bySpeed);  /* 0x001E4FF8 */
int  car_shift_semi_auto(RaceCarState *car, GearInput *in);          /* 0x001E5458 */
int  car_shift_manual(RaceCarState *car, GearInput *in);             /* 0x001E5540 */
int  car_shift_sequential(RaceCarState *car, GearInput *in);         /* 0x001E5600 */
/* T344 */
f32  car_centre_coupling(RaceCarState *car, f32 drive, f32 frontWheelTq, f32 rearWheelTq); /* 0x001E5998 */
f32  clutch_slip_factor(f32 slip);                                   /* 0x001E5B30 */

#endif /* U080_H */
