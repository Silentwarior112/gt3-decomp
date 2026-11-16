#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/camera/T383_camera_manager.cpp"
#pragma implementation "src/game/camera/T383_camera_manager.cpp"
#pragma implementation "T383_camera_manager.h"
#endif
/* T383 (0x00207768..0x0020A2E0): CameraManager - the race camera of up to four
 * views: construction, init, the per-frame update (the view mode from the
 * pad, the mode handlers of T387..T391), the i.LINK screen wall (the window
 * and the turn of each screen), the forwarders to the current view's camera,
 * the accessors and the typeinfo functions of CameraManager and
 * RaceCameraBase. */
/* Retail T383's .rodata (0x0033EEA8): <new>'s "bad_alloc" (nothing references
 * it: DATA_START), the vtable, then the type names in reverse definition
 * order: the garage's (GarageWheelList .. Cell<GarageCar>, the GameStatus
 * header read after the camera manager's), CameraManager, RaceCameraBase,
 * UnitArena<64> (T153 read first). */
#include <new>                                  /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO                /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"              /* RaceCourse (seen first: UnitArena<64>) */
#include "game/camera/camera_manager_internal.h"
#include "game/camera/T383_camera_manager.h"           /* CameraManager, CameraView, CameraProjection, CarProgress,
                                                   ScreenLayout, CameraViewKind, RaceCameraBase; through it
                                                   T377 / T378 / T392 / T393 / T395 / T396 / T397..T406 /
                                                   T410 / T531 */
#include "game/camera/T384_camera_groove_meter.h"      /* CameraGrooveMeter */
#include "game/camera/T385_camera_demo.h"              /* CourseCameraData (= CourseCamera) */
#include "game/camera/T172_course_camera.h"            /* CourseCamera, CourseCameraRecord */
#include "game/course/T178_course_env.h"               /* CourseEnv (the colour correction values) */
#include "game/course/T179_course_data.h"              /* CourseData (env, replay) */
#include "game/race/core/T121_race_entry.h"               /* RaceEntryCar, RaceEntryBase */
#include "game/race/core/T122_race_event_queue.h"         /* RaceEventQueue */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization */
#include "game/race/core/T129_race_base.h"                /* RaceBase (replayMode, virtual_5) */
#include "game/race/core/T413_race_input.h"               /* RaceInput (replayMode), Control::getButtonsOn */
#include "game/save/T428_game_options.h"             /* GameOptions */
#include "game/save/T429_game_status.h"              /* gt2k_game::GameStatus, getGameStatus (the garage names) */
#include "game/camera/T394_replay_view_mode.h"         /* ReplayViewInfo, F_0020E930, F_0020EA58, F_0020EBA0 */
#include "game/car/physics/T289_car_mode.h"                 /* car_display_level, CAR_SHOW_FULL */
#include "game/camera/T379_camera_rot_damper.h"        /* angle_wrap360 */
#include "game/sound/T176_car_sound.h"                /* g_003521E4 */
#include "engine/pgl/G001_flare.h"                    /* gfx_*, pgl_*, gs_*, vu0_*, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"              /* sinf, cosf */
// DATA_START 0x0033EEA8

/* RaceBase::replayMode (T129): the race was started as a demo replay. */
#define RACE_REPLAY_DEMO 2

// FUNC 0x00207768 CameraView::CameraView
/* Only the members: TargetChooser's Random (its inline constructor stores
 * 0x87654321 at +0x2C8), then the ten cameras in order (OnBoardCamera,
 * DesignerCamera, PasslineCamera, StartingCamera, AutomaticCamera,
 * AutomountCamera, LoggerCamera, MatrixCamera, ZoomupCamera, HeliCamera);
 * the exception cleanups destroy the ones already built (flags 0). */
CameraView::CameraView()
{
    /* implicit: OnBoardCamera, ~OnBoardCamera, DesignerCamera, ~DesignerCamera, PasslineCamera,
     * ~PasslineCamera, StartingCamera, ~StartingCamera, AutomaticCamera, ~AutomaticCamera,
     * AutomountCamera, ~AutomountCamera, LoggerCamera, ~LoggerCamera, HeliCamera, MatrixCamera,
     * ~MatrixCamera, ZoomupCamera, ~ZoomupCamera (compiler-generated: bases, members, deleting
     * form) */
}

// FUNC 0x00207960 CameraManager::CameraManager
/* The vptr, the four CameraView, rng (Random's 0x87654321), the
 * CameraGrooveMeter, then init(NULL, 0).  The exception cleanups: the
 * CameraGrooveMeter, the views built so far (each camera last first:
 * HeliCamera, ZoomupCamera, MatrixCamera, LoggerCamera, AutomountCamera,
 * AutomaticCamera, StartingCamera, PasslineCamera, DesignerCamera,
 * OnBoardCamera; flags 2), then CameraBase::~CameraBase. */
