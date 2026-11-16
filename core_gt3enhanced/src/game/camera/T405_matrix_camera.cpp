/* T405: MatrixCamera - a randomised sweep around the car: yaw, height, offset
 * and field of view are drawn by setup and interpolated
 * over `time' seconds by the update (update); the camera is lifted above
 * the road when the road under it is higher.
 *
 * Retail T405's .rodata opens with <new>'s "bad_alloc" (0x0033EB90, nothing
 * references it: DATA_START), then update's jump table, the vtable and the
 * type names 12MatrixCamera, 14RaceCameraBase, t9UnitArena1Ui64. */
// DATA_START 0x0033EB90
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/T405_matrix_camera.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00204EA8 MatrixCamera::MatrixCamera
//   (after VariableCameraBase::VariableCameraBase; rng: Random's inline
//   constructor, 0x87654321)
MatrixCamera::MatrixCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    setType(CAMERA_TYPE_MOUNT);             /* inline */
}

// FUNC 0x00204EF8 MatrixCamera::~MatrixCamera
//   Sets the vptr and calls VariableCameraBase::~VariableCameraBase with the
//   flags unchanged.
MatrixCamera::~MatrixCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00204F20 MatrixCamera::virtual_8
//   (a virtual call of slot 23)
void MatrixCamera::virtual_8(CameraTargetInfo *target)
{
    setup(target, 0.0f);
}

// FUNC 0x00204F50 MatrixCamera::setup
//   A new sweep of `time' seconds (at least 0.01): every value is a fresh
//   draw of the generator (seeded from the clock).
void MatrixCamera::setup(CameraTargetInfo *target, f32 time)
{
    f32 sweepTime = time;
    f32 fovDeg;

    rng.seed = (s32)gfx_perf_counter0();
    gamma[0] = 1.0f;
    zoom[0] = 1.0f / tanf(0.47996554f);    /* 27.5 degrees (overwritten below) */
    targetLength[0] = 100.0f;
    counter = 0;

    rng.next();
    radius = rng.next01() * 5.0f;
    rng.next();
    yaw0 = (rng.next01() - 0.5f) * 30.0f * 2.0f + 0.0f;
    if (3.0f < time)
        sweepTime = 3.0f;
    rng.next();
    yawSweep = rng.next01() * 220.0f * sweepTime + 100.0f;
    rng.next();
    yawDir = 1.0f;
    if (rng.next01() < 0.5f)
        yawDir = -1.0f;
    rng.next();
    x0 = rng.next01() - 0.5f;
    rng.next();
    x1 = rng.next01() * 3.0f - 1.5f;
    rng.next();
    z0 = rng.next01() - 0.5f;
    rng.next();
    z1 = rng.next01() * 3.0f - 1.5f;
    rng.next();
    height0 = rng.next01() + 0.1f;
    rng.next();
    heightSweep = rng.next01() * 2.0f;
    rng.next();
    fovDeg = rng.next01() * 30.0f + 55.0f - 15.0f;
    fov = fovDeg;
    zoom[0] = 1.0f / tanf(fovDeg * 0.5f * U091_TWO_PI / 360.0f);
    rng.next();
    bankForcibly = rng.next01() * 40.0f - 20.0f;

    if (time < 0.01f)
        time = 0.01f;
    duration = time;
    timeLeft = time;
    onRoad = 0;
    updated = 0;
    animMode = 0;
}

// FUNC 0x00205218 MatrixCamera::stepAnimMode
//   (? name; no callers)  Once the camera has been updated over the road:
//   animMode 0 -> 1, returns 1.
int MatrixCamera::stepAnimMode()
{
    if (animMode > 0)
        return 0;
    if (updated != 0 && onRoad != 0) {
        animMode = animMode + 1;
        return 1;
    }
    return 0;
}

