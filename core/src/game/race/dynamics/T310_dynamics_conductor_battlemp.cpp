/* T310 (0x001F03A8..0x001F05F8): DynamicsConductorBattleMP, the conductor of
 * the multi-player battle (RaceLinkBattle, T138: the i.LINK battle): the
 * Battle2P options and checkpoint gaps, the base lap end (slot 17) and a cap
 * on the slow car boost (slot 3).
 *
 * Retail T310's .rodata (0x0033CEA0..0x0033D039): "bad_alloc" (nothing
 * references it: DATA_START), the vtable (0x0033CEB0), the type names
 * 25DynamicsConductorBattleMP, t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T310_dynamics_conductor_battlemp.h"
// DATA_START 0x0033CEA0

// FUNC 0x001F03A8 DynamicsConductorBattleMP::DynamicsConductorBattleMP
/* Caller: RaceLinkBattle::RaceLinkBattle (0x00184E00).  The base
 * constructor (0x001F2A10), then the vtable 0x0033CEB0. */
DynamicsConductorBattleMP::DynamicsConductorBattleMP()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F03E8 DynamicsConductorBattleMP::virtual_29
int DynamicsConductorBattleMP::virtual_29()
{
    return org->option.splitBattle.raceLaps;
}

// FUNC 0x001F03F8 DynamicsConductorBattleMP::SetSlowCarBoostParameters
int DynamicsConductorBattleMP::SetSlowCarBoostParameters(RaceRuleOptions *o)
{
    return rules_boost_preset(o, org->option.splitBattle.boost);
}

// FUNC 0x001F0420 DynamicsConductorBattleMP::SetTireWearParameters
int DynamicsConductorBattleMP::SetTireWearParameters(RaceRuleOptions *o)
{
    if (getCourseKind() == 2)
        return 0;
    return rules_tire_wear_preset(o, org->option.splitBattle.tireDamage != 0 ? 2 : 0);
}

// FUNC 0x001F0478 DynamicsConductorBattleMP::virtual_3
/* The base factor, at most boost.boostGain + 1; when capped, st's
 * timeScale (which the base set to factor x scale) becomes cap x
 * boost.scale. */
f32 DynamicsConductorBattleMP::virtual_3(DynCarState *ref, DynCarState *st, f32 scale)
{
    f32 factor = DynamicsConductor::virtual_3(ref, st, scale);
    SlowCarBoost *b = &boost;
    f32 cap = b->boostGain + 1.0f;

    if (factor <= cap)
        return factor;
    factor = cap;
    st->timeScale = factor * b->scale;
    return factor;
}

// FUNC 0x001F04E8 DynamicsConductorBattleMP::virtual_34
/* 0; isDirtCourse's answer (course kind 2) is dropped. */
int DynamicsConductorBattleMP::virtual_34()
{
    isDirtCourse();
    return 0;
}

// FUNC 0x001F0510 DynamicsConductorBattleMP::chargePenaltyBySurface
void DynamicsConductorBattleMP::chargePenaltyBySurface(RaceCarState *car, int left, int right)
{
}

// FUNC 0x001F0518 DynamicsConductorBattleMP::ReportTimeDifference
void DynamicsConductorBattleMP::ReportTimeDifference()
{
    reportGateTimes();
}

// FUNC 0x001F0538 DynamicsConductorBattleMP::IssuePassingGate
void DynamicsConductorBattleMP::IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split)
{
    recordGate(car, lap, gate, ms);
}

// FUNC 0x001F0558 DynamicsConductorBattleMP::processCollision_
void DynamicsConductorBattleMP::processCollision_()
{
    collideAllCars();
}

// FUNC 0x001F0578 DynamicsConductorBattleMP::getSlowCarBoostType_
int DynamicsConductorBattleMP::getSlowCarBoostType_()
{
    return rules.enemyBoostType;
}

// FUNC 0x001F0588 DynamicsConductorBattleMP::getEachEntrantInfo_
void DynamicsConductorBattleMP::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoRace(car, tyreFlag, b, c, d);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F05A8 __tf25DynamicsConductorBattleMP
/* Compiler-generated typeinfo function: the __si_type_info
 * __ti25DynamicsConductorBattleMP (0x004B2840), base __ti17DynamicsConductor
 * (0x00353410, gp-0x45E0), built on the first call. */
extern "C" const type_info &__tf25DynamicsConductorBattleMP(void)
{
    if (__ti25DynamicsConductorBattleMP[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti25DynamicsConductorBattleMP, "25DynamicsConductorBattleMP",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti25DynamicsConductorBattleMP;
}
#endif
