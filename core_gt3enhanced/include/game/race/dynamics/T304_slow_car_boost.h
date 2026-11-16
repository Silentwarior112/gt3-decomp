/* T304_slow_car_boost.h - SlowCarBoost, the catch-up of the race rules, and
 * the time scale of a car (translation unit T304, unit u082),
 * 0x001ECBA8..0x001ED000.
 *
 * The race rules give a boost to a car behind the reference car (up to
 * 1 + boostGain, ramping in between boostStart and boostEnd metres, at once
 * when a lap down) and a brake to a car ahead of it (down to 1 - slowGain);
 * the factor becomes the car's DynCarState.timeScale (x scale), from which
 * car_time_scale derives the frame time: the boosted car runs its dynamics
 * faster.  DynamicsConductor::virtual_3 (0x001ECEB8, slot 3) is defined here
 * too; T305_dynamics_conductor.h declares it in the class. */
#ifndef GAME_T304_SLOW_CAR_BOOST_H
#define GAME_T304_SLOW_CAR_BOOST_H

#include "game/game.h"

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */
class DynCarState;                          /* T288_car_sound.h */

/* DynamicsConductor +0xAC (DynamicsConductor::getSlowCarBoost; Lupo 2
 * getBoostParameters), from RaceRuleOptions +0x20..+0x2A
 * (dc_setup_slow_car_boost).  Not polymorphic.  Size 0x1C (the conductor's
 * cars follow at +0xC8). */
class SlowCarBoost {
public:
    f32 slowGain;                           /* 0x00 fastEnemyRestrainMax x 0.01 (u083 f00) */
    f32 slowStart;                          /* 0x04 fastEnemyRestrainThreshold1, m */
    f32 slowEnd;                            /* 0x08 fastEnemyRestrainThreshold2 x 10 m */
    f32 boostGain;                          /* 0x0C slowEnemyBoostMax x 0.01 (u083 limit: BattleMP caps the
                                               factor at it + 1) */
    f32 boostStart;                         /* 0x10 slowEnemyBoostThreshold1, m */
    f32 boostEnd;                           /* 0x14 slowEnemyBoostThreshold2 x 10 m */
    f32 scale;                              /* 0x18 enemyDtMasterVolume x 0.01 (0: 1.0); timeScale = factor x it */
};
EGCS_SIZE(SlowCarBoost, 0x1C);

/* From DynamicsConductor::virtual_20.  enable 0: no boost, scale 1. */
void dc_setup_slow_car_boost(DynamicsConductor *dc, int enable);       /* 0x001ECBA8 */
/* st's time scale from the boost alone (1 for a car with flag bit 4 or laps
 * ahead); ref is not read. */
void boost_set_time_scale(SlowCarBoost *b, DynCarState *ref, DynCarState *st, int lapDiff, f32 dist); /* 0x001ECC80 */
/* No callers.  The boost of b against a and of a against b. */
void dc_boost_pair(DynamicsConductor *dc, DynCarState *a, DynCarState *b); /* 0x001ECD10 */
/* st behind the reference by lapDiff laps / dist metres. */
f32  boost_factor(SlowCarBoost *b, DynCarState *st, int lapDiff, f32 dist); /* 0x001ECDA8 */
/* st ahead of the reference by lapDiff laps / dist metres (st is not read). */
f32  slow_factor(SlowCarBoost *b, DynCarState *st, int lapDiff, f32 dist);  /* 0x001ECE40 */
/* dt = timeScale / 60, st.invDt = 60 / timeScale (DynCar_initialize,
 * car_pre_step). */
void car_time_scale(RaceCarState *car);                                /* 0x001ECFC8 */

#endif /* GAME_T304_SLOW_CAR_BOOST_H */
