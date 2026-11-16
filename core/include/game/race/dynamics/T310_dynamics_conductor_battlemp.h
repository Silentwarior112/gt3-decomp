/* T310_dynamics_conductor_battlemp.h - DynamicsConductorBattleMP, the
 * conductor of the multi-player battle (translation unit T310, unit u083),
 * 0x001F03A8..0x001F05F8.
 *
 * The Battle2P options and checkpoint gaps, the base lap end (slot 17) and a
 * cap on the slow car boost (slot 3). */
#ifndef GAME_T310_DYNAMICS_CONDUCTOR_BATTLEMP_H
#define GAME_T310_DYNAMICS_CONDUCTOR_BATTLEMP_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2840 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf25DynamicsConductorBattleMP(void);    /* 0x001F05A8 */
}
#endif

/* vtable 0x0033CEB0 (42 slots: [0] [3] [9] [10] [11] [14] [23] [29] [31]
 * [34] [39] its own).  typeinfo __tf25DynamicsConductorBattleMP 0x001F05A8
 * (si, base DynamicsConductor).  vptr +0xE3A8.  Size 0xE3AC: no data of its
 * own (the constructor stores only the vtable). */
class DynamicsConductorBattleMP : public DynamicsConductor {
public:
    DynamicsConductorBattleMP();            /* 0x001F03A8 (F_00184E00) */

    virtual void ReportTimeDifference();    /* [ 0] +0x00C 0x001F0518 reportGateTimes() */
    /* The base factor, at most boost.boostGain + 1; when capped,
     * st.timeScale becomes cap x boost.scale. */
    virtual f32  virtual_3(DynCarState *ref, DynCarState *st, f32 scale); /* [ 3] +0x024 0x001F0478 */
    virtual int  SetTireWearParameters(RaceRuleOptions *o);     /* [ 9] +0x054 0x001F0420 as Battle2P's */
    virtual int  SetSlowCarBoostParameters(RaceRuleOptions *o); /* [10] +0x05C 0x001F03F8 as Battle2P's */
    virtual void processCollision_();       /* [11] +0x064 0x001F0558 collideAllCars() */
    virtual void IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split); /* [14] +0x07C 0x001F0538 recordGate */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F0588 getEntrantInfoRace */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F03E8 the battle's lap count */
    virtual int  getSlowCarBoostType_();    /* [31] +0x104 0x001F0578 rules.enemyBoostType */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F04E8 (isDirtCourse's answer is dropped) */
    virtual void chargePenaltyBySurface(RaceCarState *car, int left, int right); /* [39] +0x144 0x001F0510 empty */
};
EGCS_SIZE(DynamicsConductorBattleMP, 0xE3AC);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti25DynamicsConductorBattleMP[3];  /* 0x004B2840 the __si_type_info (u083 g_004B2840) */
#endif

#endif /* GAME_T310_DYNAMICS_CONDUCTOR_BATTLEMP_H */
