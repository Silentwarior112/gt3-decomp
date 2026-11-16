/* T106 - the prize car presentation helpers and gt2k_menu::CarGetMenuCamera.
 * 0x0013B848..0x0013C8F0.
 *
 * Used by T107 (CarsItem, the prize car screen) and T061 (CarGetControl):
 *   - CarRoulette (CarsItem +0x100): a cursor running over the 1..4 cars;
 *     start() spins it at 0.1 slot a frame, stop() computes the constant
 *     deceleration that ends it, after whole turns worth about 12 slots, in
 *     the middle of the `target' slot (the car that is won).
 *   - CarTurntable (CarGetControl +0xEA4): the cars on a circle sized from
 *     their dimensions; the table turns at -12.5 degrees a second and each
 *     car on itself at 60 degrees a second, both slowing down once
 *     `stopping' is set.
 *   - draw_table_floor: the textured square under the cars.
 *   - CarGetMenuCamera (CarsItem +0xED4): a spring camera that first looks
 *     at the whole table from above, then, two `next' requests later,
 *     follows the car that was won.
 * draw_ring_arc / draw_ring (rings on the floor) and the two CarTurntable
 * matrix helpers have no callers (? debug drawing, or an earlier floor). */
/* The includes: retail T106's .rodata (0x00319290..0x00319357) is
 * SimpleCameraBase::print's format string (T412's inline function: the
 * original included T412's header first), CarTurntable's double constants,
 * the jump table of CarGetMenuCamera::update and its double constants, the
 * vtable and CarGetMenuCamera's type name alone.  ref/data/u020/notes.md. */
// DATA_START 0x00319290
#include "game/camera/T412_simple_camera_base.h"   /* (no use: SimpleCameraBase::print's literal) */
#include "game/menu/gtmode/T106_car_get_camera.h"       /* own */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_alloc_tail, pgl_draw_strip, pgl_sincos_deg,
                                               pgl_translate, vu0_rotate_y, glu_look_at,
                                               gfx_set_texture_regs, Tex1Image */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "engine/input/T475_pad_manager.h"          /* g_002EB170 (the spare Control) */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250 (the VramAllocator of g_002F4230) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pdistd/T531_random.h"               /* random_float */
#define U020_T106                           /* trophy_get_internal.h: this unit's constants */
#include "game/menu/gtmode/trophy_get_internal.h"

/* CarRoulette's speeds (slots a frame). */
#define ROULETTE_START_SPEED    0.1f
#define ROULETTE_MIN_SPEED      (1.0f / 60.0f)  /* while stopping */
#define ROULETTE_TURN_SLOTS     12              /* the whole turns stop() adds: count * (12 / count) slots */

/* CarTurntable's speeds (degrees a frame, 60 frames a second). */
#define TURNTABLE_SPEED         (-12.5f / 60.0f)    /* the table */
#define TURNTABLE_CAR_SPIN      1.0f                /* each car on itself */
#define TURNTABLE_CAR_SPIN_MIN  0.35f               /* the cars' spin once stopping */
#define TURNTABLE_SLOW_DOWN     (1.0f / 720.0f)     /* a frame, once stopping */

/* The vertex formats of pgl_draw_strip (positions, texture coordinates). */
struct Vec3f {
    f32 x, y, z;
};
struct Vec2f {
    f32 u, v;
};
/* draw_table_floor's primitive: 5 qwords of gfx_alloc_tail. */
struct FloorQuad {
    Vec3f vert[4];                          /* 0x00 */
    Vec2f uv[4];                            /* 0x30 */
};

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* CarRoulette                                                              */
/* ------------------------------------------------------------------------ */

/* Callers: CarRoulette::update, CarsItem::virtual_1 (T107). */
// FUNC 0x0013B848 gt2k_menu::car_roulette_slot
int car_roulette_slot(int pos)
{
    return pos;
}

/* remain is not set.  (CarsItem's constructor calls it.) */
// FUNC 0x0013B850 gt2k_menu::CarRoulette::CarRoulette
CarRoulette::CarRoulette(s8 n, s8 t)
{
    count = n;
    target = t;
    pos = 0.0f;
    speed = 0.0f;
    accel = 0.0f;
    state = 0;
}

// FUNC 0x0013B880 gt2k_menu::CarRoulette::start
void CarRoulette::start()
{
    state = 1;
    speed = ROULETTE_START_SPEED;
}

