/* T341 (0x001E3BF0..0x001E3EA0): the control mode requests.  Each moves the
 * computer driver to the mode's enemy line (ai_change_line; now != 0:
 * without the blend from the old line) and on success sets
 * DynCarState.ctrlMode; they return 1 when the mode is set, 0 when the line
 * was refused (the mode is left as it was).  The first pass read the lines
 * as gears and the mode as a gear mode. */
/* Retail T341's .rodata (0x0033C688): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START); the original saw the
 * race headers with the computer driver's (ai_change_line), as the AI
 * files T362..T376 do. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T341_gear_mode.h"
// DATA_START 0x0033C688

/* Back to the car's driver: recover = 0; the pad driver takes LINE_7; a
 * computer car out of the pits (CTRL_PIT_IN / BOX / OUT) takes the racing
 * line, from any other mode it just switches.  Callers: the conductors. */
// FUNC 0x001E3BF0 car_mode_driver
int car_mode_driver(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    AIDriver    *ai = &car->ai;

    ai->recover = 0;
    if (st->driver == DRIVER_PAD) {
        if (ai_change_line(ai, LINE_7, 0) == 0)
            return 0;
    } else {
        u8 mode = st->ctrlMode;
        if (mode == CTRL_PIT_IN || mode == CTRL_PIT_OUT || mode == CTRL_PIT_BOX) {
            if (ai_change_line(ai, LINE_RACING, 0) == 0)
                return 0;
        }
    }
    st->ctrlMode = CTRL_DRIVER;
    return 1;
}

/* The computer takes the car (car_finish, the free run): through the
 * racing line out of the pit modes, or out of CTRL_DRIVER for the pad
 * driver; else it just switches. */
// FUNC 0x001E3C78 car_mode_computer
int car_mode_computer(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int needLine = 0;

    if (st->ctrlMode == CTRL_PIT_IN || st->ctrlMode == CTRL_PIT_OUT || st->ctrlMode == CTRL_PIT_BOX)
        needLine = 1;
    if (st->driver == DRIVER_PAD && st->ctrlMode == CTRL_DRIVER)
        needLine = 1;
    if (needLine != 0) {
        if (ai_change_line(&car->ai, LINE_RACING, 0) == 0)
            return 0;
    }
    st->ctrlMode = CTRL_COMPUTER;
    return 1;
}

/* Into the pits: the pit entry line at once; DYN_FLAG_PIT_CALL set. */
// FUNC 0x001E3D08 car_mode_pit_in
int car_mode_pit_in(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (ai_change_line(&car->ai, LINE_PIT_ENTRY, 1) == 0)
        return 0;
    st->ctrlMode = CTRL_PIT_IN;
    st->flags |= DYN_FLAG_PIT_CALL;
    return 1;
}

/* At the box: the pit lane at once. */
// FUNC 0x001E3D60 car_mode_pit_box
int car_mode_pit_box(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (ai_change_line(&car->ai, LINE_PIT_LANE, 1) == 0)
        return 0;
    st->ctrlMode = CTRL_PIT_BOX;
    return 1;
}

/* Out of the pits: the pit entry line at once. */
// FUNC 0x001E3DA8 car_mode_pit_out
int car_mode_pit_out(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (ai_change_line(&car->ai, LINE_PIT_ENTRY, 1) == 0)
        return 0;
    st->ctrlMode = CTRL_PIT_OUT;
    return 1;
}

/* Rolling to a stop on the racing line (no callers). */
// FUNC 0x001E3DF0 car_mode_stop_roll
int car_mode_stop_roll(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (ai_change_line(&car->ai, LINE_RACING, 0) == 0)
        return 0;
    st->ctrlMode = CTRL_STOP_ROLL;
    return 1;
}

/* Braking to a stop on the racing line (car_finish). */
// FUNC 0x001E3E38 car_mode_stop_brake
int car_mode_stop_brake(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (ai_change_line(&car->ai, LINE_RACING, 0) == 0)
        return 0;
    st->ctrlMode = CTRL_STOP_BRAKE;
    return 1;
}

/* Stop (car_finish). */
// FUNC 0x001E3E80 car_mode_stop
int car_mode_stop(RaceCarDynamics *car)
{
    car->st.ctrlMode = CTRL_STOP;
    return 1;
}

/* Still (ai_stop, once the car stands). */
// FUNC 0x001E3E90 car_mode_still
int car_mode_still(RaceCarDynamics *car)
{
    car->st.ctrlMode = CTRL_STILL;
    return 1;
}
