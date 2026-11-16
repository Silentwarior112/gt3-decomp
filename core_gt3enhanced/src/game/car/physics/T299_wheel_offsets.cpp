/* T299 - the wheel positions relative to the body, from the body matrix:
 * wheel 0 front (+frontDist) at -halfTrack, 1 front at +halfTrack, 2 rear
 * (-rearDist) at -halfTrack, 3 rear at +halfTrack. */
/* Retail T299's .rodata (0x0033C8C0, nothing references it: DATA_START):
 * "bad_alloc", "t9UnitArena1Ui64" (the race headers, as T296). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T299_wheel_offsets.h"
#include "game/car/physics/suspension_internal.h"
// DATA_START 0x0033C8C0

// FUNC 0x001E9CE8 car_wheel_heights
/* The z offsets (offZ) of the wheels. */
void car_wheel_heights(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    f32 forwardZ = st->rot[0][2];
    f32 lateralZ = st->rot[1][2];
    f32 frontAxle = forwardZ * p->frontDist;
    f32 rearAxle = forwardZ * p->rearDist;
    f32 frontSide = lateralZ * p->halfTrack[0];
    f32 rearSide = lateralZ * p->halfTrack[1];

    st->wheel[0].offZ = -frontSide + frontAxle;
    st->wheel[1].offZ = frontSide + frontAxle;
    st->wheel[2].offZ = -rearSide - rearAxle;
    st->wheel[3].offZ = rearSide - rearAxle;
}

// FUNC 0x001E9D48 car_wheel_planar_offsets
/* The x / y offsets (offX, offY) of the wheels. */
void car_wheel_planar_offsets(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    f32 forwardX = st->rot[0][0];
    f32 forwardY = st->rot[0][1];
    f32 lateralX = st->rot[1][0];
    f32 lateralY = st->rot[1][1];
    f32 frontAxleX = forwardX * p->frontDist;
    f32 frontAxleY = forwardY * p->frontDist;
    f32 rearAxleX = forwardX * p->rearDist;
    f32 rearAxleY = forwardY * p->rearDist;
    f32 frontSideX = lateralX * p->halfTrack[0];
    f32 frontSideY = lateralY * p->halfTrack[0];
    f32 rearSideX = lateralX * p->halfTrack[1];
    f32 rearSideY = lateralY * p->halfTrack[1];

    st->wheel[0].offX = -frontSideX + frontAxleX;
    st->wheel[0].offY = -frontSideY + frontAxleY;
    st->wheel[1].offX = frontSideX + frontAxleX;
    st->wheel[1].offY = frontSideY + frontAxleY;
    st->wheel[2].offX = -rearSideX - rearAxleX;
    st->wheel[2].offY = -rearSideY - rearAxleY;
    st->wheel[3].offX = rearSideX - rearAxleX;
    st->wheel[3].offY = rearSideY - rearAxleY;
}

// FUNC 0x001E9DE8 car_wheel_offsets
void car_wheel_offsets(RaceCarDynamics *car)
{
    car_wheel_heights(car);
    car_wheel_planar_offsets(car);
}
