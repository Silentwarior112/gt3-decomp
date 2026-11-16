/* T402 - AutomountCamera and CameraWobble (0x00204030..0x002047D0).
 *
 * A random mount on the car: set up (setup) with a random offset
 * around it (1..9 out, 3..7 up), a random field of view (55..85 degrees)
 * and bank (-20..20); the update aims it at the car's centre from there,
 * with four wobbling values (CameraWobble) shaking the angles and the
 * place, and follows the car's heading.  getChangeLevel asks for a change
 * while the target is in a zone of mask 1 of the course's replay cameras.
 * HeliCamera (T404) derives from it.
 *
 * Retail T402's .rodata opens with <new>'s "bad_alloc" (0x0033E840, nothing
 * references it: DATA_START) and ends with the type names 15AutomountCamera,
 * 14RaceCameraBase, t9UnitArena1Ui64: the file saw RaceCameraBase (T383),
 * defined after UnitArena<64> (T153) and before the file's class.
 */
// DATA_START 0x0033E840
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/cameras_internal.h"
#include "game/camera/T402_automount_camera.h"

// FUNC 0x00204030 CameraWobble::reset
void CameraWobble::reset()
{
    target = 0.0f;
    value = 0.0f;
    speed = 0.0f;
}

/* one frame in ten, a new target in -128..128 */
// FUNC 0x00204040 CameraWobble::retarget
void CameraWobble::retarget(Random *rng)
{
    rng->next();
    if (rng->next01() < 0.1f) {
        rng->next();
        target = rng->next01() * 256.0f - 128.0f;
    }
}

// FUNC 0x002040C0 CameraWobble::get
f32 CameraWobble::get()
{
    return value * 0.0078125f;              /* / 128 */
}

/* A spring of stiffness k with damping 10, at 60 steps a second. */
// FUNC 0x002040D8 CameraWobble::update
void CameraWobble::update(Random *rng, f32 k)
{
    f32 v, s;

    retarget(rng);
    v = value;
    s = speed;
    s = s + (k * (target - v) + s * -10.0f) / 60.0f;
    speed = s;
    value = v + s / 60.0f;
}

// FUNC 0x00204160 AutomountCamera::AutomountCamera
AutomountCamera::AutomountCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* VariableCameraBase::VariableCameraBase(), rng's inline Random() (seed
     * 0x87654321), the vtable, then the inline setType(5). */
    setType(5);
}

// FUNC 0x002041B0 AutomountCamera::~AutomountCamera
AutomountCamera::~AutomountCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* the vtable, then VariableCameraBase::~VariableCameraBase(this, flags) */
}

/* set up, through the vtable (slot 23) */
// FUNC 0x002041D8 AutomountCamera::virtual_8
void AutomountCamera::virtual_8(CameraTargetInfo *target)
{
    setup(target, 0.0f);
}

/* Set up: target and t are not used.  The generator is seeded from the
 * clock.  Caller (non-virtual): HeliCamera::setupOnRecord. */
// FUNC 0x00204208 AutomountCamera::setup
void AutomountCamera::setup(CameraTargetInfo *target, f32 t)
{
    Random *r = &rng;
    int seed = (int)gfx_perf_counter0();    /* the low word, sign-extended */
    f32 z, dist, angle, deg;

    gamma[0] = 1.0f;
    inZone = 0;
    rng.seed = seed;
    z = 1.0f / tanf(0.61086530f);           /* half of 70 degrees */
    counter = 0;
    targetLength[0] = 100.0f;
    zoom[0] = z;
    wobble[0].reset();
    wobble[1].reset();
    wobble[2].reset();
    wobble[3].reset();
    r->next();
    dist = r->next01() * 8.0f + 1.0f;
    r->next();
    angle = r->next01() * 6.2831855f;
    offX = dist * cosf(angle);
    offZ = dist * sinf(angle);
    r->next();
    offY = r->next01() * 4.0f + 3.0f;
    r->next();
    deg = r->next01() * 30.0f + 70.0f - 15.0f;
    fov = deg;
    zoom[0] = 1.0f / tanf(deg * 0.5f * 6.2831855f / 360.0f);
    r->next();
    bankForcibly = r->next01() * 40.0f - 20.0f;
}

