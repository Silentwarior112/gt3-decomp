/* T391_camera_logger.cpp - the camera mode handler of the data logger
 * (0x0020D208..0x0020D5E8): CameraManager::update (0x00208BE8) gives every
 * view kind 5 and calls it while RaceOrganization::loggerMode is set.
 * Declared in T383_camera_manager.h (T391 has no header of its own). */
/* Retail T391's .rodata (0x0033F3E0): <new>'s "bad_alloc", the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (nothing references them:
 * DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager::updateLogger */
// DATA_START 0x0033F3E0

// FUNC 0x0020D208 CameraManager::updateLogger
//   The target follows the up / down buttons, forcedTarget (set by the
//   logger, F_0020A060) or a re-target request; cross cycles the view:
//   designer, onboard, logger camera.  Returns 1 when the camera changed.
//   dc is not used.
int CameraManager::updateLogger(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player)
{
    int changed = 0;                        /* fp */
    int forceChange = 0;                    /* s5 */
    int retarget = targetReset[player] != 0;    /* s1 */
    int targetChanged = 0;                  /* sp+0x00 */
    int manual = 0;                         /* sp+0x04 */
    int newTarget = -1;                     /* sp+0x08 */
    int counting = org->countDown > 0;      /* sp+0x0C */
    u32 buttons = 0;                        /* sp+0x10 */
    CameraView *view;
    CameraTargetSelector *selector;
    CameraTargetInfo *target;

    if (input != NULL)
        buttons = input->getButtonsOn();
    targetReset[player] = 0;
    {
        RaceEntryBase *entries = org->entries;
        RaceEntryCar *next = NULL;
        u32 dir = buttons & CAMBTN_UP_DOWN;
        RaceEntryCar *car;
        int forced;

        view = &views[player];
        target = &view->targetInfo;
        selector = &view->selector;
        car = entries->cars[target->index[0]];
        switch (dir) {
        case CAMBTN_UP:
            next = prevWatchableByRank(entries, car);
            break;
        case CAMBTN_DOWN:
            next = nextWatchableByRank(entries, car);
            break;
        }
        forced = forcedTarget;
        if (forced >= 0) {
            forcedTarget = -1;
            next = org->entries->cars[forced];
        }

        if (next == NULL) {
            if (!retarget)
                goto run_camera;
        } else if (target->car[0] != next) {
            targetChanged = 1;
            forceChange = 1;
            newTarget = next->number;
            manual = 1;
            if (kinds[player].getView() == VIEW_DESIGNER)
                forceChange = 0;
            if (counting)
                forceChange = 1;
        }
    }

    if (retarget) {
        forceChange = 1;
        newTarget = target->index[0];
    }
    if (newTarget >= 0) {
        target->setCar(newTarget, 15);
        if (manual)
            target->wide = 1;
        selector->holdTimer[newTarget] = 15;
        target->refresh();
        forceChange = 1;
    }

run_camera:
    if (buttons & CAMBTN_CROSS) {
        kinds[player].view = kinds[player].view + 1;
        if (!(kinds[player].view < 3))
            kinds[player].view = VIEW_DESIGNER;
        forceChange = 1;
    }
    switch (kinds[player].view) {
    case VIEW_LOGGER:
        changed = F_002096A0(player, org, input);
        break;
    case VIEW_ONBOARD: {
        CameraView *v = &views[player];
        OnBoardCamera *onboard = &v->onboard;
        int mount = onboard->getMount();
        changed = F_002096C0(player, org, input, forceChange);
        onboard->findMountDown(mount, 8, 0);
        break;
    }
    case VIEW_DESIGNER: {
        int flags = DESIGNER_FADE;
        if (forceChange || retarget)
            flags = DESIGNER_FADE | DESIGNER_RESET;
        changed = updateDesigner(player, org, input, flags);
        break;
    }
    }
    if (forceChange)
        changed = 1;

    if (changed) {
        VariableCameraBase *cam;
        target->finishBlend();
        cam = views[player].camera;
        if (cam->trueType == CAMTYPE_DESIGNER) {
            if (cam->type == CAMTYPE_ONBOARD)
                F_00209808(1, &views[player].designer);
        }
    }
    {
        CameraView *v = &views[player];
        if (targetChanged)
            F_0020EE28(&v->request, newTarget, -1);
        if (changed)
            F_0020EE50(&v->request, views[player].camera->type);
    }
    return changed;
}