// FUNC 0x00205258 matrix_ease
//   An ease over 0..90 degrees: 90 x (0.128572 t^3 - 0.557143 t^2 + 1.428572 t),
//   t = deg / 90 clamped to 0..1 (90 at 90; slope 10/7 at 0, 0.7 at 90).
//   Applied per quarter (update), the sweep passes the car's axis (0 /
//   180 degrees) quickly and slows at its sides (90 / 270).
static f32 matrix_ease(f32 deg)
{
    f32 t = deg / 90.0f;
    if (t < 0.0f)
        t = 0.0f;
    if (1.0f < t)
        t = 1.0f;
    return ((t * 0.12857151f + -0.557143f) * t + 1.4285715f) * t * 90.0f;
}

// FUNC 0x002052C0 MatrixCamera::update
//   The view: about the car's position and yaw, turned by the sweep's yaw,
//   moved by the offset (x, 0, z), then (0, height, distance) away, pitched
//   down at the car and banked.  When the road under the camera is higher
//   than the camera, the height is raised and the view built again.
int MatrixCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress)
{
    RaceCarState *car;
    f32 dur, left, k, extent, rearExtent, sideExtent, distance, height, yaw, pitch, offX, offZ;
    f32 roadY, bank;
    /* retail's frame: eye 0x00, plane 0x10, eyePos 0x20, sector 0x30 */
    f32 eye[4], plane[4], eyePos[3];
    int sector, lifted, noRoad, intrWasOn;

    if (animMode != 0)
        return 1;

    dur = duration;
    car = target->car[0]->state;
    timeLeft = timeLeft - U091_FRAME_TIME;
    left = timeLeft;
    if (dur < left)
        left = dur;
    if (left < 0.0f)
        left = 0.0f;
    k = 1.0 - (dur - left) / dur;           /* implicit: in double (fp-bit fptodp, dpsub, dptofp) */

    /* the car's extent from its centre: the largest of front, rear, width */
    extent = car->params.lenFront + car->params.originOffset;
    rearExtent = car->params.lenRear - car->params.originOffset;
    sideExtent = car->params.width;
    if (extent < rearExtent)
        extent = rearExtent;
    if (extent < sideExtent)
        extent = sideExtent;
    distance = extent + 0.2f + radius;
    if (distance + z0 - 0.2f < extent)
        z0 = 1.0f;

    height = height0 + heightSweep * k;
    yaw = angle_wrap360(yaw0 + yawSweep * k * yawDir);
    switch ((s32)yaw / 90) {                /* jump table 0x0033EBA0: eased towards each quarter */
    case 0: yaw = matrix_ease(yaw); break;
    case 4: yaw = matrix_ease(yaw - 360.0f); break;    /* (retail's block order: 0, 4, 1, 2, 3) */
    case 1: yaw = 180.0f - matrix_ease(180.0f - yaw); break;
    case 2: yaw = matrix_ease(yaw - 180.0f) + 180.0f; break;
    case 3: yaw = 360.0f - matrix_ease(360.0f - yaw); break;
    }

    lifted = 0;
    pitch = camera_pitch_deg(distance, -height, 0.0f);
    noRoad = 1;
    pitch = pitch - fov / 3.0f * 0.4f * cosf(pitch * U091_PI / 180.0f);
    offX = x0 + (x1 - x0) * k;
    offZ = z0 + (z1 - z0) * k;
    bank = bankForcibly;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_rotate_z(bank);
    vu0_rotate_x(pitch);
    pgl_translate(0.0f, -height, -distance);
    vu0_rotate_y(yaw);
    pgl_translate(-offX, 0.0f, -offZ);
    vu0_rotate_y(-car_yaw(car, 0.0f) * U091_RAD_TO_DEG);
    pgl_translate(-car_view_x(car, 0.0f), -car_view_y(car, 0.0f), -car_view_z(car, 0.0f));
    pgl_get_matrix(&matrix[0][0]);

    /* the eye: the inverse view applied to the origin */
    sector = -1;
    vu0_invert_rigid();
    eye[0] = 0.0f;
    eye[3] = 1.0f;
    eye[1] = 0.0f;
    eye[2] = 0.0f;
    vu0_apply_matrix(eye, eye);
    eyePos[0] = eye[0];
    eyePos[1] = eye[1];
    eyePos[2] = eye[2];
    org->course->getPlane(eyePos, &sector, plane);
    if (sector >= 0) {
        noRoad = 0;
        if (plane[1] != 0.0f) {
            /* the road's height under the eye, 0.2 m up */
            roadY = -(eyePos[0] * plane[0] + eyePos[2] * plane[2] + plane[3]) / plane[1] + 0.2f;
            eyePos[1] = roadY;
            if (eye[1] < roadY) {
                lifted = 1;
                height = height + roadY - eye[1];
            }
        }
    }
    if (lifted) {
        vu0_load_identity_regs();
        vu0_rotate_z(bank);
        vu0_rotate_x(pitch);
        pgl_translate(0.0f, -height, -distance);
        vu0_rotate_y(yaw);
        pgl_translate(-offX, 0.0f, -offZ);
        vu0_rotate_y(-car_yaw(car, 0.0f) * U091_RAD_TO_DEG);
        pgl_translate(-car_view_x(car, 0.0f), -car_view_y(car, 0.0f), -car_view_z(car, 0.0f));
        pgl_get_matrix(&matrix[0][0]);
    }
    pgl_pop_matrix();
    if (intrWasOn)
        __asm__ volatile("ei");

    /* sqrt.s as a volatile asm: retail stores updated after it */
    targetLength[0] = boot_sqrtf_v(distance * distance + height * height + 0.0f);
    onRoad = noRoad ^ 1;
    updated = 1;
    return 1;
}