CameraManager::CameraManager()
{
    /* implicit: ~OnBoardCamera, ~DesignerCamera, ~PasslineCamera, ~StartingCamera,
     * ~AutomaticCamera, ~AutomountCamera, ~LoggerCamera, ~HeliCamera, ~MatrixCamera,
     * ~ZoomupCamera, CameraView, CameraGrooveMeter, ~CameraGrooveMeter, ~CameraBase
     * (compiler-generated: bases, members, deleting form) */
    init(NULL, 0);
}

// FUNC 0x00207BC0 CameraManager::~CameraManager
/* The CameraGrooveMeter, then the views (last first, each camera last first:
 * HeliCamera, ZoomupCamera, MatrixCamera, LoggerCamera, AutomountCamera,
 * AutomaticCamera, StartingCamera, PasslineCamera, DesignerCamera,
 * OnBoardCamera; flags 2), then CameraBase::~CameraBase with the flags. */
CameraManager::~CameraManager()
{
    /* implicit: ~OnBoardCamera, ~DesignerCamera, ~PasslineCamera, ~StartingCamera,
     * ~AutomaticCamera, ~AutomountCamera, ~LoggerCamera, ~HeliCamera, ~MatrixCamera,
     * ~ZoomupCamera, ~CameraGrooveMeter, ~CameraBase (compiler-generated: bases, members, deleting
     * form) */
}

// FUNC 0x00207CC0 CameraManager::resetState
/* (Matching: the flags of 0x23C..0x270 are CameraFlag, bool to egcs, so
 * their stores keep source order with the float ones; marker2Armed before
 * soundLevelFull and demoTimer last: egcs schedules the store that frees a
 * register first.) */
void CameraManager::resetState()
{
    GameOptions *options;

    forcedTarget = -1;
    colourOverride = 0;
    crossFadeTime = 0.5f;
    levelOverride = 0;
    levelLo = 0.0f;
    levelHi = 1.0f;
    colour[0] = 0.0f;
    colour[1] = 0.0f;
    colour[2] = 0.0f;
    saturation = 1.0f;
    frameEffect = 0;
    frameEffectLevel = 0.0f;
    frameEffectFading = 0;
    crossFade = 0;
    loggerSeek = 0;
    marker1Enabled = 0;
    marker1Armed = 0;
    unk_962B = 0;
    marker2Random = 0;
    soundLevelApply = 0;
    markerTimeout = 60;
    marker2Armed = 1;
    soundLevelFull = 1;
    options = &getGameStatus()->options;
    soundLevel = options->replaySoundLevel;
    mode1Style = options->replayCutStyle;
    if (soundLevel < 0.0f)
        soundLevel = 0.0f;
    if (soundLevel > 1.0f)
        soundLevel = 1.0f;
    if (mode1Style < 0.0f)
        mode1Style = 0.0f;
    if (mode1Style > 1.0f)
        mode1Style = 1.0f;
    g_003521E4 = 1.0f;
    autoTimer = 0;
    demoIndex = 0;
    demoTimer = 0;
}

// FUNC 0x00207DE0 CameraManager::updateWindowCentred
void CameraManager::updateWindowCentred()
{
    updateWindow();
    proj.left = -SCREEN_HALF_W;
    proj.right = SCREEN_HALF_W;
}

// FUNC 0x00207E20 CameraManager::updateWindow
/* The window of this screen in a wall of 640 x 448 screens (the tiles count
 * from 1; the u32 - 1 is converted unsigned). */
void CameraManager::updateWindow()
{
    f32 left, right, top, bottom;

    if (layoutFlags & WALL_LEFT_EDGE) {
        if (tileFromRight != 0)
            left = -SCREEN_W - (f32)(tileFromRight - 1) * SCREEN_W;
        else
            left = (f32)(tileFromLeft - 1) * SCREEN_W + 0.0f;
    } else {
        if (tileFromRight != 0)
            left = -SCREEN_HALF_W - (f32)(tileFromRight - 1) * SCREEN_W;
        else
            left = (f32)(tileFromLeft - 1) * SCREEN_W + -SCREEN_HALF_W;
    }
    right = left + SCREEN_W;
    if (layoutFlags & WALL_TOP_EDGE) {
        if (tileFromBottom != 0)
            top = (f32)(tileFromBottom - 1) * SCREEN_H + SCREEN_H;
        else
            top = 0.0f - (f32)(tileFromTop - 1) * SCREEN_H;
    } else {
        if (tileFromBottom != 0)
            top = (f32)(tileFromBottom - 1) * SCREEN_H + SCREEN_HALF_H;
        else
            top = SCREEN_HALF_H - (f32)(tileFromTop - 1) * SCREEN_H;
    }
    bottom = top - SCREEN_H;
    proj.left = left;
    proj.right = right;
    proj.top = top;
    proj.bottom = bottom;
}

/* The five tests return bool (game/camera/T383_camera_manager.h; u047 matching:
 * RaceLinkBattle::virtual_8 passes them on to bool parameters unconverted);
 * their code is the same as with int. */
