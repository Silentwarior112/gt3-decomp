/* T405_matrix_camera.h - MatrixCamera (translation unit T405, unit u091),
 * 0x00204EA8..0x00205980.
 *
 * A randomised sweep around the car (replay / race start): yaw, height,
 * offset and field of view are drawn by setup and
 * interpolated over `time' seconds by the update; the camera is lifted above
 * the road when the road under it is higher.
 */
#ifndef GAME_T405_MATRIX_CAMERA_H
#define GAME_T405_MATRIX_CAMERA_H

#include "game/game.h"
#include "game/camera/T397_variable_camera_base.h" /* base VariableCameraBase */
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */
class type_info;                            /* T548_tinfo.h */

/* vtable 0x0033EBB8 (24 slots; [23] new).  typeinfo __tf12MatrixCamera
 * 0x00205930 (__si_type_info 0x004B2940, base VariableCameraBase).  Size
 * 0x180 (CameraView +0x1FC0). */
class MatrixCamera : public VariableCameraBase {
public:
    MatrixCamera();                         /* 0x00204EA8 type 5 */
    virtual ~MatrixCamera();                /* [0] +0x0C 0x00204EF8 */
    virtual void applyView();               /* [2] +0x1C 0x002058C8 the current matrix times `matrix' */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x00204F20 setup(target, 0) */
    virtual int  virtual_9();               /* [9] +0x54 0x00205850 3 */
    virtual int  getEffect();              /* [10] +0x5C 0x00205858 0x4000 */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00205870 the projection scaled by zoom[0] */
    /* the view of this frame of the sweep; returns 1 */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x002052C0 */
    virtual int  getMount();                /* [13] +0x74 0x00205928 -1 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x00205920 1 */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00205860 0 */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x00205868 target.changed */
    virtual int  isEnableChanging();        /* [17] +0x94 0x00205918 1 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x00205910 1 */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x00205908 0 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x002058F8 1 */
    /* a new random sweep of `time' seconds (the generator from the clock) */
    virtual void setup(CameraTargetInfo *target, f32 time);                    /* [23] +0xC4 0x00204F50 */

    /* animMode 0 -> 1 once updated over the road; 1: it stepped (no callers) */
    int  stepAnimMode();                    /* 0x00205218 */
    s8   getAnimMode();                     /* 0x002058E8 */
    void setCurrentTime(f32 t);             /* 0x002058F0 timeLeft = t */

    Random rng;                             /* 0x100 seeded from the clock (F_002502C8) at set up */
    u8     unk_104[0x8];                    /* 0x104 ? (not accessed) */
    f32    fov;                             /* 0x10C degrees, 40..70 */
    f32    radius;                          /* 0x110 extra distance, 0..5 */
    f32    yaw0;                            /* 0x114 start yaw, -30..30 */
    f32    yawSweep;                        /* 0x118 degrees swept, 100 + 0..220 x min(time, 3) */
    f32    yawDir;                          /* 0x11C +-1 */
    f32    height0;                         /* 0x120 0.1..1.1 */
    f32    heightSweep;                     /* 0x124 0..2 */
    f32    x1;                              /* 0x128 the end offset x, -1.5..1.5 */
    f32    x0;                              /* 0x12C the start offset x, -0.5..0.5 */
    f32    z1;                              /* 0x130 */
    f32    z0;                              /* 0x134 */
    f32    duration;                        /* 0x138 s */
    f32    timeLeft;                        /* 0x13C s */
    u8     onRoad;                          /* 0x140 the road under the camera was found */
    u8     updated;                         /* 0x141 */
    s8     animMode;                        /* 0x142 */
};
EGCS_SIZE(MatrixCamera, 0x180);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf12MatrixCamera(void);             /* 0x00205930 */
extern u32 __ti12MatrixCamera[3];           /* 0x004B2940 its __si_type_info (only T405 uses it) */
}
#endif

#endif /* GAME_T405_MATRIX_CAMERA_H */
