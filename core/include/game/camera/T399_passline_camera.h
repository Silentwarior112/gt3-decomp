/* T399_passline_camera.h - PasslineCamera (translation unit T399, unit
 * u090), 0x00202AC0..0x00202F60.
 *
 * A course camera on the first record of "cameras/start.cam" (T153 loads it):
 * the record's camera and look-at tables at timeRange (0..1, set from outside;
 * StartingCamera T400 drives it), without damping.
 */
#ifndef GAME_T399_PASSLINE_CAMERA_H
#define GAME_T399_PASSLINE_CAMERA_H

#include "game/game.h"
#include "game/camera/T397_variable_camera_base.h" /* base VariableCameraBase */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class CourseCameraRecord;                   /* T172 */
class RaceCourse;                           /* T153 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */

/* vtable 0x0033E520 (24 slots; [23] new).  typeinfo __tf14PasslineCamera
 * 0x00202F10 (__si_type_info 0x004B28E0, base VariableCameraBase).  Size
 * 0x140 (CameraView +0x1940; data to 0x10C, then the 64-byte alignment). */
class PasslineCamera : public VariableCameraBase {
public:
    PasslineCamera();                       /* 0x00202AC0 type 1 */
    virtual ~PasslineCamera();              /* [0] +0x0C 0x00202B00 */
    virtual void applyView();               /* [2] +0x1C 0x00202E80 the current matrix times `matrix' */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x00202B28 setup(target, NULL) */
    virtual int  virtual_9();               /* [9] +0x54 0x00202E18 */
    virtual int  getEffect();              /* [10] +0x5C 0x00202E20 effect */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00202E28 the projection scaled by zoom */
    /* 0 on the first call after a set up, else 1 */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00202B90 */
    virtual int  getMount();                /* [13] +0x74 0x00202F08 -1 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x00202F00 0 */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00202EF8 0 */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x00202EF0 0 */
    virtual int  isEnableChanging();        /* [17] +0x94 0x00202EE8 1 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x00202EE0 1 */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x00202ED8 0 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x00202EC8 0 */
    /* Set up: with a course, the first record of start.cam. */
    virtual void setup(CameraTargetInfo *target, RaceCourse *course);          /* [23] +0xC4 0x00202B58 */

    f32  getTimeRange();                    /* 0x00202EA0 */
    void setTimeRange(f32 t);               /* 0x00202EA8 */
    s16  F_00202EB0();                      /* 0x00202EB0 unk_104 (no callers) */
    RaceCourse *getCourse();                /* 0x00202EB8 course (no callers; u090 F_00202EB8) */
    CourseCameraRecord *getCameraData();    /* 0x00202EC0 */

    CourseCameraRecord *cameraData;         /* 0x100 the first record of start.cam (NULL: no course) */
    s16                 unk_104;            /* 0x104 */
    u8                  unk_106[2];         /* 0x106 ? */
    f32                 timeRange;          /* 0x108 the parameter in the record (0..1) */
};
EGCS_SIZE(PasslineCamera, 0x140);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tf14PasslineCamera(void);   /* 0x00202F10 */
/* Its __si_type_info, which it fills (__tf14StartingCamera, T400, uses it). */
extern u32 __ti14PasslineCamera[3];         /* 0x004B28E0 */
}
#endif

#endif /* GAME_T399_PASSLINE_CAMERA_H */