// FUNC 0x002080E0 CameraManager::isCenterTopLeft
/* The screen above and left of the wall's centre (RaceLinkBattle: the
 * top left panel). */
bool CameraManager::isCenterTopLeft()
{
    return tileFromRight == 1 && tileFromBottom == 1;
}

// FUNC 0x00208110 CameraManager::isCenterTopRight
/* Above and right of the centre (the top right panel). */
bool CameraManager::isCenterTopRight()
{
    return tileFromLeft == 1 && tileFromBottom == 1;
}

// FUNC 0x00208140 CameraManager::isCenterBottomLeft
/* Below and left of the centre (the bottom left panel). */
bool CameraManager::isCenterBottomLeft()
{
    return tileFromRight == 1 && tileFromTop == 1;
}

// FUNC 0x00208170 CameraManager::isCenterBottomRight
/* Below and right of the centre (the bottom right panel). */
bool CameraManager::isCenterBottomRight()
{
    return tileFromLeft == 1 && tileFromTop == 1;
}

// FUNC 0x002081A0 CameraManager::isCenterScreen
/* The centre screen (the center panel, T131's mirror): the same code as
 * isCenterBottomRight. */
bool CameraManager::isCenterScreen()
{
    return tileFromLeft == 1 && tileFromTop == 1;
}

// FUNC 0x002081D0 CameraManager::setScreenLayout
/* (Matching: an explicit `case 0' gives retail's == 1, < 2, == 2 decision
 * tree; the loop counts up (egcs reverses it into retail's count-down).) */
void CameraManager::setScreenLayout(const ScreenLayout *layout)
{
    int modeX = layout->modeX;
    int nX = layout->nX;
    int modeY = layout->modeY;
    int nY = layout->nY;
    u32 flags = layout->flags;
    int i;

    switch (modeX) {
    case WALL_COUNT_FROM_RIGHT:
        tileFromRight = nX;
        tileFromLeft = 0;
        break;
    case WALL_COUNT_FROM_LEFT:
        tileFromLeft = nX;
        tileFromRight = 0;
        break;
    case 0:
    default:
        tileFromRight = 1;
        tileFromLeft = 1;
        break;
    }
    switch (modeY) {
    case WALL_COUNT_FROM_BOTTOM:
        tileFromBottom = nY;
        tileFromTop = 0;
        break;
    case WALL_COUNT_FROM_TOP:
        tileFromTop = nY;
        tileFromBottom = 0;
        break;
    case 0:
    default:
        tileFromBottom = 1;
        tileFromTop = 1;
        break;
    }
    if (flags & WALL_TURN_45) {
        for (i = 0; i < 4; i++)
            views[i].onboard.fov = 45.0f;
    }
    layoutFlags = flags;
    if ((flags & WALL_TURN_MASK) == WALL_TURNED)
        updateWindowCentred();
    else
        updateWindow();
}

/* The race plays as a demo (init): a helper returning the comparison, as
 * retail tests replayMode with `xori 2; bnez' (a store-flag tested), not
 * `li 2; bne' (u092 matching; T388's updateMode0 keeps the same test in a
 * variable). */
static inline int isDemoReplay(RaceOrganization *org)
{
    return org->race->replayMode == RACE_REPLAY_DEMO;
}

// FUNC 0x00208320 CameraManager::init
/* Also run by the constructor with org NULL.  Every view gets the view mode
 * and camera target of the game options, its parts set up on car `car' and
 * all ten cameras set up; with a race, the cameras get the course and its
 * camera file.  The cameras and parts are reached through the inline
 * accessors, one call per use: retail strength-reduces each use into a
 * pointer of its own (two per camera), and calls virtually through them.
 * The stores at the top in field order but tileFromTop last (egcs puts the
 * store that frees a register first); `view' (the request, index[0]) is set
 * at the loop top, so loop.c derives index[0] from the second zoomup pointer
 * (-0x1F98) as retail does.  Not exact (96.0%, the same with the reference
 * compiler 991111-01): the three loop invariants GCSE hoists (&demoTarget,
 * &options->target[0], &correction) get their spill slots in another order
 * (0x20 / 0x18 / 0x1C, retail 0x18 / 0x1C / 0x20), the demo test's
 * registers (v0 / v1 / a0) differ, and the pointer updates after
 * F_0020EDE8 are placed otherwise.  Tried: index[0] through getTargetInfo /
 * getView / views[i] / F_00209BC8, request through getView or a local,
 * camera / kind argument forms, helper forms for the demo test. */
