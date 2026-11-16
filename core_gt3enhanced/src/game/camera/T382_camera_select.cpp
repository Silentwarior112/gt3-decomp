/* T382: the pitch / yaw (degrees) of a direction, and the pickers of a
 * course camera record (T172) for a course position: at the target's
 * position, on a later lap, or for a car of the CarProgress (T408) that will
 * stay in it long enough.
 *
 * Retail T382's .rodata: <new>'s "bad_alloc" (0x0033EE68, nothing references
 * it: DATA_START), 14RaceCameraBase, t9UnitArena1Ui64 (see T407's head);
 * T383's "bad_alloc" follows at 0x0033EEA8. */
// DATA_START 0x0033EE68
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/T382_camera_select.h"
#include "game/camera/T408_car_progress.h"        /* CarProgress */
#include "game/camera/camera_motion_internal.h"

#define RECORD_LIST_MAX 32                  /* the records collected at a position */

// FUNC 0x00207208 camera_pitch_deg
f32 camera_pitch_deg(f32 x, f32 y, f32 z)
{
    return -vu0_atan2f(y, boot_sqrtf_v(x * x + z * z)) * U091_RAD_TO_DEG;
}

// FUNC 0x00207248 camera_yaw_deg
f32 camera_yaw_deg(f32 x, f32 y, f32 z)
{
    return (-vu0_atan2f(x, z) + U091_PI) * U091_RAD_TO_DEG;
}

// FUNC 0x00207278 camera_record_in_range
int camera_record_in_range(CourseCameraRecord *rec, f32 pos, f32 lo, f32 hi)
{
    f32 t = (pos - rec->start) / (rec->end - rec->start);
    int inside = 0;
    if (lo <= t && t <= hi)
        inside = 1;
    return inside;
}

// FUNC 0x002072C0 camera_find_record
CourseCameraRecord *camera_find_record(RaceOrganization *org, CameraTargetInfo *target, int wide)
{
    CourseCameraRecord *list[RECORD_LIST_MAX];
    RaceCourse *course = org->course;
    f32 hi = 0.6f;
    f32 pos;
    int n, i;

    if (wide)
        hi = 1.0f;
    pos = target->distance;
    n = course->collectReplayCameras(list, RECORD_LIST_MAX, pos);
    for (i = 0; i < n; i++) {
        if (camera_record_in_range(list[i], pos, 0.0f, hi))
            return list[i];
    }
    return 0;
}

// FUNC 0x00207370 camera_find_next_record
CourseCameraRecord *camera_find_next_record(RaceOrganization *org, CameraTargetInfo *target, int *lap)
{
    CourseCameraRecord *list[RECORD_LIST_MAX];
    RaceCourse *course = org->course;
    int laps, l, tries, i, n;
    f32 pos;

    laps = course->getReplayLoopCount();
    *lap = -1;
    l = target->lap + 1;
    for (tries = 0; tries < laps; tries++, l++) {
        if (laps < l)
            l = 1;
        pos = (f32)l * course->getLapLength() + target->lapDistance;
        n = course->collectReplayCameras(list, RECORD_LIST_MAX, pos);
        for (i = 0; i < n; i++) {
            if (camera_record_in_range(list[i], pos, 0.0f, 0.6f)) {
                *lap = l;
                return list[i];
            }
        }
    }
    return 0;
}

// FUNC 0x002074A0 camera_find_next_record_for
//   The time the car stays in a record is counted in frames: the distance to
//   the record's end (start when going backwards) over the car's distance a
//   frame (at least 0.05 m), against minTime x 60.
CourseCameraRecord *camera_find_next_record_for(RaceOrganization *org, CarProgress *prog, int car,
                                                CourseCameraRecord *cur, int *lap, f32 minTime)
{
    CourseCameraRecord *list[RECORD_LIST_MAX];
    CourseCameraRecord *rec;
    RaceCourse *course;
    f32 curX, curZ, pos, step, frames, dx, dz;
    int laps, l, tries, i, n, ok;

    curZ = 0.0f;
    course = org->course;
    curX = 0.0f;
    laps = course->getReplayLoopCount();
    *lap = 0;
    if (cur != 0) {
        curX = interpolate_table(cur->pos[0], 0.0f);
        curZ = interpolate_table(cur->pos[2], 0.0f);
    }
    minTime = minTime * U091_FPS;      /* now in frames */
    l = prog->cars[car].lap + 1;
    for (tries = 0; tries < laps; tries++, l++) {
        if (laps < l)
            l = 1;
        pos = (f32)l * course->getLapLength() + prog->cars[car].cur;
        step = prog->delta(car);
        n = course->collectReplayCameras(list, RECORD_LIST_MAX, pos);
        for (i = 0; i < n; i++) {
            rec = list[i];
            if (rec == cur)
                continue;
            ok = camera_record_in_range(rec, pos, 0.0f, 0.8f) != 0;
            /* (the clamped step stays for the next records) */
            if (step < 0.0f) {
                if (-0.05f < step)
                    step = -0.05f;
                frames = -(pos - list[i]->start);
            } else {
                if (step < 0.05f)
                    step = 0.05f;
                frames = list[i]->end - pos;
            }
            frames = frames / step;
            if (frames < 0.01f)
                frames = 0.01f;
            if (frames < minTime)
                ok = 0;
            if (cur != 0) {
                /* the cameras at the records' starts, 15 m apart at least */
                dx = interpolate_table(list[i]->pos[0], 0.0f) - curX;
                dz = interpolate_table(list[i]->pos[2], 0.0f) - curZ;
                if (dx * dx + dz * dz < 225.0f)
                    ok = 0;
            }
            if (ok) {
                *lap = l;
                return list[i];
            }
        }
    }
    return 0;
}
