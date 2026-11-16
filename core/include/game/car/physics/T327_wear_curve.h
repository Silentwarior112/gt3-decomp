/* T327_wear_curve.h - WearCurve, the tyre wear curve of a race (translation
 * unit T327, unit u076), 0x001DA968..0x001DAB28.
 *
 * DynamicsConductor +0x84 (u077 DcSub84, u081 DcTyreSettings): four (wear,
 * grip down) points built from the RaceRuleOptions tw* bytes, all 0 unless
 * they are enabled and ordered (initial < 0 < good < terminal < max). */
#ifndef GAME_T327_WEAR_CURVE_H
#define GAME_T327_WEAR_CURVE_H

#include "game/game.h"

class RaceRuleOptions;                      /* T326_race_rule_options.h (pointers only) */

/* Not polymorphic.  Size 0x28 (DynamicsConductor +0x84..+0xAC). */
class WearCurve {
public:
    void clear();                           /* 0x001DA968 all 0 */
    /* clear(), then with enable and o->twMaxTireWear the four points
     * (x 10000 for the wear, / 100 for the grip down) if they are ordered.
     * DynamicsConductor::virtual_20 (T317) passes its own argument in $a3;
     * it is not read. */
    void setup(RaceRuleOptions *o, int enable, int unused); /* 0x001DA998 */

    f32 x0;                                 /* 0x00 -twInitialTireWear x 10000 (< 0; u077 DcSub84.f00) */
    f32 y0;                                 /* 0x04 twInitialTireGripDown / 100 (u077 f04) */
    f32 x1;                                 /* 0x08 twGoodTireWear */
    f32 y1;                                 /* 0x0C */
    f32 x2;                                 /* 0x10 twTerminalTireWear */
    f32 y2;                                 /* 0x14 */
    f32 x3;                                 /* 0x18 twMaxTireWear (0: no wear; u077 f18, u081 wearRate) */
    f32 y3;                                 /* 0x1C */
    f32 pitGrip;                            /* 0x20 (100 - twPitTireGripDown) / 100 */
                                            /* a tyre whose grip is below it needs a pit stop
                                               (car_tyre_check, T296; u081 wornLimit) */
    f32 slipCap;                            /* 0x24 g_002D28B0.wearSlipCap / 100 (1.0) */
                                            /* the cap of a wheel's slip in its wear rate
                                               (car_tyre_wear, T296; u081 squealMax) */
};
EGCS_SIZE(WearCurve, 0x28);

#endif /* GAME_T327_WEAR_CURVE_H */