void CameraManager::init(RaceOrganization *org, int car)
{
    GameOptions *options = &getGameStatus()->options;
    int i;

    rng.setSeed((u32)gfx_perf_counter0());
    proj.left = -SCREEN_HALF_W;
    proj.right = SCREEN_HALF_W;
    proj.top = SCREEN_HALF_H;
    proj.bottom = -SCREEN_HALF_H;
    proj.nearClip = 0.1f;
    proj.farClip = 750.0f;
    proj.focal = 320.0f;
    shift = 0.2f;
    frame = 0;
    tileFromRight = 1;
    tileFromLeft = 1;
    tileFromBottom = 1;
    layoutFlags = 0;
    tileFromTop = 1;
    for (i = 3; i >= 0; i--)
        targetReset[i] = 1;
    demoMode = 0;
    changeRequest = 1;
    nextMarkerTime = 4.0f;
    groove.reset();
    resetState();
    viewCount = 1;
    current = 0;
    progress.init(org);

    for (i = 0; i < 4; i++) {
        CameraView *view = &views[i];
        u32 viewMode = options->viewMode;   /* compared unsigned (sltiu) */
        int targetKind;
        ReplayViewInfo *modeInfo;

        kinds[i].view = 0;
        if (viewMode >= VIEWMODE_COUNT)
            viewMode = VIEWMODE_TRADITIONAL;
        targetKind = options->target[viewMode];
        modeInfo = F_0020EA58(viewMode);
        /* sic: bit viewMode of the allowed targets, not bit targetKind
         * (F_0020EBA0, T394, tests the target): with T394's table only
         * traditional keeps a target it does not allow. */
        if (((modeInfo->targets >> viewMode) & 1) == 0)
            targetKind = modeInfo->defTarget;
        kinds[i].kind = viewMode;
        kinds[i].variant = targetKind;
        if (org != NULL && i == 0 && isDemoReplay(org) && org->backgroundMode == 0) {
            int demoViewMode, demoTarget;

            rng.next();
            F_0020E930(&demoViewMode, &demoTarget, rng.next01());
            kinds[0].kind = demoViewMode;
            kinds[0].variant = demoTarget;
            mode1Style = 0.0f;
        }
        setInput(i, NULL);
        setCamera(i, getDesignerCamera(i));
        getSelector(i)->reset();
        getTargetInfo(i)->init(org, car);
        F_0020E020(getSwitcher(i), org, car, getViewVariant(i));
        getOnBoardCamera(i)->virtual_8(getTargetInfo(i));
        getDesignerCamera(i)->setupOnRecord(getTargetInfo(i), -1, NULL, NULL, -1);
        getPasslineCamera(i)->setup(getTargetInfo(i), NULL);
        getStartingCamera(i)->setup(getTargetInfo(i), NULL);
        getAutomaticCamera(i)->virtual_8(getTargetInfo(i));
        getAutomountCamera(i)->virtual_8(getTargetInfo(i));
        getMatrixCamera(i)->virtual_8(getTargetInfo(i));
        getHeliCamera(i)->virtual_8(getTargetInfo(i));
        getLoggerCamera(i)->virtual_8(getTargetInfo(i));
        getZoomupCamera(i)->virtual_8(getTargetInfo(i));
        if (org != NULL) {
            RaceCourse *course = org->course;
            CourseCamera *courseCams;

            getOnBoardCamera(i)->virtual_8(getTargetInfo(i));
            if (org->race->virtual_5() == 0) {
                int mount = -1;

                switch (i) {
                case 0:
                    mount = getGameStatus()->getGameOptions()->onboardMount[0];
                    break;
                case 1:
                    mount = getGameStatus()->getGameOptions()->onboardMount[1];
                    break;
                }
                if (mount >= 0)
                    getOnBoardCamera(i)->setBasicMount(mount);
            }
            courseCams = course->data->replay;
            getDesignerCamera(i)->setupOnRecord(getTargetInfo(i), -1, course, courseCams->records + courseCams->count - 1, 0);
            getPasslineCamera(i)->setup(getTargetInfo(i), course);
            getStartingCamera(i)->setup(getTargetInfo(i), course);
            getAutomaticCamera(i)->setup(getTargetInfo(i), course, 0.0f);
            getAutomountCamera(i)->virtual_8(getTargetInfo(i));
            getMatrixCamera(i)->setup(getTargetInfo(i), 10.0f);
            getHeliCamera(i)->setupOnRecord(getTargetInfo(i), -1, NULL);
            getLoggerCamera(i)->virtual_8(getTargetInfo(i));
            getZoomupCamera(i)->setup(getTargetInfo(i), 0.0f);
            F_0020ED28(&correction, course->data->env);
        }
        F_0020EDE8(&view->request);
        F_0020EE28(&view->request, view->targetInfo.index[0], -1);
        F_0020EE50(&view->request, getCamera(i)->type);
        F_0020EE60(&view->request, getViewKind(i));
    }
}

// FUNC 0x00208B48 CameraManager::setViewCount
void CameraManager::setViewCount(int n)
{
    viewCount = n;
}

// FUNC 0x00208B58 CameraManager::setCurrentView
void CameraManager::setCurrentView(int view)
{
    current = view;
}

