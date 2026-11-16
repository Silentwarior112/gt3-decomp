/* T403_logger_camera.h - LoggerCamera (translation unit T403, unit u090),
 * 0x002047D0..0x00204B10.
 *
 * A fixed chase camera: `distance' behind and `height' above the car,
 * pitched down by `pitch' degrees, turning with the car's heading (not its
 * pitch or roll).  Field of view 80 degrees.
 */
#ifndef GAME_T403_LOGGER_CAMERA_H
#define GAME_T403_LOGGER_CAMERA_H

#include "game/game.h"
#include "game/camera/T397_variable_camera_base.h" /* base VariableCameraBase */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */

/* vtable 0x0033E970 (23 slots: no slot of its own).  typeinfo
 * __tf12LoggerCamera 0x00204AC0 (__si_type_info 0x004B2920, base
 * VariableCameraBase).  Size 0x140 (CameraView +0x1E80). */
class LoggerCamera : public VariableCameraBase {
public:
    LoggerCamera();                         /* 0x002047D0 type 0 */
    virtual ~LoggerCamera();                /* [0] +0x0C 0x00204810 */
    virtual void applyView();               /* [2] +0x1C 0x00204A58 the current matrix times `matrix' */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x00204838 pitch 20, height 5, distance 10 */
    virtual int  virtual_9();               /* [9] +0x54 0x002049F0 2 */
    virtual int  getEffect();              /* [10] +0x5C 0x002049F8 effect */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00204A00 the projection scaled by zoom */
    /* returns 1; a target change (target->changed) clears doppler */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x002048C8 */
    virtual int  getMount();                /* [13] +0x74 0x00204AB8 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x00204AB0 */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00204AA8 */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x00204AA0 */
    virtual int  isEnableChanging();        /* [17] +0x94 0x00204A98 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x00204A90 */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x00204A88 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x00204A78 */

    f32 height;                             /* 0x100 above the car */
    f32 distance;                           /* 0x104 behind the car */
    f32 fov;                                /* 0x108 degrees (set, not read here) */
    f32 pitch;                              /* 0x10C degrees down */
};
EGCS_SIZE(LoggerCamera, 0x140);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tf12LoggerCamera(void);   /* 0x00204AC0 */
/* Its __si_type_info, which it fills. */
extern u32 __ti12LoggerCamera[3];           /* 0x004B2920 */
}
#endif

#endif /* GAME_T403_LOGGER_CAMERA_H */
