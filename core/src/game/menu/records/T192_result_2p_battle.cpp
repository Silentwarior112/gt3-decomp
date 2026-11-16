/* T192: Result2PBattle - the result board of a split screen battle: no
 * player (player = -1, every entry counts as one), leaves when ResultArcade's
 * timer runs out (the pad is ignored).
 */
/* Retail's .rodata (0x00327130..0x00327326): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor), the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: Result2PBattle, ResultTermBar, ResultTitleBar,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, RaceCameraBase, UnitArena<64>,
 * Operator: T117's set. */
// DATA_START 0x00327130
#include <new>
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen) */
#include "game/menu/records/T200_result_board.h"         /* ResultBoard::isDone */
#include "game/menu/records/T117_result_arcade.h"        /* base ResultArcade */
#include "game/menu/records/T192_result_2p_battle.h"
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/menu/records/results_internal.h"                           /* RESULT_FRAME_TIME */

// FUNC 0x00176690 Result2PBattle::start
//   ResultArcade's start, then no player's car.
void Result2PBattle::start(RaceOrganization &org, int replay)
{
    ResultArcade::start(org, replay);
    player = -1;
}

// FUNC 0x001766C0 Result2PBattle::isPlayer
//   Every entry is a player.
int Result2PBattle::isPlayer(int car)
{
    return 1;
}

// FUNC 0x001766C8 Result2PBattle::waitEnd
//   Once the board is in place the timer runs down; below zero posts
//   RACE_EVENT_END.  `skip' is ignored.
void Result2PBattle::waitEnd(RaceOrganization &org, int skip)
{
    if (board.isDone() == 0)
        return;
    timer = timer - RESULT_FRAME_TIME;
    if (timer < 0.0f)
        org.eventQueue.post(RACE_EVENT_END, 0, 0);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00176738 __tf14Result2PBattle
//   Builds the __si_type_info __ti14Result2PBattle (name 0x00327198, base
//   __ti12ResultArcade) on the first call.
extern "C" const type_info &__tf14Result2PBattle(void)
{
    if (__ti14Result2PBattle[0] == 0) {
        __tf12ResultArcade();
        __rtti_si(__ti14Result2PBattle, "14Result2PBattle",
                  (const type_info *)__ti12ResultArcade);
    }
    return *(const type_info *)__ti14Result2PBattle;
}
#endif

// FUNC 0x00176788 Result2PBattle::~Result2PBattle
/* implicit (game/menu/records/T192_result_2p_battle.h declares none); egcs emits this
 * copy for the vtable: ~ResultArcade inlined (board.~ResultBoard(), flags 2;
 * vptr _vt$14RaceResultBase), operator delete(this) when flags & 1 */
/* implicit: ~ResultBoard, operator delete (the base / member destructors,
 * flags & 1) */