// FUNC 0x00208B68 CameraManager::getColourCorrection
/* out[0..1] lo / hi (the course's, or levelLo / levelHi), out[2..4] r, g, b
 * and out[5] the saturation (the course's, or colour / saturation). */
void CameraManager::getColourCorrection(const CourseEnv *env, f32 *out)
{
    out[2] = env->correctionColor[0];
    out[3] = env->correctionColor[1];
    out[4] = env->correctionColor[2];
    out[5] = env->correctionSaturation;
    out[0] = correction.cur[0];
    out[1] = correction.cur[1];
    if (levelOverride != 0) {
        out[0] = levelLo;
        out[1] = levelHi;
    }
    if (colourOverride != 0) {
        out[2] = colour[0];
        out[3] = colour[1];
        out[4] = colour[2];
        out[5] = saturation;
    }
}

// FUNC 0x00208BE8 CameraManager::update
/* Per frame.  The demo cameras run alone when on; else the marker clock and
 * the progress, then per view: the view mode (split screen: traditional;
 * the logger: its own), the target selector and chooser (not during the
 * countdown), the target, the pad's view mode change (posts
 * RACE_EVENT_VIEW_MODE), the mode handler, the camera's own update; then the
 * markers.  (Matching: the locals in retail's stack slot order; the kinds
 * read through CameraViewKind's inline getters; the target, selector and
 * chooser pointers set at the loop top (spilled as retail's); the input's
 * mode dispatched by a switch (== 0, < 0, < 4, as retail tests it).) */
void CameraManager::update(RaceOrganization *org, DynamicsConductor *dc)
{
    int countingDown = org->countDown > 0;
    int changed = 0;
    int loggerMode = org->loggerMode;
    int split = 0;
    int i;

    if (viewCount >= 2)
        split = getGameStatus()->getGameOptions()->replaySplit != 0;
    crossFade = 0;
    crossFadeTime = 0.5f;
    if (demoMode != 0) {
        updateDemoCameras(org, dc);
        return;
    }
    updateMarkerTime(org);
    progress.update(org);
    for (i = 0; i < viewCount; i++) {
        CameraView *view = &views[i];
        RaceInput *input = views[i].input;
        CameraTargetInfo *target = &view->targetInfo;
        CameraTargetSelector *selector = &view->selector;
        TargetChooser *chooser = &view->chooser;
        int replayMode = INPUT_LIVE;
        u32 pressed = 0;
        s16 viewMode;
        s16 viewVariant;
        VariableCameraBase *camera;

        if (input != NULL) {
            replayMode = input->replayMode;
            pressed = input->getButtonsOn();
        }
        if (split) {
            kinds[i].kind = VIEWMODE_TRADITIONAL;
            kinds[i].variant = 0;
        }
        if (loggerMode) {
            replayMode = 1;
            kinds[i].variant = 0;
            kinds[i].kind = VIEWMODE_LOGGER;
        }
        viewMode = kinds[i].getKind();
        target->kind = viewMode;
        viewVariant = kinds[i].getVariant();
        target->variant = viewVariant;
        F_0020EE10(&view->request);
        if (!countingDown) {
            selector->update(org, dc, views[i].camera->isEnableTargetChange() ^ 1);
            F_0020E1F0(chooser, org, target, selector, kinds[i].getVariant());
        }
        target->update(1);
        switch (replayMode) {
        case INPUT_LIVE:
            changed = updateStart(org, dc, input, i);
            if (i == 0)
                getGameStatus()->getGameOptions()->onboardMount[0] = getOnBoardCamera(0)->getMount();
            if (i == 1)
                getGameStatus()->getGameOptions()->onboardMount[1] = getOnBoardCamera(1)->getMount();
            break;
        case 1:
        case 2:
        case 3:
            if (loggerMode) {
                changed = updateLogger(org, dc, input, i);
            } else {
                if (!split) {
                    int oldMode = kinds[i].getKind();
                    int newMode = oldMode;

                    if (pressed & CAMBTN_VIEWMODE_PREV) {
                        newMode = newMode - 1;
                        if (newMode <= -1)
                            newMode = VIEWMODE_COUNT - 1;
                    }
                    if (pressed & CAMBTN_VIEWMODE_NEXT) {
                        newMode = newMode + 1;
                        if (newMode >= VIEWMODE_COUNT)
                            newMode = VIEWMODE_TRADITIONAL;
                    }
                    if (newMode != oldMode) {
                        int sub;

                        F_0020EBA0(this, org, i, newMode, getGameStatus()->getGameOptions()->target[newMode]);
                        sub = VIEWMODE_SUB_NONE;
                        if (newMode == VIEWMODE_AVANTGARDE) {
                            sub = VIEWMODE_SUB_STYLE_B;
                            if (mode1Style < 0.5f)
                                sub = VIEWMODE_SUB_STYLE_A;
                        }
                        org->eventQueue.post(RACE_EVENT_VIEW_MODE, (sub << 16) | (newMode << 8) | i, 0);
                    }
                }
                switch (kinds[i].getKind()) {
                case VIEWMODE_AVANTGARDE:
                    changed = updateMode1(org, dc, input, i);
                    break;
                case VIEWMODE_BROADCAST:
                    changed = updateMode2(org, dc, input, i);
                    break;
                case VIEWMODE_TRADITIONAL:
                default:
                    changed = updateMode0(org, dc, input, i);
                    break;
                }
            }
            break;
        }
        /* (a replayMode outside 0..3 keeps the previous view's `changed') */
        camera = views[i].camera;
        camera->updateFrame(org, input, target, &progress);
        if (camera->disableCrossOver() != 0)
            crossFade = 0;
        if (changed != 0)
            camera->doppler = 0;
        target->save();
    }
    frame++;
    updateMarkers(org);
}

