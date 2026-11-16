/* T417_time_trial_records.h - TimeTrialRecords, the arcade time trial
 * records of the save data (translation unit T417, unit u096),
 * 0x00213210..0x00213E90; and two records of other units' data that the
 * pass-2 plan put here: ChampionshipSaveEntry (an entry of T422's
 * ChampionshipSave) and RaceDataEntry (the event record of the race data
 * base, T484's RaceEvent).
 *
 * GameStatus+0x246F8: the 10 arcade time trial courses AT010001..AT100001,
 * 12 places each (time, name, date, ranking code) and the best place ever
 * taken.  reset() fills them with three preset records per course: the
 * times from the course's race data base entry (RaceEvent::time[3]), the
 * names Polyphony staff names (g_002E1080).  A name whose first byte is
 * negative is a preset record: ~name[0] indexes the preset names. */
#ifndef GAME_T417_TIME_TRIAL_RECORDS_H
#define GAME_T417_TIME_TRIAL_RECORDS_H

#include "game/game.h"
#include "game/save/T415_machine_test_records.h"     /* RankingCode (by value) */
#include "game/save/T418_best_place.h"               /* BestPlace (by value) */
#include "game/data/T482_car_equipments.h"           /* CarEquipments (by value) */

/* The record RaceDataBase::F_00238440 finds (RaceDataBase+0x18).  u096 and
 * u097 knew two fields of it (+0x1C4 the three preset times, u096's
 * PresetTime {u8 min, sec; u16 msec} = T484's RaceTime; +0x1E5 u097's
 * `league', T484's `difficulty'): it is the database owner's RaceEvent
 * (T484, unit u108), so this is that class, not a second definition. */
struct RaceEvent;
typedef RaceEvent RaceDataEntry;

/* An entry of the suspended championship (ChampionshipSave+0x90, 8 of
 * them, T422): a car and its points.  No vtable of its own (CarEquipments is
 * polymorphic, vptr +0x1DC).  Size 0x1F0 (ChampionshipSave's stride:
 * entries at 0x90, the object 0x1010 bytes). */
class ChampionshipSaveEntry {
public:
    CarEquipments car;                          /* 0x000 */
    int points;                                 /* 0x1E8 */
    u8  unk_1EC[4];                             /* 0x1EC ? (not saved) */
};
EGCS_SIZE(ChampionshipSaveEntry, 0x1F0);

/* One course: 12 places, best first.  Size 0x2A4 (getSize; the courses'
 * stride). */
class TimeTrialCourse {
public:
    void clear(int i);                          /* 0x00213228 */
    void clearAll();                            /* 0x002132C0 */
    int  findPlace(u32 time);                   /* 0x00213310 place a time would take, -1 none */
    int  enterTime(u32 time);                   /* 0x00213370 inserts it, updates best; place or -1 */
    u32  getTime(int i);                        /* 0x00213420 */
    int  insertTime(u32 time);                  /* 0x00213430 inserts it; place or -1 */
    void insertName(const char *name, int place); /* 0x002134F8 (and the date) */
    const char *getName(int i);                 /* 0x002135E8 a preset record's: time_trial_preset_name */
    void insertCode(const RankingCode *code, int place); /* 0x00213620 */
    RankingCode *getCode(int i);                /* 0x002136E0 */
    void insertDate(int place);                 /* 0x002136F0 */
    u32  getDate(int i);                        /* 0x00213778 a preset record's: 0x027CC4BC
                                                   (clock_seconds of 2001-04-28 00:01) */
    void remove(int i);                         /* 0x002137A8 the places after it move up */
    int  isPlayerRecord(int i);                 /* 0x002138A8 0 for a preset record */
    static int getSize();                       /* 0x002138C0 0x2A4 */
    const u8 *load(const u8 *src, int arg);     /* 0x002138E0 */
    u8 *save(u8 *dst);                          /* 0x00213930 */
    /* inline (TimeTrialRecords::countFirstPlaces / allFirstPlaces: retail
     * keeps &courses[c] in the loop register and reads +0x2A0 off it, as
     * an inlined member's fresh `this' does; ? name) */
    int  getBestPlace() { return best.get(); }

    u32  times[12];                             /* 0x000 0x157529FF = none */
    char names[12][0x10];                       /* 0x030 */
    u32  dates[12];                             /* 0x0F0 */
    RankingCode codes[12];                      /* 0x120 */
    BestPlace best;                             /* 0x2A0 */
};
EGCS_SIZE(TimeTrialCourse, 0x2A4);

/* GameStatus+0x246F8.  No vtable.  Size 0x1A68 (10 courses; GameStatus's
 * next member, FreeRunRecords, at 0x26160). */
class TimeTrialRecords {
public:
    TimeTrialRecords();                         /* 0x00213978 */
    void reset();                               /* 0x002139D0 */
    int  findPlace(int c, u32 time);            /* 0x00213BA8 */
    int  enterTime(int c, u32 time);            /* 0x00213BD0 */
    u32  getTime(int c, int i);                 /* 0x00213BF8 */
    void insertName(int c, const char *name, int place); /* 0x00213C20 */
    const char *getName(int c, int i);          /* 0x00213C50 */
    void insertCode(int c, const RankingCode *code, int place); /* 0x00213C78 */
    RankingCode *getCode(int c, int i);         /* 0x00213CA8 */
    u32  getDate(int c, int i);                 /* 0x00213CD0 */
    void remove(int c, int i);                  /* 0x00213CF8 */
    int  isPlayerRecord(int c, int i);          /* 0x00213D20 */
    int  countFirstPlaces();                    /* 0x00213D48 courses whose best place is 1st */
    int  numCourses();                          /* 0x00213D80 10 */
    int  allFirstPlaces();                      /* 0x00213D88 1 if every best place is 1st */
    static int getSize();                       /* 0x00213DC0 */
    const u8 *load(const u8 *src, int arg);     /* 0x00213DE8 */
    u8 *save(u8 *dst);                          /* 0x00213E48 */

    TimeTrialCourse courses[10];                /* 0x0000 [g_002E10F8 index] */
};
EGCS_SIZE(TimeTrialRecords, 0x1A68);

const char *time_trial_preset_name(int i);      /* 0x00213210 g_002E1080[i] */

/* The 30 preset record names, 3 per course ("T.Izumi", "N.Ishibashi",
 * "S.Ebihara", ... "K.Minemura"). */
extern const char *g_002E1080[30];              /* 0x002E1080 */
/* The 10 course codes "AT010001" .. "AT100001". */
extern const char *g_002E10F8[10];              /* 0x002E10F8 */

#endif /* GAME_T417_TIME_TRIAL_RECORDS_H */
