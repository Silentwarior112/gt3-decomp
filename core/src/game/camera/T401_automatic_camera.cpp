/* T401 - AutomaticCamera (0x002031D0..0x00204030).
 *
 * A random still camera, placed once by setup and not moved
 * by the update: either some way ahead of the car on its course of the
 * moment (placeAhead) or beside it, ahead of where it will be (placeBeside),
 * dropped onto the road plane with a random height; if the road plane is
 * not found, a camera at the car looking along its motion.  The field of
 * view is a random 55..85 degrees.  The generator is seeded from the clock.
 *
 * The car's motion of the frame is car_view_*(car, 0) - car_view_*_now(car);
 * x 60 it is the motion of a second.
 *
 * Retail T401's .rodata opens with <new>'s "bad_alloc" (0x0033E720, nothing
 * references it: DATA_START) and ends with the type names 15AutomaticCamera,
 * 14RaceCameraBase, t9UnitArena1Ui64: the file saw RaceCameraBase (T383),
 * defined after UnitArena<64> (T153) and before the file's class.
 */
// DATA_START 0x0033E720
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/cameras_internal.h"
#include "game/camera/T401_automatic_camera.h"

/* The heading of a direction, degrees: 180 - atan2(x, z) (y unused); the
 * same code as T382's camera_yaw_deg, a copy of its own here (? a static
 * inline of the source). */
// FUNC 0x002031D0 automatic_heading
static f32 automatic_heading(f32 x, f32 y, f32 z)
{
    return (-vu0_atan2f(x, z) + 3.1415927f) * 57.29578f;
}

// FUNC 0x00203200 AutomaticCamera::AutomaticCamera
AutomaticCamera::AutomaticCamera()
{
    /* implicit: VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* VariableCameraBase::VariableCameraBase(), rng's inline Random() (seed
     * 0x87654321), the vtable, then the inline setType(4). */
    setType(4);
}

// FUNC 0x00203250 AutomaticCamera::~AutomaticCamera
AutomaticCamera::~AutomaticCamera()
{
    /* implicit: ~VariableCameraBase (compiler-generated: bases, members, deleting form) */
    /* the vtable, then VariableCameraBase::~VariableCameraBase(this, flags) */
}

/* set up without a course (an identity view), through the vtable (slot 23) */
// FUNC 0x00203278 AutomaticCamera::virtual_8
void AutomaticCamera::virtual_8(CameraTargetInfo *target)
{
    setup(target, NULL, 0.0f);
}

/* Set up.  ahead scales how far ahead of the car placeAhead goes. */
// FUNC 0x002032A8 AutomaticCamera::setup
void AutomaticCamera::setup(CameraTargetInfo *target, RaceCourse *course, f32 ahead)
{
    Random *r = &rng;
    int seed = (int)gfx_perf_counter0();    /* the low word, sign-extended */
    f32 z;
    int intrWasOn;

    gamma[0] = 1.0f;
    placeCourse = course;
    rng.seed = seed;
    z = 1.0f / tanf(0.61086530f);           /* half of 70 degrees */
    counter = 0;
    targetLength[0] = 100.0f;
    zoom[0] = z;
    if (course == NULL) {
        intrWasOn = intr_suspend();
        pgl_push_matrix();
        vu0_load_identity_regs();
        pgl_get_matrix(matrix[0]);
        pgl_pop_matrix();
        intr_resume(intrWasOn);
        return;
    }
    {
        RaceCarState *car = target->car[0]->state;
        int placed;

        r->next();
        if (0.5f < r->next01())
            placed = placeAhead(car, course, ahead);
        else
            placed = placeBeside(car, course, ahead);
        if ((placed ^ 1) != 0) {
            /* no road found: at the car, 1 up, looking along its motion */
            f32 eye[3];                     /* sp+0x00 */
            f32 rot[3];                     /* sp+0x10 x, y, z (degrees) */
            f32 carX = car_view_x(car, 0.0f);
            f32 carY = car_view_y(car, 0.0f);
            f32 carZ = car_view_z(car, 0.0f);
            f32 moveX = carX - car_view_x_now(car);
            f32 moveY = carY - car_view_y_now(car);
            f32 moveZ = carZ - car_view_z_now(car);

            eye[0] = carX;
            eye[2] = carZ;
            eye[1] = carY + 1.0f;
            r->next();
            rot[0] = r->next01() * 20.0f - 10.0f;
            rot[1] = automatic_heading(moveX, moveY, moveZ);
            rot[2] = 0.0f;
            intrWasOn = intr_suspend();
            pgl_push_matrix();
            vu0_load_identity_regs();
            vu0_rotate_z(rot[2]);
            vu0_rotate_x(rot[0]);
            vu0_rotate_y(rot[1]);
            pgl_translate(-eye[0], -eye[1], -eye[2]);
            pgl_get_matrix(matrix[0]);
            pgl_pop_matrix();
            intr_resume(intrWasOn);
        }
    }
}