// FUNC 0x00209130 CameraManager::begin2D
/* The GS state for 2D drawing: blending on, no depth test / lighting /
 * culling, a 640 x 448 orthographic projection, an identity modelview.
 * org and buf (RacePS2Base::drawView's) are not read. */
void CameraManager::begin2D(RaceOrganization *org, GSBuffer *buf)
{
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_CPU_LIGHTING);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_FLAG27);
    gfx_disable(PGL_CAP_COLOR_MATERIAL);
    gfx_disable(PGL_CAP_FLAG24);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    pgl_ortho(0.0f, SCREEN_W, SCREEN_H, 0.0f, -1.0f, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
}

// FUNC 0x002091E0 CameraManager::isOwnCar
/* The current view looks from car (mode 0 and car is its target). */
int CameraManager::isOwnCar(RaceEntryCar *car)
{
    CameraTargetInfo *target = &views[current].targetInfo;
    int result = 0;

    if (target->mode == 0)
        result = car == target->car[0];
    return result;
}

// FUNC 0x00209220 CameraManager::F_00209220
/* Slot 9 of the current camera (T397: 3; the onboard camera by its mount). */
int CameraManager::F_00209220()
{
    return views[current].camera->virtual_9();
}

// FUNC 0x00209268 CameraManager::isDrawBackMirror
int CameraManager::isDrawBackMirror()
{
    return views[current].camera->isDrawBackMirror();
}

// FUNC 0x002092B0 CameraManager::virtual_12
/* 1 unless the current camera's slot 9 is 0 and the view already follows
 * entry `car', or the onboard camera is in use and inside that car. */
int CameraManager::virtual_12(RaceOrganization *org, int car)
{
    int result = 1;
    VariableCameraBase *camera;

    if (F_00209220() == 0) {
        int index = views[current].targetInfo.index[0];
        result = index != car;
    }
    camera = views[current].camera;
    if (camera == &views[current].onboard) {
        if (static_cast<OnBoardCamera *>(camera)->isInsideCar(org, org->entries->cars[car]) != 0)
            result = 0;
    }
    return result;
}

// FUNC 0x00209370 CameraManager::F_00209370
/* virtual_12 for a car pointer. */
int CameraManager::F_00209370(RaceOrganization *org, RaceEntryCar *car)
{
    int result = 1;
    VariableCameraBase *camera;

    if (F_00209220() == 0)
        result = views[current].targetInfo.car[0] != car;
    camera = views[current].camera;
    if (camera == &views[current].onboard) {
        if (static_cast<OnBoardCamera *>(camera)->isInsideCar(org, car) != 0)
            result = 0;
    }
    return result;
}

// FUNC 0x00209420 CameraManager::getCameraKind
/* The current camera's slot 9 as 0 (0, 1), 1 (2) or 2 (anything else).
 * (Matching: retail tests == 2 (its block last), < 3, >= 0 one by one; a
 * `kind < 3 && kind >= 0' folds into one unsigned compare.) */
int CameraManager::getCameraKind()
{
    int result = 2;
    int kind = views[current].camera->virtual_9();

    if (kind != 2) {
        if (kind < 3) {
            if (kind >= 0)
                result = 0;
        }
    } else {
        result = 1;
    }
    return result;
}

// FUNC 0x00209498 CameraManager::F_00209498
/* frameEffectLevel clamped to 0..1. */
f32 CameraManager::F_00209498()
{
    f32 level = frameEffectLevel;

    if (level < 0.0f)
        level = 0.0f;
    if (level > 1.0f)
        level = 1.0f;
    return level;
}

// FUNC 0x002094D0 CameraManager::applyView
/* The screen's shift and, in a turned wall, its turn; then the camera's
 * slot 2.  (Matching: the explicit `case 0', as in setScreenLayout.) */
