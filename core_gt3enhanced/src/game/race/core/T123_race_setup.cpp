/* T123_race_setup.cpp - the race object pool and the RaceOrganization
 * factory (0x00179878..0x00179B78).
 *
 * g_002B3BF0 is a pdistd::Allocator; game_init gives it its memory
 * (race_pool_assign) and the race classes' operator new / delete use it.
 * race_organization_create turns a RaceInformation into a ready
 * RaceOrganization: course, race database, entries, options, codes, time
 * limit and the cars.
 */
/* Retail's .rodata (0x00327440..0x00327461, no code reference): "bad_alloc"
 * (<new>'s inline what(), written when the header is read) and the type
 * name of UnitArena<64> only. */
// DATA_START 0x00327440
#include <new>
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (new RaceCourse) */
#include "game/race/core/T123_race_setup.h"           /* T530 pdistd::Allocator */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T124_race_information.h"     /* RaceInformation; T484 RaceDataBase, T115 RaceOption */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (race_organization_create, the set-up slots) */

/* The race object pool (.data 0x002B3BF0, 0x18 bytes: a public object with a
 * constructor, zero in the file).  egcs generates its static initialiser
 * and destructor, named after the unit's first global, g_002B3BF0, and
 * emits them at the end of the unit, the destructor first.  (The markers
 * sit at the lines egcs gives the two functions in its stabs: the
 * initialiser's is this definition, the destructor's the end of the file;
 * tools/funcmap.py maps functions to markers by line.) */
// FUNC 0x00179B50 race_pool_static_ctor (_GLOBAL_$I$g_002B3BF0)
//   The .ctors entry (0x0028F0E4): pdistd::Allocator::Allocator(0).
pdistd::Allocator g_002B3BF0(0);            /* 0x002B3BF0 */

// FUNC 0x00179878 race_pool_assign
//   Caller: game_init (T007).  Returns what pdistd::Allocator::assign
//   returns (the previous arena).
void *race_pool_assign(void *base, u32 size)
{
    return g_002B3BF0.assign(base, size);
}

// FUNC 0x001798A0 race_organization_create
//   13 callers (the race set-up of every mode).  The new-expressions give
//   their memory back when a constructor throws (RaceCourse::operator
//   delete(p, 0x14), operator delete, RaceOrganization::operator delete(p,
//   0x8C8)).  A refused entry (addEntry -1) is deleted (virtual destructor).
RaceOrganization *race_organization_create(RaceInformation *info)
{
    RaceCourse *course = new RaceCourse;    /* RaceCourse::operator new(0x14) */
    RaceDataBase *db = new RaceDataBase;    /* operator new(0x28) */
    /* implicit: operator new, operator delete (new RaceDataBase and the
     * landing pads) */
    RaceEntryBase *entries = info->createEntries();
    RaceOrganization *org = new RaceOrganization;   /* RaceOrganization::operator new(0x8C8) */

    org->setRaceDB(db);
    org->option = info->option;
    org->setEntries(entries);
    org->setCourse(course);
    org->setRaceCode(info->getRaceCode());
    org->setCourseCode(info->getCourseCode());
    org->timeLimit = info->getTimeLimit();
    int n = info->getEntryCount();
    for (int i = 0; i < n; i++) {           /* (loop.c reverses it: bnel at the bottom) */
        RaceEntryCar *car = entries->createEntry();
        if (org->entries->addEntry(car) != -1)
            info->setupEntry(car);
        else if (car != 0)
            delete car;
    }
    info->setupOrganization(org);
    org->updateColorIndices();              /* F_0017A900 */
    return org;
}

/* Compiler-generated: egcs's static destructor of the pool (its initialiser:
 * see g_002B3BF0 above). */
// FUNC 0x00179B28 race_pool_static_dtor (_GLOBAL_$D$g_002B3BF0)
//   The .dtors entry (0x0028F20C): pdistd::Allocator::~Allocator (flags 2).
/* implicit: ~Allocator (the static destructor of g_002B3BF0) */
