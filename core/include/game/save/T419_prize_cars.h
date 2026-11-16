/* T419_prize_cars.h - the prize cars a race gives (translation unit T419,
 * unit u096), 0x00213ED8..0x00215048.
 *
 * A prize set (T435 database, product_config) is named after what wins it: a
 * race code ("...0000" a race clear, "...NN00" a championship, "...000N" an
 * endurance race, "L0B0000".. a licence's all gold) or an event
 * ("arcade_easy_complete", "gtmode_complete", "achieve_half", ...).
 * prize_set_classify turns the name into a reason (PrizeReason,
 * prize_reason_name) and a category (PrizeCategory); PrizeCarList checks each
 * set against the race just run and keeps, per category, the first set that
 * is won now if it is still to be given (a single-car set not won before, or
 * the race's own multi-car set).  The prize_check_* functions are the checks
 * of the categories. */
#ifndef GAME_T419_PRIZE_CARS_H
#define GAME_T419_PRIZE_CARS_H

#include "game/game.h"

/* Why a prize set is given (PrizeCheck::reason, PrizeCarList::reason): the
 * names prize_reason_name returns, PRIZE_REASON_ + the name.  Stored as s8;
 * prize_reason_valid: 0..22. */
enum PrizeReason {
    PRIZE_REASON_UNKNOWN = -1,
    PRIZE_REASON_SOLO_RACE_CLEAR = 0,           /* set "...0000": every race of the series won */
    PRIZE_REASON_CHAMPIONSHIP_WIN = 1,          /* set "...NN00" */
    PRIZE_REASON_ENDURO_WIN = 2,                /* set "...000N" */
    PRIZE_REASON_LICENSE_B_ALL_GOLD = 3,        /* sets "L0B0000" .. "L0R0000" */
    PRIZE_REASON_LICENSE_A_ALL_GOLD = 4,
    PRIZE_REASON_LICENSE_IB_ALL_GOLD = 5,
    PRIZE_REASON_LICENSE_IA_ALL_GOLD = 6,
    PRIZE_REASON_LICENSE_S_ALL_GOLD = 7,
    PRIZE_REASON_LICENSE_D_ALL_GOLD = 8,        /* the R licence ("L0R0000"; the name says D) */
    PRIZE_REASON_ARCADE_EASY_COMPLETE = 9,
    PRIZE_REASON_ARCADE_NORMAL_COMPLETE = 10,
    PRIZE_REASON_ARCADE_HARD_COMPLETE = 11,
    PRIZE_REASON_ARCADE_PRO_COMPLETE = 12,
    PRIZE_REASON_GTMODE_BEGINNER_COMPLETE = 13,
    PRIZE_REASON_GTMODE_AMA_COMPLETE = 14,
    PRIZE_REASON_GTMODE_PRO_COMPLETE = 15,
    PRIZE_REASON_GTMODE_DIRT_COMPLETE = 16,
    PRIZE_REASON_GTMODE_ENDURO_COMPLETE = 17,
    PRIZE_REASON_ACHIEVE_50 = 18,
    PRIZE_REASON_ACHIEVE_75 = 19,
    PRIZE_REASON_ACHIEVE_100 = 20,
    PRIZE_REASON_GTMODE_COMPLETE = 21,
    PRIZE_REASON_ARCADE_COMPULSOLY_COMPLETE = 22, /* (sic) every time trial course won */
    PRIZE_REASON_COUNT = 23
};

/* The categories of prize sets (prize_set_classify); PrizeCarList keeps one
 * set per category. */
enum PrizeCategory {
    PRIZE_CATEGORY_RACE = 0,                    /* prize_check_race: races, championships,
                                                   endurance races, licences, the time trials */
    PRIZE_CATEGORY_SERIES = 1,                  /* prize_check_series: an arcade difficulty, a GT
                                                   mode league / the dirt / endurance events */
    PRIZE_CATEGORY_GTMODE_COMPLETE = 2,         /* prize_check_gtmode_complete */
    PRIZE_CATEGORY_ACHIEVE_HALF = 3,            /* prize_check_achieve_half */
    PRIZE_CATEGORY_ACHIEVE_3_QUARTERS = 4,      /* prize_check_achieve_3_quarters */
    PRIZE_CATEGORY_ACHIEVE_COMPLETE = 5,        /* prize_check_achieve_complete */
    PRIZE_CATEGORY_COUNT = 6
};

