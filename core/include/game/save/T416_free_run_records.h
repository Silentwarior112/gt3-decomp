/* T416_free_run_records.h - FreeRunRecords, the free run (time attack)
 * rankings of the save data (translation unit T416, unit u095),
 * 0x002127D0..0x00213210.
 *
 * GameStatus+0x26160: 44 ranking tables of 12 places, one per course of the
 * T435 database's "TIME_ATTACK" list (table(course): the course's index in
 * that list, not range-checked).  The table code is T416's own copy of
 * T415's MachineTestTable (times only, no speed functions; deleted places are
 * always cleared as empty times, 0x157529FF). */
#ifndef GAME_T416_FREE_RUN_RECORDS_H
#define GAME_T416_FREE_RUN_RECORDS_H

#include "game/game.h"
#include "game/save/T415_machine_test_records.h"     /* RankingCode (by value) */

/* One course: 12 places, fastest first (? name).  Size 0x300 (size(),
 * load / save copy 0x300). */
class FreeRunTable {
public:
    void clearEntry(int i);                     /* 0x002127D0 */
    void clear();                               /* 0x00212878 */
    int  findTime(u32 time);                    /* 0x002128C8 place a new time would take, -1 none */
    int  insertTime(u32 time);                  /* 0x00212928 inserts it; place or -1 */
    u32  getTime(int i);                        /* 0x002129C0 */
    void setName(const char *name, int i);      /* 0x002129D0 (and the date) */
    char *getName(int i);                       /* 0x00212AC0 */
    void setCar(u64 car, int i);                /* 0x00212AD0 */
    u64  getCar(int i);                         /* 0x00212B30 */
    void setCode(const char *code, int i);      /* 0x00212B40 */
    char *getCode(int i);                       /* 0x00212C00 */
    void insertDate(int i);                     /* 0x00212C10 */
    u32  getDate(int i);                        /* 0x00212C98 */
    void remove(int i);                         /* 0x00212CA8 the places after it move up */
    static int size();                          /* 0x00212DD0 0x300 */
    const u8 *load(const u8 *src, int version); /* 0x00212DD8 version unused (callers pass 0) */
    u8  *save(u8 *dst);                         /* 0x00212E08 */

    u32         time[12];                       /* 0x000 */
    char        name[12][0x10];                 /* 0x030 */
    u64         car[12];                        /* 0x0F0 */
    u32         date[12];                       /* 0x150 */
    RankingCode code[12];                       /* 0x180 */
};
EGCS_SIZE(FreeRunTable, 0x300);

/* GameStatus+0x26160.  No vtable.  Size 0x8400 (size(); GameStatus's next
 * member, MachineTestRecords, at 0x2E560). */
class FreeRunRecords {
public:
    FreeRunRecords();                           /* 0x00212E40 */
    void init();                                /* 0x00212E68 */
    FreeRunTable *table(const char *course);    /* 0x00212EB0 */
    int  findTime(const char *course, u32 time);    /* 0x00212F00 */
    int  insertTime(const char *course, u32 time);  /* 0x00212F30 */
    u32  getTime(const char *course, int i);        /* 0x00212F60 */
    void setName(const char *course, const char *name, int i); /* 0x00212F90 */
    char *getName(const char *course, int i);       /* 0x00212FD0 */
    void setCar(const char *course, u64 car, int i); /* 0x00213000 */
    u64  getCar(const char *course, int i);         /* 0x00213040 */
    void setCode(const char *course, const char *code, int i); /* 0x00213070 */
    char *getCode(const char *course, int i);       /* 0x002130B0 */
    u32  getDate(const char *course, int i);        /* 0x002130E0 */
    void remove(const char *course, int i);         /* 0x00213110 */
    static int size();                          /* 0x00213140 0x8400 */
    const u8 *load(const u8 *src, int arg);     /* 0x00213168 arg unused (GameStatus passes 0) */
    u8  *save(u8 *dst);                         /* 0x002131C0 */

    FreeRunTable tables[0x2C];                  /* 0x0000 [course index] */
};
EGCS_SIZE(FreeRunRecords, 0x8400);

#endif /* GAME_T416_FREE_RUN_RECORDS_H */
