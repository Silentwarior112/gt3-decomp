/* T390_camera_mode1.cpp - the camera mode handler of view kind 1
 * (0x0020C510..0x0020D208): at every cut (a BGM cut request, a re-target
 * request or a cut button) a random new target, a random camera and random
 * screen effects (the colour correction overrides, the frame effect, a
 * cross-fade) are picked.  updateMode1 is declared in T383_camera_manager.h;
 * pickAutoCamera has no header (camera_modes_internal.h). */
/* Retail T390's .rodata (0x0033F2A0): <new>'s "bad_alloc", updateMode1's
 * jump table, the GameStatus's garage type names (with GarageCar's base
 * list), then "14RaceCameraBase", "t9UnitArena1Ui64": reverse definition
 * order, so T153 first, the camera headers, the GameStatus last.  Nothing
 * references "bad_alloc": DATA_START. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager::updateMode1 */
#include "game/save/T428_game_options.h"         /* GameOptions replaySoundLevel / replayCutStyle */
#include "game/save/T429_game_status.h"          /* getGameStatus (seen: the garage classes) */
// DATA_START 0x0033F2A0

/* T390's .data (0x002DED30..0x002DEDB4, before its frame info), in address
 * order; each array is 8-aligned, so the 4 bytes after each are padding. */

/* The colours of mode 1's colour override: red, blue, cyan, grey .3, white. */
f32 g_002DED30[5][3] = {                    /* 0x002DED30 */
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f },
    { 0.3f, 0.3f, 0.3f },
    { 1.0f, 1.0f, 1.0f }
};
/* pickAutoCamera's tables: during the countdown (always the designer), the
 * next marker < 3 s away, 3 s or more. */
f32 g_002DED70[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };        /* 0x002DED70 */
f32 g_002DED88[5] = { 0.35f, 0.6f, 0.7f, 0.85f, 1.0f };      /* 0x002DED88 */
f32 g_002DEDA0[5] = { 0.0f, 0.3f, 0.5f, 0.7f, 1.0f };        /* 0x002DEDA0 */

// FUNC 0x0020C510 pickAutoCamera
//   The camera of a cut from a random r in [0, 1): the first i with
//   r < table[i] (0 automatic, 1 automount, 2 matrix, 3 zoomup, 4 designer
//   onboard).  The table depends on the time to the next marker; during the
//   countdown it is {0, 0, 0, 0, 1}: always 4.
int pickAutoCamera(int counting, f32 nextMarker, f32 r)
{
    const f32 *table = g_002DED88;
    int i;

    if (3.0f <= nextMarker)
        table = g_002DEDA0;
    if (counting)
        table = g_002DED70;
    for (i = 0; i < 5; i++) {
        if (r < table[i])
            return i;
    }
    return 4;
}

