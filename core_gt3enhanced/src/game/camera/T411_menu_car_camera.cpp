/* T411 - MenuCarCamera, 0x0020EFE0..0x00210008: the menu camera that orbits
 * a car (showroom, garage, car wash, prize cars, ...).
 *
 * Every coordinate is a MotionValue spring (T260): the eye trans (moved only
 * by updateTrans_, which has no callers), the point looked at interest, the
 * orbit angles tumbleX / tumbleY (pushed by a constant force each frame and
 * wrapped to [0, 360)), the orbit distance dist, and a translation shift and
 * a rotation tilt of the view.  The view (F_0020F9D8) puts the eye at
 * (0, 0, dist) turned by the orbit angles, looks from it at interest and
 * stores it back into trans.  The presets come from the showroom record of
 * the camera's mode (T215, values 0x34..0x4B); mode 2 mirrors them.
 *
 * The setters come in pairs: (values) and (const f32 *v) reading the same
 * values from a table; a MotionValue set to a value is at rest there.
 *
 * Matching (u094): the stores of each setter are in the natural member
 * order (sched1 emits the last of a run of independent stores first, so
 * retail's damping-first order is the source's damping-last one).
 *
 * Data: retail T411's .rodata (0x0033F648) is SimpleCameraBase::print's
 * string (the inline function's literal, written where T412's header is
 * read: first; nothing references it, DATA_START), loadPresetMirrored's
 * 360.0, the vtable and "13MenuCarCamera" only (none of the garage lists',
 * OperatorList's or RaceCameraBase's names).  Its .bss holds the two view
 * vectors (0x0049D0C0, 0x0049D0D0).
 */
// DATA_START 0x0033F648
#include "game/camera/camera_base_internal.h"
#include "game/camera/T412_simple_camera_base.h"   /* first: SimpleCameraBase::print's literal */
#include "game/camera/T410_camera_base.h"          /* CameraBase */
#include "game/camera/T411_menu_car_camera.h"
#include "engine/pgl/G001_flare.h"                /* pgl_matrix_mode / push / pop / frustum / translate,
                                               vu0_load_identity_regs, vu0_scale, vu0_rotate_x / y,
                                               vu0_apply_matrix, glu_look_at */
#include "engine/media/G002_mpeg_frameq.h"          /* tanf */
#include "game/menu/gtmode/T215_showroom_data.h"        /* showroom_index, showroom_value, showroom_value_ptr, showroom_table */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "engine/input/T475_pad_manager.h"          /* g_002EB170 (a spare Control) */
#include "engine/pdistd/T538_fmod_wrap.h"            /* fmod_wrap */

/* F_0020F9D8's eye, transformed from the point (0, 0, dist, 1) (.bss, file
 * statics: VU0 vectors, 16-byte aligned as the SDK's sceVu0FVECTOR; the
 * typedef keeps the attribute off the declarations, whose address comments
 * the scanner reads). */
typedef f32 MenuCameraVector[4] __attribute__((aligned(16)));
static MenuCameraVector g_0049D0C0;         /* 0x0049D0C0 */
static MenuCameraVector g_0049D0D0;         /* 0x0049D0D0 */

/* Caller: reset.  Every spring and its parameters 0 (mode and proj stay). */
// FUNC 0x0020EFE0 MenuCarCamera::clear
void MenuCarCamera::clear()
{
    transX.set(0);
    transY.set(0);
    transZ.set(0);
    transGoal[0] = 0;
    transGoal[1] = 0;
    transGoal[2] = 0;
    transSpring = 0;
    transDamping = 0;
    interestX.set(0);
    interestY.set(0);
    interestZ.set(0);
    interestGoal[0] = 0;
    interestGoal[1] = 0;
    interestGoal[2] = 0;
    interestSpring = 0;
    interestDamping = 0;
    unk_078.set(0);
    unk_084.set(0);
    unk_090.set(0);
    unk_09C = 0;
    unk_0A0 = 0;
    tumbleX.set(0);
    tumbleY.set(0);
    rotSpeed[0] = 0;
    rotSpeed[1] = 0;
    rotDamping = 0;
    dist.set(0);
    distGoal = 0;
    distSpring = 0;
    distDamping = 0;
    shiftX.set(0);
    shiftY.set(0);
    shiftGoal[0] = 0;
    shiftGoal[1] = 0;
    shiftSpring = 0;
    shiftDamping = 0;
    tiltX.set(0);
    tiltY.set(0);
    tiltGoal[0] = 0;
    tiltGoal[1] = 0;
    tiltSpring = 0;
    tiltDamping = 0;
}

