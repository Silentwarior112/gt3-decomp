/* T127_race_records.cpp - two small record keepers (0x0017B6B8..0x0017B778):
 * the best time (smaller is better, RACE_TIME_INVALID = nothing yet) and the
 * top speed (larger is better, capped at 999.9).  The class names are
 * guesses: the Lupo 2 symbols only name the accessors that return them
 * (RaceEntryCar::getTimeRecord, RaceOrganization::getSpeedRecord).
 */
/* Retail T127 has no .rodata (no type names): its own header and T128's
 * RACE_TIME_INVALID only. */
#include "game/race/core/T127_race_records.h"
#include "game/race/core/T128_race_lap_record.h"     /* RACE_TIME_INVALID */

/* SpeedRecord: FLT_MAX (the .sdata word 0x00351DEC) counts as "unset"; the
 * cap (gp-0x7D1C, 0x4479F99A). */
#define SPEED_UNSET  3.4028235e38f
#define SPEED_MAX    999.9f

// FUNC 0x0017B6B8 TimeRecord::TimeRecord
//   time = RACE_TIME_INVALID (0x157529FF).  Caller: RaceEntryCar::RaceEntryCar.
TimeRecord::TimeRecord()
{
    time = RACE_TIME_INVALID;
}

// FUNC 0x0017B6D0 TimeRecord::update
//   Takes t when nothing is recorded yet or t is smaller (unsigned), clamped
//   to RACE_TIME_INVALID; returns whether the record changed.  Caller:
//   dc_post_lap_time.  (t is clamped in place: movn on t.)
bool TimeRecord::update(u32 t)
{
    if (time == RACE_TIME_INVALID || t < time) {
        if (t > RACE_TIME_INVALID)
            t = RACE_TIME_INVALID;
        time = t;
        return true;
    }
    return false;
}

// FUNC 0x0017B708 SpeedRecord::SpeedRecord
//   speed = 0 (a plain `sw $zero').  Callers: RaceOrganization::
//   RaceOrganization, RaceDisplay::RaceDisplay.
SpeedRecord::SpeedRecord()
{
    speed = 0;
}

// FUNC 0x0017B718 SpeedRecord::update
//   Takes v when the record still holds FLT_MAX ("unset") or v is larger; v
//   is capped at 999.9.  Returns whether the record changed.  Caller:
//   RaceDisplay::virtual_2.  Retail computes `speed != FLT_MAX' as a value
//   (bc1f over li 1 / li 0) and branches on it: a flag, not the || alone.
bool SpeedRecord::update(f32 v)
{
    bool set = (speed != SPEED_UNSET);
    if (!set || speed < v) {
        if (v > SPEED_MAX)
            v = SPEED_MAX;
        speed = v;
        return true;
    }
    return false;
}
