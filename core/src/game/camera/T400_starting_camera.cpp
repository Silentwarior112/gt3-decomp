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
 * a second time, ? or a copy of its body), except timeRange. */
// FUNC 0x00202FC8 StartingCamera::setup
void StartingCamera::setup(CameraTargetInfo *target, RaceCourse *course)
{
    PasslineCamera::setup(target, course);
    this->course = course;
    cameraData = NULL;
    if (course != NULL)
        cameraData = g_00351F94->records;
    gamma[0] = 1.0f;
    zoom[0] = 1.4281480f;                   /* 1 / tan(35 degrees) */
    counter = 0;
}

/* Always returns 0 (every frame is a "first" one: no interpolation). */
// FUNC 0x00203028 StartingCamera::update
int StartingCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                               CarProgress *progress)
{
    f32 param = (f32)(org->countDown - 60);
    f32 x, y, z, yaw, pitch, roll;
    int intrWasOn;

    param = 1.0f - param / 600.0f;
    if (1.0f < param)
        param = 1.0f;
    if (param < 0.0f)
        param = 0.0f;
    timeRange = param;
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
