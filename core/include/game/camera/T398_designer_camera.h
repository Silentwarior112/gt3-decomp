/* T398_designer_camera.h - DesignerCamera (translation unit T398, unit
 * u090), 0x00201418..0x00202AC0.
 *
 * The course ("designer") cameras of the replay.  Set up (setupOnRecord) with a
 * record of the course's camera file (CourseCameraRecord, T172): the record
 * covers [start, end] of the course; t (0..1) is the target's place in it (or
 * the time since the cut when the record has a duration), and the record's
 * tables give the camera position, the look-at point and the zoom at t.
 * Coordinate mode 3 records (and a car in the pit lane: setForceOnboard) use
 * the OnBoardCamera base instead.  The yaw and pitch towards the target go
 * through two CameraRotDampers (T379), the zoom through a CameraFollow (T407);
 * a history of 128 frames of yaw / zoom is kept.
 */
#ifndef GAME_T398_DESIGNER_CAMERA_H
#define GAME_T398_DESIGNER_CAMERA_H

#include "game/game.h"
#include "game/camera/T378_onboard_camera.h"       /* base OnBoardCamera */
#include "game/camera/T379_camera_rot_damper.h"    /* CameraRotDamper, CameraSway (held by value) */
#include "game/camera/T380_camera_spring.h"        /* CameraFollow, CameraBank (held by value) */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class CourseCameraRecord;                   /* T172 */
class RaceCourse;                           /* T153 */
class RaceEntryCar;                         /* T121 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */

/* ? name.  One frame of DesignerCamera's history, 0x14 bytes; [0] is this
 * frame. */
struct DesignerCameraFrame {
    f32 targetYaw;                          /* 0x00 the heading to the look-at point */
    f32 yaw;                                /* 0x04 the damped heading */
    f32 yawSpeed;                           /* 0x08 the yaw damper's speed */
    f32 zoom;                               /* 0x0C */
    f32 yawError;                           /* 0x10 |yaw - targetYaw| */
};
EGCS_SIZE(DesignerCameraFrame, 0x14);

/* vtable 0x0033E410 (25 slots: [8] OnBoardCamera's 0x001FF970; [24] new).
 * typeinfo __tf14DesignerCamera 0x00202A70 (__si_type_info 0x004B28D0, base
 * OnBoardCamera).  Size 0x14C0 (CameraView +0x480, the next member at
 * +0x1940; the data end at 0x148C, the rest is the 64-byte alignment of
 * VariableCameraBase::matrix). */
class DesignerCamera : public OnBoardCamera {
public:
    DesignerCamera();                       /* 0x00201480 type 1 */
    virtual ~DesignerCamera();              /* [0] +0x0C 0x00201530 */
    virtual void applyView();               /* [2] +0x1C 0x002029C0 the current matrix times `matrix' */
    virtual int  virtual_9();               /* [9] +0x54 0x00202450 3, the onboard one in mode 3 */
    virtual int  getEffect();              /* [10] +0x5C 0x00202488 effect */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00202960 the projection: focal x zoom, nearClip x 0.700208 x zoom */
    /* 0 on the first call after a set up, else 1 */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00201830 */
    virtual int  getMount();                /* [13] +0x74 0x002024C0 mode 3: the onboard mount, else -1 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x002024E0 coordMode != 3 */
    /* 0 stay, 1 may change, 2 change */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00202588 */
    /* the target left the record's range (with its changed flag) */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x002026F0 */
    virtual int  isEnableChanging();        /* [17] +0x94 0x002026E0 giveupTime < 1 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x00202A58 targetChangeOk != 0 */
    virtual int  isRangeOver();             /* [19] +0xA4 0x00202A50 rangeOver */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x00202A48 0 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x00202490 the record's (mode 3: 1, no record: 0) */
    virtual int  disableCrossOver();        /* [22] +0xBC 0x00202758 pitForced */
    virtual void applyMountView(RaceEntryCar *car);         /* [23] +0xC4 0x00202430 OnBoardCamera's */
    /* Set up on record `data' of the course's camera file, camera number
     * `number' (CameraManager T383, T409). */
    virtual void setupOnRecord(CameraTargetInfo *target, int innerLapOffset, RaceCourse *course, CourseCameraRecord *data, int number); /* [24] +0xCC 0x00201590 */

