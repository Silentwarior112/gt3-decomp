/* T387_camera_start.cpp - the camera of a live race (0x0020B788..0x0020B8E8).
 * CameraManager::update (0x00208BE8) calls it instead of the view kind's
 * handler while the view's input is live (no input, or RaceInput::replayMode
 * 0) outside the logger: the starting camera during the start countdown, the
 * onboard camera from its last second on.  Declared in T383_camera_manager.h
 * (T387 has no header of its own). */
/* Retail T387's .rodata (0x0033F1A8): <new>'s "bad_alloc", the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (nothing references them:
 * DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager::updateStart */
// DATA_START 0x0033F1A8

// FUNC 0x0020B788 CameraManager::updateStart
//   From the last second of the countdown on (and through the race) the
//   onboard camera (a change of its mount counts as a camera change); before
//   that the starting camera, with a 1 s cross-fade at 3, 2 and 1 s left of
//   the countdown.  Returns 1 when the camera changed.  dc is not used.
int CameraManager::updateStart(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player)
{
    int framesLeft;
    int changed;

    if (input != NULL)
        input->getButtonsOn();              /* result unused */
    framesLeft = org->countDown - 60;
    if (framesLeft < 0) {
        CameraView *view = &views[player];
        OnBoardCamera *onboard = &view->onboard;
        int mount = onboard->getMount();
        changed = F_002096C0(player, org, input, 0);
        if (onboard->getMount() != mount)
            changed = 1;
    } else {
        changed = F_00209660(player, org, input);
        if (framesLeft == 0 || framesLeft == 60 || framesLeft == 120) {
            crossFade = 1;
            crossFadeTime = 1.0f;
        }
    }
    {
        CameraView *view = &views[player];
        if (changed)
            F_0020EE50(&view->request, views[player].camera->type);
    }
    return changed;
}
