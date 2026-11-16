/* T312_dc_license.cpp - DynamicsConductorLicense (0x001F0AD8..0x001F1190).
 *
 * The conductor of the license tests.  rules.launchSpeed non-zero: a
 * rolling start (the clock of the lap starts at the line); rules.licenseType
 * 2: the test ends at the first finish line; 3: multi-lap, a lap end posts
 * as a section time and the last lap ends the test.  A lap of k checkpoints
 * counts as k sections in a row (slots 7, 15, 17).  A course out or a
 * collision fails the test (slot 13 licenseCheckStop, slots 36..40 the
 * rules).  The rules are read through the inline getRules() (retail keeps
 * &rules in a register, then the field offset).
 *
 * Retail T312's .rodata opens with <new>'s "bad_alloc" (0x0033D1E0, nothing
 * references it: DATA_START), then the vtable and the type names.
 */
// DATA_START 0x0033D1E0
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/dynamics/T312_dc_license.h"           /* last (dc_modes_internal.h) */

// FUNC 0x001F0AD8 DynamicsConductorLicense::DynamicsConductorLicense
//   DynamicsConductor(), then the vptr (0x0033D1F0).  Caller: RaceLicense
//   (T144).
DynamicsConductorLicense::DynamicsConductorLicense()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F0B18 DynamicsConductorLicense::initialize
//   The base's, then the race result reset: no time, rank -2.
void DynamicsConductorLicense::initialize(int mode)
{
    DynamicsConductor::initialize(mode);
    org->result.rank = -2;
    org->result.time = RACE_TIME_INVALID;
}

// FUNC 0x001F0B58 DynamicsConductorLicense::virtual_26
//   The base's (empty), then the run result.
void DynamicsConductorLicense::virtual_26()
{
    DynamicsConductor::virtual_26();
    checkRunResult();
}

// FUNC 0x001F0B88 DynamicsConductorLicense::virtual_29
//   The base's lap count (rules.laps).
int DynamicsConductorLicense::virtual_29()
{
    return DynamicsConductor::virtual_29();
}

// FUNC 0x001F0BA8 rules_rolling_start
//   1 when the rules give a launch speed: a rolling start (the base's
//   getStartingFormat and License's isDataLoggerStandingStart test the
//   opposite).  Callers: CurrentLapTime, virtual_16.
int rules_rolling_start(RaceRuleOptions *r)
{
    return r->launchSpeed != 0;
}

// FUNC 0x001F0BB8 DynamicsConductorLicense::isDataLoggerStandingStart
//   A standing start: no launch speed.
int DynamicsConductorLicense::isDataLoggerStandingStart()
{
    return getRules()->launchSpeed == 0;
}

// FUNC 0x001F0BD0 DynamicsConductorLicense::isDataLoggerMultiLap
//   A multi-lap test (licenseType 3) of more than one lap.
int DynamicsConductorLicense::isDataLoggerMultiLap()
{
    if (getRules()->licenseType != 3)
        return 0;
    return getRules()->laps != 1;
}

// FUNC 0x001F0C00 DynamicsConductorLicense::virtual_7
//   The car's logger section: the base's unless multi-lap (slot 6 is a
//   virtual call); else the sections of the laps run plus the next one,
//   less one with flag.
int DynamicsConductorLicense::virtual_7(RaceCarState *car, int flag)
{
    int lap, section;

    if (isDataLoggerMultiLap() == 0)
        return DynamicsConductor::virtual_7(car, flag);
    lap = car->st.lap;
    section = (lap - 1) * dc_section_count(this) + car->st.section;
    if (flag != 0)
        section--;
    return section;
}

// FUNC 0x001F0C90 DynamicsConductorLicense::CurrentLapTime
//   The car's time: its best lap once finished; else the running lap time
//   (a standing start) or the time since the line (a rolling start).
u32 DynamicsConductorLicense::CurrentLapTime(RaceCarState *car)
{
    if (car->st.finished != 0)
        return car_best_lap(car);
    if (rules_rolling_start(getRules()) == 0)
        return lapTime(car);
    return lapTimeIfStarted(car);
}

// FUNC 0x001F0D00 DynamicsConductorLicense::getSpecialStart
//   Start type 2 if it applies (rollingStart); else start type 1 (startAtLaunchPoint)
//   and the rules' start speed (startWithLaunchSpeed, the base's slot 24).  Returns
//   the result of the last call.
int DynamicsConductorLicense::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    int done = rollingStart(car, startType, startArg, speedKmh);

    if (done == 0) {
        startAtLaunchPoint(car, startType, startArg, speedKmh);
        done = startWithLaunchSpeed(car, startType, startArg, speedKmh);
    }
    return done;
}

