/* T415_machine_test_records.h - MachineTestRecords, the machine test
 * rankings of the save data (translation unit T415, unit u095),
 * 0x00211B58..0x002127D0; and RankingCode, the ranking code of a record
 * (also in T416, T417, T420).
 *
 * GameStatus+0x2E560: three ranking tables of 12 places (kind 0: 0-400 m
 * times, 1: 0-1000 m times, ascending, empty places 0x157529FF; 2: max
 * speeds * 100000, descending, empty places 0).  Each place has a time, a
 * name (15 characters), a car (u64 code), the date it was set and a 32-byte
 * ranking code.  The set* functions insert at a place (the places after it
 * move down); the time / speed is inserted first by insertTime /
 * insertSpeed, which return the place (-1 none). */
#ifndef GAME_T415_MACHINE_TEST_RECORDS_H
#define GAME_T415_MACHINE_TEST_RECORDS_H

#include "game/game.h"

/* The machine test kinds: MachineTestRecords' table index (tables[kind]),
 * RaceMachineTest(kind), ResultMachineTest::kind, the short menus
 * qm_gt_mt_400 / _1000 / _max.mbl (race codes "M400", "M1000", "MMAX"). */
enum MachineTestKind {
    MACHINE_TEST_400M      = 0,                 /* 0-400 m, a time */
    MACHINE_TEST_1000M     = 1,                 /* 0-1000 m, a time */
    MACHINE_TEST_MAX_SPEED = 2,                 /* the max speed challenge, a speed (x 100000) */
    MACHINE_TEST_KINDS     = 3
};

/* A ranking code: the 30-character password of a record (F_0012F3F0) in a
 * 32-byte char array, copied as a whole; "------------------------------"
 * when empty.  Size 0x20 (the record tables' strides). */
class RankingCode {
public:
    char s[0x20];                               /* 0x00 */
};
EGCS_SIZE(RankingCode, 0x20);

/* One ranking table of 12 places (? name).  T416 has its own copy of the
 * code (FreeRunTable).  Size 0x300 (size(), load / save copy 0x300). */
class MachineTestTable {
public:
    void clearEntry(int i);                     /* 0x00211B58 an empty time place */
    void clear();                               /* 0x00211C00 */
    void clearEntryZero(int i);                 /* 0x00211C50 an empty speed place (time 0, code kept) */
    void clearZero();                           /* 0x00211C80 */
    int  findTime(u32 time);                    /* 0x00211CD0 place a new time would take, -1 none */
    int  insertTime(u32 time);                  /* 0x00211D30 inserts it; place or -1 */
    u32  getTime(int i);                        /* 0x00211DC8 */
    int  findSpeed(f32 speed);                  /* 0x00211DD8 */
    int  insertSpeed(f32 speed);                /* 0x00211E50 */
    f32  getSpeed(int i);                       /* 0x00211EF0 */
    void setName(const char *name, int i);      /* 0x00211F38 (and the date) */
    char *getName(int i);                       /* 0x00212028 */
    void setCar(u64 car, int i);                /* 0x00212038 */
    u64  getCar(int i);                         /* 0x00212098 */
    void setCode(const char *code, int i);      /* 0x002120A8 */
    char *getCode(int i);                       /* 0x00212168 */
    void insertDate(int i);                     /* 0x00212178 the current date (clock_seconds) */
    u32  getDate(int i);                        /* 0x00212200 */
    void remove(int i, int zero);               /* 0x00212210 the places after it move up */
    static int size();                          /* 0x00212358 0x300 */
    const u8 *load(const u8 *src, int version); /* 0x00212360 version unused (callers pass 0) */
    u8  *save(u8 *dst);                         /* 0x00212390 */

    u32         time[12];                       /* 0x000 times (speed tables: speed * 100000) */
    char        name[12][0x10];                 /* 0x030 */
    u64         car[12];                        /* 0x0F0 */
    u32         date[12];                       /* 0x150 */
    RankingCode code[12];                       /* 0x180 */
};
EGCS_SIZE(MachineTestTable, 0x300);

/* GameStatus+0x2E560.  No vtable.  Size 0x900 (size(); GameStatus's next
 * member, PrizeSetFlags, at 0x2EE60). */
class MachineTestRecords {
public:
    MachineTestRecords();                       /* 0x002123C8 */
    void init();                                /* 0x002123F0 */
    MachineTestTable *table(int kind);          /* 0x00212428 */
    int  findTime(int kind, u32 time);          /* 0x00212438 */
    int  insertTime(int kind, u32 time);        /* 0x00212468 */
    u32  getTime(int kind, int i);              /* 0x00212498 */
    int  findSpeed(int kind, f32 speed);        /* 0x002124C8 */
    int  insertSpeed(int kind, f32 speed);      /* 0x002124F8 */
    f32  getSpeed(int kind, int i);             /* 0x00212528 */
    void setName(int kind, const char *name, int i); /* 0x00212558 */
    char *getName(int kind, int i);             /* 0x00212598 */
    void setCar(int kind, u64 car, int i);      /* 0x002125C8 */
    u64  getCar(int kind, int i);               /* 0x00212608 */
    void setCode(int kind, const char *code, int i); /* 0x00212638 */
    char *getCode(int kind, int i);             /* 0x00212668 */
    u32  getDate(int kind, int i);              /* 0x00212690 */
    void remove(int kind, int i);               /* 0x002126C0 kind 2 clears its last place with zeros */
    static int size();                          /* 0x00212708 0x900 */
    const u8 *load(const u8 *src, int arg);     /* 0x00212730 arg unused (GameStatus passes 0) */
    u8  *save(u8 *dst);                         /* 0x00212788 */

    MachineTestTable tables[MACHINE_TEST_KINDS]; /* 0x000 [kind] */
};
EGCS_SIZE(MachineTestRecords, 0x900);

#endif /* GAME_T415_MACHINE_TEST_RECORDS_H */
