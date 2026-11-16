/* T125_race_organization.h - RaceOrganization, RaceResult, RaceDBRecord
 * (translation unit T125, unit u043), 0x0017A038..0x0017B498.
 *
 * RaceOrganization is the race-wide state that race_organization_create
 * (T123) builds from a RaceInformation: the option block, the entry list,
 * the race database and its selected race, the course, the result, the
 * start countdown and the time limit, the logger flags, the event queue and
 * the records.  The running race (RaceBase, T129) points at it (RaceBase +4)
 * and it points back (+0x3C).
 *
 * Inline in the original: 0x0017B050..0x0017B450 (no callers) are the
 * out-of-line copies of RaceOrganization's inline members; Lupo 2 has all of
 * them as linkonce copies in the reverse order (setPosition, eventCount,
 * eventPut / eventGet / eventCheck in pairs, ..., getEntry, the arcade and
 * split battle option getters, ..., getBackgroundMode).  They are declared
 * out of line here (T125 defines them in address order: the same code).
 * The Lupo 2 names are the getters 0x0017B180..0x0017B450 (symbols.csv and
 * docs/symbols/lupo2_to_gt3_map.csv); the others are the first pass's or,
 * where they say what the function does, ones taken from its code (notes
 * h23).  (u043's matching pass: 0x0017B3B0..0x0017B3F0 are RaceOrganization's,
 * not RaceEntryBase's.)
 */
#ifndef GAME_T125_RACE_ORGANIZATION_H
#define GAME_T125_RACE_ORGANIZATION_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/data/T484_race_database.h"        /* RaceEvent (RaceDBRecord), RaceDataBase */
#include "game/menu/race_menu/T115_race_option.h"          /* RaceOption (held by value) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue (held by value) */
#include "game/race/core/T127_race_records.h"         /* SpeedRecord (held by value) */
#include "game/race/core/T128_race_lap_record.h"      /* LapRecord (held by value) */

class type_info;
class RaceEntryBase;                        /* T121 (pointers only: forward declarations) */
class RaceEntryCar;                         /* T121 */
class RaceLapTime;                          /* T126 */
class RaceCourse;                           /* T153 */
class RaceBase;                             /* T129 */
class RaceBGMBase;                          /* T175 */
class RaceInput;                            /* T413 */
class CarEquipments;                        /* T482 */
class SplitBattleOption;                    /* T089 */
class _UnitArenaBase;                       /* T471 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The typeinfo function (vtable entry 0): 0x0017B458 (__user_type_info
 * 0x00353380, name 0x00327840).  Declared for the GCC 14 build only: a
 * declaration egcs sees keeps it from emitting its own. */
extern "C" {
const type_info &__tf16RaceOrganization(void);
}
#endif

/* The race record of table 0x20 that RaceDataBase::event points at: T484's
 * RaceEvent (h11).  u043's names map to it: byte_199 = numberOfLaps,
 * target1..3 = time[0..2], startingCount = countDown (0x1F2), startAction
 * (0x1F3); u042 maxEntries = entryCount, u033 kind = needTire, u016 / u017
 * unk1D3 = trophyType, u051 unk1E4 = limitTime. */
typedef RaceEvent RaceDBRecord;

/* The result block of a race (RaceOrganization +0x40), read by the result
 * screens.  Not polymorphic.  Size 0x10. */
class RaceResult {
public:
    /* rank -2, time RACE_TIME_INVALID, the flags 0.  Callers:
     * RaceOrganization::initRaceState, DynamicsConductor::
     * recordOneLapGhostControl.  (Inline: emitted in T125.) */
    void reset();                           /* 0x0017B030 */
    /* Inline getters (? names, u017): T053 copies the organization's result
     * to the stack and reads the flags through them (the copy's own address
     * in a register, the flag's offset). */
    int  getBeatGhost() { return beatGhost; }
    int  getLapRecord() { return lapRecord; }

    int rank;                               /* 0x00 -2 after reset (u083 resultRank) */
    u32 time;                               /* 0x04 ms (u083 resultTime) */
    /* Car 0 beat car 1's (the ghost's) lap (T305 checkRunResult): the new
     * ghost is offered (u016 / u084; u043 field_08). */
    int beatGhost;                          /* 0x08 */
    /* A lap at or under the race best (T305 checkLapRecord): the ghost save
     * and the data logger are offered (u043 field_0C). */
    int lapRecord;                          /* 0x0C */
};
EGCS_SIZE(RaceResult, 0x10);

/* vtable 0x00327658 (1 slot).  typeinfo __tf16RaceOrganization 0x0017B458.
 * vptr +0x8C0 (after the data).  Size 0x8C8 (operator new(0x8C8) in
 * race_organization_create; the u64 members align it to 8). */