/* Caller: CarGetMenuCamera::update (T106). */
// FUNC 0x0020F150 MenuCarCamera::setTrans
void MenuCarCamera::setTrans(f32 x, f32 y, f32 z)
{
    transX.set(x);
    transY.set(y);
    transZ.set(z);
}

// FUNC 0x0020F188 MenuCarCamera::setTrans
void MenuCarCamera::setTrans(const f32 *v)
{
    transX.set(v[0]);
    transY.set(v[1]);
    transZ.set(v[2]);
}

/* Caller: CarGetMenuCamera::update (T106). */
// FUNC 0x0020F1C8 MenuCarCamera::setTransGoal
void MenuCarCamera::setTransGoal(f32 x, f32 y, f32 z, f32 spring, f32 damping)
{
    transGoal[0] = x;
    transGoal[1] = y;
    transGoal[2] = z;
    transSpring = spring;
    transDamping = damping;
}

// FUNC 0x0020F1E0 MenuCarCamera::setTransGoal
void MenuCarCamera::setTransGoal(const f32 *v)
{
    transGoal[0] = v[0];
    transGoal[1] = v[1];
    transGoal[2] = v[2];
    transSpring = v[3];
    transDamping = v[4];
}

/* Caller: CarGetMenuCamera::update (T106). */
// FUNC 0x0020F210 MenuCarCamera::setInterest
void MenuCarCamera::setInterest(f32 x, f32 y, f32 z)
{
    interestX.set(x);
    interestY.set(y);
    interestZ.set(z);
}

/* Callers: loadPreset, loadPresetMirrored. */
// FUNC 0x0020F248 MenuCarCamera::setInterest
void MenuCarCamera::setInterest(const f32 *v)
{
    interestX.set(v[0]);
    interestY.set(v[1]);
    interestZ.set(v[2]);
}

/* Caller: CarGetMenuCamera::update (T106). */
// FUNC 0x0020F288 MenuCarCamera::setInterestGoal
void MenuCarCamera::setInterestGoal(f32 x, f32 y, f32 z, f32 spring, f32 damping)
{
    interestGoal[0] = x;
    interestGoal[1] = y;
    interestGoal[2] = z;
    interestSpring = spring;
    interestDamping = damping;
}

/* Callers: loadPreset, loadPresetMirrored. */
// FUNC 0x0020F2A0 MenuCarCamera::setInterestGoal
void MenuCarCamera::setInterestGoal(const f32 *v)
{
    interestGoal[0] = v[0];
    interestGoal[1] = v[1];
    interestGoal[2] = v[2];
    interestSpring = v[3];
    interestDamping = v[4];
}

/* No callers. */
// FUNC 0x0020F2D0 MenuCarCamera::F_0020F2D0
void MenuCarCamera::F_0020F2D0(f32 x, f32 y, f32 z)
{
    unk_078.set(x);
    unk_084.set(y);
    unk_090.set(z);
}

/* No callers. */
// FUNC 0x0020F308 MenuCarCamera::F_0020F308
void MenuCarCamera::F_0020F308(const f32 *v)
{
    unk_078.set(v[0]);
    unk_084.set(v[1]);
    unk_090.set(v[2]);
}

/* No callers. */
// FUNC 0x0020F348 MenuCarCamera::F_0020F348
void MenuCarCamera::F_0020F348(f32 a, f32 b)
{
    unk_09C = a;
    unk_0A0 = b;
}

/* No callers. */
// FUNC 0x0020F358 MenuCarCamera::F_0020F358
void MenuCarCamera::F_0020F358(const f32 *v)
{
    unk_09C = v[0];
    unk_0A0 = v[1];
}

