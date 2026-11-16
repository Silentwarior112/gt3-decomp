/* T401_automatic_camera.h - AutomaticCamera (translation unit T401, unit
 * u090), 0x002031D0..0x00204030.
 *
 * A random still camera, placed once by setup and not moved by
 * the update: either some way ahead of the car on its course of the moment
 * (placeAhead) or beside it, ahead of where it will be (placeBeside), dropped
 * onto the road plane with a random height; when no road plane is found, a
 * camera at the car looking along its motion.  The field of view is a random
 * 55..85 degrees; the generator is seeded from the clock at each set up.
 */
#ifndef GAME_T401_AUTOMATIC_CAMERA_H
#define GAME_T401_AUTOMATIC_CAMERA_H

#include "game/game.h"
#include "game/camera/T397_variable_camera_base.h" /* base VariableCameraBase */
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */

class CameraProjection;                     /* T383 (pointers only) */
class CameraTargetInfo;                     /* T377 */
class CarProgress;                          /* T408 */
class RaceCarState;                         /* T302 */
class RaceCourse;                           /* T153 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */

/* vtable 0x0033E730 (24 slots; [23] new).  typeinfo __tf15AutomaticCamera
 * 0x00203FE0 (__si_type_info 0x004B2900, base VariableCameraBase).  Size
 * 0x140 (CameraView +0x1BC0). */
class AutomaticCamera : public VariableCameraBase {
public:
    AutomaticCamera();                      /* 0x00203200 type 4 */
    virtual ~AutomaticCamera();             /* [0] +0x0C 0x00203250 */
    virtual void applyView();               /* [2] +0x1C 0x00203F78 the current matrix times `matrix' */
    virtual void virtual_8(CameraTargetInfo *target);       /* [8] +0x4C 0x00203278 */
    virtual int  virtual_9();               /* [9] +0x54 0x00203F10 3 */
    virtual int  getEffect();              /* [10] +0x5C 0x00203F18 effect */
    virtual void setProjection(const CameraProjection *proj);  /* [11] +0x64 0x00203F20 the projection scaled by zoom */
    virtual int  update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress);  /* [12] +0x6C 0x00203EF0 */
    virtual int  getMount();                /* [13] +0x74 0x00203FD8 -1 */
    virtual int  isTargetDistEnable();      /* [14] +0x7C 0x00203FD0 0 */
    virtual int  getChangeLevel(RaceOrganization &org, int viewMode);              /* [15] +0x84 0x00203FC8 0 */
    virtual int  isNeedReset(RaceOrganization &org, CameraTargetInfo &target);  /* [16] +0x8C 0x00203FC0 0 */
    virtual int  isEnableChanging();        /* [17] +0x94 0x00203FB8 1 */
    virtual int  isEnableTargetChange();    /* [18] +0x9C 0x00203FB0 1 */
    virtual int  isDrawBackMirror();        /* [20] +0xAC 0x00203FA8 0 */
    virtual f32  getFadeOutTime();          /* [21] +0xB4 0x00203F98 0 */
    /* Set up (course NULL: an identity view); `ahead' scales how far ahead
     * of the car placeAhead goes. */
    virtual void setup(CameraTargetInfo *target, RaceCourse *course, f32 ahead);       /* [23] +0xC4 0x002032A8 */

    /* 1: placed (the road plane was found) */
    int  placeAhead(RaceCarState *car, RaceCourse *course, f32 ahead);     /* 0x00203528 */
    int  placeBeside(RaceCarState *car, RaceCourse *course, f32 ahead);    /* 0x00203958 */

    RaceCourse *placeCourse;                /* 0x100 the course given at set up (NULL: none) */
    Random      rng;                        /* 0x104 seeded from the clock (gfx_perf_counter0) at set up */
};
EGCS_SIZE(AutomaticCamera, 0x140);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tf15AutomaticCamera(void);   /* 0x00203FE0 */
/* Its __si_type_info, which it fills. */
extern u32 __ti15AutomaticCamera[3];        /* 0x004B2900 */
}
#endif

#endif /* GAME_T401_AUTOMATIC_CAMERA_H */
