/* game_options_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u097,
 * 0x00216308..0x002185D8.
 *
 * Translation units (all parts of the save data, gt2k_game::GameStatus):
 *   T424  ArcadeRecords (GameStatus+0x23BB8): per arcade difficulty (E, N, H,
 *         P = the second letter of the race code "AECT0001"...) and per
 *         "SINGLE_RACE" course, the 4 best times and the best place.
 *   T434  CourseBestTimes (GameStatus+0x23B08, ? name): 44 best times, one
 *         per entry of the product database's table at +0x1C (? which list).
 *   T425  EventResults (GameStatus+0x23510): the best place of each of the
 *         382 GT mode events (the product database's event table), and the
 *         "series complete" checks behind the ending / the prizes.
 *   T426  ControllerConfig (GameOptions+0x638, ? name): the key assignments
 *         of 4 controller modes x 2 ports and the USB wheel calibration.
 *   T427  gt2k_game::MenuStatus (GameStatus+0x307C8): the menu state (page
 *         to go to, current event, the cars of the race modes, ...); and
 *         T427's unreferenced copies of GarageCar's inline destructors and
 *         typeinfo functions.
 *   T428  GameOptions (GameStatus+0x2FF38, 0x888 bytes, ? name): the game
 *         options and their defaults.
 *
 * Every class is defined in its canonical header (include/game/T424..T428,
 * T434).  Since the data pass (2026-10-17) each source includes the
 * canonical headers it needs itself, in the order that gives the original's
 * type names (each file's comment lists them); this header has the unit's
 * local constants only.  Save data: every class has a static getSize(),
 * load(src[, arg]) (returns the end of what it read; arg is unused) and
 * save(dst) (returns the end).
 */
#ifndef U097_H
#define U097_H

#include "types.h"
#include "libc.h"                       /* memcpy, memset, strcpy, strlen (core 0x00269A68,
                                           the MMI strlen) */
#include <stddef.h>

/* ======================================================================== */
/* Local constants                                                          */
/* ======================================================================== */

/* BestPlace (T418) values: lower is better. */
#define PLACE_WON   0                   /* first place */
#define PLACE_NONE  8                   /* never raced */

/* The course list of the arcade single races (ArcadeRecords). */
#define ARCADE_COURSE_LIST "SINGLE_RACE"

/* ArcadeRecords::difficultyOf: the second letter of an arcade race code. */
enum ArcadeDifficulty {
    ARCADE_EASY         = 0,            /* 'E' */
    ARCADE_NORMAL       = 1,            /* 'N' */
    ARCADE_HARD         = 2,            /* 'H' */
    ARCADE_PROFESSIONAL = 3,            /* 'P' */
    ARCADE_DIFFICULTIES = 4             /* also "not an arcade difficulty" */
};

/* The GT mode leagues: RaceEvent::difficulty (T484) of a series' races. */
enum GtLeague {
    LEAGUE_BEGINNER     = 1,
    LEAGUE_AMATEUR      = 2,
    LEAGUE_PROFESSIONAL = 3
};

/* The race_name_split kinds EventResults checks (RACE_KIND_GT_ENDURANCE,
 * RACE_KIND_GT_DIRT): enum RaceKind, game/data/T489_car_database.h. */

/* MenuStatus::raceCarKind[]: where raceCar[player] comes from. */
enum RaceCarKind {
    RACE_CAR_BY_CODE    = 0,            /* built from a car code in the player's buffer */
    RACE_CAR_GARAGE     = 1,            /* a garage entry itself */
    RACE_CAR_COPY       = 2,            /* a copy in the player's buffer */
    RACE_CAR_NONE       = 3
};

/* ControllerConfig: the controller modes (keys[mode]) and the wheel modes
 * that have a calibration (calib[mode - CONFIG_WHEEL_FIRST]): T426's
 * ControllerMode. */

/* WheelCalibration defaults (raw USB report ranges). */
#define WHEEL_STEER_RANGE   0x400       /* steering 0..0x400, centre 0x200 */
#define WHEEL_PEDAL_RANGE   0x100       /* the two pedal axes 0..0x100 */

#endif /* U097_H */