/* No callers. */
// FUNC 0x0020F370 MenuCarCamera::setOrbit
void MenuCarCamera::setOrbit(f32 pitch, f32 yaw, f32 d)
{
    tumbleX.set(pitch);
    tumbleY.set(yaw);
    dist.set(d);
}

/* Callers: loadPreset, loadPresetMirrored. */
// FUNC 0x0020F3A8 MenuCarCamera::setOrbit
void MenuCarCamera::setOrbit(const f32 *v)
{
    tumbleX.set(v[0]);
    tumbleY.set(v[1]);
    dist.set(v[2]);
}

/* No callers. */
// FUNC 0x0020F3E8 MenuCarCamera::setOrbitMotion
void MenuCarCamera::setOrbitMotion(f32 pitchSpeed, f32 yawSpeed, f32 damping,
                                   f32 dGoal, f32 dSpring, f32 dDamping)
{
    rotSpeed[0] = pitchSpeed;
    rotSpeed[1] = yawSpeed;
    rotDamping = damping;
    distGoal = dGoal;
    distSpring = dSpring;
    distDamping = dDamping;
}

/* Callers: loadPreset, loadPresetMirrored. */
// FUNC 0x0020F408 MenuCarCamera::setOrbitMotion
void MenuCarCamera::setOrbitMotion(const f32 *v)
{
    rotSpeed[0] = v[0];
    rotSpeed[1] = v[1];
    rotDamping = v[2];
    distGoal = v[3];
    distSpring = v[4];
    distDamping = v[5];
}

/* No callers.  At rest at (x, y): the goals too. */
// FUNC 0x0020F440 MenuCarCamera::setShift
void MenuCarCamera::setShift(f32 x, f32 y)
{
    shiftX.set(x);
    shiftY.set(y);
    shiftGoal[0] = x;
    shiftGoal[1] = y;
}

/* Callers: loadPreset, loadPresetMirrored.  The values only (the goals
 * stay). */
// FUNC 0x0020F470 MenuCarCamera::setShift
void MenuCarCamera::setShift(const f32 *v)
{
    shiftX.set(v[0]);
    shiftY.set(v[1]);
}

/* No callers. */
// FUNC 0x0020F4A0 MenuCarCamera::setShiftGoal
void MenuCarCamera::setShiftGoal(f32 x, f32 y, f32 spring, f32 damping)
{
    shiftGoal[0] = x;
    shiftGoal[1] = y;
    shiftSpring = spring;
    shiftDamping = damping;
}

/* No callers. */
// FUNC 0x0020F4B8 MenuCarCamera::setShiftGoal
void MenuCarCamera::setShiftGoal(const f32 *v)
{
    shiftGoal[0] = v[0];
    shiftGoal[1] = v[1];
    shiftSpring = v[2];
    shiftDamping = v[3];
}

/* Caller: gt2k_menu::Car::setView (T212).  At rest at (x, y): the goals
 * too. */
// FUNC 0x0020F4E0 MenuCarCamera::setTilt
void MenuCarCamera::setTilt(f32 x, f32 y)
{
    tiltX.set(x);
    tiltY.set(y);
    tiltGoal[0] = x;
    tiltGoal[1] = y;
}

/* Callers: loadPreset, loadPresetMirrored.  The values and the goals:
 * v[0..1] the values, v[2..3] the goals (unlike setShift(const f32 *)). */
// FUNC 0x0020F510 MenuCarCamera::setTilt
void MenuCarCamera::setTilt(const f32 *v)
{
    tiltX.set(v[0]);
    tiltY.set(v[1]);
    tiltGoal[0] = v[2];
    tiltGoal[1] = v[3];
}

/* No callers. */
// FUNC 0x0020F550 MenuCarCamera::setTiltGoal
void MenuCarCamera::setTiltGoal(f32 x, f32 y, f32 spring, f32 damping)
{
    tiltGoal[0] = x;
    tiltGoal[1] = y;
    tiltSpring = spring;
    tiltDamping = damping;
}

