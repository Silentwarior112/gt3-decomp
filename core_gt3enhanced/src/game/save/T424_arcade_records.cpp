/* T424 (0x00216308..0x00216BB8): the arcade race records of the save data,
 * GameStatus+0x23BB8.  For each difficulty (the second letter of the race
 * code: 'E' easy, 'N' normal, 'H' hard, 'P' professional) and each course of
 * the product database's "SINGLE_RACE" list: the 4 best times and the best
 * place (BestPlace, T418: 0 = won, 8 = never raced).
 *
 * Retail's .rodata (0x003401E8..0x003402E0): "SINGLE_RACE", then the
 * GameStatus's garage type names (GarageWheelList .. Cell<GarageCar>): the
 * original saw the GameStatus and nothing else with a type name. */
#include "game/save/game_options_internal.h"
#include "game/race/core/T128_race_lap_record.h"  /* RACE_TIME_INVALID (0x157529FF: "no time") */
#include "game/data/T435_product_config.h"   /* product_config, the course tables */
#include "game/data/T481_car_data_base.h"    /* nameToCode */
#include "game/save/T429_game_status.h"      /* getGameStatus (updateArcadeProgress); the garage */
#include "game/save/T424_arcade_records.h"

/* ---- ArcadeRecord -------------------------------------------------------- */

// FUNC 0x00216308 ArcadeRecord::ArcadeRecord
ArcadeRecord::ArcadeRecord()
{
    /* the BestPlace member constructor (inline: place = 8), then clear() */
    clear();
}

// FUNC 0x00216338 ArcadeRecord::clear
void ArcadeRecord::clear()
{
    int slot;

    for (slot = 3; slot >= 0; slot--)
        times[slot] = RACE_TIME_INVALID;
    place.clear();
}

/* 1 if p is better than the best place (it is then taken). */
// FUNC 0x00216370 ArcadeRecord::updatePlace
int ArcadeRecord::updatePlace(int p)
{
    return place.update(p);
}

// FUNC 0x00216390 ArcadeRecord::getPlace
int ArcadeRecord::getPlace()
{
    return place.get();
}

/* Inserts time t into the 4 best (unsigned compares; an empty slot or a
 * slower-or-equal time makes room): returns the slot it went to, or -1 if t
 * is slower than all four. */
// FUNC 0x00216398 ArcadeRecord::addTime
int ArcadeRecord::addTime(u32 t)
{
    int rank, slot;

    for (rank = 0; rank < 4; rank++) {
        if (times[rank] == RACE_TIME_INVALID || !(times[rank] < t))
            break;
    }
    if (rank < 4) {
        for (slot = 3; rank < slot; slot--)
            times[slot] = times[slot - 1];
        times[rank] = t;
        return rank;
    }
    return -1;
}

// FUNC 0x00216460 ArcadeRecord::getTime
u32 ArcadeRecord::getTime(int i)
{
    return times[i];
}

// FUNC 0x00216470 ArcadeRecord::getSize
int ArcadeRecord::getSize()
{
    return BestPlace::getSize() + 0x10;         /* + times[4] */
}

// FUNC 0x00216490 ArcadeRecord::load
const u8 *ArcadeRecord::load(const u8 *src, int arg)
{
    u32 n = sizeof(times);                      /* in a variable, egcs calls memcpy */

    memcpy(times, src, n);
    return place.load(src + n, arg);
}

// FUNC 0x002164E0 ArcadeRecord::save
u8 *ArcadeRecord::save(u8 *dst)
{
    u32 n = sizeof(times);

    memcpy(dst, times, n);
    return place.save(dst + n);
}

/* ---- ArcadeRecords ------------------------------------------------------- */

// FUNC 0x00216528 ArcadeRecords::ArcadeRecords
ArcadeRecords::ArcadeRecords()
{
    /* implicit: ArcadeRecord (compiler-generated: bases, members, deleting form) */
    /* the 144 element constructors (ArcadeRecord::ArcadeRecord, a
     * count-down loop), then clear() */
    clear();
}

// FUNC 0x00216590 ArcadeRecords::clear
void ArcadeRecords::clear()
{
    int i;

    for (i = 0; i < 144; i++)
        rec[i].clear();
}

/* The difficulty of an arcade race code (its second character). */
// FUNC 0x002165D0 ArcadeRecords::difficultyOf
u32 ArcadeRecords::difficultyOf(const char *race)
{
    switch (race[1]) {
    case 'E': return ARCADE_EASY;
    case 'N': return ARCADE_NORMAL;
    case 'H': return ARCADE_HARD;
    case 'P': return ARCADE_PROFESSIONAL;
    default:  return ARCADE_DIFFICULTIES;
    }
}

/* The record of course `course' (a nameToCode code) at the difficulty of
 * `race'.  Quirks kept: an unknown difficulty gives rec[-1], an unknown
 * course rec[difficulty * n - 1]. */
