/* T297 - the steering angle of each wheel: the inner / outer share of the
 * steering on the front axle plus the toe of the axle. */
/* Retail T297's .rodata (0x0033C848, nothing references it: DATA_START):
 * "bad_alloc", "t9UnitArena1Ui64" (the race headers, as T296). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T297_wheel_steer.h"
#include "game/car/physics/suspension_internal.h"
// DATA_START 0x0033C848

// FUNC 0x001E8910 wheel_steer_angle
/* Front wheels: steer x steerGain[wheel] for steer >= 0, steerGain[1 -
 * wheel] below (the inner / outer share swaps with the direction).  (The
 * fourth argument is passed (0) but not used.) */
f32 wheel_steer_angle(CarParams *p, int wheel, f32 steer, f32 unused)
{
    f32 angle = 0.0f;

    if (wheel < 2) {
        angle = steer;
        if (steer >= 0.0f)
            angle *= p->steerGain[wheel];
        else
            angle *= p->steerGain[1 - wheel];
    }
    f32 toe = p->toe[wheel >> 1];
    if ((wheel & 1) == 0)
        toe = -toe;
    return angle + toe;
}

// FUNC 0x001E8978 car_wheel_steer_angles
/* The wheels' steering angles and their sine / cosine. */
void car_wheel_steer_angles(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int i;

    for (i = 0; i < 4; i++)
        st->wheel[i].steer = wheel_steer_angle(&car->params, i, st->steer, 0.0f);
    for (i = 0; i < 4; i++) {
        st->tyreForce[i].cosSteer = cosf(st->wheel[i].steer);
        st->tyreForce[i].sinSteer = sinf(st->wheel[i].steer);
    }
}
