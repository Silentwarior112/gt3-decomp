/* T308 (0x001EF8B8..0x001EFFD0): DynamicsConductor - the lap count (slot 29),
 * lap times, the battles' checkpoint time table (recordGate), the race
 * clock (slot 13), checkpoints / lap ends (slots 15..18), and the record
 * checks of the time trial / license / machine test results.
 *
 * Retail T308's .rodata (0x0033CCC0..0x0033CCF9; nothing references it:
 * DATA_START): "bad_alloc", the type names 14RaceCameraBase,
 * t9UnitArena1Ui64 (T153 is read before T129: RaceBase, checkRunResult). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/core/T129_race_base.h"            /* RaceBase::virtual_26 (the player's RaceInput);
                                               (seen: RaceCameraBase) */
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033CCC0

// FUNC 0x001EF8B8 DynamicsConductor::virtual_29
/* The race's lap count. */
int DynamicsConductor::virtual_29()
{
    return rules.laps;
}

// FUNC 0x001EF8C8 DynamicsConductor::lapTime
/* ms since the car's last lap end. */
u32 DynamicsConductor::lapTime(RaceCarState *car)
{
    u32 start = car->st.lapStartTime;

    return clock / 3 - start;
}

// FUNC 0x001EF8F8 DynamicsConductor::lapTimeIfStarted
/* lapTime, 0 before the start line. */
u32 DynamicsConductor::lapTimeIfStarted(RaceCarState *car)
{
    if (car->st.lap == 0)
        return 0;
    return lapTime(car);
}

// FUNC 0x001EF920 DynamicsConductor::recordGate
/* The battles' slot 14: car passed checkpoint gate of lap at time ms.  A
 * newer lap restarts the checkpoint's list; a car a lap or more behind gets
 * its lap gap shown (dc_post_lap_count_diff); else the time is sorted into
 * the list after the times already reported.  Bug kept: the insertion moves
 * the later times up but not their cars (gateCar[]), so a car is lost.
 * 90.7%: retail branches to the scan when count != done and lays the
 * skip path (with the insertion loop's hoisted gate x 32 / x 8) first; the
 * guard as written jumps over the scan.  Tried: while / for / do forms,
 * the guard's sense, early returns instead of else-if. */
void DynamicsConductor::recordGate(int car, int lap, int gate, u32 time)
{
    int listLap = gateLap[gate];
    int slot, i;

    if (listLap < lap) {
        gateLap[gate] = lap;
        gateCount[gate] = 1;
        gateDone[gate] = 0;
        gateTime[gate][0] = time;
        gateCar[gate][0] = car;
    } else if (lap == listLap) {
        slot = gateCount[gate];
        if (slot != gateDone[gate]) {           /* (retail tests it first) */
            while (gateDone[gate] < slot) {
                if (!(time < gateTime[gate][slot - 1]))
                    break;
                slot--;
            }
        }
        for (i = gateCount[gate]; slot < i; i--)
            gateTime[gate][i] = gateTime[gate][i - 1];
        gateTime[gate][slot] = time;
        gateCar[gate][slot] = car;
        gateCount[gate]++;
    } else {
        dc_post_lap_count_diff(this, car, listLap - lap);
    }
}

// FUNC 0x001EFAE8 DynamicsConductor::virtual_15
/* A checkpoint of the lap: its time events, recorded unless the car's fault
 * byte is set (raw is not read). */
void DynamicsConductor::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceCarState *c = getCar(car);

    dc_post_section_time(this, car, lap - 1, gate, ms, split, c->st.fault);
}

// FUNC 0x001EFB68 DynamicsConductor::virtual_16
/* Empty: the start line. */
void DynamicsConductor::virtual_16(int car, u32 ms, int frac50, f32 t)
{
}

// FUNC 0x001EFB70 DynamicsConductor::virtual_17
/* The end of a lap.  The car's race ends when the leader has finished, the
 * lap count is reached or the time limit is over: event 5, car_finish(3),
 * and for car 0 setFinishTimer(3).  The next lap starts at ms. */
