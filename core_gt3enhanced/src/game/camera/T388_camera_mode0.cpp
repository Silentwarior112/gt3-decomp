/* T388_camera_mode0.cpp - the camera mode handler of view kinds 0 and 3 (and
 * any kind CameraManager::update does not give to T389 / T390)
 * (0x0020B8E8..0x0020BE58). */
/* Retail T388's .rodata (0x0033F1E8): <new>'s "bad_alloc", the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (nothing references them:
 * DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T388_camera_mode0.h"         /* T388 (CameraSet; the method is declared in T383) */
#include "game/enhanced.h"                  /* ENH_CAMERA_ZOOM_BUTTON, ENH_CAMERA_START_SEQUENCE */
// DATA_START 0x0033F1E8

/* The demo's automatic "car ahead" cut: after this many frames without a
 * cut.  Lupo 2 2400 (40 s, L2 0x00211F38, cameras_sound.md C6 c); GT3 300. */
#if ENH_CAMERA_START_SEQUENCE
#define AUTO_CUT_FRAMES     2400
#else
#define AUTO_CUT_FRAMES     300
#endif

/* The enhanced engine (Lupo 2, cameras_sound.md C5): the zoom spring's
 * target while CAMBTN_ZOOM is held, and the TV camera's zoom factor at it
 * (L2 0x00212364..0x002123BC: 1 + value x 4 / 20, within 1..4). */
#define ZOOM_SPRING_HOLD    20.0f
#define ZOOM_MULT_MAX       4.0f

/* DynamicsConductor::getStartingFormat (slot 8) values with no start
 * sequence in view mode 0 (Lupo 2 L2 0x00211F0C..0x00211F1C: 0 and 2): a
 * rolling start (launch speed) and the free run / time trial conductors.
 * (The race display's START_FORMAT_*, race_display_internal.h.) */
enum {
    START_SEQ_FORMAT_ROLLING  = 0,
    START_SEQ_FORMAT_FREE_RUN = 2
};

