/* T334 - the tyre smoke: 0x001DF148..0x001DF2B0.  Both functions are called
 * by the per-car step car_step (T302).
 *
 * Data (retail): .rodata 0x0033C470..0x0033C491, nothing the code
 * references: <new>'s "bad_alloc", then UnitArena<64>'s type name (T153
 * with GAME_T153_EMITS_TYPEINFO: RaceCourse::emitSmoke); .lit4 0x003502A8. */
// DATA_START 0x0033C470
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::emitSmoke (seen: UnitArena<64>) */
#include "game/car/physics/car_engine_internal.h"
#include "game/effects/T334_tyre_smoke.h"

// FUNC 0x001DF148 F_001DF148
void F_001DF148(RaceCarState *car)
{
    /* empty */
}

/* One SmokeSource per wheel to the course (RaceCourse::emitSmoke): the
 * wheel's position in the course axes (x, z, -y: car x + offX, the ground
 * height under the wheel, -(car y + offY)), yaw + steer, the contact's u as
 * intensity, the ground plane with d lowered by 0.05 x normal.y, the
 * wheel's smoke, slip, velX and tread speed. */
// FUNC 0x001DF150 tyre_smoke_emit
void tyre_smoke_emit(RaceCarState *car)
{
    DynCarState *st = &car->st;
    SmokeSource src;                        /* sp+0x00 */

    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];

        src.flags = i;
        src.kind = w->surface;
        src.intensity = car->contact[i].u;
        src.rate = w->smoke;
        src.angle = st->yaw + w->steer;
        src.pos[0] = st->pos[0] + w->offX;
        src.pos[1] = w->groundZ;
        src.pos[2] = -(st->pos[1] + w->offY);
        /* the plane: normal[3] and d, copied as four words */
        f32 *dst = src.plane;
        const f32 *from = car->contact[i].normal;
        for (int k = 0; k < 4; k++)
            *dst++ = *from++;
        src.f30 = w->slip;
        src.f34 = w->velX;
        src.f38 = w->spin;
        src.plane[3] = src.plane[3] - src.plane[1] * 0.05f;
        car->course->emitSmoke(&src);      /* 0x0018FDE0 */
    }
}
