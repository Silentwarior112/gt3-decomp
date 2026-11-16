/* T327 - WearCurve (DynamicsConductor +0x84): four (wear, grip down) points
 * built from the RaceRuleOptions tw* bytes, all 0 unless they are enabled
 * and ordered (initial < 0 < good < terminal < max). */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T327_wear_curve.h"

#define WEAR_UNIT 10000.0f                      /* the tw*TireWear bytes x 10000 */

// FUNC 0x001DA968 WearCurve::clear
void WearCurve::clear()
{
    /* setup's order (sched1 puts the block's last store first) */
    x3 = 0.0f;
    y3 = 0.0f;
    pitGrip = 0.0f;
    x0 = 0.0f;
    y0 = 0.0f;
    x1 = 0.0f;
    y1 = 0.0f;
    x2 = 0.0f;
    y2 = 0.0f;
    slipCap = 0.0f;
}

/* enable: the conductor's SetTireWearParameters (slot 9), not on course
 * kind 2 (DynamicsConductor::virtual_20). */
// FUNC 0x001DA998 WearCurve::setup
void WearCurve::setup(RaceRuleOptions *o, int enable, int unused)
{
    clear();
    if (enable == 0)
        return;
    if (o->twMaxTireWear == 0)
        return;
    if (!(o->twTerminalTireWear < o->twMaxTireWear))
        return;
    if (!(o->twTerminalTireGripDown < o->twMaxTireGripDown))
        return;
    if (!(o->twGoodTireWear < o->twTerminalTireWear))
        return;
    if (!(o->twGoodTireGripDown < o->twTerminalTireGripDown))
        return;
    if (!(o->twPitTireGripDown < o->twMaxTireGripDown))
        return;
    x3 = (f32)o->twMaxTireWear * WEAR_UNIT;
    y3 = (f32)o->twMaxTireGripDown * 0.01f;
    pitGrip = (f32)(100 - o->twPitTireGripDown) * 0.01f;
    x0 = -(f32)o->twInitialTireWear * WEAR_UNIT;
    y0 = (f32)o->twInitialTireGripDown * 0.01f;
    x1 = (f32)o->twGoodTireWear * WEAR_UNIT;
    y1 = (f32)o->twGoodTireGripDown * 0.01f;
    x2 = (f32)o->twTerminalTireWear * WEAR_UNIT;
    y2 = (f32)o->twTerminalTireGripDown * 0.01f;
    slipCap = (f32)g_002D28B0.wearSlipCap / 100.0f;
}