// FUNC 0x001F0D90 DynamicsConductorLicense::virtual_15
//   A checkpoint: posted as section (lap - 1) x checkpoints + gate of lap 1
//   (slot 32 is a virtual call).  raw is not read.
void DynamicsConductorLicense::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceCarState *c = getCar(car);
    int gates = getGateCount();

    dc_post_section_time(this, car, 1, (lap - 1) * gates + gate, ms, split, c->st.fault);
}

// FUNC 0x001F0E40 DynamicsConductorLicense::virtual_16
//   The start line, for a rolling start: the lap starts at ms, the driver
//   gets the car (control mode 0) and, for car 0, the start event.
void DynamicsConductorLicense::virtual_16(int car, u32 ms, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;

    if (rules_rolling_start(getRules()) != 0) {
        st->lapStartTime = ms;
        car_mode_driver(c);
        if (car == 0)
            org->eventQueue.post(RACE_EVENT_START, 0, 0);
    }
}

// FUNC 0x001F0ED8 DynamicsConductorLicense::virtual_17
//   A lap's end: in a multi-lap test the last lap (slot 29) ends the test
//   with split; any other lap end posts as the section lap x checkpoints - 1
//   (slot 32).  raw is not read.
void DynamicsConductorLicense::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);

    if (getRules()->licenseType == 3 && virtual_29() == lap) {
        licenseFinished(c, split);
        return;
    }
    dc_post_section_time(this, car, lap, lap * getGateCount() - 1, ms, split, c->st.fault);
}

// FUNC 0x001F0FC8 DynamicsConductorLicense::virtual_18
//   The finish line: a licenseType 2 test ends with split (1); else 0.
int DynamicsConductorLicense::virtual_18(int car, int lap, u32 raw, u32 ms, u32 split)
{
    if (getRules()->licenseType == 2) {
        licenseFinished(getCar(car), split);
        return 1;
    }
    return 0;
}

// FUNC 0x001F1030 DynamicsConductorLicense::virtual_12
//   Nothing per frame.
void DynamicsConductorLicense::virtual_12()
{
}

// FUNC 0x001F1038 DynamicsConductorLicense::virtual_13
//   The stop / fault check of every car, then the base's race clock.
void DynamicsConductorLicense::virtual_13()
{
    int i;

    for (i = 0; i < carCount; i++)
        licenseCheckStop(getCar(i));
    DynamicsConductor::virtual_13();
}

// FUNC 0x001F10B8 DynamicsConductorLicense::virtual_34
int DynamicsConductorLicense::virtual_34()
{
    return 3;
}

// FUNC 0x001F10C0 DynamicsConductorLicense::virtual_35
int DynamicsConductorLicense::virtual_35()
{
    return 2;
}

// FUNC 0x001F10C8 DynamicsConductorLicense::virtual_36
//   The course-out level of all four wheels on counted ground.
int DynamicsConductorLicense::virtual_36()
{
    return 1;
}

// FUNC 0x001F10D0 DynamicsConductorLicense::virtual_37
//   The course-out level of all four wheels off the course: 1 unless on
//   a dirt course.
int DynamicsConductorLicense::virtual_37()
{
    return isDirtCourse() == 0;
}

// FUNC 0x001F10F0 DynamicsConductorLicense::isPitEnabled
int DynamicsConductorLicense::isPitEnabled()
{
    return 0;
}

// FUNC 0x001F10F8 DynamicsConductorLicense::issueCrashEvent
//   The collision rule of the tests (F_001DA130, T324).
void DynamicsConductorLicense::issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force)
{
    F_001DA130(car, kind, arg, force);
}

// FUNC 0x001F1118 DynamicsConductorLicense::getEachEntrantInfo_
//   getEntrantInfoTrial.
void DynamicsConductorLicense::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoTrial(car, tyreFlag, b, c, d);
}

// FUNC 0x001F1138 DynamicsConductorLicense::isDataLoggerSupported
int DynamicsConductorLicense::isDataLoggerSupported()
{
    return 1;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F1140 __tf24DynamicsConductorLicense
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti24DynamicsConductorLicense (0x004B2860, base __ti17DynamicsConductor)
//   on the first call.
extern "C" const type_info &__tf24DynamicsConductorLicense(void)
{
    if (__ti24DynamicsConductorLicense[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti24DynamicsConductorLicense, "24DynamicsConductorLicense",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti24DynamicsConductorLicense;
}
#endif
