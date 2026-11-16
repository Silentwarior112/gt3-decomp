/* T304 - the slow car boost (catch-up) and the car's time scale:
 * 0x001ECBA8..0x001ED000.
 *
 * The race rules give a boost to a car behind the reference car (up to
 * 1 + boostGain, ramping in between boostStart and boostEnd metres, at once
 * when a lap down) and a brake to a car ahead of it (down to 1 - slowGain);
 * the factor becomes the car's time scale (x the rules' scale), from which
 * car_time_scale derives the frame time: the boosted car simply runs its
 * dynamics faster.
 *
 * Retail T304's .rodata (0x0033C9E8, nothing references it: DATA_START):
 * "bad_alloc", t9UnitArena1Ui64. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_motion_internal.h"
#include "game/race/dynamics/T304_slow_car_boost.h"
// DATA_START 0x0033C9E8

/* From DynamicsConductor::virtual_20 (its SetSlowCarBoostParameters
 * result).  enable 0: no boost, scale 1. */
// FUNC 0x001ECBA8 dc_setup_slow_car_boost
void dc_setup_slow_car_boost(DynamicsConductor *dc, int enable)
{
    if (enable != 0) {
        RaceRuleOptions *rules = &dc->rules;
        dc->boost.slowStart = (f32)rules->fastEnemyRestrainThreshold1;
        dc->boost.slowEnd = (f32)rules->fastEnemyRestrainThreshold2 * 10.0f;
        dc->boost.slowGain = (f32)rules->fastEnemyRestrainMax * 0.01f;
        dc->boost.boostStart = (f32)rules->slowEnemyBoostThreshold1;
        dc->boost.boostEnd = (f32)rules->slowEnemyBoostThreshold2 * 10.0f;
        dc->boost.boostGain = (f32)rules->slowEnemyBoostMax * 0.01f;
        int volume = rules->enemyDtMasterVolume;
        if (volume == 0)
            volume = 100;
        dc->boost.scale = (f32)volume * 0.01f;
        return;
    }
    dc->boost.slowStart = 0.0f;
    dc->boost.slowEnd = 0.0f;
    dc->boost.slowGain = 0.0f;
    dc->boost.boostStart = 0.0f;
    dc->boost.boostEnd = 0.0f;
    dc->boost.boostGain = 0.0f;
    dc->boost.scale = 1.0f;
}

/* st's time scale from the boost alone (1 when st has
 * DYN_FLAG_COUNTED_GROUND or is laps ahead); ref is not read. */
// FUNC 0x001ECC80 boost_set_time_scale
void boost_set_time_scale(SlowCarBoost *b, DynCarState *ref, DynCarState *st, int lapDiff, f32 dist)
{
    f32 factor;

    if ((st->flags & DYN_FLAG_COUNTED_GROUND) || lapDiff < 0) {
        factor = 1.0f;
    } else if (lapDiff > 0 || b->boostEnd <= dist) {
        factor = b->boostGain + 1.0f;
    } else if (b->boostStart < dist) {
        factor = b->boostGain * ((dist - b->boostStart) / (b->boostEnd - b->boostStart)) + 1.0f;
    } else {
        factor = 1.0f;
    }
    st->timeScale = factor;
}

/* No callers.  The boost of b against a and of a against b. */
// FUNC 0x001ECD10 dc_boost_pair
void dc_boost_pair(DynamicsConductor *dc, DynCarState *a, DynCarState *b)
{
    int lapDiff;
    f32 dist;

    if (dc->boost.boostGain == 0.0f)
        return;
    dc->getGap(a, b, &lapDiff, &dist);
    boost_set_time_scale(&dc->boost, a, b, lapDiff, dist);
    boost_set_time_scale(&dc->boost, b, a, -lapDiff, -dist);
}

/* st behind the reference by lapDiff laps / dist metres. */
// FUNC 0x001ECDA8 boost_factor
f32 boost_factor(SlowCarBoost *b, DynCarState *st, int lapDiff, f32 dist)
{
    f32 factor;

    if (st->flags & DYN_FLAG_COUNTED_GROUND)
        factor = 1.0f;
    else if (lapDiff > 0 || b->boostEnd <= dist)
        factor = b->boostGain + 1.0f;
    else if (b->boostStart < dist)
        factor = b->boostGain * ((dist - b->boostStart) / (b->boostEnd - b->boostStart)) + 1.0f;
    else
        factor = 1.0f;
    return factor;
}

/* st ahead of the reference by lapDiff laps / dist metres; st is not read. */
// FUNC 0x001ECE40 slow_factor
f32 slow_factor(SlowCarBoost *b, DynCarState *st, int lapDiff, f32 dist)
{
    f32 factor;

    if (lapDiff > 0 || b->slowEnd <= dist)
        factor = 1.0f - b->slowGain;
    else if (b->slowStart < dist)
        factor = 1.0f - b->slowGain * ((dist - b->slowStart) / (b->slowEnd - b->slowStart));
    else
        factor = 1.0f;
    return factor;
}

/* Slot 3 of every conductor (DynamicsConductorBattleMP::virtual_3 calls it;
 * DynamicsConductor::update picks the pairs by slot 31).  1 for a finished
 * car; else the boost when ref is ahead of st (laps, then distance) and the
 * boost is on, the brake when st is ahead (or level) and the brake is on.
 * Returns factor x scale; st's time scale = that x boost.scale. */
// FUNC 0x001ECEB8 DynamicsConductor::virtual_3
f32 DynamicsConductor::virtual_3(DynCarState *ref, DynCarState *st, f32 scale)
{
    f32 factor = 1.0f;

    if (st->finished == 0) {
        int lapDiff;
        f32 dist;
        getGap(ref, st, &lapDiff, &dist);
        int behind;                             /* st behind ref */
        if (lapDiff > 0)
            behind = 1;
        else if (lapDiff < 0)
            behind = 0;
        else if (0.0f < dist)
            behind = 1;
        else
            behind = 0;
        if (behind != 0) {
            if (boost.boostGain != 0.0f)
                factor = boost_factor(&boost, st, lapDiff, dist);
        } else {
            if (boost.slowGain != 0.0f)
                factor = slow_factor(&boost, st, -lapDiff, -dist);
        }
    }
    factor = factor * scale;
    st->timeScale = factor * boost.scale;
    return factor;
}

/* From DynCar_initialize (T330) and car_pre_step: dt = timeScale / 60,
 * invDt = 60 / timeScale. */
// FUNC 0x001ECFC8 car_time_scale
void car_time_scale(RaceCarState *car)
{
    DynCarState *st = &car->st;
    car->dt = st->timeScale * 0.016666668f;     /* x 1/60 */
    st->invDt = FPS / st->timeScale;
}
