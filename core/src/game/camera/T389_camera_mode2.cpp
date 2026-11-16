/* T389_camera_mode2.cpp - the camera mode handler of view kind 2
 * (0x0020BE58..0x0020C510).  Declared in T383_camera_manager.h (T389 has no
 * header of its own). */
/* Retail T389's .rodata (0x0033F228): <new>'s "bad_alloc", the two literals
 * of updateMode2, the type names "14RaceCameraBase", "t9UnitArena1Ui64".
 * Nothing references "bad_alloc": DATA_START. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager::updateMode2 */
// DATA_START 0x0033F228

// FUNC 0x0020BE58 CameraManager::updateMode2
//   Like updateMode0, but the target only goes to cars drawn in full
//   (nextWatchableEntry / isWatchable) and, with the camera target variant set (not
//   0: the leader / automatic), it comes from the target chooser (T393) when
//   the designer camera may change, the chooser has a new car, the selector
//   does not hold the current one and the camera may cut.  After a camera
//   change to a course camera (trueType 1) there is a 7% chance of a cut to
//   the designer's onboard view on a mount looking at the target's second
//   car ("TEST : Automatic ONBOARD").  Returns 1 when the camera changed.
//   dc is not used.
int CameraManager::updateMode2(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player)
{
    int secondCar = -1;                     /* fp */
    int forceChange = 0;                    /* s3 */
    int newTarget = -1;                     /* s7 */
    int targetChanged = 0;                  /* sp+0x08 (spill slots in declaration order) */
    int subChanged = 0;                     /* sp+0x0C */
    int forceRecord = 0;                    /* sp+0x10 updateDesigner's DESIGNER_FORCE */
    int retarget = targetReset[player] != 0;    /* sp+0x14 */
    int wanted = -1;                        /* sp+0x18 */
    int manual = 0;                         /* sp+0x1C */
    u32 buttons = 0;                        /* sp+0x20 */
    int changed;
    CameraView *view;
    CameraTargetInfo *target;
    CameraTargetSelector *selector;

    if (input != NULL)
        buttons = input->getButtonsOn();
    targetReset[player] = 0;
    view = &views[player];
    target = &view->targetInfo;
    selector = &view->selector;

    if (kinds[player].getVariant() == 0) {
        /* manual: the buttons */
        RaceEntryBase *entries = org->entries;
        RaceEntryCar *next = NULL;
        u32 dir = buttons & CAMBTN_UP_DOWN;
        RaceEntryCar *car = entries->cars[target->index[0]];
        switch (dir) {
        case CAMBTN_UP:
            next = prevWatchableByRank(entries, car);
            break;
        case CAMBTN_DOWN:
            next = nextWatchableByRank(entries, car);
            break;
        }
        if (next != NULL && next->number != target->index[0]) {
            wanted = next->number;
            manual = 1;
            forceChange = 1;
        }
    } else {
        /* the chooser's car */
        TargetChooser *chooser = &view->chooser;
        DesignerCamera *designer = &view->designer;
        int may = designer->isEnableChanging() != 0;
        int held;
        if (chooser->pending == 0)
            may = 0;
        held = selector->holdTimer[target->index[0]] > 0;
        if (held)
            may = 0;
        if (may && canCut(player, org, 0)) {
            wanted = chooser->next;
            F_0020E790(chooser, org, selector);
        }
    }

    if (wanted < 0) {
        if (!retarget)
            goto follow;
    } else if (target->index[0] != wanted) {
        newTarget = wanted;
        targetChanged = 1;
        if (kinds[player].getView() != VIEW_DESIGNER)
            forceChange = targetChanged;
    }

    if (retarget) {
        newTarget = target->index[0];
        forceChange = 1;
    }
    if (newTarget >= 0) {
        subChanged = 1;
        newTarget = nextWatchableEntry(org, newTarget);
        secondCar = selector->slot[newTarget].getTarget();
        if (secondCar >= 0 && !isWatchable(org->entries->cars[secondCar]))
            secondCar = -1;
        if (secondCar < 0) {
            if (target->setCar(newTarget, 120))
                forceChange = subChanged;
        } else {
            if (target->setPair(newTarget, secondCar, 120))
                forceChange = subChanged;
        }
        if (manual)
            target->wide = 1;
        selector->holdTimer[newTarget] = 120;
        target->refresh();
        if (manual) {
            forceRecord = 1;
        } else {
            DesignerCamera *designer = &views[player].designer;
            if (designer->isTargetOutOfFrame(target) || designer->isNeedReset(*org, *target)) {
                forceRecord = 1;
                org->eventQueue.post(RACE_EVENT_ERROR, (int)"ilegal target reset.\n", 0);
            }
        }
    }
    goto run_camera;

follow: {
        /* follow the selector */
        CameraTargetSlot *slot;
        newTarget = target->index[0];
        slot = &selector->slot[newTarget];
        if (slot->isChanged()) {
            subChanged = 1;
            newTarget = nextWatchableEntry(org, newTarget);
            secondCar = selector->slot[newTarget].getTarget();
            if (secondCar >= 0 && !isWatchable(org->entries->cars[secondCar]))
                secondCar = -1;
            if (secondCar < 0) {
                if (target->setCar(newTarget, 120))
                    forceChange = subChanged;
            } else {
                if (target->setPair(newTarget, secondCar, 120))
                    forceChange = subChanged;
            }
            selector->holdTimer[newTarget] = 120;
            target->refresh();
        }
    }

run_camera:
    if (buttons & CAMBTN_CROSS) {
        forceChange = 1;
        kinds[player].view ^= 1;
    }
    if (kinds[player].view != VIEW_DESIGNER) {
        CameraView *v = &views[player];
        OnBoardCamera *onboard = &v->onboard;
        int mount = onboard->getMount();
        changed = F_002096C0(player, org, input, forceChange);
        onboard->findMountDown(mount, 8, 0);
    } else {
        int flags = DESIGNER_FADE;
        if (forceChange || retarget)
            flags = DESIGNER_FADE | DESIGNER_RESET;
        changed = updateDesigner(player, org, input, flags | forceRecord);
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
            rng.next();
            if (rng.next01() < 0.07f) {
                int other = target->getSecondCar();
                if (other >= 0) {
                    DesignerCamera *designer = &views[player].designer;
                    designer->setForceOnboard(target);
                    F_00209738(org, other, designer);
                    org->eventQueue.post(RACE_EVENT_ERROR, (int)"TEST : Automatic ONBOARD ", 0);
                }
            }
        }
    }
    {
        CameraView *v = &views[player];
        if (targetChanged)
            F_0020EE28(&v->request, newTarget, secondCar);
        if (subChanged)
            F_0020EE40(&v->request, secondCar);
        if (changed)
            F_0020EE50(&v->request, views[player].camera->type);
        if (retarget)
            F_0020EE60(&v->request, kinds[player].getKind());
    }
    return changed;
}