/* No callers. */
// FUNC 0x0020F568 MenuCarCamera::setTiltGoal
void MenuCarCamera::setTiltGoal(const f32 *v)
{
    tiltGoal[0] = v[0];
    tiltGoal[1] = v[1];
    tiltSpring = v[2];
    tiltDamping = v[3];
}

/* Caller: reset.  The preset of the showroom record of mode (see the
 * SHOWROOM_* values).  QUIRK: setTilt(const f32 *) reads four values from
 * SHOWROOM_SHIFT + 2 = 0x49, so its goals are the field of view (0x4B) and
 * the word after the record (0x4C: the next record's first word); harmless,
 * tiltSpring stays 0 (clear()), so the goals never pull. */
// FUNC 0x0020F590 MenuCarCamera::loadPreset
void MenuCarCamera::loadPreset()
{
    int record = showroom_index(this);

    setInterest(showroom_value_ptr(showroom_table(), SHOWROOM_INTEREST, record));
    setInterestGoal(showroom_value_ptr(showroom_table(), SHOWROOM_INTEREST_GOAL, record));
    setOrbit(showroom_value_ptr(showroom_table(), SHOWROOM_ORBIT, record));
    setOrbitMotion(showroom_value_ptr(showroom_table(), SHOWROOM_ORBIT_MOTION, record));
    setShift(showroom_value_ptr(showroom_table(), SHOWROOM_SHIFT, record));
    setTilt(showroom_value_ptr(showroom_table(), SHOWROOM_SHIFT, record) + 2);
    f32 fov = showroom_value(showroom_table(), SHOWROOM_FOV, record);
    /* the focal distance of the 640 x 448 window for that field of view */
    f32 focal = MENU_CAMERA_HALF_W / tanf(((fov * 0.5f) / 180.0f) * 3.1415927f);
    setProjection(-MENU_CAMERA_HALF_W, MENU_CAMERA_HALF_W, -MENU_CAMERA_HALF_H, MENU_CAMERA_HALF_H,
                  MENU_CAMERA_NEAR, MENU_CAMERA_FAR, focal);
}

/* Caller: reset (mode 2).  The preset mirrored in x: interest x, the yaw
 * (360 - yaw, in double precision), its speed, shift x and the tilt about y
 * negated (the same QUIRK of setTilt as loadPreset).  Retail reads and
 * writes the negated values through the MotionValue (MotionValue's inline
 * get / setValue: addiu v0, this, off; lwc1 8(v0)) and keeps &tumbleY in a
 * saved register from the start (getTumbleDynamY, before showroom_index). */
// FUNC 0x0020F718 MenuCarCamera::loadPresetMirrored
void MenuCarCamera::loadPresetMirrored()
{
    MotionValue *yaw = getTumbleDynamY();
    int record = showroom_index(this);

    setInterest(showroom_value_ptr(showroom_table(), SHOWROOM_INTEREST, record));
    interestX.setValue(-interestX.get());
    setInterestGoal(showroom_value_ptr(showroom_table(), SHOWROOM_INTEREST_GOAL, record));
    interestGoal[0] = -interestGoal[0];
    setOrbit(showroom_value_ptr(showroom_table(), SHOWROOM_ORBIT, record));
    /* implicit: fptodp / dpsub / dptofp: the soft-float double helpers */
    yaw->value = (f32)(360.0 - (f64)yaw->value);
    setOrbitMotion(showroom_value_ptr(showroom_table(), SHOWROOM_ORBIT_MOTION, record));
    rotSpeed[1] = -rotSpeed[1];
    setShift(showroom_value_ptr(showroom_table(), SHOWROOM_SHIFT, record));
    shiftX.setValue(-shiftX.get());
    shiftGoal[0] = -shiftGoal[0];
    setTilt(showroom_value_ptr(showroom_table(), SHOWROOM_SHIFT, record) + 2);
    tiltY.setValue(-tiltY.get());
    tiltGoal[1] = -tiltGoal[1];
    f32 fov = showroom_value(showroom_table(), SHOWROOM_FOV, record);
    /* the focal distance of the 640 x 448 window for that field of view */
    f32 focal = MENU_CAMERA_HALF_W / tanf(((fov * 0.5f) / 180.0f) * 3.1415927f);
    setProjection(-MENU_CAMERA_HALF_W, MENU_CAMERA_HALF_W, -MENU_CAMERA_HALF_H, MENU_CAMERA_HALF_H,
                  MENU_CAMERA_NEAR, MENU_CAMERA_FAR, focal);
}

