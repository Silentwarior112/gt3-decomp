/* T301_car_motion.h - the car's ground attitude, velocities and speed
 * (translation unit T301, unit u082), 0x001EA4D8..0x001EAA88.
 *
 * car_ground_attitude derives a ground position / pitch / roll from the
 * four wheel heights (DynCarState groundPos / groundRot, groundPitch /
 * groundRoll); the wheel velocities (the yaw rate x each wheel's offset),
 * the local speed on the ground frame, the tread and axle speeds and the
 * speed part of the frame (the trip meter, the condition, the top speed,
 * the smoothed display speed) are used inside u082.  car_set_condition is
 * called by DynCar_initialize (T330), car_trip_display by the race display
 * (u051). */
#ifndef GAME_T301_CAR_MOTION_H
#define GAME_T301_CAR_MOTION_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class CarCondition;                         /* T280_car_condition.h */

void car_ground_attitude(RaceCarState *car);                            /* 0x001EA4D8 (also from DynCar_place, T330) */
/* DynWheel.velX / velY = speedX / speedY + yawRate x the wheel's offset
 * (halfTrack[], frontDist / rearDist). */
void car_wheel_velocities(RaceCarState *car);                           /* 0x001EA5F8 */
/* speedX / speedY: vel along groundRot[0] / [1], then the wheels'. */
void car_local_speed(RaceCarState *car);                                /* 0x001EA6B0 */
/* The tread speed (spin) of every wheel with no DynWheel.flags bit; an
 * axle's speed follows its wheels when one of them was set or its
 * gear.diffType is 0. */
void car_wheel_spin(RaceCarState *car);                                 /* 0x001EA720 */
void car_speed_view(RaceCarState *car);                                 /* 0x001EA870 speedView follows speed at 400 km/h a second */
void car_set_condition(RaceCarState *car, const CarCondition *c);       /* 0x001EA8C0 DynCarState.condition = *c, tripStep = 0 */
/* This frame's distance to the condition (addOdometer keeps the rest in
 * tripStep) and to the trip meter (modulo 8000 km); not when speed < 0. */
void car_trip(RaceCarState *car);                                       /* 0x001EA908 */
f32  car_trip_display(RaceCarState *car);                               /* 0x001EA988 the trip in the speed unit (x 0.625: miles) */
/* The speed part of car_step: local speed, speed, trip, the condition's
 * mileages, topSpeed (until finished), speedView, the wheel spin. */
void car_speed_step(RaceCarState *car);                                 /* 0x001EA9B8 */

#endif /* GAME_T301_CAR_MOTION_H */
