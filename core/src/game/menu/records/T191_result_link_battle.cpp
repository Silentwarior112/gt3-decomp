/* T191: ResultLinkBattle - the result board of an i.LINK battle.  When the
 * first car reaches the goal the others get 20 s (waitTimer); then the
 * board finishes by itself and leaves 20 s later (the pad is ignored).
 */
/* Retail's .rodata (0x00326F38..0x0032712E): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor), the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: ResultLinkBattle, ResultTermBar, ResultTitleBar,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, RaceCameraBase, UnitArena<64>,
 * Operator: T117's set. */
// DATA_START 0x00326F38
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
#include "game/menu/records/T191_result_link_battle.h"
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/menu/records/results_internal.h"                           /* RESULT_FRAME_TIME */

// FUNC 0x00176478 ResultLinkBattle::start
//   ResultArcade's start; no goal yet, waitTimer and timer 20 s.
void ResultLinkBattle::start(RaceOrganization &org, int replay)
{
    ResultArcade::start(org, replay);
    started = 0;
    timer = 20.0f;
    waitTimer = 20.0f;
}

// FUNC 0x001764B0 ResultLinkBattle::update
//   ResultArcade's update; after the first goal waitTimer runs down, and
//   once it is below zero finish() (slot 6) unless done.
void ResultLinkBattle::update(RaceOrganization &org, Control *pad)
{
    ResultArcade::update(org, pad);
    if (started == 0)
        return;
    waitTimer = waitTimer - RESULT_FRAME_TIME;
    if (waitTimer < 0.0f && done == 0)
        finish(org);
}

// FUNC 0x00176530 ResultLinkBattle::waitEnd
//   Once the board is in place: the timer runs when every car is in or the
//   wait is over; below zero posts RACE_EVENT_END.  `skip' is ignored.
void ResultLinkBattle::waitEnd(RaceOrganization &org, int skip)
{
    if (board.isDone() == 0)
        return;
    if (allFinished != 0 || waitTimer < 0.0f)
        timer = timer - RESULT_FRAME_TIME;
    if (timer < 0.0f)
        org.eventQueue.post(RACE_EVENT_END, 0, 0);
}

// FUNC 0x001765C8 ResultLinkBattle::goal
//   Starts the wait, then ResultArcade's goal.
void ResultLinkBattle::goal(RaceOrganization &org, int car)
{
    started = 1;
    ResultArcade::goal(org, car);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001765E8 __tf16ResultLinkBattle
//   Builds the __si_type_info __ti16ResultLinkBattle (name 0x00326FA0, base
//   __ti12ResultArcade) on the first call.
extern "C" const type_info &__tf16ResultLinkBattle(void)
{
    if (__ti16ResultLinkBattle[0] == 0) {
        __tf12ResultArcade();
        __rtti_si(__ti16ResultLinkBattle, "16ResultLinkBattle",
                  (const type_info *)__ti12ResultArcade);
    }
    return *(const type_info *)__ti16ResultLinkBattle;
}
#endif

// FUNC 0x00176638 ResultLinkBattle::~ResultLinkBattle
/* implicit (game/menu/records/T191_result_link_battle.h declares none); egcs emits this
 * copy for the vtable: ~ResultArcade inlined (board.~ResultBoard(), flags 2;
 * vptr _vt$14RaceResultBase), operator delete(this) when flags & 1 */
/* implicit: ~ResultBoard, operator delete (the base / member destructors,
 * flags & 1) */
