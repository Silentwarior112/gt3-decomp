#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/race/dynamics/T305_dynamics_conductor.cpp"
#pragma implementation "src/game/race/dynamics/T305_dynamics_conductor.cpp"
#pragma implementation "T305_dynamics_conductor.h"
#endif
/* T305 (0x001ED000..0x001ED9F0): DynamicsConductor - the running order
 * (slots 1 / 2 and the car comparison), the table of the lap / distance gaps
 * between every two cars, the class-inline accessors (Lupo 2 has them in the
 * class: T305 emits out-of-line copies, most with no callers), the base
 * answers of several slots and the typeinfo function.
 *
 * Retail T305's .rodata (0x0033CA10..0x0033CBA1): "bad_alloc" (nothing
 * references it: DATA_START), DynamicsConductor's vtable (0x0033CA20), the
 * type names 17DynamicsConductor, t9UnitArena1Ui64 (reverse definition
 * order: T153 is read before the class). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033CA10

// FUNC 0x001ED000 DynamicsConductor::getOrder
/* The car index at running position i. */
int DynamicsConductor::getOrder(int i)
{
    return order[i];
}

// FUNC 0x001ED018 car_effective_lap
/* The lap count less the times the car backed over the line. */
int car_effective_lap(DynCarState *st)
{
    return st->lap - st->lapAdjust;
}

// FUNC 0x001ED028 DynamicsConductor::getGap
/* How far a is ahead of b: whole laps and the course distance, the distance
 * kept with the sign of the laps (a lap is traded for one course length). */
void DynamicsConductor::getGap(DynCarState *a, DynCarState *b, int *laps, f32 *dist)
{
    int lapGap = car_effective_lap(a) - car_effective_lap(b);
    f32 distGap = a->lapDistance - b->lapDistance;

    if (lapGap > 0 && distGap < 0.0f) {
        lapGap -= 1;
        distGap += getLapLength();
    } else if (lapGap < 0 && 0.0f < distGap) {
        lapGap += 1;
        distGap -= getLapLength();
    }
    *laps = lapGap;
    *dist = distGap;
}

// FUNC 0x001ED108 car_compare
/* > 0: a is ahead of b, < 0: b is ahead, 0: level.  More (effective) laps
 * first; on the same lap the course position when neither has finished; a
 * finished car ranks behind one that has not (as the code reads); two
 * finished cars by their last lap end time, the earlier first. */
int car_compare(DynCarState *a, DynCarState *b)
{
    int lapA = car_effective_lap(a);
    int lapB = car_effective_lap(b);
    int diff;

    if (lapA > lapB)
        return 1;
    if (lapA < lapB)
        return -1;
    if ((b->finished | a->finished) == 0) {
        if (a->lapDistance > b->lapDistance)
            return 1;
        if (a->lapDistance < b->lapDistance)
            return -1;
    } else if (b->finished != 0 && a->finished != 0) {
        diff = b->lapStartTime - a->lapStartTime;
        if (diff < 0)
            return -1;
        if (diff > 0)
            return 1;
    } else {
        if (b->finished != 0)
            return 1;
        return -1;
    }
    return 0;
}

// FUNC 0x001ED1F0 DynamicsConductor::virtual_2
/* Sorts order[] by car_compare (insertion sort, leader first) and gives
 * every car its 1-based running position (st.rank). */
void DynamicsConductor::virtual_2()
{
    int i, j, car;

    for (i = 1; i < carCount; i++) {
        car = order[i];
        for (j = i - 1; j >= 0; j--) {
            if (car_compare(&getCar(order[j])->st, &getCar(car)->st) >= 0)
                break;
            order[j + 1] = order[j];
        }
        order[j + 1] = car;
    }
    for (i = 0; i < carCount; i++)
        getCar(order[i])->st.rank = i + 1;
}

// FUNC 0x001ED358 DynamicsConductor::virtual_1
/* The order back to the car indices (the grid order). */
void DynamicsConductor::virtual_1()
{
    int i;

    for (i = 7; i >= 0; i--)
        order[i] = -1;
    for (i = 0; i < carCount; i++) {
        getCar(i)->st.rank = i + 1;
        order[i] = i;
    }
}

// FUNC 0x001ED410 DynamicsConductor::clearGapTable
void DynamicsConductor::clearGapTable()
{
    memset(gapLap, 0, sizeof(gapLap));
    memset(gapDist, 0, sizeof(gapDist));
}

// FUNC 0x001ED458 DynamicsConductor::updateGapTable
/* gapLap[i][j] / gapDist[i][j]: how far car j is ahead of car i (the laps
 * clamped to -127..127). */
void DynamicsConductor::updateGapTable()
{
    int n = carCount;
    int i, j;
    int laps;
    f32 dist;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (i == j)
                continue;
            getGap(&getCar(j)->st, &getCar(i)->st, &laps, &dist);
            if (laps >= 0x80)
                laps = 0x7F;
            else if (laps < -0x7F)
                laps = -0x7F;
            gapLap[i][j] = (s8)laps;
            gapDist[i][j] = dist;
        }
    }
}

