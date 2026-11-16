/* T379: CameraRotDamper - a damped angle (degrees) for DesignerCamera: a
 * history of the requested angle, a prediction from its last change, and a
 * speed-limited chase with a random gain; plus the angle helpers and
 * CameraCurve, a cubic ease. */
#include "game/camera/T379_camera_rot_damper.h"
#include "game/camera/camera_motion_internal.h"
#include "game/enhanced.h"                  /* ENH_CAMERA_TV_AIM */

#define ROT_HISTORY 128                     /* CameraRotDamper::history / delta */

/* Mode 1 (DesignerCamera's pitch damper): the aim is lowered by a x k x
 * range x cos(pitch), range the field-of-view rate (30 / zoom).  Lupo 2:
 * k 0.14, the car nearer the shot's vertical centre (L2 0x0020DADC /
 * 0x0020DBF4, cameras_sound.md C1); GT3 0.4. */
#if ENH_CAMERA_TV_AIM
#define ROT_AIM_OFFSET 0.14f
#else
#define ROT_AIM_OFFSET 0.4f
#endif

// FUNC 0x00206260 angle_diff
f32 angle_diff(f32 a, f32 b)
{
    f32 d = a - b;
    if (180.0f < d)
        d = d - 360.0f;
    if (d < -180.0f)
        d = d + 360.0f;
    return d;
}

// FUNC 0x002062B0 angle_wrap360
f32 angle_wrap360(f32 a)
{
    int n;
    if (360.0f < a) {
        n = (int)(a / 360.0f);
        a = a - (f32)n * 360.0f;
    }
    if (a < 0.0f) {
        a = -a;
        n = (int)(a / 360.0f);
        a = a - (f32)n * 360.0f;
        a = 360.0f - a;
    }
    return a;
}

// FUNC 0x00206330 CameraCurve::init
//   implicit: In double (fp-bit fptodp, dpadd, dpsub, dpmul, dpdiv, dptofp): a = p + q
//   - 2 / r, b = -2 p - q + 3 / r,
//   c = p, scale = r, so that eval(1) = 1 and the start slope is p r.
//   (1 / r is divided once per line; x 2.0 folds to x + x.)
void CameraCurve::init(f32 p, f32 q, f32 r)
{
    a = ((double)p + (double)q) - (1.0 / (double)r) * 2.0;
    b = ((double)p * -2.0 - (double)q) + (1.0 / (double)r) * 3.0;
    c = p;
    scale = r;
}

// FUNC 0x00206460 CameraCurve::eval
f32 CameraCurve::eval(f32 t)
{
    if (t < 0.0f)
        t = 0.0f;
    if (1.0f < t)
        t = 1.0f;
    return ((a * t + b) * t + c) * t * scale;
}

// FUNC 0x002064B8 CameraRotDamper::reset
void CameraRotDamper::reset(f32 angle)
{
    int i;
    for (i = 0; i < ROT_HISTORY; i++) {
        history[i] = 0.0f;
        delta[i] = 0.0f;
    }
    history[0] = angle;
    count = 0;
    curve.init(1.0f, 0.0f, 2.0f);
}

// FUNC 0x00206520 CameraRotDamper::push
void CameraRotDamper::push(f32 angle)
{
    int i;

    if (count < ROT_HISTORY)
        count++;
    for (i = ROT_HISTORY - 1; i > 0; i--) {
        history[i] = history[i - 1];
        delta[i] = delta[i - 1];
    }
    history[0] = angle;
    delta[0] = 0.0f;
    if (count >= 2)
        delta[0] = angle_diff(angle, history[1]);
}

// FUNC 0x002065A8 CameraRotDamper::predict
//   The newest angle plus 5 x its last change, the change eased: range x
//   curve(change / (3 range)), signed.
f32 CameraRotDamper::predict(f32 range)
{
    s64 n = count;
    f32 change, base;
    int i;

    base = history[0];
    /* an empty loop over the history is left in the code (? a sum whose
     * terms were optimised away) */
    if (n > 0)
        for (i = 0; i < n; i++)
            ;
    change = delta[0] * 5.0f;
    base = base + change;
    if (0.0f < change)
        change = range * curve.eval(change / (range * 3.0f));
    if (change < 0.0f)
        change = -range * curve.eval(-change / (range * 3.0f));
    return base + change;
}