void CameraManager::applyView()
{
    f32 shiftX;

    switch (layoutFlags & WALL_SHIFT_MASK) {
    case WALL_SHIFT_PLUS:
        shiftX = shift * 0.5f;
        break;
    case WALL_SHIFT_MINUS:
        shiftX = -shift * 0.5f;
        break;
    case 0:
    default:
        shiftX = 0.0f;
        break;
    }
    pgl_translate(shiftX, 0.0f, 0.0f);
    if ((layoutFlags & WALL_TURN_MASK) == WALL_TURNED) {
        f32 stepDeg = 60.0f;

        if (layoutFlags & WALL_TURN_45)
            stepDeg = 45.0f;
        if (tileFromRight >= 2)
            vu0_rotate_y(-((f32)(tileFromRight - 1) * stepDeg));
        if (tileFromLeft >= 2)
            vu0_rotate_y((f32)(tileFromLeft - 1) * stepDeg);
    }
    views[current].camera->applyView();
}

/* The camera switches of the mode handlers (T387..T391): org and input are
 * passed by every caller but not used. */

// FUNC 0x00209660 CameraManager::F_00209660
int CameraManager::F_00209660(int view, RaceOrganization *org, RaceInput *input)
{
    views[view].camera = &views[view].starting;
    return 0;
}

// FUNC 0x00209680 CameraManager::F_00209680
int CameraManager::F_00209680(int view, RaceOrganization *org, RaceInput *input)
{
    views[view].camera = &views[view].passline;
    return 0;
}

// FUNC 0x002096A0 CameraManager::F_002096A0
int CameraManager::F_002096A0(int view, RaceOrganization *org, RaceInput *input)
{
    views[view].camera = &views[view].logger;
    return 0;
}

// FUNC 0x002096C0 CameraManager::F_002096C0
int CameraManager::F_002096C0(int view, RaceOrganization *org, RaceInput *input, int reinit)
{
    if (reinit) {
        OnBoardCamera *onboard = &views[view].onboard; /* (through the vtable) */

        onboard->virtual_8(&views[view].targetInfo);
    }
    views[view].camera = &views[view].onboard;
    return 0;
}

// FUNC 0x00209738 CameraManager::F_00209738
/* A random one of cam's mounts that look towards entry `car' (when there are
 * two or more; else mount 0).  (Matching: the pick lives in `mount', as
 * retail keeps both in one register.) */
void CameraManager::F_00209738(RaceOrganization *org, int car, OnBoardCamera *cam)
{
    int mounts[MOUNT_SEARCH_MAX];
    int mount = 0;
    int count = cam->findMountsToward(org, car, mounts, MOUNT_SEARCH_MAX,
                                      MOUNT_TOWARD_ANY_FLAGS, MOUNT_TOWARD_ALL_FLAGS);

    if (count >= 2) {
        rng.next();
        mount = (int)((f32)count * rng.next01()) % count;
        if (mount >= count)
            mount = 0;
        else
            mount = mounts[mount];
    }
    cam->setMount(mount);
}

// FUNC 0x00209808 CameraManager::F_00209808
/* A random one of cam's mounts in the directions dirMask (two or more; else
 * mount 0). */
void CameraManager::F_00209808(int dirMask, OnBoardCamera *cam)
{
    int mounts[MOUNT_SEARCH_MAX];
    int mount = 0;
    int count = cam->findMountsByDirection(dirMask, mounts, MOUNT_SEARCH_MAX,
                                           MOUNT_BY_DIR_ANY_FLAGS, MOUNT_BY_DIR_ALL_FLAGS);

    if (count >= 2) {
        rng.next();
        mount = (int)((f32)count * rng.next01()) % count;
        if (mount >= count)
            mount = 0;
        else
            mount = mounts[mount];
    }
    cam->setMount(mount);
}

// FUNC 0x002098D8 CameraManager::isWatchable
/* The car may be looked at: it is shown in full (car_display_level, T289). */
int CameraManager::isWatchable(RaceEntryCar *car)
{
    return car_display_level(car->state) == CAR_SHOW_FULL;
}

// FUNC 0x00209900 CameraManager::nextWatchableByRank
/* The next car in rank order (wrapping to the leader) that isWatchable
 * accepts; car itself when none.  entries->count is compared signed here. */
RaceEntryCar *CameraManager::nextWatchableByRank(RaceEntryBase *entries, RaceEntryCar *car)
{
    int count = entries->count;
    RaceEntryCar *next = car;
    int i;

    for (i = 0; i < count; i++) {
        next = next->rankNext;
        if (next == NULL) {
            next = entries->rankTop;
            if (next == NULL)
                continue;   /* quirk: the next pass reads NULL->rankNext */
        }
        if (isWatchable(next))
            return next;
    }
    return car;
}

// FUNC 0x002099A0 CameraManager::prevWatchableByRank
/* The same backwards (wrapping to the last car). */
RaceEntryCar *CameraManager::prevWatchableByRank(RaceEntryBase *entries, RaceEntryCar *car)
{
    int count = entries->count;
    RaceEntryCar *prev = car;
    int i;

    for (i = 0; i < count; i++) {
        prev = prev->rankPrev;
        if (prev == NULL) {
            prev = entries->rankEnd;
            if (prev == NULL)
                continue;   /* quirk: the next pass reads NULL->rankPrev */
        }
        if (isWatchable(prev))
            return prev;
    }
    return car;
}

