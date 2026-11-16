/* T315_dc_free_practice.cpp - DynamicsConductorFreePractice (0x001F2010..0x001F2748).
 *
 * The conductor of the free practice / qualifying session (RaceFreePractice,
 * T136).  The running order is the entries' order at first (slot 1), then
 * the cars by best lap (slot 2, an insertion sort); a car's best lap starts
 * from its entry's best of the session (slot 21).  Section and lap times
 * post a RaceDisplayTimeDiffEvent against the race record
 * (RaceOrganization::lapRecord), and a lap under the record the
 * information "provisional pole"; car 0's start line "start the qualifying
 * attack".  There are no laps to finish (slot 29 = 0), the pit stop is
 * always due (slot 41), no course-out rule (slot 39).
 *
 * The unit also has the out-of-line copies of RaceDisplayTimeDiffEvent's
 * inline getEventType and its typeinfo function (T154's are 0x00195788 /
 * 0x00195790).
 *
 * Retail T315's .rodata opens with <new>'s "bad_alloc" (0x0033D6D8, nothing
 * references it: DATA_START), then the vtable and the type names.
 */
// DATA_START 0x0033D6D8
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T194_EMITS_TYPEINFO            /* 20RaceDisplayEventBase's type name here too, as retail */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/display/T194_race_display_event.h"   /* the display events (T154's RaceDisplayTimeDiffEvent .. GeneralTimeEvent too) */
#include "game/race/dynamics/T315_dc_free_practice.h"     /* last (dc_modes_internal.h) */

// FUNC 0x001F2010 DynamicsConductorFreePractice::DynamicsConductorFreePractice
//   DynamicsConductor(), then the vptr (0x0033D6E8).  Caller:
//   RaceFreePractice (T136).
DynamicsConductorFreePractice::DynamicsConductorFreePractice()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F2050 DynamicsConductorFreePractice::getEachEntrantInfo_
//   getEntrantInfoRace (as SinglePlayer and the battles).
void DynamicsConductorFreePractice::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoRace(car, tyreFlag, b, c, d);
}

// FUNC 0x001F2070 DynamicsConductorFreePractice::getSpecialStart
//   Car 0: start type 6, argument 0, 0 km/h; car n: start type 5, argument
//   -(n x 200 + 300), 120 km/h.  The code sets no result: $v0 is left 6 or
//   the start argument (initialize ignores it); returned here, so that the
//   function does not fall off its end.
int DynamicsConductorFreePractice::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    if (car == 0) {
        *startType = 6;
        *startArg = 0;
        *speedKmh = 0;
#ifndef BOOT_OLD_GCC
        return 6;
#endif
    } else {
        *startType = 5;
        *startArg = -(car * 200 + 300);
        *speedKmh = 120;
#ifndef BOOT_OLD_GCC
        return -(car * 200 + 300);
#endif
    }
}

// FUNC 0x001F20B0 DynamicsConductorFreePractice::virtual_21
//   The base's; then every car's best lap is updated with its entry's best
//   of the session (sessionBest.total).
void DynamicsConductorFreePractice::virtual_21(int a)
{
    RaceOrganization *o;
    int i;

    DynamicsConductor::virtual_21(a);
    o = org;
    for (i = 0; i < carCount; i++)
        car_update_best_lap(getCar(i), o->entries->cars[i]->sessionBest.total);
}

// FUNC 0x001F2148 DynamicsConductorFreePractice::virtual_29
//   The lap count: none.
int DynamicsConductorFreePractice::virtual_29()
{
    return 0;
}

// FUNC 0x001F2150 DynamicsConductorFreePractice::virtual_15
//   A checkpoint: for a car without fault and a race record of the section,
//   the time difference split - record; then the section time events (the
//   lap not decremented).  raw is not read.
void DynamicsConductorFreePractice::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceOrganization *o = org;
    RaceCarState *c = getCar(car);
    u8 fault = c->st.fault;

    if (fault == 0) {
        u32 record = o->lapRecord.section[gate];

        if (record != RACE_TIME_INVALID) {
            RaceDisplayTimeDiffEvent ev;    /* implicit: the inline constructor: RaceDisplayDiffEvent(), the vptr */

            ev.diff = split - record;
            ev.car = car;
            ev.post(o);
        }
    }
    dc_post_section_time(this, car, lap, gate, ms, split, c->st.fault);
}

// FUNC 0x001F2248 DynamicsConductorFreePractice::virtual_16
//   The start line: for car 0 the information "start the qualifying attack"
//   (car 0); the lap starts at ms.
void DynamicsConductorFreePractice::virtual_16(int car, u32 ms, int frac50, f32 t)
{
    DynCarState *st = &getCar(car)->st;

    if (car == 0) {
        RaceOrganization *o = org;
        RaceDisplayInformationEvent ev;

        ev.information = INFO_QUALIFY_ATTACK;
        ev.car = 0;
        ev.post(o);
    }
    st->lapStartTime = ms;
}

