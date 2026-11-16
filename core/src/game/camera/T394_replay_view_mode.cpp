/* T394 - the replay view modes, 0x0020E930..0x0020ED28.
 *
 * Four view modes (GameOptions viewMode; ReplayOption's traditional,
 * avantgarde, broadcast, crs, T196), each with the camera targets it allows
 * (g_002DF228); a view keeps its mode and target in CameraManager::kinds[]
 * (kind / variant).
 *
 * Retail T394's .data opens with g_002DF228 (defined here); its .rodata
 * (0x0033F4A0, nothing references it: DATA_START) has <new>'s "bad_alloc",
 * the garage names (the GameStatus, T429), 14RaceCameraBase and
 * t9UnitArena1Ui64: UnitArena<64> (T153) defined first, then RaceCameraBase
 * (T383), then the garage classes.
 */
// DATA_START 0x0033F4A0
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getGameOptions / getMenuStatus
                                               (the garage names) */
#include "game/camera/camera_base_internal.h"
#include "game/camera/T394_replay_view_mode.h"
#include "game/camera/T393_target_chooser.h"       /* F_0020E020 */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo (setCar, refresh, finishBlend) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView, CameraViewKind */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus (replayViewMode, replayCount) */
#include "game/save/T428_game_options.h"         /* GameOptions (replayBgm, viewMode, target[]) */

/* The targets each view mode allows (bit t: target t), whether it has its own
 * BGM, and its default target. */
ReplayViewInfo g_002DF228[4] = {            /* 0x002DF228 */
    { 1, 0, 0, 0 },                         /* traditional: manual */
    { 4, 1, 2, 0 },                         /* avantgarde: auto, its own BGM */
    { 7, 0, 2, 0 },                         /* broadcast: every target, auto */
    { 1, 0, 0, 0 },                         /* crsline: manual */
};

/* Caller: CameraManager::init (T383), for a demo replay.  A random view mode
 * and target from draw (0..1): draw < 0.33 traditional (manual), draw < 0.66
 * broadcast (the target (int)(draw * 100) % 3), else avantgarde (auto); the
 * fourth demo replay since the last avantgarde one is avantgarde. */
// FUNC 0x0020E930 F_0020E930
void F_0020E930(int *viewMode, int *target, f32 draw)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    int count = menu->replayCount + 1;

    if (draw < 0.33f)
        *viewMode = VIEW_MODE_TRADITIONAL;
    else if (draw < 0.66f)
        *viewMode = VIEW_MODE_BROADCAST;
    else
        *viewMode = VIEW_MODE_AVANTGARDE;
    if (count >= 4)
        *viewMode = VIEW_MODE_AVANTGARDE;

    switch (*viewMode) {
    case VIEW_MODE_TRADITIONAL:
        *viewMode = VIEW_MODE_TRADITIONAL;
        *target = VIEW_TARGET_MANUAL;
        break;
    case VIEW_MODE_BROADCAST: {
        *viewMode = VIEW_MODE_BROADCAST;
        switch ((int)(draw * 100.0f) % 3) {
        case 0:
            *target = VIEW_TARGET_MANUAL;
            break;
        case 1:
            *target = VIEW_TARGET_TOP;
            break;
        default:
            *target = VIEW_TARGET_AUTO;
            break;
        }
        break;
    }
    default:
        *viewMode = VIEW_MODE_AVANTGARDE;
        *target = VIEW_TARGET_AUTO;
        menu->replayCount = 0;
        break;
    }
    menu->replayViewMode = *viewMode;
    menu->replayCount = menu->replayCount + 1;
}

/* Callers: CameraManager::init, GameOptions::setDefaults (T428).  The modes
 * past the table read mode 0 (no check below 0). */
// FUNC 0x0020EA58 F_0020EA58
ReplayViewInfo *F_0020EA58(int viewMode)
{
    if (viewMode >= VIEW_MODE_COUNT)
        viewMode = VIEW_MODE_TRADITIONAL;
    return &g_002DF228[viewMode];
}

