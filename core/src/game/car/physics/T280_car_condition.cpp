/* T280 - CarCondition: the odometer, the engine and oil mileages (bounded
 * by ConditionModel g_002D2B00) and the dirt of a car. */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T280_car_condition.h"

#define ODOMETER_MAX 0xEE6B27A6u                /* 3999999910 */
#define DIRT_MAX     0.8f

// FUNC 0x001D9AF0 CarCondition::clear
void CarCondition::clear()
{
    memset(this, 0, sizeof(CarCondition));
}

// FUNC 0x001D9B10 CarCondition::CarCondition
CarCondition::CarCondition()
{
    clear();
}

// FUNC 0x001D9B38 CarCondition::setOdometer
void CarCondition::setOdometer(u32 v)
{
    if (v > ODOMETER_MAX)
        odometer = ODOMETER_MAX;
    else
        odometer = v;
}

/* The odometer (2 m units) / 50: tenths of a km, or / 80: tenths of a mile
 * in the imperial setting. */
// FUNC 0x001D9B60 CarCondition::odometerDistance
u32 CarCondition::odometerDistance()
{
    if (g_002F59D8.system != UNIT_IMPERIAL)
        return odometer / 50;
    return odometer / 80;
}

/* d metres: counts d / 2 whole units; returns the rest of d (0 if d <= 0
 * or the odometer reached its maximum). */
// FUNC 0x001D9BB0 CarCondition::addOdometer
f32 CarCondition::addOdometer(f32 d)
{
    f32 rest = 0.0f;

    if (!(d <= 0.0f)) {
        int units = (int)d / 2;

        odometer += units;
        if (odometer > ODOMETER_MAX - 1) {
            odometer = ODOMETER_MAX;
            return rest;
        }
        rest = d - (f32)(units * 2);
    }
    return rest;
}

// FUNC 0x001D9C20 CarCondition::setEngineMileage
void CarCondition::setEngineMileage(f32 v)
{
    if (g_002D2B00.maxEngineMileage() < v)
        engineMileage = g_002D2B00.maxEngineMileage();
    else
        engineMileage = v;
}

// FUNC 0x001D9C80 CarCondition::addEngineMileage
void CarCondition::addEngineMileage(f32 d)
{
    engineMileage += d;
    if (g_002D2B00.maxEngineMileage() < engineMileage)
        engineMileage = g_002D2B00.maxEngineMileage();
}

// FUNC 0x001D9CE8 CarCondition::resetEngineMileage
void CarCondition::resetEngineMileage()
{
    engineMileage = g_002D2B00.newEngineMileage();
}

// FUNC 0x001D9D18 CarCondition::setOilMileage
void CarCondition::setOilMileage(f32 v)
{
    if (g_002D2B00.maxOilMileage() < v)
        oilMileage = g_002D2B00.maxOilMileage();
    else
        oilMileage = v;
}

/* New oil runs from newOilMileage() (= -maxOilMileage()) up to 0; crossing 0
 * puts it at -newOilMileage() (= maxOilMileage()) at once: the oil is used
 * up. */
// FUNC 0x001D9D78 CarCondition::addOilMileage
void CarCondition::addOilMileage(f32 d)
{
    if (oilMileage < 0.0f) {
        oilMileage = oilMileage + d;
        if (0.0f <= oilMileage)
            oilMileage = -g_002D2B00.newOilMileage();
    } else {
        oilMileage = oilMileage + d;
    }
    if (g_002D2B00.maxOilMileage() < oilMileage)
        oilMileage = g_002D2B00.maxOilMileage();
}

// FUNC 0x001D9E20 CarCondition::oilNeedsChange
int CarCondition::oilNeedsChange()
{
    return g_002D2B00.oilChangeMileage() <= oilMileage;
}

/* (oil - newOilMileage() while negative) / maxOilMileage(), at most 1; 1
 * when maxOilMileage() is 0. */
// FUNC 0x001D9E68 CarCondition::oilDirtiness
f32 CarCondition::oilDirtiness()
{
    f32 life = g_002D2B00.maxOilMileage();
    f32 used, ratio;

    if (life == 0.0f)
        return 1.0f;
    used = oilMileage;
    if (used < 0.0f)
        used -= g_002D2B00.newOilMileage();
    ratio = used / life;
    if (1.0f < ratio)
        ratio = 1.0f;
    return ratio;
}

// FUNC 0x001D9F18 CarCondition::setDirt
void CarCondition::setDirt(f32 v)
{
    if (DIRT_MAX < v)
        dirt = DIRT_MAX;
    else
        dirt = v;
}

// FUNC 0x001D9F38 CarCondition::addDirt
void CarCondition::addDirt(f32 d)
{
    dirt += d;
    if (DIRT_MAX < dirt)
        dirt = DIRT_MAX;
}

/* The engine of a new car (GameStatus::buyCar 0x00219250, through
 * CarInformation::resetEngine 0x00238280): a second name for
 * resetEngineMileage. */
// FUNC 0x001D9F60 CarCondition::resetEngine
void CarCondition::resetEngine()
{
    resetEngineMileage();
}

// FUNC 0x001D9F80 CarCondition::changeOil
void CarCondition::changeOil()
{
    setOilMileage(g_002D2B00.newOilMileage());
}
