/* T362_ai_params.h - the computer driver's parameters (translation unit
 * T362, unit u086), 0x001F5FD8..0x001F6578.
 *
 * ai_load_params turns the 0x20-byte CourseDriveParam record (T361) of the
 * car's drive class and the course kind into AIParams (AIDriver +0xC; km/h
 * are kept in m/s, x 0.2777778), including a second set of driving aids
 * (TcsParams, the block of CarDynamics +0xE7C).  ai_apply_rules then
 * scales it by the race rules' per-drive-class handicaps (RaceRuleOptions
 * enemySkillA / B / C / C2 [driveClass], T326: throttle %, brake %, corner
 * speed %, corner speed km/h), and spreads the driver 2 cars of a
 * conductor in mode 2 (DynamicsConductor.mode, +0xB7AC) by their index.
 * The enhanced engine (race_logic.md A8, A9): the spread in every race
 * (ENH_AI_DRIVER_SPREAD), C2 the physics' grip handicap instead of km/h
 * (ENH_AI_SKILL_AS_GRIP), Lupo 2's cool-down (ENH_AI_COOL_DOWN).
 * Callers: the car set-up (DynCar_initialize, T330) and car_finish (T302). */
#ifndef GAME_T362_AI_PARAMS_H
#define GAME_T362_AI_PARAMS_H

#include "game/game.h"
#include "game/car/physics/T356_brake_assist.h"         /* TcsParams (held by value) */

struct AIDriver;                            /* T361_drive_params.h (pointers only) */
class CarDynamics;                          /* T279_meter_scale.h */

/* The driver's parameters, 0x74 bytes at AIDriver +0xC, from the record r
 * (CourseDriveParam, T361). */
struct AIParams {
    f32       brakeScale;                   /* 0x00 100 / r.brake: < 1 brakes later, > 1 keeps a margin */
    f32       cornerScale;                  /* 0x04 r.cornerScale / 100 */
    f32       cornerOffset;                 /* 0x08 r.cornerOffset - 100 km/h */
    f32       steerGain;                    /* 0x0C r.steerGain x 5 / 100: the yaw-rate correction */
    f32       slideThrottle;                /* 0x10 r.slideThrottle / 100: the throttle cut beyond the steering limit */
    f32       slideBrake;                   /* 0x14 r.slideBrake / 100: the brake added beyond it */
    f32       lookInvSpeed;                 /* 0x18 1 / (r.lookSpeed x 2 km/h) */
    f32       lookNear;                     /* 0x1C r.lookNear m: the look-ahead at rest */
    f32       lookFar;                      /* 0x20 r.lookFar m: the look-ahead from r.lookSpeed x 2 km/h on */
    TcsParams assist;                       /* 0x24 the driving aids (r.brakeGain .. r.slipTarget) */
    f32       gripBalance;                  /* 0x58 r.gripBalance / 100: the front tyres' share */
    f32       cornerBonus;                  /* 0x5C r.cornerBonus km/h, after bonusDelay frames in a corner */
    f32       steerRate;                    /* 0x60 r.steerRate x 10 deg/s */
    f32       pitSpeed;                     /* 0x64 r.pitSpeed km/h (0: 80), the pit lane speed */
    s32       bonusDelay;                   /* 0x68 r.bonusDelay x 10 frames */
    f32       throttleScale;                /* 0x6C 1; the rules' enemySkillA, ai_reduce_pace 0.5 */
    f32       downforce;                    /* 0x70 (liftCoef[0] + liftCoef[1]) / weight (ai_set_downforce) */
};
EGCS_SIZE(AIParams, 0x74);

/* The parameters of record (drive class idx, course kind mode): T361's
 * ai_drive_param.  idx: CarDynamics.driveClass, mode:
 * DynamicsConductor::getCourseKind. */
void ai_load_params(AIDriver *ai, int idx, int mode);                           /* 0x001F5FD8 */
/* The race rules' handicaps of drive class idx (see above). */
void ai_apply_rules(AIDriver *ai, int idx);                                     /* 0x001F6380 */
/* Half throttle and 80% corner speeds (car_finish); the enhanced engine:
 * throttle x0.5 (at least 0.4), corners x0.5, braking scale x0.8. */
void ai_reduce_pace(AIDriver *ai);                                              /* 0x001F6518 */
/* Empty (T330 calls it instead of ai_apply_rules for a mode 0 car). */
void F_001F6540(AIDriver *ai);                                                  /* 0x001F6540 */
/* Empty (car_finish). */
void F_001F6548(AIDriver *ai);                                                  /* 0x001F6548 */
/* params.downforce: the car's lift coefficients over its weight (CarDynamics +0xE64, +0xE50). */
void ai_set_downforce(AIDriver *ai, CarDynamics *dyn);                          /* 0x001F6550 */

#endif /* GAME_T362_AI_PARAMS_H */
