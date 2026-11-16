/* T406: ZoomupCamera - a close-up from a random point near the car (drawn at
 * setup), shaken by two CameraShakes (T378) and two
 * CameraSprings (T380) that grow with the car's speed; the eye is raised when
 * the road under it is higher.
 *
 * Retail T406's .rodata opens with <new>'s "bad_alloc" (0x0033ECC0, nothing
 * references it: DATA_START), then the vtable and the type names
 * 12ZoomupCamera, 14RaceCameraBase, t9UnitArena1Ui64. */
// DATA_START 0x0033ECC0
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/T406_zoomup_camera.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00205980 ZoomupCamera::ZoomupCamera
//   (after VariableCameraBase::VariableCameraBase; rng: Random's inline
//   constructor, 0x87654321)
ZoomupCamera::ZoomupCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    setType(CAMERA_TYPE_MOUNT);             /* inline */
}

// FUNC 0x002059D0 ZoomupCamera::~ZoomupCamera
//   Sets the vptr and calls VariableCameraBase::~VariableCameraBase with the
//   flags unchanged.
ZoomupCamera::~ZoomupCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x002059F8 ZoomupCamera::virtual_8
void ZoomupCamera::virtual_8(CameraTargetInfo *target)
{
}

// FUNC 0x00205A00 ZoomupCamera::setup
//   A new close-up (time is not used): every value is a fresh draw of the
//   generator (seeded from the clock).
void ZoomupCamera::setup(CameraTargetInfo *target, f32 time)
{
    RaceCarState *car;
    f32 front, rear, along, length, fovDeg;

    rng.seed = (s32)gfx_perf_counter0();
    car = target->car[0]->state;
    front = car->params.lenFront + car->params.originOffset;
    rear = car->params.lenRear - car->params.originOffset;
    gamma[0] = 1.0f;
    zoom[0] = 1.0f / tanf(0.21816616f);    /* 12.5 degrees (overwritten below) */
    counter = 0;
    along = front;

    rng.next();
    bankForcibly = rng.next01() * 40.0f - 20.0f;
    rng.next();
    behind = rng.next01() < 0.5f ? 1 : 0;
    if (behind)
        along = rear;
    rng.next();
    offZ = along + 2.5f + rng.next01() * 4.0f;     /* 2.5..6.5 m off the car's end */
    rng.next();
    offY = -(rng.next01() * 1.2f + 0.1f);
    rng.next();
    offX = rng.next01() * 3.0f - 1.5f;
    rng.next();
    if (rng.next01() < 0.3f) {
        offX = 0.0f;
        bankForcibly = 0.0f;
    }
    rng.next();
    aimY = -(rng.next01() * 0.5f + 0.2f);
    length = offZ;
    rng.next();
    if (rng.next01() < 0.5f)
        length = length - along;
    targetLength[0] = length;
    rng.next();
    fovDeg = rng.next01() * 15.0f + 25.0f;
    fov = fovDeg;
    zoom[0] = 1.0f / tanf(fovDeg * 0.5f * U091_TWO_PI / 360.0f);
    pitchWobble.reset();
    yawWobble.reset();
    yawShake.reset(0.0f, 0.0f);
    pitchShake.reset(0.0f, 0.0f);
}

