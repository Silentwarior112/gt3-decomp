/* T124_race_information.cpp - RaceInformation (0x00179B78..0x0017A038).
 *
 * The description of a race to set up (race_organization_create reads it):
 * the race database (a RaceDataBase member at +0), the option block, the
 * race code, the number of entries, the start countdown and the time limit.
 * The mode classes (RaceSinglePlayerInformation, RaceBattleInformationT<n>,
 * RaceSolitaireInformation, ...) override createEntries and the set-up
 * slots.  assignCars gives entries random cars of the race's car list
 * without repeating one, with the generator g_00351DE0.
 */
/* Retail's .rodata (0x00327468..0x0032762D): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the vtable, then the type names
 * in the reverse order of the classes' definitions: RaceInformation,
 * UnitArena<64>, the garage (GarageWheelList .. Cell<GarageCar>, GarageCar's
 * base list after its name) and RegurationResult: RegurationResult's header,
 * the GameStatus, the course (emitting UnitArena<64>'s name), then the own
 * header.  Random first: the declaration of g_00351DE0 in T124's header must
 * see a complete type, or egcs addresses it with %hi / %lo; retail uses
 * $gp. */
// DATA_START 0x00327468
#include <new>
#include "engine/pdistd/T531_random.h"               /* Random (g_00351DE0) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used) */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/race/core/T124_race_information.h"     /* T484 RaceDataBase / RaceEvent, T115 RaceOption */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (cars), RaceEntryCar (info) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, resetEntryOrder) */
#include "game/data/T489_car_database.h"         /* db_car_is_locked */
#include "game/menu/race_menu/T193_race_color_select.h"    /* race_pick_random_colors */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_user */
#include "runtime/cxx/T548_tinfo.h"
#endif

// FUNC 0x00179B78 RaceInformation::RaceInformation
//   7 callers (the mode classes' constructors).  RaceDataBase::RaceDataBase
//   (the vptr is stored first), the option block from the game options
//   (race_option_from_game_options; a throw there runs RaceDataBase::
//   ~RaceDataBase, flags 0), then the words.
RaceInformation::RaceInformation()
{
    /* implicit: RaceDataBase, ~RaceDataBase (the base's constructor and its
     * cleanup on a throw) */
    race_option_from_game_options(&option);
    raceCode = 0;                           /* sd */
    entryCount = 0;
    countDown = 0;
    timeLimit = 0;
}

// FUNC 0x00179BF0 RaceInformation::~RaceInformation
//   (slot 0.)  RaceDataBase::~RaceDataBase (flags 2), then operator delete
//   with flags bit 0.
RaceInformation::~RaceInformation()
{
    /* implicit: ~RaceDataBase, operator delete (the base / member destructors,
     * flags & 1) */
}

// FUNC 0x00179C48 RaceInformation::setRaceCode
//   13 callers.  Selects the race's record (RaceDataBase::select) and caps
//   the number of entries at the race's entry count (RaceEvent::entryCount,
//   compared signed).
void RaceInformation::setRaceCode(u64 code)
{
    raceCode = code;
    db.select(code);
    if ((int)db.event->entryCount < entryCount)
        entryCount = db.event->entryCount;
}

// FUNC 0x00179C90 RaceInformation::getRaceCode
//   (slot 1.)
u64 RaceInformation::getRaceCode()
{
    return raceCode;
}

// FUNC 0x00179C98 RaceInformation::getCourseCode
//   (slot 2.)
u64 RaceInformation::getCourseCode()
{
    return db.getCourse();
}

// FUNC 0x00179CB8 RaceInformation::setCourseCode
//   (slot 3.)
void RaceInformation::setCourseCode(u64 code)
{
    db.setCourse(code);
}

// FUNC 0x00179CD8 RaceInformation::createEntries
//   (slot 4.)  None here: the mode classes create them.
RaceEntryBase *RaceInformation::createEntries()
{
    return 0;
}

// FUNC 0x00179CE0 RaceInformation::getCountDownOr
//   No callers.  countDown < 0: 0 (none); 0: def; else countDown.
int RaceInformation::getCountDownOr(int def)
{
    int v = countDown;
    int result = 0;
    if (v >= 0)
        result = (v != 0) ? v : def;
    return result;
}

// FUNC 0x00179D00 RaceInformation::getCountDown
//   (slot 5.)
int RaceInformation::getCountDown()
{
    return countDown;
}

// FUNC 0x00179D08 RaceInformation::getTimeLimitOr
//   Caller: RaceArcadeDemoInformation::virtual_6.  timeLimit < 0: 0 (none);
//   0: def; else timeLimit.
int RaceInformation::getTimeLimitOr(int def)
{
    int v = timeLimit;
    int result = 0;
    if (v >= 0)
        result = (v != 0) ? v : def;
    return result;
}

// FUNC 0x00179D28 RaceInformation::getTimeLimit
//   (slot 6.)
int RaceInformation::getTimeLimit()
{
    return timeLimit;
}

