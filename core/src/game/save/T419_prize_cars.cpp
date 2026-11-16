/* T419 (0x00213ED8..0x00215048): the prize cars a race gives.
 *
 * A prize set (the product database's, product_config) is named after what
 * wins it: a race code ("...0000" every race of a series won, "...NN00" a
 * championship, "...000N" an endurance race, "L0B0000".. a licence's all
 * gold) or an event ("arcade_easy_complete", "gtmode_complete",
 * "achieve_half", ...).  prize_set_classify turns the name into a reason
 * (PrizeReason, prize_reason_name) and a category (PrizeCategory);
 * PrizeCarList checks each set against the race just run and keeps, per
 * category, the first set that is won now if it is still to be given. */
#include "game/save/prize_records_internal.h"
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult, its type name the
                                               file's last, so defined before the garage's) */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus (and through
                                               it T424 ArcadeRecords, T425 EventResults) */
#include "game/save/T419_prize_cars.h"

// FUNC 0x00213ED8 prize_reason_name
const char *prize_reason_name(s8 reason)
{
    switch (reason) {
    case PRIZE_REASON_UNKNOWN:                      return "UNKNOWN_REASON";
    case PRIZE_REASON_SOLO_RACE_CLEAR:              return "SOLO_RACE_CLEAR";
    case PRIZE_REASON_CHAMPIONSHIP_WIN:             return "CHAMPIONSHIP_WIN";
    case PRIZE_REASON_ENDURO_WIN:                   return "ENDURO_WIN";
    case PRIZE_REASON_LICENSE_B_ALL_GOLD:           return "LICENSE_B_ALL_GOLD";
    case PRIZE_REASON_LICENSE_A_ALL_GOLD:           return "LICENSE_A_ALL_GOLD";
    case PRIZE_REASON_LICENSE_IB_ALL_GOLD:          return "LICENSE_IB_ALL_GOLD";
    case PRIZE_REASON_LICENSE_IA_ALL_GOLD:          return "LICENSE_IA_ALL_GOLD";
    case PRIZE_REASON_LICENSE_S_ALL_GOLD:           return "LICENSE_S_ALL_GOLD";
    case PRIZE_REASON_LICENSE_D_ALL_GOLD:           return "LICENSE_D_ALL_GOLD";
    case PRIZE_REASON_ARCADE_EASY_COMPLETE:         return "ARCADE_EASY_COMPLETE";
    case PRIZE_REASON_ARCADE_NORMAL_COMPLETE:       return "ARCADE_NORMAL_COMPLETE";
    case PRIZE_REASON_ARCADE_HARD_COMPLETE:         return "ARCADE_HARD_COMPLETE";
    case PRIZE_REASON_ARCADE_PRO_COMPLETE:          return "ARCADE_PRO_COMPLETE";
    case PRIZE_REASON_GTMODE_BEGINNER_COMPLETE:     return "GTMODE_BEGINNER_COMPLETE";
    case PRIZE_REASON_GTMODE_AMA_COMPLETE:          return "GTMODE_AMA_COMPLETE";
    case PRIZE_REASON_GTMODE_PRO_COMPLETE:          return "GTMODE_PRO_COMPLETE";
    case PRIZE_REASON_GTMODE_DIRT_COMPLETE:         return "GTMODE_DIRT_COMPLETE";
    case PRIZE_REASON_GTMODE_ENDURO_COMPLETE:       return "GTMODE_ENDURO_COMPLETE";
    case PRIZE_REASON_ACHIEVE_50:                   return "ACHIEVE_50";
    case PRIZE_REASON_ACHIEVE_75:                   return "ACHIEVE_75";
    case PRIZE_REASON_ACHIEVE_100:                  return "ACHIEVE_100";
    case PRIZE_REASON_GTMODE_COMPLETE:              return "GTMODE_COMPLETE";
    case PRIZE_REASON_ARCADE_COMPULSOLY_COMPLETE:   return "ARCADE_COMPULSOLY_COMPLETE";   /* (sic) */
    }
    return "???";
}