/* Callers: ReplayOption (T196).  The same; cm is not used. */
// FUNC 0x0020EA78 F_0020EA78
ReplayViewInfo *F_0020EA78(CameraManager *cm, int viewMode)
{
    if (viewMode >= VIEW_MODE_COUNT)
        viewMode = VIEW_MODE_TRADITIONAL;
    return &g_002DF228[viewMode];
}

/* No callers.  The next target of view's mode allowed after its current
 * one, going down (dir 0) or up, wrapping 0..2; the current one if none of
 * the next three is allowed.  kind / variant through CameraViewKind's inline
 * getters: retail builds &kinds[view] anew for each read and loads +4 / +6
 * from it (a direct field access folds 0x96E4 into the load). */
// FUNC 0x0020EA98 F_0020EA98
int F_0020EA98(CameraManager *cm, int dir, int view)
{
    s8 allowed = g_002DF228[cm->kinds[view].getKind()].targets;
    int t = cm->kinds[view].getVariant();
    int tries;

    for (tries = 0; tries < VIEW_TARGET_COUNT; tries++) {
        if (dir != 0) {
            t = t + 1;
            if (t >= VIEW_TARGET_COUNT)
                t = 0;
        } else {
            t = t - 1;
            if (t < 0)
                t = VIEW_TARGET_COUNT - 1;
        }
        if ((allowed >> t) & 1)
            return t;
    }
    return cm->kinds[view].getVariant();
}

/* Caller: ReplayOption::update (T196).  Applies the game options' view mode
 * and its target to view. */
// FUNC 0x0020EB38 F_0020EB38
void F_0020EB38(CameraManager *cm, RaceOrganization *org, int view)
{
    GameOptions *options = getGameStatus()->getGameOptions();
    F_0020EBA0(cm, org, view, options->viewMode, options->target[options->viewMode]);
}

/* Callers: ReplayOption (T196), CameraManager::update (T383), F_0020EB38.
 * Sets view's mode and target (clamped to the table; a target the mode does
 * not allow becomes its defTarget) and its targetReset; traditional,
 * avantgarde and crsline take the view's car again without a blend; restarts
 * the view's TargetChooser and the camera state, and stores both in the game
 * options (a mode with its own BGM turns the replay BGM on).
 *
 * The parameters are clamped in place: retail's `x = 0; slt -1 < param;
 * movn x, param' (a ternary into a new variable is a MAX_EXPR that compares
 * the copy, `x = param; movz x, 0').  Open (99.0%): retail computes
 * &options->target[viewMode] before the viewMode store; here sched1 puts the
 * store first (it is viewMode's last use). */
// FUNC 0x0020EBA0 F_0020EBA0
void F_0020EBA0(CameraManager *cm, RaceOrganization *org, int view, int viewMode,
                int target)
{
    viewMode = (viewMode < 0) ? 0 : viewMode;
    target = (target < 0) ? 0 : target;
    if (viewMode >= VIEW_MODE_COUNT)
        viewMode = VIEW_MODE_TRADITIONAL;
    if (target >= VIEW_TARGET_COUNT)
        target = VIEW_TARGET_MANUAL;

    ReplayViewInfo *info = &g_002DF228[viewMode];
    if (((info->targets >> target) & 1) == 0)
        target = info->defTarget;
    int ownBgm = info->bgm != 0;
    CameraTargetInfo *ti = &cm->views[view].targetInfo;

    cm->kinds[view].kind = viewMode;
    cm->kinds[view].variant = target;
    cm->targetReset[view] = 1;
    /* the `viewMode >= 0' test is in the code, though viewMode is clamped above */
    if (viewMode >= 0 && (viewMode < VIEW_MODE_BROADCAST || viewMode == VIEW_MODE_CRSLINE)) {
        ti->setCar(ti->index[0], 1);
        ti->refresh();
        ti->finishBlend();
    }
    F_0020E020(&cm->views[view].chooser, org, ti->index[0], target);
    cm->resetState();

    GameOptions *options = getGameStatus()->getGameOptions();
    options->viewMode = viewMode;
    options->target[viewMode] = target;
    if (ownBgm)
        options->replayBgm = 1;
}
