/* T361_drive_params.h - the course drive parameters and the computer driver
 * AIDriver (translation unit T361, unit u085), 0x001F5EF0..0x001F5FD8.
 *
 * T361 picks the 0x20-byte CourseDriveParam record of a car: the course
 * data's +0x94 table [drive class] or, when the course has none, the default
 * record of the course kind (g_002D9A08[3], u085).  The computer driver's
 * parameters are made from it (ai_load_params, T362); record 0 also gives
 * DynamicsConductor's start type 3 (courseStart: startArg, startSpeed).
 *
 * AIDriver is the object the planner called DynSub_EC8: RaceCarState +0xEC8,
 * 0xBC bytes, the computer driver of a car (u086's name; u077 / u085
 * DynSub_EC8, u079 / u082 Transmission; u083 sub_EC8).  Both other names are
 * typedefs of it here: T300's first Transmission had AIParams flattened, but
 * ai_look_distance takes ai + 0xC (an AIParams *) and ai_load_params builds
 * ai + 0xC / ai + 0x30, so AIParams is a sub-object.  AIDriver's code is
 * T362..T367 (u086): the parameters, the gear, the corner speeds, the
 * traffic, the driving, the set-up and the line changes.  Every function
 * takes the AIDriver and reaches the car through `car' (CarDynamics at +0x10,
 * the frame time at +0xEC0, the run state at +0xF84).  It follows one of the
 * course's enemy lines (CourseLines, T368), looks ahead along it, steers at
 * the look-ahead point, brakes for the next corner from the tyre grip, holds
 * the corner speeds, picks a gear and moves to another line around the cars
 * near it. */
#ifndef GAME_T361_DRIVE_PARAMS_H
#define GAME_T361_DRIVE_PARAMS_H

#include "game/game.h"
#include "game/race/ai/T362_ai_params.h"            /* AIParams (held by value) */

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */
struct CourseLines;                         /* T368_enemy_lines.h */

/* A record of the course data's +0x94 table / of g_002D9A08, 0x20 bytes: the
 * computer driver's parameters for a drive class on this course (each byte
 * goes to AIParams / its TcsParams as ai_load_params says). */
struct CourseDriveParam {
    u8 brake;                               /* 0x00 AIParams.brakeScale = 100 / it */
    u8 cornerScale;                         /* 0x01 % of the corner speed */
    u8 cornerOffset;                        /* 0x02 km/h + 100 added to the corner speed */
    u8 steerGain;                           /* 0x03 x 5 % */
    u8 slideThrottle;                       /* 0x04 % */
    u8 slideBrake;                          /* 0x05 % */
    u8 lookSpeed;                           /* 0x06 x 2 km/h: the speed of the far look-ahead */
    u8 lookNear;                            /* 0x07 m */
    u8 lookFar;                             /* 0x08 m */
    u8 brakeGain;                           /* 0x09 % (TcsParams) */
    u8 brakeSlip;                           /* 0x0A % */
    u8 fadeGain;                            /* 0x0B / 10 */
    u8 fadeStart;                           /* 0x0C % */
    u8 throttleGain;                        /* 0x0D / 10 */
    u8 gripBalance;                         /* 0x0E % the front tyres' share */
    u8 cornerBonus;                         /* 0x0F km/h */
    u8 steerRate;                           /* 0x10 x 10 deg/s */
    u8 bonusDelay;                          /* 0x11 x 10 frames */
    u8 pitSpeed;                            /* 0x12 km/h (0: 80) */
    u8 assistFlags;                         /* 0x13 bits 0..7: brakeFront, brakeRear, absFront, absRear, */
                                            /* brakeFront2, brakeRear2, absFront2, absRear2 */
    u8 absGain;                             /* 0x14 % */
    u8 brakeGain2;                          /* 0x15 % */
    u8 brakeSlip2;                          /* 0x16 % */
    u8 absGain2;                            /* 0x17 % */
    u8 slipTarget;                          /* 0x18 % (0 or 100: 1.0) */
    u8 startArg;                            /* 0x19 x -10: the start type 3 argument (record 0) */
    u8 startSpeed;                          /* 0x1A x 5 km/h: the start type 3 speed (record 0) */
    u8 unk_1B[5];                           /* 0x1B */
};
EGCS_SIZE(CourseDriveParam, 0x20);

/* The computer driver of a car, RaceCarState +0xEC8, 0xBC bytes.  Set up by
 * ai_init / ai_construct (T367); u051 shows gear / gearSet. */
