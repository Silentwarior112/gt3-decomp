/* T404: HeliCamera - an AutomountCamera that follows its car along a course
 * camera record and reports how far along the record the car is.
 *
 * Retail T404's .rodata opens with <new>'s "bad_alloc" (0x0033EA70, nothing
 * references it: DATA_START), then the vtable and the type names
 * 10HeliCamera, 14RaceCameraBase, t9UnitArena1Ui64 (as T401..T403). */
// DATA_START 0x0033EA70
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/T404_heli_camera.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00204B10 HeliCamera::HeliCamera
//   (after AutomountCamera::AutomountCamera)
HeliCamera::HeliCamera()
{
    /* implicit: AutomountCamera (compiler-generated: bases, members, deleting form) */
    setType(CAMERA_TYPE_HELI);              /* inline */
}

// FUNC 0x00204B50 HeliCamera::~HeliCamera
//   Sets the vptr and calls AutomountCamera::~AutomountCamera with the flags
//   unchanged.
HeliCamera::~HeliCamera()
{
    /* implicit: ~AutomountCamera (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00204B78 HeliCamera::virtual_8
//   Set up on target: no record, the target's own course position (a
//   virtual call of slot 24).
void HeliCamera::virtual_8(CameraTargetInfo *target)
{
    setupOnRecord(target, -1, 0);
}

// FUNC 0x00204BA8 HeliCamera::setupOnRecord
void HeliCamera::setupOnRecord(CameraTargetInfo *target, int lap, CourseCameraRecord *rec)
{
    AutomountCamera::setup(target, 0.0f);
    this->lap = lap;
    record = rec;
    giveupState = GIVEUP_RANGE_OR_TIME;
    giveupMode = 1800;                      /* 30 s */
    giveupTime = 180;                       /* 3 s */
    progress = 0.5f;
}

// FUNC 0x00204C18 HeliCamera::getCoursePos
//   The target's course position: on lap `lap' of the course when one was
//   given, else its own (counted over the laps).
f32 HeliCamera::getCoursePos(RaceOrganization *org, CameraTargetInfo *target)
{
    /* The lap distance is read before the call (retail keeps it in $f20). */
    if (lap < 0)
        return target->distance;
    f32 pos = target->lapDistance;
    return pos + (f32)lap * org->course->getLapLength();   /* RaceCourse 0x0018FA10 */
}

// FUNC 0x00204C70 HeliCamera::update
int HeliCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target, CarProgress *progress)
{
    int result = AutomountCamera::update(org, input, target, progress);
    if (record != 0) {
        f32 pos = getCoursePos(org, target);
        this->progress = (pos - record->start) / (record->end - record->start);
    }
    return result;
}

// FUNC 0x00204CF8 heli_range_level
//   CHANGE_LEVEL_NONE while t is in 0..1; outside, CHANGE_LEVEL_NOW for view
//   modes 0 / 3, CHANGE_LEVEL_SOFT for the others.
static int heli_range_level(int viewMode, f32 t)
{
    int level = CHANGE_LEVEL_NONE;
    if (viewMode == 0 || viewMode == 3) {
        if (t < 0.0f)
            level = CHANGE_LEVEL_NOW;
        else if (1.0f < t)
            level = CHANGE_LEVEL_NOW;
    } else {
        if (t < 0.0f)
            level = CHANGE_LEVEL_SOFT;
        else if (1.0f < t)
            level = CHANGE_LEVEL_SOFT;
    }
    return level;
}

// FUNC 0x00204D70 HeliCamera::getChangeLevel
//   level: the view mode of the player (CameraManager::canCut passes
//   playerMode[player].mode).
int HeliCamera::getChangeLevel(RaceOrganization &org, int level)
{
    /* One result variable ($a1 from the prologue on); retail keeps the 2 of
     * the case test in $s1 and reuses it for GIVEUP_RANGE_OR_TIME's result. */
    int r = CHANGE_LEVEL_NONE;
    if (inZone != 0)
        return CHANGE_LEVEL_NOW;
    switch (giveupState) {
    case GIVEUP_ON_RANGE:
        r = heli_range_level(level, progress);
        break;
    case GIVEUP_TIME_THEN_RANGE:
        if (giveupMode == 0)
            r = heli_range_level(level, progress);
        break;
    case GIVEUP_ON_TIME:
        if (giveupMode != 0)
            r = CHANGE_LEVEL_NONE;
        else
            r = CHANGE_LEVEL_NOW;
        break;
    case GIVEUP_RANGE_OR_TIME:
        /* the call is made whatever the timer */
        r = heli_range_level(level, progress);
        if (giveupMode == 0)
            r = CHANGE_LEVEL_NOW;
        break;
    }
    return r;
}

// FUNC 0x00204E38 HeliCamera::isEnableChanging
int HeliCamera::isEnableChanging()
{
    return giveupTime < 1;
}

// FUNC 0x00204E48 HeliCamera::getFadeOutTime
f32 HeliCamera::getFadeOutTime()
{
    return 1.0f;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __si_type_info
 * __ti10HeliCamera (0x004B2930, name 0x0033EB50 "10HeliCamera", base
 * __ti15AutomountCamera 0x004B2910) on the first call. */
// FUNC 0x00204E58 __tf10HeliCamera
extern "C" void *__tf10HeliCamera(void)
{
    if (__ti10HeliCamera[0] == 0) {
        __tf15AutomountCamera();
        __rtti_si(__ti10HeliCamera, "10HeliCamera", (const type_info *)__ti15AutomountCamera);
    }
    return __ti10HeliCamera;
}
#endif
