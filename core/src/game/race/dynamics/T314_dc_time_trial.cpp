/* T314_dc_time_trial.cpp - DynamicsConductorTimeTrial (0x001F1AD8..0x001F2010).
 *
 * The conductor of a time trial run (RaceTimeTrial, T140): one lap (slot 29
 * = 1), the run ends at the first lap's end (slot 17), without a start
 * countdown.  A collision or a course out of car 0 (DynCarState.fault, set
 * by the rules of slots 36..40) disqualifies the run (slot 12 ->
 * disqualify: the event RaceTimeTrialDisplay (T141) shows, car_finish 2).
 *
 * Retail T314's .rodata opens with <new>'s "bad_alloc" (0x0033D538, nothing
 * references it: DATA_START), then the vtable and the type names.
 */
// DATA_START 0x0033D538
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/dynamics/T314_dc_time_trial.h"        /* last (dc_modes_internal.h) */

// FUNC 0x001F1AD8 DynamicsConductorTimeTrial::DynamicsConductorTimeTrial
//   DynamicsConductor(), then the vptr (0x0033D548).  Caller: RaceTimeTrial
//   (T140).
DynamicsConductorTimeTrial::DynamicsConductorTimeTrial()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F1B18 DynamicsConductorTimeTrial::virtual_20
//   The base's.
void DynamicsConductorTimeTrial::virtual_20(int a)
{
    DynamicsConductor::virtual_20(a);
}

// FUNC 0x001F1B38 DynamicsConductorTimeTrial::virtual_21
//   The base's.
void DynamicsConductorTimeTrial::virtual_21(int a)
{
    DynamicsConductor::virtual_21(a);
}

// FUNC 0x001F1B58 DynamicsConductorTimeTrial::getEachEntrantInfo_
//   getEntrantInfoTrial.
void DynamicsConductorTimeTrial::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoTrial(car, tyreFlag, b, c, d);
}

// FUNC 0x001F1B78 DynamicsConductorTimeTrial::virtual_29
//   The lap count: 1.
int DynamicsConductorTimeTrial::virtual_29()
{
    return 1;
}

// FUNC 0x001F1B80 DynamicsConductorTimeTrial::CurrentLapTime
//   The car's time: its best lap once finished, else the time since the
//   line.
u32 DynamicsConductorTimeTrial::CurrentLapTime(RaceCarState *car)
{
    if (car->st.finished != 0)
        return car_best_lap(car);
    return lapTimeIfStarted(car);
}

// FUNC 0x001F1BB8 DynamicsConductorTimeTrial::disqualify
//   Unless a menu runs over the race or the car has finished: for car index
//   0 the disqualification event (RACE_EVENT_DISQUALIFIED, arg the index, 0;
//   RaceTimeTrialDisplay plays its sequence), then the car's run ends as
//   disqualified (car_finish 2; DynamicsConductor::licenseFailed, T352, does
//   the same with event 9).  reason (0 a course out, 1 a collision) is not
//   read.  Caller: virtual_12.
void DynamicsConductorTimeTrial::disqualify(RaceCarState *car, int reason)
{
    RaceOrganization *o = org;
    DynCarState *st;

    if (o->backgroundMode != 0)
        return;
    st = &car->st;
    if (st->finished != 0)
        return;
    if (st->carIndex == 0)
        o->eventQueue.post(RACE_EVENT_DISQUALIFIED, st->carIndex, 0);
    car_finish(car, FINISH_DISQUALIFIED);
}

// FUNC 0x001F1C20 DynamicsConductorTimeTrial::virtual_15
//   A checkpoint: the difference to the reference lap's section, then the
//   section time events (lap - 1, as the base).  raw is not read.
void DynamicsConductorTimeTrial::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceCarState *c = getCar(car);

    dc_post_section_diff(this, car, gate, split);
    dc_post_section_time(this, car, lap - 1, gate, ms, split, c->st.fault);
}

// FUNC 0x001F1CC0 DynamicsConductorTimeTrial::virtual_16
//   The start line: a car driven by the pad gets it back (control mode 0)
//   and the start event (arg car); the lap starts at ms.
void DynamicsConductorTimeTrial::virtual_16(int car, u32 ms, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;

    if (st->driver == 0) {
        car_mode_driver(c);
        org->eventQueue.post(RACE_EVENT_START, car, 0);
    }
    st->lapStartTime = ms;
}