/* The distance to the middle of the target slot (wrapped to >= 0), plus
 * count * (12 / count) slots (whole turns), covered with a constant
 * deceleration from the current speed: v^2 / (2 (d + 1)). */
// FUNC 0x0013B898 gt2k_menu::CarRoulette::stop
void CarRoulette::stop()
{
    f32 distance = ((f32)target + 0.5f) - pos;
    state = 2;
    while (distance < 0.0f)
        distance += (f32)count;
    remain = (f32)(count * (ROULETTE_TURN_SLOTS / count)) + distance;
    accel = -(speed * speed) / (2.0f * (remain + 1.0f));
}

/* While stopping the speed does not drop below 1/60 slot a frame; it ends
 * once the distance is covered and the cursor is in the target slot.  The
 * position wraps to 0 .. count. */
// FUNC 0x0013B960 gt2k_menu::CarRoulette::update
void CarRoulette::update()
{
    switch (state) {
    case 0:                                 /* idle */
    case 1:                                 /* spinning */
        break;
    case 2:                                 /* stopping */
        speed = speed + accel;
        remain = remain - speed;
        if (speed <= ROULETTE_MIN_SPEED)
            speed = ROULETTE_MIN_SPEED;
        if (remain <= 0.0f && car_roulette_slot((int)pos) == target) {
            speed = 0.0f;
            state = 0;
        }
        break;
    }
    pos = pos + speed;
    while ((f32)count <= pos)
        pos = pos - (f32)(int)(pos / (f32)count) * (f32)count;
}

/* ------------------------------------------------------------------------ */
/* CarTurntable                                                             */
/* ------------------------------------------------------------------------ */

/* halfSize / height: per car, in metres (T107 CarsItem::setupLayout: T232
 * MenuCarSpec::length * 0.001 * 0.5 and MenuCarSpec::height * 0.001, both
 * in mm): `spacing' is the largest sum of two neighbours' half lengths,
 * x 1.1.  The radius puts neighbours `spacing' apart (1 car: 0; 2: half of
 * it; 3: side / sqrt 3, a double constant; 4: side / sqrt 2; more:
 * unchanged).  The cars' headings start 80..100 degrees apart from a
 * random 0..20. */
// FUNC 0x0013BA80 gt2k_menu::CarTurntable::setup
void CarTurntable::setup(const f32 *halfSize, const f32 *height)
{
    int i;
    for (i = 0; i < count; i++) {
        f32 carHeight = height[i] * 1.1f;
        f32 pairSpacing = (halfSize[i] + halfSize[(i + 1) % count]) * 1.1f;
        if (i == 0) {
            spacing = pairSpacing;
            maxHeight = carHeight;
        } else {
            spacing = pairSpacing < spacing ? spacing : pairSpacing;
            maxHeight = carHeight < maxHeight ? maxHeight : carHeight;
        }
    }
    switch (count) {
    case 1:
        radius = 0.0f;
        break;
    case 2:
        radius = spacing * 0.5f;
        break;
    case 3:
        /* soft-float: fptodp, dpmul by the double 0x003192A8, dptofp */
        /* implicit: fptodp, dpmul, dptofp (soft-float double arithmetic) */
        radius = (f32)((f64)spacing * 0.5773502691896257);
        break;
    case 4:
        radius = spacing * 0.707107f;
        break;
    }
    f32 prevAngle = random_float() * 20.0f;
    for (i = 0; i < count; i++) {
        carAngle[i] = random_float() * 20.0f + 80.0f + prevAngle;
        while (carAngle[i] >= 360.0f)
            carAngle[i] -= 360.0f;
        prevAngle = carAngle[i];
    }
    speed = TURNTABLE_SPEED;
    carSpin = TURNTABLE_CAR_SPIN;
    angle = 0.0f;
}

/* angle += d, wrapped once to 0 .. 360 (? name; an inline helper of the
 * original: retail walks carAngle with a pointer and reloads carSpin on
 * every pass, the code of a reference argument; indexed in place, loop.c
 * hoists carSpin and keeps the index: 90.9%). */
static inline void add_angle(f32 &angle, f32 d)
{
    f32 a = angle + d;
    angle = a;
    if (a >= 360.0f)
        angle = a - 360.0f;
    else if (a < 0.0f)
        angle = a + 360.0f;
}

/* Once stopping: the cars' spin drops by 1/720 a frame down to 0.35 (an
 * ABS() compare in double precision, the bound's sign tested at run time
 * as well); the table's speed rises by 1/720 a frame until it would change
 * sign, then 0.  Angles wrap once to 0 .. 360. */
