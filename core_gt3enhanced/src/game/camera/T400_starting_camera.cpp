/* T400 - StartingCamera (0x00202F60..0x002031D0).
 *
 * The camera of the race start: PasslineCamera on start.cam's first record,
 * its timeRange running from 0 to 1 while the start countdown
 * (RaceOrganization::countDown, frames) goes from 660 down to 60; the
 * resulting matrix is then turned and moved by the target's angles and
 * position (the record is relative to the car).
 *
 * Retail T400's .rodata opens with <new>'s "bad_alloc" (0x0033E618, nothing
 * references it: DATA_START) and ends with the type names 14StartingCamera,
 * t9UnitArena1Ui64: UnitArena<64> (T153) defined before the file's class.
 */
// DATA_START 0x0033E618
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after the file's own */
#include "game/camera/cameras_internal.h"
#include "game/camera/T400_starting_camera.h"
#include "game/enhanced.h"                  /* ENH_CAMERA_START_SEQUENCE */

/* The countdown frames the sweep's parameter runs over: Lupo 2 1200, half
 * as fast (L2 0x002176EC, cameras_sound.md C6); GT3 600. */
#if ENH_CAMERA_START_SEQUENCE
#define START_SWEEP_FRAMES 1200.0f
#else
#define START_SWEEP_FRAMES 600.0f
#endif

// FUNC 0x00202F60 StartingCamera::StartingCamera
StartingCamera::StartingCamera()
{
    /* implicit: PasslineCamera (compiler-generated: bases, members, deleting form) */
    /* PasslineCamera::PasslineCamera(), the vtable, then the inline
     * setType(3). */
    setType(3);
}

// FUNC 0x00202FA0 StartingCamera::~StartingCamera
StartingCamera::~StartingCamera()
{
    /* implicit: ~PasslineCamera (compiler-generated: bases, members, deleting form) */
    /* the vtable, then PasslineCamera::~PasslineCamera(this, flags) */
}

/* The same stores again after PasslineCamera's (the base's set up inlined
 * a second time, ? or a copy of its body), except timeRange.  The enhanced
 * engine: the set up with ownClock 0. */
// FUNC 0x00202FC8 StartingCamera::setup
void StartingCamera::setup(CameraTargetInfo *target, RaceCourse *course)
{
    setup(target, course, 0);
}

/* The enhanced engine (Lupo 2 L2 0x002175A0, cameras_sound.md C6): GT3's
 * set up, and the start state: running, ownClock (1 in replays: after the
 * countdown the sweep goes on by its own frames, update), its frame 0.
 * Lupo 2 takes the record from the course (its package's start / replay
 * camera, +0xA4 / +0xA8 by ownClock) or else from cameras/start.cam /
 * cameras/replay.cam; GT3's courses have none, and GT3 loads only
 * start.cam (T153), so the record is start.cam's, as before.  Callers:
 * CameraManager::init (T383), the 2-argument setup. */
void StartingCamera::setup(CameraTargetInfo *target, RaceCourse *course, int ownClock)
{
    PasslineCamera::setup(target, course);
    this->course = course;
    cameraData = NULL;
    if (course != NULL)
        cameraData = g_00351F94->records;
    this->ownClock = ownClock;
    gamma[0] = 1.0f;
    zoom[0] = 1.4281480f;                   /* 1 / tan(35 degrees) */
    clockFrame = 0;
    counter = 0;
    startState = 0;
}

/* Always returns 0 (every frame is a "first" one: no interpolation).
 * The enhanced engine (Lupo 2 L2 0x00217640, ENH_CAMERA_START_SEQUENCE,
 * cameras_sound.md C6): the sweep over 1200 frames (GT3 600); with
 * ownClock, countDown + 60 frames while the countdown runs and then 60 less
 * the frames since it ended; once held (startState 1) the sweep is at its
 * end and the next frames leave the view alone (2); the record mirrored for
 * a car on the right of the grid (an odd RaceEntryCar::order, Lupo 2's
 * start position). */
// FUNC 0x00203028 StartingCamera::update
int StartingCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                               CarProgress *progress)
{
    f32 param;
    f32 x, y, z, yaw, pitch, roll;
    int intrWasOn;

#if ENH_CAMERA_START_SEQUENCE
    if (startState == 2)
        return 0;                           /* stopped: the last view stays */
    if (ownClock) {
        param = (f32)org->countDown;
        if (param <= 0.0f) {
            param = (f32)(60 - clockFrame);
            clockFrame = clockFrame + 1;
        } else {
            param = param + 60.0f;
        }
    } else {
        param = (f32)(org->countDown - 60);
    }
#else
    param = (f32)(org->countDown - 60);
#endif
    param = 1.0f - param / START_SWEEP_FRAMES;
    if (1.0f < param)
        param = 1.0f;
    if (param < 0.0f)
        param = 0.0f;
    timeRange = param;
#if ENH_CAMERA_START_SEQUENCE
    if (startState != 0) {                  /* held: at the end, then stopped */
        timeRange = 1.0f;
        startState = startState + 1;
    }
    if (target->car[0] != NULL)
        inverseX = target->car[0]->getOrder() & 1;
#endif
    PasslineCamera::update(org, input, target, progress);
    x = target->pos[0];
    y = target->pos[1];
    z = target->pos[2];
    yaw = target->yaw;
    pitch = target->pitch;
    roll = target->roll;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    pgl_load_matrix(matrix[0]);
    vu0_rotate_z(roll * 57.29578f);
    vu0_rotate_x(-pitch * 57.29578f);
    vu0_rotate_y(-yaw * 57.29578f);
    pgl_translate(-x, -y, -z);
    pgl_get_matrix(matrix[0]);
    pgl_pop_matrix();
    intr_resume(intrWasOn);
    return 0;
}

// FUNC 0x00203178 StartingCamera::getMount(void)
int StartingCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00203180 __tf14StartingCamera
extern "C" void *__tf14StartingCamera(void)
{
    if (__ti14StartingCamera[0] == 0) {
        __tf14PasslineCamera();
        __rtti_si(__ti14StartingCamera, "14StartingCamera",
                  (const type_info *)__ti14PasslineCamera);
    }
    return __ti14StartingCamera;
}
#endif
