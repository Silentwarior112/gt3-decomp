/* T380_camera_spring.h - CameraSpring (translation unit T380, unit u091),
 * 0x00206A80..0x00206B30.
 *
 * A damped spring driven by the change of its input, 60 steps a second
 * (OnBoardCamera T378, ZoomupCamera T406).
 *
 * Also here: the two small dampers of translation unit T407 (u091,
 * 0x00206B30..0x00206CC0), which has no canonical header of its own:
 * CameraFollow (DesignerCamera's zoom) and CameraBank (DesignerCamera's bank
 * animation), held by value by DesignerCamera (T398).
 */
#ifndef GAME_T380_CAMERA_SPRING_H
#define GAME_T380_CAMERA_SPRING_H

#include "game/game.h"

/* ? name.  vel += (in - prev - 12 vel) / 60; pos += vel / 60.  0xC bytes. */
class CameraSpring {
public:
    f32  update(f32 in);                    /* 0x00206A80 one step; returns pos */
    void reset(f32 pos, f32 prev);          /* 0x00206AE0 vel = 0 */
    f32  clampedPos();                      /* 0x00206AF0 pos clamped to -1..1 */

    f32 vel;                                /* 0x0 */
    f32 pos;                                /* 0x4 */
    f32 prev;                               /* 0x8 the last input */
};
EGCS_SIZE(CameraSpring, 0xC);

/* ? name (u090: ZoomDamper).  T407: chases a target, faster (x gain) while
 * it is above; vel limited to -30..10.  0xC bytes. */
class CameraFollow {
public:
    /* x = target = v, vel = 0.  gain (as update's: DesignerCamera passes
     * its zoom gain in $f13) is not read. */
    void reset(f32 v, f32 gain);            /* 0x00206B30 */
    /* one step towards target; returns the new x (the callers use it) */
    f32  update(f32 target, f32 gain);      /* 0x00206B40 */

    f32 x;                                  /* 0x0 */
    f32 target;                             /* 0x4 */
    f32 vel;                                /* 0x8 -30..10 */
};
EGCS_SIZE(CameraFollow, 0xC);

/* ? name (u090: BankAnim).  T407: an angle sprung back to 0, vel limited to
 * -900..900.  8 bytes. */
class CameraBank {
public:
    void reset(f32 a);                      /* 0x00206BE8 angle = a, vel = 0 */
    /* one step; returns the new angle (the callers use it) */
    f32  update();                          /* 0x00206C28 */

    f32 angle;                              /* 0x0 */
    f32 vel;                                /* 0x4 */
};
EGCS_SIZE(CameraBank, 0x8);

#endif /* GAME_T380_CAMERA_SPRING_H */