/* Ahead of the car along its motion (ahead x 0.5..0.85 seconds, or
 * 0.05..0.2 when looking back at it), 100 up and dropped onto the road
 * plane under it (0.1..1.1 above it).  Returns 1 when the road plane was
 * found. */
// FUNC 0x00203528 AutomaticCamera::placeAhead
int AutomaticCamera::placeAhead(RaceCarState *car, RaceCourse *course, f32 ahead)
{
    int notFound = 1;
    int lookBack = 0;
    f32 eye[3];                             /* sp+0x00 */
    f32 rot[3];                             /* sp+0x10 */
    f32 plane[4];                           /* sp+0x20 a x + b y + c z + d = 0 */
    int sector = -1;                        /* sp+0x30 the road sector hint */
    f32 carX, carY, carZ, dx, dy, dz, time, heading;
    int intrWasOn;

    carX = car_view_x(car, 0.0f);
    carY = car_view_y(car, 0.0f);
    carZ = car_view_z(car, 0.0f);
    dx = carX - car_view_x_now(car);
    dy = carY - car_view_y_now(car);
    dz = carZ - car_view_z_now(car);
    rng.next();
    if (0.5f < rng.next01())
        lookBack = 1;
    rng.next();
    /* a random field of view, 70 +- 15 degrees */
    zoom[0] = 1.0f / tanf((rng.next01() * 30.0f + 70.0f - 15.0f) * 0.5f * 6.2831855f / 360.0f);
    rng.next();
    if (lookBack)
        time = ahead * (rng.next01() * 0.15f + 0.05f);
    else
        time = ahead * (rng.next01() * 0.35f + 0.5f);
    rng.next();
    {
        f32 rnd = rng.next01();

        eye[0] = carX + dx * 60.0f * time + (rnd + rnd - 1.0f);
        eye[1] = carY + dy * 60.0f * time + 100.0f;
    }
    rng.next();
    {
        f32 rnd = rng.next01();

        eye[2] = carZ + dz * 60.0f * time + (rnd + rnd - 1.0f);
    }
    course->getPlane(eye, &sector, plane);
    if (sector >= 0 && plane[1] != 0.0f) {
        notFound = 0;
        eye[1] = -(eye[0] * plane[0] + eye[2] * plane[2] + plane[3]) / plane[1] + 0.1f;
        rng.next();
        eye[1] = eye[1] + rng.next01();
    }
    rng.next();
    rot[0] = rng.next01() * 20.0f + -5.0f - 10.0f;
    rng.next();
    dx = carX - eye[0];
    dy = carY - eye[1];
    dz = carZ - eye[2];
    if (lookBack) {
        dx = -dx;
        dy = -dy;
        dz = -dz;
    }
    heading = automatic_heading(dx, dy, dz);
    rot[1] = heading + rng.next01() * 20.0f - 10.0f;    /* the generator stepped before the heading */
    rng.next();
    rot[2] = rng.next01() * 30.0f - 15.0f;

    intrWasOn = intr_suspend();
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_rotate_z(rot[2]);
    vu0_rotate_x(rot[0]);
    vu0_rotate_y(rot[1]);
    pgl_translate(-eye[0], -eye[1], -eye[2]);
    pgl_get_matrix(matrix[0]);
    pgl_pop_matrix();
    intr_resume(intrWasOn);
    return notFound ^ 1;
}

