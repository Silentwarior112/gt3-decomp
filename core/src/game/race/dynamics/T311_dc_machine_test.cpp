/* T311_dc_machine_test.cpp - DynamicsConductorMachineTest (0x001F05F8..0x001F0AD8).
 *
 * The conductor of the machine test (RaceMachineTest, T148: the courses
 * "M400" / "M1000" / "MMAX").  The rules' lap count picks the test
 * (testKind): MT_TEST_400M one line at 400 m, MT_TEST_1000M two (400 m,
 * 1000 m), MT_TEST_MAX_SPEED / MT_TEST_DEFAULT three (400 m, 1000 m, 6845 m;
 * g_002D75F0).  The line tests post the line time and check the record, the
 * max speed test posts the top speed.  There are no laps (slot 29 = 0) and
 * the lap end is ignored.
 *
 * Retail T311's .rodata (0x0033D040..): <new>'s "bad_alloc" (nothing
 * references it: DATA_START), the vtable, the type names; its .data: the
 * lines g_002D75F0, then the frame info.
 */
// DATA_START 0x0033D040
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/dynamics/T311_dc_machine_test.h"      /* last (dc_modes_internal.h) */

/* The lines of the test, m (the finish line last; -1: no line). */
f32 g_002D75F0[4] = { 400.0f, 1000.0f, 6845.0f, -1.0f }; /* 0x002D75F0 */

// FUNC 0x001F05F8 DynamicsConductorMachineTest::DynamicsConductorMachineTest
//   DynamicsConductor(), then the vptr (0x0033D050).  Caller: RaceMachineTest
//   (T148).
DynamicsConductorMachineTest::DynamicsConductorMachineTest()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F0638 DynamicsConductorMachineTest::virtual_29
//   The lap count: none.
int DynamicsConductorMachineTest::virtual_29()
{
    return 0;
}

// FUNC 0x001F0640 DynamicsConductorMachineTest::CurrentLapTime
//   The car's time: its best lap once it has finished, else the running
//   lap time.
u32 DynamicsConductorMachineTest::CurrentLapTime(RaceCarState *car)
{
    if (car->st.finished != 0)
        return car_best_lap(car);
    return lapTime(car);
}

// FUNC 0x001F0678 DynamicsConductorMachineTest::testKind
//   The test (MT_TEST_*): the base's lap count (rules.laps from the test
//   course's race record, a direct call of DynamicsConductor::virtual_29),
//   0 when it is 4 or more.  Not RaceMachineTest's kind (0 / 1 / 2, T148).
int DynamicsConductorMachineTest::testKind()
{
    int kind = DynamicsConductor::virtual_29();

    if (kind >= 4)
        kind = MT_TEST_DEFAULT;
    return kind;
}

// FUNC 0x001F06A0 DynamicsConductorMachineTest::getLapDistance
//   The car's course position: the predicted one.
f32 DynamicsConductorMachineTest::getLapDistance(RaceCarState *car)
{
    return noseDistance(car);
}

// FUNC 0x001F06C0 DynamicsConductorMachineTest::getGateCount
//   The lines (checkpoints) of the test: 1, 2 or 3.
int DynamicsConductorMachineTest::getGateCount()
{
    int lines = 1;

    switch (testKind()) {
    case MT_TEST_400M:
        break;
    case MT_TEST_1000M:
        lines = 2;
        break;
    case MT_TEST_DEFAULT:
    case MT_TEST_MAX_SPEED:
        lines = 3;
        break;
    }
    return lines;
}

// FUNC 0x001F0730 DynamicsConductorMachineTest::virtual_30
//   The finish line: the last line of the test (slot 32 is a virtual call).
f32 DynamicsConductorMachineTest::virtual_30()
{
    return g_002D75F0[getGateCount() - 1];
}