class RaceOrganization {
public:
    static void *operator new(size_t size);             /* 0x0017A038 the race pool, 16-byte aligned */
    static void  operator delete(void *p, size_t size); /* 0x0017A060 */
    /* The members, then the pointers and flags cleared, timerActive 1 (the
     * option block, the codes, result, startFlag, startAction and
     * loggerSector are left). */
    RaceOrganization();                                 /* 0x0017A088 */
    virtual ~RaceOrganization();                        /* [0] +0x0C 0x0017A150 deletes the DB, the entries, the course */

    /* entries->virtual_4(mode) (the one-car view of a solitaire list), then
     * loggerMode = mode.  The logger menus (logger_run / logger_main). */
    void setLoggerMode(int mode);                       /* 0x0017A1C0 */
    bool isArcadeSingle();                              /* 0x0017A208 dynamic_cast<RaceArcadeSingle *>(race) */
    void setRaceCode(u64 code);                         /* 0x0017A260 raceDB->select(code) too */
    void releaseRaceDB();                               /* 0x0017A288 delete raceDB */
    void setRaceDB(RaceDataBase *db);                   /* 0x0017A2D0 */
    void releaseEntries();                              /* 0x0017A308 delete entries */
    /* RaceEntryCar::unloadModel on car `car' (< 0: every car; past the
     * end: none). */
    void unloadModels(int car);                         /* 0x0017A350 */
    void setEntries(RaceEntryBase *e);                  /* 0x0017A3F0 */
    void releaseCourse();                               /* 0x0017A428 delete course */
    void setCourse(RaceCourse *c);                      /* 0x0017A470 */
    /* Unloads the course data (when there is a course, RaceCourse::unload) and
     * returns the arena they come from (g_002BB5B8, a UnitArena<64>): the
     * memory card code takes a work unit of it (race_memcard, run_split_battle,
     * setting_menu).  (u043 F_0017A4A8, const void *.) */
    _UnitArenaBase *unloadCourseData();                 /* 0x0017A4A8 */
    int  F_0017A4D8();                                  /* 0x0017A4D8 the course data table's entries of key 0 */
    f32  F_0017A500(int idx);                           /* 0x0017A500 the idx-th of them */
    /* The course runway's markers of key 1: the data logger's sectors
     * (dc_section_count / dc_section_value, LoggerRaceGraph::setup; u043
     * F_0017A528 / F_0017A550). */
    int  getLoggerSectorCount();                        /* 0x0017A528 */
    f32  getLoggerSectorPosition(int idx);              /* 0x0017A550 */
    /* The race start (RaceBase::virtual_8): result.reset(), the event queue
     * cleared, the start action and count from the race record, the time
     * limit armed. */
    void initRaceState();                               /* 0x0017A578 */
    /* Per frame (RaceBase::virtual_29): a pending start action posts its
     * events (2, 0x14) and sets the countdown; the countdown (event 4 at
     * 0), then the time limit (event 7 on expiry). */
    void updateStartAndTimer();                         /* 0x0017A5E8 */
    /* entries->sortByRank(); event 8 stops the time limit; event 0xD (arg
     * 0) sets loggerSector. */
    void updateRanksAndEvents();                        /* 0x0017A780 */
    void F_0017A810();                                  /* 0x0017A810 empty */
    void resetEntryOrder();                             /* 0x0017A818 car i's order = i */
    /* The car of order `from' gets order `to' (clamped), the ones between
     * shift by one. */
    void moveEntryOrder(int from, int to);              /* 0x0017A858 */
    void updateColorIndices();                          /* 0x0017A900 RaceEntryCar::updateColorIndex on every car */
    /* Every car: input = race->virtual_26(info.inputId); an enabled input
     * gets the car's vibration (outside replays) and starts its record
     * (RaceBase::virtual_9). */
    void attachInputs(RaceBase *race);                  /* 0x0017A970 */
    void F_0017AA48();                                  /* 0x0017AA48 the enabled inputs' records stopped (Recorder::stop) */
    /* Prepares car 1 (the ghost / second car; u043 F_0017AAD8): with flag, not
     * in a replay or with the ghost option off; a second entry if needed;
     * unless mode 2 car 1 = car 0 (copyFrom); the ghost's record buffer:
     * mode 0 buffer 1 = buffer 0, 1 slot 0 = slot 2, 2 kept (an info
     * block); the ghost input released; flag: wired up again. */
    void setupGhost(int mode, int flag);                /* 0x0017AAD8 mode 0: the time trial / machine /
                                                           licence tests, 1 the free run (u016 / u017) */
    void F_0017AD28();                                  /* 0x0017AD28 race->virtual_23(), car 0's updateGhostLapTime() */
    int  getTargetCount();                              /* 0x0017AD70 5: the ranks licenseFinished walks (u043 F_0017AD70) */
    int  getTargetTime(int rank);                       /* 0x0017AD78 the record's time[rank - 1] in ms (1..3), else 0 */
    RaceBGMBase *getBGM();                              /* 0x0017AE20 race->bgm, or the dummy g_003521D0 (u043 getRaceInput) */
    void setCourseCode(u64 code);                       /* 0x0017AE40 raceDB->setCourse(code) too */
    void setEntryVibration(int value, int entry);       /* 0x0017AE68 (u043 setEntryField20F8) */
    void setEntryTransmission(int value, int entry);    /* 0x0017AE88 (u043 setEntryField20E4) */
    /* The car's section time; the difference to the race-wide record. */
    int  recordSectionTime(int entry, int section, u32 time);   /* 0x0017AEA8 */
    /* The car's lap total (its own best updated); the difference to the
     * race-wide record, which takes the lap when update and it is better. */
    int  recordLapTime(int entry, u32 total, int update);       /* 0x0017AF10 */
    void setEntrySteeringAssist(int value, int entry);       /* 0x0017AF98 */
    void setEntryAsm(int value, int entry);       /* 0x0017AFB8 */
    void setEntryTcs(int value, int entry);        /* 0x0017AFD8 value != 0 */
    void setEntryInfo(int entry, const CarEquipments *src);     /* 0x0017AFF8 the car's info = *src (operator=) */
    void setEntryOrder(int entry, int order);           /* 0x0017B050 */
    u8   getEventCount(int type);                       /* 0x0017B070 eventQueue.counts[type] */
    void postEvent(int type, int arg, int delay);       /* 0x0017B080 */
    void postEvent2(int type, int arg, int delay);      /* 0x0017B0A0 the same */
    int  takeEvent(int type, int *arg);                 /* 0x0017B0C0 find(type, arg, 1) */
    int  takeEvent2(int type, int *arg);                /* 0x0017B0E0 the same */
    int  findNthEvent(int type, int *arg, int nth);     /* 0x0017B100 */
    int  findNthEvent2(int type, int *arg, int nth);    /* 0x0017B120 the same */
    int  findEvent(int type, int *arg);                 /* 0x0017B140 find(type, arg, 0) */
    int  findEvent2(int type, int *arg);                /* 0x0017B160 the same */
    RaceEventQueue *getEventQueue();                    /* 0x0017B180 */
    u64  getCourseCode();                               /* 0x0017B188 */
    void clearCourseModel();                            /* 0x0017B190 course->unload(), no NULL check */
    RaceCourse *getCourse();                            /* 0x0017B1B0 */
    RaceResult *getResult();                            /* 0x0017B1B8 */
    int  getTimeRemain();                               /* 0x0017B1C0 */
    void setTimeLimit(int v);                           /* 0x0017B1C8 */
    bool isTimeLimit();                                 /* 0x0017B1D0 */
    int  getCountDown();                                /* 0x0017B1E0 */
    int  getStartingCount();                            /* 0x0017B1E8 */
    void setCountDown(int v);                           /* 0x0017B1F0 startingCount = v */
    bool isCountDown();                                 /* 0x0017B1F8 */
    void clearCountDown();                              /* 0x0017B208 startAction = -1 */
    void clearStartAction();                            /* 0x0017B218 startFlag = 0 */
    int  getStartAction();                              /* 0x0017B220 */
    void setStartAction(int v);                         /* 0x0017B228 */
    u64  getRaceCode();                                 /* 0x0017B230 */
    RaceBase *getRace();                                /* 0x0017B238 */
    void setRace(RaceBase *r);                          /* 0x0017B240 */
    SpeedRecord *getSpeedRecord();                      /* 0x0017B248 */
    SpeedRecord *getSpeedRecord0();                     /* 0x0017B250 */
    void setLapRecordTotal(u32 t);                      /* 0x0017B258 */
    void setLapRecordSection(int i, u32 t);             /* 0x0017B260 */
    u32  getLapRecordTotal();                           /* 0x0017B270 */
    u32  getLapRecordSection(int i);                    /* 0x0017B278 */
    u32  getLapRecordTotal2();                          /* 0x0017B288 the same */
    bool updateLapRecordTotal(u32 t);                   /* 0x0017B290 */
    void setLapRecordTotal2(u32 t);                     /* 0x0017B2B0 the same */
    RaceInput *getEntryInput(u32 idx);                  /* 0x0017B2B8 (u043 getEntryDynamics) */
    RaceLapTime *getLapTime(u32 idx);                   /* 0x0017B2D8 */
    RaceEntryCar *getEntryCar(u32 idx);                 /* 0x0017B2F8 */
    int  getEntryCount();                               /* 0x0017B310 */
    RaceEntryCar *createEntry();                        /* 0x0017B320 entries->createEntry() */
    int  canAddEntry(RaceEntryCar *car);                /* 0x0017B350 entries->canAddEntry(car) (Lupo 2 inquiry) */
    int  addEntry(RaceEntryCar *car);                   /* 0x0017B380 */
    /* Lupo 2's names (in Lupo 2 these accessors are inline copies, in the
     * reverse order; 0x0017B3B0..0x0017B400 were mapped to RaceEntryBase's
     * by their code). */
    RaceEntryBase *getEntry();                          /* 0x0017B3B0 entries */
    RaceDataBase *getRaceDB();                          /* 0x0017B3B8 */
    int  getSplitBattleOptionSlowCarBoost();            /* 0x0017B3C0 option.splitBattle.boost */
    int  getSplitBattleOptionHandicap();                /* 0x0017B3C8 option.splitBattle.handicap */
    bool getSplitBattleOptionTireDamage();              /* 0x0017B3D0 option.splitBattle.tireDamage */
    int  getSplitBattleOptionLaps();                    /* 0x0017B3D8 option.splitBattle.raceLaps */
    SplitBattleOption *getSplitBattleOption();          /* 0x0017B3E0 &option.splitBattle */
    int  getArcadeOptionTireDamage();                   /* 0x0017B3E8 option.tireDamage */
    int  getArcadeOptionLaps();                         /* 0x0017B3F0 option.raceLaps */
    RaceOption *getArcadeOption();                      /* 0x0017B3F8 &option (= this) */
    RaceOption *getRaceOption();                        /* 0x0017B400 &option (= this) */
    /* The flag getters return the int fields as they are (a plain lw; a
     * bool return would add sltu); the setters take bool (Lupo 2).  The
     * fields stay int: T052's run_time_trial (u016) keeps the
     * demonstration store behind an int load, as an int store. */
    void setDemonstration(bool v);                      /* 0x0017B408 */
    int  getDemonstration() const;                      /* 0x0017B410 */
    void setRacePreviewMode(bool v);                    /* 0x0017B418 */
    int  getRacePreviewMode();                          /* 0x0017B420 */
    int  checkLoggerSector();                           /* 0x0017B428 */
    void setLoggerCreating(bool v);                     /* 0x0017B430 */
    int  getLoggerCreating();                           /* 0x0017B438 */
    int  getLoggerMode();                               /* 0x0017B440 */
    void setBackgroundMode(bool v);                     /* 0x0017B448 */
    int  getBackgroundMode();                           /* 0x0017B450 */