/* Callers: gt2k_menu::Car::setCar, gt2k_menu::Car::virtual_1 (T212).  The
 * preset of mode, and one frame of the springs (virtual_8 through the
 * vtable, with the spare Control). */
// FUNC 0x0020F918 MenuCarCamera::reset
void MenuCarCamera::reset()
{
    clear();
    if (type == MENU_CAMERA_MODE_MIRRORED)
        loadPresetMirrored();
    else
        loadPreset();
    virtual_8(&g_002EB170);
}

/* Callers: gt2k_menu::Car::setView (T212), loadPreset, loadPresetMirrored. */
// FUNC 0x0020F980 MenuCarCamera::setProjection
void MenuCarCamera::setProjection(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f, f32 d)
{
    proj[0] = l;
    proj[1] = r;
    proj[2] = b;
    proj[3] = t;
    proj[4] = n;
    proj[5] = f;
    proj[6] = d;
}

// FUNC 0x0020F9A0 MenuCarCamera::applyProjection
void MenuCarCamera::applyProjection()
{
    pgl_frustum(proj[0], proj[1], proj[2], proj[3], proj[4], proj[5], proj[6]);
}

/* Caller: applyView.  The eye: (0, 0, dist) scaled in x by scaleX, turned
 * about y by tumbleY and about x by tumbleX (on a pushed modelview level); then on
 * the modelview itself the shift, the tilt and the look-at from the eye to
 * interest (up = +y); the eye is stored into trans, at rest.  The two vectors
 * are used as the arrays themselves (retail: element 0 through %lo, the
 * others through the address passed to vu0_apply_matrix), the interest
 * through MotionValue::get (its loads come before the eye's). */
// FUNC 0x0020F9D8 MenuCarCamera::F_0020F9D8
void MenuCarCamera::F_0020F9D8(f32 scaleX)
{
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    vu0_load_identity_regs();
    g_0049D0D0[0] = 0.0f;
    g_0049D0D0[1] = 0.0f;
    g_0049D0D0[2] = dist.value;
    g_0049D0D0[3] = 1.0f;
    vu0_scale(scaleX, 1.0f, 1.0f);
    vu0_rotate_y(tumbleY.value);
    vu0_rotate_x(tumbleX.value);
    vu0_apply_matrix(g_0049D0C0, g_0049D0D0);
    pgl_pop_matrix();

    pgl_translate(shiftX.value, shiftY.value, 0.0f);
    vu0_rotate_y(tiltY.value);
    vu0_rotate_x(tiltX.value);
    glu_look_at(g_0049D0C0[0], g_0049D0C0[1], g_0049D0C0[2],
                interestX.get(), interestY.get(), interestZ.get(),
                0.0f, 1.0f, 0.0f);
    transX.set(g_0049D0C0[0]);
    transY.set(g_0049D0C0[1]);
    transZ.set(g_0049D0C0[2]);
}

// FUNC 0x0020FB30 MenuCarCamera::applyView
void MenuCarCamera::applyView()
{
    F_0020F9D8(1.0f);
}

/* The value wrapped into [lo, hi) (fmod_wrap, T538). */
static inline void wrapValue(MotionValue *m, f32 lo, f32 hi)
{
    m->value = m->value - lo;
    m->value = fmod_wrap(1, m->value, hi - lo) + lo;
}

/* Callers: CarGetMenuCamera::update (T106), gt2k_menu::Car::virtual_1
 * (T212), reset.  Each spring pulled towards its goal by (goal - value) *
 * spring (MotionValue::pull: the value read through the MotionValue, as in
 * updateTrans_) and damped; the angles pushed by their speed and wrapped to
 * [0, 360) (the `- 0.0f' / `+ 0.0f' are in the code: wrapValue, an inline
 * wrap into [lo, hi) around fmod_wrap; retail keeps &tumbleX / &tumbleY in
 * $s1 / $s2 and lo in $f20).  ctl is not used. */