// FUNC 0x001F22C0 DynamicsConductorFreePractice::virtual_17
//   A lap's end: for a car without fault the time difference to the race's
//   best lap (if there is one) and, under it, the information "provisional
//   pole" (also when there is no record: any time is under "no time"); then
//   the lap time events, the faults cleared; the lap starts at ms.  raw is
//   not read.
void DynamicsConductorFreePractice::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceOrganization *o = org;
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    u8 fault = c->st.fault;

    if (fault == 0) {
        u32 record = o->lapRecord.total;

        if (record != RACE_TIME_INVALID) {
            RaceDisplayTimeDiffEvent ev;    /* implicit: the inline constructor: RaceDisplayDiffEvent(), the vptr */

            ev.diff = split - record;
            ev.car = car;
            ev.post(o);
        }
        if (split < record) {
            RaceDisplayInformationEvent info;

            info.information = INFO_PROVISIONAL_POLE;
            info.car = car;
            info.post(o);
        }
    }
    dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
    DynCar_clearFaults(c);
    st->lapStartTime = ms;
}

// FUNC 0x001F23E0 DynamicsConductorFreePractice::virtual_1
//   The order reset: all -1, then every car at its entry's order (its rank
//   order + 1).
void DynamicsConductorFreePractice::virtual_1()
{
    int i;

    for (i = 7; i >= 0; i--)
        order[i] = -1;
    for (i = 0; i < carCount; i++) {
        int pos = org->entries->cars[i]->order;

        getCar(i)->st.rank = pos + 1;
        order[pos] = i;
    }
}

// FUNC 0x001F24C0 DynamicsConductorFreePractice::virtual_2
//   The order by best lap (an insertion sort, unsigned compare, stable),
//   then rank = position + 1.
void DynamicsConductorFreePractice::virtual_2()
{
    int i, j;

    for (i = 1; i < carCount; i++) {
        int key = order[i];

        for (j = i - 1; j >= 0; j--) {
            u32 other = car_best_lap(getCar(order[j]));

            if (!(car_best_lap(getCar(key)) < other))
                break;
            order[j + 1] = order[j];
        }
        order[j + 1] = key;
    }
    for (i = 0; i < carCount; i++)
        getCar(order[i])->st.rank = i + 1;
}

// FUNC 0x001F2630 DynamicsConductorFreePractice::virtual_34
int DynamicsConductorFreePractice::virtual_34()
{
    return 0;
}

// FUNC 0x001F2638 DynamicsConductorFreePractice::chargePenaltyBySurface
//   The course-out rule: none.
void DynamicsConductorFreePractice::chargePenaltyBySurface(RaceCarState *car, int left, int right)
{
}

// FUNC 0x001F2640 DynamicsConductorFreePractice::EveryTimePitStop
int DynamicsConductorFreePractice::EveryTimePitStop()
{
    return 1;
}

// FUNC 0x001F2648 DynamicsConductorFreePractice::processCollision_
//   collideAllCars (the base's calls collideWalls(0)).
void DynamicsConductorFreePractice::processCollision_()
{
    collideAllCars();
}

// FUNC 0x001F2668 DynamicsConductorFreePractice::SetSlowCarBoostParameters
//   No slow car boost.
int DynamicsConductorFreePractice::SetSlowCarBoostParameters(RaceRuleOptions *o)
{
    return 0;
}

// FUNC 0x001F2670 DynamicsConductorFreePractice::SetTireWearParameters
//   The tyre wear is on.
int DynamicsConductorFreePractice::SetTireWearParameters(RaceRuleOptions *o)
{
    return 1;
}

// FUNC 0x001F2678 DynamicsConductorFreePractice::CurrentLapTime
//   The car's time since the line.
u32 DynamicsConductorFreePractice::CurrentLapTime(RaceCarState *car)
{
    return lapTimeIfStarted(car);
}

// FUNC 0x001F2698 DynamicsConductorFreePractice::getStartingFormat
//   startAction -1.
int DynamicsConductorFreePractice::getStartingFormat()
{
    return START_FORMAT_NONE;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F26A0 __tf29DynamicsConductorFreePractice
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti29DynamicsConductorFreePractice (0x004B2890, base
//   __ti17DynamicsConductor) on the first call.
extern "C" const type_info &__tf29DynamicsConductorFreePractice(void)
{
    if (__ti29DynamicsConductorFreePractice[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti29DynamicsConductorFreePractice, "29DynamicsConductorFreePractice",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti29DynamicsConductorFreePractice;
}
#endif

// FUNC 0x001F26F0 RaceDisplayTimeDiffEvent::getEventType
/* inline in game/race/display/T154_race_display.h (0xF); egcs emits this copy (vtable copy
 * 0x0033D840, no callers) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001F26F8 __tf24RaceDisplayTimeDiffEvent
//   This unit's copy of T154's typeinfo function: builds the shared
//   __si_type_info __ti24RaceDisplayTimeDiffEvent (0x004B2170, base
//   __ti20RaceDisplayDiffEvent 0x004B2180) with its own name string
//   (0x0033D8C0).  Weak: T154 (u052) has the linked copy.
extern "C" __attribute__((weak)) const type_info &__tf24RaceDisplayTimeDiffEvent(void)
{
    if (__ti24RaceDisplayTimeDiffEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti24RaceDisplayTimeDiffEvent, "24RaceDisplayTimeDiffEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti24RaceDisplayTimeDiffEvent;
}
#endif
