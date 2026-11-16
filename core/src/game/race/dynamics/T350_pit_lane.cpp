/* T350 (0x001ED9F0..0x001EE288): the pit lane of a car.  DynCarState.ctrlMode
 * (CTRL_*): 0 the driver, 1 the computer, 2 in the pit lane towards the box,
 * 3 at the box (the stop runs pitTimer frames), 4 leaving.  The positions
 * come from the course's pit gates (DynamicsConductor::pitGateDistance: 1 the pit
 * entry, -1 / -2 / -3 the exit side, 3, 0) and boxes (pitBoxDistance).  A
 * position below half a lap counts one course length further, so that the pit
 * lane is continuous across the line.  The T341 requests car_mode_driver /
 * car_mode_pit_in / car_mode_pit_box / car_mode_pit_out set ctrlMode 0 / 2 / 3 / 4.
 *
 * Retail T350's .rodata (0x0033CBA8..0x0033CC41; nothing references it:
 * DATA_START): "bad_alloc", then the type names 27RaceDisplayGeneralTimeEvent,
 * 23RaceDisplayLapDiffEvent, 24RaceDisplayTimeDiffEvent, 20RaceDisplayEventBase,
 * t9UnitArena1Ui64 (T303's set: T153 is read before T194, whose information
 * event the pit lane posts). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#define GAME_T194_EMITS_TYPEINFO            /* 20RaceDisplayEventBase's type name, as retail */
#include "game/race/display/T194_race_display_event.h"   /* RaceDisplayInformationEvent; T154's GeneralTime /
                                               LapDiff / TimeDiff (only inline virtuals: their names) */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T350_pit_lane.h"
// DATA_START 0x0033CBA8

// FUNC 0x001ED9F0 dc_post_pit_in
/* The information event 1 of the car: RaceDisplay::pollInformationEvents
 * (T154) shows it as "<maker> has pitted in". */
void dc_post_pit_in(DynamicsConductor *dc, int car)
{
    RaceOrganization *org = dc->org;
    RaceDisplayInformationEvent ev;

    ev.car = car;
    ev.information = 1;
    ev.post(org);
}

// FUNC 0x001EDA40 dc_post_pit_out
/* The information event 6 of the car, posted at the pit exit: the display
 * shows the start display and plays the start sound for the player's car
 * (RaceDisplay::pollInformationEvents: put_start_display, sound 0x15). */
void dc_post_pit_out(DynamicsConductor *dc, int car)
{
    RaceOrganization *org = dc->org;
    RaceDisplayInformationEvent ev;

    ev.car = car;
    ev.information = 6;
    ev.post(org);
}

// FUNC 0x001EDA90 car_in_pit_lane
/* 1 while the car is in the pit lane: ctrlMode 2..4 and between the entry
 * gate 1 and the exit gate -1.  Caller: DesignerCamera::update. */
int car_in_pit_lane(RaceCarState *car)
{
    DynCarState *st = &car->st;
    DynamicsConductor *dc = car->dc;
    f32 len, pos;

    if ((u32)(st->ctrlMode - CTRL_PIT_IN) < 3) {
        len = dc->getLapLength();
        pos = st->lapDistance;
        if (pos < len * 0.5f)
            pos += len;
        /* neither before the entry nor past the exit */
        if (!(pos < dc->pitGateDistance(1)) && !(dc->pitGateDistance(-1) < pos))
            return 1;
    }
    return 0;
}

// FUNC 0x001EDB40 car_update_pit
/* The pit lane state machine of a car after it moved from prev to pos (the
 * course positions of this and the last frame).  `service': the car wants
 * service - bit 0 a tyre is not new (its wear differs from the wear curve's
 * x0), bit 1 a wheel's needsService (? damaged; T296 car_tyre_check), bit 2 slot 41
 * asks for a stop.  The `dc' local is the conductor read once at the start;
 * the service test and the box tests read car->dc again (the code does: a
 * second load).  Each case has its own gate locals (retail keeps no gate
 * value across the cases). */
