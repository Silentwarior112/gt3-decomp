/* T400_starting_camera.h - StartingCamera (translation unit T400, unit
 * u090), 0x00202F60..0x002031D0.
 *
 * The camera of the race start: PasslineCamera (T399) on start.cam's first
 * record, its timeRange running from 0 to 1 while the start countdown
 * (RaceOrganization::countDown, +0x5C) goes from 660 down to 60 frames, the
 * resulting matrix then turned and moved by the target's angles and position.
 */
#ifndef GAME_T400_STARTING_CAMERA_H
#define GAME_T400_STARTING_CAMERA_H

#include "game/game.h"
#include "game/camera/T399_passline_camera.h"      /* base PasslineCamera */

class CameraTargetInfo;                     /* T377 (pointers only) */
class CarProgress;                          /* T408 */
class RaceCourse;                           /* T153 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */

/* vtable 0x0033E628 (24 slots; the others PasslineCamera's).  typeinfo
 * __tf14StartingCamera 0x00203180 (__si_type_info 0x004B28F0, base
 * PasslineCamera).  No data of its own; size 0x140 (CameraView +0x1A80). */
class StartingCamera : public PasslineCamera {
public:
    StartingCamera();                       /* 0x00202F60 type 3 */
    virtual ~StartingCamera();              /* [0] +0x0C 0x00202FA0 */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00203028 */
    virtual int  getMount();                /* [13] +0x74 0x00203178 -1 */
    virtual void setup(CameraTargetInfo *target, RaceCourse *course);          /* [23] +0xC4 0x00202FC8 PasslineCamera's set up, timeRange kept */
};
EGCS_SIZE(StartingCamera, 0x140);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tf14StartingCamera(void);   /* 0x00203180 */
/* Its __si_type_info, which it fills. */
extern u32 __ti14StartingCamera[3];         /* 0x004B28F0 */
}
#endif

#endif /* GAME_T400_STARTING_CAMERA_H */
