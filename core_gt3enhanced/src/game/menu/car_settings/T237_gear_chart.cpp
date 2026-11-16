/* T237 - gt2k_menu::GearChart (0x001C5E50..0x001C6410).
 *
 * The gear chart of the setting screen: for every gear a line from its
 * lowest to its highest (speed, rpm) point, x = speed (0 .. 97.22 m/s =
 * 350 km/h across the Item), y = rpm (0 .. the highest rpm of all gears up
 * the Item).  The chart is recomputed every frame from the car's parameter
 * block; a gear whose ratio (gear x final) changed this frame is drawn in
 * another colour.
 *
 * Data (retail .rodata 0x003365F8..0x0033679E): GearChart's vtable, then
 * the type names GearChart, OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, GarageCarList, List<GarageCar>,
 * GarageCar, Cell<GarageCar>, Operator: the original read the garage car
 * list before Item's header.  .lit4 0x00350078 (97.22).
 */
#include "game/save/T429_garage_car_list.h"      /* (first: the garage names) GarageCar (car) */
#include "game/menu/car_settings/T237_gear_chart.h"
#include "engine/pgl/G001_flare.h"                /* gfx_enable / gfx_disable, gfx_blend_func, pgl_color */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable::getPosition */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_rect, menu_clip_full */
#include "game/car/physics/T292_gear_calc.h"            /* gear_chart_of_spec */
#include "game/common/T494_text_utils.h"           /* draw_line_quad */
#include "game/menu/framework/menu_links_internal.h"                           /* (T229: PageLayout::getTexture) */

namespace gt2k_menu {

/* The chart's speed range: 350 km/h in m/s (gp-0x7978). */
static const f32 CHART_MAX_SPEED = 97.22222f;

/* The line colours (RGBA8, R in the low byte, 0x80 = 1.0). */
enum {
    GEAR_COLOR_CHANGED = 0x80004080,        /* the gear's ratio changed this frame */
    GEAR_COLOR_NORMAL  = 0x8080FF80,        /* the other gears */
    GEAR_COLOR_GEAR0   = 0x8080C0FF         /* gear 0 (? reverse); never drawn: the loop starts at 1 */
};

/* ratio[] from the car's current setting, then its spec and chart. */
// FUNC 0x001C5E50 gt2k_menu::GearChart::GearChart
GearChart::GearChart(GarageCar *car_, MenuLayoutItem *res, MenuTexture *tex)
    : Item(res, tex)
{
    /* Item(res, tex); on a throw Item::~Item */
    car = car_;
    /* through the member (retail reads the car through a copy of the
     * stored pointer); the final ratio is a loop invariant (hoisted) */
    for (int gear = 0; gear < 8; gear++) {
        ratio[gear] = ((f32)car->gearRatio[gear] / 1000.0f) * ((f32)car->finalGearRatio / 1000.0f);
        changed[gear] = 0;
    }
    car->getSpec(&spec);
    gear_chart_of_spec(&spec, &chart, 0);   /* cond $a2 = 0 (not read) */
}

/* The spec and chart again, changed[] / ratio[] against the car's setting,
 * maxRpm = the highest rpmHigh of gears 1 .. count. */
// FUNC 0x001C5F58 gt2k_menu::GearChart::virtual_1
void GearChart::virtual_1()
{
    car->getSpec(&spec);
    gear_chart_of_spec(&spec, &chart, 0);   /* cond $a2 = 0, as in the constructor */

    /* one gear variable for both loops; the final ratio is a loop
     * invariant (hoisted, one 1000.0 register as retail) */
    int gear;
    for (gear = 0; gear < 8; gear++) {
        f32 r = ((f32)car->gearRatio[gear] / 1000.0f) * ((f32)car->finalGearRatio / 1000.0f);
        if (ratio[gear] != r) {
            ratio[gear] = r;
            changed[gear] = 1;
        } else {
            changed[gear] = 0;
        }
    }

    maxRpm = 0.0f;
    for (gear = 1; gear <= chart.count; gear++) {
        f32 rpm = (f32)chart.rpmHigh[gear];
        if (maxRpm < rpm)
            maxRpm = rpm;
    }
}

/* Item's draw, then the lines (alpha blend on, blending off, antialiasing on)
 * clipped to the Item; the origin is its bottom left corner. */
// FUNC 0x001C6070 gt2k_menu::GearChart::virtual_2
void GearChart::virtual_2()
{
    Item::virtual_2();

    int gearCount = chart.count;
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
    gfx_disable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_ANTIALIAS);

    f32 originX, originY;
    getPosition(&originX, &originY);
    menu_clip_rect(originX, originY, w, h);
    originX -= w * 0.5f;                    /* the bottom left corner */
    originY += h * 0.5f;

    for (int gear = 1; gear <= gearCount; gear++) {
        f32 rpmScale = h / maxRpm;
        f32 speedScale = w / CHART_MAX_SPEED;
        /* (speed, rpm) to the screen in steps, each in its variable (retail
         * computes every step in the point's own register) */
        f32 x0 = chart.speedLow[gear];
        f32 y0 = (f32)chart.rpmLow[gear];
        f32 x1 = chart.speedHigh[gear];
        f32 y1 = (f32)chart.rpmHigh[gear];
        x0 *= speedScale;
        y0 *= rpmScale;
        x1 *= speedScale;
        y1 *= rpmScale;
        y0 = -y0;                           /* rpm grows upwards */
        y1 = -y1;
        x0 += originX;
        y0 += originY;
        x1 += originX;
        y1 += originY;
        f32 width = 4.0f;
        if (gear == 0)                      /* dead: gear starts at 1 */
            width = 8.0f;

        if (changed[gear] != 0)
            pgl_color(GEAR_COLOR_CHANGED);
        else if (gear != 0)
            pgl_color(GEAR_COLOR_NORMAL);
        else
            pgl_color(GEAR_COLOR_GEAR0); /* dead */

        draw_line_quad(x0, y0, 0.0f, x1, y1, 0.0f, width);
    }

    menu_clip_full();
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_ANTIALIAS);
}

/* The hook of "gear_chart": NULL when getCar(owner) gives no car (z is left
 * at Item's). */
// FUNC 0x001C62C8 gt2k_menu::create_gear_chart
Transitive *create_gear_chart(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *res,
                              int p8, int pC, int p10, int p14)
{
    GearChartCarFn *getCar = (GearChartCarFn *)p8;
    MenuTexture *tex = layout->getTexture(res);
    GarageCar *car = getCar((ControlBase *)ctx);
    if (car == 0)
        return 0;
    /* Object::operator new(0x590); Object::operator delete if the constructor
     * throws */
    return new GearChart(car, res, tex);  /* implicit: Object::operator delete (if the constructor throws) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C6378 __tfQ29gt2k_menu9GearChart
extern "C" void *__tfQ29gt2k_menu9GearChart(void)
{
    if (__tiQ29gt2k_menu9GearChart[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9GearChart, "Q29gt2k_menu9GearChart",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9GearChart;
}
#endif

// FUNC 0x001C63C8 gt2k_menu::GearChart::~GearChart
/* the implicit destructor (game/menu/car_settings/T237_gear_chart.h): Item::~Item(0) without a
 * vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
