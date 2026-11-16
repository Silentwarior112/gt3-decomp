/* T303 - the car's best lap and the race display time events:
 * 0x001EC4B0..0x001ECBA8.
 *
 * The conductors' gate handlers (slots 15 / 17 and their overrides) post,
 * through these, the display events of a car's section and lap times
 * (RaceDisplayCheckPointEvent / LapTimeEvent followed by a
 * RaceDisplayGeneralTimeEvent with the time), the differences to the race's
 * reference lap (RaceDisplayTimeDiffEvent), the speed (SpeedEvent) and the
 * lap differences (LapDiffEvent), and record the times in the race
 * organization.  The T154 event classes' getEventType and typeinfo
 * functions are emitted here again (inline functions).
 *
 * The T154 events have inline constructors: each builds the
 * RaceDisplayDiffEvent base (its constructor 0x00190500 is called) and
 * stores T154's vtable.
 *
 * Retail T303's .rodata (0x0033C8E8..0x0033C9E1; "bad_alloc" first, which
 * nothing references: DATA_START): "bad_alloc", the three event vtables,
 * then the type names 27RaceDisplayGeneralTimeEvent, 23RaceDisplayLapDiffEvent,
 * 24RaceDisplayTimeDiffEvent, 20RaceDisplayEventBase, t9UnitArena1Ui64
 * (reverse definition order: T153 is read before T194). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#define GAME_T194_EMITS_TYPEINFO            /* 20RaceDisplayEventBase's type name here too, as retail
                                               (after the three events'; T315 / T316 alike) */
#include "game/race/display/T194_race_display_event.h"   /* the events; T154's GeneralTime / LapDiff / TimeDiff
                                               (only inline virtuals: their names) */
#include "game/car/physics/car_motion_internal.h"
#include "game/race/dynamics/T303_race_time_events.h"
#ifndef BOOT_OLD_GCC                        /* (types.h defines it: after the headers) */
#include "cxxrt.h"                          /* __rtti_si (the typeinfo copies below; egcs's <new> and
                                               cxxrt.h conflict, egcs generates them itself) */
#endif
// DATA_START 0x0033C8E8

/* From DynCar_initialize (T330). */
// FUNC 0x001EC4B0 car_reset_best_lap
void car_reset_best_lap(RaceCarState *car)
{
    car->bestLap = RACE_TIME_INVALID;
}

/* 1 when t beats the car's best lap (unsigned). */
// FUNC 0x001EC4C0 car_update_best_lap
int car_update_best_lap(RaceCarState *car, u32 t)
{
    if (t >= car->bestLap)
        return 0;
    car->bestLap = t;
    return 1;
}

// FUNC 0x001EC4E0 car_best_lap
u32 car_best_lap(RaceCarState *car)
{
    return car->bestLap;
}

/* 1 when the car's times are not recorded: no controller, the conductor's
 * mode (a replay), a race preview or the background race. */
// FUNC 0x001EC4E8 race_times_hidden
int race_times_hidden(RaceOrganization *org, DynamicsConductor *dc, RaceCarState *car)
{
    if (car->st.noController != 0)
        return 1;
    if (dc->mode != 0)
        return 1;
    if (org->racePreviewMode != 0)
        return 1;
    return org->backgroundMode != 0;
}

/* From the slot 15 handlers (lap = their lap - 1, noRecord = the car's
 * st.fault).  The CheckPoint and GeneralTime events, then the
 * organization's section record.  unused4 is passed by every caller and not
 * read. */
// FUNC 0x001EC520 dc_post_section_time
void dc_post_section_time(DynamicsConductor *dc, int car, int lap, int section, u32 unused4, u32 time, int noRecord)
{
    RaceCarState *c = dc->getCar(car);
    RaceOrganization *org = dc->org;
    int record = race_times_hidden(org, dc, c) == 0;

    RaceDisplayCheckPointEvent checkPoint;
    checkPoint.lap = lap;
    checkPoint.car = car;
    checkPoint.checkPoint = section;
    checkPoint.post(org);

    RaceDisplayGeneralTimeEvent timeEvent;     /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    timeEvent.car = car;
    timeEvent.setTime(time);
    timeEvent.post(org);

    if (noRecord == 0 && record != 0)
        org->recordSectionTime(car, section, time);
}

