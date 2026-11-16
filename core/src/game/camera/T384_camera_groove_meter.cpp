/* T384 (0x0020A2E0..0x0020A9C0): CameraGrooveMeter - eight arcs round the
 * screen centre (320, 224) with random radii, start angles, sweeps and spins;
 * start() runs them for 180 frames, during which draw() shrinks them towards
 * their radii and fades them out.  Plus the arc drawing helpers.  (Unused in
 * the game: update and draw have no callers, and start's only caller is
 * reset, which sets count back to 0.) */
/* Retail T384's .rodata (0x0033F060) is the vtable and "17CameraGrooveMeter"
 * only: no <new>, and no header with another polymorphic class of inline
 * virtuals (the camera manager's RaceCameraBase, the GameStatus's garage
 * classes, T153's UnitArena<64>). */
#include "game/camera/camera_manager_internal.h"
#include "game/camera/T384_camera_groove_meter.h"      /* CameraGrooveMeter, GrooveRing, F_0020A2E0 .. F_0020A360 */
#include "game/camera/T379_camera_rot_damper.h"        /* angle_wrap360 */
#include "engine/pgl/G001_flare.h"                    /* gfx_perf_counter0, pgl_color4f, gs_draw_line_f */
#include "engine/media/G002_mpeg_frameq.h"              /* sinf, cosf */

#define GROOVE_FRAMES   180                     /* start(): the frames the meter runs */
#define GROOVE_CENTRE_X 320.0f                  /* the screen centre */
#define GROOVE_CENTRE_Y 224.0f

// FUNC 0x0020A2E0 F_0020A2E0
/* The x of the point at angle `rad' on a circle of radius r round x cx. */
f32 F_0020A2E0(f32 cx, f32 r, f32 rad)
{
    return cx + r * cosf(rad);
}

// FUNC 0x0020A320 F_0020A320
/* The y of the same point (y grows downwards on the screen). */
f32 F_0020A320(f32 cy, f32 r, f32 rad)
{
    return cy - r * sinf(rad);
}

// FUNC 0x0020A360 F_0020A360
/* Draws an arc of radius r round (cx, cy) from startDeg over sweepDeg (and
 * one step more), in line segments: the step count doubles from 16 until a
 * step is shorter than 8 (r tan(2 pi / n) < 8).  (Matching: the segment
 * loop is a do / while that steps first; `angle + start' is written out at
 * both calls, a variable for it takes another register.) */
void F_0020A360(f32 cx, f32 cy, f32 r, f32 startDeg, f32 sweepDeg)
{
    f32 start = startDeg * 3.1415927f / 180.0f;
    f32 sweep = sweepDeg * 3.1415927f / 180.0f;
    f32 steps = 16.0f;
    f32 stepRad, angle, x, y, prevX, prevY, rad;

    for (;;) {
        rad = 6.2831855f / steps;
        if (r * (sinf(rad) / cosf(rad)) < 8.0f)
            break;
        steps += steps;
    }
    angle = 0.0f;
    stepRad = 6.2831855f / steps;              /* (the same constant, another .sdata copy) */
    x = F_0020A2E0(cx, r, start + angle);
    y = F_0020A320(cy, r, start + angle);
    do {
        angle += stepRad;
        prevY = y;
        prevX = x;
        x = F_0020A2E0(cx, r, angle + start);
        y = F_0020A320(cy, r, angle + start);
        gs_draw_line_f(prevX, prevY, x, y);
    } while (angle <= sweep);
}

// FUNC 0x0020A500 CameraGrooveMeter::CameraGrooveMeter
/* The vptr (+0x98), rng (Random's 0x87654321), then reset(). */
CameraGrooveMeter::CameraGrooveMeter()
{
    reset();
}

// FUNC 0x0020A540 CameraGrooveMeter::~CameraGrooveMeter
/* Nothing to destroy; egcs's flags test calls operator delete for bit 0. */
CameraGrooveMeter::~CameraGrooveMeter()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0020A570 CameraGrooveMeter::reset
void CameraGrooveMeter::reset()
{
    rng.setSeed((u32)gfx_perf_counter0());
    start(0.0f, 0.0f);
    count = 0;
}

// FUNC 0x0020A5B8 CameraGrooveMeter::start
/* New random rings: radius 50 + 10 i + 0..2, start angle 0..360, sweep
 * 80..280, speed +-1..3.  (Matching: rings[i].field at each use: retail
 * strength-reduces each field into a pointer of its own.) */
void CameraGrooveMeter::start(f32 a, f32 b)
{
    int i;

    if (a < 0.0f)
        a = 0.0f;
    if (a > 1.0f)
        a = 1.0f;
    if (b < 0.0f)
        b = 0.0f;
    if (b > 1.0f)
        b = 1.0f;
    levelA = a;
    count = GROOVE_FRAMES;
    angle = 0.0f;
    levelB = b;
    spin = b * 5.0f + 0.2f;
    for (i = 0; i < 8; i++) {
        rng.next();
        rings[i].radius = (f32)i * 10.0f + 50.0f + 2.0f * rng.next01();
        rng.next();
        rings[i].angle = angle_wrap360(rng.next01() * 360.0f);
        rng.next();
        rings[i].sweep = rng.next01() * 200.0f + 80.0f;
        rng.next();
        rings[i].speed = 2.0f * rng.next01() + 1.0f;
        rng.next();
        if (rng.next01() < 0.5f)
            rings[i].speed = -rings[i].speed;
    }
}

// FUNC 0x0020A798 CameraGrooveMeter::update
void CameraGrooveMeter::update()
{
    if (count > 0)
        count = count - 1;
    angle = angle_wrap360(angle + spin);
}

// FUNC 0x0020A7E0 CameraGrooveMeter::draw
/* While running: radius scale 2 (count / 180)^8 + 1 (the 2x + 1 computed in
 * double: the soft-float fptodp, dpadd twice, dptofp), alpha count / 90
 * clamped to 0..1, the colour from levelB.  (Matching: one variable for the
 * count / 180 and the scale, retail keeps both in $f21; levelB read once
 * after the double arithmetic.) */
void CameraGrooveMeter::draw()
{
    if (count > 0) {
        f32 frames = (f32)count;
        f32 scale = frames / 180.0f;
        f32 alpha;
        f32 halfLevel, level;
        int i;

        /* implicit: fptodp, dpadd, dptofp (double arithmetic, soft-float) */
        scale = (f32)((f64)(scale * scale * scale * scale * scale * scale * scale * scale) * 2.0 + 1.0);
        level = levelB;
        alpha = frames / 90.0f;
        if (alpha > 1.0f)
            alpha = 1.0f;
        if (alpha < 0.0f)
            alpha = 0.0f;
        halfLevel = level * 0.5f;
        pgl_color4f(halfLevel + 0.2f, 0.8f - level * 0.6f, 0.5f - halfLevel, alpha);
        for (i = 0; i < 8; i++)
            F_0020A360(GROOVE_CENTRE_X, GROOVE_CENTRE_Y, rings[i].radius * scale,
                       rings[i].angle + angle * rings[i].speed, rings[i].sweep);
    }
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0020A980 __tf17CameraGrooveMeter
/* Compiler-generated: the __user_type_info at 0x00353430 (gp-0x45C0). */
extern "C" void *__tf17CameraGrooveMeter(void)
{
    if (__ti17CameraGrooveMeter[0] == 0)
        __rtti_user(__ti17CameraGrooveMeter, "17CameraGrooveMeter");
    return __ti17CameraGrooveMeter;
}
#endif
