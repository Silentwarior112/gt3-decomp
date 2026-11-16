/* T313_dc_free_run.cpp - DynamicsConductorFreeRun (0x001F1190..0x001F1AD8).
 *
 * The conductor of the time trial's free run, with the one-lap ghost
 * (RaceFreeRun, T142).  The player's input (INPUT_PLAYER) records each lap
 * of car 0 in its lap record (RaceInput::lap): at the line the head of the
 * record's slot (a OneLapGhostInfo) gets a snapshot of the car and the record
 * restarts (ghost_record_start); at the lap's end the lap time goes into the
 * head and a lap better than the best one (RaceInput::bestLap) becomes the
 * best (RaceInput::keepLap), offering the ghost save (result.lapRecord) and,
 * when it also beats the ghost car's lap, the new ghost (result.beatGhost,
 * RaceOrganization::setupGhost).  With two cars, car 1 is the ghost: its
 * input (INPUT_GHOST) plays its record, and at the line the ghost car is put
 * back to the record's snapshot (ghost_restart).  There are no laps to
 * finish (slot 29 = 0).
 *
 * Retail T313's .rodata opens with <new>'s "bad_alloc" (0x0033D380, nothing
 * references it: DATA_START), then the vtable and the type names.
 */
// DATA_START 0x0033D380
#include <new>                              /* (unused: "bad_alloc") */
#include "game/race/dynamics/dc_modes_internal.h"
#include "game/race/core/T129_race_base.h"            /* RaceBase::virtual_26 (the input of an input id) */
#include "game/race/dynamics/T313_dc_free_run.h"             /* last (dc_modes_internal.h) */

// FUNC 0x001F1190 DynamicsConductorFreeRun::DynamicsConductorFreeRun
//   DynamicsConductor(), then the vptr (0x0033D390).  Caller: RaceFreeRun
//   (T142).
DynamicsConductorFreeRun::DynamicsConductorFreeRun()
{
    /* implicit: DynamicsConductor (the base constructor) */
}

// FUNC 0x001F11D0 DynamicsConductorFreeRun::virtual_20
//   The base's.
void DynamicsConductorFreeRun::virtual_20(int a)
{
    DynamicsConductor::virtual_20(a);
}

// FUNC 0x001F11F0 DynamicsConductorFreeRun::virtual_21
//   The base's; then (mode 0) a best lap record without a time gets "no
//   time", so that any lap beats it.
void DynamicsConductorFreeRun::virtual_21(int a)
{
    DynamicsConductor::virtual_21(a);
    if (mode == 0) {
        RaceInput *player = org->race->virtual_26(INPUT_PLAYER);
        OneLapGhostInfo *best = (OneLapGhostInfo *)player->bestLap->getSlot();

        if (best->time == 0)
            best->time = RACE_TIME_INVALID;
    }
}

// FUNC 0x001F1268 DynamicsConductorFreeRun::getEachEntrantInfo_
//   getEntrantInfoTrial, then a non-zero *d (DynCar_initialize's noController)
//   becomes 2.
void DynamicsConductorFreeRun::getEachEntrantInfo_(int car, int *tyreFlag, int *b, int *c, int *d)
{
    getEntrantInfoTrial(car, tyreFlag, b, c, d);
    if (*d != 0)
        *d = 2;
}

// FUNC 0x001F12A0 DynamicsConductorFreeRun::virtual_29
//   The lap count: none.
int DynamicsConductorFreeRun::virtual_29()
{
    return 0;
}

// FUNC 0x001F12A8 DynamicsConductorFreeRun::virtual_15
//   A checkpoint: the difference to the reference lap's section, then the
//   section time events (lap - 1, as the base).  raw is not read.
void DynamicsConductorFreeRun::virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split)
{
    RaceCarState *c = getCar(car);

    dc_post_section_diff(this, car, gate, split);
    dc_post_section_time(this, car, lap - 1, gate, ms, split, c->st.fault);
}

// FUNC 0x001F1348 ghost_record_start
//   The start of a recorded lap of input in: the head of its lap record's
//   slot gets the car's snapshot (saveSector), frac50 and t, and the
//   remainder (clock + 50) % 3 of the start time; the lap record rewinds
//   (RaceInput::resetLap, given the start time (clock + 50) / 3).  Callers:
//   virtual_16, virtual_17.
//   89.9%: retail keeps the quotient (and the divisor 3) in $a1, the call's
//   argument register; here sched1 puts the copy to $a1 between the two
//   uses of the quotient (x 3 as sll + addu), so it gets $v0 and a copy.
//   The remainder is multiplied back as in retail (the clockRem inline of
//   T305; `%' gives mfhi), the quotient computed before the u8 store (after
//   it the clock would be reloaded).  Tried (56 forms): clockMs / clockRem
//   inlines, int / u32 ms, a rem local, the t store's place, the reference
//   compiler (the same).
void ghost_record_start(RaceInput *in, RaceCarState *car, f32 t, int frac50)
{
    OneLapGhostInfo *head = (OneLapGhostInfo *)in->lap->getSlot();
    DynamicsConductor *dc = car->dc;
    u32 clock;
    u32 ms;

    dc->saveSector(head, car->st.carIndex);
    head->frac50 = frac50;
    head->t = t;
    clock = dc->clock + 50;
    ms = clock / 3;
    head->clockRem = clock - ms * 3;
    in->resetLap(ms);
}