/* From the slot 17 handlers.  The car's best lap, the lap time and
 * general time events, then the lap time list of the entry, its best lap
 * and the organization's record.  unused3 is not read. */
// FUNC 0x001EC620 dc_post_lap_time
void dc_post_lap_time(DynamicsConductor *dc, int car, int lap, u32 unused3, u32 time, int noRecord)
{
    RaceCarState *c = dc->getCar(car);
    car_update_best_lap(c, time);
    RaceOrganization *org = dc->org;
    int record = race_times_hidden(org, dc, c) == 0;

    RaceDisplayLapTimeEvent lapEvent;
    lapEvent.lap = lap;
    lapEvent.car = car;
    lapEvent.post(org);

    RaceDisplayGeneralTimeEvent timeEvent;     /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    timeEvent.car = car;
    timeEvent.setTime(time);
    timeEvent.post(org);

    if (record != 0) {
        org->entries->cars[car]->lapTime.add(time);
        if (noRecord == 0) {
            org->entries->cars[car]->timeRecord.update(time);
            org->recordLapTime(car, time, record);
        }
    }
}

/* From FreeRun / TimeTrial slot 15: the time minus the reference lap's
 * section time, unless the conductor's mode, the car's st.fault or no
 * reference. */
// FUNC 0x001EC758 dc_post_section_diff
void dc_post_section_diff(DynamicsConductor *dc, int car, int section, u32 time)
{
    if (dc->mode != 0)
        return;
    if (dc->getCar(car)->st.fault != 0)
        return;
    RaceOrganization *org = dc->org;
    u32 ref = org->lapRecord.section[section];
    if (ref == RACE_TIME_INVALID)
        return;
    RaceDisplayTimeDiffEvent diffEvent;        /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    diffEvent.car = car;
    diffEvent.diff = time - ref;
    diffEvent.post(org);
}

/* From FreeRun / TimeTrial slot 17: the same against the reference lap's
 * total. */
// FUNC 0x001EC818 dc_post_lap_diff
void dc_post_lap_diff(DynamicsConductor *dc, int car, u32 time)
{
    if (dc->mode != 0)
        return;
    if (dc->getCar(car)->st.fault != 0)
        return;
    RaceOrganization *org = dc->org;
    u32 ref = org->lapRecord.total;
    if (ref == RACE_TIME_INVALID)
        return;
    RaceDisplayTimeDiffEvent diffEvent;        /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    diffEvent.car = car;
    diffEvent.diff = time - ref;
    diffEvent.post(org);
}

/* From DynamicsConductorMachineTest::virtual_18: the entry's top speed
 * (when recorded) and a speed event in the speed unit.  noRecord (the
 * caller passes st.fault) is not read. */
// FUNC 0x001EC8C0 dc_post_speed
void dc_post_speed(DynamicsConductor *dc, int car, f32 speed, int noRecord)
{
    RaceOrganization *org = dc->org;

    if (race_times_hidden(org, dc, dc->getCar(car)) == 0)
        org->entries->cars[car]->topSpeed = speed;
    RaceDisplaySpeedEvent speedEvent;
    speedEvent.car = car;
    speedEvent.speed = g_002F59D8.convSpeed(speed);
    speedEvent.post(org);
}

/* Empty (from DynamicsConductor::reportGateTimes, T307). */
// FUNC 0x001EC960 F_001EC960
void F_001EC960(DynamicsConductor *dc, int car)
{
}

/* Empty (from reportGateTimes). */
// FUNC 0x001EC968 F_001EC968
void F_001EC968(DynamicsConductor *dc, int car)
{
}

/* Empty (from reportGateTimes). */
// FUNC 0x001EC970 F_001EC970
void F_001EC970(DynamicsConductor *dc, int car)
{
}

