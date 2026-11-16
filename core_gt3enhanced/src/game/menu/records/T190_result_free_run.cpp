/* T190: ResultFreeRun - the result screen of a free run (held by RaceFreeRun
 * at +0x21800): the place the best lap would take in the free run records
 * of the GameStatus (T416) under the course name.
 */
/* Retail's .rodata (0x00326C50..0x00326F36): "bad_alloc" (<new>'s inline
 * what(), written when the header is read; unreferenced: the anchor), the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: ResultFreeRun, ResultTermBar, ResultTitleBar,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, RaceCameraBase, UnitArena<64>,
 * GarageWheelList .. Cell<GarageCar> (the GameStatus header read first),
 * Operator: T120's set. */
// DATA_START 0x00326C50
#include <new>
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (freeRun) */
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
#include "game/menu/records/T190_result_free_run.h"
#include "libc.h"                           /* strcpy */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (lapTime) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries) */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime::getBest */
#include "game/save/T416_free_run_records.h"     /* FreeRunRecords::findTime */
#include "game/menu/records/results_internal.h"

// FUNC 0x001762E8 ResultFreeRun::start
//   ResultImplementRecords' start, then a 0.1 s delay and a 15 s timer.
void ResultFreeRun::start(RaceOrganization &org, int replay)
{
    ResultImplementRecords::start(org, replay);
    delay = 0.1f;
    timer = 15.0f;
}

// FUNC 0x00176320 ResultFreeRun::setCourse
//   Caller: RaceFreeRun::RaceFreeRun.  NULL clears the name (no length
//   check).
void ResultFreeRun::setCourse(const char *name)
{
    if (name != NULL)
        strcpy(course, name);
    else
        course[0] = 0;
}

// FUNC 0x00176350 ResultFreeRun::checkRecords
//   The place the best lap (getRecord) would take in the course's records
//   (-1: none).
int ResultFreeRun::checkRecords(RaceOrganization &org)
{
    FreeRunRecords *records = &getGameStatus()->freeRun;
    return records->findTime(course, getRecord(org));
}

// FUNC 0x001763C0 ResultFreeRun::getRecord
//   The player's best lap.
int ResultFreeRun::getRecord(RaceOrganization &org)
{
    return org.entries->cars[player]->lapTime.getBest(-1);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x001763F8 __tf13ResultFreeRun
//   Builds the __si_type_info __ti13ResultFreeRun (name 0x00326CC8, base
//   __ti22ResultImplementRecords) on the first call.
extern "C" const type_info &__tf13ResultFreeRun(void)
{
    if (__ti13ResultFreeRun[0] == 0) {
        __tf22ResultImplementRecords();
        __rtti_si(__ti13ResultFreeRun, "13ResultFreeRun",
                  (const type_info *)__ti22ResultImplementRecords);
    }
    return *(const type_info *)__ti13ResultFreeRun;
}
#endif

// FUNC 0x00176448 ResultFreeRun::~ResultFreeRun
/* implicit (game/menu/records/T190_result_free_run.h declares none); egcs emits this copy
 * for the vtable: ~RaceResultBase inlined (vptr _vt$14RaceResultBase),
 * operator delete(this) when flags & 1 */
/* implicit: operator delete (flags & 1) */