// FUNC 0x0013BCE0 gt2k_menu::CarTurntable::update
void CarTurntable::update()
{
    if (stopping != 0) {
        /* soft-float: dpcmp (three compares), fptodp, dpsub */
        /* implicit: dpcmp, fptodp, dpsub (soft-float double arithmetic) */
        f64 limit = TURNTABLE_CAR_SPIN_MIN;     /* (f64)0.35f, 0x003192B0 */
        if (limit < 0.0)
            limit = -TURNTABLE_CAR_SPIN_MIN;    /* 0x003192B8 */
        f64 spin = carSpin;
        if (spin < 0.0)
            spin = 0.0 - spin;
        if (limit < spin)
            carSpin = carSpin + -TURNTABLE_SLOW_DOWN;
        else
            carSpin = TURNTABLE_CAR_SPIN_MIN;
        speed = speed + TURNTABLE_SLOW_DOWN;
        if (speed * TURNTABLE_SPEED < 0.0f)
            speed = 0.0f;
    }
    for (int i = 0; i < count; i++)
        add_angle(carAngle[i], carSpin);
    add_angle(angle, speed);
}

/* Car i at i * 360 / count degrees on the circle. */
// FUNC 0x0013BE88 gt2k_menu::CarTurntable::getCarPos
void CarTurntable::getCarPos(int i, f32 *x, f32 *z)
{
    f32 sinA, cosA;                         /* sp+0x0, sp+0x4 */
    pgl_sincos_deg((f32)i * 360.0f / (f32)count, &sinA, &cosA);
    *x = radius * cosA;
    *z = radius * sinA;
}

/* Callers: CarGetMenuCamera::update and three T107 functions. */
// FUNC 0x0013BF18 gt2k_menu::CarTurntable::getWorldPos
void CarTurntable::getWorldPos(int i, f32 *x, f32 *z)
{
    f32 tableX, tableZ;                     /* sp+0x0, sp+0x4 */
    f32 sinA, cosA;                         /* sp+0x8, sp+0xC */
    getCarPos(i, &tableX, &tableZ);
    pgl_sincos_deg(angle, &sinA, &cosA);
    *x = tableX * cosA + tableZ * sinA;
    *z = -tableX * sinA + tableZ * cosA;
}

/* The current matrix turned by the table's angle (no callers). */
// FUNC 0x0013BFA0 gt2k_menu::CarTurntable::rotate
void CarTurntable::rotate()
{
    vu0_rotate_y(angle);
}

/* To car i's place on the table, turned by its heading (no callers). */
// FUNC 0x0013BFC0 gt2k_menu::CarTurntable::transformCar
void CarTurntable::transformCar(int i)
{
    f32 x, z;                               /* sp+0x0, sp+0x4 */
    getCarPos(i, &x, &z);
    pgl_translate(x, 0.0f, z);
    vu0_rotate_y(carAngle[i]);
}

/* ------------------------------------------------------------------------ */
/* Floor drawing                                                            */
/* ------------------------------------------------------------------------ */

/* A flat ring between radius 1 and `radius' from angle `from' to `to', 15
 * steps (a strip of 32 vertices); spans over 120 degrees are halved
 * recursively.  Only called by itself.  The abs is the volatile abs.s
 * (boot_fabsf: retail loads 120 after it); `a = from' after the allocation
 * keeps `from' in $f21 across the call (reorg puts the copy in the jal's
 * delay slot); the int counter with a pointer per step is what loop.c
 * turns into retail's signed `v < verts + 32' test without an entry test. */
// FUNC 0x0013C018 gt2k_menu::draw_ring_arc
void draw_ring_arc(f32 from, f32 to, f32 radius)
{
    f32 span = to - from;
    if (boot_fabsf(span) > 120.0f) {
        f32 mid = (from + to) * 0.5f;
        draw_ring_arc(from, mid, radius);
        draw_ring_arc(mid, to, radius);
        return;
    }
    Vec3f *verts = (Vec3f *)gfx_alloc_tail(0x18);
    f32 a = from;
    f32 step = span / 15.0f;
    for (int i = 0; i < 16; i++) {
        f32 sinA, cosA;                     /* sp+0x0, sp+0x4 */
        Vec3f *v = &verts[i * 2];
        pgl_sincos_deg(a, &sinA, &cosA);
        a += step;
        v[0].x = cosA;
        v[0].y = 0.0f;
        v[0].z = sinA;
        v[1].x = cosA * radius;
        v[1].y = 0.0f;
        v[1].z = sinA * radius;
    }
    pgl_draw_strip(verts, NULL, NULL, NULL, 32);
}

