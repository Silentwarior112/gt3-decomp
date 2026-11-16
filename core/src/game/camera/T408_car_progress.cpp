/* T408: CarProgress (game/camera/T408_car_progress.h; the CameraManager's member
 * at +0x278) - the lap distance of every car this
 * frame and the last, and its lap within the course's camera file, for the
 * replay camera director.
 *
 * Retail T408's .rodata: <new>'s "bad_alloc" (0x0033EE28, nothing references
 * it: DATA_START), 14RaceCameraBase, t9UnitArena1Ui64 (see T407's head). */
// DATA_START 0x0033EE28
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/T408_car_progress.h"
#include "game/camera/camera_motion_internal.h"

/* The lap of the camera file a car is on: (lap - 1) % laps, 0 before the
 * start (the car's lap is 1 on the first lap). */
static inline int car_camera_lap(RaceCarState *car, int laps)
{
    int lap = car->st.lap;
    if (lap > 0)
        lap = (lap - 1) % laps;
    else
        lap = 0;
    return lap;
}

// FUNC 0x00206F10 CarProgress::init
void CarProgress::init(RaceOrganization *org)
{
    int i, laps, count;
    RaceCarState *car;

    length = 1.0f;
    ready = 0;
    frames = 0;
    for (i = 0; i < 7; i++) {
        cars[i].prev = 0.0f;
        cars[i].cur = 0.0f;
        cars[i].lap = 0;
    }
    if (org == 0)
        return;
    laps = org->course->getReplayLoopCount();  /* RaceCourse 0x0018FD40 */
    count = (int)org->entries->count;       /* compared signed here */
    length = org->course->getLapLength();   /* RaceCourse 0x0018FA10 */
    for (i = 0; i < count; i++) {
        car = org->entries->cars[i]->state;
        cars[i].lap = car_camera_lap(car, laps);
        cars[i].cur = car_lap_distance(car, 0.0f);
    }
}

// FUNC 0x00207050 CarProgress::update
void CarProgress::update(RaceOrganization *org)
{
    int i, laps, count;
    RaceCarState *car;

    laps = org->course->getReplayLoopCount();  /* RaceCourse 0x0018FD40 */
    count = (int)org->entries->count;       /* compared signed here */
    for (i = 0; i < count; i++) {
        car = org->entries->cars[i]->state;
        cars[i].lap = car_camera_lap(car, laps);
        cars[i].prev = cars[i].cur;
        cars[i].cur = car_lap_distance(car, 0.0f);
    }
    frames = frames + 1;
    if (frames >= 2)
        ready = 1;
}

// FUNC 0x00207178 CarProgress::delta
//   Unwrapped across the start line: a jump of more than a third of the lap
//   is taken as the line.
f32 CarProgress::delta(int car)
{
    f32 len, d;

    if (ready == 0)
        return 1.0f;
    len = length;
    d = cars[car].cur - cars[car].prev;
    if (len / 3.0f < d)
        d = -(len - d);
    if (d < -len / 3.0f)
        d = d + len;
    return d;
}
