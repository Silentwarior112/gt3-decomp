/* T377_camera_target_info.h - CameraTargetInfo (translation unit T377, unit
 * u089), 0x001FED38..0x001FF788.
 *
 * What a camera view looks at (CameraView +0x1A0, T383): one car (mode 0) or
 * the midpoint of two (mode 1).  update() recomputes the point from the cars'
 * states; when the target changes (startBlend, from setCar / setPair) the
 * point blends from the old target to the new one over up to 60 frames along
 * an ease curve (T379).  save() keeps a snapshot of the position, from which
 * OnBoardCamera gets the target's speed.  The class name is the one in the
 * mangled isNeedReset(RaceOrganization&, CameraTargetInfo&) of the cameras;
 * the method names are the first pass's.
 */
#ifndef GAME_T377_CAMERA_TARGET_INFO_H
#define GAME_T377_CAMERA_TARGET_INFO_H

#include "game/game.h"
#include "game/camera/T379_camera_rot_damper.h"    /* CameraCurve (held by value) */

class RaceEntryCar;                         /* T121 (pointers only) */
class RaceOrganization;                     /* T125 */

/* Not polymorphic.  Size 0xD4 (CameraView +0x1A0, the next member at
 * +0x274). */
class CameraTargetInfo {
public:
    /* car `car' of org, mode 0, lap 1, curve.init(0, 0, 3) */
    void init(RaceOrganization *org, int car);       /* 0x001FED38 */
    /* advance: one blend frame passes (and `changed' is cleared) */
    void update(int advance);                        /* 0x001FF068 */
    void save();                                     /* 0x001FF380 savedIndex / savedMode / savedPos */
    void finishBlend();                              /* 0x001FF3D8 blendLeft = 0, update(0) */
    /* the current target becomes the blend's source; frames up to 60 */
    void startBlend(int car, int frames);            /* 0x001FF400 */
    int  setCar(int car, int frames);                /* 0x001FF490 1: a blend was running */
    /* two cars (car 0 alone when one is out of range) */
    int  setPair(int a, int b, int frames);          /* 0x001FF4F8 1: a blend was running */
    void refresh();                                  /* 0x001FF5D8 update(0) */
    f32  getSingleWeight();                          /* 0x001FF5F8 1 - the blend left (mode 0), else 1 */
    int  getSecondCar();                             /* 0x001FF648 index[1] in mode 1, else -1 */
    int  getSavedPosition(f32 *x, f32 *y, f32 *z);   /* 0x001FF668 0: nothing saved */
    void getCarPosition(int k, f32 *x, f32 *y, f32 *z); /* 0x001FF698 car[k] now */
    /* ? name.  The blended point (pos), inline: no copy in the binary (the
     * class has no vtable, so egcs emits none).  DesignerCamera::update
     * (T398) computes the three addresses before the loads and passes them
     * on to getCarPosition, as an inline's arguments are (u089 revisit,
     * 2026-10-10; u090's stand-in designer_target_pos before). */
    void getPosition(f32 *x, f32 *y, f32 *z) { *x = pos[0]; *y = pos[1]; *z = pos[2]; }
    int  isTarget(int car);                          /* 0x001FF728 (no callers) */

    s16               kind;                 /* 0x00 0..5 (3, 5: the course cameras cut hard) */
    s16               variant;              /* 0x02 */
    RaceOrganization *org;                  /* 0x04 */
    s16               index[7];             /* 0x08 the entries: [0] the car, [1] the second (mode 1) */
    s16               mode;                 /* 0x16 0: one car, 1: the midpoint of two */
    /* A union member in the original: retail's car[] accesses have alias
     * set 0 (egcs gives every access through a union member set 0).  The
     * store to car[0] makes setPair 0x001FF4F8 reload org->entries, and
     * OnBoardCamera::virtual_8 0x001FF970 keeps the car[0] load behind the
     * stores before it: both exact only so (MatrixCamera::update, T405,
     * reads car[0] before its timeLeft store for the same reason).  The
     * union's other members are not known (only car[] is accessed). */
    union {
    RaceEntryCar     *car[7];               /* 0x18 org->entries->cars[index[i]] (? 7, as index:
                                               [0], [1] used) */
    };
    u16               changed;              /* 0x34 1: the car changed since save(); update(1) clears it */
    /* Alias set 0 as well (u093 matching): CameraManager::updateDesigner
     * 0x0020AD18 keeps its wide load ahead of the views[].camera store
     * written after it, exact only so (with a plain u16 the store is
     * scheduled first).  A union member in the original, others unknown;
     * no other function of the files that access wide changes. */
    union {
    u16               wide;                 /* 0x36 the course camera search takes whole records
                                               (T409, camera_find_record); startBlend and the end of
                                               a blend clear it */
    };
    f32               distance;             /* 0x38 the course position counted over the laps */
    f32               lapDistance;          /* 0x3C in the lap */
    f32               pos[3];               /* 0x40 blended */
    f32               yaw;                  /* 0x4C of the car (mode 0), radians */
    f32               pitch;                /* 0x50 */
    f32               roll;                 /* 0x54 */
    f32               weight;               /* 0x58 (? how much the view is the single car's) */
    f32               rawDistance;          /* 0x5C before the blend */
    f32               rawLapDistance;       /* 0x60 */
    f32               rawPos[3];            /* 0x64 */
    int               lap;                  /* 0x70 wrapped to the course's camera laps (0 before the start) */
    int               savedIndex[7];        /* 0x74 */
    s16               savedMode;            /* 0x90 */
    s16               saved;                /* 0x92 1: save() was called */
    f32               savedPos[3];          /* 0x94 */
    int               fromMode;             /* 0xA0 the blend's source mode (-1: none yet) */
    RaceEntryCar     *fromCar[7];           /* 0xA4 (? 7, as car) */
    CameraCurve       curve;                /* 0xC0 the blend's ease */
    s16               blendLeft;            /* 0xD0 frames */
    s16               blendTotal;           /* 0xD2 */
};
EGCS_SIZE(CameraTargetInfo, 0xD4);

#endif /* GAME_T377_CAMERA_TARGET_INFO_H */