// FUNC 0x00209A40 CameraManager::nextWatchableEntry
/* The first entry index from start on (wrapping) that isWatchable accepts;
 * start when none (the count signed, the modulo signed).  (Matching: retail
 * keeps the count twice, the loop bound and the divisor, in two registers:
 * two variables.) */
int CameraManager::nextWatchableEntry(RaceOrganization *org, int start)
{
    int count = org->entries->count;
    int n = count;
    int i;

    for (i = 0; i < count; i++) {
        int index = (start + i) % n;

        if (isWatchable(org->entries->cars[index]))
            return index;
    }
    return start;
}

// FUNC 0x00209B00 CameraManager::prevWatchableEntry
/* The same backwards. */
int CameraManager::prevWatchableEntry(RaceOrganization *org, int start)
{
    int count = org->entries->count;
    int n = count;
    int i;

    for (i = 0; i < count; i++) {
        int index = (start + count - i) % n;

        if (isWatchable(org->entries->cars[index]))
            return index;
    }
    return start;
}

// FUNC 0x00209BC8 CameraManager::F_00209BC8
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209BE0 CameraManager::F_00209BE0
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C00 CameraManager::setFarClip
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C08 CameraManager::setNearClip
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C10 CameraManager::F_00209C10
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C38 CameraManager::F_00209C38
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C68 CameraManager::F_00209C68
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209C98 CameraManager::F_00209C98
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209CC8 CameraManager::getPositionZ
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209D10 CameraManager::getPositionY
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209D58 CameraManager::getPositionX
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209DA0 CameraManager::F_00209DA0
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209DC0 CameraManager::F_00209DC0
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209DE8 CameraManager::virtual_11
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209E30 CameraManager::F_00209E30
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209E58 CameraManager::applyProjection
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209EA0 CameraManager::isTargetDistEnable
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209EE8 CameraManager::virtual_10
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209F10 CameraManager::F_00209F10
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209F30 CameraManager::setTargetCar
/* The current view follows car `car' (no blend). */
/* implicit: setCar (the inline body); inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209F68 CameraManager::F_00209F68
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209F78 CameraManager::F_00209F78
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209F88 CameraManager::F_00209F88
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FA0 CameraManager::getTileFromTop
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FB0 CameraManager::getTileFromBottom
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FC0 CameraManager::getTileFromLeft
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FD0 CameraManager::getTileFromRight
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FE0 CameraManager::setInput
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x00209FF8 CameraManager::getCurrentView
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A008 CameraManager::getViewCount
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A018 CameraManager::F_0020A018
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A020 CameraManager::F_0020A020
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A028 CameraManager::F_0020A028
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A030 CameraManager::F_0020A030
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A038 CameraManager::F_0020A038
/* One of the two colour correction overrides is on. */
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A060 CameraManager::F_0020A060
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A068 CameraManager::F_0020A068
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A078 CameraManager::getViewVariant
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A090 CameraManager::getViewKind
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A0A8 CameraManager::getCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A0C0 CameraManager::setCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A0D8 CameraManager::getInput
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A0F0 CameraManager::getView
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A108 CameraManager::getSwitcher
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A120 CameraManager::getSelector
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A138 CameraManager::getTargetInfo
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A150 CameraManager::getMatrixCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A168 CameraManager::getHeliCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A180 CameraManager::getLoggerCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A198 CameraManager::getZoomupCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A1B0 CameraManager::getAutomountCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A1C8 CameraManager::getAutomaticCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A1E0 CameraManager::getStartingCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A1F8 CameraManager::getPasslineCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A210 CameraManager::getDesignerCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x0020A228 CameraManager::getOnBoardCamera
/* inline in game/camera/T383_camera_manager.h; egcs emits this copy (#pragma implementation) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0020A240 __tf13CameraManager
/* Compiler-generated: the __si_type_info at 0x004B2960, base RaceCameraBase
 * (0x004B1200). */
extern "C" void *__tf13CameraManager(void)
{
    if (__ti13CameraManager[0] == 0) {
        __tf14RaceCameraBase();
        __rtti_si(__ti13CameraManager, "13CameraManager", (const type_info *)__ti14RaceCameraBase);
    }
    return __ti13CameraManager;
}

// FUNC 0x0020A290 __tf14RaceCameraBase
/* Compiler-generated: the __si_type_info at 0x004B1200, base CameraBase
 * (__ti10CameraBase 0x00353438, gp-0x45B8). */
extern "C" void *__tf14RaceCameraBase(void)
{
    if (__ti14RaceCameraBase[0] == 0) {
        __tf10CameraBase();
        __rtti_si(__ti14RaceCameraBase, "14RaceCameraBase", (const type_info *)__ti10CameraBase);
    }
    return __ti14RaceCameraBase;
}
#endif
