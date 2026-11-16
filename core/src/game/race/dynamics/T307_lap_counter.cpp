/* T307 (0x001EE7E8..0x001EF488): the lap counter.  Every frame
 * (DynamicsConductor::update, T317) a car's course position is taken (slot
 * 27) and compared with the last one: checkpoints (slot 33), logger sections
 * (dc_section_value) and the slot 30 line inside the lap, the start / finish
 * line where the position wraps.  Crossings are timed to a fiftieth of the
 * frame (frac = (int)(fraction x 50), added to the clock).
 *
 * Retail T307's .rodata (0x0033CC70..0x0033CC91; nothing references it:
 * DATA_START): "bad_alloc", t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033CC70

// FUNC 0x001EE7E8 DynamicsConductor::clearGateTimes
/* Called by virtual_20 (T317) at the start: the clock and the battles'
 * checkpoint time table. */
void DynamicsConductor::clearGateTimes()
{
    int g, i;

    clock = 0;
    for (g = 0; g < 4; g++) {
        for (i = 0; i < 8; i++)             /* reversed by loop.c (bgez) */
            gateTime[g][i] = RACE_TIME_INVALID;
        gateLap[g] = -1;
        gateCount[g] = 0;
        gateDone[g] = 0;
    }
}

// FUNC 0x001EE878 DynamicsConductor::noseDistance
/* The course distance of the car's nose: pos + rot[0] (the forward row of
 * the rotation) x params.lenFront (the body ahead of the centre).  The name
 * is the first pass's and misleads (no prediction: a rename to noseDistance
 * is in the notes).  DynamicsConductorMachineTest::getLapDistance.  The query
 * record keeps the unconverted point after itself. */
f32 DynamicsConductor::noseDistance(RaceCarState *car)
{
    DynCarState *st = &car->st;
    f32 ahead = car->params.lenFront;
    f32 x = st->pos[0] + st->rot[0][0] * ahead;
    f32 y = st->pos[1] + st->rot[0][1] * ahead;
    f32 z = st->pos[2] + st->rot[0][2] * ahead;
    RunwayPosWorld q;

    q.world[0] = x;
    q.world[1] = y;
    q.world[2] = z;
    q.pos.sector = st->sector;
    q.pos.x = x;
    q.pos.y = z;
    q.pos.z = -y;
    return course->getLapDistance(&q.pos);       /* 0x0018FA20 */
}

// FUNC 0x001EE900 DynamicsConductor::courseDistance
/* The course position of the car: the runway's axes are (x, z, -y). */
f32 DynamicsConductor::courseDistance(RaceCarState *car)
{
    DynCarState *st = &car->st;
    RunwayPos p;

    p.x = st->pos[0];
    p.y = st->pos[2];
    p.z = -st->pos[1];
    p.sector = st->sector;
    return course->getLapDistance(&p);           /* 0x0018FA20 */
}

// FUNC 0x001EE958 DynamicsConductor::getLapDistance
f32 DynamicsConductor::getLapDistance(RaceCarState *car)
{
    return courseDistance(car);
}

// FUNC 0x001EE978 DynamicsConductor::countLap
/* prev: the car's course position of the last frame.  Returns 1 when the
 * car went backwards (or backed over the line), else 0.  `raw' is the
 * crossing's clock value, `ms' it in ms, `split' the time since the lap's
 * start; frac / remain the crossing's fiftieths of the frame before / after
 * it.  One exit (retail keeps `backwards' in a stack slot and reloads it at
 * the end); each part has its own locals.  98.7%: after the multi-lap
 * sector's virtual_7 retail loads org before copying the result to s0
 * (org in v1); ours copies first (org in v0).  Tried: the sector at function
 * / block scope, u8 / u32 sector, `sector = ...' inside the call, a `next'
 * local, virtual_7 returning u8, a creating local; 991111-01 gives the same. */
