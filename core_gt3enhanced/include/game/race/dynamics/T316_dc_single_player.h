/* T316_dc_single_player.h - DynamicsConductorSinglePlayer, the conductor of
 * the single player races (translation unit T316, unit u084),
 * 0x001F2748..0x001F2A10.
 *
 * RaceArcade +0x261D4 (T134, u046 dcSingle) and others.  Its rules start as
 * the defaults g_002D2988 (T281_battle_rule_presets.h); slot 0 reports the
 * checkpoint times (reportGateTimes) and slot 14 records them (recordGate).
 * For startAction 3 (a start without countdown) every car starts at 80 km/h,
 * car 0 with start type 8 (slot 24), and car 0 gets the car back when its
 * countdown runs out (slot 12). */
#ifndef GAME_T316_DC_SINGLE_PLAYER_H
#define GAME_T316_DC_SINGLE_PLAYER_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B28A0 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf29DynamicsConductorSinglePlayer(void); /* 0x001F29C0 */
}
#endif

/* vtable 0x0033D920 (42 slots: [0] [9] [10] [11] [12] [14] [16] [23] [24]
 * [31] [34] [39] its own).  typeinfo __tf29DynamicsConductorSinglePlayer
 * 0x001F29C0 (si, base DynamicsConductor).  vptr +0xE3A8.  Size 0xE3AC (no
 * data of its own; RaceArcade ends with it). */
class DynamicsConductorSinglePlayer : public DynamicsConductor {
public:
    /* The base, the vtable, then rules = g_002D2988 (0x40 bytes, unaligned
     * 8-byte copies). */
    DynamicsConductorSinglePlayer();        /* 0x001F2748 (F_00182260, F_0018EF88) */

    virtual void ReportTimeDifference();    /* [ 0] +0x00C 0x001F2920 reportGateTimes() */
    virtual int  SetTireWearParameters(RaceRuleOptions *o);     /* [ 9] +0x054 0x001F2998 1 */
    virtual int  SetSlowCarBoostParameters(RaceRuleOptions *o); /* [10] +0x05C 0x001F2990 1 */
    virtual void processCollision_();       /* [11] +0x064 0x001F2960 collideAllCars() */
    /* startAction 3: car 0's countdown; when it runs out the driver gets the
     * car back (car_mode_driver) and event 4. */
    virtual void virtual_12();              /* [12] +0x06C 0x001F2860 */
    virtual void IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split); /* [14] +0x07C 0x001F2940 recordGate (split
                                               not read) */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t); /* [16] +0x08C 0x001F28E8 empty */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F29A0 getEntrantInfoRace */
    /* startAction 3: *startType = 8 (car 0 only), *speedKmh = 0x50; else
     * startWithLaunchSpeed. */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F2810 */
    virtual int  getSlowCarBoostType_();    /* [31] +0x104 0x001F2980 rules.enemyBoostType */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F28F0 0 (isDirtCourse's answer is dropped) */
    virtual void chargePenaltyBySurface(RaceCarState *car, int left, int right); /* [39] +0x144 0x001F2918 empty */
};
/* The enhanced engine: DynamicsConductor's (GT3 0xE3AC). */
EGCS_SIZE(DynamicsConductorSinglePlayer, 0xE58C);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti29DynamicsConductorSinglePlayer[3]; /* 0x004B28A0 the __si_type_info (u084 g_004B28A0) */
#endif

#endif /* GAME_T316_DC_SINGLE_PLAYER_H */
