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

/* ------------------------------------------------------------------------ */
/* The enhanced engine: the zoom button's spring (Lupo 2, cameras_sound.md  */
/* C5; T395_step_pair.h), after GT3's functions.                           */
/* ------------------------------------------------------------------------ */

#define ZOOM_SPRING_K       40.0f           /* the pull per unit off the target */
#define ZOOM_SPRING_DAMP    (-10.0f)        /* x the speed */
#define ZOOM_SPRING_STEPS   60.0f           /* steps a second */
#define ZOOM_SPRING_SPEED   900.0f          /* the speed's limit */
#define ZOOM_SPRING_RELEASE 0.8f            /* the value's factor a step at / above the target */

/* Lupo 2 L2 0x002154E8 (? name).  Callers: CameraManager::init (each view). */
void CameraZoomSpring::reset()
{
    value = 0.0f;
    target = 0.0f;
    speed = 0.0f;
    update();
}

/* Lupo 2 L2 0x00215510 (? name): one step of the spring while the value is
 * below the target, else the quick release (x 0.8, not below the target);
 * the speed is kept across a release.  Callers: CameraManager::update (each
 * view, every frame). */
f32 CameraZoomSpring::update()
{
    f32 v = value;

    if (v < target) {
        f32 s;

        speed = speed + ((target - v) * ZOOM_SPRING_K + speed * ZOOM_SPRING_DAMP) / ZOOM_SPRING_STEPS;
        if (ZOOM_SPRING_SPEED < speed)
            speed = ZOOM_SPRING_SPEED;
        s = speed;
        if (s < -ZOOM_SPRING_SPEED) {
            speed = -ZOOM_SPRING_SPEED;
            s = -ZOOM_SPRING_SPEED;
        }
        value = v + s / ZOOM_SPRING_STEPS;
    } else {
        value = v * ZOOM_SPRING_RELEASE;
        if (value < target)
            value = target;
    }
    return value;
}