// FUNC 0x0020B8E8 CameraManager::updateMode0
//   The target follows the up / down buttons (the car ahead / behind in the
//   race order), a re-target request, or - in kinds other than 0 and 3 - the
//   target selector's slot of the current target when it changed.  In a demo
//   (replayMode 2, not a race preview) the "car ahead" button is pressed for
//   the player every 300 frames when the camera may cut.  Then the onboard
//   or the designer camera runs.  Returns 1 when the camera changed.  dc is
//   not used.
//   The enhanced engine (Lupo 2 L2 0x00211D60):
//   - ENH_CAMERA_START_SEQUENCE (cameras_sound.md C6): the start of a
//     replay shows the starting camera (F_00209660) while the countdown runs
//     and for one second after it (startClock, from update), with a 1 s
//     cross-fade at 3, 2 and 1 s before its end; on the first frame after
//     the countdown the starting camera is held (StartingCamera::setStatic);
//     the first normal frame after it forces a cut.  Not for a rolling start
//     or the free run / time trial conductors (dc->getStartingFormat(): dc
//     is read then).  The demo's automatic cut every 2400 frames (GT3 300).
//   - ENH_CAMERA_ZOOM_BUTTON (C5): with the designer camera, CAMBTN_ZOOM held
//     raises the view's zoom spring, and the camera's zoomMult follows it.
//   Lupo 2 also stopped cross-fading on a target change here; GT3's
//   cross-fade is kept (outside this port's survey entries).
int CameraManager::updateMode0(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player)
{
    int forceChange = 0;                    /* fp */
    int newTarget = -1;                     /* s7 */
    int preview = org->racePreviewMode;
    int demo = org->race->replayMode == 2;
    int targetChanged = 0;                  /* sp+0x04 (spill slots in declaration order) */
    int takeWide = 0;                       /* sp+0x08 */
    int manual = 1;                         /* sp+0x0C 0 after the demo's automatic button */
    int subChanged = 0;                     /* sp+0x10 */
    int autoCut = 0;                        /* sp+0x14 */
    int retarget = targetReset[player] != 0;    /* sp+0x18 */
    int secondCar = -1;                     /* sp+0x1C */
    int counting = org->countDown > 0;      /* sp+0x20 */
    u32 buttons = 0;                        /* sp+0x24 */
    int changed;
    CameraView *view;
    CameraTargetInfo *target;
    CameraTargetSelector *selector;
#if ENH_CAMERA_ZOOM_BUTTON
    u32 held = 0;                           /* Lupo 2: the buttons held (the zoom) */
#endif
#if ENH_CAMERA_START_SEQUENCE
    /* Lupo 2: the frames left of the start sequence (60 / 120 / 180: a
     * cross-fade), < 0 when it is over */
    int startFrame;

    if (counting) {
        startFrame = org->countDown;
    } else {
        startFrame = 60 - startClock;
        counting = startClock < 60;
        if (startFrame == 59)
            views[player].starting.setStatic();
    }
#endif

    if (input != NULL) {
        buttons = input->getButtonsOn();
#if ENH_CAMERA_ZOOM_BUTTON
        held = input->getButtons();
#endif
    }
    targetReset[player] = 0;
#if ENH_CAMERA_START_SEQUENCE
    switch (dc->getStartingFormat()) {
    case START_SEQ_FORMAT_ROLLING:
    case START_SEQ_FORMAT_FREE_RUN:
        startFrame = -60;
        break;
    }
#endif
    autoTimer = autoTimer + 1;
    {
        int autoButton = manual;
        if (!demo)
            autoButton = 0;
        if (preview)
            autoButton = 0;
        if (autoTimer < AUTO_CUT_FRAMES)
            autoButton = 0;
        if (autoButton && canCut(player, org, 1)) {
        autoTimer = 0;
        manual = 0;
        autoCut = 1;
        buttons |= CAMBTN_UP;
        }
    }

    {
        u32 dir = buttons & CAMBTN_UP_DOWN;
        RaceEntryBase *entries = org->entries;
        RaceEntryCar *next = NULL;
        RaceEntryCar *car;

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

        if (next == NULL) {
            if (!retarget)
                goto follow;
        } else if (target->car[0] != next) {
            newTarget = next->number;
            targetChanged = 1;
            takeWide = manual;
            forceChange = targetChanged;
            if (kinds[player].getView() == VIEW_DESIGNER)
                forceChange = 0;
            secondCar = selector->slot[newTarget].getTarget();
            if (counting)
                forceChange = targetChanged;
            crossFade = targetChanged;
            crossFadeTime = 1.0f;
        }

        if (retarget) {
            newTarget = target->index[0];
            forceChange = 1;
        }
        if (newTarget >= 0) {
            int kind;
            subChanged = 1;
            kind = kinds[player].getKind();
            if (kind == 0 || kind == 3)
                secondCar = -1;
            else
                secondCar = selector->slot[newTarget].getTarget();
            if (secondCar < 0) {
                if (target->setCar(newTarget, 15))
                    forceChange = 1;
            } else {
                if (target->setPair(newTarget, secondCar, 15))
                    forceChange = 1;
            }
            if (takeWide)
                target->wide = 1;
            selector->holdTimer[newTarget] = 15;
            target->refresh();
        }
        goto run_camera;

    follow: {
            /* follow the selector */
            int kind = kinds[player].getKind();
            newTarget = target->index[0];
            if (kind != 0 && kind != 3) {
                if (selector->slot[newTarget].isChanged()) {
                    subChanged = 1;
                    secondCar = selector->slot[newTarget].getTarget();
                    if (secondCar < 0)
                        forceChange = target->setCar(newTarget, 120) ? subChanged : 0;
                    else
                        forceChange = target->setPair(newTarget, secondCar, 120) ? subChanged : 0;
                    selector->holdTimer[newTarget] = 120;
                    target->refresh();
                }
            }
        }
    }

run_camera:
    if (buttons & CAMBTN_CROSS) {
        forceChange = 1;
        kinds[player].view ^= 1;
    }
#if ENH_CAMERA_START_SEQUENCE
    if (startFrame >= 0) {
        /* Lupo 2 (L2 0x002123C8..0x00212418): the starting camera */
        changed = F_00209660(player, org, input);
        startShown = 1;
        if (startFrame == 60 || startFrame == 120 || startFrame == 180) {
            crossFade = 1;
            crossFadeTime = 1.0f;
        }
    } else {
        if (startShown) {                   /* the first frame after it: a cut */
            startShown = 0;
            autoCut = 1;
        }
#endif
    if (kinds[player].view != VIEW_DESIGNER) {
        CameraView *view = &views[player];
        OnBoardCamera *onboard = &view->onboard;
        int mount = onboard->getMount();
        changed = F_002096C0(player, org, input, forceChange);
        onboard->findMountDown(mount, 8, 0);
    } else {
        int flags = DESIGNER_FADE;
        if (forceChange || retarget)
            flags = DESIGNER_FADE | DESIGNER_RESET;
        changed = updateDesigner(player, org, input, flags | autoCut);  /* autoCut: DESIGNER_FORCE */
#if ENH_CAMERA_ZOOM_BUTTON
        {
            /* Lupo 2 (L2 0x00212328..0x002123C4): the zoom button */
            CameraZoomSpring *spring = &views[player].zoomSpring;
            f32 mult;

            spring->setTarget(0.0f);
            if (held & CAMBTN_ZOOM)
                spring->setTarget(ZOOM_SPRING_HOLD);
            mult = spring->get() * ZOOM_MULT_MAX / ZOOM_SPRING_HOLD + 1.0f;
            if (mult < 1.0f)
                mult = 1.0f;
            if (ZOOM_MULT_MAX < mult)
                mult = ZOOM_MULT_MAX;
            views[player].designer.setZoomMult(mult);
        }
#endif
    }
#if ENH_CAMERA_START_SEQUENCE
    }
#endif
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
        CameraView *view = &views[player];
        if (targetChanged)
            F_0020EE28(&view->request, newTarget, secondCar);
        if (subChanged)
            F_0020EE40(&view->request, secondCar);
        if (changed)
            F_0020EE50(&view->request, views[player].camera->type);
        if (retarget)
            F_0020EE60(&view->request, kinds[player].getKind());
    }
    return changed;
}
