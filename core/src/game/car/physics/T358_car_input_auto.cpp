/* T358 (0x001E35D8..0x001E3708): the computer driver's input of a car (the
 * driver 1 / 2, or the pad car while the computer has it: ctrlMode != 0).
 * (T358 has no canonical header: car_input_ai is used inside u079 only, by
 * car_input_update, T342.) */
/* Retail T358's .rodata (0x0033C660): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START); the original saw the
 * race headers (its code asks the conductor, DynamicsConductor::getCountDown,
 * and drives the computer driver, ai_drive / ai_idle / ai_rev_engine). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_input_internal.h"
// DATA_START 0x0033C660

/* The car held at the start (DynCarState.stopped: the conductor's
 * countdown) or in the pit box (pitTimer): the start revving
 * (ai_rev_engine) with the brake off and the steering straight.  Else, in
 * CTRL_STILL, no clutch and ai_idle; otherwise the clutch (with the clutch
 * free: engaged above 3/4 of the rev limit; else engaged unless the rpm is
 * below the idle) and a frame of driving (ai_drive).  `in' is passed by the
 * caller but not read. */
// FUNC 0x001E35D8 car_input_ai
void car_input_ai(RaceCarDynamics *car, CarInput *in)
{
    CarDynState *st = &car->st;

    st->handbrake = 0.0f;
    car->st.ctrl.reverse = 0;
    car->st.ctrl.shift = 0;

    if (st->stopped != 0) {
        ai_rev_engine(&car->ai, car->dc->getCountDown());
        car_set_brake(car, 0.0f);
        st->steer = 0.0f;
    } else if (st->pitTimer != 0) {
        ai_rev_engine(&car->ai, st->pitTimer);
        car_set_brake(car, 0.0f);
        st->steer = 0.0f;
    } else if (st->ctrlMode == CTRL_STILL) {
        st->clutch = 0.0f;
        ai_idle(&car->ai);
    } else {
        CarParams *p = &car->params;

        if (st->clutchState == CLUTCH_FREE) {
            st->clutch = 0.0f;
            if (p->gear.revLimit * 3 / 4 < st->rpm)
                st->clutch = 1.0f;
        } else {
            st->clutch = 1.0f;
            if (st->rpm < p->engine.idle)
                st->clutch = 0.0f;
        }
        ai_drive(&car->ai);
    }
}
