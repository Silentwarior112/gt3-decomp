/* T362 (0x001F5FD8..0x001F6578): the computer driver's parameters.
 * Callers: DynCar_initialize (T330) and car_finish (T302). */
/* Retail T362's .rodata (0x0033DC70): <new>'s "bad_alloc", then the type
 * names 14RaceCameraBase and t9UnitArena1Ui64 (reverse definition order;
 * nothing references them: DATA_START).  No .sdata. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/T383_race_camera_base.h"     /* (seen: RaceCameraBase) */
#include "game/race/ai/T362_ai_params.h"
#include "game/race/ai/ai_internal.h"
// DATA_START 0x0033DC70

/* The CourseDriveParam record of the car's drive class (idx) and the course
 * kind (mode; ai_drive_param, T361) turned into AIParams. */
// FUNC 0x001F5FD8 ai_load_params
void ai_load_params(AIDriver *ai, int idx, int mode)
{
    const CourseDriveParam *r = ai_drive_param(ai, mode, idx);
    AIParams *p = &ai->params;
    TcsParams *t = &p->assist;
    u8 flags;
    int pit;

    p->steerGain = (f32)(r->steerGain * 5) * 0.01f;
    p->lookNear = (f32)r->lookNear;
    p->lookFar = (f32)r->lookFar;
    p->lookInvSpeed = 1.0f / ((f32)(r->lookSpeed * 2) * KMH_TO_MS);
    p->throttleScale = 1.0f;
    p->brakeScale = 100.0f / (f32)r->brake;
    p->cornerScale = (f32)r->cornerScale * 0.01f;
    p->cornerOffset = (f32)(r->cornerOffset - 100) * KMH_TO_MS;
    p->slideThrottle = (f32)r->slideThrottle * 0.01f;
    p->slideBrake = (f32)r->slideBrake * 0.01f;
    p->gripBalance = (f32)r->gripBalance * 0.01f;
    p->cornerBonus = (f32)r->cornerBonus * KMH_TO_MS;
    p->steerRate = (f32)(r->steerRate * 10) * DEG_TO_RAD;
    p->bonusDelay = r->bonusDelay * 10;
    /* An if, not a ?: (that one branches, and the branch ends the block in
     * which egcs keeps 0.01 and 0.2777778 in registers). */
    pit = r->pitSpeed;
    if (pit == 0)
        pit = 80;
    p->pitSpeed = (f32)pit * KMH_TO_MS;

    /* The driving aids (bits 0..3: the first set, 4..7: the second). */
    flags = r->assistFlags;
    t->brakeGain = (f32)r->brakeGain * 0.01f;
    t->brakeSlip = (f32)r->brakeSlip * 0.01f;
    t->absGain = (f32)r->absGain * 0.01f;
    if (flags & 1)
        t->brakeFront = 1;
    else
        t->brakeFront = 0;
    t->brakeRear = (flags >> 1) & 1;
    t->absFront = (flags >> 2) & 1;
    t->absRear = (flags >> 3) & 1;
    /* egcs folds each pair of byte tests into one test of the word at +0x0C
     * (lw; & 0xFFFF: brakeFront / brakeRear, & 0xFFFF0000: absFront /
     * absRear), as u080 found for its byte pairs. */
    t->brakeOn = ((t->brakeFront != 0 || t->brakeRear != 0) && t->brakeGain != 0.0f) ? 1 : 0;
    t->absOn = ((t->absFront != 0 || t->absRear != 0) && t->absGain != 0.0f) ? 1 : 0;

    t->brakeGain2 = (f32)r->brakeGain2 * 0.01f;
    t->brakeSlip2 = (f32)r->brakeSlip2 * 0.01f;
    t->absGain2 = (f32)r->absGain2 * 0.01f;
    t->brakeFront2 = (flags >> 4) & 1;
    t->brakeRear2 = (flags >> 5) & 1;
    t->absFront2 = (flags >> 6) & 1;
    t->absRear2 = flags >> 7;
    /* The same fold on the word at +0x1C. */
    t->brakeOn2 = ((t->brakeFront2 != 0 || t->brakeRear2 != 0) && t->brakeGain2 != 0.0f) ? 1 : 0;
    t->absOn2 = ((t->absFront2 != 0 || t->absRear2 != 0) && t->absGain2 != 0.0f) ? 1 : 0;

    t->fadeGain = (f32)r->fadeGain * 0.1f;
    t->fadeStart = (f32)r->fadeStart * 0.01f;
    t->throttleGain = (f32)r->throttleGain * 0.1f;
    if (r->slipTarget == 100 || r->slipTarget == 0)
        t->slipTarget = 1.0f;
    else
        t->slipTarget = (f32)r->slipTarget * 0.01f;
}