struct AIDriver {
    DynamicsConductor *conductor;           /* 0x00 */
    RaceCarState      *car;                 /* 0x04 */
    CourseLines       *lines;               /* 0x08 conductor + 0xBBA8 (u086: course) */
    AIParams           params;              /* 0x0C (u079: params.assist at +0x30 is "tcs2") */
    u8                 recover;             /* 0x80 0 on the line, 1 backing up, 2 turning round forward (u086) */
                                            /* 1: reverse while the heading error is above 10 deg (10..60 when
                                               already reversing), 10 m look-ahead, left on a rear hit; 2: 50
                                               km/h, full lock beyond 80 deg, 40 m, left on a front hit beyond
                                               60 deg */
    u8                 line;                /* 0x81 the enemy line followed */
    u8                 corner;              /* 0x82 the next node (corner) on it */
    u8                 state;               /* 0x83 0 full throttle, 1 / 2 in a corner, 3 / 4 braking */
    u8                 nextKind;            /* 0x84 the next corner's kind (LineNode.kind) */
    u8                 inPitLane;           /* 0x85 1 on line 4, and on line 3 in the pit speed zone */
    u8                 prevLine;            /* 0x86 the line before a change (blended out) */
    u8                 unk_87;              /* 0x87 */
    u16                holdCount;           /* 0x88 frames in the corner (at most 6000) */
    u8                 unk_8A[2];           /* 0x8A */
    f32                segSpeed;            /* 0x8C the speed of the corner the car is in */
    f32                nextSpeed;           /* 0x90 the speed of the next corner (T330 start type 4: the start speed) */
    f32                lineBlend;           /* 0x94 1 -> 0 after a line change */
    s8                 speedStep;           /* 0x98 -2..2: the corner speeds + it x 10 km/h */
    s8                 unk_99;              /* 0x99 -2..2 (ai_step_unk99) */
    s16                lineTimer;           /* 0x9A frames before the next traffic line change */
    f32                steer;               /* 0x9C rad */
    f32                throttle;            /* 0xA0 0..1 */
    f32                brake;               /* 0xA4 0..1 */
    s16                reverse;             /* 0xA8 -> the run state's ctrl.reverse */
    s16                shiftTimer;          /* 0xAA -> the run state's shiftTimer when non-zero (12: part throttle) */
    s8                 gear;                /* 0xAC the gear wanted (-1: none) */
    s8                 gearSet;             /* 0xAD 1 while braking sets the gear (ai_update_gear; lb, u051) */
    u8                 unk_AE;              /* 0xAE */
    s8                 brakeState;          /* 0xAF 0, 1 within margin + 50 m of the braking point, 2 braking, */
                                            /* 3 full throttle in a kind 2 corner (no change to the - side
                                               line), 4 in a kind 1 corner (not to the + side), 5 part throttle
                                               (no change) (u086) */
    f32                caution;             /* 0xB0 0..1 the closest car's threat (T365) */
    s8                 traffic[8];          /* 0xB4 the cars around: 2 in the zone, 1 to that side; zones */
                                            /* 0 ahead, 1 ahead -, 2 ahead +, 3 beside -, 4 beside +, */
                                            /* 5 behind -, 6 behind +, 7 behind */
};
EGCS_SIZE(AIDriver, 0xBC);

typedef AIDriver DynSub_EC8;                /* u077, u085 (the plan's name) */
typedef AIDriver Transmission;              /* u079, u082 (the plan's T300 name; not a gearbox) */

/* The drive class of a car from its spec's drivetrain (CarSpec layout,
 * awdType): 1 FF, and 4WD with awdType 0; 3 4WD with awdType 3; 2 the other
 * 4WD; 0 the rest (CarDynamics.driveClass, T284). */
int  drive_class(int layout, int awdType);                                      /* 0x001F5EF0 (F_001F5EF0) */
/* The course's record [idx] (idx >= 4: 0), else g_002D9A08[kind 1 / 2, any
 * other kind 0]. */
CourseDriveParam *getDriveParam(DynamicsConductor *dc, int kind, u32 idx);      /* 0x001F5F38 */
/* getDriveParam(ai->conductor, kind, idx) (ai_load_params). */
CourseDriveParam *ai_drive_param(AIDriver *ai, int kind, u32 idx);              /* 0x001F5FB8 (F_001F5FB8) */

#endif /* GAME_T361_DRIVE_PARAMS_H */
