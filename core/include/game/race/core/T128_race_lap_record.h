/* T128_race_lap_record.h - LapRecord (translation unit T128, unit u043),
 * 0x0017B778..0x0017B8F0.
 *
 * Four section times and a lap total (ms), all RACE_TIME_INVALID until set.
 * Each car keeps its session best (RaceEntryCar +0x11C8) and its current /
 * last lap (+0x11DC); RaceOrganization +0x8AC is the race-wide record.  The
 * class name is a guess (the Lupo 2 symbols name the accessor
 * RaceEntryCar::getLapRecord).
 */
#ifndef GAME_T128_RACE_LAP_RECORD_H
#define GAME_T128_RACE_LAP_RECORD_H

#include "game/game.h"

/* 0x157529FF = 359,999,999 ms: the "no time recorded" value of every record
 * (TimeRecord, LapRecord, RaceCarState::bestLap, the ghost lap time). */
#ifndef RACE_TIME_INVALID
#define RACE_TIME_INVALID 0x157529FF
#endif

/* Not polymorphic.  Size 0x14 (RaceEntryCar: +0x11C8, +0x11DC, then
 * +0x11F0). */
class LapRecord {
public:
    LapRecord();                            /* 0x0017B778 reset() */
    void reset();                           /* 0x0017B7A0 all five words RACE_TIME_INVALID */
    /* Takes t when nothing is recorded yet or t is smaller (unsigned); no
     * clamping.  true when it changed. */
    bool updateTotal(u32 t);                /* 0x0017B7E0 */
    /* Copies o when its total is better or this record is still empty. */
    bool update(const LapRecord &o);        /* 0x0017B810 */
    int  sectionDiff(const LapRecord &o, int i) const;  /* 0x0017B878 diff(section[i], o.section[i]) */
    int  totalDiff(const LapRecord &o) const;           /* 0x0017B8A8 diff(total, o.total) */
    /* a - b, or RACE_TIME_INVALID when either is "no time". */
    static int diff(u32 a, u32 b);          /* 0x0017B8C8 */

    u32 section[4];                         /* 0x00 */
    u32 total;                              /* 0x10 */
};
EGCS_SIZE(LapRecord, 0x14);

#endif /* GAME_T128_RACE_LAP_RECORD_H */
