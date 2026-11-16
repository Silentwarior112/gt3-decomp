/* T408_car_progress.h - CarProgress (translation unit T408, unit u091),
 * 0x00206F10..0x00207208.
 *
 * Its own header, included by T383_camera_manager.h where it defined it (its
 * includers see the same declarations in the same order).  The cameras of
 * u090 need only this: with the camera manager's header egcs writes
 * "14RaceCameraBase" into their .rodata (the type name of a class they do not
 * use), which retail T398..T403 do not have (T398: the type name right after
 * the 1.1 of update, 0x0033E4E0).
 */
#ifndef GAME_T408_CAR_PROGRESS_H
#define GAME_T408_CAR_PROGRESS_H

#include "game/game.h"

class RaceOrganization;                     /* T125 */

/* T408 (0x00206F10..0x00207208, u091): the lap distance
 * of every car this frame and the last, for the camera director.
 * CameraManager +0x278: init / update from CameraManager::init / update, and
 * every camera's update gets it (VariableCameraBase::updateFrame / slot 12;
 * T382's camera_find_next_record_for).  u092 called it CameraCommon, u093
 * CameraCourseSearch.  Not polymorphic.  Size 0x60 (u091; init's loop ends at
 * +0x60): in CameraManager +0x2D8..+0x2FF is the alignment of the views. */
class CarProgressEntry {
public:
    int lap;                                /* 0x0 (lap - 1) % the laps of the course's camera file */
    f32 prev;                               /* 0x4 the lap distance a frame ago */
    f32 cur;                                /* 0x8 the lap distance */
};
EGCS_SIZE(CarProgressEntry, 0xC);

class CarProgress {
public:
    void init(RaceOrganization *org);       /* 0x00206F10 (org may be NULL) */
    void update(RaceOrganization *org);     /* 0x00207050 once a frame */
    f32  delta(int car);                    /* 0x00207178 the distance car moved since the last frame; 1 before two updates */

    int              ready;                 /* 0x00 1 after two updates */
    int              frames;                /* 0x04 */
    f32              length;                /* 0x08 the lap length */
    CarProgressEntry cars[7];               /* 0x0C */
};
EGCS_SIZE(CarProgress, 0x60);

#endif /* GAME_T408_CAR_PROGRESS_H */
