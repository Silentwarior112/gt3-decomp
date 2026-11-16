/* T164 - RaceMiniMap: the course map of the race display with a dot per
 * car, and the course file's mini map record.  0x0019E230..0x0019E6F0.
 * The class (game/race/display/T154_race_display.h), its typeinfo and destructor are
 * T154's; the last two functions are this unit's copies (never called).
 * T164 has no header of its own: RaceMiniMapData is in race_meters_internal.h.
 */
#include "game/race/display/race_meters_internal.h"

/* T164's data starts unreferenced: "bad_alloc" (<new>, the first item of
 * its .rodata, before "1pcar_dot") and the "" of T157's inline
 * RaceMessageDisplay::clear (its only .sdata; T165 / T166 read no race
 * display header and have none, T167's .sdata starts at 0x0035216C). */
// DATA_START 0x0032FCC0
// DATA_START 0x00352168

/* ======================================================================== */
/* RaceMiniMapData                                                          */
/* ======================================================================== */

// FUNC 0x0019E230 RaceMiniMapData::relocate
//   ? name.  Caller: course_data_relocate (T179, relocating the loaded course data:
//   this is CourseData::miniMap).  The map image's offset (from this
//   record) becomes a pointer, then the image is relocated.
void RaceMiniMapData::relocate()
{
    image = (Tex1Image *)((u32)image + (u32)this);     /* addu offset, this */
    tex1_relocate(image);
}

// FUNC 0x0019E258 RaceMiniMapData::project
//   ? name.  Caller: RaceMiniMap::drawCar.  (x, z) relative to the origin,
//   scaled by `scale' and rotated by `angle' (about the view axis) through
//   the matrix stack, gives the offset from the map's centre in pixels.
//   Both vectors come from the tail of the prim buffer.
void RaceMiniMapData::project(f32 *outX, f32 *outY, f32 x, f32 z)
{
    f32 *in = (f32 *)gfx_alloc_tail(1);
    f32 *out = (f32 *)gfx_alloc_tail(1);

    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_scale(scale, scale, 1.0f);
    vu0_rotate_z(angle);
    in[0] = x - originX;
    in[1] = z - originZ;
    in[2] = 0.0f;
    in[3] = 0.0f;                           /* w = 0: no translation */
    vu0_apply_matrix(out, in);
    pgl_pop_matrix();
    *outX = out[0];
    *outY = out[1];
}

/* ======================================================================== */
/* RaceMiniMap                                                              */
/* ======================================================================== */

// FUNC 0x0019E340 RaceMiniMap::RaceMiniMap
//   Caller: RaceDisplay's constructor.  RaceDisplayObjectBase's constructor
//   (0x00196490), then T154's vtable 0x0032E290.
RaceMiniMap::RaceMiniMap()
{
    /* implicit: RaceDisplayObjectBase (base constructor) */
    playerDot = 0;
    enemyDot = 0;
}

// FUNC 0x0019E380 RaceMiniMap::init
//   ? name.  Caller: RaceDisplay::virtual_0.  The two dot images, the
//   course's mini map record, scale 1, and the map image's size from its
//   CLAMP register (MAXU / MAXV + 1).
void RaceMiniMap::init(RaceOrganization *org)
{
    RaceMiniMapData *map;
    u64 clamp;

    playerDot = race_display_find_image("1pcar_dot");
    enemyDot = race_display_find_image("ecar_dot");
    map = org->course->data->miniMap;
    scaleX = 1.0f;
    scaleY = 1.0f;
    data = map;
    clamp = map->image->tex->clamp;
    mapW = (s16)(((clamp >> 14) & 0x3FF) + 1);      /* CLAMP.MAXU */
    mapH = (s16)(((clamp >> 34) & 0x3FF) + 1);      /* CLAMP.MAXV */
}

// FUNC 0x0019E420 RaceMiniMap::drawMap
//   Caller: RaceDisplay::virtual_3.  When enabled: the
//   map image stretched over (x, y, mapW, mapH) at the display object's
//   scale, then the cars from the last ranked up (rankEnd .. rankPrev), the
//   target car last with the player's dot.  With an alignment of exactly
//   RACE_ALIGN_WIDE | LEFT (4) in the wide screen mode the dots are moved
//   106.67 pixels left (as race_align_pos moves the map image; the constant
//   is 0xC2D5566C, a hair beyond T156 / T158's 106.66667).
void RaceMiniMap::drawMap(RaceOrganization *org, int targetCar)
{
    RaceEntryCar *car, *target;

    if (!enabled)
        return;
    pgl_push_matrix();
    vu0_scale(scaleX, scaleY, 1.0f);
    pgl_color(MINIMAP_DOT_COLOR);
    race_draw_image_rect(data->image, (f32)x, (f32)y, (f32)mapW, (f32)mapH,
                         alignX, alignY);
    if (alignX == (RACE_ALIGN_WIDE | RACE_ALIGN_LEFT) && g_002F424C != 0)
        pgl_translate(-106.66879f, 0.0f, 0.0f);
    target = NULL;
    for (car = org->entries->rankEnd; car != NULL; car = car->rankPrev) {
        if (car->number == targetCar)
            target = car;
        else
            drawCar(car, enemyDot);
    }
    if (target != NULL)
        drawCar(target, playerDot);
    pgl_pop_matrix();
}

// FUNC 0x0019E560 RaceMiniMap::drawCar
//   A car's dot at its projected position from the map's centre, by
//   car_display_level of its state: none for CAR_SHOW_HIDDEN (or <= 0, > 3), half
//   transparent for CAR_SHOW_MAP / CAR_SHOW_GHOST, opaque for CAR_SHOW_FULL.
//   A switch (egcs's compare tree: <= 0, < 3 likely to the faded case, == 3).
void RaceMiniMap::drawCar(RaceEntryCar *car, Tex1Image *dot)
{
    RaceCarState *state = car->state;
    f32 offX, offY;                         /* sp+0x0, sp+0x4 */
    f32 worldX, worldZ;
    int centerX, centerY;

    pgl_color(MINIMAP_DOT_COLOR);
    switch (car_display_level(state)) {
    case CAR_SHOW_MAP:
    case CAR_SHOW_GHOST:
        pgl_color(MINIMAP_DOT_FADED);
        break;
    case CAR_SHOW_FULL:
        break;
    default:                                /* CAR_SHOW_HIDDEN (or out of range) */
        return;
    }
    worldX = car_view_x(state, 0.0f);
    worldZ = car_view_z(state, 0.0f);
    data->project(&offX, &offY, worldX, worldZ);
    centerX = x + (mapW >> 1);
    centerY = y + (mapH >> 1);
    race_display_draw_tex(dot, (f32)centerX + offX, (f32)centerY + offY);
}

// FUNC 0x0019E678 RaceMiniMap::~RaceMiniMap
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (of 0x001959C8):
 * the vptr (0x0032E290), then ~RaceDisplayObjectBase with the caller's
 * flags. */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

/* T164's copy of 0x001959F0 (its own name string 0x0032FD48):
 * __si_type_info at 0x004B12A0, base __ti21RaceDisplayObjectBase
 * (gp-0x4650).  Weak so that T154's copy links too. */
#ifndef BOOT_OLD_GCC
// FUNC 0x0019E6A0 __tf11RaceMiniMap
extern "C" __attribute__((weak)) const type_info &__tf11RaceMiniMap(void)
{
    if (__ti11RaceMiniMap[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti11RaceMiniMap, "11RaceMiniMap",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti11RaceMiniMap;
}
#endif