/* Beside the car: a random offset (sideways 0.7..2.2 widths, along up to
 * its length, either side) in the car's frame, 100 up and dropped onto the
 * road plane (0.1..0.7 above it), turned towards a point 1..3 seconds
 * ahead of the car plus half the field of view (to the side the car comes
 * from).  ahead is not used.  Returns 1 when the road plane was found. */
// FUNC 0x00203958 AutomaticCamera::placeBeside
int AutomaticCamera::placeBeside(RaceCarState *car, RaceCourse *course, f32 ahead)
{
    int notFound = 1;
    f32 eye[3];                             /* sp+0x00 the offset, then the place */
    f32 rot[3];                             /* sp+0x10 */
    f32 plane[4];                           /* sp+0x20 a x + b y + c z + d = 0 */
    f32 aim[3];                             /* sp+0x30 (stored, not read again) */
    f32 toCar[3];                           /* sp+0x40 */
    f32 toAim[3];                           /* sp+0x50 */
    f32 world[4];                           /* sp+0x60 */
    int sector = -1;                        /* sp+0x70 the road sector hint */
    f32 carX, carY, carZ, moveX, moveY, moveZ, length, width, side, halfFov;

    carX = car_view_x(car, 0.0f);
    side = 1.0f;
    carY = car_view_y(car, 0.0f);
    carZ = car_view_z(car, 0.0f);
    moveX = carX - car_view_x_now(car);
    moveY = carY - car_view_y_now(car);
    moveZ = carZ - car_view_z_now(car);
    length = car->params.lenFront + car->params.originOffset +
             (car->params.lenRear - car->params.originOffset);
    width = car->params.width;
    rng.next();
    {
        f32 randomWidth = width * rng.next01();

        eye[1] = 0.0f;
        eye[0] = width * 0.7f + randomWidth * 1.5f;
    }
    rng.next();
    eye[2] = length * rng.next01();
    rng.next();
    if (rng.next01() < 0.5f)
        eye[0] = -eye[0];
    rng.next();
    if (rng.next01() < 0.5f)
        eye[2] = -eye[2];

    /* the offset into the world: the inverse of the car's view matrix */
    {
        int intrWasOn = intr_suspend();
        pgl_push_matrix();
        vu0_load_identity_regs();
        pgl_translate(-eye[0], -eye[1], -eye[2]);
        vu0_rotate_x(-car_pitch(car, 0.0f) * 57.29578f);
        vu0_rotate_y(-car_yaw(car, 0.0f) * 57.29578f);
        {
            f32 negX = -car_view_x(car, 0.0f);
            f32 negY = -car_view_y(car, 0.0f);

            pgl_translate(negX, negY, -car_view_z(car, 0.0f));
        }
        vu0_invert_rigid();
        world[0] = 0.0f;
        world[1] = 0.0f;
        world[2] = 0.0f;
        world[3] = 1.0f;
        vu0_apply_matrix(world, world);
        pgl_pop_matrix();
        intr_resume(intrWasOn);
    }

    eye[0] = world[0];
    eye[2] = world[2];
    eye[1] = world[1] + 100.0f;
    course->getPlane(eye, &sector, plane);
    if (sector >= 0 && plane[1] != 0.0f) {
        notFound = 0;
        eye[1] = -(eye[0] * plane[0] + eye[2] * plane[2] + plane[3]) / plane[1] + 0.1f;
        rng.next();
        eye[1] = eye[1] + rng.next01() * 0.6f;
    }
    rng.next();
    halfFov = (rng.next01() * 30.0f + 70.0f - 15.0f) * 0.5f;    /* 70 +- 15 degrees */
    zoom[0] = 1.0f / tanf(halfFov * 6.2831855f / 360.0f);
    rng.next();
    {
        f32 seconds = rng.next01();

        seconds = seconds + seconds + 1.0f;
        aim[0] = carX + moveX * 60.0f * seconds;
        aim[1] = carY + moveY * 60.0f * seconds;
        aim[2] = carZ + moveZ * 60.0f * seconds;
    }
    toCar[0] = carX - eye[0];
    toAim[0] = aim[0] - eye[0];
    toCar[1] = carY - eye[1];
    toAim[1] = aim[1] - eye[1];
    toCar[2] = carZ - eye[2];
    toAim[2] = aim[2] - eye[2];
    rot[1] = automatic_heading(toAim[0], toAim[1], toAim[2]);
    if (toCar[2] * toAim[0] - toCar[0] * toAim[2] < 0.0f)
        side = -1.0f;
    rng.next();
    {
        f32 rnd = rng.next01();

        rot[1] = rot[1] + side * halfFov;
        rot[0] = rnd * 20.0f + -5.0f - 10.0f;
    }
    rng.next();
    rot[2] = rng.next01() * 30.0f - 15.0f;

    {
        int intrWasOn = intr_suspend();
        pgl_push_matrix();
        vu0_load_identity_regs();
        vu0_rotate_z(rot[2]);
        vu0_rotate_x(rot[0]);
        vu0_rotate_y(rot[1]);
        pgl_translate(-eye[0], -eye[1], -eye[2]);
        pgl_get_matrix(matrix[0]);
        pgl_pop_matrix();
        intr_resume(intrWasOn);
    }
    return notFound ^ 1;
}

