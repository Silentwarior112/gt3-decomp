/* T316_dc_single_player.cpp - DynamicsConductorSinglePlayer (0x001F2748..0x001F2A10).
 *
 * The conductor of the single player races (RaceArcade T134, RaceChampionship
 * T151 and others).  Its rules start as the defaults g_002D2988 (T281); slot
 * 0 reports the checkpoint time differences (reportGateTimes) and slot 14
 * records the checkpoint times (recordGate), as the battles do; the slow car
 * boost and the tyre wear are on (slots 9 / 10), the boost type is the
 * rules' (slot 31), no course-out rule (slot 39).  For a start without
 * countdown (startAction 3) the cars start rolling at 80 km/h, car 0 from
 * start type 8 (slot 24), and car 0 gets the car when its countdown runs out
 * (slot 12).
 *
 * Retail T316's .rodata opens with <new>'s "bad_alloc" (0x0033D910, nothing
 * references it: DATA_START), then the vtable and the type names.
 */
// DATA_START 0x0033D910
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T194_EMITS_TYPEINFO            /* 20RaceDisplayEventBase's type name here too, as retail */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/display/T194_race_display_event.h"   /* the display events (T154's RaceDisplayTimeDiffEvent .. GeneralTimeEvent too) */
#include "game/race/dynamics/T316_dc_single_player.h"     /* last (dc_modes_internal.h) */

// FUNC 0x001F2748 DynamicsConductorSinglePlayer::DynamicsConductorSinglePlayer
//   DynamicsConductor(), the vptr (0x0033D920), then the rules = the 0x40
//   default bytes at 0x002D2988 (unaligned 8-byte copies).  Callers:
//   RaceArcade (T134), RaceChampionship (T151).
DynamicsConductorSinglePlayer::DynamicsConductorSinglePlayer()
{
    /* implicit: DynamicsConductor (the base constructor) */
    rules = g_002D2988;
}

// FUNC 0x001F2810 DynamicsConductorSinglePlayer::getSpecialStart
//   Without countdown: start type 8 (car 0 only), 80 km/h; else the rules'
//   start speed (startWithLaunchSpeed, the base's slot 24) and its result.  The first
//   path sets no result: $v0 is left 0x50 (initialize ignores it); returned
//   here, so that the function does not fall off its end.
int DynamicsConductorSinglePlayer::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    if (org->startAction == START_ACTION_NO_COUNTDOWN) {
        if (car == 0)
            *startType = 8;
        *speedKmh = 80;
#ifndef BOOT_OLD_GCC
        return 80;
#endif
    } else
        return startWithLaunchSpeed(car, startType, startArg, speedKmh);
}

// FUNC 0x001F2860 DynamicsConductorSinglePlayer::virtual_12
//   Per frame, without countdown: car 0's countdown; when it runs out the
//   driver gets the car (control mode 0) and the start event.
void DynamicsConductorSinglePlayer::virtual_12()
{
    RaceOrganization *o = org;

    if (o->startAction == START_ACTION_NO_COUNTDOWN) {
        RaceCarState *c = getCar(0);
        int fire;

        if (c->countdown <= 0)
            fire = 0;
        else
            fire = --c->countdown == 0;
        if (fire != 0) {
            car_mode_driver(c);
            o->eventQueue.post(RACE_EVENT_START, 0, 0);
        }
    }
}

// FUNC 0x001F28E8 DynamicsConductorSinglePlayer::virtual_16
//   The start line: nothing.
void DynamicsConductorSinglePlayer::virtual_16(int car, u32 ms, int frac50, f32 t)
{
}

// FUNC 0x001F28F0 DynamicsConductorSinglePlayer::virtual_34
//   isDirtCourse is called and its result dropped: 0.
//   77.8%: retail copies the result to $v1 before the 0 (daddu v1,v0; a
//   dead copy, as the battles' copies 0x001F0298 / 0x001F04E8, u083).
//   Tried: ?: / if / switch with equal arms, locals, `& 0', `0 *': egcs
//   drops the result at once.
int DynamicsConductorSinglePlayer::virtual_34()
{
    isDirtCourse();
    return 0;
}

// FUNC 0x001F2918 DynamicsConductorSinglePlayer::chargePenaltyBySurface
//   The course-out rule: none.
void DynamicsConductorSinglePlayer::chargePenaltyBySurface(RaceCarState *car, int left, int right)
{
}

// FUNC 0x001F2920 DynamicsConductorSinglePlayer::ReportTimeDifference
//   The checkpoint times recorded since the last report, as display events.
void DynamicsConductorSinglePlayer::ReportTimeDifference()
{
    reportGateTimes();
}

// FUNC 0x001F2940 DynamicsConductorSinglePlayer::IssuePassingGate
//   A checkpoint: its time ms is recorded; split is not read.
void DynamicsConductorSinglePlayer::IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split)
{
    recordGate(car, lap, gate, ms);
}

// FUNC 0x001F2960 DynamicsConductorSinglePlayer::processCollision_
//   collideAllCars (the base's calls collideWalls(0)).
void DynamicsConductorSinglePlayer::processCollision_()
{
    collideAllCars();
}

// FUNC 0x001F2980 DynamicsConductorSinglePlayer::getSlowCarBoostType_
//   The rules' boost type.
int DynamicsConductorSinglePlayer::getSlowCarBoostType_()
{
    return rules.enemyBoostType;
}

// FUNC 0x001F2990 DynamicsConductorSinglePlayer::SetSlowCarBoostParameters
//   The slow car boost of the rules is on.
int DynamicsConductorSinglePlayer::SetSlowCarBoostParameters(RaceRuleOptions *o)
{
    return 1;
}

// FUNC 0x001F2998 DynamicsConductorSinglePlayer::SetTireWearParameters
//   The tyre wear is on.
int DynamicsConductorSinglePlayer::SetTireWearParameters(RaceRuleOptions *o)
{
    return 1;
}

// FUNC 0x001F29A0 DynamicsConductorSinglePlayer::getEachEntrantInfo_
//   getEntrantInfoRace.
void DynamicsConductorSinglePlayer::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoRace(car, tyreFlag, b, c, d);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F29C0 __tf29DynamicsConductorSinglePlayer
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti29DynamicsConductorSinglePlayer (0x004B28A0, base
//   __ti17DynamicsConductor) on the first call.
extern "C" const type_info &__tf29DynamicsConductorSinglePlayer(void)
{
    if (__ti29DynamicsConductorSinglePlayer[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti29DynamicsConductorSinglePlayer, "29DynamicsConductorSinglePlayer",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti29DynamicsConductorSinglePlayer;
}
#endif