// FUNC 0x00179D30 RaceInformation::setEntryCount
//   (slot 7.)
void RaceInformation::setEntryCount(int n)
{
    entryCount = n;
}

// FUNC 0x00179D38 RaceInformation::getEntryCount
//   (slot 8.)
int RaceInformation::getEntryCount()
{
    return entryCount;
}

// FUNC 0x00179D40 RaceInformation::setupEntry
//   (slot 10.)  Called for every entry race_organization_create adds;
//   nothing here.
void RaceInformation::setupEntry(RaceEntryCar *car)
{
}

// FUNC 0x00179D48 RaceInformation::setupOrganization
//   (slot 11.)
void RaceInformation::setupOrganization(RaceOrganization *org)
{
    org->resetEntryOrder();
}

// FUNC 0x00179D68 RaceInformation::setRandomSeed
//   Caller: RaceLinkBattleInformation::virtual_11, with F_0022DA08() (T473:
//   the sum of the link nodes' random bytes), before its assignCars: every
//   console of an i.LINK battle draws the same cars.
void RaceInformation::setRandomSeed(int seed)
{
    g_00351DE0.seed = seed;
}

// FUNC 0x00179D70 RaceInformation::assignCars
//   (slot 12.)  The n entries entries[0..n) of org get random cars of the
//   race's entry car list (RaceDataBase::setupEntryCar), each car once while
//   unused ones are left; the cars db_car_is_locked refuses count as used
//   from the start.  Then race_pick_random_colors(org, &g_00351DE0).
//   QUIRK: `left' counts down from all the cars, locked ones included: with
//   a locked car, once every free car is taken the search for an unused one
//   never ends.
void RaceInformation::assignCars(RaceOrganization *org, int *entries, int n)
{
    int cars = db.getNumEntryCars();
    if (n > 0) {
        int *used = (int *)__builtin_alloca(cars * sizeof(int));
        for (int i = 0; i < cars; i++)
            used[i] = db_car_is_locked(db.getEntryCar(i));
        int left = cars;
        for (int j = 0; j < n; j++) {
            CarEquipments *car = &org->entries->cars[entries[j]]->info;
            int k = (int)(g_00351DE0.next01() * (f32)cars);
            if (left != 0) {
                while (used[k] != 0)
                    k = (k + 1) % cars;
                used[k] = 1;
                left--;
            }
            db.setupEntryCar(k, car);
        }
        race_pick_random_colors(*org, &g_00351DE0);
    }
}

// FUNC 0x00179F28 RaceInformation::assignCarRange
//   (slot 13.)  assignCars (a virtual call) for the entries from .. to - 1.
void RaceInformation::assignCarRange(RaceOrganization *org, int from, int to)
{
    int n = to - from;
    if (n > 0) {
        int *entries = (int *)__builtin_alloca(n * sizeof(int));
        for (int i = 0; i < n; i++)
            entries[i] = i + from;
        assignCars(org, entries, n);
    }
}

// FUNC 0x00179FC0 RaceInformation::virtual_9
//   (slot 9.)  0 here; RaceChampionshipInformation returns entry i's points.
int RaceInformation::virtual_9(int i)
{
    return 0;
}

// FUNC 0x00179FD8 RaceInformation::getRaceDB
//   (slot 14.)  The database is the first member.
/* inline in game/race/core/T124_race_information.h; egcs emits this copy (after the
 * static initialiser, with the deferred functions) */

// FUNC 0x00179FE0 RaceInformation::getRaceOption
/* inline in game/race/core/T124_race_information.h; egcs emits this copy */

// FUNC 0x00179FE8 RaceInformation::setTimeLimit
/* inline in game/race/core/T124_race_information.h; egcs emits this copy */

// FUNC 0x00179FF0 RaceInformation::setCountDown
/* inline in game/race/core/T124_race_information.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x00179FF8 __tf15RaceInformation
//   Builds the __user_type_info __ti15RaceInformation on the first call.
extern "C" const type_info &__tf15RaceInformation(void)
{
    if (__ti15RaceInformation[0] == 0)
        __rtti_user(__ti15RaceInformation, "15RaceInformation");
    return *(const type_info *)__ti15RaceInformation;
}
#endif

/* The generator (.sdata 0x00351DE0, gp-0x5C10: a public object with a
 * constructor, zero in the file). */
Random g_00351DE0;                          /* 0x00351DE0 */

/* Compiler-generated: egcs's static initialiser of g_00351DE0 (.ctors entry
 * 0x0028F0E8), Random's inline constructor (seed 0x87654321), emitted after
 * the unit's last function (before the inline copies getRaceDB ..
 * setCountDown and __tf15RaceInformation); named after the
 * unit's first global, the constructor __15RaceInformation.  The marker is
 * the file's last: egcs gives the function the end of the file as its
 * stabs line, and tools/funcmap.py maps functions to markers by line. */
// FUNC 0x00179FC8 race_info_static_ctor (_GLOBAL_$I$__15RaceInformation)
/* implicit: Random (the static initialiser of g_00351DE0) */
