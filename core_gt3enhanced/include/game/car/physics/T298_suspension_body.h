/* T298_suspension_body.h - the ground contact, the suspension and the body
 * motion of a car (translation unit T298, unit u081), 0x001E8AE0..0x001E9CE8.
 *
 * The body step of a car (car_step, T302) calls, in this order:
 * car_ground_contact (the 4 ground queries under the wheels: the ground
 * height, the surface and its flags, the surface bumps; the runway sector),
 * car_susp_prepare (the ground's cosine to the body, the wheel gaps, the
 * ground bits), car_suspension_forces (springs, bump stops, dampers, the
 * anti-roll bars, the camber and the track change, the vertical force of
 * each corner with the load transfer), car_body_motion (pitch / roll /
 * heave, the body matrix, gravity) and car_ground_penetration (lifts and
 * turns the body out of the ground).  wheel_camber / wheel_lateral_pos
 * evaluate the suspension geometry for a steer and a stroke (also the
 * set-up code of u076 / u077).  The pit stop jack is here too
 * (car_pit_jack, inside u081: it lifts the car 0.3 m by the pit timer and
 * turns it to the pit box's heading; car_body_motion is skipped meanwhile).
 *
 * CarParams is u079 / u081 / u082's name for CarDynamics (T279), the car's
 * physics parameter block at RaceCarState +0x10 (u081's names are merged
 * there).  u081 / u082 ended it at +0xE7C, before the driving aids; but
 * CarDynamics_init (T284) writes the aids through the same pointer at
 * +0xE7C and ShowRoomCar (T278) holds the block as 0xEB0 bytes, so it is
 * 0xEB0 with them (CarDynamics.tcs).
 * It is a typedef here, with no second definition: other headers must not
 * forward-declare `class CarParams'. */
#ifndef GAME_T298_SUSPENSION_BODY_H
#define GAME_T298_SUSPENSION_BODY_H

#include "game/game.h"
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics (CarParams) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

typedef CarDynamics CarParams;              /* u079, u081, u082 */
EGCS_SIZE(CarParams, 0xEB0);

void car_ground_contact(RaceCarDynamics *car);                          /* 0x001E8B68 */
void car_susp_prepare(RaceCarDynamics *car);                            /* 0x001E91A8 */
/* The axle's camber + camberGain x stroke; on the front axle the steering
 * adds casterCamber x steer x the wheel's steering share. */
f32  wheel_camber(CarParams *p, int wheel, f32 steer, f32 stroke);      /* 0x001E9218 */
/* The y of the wheel's contact point: +-halfTrack (even wheels negative),
 * less (1 - cos(atan2(stroke, swingArm))) x swingArm; steer is not read. */
f32  wheel_lateral_pos(CarParams *p, int wheel, f32 steer, f32 stroke); /* 0x001E92A8 */
void car_suspension_forces(RaceCarDynamics *car);                       /* 0x001E9348 */
void car_body_motion(RaceCarDynamics *car);                             /* 0x001E96F0 */
void car_ground_penetration(RaceCarDynamics *car);                      /* 0x001E9998 */

/* The enhanced engine: the SLs' active suspension (CAR_KIND_SL, physics.md
 * 26): its flag (1 as Lupo 2 ships; CarDynamics_init) and its table, built
 * at the race's set-up (DynamicsConductor::virtual_20; Lupo 2 L2 0x001E2140,
 * 0x001E2150). */
int  sl_active_suspension_on();
void sl_active_suspension_init();

#endif /* GAME_T298_SUSPENSION_BODY_H */
