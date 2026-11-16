/* T352 (0x001EF520..0x001EF8B8): the end of a license test.  Events posted to
 * the race event queue: 9 failed, 5 finished (car index, delay 2).
 *
 * Retail T352's .rodata (0x0033CC98..0x0033CCB9; nothing references it:
 * DATA_START): "bad_alloc", t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033CC98

// FUNC 0x001EF520 DynamicsConductor::licenseFailed
/* reason (3: course out, 4: collision) is not read. */
void DynamicsConductor::licenseFailed(RaceCarState *car, int reason)
{
    RaceOrganization *o = org;
    DynCarState *st = &car->st;

    if (o->backgroundMode != 0)
        return;
    car_finish(car, 2);
    if (st->noController != 0)
        return;
    if (o->demonstration != 0)
        return;
    o->eventQueue.post(RACE_EVENT_LICENSE_FAILED, st->carIndex, 0);
    if (mode == 0) {
        o->result.rank = -1;
        o->result.time = RACE_TIME_INVALID;
    }
}

// FUNC 0x001EF5C0 DynamicsConductor::licenseFinished
/* The car's run ends with time: the rank is i + 1 for the first i of the
 * getTargetCount() (5) targets whose time getTargetTime(i) the time does not
 * exceed.  getTargetTime gives the record's gold / silver / bronze for i 1..3
 * and 0 for i 0 and 4, so the ranks are 2..4 (1 only for a time of 0); none
 * fails the test (event 9).  Outside a replay (mode 0) the result gets the
 * rank and the time and the lap record is checked.
 * Callers: licenseCheckStop, DynamicsConductorLicense::virtual_17 /
 * virtual_18. */
void DynamicsConductor::licenseFinished(RaceCarState *car, u32 time)
{
    DynCarState *st = &car->st;
    RaceOrganization *o = org;
    int found = 0;
    int finishMode = 3;
    int rank, count, i;

    /* Cases 0, 3 and 4 apart (retail's case tree: == 1, == 0, then 3..4);
     * every type but 1 keeps 3.  98.2%: retail tests 3..4 with slti, the
     * u32 index (needed for the == 0 leaf) gives sltiu; an int index gives
     * an extra range test. */
    switch ((u32)getRules()->licenseType) {
    case 1:                                 /* the stop-in-the-box test */
        finishMode = 1;
        break;
    case 0:
    case 3:
    case 4:
        finishMode = 3;
        break;
    }
    car_finish(car, finishMode);
    if (st->noController == 0) {
        if (o->backgroundMode != 0)
            return;
        if (o->demonstration != 0) {
            o->eventQueue.post(RACE_EVENT_GOAL, st->carIndex, 2);
        } else {
            rank = 0;
            count = o->getTargetCount();
            for (i = 0; i < count; i++) {
                if (!((u32)o->getTargetTime(i) < time)) {
                    rank = i + 1;
                    found = 1;
                    break;
                }
            }
            if (found) {
                if (mode != 0)
                    o->eventQueue.post(RACE_EVENT_GOAL, st->carIndex, 2);
            } else {
                o->eventQueue.post(RACE_EVENT_LICENSE_FAILED, st->carIndex, 0);
            }
            if (mode == 0) {
                o->result.rank = rank;
                o->result.time = time;
                checkLapRecord(st->carIndex, time);
            }
        }
    }
    dc_post_lap_time(this, st->carIndex, 1, time, time, car->st.fault);
}

// FUNC 0x001EF780 DynamicsConductor::licenseCheckStop
/* License slot 13: a course out (fault bit 1) or a collision (bit 0) fails
 * the test; in a stop test (rules.licenseType 1) a stop in the box ends it,
 * the time interpolated by the speed left (km/h, 0..1) as a frame fraction. */
void DynamicsConductor::licenseCheckStop(RaceCarState *car)
{
    DynCarState *st = &car->st;
    f32 speed;
    u32 time;

    if (st->finished != 0)
        return;
    if (st->fault & 2) {
        licenseFailed(car, 3);
        return;
    }
    if (st->fault & 1) {
        licenseFailed(car, 4);
        return;
    }
    if (getRules()->licenseType != 1)
        return;
    if (car_stopped_in_box(car, &speed) == 0)
        return;
    time = (clock + (int)(speed * 50.0f)) / 3 - st->lapStartTime;
    if (org->loggerMode != 0) {
        int sector = virtual_7(car, 1);

        car_sector_end(car, sector, time, org->loggerCreating);
    }
    licenseFinished(car, time);
}