// FUNC 0x00205850 MatrixCamera::virtual_9
int MatrixCamera::virtual_9()
{
    return 3;
}

// FUNC 0x00205858 MatrixCamera::getEffect
//   The effect bit 0x4000 (as a course record's effect[1]).
int MatrixCamera::getEffect()
{
    return 0x4000;
}

// FUNC 0x00205860 MatrixCamera::getChangeLevel
int MatrixCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return CHANGE_LEVEL_NONE;
}

// FUNC 0x00205868 MatrixCamera::isNeedReset
int MatrixCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return target.changed;
}

// FUNC 0x00205870 MatrixCamera::setProjection
//   The projection, its focal scaled by zoom[0].
void MatrixCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip, proj->farClip,
                proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

// FUNC 0x002058C8 MatrixCamera::applyView
void MatrixCamera::applyView()
{
    pgl_mult_matrix(&matrix[0][0]);
}

// FUNC 0x002058E8 MatrixCamera::getAnimMode
s8 MatrixCamera::getAnimMode()
{
    return animMode;
}

// FUNC 0x002058F0 MatrixCamera::setCurrentTime
void MatrixCamera::setCurrentTime(f32 t)
{
    timeLeft = t;
}

// FUNC 0x002058F8 MatrixCamera::getFadeOutTime
f32 MatrixCamera::getFadeOutTime()
{
    return 1.0f;
}

// FUNC 0x00205908 MatrixCamera::isDrawBackMirror
int MatrixCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00205910 MatrixCamera::isEnableTargetChange
int MatrixCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x00205918 MatrixCamera::isEnableChanging
int MatrixCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00205920 MatrixCamera::isTargetDistEnable
int MatrixCamera::isTargetDistEnable()
{
    return 1;
}

// FUNC 0x00205928 MatrixCamera::getMount
int MatrixCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __ti12MatrixCamera (0x004B2940, name 0x0033EC80 "12MatrixCamera", base
 * __ti18VariableCameraBase 0x004B28B0) on the first call. */
// FUNC 0x00205930 __tf12MatrixCamera
extern "C" void *__tf12MatrixCamera(void)
{
    if (__ti12MatrixCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti12MatrixCamera, "12MatrixCamera", (const type_info *)__ti18VariableCameraBase);
    }
    return __ti12MatrixCamera;
}
#endif
