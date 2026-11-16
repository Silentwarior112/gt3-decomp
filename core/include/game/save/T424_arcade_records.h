/* T424_arcade_records.h - ArcadeRecords, the arcade race records of the save
 * data (translation unit T424, unit u097), 0x00216308..0x00216BB8.
 *
 * GameStatus+0x23BB8: for each difficulty (the second letter of the race
 * code "AECT0001"...: 'E' easy, 'N' normal, 'H' hard, 'P' professional) and
 * each course of the T435 database's "SINGLE_RACE" list, the 4 best times and
 * the best place (BestPlace, T418: 0 = won, 8 = never raced).  rec[] is
 * indexed difficulty * n + course, n = the number of "SINGLE_RACE" courses
 * (34 by maxScore); 144 = 4 x 36 records are reserved. */
#ifndef GAME_T424_ARCADE_RECORDS_H
#define GAME_T424_ARCADE_RECORDS_H

#include "game/game.h"
#include "game/save/T418_best_place.h"               /* BestPlace (by value) */

/* One course at one difficulty.  Size 0x14 (getSize; the 144 records fill
 * 0xB40). */
class ArcadeRecord {
public:
    ArcadeRecord();                             /* 0x00216308 */
    void clear();                               /* 0x00216338 */
    int  updatePlace(int p);                    /* 0x00216370 1 if p is a new best */
    int  getPlace();                            /* 0x00216390 */
    int  addTime(u32 t);                        /* 0x00216398 its rank (0..3), -1 if not in the 4 */
    u32  getTime(int i);                        /* 0x00216460 */
    static int getSize();                       /* 0x00216470 0x14 */
    const u8 *load(const u8 *src, int arg);     /* 0x00216490 */
    u8 *save(u8 *dst);                          /* 0x002164E0 */

    u32       times[4];                         /* 0x00 fastest first, 0x157529FF = none */
    BestPlace place;                            /* 0x10 */
};
EGCS_SIZE(ArcadeRecord, 0x14);

/* GameStatus+0x23BB8.  No vtable.  Size 0xB40 (getSize; GameStatus's next
 * member, TimeTrialRecords, at 0x246F8). */
class ArcadeRecords {
public:
    ArcadeRecords();                            /* 0x00216528 */
    void clear();                               /* 0x00216590 */
    u32  difficultyOf(const char *race);        /* 0x002165D0 0 E, 1 N, 2 H, 3 P, 4 other */
    /* Quirks: an unknown difficulty gives rec[-1], an unknown course
     * rec[difficulty * n - 1]. */
    ArcadeRecord *find(const char *race, s64 course); /* 0x00216640 course: a nameToCode code */
    ArcadeRecord *at(int difficulty, int course);     /* 0x00216720 (no callers) */
    void updatePlace(const char *race, s64 course, int p); /* 0x00216780 a new best: GameStatus::updateArcadeProgress */
    int  getPlace(const char *race, s64 course);       /* 0x002167C8 */
    int  addTime(const char *race, s64 course, u32 t); /* 0x002167F0 (no callers) */
    u32  getTime(const char *race, s64 course, int i); /* 0x00216820 (no callers) */
    int  allWon(int difficulty, int count);     /* 0x00216850 the first `count' courses (all if < 0) won */
    int  allWonEasy();                          /* 0x00216900 */
    int  allWonNormal();                        /* 0x00216920 */
    int  allWonHard();                          /* 0x00216940 */
    int  allWonProfessional();                  /* 0x00216960 */
    int  score();                               /* 0x00216980 per course 3 hard, 2 normal, 1 easy won */
    int  maxScore();                            /* 0x00216AE8 102 (34 courses x 3) */
    static int getSize();                       /* 0x00216AF0 0xB40 */
    const u8 *load(const u8 *src, int arg);     /* 0x00216B18 arg unused (GameStatus passes 0) */
    u8 *save(u8 *dst);                          /* 0x00216B70 */

    ArcadeRecord rec[144];                      /* 0x000 [difficulty * courses + course] */
};
EGCS_SIZE(ArcadeRecords, 0xB40);

#endif /* GAME_T424_ARCADE_RECORDS_H */
