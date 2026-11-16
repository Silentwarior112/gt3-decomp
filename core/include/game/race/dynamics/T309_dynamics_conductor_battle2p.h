/* T309_dynamics_conductor_battle2p.h - DynamicsConductorBattle2P, the
 * conductor of the two-player split screen battle (translation unit T309,
 * unit u083), 0x001EFFD0..0x001F03A8.
 *
 * Its options are the race's battle options (RaceOrganization +0x08..+0x14):
 * the lap count, a rule mode, a handicap and the boost preset (T281).  The
 * checkpoint times of the cars are collected (recordGate) and reported as
 * gaps (reportGateTimes); when a car's race ends, the other cars are stopped
 * (st.driver 2). */
#ifndef GAME_T309_DYNAMICS_CONDUCTOR_BATTLE2P_H
#define GAME_T309_DYNAMICS_CONDUCTOR_BATTLE2P_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2830 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf25DynamicsConductorBattle2P(void);    /* 0x001F0358 */
}
#endif

/* vtable 0x0033CD10 (42 slots: [0] [9] [10] [11] [14] [17] [23] [24] [29]
 * [31] [34] [39] its own, the others DynamicsConductor's).  typeinfo
 * __tf25DynamicsConductorBattle2P 0x001F0358 (si, base DynamicsConductor).
 * vptr +0xE3A8.  Size 0xE3AC: no data of its own (RaceBattle2P +0xC8C0,
 * then a RaceInput at +0x1AC6C, T137). */
class DynamicsConductorBattle2P : public DynamicsConductor {
public:
    DynamicsConductorBattle2P();            /* 0x001EFFD0 (F_00183480) */

    virtual void ReportTimeDifference();    /* [ 0] +0x00C 0x001F02C8 reportGateTimes() */
    virtual int  SetTireWearParameters(RaceRuleOptions *o);     /* [ 9] +0x054 0x001F00C0 rules_tire_wear_preset (T281: mode 2 with
                                               the battle rule +0x0C), 0 on course kind 2 */
    virtual int  SetSlowCarBoostParameters(RaceRuleOptions *o); /* [10] +0x05C 0x001F0098 rules_boost_preset(o, RaceOrganization
                                               +0x14) */
    virtual void processCollision_();       /* [11] +0x064 0x001F0308 collideAllCars() */
    virtual void IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split); /* [14] +0x07C 0x001F02E8 recordGate */
    /* As the base without the time limit; the end of one car's race stops
     * every other car still racing (st.driver 2, ai_find_corner). */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F0118 */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F0338 getEntrantInfoRace */
    /* The handicap h (RaceOrganization +0x10): *startType = 1, *startArg -10
     * or min(-|h|, -20) by the car; then startWithLaunchSpeed sets *speedKmh. */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F0010 */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F0088 the battle's lap count */
    virtual int  getSlowCarBoostType_();    /* [31] +0x104 0x001F0328 rules.enemyBoostType */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F0298 (isDirtCourse's answer is dropped) */
    virtual void chargePenaltyBySurface(RaceCarState *car, int left, int right); /* [39] +0x144 0x001F02C0 empty */
};
EGCS_SIZE(DynamicsConductorBattle2P, 0xE3AC);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti25DynamicsConductorBattle2P[3];  /* 0x004B2830 the __si_type_info (u083 g_004B2830) */
#endif

#endif /* GAME_T309_DYNAMICS_CONDUCTOR_BATTLE2P_H */