// FUNC 0x00214038 prize_reason_valid
int prize_reason_valid(s8 reason)
{
    return (u8)reason < PRIZE_REASON_COUNT;
}

/* The GT mode league of a race code: 'G'.. "_e_" 1 (beginner), "_n_" 2
 * (amateur), "_h_" 3 (professional); 'D' 4; 'E' 5; else 0. */
// FUNC 0x00214048 race_league
int race_league(const char *race)
{
    if (race == NULL)                   /* on its own: retail moves race into $a0 again for */
        return 0;                       /* strlen (with `race != NULL &&' jump2 drops the move) */
    if ((int)strlen(race) > 0) {
        if (race[0] == 'G') {
            if (strstr(race, "_e_"))
                return 1;
            if (strstr(race, "_n_"))
                return 2;
            if (strstr(race, "_h_"))
                return 3;
        } else if (race[0] == 'E')
            return 5;
        else if (race[0] == 'D')
            return 4;
    }
    return 0;
}

/* *reason and *category of prize set `set'; neither is written for an unknown
 * name, only *category (PRIZE_CATEGORY_RACE) for an "L...0000" that is not a
 * licence. */
// FUNC 0x002140F8 prize_set_classify
void prize_set_classify(const char *set, s8 *reason, PrizeCategory *category)
{
    int len = strlen(set);

    if (len >= 5 && set[len - 4] == '0' && set[len - 3] == '0'
        && set[len - 2] == '0' && set[len - 1] == '0') {
        /* "...0000": a series of races, or a licence */
        if (set[0] == 'L') {
            if (str_equal(set, "L0B0000")) {
                *reason = PRIZE_REASON_LICENSE_B_ALL_GOLD;
            } else if (str_equal(set, "L0A0000")) {
                *reason = PRIZE_REASON_LICENSE_A_ALL_GOLD;
            } else if (str_equal(set, "LIB0000")) {
                *reason = PRIZE_REASON_LICENSE_IB_ALL_GOLD;
            } else if (str_equal(set, "LIA0000")) {
                *reason = PRIZE_REASON_LICENSE_IA_ALL_GOLD;
            } else if (str_equal(set, "L0S0000")) {
                *reason = PRIZE_REASON_LICENSE_S_ALL_GOLD;
            } else if (str_equal(set, "L0R0000")) {
                *reason = PRIZE_REASON_LICENSE_D_ALL_GOLD;
            }
        } else {
            *reason = PRIZE_REASON_SOLO_RACE_CLEAR;
        }
        *category = PRIZE_CATEGORY_RACE;
    } else if (len >= 5 && set[len - 4] >= '0' && set[len - 4] <= '9'
               && set[len - 3] >= '0' && set[len - 3] <= '9'
               && set[len - 2] == '0' && set[len - 1] == '0') {
        /* "...NN00": a championship */
        *reason = PRIZE_REASON_CHAMPIONSHIP_WIN;
        *category = PRIZE_CATEGORY_RACE;
    } else if (len >= 5 && set[len - 4] == '0' && set[len - 3] == '0'
               && set[len - 2] == '0' && set[len - 1] >= '1' && set[len - 1] <= '9') {
        /* "...000N": an endurance race */
        *reason = PRIZE_REASON_ENDURO_WIN;
        *category = PRIZE_CATEGORY_RACE;
    } else if (str_equal(set, "arcade_compulsory_complete")) {
        *reason = PRIZE_REASON_ARCADE_COMPULSOLY_COMPLETE;
        *category = PRIZE_CATEGORY_RACE;
    } else if (str_equal(set, "arcade_easy_complete")) {
        *reason = PRIZE_REASON_ARCADE_EASY_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "arcade_normal_complete")) {
        *reason = PRIZE_REASON_ARCADE_NORMAL_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "arcade_hard_complete")) {
        *reason = PRIZE_REASON_ARCADE_HARD_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "arcade_pro_complete")) {
        *reason = PRIZE_REASON_ARCADE_PRO_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_beginner_complete")) {
        *reason = PRIZE_REASON_GTMODE_BEGINNER_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_amateur_complete")) {
        *reason = PRIZE_REASON_GTMODE_AMA_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_pro_complete")) {
        *reason = PRIZE_REASON_GTMODE_PRO_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_dirt_complete")) {
        *reason = PRIZE_REASON_GTMODE_DIRT_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_enduro_complete")) {
        *reason = PRIZE_REASON_GTMODE_ENDURO_COMPLETE;
        *category = PRIZE_CATEGORY_SERIES;
    } else if (str_equal(set, "gtmode_complete")) {
        *reason = PRIZE_REASON_GTMODE_COMPLETE;
        *category = PRIZE_CATEGORY_GTMODE_COMPLETE;
    } else if (str_equal(set, "achieve_half")) {
        *reason = PRIZE_REASON_ACHIEVE_50;
        *category = PRIZE_CATEGORY_ACHIEVE_HALF;
    } else if (str_equal(set, "achieve_3_quoter")) {        /* (sic) */
        *reason = PRIZE_REASON_ACHIEVE_75;
        *category = PRIZE_CATEGORY_ACHIEVE_3_QUARTERS;
    } else if (str_equal(set, "achieve_complete")) {
        *reason = PRIZE_REASON_ACHIEVE_100;
        *category = PRIZE_CATEGORY_ACHIEVE_COMPLETE;
    }
}

