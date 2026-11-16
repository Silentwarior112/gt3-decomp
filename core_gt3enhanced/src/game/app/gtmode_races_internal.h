/* gtmode_races_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u017,
 * 0x00131410..0x00134630, second pass.
 *
 * Translation units:
 *   T053  the race modes of the GT mode (game/app/T053_gtmode_races.h): the
 *         machine test, the licence tests, one GT mode race and a
 *         championship, each a race under a short menu (T102); plus T053's
 *         unreferenced out-of-line copies of inline functions of other
 *         classes (the ShortMenuArgs entry lines' clear(), Cell<GarageCar>'s
 *         and GarageCar's destructors and typeinfo functions).
 *   T054  the 2-player split screen battle of the arcade mode
 *         (game/app/T054_split_battle.h) and its copies of the two entry
 *         lines' clear().
 *
 * Every class is defined in a canonical header (include/game/).  Each source
 * includes the canonical headers it needs itself, in the order that gives
 * the original's typeinfo names (the data pass, ref/data/u017/notes.md:
 * egcs writes them in the reverse order of the classes' definitions, and a
 * header that defines classes the original did not see adds names retail
 * does not have); this header has only what this unit uses alone: named
 * constants for the values the race modes pass around, and (GCC 14) T053's
 * copy of GarageCar's typeinfo base list.
 */
#ifndef U017_H
#define U017_H

#include "types.h"

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */
/* race_begin's control record pool (AutomobileControlRecord::allocBuffers):
 * buffers of 0x20000 bytes, 2 of them (u001's / u016's names). */
enum { CONTROL_RECORD_SIZE = 0x20000, CONTROL_RECORDS = 2 };

/* RacePS2Base::virtual_41's / RaceBase::virtual_8's `replay' (RaceBase::
 * replayMode): RACE_LIVE / RACE_REPLAY / RACE_DEMO (T129). */

/* MenuStatus::raceCarKind[]: where raceCar[player] comes from (u097's
 * names, as u016). */
enum {
    RACE_CAR_BY_CODE = 0,                   /* built from a car code: an arcade car */
    RACE_CAR_GARAGE  = 1,                   /* a garage entry itself */
    RACE_CAR_COPY    = 2,                   /* a copy in the player's buffer */
    RACE_CAR_NONE    = 3
};


/* The machine test kinds MACHINE_TEST_400M / 1000M / MAX_SPEED: T415. */

/* make_ranking_code's kind: RankingKind (T431). */

/* The longest time a record takes: the ranking code's time field has 22
 * bits (RankingRecord::time, T431); the max speed test checks its speed
 * x 1000 against the same range. */
enum { RECORD_TIME_MAX = 0x3FFFFF };

/* The number of places of the machine test tables the short menu shows
 * (the first three of ShortMenuArgs::rank; the tables have 12). */
enum { RECORD_LINES_SHOWN = 3 };

/* license_record_result's *flags bits (T102). */
enum {
    LICENSE_FLAG_PASSED   = 1,              /* the licence is passed now and was not before */
    LICENSE_FLAG_ALL_GOLD = 2               /* the same for all gold */
};

/* The licence index of LicenseRecords for the race kinds
 * RACE_KIND_LICENSE_B .. _R (T489) of a licence test's code: T420's
 * LicenseIndex. */

/* LicenseMedal, one worse than bronze: no medal yet in this session. */
enum { LICENSE_MEDAL_WORST = 3 };

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* The places of a race that win the prize money without the consolation
 * screen (trophy_prize_screen's `consolation' is 0 for places 0..2). */
enum { PODIUM_PLACES = 3 };

/* A race of fewer cars shows the prize screen only for a win. */
enum { PRIZE_ALL_PLACES_MIN_CARS = 6 };

/* trophy_get's trophy (T060): a random "champ_NN" cup (0..9 pick one). */
enum { TROPHY_RANDOM = -1 };

/* ======================================================================== */
/* RTTI (modern compiler only; egcs generates these itself)                 */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __class_type_info::base_info */
/* T053's copy of GarageCar's base list (the same words as T036's
 * g_00311EA0): {Cell<GarageCar> 0x004B1350, offset 0, public},
 * {CarInformation 0x004B2A50, offset 0x10, public}.  egcs writes it itself
 * (after the type name "9GarageCar" in T053's .rodata). */
extern const __class_type_info::base_info g_00316448[2];   /* 0x00316448 */
#endif

#endif /* U017_H */
