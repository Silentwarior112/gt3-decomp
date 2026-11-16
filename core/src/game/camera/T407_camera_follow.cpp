/* T407: two small dampers of DesignerCamera (T398; no header of their own:
 * game/camera/T380_camera_spring.h).  CameraFollow chases a target, faster (x gain)
 * while the target is above it; CameraBank springs an angle back to 0.
 *
 * Retail's .rodata after T379's type name (0x0033EDF9) has three files'
 * sections with nothing referenced by code: "bad_alloc" + t9UnitArena1Ui64
 * (0x0033EE00), then twice "bad_alloc" + 14RaceCameraBase + t9UnitArena1Ui64
 * (0x0033EE28, 0x0033EE68), then T383's "bad_alloc" (0x0033EEA8).  T408 and
 * T382 call RaceCourse (so they saw UnitArena<64>): theirs are the last two.
 * The first is one of T380 / T407 / T381; taken as T407's: DesignerCamera's
 * helpers, and retail's T398 (DesignerCamera) sees exactly UnitArena<64>,
 * no RaceCameraBase (T379, which shares CameraSway's header with T381, sees
 * nothing; the data pass notes, ref/data/u091/notes.md). */
// DATA_START 0x0033EE00
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T380_camera_spring.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00206B30 CameraFollow::reset
void CameraFollow::reset(f32 v, f32 gain)
{
    vel = 0.0f;
    target = v;
    x = v;
}

// FUNC 0x00206B40 CameraFollow::update
f32 CameraFollow::update(f32 target, f32 gain)
{
    f32 from = x;
    f32 force, v;

    this->target = target;
    force = (target - from) * 30.0f;
    if (from < target)
        force = force * gain;
    vel = vel + (force + vel * -10.0f) / U091_FPS;
    if (10.0f < vel)
        vel = 10.0f;
    v = vel;
    if (v < -30.0f) {
        vel = -30.0f;
        v = -30.0f;
    }
    x = from + v / U091_FPS;
    return x;
}

// FUNC 0x00206BE8 CameraBank::reset
//   (one update step is run and its angle thrown away)
void CameraBank::reset(f32 a)
{
    angle = a;
    vel = 0.0f;
    update();
    angle = a;
}

// FUNC 0x00206C28 CameraBank::update
f32 CameraBank::update()
{
    f32 from = angle;
    f32 v;

    vel = vel + ((0.0f - from) * 30.0f + vel * -10.0f) / U091_FPS;
    if (900.0f < vel)
        vel = 900.0f;
    v = vel;
    if (v < -900.0f) {
        vel = -900.0f;
        v = -900.0f;
    }
    angle = from + v / U091_FPS;
    return angle;
}