// FUNC 0x002066A8 CameraRotDamper::CameraRotDamper
//   (shake.rng: Random's inline constructor, 0x87654321)
CameraRotDamper::CameraRotDamper()
{
    range = 90.0f;
    angle = 0.0f;
    speed = 0.0f;
}

// FUNC 0x002066E0 CameraRotDamper::~CameraRotDamper
//   (egcs: the flags bit 0 calls operator delete)
CameraRotDamper::~CameraRotDamper()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00206710 CameraRotDamper::start
f32 CameraRotDamper::start(f32 angle, f32 prev, f32 range, int mode, f32 a)
{
    switch (mode) {
    case 0:
        break;
    case 1:
        angle = angle - a * (range * ROT_AIM_OFFSET * cosf(angle * U091_PI / 180.0f));    /* Lupo 2: 0.14 (L2 0x0020DA88) */
        break;
    }
    this->angle = angle;
    reset(prev);
    push(angle);
    speed = angle_diff(angle, prev);
    shake.init();
    this->mode = mode;
    curve2.init(1.0f, 0.0f, 2.0f);
    return angle;
}

// FUNC 0x00206808 CameraRotDamper::update
//   The acceleration grows with the error to the prediction, with a random
//   gain (0.2..1.2 from shake) unless `fast'.
f32 CameraRotDamper::update(f32 angle, f32 range, f32 a, int fast)
{
    f32 predicted, error, gain, accel, v, wrapped;

    shake.update(1.0f);
    this->range = range;
    switch (mode) {
    case 0:
        break;
    case 1:
        angle = angle - a * ROT_AIM_OFFSET * range * cosf(angle * U091_PI / 180.0f);    /* Lupo 2: 0.14 (L2 0x0020DB80) */
        break;
    }
    push(angle);
    predicted = predict(range);
    error = angle_diff(angle, this->angle) + (predicted - angle);
    gain = (shake.get() + 1.0f) * 0.5f + 0.2f;
    if (fast)
        gain = 1.0f;
    accel = (gain * 300.0f * error + speed * -30.0f) / U091_FPS * 2.0f;
    if (!fast) {
        if (0.0f < accel) {
            if (25.0f < accel)
                accel = 25.0f;
        } else {
            if (accel < -25.0f)
                accel = -25.0f;
        }
    }
    v = speed + accel;
    speed = v;
    if (v < 0.0f) {
        if (v < -250.0f)
            speed = -250.0f;
    } else {
        if (250.0f < v)
            speed = 250.0f;
    }
    wrapped = angle_wrap360(this->angle + speed / U091_FPS);
    this->angle = wrapped;
    return wrapped;
}

// FUNC 0x00206A10 CameraRotDamper::getAngle
/* inline in game/camera/T379_camera_rot_damper.h; egcs emits this copy here (the
 * unit of the key function ~CameraRotDamper) */

// FUNC 0x00206A18 CameraRotDamper::getShake
/* inline in game/camera/T379_camera_rot_damper.h; egcs emits this copy here */
/* implicit: CameraSway::get (shake.get()) */

// FUNC 0x00206A38 CameraRotDamper::getSpeed
/* inline in game/camera/T379_camera_rot_damper.h; egcs emits this copy here */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti15CameraRotDamper (0x00353428, gp-0x45C8, name 0x0033EDE8
 * "15CameraRotDamper"; no bases) on the first call. */
// FUNC 0x00206A40 __tf15CameraRotDamper
extern "C" void *__tf15CameraRotDamper(void)
{
    if (__ti15CameraRotDamper[0] == 0)
        __rtti_user(__ti15CameraRotDamper, "15CameraRotDamper");
    return __ti15CameraRotDamper;
}
#endif