/* Empty (from reportGateTimes). */
// FUNC 0x001EC978 F_001EC978
void F_001EC978(DynamicsConductor *dc, int car, int diff)
{
}

/* From reportGateTimes: a TimeDiff event of diff ms. */
// FUNC 0x001EC980 dc_post_time_diff
void dc_post_time_diff(DynamicsConductor *dc, int car, int diff)
{
    RaceOrganization *org = dc->org;
    RaceDisplayTimeDiffEvent diffEvent;        /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    diffEvent.car = car;
    diffEvent.diff = diff;
    diffEvent.post(org);
}

/* From reportGateTimes; the same code as dc_post_time_diff. */
// FUNC 0x001EC9E0 dc_post_time_diff2
void dc_post_time_diff2(DynamicsConductor *dc, int car, int diff)
{
    RaceOrganization *org = dc->org;
    RaceDisplayTimeDiffEvent diffEvent;        /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    diffEvent.car = car;
    diffEvent.diff = diff;
    diffEvent.post(org);
}

/* From DynamicsConductor::recordGate (T308): a LapDiff event of diff
 * laps. */
// FUNC 0x001ECA40 dc_post_lap_count_diff
void dc_post_lap_count_diff(DynamicsConductor *dc, int car, int diff)
{
    RaceOrganization *org = dc->org;
    RaceDisplayLapDiffEvent diffEvent;         /* implicit: base ctor RaceDisplayDiffEvent(), T154's vtable */
    diffEvent.car = car;
    diffEvent.diff = diff;
    diffEvent.post(org);
}

/* ======================================================================== */
/* Out-of-line copies of the T154 event classes' inline functions          */
/* ======================================================================== */

// FUNC 0x001ECAA0 RaceDisplayGeneralTimeEvent::getEventType
/* inline in game/race/display/T154_race_display.h (0xE); egcs emits this copy */

/* __si_type_info at 0x004B2190, base __ti20RaceDisplayDiffEvent (this
 * unit's copy of the name string, 0x0033C958).  Weak so that T154's copy
 * links too. */
#ifndef BOOT_OLD_GCC
// FUNC 0x001ECAA8 __tf27RaceDisplayGeneralTimeEvent
extern "C" __attribute__((weak)) const type_info &__tf27RaceDisplayGeneralTimeEvent(void)
{
    if (__ti27RaceDisplayGeneralTimeEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti27RaceDisplayGeneralTimeEvent, "27RaceDisplayGeneralTimeEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti27RaceDisplayGeneralTimeEvent;
}
#endif

// FUNC 0x001ECAF8 RaceDisplayLapDiffEvent::getEventType
/* inline in game/race/display/T154_race_display.h (0x10); egcs emits this copy */

/* __si_type_info at 0x004B21C0 (name string 0x0033C978).  Weak (T154's
 * copy). */
#ifndef BOOT_OLD_GCC
// FUNC 0x001ECB00 __tf23RaceDisplayLapDiffEvent
extern "C" __attribute__((weak)) const type_info &__tf23RaceDisplayLapDiffEvent(void)
{
    if (__ti23RaceDisplayLapDiffEvent[0] == 0) {
        __tf20RaceDisplayDiffEvent();
        __rtti_si(__ti23RaceDisplayLapDiffEvent, "23RaceDisplayLapDiffEvent",
                  (const type_info *)__ti20RaceDisplayDiffEvent);
    }
    return *(const type_info *)__ti23RaceDisplayLapDiffEvent;
}
#endif

// FUNC 0x001ECB50 RaceDisplayTimeDiffEvent::getEventType
/* inline in game/race/display/T154_race_display.h (0xF); egcs emits this copy */

/* __si_type_info at 0x004B2170 (name string 0x0033C998).  Weak (T154's and
 * T315's copies). */
#ifndef BOOT_OLD_GCC
// FUNC 0x001ECB58 __tf24RaceDisplayTimeDiffEvent
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
