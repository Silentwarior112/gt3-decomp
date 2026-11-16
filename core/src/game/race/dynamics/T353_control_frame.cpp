/* T353 - CarInput::clear: one frame of a car's input (CarInput, declared in
 * T342_car_input.h; u085's first pass called it ControlFrame), the unit of
 * the control record (AutomobileControlRecord::Recorder, T318).  Callers:
 * Recorder::reset / play and RaceInput::read (T413). */
#include "game/race/dynamics/control_record_internal.h"
#include "game/car/physics/T342_car_input.h"

// FUNC 0x001F4330 CarInput::clear
/* Everything 0: the recorded fields in offset order (handbrake before
 * throttle2), unk_3 last.  egcs emits the last store first and the one
 * before it in the jr's delay slot, which gives retail's unk_3 first,
 * throttle2 last. */
void CarInput::clear()
{
    flags = 0;
    device = 0;
    shift = 0;
    steer = 0;
    steerMag = 0;
    throttle = 0;
    brake = 0;
    handbrake = 0;
    throttle2 = 0;
    unk_3 = 0;
}