/* The whole ring at y = -0.01 as four 90-degree strips of 16 steps of 6
 * degrees, coloured innerColor at radius 1 and outerColor at `radius'.
 * No callers.  The angle step is in the for's increment (retail hoists
 * -0.01 before 6.0 and fills the call's delay slot with the counter). */
// FUNC 0x0013C170 gt2k_menu::draw_ring
void draw_ring(u32 outerColor, u32 innerColor, f32 radius)
{
    for (int quarter = 0; quarter < 4; quarter++) {
        f32 a = (f32)quarter * 90.0f;
        Vec3f *verts = (Vec3f *)gfx_alloc_tail(0x18);
        u32 *colors = (u32 *)gfx_alloc_tail(8);
        Vec3f *v = verts;
        u32 *c = colors;
        for (int k = 0; k < 16; k++, a += 6.0f) {
            f32 sinA, cosA;                 /* sp+0x0, sp+0x4 */
            pgl_sincos_deg(a, &sinA, &cosA);
            v[0].x = cosA;
            v[0].y = -0.01f;
            v[0].z = sinA;
            c[0] = innerColor;
            c[1] = outerColor;
            v[1].x = cosA * radius;
            v[1].y = -0.01f;
            v[1].z = cosA * radius;         /* sic: cos again (sin * radius meant; bug of the
                                               unreferenced original) */
            v += 2;
            c += 2;
        }
        pgl_draw_strip(verts, NULL, colors, NULL, 32);
    }
}

/* The textured square -1..1 on the y = 0 plane (T107 scales it to the
 * table).  Nothing without a texture.  The texture set is made resident
 * (g_002F4250, the VramAllocator of g_002F4230) and its first texture's GS
 * registers are set. */
// FUNC 0x0013C2D8 gt2k_menu::draw_table_floor
void draw_table_floor(Tex1Image *tex)
{
    if (tex == NULL)
        return;
    FloorQuad *quad = (FloorQuad *)gfx_alloc_tail(5);
    quad->vert[0].x = -1.0f;
    quad->vert[0].y = 0.0f;
    quad->vert[0].z = -1.0f;
    quad->vert[1].x = 1.0f;
    quad->vert[1].y = 0.0f;
    quad->vert[1].z = -1.0f;
    quad->vert[2].x = -1.0f;
    quad->vert[2].y = 0.0f;
    quad->vert[2].z = 1.0f;
    quad->vert[3].x = 1.0f;
    quad->vert[3].y = 0.0f;
    quad->vert[3].z = 1.0f;
    quad->uv[0].u = 0.0f;
    quad->uv[0].v = 0.0f;
    quad->uv[1].u = 1.0f;
    quad->uv[1].v = 0.0f;
    quad->uv[2].u = 0.0f;
    quad->uv[2].v = 1.0f;
    quad->uv[3].u = 1.0f;
    quad->uv[3].v = 1.0f;
    g_002F4250.loadTex1(tex);
    gfx_set_texture_regs(tex->tex);
    pgl_draw_strip(quad, NULL, NULL, quad->uv, 4);
}

/* ------------------------------------------------------------------------ */
/* CarGetMenuCamera                                                         */
/* ------------------------------------------------------------------------ */

/* Once a frame (CarsItem::virtual_1): the state step, the inherited springs
 * (MenuCarCamera::virtual_8, a direct call, with the spare Control), then
 * the eye springs, which virtual_8 leaves alone (MenuCarCamera's inline
 * updateTrans_: three MotionValue::update).  selected: the slot the
 * roulette shows, -1 if none; count: the number of cars.  (Jump table
 * 0x003192C0, 6 cases.) */
