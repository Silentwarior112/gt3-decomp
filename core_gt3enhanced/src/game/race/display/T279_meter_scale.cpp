/* T279 - a performance figure of a car, and the scales of the speedometer
 * and the tachometer for a car. */
#include "game/car/physics/car_spec_internal.h"
#include "game/race/display/T279_meter_scale.h"

// FUNC 0x001D98D0 ValuateAutomobilePerformance
f32 ValuateAutomobilePerformance(CarEquipments *equip)
{
    CarSpec         spec;
    CarDynamics     dyn;
    EngineSpecSheet sheet;
    CarCondition    fresh;

    SetupCarSpec(&spec, equip, 0, 0, 0, "ValuateAutomobilePerformance()");
    CarDynamics_init(&dyn, &spec, 0, NULL, &g_002D2988, &fresh);   /* 0x001DBFD8 */
    engine_spec_sheet(&dyn.engine, &sheet, &fresh);
    return (dyn.mass * 300.0f) / (sheet.maxPower * sheet.maxPower);
}

// FUNC 0x001D9978 SelectMeterScale
void SelectMeterScale(CarDynamics *dyn, int *unit, int *speedScale, int *redZone,
                      int *tachoScale)
{
    int steps;

    *unit = g_002F59D8.system;
    steps = (int)((dyn->maxSpeed * 1.1f) / 22.222223f) + 1;     /* 22.2 m/s = 80 km/h */
    if (steps < 2)
        steps = 2;
    else if (steps >= 5)
        steps = 4;
    /* A store in each arm (jump2 merges the two mult / sw tails): written
     * once after a ?:, sched1 moves the loads below over the store. */
    if (*unit != UNIT_METRIC)
        *speedScale = steps * 50;                               /* mph */
    else
        *speedScale = steps * 80;                               /* km/h */
    *redZone = dyn->gear.revLimit;
    *tachoScale = ((dyn->engine.redline + 1500) / 1000) * 1000;
}

// FUNC 0x001D9A10 SelectMeterScale
void SelectMeterScale(RaceCarState *car, int *unit, int *speedScale, int *redZone,
                      int *tachoScale)
{
    SelectMeterScale(&car->params, unit, speedScale, redZone, tachoScale);
}

// FUNC 0x001D9A30 SelectMeterScale
void SelectMeterScale(CarEquipments *equip, int *unit, int *speedScale, int *redZone,
                      int *tachoScale)
{
    CarSpec      spec;
    CarDynamics  dyn;
    CarCondition fresh;

    SetupCarSpec(&spec, equip, 0, 0, 0, "SelectMeterScale()");
    CarDynamics_init(&dyn, &spec, 0, NULL, &g_002D2988, &fresh);   /* 0x001DBFD8 */
    SelectMeterScale(&dyn, unit, speedScale, redZone, tachoScale);
}
