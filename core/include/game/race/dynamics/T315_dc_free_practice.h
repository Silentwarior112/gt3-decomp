/* T315_dc_free_practice.h - DynamicsConductorFreePractice, the conductor of
 * free practice (translation unit T315, unit u084), 0x001F2010..0x001F2748.
 *
 * RaceFreePractice +0x25888 (T136, u046 dcFree).  The running order is the
 * entries' start order at first (slot 1), then the cars by best lap (slot 2,
 * an insertion sort); a car's best lap starts from the entry's saved best
 * (slot 21).  Section and lap times post a RaceDisplayTimeDiffEvent against
 * the race record (RaceOrganization +0x8AC), and a lap under the record the
 * information 2.  There are no laps to finish (slot 29 = 0).
 *
 * T315 also emits out-of-line copies of RaceDisplayTimeDiffEvent's inline
 * functions (T154): getEventType 0x001F26F0 (0xF, vtable copy 0x0033D840,
 * no callers) and the typeinfo function below. */
#ifndef GAME_T315_DC_FREE_PRACTICE_H
#define GAME_T315_DC_FREE_PRACTICE_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2890 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf29DynamicsConductorFreePractice(void); /* 0x001F26A0 */
}
#endif

/* vtable 0x0033D6E8 (42 slots: [1] [2] [8] [9] [10] [11] [15] [16] [17] [21]
 * [23] [24] [28] [29] [34] [39] [41] its own).  typeinfo
 * __tf29DynamicsConductorFreePractice 0x001F26A0 (si, base
 * DynamicsConductor).  vptr +0xE3A8.  Size 0xE3AC: the constructor stores
 * only the vtable and no code reads +0xE3AC..; the 0xC bytes u046 / u084
 * gave it (0xE3B8) are the tail of RaceFreePractice, whose classes are
 * 64-byte aligned (0x25888 + 0xE3AC = 0x33C34, size 0x33C40). */
class DynamicsConductorFreePractice : public DynamicsConductor {
public:
    DynamicsConductorFreePractice();        /* 0x001F2010 (F_00182F40, RaceFreePractice) */

    /* All -1, then every car at its entry's order (rank = order + 1). */
    virtual void virtual_1();               /* [ 1] +0x014 0x001F23E0 */
    /* The order by best lap (an insertion sort, unsigned, stable), then
     * rank = position + 1. */
    virtual void virtual_2();               /* [ 2] +0x01C 0x001F24C0 */
    virtual int  getStartingFormat();       /* [ 8] +0x04C 0x001F2698 3 */
    virtual int  SetTireWearParameters(RaceRuleOptions *o);     /* [ 9] +0x054 0x001F2670 1 */
    virtual int  SetSlowCarBoostParameters(RaceRuleOptions *o); /* [10] +0x05C 0x001F2668 0 */
    virtual void processCollision_();       /* [11] +0x064 0x001F2648 collideAllCars() */
    /* For a car without fault and a race record of the section: the time
     * difference split - record; then the section time events (the lap not
     * decremented). */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001F2150 */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t); /* [16] +0x08C 0x001F2248 car 0: information 3 ("start
                                               the qualifying attack", T154 pollInformationEvents); the lap
                                               starts at ms */
    /* For a car without fault the difference against the race's best lap
     * and, under it, information 2 ("<maker> is on provisional pole", T154
     * pollInformationEvents; also with no record yet); the lap time events,
     * DynCar_clearFaults; the lap starts at ms. */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F22C0 */
    /* The base's; then every car's best lap from its entry's saved best
     * (car_update_best_lap). */
    virtual void virtual_21(int a);         /* [21] +0x0B4 0x001F20B0 */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F2050 getEntrantInfoRace */
    /* Car 0: 6, 0, 0; car n: 5, -(n x 200 + 300), 120.  Sets no result
     * ($v0 is left 6 or the start argument). */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F2070 */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001F2678 lapTimeIfStarted(car) */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F2148 0: no laps */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F2630 */
    virtual void chargePenaltyBySurface(RaceCarState *car, int left, int right); /* [39] +0x144 0x001F2638 empty */
    virtual int  EveryTimePitStop();        /* [41] +0x154 0x001F2640 1 */
};
EGCS_SIZE(DynamicsConductorFreePractice, 0xE3AC);

/* The copy of T154's RaceDisplayTimeDiffEvent typeinfo function (T303 has
 * one too; see T303_race_time_events.h). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf24RaceDisplayTimeDiffEvent(void);     /* 0x001F26F8 (T154 0x00195790, T303 0x001ECB58) */
}
#endif

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti29DynamicsConductorFreePractice[3]; /* 0x004B2890 the __si_type_info (u084 g_004B2890) */
#endif

#endif /* GAME_T315_DC_FREE_PRACTICE_H */