// FUNC 0x0020C580 CameraManager::updateMode1
//   Up / down step soundLevel, the 0..1 level of the second sound volume
//   factor g_003521E4 (T176), by 0.1 (applied unless BGM marker 8 holds it
//   at 1.0, and saved in the options); right / left switch mode1Style (race
//   event 0x15 with style 1 or 2).  At a cut: a random car other than the target (the first one drawn in
//   full from there), a course camera record for the designer camera on it
//   (trying the following cars), then with 25% (or when no record was found)
//   one of the other cameras (pickAutoCamera), and the random effects:
//   levelOverride / levelLo / levelHi, frameEffect / frameEffectLevel /
//   frameEffectFading, a cross-fade, colourOverride / colour / saturation.
//   Returns 1 when the camera changed.  dc is not used.
int CameraManager::updateMode1(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player)
{
    int changed = 0;                        /* sp+0x08 (spill slots in declaration order) */
    f32 fadeBias = 0.0f;                    /* f22 -0.3 at a request-3 cut: fewer cross-fades */
    f32 effectBias = 0.0f;                  /* f23 0.5 with the matrix camera: no frame effect */
    u32 buttons = 0;                        /* s1 */
    int targetChanged;                      /* sp+0x0C */
    int newTarget;                          /* sp+0x10 */
    int retarget = targetReset[player] != 0;    /* sp+0x14 */
    int counting;                           /* sp+0x18 */
    int cut;                                /* s2 */
    int request;                            /* s5 */
    f32 oldStyle;
    GameOptions *options;
    CameraView *view;
    CameraTargetSelector *selector;
    CameraTargetInfo *target;
    int car0;

    targetReset[player] = 0;
    targetChanged = 0;
    newTarget = -1;
    counting = org->countDown > 0;
    if (input != NULL)
        buttons = input->getButtonsOn();

    if (soundLevelApply != 0) {
        f32 level = soundLevel;
        if (soundLevelFull != 0)
            level = 1.0f;
        g_003521E4 = level;
    }

    options = &getGameStatus()->options;
    switch (buttons & CAMBTN_UP_DOWN) {
    case CAMBTN_DOWN:
        soundLevel = soundLevel - 0.1f;
        if (soundLevel < 0.0f)
            soundLevel = 0.0f;
        if (soundLevelFull == 0)
            g_003521E4 = soundLevel;
        options->replaySoundLevel = soundLevel;
        break;
    case CAMBTN_UP:
        soundLevel = soundLevel + 0.1f;
        if (1.0f < soundLevel)
            soundLevel = 1.0f;
        if (soundLevelFull == 0)
            g_003521E4 = soundLevel;
        options->replaySoundLevel = soundLevel;
        break;
    }
    oldStyle = mode1Style;
    switch (buttons & CAMBTN_LEFT_RIGHT) {
    case CAMBTN_LEFT:
        mode1Style = oldStyle - 1.1f;
        if (mode1Style < 0.0f)
            mode1Style = 0.0f;
        options->replayCutStyle = mode1Style;
        break;
    case CAMBTN_RIGHT:
        mode1Style = oldStyle + 1.1f;
        if (1.0f < mode1Style)
            mode1Style = 1.0f;
        options->replayCutStyle = mode1Style;
        break;
    }
    if (mode1Style != oldStyle) {
        int style = 2;
        if (mode1Style < 0.5f)
            style = 1;
        org->eventQueue.post(RACE_EVENT_VIEW_MODE, (style << 16) | (1 << 8) | player, 0);
    }
    cut = 0;
    if (buttons & CAMBTN_CROSS2)
        cut = 1;
    if (buttons & CAMBTN_CIRCLE)
        cut = 1;
    if (buttons & CAMBTN_R1)
        cut = 1;
    if (buttons & CAMBTN_L1)
        cut = 1;

    if (frameEffect != 0 && frameEffectFading != 0) {
        frameEffectLevel = frameEffectLevel - 0.02f;
        if (frameEffectLevel < 0.0f)
            frameEffectFading = 0;
    }

    /* follow the selector (the second car only) */
    view = &views[player];
    target = &view->targetInfo;
    selector = &view->selector;
    car0 = target->index[0];
    if (selector->slot[car0].isChanged()) {
        int secondCar = selector->slot[car0].getTarget();
        if (secondCar >= 0 && !isWatchable(org->entries->cars[secondCar]))
            secondCar = -1;
        if (secondCar < 0)
            target->setCar(car0, 120);
        else
            target->setPair(car0, secondCar, 120);
        target->refresh();
    }

    request = changeRequest;
    if (counting)
        request = 0;
    if (request > 0 || retarget || cut) {
        f32 markerTime = nextMarkerTime;                /* f20 */
        int carCount = org->entries->count;        /* fp (compared signed) */
        DesignerCamera *designer = &views[player].designer;   /* s4 */
        Random *random = &rng;                          /* s2 */
        RaceCourse *course = org->course;               /* sp+0x20 */
        int oldTarget = target->index[0];               /* sp+0x24 */
        int car;
        int secondCar;
        int tries;
        int lap;                                        /* sp+0x00 */
        CourseCameraRecord *rec;                        /* sp+0x28 */
        DesignerCamera *designer2;                      /* sp+0x2C */
        f32 r;
        f32 pick;                                       /* the colour override's draw */

        if (request == 3)
            fadeBias = -0.3f;

        /* a random car, not the current one */
        random->next();
        car = (int)(random->next01() * (f32)carCount);
        if (car == oldTarget)
            car = car + 1;
        car = nextWatchableEntry(org, car < carCount ? car : 0);
        secondCar = selector->slot[car].getTarget();
        if (secondCar >= 0 && !isWatchable(org->entries->cars[secondCar]))
            secondCar = -1;
        if (secondCar < 0 || request == 1)
            target->setCar(car, 1);
        else
            target->setPair(car, secondCar, 1);
        newTarget = car;
        target->refresh();

        /* a course camera record for it, else for the following cars */
        tries = 0;
        rec = NULL;
        targetChanged = car != oldTarget;
        designer2 = &views[player].designer;
        while (tries < carCount) {
            rec = camera_find_next_record_for(org, &progress, car, designer2->cameraData, &lap, markerTime);
            if (rec != NULL)
                break;
            car = car + 1;
            car = nextWatchableEntry(org, car < carCount ? car : 0);
            secondCar = selector->slot[car].getTarget();
            if (secondCar >= 0 && !isWatchable(org->entries->cars[secondCar]))
                secondCar = -1;
            if (secondCar < 0)
                target->setCar(car, 1);
            else
                target->setPair(car, secondCar, 1);
            tries++;
            target->refresh();
            targetChanged = car != oldTarget;
            if (targetChanged)
                newTarget = car;
        }

        if (rec != NULL) {
            /* the designer camera on it, with random start offsets (the
             * inlined setInitZoom / setInitRotOffset / setInitBankAnim) */
            views[player].camera = designer;
            changed = 1;
            designer->setupOnRecord(target, lap, course, rec, camera_record_number(course, rec));
            designer->giveupState = 2;
            designer->giveupMode = 600;
            random->next();
            designer->bankForcibly = random->next01() * 20.0f - 10.0f;
            random->next();
            if (random->next01() < 0.17f) {
                designer->initZoomSet = changed;    /* the code stores `changed' (1) */
                designer->initZoom = 1.0f;
            }
            random->next();
            if (random->next01() < 0.3f) {
                f32 yaw = random->next01() * 60.0f;     /* (no next() before this one) */
                designer->initRotPitch = 0;
                designer->initRotOffsetSet = changed;
                designer->initRotYaw = yaw - 90.0f;
            }
            random->next();
            if (random->next01() < 0.1f) {
                random->next();
                f32 bank = random->next01() * 200.0f - 100.0f;
                designer->bankAnim.reset(bank);
            }
        }

        /* or another camera */
        random->next();
        if (random->next01() < 0.25f || rec == NULL) {
            changed = 1;
            random->next();
            switch (pickAutoCamera(counting, markerTime, random->next01())) {
            case 0: {
                AutomaticCamera *cam = &views[player].automatic;
                views[player].camera = cam;
                cam->setup(target, course, markerTime);
                break;
            }
            case 1: {
                AutomountCamera *cam = &views[player].automount;
                views[player].camera = cam;
                cam->setup(target, markerTime);
                break;
            }
            case 2: {
                MatrixCamera *cam;
                effectBias = 0.5f;
                cam = &views[player].matrix;
                views[player].camera = cam;
                cam->setup(target, markerTime);
                break;
            }
            case 3: {
                ZoomupCamera *cam = &views[player].zoomup;
                views[player].camera = cam;
                cam->setup(target, markerTime);
                break;
            }
            case 4:
            default: {
                /* the designer forced onboard, on a random mount from 3 */
                int mount = 0;
                int count;
                int span;
                views[player].camera = designer;
                designer->setForceOnboard(target);
                designer->giveupState = 2;
                designer->giveupMode = 600;
                count = designer->getMountCount();
                span = count - 3;
                if (span > 0) {
                    random->next();
                    mount = (int)(random->next01() * (f32)span);
                    if (!(-1 < mount))
                        mount = 0;
                    if (!(mount < span))
                        mount = count - 4;
                    mount = mount + 3;
                }
                designer->setMount(mount);
                break;
            }
            }
        }

        /* the level override: lo 0..0.175, hi 0.2..1 (swapped at r < 0.0125) */
        random->next();
        r = random->next01();
        levelOverride = 0;
        if (r < 0.25f) {
            f32 lo;
            f32 hi;
            levelOverride = 1;
            random->next();
            lo = random->next01() * 0.175f;
            if (0.175f < lo)
                lo = 0.175f;
            random->next();
            hi = random->next01() * 0.8f + 0.2f;
            if (r < 0.0125f) {
                f32 swap = lo;
                lo = hi;
                hi = swap;
            }
            levelLo = lo;
            levelHi = hi;
        }

        /* the frame effect: 0.5..2.0, fading out half of the time */
        frameEffect = 0;
        random->next();
        if (random->next01() - effectBias < 0.1f) {
            f32 level;
            frameEffect = 1;
            random->next();
            level = random->next01() * 150.0f;
            frameEffectFading = 0;
            frameEffectLevel = (level + 50.0f) / 100.0f;
            random->next();
            if (random->next01() < 0.5f)
                frameEffectFading = 1;
        }

        random->next();
        if (random->next01() - fadeBias < 0.4f)
            crossFade = 1;

        /* the colour override: kept with 80%, else switched on with 10% */
        random->next();
        pick = random->next01();
        if (colourOverride != 0) {
            colourOverride = 0;
            if (pick < 0.8f)
                colourOverride = 1;
        } else if (pick < 0.1f) {
            f32 q;
            int k;
            colourOverride = 1;
            random->next();
            q = random->next01();
            saturation = 0.0f;
            if (q < 0.7f) {
                saturation = 1.0f;
            } else if (q < 0.85f) {
                random->next();
                saturation = random->next01();
            }
            random->next();
            k = (int)(random->next01() * 5.0f) % 5;
            colour[0] = g_002DED30[k][0];
            colour[1] = g_002DED30[k][1];
            colour[2] = g_002DED30[k][2];
        } else {
            colourOverride = 0;
        }
    }

    if (changed)
        target->finishBlend();
    {
        CameraView *v = &views[player];
        if (targetChanged)
            F_0020EE28(&v->request, newTarget, -1);
        if (changed)
            F_0020EE50(&v->request, views[player].camera->type);
        if (retarget)
            F_0020EE60(&v->request, kinds[player].getKind());
    }
    return changed;
}