// FUNC 0x001F1400 ghost_restart
//   For an input that is not held (the running ghost): its car goes back
//   to the snapshot at the head of the record it plays (restoreSector), on
//   lap lap; the record rewinds; the car's interpolation offset becomes t -
//   the record's t, plus 1 when negative (then, outside the logger, the
//   ghost car (index 1) runs an extra conductor frame, update(1)).
//   Callers: dc_logger_start (T290), virtual_16, virtual_17.
void ghost_restart(RaceInput *in, RaceCarState *car, int lap, f32 t)
{
    AutomobileControlRecord::Manager *record;
    OneLapGhostInfo *head;
    DynamicsConductor *dc;
    DynCarState *st;
    f32 offset;

    if (in->hold != 0)
        return;
    record = &in->rec;
    st = &car->st;
    head = (OneLapGhostInfo *)record->getSlot();
    dc = car->dc;
    dc->restoreSector(st->carIndex, head);
    st->lap = lap;
    record->reset();
    offset = t - head->t;
    if (offset < 0.0f) {
        if (dc->org->loggerMode == 0 && st->carIndex == 1)
            dc->update(1);
        offset += 1.0f;
    }
    car->interpBias = offset;
}

// FUNC 0x001F14E8 DynamicsConductorFreeRun::recordOneLapGhostControl
//   The controller packet of an enabled input goes into the player's lap
//   record (RaceInput::recordLap).
void DynamicsConductorFreeRun::recordOneLapGhostControl(RaceInput *in, CarInput *packet)
{
    if (in->enabled != 0)
        org->race->virtual_26(INPUT_PLAYER)->recordLap(packet);
}

// FUNC 0x001F1540 DynamicsConductorFreeRun::virtual_16
//   The start line.  Car 0: a car the driver does not drive gets back to
//   him (control mode 0, countdown 0, the start event); the ghost car is put
//   back (two cars, outside the logger); a new lap record starts (mode 0).
//   Every car: a car without controller or a logger run drives itself
//   (control mode 1); the lap starts at ms.
void DynamicsConductorFreeRun::virtual_16(int car, u32 ms, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    RaceOrganization *o = org;
    DynCarState *st = &c->st;

    if (car == 0) {
        RaceBase *race = o->race;

        if (st->ctrlMode != 0) {
            car_mode_driver(c);
            c->countdown = 0;
            o->eventQueue.post(RACE_EVENT_START, 0, 0);
        }
        if (o->loggerMode == 0 && carCount == 2) {
            RaceInput *ghost = race->virtual_26(INPUT_GHOST);

            ghost_restart(ghost, getCar(1), 0, t);
        }
        if (mode == 0)
            ghost_record_start(race->virtual_26(INPUT_PLAYER), c, t, frac50);
    }
    if (st->noController != 0 || org->loggerCreating != 0)
        car_mode_computer(c);
    st->lapStartTime = ms;
}

// FUNC 0x001F16B8 DynamicsConductorFreeRun::virtual_17
//   A lap's end: the lap difference and lap time events.  Car 0, mode 0:
//   the lap record stops; a complete lap (the record not full, under
//   RaceInput::maxFrames() frames) gets its time, and when it beats the
//   best one it becomes the best (result.lapRecord = 1); when it also beats
//   the ghost car's lap (or the ghost input is held) result.beatGhost = 1
//   and the ghost car is prepared again (setupGhost(1, 1)).  Then as slot
//   16: the ghost is put back on lap lap, a new record starts.  Every car:
//   control mode 1 as in slot 16, the faults cleared, the lap starts at ms.
//   raw is not read.  Retail computes complete / better / beatsGhost as 1 /
//   0 values (s6 = 1, movz) and tests them: the flags are assigned in
//   if / else arms, as written.
void DynamicsConductorFreeRun::virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t)
{
    RaceCarState *c = getCar(car);
    DynCarState *st = &c->st;
    RaceOrganization *o;

    dc_post_lap_diff(this, car, split);
    dc_post_lap_time(this, car, lap, ms, split, c->st.fault);
    o = org;
    if (car == 0) {
        RaceBase *race = o->race;
        RaceInput *player = race->virtual_26(INPUT_PLAYER);

        if (mode == 0) {
            int complete;

            if (player->isLapFinished() == 0)
                complete = 1;
            else
                complete = 0;
            player->stopLap();
            if (player->lap->rec.frames >= RaceInput::maxFrames())
                complete = 0;
            if (complete != 0) {
                int better;

                ((OneLapGhostInfo *)player->lap->getSlot())->time = split;
                if (split < ((OneLapGhostInfo *)player->bestLap->getSlot())->time)
                    better = 1;
                else
                    better = 0;
                if (better != 0) {
                    RaceResult *res;
                    int beatsGhost;

                    player->keepLap();
                    res = &o->result;
                    res->lapRecord = 1;
                    if (race->virtual_26(INPUT_GHOST)->hold != 0) {
                        beatsGhost = 1;
                    } else {
                        if (split < ((OneLapGhostInfo *)race->virtual_26(INPUT_GHOST)->rec.getSlot())->time)
                            beatsGhost = 1;
                        else
                            beatsGhost = 0;
                    }
                    if (beatsGhost != 0) {
                        res->beatGhost = 1;
                        o->setupGhost(1, 1);
                    }
                }
            }
        }
        if (o->loggerMode == 0 && carCount == 2) {
            RaceInput *ghost = race->virtual_26(INPUT_GHOST);

            ghost_restart(ghost, getCar(1), lap, t);
        }
        if (mode == 0)
            ghost_record_start(race->virtual_26(INPUT_PLAYER), c, t, frac50);
    }
    if (st->noController != 0 || org->loggerCreating != 0)
        car_mode_computer(c);
    DynCar_clearFaults(c);
    st->lapStartTime = ms;
}

