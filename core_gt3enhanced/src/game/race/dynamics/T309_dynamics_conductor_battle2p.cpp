/* T309 (0x001EFFD0..0x001F03A8): DynamicsConductorBattle2P, the conductor of
 * the two-player split screen battle.  Its options are the race's split
 * battle options (RaceOrganization::option.splitBattle: the lap count, the
 * tyre damage, a handicap and the boost preset).  The checkpoint times of
 * the cars are collected (recordGate) and reported as gaps
 * (reportGateTimes); when a car's race ends, the other cars are stopped
 * (st.driver 2).
 *
 * Retail T309's .rodata (0x0033CD00..0x0033CE99): "bad_alloc" (nothing
 * references it: DATA_START), the vtable (0x0033CD10), the type names
 * 25DynamicsConductorBattle2P, t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T309_dynamics_conductor_battle2p.h"
#include "game/race/modes/race_pod_rules.h"  /* pod_rules_lap_check (the enhanced engine's Pod race) */
// DATA_START 0x0033CD00

// FUNC 0x001EFFD0 DynamicsConductorBattle2P::DynamicsConductorBattle2P
/* Caller: RaceSplitBattle::RaceSplitBattle (0x00183480).  The base
 * constructor (0x001F2A10), then the vtable 0x0033CD10. */
DynamicsConductorBattle2P::DynamicsConductorBattle2P()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F0010 DynamicsConductorBattle2P::getSpecialStart
/* The handicap h (option.splitBattle.handicap, -150..151 by 5 m): without
 * one, nothing.  Else *startType = 1 and *startArg: for h > 0 car 0 gets
 * -10, the others min(-h, -20); for h < 0 car 0 gets min(h, -20), the others
 * -10.  Then the base rule (startWithLaunchSpeed) sets *speedKmh.  Without a
 * handicap the code sets no result: $v0 is the handicap read, 0 (initialize
 * ignores it).  The original had no return statement there (u084 found the
 * same in the other conductors' getSpecialStart); GCC 14 returns that 0. */
int DynamicsConductorBattle2P::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    int handicap = org->option.splitBattle.handicap;
    int argOthers, argCar0;

    if (handicap != 0) {
        if (handicap > 0) {
            argCar0 = -10;
            argOthers = -handicap;
            if (!(argOthers < -19))
                argOthers = -20;
        } else {
            argCar0 = handicap;
            argOthers = -10;
            if (!(argCar0 < -19))
                argCar0 = -20;
        }
        *startType = 1;
        if (car == 0)
            *startArg = argCar0;
        else
            *startArg = argOthers;
        return startWithLaunchSpeed(car, startType, startArg, speedKmh);
    }
#ifndef BOOT_OLD_GCC
    return 0;
#endif
}

// FUNC 0x001F0088 DynamicsConductorBattle2P::virtual_29
/* The battle's lap count. */
int DynamicsConductorBattle2P::virtual_29()
{
    return org->option.splitBattle.raceLaps;
}

// FUNC 0x001F0098 DynamicsConductorBattle2P::SetSlowCarBoostParameters
/* The boost preset of the battle option (0 none, 1 low, 2 high). */
int DynamicsConductorBattle2P::SetSlowCarBoostParameters(RaceRuleOptions *o)
{
    return rules_boost_preset(o, org->option.splitBattle.boost);
}

// FUNC 0x001F00C0 DynamicsConductorBattle2P::SetTireWearParameters
/* No tyre wear on course kind 2; else preset 2 with the battle's tyre
 * damage, 0 (off) without. */
int DynamicsConductorBattle2P::SetTireWearParameters(RaceRuleOptions *o)
{
    if (getCourseKind() == 2)
        return 0;
    return rules_tire_wear_preset(o, org->option.splitBattle.tireDamage != 0 ? 2 : 0);
}

// FUNC 0x001F0118 DynamicsConductorBattle2P::virtual_17
/* As DynamicsConductor::virtual_17 without the time limit; the end of one
 * car's race stops every other car still driven by its driver (st.driver
 * 2, its computer driver's corners found again).  The enhanced engine
 * (Lupo 2 L2 0x001EE170): the Pod race's lap check first, as in
 * DynamicsConductor::virtual_17 (the 2-player Pod race). */
void DynamicsConductorBattle2P::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    int laps, ends, i;
    RaceCarState *other;

    if (pod_rules_lap_check(this, car) == 0) {
        pod_emotion_set(c, 7, 0xFF);        /* sabishii, held (Lupo 2 L2 0x001F7BAC) */
        return;
    }
    dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
    laps = virtual_29();
    ends = 0;
    if (getCar(getOrder(0))->st.finished != 0)
        ends = 1;
    if (laps != 0 && lap == laps)
        ends = 1;
    if (ends != 0) {
        org->eventQueue.post(RACE_EVENT_GOAL, car, 2);
        car_finish(getCar(car), 3);
        for (i = 0; i < carCount; i++) {
            DynCarState *ost;

            other = getCar(i);
            ost = &other->st;
            if (i != car && ost->driver == 0) {
                ost->driver = 2;
                ai_find_corner(&other->ai);
            }
        }
    }
    DynCar_clearFaults(c);
    st->lapStartTime = ms;
}

// FUNC 0x001F0298 DynamicsConductorBattle2P::virtual_34
/* 0; isDirtCourse's answer (course kind 2) is dropped. */
int DynamicsConductorBattle2P::virtual_34()
{
    isDirtCourse();
    return 0;
}

// FUNC 0x001F02C0 DynamicsConductorBattle2P::chargePenaltyBySurface
/* No course-out rule in the battles. */
void DynamicsConductorBattle2P::chargePenaltyBySurface(RaceCarState *car, int left, int right)
{
}

// FUNC 0x001F02C8 DynamicsConductorBattle2P::ReportTimeDifference
void DynamicsConductorBattle2P::ReportTimeDifference()
{
    reportGateTimes();
}

// FUNC 0x001F02E8 DynamicsConductorBattle2P::IssuePassingGate
void DynamicsConductorBattle2P::IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split)
{
    recordGate(car, lap, gate, ms);
}

// FUNC 0x001F0308 DynamicsConductorBattle2P::processCollision_
/* The collision step of all cars together. */
void DynamicsConductorBattle2P::processCollision_()
{
    collideAllCars();
}

// FUNC 0x001F0328 DynamicsConductorBattle2P::getSlowCarBoostType_
int DynamicsConductorBattle2P::getSlowCarBoostType_()
{
    return rules.enemyBoostType;
}

// FUNC 0x001F0338 DynamicsConductorBattle2P::getEachEntrantInfo_
void DynamicsConductorBattle2P::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoRace(car, tyreFlag, b, c, d);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F0358 __tf25DynamicsConductorBattle2P
/* Compiler-generated typeinfo function: the __si_type_info
 * __ti25DynamicsConductorBattle2P (0x004B2830), base __ti17DynamicsConductor
 * (0x00353410, gp-0x45E0), built on the first call. */
extern "C" const type_info &__tf25DynamicsConductorBattle2P(void)
{
    if (__ti25DynamicsConductorBattle2P[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti25DynamicsConductorBattle2P, "25DynamicsConductorBattle2P",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti25DynamicsConductorBattle2P;
}
#endif