/* Always returns 1.  Caller (non-virtual): HeliCamera::update. */
// FUNC 0x002043E0 AutomountCamera::update
int AutomountCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                                CarProgress *progress)
{
    f32 lapPos = target->lapDistance;
    RaceCarState *car;
    f32 eyeX, eyeY, eyeZ, swing, rotX, pitch, yaw, dist;
    int intrWasOn;

    inZone = 0;
    if (org->course->data->replay->zoneInRange(lapPos, lapPos + 1.0f, 1))
        inZone = 1;
    car = target->car[0]->state;
    wobble[0].update(&rng, 18.0f);
    wobble[1].update(&rng, 18.0f);
    wobble[2].update(&rng, 5.0f);
    wobble[3].update(&rng, 5.0f);

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    vu0_load_identity_regs();
    eyeX = offX + 2.0f * wobble[2].get() - 1.0f;
    eyeY = offY;
    eyeZ = offZ + 2.0f * wobble[3].get() - 1.0f;
    swing = fov / 7.0f;
    rotX = wobble[0].get() * swing - swing * 0.5f;
    vu0_rotate_y(wobble[1].get() * swing - swing * 0.5f);
    vu0_rotate_x(rotX);
    /* aimed at the car's centre from the mount */
    pitch = camera_pitch_deg(-offX, -offY, -offZ);
    yaw = camera_yaw_deg(-offX, -offY, -offZ);
    vu0_rotate_z(bankForcibly);
    vu0_rotate_x(pitch);
    vu0_rotate_y(yaw);
    pgl_translate(-eyeX, -eyeY, -eyeZ);
    dist = boot_sqrtf_v(eyeX * eyeX + eyeY * eyeY + eyeZ * eyeZ);
    /* the mount turns with the car's heading */
    vu0_rotate_y(-car_yaw(car, 0.0f) * 57.29578f);
    {
        f32 negX = -car_view_x(car, 0.0f);
        f32 negY = -car_view_y(car, 0.0f);

        pgl_translate(negX, negY, -car_view_z(car, 0.0f));
    }
    targetLength[0] = dist;
    pgl_get_matrix(matrix[0]);
    pgl_pop_matrix();
    intr_resume(intrWasOn);
    return 1;
}

// FUNC 0x002046A8 AutomountCamera::virtual_9
int AutomountCamera::virtual_9()
{
    return 3;
}

// FUNC 0x002046B0 AutomountCamera::getEffect
int AutomountCamera::getEffect()
{
    return effect;
}

// FUNC 0x002046B8 AutomountCamera::getChangeLevel
int AutomountCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return inZone == 0 ? 0 : 2;
}

// FUNC 0x002046C8 AutomountCamera::isNeedReset
int AutomountCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return target.changed;
}

/* the projection, the focal length scaled by the zoom (the inline getZoom:
 * retail loads the zoom first, into its own register) */
// FUNC 0x002046D0 AutomountCamera::setProjection
void AutomountCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip,
                proj->farClip, proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

/* the current matrix times the view */
// FUNC 0x00204728 AutomountCamera::applyView
void AutomountCamera::applyView()
{
    pgl_mult_matrix(matrix[0]);
}

// FUNC 0x00204748 AutomountCamera::getFadeOutTime(void)
f32 AutomountCamera::getFadeOutTime()
{
    return 0.0f;
}

// FUNC 0x00204758 AutomountCamera::isDrawBackMirror(void)
int AutomountCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00204760 AutomountCamera::isEnableTargetChange(void)
int AutomountCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x00204768 AutomountCamera::isEnableChanging(void)
int AutomountCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00204770 AutomountCamera::isTargetDistEnable(void)
int AutomountCamera::isTargetDistEnable()
{
    return 0;
}

// FUNC 0x00204778 AutomountCamera::getMount(void)
int AutomountCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00204780 __tf15AutomountCamera
extern "C" void *__tf15AutomountCamera(void)
{
    if (__ti15AutomountCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti15AutomountCamera, "15AutomountCamera",
                  (const type_info *)__ti18VariableCameraBase);
    }
    return __ti15AutomountCamera;
}
#endif
