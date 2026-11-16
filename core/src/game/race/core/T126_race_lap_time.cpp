/* T126_race_lap_time.cpp - RaceLapTime (0x0017B498..0x0017B6B8): the lap times
 * of one car (RaceEntryCar::lapTime, +0x210; size 0xFB8).  The last 1000
 * laps are kept; older ones are shifted out (`start' counts them).
 *
 * T126 holds the constructor, the destructor, the methods and the typeinfo
 * function; after getTotal come egcs's out-of-line copies of three inline
 * accessors.
 */
/* Retail T126's .rodata is its vtable and "11RaceLapTime" only
 * (0x003279A0..0x003279BF): its own header alone. */
#include "game/race/core/T126_race_lap_time.h"
#include "runtime/newlib/G007_stdio.h"                /* memmove */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_user */
#endif

/* The number of laps the array keeps. */
#define LAP_TIME_MAX 1000

// FUNC 0x0017B498 RaceLapTime::RaceLapTime
//   current = 0 (before the vptr store), then max = 0.  start, total and mode
//   are left alone (reset() sets them).  Caller: RaceEntryCar::RaceEntryCar.
RaceLapTime::RaceLapTime()
    : current(0)
{
    max = 0;
}

// FUNC 0x0017B4B8 RaceLapTime::~RaceLapTime
//   Slot 0 of the vtable; flags & 1 frees the object with operator delete.
RaceLapTime::~RaceLapTime()
{
    /* implicit: operator delete (flags & 1) */
}

// FUNC 0x0017B4E8 RaceLapTime::reset
//   mode 0: record laps (everything cleared); any other mode keeps the stored
//   laps and makes add() only count new ones (a replay).  `current' is always
//   cleared.  Callers: RaceEntryCar::resetLapTime (0), RaceOrganization::
//   setupGhost (1), RaceBase::virtual_8 (replay != 0).
void RaceLapTime::reset(int newMode)
{
    current = 0;
    mode = newMode;
    if (newMode == 0) {
        start = 0;
        max = 0;
        total = 0;
    }
}

// FUNC 0x0017B508 RaceLapTime::add
//   Counts the lap and, when recording, adds it to the total and stores it;
//   with 1000 laps stored the oldest one is dropped (memmove of the other
//   999, start + 1).  Callers: RaceEntryCar::resetLapTime, dc_post_lap_time.
void RaceLapTime::add(u32 time)
{
    current++;
    if (mode == 0) {
        total += time;
        int stored = max;
        if (stored == LAP_TIME_MAX) {
            start++;
            memmove(&times[0], &times[1], (LAP_TIME_MAX - 1) * sizeof(u32));
            max = max - 1;
            stored = max;
        }
        times[stored] = time;
        max = stored + 1;
    }
}

// FUNC 0x0017B5B8 RaceLapTime::get
//   The time of absolute lap `lap', 0 when it is not (or no longer) stored.
//   Unreferenced.
u32 RaceLapTime::get(int lap) const
{
    if (lap >= start && lap < start + max)
        return times[lap - start];
    return 0;
}

// FUNC 0x0017B5F8 RaceLapTime::getBest
//   The smallest stored time (unsigned) among the stored laps 0..last; last
//   < 0 or past the end means all of them.  0xFFFFFFFF when nothing is
//   stored.  Callers: ResultFreeRun::virtual_10, RaceBase::virtual_7 (-1).
u32 RaceLapTime::getBest(int last) const
{
    u32 best = 0xFFFFFFFF;
    if (last < 0 || last >= max)
        last = max - 1;
    for (int i = 0; i <= last; i++) {
        if (times[i] < best)
            best = times[i];
    }
    return best;
}

// FUNC 0x0017B658 RaceLapTime::getTotal
u32 RaceLapTime::getTotal() const
{
    return total;
}

// FUNC 0x0017B660 RaceLapTime::getMax
/* inline in game/race/core/T126_race_lap_time.h; egcs emits this copy (the Lupo 2
 * map's T129 copy, 0x0017CE78, is RaceBase::getBGM) */

// FUNC 0x0017B668 RaceLapTime::getStart
/* inline in game/race/core/T126_race_lap_time.h; egcs emits this copy (0x0017CEB8
 * is RaceBase::getDynamicsConductor) */

// FUNC 0x0017B670 RaceLapTime::getCurrent
/* inline in game/race/core/T126_race_lap_time.h; egcs emits this copy (0x0017CEC8
 * is RaceBase::getOrganization) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0017B678 __tf11RaceLapTime
//   Typeinfo function (vtable entry 0 of RaceLapTime): builds the
//   __user_type_info __ti11RaceLapTime (0x00353388) on the first call (name
//   "11RaceLapTime" at 0x003279B0) and returns it.
extern "C" const type_info &__tf11RaceLapTime(void)
{
    if (__ti11RaceLapTime[0] == 0)
        __rtti_user(__ti11RaceLapTime, "11RaceLapTime");
    return *(const type_info *)__ti11RaceLapTime;
}
#endif