// FUNC 0x00205CA8 ZoomupCamera::update
//   The shake from the target's speed, then the view; once, the eye is
//   checked against the road: when the road is higher, the view's y offset
//   (offY, negative: the eye above the car) is lowered by the difference,
//   which raises the eye, and the view is built again.
int ZoomupCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress)
{
    RaceCarState *car;
    /* retail's frame: eye 0x00, plane 0x10, eyePos 0x20, then the
     * addressable scalars vx .. nz 0x30..0x44 and sector 0x48 */
    f32 eye[4], plane[4], eyePos[3];
    f32 vx, vy, vz;
    f32 nx, ny, nz;
    int sector;
    f32 speedSq, excess, gain, x, y, z, pitch, yaw, roadY, carX, carY, carZ;
    int checked, raised, intrWasOn;

    speedSq = 0.0f;
    car = target->car[0]->state;
    if (target->getSavedPosition(&vx, &vy, &vz) != 0) {
        target->getCarPosition(0, &nx, &ny, &nz);
        vx = (nx - vx) * U091_FPS;
        vy = (ny - vy) * U091_FPS;
        vz = (nz - vz) * U091_FPS;
        speedSq = vx * vx + vy * vy + vz * vz;
    }
    /* the squared speed above (1.389 m/s)^2 (5 km/h), up to (11.11 m/s)^2 more
     * (40 km/h) */
    excess = speedSq - 1.9290122f;
    if (excess < 0.0f)
        excess = 0.0f;
    if (123.45678f < excess)
        excess = 123.45678f;
    gain = excess * 57.5f / 123.45678f;
    pitchWobble.update(&rng, gain);
    yawWobble.update(&rng, gain);
    rng.next();
    if (rng.next01() < 0.5f) {
        rng.next();
        yawShake.update((rng.next01() - 0.5f) * 750.0f);
        rng.next();
        pitchShake.update((rng.next01() - 0.5f) * 750.0f);
    } else {
        yawShake.update(0.0f);
        pitchShake.update(0.0f);
    }
    checked = 0;
    shake = excess / 123.45678f;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    x = offX;
    y = offY;
    z = offZ;
    if (behind)
        z = -z;
    /* A label loop (no loop notes: retail's register priorities, `checked'
     * in a stack slot, intrWasOn in $fp), run twice at most. */
again:
    pitch = camera_pitch_deg(x, y - aimY, z);
    yaw = camera_yaw_deg(x, y - aimY, z);
    vu0_load_identity_regs();
    vu0_rotate_x(pitchWobble.getValue());
    vu0_rotate_x(pitchShake.clampedPos() * shake);
    vu0_rotate_y(yawWobble.getValue() * 0.8f);
    vu0_rotate_y(yawShake.clampedPos() * 0.25f * shake);
    vu0_rotate_z(bankForcibly);
    vu0_rotate_x(pitch);
    vu0_rotate_y(yaw);
    pgl_translate(x, y, z);
    vu0_rotate_y(-car_yaw(car, 0.0f) * U091_RAD_TO_DEG);
    carX = -car_view_x(car, 0.0f);
    carY = -car_view_y(car, 0.0f);
    carZ = -car_view_z(car, 0.0f);
    pgl_translate(carX, carY, carZ);
    pgl_get_matrix(&matrix[0][0]);
    if (checked)
        goto done;

    /* the eye: the inverse view applied to the origin */
    raised = 0;
    sector = -1;
    checked = 1;
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
    if (sector >= 0 && plane[1] != 0.0f) {
        /* the road's height under the eye, 0.2 m up */
        roadY = -(eyePos[0] * plane[0] + eyePos[2] * plane[2] + plane[3]) / plane[1] + 0.2f;
        eyePos[1] = roadY;
        if (eye[1] < roadY) {
            raised = 1;
            y = y - (roadY - eye[1]);
        }
    }
    if (raised)
        goto again;
done:
    pgl_pop_matrix();
    if (intrWasOn)
        __asm__ volatile("ei");
    return 1;
}

// FUNC 0x00206140 ZoomupCamera::virtual_9
int ZoomupCamera::virtual_9()
{
    return 3;
}

// FUNC 0x00206148 ZoomupCamera::getEffect
//   The effect bit 0x4000 (as a course record's effect[1]).
int ZoomupCamera::getEffect()
{
    return 0x4000;
}

// FUNC 0x00206150 ZoomupCamera::getChangeLevel
int ZoomupCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return CHANGE_LEVEL_NONE;
}

// FUNC 0x00206158 ZoomupCamera::isNeedReset
int ZoomupCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return target.changed;
}

// FUNC 0x00206160 ZoomupCamera::setProjection
//   The projection, its focal scaled by zoom[0].
void ZoomupCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip, proj->farClip,
                proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

// FUNC 0x002061B8 ZoomupCamera::applyView
void ZoomupCamera::applyView()
{
    pgl_mult_matrix(&matrix[0][0]);
}

// FUNC 0x002061D8 ZoomupCamera::getFadeOutTime
f32 ZoomupCamera::getFadeOutTime()
{
    return 1.0f;
}

// FUNC 0x002061E8 ZoomupCamera::isDrawBackMirror
int ZoomupCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x002061F0 ZoomupCamera::isEnableTargetChange
int ZoomupCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x002061F8 ZoomupCamera::isEnableChanging
int ZoomupCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00206200 ZoomupCamera::isTargetDistEnable
int ZoomupCamera::isTargetDistEnable()
{
    return 1;
}

// FUNC 0x00206208 ZoomupCamera::getMount
int ZoomupCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __ti12ZoomupCamera (0x004B2950, name 0x0033ED98 "12ZoomupCamera", base
 * __ti18VariableCameraBase 0x004B28B0) on the first call. */
// FUNC 0x00206210 __tf12ZoomupCamera
extern "C" void *__tf12ZoomupCamera(void)
{
    if (__ti12ZoomupCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti12ZoomupCamera, "12ZoomupCamera", (const type_info *)__ti18VariableCameraBase);
    }
    return __ti12ZoomupCamera;
}
#endif
