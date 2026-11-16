/* T149_race_replay_information.h - RaceReplayInformation, the race
 * information of a saved race (translation unit T149, unit u049),
 * 0x0018D1D8..0x0018D4F8.
 *
 * Built on the stack by play_saved_replay / logger_main from the RaceInfoBlock at
 * the head of a replay / ghost save (RaceBase::virtual_7 wrote it): the
 * race / course codes, the option block, the car count and every car come
 * from the block; the race object itself is created by its class name
 * (RaceInfoBlock::raceName) through the race class registry.
 *
 * The virtual methods keep RaceInformation's slot names (T124). */
#ifndef GAME_T149_RACE_REPLAY_INFORMATION_H
#define GAME_T149_RACE_REPLAY_INFORMATION_H

#include "game/game.h"
#include "game/race/core/T124_race_information.h"         /* base RaceInformation */

class RaceBase;                                 /* T129 (pointers only) */
struct RaceInfoBlock;                           /* T129 (pointers only) */
struct RaceInfoSlot;                            /* T129 (pointers only) */
class RaceEntryBase;                            /* T121 (pointers only) */
class RaceEntryCar;                             /* T121 (pointers only) */
class RaceOrganization;                         /* T125 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* (__si_type_info 0x004B20D0, base RaceInformation 0x00353378) */
extern "C" const type_info &__tf21RaceReplayInformation(void); /* 0x0018D4A8 */
#endif

/* vtable 0x0032D200 (15 slots; 0, 1, 2, 4, 8, 10 and 11 its own), typeinfo
 * __tf21RaceReplayInformation 0x0018D4A8.  vptr +0x54 (RaceInformation's);
 * the data end at 0x5C, 0x60 bytes (RaceInformation is 8-byte aligned: its
 * u64 raceCode at +0x40 is read with ld). */
class RaceReplayInformation : public RaceInformation {
public:
    /* RaceInformation, blk; the race code (getRaceCode, called directly)
     * and the option block from the block. */
    RaceReplayInformation(RaceInfoBlock *blk);  /* 0x0018D1D8 */
    virtual ~RaceReplayInformation();           /* [ 0] +0x0C 0x0018D290 */
    virtual u64  getRaceCode();                 /* [ 1] +0x14 0x0018D2B8 blk->raceCode */
    virtual u64  getCourseCode();               /* [ 2] +0x1C 0x0018D2C8 blk->courseCode */
    virtual RaceEntryBase *createEntries();     /* [ 4] +0x2C 0x0018D2D8 new RaceEntryPS2 */
    virtual int  getEntryCount();               /* [ 8] +0x4C 0x0018D330 blk->carCount */
    /* The entry is loaded from the block's slot of its number
     * (race_entry_load_slot). */
    virtual void setupEntry(RaceEntryCar *car);             /* [10] +0x5C 0x0018D340 */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x0018D378 nothing */

    /* The race of the block, by its class name (in buf, or new when buf is
     * NULL): race_create_by_name(name, buf, size); size is the buffer's
     * (logger_main passes its alloca size).  Callers: play_saved_replay,
     * logger_main (which needs the RaceSolitaire: a cast).  (u049
     * F_0018D380; the name u018 / u049's.) */
    RaceBase *createRace(void *buf, int size);  /* 0x0018D380 */
    /* race_destroy(race, heap).  Caller: play_saved_replay.  (u049
     * F_0018D3A0.) */
    void destroyRace(RaceBase *race, int heap); /* 0x0018D3A0 */

    RaceInfoBlock *blk;                         /* 0x58 the block (stored before the vptr) */
};
EGCS_SIZE(RaceReplayInformation, 0x60);

/* Loads entry car from slot (a RaceInfoSlot of a RaceInfoBlock, 0x280
 * bytes): the first 0x1D0 bytes of its CarEquipments, its name, input id,
 * order and the five option bytes.  Callers: RaceReplayInformation::
 * setupEntry, RaceBase::virtual_19 (u043).  (u049 F_0018D3C0, void *slot.) */
void race_entry_load_slot(RaceEntryCar *car, const RaceInfoSlot *slot);    /* 0x0018D3C0 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf21RaceReplayInformation fills
 * (u049: g_004B20D0). */
extern "C" u32 __ti21RaceReplayInformation[3];  /* 0x004B20D0 */
#endif

#endif /* GAME_T149_RACE_REPLAY_INFORMATION_H */
