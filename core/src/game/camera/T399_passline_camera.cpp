/* T399 - PasslineCamera (0x00202AC0..0x00202F60).
 *
 * A course camera on the first record of "cameras/start.cam" (g_00351F94,
 * T153): the record's camera and look-at tables at timeRange (0..1, set from
 * outside; StartingCamera drives it), no damping.
 *
 * Retail T399's .rodata opens with <new>'s "bad_alloc" (0x0033E510, nothing
 * references it: DATA_START) and ends with the type names 14PasslineCamera,
 * t9UnitArena1Ui64: UnitArena<64> (T153) defined before the file's class.
 */
// DATA_START 0x0033E510
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after the file's own */
#include "game/camera/cameras_internal.h"
#include "game/camera/T399_passline_camera.h"

// FUNC 0x00202AC0 PasslineCamera::PasslineCamera
PasslineCamera::PasslineCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* VariableCameraBase::VariableCameraBase(), the vtable, then the inline
     * setType(1). */
    setType(1);
}

// FUNC 0x00202B00 PasslineCamera::~PasslineCamera
PasslineCamera::~PasslineCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* the vtable, then VariableCameraBase::~VariableCameraBase(this, flags) */
}

/* set up without a course (no record), through the vtable (slot 23) */
// FUNC 0x00202B28 PasslineCamera::virtual_8
void PasslineCamera::virtual_8(CameraTargetInfo *target)
{
    setup(target, NULL);
}

/* Set up: with a course, the first record of start.cam. */
// FUNC 0x00202B58 PasslineCamera::setup
void PasslineCamera::setup(CameraTargetInfo *target, RaceCourse *course)
{
    this->course = course;
    cameraData = NULL;
    if (course != NULL)
        cameraData = g_00351F94->records;
    gamma[0] = 1.0f;
    zoom[0] = 1.4281480f;                   /* 1 / tan(35 degrees) */
    counter = 0;
    timeRange = 0.0f;
}

/* Returns 0 on the first call after set up, else 1.  Note the translation
 * by (x, -y, z) of the camera position with the heading turned by 180
 * degrees (StartingCamera's view on top of it). */
// FUNC 0x00202B90 PasslineCamera::update
int PasslineCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                               CarProgress *progress)
{
    int notFirst = 1;
    f32 param;
    CourseCameraRecord *rec;
    f32 camX, camY, camZ, dirX, dirY, dirZ, pitch, yaw, dist, limit;
    f32 zm = 1.0f;                          /* (overwritten on both paths) */
    f32 roll;
    int intrWasOn;
    u16 effectBits;

    if (counter == 0) {
        notFirst = 0;
        counter = 1;
    }
    param = timeRange;
    rec = cameraData;
    roll = 0.0f;                            /* no roll */
    if (param < 0.0f)
        param = 0.0f;
    if (1.0f < param)
        param = 1.0f;
    camX = interpolate_table(rec->pos[0], param);
    camY = interpolate_table(rec->pos[1], param);
    camZ = interpolate_table(rec->pos[2], param);
    dirX = interpolate_table(rec->target[0], param);
    dirY = interpolate_table(rec->target[1], param);
    dirZ = interpolate_table(rec->target[2], param);
    dirX -= camX;
    dirY -= camY;
    dirZ -= camZ;
    pitch = camera_pitch_deg(dirX, dirY, dirZ);
    yaw = camera_yaw_deg(dirX, dirY, dirZ);
    dist = boot_sqrtf_v(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (rec->flags & CAMREC_ZOOM_BY_DIST)
        zm = dist * interpolate_table(rec->zoom, 0.0f);
    else
        zm = interpolate_table(rec->zoom, param);
    limit = rec->zoomMax;
    if (limit < zm)
        zm = limit;
    limit = rec->zoomMin;
    if (zm < limit)
        zm = limit;
    targetLength[0] = dist;
    zoom[0] = zm;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_rotate_z(roll);
    vu0_rotate_x(pitch);
    vu0_rotate_y(yaw + 180.0f);
    pgl_translate(camX, -camY, camZ);
    pgl_get_matrix(matrix[0]);
    pgl_pop_matrix();
    intr_resume(intrWasOn);

    effectBits = 0;
    if (0.0f < rec->effect[0])
        effectBits = CAMERA_EFFECT_0;
    if (0.0f < rec->effect[1])
        effectBits |= CAMERA_EFFECT_1;
    if (0.0f < rec->effect[2])
        effectBits |= CAMERA_EFFECT_2;
    effect = (s16)effectBits;
    return notFirst;
}

// FUNC 0x00202E18 PasslineCamera::virtual_9
int PasslineCamera::virtual_9()
{
    return 3;
}

// FUNC 0x00202E20 PasslineCamera::getEffect
int PasslineCamera::getEffect()
{
    return effect;
}

/* the projection, the focal length scaled by the zoom (the inline getZoom:
 * retail loads the zoom first, into its own register) */
// FUNC 0x00202E28 PasslineCamera::setProjection
void PasslineCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip,
                proj->farClip, proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

/* the current matrix times the view */
// FUNC 0x00202E80 PasslineCamera::applyView
void PasslineCamera::applyView()
{
    pgl_mult_matrix(matrix[0]);
}

// FUNC 0x00202EA0 PasslineCamera::getTimeRange(void)
f32 PasslineCamera::getTimeRange()
{
    return timeRange;
}

// FUNC 0x00202EA8 PasslineCamera::setTimeRange(float)
void PasslineCamera::setTimeRange(f32 t)
{
    timeRange = t;
}

/* no callers */
// FUNC 0x00202EB0 PasslineCamera::F_00202EB0
s16 PasslineCamera::F_00202EB0()
{
    return unk_104;
}

/* no callers */
// FUNC 0x00202EB8 PasslineCamera::getCourse
RaceCourse *PasslineCamera::getCourse()
{
    return course;
}

// FUNC 0x00202EC0 PasslineCamera::getCameraData(void)
CourseCameraRecord *PasslineCamera::getCameraData()
{
    return cameraData;
}

// FUNC 0x00202EC8 PasslineCamera::getFadeOutTime(void)
f32 PasslineCamera::getFadeOutTime()
{
    return 0.0f;
}

// FUNC 0x00202ED8 PasslineCamera::isDrawBackMirror(void)
int PasslineCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00202EE0 PasslineCamera::isEnableTargetChange(void)
int PasslineCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x00202EE8 PasslineCamera::isEnableChanging(void)
int PasslineCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00202EF0 PasslineCamera::isNeedReset(RaceOrganization&, CameraTargetInfo&)
int PasslineCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return 0;
}

// FUNC 0x00202EF8 PasslineCamera::getChangeLevel(RaceOrganization&, int)
int PasslineCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return 0;
}

// FUNC 0x00202F00 PasslineCamera::isTargetDistEnable(void)
int PasslineCamera::isTargetDistEnable()
{
    return 0;
}

// FUNC 0x00202F08 PasslineCamera::getMount(void)
int PasslineCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00202F10 __tf14PasslineCamera
extern "C" void *__tf14PasslineCamera(void)
{
    if (__ti14PasslineCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti14PasslineCamera, "14PasslineCamera",
                  (const type_info *)__ti18VariableCameraBase);
    }
    return __ti14PasslineCamera;
}
#endif