// FUNC 0x001ED5A0 DynamicsConductor::gapLaps
int DynamicsConductor::gapLaps(int i, int j)
{
    return gapLap[i][j];
}

// FUNC 0x001ED5C0 DynamicsConductor::gapDistance
f32 DynamicsConductor::gapDistance(int i, int j)
{
    return gapDist[i][j];
}

// FUNC 0x001ED5E0 DynamicsConductor::gapNearest
/* The gap with the distance brought into -length/2..length/2 (a lap is
 * traded for the course length).  Callers: CarPairStates::strongest /
 * activeMask (T360). */
int DynamicsConductor::gapNearest(int i, int j, f32 *dist)
{
    int laps = gapLaps(i, j);
    f32 d = gapDistance(i, j);
    f32 len = getLapLength();
    f32 half = len * 0.5f;

    if (half < d) {
        d -= len;
        laps += 1;
    } else if (d < -half) {
        d += len;
        laps -= 1;
    }
    *dist = d;
    return laps;
}

// FUNC 0x001ED6A8 DynamicsConductor::GetOptionTT_IsGhostMapped
/* Lupo 2's inline accessor of this size and place (the accessors
 * 0x001ED6A8..0x001ED990 are Lupo 2's in reverse order); no callers. */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6B0 DynamicsConductor::GetOptionArcadeBattle_CrashDamage
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6B8 DynamicsConductor::GetOptionArcadeRace_CrashDamage
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6C0 DynamicsConductor::IsOneWayCourse
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6C8 DynamicsConductor::getMode
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6D8 DynamicsConductor::EveryTimePitStop
/* No pit stop is due (car_update_pit's service request bit 2). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED6E0 DynamicsConductor::isCountingDown
/* implicit: getCountDown (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED700 DynamicsConductor::getSlowCarBoostType_ (getSlowCarBoostType___17DynamicsConductor)
/* -1: update pairs no cars for the slow car boost (slot 3). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED708 DynamicsConductor::isTimeOver
/* ms at or past the race's time limit (never without one). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED738 DynamicsConductor::getTimeLimit
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED750 DynamicsConductor::CurrentLapTime
/* implicit: lapTime (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED770 DynamicsConductor::clockMs
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED7A0 DynamicsConductor::clockRem
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED7D8 DynamicsConductor::getClockMs
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED800 DynamicsConductor::getRules
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED810 DynamicsConductor::GetRaceWatcher
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED820 DynamicsConductor::getGapDistance
/* implicit: gapDistance (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED840 DynamicsConductor::getGapLaps
/* implicit: gapLaps (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED860 DynamicsConductor::recordOneLapGhostControl
/* Empty: only the free run records the car's input for its ghost. */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED868 DynamicsConductor::getWearCurve
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED870 DynamicsConductor::GetCollisionProcessor
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED880 DynamicsConductor::getCourse
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED890 DynamicsConductor::getCarCount
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED8A0 DynamicsConductor::setCarCount
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED8B0 DynamicsConductor::IssuePassingGate
/* Empty: the battles record the checkpoint time (recordGate). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED8B8 DynamicsConductor::processCollision_ (processCollision___17DynamicsConductor)
/* The collision step of each car alone (collideWalls(0)). */
/* implicit: collideWalls (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED8D8 DynamicsConductor::IsInsidePitRedArea
/* pos arrives in $f12 and goes on untouched. */
/* implicit: pit_lane_contains (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED900 DynamicsConductor::IsInsidePitYellowArea
/* implicit: pit_lane_contains (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED928 DynamicsConductor::GetEnemyLineProcessorOld
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED938 DynamicsConductor::SetSlowCarBoostParameters
/* 0: the rules' own boost (virtual_20 passes it to dc_setup_slow_car_boost). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED940 DynamicsConductor::getReplaySpeed
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED950 DynamicsConductor::setReplaySpeed
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED960 DynamicsConductor::isDataLoggerMultiLap
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED968 DynamicsConductor::isDataLoggerStandingStart
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED970 DynamicsConductor::isDataLoggerSupported
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED978 DynamicsConductor::getOrganization
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED980 DynamicsConductor::getSlowCarBoost
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED988 DynamicsConductor::ReportTimeDifference
/* Empty; the battles report the checkpoint time differences
 * (reportGateTimes). */
/* inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001ED990 DynamicsConductor::getRunState
/* implicit: getCar (the inline body); inline in game/race/dynamics/T305_dynamics_conductor.h; egcs emits this copy (#pragma implementation) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001ED9B0 __tf17DynamicsConductor
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti17DynamicsConductor (0x00353410, gp-0x45E0) on the first call (no
 * bases).  The subclasses' typeinfo functions call it for their base. */
extern "C" const type_info &__tf17DynamicsConductor(void)
{
    if (__ti17DynamicsConductor[0] == 0)
        __rtti_user(__ti17DynamicsConductor, "17DynamicsConductor");
    return *(const type_info *)__ti17DynamicsConductor;
}
#endif
