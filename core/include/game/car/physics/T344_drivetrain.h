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

#endif /* GAME_T344_DRIVETRAIN_H */