/* 1 if a and b are equal but for their last 4 characters. */
// FUNC 0x00214480 race_same_series
int race_same_series(const char *a, const char *b)
{
    int lenA = strlen(a);
    int lenB = strlen(b);
    char baseA[0x40];                           /* sp+0x00 */
    char baseB[0x40];                           /* sp+0x40 */
    strcpy(baseA, a);
    strcpy(baseB, b);
    baseA[lenA - 4] = 0;
    baseB[lenB - 4] = 0;
    return strcmp(baseA, baseB) == 0;
}

/* PRIZE_CATEGORY_RACE: race `race' (its kind from race_name_split) against
 * set `set' of reason `reason'. */
// FUNC 0x00214510 prize_check_race
PrizeCheck prize_check_race(const char *race, const char *set, s8 reason)
{
    int kind;                                   /* sp+0xC0 */
    int index;                                  /* sp+0xC4 */
    PrizeCheck result;                          /* sp+0x00 */

    race_name_split(race, &kind, &index);
    result.ok = 0;
    result.reason = reason;
    switch (kind) {
    case RACE_KIND_TIME_TRIAL:
        if (reason != PRIZE_REASON_ARCADE_COMPULSOLY_COMPLETE)
            return result;
        result.ok = getGameStatus()->getTimeTrialRecords()->allFirstPlaces();
        return result;
    case RACE_KIND_GT_SINGLE:
    case RACE_KIND_GT_DIRT:
        /* every race "<race without its last 2 characters>01".."09" that
         * exists has been won */
        if (reason != PRIZE_REASON_SOLO_RACE_CLEAR || !race_same_series(race, set))
            return result;
        {
            u8 allWon = 1;
            EventResults *events = getGameStatus()->getEventResults();
            char series[0x40];                  /* sp+0x10 */
            char member[0x40];                  /* sp+0x50 */
            int len = strlen(race);
            strcpy(series, race);
            series[len - 2] = 0;
            for (int i = 1; i < 10; i++) {
                sprintf(member, "%s%02d", series, i);
                RaceDataBase db;                /* sp+0x90 */
                if (db.select(nameToCode(member))) {
                    if (events->getPlace(member) != 0) {
                        allWon = 0;
                        break;
                    }
                } else
                    break;
            }
            result.ok = allWon;
            return result;
        }
    case RACE_KIND_GT_CHAMPIONSHIP:
        if (reason != PRIZE_REASON_CHAMPIONSHIP_WIN || !race_same_series(race, set))
            return result;
        result.ok = getGameStatus()->getEventResults()->getPlace(race) == 0;
        return result;
    case RACE_KIND_GT_ENDURANCE:
        if (reason != PRIZE_REASON_ENDURO_WIN || !race_same_series(race, set))
            return result;
        result.ok = getGameStatus()->getEventResults()->getPlace(race) == 0;
        return result;
    case RACE_KIND_LICENSE_B:
        if (reason != PRIZE_REASON_LICENSE_B_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(0);
        return result;
    case RACE_KIND_LICENSE_A:
        if (reason != PRIZE_REASON_LICENSE_A_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(1);
        return result;
    case RACE_KIND_LICENSE_IB:
        if (reason != PRIZE_REASON_LICENSE_IB_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(2);
        return result;
    case RACE_KIND_LICENSE_IA:
        if (reason != PRIZE_REASON_LICENSE_IA_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(3);
        return result;
    case RACE_KIND_LICENSE_S:
        if (reason != PRIZE_REASON_LICENSE_S_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(4);
        return result;
    case RACE_KIND_LICENSE_R:
        if (reason != PRIZE_REASON_LICENSE_D_ALL_GOLD)
            return result;
        result.ok = getGameStatus()->getLicenseRecords()->isAllGold(5);
        return result;
    }
    return result;
}

/* PRIZE_CATEGORY_SERIES: the arcade difficulties (race[1] 'E', 'N', 'H',
 * 'P') and the GT mode leagues and dirt / endurance events.
 * Not exact (94.5%, 160 of 163 instructions; kept honest, no fakematch):
 * retail keeps a second copy of `league' in $s3, made before getGameStatus
 * (a pseudo that crosses the call) and used only by the case tree's `== 3'
 * test, hence one more saved register and a 0x70 frame (ours 0x60).
 * Why (ref/matching/u096/notes.md, follow-up 2026-10-17, from the RTL
 * dumps): CSE does split a copied switch index like that (`== 3' is on its
 * own extended-block path), so a plain `int lq = league;' before the call
 * gives retail's code with -fno-gcse; but GCSE's copy propagation folds the
 * copy back into `league' (two passes, one level of copies each).  Retail's
 * copy survived GCSE, so its RTL had a longer copy chain or the copy was
 * not a register when CSE1 ran.  An exact form exists: the copy passed
 * through a no-op inline taking `const int &' (the address keeps it in
 * memory until purge_addressof, after CSE1, so cprop stops one copy short
 * and CSE2 splits as retail; build/scratch/u096/fu2/fakematch_T419.cpp),
 * but nothing in the original would take that address, so it is not kept.
 * Also tried (about 80 forms, the reference compiler 991111-01 the same):
 * league as u32 / s16 / s8 / u8 / enum / const / a const reference, a copy
 * in a second variable before or after the call, inline wrappers by value,
 * `switch ((u32)league)', other case orders, an empty case 0, a default,
 * an if chain, dead stores, do { } while (0), narrower race_league return
 * types, other flags.  Consequence: the function is 12 bytes shorter than
 * retail's, so its FDE (T419's frame info, 8 bytes) differs; the full link
 * places T419's .data anyway (its variables are exact). */
// FUNC 0x00214868 prize_check_series
PrizeCheck prize_check_series(const char *race, s8 reason)
{
    int kind;                                   /* sp+0x10 */
    int index;                                  /* sp+0x14 */
    PrizeCheck result;                          /* sp+0x00 */

    race_name_split(race, &kind, &index);
    result.ok = 0;
    result.reason = reason;
    switch (kind) {
    case RACE_KIND_ARCADE:
    case RACE_KIND_TIME_TRIAL:
        if ((int)strlen(race) >= 3) {
            ArcadeRecords *arcade = getGameStatus()->getArcadeRecords();
            switch (race[1]) {
            case 'E':
                if (reason != PRIZE_REASON_ARCADE_EASY_COMPLETE)
                    return result;
                result.ok = arcade->allWonEasy();
                return result;
            case 'N':
                if (reason != PRIZE_REASON_ARCADE_NORMAL_COMPLETE)
                    return result;
                result.ok = arcade->allWonNormal();
                return result;
            case 'H':
                if (reason != PRIZE_REASON_ARCADE_HARD_COMPLETE)
                    return result;
                result.ok = arcade->allWonHard();
                return result;
            case 'P':
                if (reason != PRIZE_REASON_ARCADE_PRO_COMPLETE)
                    return result;
                result.ok = arcade->allWonProfessional();
                return result;
            }
        }
        /* fall through: any other code (a time trial "AT..") is checked as
         * an endurance race */
    case RACE_KIND_GT_ENDURANCE:
        if (reason != PRIZE_REASON_GTMODE_ENDURO_COMPLETE)
            return result;
        result.ok = getGameStatus()->getEventResults()->enduranceEventsWon();
        return result;
    case RACE_KIND_GT_DIRT:
        if (reason != PRIZE_REASON_GTMODE_DIRT_COMPLETE)
            return result;
        result.ok = getGameStatus()->getEventResults()->dirtEventsWon();
        return result;
    case RACE_KIND_GT_SINGLE:
    case RACE_KIND_GT_CHAMPIONSHIP: {
        int league = race_league(race);
        EventResults *events = getGameStatus()->getEventResults();
        switch (league) {
        case 1:
            if (reason != PRIZE_REASON_GTMODE_BEGINNER_COMPLETE)
                return result;
            result.ok = events->beginnerLeagueWon();
            return result;
        case 2:
            if (reason != PRIZE_REASON_GTMODE_AMA_COMPLETE)
                return result;
            result.ok = events->amateurLeagueWon();
            return result;
        case 3:
            if (reason != PRIZE_REASON_GTMODE_PRO_COMPLETE)
                return result;
            result.ok = events->professionalLeagueWon();
            return result;
        }
        break;
    }
    }
    return result;
}

/* PRIZE_CATEGORY_GTMODE_COMPLETE: after a GT mode race (or a licence test or
 * machine test: any kind 5..17). */
// FUNC 0x00214AF8 prize_check_gtmode_complete
PrizeCheck prize_check_gtmode_complete(const char *race, s8 reason)
{
    int kind;                                   /* sp+0x10 */
    int index;                                  /* sp+0x14 */
    PrizeCheck result;                          /* sp+0x00 */

    race_name_split(race, &kind, &index);
    result.ok = 0;
    result.reason = reason;
    if (kind < RACE_KIND_COUNT) {
        if (kind >= RACE_KIND_GT_SINGLE) {
            if (reason != PRIZE_REASON_GTMODE_COMPLETE)
                return result;
            result.ok = getGameStatus()->getEventResults()->allWon();
            return result;
        }
    }
    return result;
}

// FUNC 0x00214BB8 prize_check_achieve_half
PrizeCheck prize_check_achieve_half(s8 reason)
{
    PrizeCheck result;
    result.ok = 0;
    result.reason = reason;
    if (reason != PRIZE_REASON_ACHIEVE_50)
        return result;
    result.ok = getGameStatus()->getCompletionRatio() >= 0.5f;
    return result;
}

// FUNC 0x00214C40 prize_check_achieve_3_quarters
PrizeCheck prize_check_achieve_3_quarters(s8 reason)
{
    PrizeCheck result;
    result.ok = 0;
    result.reason = reason;
    if (reason != PRIZE_REASON_ACHIEVE_75)
        return result;
    result.ok = getGameStatus()->getCompletionRatio() >= 0.75f;
    return result;
}

// FUNC 0x00214CC8 prize_check_achieve_complete
PrizeCheck prize_check_achieve_complete(s8 reason)
{
    PrizeCheck result;
    result.ok = 0;
    result.reason = reason;
    if (reason != PRIZE_REASON_ACHIEVE_100)
        return result;
    result.ok = getGameStatus()->getCompletionRatio() >= 1.0f;
    return result;
}

/* For every prize set of the database: a set of fewer than 2 cars is a
 * candidate unless already won (GameStatus::prizeSets); a bigger one only if
 * it is `race' itself and arg < 1 (the callers pass 0 or -1, so always).
 * Once a check of a category has succeeded, the category's later sets are
 * not checked again (and cannot be kept); a candidate that passes is kept
 * for its category.  (A constructor: n = 0 first, returns this.) */
// FUNC 0x00214D50 PrizeCarList::PrizeCarList
PrizeCarList::PrizeCarList(const char *race, int arg)
{
    n = 0;
    if (race == NULL)
        return;

    int categoryDone[PRIZE_CATEGORY_COUNT];             /* sp+0x00 */
    const char *keptSet[PRIZE_CATEGORY_COUNT];          /* sp+0x20 */
    s8 keptReason[PRIZE_CATEGORY_COUNT];                /* sp+0x40 */
    int i;                                      /* the first and the last loop's (retail: both $a3) */
    for (i = 0; i < PRIZE_CATEGORY_COUNT; i++) {
        categoryDone[i] = 0;
        keptSet[i] = NULL;
        keptReason[i] = PRIZE_REASON_UNKNOWN;
    }

    PrizeSetFlags *wonSets = getGameStatus()->getPrizeSetFlags();
    int setCount = product_config()->prizeSetCount();
    s8 reason;                                  /* sp+0x60 */
    PrizeCategory category;                     /* sp+0x80: not reset per set, see prize_set_classify;
                                                   an enum (its own alias set: categoryDone's store
                                                   does not make egcs reload it) */
    for (int i = 0; i < setCount; i++) {
        const char *set = product_config()->prizeSetName(i);
        if (set == NULL)
            continue;

        int candidate = 0;
        if (product_config()->prizeCarCount(set) >= 2) {      /* (retail's block order) */
            if (race != NULL && str_equal(race, set)) {
                if (arg < 1)
                    candidate = 1;
            }
        } else
            candidate = wonSets->isSet(set) == 0;

        PrizeCheck check;                       /* sp+0x50 */
        check.ok = 0;
        check.reason = PRIZE_REASON_UNKNOWN;
        prize_set_classify(set, &reason, &category);
        if (categoryDone[category] != 1) {
            switch ((u32)category) {
            case PRIZE_CATEGORY_RACE:
                check = prize_check_race(race, set, reason);
                break;
            case PRIZE_CATEGORY_SERIES:
                check = prize_check_series(race, reason);
                break;
            case PRIZE_CATEGORY_GTMODE_COMPLETE:
                check = prize_check_gtmode_complete(race, reason);
                break;
            case PRIZE_CATEGORY_ACHIEVE_HALF:
                check = prize_check_achieve_half(reason);
                break;
            case PRIZE_CATEGORY_ACHIEVE_3_QUARTERS:
                check = prize_check_achieve_3_quarters(reason);
                break;
            case PRIZE_CATEGORY_ACHIEVE_COMPLETE:
                check = prize_check_achieve_complete(reason);
                break;
            }
            categoryDone[category] = check.isOk();
        }
        if (candidate && check.isOk()) {
            keptSet[category] = set;
            keptReason[category] = check.getReason();
        }
    }

    n = 0;
    for (i = 0; i < PRIZE_CATEGORY_COUNT; i++) {
        if (keptSet[i] != NULL) {
            this->set[n] = keptSet[i];
            this->reason[n] = keptReason[i];
            n++;
        }
    }
}

// FUNC 0x00215020 PrizeCarList::count
int PrizeCarList::count()
{
    return n;
}

// FUNC 0x00215028 PrizeCarList::getSet
const char *PrizeCarList::getSet(int i)
{
    return set[i];
}

// FUNC 0x00215038 PrizeCarList::getReason
s8 PrizeCarList::getReason(int i)
{
    return reason[i];
}
