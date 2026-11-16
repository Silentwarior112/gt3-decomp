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

#endif /* GAME_T395_STEP_PAIR_H */
