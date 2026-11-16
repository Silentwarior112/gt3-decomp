/* T128_race_lap_record.cpp - LapRecord (0x0017B778..0x0017B8F0): four section
 * times and a lap total (0x14 bytes).  All five words start as
 * RACE_TIME_INVALID.  Each car keeps its session best (RaceEntryCar::
 * sessionBest, +0x11C8) and its current / last lap (lapRecord, +0x11DC);
 * RaceOrganization::lapRecord (+0x8AC) is the race-wide record.  The class
 * name is a guess (the Lupo 2 symbols name RaceEntryCar::getLapRecord).
 */
/* Retail T128 has no .rodata (no type names): its own header only. */
#include "game/race/core/T128_race_lap_record.h"

/* The number of sections of a lap. */
#define LAP_SECTIONS 4

// FUNC 0x0017B778 LapRecord::LapRecord
//   reset().  Callers: RaceEntryCar::RaceEntryCar, RaceOrganization::
//   RaceOrganization.
LapRecord::LapRecord()
{
    reset();
}

// FUNC 0x0017B7A0 LapRecord::reset
//   The four sections (stored from the last to the first) and the total
//   RACE_TIME_INVALID.
void LapRecord::reset()
{
    for (int i = LAP_SECTIONS - 1; i >= 0; i--)
        section[i] = RACE_TIME_INVALID;
    total = RACE_TIME_INVALID;
}

// FUNC 0x0017B7E0 LapRecord::updateTotal
//   Takes t as the total when nothing is recorded yet or t is smaller
//   (unsigned); no clamping.  Returns whether it changed.  Caller:
//   RaceOrganization::updateLapRecordTotal.
bool LapRecord::updateTotal(u32 t)
{
    if (total == RACE_TIME_INVALID || t < total) {
        total = t;
        return true;
    }
    return false;
}

// FUNC 0x0017B810 LapRecord::update
//   Copies the four sections and the total of o when o's total is better
//   (smaller) or this record is still empty.  Returns whether it copied.
//   Callers: RaceEntryCar::recordLapTime, RaceOrganization::recordLapTime.
bool LapRecord::update(const LapRecord &o)
{
    if (total == RACE_TIME_INVALID || o.total < total) {
        for (int i = 0; i < LAP_SECTIONS; i++)
            section[i] = o.section[i];
        total = o.total;
        return true;
    }
    return false;
}

// FUNC 0x0017B878 LapRecord::sectionDiff
//   diff(section[i], o.section[i]).  Callers: RaceEntryCar::
//   recordSectionTime, RaceOrganization::recordSectionTime.
int LapRecord::sectionDiff(const LapRecord &o, int i) const
{
    return diff(section[i], o.section[i]);
}

// FUNC 0x0017B8A8 LapRecord::totalDiff
//   diff(total, o.total).  Callers: RaceEntryCar::recordLapTime,
//   RaceOrganization::recordLapTime.
int LapRecord::totalDiff(const LapRecord &o) const
{
    return diff(total, o.total);
}

// FUNC 0x0017B8C8 LapRecord::diff
//   a - b, or RACE_TIME_INVALID when either side is "no time".
int LapRecord::diff(u32 a, u32 b)
{
    if (a == RACE_TIME_INVALID || b == RACE_TIME_INVALID)
        return RACE_TIME_INVALID;
    return a - b;
}
