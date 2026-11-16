/* T420_license_records.h - LicenseRecords, the licence test records of the
 * save data (translation unit T420, unit u096), 0x00215048..0x00215D00.
 *
 * GameStatus+0x1F550: 6 licences (B, A, IB, IA, S, R) x 8 tests, each a medal
 * (0 gold, 1 silver, 2 bronze, -1 none) and a 6-place ranking (time, name,
 * date, ranking code).  Quirk: reset() clears 7 licences, the 7th being the
 * 0xAA0 bytes after the object: GameStatus's EventResults (+0x23510),
 * CourseBestTimes and the first 0x3F8 bytes of ArcadeRecords.  Harmless:
 * GameStatus::init clears those three right after. */
#ifndef GAME_T420_LICENSE_RECORDS_H
#define GAME_T420_LICENSE_RECORDS_H

#include "game/game.h"
#include "game/save/T415_machine_test_records.h"     /* RankingCode (by value) */

/* The licences: LicenseRecords' first index (tests[licence][test]), the
 * race kinds RACE_KIND_LICENSE_B .. _R (T489) of a licence test's code,
 * GameStatus::largeTrophy bit 1 << licence, LicenseGetControl::param. */
enum LicenseIndex {
    LICENSE_B     = 0,                          /* "lic_b", "b_mess*" */
    LICENSE_A     = 1,
    LICENSE_IB    = 2,
    LICENSE_IA    = 3,
    LICENSE_S     = 4,                          /* GameStatus::isLicenseSPassed */
    LICENSE_R     = 5,                          /* the rally licence ("L0R.."; files "lic_d", "d_mess*") */
    LICENSE_COUNT = 6
};

enum {
    LICENSE_TESTS  = 8,                         /* tests of a licence */
    LICENSE_PLACES = 6                          /* places of a test's ranking */
};

/* LicenseTest::medal (lower is better). */
enum LicenseMedal {
    LICENSE_MEDAL_NONE = -1,
    LICENSE_MEDAL_GOLD = 0,
    LICENSE_MEDAL_SILVER = 1,
    LICENSE_MEDAL_BRONZE = 2
};

/* One licence test.  Size 0x154 (getSize; reset's stride 0xAA0 per 8). */
class LicenseTest {
public:
    void clear();                               /* 0x00215048 */
    int  getMedal();                            /* 0x002150F8 */
    void setMedal(int medal);                   /* 0x00215100 keeps the best */
    int  insertTime(u32 time);                  /* 0x00215128 place or -1 */
    u32  getTime(int i);                        /* 0x002151F0 */
    void insertName(const char *name, int place); /* 0x00215200 (and the date) */
    char *getName(int i);                       /* 0x002152F0 */
    void insertDate(int place);                 /* 0x00215300 */
    void insertCode(const RankingCode *code, int place); /* 0x00215388 */
    RankingCode *getCode(int i);                /* 0x00215448 */
    u32  getDate(int i);                        /* 0x00215458 */
    void remove(int i);                         /* 0x00215468 the last ranking code is kept */
    static int getSize();                       /* 0x00215580 0x154 */
    const u8 *load(const u8 *src, int arg);     /* 0x00215588 */
    u8 *save(u8 *dst);                          /* 0x002155B8 */

    int  medal;                                 /* 0x000 LicenseMedal */
    char names[LICENSE_PLACES][0x10];           /* 0x004 */
    u32  times[LICENSE_PLACES];                 /* 0x064 0x157529FF = none */
    u32  dates[LICENSE_PLACES];                 /* 0x07C */
    RankingCode codes[LICENSE_PLACES];          /* 0x094 */
};
EGCS_SIZE(LicenseTest, 0x154);

/* GameStatus+0x1F550.  No vtable.  Size 0x3FC0 (getSize 48 tests;
 * GameStatus's next member, EventResults, at 0x23510). */
class LicenseRecords {
public:
    LicenseRecords();                           /* 0x002155F0 */
    void reset();                               /* 0x00215618 */
    int  getMedal(int lic, int test);           /* 0x002156A0 */
    void setMedal(int lic, int test, int medal); /* 0x002156D0 */
    int  isPassed(int lic, int first7);         /* 0x00215708 every test (the first 7) has a medal */
    int  isAllGold(int lic);                    /* 0x00215790 */
    int  highestPassed();                       /* 0x002157E8 4..0, -1 none */
    void passAll(int lic);                      /* 0x00215840 bronze for every test (no caller) */
    int  countMedal(int medal);                 /* 0x00215890 */
    int  insertTime(int lic, int test, u32 time); /* 0x00215940 */
    u32  getTime(int lic, int test, int i);     /* 0x00215978 */
    void insertName(int lic, int test, const char *name, int place); /* 0x002159B0 */
    char *getName(int lic, int test, int i);    /* 0x002159E8 */
    void insertCode(int lic, int test, const RankingCode *code, int place); /* 0x00215A20 */
    RankingCode *getCode(int lic, int test, int i); /* 0x00215A58 */
    u32  getDate(int lic, int test, int i);     /* 0x00215A90 */
    void remove(int lic, int test, int i);      /* 0x00215AC8 */
    int  countPassed();                         /* 0x00215B00 tests with a medal */
    int  numTests();                            /* 0x00215BA0 48 */
    static int getSize();                       /* 0x00215BA8 */
    const u8 *load(const u8 *src, int arg);     /* 0x00215BD0 */
    u8 *save(u8 *dst);                          /* 0x00215C70 */

    LicenseTest tests[LICENSE_COUNT][LICENSE_TESTS]; /* 0x0000 [LicenseIndex][test] */
};
EGCS_SIZE(LicenseRecords, 0x3FC0);

#endif /* GAME_T420_LICENSE_RECORDS_H */
