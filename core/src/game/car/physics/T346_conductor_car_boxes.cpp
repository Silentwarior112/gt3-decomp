/* T346 - the DynamicsConductor's car boxes (CarBox, T376): the cars'
 * position / velocity / yaw go to the collision boxes before the collision
 * step and come back after it (callers: DynamicsConductor::virtual_21,
 * collideAllCars, collideWalls). */
/* Retail T346's .rodata (0x0033C820, nothing references it: DATA_START):
 * "bad_alloc", "t9UnitArena1Ui64" (the race headers, as T296). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T346_conductor_car_boxes.h"
#include "game/car/physics/suspension_internal.h"
// DATA_START 0x0033C820

// FUNC 0x001E8730 conductor_store_car_boxes
void conductor_store_car_boxes(DynamicsConductor *dc)
{
    int i;

    for (i = 0; i < dc->carCount; i++) {
        RaceCarDynamics *car = dc->getCar(i);
        CarDynState *st = &car->st;
        CarBoxCarState *bs = car_box_car_state(&dc->GetCollisionProcessor()->box[i]);
        int axis;
        for (axis = 0; axis < 3; axis++) {
            bs->pos[axis] = st->pos[axis];
            bs->vel[axis] = st->vel[axis];
        }
        bs->sector = st->sector;
        bs->yaw = st->yaw;
        bs->yawRate = st->yawRate;
    }
}

// FUNC 0x001E8828 conductor_load_car_boxes
/* (The sector is not copied back.) */
void conductor_load_car_boxes(DynamicsConductor *dc)
{
    int i;

    for (i = 0; i < dc->carCount; i++) {
        RaceCarDynamics *car = dc->getCar(i);
        CarDynState *st = &car->st;
        CarBoxCarState *bs = car_box_car_state(&dc->GetCollisionProcessor()->box[i]);
        int axis;
        for (axis = 0; axis < 3; axis++) {
            st->pos[axis] = bs->pos[axis];
            st->vel[axis] = bs->vel[axis];
        }
        st->yaw = bs->yaw;
        st->yawRate = bs->yawRate;
    }
}
