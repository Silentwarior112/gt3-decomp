/* T283 - ConditionModel g_002D2B00: the engine and oil mileage thresholds,
 * built once from g_002D28B0.cond[] (every getter builds it on first use),
 * and the power factor of a car's condition:
 *   engine: new at engineNew (power x (1 - runInLoss)), run in at 0,
 *           worn from wearStart to engineMax (down to 1 - wearLoss);
 *   oil:    new at -oilLife, + oilBonus up to oilBonusEnd, then fading to
 *           + 0 at 0; needs a change from oilChangeAt. */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T283_condition_model.h"

#define MILEAGE_UNIT 10000                      /* cond[] x 10000 */

// FUNC 0x001DAB28 ConditionModel::ConditionModel
ConditionModel::ConditionModel()
{
    init();
}

// FUNC 0x001DAB50 ConditionModel::init
void ConditionModel::init()
{
    DynParams *p = &g_002D28B0;

    if (p->conditionReady != 0)
        return;
    runInLoss = (f32)p->cond[1] * 0.001f;
    engineNew = -(f32)(p->cond[0] * MILEAGE_UNIT);
    wearStart = (f32)(p->cond[2] * MILEAGE_UNIT);
    engineMax = (f32)(p->cond[3] * MILEAGE_UNIT);   /* stored twice (the cond[4] */
    wearLoss = (f32)p->cond[4] * 0.001f;            /* byte load in between keeps */
    engineMax += wearStart;                         /* the first store alive) */
    oilBonus = (f32)p->cond[6] * 0.001f;
    oilBonusEnd = -(f32)(p->cond[7] * MILEAGE_UNIT);
    oilLife = (f32)((p->cond[5] + p->cond[7]) * MILEAGE_UNIT);
    oilChangeAt = (f32)(p->cond[8] * MILEAGE_UNIT);
    p->conditionReady = 1;
}

// FUNC 0x001DAC48 ConditionModel::newEngineMileage
f32 ConditionModel::newEngineMileage()
{
    init();
    return engineNew;
}

// FUNC 0x001DAC70 ConditionModel::maxEngineMileage
f32 ConditionModel::maxEngineMileage()
{
    init();
    return engineMax;
}

// FUNC 0x001DAC98 ConditionModel::newOilMileage
f32 ConditionModel::newOilMileage()
{
    init();
    return -oilLife;
}

// FUNC 0x001DACC8 ConditionModel::maxOilMileage
f32 ConditionModel::maxOilMileage()
{
    init();
    return oilLife;
}

// FUNC 0x001DACF0 ConditionModel::oilChangeMileage
f32 ConditionModel::oilChangeMileage()
{
    init();
    return oilChangeAt;
}

/* The distance the car covered this frame (its speed x the frame time). */
// FUNC 0x001DAD18 ConditionModel::distance
f32 ConditionModel::distance(RaceCarState *car)
{
    return car->speed * car->dt;
}

// FUNC 0x001DAD28 ConditionModel::powerFactor
f32 ConditionModel::powerFactor(CarCondition *c)
{
    f32 engine, oil;
    f32 engineFactor, oilFactor;

    init();
    engine = c->engineMileage;
    engineFactor = 1.0f;
    if (engine < engineNew)
        engineFactor = 1.0f - runInLoss;
    else if (engine < 0.0f)                     /* running in */
        engineFactor = 1.0f - (engine * runInLoss) / engineNew;
    else if (engine <= wearStart)               /* run in */
        ;
    else if (engine < engineMax)                /* wearing */
        engineFactor = 1.0f - ((engine - wearStart) * wearLoss) / (engineMax - wearStart);
    else
        engineFactor = 1.0f - wearLoss;

    oil = c->oilMileage;
    oilFactor = 1.0f;
    if (oil < oilBonusEnd)
        oilFactor = oilBonus + 1.0f;
    else if (oil < 0.0f)                        /* the bonus fading */
        oilFactor = (oil * oilBonus) / oilBonusEnd + 1.0f;
    return engineFactor * oilFactor;
}

/* The model, default-constructed (.data 0x002D2B00, 0x24 bytes right before
 * the unit's frame block: egcs puts a public object that has a constructor
 * in .data, zero-filled). */
ConditionModel g_002D2B00;                  /* 0x002D2B00 */

/* Compiler-generated: egcs's static constructor of g_002D2B00 (.ctors entry
 * 0x0028F164), named after the file's first global function and emitted at
 * the end of the unit: ConditionModel() on the object. */
// FUNC 0x001DAE50 T283_static_init (_GLOBAL_$I$__14ConditionModel)
/* implicit: ConditionModel (the static constructor of g_002D2B00) */
