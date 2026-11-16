/* T126_race_lap_time.h - RaceLapTime (translation unit T126, unit u043),
 * 0x0017B498..0x0017B6B8.
 *
 * The lap times of one car (RaceEntryCar +0x210): the last 1000 laps (older
 * ones are shifted out, `start' counts them), their sum and the number of
 * laps driven.  reset(mode != 0) freezes it (a replay: add() only counts).
 *
 * Inline in the original: getMax, getStart and getCurrent, defined in the
 * class below (egcs emitted their out-of-line copies in T126, 0x0017B660 /
 * 0x0017B668 / 0x0017B670, before the typeinfo function), and probably
 * getTotal (one copy: declared out of line).  The Lupo 2 mangled names
 * (getMax__11RaceLapTime, ...) have no const.  (The Lupo 2 map also put
 * these names on T129's 0x0017CE78 / 0x0017CEB8 / 0x0017CEC8, a code match:
 * those are RaceBase's getBGM / getDynamicsConductor / getOrganization,
 * u043's matching pass.)
 */
#ifndef GAME_T126_RACE_LAP_TIME_H
#define GAME_T126_RACE_LAP_TIME_H

#include "game/game.h"

class type_info;

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The typeinfo function (vtable entry 0): 0x0017B678 (__user_type_info
 * 0x00353388, name 0x003279B0).  Declared for the GCC 14 build only: a
 * declaration egcs sees keeps it from emitting its own. */
extern "C" {
const type_info &__tf11RaceLapTime(void);
}
#endif

/* vtable 0x003279A0 (1 slot).  typeinfo __tf11RaceLapTime 0x0017B678.
 * vptr +0xFB4 (after the data).  Size 0xFB8 (RaceEntryCar +0x210, then
 * +0x11C8). */
class RaceLapTime {
public:
    RaceLapTime();                          /* 0x0017B498 current = 0, max = 0 */
    virtual ~RaceLapTime();                 /* [0] +0x0C 0x0017B4B8 */
    /* mode 0: record (everything cleared); else keep the stored laps and
     * only count new ones.  current is always cleared. */
    void reset(int mode);                   /* 0x0017B4E8 */
    /* Counts the lap; when recording adds it to total and stores it (the
     * oldest of 1000 is dropped). */
    void add(u32 time);                     /* 0x0017B508 */
    /* The time of absolute lap `lap', 0 when it is not (or no longer) stored. */
    u32  get(int lap) const;                /* 0x0017B5B8 */
    /* The smallest stored time of laps 0..last (last < 0 or past the end:
     * all); 0xFFFFFFFF when none. */
    u32  getBest(int last) const;           /* 0x0017B5F8 */
    u32  getTotal() const;                  /* 0x0017B658 */
    int  getMax() { return max; }           /* 0x0017B660 */
    int  getStart() { return start; }       /* 0x0017B668 */
    int  getCurrent() { return current; }   /* 0x0017B670 */

    int mode;                               /* 0x000 0 record, else frozen (replay) (? name) */
    int current;                            /* 0x004 laps added so far (also while frozen) */
    int start;                              /* 0x008 the absolute index of times[0] */
    int max;                                /* 0x00C the number of stored laps (<= 1000) */
    u32 total;                              /* 0x010 the sum of the added laps (the race time) */
    u32 times[1000];                        /* 0x014 */
    /*  vptr                                   0xFB4 */
};
EGCS_SIZE(RaceLapTime, 0xFB8);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti11RaceLapTime[2];        /* 0x00353388 gp-0x4668 (u043: g_00353388) */
#endif

#endif /* GAME_T126_RACE_LAP_TIME_H */
