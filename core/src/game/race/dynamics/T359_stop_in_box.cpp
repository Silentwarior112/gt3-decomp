/* T359 (0x001EF488..0x001EF520): the stop test of the licenses. */
#include "game/race/dynamics/dynamics_conductor_internal.h"

// FUNC 0x001EF488 car_stopped_in_box
/* 1 when all four wheels' road contacts are in the stop box (attr2 bit 2:
 * the triangle attribute of the box) and the car moves at 1 km/h or less;
 * *speed then gets the speed in km/h (0..1). */
int car_stopped_in_box(RaceCarState *car, f32 *speed)
{
    DynCarState *st = &car->st;
    int inBox = 0;
    int i;
    f32 v;

    *speed = 0.0f;
    for (i = 0; i < 4; i++) {
        if (car->contact[i].attr2 & 4)
            inBox++;
    }
    if (inBox < 4)
        return 0;
    v = boot_sqrtf_v(st->vel[0] * st->vel[0] + st->vel[1] * st->vel[1] + st->vel[2] * st->vel[2]);
    if (KMH_1 < v)
        return 0;
    *speed = v / KMH_1;
    return 1;
}
