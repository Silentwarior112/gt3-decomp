/* T406_zoomup_camera.h - ZoomupCamera (translation unit T406, unit u091),
 * 0x00205980..0x00206260.
 *
 * A close-up from a random point near the car (drawn by setup), shaken by two CameraShakes (T378) and two CameraSprings (T380)
 * that grow with the car's speed; lifted above the road when the road under
 * it is higher.
 */
#ifndef GAME_T406_ZOOMUP_CAMERA_H
#define GAME_T406_ZOOMUP_CAMERA_H

#include "game/game.h"
#include "game/camera/T397_variable_camera_base.h" /* base VariableCameraBase */
#include "game/camera/T378_onboard_camera.h"       /* CameraShake (held by value) */
#include "game/camera/T380_camera_spring.h"        /* CameraSpring (held by value) */
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */
class type_info;                            /* T548_tinfo.h */

/* vtable 0x0033ECD0 (24 slots; [23] new).  typeinfo __tf12ZoomupCamera
 * 0x00206210 (__si_type_info 0x004B2950, base VariableCameraBase).  Size
 * 0x180 (CameraView +0x2140). */
class ZoomupCamera : public VariableCameraBase {
public:
    ZoomupCamera();                         /* 0x00205980 type 5 */
    virtual ~ZoomupCamera();                /* [0] +0x0C 0x002059D0 */
    virtual void applyView();               /* [2] +0x1C 0x002061B8 the current matrix times `matrix' */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x002059F8 empty */
    virtual int  virtual_9();               /* [9] +0x54 0x00206140 3 */
    virtual int  getEffect();              /* [10] +0x5C 0x00206148 0x4000 */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00206160 the projection scaled by zoom[0] */
    /* the view; once, the camera is checked against the road (offY lowered
     * when the road is higher); returns 1 */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00205CA8 */
    virtual int  getMount();                /* [13] +0x74 0x00206208 -1 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x00206200 1 */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00206150 0 */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x00206158 target.changed */
    virtual int  isEnableChanging();        /* [17] +0x94 0x002061F8 1 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x002061F0 1 */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x002061E8 0 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x002061D8 1 */
    /* a new random close-up (the generator from the clock; time unused) */
    virtual void setup(CameraTargetInfo *target, f32 time);                    /* [23] +0xC4 0x00205A00 */

    Random       rng;                       /* 0x100 seeded from the clock (F_002502C8) at set up */
    u8           unk_104[0x8];              /* 0x104 ? (not accessed) */
    f32          fov;                       /* 0x10C degrees, 25..40 */
    f32          offZ;                      /* 0x110 the camera offset along the car */
    f32          offY;                      /* 0x114 height (negative: up) */
    f32          offX;                      /* 0x118 sideways, -1.5..1.5 (0: 30%) */
    u8           unk_11C[0x8];              /* 0x11C ? (not accessed) */
    f32          aimY;                      /* 0x124 the aim point's height */
    int          behind;                    /* 0x128 1: offZ is behind the car */
    CameraShake  pitchWobble;               /* 0x12C */
    CameraShake  yawWobble;                 /* 0x138 */
    CameraSpring yawShake;                  /* 0x144 */
    CameraSpring pitchShake;                /* 0x150 */
    f32          shake;                     /* 0x15C 0..1 from the car's speed */
};
EGCS_SIZE(ZoomupCamera, 0x180);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf12ZoomupCamera(void);             /* 0x00206210 */
extern u32 __ti12ZoomupCamera[3];           /* 0x004B2950 its __si_type_info (only T406 uses it) */
}
#endif

#endif /* GAME_T406_ZOOMUP_CAMERA_H */