int DynamicsConductor::countLap(int car, f32 prev)
{
    int backwards = 0;
    RaceCarState *c = getCar(car);
    DynCarState *st = &getCar(car)->st;
    f32 len = getLapLength();
    f32 pos = st->lapDistance;

    if (prev <= pos) {
        if (prev + len * 0.5f < pos) {
            /* backed over the line: one more lap to make good */
            backwards = 1;
            CAR_VIEW_ADD_LAP_DISTANCE(c, len);
            st->lapAdjust++;
            if (st->lapAdjust >= 0xFB)
                st->lapAdjust = 0xFA;
        } else if (st->lapAdjust == 0 && st->lap != 0 && st->finished == 0) {
            /* the next checkpoint */
            f32 gate = getGatePosition(st->gate);

            if (prev < gate && gate <= pos) {
                u32 raw = clock + (int)((gate - prev) / (pos - prev) * 50.0f);
                u32 ms = raw / 3;
                u32 split = ms - st->lapStartTime;

                if (isDataLoggerSupported() == 0)
                    IssuePassingGate(car, st->lap - 1, st->gate, ms, split);
                virtual_15(car, st->lap, st->gate, raw, ms, split);
                st->gate++;
            }

            /* the next logger section */
            if (car_logger_active(c) != 0 && org->loggerMode != 0) {
                f32 boundary = dc_section_value(this, st->section);

                if (prev < boundary && boundary <= pos) {
                    f32 t = (boundary - prev) / (pos - prev);
                    int frac = (int)(t * 50.0f);
                    u32 split = (clock + frac) / 3 - st->lapStartTime;
                    int sector = virtual_7(c, 1);
                    int remain;

                    car_sector_end(c, sector, split, org->loggerCreating);
                    st->section++;
                    remain = 50 - frac;
                    sector = virtual_7(c, 0);
                    if (org->loggerCreating != 0)
                        car_sector_begin(c, sector, t, remain, boundary, pos);
                }
            }

            /* the slot 30 line */
            if (hasFinishLine() != 0) {
                f32 line = virtual_30();

                if (prev < line && line <= pos) {
                    u32 raw = clock + (int)((line - prev) / (pos - prev) * 50.0f);
                    u32 ms = raw / 3;
                    u32 split = ms - st->lapStartTime;
                    int logging = car_logger_active(c);
                    int closes = virtual_18(car, st->lap, raw, ms, split);

                    if (logging != 0 && closes != 0 && org->loggerMode != 0)
                        car_sector_end(c, virtual_7(c, 1), split, org->loggerCreating);
                }
            }
        }
    } else if (pos + len * 0.5f < prev) {
        /* crossed the start / finish line */
        CAR_VIEW_SUB_LAP_DISTANCE(c, len);
        if (st->lapAdjust != 0) {
            st->lapAdjust--;
        } else if (st->finished == 0) {
            f32 rest = len - prev;
            u8 lastGate = st->gate;
            int lastLap = st->lap;
            f32 t;
            int frac, remain;
            u32 raw, ms, split;

            st->gate = 0;
            t = rest / (rest + pos);
            st->lap = st->lap + 1;
            frac = (int)(t * 50.0f);
            raw = clock + frac;
            remain = 50 - frac;
            ms = raw / 3;
            split = ms - st->lapStartTime;
            if (lastLap == 0) {
                /* the start */
                virtual_16(car, ms, remain, t);
                if (car_logger_active(c) != 0 && isDataLoggerStandingStart() == 0 && org->loggerCreating != 0)
                    car_sector_begin(c, 0, t, remain, 0.0f, pos);
            } else {
                int logging;

                if (isDataLoggerSupported() == 0)
                    IssuePassingGate(car, lastLap - 1, lastGate, ms, split);
                logging = car_logger_active(c);
                virtual_17(car, lastLap, raw, ms, split, remain, t);
                if (logging != 0 && org->loggerMode != 0) {
                    int sector = virtual_7(c, 1);

                    car_sector_end(c, sector, split, org->loggerCreating);
                    if (isDataLoggerMultiLap() != 0 && st->finished == 0)
                        car_sector_begin(c, sector + 1, t, remain, 0.0f, pos);
                }
            }
            st->section = 0;
        }
    } else {
        /* going backwards */
        backwards = 1;
    }
    return backwards;
}

// FUNC 0x001EF060 DynamicsConductor::updateCar
/* The per-frame course position of a car: lap counting once the countdown
 * is over, the "wrong way" message when it goes backwards faster than
 * 5 km/h (cleared below 3 km/h), the auto drive message, the pit lane. */
void DynamicsConductor::updateCar(int car)
{
    DynCarState *st = &getCar(car)->st;
    f32 prev = st->lapDistance;
    f32 speed;

    st->lapDistance = getLapDistance(getCar(car));
    if (getCountDown() > 0)
        return;
    if (countLap(car, prev) == 1) {
        st->flags |= DYN_FLAG_BACKWARDS;
        speed = boot_sqrtf_v(st->speedX * st->speedX + st->speedY * st->speedY);
        if (KMH_5 < speed) {
            if (car_box_has_hit(&GetCollisionProcessor()->box[car]) == 0 &&    /* 0x001FB838 */
                !((u32)((u8)st->driver - 1) < 2) && st->ctrlMode == CTRL_DRIVER)
                DynCar_setMessage(getCar(car), CARMSG_WRONG_WAY);
        } else if (speed < KMH_3) {
            DynCar_setMessage(getCar(car), CARMSG_NONE);
        }
    } else {
        st->flags &= ~DYN_FLAG_BACKWARDS;
        DynCar_setMessage(getCar(car), CARMSG_NONE);
    }
    if (st->ctrlMode == CTRL_COMPUTER && st->finished == 0) {
        if (st->fullThrottle != 0)
            DynCar_setMessage(getCar(car), CARMSG_AUTO_DRIVE_TH);
        else
            DynCar_setMessage(getCar(car), CARMSG_AUTO_DRIVE);
    }
    car_update_pit(getCar(car), st->lapDistance, prev);
}

// FUNC 0x001EF268 DynamicsConductor::reportGateTimes
/* The battles' slot 0 (ReportTimeDifference): for each checkpoint, the times
 * recorded since the last report go out as display events - the gap to the
 * first car and to the car before (dc_post_time_diff2 / F_001EC978), and to
 * the first car its lead over the second (dc_post_time_diff). */
void DynamicsConductor::reportGateTimes()
{
    int g, k, c;
    u32 time;

    for (g = 0; g < getGateCount(); g++) {
        if (gateDone[g] == gateCount[g])
            continue;
        for (k = gateDone[g]; k < gateCount[g]; k++) {
            c = gateCar[g][k];
            if (k == 0) {
                F_001EC960(this, c);
                F_001EC970(this, c);
            } else {
                time = gateTime[g][k];
                dc_post_time_diff2(this, c, time - gateTime[g][0]);
                F_001EC978(this, c, time - gateTime[g][k - 1]);
                if (k == carCount - 1)
                    F_001EC968(this, c);
                else
                    F_001EC970(this, c);
                if (k == 1)
                    dc_post_time_diff(this, gateCar[g][0], gateTime[g][0] - time);
            }
        }
        gateDone[g] = gateCount[g];
    }
}
