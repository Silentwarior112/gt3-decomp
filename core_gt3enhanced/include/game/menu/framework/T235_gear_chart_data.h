/* T235_gear_chart_data.h - GearChartData, the gear chart of the setting
 * screen (a plain record; split from T235_color_box.h, which includes it
 * where it was).
 *
 * Its own header so that GearChart (T237_gear_chart.h) and the gear
 * calculation (T292) need not see ColorBox's header: that one reads
 * T251_smart_index.h, which writes SmartIndexT<int>'s type name into every
 * includer, and retail's T237 has no such name (u069's data pass). */
#ifndef GAME_T235_GEAR_CHART_DATA_H
#define GAME_T235_GEAR_CHART_DATA_H

#include "game/game.h"

/* The gear chart gear_chart_of_spec (T292) computes from a CarSpec: for gear i
 * (1 .. count) a line from (speedLow[i], rpmLow[i]) to (speedHigh[i],
 * rpmHigh[i]).  Speeds in m/s (GearChart is 97.22 m/s = 350 km/h wide).
 * Size 0x64 (GearChart+0x4E8 .. +0x54C). */
struct GearChartData {
    int count;                              /* 0x00 number of gears */
    u16 rpmLow[8];                          /* 0x04 */
    u16 rpmHigh[8];                         /* 0x14 */
    f32 speedLow[8];                        /* 0x24 m/s */
    f32 speedHigh[8];                       /* 0x44 m/s */
};
EGCS_SIZE(GearChartData, 0x64);

#endif /* GAME_T235_GEAR_CHART_DATA_H */