// FUNC 0x00216640 ArcadeRecords::find
ArcadeRecord *ArcadeRecords::find(const char *race, s64 course)
{
    u32 difficulty = difficultyOf(race);
    int index = -1;

    if (difficulty < ARCADE_DIFFICULTIES) {
        int found = -1;
        const char *list = ARCADE_COURSE_LIST;  /* one register for both calls, as retail */
        int i = 0;
        int courses = product_config()->courseCount(list);

        for (; i < courses; i++) {
            if ((s64)nameToCode(product_config()->courseName(i, list)) == course) {
                found = i;
                break;
            }
        }
        index = difficulty * courses + found;
    }
    return &rec[index];
}

/* (No callers.) */
// FUNC 0x00216720 ArcadeRecords::at
ArcadeRecord *ArcadeRecords::at(int difficulty, int course)
{
    return &rec[difficulty * product_config()->courseCount(ARCADE_COURSE_LIST) + course];
}

/* Records place p; if it is a new best, the GameStatus updates the arcade
 * progress. */
// FUNC 0x00216780 ArcadeRecords::updatePlace
void ArcadeRecords::updatePlace(const char *race, s64 course, int p)
{
    if (find(race, course)->updatePlace(p) != 0)
        getGameStatus()->updateArcadeProgress();
}

// FUNC 0x002167C8 ArcadeRecords::getPlace
int ArcadeRecords::getPlace(const char *race, s64 course)
{
    return find(race, course)->getPlace();
}

/* (No callers.) */
// FUNC 0x002167F0 ArcadeRecords::addTime
int ArcadeRecords::addTime(const char *race, s64 course, u32 t)
{
    return find(race, course)->addTime(t);
}

/* (No callers.) */
// FUNC 0x00216820 ArcadeRecords::getTime
u32 ArcadeRecords::getTime(const char *race, s64 course, int i)
{
    return find(race, course)->getTime(i);
}

/* 1 if the first `count' courses (all of them if count < 0) have been won
 * at `difficulty'.  (A result variable and `break': retail's loop is entered
 * by a jump to its test and returns the variable's register.) */
// FUNC 0x00216850 ArcadeRecords::allWon
int ArcadeRecords::allWon(int difficulty, int count)
{
    int result = 1;
    int i = 0;
    int courses = product_config()->courseCount(ARCADE_COURSE_LIST);
    int first = courses * difficulty;

    if (!(-1 < count))
        count = courses;
    for (; i < count; i++) {
        if (rec[first + i].getPlace() != PLACE_WON) {
            result = 0;
            break;
        }
    }
    return result;
}

// FUNC 0x00216900 ArcadeRecords::allWonEasy
int ArcadeRecords::allWonEasy()
{
    return allWon(ARCADE_EASY, -1);
}

// FUNC 0x00216920 ArcadeRecords::allWonNormal
int ArcadeRecords::allWonNormal()
{
    return allWon(ARCADE_NORMAL, -1);
}

// FUNC 0x00216940 ArcadeRecords::allWonHard
int ArcadeRecords::allWonHard()
{
    return allWon(ARCADE_HARD, -1);
}

// FUNC 0x00216960 ArcadeRecords::allWonProfessional
int ArcadeRecords::allWonProfessional()
{
    return allWon(ARCADE_PROFESSIONAL, -1);
}

/* The arcade completion score: per course 3 if won at hard, else 2 if won
 * at normal, else 1 if won at easy (professional does not count).  Called by
 * GameStatus::getCompletionRatio.  (Each loop indexes rec[] itself: retail
 * computes its pointers per loop.) */
// FUNC 0x00216980 ArcadeRecords::score
int ArcadeRecords::score()
{
    int s = 0;
    int courses = product_config()->courseCount(ARCADE_COURSE_LIST);
    int i;

    for (i = 0; i < courses; i++) {
        if (rec[i + ARCADE_EASY * courses].getPlace() == PLACE_WON ||
            rec[i + ARCADE_NORMAL * courses].getPlace() == PLACE_WON ||
            rec[i + ARCADE_HARD * courses].getPlace() == PLACE_WON)
            s++;
    }
    for (i = 0; i < courses; i++) {
        if (rec[i + ARCADE_NORMAL * courses].getPlace() == PLACE_WON ||
            rec[i + ARCADE_HARD * courses].getPlace() == PLACE_WON)
            s++;
    }
    for (i = 0; i < courses; i++) {
        if (rec[i + ARCADE_HARD * courses].getPlace() == PLACE_WON)
            s++;
    }
    return s;
}

/* The maximum of score(): a constant (34 courses x 3). */
// FUNC 0x00216AE8 ArcadeRecords::maxScore
int ArcadeRecords::maxScore()
{
    return 102;
}

// FUNC 0x00216AF0 ArcadeRecords::getSize
int ArcadeRecords::getSize()
{
    return ArcadeRecord::getSize() * 144;
}

/* arg is not passed on: every record gets 0. */
// FUNC 0x00216B18 ArcadeRecords::load
const u8 *ArcadeRecords::load(const u8 *src, int arg)
{
    int i;

    for (i = 0; i < 144; i++)
        src = rec[i].load(src, 0);
    return src;
}

// FUNC 0x00216B70 ArcadeRecords::save
u8 *ArcadeRecords::save(u8 *dst)
{
    int i;

    for (i = 0; i < 144; i++)
        dst = rec[i].save(dst);
    return dst;
}
