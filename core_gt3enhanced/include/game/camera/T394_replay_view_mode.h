/* T394_replay_view_mode.h - the replay view modes (translation unit T394,
 * unit u094), 0x0020E930..0x0020ED28.
 *
 * Four view modes (GameOptions viewMode, T428; ReplayOption's "traditional"
 * .. "crs", T196), each with the camera targets it allows (g_002DF228); a view
 * keeps its mode and target in CameraManager +0x96E0 (CameraViewKind kind /
 * variant, T383).  Also the random choice of a mode for a demo replay, and
 * applying a mode and a target to a view.
 */
#ifndef GAME_T394_REPLAY_VIEW_MODE_H
#define GAME_T394_REPLAY_VIEW_MODE_H

#include "game/game.h"

class CameraManager;                        /* T383 (pointers only) */
class RaceOrganization;                     /* T125 */

/* What a view mode allows.  Not polymorphic.  Size 4 (the g_002DF228 stride,
 * F_0020EA58: index << 2).  u092 called it CameraKindInfo (mask,
 * defaultVariant). */
class ReplayViewInfo {
public:
    s8 targets;                             /* 0x0 bit t: camera target t (0 the car, 1 the leader, 2 auto) is allowed */
    s8 bgm;                                 /* 0x1 != 0: the mode has its own BGM (F_0020EBA0 turns replayBgm on) */
    s8 defTarget;                           /* 0x2 the target used when the chosen one is not allowed */
    s8 unk_3;                               /* 0x3 */
};
EGCS_SIZE(ReplayViewInfo, 0x4);

/* T394 .data (defined in T394): {1, 0, 0}, {4, 1, 2}, {7, 0, 2}, {1, 0, 0}. */
extern ReplayViewInfo g_002DF228[4];        /* 0x002DF228 (only T394 uses it) */

/* CameraManager::init: a random mode and target for a demo replay from r
 * (0..1): r < 0.33 mode 0 (target 0), r < 0.66 mode 2 (target (int)(r * 100)
 * % 3), else mode 1 (target 2); after three replays without mode 1 the next
 * one is mode 1 (MenuStatus replayCount / replayViewMode, T427). */
void F_0020E930(int *viewMode, int *target, f32 r);             /* 0x0020E930 */
/* CameraManager::init, GameOptions (T428): &g_002DF228[viewMode], mode 0 past
 * the table (no check below 0). */
ReplayViewInfo *F_0020EA58(int viewMode);                       /* 0x0020EA58 */
ReplayViewInfo *F_0020EA78(CameraManager *cm, int viewMode);    /* 0x0020EA78 the same (cm not used; ReplayOption) */
/* No callers.  The next target of view's mode allowed after its current one,
 * going down (dir 0) or up, wrapping 0..2; the current one if none is. */
int  F_0020EA98(CameraManager *cm, int dir, int view);          /* 0x0020EA98 */
/* ReplayOption: F_0020EBA0 with the game options' viewMode and target. */
void F_0020EB38(CameraManager *cm, RaceOrganization *org, int view);    /* 0x0020EB38 */
/* ReplayOption, CameraManager::update, F_0020EB38.  Sets view's mode and
 * target (clamped to the table; a target the mode does not allow becomes its
 * defTarget), sets cm's targetReset[view]; modes 0, 1, 3 take the view's car
 * again without a blend; restarts the view's TargetChooser, then
 * cm->resetState(), and stores both in the game options. */
void F_0020EBA0(CameraManager *cm, RaceOrganization *org, int view, int viewMode,
                int target);                                    /* 0x0020EBA0 */

#endif /* GAME_T394_REPLAY_VIEW_MODE_H */
