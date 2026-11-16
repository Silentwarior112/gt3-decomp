/* T379_camera_rot_damper.h - CameraRotDamper, CameraCurve and the angle
 * helpers (translation unit T379, unit u091), 0x00206260..0x00206A80.
 *
 * CameraRotDamper damps an angle (degrees) for DesignerCamera (T398, its yaw
 * and pitch): a history of the requested angle, a prediction from its last
 * change, and a speed-limited chase with a random gain (a CameraSway).
 * CameraCurve is a cubic ease 0 -> 1 (also the blend curve of
 * CameraTargetInfo, T377).
 *
 * Also here: CameraSway, the random sway of translation unit T381 (u091,
 * 0x00206CC0..0x00206F10), which has no canonical header of its own; it is
 * held by value by CameraRotDamper and by DesignerCamera.
 */
#ifndef GAME_T379_CAMERA_ROT_DAMPER_H
#define GAME_T379_CAMERA_ROT_DAMPER_H

#include "game/game.h"
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

class type_info;                            /* T548_tinfo.h */

/* a - b wrapped into -180..180 (degrees) */
f32 angle_diff(f32 a, f32 b);               /* 0x00206260 */
/* a wrapped into 0..360 */
f32 angle_wrap360(f32 a);                   /* 0x002062B0 */

/* ? name.  A cubic ease curve, 0x10 bytes: eval(t) = ((a t + b) t + c) t
 * scale, t clamped to 0..1; init(p, q, r) makes eval(1) = 1 with start
 * slope p (c = p, scale = r). */
struct CameraCurve {
    /* a = p + q - 2 / r, b = -2 p - q + 3 / r (computed in double) */
    void init(f32 p, f32 q, f32 r);         /* 0x00206330 */
    f32  eval(f32 t);                       /* 0x00206460 */

    f32 a;                                  /* 0x0 t^3 */
    f32 b;                                  /* 0x4 t^2 */
    f32 c;                                  /* 0x8 t (the start slope) */
    f32 scale;                              /* 0xC */
};
EGCS_SIZE(CameraCurve, 0x10);

/* ? name (u090; u091 called it CameraShake, the name of T378's head shake).
 * T381: every 3rd frame, with a probability that grows with `amount', a new
 * random target in -102.4..76.8 (128 x -0.8..0.6); the value chases it with
 * a spring.  Read as value / 128.  0x14 bytes. */
struct CameraSway {
    /* rng from the clock (F_002502C8), the rest 0 */
    void init();                            /* 0x00206CC0 */
    void pickTarget(f32 amount);            /* 0x00206D08 */
    f32  get();                             /* 0x00206E50 value / 128 */
    void update(f32 amount);                /* 0x00206E68 pickTarget, one spring step */

    f32    value;                           /* 0x00 */
    f32    target;                          /* 0x04 */
    s16    gain;                            /* 0x08 0 just after a new target, then 1 */
    s16    tick;                            /* 0x0A a new target is considered every 3rd frame */
    f32    vel;                             /* 0x0C */
    Random rng;                             /* 0x10 (0x87654321 by its constructor) */
};
EGCS_SIZE(CameraSway, 0x14);

/* vtable 0x0033EDD8 (1 slot).  typeinfo __tf15CameraRotDamper 0x00206A40
 * (__user_type_info 0x00353428, gp-0x45C8).  0x44C bytes of data, then the
 * egcs vptr (+0x44C); 8-byte aligned (count is 64-bit).  Size 0x450
 * (DesignerCamera +0x188 and +0x5D8). */
class CameraRotDamper {
public:
    CameraRotDamper();                      /* 0x002066A8 range 90 */
    virtual ~CameraRotDamper();             /* [0] +0x0C 0x002066E0 */

    /* history = 0 but [0] = angle, count = 0, curve.init(1, 0, 2) */
    void reset(f32 angle);                  /* 0x002064B8 */
    void push(f32 angle);                   /* 0x00206520 into the history, delta[0] = its change */
    /* the newest angle + 5 x its last change, eased by curve */
    f32  predict(f32 range);                /* 0x002065A8 */
    /* Start at `angle' coming from `prev' (mode 1: angle corrected by
     * range x a x 0.4 x cos(angle)); returns the angle.  The int argument
     * after the floats (u090): DesignerCamera::update, the only caller, sets
     * $a1 after $f12..$f14, as egcs does in parameter order (same registers
     * either way: the callee's code is unchanged). */
    f32  start(f32 angle, f32 prev, f32 range, int mode, f32 a);    /* 0x00206710 */
    /* One frame of the chase of `angle' (+-25 a frame and a random gain
     * unless `fast'; the speed +-250 degrees / s); returns the new angle.
     * `fast' last, as start's mode (u090). */
    f32  update(f32 angle, f32 range, f32 a, int fast);             /* 0x00206808 */
    f32  getAngle();                        /* 0x00206A10 inline, below */
    f32  getShake();                        /* 0x00206A18 shake.get() (inline) */
    f32  getSpeed();                        /* 0x00206A38 (inline) */

    s64         count;                      /* 0x000 samples in the history (64-bit ld / sd), up to 128 */
    f32         history[128];               /* 0x008 [0]: the newest angle */
    f32         delta[128];                 /* 0x208 angle_diff(history[i], history[i + 1]) */
    CameraCurve curve;                      /* 0x408 the prediction's ease */
    f32         angle;                      /* 0x418 the damped angle, degrees */
    f32         range;                      /* 0x41C 90 at construction */
    int         mode;                       /* 0x420 1: the angle is corrected by a cos term */
    f32         speed;                      /* 0x424 degrees / s, -250..250 */
    CameraSway  shake;                      /* 0x428 the random gain */
    CameraCurve curve2;                     /* 0x43C */
    /*  vptr                                   0x44C */
};
EGCS_SIZE(CameraRotDamper, 0x450);

/* The accessors are inline (their copies 0x00206A10..0x00206A38 have no
 * callers; DesignerCamera::update reads the speed through getSpeed, retail
 * keeping &yawDamper in a register: the inline's `this').  egcs emits the
 * copies in T379, the unit of the key function ~CameraRotDamper, in reverse
 * definition order before __tf15CameraRotDamper, as the binary has them:
 * so defined in the reverse order of their addresses (u091 revisit,
 * 2026-10-10). */
inline f32 CameraRotDamper::getSpeed() { return speed; }        /* 0x00206A38 */
inline f32 CameraRotDamper::getShake() { return shake.get(); }  /* 0x00206A18 */
inline f32 CameraRotDamper::getAngle() { return angle; }        /* 0x00206A10 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf15CameraRotDamper(void);          /* 0x00206A40 */
extern u32 __ti15CameraRotDamper[2];        /* 0x00353428 gp-0x45C8 its __user_type_info (only T379 uses it) */
}
#endif

#endif /* GAME_T379_CAMERA_ROT_DAMPER_H */
