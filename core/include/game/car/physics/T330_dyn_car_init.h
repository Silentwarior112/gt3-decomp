/* T330_dyn_car_init.h - the set-up of one car of the DynamicsConductor
 * (translation unit T330, unit u077), 0x001DD0C8..0x001DDFF0.
 *
 * DynCar_initialize (from DynamicsConductor::initialize, slot 22:
 * "DynamicsConductor::initialize()") copies the car's condition, builds its
 * CarDynamics (CarDynamics_init, T284), places it on the course (ground
 * queries for the body and the four wheels), resets the dynamic state, the
 * start position / grid and the rolling start speed.
 *
 * Names: the plan's classes of this unit are other headers' classes.  u077's
 * DynCar is RaceCarState (T302: the same 0x16DC bytes, CarDynamics at +0x10,
 * DynCarState at +0xF84, the ground contacts at +0x14BC), its DynState is
 * DynCarState (T288, +0xF84) and its GroundQuery (0x34, filled by
 * F_0018FA80) is RunwayContact (T153).  They are typedefs here, with no
 * second definition; other headers must not forward-declare `class DynCar',
 * `class DynState' or `class GroundQuery'. */
#ifndef GAME_T330_DYN_CAR_INIT_H
#define GAME_T330_DYN_CAR_INIT_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class DynCarState;                          /* T288_car_sound.h */
struct RunwayContact;                       /* T153_race_course.h */
class CarCondition;                         /* T280_car_condition.h */
struct CarSpec;                             /* T482_car_equipments.h */

typedef RaceCarState  DynCar;               /* u077, u085 */
typedef DynCarState   DynState;             /* u077, u085 */
typedef RunwayContact GroundQuery;          /* u077, u081 */

/* st->driver = driver, st->ctrlMode = 0 (u077 DynState_setMode). */
void DynCarState_setDriver(DynCarState *st, int driver);    /* 0x001DD0C8 */
/* st->transMode = transMode for driver 0, 0 for drivers 1 and 2 (first
 * pass F_001DD0D8). */
void DynCarState_setTransMode(DynCarState *st, int transMode); /* 0x001DD0D8 */
/* Per wheel: DynWheel +0x5C / +0x5D / +0x94 / +0xA0 cleared, +0x58 = 1; with
 * the conductor's WearCurve active (x3 != 0): +0xA0 = x0 (the initial
 * wear), +0x5D = -0x3F if it is negative, +0x58 = 1 - y0. */
void DynCar_initTyreWear(RaceCarState *car);                /* 0x001DD110 (F_001DD110) */
void DynCarState_resetWheels(DynCarState *st);              /* 0x001DD1A8 (u077 DynState_resetWheels) */
/* Puts the car at (x, y) with heading yaw on runway sector `sector': the
 * ground height of the body and of the four wheels (their surface types),
 * the wheel loads, the pitch from the axle heights, lapDistance.  Returns 0. */
int  DynCar_place(RaceCarState *car, int sector, f32 x, f32 y, f32 z, f32 yaw); /* 0x001DD210 */
void DynCar_clearFaults(RaceCarState *car);                 /* 0x001DD5B8 courseOut = fault = 0 (F_001DD5B8) */
/* speed (+0x16A8) and +0x16D8 = v; the top speed reached (+0x16AC) follows. */
void DynCar_setSpeed(RaceCarState *car, f32 v);             /* 0x001DD5C8 */
void DynCar_resetState(RaceCarState *car);                  /* 0x001DD5F0 */
/* A rolling start at v (m/s): the velocity along the heading, the gear for
 * the speed, the engine speed (between idle and the redline), the turbo,
 * the wheel and axle speeds. */
void DynCar_setInitialSpeed(RaceCarState *car, f32 v);      /* 0x001DD8A8 */
/* startType: 1..5 the grid from racingLinePoint (falls back to 0), 6 / 7 the grid
 * of F_001F9E30, 0 and 8 move (x, y) back by lenFront along the heading; 4
 * takes the start speed from the +0xEC8 block (+0x90), the others speedKmh.
 * driver / transMode / noController go to the DynCarState; cond (may be
 * NULL: a cleared one) is copied to its condition.  Returns
 * CarDynamics_init's result. */
int  DynCar_initialize(RaceCarState *car, CarSpec *spec, int sector, int driver,
                       int transMode, int noController, int carIdx, int startType,
                       f32 x, f32 y, f32 z, f32 yaw, int startArg, int speedKmh,
                       CarCondition *cond);                 /* 0x001DDAD0 */

#endif /* GAME_T330_DYN_CAR_INIT_H */
