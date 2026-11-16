/* T189: ResultTimeTrial - the result screen of a time trial (held by
 * RaceTimeTrial at +0x2F9E0): the place the race time would take in the
 * time trial records of the GameStatus (T417, table `index').
 */
/* Retail's .rodata (0x00326960..0x00326C4E): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor), the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: ResultTimeTrial, ResultTermBar, ResultTitleBar,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, RaceCameraBase, UnitArena<64>,
 * GarageWheelList .. Cell<GarageCar> (the GameStatus header read first),
 * Operator: T120's set. */
// DATA_START 0x00326960
#include <new>
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (timeTrial) */
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* (seen, not used) */
#include "game/menu/records/T116_race_result_base.h"     /* RaceResultBase */
#include "game/menu/records/T119_result_implement_records.h" /* base ResultImplementRecords */
#include "game/menu/records/T189_result_time_trial.h"
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (lapTime) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries) */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime (total) */
#include "game/save/T417_time_trial_records.h"   /* TimeTrialRecords::findPlace */
#include "game/menu/records/results_internal.h"

// FUNC 0x00176200 ResultTimeTrial::setIndex
//   Caller: RaceTimeTrial::virtual_8.
void ResultTimeTrial::setIndex(int i)
{
    index = i;
}

// FUNC 0x00176208 ResultTimeTrial::checkRecords
//   The place the player's race time would take (-1: none).
int ResultTimeTrial::checkRecords(RaceOrganization &org)
{
    u32 time = org.entries->cars[player]->lapTime.total;
    return getGameStatus()->timeTrial.findPlace(index, time);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00176268 __tf15ResultTimeTrial
//   Builds the __si_type_info __ti15ResultTimeTrial (name 0x003269D8, base
//   __ti22ResultImplementRecords) on the first call.
extern "C" const type_info &__tf15ResultTimeTrial(void)
{
    if (__ti15ResultTimeTrial[0] == 0) {
        __tf22ResultImplementRecords();
        __rtti_si(__ti15ResultTimeTrial, "15ResultTimeTrial",
                  (const type_info *)__ti22ResultImplementRecords);
    }
    return *(const type_info *)__ti15ResultTimeTrial;
}
#endif

// FUNC 0x001762B8 ResultTimeTrial::~ResultTimeTrial
/* implicit (game/menu/records/T189_result_time_trial.h declares none); egcs emits this
 * copy for the vtable: ~RaceResultBase inlined (vptr _vt$14RaceResultBase),
 * operator delete(this) when flags & 1 */
/* implicit: operator delete (flags & 1) */
