/* T127_race_records.h - TimeRecord, SpeedRecord (translation unit T127, unit
 * u043), 0x0017B6B8..0x0017B778.
 *
 * Two small record keepers: the best time (smaller is better; 0x157529FF,
 * RACE_TIME_INVALID of T128, means "nothing yet") and the top speed (larger
 * is better, capped at 999.9; FLT_MAX also counts as unset).  The class
 * names are guesses: the Lupo 2 symbols only name the accessors that return
 * them (RaceEntryCar::getTimeRecord, RaceOrganization::getSpeedRecord).
 * Users: RaceEntryCar +0x11F0 (the car's best lap, T303 updates it),
 * RaceOrganization +0x8A4 / +0x8A8, RaceDisplay (u051 / u052).
 */
#ifndef GAME_T127_RACE_RECORDS_H
#define GAME_T127_RACE_RECORDS_H

#include "game/game.h"

/* Not polymorphic.  Size 4 (RaceEntryCar: +0x11F0, then +0x11F4). */
class TimeRecord {
public:
    TimeRecord();                           /* 0x0017B6B8 time = RACE_TIME_INVALID */
    /* Takes t when nothing is recorded yet or t is smaller (unsigned),
     * clamped to RACE_TIME_INVALID; true when the record changed. */
    bool update(u32 t);                     /* 0x0017B6D0 */

    u32 time;                               /* 0x00 ms */
};
EGCS_SIZE(TimeRecord, 0x4);

/* Not polymorphic.  Size 4 (RaceOrganization: +0x8A4, then +0x8A8). */
class SpeedRecord {
public:
    SpeedRecord();                          /* 0x0017B708 speed = 0 */
    /* Takes v when the record holds FLT_MAX ("unset") or v is larger; v is
     * capped at 999.9.  true when the record changed. */
    bool update(f32 v);                     /* 0x0017B718 */

    f32 speed;                              /* 0x00 */
};
EGCS_SIZE(SpeedRecord, 0x4);

#endif /* GAME_T127_RACE_RECORDS_H */
