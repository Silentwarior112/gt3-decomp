/* T116 - RaceResultBase: the end-of-race result sequence of a race mode
 * (game/menu/records/T116_race_result_base.h).  0x00174B48..0x00174D50.
 *
 * The race mode keeps one (ResultArcade, ResultImplementRecords, ...) and
 * drives it: start, then update every frame with the race organization and
 * the pad.  update takes the goal events (RACE_EVENT_GOAL, the car's entry
 * number as argument) of the organization's event queue: goal(car) for
 * each, and finish() once the player's car is among them.  T116 also holds
 * the out-of-line copies of the class's inline members (0x00174C90..
 * 0x00174D10, after the out-of-line methods). */
/* Retail's .rodata (0x00325ED8..0x00325F78): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the vtable, then the type names
 * in the reverse order of the classes' definitions: RaceResultBase,
 * RaceCameraBase, UnitArena<64>. */
// DATA_START 0x00325ED8
#include <new>
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (rankTop), RaceEntryCar (rankNext, number) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue (counts, findNth), RACE_EVENT_GOAL */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, eventQueue) */
#include "game/menu/records/T116_race_result_base.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_user */
#endif

/* index.txt: RaceResultBase::virtual_1 (slot 1). */
// FUNC 0x00174B48 RaceResultBase::start
void RaceResultBase::start(RaceOrganization &org, int replay)
{
    done = 0;
}

/* index.txt: RaceResultBase::virtual_3 (slot 3).  For each pending goal
 * event (counts[RACE_EVENT_GOAL] of the queue, read once): its car,
 * goal(org, car) (virtual), and when it is the player's car (virtual
 * isPlayer) and the sequence has not started yet, finish(org) (virtual).
 * `pad' is not used here.  Callers: ResultArcade::update,
 * ResultChampionship::update, ResultImplementRecords::update. */
// FUNC 0x00174B50 RaceResultBase::update
void RaceResultBase::update(RaceOrganization &org, Control *pad)
{
    int pending = org.eventQueue.counts[RACE_EVENT_GOAL];

    for (int nth = 0; nth < pending; nth++) {
        int car = 0;                        /* sp+0x0, cleared before every search */

        if (org.eventQueue.findNth(RACE_EVENT_GOAL, &car, nth) != 0) {
            goal(org, car);
            if (isPlayer(car) && done == 0)
                finish(org);
        }
    }
}

/* index.txt: RaceResultBase::virtual_7 (slot 7 of every result class). */
// FUNC 0x00174C30 RaceResultBase::isPlayer
int RaceResultBase::isPlayer(int car)
{
    return car == player;
}

/* index.txt: RaceResultBase::virtual_6 (slot 6). */
// FUNC 0x00174C40 RaceResultBase::finish
void RaceResultBase::finish(RaceOrganization &org)
{
    done = 1;
}

/* The player's place in the rank order list of the entries (0 = first),
 * -1 if the player's car is not in it.  Caller: ResultChampionship::
 * update (T118). */
// FUNC 0x00174C50 RaceResultBase::getPlayerRank
int RaceResultBase::getPlayerRank(RaceOrganization &org)
{
    int rank = 0;

    for (RaceEntryCar *car = org.entries->rankTop; car != NULL; car = car->rankNext, rank++) {
        if (car->number == player)
            return rank;
    }
    return -1;
}

// FUNC 0x00174C90 RaceResultBase::goal
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (slot 5,
 * also of the ResultImplementRecords family): nothing */

// FUNC 0x00174C98 RaceResultBase::isStartResultSequence
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (no callers):
 * done */

// FUNC 0x00174CA0 RaceResultBase::getCarNumber
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (no callers):
 * player */

// FUNC 0x00174CA8 RaceResultBase::setCarNumber
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (no callers):
 * player = car */

// FUNC 0x00174CB0 RaceResultBase::render
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (slot 4):
 * nothing */

// FUNC 0x00174CB8 RaceResultBase::cleanup
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (slot 2):
 * nothing */

// FUNC 0x00174CC0 RaceResultBase::~RaceResultBase
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (slot 0):
 * the vptr, then operator delete (0x0027C0C0) when flags & 1 */
/* implicit: operator delete (flags & 1) */

// FUNC 0x00174CF0 RaceResultBase::RaceResultBase
/* inline in game/menu/records/T116_race_result_base.h; egcs emits this copy (no callers:
 * the race modes expand it): done = 0, the vptr, player = 0 */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (no bases: __rtti_user; the node is
 * in .sbss at gp-0x4698).  Callers: __tf12ResultArcade,
 * __tf22ResultImplementRecords. */
// FUNC 0x00174D10 __tf14RaceResultBase
extern "C" void *__tf14RaceResultBase(void)
{
    if (__ti14RaceResultBase[0] == 0)
        __rtti_user(__ti14RaceResultBase, "14RaceResultBase");
    return __ti14RaceResultBase;
}
#endif