    RaceOption      option;                 /* 0x000 a copy of RaceInformation's (u083: battle08..14 in it) */
    RaceEntryBase  *entries;                /* 0x018 */
    /*  0x01C: alignment */
    u64             raceCode;               /* 0x020 */
    RaceDataBase   *raceDB;                 /* 0x028 */
    u64             courseCode;             /* 0x030 */
    RaceCourse     *course;                 /* 0x038 */
    RaceBase       *race;                   /* 0x03C the running race */
    RaceResult      result;                 /* 0x040 */
    int             startFlag;              /* 0x050 a start action is pending (initRaceState) */
    int             startAction;            /* 0x054 -1 none, 0..3 */
    int             startingCount;          /* 0x058 seconds of the start countdown */
    int             countDown;              /* 0x05C frames left of it */
    int             timerActive;            /* 0x060 the time limit runs (? name) */
    int             timeLimit;              /* 0x064 seconds */
    int             timeRemain;             /* 0x068 frames */
    int             backgroundMode;         /* 0x06C */
    int             loggerMode;             /* 0x070 */
    int             loggerCreating;         /* 0x074 */
    int             loggerSector;           /* 0x078 event 0xD (arg 0) this frame */
    int             racePreviewMode;        /* 0x07C */
    int             demonstration;          /* 0x080 */
    RaceEventQueue  eventQueue;             /* 0x084 */
    SpeedRecord     speedRecord0;           /* 0x8A4 */
    SpeedRecord     speedRecord;            /* 0x8A8 */
    LapRecord       lapRecord;              /* 0x8AC the race-wide best lap and sections */
    /*  0x8C0 vptr, then 0x8C4: alignment */
};
EGCS_SIZE(RaceOrganization, 0x8C8);

/* ------------------------------------------------------------------------ */
/* Data                                                                     */
/* ------------------------------------------------------------------------ */

/* (The plan put these here; they are declared by their owners now: the
 * dummy BGM g_003521D0 in T175_race_bgm_base.h, the sound globals
 * g_003521E4 / g_003521E8 in T176_car_sound.h, __ti8RaceBase in
 * T129_race_base.h.) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti16RaceOrganization[2];       /* 0x00353380 gp-0x4670 (u043: g_00353380) */
}
#endif

#endif /* GAME_T125_RACE_ORGANIZATION_H */