// FUNC 0x001F1D40 DynamicsConductorTimeTrial::virtual_17
//   A lap's end, unless a menu runs over the race: the run ends; for a car
//   with a controller the finish timer (car 0), the goal event (arg car,
//   delay 2) and the lap record check (mode 0); then the lap difference and
//   lap time events; the lap starts at ms.  raw is not read.
void DynamicsConductorTimeTrial::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    RaceOrganization *o = org;
    DynCarState *st = &c->st;

    if (o->backgroundMode != 0)
        return;
    car_finish(c, FINISH_RUN_ENDS);
    if (st->noController == 0) {
        if (car == 0)
            setFinishTimer(TT_FINISH_SECONDS);
        o->eventQueue.post(RACE_EVENT_GOAL, car, 2);
        if (mode == 0)
            checkLapRecord(st->carIndex, split);
    }
    dc_post_lap_diff(this, car, split);
    dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
    st->lapStartTime = ms;
}

// FUNC 0x001F1E50 DynamicsConductorTimeTrial::virtual_12
//   Per frame: a course out or a collision of car 0 disqualifies the run
//   (the course out first).
void DynamicsConductorTimeTrial::virtual_12()
{
    RaceCarState *c = getCar(0);
    u8 fault = c->st.fault;

    if (fault & FAULT_COURSE_OUT)
        disqualify(c, 0);
    else if (fault & FAULT_COLLISION)
        disqualify(c, 1);
}

// FUNC 0x001F1EB0 DynamicsConductorTimeTrial::virtual_13
//   The base's race clock.
void DynamicsConductorTimeTrial::virtual_13()
{
    DynamicsConductor::virtual_13();
}

// FUNC 0x001F1ED0 DynamicsConductorTimeTrial::virtual_26
//   The base's (empty), then the run result.
void DynamicsConductorTimeTrial::virtual_26()
{
    DynamicsConductor::virtual_26();
    checkRunResult();
}

// FUNC 0x001F1F00 DynamicsConductorTimeTrial::virtual_34
//   3 unless on course kind 2.
int DynamicsConductorTimeTrial::virtual_34()
{
    return (isDirtCourse() != 0) ? 0 : 3;
}

// FUNC 0x001F1F28 DynamicsConductorTimeTrial::virtual_35
int DynamicsConductorTimeTrial::virtual_35()
{
    return 2;
}

// FUNC 0x001F1F30 DynamicsConductorTimeTrial::virtual_36
//   The course-out level of all four wheels on counted ground.
int DynamicsConductorTimeTrial::virtual_36()
{
    return 1;
}

// FUNC 0x001F1F38 DynamicsConductorTimeTrial::virtual_37
//   The course-out level of all four wheels off the course: 1 unless on
//   a dirt course.
int DynamicsConductorTimeTrial::virtual_37()
{
    return isDirtCourse() == 0;
}

// FUNC 0x001F1F58 DynamicsConductorTimeTrial::isPitEnabled
int DynamicsConductorTimeTrial::isPitEnabled()
{
    return 0;
}

// FUNC 0x001F1F60 DynamicsConductorTimeTrial::issueCrashEvent
//   The collision rule (F_001DA130, T324), as License's.
void DynamicsConductorTimeTrial::issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force)
{
    F_001DA130(car, kind, arg, force);
}

// FUNC 0x001F1F80 DynamicsConductorTimeTrial::getSpecialStart
//   Slot 25 (a virtual call) with the same arguments.  The code sets no
//   result: $v0 is what slot 25 left (initialize ignores it).  The
//   original had no return statement; GCC 14 returns 0, so that the
//   function does not fall off its end.
int DynamicsConductorTimeTrial::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    virtual_25(car, startType, startArg, speedKmh);
#ifndef BOOT_OLD_GCC
    return 0;
#endif
}

// FUNC 0x001F1FB0 DynamicsConductorTimeTrial::getStartingFormat
//   No start countdown (startAction 3).
int DynamicsConductorTimeTrial::getStartingFormat()
{
    return START_FORMAT_NO_COUNTDOWN;
}

// FUNC 0x001F1FB8 DynamicsConductorTimeTrial::isDataLoggerSupported
int DynamicsConductorTimeTrial::isDataLoggerSupported()
{
    return 1;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F1FC0 __tf26DynamicsConductorTimeTrial
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti26DynamicsConductorTimeTrial (0x004B2880, base
//   __ti17DynamicsConductor) on the first call.
extern "C" const type_info &__tf26DynamicsConductorTimeTrial(void)
{
    if (__ti26DynamicsConductorTimeTrial[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti26DynamicsConductorTimeTrial, "26DynamicsConductorTimeTrial",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti26DynamicsConductorTimeTrial;
}
#endif
