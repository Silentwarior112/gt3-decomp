/* T348_wheel_speed_factor.h - the wheel speed factors (translation unit
 * T348, unit u081), 0x001E9E18..0x001E9E80.
 *
 * DynWheel.speedFactor = min(|the wheel's velocity| x 0.02, 1), 1 from 50
 * m/s: it scales the surface bumps (car_ground_contact, T298).  Not T300's
 * car_wheel_speed_factors (0x001EA3D8, RaceCarState *), which sets
 * lowSpeedFactor and velAngle. */
#ifndef GAME_T348_WHEEL_SPEED_FACTOR_H
#define GAME_T348_WHEEL_SPEED_FACTOR_H

#include "game/game.h"
#include "game/car/physics/T296_car_drivetrain.h"       /* CarDynState (a typedef of DynCarState) */

class DynCarState;                          /* T288_car_sound.h */

void car_wheel_speed_factors(CarDynState *st);     /* 0x001E9E18 */

#endif /* GAME_T348_WHEEL_SPEED_FACTOR_H */