// FUNC 0x001F1968 DynamicsConductorFreeRun::virtual_12
//   Per frame: car 0's countdown; when it runs out the driver gets the car
//   (control mode 0) and the start event.
void DynamicsConductorFreeRun::virtual_12()
{
    RaceCarState *c = getCar(0);
    int fire;

    if (c->countdown <= 0)
        fire = 0;
    else
        fire = --c->countdown == 0;
    if (fire != 0) {
        car_mode_driver(c);
        org->eventQueue.post(RACE_EVENT_START, 0, 0);
    }
}

// FUNC 0x001F19E0 DynamicsConductorFreeRun::virtual_13
//   The base's race clock.
void DynamicsConductorFreeRun::virtual_13()
{
    DynamicsConductor::virtual_13();
}

// FUNC 0x001F1A00 DynamicsConductorFreeRun::virtual_34
int DynamicsConductorFreeRun::virtual_34()
{
    return 0;
}

// FUNC 0x001F1A08 DynamicsConductorFreeRun::virtual_35
int DynamicsConductorFreeRun::virtual_35()
{
    return 0;
}

// FUNC 0x001F1A10 DynamicsConductorFreeRun::virtual_36
//   No course-out level on counted ground.
int DynamicsConductorFreeRun::virtual_36()
{
    return 0;
}

// FUNC 0x001F1A18 DynamicsConductorFreeRun::virtual_37
//   No course-out level off the course.
int DynamicsConductorFreeRun::virtual_37()
{
    return 0;
}

// FUNC 0x001F1A20 DynamicsConductorFreeRun::isPitEnabled
int DynamicsConductorFreeRun::isPitEnabled()
{
    return 0;
}

// FUNC 0x001F1A28 DynamicsConductorFreeRun::CurrentLapTime
//   The car's time since the line.
u32 DynamicsConductorFreeRun::CurrentLapTime(RaceCarState *car)
{
    return lapTimeIfStarted(car);
}

// FUNC 0x001F1A48 DynamicsConductorFreeRun::getSpecialStart
//   Slot 25 (a virtual call) with the same arguments.  The code sets no
//   result: $v0 is what slot 25 left (initialize ignores it).  The
//   original had no return statement; GCC 14 returns 0, so that the
//   function does not fall off its end.
int DynamicsConductorFreeRun::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    virtual_25(car, startType, startArg, speedKmh);
#ifndef BOOT_OLD_GCC
    return 0;
#endif
}

// FUNC 0x001F1A78 DynamicsConductorFreeRun::getStartingFormat
//   No start countdown (startAction 3).
int DynamicsConductorFreeRun::getStartingFormat()
{
    return START_FORMAT_NO_COUNTDOWN;
}

// FUNC 0x001F1A80 DynamicsConductorFreeRun::isDataLoggerSupported
int DynamicsConductorFreeRun::isDataLoggerSupported()
{
    return 1;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001F1A88 __tf24DynamicsConductorFreeRun
//   Compiler-generated typeinfo function: builds the __si_type_info
//   __ti24DynamicsConductorFreeRun (0x004B2870, base __ti17DynamicsConductor)
//   on the first call.
extern "C" const type_info &__tf24DynamicsConductorFreeRun(void)
{
    if (__ti24DynamicsConductorFreeRun[0] == 0) {
        __tf17DynamicsConductor();
        __rtti_si(__ti24DynamicsConductorFreeRun, "24DynamicsConductorFreeRun",
                  (const type_info *)__ti17DynamicsConductor);
    }
    return *(const type_info *)__ti24DynamicsConductorFreeRun;
}
#endif