void DynamicsConductor::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    int laps, ends, over;
    u32 limit;

    dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
    laps = virtual_29();
    ends = 0;
    if (getCar(getOrder(0))->st.finished != 0)
        ends = 1;
    if (laps != 0 && lap == laps)
        ends = 1;
    limit = rules.timeLimit * 60000;
    over = 0;
    if (limit != 0 && !(ms < limit))
        over = 1;
    if (over != 0)
        ends = 1;
    if (ends != 0) {
        org->eventQueue.post(RACE_EVENT_GOAL, car, 2);
        car_finish(getCar(car), 3);
        if (car == 0)
            setFinishTimer(3);
    }
    DynCar_clearFaults(c);
    st->lapStartTime = ms;
}

// FUNC 0x001EFCC8 DynamicsConductor::virtual_18
/* The slot 30 line: 0, the logger section is not closed there. */
int DynamicsConductor::virtual_18(int car, int lap, u32 raw, u32 ms, u32 split)
{
    return 0;
}

// FUNC 0x001EFCD0 DynamicsConductor::virtual_12
void DynamicsConductor::virtual_12()
{
}

// FUNC 0x001EFCD8 DynamicsConductor::virtual_13
/* Per frame.  At the last countdown frame the standing-start logger
 * sections begin; after the countdown the clock runs (50 a frame, held at
 * RACE_CLOCK_MAX = 99:59:59.999). */
void DynamicsConductor::virtual_13()
{
    int i;
    RaceCarState *c;

    if (getCountDown() == 1) {
        if (isDataLoggerStandingStart() != 0 && org->loggerCreating != 0) {
            for (i = 0; i < carCount; i++) {
                c = getCar(i);
                if (car_logger_active(c) != 0)
                    car_sector_begin(c, 0, 1.0f, 0, 0.0f, c->st.lapDistance);
            }
        }
    }
    if (getCountDown() <= 0) {
        clock += 50;
        if (clock > RACE_CLOCK_MAX)
            clock = RACE_CLOCK_MAX;
    }
}

// FUNC 0x001EFDE8 DynamicsConductor::checkLapRecord
/* Car 0's time against the race's best lap: at or under it (live, no
 * background race, no logger), result.lapRecord = 1 (the ghost save and the
 * data logger are offered) and, with a rank, event 0x13 is posted.  1 if
 * so. */
int DynamicsConductor::checkLapRecord(int car, u32 time)
{
    RaceOrganization *o;

    if (car != 0)
        return 0;
    o = org;
    if (o->backgroundMode != 0)
        return 0;
    if (mode != 0)
        return 0;
    if (o->loggerMode != 0)
        return 0;
    if (time == RACE_TIME_INVALID)
        return 0;
    if (o->lapRecord.total < time)
        return 0;
    o->result.lapRecord = 1;
    if (o->result.rank != 0)
        o->eventQueue.post(RACE_EVENT_LAP_RECORD, 0, 0);
    return 1;
}

// FUNC 0x001EFE80 DynamicsConductor::checkRunResult
/* Slot 26 of MachineTest / License / TimeTrial.  While the player's logger
 * record (RaceBase::virtual_26(0x100): its RaceInput's recorder) is full
 * (36000 frames = 10 min) or finished, result.lapRecord is cleared.  Else,
 * once car 0 has a best lap that beats car 1's (or car 1 is a computer car,
 * or there is no car 1), result.beatGhost = 1 (the ghost is offered) and 1 is
 * returned.  The limit through RaceInput::maxFrames() (T413's inline, as
 * u084's FreeRun): retail keeps `slt frames, 36000'; a literal folds to
 * `> 35999'. */
int DynamicsConductor::checkRunResult()
{
    RaceOrganization *o = org;
    AutomobileControlRecord::Recorder *rec;
    RaceResult *res;
    int full;
    u32 bestLap0;

    if (o->backgroundMode != 0)
        return 0;
    if (mode != 0)
        return 0;
    if (o->loggerMode != 0)
        return 0;
    res = &o->result;
    rec = &o->race->virtual_26(INPUT_ID_PLAYER)->rec.rec;
    full = 0;
    if (rec->finished != 0)
        full = 1;
    if (rec->frames >= RaceInput::maxFrames())
        full = 1;
    if (full != 0) {
        if (res->lapRecord != 0)
            res->lapRecord = 0;
        return 0;
    }
    if (car_best_lap(getCar(0)) == RACE_TIME_INVALID)
        return 0;
    if (carCount != 1 && getCar(1)->st.driver == 0) {
        bestLap0 = car_best_lap(getCar(0));
        if (!(bestLap0 < car_best_lap(getCar(1))))
            return 0;
    }
    res->beatGhost = 1;
    return 1;
}