/* The result of a prize check, returned by value (hidden pointer): 4 bytes,
 * the callers copy it as a word.  Size 4.  The constructors are what retail
 * shows: the checks return through a buffer in $a0 (egcs does that only for
 * a class with a constructor), and their `return result;' copies the two
 * members byte by byte, both read back from the local (a copy constructor
 * through an inline setter: its arguments are fresh pseudos; a plain
 * member-wise copy forwards the value just stored, the implicit copy
 * would move the word). */
class PrizeCheck {
public:
    /* set first: an inline member is integrated only after its body */
    void set(u8 newOk, s8 newReason) { ok = newOk; reason = newReason; }   /* ? name */
    PrizeCheck() {}
    PrizeCheck(const PrizeCheck &c) { set(c.ok, c.reason); }
    /* PrizeCarList reads through these: `categoryDone[category] =
     * check.isOk()' (a call: egcs evaluates it before the element's
     * address, as retail) */
    int  isOk() const { return ok; }            /* ? name */
    s8   getReason() const { return reason; }   /* ? name */

    u8 ok;                                      /* 0x0 */
    s8 reason;                                  /* 0x1 the reason (prize_reason_name) */
    u8 unk_2[2];                                /* 0x2 ? never written */
} __attribute__((aligned(4)));
EGCS_SIZE(PrizeCheck, 4);

/* The prize sets race `race' gives: at most one per category (0..5).  A
 * local object of car_get_prizes (u021, T061).  No vtable.  Size 0x24. */
class PrizeCarList {
public:
    /* arg < 1: a set of 2 or more cars counts when it is `race' itself (the
     * arg of car_get_prizes).  u021's call passes a 4th argument the
     * constructor does not read. */
    PrizeCarList(const char *race, int arg);    /* 0x00214D50 */
    int count();                                /* 0x00215020 */
    const char *getSet(int i);                  /* 0x00215028 */
    s8 getReason(int i);                        /* 0x00215038 */

    int n;                                      /* 0x00 */
    const char *set[6];                         /* 0x04 prize set codes (T435) [PRIZE_CATEGORY_COUNT] */
    s8 reason[6];                               /* 0x1C why each is given (PrizeReason) */
};
EGCS_SIZE(PrizeCarList, 0x24);

/* "UNKNOWN_REASON" (-1), "SOLO_RACE_CLEAR" (0) .. "ARCADE_COMPULSOLY_COMPLETE"
 * (22), else "???". */
const char *prize_reason_name(s8 reason);       /* 0x00213ED8 */
int  prize_reason_valid(s8 reason);             /* 0x00214038 (u8) reason < 23 */
/* 'G'.. "_e_" 1 (beginner), "_n_" 2 (amateur), "_h_" 3 (professional); 'D'
 * 4; 'E' 5; else 0. */
int  race_league(const char *race);             /* 0x00214048 */
/* Neither is written for an unknown name, only *category (0) for an
 * "L...0000" that is not a licence. */
void prize_set_classify(const char *set, s8 *reason, PrizeCategory *category); /* 0x002140F8 */
int  race_same_series(const char *a, const char *b); /* 0x00214480 equal but for the last 4 characters */
PrizeCheck prize_check_race(const char *race, const char *set, s8 reason); /* 0x00214510 category 0 */
PrizeCheck prize_check_series(const char *race, s8 reason);           /* 0x00214868 category 1 */
PrizeCheck prize_check_gtmode_complete(const char *race, s8 reason);  /* 0x00214AF8 category 2 */
PrizeCheck prize_check_achieve_half(s8 reason);                       /* 0x00214BB8 */
PrizeCheck prize_check_achieve_3_quarters(s8 reason);                 /* 0x00214C40 */
PrizeCheck prize_check_achieve_complete(s8 reason);                   /* 0x00214CC8 */

#endif /* GAME_T419_PRIZE_CARS_H */
