/* T123_race_setup.h - the race object pool and the RaceOrganization factory
 * (translation unit T123, unit u042), 0x00179878..0x00179B78.
 *
 * g_002B3BF0 is a pdistd::Allocator: main gives it its memory
 * (race_pool_assign, from F_00102C00) and the class-specific operator new /
 * delete of RaceEntryCar, RaceEntryBase, RaceEntryPS2, RaceOrganization,
 * RaceBase, RacePS2Base, RaceBGMPS2, RaceBGMBase and the race modes allocate
 * from it (16-byte aligned).  race_organization_create turns a
 * RaceInformation (T124) into a ready RaceOrganization (T125).
 */
#ifndef GAME_T123_RACE_SETUP_H
#define GAME_T123_RACE_SETUP_H

#include "game/game.h"
#include "engine/pdistd/T530_allocator.h"            /* pdistd::Allocator (the pool, held by value) */

class RaceOrganization;                     /* T125 (pointers only: forward declarations) */
class RaceInformation;                      /* T124 */

/* The race object pool (.data, 0x18 bytes, zero in the file: T123 defines it,
 * a public object with a constructor).  Built by T123's static constructor. */
extern pdistd::Allocator g_002B3BF0;        /* 0x002B3BF0 */

/* pdistd::Allocator::assign(base, size) on the pool; returns what it returns. */
void *race_pool_assign(void *base, u32 size);                       /* 0x00179878 */
/* new RaceCourse, new RaceDataBase, info->createEntries(), new
 * RaceOrganization; then the DB, the option block, the entries, the course,
 * the race / course codes, the time limit; info->getEntryCount() entries
 * (createEntry / addEntry, info->setupEntry; a refused one is deleted);
 * info->setupOrganization(org); org->updateColorIndices().  13 callers (the
 * race set-up of every mode). */
RaceOrganization *race_organization_create(RaceInformation *info);  /* 0x001798A0 */
/* 0x00179B28 (.dtors 0x0028F20C) / 0x00179B50 (.ctors 0x0028F0E4): egcs's
 * static destructor and initialiser of the pool (T123 defines it):
 * _GLOBAL_$D$g_002B3BF0 (~Allocator, flags 2) and _GLOBAL_$I$g_002B3BF0
 * (Allocator(0)); their FUNC markers name them race_pool_static_dtor /
 * _ctor.  T129 has a pair of its own (race_mem_pool_static_dtor / _ctor). */

#endif /* GAME_T123_RACE_SETUP_H */