void car_update_pit(RaceCarState *car, f32 pos, f32 prev)
{
    DynCarState *st = &car->st;
    DynamicsConductor *dc = car->dc;
    f32 len;
    int service, raceDone, i;

    if (dc->hasPitLane() == 0)
        return;                             /* no pit lane on the course */
    len = dc->getLapLength();
    if (pos < len * 0.5f)
        pos += len;
    if (prev < len * 0.5f)
        prev += len;
    if (st->pitTimer != 0)
        DynCar_setMessage(car, CARMSG_PIT_STOP);

    if (st->flags & DYN_FLAG_BACKWARDS) {
        /* going backwards: only the pit exit is watched (not for drivers 1 /
         * 2, the computer cars) */
        f32 gate;

        if ((u32)((u8)st->driver - 1) < 2)
            return;
        /* one word access of +0x534: (word & 0x10FF) == 0x1000 */
        if (st->ctrlMode != CTRL_DRIVER || !(st->flags & DYN_FLAG_COUNTED_GROUND))
            return;
        if (!(dc->pitGateDistance(3) < pos))
            return;
        if (!(pos < dc->pitGateDistance(-2)))
            return;
        gate = dc->pitGateDistance(-3);
        if (gate < prev && pos <= gate) {
            car_mode_pit_out(car);
            DynCar_setMessage(car, CARMSG_PIT_EXIT_BACK);
            return;
        }
        if (KMH_5 < st->speedX || st->speedX < -KMH_5 ||
            KMH_5 < st->speedY || st->speedY < -KMH_5) {
            if (car_box_has_hit(&car->dc->GetCollisionProcessor()->box[st->carIndex]) == 0)    /* 0x001FB838 */
                DynCar_setMessage(car, CARMSG_WRONG_WAY_PIT);
        }
        return;
    }

    service = 0;
    for (i = 0; i < 4; i++) {
        if (!(st->wheel[i].wear == car->dc->wear.x0))
            service |= 1;
        if (st->wheel[i].needsService != 0)
            service |= 2;
    }
    if (car->dc->EveryTimePitStop() != 0)
        service |= 4;
    raceDone = car_race_done(car);

    switch (st->ctrlMode) {
    case CTRL_PIT_IN: {
        /* towards the box */
        f32 gate = dc->pitGateDistance(1);
        f32 box;

        if (gate <= pos && gate < len && !(st->flags & DYN_FLAG_COUNTED_GROUND)) {
            car_mode_driver(car);
            DynCar_setMessage(car, CARMSG_NONE);
            return;
        }
        if (prev < gate && gate <= pos)
            dc_post_pit_in(dc, st->carIndex);
        box = dc->pitBoxDistance(st->pitBox) - 25.0f;
        if (prev < box && box <= pos) {
            /* 25 m before the box */
            if (raceDone != 0) {
                car_mode_pit_out(car);
                DynCar_setMessage(car, CARMSG_PIT_RACE_DONE);
            } else if (service == 0) {
                car_mode_pit_out(car);
                DynCar_setMessage(car, CARMSG_PIT_NO_NEED);
            } else if (KMH_100 <= st->speedX) {
                car_mode_pit_out(car);
                DynCar_setMessage(car, CARMSG_PIT_TOO_FAST);
            } else {
                car_mode_pit_box(car);
            }
        } else {
            DynCar_setMessage(car, CARMSG_PIT_LANE);
        }
        return;
    }

    case CTRL_PIT_BOX:
        /* at the box */
        if (st->pitTimer == 0) {
            f32 box = dc->pitBoxDistance(st->pitBox);

            if (service != 0 && prev < box && box <= pos) {
                DynCar_resetState(car);
                st->pitTimer = PIT_STOP_FRAMES;
                DynCar_setMessage(car, CARMSG_PIT_STOP);
                return;
            }
            if (box + 0.5f <= pos) {
                car_mode_pit_out(car);
                st->flags &= ~DYN_FLAG_TYRE_DUE;
            }
        } else if (st->pitTimer == PIT_TYRE_FRAME) {
            DynCar_initTyreWear(car);
        }
        return;

    case CTRL_PIT_OUT: {
        /* leaving */
        f32 gate = dc->pitGateDistance(1);
        f32 gateOut, gateLast;

        if (gate <= pos && pos < len && !(st->flags & DYN_FLAG_COUNTED_GROUND)) {
            car_mode_driver(car);
            DynCar_setMessage(car, CARMSG_NONE);
        }
        if (st->driver != 0 || st->finished != 0) {
            gateOut = dc->pitGateDistance(-1);
            gateLast = dc->pitGateDistance(-2);
        } else {
            gateOut = dc->pitGateDistance(-2);
            gateLast = dc->pitGateDistance(-3);
        }
        if (len + len * 0.5f <= gateOut)
            gateOut -= len;
        if (prev < gateOut && gateOut <= pos) {
            if (st->finished == 0) {
                car_mode_driver(car);
                if (raceDone == 0)
                    dc_post_pit_out(dc, st->carIndex);
            } else {
                car_finish(car, 3);
            }
            st->flags &= ~DYN_FLAG_PIT_CALL;
            return;
        }
        if (gateLast <= pos) {
            if (raceDone != 0)
                return;
            if (st->fullThrottle != 0)
                DynCar_setMessage(car, CARMSG_AUTO_DRIVE_TH);
            else
                DynCar_setMessage(car, CARMSG_AUTO_DRIVE);
            return;
        }
        if (st->finished != 0)
            return;
        DynCar_setMessage(car, CARMSG_PIT_LANE);
        return;
    }

    default:
        if (st->driver == 0 && st->ctrlMode != CTRL_COMPUTER) {
            /* the driver's car drives into the pit lane */
            f32 gate;

            if (!(st->flags & DYN_FLAG_COUNTED_GROUND))
                return;
            gate = dc->pitGateDistance(1);
            if (prev < gate && gate <= pos) {
                car_mode_pit_in(car);
                if (0.0f < st->speedX)
                    dc_post_pit_in(dc, st->carIndex);
            }
            return;
        }
        /* a computer car */
        if (st->flags & DYN_FLAG_COUNTED_GROUND) {
            if (car_box_hit_wall(&car->dc->GetCollisionProcessor()->box[st->carIndex]) != 0 && pos < len) {   /* 0x001FB860 */
                car_mode_pit_in(car);
                car->ai.recover = 2;
                return;
            }
        }
        if (!(st->flags & DYN_FLAG_PIT_CALL))
            return;
        if (raceDone != 0)
            return;
        {
            f32 gate = dc->pitGateDistance(0);

            if (prev < gate && gate <= pos)
                car_mode_pit_in(car);
        }
        return;
    }
}