/* Returns 0 on the first call after set up, else 1; the view stays. */
// FUNC 0x00203EF0 AutomaticCamera::update
int AutomaticCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                                CarProgress *progress)
{
    int notFirst = 1;

    if (counter == 0) {
        notFirst = 0;
        counter = 1;
    }
    return notFirst;
}

// FUNC 0x00203F10 AutomaticCamera::virtual_9
int AutomaticCamera::virtual_9()
{
    return 3;
}

// FUNC 0x00203F18 AutomaticCamera::getEffect
int AutomaticCamera::getEffect()
{
    return effect;
}

/* the projection, the focal length scaled by the zoom (the inline getZoom:
 * retail loads the zoom first, into its own register) */
// FUNC 0x00203F20 AutomaticCamera::setProjection
void AutomaticCamera::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip,
                proj->farClip, proj->focal * getZoom(0));
    setLodBias(getZoom(0));
}

/* the current matrix times the view */
// FUNC 0x00203F78 AutomaticCamera::applyView
void AutomaticCamera::applyView()
{
    pgl_mult_matrix(matrix[0]);
}

// FUNC 0x00203F98 AutomaticCamera::getFadeOutTime(void)
f32 AutomaticCamera::getFadeOutTime()
{
    return 0.0f;
}

// FUNC 0x00203FA8 AutomaticCamera::isDrawBackMirror(void)
int AutomaticCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00203FB0 AutomaticCamera::isEnableTargetChange(void)
int AutomaticCamera::isEnableTargetChange()
{
    return 1;
}

// FUNC 0x00203FB8 AutomaticCamera::isEnableChanging(void)
int AutomaticCamera::isEnableChanging()
{
    return 1;
}

// FUNC 0x00203FC0 AutomaticCamera::isNeedReset(RaceOrganization&, CameraTargetInfo&)
int AutomaticCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    return 0;
}

// FUNC 0x00203FC8 AutomaticCamera::getChangeLevel(RaceOrganization&, int)
int AutomaticCamera::getChangeLevel(RaceOrganization &org, int level)
{
    return 0;
}

// FUNC 0x00203FD0 AutomaticCamera::isTargetDistEnable(void)
int AutomaticCamera::isTargetDistEnable()
{
    return 0;
}

// FUNC 0x00203FD8 AutomaticCamera::getMount(void)
int AutomaticCamera::getMount()
{
    return -1;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00203FE0 __tf15AutomaticCamera
extern "C" void *__tf15AutomaticCamera(void)
{
    if (__ti15AutomaticCamera[0] == 0) {
        __tf18VariableCameraBase();
        __rtti_si(__ti15AutomaticCamera, "15AutomaticCamera",
                  (const type_info *)__ti18VariableCameraBase);
    }
    return __ti15AutomaticCamera;
}
#endif
