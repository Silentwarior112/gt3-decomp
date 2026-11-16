/* arcade_race_modes_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u016,
 * 0x0012F840..0x00131410, second pass.
 *
 * Translation unit:
 *   T052  the three arcade race sessions (game/app/T052_arcade_race_modes.h):
 *           run_arcade_race   RaceArcade + RaceArcadeInformation,
 *                             result menu "menu/qm_arc_result.mbl"
 *           run_free_run      RaceFreeRun + RaceFreeRunInformation,
 *                             "menu/qm_arc_tt_free.mbl", records by course
 *           run_time_trial    RaceTimeTrial + RaceSolitaireInformation,
 *                             "menu/qm_arc_tt_ltd.mbl", records by test
 *         and the out-of-line copies of ShortMenuRankEntry's and
 *         ShortMenuResultEntry's inline constructors.
 *
 * Every class is defined in its canonical header (include/game/); since the
 * data pass (2026-10-17) T052 includes the headers it needs itself, in the
 * order that gives retail's type names (see there).  This header has what
 * only this unit uses: named constants for the values the sessions pass
 * around.
 */
#ifndef U016_H
#define U016_H

#include "types.h"
#include <stddef.h>

/* race_begin's control record pool (AutomobileControlRecord::allocBuffers):
 * buffers of 0x20000 bytes, 2 of them (u001's names). */
enum { CONTROL_RECORD_SIZE = 0x20000, CONTROL_RECORDS = 2 };

/* RacePS2Base::virtual_41's / RaceBase::virtual_8's `replay' (RaceBase::
 * replayMode): RACE_LIVE / RACE_REPLAY / RACE_DEMO (T129). */

/* MenuStatus::raceCarKind[]: where raceCar[player] comes from (u097's
 * names). */
enum {
    RACE_CAR_BY_CODE = 0,                   /* built from a car code: an arcade car */
    RACE_CAR_GARAGE  = 1,                   /* a garage entry itself */
    RACE_CAR_COPY    = 2,                   /* a copy in the player's buffer */
    RACE_CAR_NONE    = 3
};


/* make_ranking_code's kind: RankingKind (T431). */

/* The longest time a record takes: the ranking code's time field has 22
 * bits (RankingRecord::time, T431). */
enum { RECORD_TIME_MAX = 0x3FFFFF };

/* The number of places of a record table the short menu shows (the first
 * three of ShortMenuArgs::list / rank; the tables have 12). */
enum { RECORD_LINES_SHOWN = 3 };

#endif /* U016_H */
