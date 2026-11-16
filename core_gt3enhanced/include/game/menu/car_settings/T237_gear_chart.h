/* T237_gear_chart.h - gt2k_menu::GearChart (translation unit T237, unit
 * u069), 0x001C5E50..0x001C6410.
 *
 * The gear chart of the setting screen: for every gear a line from its
 * lowest to its highest (speed, rpm) point; x = speed (0 .. 97.22 m/s = 350
 * km/h across the Item), y = rpm (0 .. the highest rpm of all gears up the
 * Item).  The chart is recomputed every frame from the car's parameter block
 * (CarEquipments::getSpec, then gear_chart_of_spec of T292); a gear whose ratio
 * (gear x final) changed this frame is drawn in another colour.  Built by the
 * page hook create_gear_chart (0x001C62C8; operator new(0x590)). */
#ifndef GAME_T237_GEAR_CHART_H
#define GAME_T237_GEAR_CHART_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T235_gear_chart_data.h"      /* GearChartData (not ColorBox's header: retail's T237
                                               has no SmartIndexT<int> type name) */
#include "game/data/T482_car_equipments.h"       /* CarSpec (GearChart::spec) */

class GarageCar;                            /* T036 (pointers only) */

namespace gt2k_menu {
class ControlBase;                          /* T247 (pointers only) */
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuTexture;                         /* T213 */
struct MenuLayoutItem;                      /* T229 */

/* The car whose gears the chart draws (gear_chart_car, T081). */
typedef GarageCar *GearChartCarFn(ControlBase *owner);

/* vtable 0x003365F8 (8 slots: [3] .. [7] as ColorBox's).  typeinfo
 * __tfQ29gt2k_menu9GearChart 0x001C6378 (si, base Item; __si_type_info
 * 0x004B2620).  Size 0x590 (operator new in create_gear_chart). */
class GearChart : public Item {
public:
    GearChart(GarageCar *car, MenuLayoutItem *res, MenuTexture *tex); /* 0x001C5E50 ratio[] and the
                                               chart of the car's current setting */
    /*  [0] +0x0C 0x001C63C8 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001C5F58 update: spec, chart, ratio[] /
                                               changed[], maxRpm again */
    virtual void virtual_2();               /* [2] +0x1C 0x001C6070 draw: Item's, then the lines */

    GarageCar    *car;                      /* 0x100 */
    CarSpec       spec;                     /* 0x104 the car's parameter block */
    GearChartData chart;                    /* 0x4E8 */
    /* bool (4 bytes in egcs, its own alias set): the constructor's stores
     * of changed[i] do not hold the car's final ratio load inside the loop
     * (loop.c hoists it, retail's order). */
    bool          changed[8];               /* 0x54C ratio[i] changed this frame */
    f32           ratio[8];                 /* 0x56C gearRatio[i] / 1000 * gearRatioFinal / 1000 */
    f32           maxRpm;                   /* 0x58C max of chart.rpmHigh[1 .. count] */
};
EGCS_SIZE(gt2k_menu::GearChart, 0x590);

/* The hook of "gear_chart" (a MenuHookFn, T229; no direct callers): ctx is the
 * screen's controller, p8 a GearChartCarFn; NULL without a car (z is left at
 * Item's).  (u034 / u069 declared it locally before the sweep.) */
Transitive *create_gear_chart(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14);  /* 0x001C62C8 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu9GearChart(void);     /* 0x001C6378 */
}
#endif

#endif /* GAME_T237_GEAR_CHART_H */