// FUNC 0x0020FB58 MenuCarCamera::virtual_8
void MenuCarCamera::virtual_8(Control *ctl)
{
    interestX.pull(interestGoal[0], interestSpring);
    interestX.update(interestDamping);
    interestY.pull(interestGoal[1], interestSpring);
    interestY.update(interestDamping);
    interestZ.pull(interestGoal[2], interestSpring);
    interestZ.update(interestDamping);
    dist.pull(distGoal, distSpring);
    dist.update(distDamping);

    tumbleX.force = tumbleX.force + rotSpeed[0];
    tumbleX.update(rotDamping);
    wrapValue(&tumbleX, 0.0f, 360.0f);  /* implicit: fmod_wrap (wrapValue) */
    tumbleY.force = tumbleY.force + rotSpeed[1];
    tumbleY.update(rotDamping);
    wrapValue(&tumbleY, 0.0f, 360.0f);

    shiftX.pull(shiftGoal[0], shiftSpring);
    shiftX.update(shiftDamping);
    shiftY.pull(shiftGoal[1], shiftSpring);
    shiftY.update(shiftDamping);
    tiltX.pull(tiltGoal[0], tiltSpring);
    tiltX.update(tiltDamping);
    tiltY.pull(tiltGoal[1], tiltSpring);
    tiltY.update(tiltDamping);
}

/* ---- The inline accessors of game/camera/T411_menu_car_camera.h, emitted after
 * T411's own functions (with the inline constructor among them); no
 * callers.  Lupo 2 names them getTumbleDynamY, getTumbleY, getType, setType,
 * updateTrans_ (u094 notes). ---- */

// FUNC 0x0020FD88 MenuCarCamera::getTumbleDynamY
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &tumbleY */

// FUNC 0x0020FD90 MenuCarCamera::getTumbleY
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: tumbleY.value */

// FUNC 0x0020FD98 MenuCarCamera::getType
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: type */

// FUNC 0x0020FDA0 MenuCarCamera::setType
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: type = t
 * (u064: the users expand it) */

// FUNC 0x0020FDA8 MenuCarCamera::MenuCarCamera
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy (the owners
 * expand it): the 16 MotionValue constructors zero them, the vptr, mode 0;
 * the other members are not set. */

// FUNC 0x0020FEC0 MenuCarCamera::updateTrans_ (updateTrans___13MenuCarCamera)
/* implicit: inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: one frame of
 * the eye springs, transX / Y / Z each pulled towards transGoal by
 * transSpring and stepped by MotionValue::update(transDamping). */

// FUNC 0x0020FF68 MenuCarCamera::getInterestZ_ (getInterestZ___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &interestZ */

// FUNC 0x0020FF70 MenuCarCamera::getInterestY_ (getInterestY___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &interestY */

// FUNC 0x0020FF78 MenuCarCamera::getInterestX_ (getInterestX___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &interestX */

// FUNC 0x0020FF80 MenuCarCamera::getTransZ_ (getTransZ___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &transZ */

// FUNC 0x0020FF88 MenuCarCamera::getTransY_ (getTransY___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &transY */

// FUNC 0x0020FF90 MenuCarCamera::getTransX_ (getTransX___13MenuCarCamera)
/* inline in game/camera/T411_menu_car_camera.h; egcs emits this copy: &transX */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0020FF98 __tf13MenuCarCamera
extern "C" void *__tf13MenuCarCamera(void)
{
    if (__ti13MenuCarCamera[0] == 0) {
        __tf10CameraBase();
        __rtti_si(__ti13MenuCarCamera, "13MenuCarCamera",
                  (const type_info *)__ti10CameraBase);
    }
    return __ti13MenuCarCamera;
}
#endif

// FUNC 0x0020FFE8 MenuCarCamera::~MenuCarCamera
/* implicit: ~CameraBase; the implicit destructor (game/camera/T411_menu_car_camera.h), the vtable's slot
 * 0: CameraBase::~CameraBase with the same flags, without a vptr store. */