// FUNC 0x0013C3A0 gt2k_menu::CarGetMenuCamera::update
void CarGetMenuCamera::update(CarTurntable *table, int selected, int count)
{
    switch (state) {
    case 0: {
        /* Overview: high above the table, looking down at its centre;
         * tan 30 degrees and 1 / tan 10 degrees as double constants
         * (soft-float: fptodp, dpmul, dpadd, dptofp). */
        f32 half = table->spacing * 0.5f + table->radius;
        f32 height = table->maxHeight;
        /* implicit: fptodp, dpmul, dpadd, dptofp (soft-float double arithmetic) */
        f32 eyeY = (f32)((f64)height + (f64)half * 0.57735);           /* 0x003192D8 */
        f32 dist = (f32)((f64)eyeY * 5.67128);                         /* 0x003192E0 */
        f32 back = 2.4f;
        if (count < 2)
            back = 2.4f * 1.5f;             /* 0x40666667, the folded product (not 3.6f) */
        setTransGoal(0.0f, height + height, half * back, 0.2f, 1.0f);
        setTrans(0.0f, eyeY, 0.0f);
        setInterest(0.0f, 0.0f, -dist);
        setInterestGoal(0.0f, 0.0f, 0.0f, 1.9f, 4.0f);
        state = 1;
    }
        /* fall through */
    case 1:
        if (next != 0) {
            next = 0;
            state = 2;
        }
        break;
    case 2:
        state = 3;
        /* fall through */
    case 3:
        if (next != 0) {
            next = 0;
            state = 4;
        }
        break;
    case 4:
        state = 5;
        /* fall through */
    case 5: {
        /* Follow the selected car: look at it from behind and above. */
        f32 x, z;                           /* sp+0x0, sp+0x4 */
        table->getWorldPos(selected, &x, &z);
        setInterestGoal(x, table->maxHeight * 0.3f, z, 1.9f, 4.0f);
        setTransGoal(x, table->maxHeight * 1.8f, z + table->spacing * 1.6f, 1.9f, 4.0f);
        break;
    }
    }
    MenuCarCamera::virtual_8(&g_002EB170);
    updateTrans_();
}

/* The view (slot 2, replacing MenuCarCamera's): a look-at from trans to
 * interest, up = (0, -dz, dy) / d with d = |(dy, dz)| kept at least 0.01
 * from 0 (the negative branch cannot happen with a square root).  Retail
 * reads the six values through the MotionValue pointers (MenuCarCamera's
 * getTransX_ .. getInterestZ_ accessors, kept in a0..a3 / v0 / v1) and has
 * the volatile sqrt.s (boot_sqrtf_v: 0.01 is loaded after it). */
// FUNC 0x0013C668 gt2k_menu::CarGetMenuCamera::applyView
void CarGetMenuCamera::applyView()
{
    MotionValue *tx = getTransX_(), *ty = getTransY_(), *tz = getTransZ_();
    MotionValue *ix = getInterestX_(), *iy = getInterestY_(), *iz = getInterestZ_();
    f32 dy = iy->value - ty->value;
    f32 dz = iz->value - tz->value;
    f32 d = boot_sqrtf_v(dz * dz + dy * dy);
    if (d < 0.01f && d >= 0.0f)
        d = 0.01f;
    else if (d > -0.01f && d < 0.0f)
        d = -0.01f;
    glu_look_at(tx->value, ty->value, tz->value, ix->value, iy->value, iz->value,
                0.0f, -dz / d, dy / d);
}

// FUNC 0x0013C750 gt2k_menu::CarGetMenuCamera::requestNext
/* inline in game/menu/gtmode/T106_car_get_camera.h (CarsItem::virtual_1 expands it);
 * egcs emits this copy (no callers): next = 1 */

// FUNC 0x0013C760 gt2k_menu::CarGetMenuCamera::CarGetMenuCamera
/* inline in game/menu/gtmode/T106_car_get_camera.h (CarsItem's constructor expands it);
 * egcs emits this copy (no callers): MenuCarCamera's inline constructor (the
 * 16 MotionValues zeroed by theirs, mode 0), the vptr, state and next 0 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * MenuCarCamera). */
// FUNC 0x0013C880 __tfQ29gt2k_menu16CarGetMenuCamera
extern "C" void *__tfQ29gt2k_menu16CarGetMenuCamera(void)
{
    if (__tiQ29gt2k_menu16CarGetMenuCamera[0] == 0) {
        __tf13MenuCarCamera();
        __rtti_si(__tiQ29gt2k_menu16CarGetMenuCamera, "Q29gt2k_menu16CarGetMenuCamera",
                  (const type_info *)__ti13MenuCarCamera);
    }
    return __tiQ29gt2k_menu16CarGetMenuCamera;
}
#endif

// FUNC 0x0013C8D0 gt2k_menu::CarGetMenuCamera::~CarGetMenuCamera
/* implicit (game/menu/gtmode/T106_car_get_camera.h); egcs emits this copy for the
 * vtable: MenuCarCamera's implicit destructor, i.e. CameraBase::~CameraBase
 * (this, flags), which does the operator delete */
/* implicit: ~CameraBase (base destructor) */
