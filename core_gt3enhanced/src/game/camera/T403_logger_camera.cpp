/* T403 - LoggerCamera (0x002047D0..0x00204B10).
 *
 * A fixed chase camera: `distance' behind and `height' above the car,
 * pitched down by `pitch' degrees, turning with the car's heading (not its
 * pitch or roll).  Field of view 80 degrees.
 *
 * Retail T403's .rodata opens with <new>'s "bad_alloc" (0x0033E960, nothing
 * references it: DATA_START) and ends with the type names 12LoggerCamera,
 * 14RaceCameraBase, t9UnitArena1Ui64: the file saw RaceCameraBase (T383),
 * defined after UnitArena<64> (T153) and before the file's class.
 */
// DATA_START 0x0033E960
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/cameras_internal.h"
#include "game/camera/T403_logger_camera.h"

// FUNC 0x002047D0 LoggerCamera::LoggerCamera
LoggerCamera::LoggerCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* VariableCameraBase::VariableCameraBase(), the vtable, then setType(0)
     * inline (type and trueType). */
    type = 0;
    trueType = 0;
}

// FUNC 0x00204810 LoggerCamera::~LoggerCamera
LoggerCamera::~LoggerCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* the vtable, then VariableCameraBase::~VariableCameraBase(this, flags) */
}

/* Set up (the target is not used). */
// FUNC 0x00204838 LoggerCamera::virtual_8
void LoggerCamera::virtual_8(CameraTargetInfo *target)
{
    f32 z;

    gamma[0] = 1.0f;
    z = 1.0f / tanf(0.69813174f);           /* half of 80 degrees */
    targetLength[0] = 100.0f;
    counter = 0;
    bankForcibly = 0.0f;
    zoom[0] = z;
    height = 5.0f;
    distance = 10.0f;
    fov = 70.0f;
    pitch = 20.0f;
}

/* Always returns 1.  input->getButtonsOn() first when there is an input
 * (the result is not used). */
// FUNC 0x002048C8 LoggerCamera::update
int LoggerCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                             CarProgress *progress)
{
    RaceCarState *car;
    int intrWasOn;

    if (input != NULL)
        input->getButtonsOn();              /* Control slot 5 (vptr +0xC0) */
    if (target->changed != 0)
        doppler = 0;
    car = target->car[0]->state;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_rotate_x(pitch);
    pgl_translate(0.0f, -height, -distance);
    vu0_rotate_y(-car_yaw(car, 0.0f) * 57.29578f);
    {
        f32 negX = -car_view_x(car, 0.0f);
        f32 negY = -car_view_y(car, 0.0f);

        pgl_translate(negX, negY, -car_view_z(car, 0.0f));
    }
    pgl_get_matrix(matrix[0]);
    pgl_pop_matrix();
    intr_resume(intrWasOn);
    return 1;
}

// FUNC 0x002049F0 LoggerCamera::virtual_9
int LoggerCamera::virtual_9()
{
    return 2;
}

// FUNC 0x002049F8 LoggerCamera::getEffect
int LoggerCamera::getEffect()
{
    return effect;
}

/* the projection, the focal length scaled by the zoom (the inline getZoom:
 * retail loads the zoom first, into its own register) */
// FUNC 0x00204A00 LoggerCamera::setProjection
void LoggerCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip,
                proj->farClip, proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

/* the current matrix times the view */
// FUNC 0x00204A58 LoggerCamera::applyView
void LoggerCamera::applyView()
{
    pgl_mult_matrix(matrix[0]);
}

// FUNC 0x00204A78 LoggerCamera::getFadeOutTime(void)
f32 LoggerCamera::getFadeOutTime()
{
    return 0.0f;
}

// FUNC 0x00204A88 LoggerCamera::isDrawBackMirror(void)
int LoggerCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00204A90 LoggerCamera::isEnableTargetChange(void)
int LoggerCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x00204A98 LoggerCamera::isEnableChanging(void)
int LoggerCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00204AA0 LoggerCamera::isNeedReset(RaceOrganization&, CameraTargetInfo&)
int LoggerCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return 0;
}

// FUNC 0x00204AA8 LoggerCamera::getChangeLevel(RaceOrganization&, int)
int LoggerCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return 0;
}

// FUNC 0x00204AB0 LoggerCamera::isTargetDistEnable(void)
int LoggerCamera::isTargetDistEnable()
{
    return 1;
}

// FUNC 0x00204AB8 LoggerCamera::getMount(void)
int LoggerCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00204AC0 __tf12LoggerCamera
extern "C" void *__tf12LoggerCamera(void)
{
    if (__ti12LoggerCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti12LoggerCamera, "12LoggerCamera",
                  (const type_info *)__ti18VariableCameraBase);
    }
    return __ti12LoggerCamera;
}
#endif