/* The race rules' handicaps of drive class idx (RaceRuleOptions
 * enemySkillA: throttle %, B: brake %, C: corner speed %, C2: corner speed
 * km/h; 0 counts as 100 %), and, in a conductor of mode 2 (the finish timer
 * races), a spread of the driver 2 cars by their index. */
// FUNC 0x001F6380 ai_apply_rules
void ai_apply_rules(AIDriver *ai, int idx)
{
    DynamicsConductor *dc = ai->conductor;
    AIParams *p = &ai->params;
    RaceRuleOptions *rules = &dc->rules;
    int throttlePct = rules->enemySkillA[idx];
    int brakePct = rules->enemySkillB[idx];
    int cornerPct = rules->enemySkillC[idx];
    int cornerKmh = rules->enemySkillC2[idx];

    if (throttlePct == 0)
        throttlePct = 100;
    if (!(throttlePct < 101))
        throttlePct = 100;
    if (brakePct == 0)
        brakePct = 100;
    if (cornerPct == 0)
        cornerPct = 100;
    if (dc->mode == 2) {
        DynCarState *st = &ai->car->st;     /* in a register of its own (addiu 0xF84) */

        if (st->driver == 2) {
            int n = st->carIndex;

            cornerPct -= (n * 4) & 4;                   /* odd cars: 4 % slower corners */
            brakePct -= (n % 3) * 7;
            cornerKmh -= ((n * 7) % 5) * 2;
            p->steerGain *= (f32)(((n + 1) % 3 - 1) * 3 + 100) * 0.01f;
        }
    }
    if (throttlePct != 100)
        p->throttleScale = (f32)throttlePct * 0.01f;
    if (brakePct != 100)
        p->brakeScale *= 100.0f / (f32)brakePct;
    if (cornerPct != 100)
        p->cornerScale *= (f32)cornerPct * 0.01f;
    if (cornerKmh != 0)
        p->cornerOffset += (f32)cornerKmh * KMH_TO_MS;
}

/* Half throttle and 80% corner speeds (car_finish, CAR_FINISH_RACE_END). */
// FUNC 0x001F6518 ai_reduce_pace
void ai_reduce_pace(AIDriver *ai)
{
    AIParams *p = &ai->params;

    p->throttleScale = 0.5f;
    p->cornerScale *= 0.8f;
}

/* Empty (DynCar_initialize calls it instead of ai_apply_rules for a mode 0
 * car). */
// FUNC 0x001F6540 F_001F6540
void F_001F6540(AIDriver *ai)
{
}

/* Empty (car_finish, before ai_apply_rules). */
// FUNC 0x001F6548 F_001F6548
void F_001F6548(AIDriver *ai)
{
}

/* The lift coefficients (x v^2) over the weight: the downforce per (m/s)^2
 * as a share of the weight (ai_corner_speed). */
// FUNC 0x001F6550 ai_set_downforce
void ai_set_downforce(AIDriver *ai, CarDynamics *dyn)
{
    ai->params.downforce = (dyn->liftCoef[0] + dyn->liftCoef[1]) / dyn->weight;
}
