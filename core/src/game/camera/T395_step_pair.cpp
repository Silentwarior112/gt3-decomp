/* T395 - StepPair (? name), 0x0020ED28..0x0020EDE8: the colour correction's
 * lo / hi of CameraManager (+0x20), starting at the course's values;
 * F_0020ED50 steps the first one towards its goal.
 *
 * Retail T395's .rodata (0x0033F5C8, nothing references it: DATA_START) has
 * <new>'s "bad_alloc" and UnitArena<64>'s type name only (T153: the course,
 * not the camera classes).  Of the files between T392 and T410 (T393..T396)
 * one has no .rodata; the course environment (CourseEnv, T178) makes T395 the
 * one that saw the race course, T396 the one with nothing. */
// DATA_START 0x0033F5C8
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"
#include "game/camera/camera_base_internal.h"
#include "game/camera/T395_step_pair.h"
#include "game/course/T178_course_env.h"           /* CourseEnv (correctionLevel) */

/* Caller: CameraManager::init (T383), with the course's CourseEnv.  The
 * values and the goals from env's correction levels. */
// FUNC 0x0020ED28 F_0020ED28
void F_0020ED28(StepPair *p, CourseEnv *env)
{
    p->unk_10 = 0;
    p->unk_14 = 0;
    p->unk_18 = 0;
    p->goal[0] = p->cur[0] = env->correctionLevel[0];
    p->goal[1] = p->cur[1] = env->correctionLevel[1];
}

/* No callers.  cur[0] one step of 0.01 towards goal[0] (not past it), then
 * at most 0.3; unk_10 = x, unk_18 = 1. */
// FUNC 0x0020ED50 F_0020ED50
void F_0020ED50(StepPair *p, f32 x)
{
    f32 goal = p->goal[0];

    if (p->cur[0] < goal) {
        p->cur[0] = p->cur[0] + 0.01f;
        if (goal <= p->cur[0])
            p->cur[0] = goal;
    }
    if (goal < p->cur[0]) {
        p->cur[0] = p->cur[0] - 0.01f;
        if (p->cur[0] <= goal)
            p->cur[0] = goal;
    }
    if (p->cur[0] > 0.3f)
        p->cur[0] = 0.3f;
    p->unk_10 = x;
    p->unk_18 = 1;
}
