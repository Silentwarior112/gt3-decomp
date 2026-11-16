/* T409_camera_change.cpp - CameraManager helpers of the mode handlers
 * (0x0020AB80..0x0020B1D8): the cross-fade into a camera, the "may the camera
 * cut now?" test and the designer camera's move over the course camera
 * records.  The three methods are declared in T383_camera_manager.h (T409
 * has no header of its own). */
/* Retail T409's .rodata (0x0033F0C8): <new>'s "bad_alloc", the two "TEST"
 * literals, then the type names "14RaceCameraBase", "t9UnitArena1Ui64"
 * (reverse definition order: T153 is read before RaceCameraBase's header).
 * Nothing references "bad_alloc": DATA_START. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager (T409's methods) */
// DATA_START 0x0033F0C8

// FUNC 0x0020AB80 CameraManager::setCrossFadeFrom
//   A cross-fade over the camera's fade-out time, at most 4 s (none when the
//   time is 0).
void CameraManager::setCrossFadeFrom(VariableCameraBase *cam)
{
    f32 time = cam->getFadeOutTime();

    if (4.0f < time)
        time = 4.0f;
    if (0.0f < time) {
        crossFadeTime = time;
        crossFade = 1;
    }
}

// FUNC 0x0020ABF0 CameraManager::canCut
//   Whether the view's camera may change now, by its change level (slot 15;
//   DesignerCamera: 0 stay, 1 may change, 2 change): level 2 always; else
//   only while a BGM cut is requested or the next marker is 1.3 s or more
//   away.  strict (updateDesigner, updateMode0): only level 1 gets that test,
//   level 0 never cuts.  In the logger: only when the camera left its range.
int CameraManager::canCut(int player, RaceOrganization *org, int strict)
{
    VariableCameraBase *cam = views[player].camera;
    int requested = changeRequest > 0;
    f32 markerTime = nextMarkerTime;
    int level = cam->getChangeLevel(*org, kinds[player].getKind());
    int ret;

    if (org->loggerMode != 0) {
        ret = views[player].camera->isRangeOver() != 0;
    } else if (strict) {
        if (level != 1) {
            if (level == 2)
                ret = 1;
            else
                ret = 0;
        } else if (markerTime < 1.3f) {
            ret = 0;
            if (requested)
                ret = 1;
        } else
            ret = 1;
    } else {
        /* !(t < 1.3) as written: the code tests c.lt.s and cuts on false
         * (the file is built without -ffast-math, which would turn it into
         * t >= 1.3, c.le.s) */
        if (level == 2 || !(markerTime < 1.3f))
            ret = 1;
        else
            ret = requested;
    }
    return ret;
}

// FUNC 0x0020AD18 CameraManager::updateDesigner
//   Runs the view's designer camera over the course camera records: it keeps
//   its record, or moves to the record at the target (or the next one on a
//   later lap) when it needs a reset, may cut (canCut) or the flags force it.
//   Without a record at all the onboard camera takes over (F_002096C0), or
//   the designer is forced onboard.  flags: DESIGNER_FORCE, DESIGNER_RESET,
//   any flag: cross-fade on a move.  Returns 1 when the camera changed (the
//   reset flag when it may not cut).
int CameraManager::updateDesigner(int player, RaceOrganization *org, RaceInput *input, int flags)
{
    CameraView *view = &views[player];
    DesignerCamera *designer = &view->designer;
    CameraTargetInfo *target = &view->targetInfo;
    RaceCourse *course;
    CourseCameraRecord *current;            /* s7 */
    int wide;                               /* sp+0xC (spill slots in declaration order) */
    int force;                              /* sp+0x10 */
    int wasOnboard;                         /* sp+0x14 the designer was forced onboard: no cross-fade into the onboard view */
    int reset;                              /* s1 */
    int lap;                                /* sp+0 */
    int nextLap;                            /* sp+4 */

    course = org->course;
    reset = flags >> 1;                     /* DESIGNER_RESET: the shift stays in reset's register */
    reset &= 1;
    current = designer->cameraData;
    force = flags & DESIGNER_FORCE;
    wasOnboard = 0;
    wide = target->wide;
    views[player].camera = designer;
    if (org->loggerMode != 0) {
        wide = 1;
        if (loggerSeek != 0) {
            loggerSeek = 0;
            force = 1;
        }
    }

    if (current == NULL) {
        CourseCameraRecord *rec = camera_find_record(org, target, 1);
        if (rec != NULL) {
            views[player].camera = designer;
            designer->setupOnRecord(target, -1, course, rec, camera_record_number(course, rec));
            return 1;
        }
        rec = camera_find_next_record(org, target, &lap);
        if (rec != NULL) {
            views[player].camera = designer;
            designer->setupOnRecord(target, lap, course, rec, camera_record_number(course, rec));
            org->eventQueue.post(RACE_EVENT_ERROR,
                                 (int)"TEST : starting camera : set OTHER forcibly !!!!", 0);
            return 1;
        }
        F_002096C0(player, org, input, 1);
        return 1;
    }

    if (force == 0) {
        if (designer->isNeedReset(*org, *target))
            reset = 1;
        wasOnboard = designer->type == CAMTYPE_ONBOARD;
        if (reset) {
            target->finishBlend();
            views[player].camera = designer;
            designer->setupOnRecord(target, designer->innerLapOffset, course, current,
                                 camera_record_number(course, current));
            designer->updateFrame(org, input, target, &progress);
            if (designer->isRangeOver()) {
                CourseCameraRecord *rec = camera_find_record(org, target, 1);
                if (rec != NULL) {
                    views[player].camera = designer;
                    designer->setupOnRecord(target, -1, course, rec, camera_record_number(course, rec));
                } else {
                    designer->setForceOnboard(target);
                }
                return 1;
            }
        }
        if (!canCut(player, org, 1))
            return reset;
    }

    CourseCameraRecord *rec = camera_find_record(org, target, wide);     /* s1, also the next record's */
    {
        int move = 0;
        if (wide)
            move = 1;
        if (force)
            move = 1;
        if (rec != current)
            move = 1;
        if (rec != NULL && move) {
            if (flags)
                setCrossFadeFrom(designer);
            views[player].camera = designer;
            designer->setupOnRecord(target, -1, course, rec, camera_record_number(course, rec));
            return 1;
        }
    }

    {
        rec = camera_find_next_record(org, target, &nextLap);
        if (rec == NULL || rec == current) {
            if (flags && !wasOnboard)
                setCrossFadeFrom(designer);
            designer->setForceOnboard(target);
            designer->setGiveup(2, 180);
            return 1;
        }
        if (flags)
            setCrossFadeFrom(designer);
        views[player].camera = designer;
        designer->setupOnRecord(target, nextLap, course, rec, camera_record_number(course, rec));
        designer->setGiveup(2, 180);
        org->eventQueue.post(RACE_EVENT_ERROR, (int)"TEST : set OTHER forcibly !!!!!!!!!", 0);
        return 1;
    }
}
