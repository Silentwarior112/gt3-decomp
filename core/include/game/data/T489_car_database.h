/* T489_car_database.h - queries of the car database g_002EBD88 (translation
 * unit T489, unit u109), 0x00239ED0..0x0023AE98: cars (table 0x1E), car
 * aliases (0x1F, 0x23), races (0x20), car lists (0x21), courses (0x22); the
 * regulation check of a car against a race (RegurationResult, T045); the
 * race-name parsing; the region lock of a car.
 *
 * T489 also has RegurationResult::isOk (0x00239ED0) and weak copies of its
 * inline destructor (0x0023AE30) and typeinfo (0x0023AE60): see
 * T045_gtmode_page_checks.h.
 *
 * The regions are the countries of g_00352FD8 (T516; CarDataBase::isJapan
 * / isUS / isUK = COUNTRY_JP / US / BR): JP (and an unknown value) reads the car
 * record's R0 names, BR the R2 names, the others name1 / name2. */
#ifndef GAME_T489_CAR_DATABASE_H
#define GAME_T489_CAR_DATABASE_H

#include "game/game.h"

class CarInformation;                           /* T483_car_information.h (pointers only) */
class RegurationResult;                         /* T045_gtmode_page_checks.h (returned by value) */
struct RaceEvent;                               /* T484_race_database.h */

/* A record of table 0x20 (a race): u109's name for RaceEvent, whose
 * definition is in T484_race_database.h.  u109's fields: licence = needLicense
 * (0x19B), carList (0x190), unk1D2 = needTire, name (0x1F0), tuning /
 * aspiration / drive = needTuning / needAspiration / needDrivetrain. */
typedef RaceEvent RaceRecord;

int  db_race_index(s64 race);                   /* 0x00239F08 index in table 0x20, -1 if absent (no callers) */
int  db_car_index(s64 car);                     /* 0x00239F30 index in table 0x1E, -1 if absent */
int  db_course_index(s64 course);               /* 0x00239F58 index in table 0x22, -1 if absent */
int  db_course_count(void);                     /* 0x00239F80 */
int  db_car_count(void);                        /* 0x00239FA8 */
s64  db_course_code(int i);                     /* 0x00239FD0 */
s64  db_car_code(int i);                        /* 0x0023A010 */
const char *db_car_name(s64 car);               /* 0x0023A050 the region's name column; NULL if absent */
const char *db_car_name2(s64 car);              /* 0x0023A0D8 the second name column; NULL if absent */
const char *db_car_short_name(s64 code);        /* 0x0023A160 code may be an alias (no callers) */
const char *db_course_name(s64 course);         /* 0x0023A1F0 CourseRecord.name0A */
const char *db_race_name(s64 race);             /* 0x0023A230 RaceEvent.name */
/* check_regulation for a copy of the ridden car (no callers). */
RegurationResult check_regulation_current_car(s64 race);    /* 0x0023A270 */
int  tire_part_index(int part);                 /* 0x0023A3F0 index of a tyre part id in the list 0x002F0670
                                                   (the length if absent; no callers) */
/* Checks car against the regulations of race: each failed rule sets its flag
 * ([0] dirt tyres (part 0x42), [1] licence, [3] drive train, [4] not a
 * regulated car, [5] aspiration, [6] tuning). */
RegurationResult check_regulation(s64 race, CarInformation *car);  /* 0x0023A438 */
int  race_dirt_tire_only(const char *race);     /* 0x0023A818 1 if the race's needTire is 9 (dirt tyres:
                                                   "dirt_tire_only", u013) */
int  F_0023A858(s64 car);                       /* 0x0023A858 car record +0xF8 (no callers) */
int  F_0023A888(s64 car);                       /* 0x0023A888 car record +0xFC (no callers) */
/* The kinds race_name_split gives a race code (the first letters of the code;
 * race_kind_text gives their DATABASERACECATEGORY texts, T493).  u096, u097
 * and u109 found them. */
enum RaceKind {
    RACE_KIND_NONE          = 0,
    RACE_KIND_ARCADE        = 1,                /* "A.."        "Arcade Race" */
    RACE_KIND_2P_BATTLE     = 2,                /* "A2.."       "2P Battle" */
    RACE_KIND_TIME_TRIAL    = 3,                /* "AT<dd>.."   "Time Trial" (TEXT_CATEGORY_LIMITED_TIME_TRIAL) */
    RACE_KIND_FREE_RUN      = 4,                /* "ATA.."      "Free Run" (TEXT_CATEGORY_FREE_TIME_TRIAL) */
    RACE_KIND_GT_SINGLE     = 5,                /* "G..00d"     "Championship" (? a single race / race series) */
    RACE_KIND_GT_CHAMPIONSHIP = 6,              /* other "G.."  "Championship" (? a championship and its races) */
    RACE_KIND_GT_ENDURANCE  = 7,                /* "E.."        "Championship" */
    RACE_KIND_GT_DIRT       = 8,                /* "D.."        "Championship" (the rally events) */
    RACE_KIND_LICENSE_B     = 9,                /* "L0B.."      "License B-"; 9..14: race_licence_split */
    RACE_KIND_LICENSE_A     = 10,               /* "L0A.." */
    RACE_KIND_LICENSE_IB    = 11,               /* "LIB.." */
    RACE_KIND_LICENSE_IA    = 12,               /* "LIA.." */
    RACE_KIND_LICENSE_S     = 13,               /* "L0S.." */
    RACE_KIND_LICENSE_R     = 14,               /* "L0R.."      "License R-" */
    RACE_KIND_MACHINE_400M  = 15,               /* "M4"         "Test 0-400m" */
    RACE_KIND_MACHINE_1000M = 16,               /* "M1"         "Test 0-1000m" */
    RACE_KIND_MACHINE_VMAX  = 17,               /* "MM"         "Test VMAX" */
    RACE_KIND_COUNT         = 18
};

/* The kind of a race name ("A..", "AT<dd>", "G..<dd>", "L..", "M4", ...: see
 * race_kind_text) and an index; kind 0, index -1 if unknown. */
void race_name_split(const char *race, int *kind, int *index);    /* 0x0023A8B8 */
void race_licence_split(const char *race, int *kind, int *index); /* 0x0023AA70 licence tests: 9..14 */
/* The DATABASERACECATEGORY text (T493) of a race kind, NULL if none. */
const char *race_kind_text(int kind);           /* 0x0023AB18 */
int  db_car_is_locked(s64 code);                /* 0x0023ABF0 1 if unknown or locked out of this region */
int  region_lock_check(int lock);               /* 0x0023AC80 1 if a car with region lock `lock`
                                                   (car record +0xF7, 0..8) is not available here */
int  db_car_short_names(const char ***out);     /* 0x0023AD68 the short names of all cars, built once
                                                   (0x00352F00); the number of cars (no callers) */

#endif /* GAME_T489_CAR_DATABASE_H */
