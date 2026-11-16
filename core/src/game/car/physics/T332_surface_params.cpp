/* T332 (0x001DE070..0x001DE158): the course's smoke types 0..10, one per
 * surface type (SmokeType, CourseEnv +0x4D0, T178), from the byte table
 * g_002D3510 (8 bytes per surface).  The code walks them from +0x0C
 * (density, CourseEnv +0x4DC: u077's SurfaceParam records). */
/* Retail T332's .rodata (0x0033C3E8): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START); the original saw the
 * race headers (RaceCourse). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (seen: UnitArena<64>) */
#include "game/car/physics/T332_surface_params.h"
#include "game/car/physics/car_param_internal.h"
// DATA_START 0x0033C3E8

/* T332's .data (then its frame info at 0x002D3590): size, grow, rise, spin,
 * fade, gravity, alphaScale, period of each surface type's smoke. */
u8 g_002D3510[16][8] = {                    /* 0x002D3510 */
    { 30, 24, 90, 114, 60, 0, 100, 0 },
    { 50, 20, 20, 30, 30, 0, 100, 60 },
    { 60, 26, 15, 30, 10, 0, 100, 60 },
    { 60, 27, 15, 30, 10, 0, 100, 60 },
    { 70, 30, 15, 30, 10, 0, 100, 32 },
    { 100, 32, 126, 246, 4, 51, 100, 6 },
    /* 6..15: zeros */
};

/* SmokeType seen from its density (+0x0C) on (u077's SurfaceParam): retail
 * walks the records from CourseEnv +0x4DC with these offsets, the view of the
 * smoke type this code had.  Same layout as SmokeType +0x0C..+0x44. */
struct SurfaceParam {
    f32 density;                            /* 0x00 SmokeType +0x0C */
    f32 size;                               /* 0x04 */
    f32 grow;                               /* 0x08 */
    f32 rise;                               /* 0x0C */
    f32 spin;                               /* 0x10 */
    f32 fade;                               /* 0x14 */
    u8  unk_18[8];                          /* 0x18 */
    f32 gravity;                            /* 0x20 */
    f32 alphaScale;                         /* 0x24 */
    u8  unk_28[0xC];                        /* 0x28 */
    u32 period;                             /* 0x34 SmokeType +0x40 */
};

// FUNC 0x001DE070 CourseEnv_initSurfaceSmoke
/* course: the conductor's RaceCourse (DynamicsConductor::virtual_20). */
void CourseEnv_initSurfaceSmoke(RaceCourse *course)
{
    SmokeType *smoke = course->data->env->smoke;
    int i;

    for (i = 0; i < 11; i++) {
        u8 *src = g_002D3510[i];
        SurfaceParam *s = (SurfaceParam *)&smoke[i].density;

        if (s->density < 0.01f)
            s->density = 1.0f;
        s->size = (f32)src[0] * 0.01f;
        s->grow = (f32)src[1] * 0.1f;
        s->rise = (f32)src[2] * 0.01f;
        s->spin = (f32)src[3];
        s->fade = (f32)src[4] * 0.01f;
        s->gravity = (f32)src[5] * 0.01f;
        s->alphaScale = (f32)src[6] * 0.01f;
        s->period = src[7];
    }
}
