/* T404_heli_camera.h - HeliCamera (translation unit T404, unit u091),
 * 0x00204B10..0x00204EA8.
 *
 * The "helicopter" replay view: an AutomountCamera (T402) that follows its
 * car along a course camera record (T172) and reports how far along the
 * record the car is (getChangeLevel); 30 s at most, 3 s before it may change.
 */
#ifndef GAME_T404_HELI_CAMERA_H
#define GAME_T404_HELI_CAMERA_H

#include "game/game.h"
#include "game/camera/T402_automount_camera.h"     /* base AutomountCamera */

class CameraTargetInfo;                     /* T377 (pointers only) */
class CarProgress;                          /* T408 */
class CourseCameraRecord;                   /* T172 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */
class type_info;                            /* T548_tinfo.h */

/* vtable 0x0033EA80 (25 slots: [23] AutomountCamera's 0x00204208; [24]
 * new).  typeinfo __tf10HeliCamera 0x00204E58 (__si_type_info 0x004B2930,
 * base AutomountCamera).  Size 0x1C0 (CameraView +0x22C0, the next member
 * at +0x2480). */
class HeliCamera : public AutomountCamera {
public:
    HeliCamera();                           /* 0x00204B10 type 6 */
    virtual ~HeliCamera();                  /* [0] +0x0C 0x00204B50 */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x00204B78 setupOnRecord(target, -1, NULL) */
    /* AutomountCamera's, then progress along the record */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00204C70 */
    /* 2 while inZone; else by giveupState (1 / 2: the giveupMode timer; 0, 3:
     * progress out of 0..1 gives 2 for levels 0 / 3, else 1) */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00204D70 */
    virtual int  isEnableChanging();        /* [17] +0x94 0x00204E38 giveupTime < 1 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x00204E48 1 */
    /* Set up on record rec (NULL: none); lap -1: the target's own course
     * position, else its lap position + lap laps.  giveupState 3,
     * giveupMode 1800, giveupTime 180 frames. */
    virtual void setupOnRecord(CameraTargetInfo *target, int lap, CourseCameraRecord *rec); /* [24] +0xCC 0x00204BA8 */

    f32  getCoursePos(RaceOrganization *org, CameraTargetInfo *target);  /* 0x00204C18 */

    CourseCameraRecord *record;             /* 0x180 the record followed (NULL: none) */
    int                 lap;                /* 0x184 -1: use the target's course position */
    f32                 progress;           /* 0x188 0..1 along the record (0.5 at set up) */
};
EGCS_SIZE(HeliCamera, 0x1C0);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf10HeliCamera(void);               /* 0x00204E58 */
extern u32 __ti10HeliCamera[3];             /* 0x004B2930 its __si_type_info (only T404 uses it) */
}
#endif

#endif /* GAME_T404_HELI_CAMERA_H */
