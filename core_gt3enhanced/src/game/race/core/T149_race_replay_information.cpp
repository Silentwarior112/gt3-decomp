/* T149 - RaceReplayInformation (u049).
 *
 * The race information of a saved race (a replay or a ghost): built on
 * the stack by play_saved_replay (T000) / logger_main (T055) from the
 * RaceInfoBlock at the head of the save (RaceBase::virtual_7 wrote it).
 * The race / course codes, the option block, the car count and every car
 * come from the block; the race object itself is created by its class name
 * (raceName) through the race class registry.
 *
 * Data (u049's data pass): retail's .rodata 0x0032D1F0..0x0032D2C0 is
 * "bad_alloc" (<new>, not referenced: an anchor), the vtable, then the type
 * names RaceReplayInformation, RaceCameraBase, UnitArena<64>: UnitArena<64>
 * (T153, emitted) and RaceCameraBase (T383) defined first.  No race display
 * (no "" in .sdata).
 */
// DATA_START 0x0032D1F0
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (egcs's <new> and cxxrt.h conflict) */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, the last one */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase: its name before T149's own) */
#include "game/race/core/T149_race_replay_information.h"
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (setupEntry, race_entry_load_slot) */
#include "game/race/core/T130_race_ps2_base.h"        /* RaceEntryPS2 (createEntries) */
#include "game/race/core/T129_race_base.h"            /* race_create_by_name, race_destroy */
#include "game/race/display/race_license_internal.h"

// FUNC 0x0018D1D8 RaceReplayInformation::RaceReplayInformation
//   RaceInformation, blk (stored before the vptr), the vptr (+0x54); the
//   race code from the block (getRaceCode is called directly) and the
//   option block (0x18 bytes, copied with unaligned double-word moves).
RaceReplayInformation::RaceReplayInformation(RaceInfoBlock *block)
    : blk(block)
{
    /* implicit: RaceInformation (base constructor), ~RaceInformation (exception cleanup) */
    setRaceCode(RaceReplayInformation::getRaceCode());
    option = blk->option;
}

// FUNC 0x0018D290 RaceReplayInformation::~RaceReplayInformation
//   (index.txt: virtual_0) The vptr, then ~RaceInformation with the
//   caller's flags (the deleting form is there).
RaceReplayInformation::~RaceReplayInformation()
{
    /* implicit: ~RaceInformation (base destructor) */
}

// FUNC 0x0018D2B8 RaceReplayInformation::getRaceCode
//   (index.txt: virtual_1)
u64 RaceReplayInformation::getRaceCode()
{
    return blk->raceCode;
}

// FUNC 0x0018D2C8 RaceReplayInformation::getCourseCode
//   (index.txt: virtual_2)
u64 RaceReplayInformation::getCourseCode()
{
    return blk->courseCode;
}

// FUNC 0x0018D2D8 RaceReplayInformation::createEntries
//   (index.txt: virtual_4) new RaceEntryPS2 (its pool operator new, 0x3C
//   bytes; operator delete(p, 0x3C) if the constructor throws).
RaceEntryBase *RaceReplayInformation::createEntries()
{
    return new RaceEntryPS2; /* implicit: RaceEntryPS2::operator delete (if the constructor throws) */
}

// FUNC 0x0018D330 RaceReplayInformation::getEntryCount
//   (index.txt: virtual_8)
int RaceReplayInformation::getEntryCount()
{
    return blk->carCount;
}

// FUNC 0x0018D340 RaceReplayInformation::setupEntry
//   (index.txt: virtual_10) The entry is loaded from the block's slot of
//   its number.
void RaceReplayInformation::setupEntry(RaceEntryCar *car)
{
    race_entry_load_slot(car, &blk->slots[car->number]);
}

// FUNC 0x0018D378 RaceReplayInformation::setupOrganization
//   (index.txt: virtual_11) Nothing.
void RaceReplayInformation::setupOrganization(RaceOrganization *org)
{
}

// FUNC 0x0018D380 RaceReplayInformation::createRace
//   (? createRace) The race of the block, by its class name (in buf with
//   arg, or new when buf is NULL).  Callers: play_saved_replay, logger_main.
RaceBase *RaceReplayInformation::createRace(void *buf, int arg)
{
    return race_create_by_name(blk->raceName, buf, arg);
}

// FUNC 0x0018D3A0 RaceReplayInformation::destroyRace
//   (? destroyRace) race_destroy(race, heap).  Caller: play_saved_replay.
void RaceReplayInformation::destroyRace(RaceBase *race, int heap)
{
    race_destroy(race, heap);
}

// FUNC 0x0018D3C0 race_entry_load_slot
//   Loads an entry from a slot of a race info block (RaceInfoSlot,
//   0x280 bytes): its CarSaveEquipments (the first 0x1D0 bytes of the
//   entry's CarEquipments, aligned double-word copies), its CarCondition
//   (0x20 bytes, unaligned copies), input id, order and the five option
//   bytes (read signed, lb; four become booleans).  Callers:
//   RaceReplayInformation::setupEntry, RaceBase::virtual_19.
//   Retail copies the condition from a copy of `slot' made first ($t0;
//   the rest reads $a1): `s' gives exactly that, an exact form, not
//   necessarily the original (an inline RaceEntryCar::setCondition, a
//   reference to the condition or a helper do not).  setInputID: inline,
//   &car->info stays in a register.
void race_entry_load_slot(RaceEntryCar *car, const RaceInfoSlot *slot)
{
    const RaceInfoSlot *s = slot;
    (CarSaveEquipments &)car->info = slot->info;
    car->condition = s->condition;
    car->info.setInputID(slot->inputId);
    car->order = slot->order;
    /* The option bytes are s8 (lb; only steeringAssist's copy sees the sign). */
    car->transmission = slot->transmission != 0;
    car->tyre = slot->tyre != 0;
    car->steeringAssist = slot->steeringAssist;
    car->assistAsm = slot->assistAsm != 0;
    car->assistTcs = slot->assistTcs != 0;
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018D4A8 __tf21RaceReplayInformation
//   Builds the __si_type_info at 0x004B20D0 (base: RaceInformation,
//   gp-0x4678) on the first call.
extern "C" const type_info &__tf21RaceReplayInformation(void)
{
    if (__ti21RaceReplayInformation[0] == 0) {
        __tf15RaceInformation();
        __rtti_si(__ti21RaceReplayInformation, "21RaceReplayInformation",
                  (const type_info *)__ti15RaceInformation);
    }
    return *(const type_info *)__ti21RaceReplayInformation;
}
#endif