// FUNC 0x001F0780 DynamicsConductorMachineTest::getGatePosition
//   The position of checkpoint gate: the lines before the finish; -1.0
//   (g_002D75F0[3]) for the finish line and after.
f32 DynamicsConductorMachineTest::getGatePosition(int gate)
{
    int finish = getGateCount() - 1;

    if (!(gate < finish))
        gate = 3;
    return g_002D75F0[gate];
}

// FUNC 0x001F07E0 DynamicsConductorMachineTest::virtual_15
//   A line passed: its section time events (the lap not decremented, unlike
//   the base), unless a menu runs over the race.  raw is not read.
void DynamicsConductorMachineTest::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceOrganization *o = org;
    RaceCarState *c = getCar(car);

    if (o->backgroundMode == 0)
        dc_post_section_time(this, car, lap, gate, ms, split, c->st.fault);
}

// FUNC 0x001F0880 DynamicsConductorMachineTest::virtual_17
//   A lap's end: nothing.
void DynamicsConductorMachineTest::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
}

// FUNC 0x001F0888 DynamicsConductorMachineTest::virtual_18
//   The finish line: the top speed is kept; unless a menu runs over the
//   race (then 0) the car's run ends, the record of a line test is checked
//   (a car with a controller, mode 0), the line time (line tests) or the top
//   speed (the max speed test) is posted and, for car 0, the goal event.
//   1.  raw is not read.
int DynamicsConductorMachineTest::virtual_18(int car, int lap, u32 raw, u32 ms, u32 split)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    RaceOrganization *o = org;
    int kind;

    if (c->topSpeed < c->speed)
        c->topSpeed = c->speed;
    if (o->backgroundMode != 0)
        return 0;

    car_finish(c, FINISH_RUN_ENDS);
    if (st->noController == 0 && mode == 0) {
        kind = testKind();
        if (kind < MT_TEST_MAX_SPEED) {
            if (kind > MT_TEST_DEFAULT)
                checkLapRecord(car, split);
        }
    }

    switch (testKind()) {
    case MT_TEST_400M:
    case MT_TEST_1000M:
        dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
        break;
    case MT_TEST_DEFAULT:
    case MT_TEST_MAX_SPEED:
        dc_post_speed(this, car, c->topSpeed, c->st.fault);
        break;
    }

    if (car == 0)
        org->eventQueue.post(RACE_EVENT_GOAL, 0, 2);
    return 1;
}

// FUNC 0x001F0A08 DynamicsConductorMachineTest::virtual_26
//   The base's (empty), then for a line test the run result.
void DynamicsConductorMachineTest::virtual_26()
{
    int kind;

    DynamicsConductor::virtual_26();
    kind = testKind();
    if (kind < MT_TEST_MAX_SPEED) {
        if (kind > MT_TEST_DEFAULT)
            checkRunResult();
    }
}

// FUNC 0x001F0A58 DynamicsConductorMachineTest::getEachEntrantInfo_
//   getEntrantInfoTrial (as License, FreeRun and TimeTrial).
void DynamicsConductorMachineTest::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoTrial(car, tyreFlag, b, c, d);
}

// FUNC 0x001F0A78 DynamicsConductorMachineTest::isDataLoggerStandingStart
int DynamicsConductorMachineTest::isDataLoggerStandingStart()
{
    return 1;
}

// FUNC 0x001F0A80 DynamicsConductorMachineTest::isDataLoggerSupported
int DynamicsConductorMachineTest::isDataLoggerSupported()
{
    return 1;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F0A88 __tf28DynamicsConductorMachineTest
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti28DynamicsConductorMachineTest (0x004B2850, base
//   __ti17DynamicsConductor gp-0x45E0) on the first call.
extern "C" const type_info &__tf28DynamicsConductorMachineTest(void)
{
    if (__ti28DynamicsConductorMachineTest[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti28DynamicsConductorMachineTest, "28DynamicsConductorMachineTest",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti28DynamicsConductorMachineTest;
}
#endif
