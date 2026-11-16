/* T395_step_pair.h - StepPair (translation unit T395, unit u094),
 * 0x0020ED28..0x0020EDE8.
 *
 * ? name.  Two values that start at their goals; F_0020ED50 (no callers)
 * steps the first one by 0.01 towards its goal and caps it at 0.3.
 * CameraManager +0x20 (T383) holds one: the lo / hi of the course's colour
 * correction, from CourseEnv +0x98C / +0x990 (F_0020ED28 in
 * CameraManager::init); CameraManager::getColourCorrection hands cur[] to
 * RacePS2Base's colour correction pass (pfx_color_correction's lo / hi).
 */
#ifndef GAME_T395_STEP_PAIR_H
#define GAME_T395_STEP_PAIR_H

#include "game/game.h"

class CourseEnv;                            /* T178 (pointers only) */

/* Not polymorphic.  0x1C bytes of data (CameraManager +0x20; the next known
 * member is at +0x23C). */
class StepPair {
public:
    f32 cur[2];                             /* 0x00 */
    f32 goal[2];                            /* 0x08 */
    f32 unk_10;                             /* 0x10 the argument of F_0020ED50 */
    f32 unk_14;                             /* 0x14 (0 by F_0020ED28; f32: its 0 store keeps the
                                               course values' loads after it, u094 matching) */
    int unk_18;                             /* 0x18 1 once F_0020ED50 ran */
};

/* cur = goal = env's two values (+0x98C, +0x990, u094's StepPairSource),
 * unk_10 / unk_14 / unk_18 0. */
void F_0020ED28(StepPair *p, CourseEnv *env);   /* 0x0020ED28 */
void F_0020ED50(StepPair *p, f32 x);            /* 0x0020ED50 (no callers) */

/* The enhanced engine (Lupo 2, cameras_sound.md C5): the zoom button's
 * spring, one per view (CameraView::zoomSpring, T383; Lupo 2 view +0x24C4).
 * value chases target with a spring (40 / s^2 per unit, damping 10, the
 * speed within +-900, 60 steps a second) while it is below it, and drops by
 * 0.8 a step (down to target) once it is not.  CameraManager::init resets
 * it, CameraManager::update steps it every frame, updateMode0 (T388) sets
 * the target (20 while the zoom button is held, else 0) and turns the value
 * into the TV camera's zoom factor.  ? name (no symbol: Lupo 2's functions
 * sit before StepPair's, L2 0x002154E8..0x002155DC).  Not polymorphic,
 * 0xC bytes. */
class CameraZoomSpring {
public:
    void reset();                           /* L2 0x002154E8 all 0, then update() */
    f32  update();                          /* L2 0x00215510 one step; returns value */
    void setTarget(f32 t) { target = t; }   /* L2 0x002155D0 */
    f32  get() { return value; }            /* L2 0x002155D8 */

    f32 value;                              /* 0x0 */
    f32 target;                             /* 0x4 */
    f32 speed;                              /* 0x8 per second */
};

#endif /* GAME_T395_STEP_PAIR_H */