    /* the onboard camera instead (the pit lane; T383 / T389 / T390) */
    void setForceOnboard(CameraTargetInfo *target);         /* 0x00201708 */
    /* the target's course position for the record (innerLapOffset < 0: the
     * total position) */
    f32  coursePosition(CameraTargetInfo *target);          /* 0x00201760 */
    int  isCourseCoord();                   /* 0x002024F0 coordMode == 0 (no callers; u090 F_002024F0) */
    /* the look-at point is out of the frame (T389) */
    int  isTargetOutOfFrame(CameraTargetInfo *target);      /* 0x00202760 */
    void setInitBankAnim(f32 bank);         /* 0x002029E0 */
    void setInitRotOffset(f32 pitch, f32 yaw);              /* 0x00202A00 */
    void setInitZoom(f32 zoom);             /* 0x00202A18 */
    int  getCoordMode();                    /* 0x00202A28 */
    s16  getCameraNumber();                 /* 0x00202A30 */
    CourseCameraRecord *getCameraData();    /* 0x00202A38 */
    u8   isForceOnboard();                  /* 0x00202A40 */
    int  getInnerLapOffset();               /* 0x00202A68 */

    CourseCameraRecord *cameraData;         /* 0x180 the record (u093: the course camera position) */
    /*  0x184: alignment (CameraRotDamper is 8-byte aligned) */
    CameraRotDamper     yawDamper;          /* 0x188 */
    CameraRotDamper     pitchDamper;        /* 0x5D8 */
    CameraSway          sway;               /* 0xA28 */
    CameraFollow        zoomDamper;         /* 0xA3C */
    u8                  unk_A48[0xC];       /* 0xA48 ? (not accessed) */
    s16                 cameraNumber;       /* 0xA54 */
    u8                  unk_A56[2];         /* 0xA56 ? */
    s16                 initZoomSet;        /* 0xA58 */
    s16                 initRotOffsetSet;   /* 0xA5A */
    f32                 initZoom;           /* 0xA5C */
    f32                 initRotPitch;       /* 0xA60 */
    f32                 initRotYaw;         /* 0xA64 */
    CameraBank          bankAnim;           /* 0xA68 */
    int                 innerLapOffset;     /* 0xA70 laps added to the target's course position (< 0:
                                               the total position; u093: the position's level) */
    int                 startLap;           /* 0xA74 the target's lap at set up */
    int                 coordMode;          /* 0xA78 cameraData->flags & 3 (3: onboard) */
    int                 flags;              /* 0xA7C cameraData->flags (| 0x10 when forced onboard; signed:
                                               the code shifts it with sra) */
    s16                 targetChangeOk;     /* 0xA80 isEnableTargetChange */
    s16                 targetChangeTimer;  /* 0xA82 frames, up to 120 */
    f32                 t;                  /* 0xA84 the parameter in the record (0..1) */
    u8                  rangeOver;          /* 0xA88 */
    u8                  movingBack;         /* 0xA89 the target car went back along the lap (CarProgress::delta < 0;
                                               u090 behind) */
    u8                  forcedOnboard;      /* 0xA8A isForceOnboard */
    u8                  pitForced;          /* 0xA8B the pit lane forced onboard this frame */
    DesignerCameraFrame history[128];       /* 0xA8C */
};
EGCS_SIZE(DesignerCamera, 0x14C0);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf14DesignerCamera(void);   /* 0x00202A70 */
/* Its __si_type_info, which it fills. */
extern u32 __ti14DesignerCamera[3];         /* 0x004B28D0 */
}
#endif

#endif /* GAME_T398_DESIGNER_CAMERA_H */
