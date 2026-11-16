/* T300_car_tyre_grip.h - the tyre wear and grip of a car (translation unit
 * T300, unit u082), 0x001E9E80..0x001EA4D8.
 *
 * car_pre_step (the first part of a car's frame, from F_001F39D8, T317):
 * held, the pitTimer countdown, the frame time (car_time_scale, T304),
 * the engine values of the frame (engine_torque_at / engine_scale, T291),
 * then car_wheel_grip - each wheel's grip from the tyre's load curve (over
 * tyre_load_per_width), the wear (car_wear_grip 0x001E9E80: the race's
 * WearCurve to wearGrip / wearLevel), the axle grip, the camber curve and
 * the surface - and car_wheel_low_speed_factors (0x001EA3D8: lowSpeedFactor,
 * velAngle).
 *
 * The plan's Transmission (u079 / u082) is not a gearbox and not defined
 * here: it is the computer driver at RaceCarState +0xEC8, AIDriver (T361,
 * `typedef AIDriver Transmission' there).  Its flat fields +0x0C..+0x7F are
 * AIDriver's params.<same name> (AIParams, T362: ai_look_distance takes
 * ai + 0xC); course, unk85, unkAA, unk99 are lines, inPitLane, shiftTimer,
 * unk_99.
 *
 * Names: 0x001E9E80 / 0x001EA348 / 0x001EA3D8 were car_tyre_wear /
 * wheel_speed_factor / car_wheel_speed_factors in the first pass; the first
 * and the last are also the names of T296's 0x001E8190 and T348's
 * 0x001E9E18, so they are car_wear_grip / wheel_low_speed_factor /
 * car_wheel_low_speed_factors here (after DynWheel.lowSpeedFactor). */
#ifndef GAME_T300_CAR_TYRE_GRIP_H
#define GAME_T300_CAR_TYRE_GRIP_H

#include "game/game.h"
#include "game/car/physics/T298_suspension_body.h"      /* CarParams (a typedef) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class DynCarState;                          /* T288_car_sound.h */
class WearCurve;                            /* T327_wear_curve.h */

/* The wear of each wheel through the race's curve (DynamicsConductor::wear):
 * DynWheel.wearGrip (at least the tyre's surfaceGrip[11]) and wearLevel.
 * 0 (nothing done) when the curve's x3 is 0 or the car has DynCarState.flags
 * bit 4; else 1. */
int  car_wear_grip(DynCarState *st, CarParams *p, WearCurve *wc);   /* 0x001E9E80 (u082 car_tyre_wear) */
f32  tyre_load_per_width(CarParams *p, int wheel, f32 load);    /* 0x001EA1C0 load / gear.tyreWidth[wheel >> 1] */
/* DynWheel.grip = load x loadCurve(load / width) [x wearGrip] x axleGrip x
 * surfaceGrip[surface] x camberCurve(+-(roll - groundRoll) + camber). */
void car_wheel_grip(RaceCarState *car);                         /* 0x001EA1E8 */
/* *factor = 0 below 1 km/h, 1 from 10 km/h, 0.4 per m/s between; returns
 * atan2f(-vy, vx), 0 when the factor is 0. */
f32  wheel_low_speed_factor(f32 *factor, f32 vx, f32 vy);       /* 0x001EA348 (u082 wheel_speed_factor) */
/* lowSpeedFactor and velAngle of the four wheels. */
void car_wheel_low_speed_factors(RaceCarState *car);            /* 0x001EA3D8 (u082 car_wheel_speed_factors) */
void car_pre_step(RaceCarState *car);                           /* 0x001EA428 */

#endif /* GAME_T300_CAR_TYRE_GRIP_H */
