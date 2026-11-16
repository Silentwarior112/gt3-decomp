/* T385 (0x0020A9C0..0x0020AB80): the course demo cameras of CameraManager -
 * view 0's designer camera steps through the demo records of the course's
 * camera file (CourseData +0x88 ->records2), one every 240 frames, asking
 * for a cross-fade (crossFade = 1, crossFadeTime = 2 s) at each change. */
/* Retail T385's .rodata (0x0033F088; nothing references it: DATA_START) is
 * <new>'s "bad_alloc" and the type names RaceCameraBase, UnitArena<64> (T153
 * read first): no GameStatus (garage names). */
#include <new>                                  /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO                /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"              /* RaceCourse (seen first: UnitArena<64>) */
#include "game/camera/camera_manager_internal.h"
#include "game/camera/T383_camera_manager.h"           /* CameraManager, CameraView (RaceCameraBase) */
#include "game/camera/T385_camera_demo.h"              /* CourseCameraData (= CourseCamera) */
#include "game/camera/T172_course_camera.h"            /* CourseCamera (count2, records2) */
#include "game/course/T179_course_data.h"              /* CourseData (replay) */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization (course) */
// DATA_START 0x0033F088

#define DEMO_RECORD_FRAMES 240                  /* the frames on one demo record */
#define DEMO_CROSSFADE_TIME 2.0f                /* seconds */

// FUNC 0x0020A9C0 CameraManager::startDemoCameras
/* 0 when the course has no demo records; else resets the state, turns the
 * demo mode on and starts record demoIndex (0 after resetState).  (Matching:
 * the course data read before demoMode is set, the rest after it.) */
int CameraManager::startDemoCameras(RaceOrganization *org)
{
    DesignerCamera *camera = &views[0].designer;
    CameraTargetInfo *target = &views[0].targetInfo;
    RaceCourse *course = org->course;
    int index = demoIndex;                      /* (read before resetState clears it) */
    CourseData *data;

    if (course->data->replay->count2 <= 0)
        return 0;
    resetState();
    data = course->data;
    demoMode = 1;
    views[0].camera = camera;
    camera->setupOnRecord(target, -1, course, &data->replay->records2[index], index);
    camera->doppler = 0;
    return 1;
}

// FUNC 0x0020AA90 CameraManager::updateDemoCameras
/* Run by update() instead of the views while the demo mode is on (dc is
 * not used). */
void CameraManager::updateDemoCameras(RaceOrganization *org, DynamicsConductor *dc)
{
    RaceInput *input = views[0].input;
    DesignerCamera *camera = &views[0].designer;
    RaceCourse *course = org->course;
    CameraTargetInfo *target = &views[0].targetInfo;
    CourseCameraData *courseCams = course->data->replay;
    int count = courseCams->count2;
    int timer = demoTimer + 1;
    int index = demoIndex;

    demoTimer = timer;
    if (timer >= DEMO_RECORD_FRAMES + 1) {
        demoTimer = 0;
        index = index + 1;
        if (index >= count)
            index = 0;
        demoIndex = index;
        crossFade = 1;
        crossFadeTime = DEMO_CROSSFADE_TIME;
        views[0].camera = camera;
        camera->setupOnRecord(target, -1, course, &courseCams->records2[index], index);
        camera->doppler = 0;
    }
    camera->updateFrame(org, input, target, &progress);
}
