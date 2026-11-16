/* T032 - gt2k_menu::CircleGauge.  0x00115F70..0x00116508.
 *
 * A page item drawn as a ring segment from angleZero (230) towards angleFull
 * (30): a shadow ring over the whole range, then the value in colour 1, the
 * part past 80% in colour 2.  The value animates in once `hold' is cleared:
 * up to 1.0 by 0.04 a frame, then down to the target by 0.02.  The target
 * is one statistic of the car's spec record (CircleGaugeKind: the handling /
 * acceleration bytes +0x0C / +0x0D as tenths, 0 shown as 7; the top speed in
 * km/h / 350, at most 1).
 *
 * The outside headers first, its own last: retail's .rodata (0x0030DE30..
 * 0x0030DF88) has the jump table, the vtable, CircleGauge's type name right
 * after it, then only the operator classes of Item's bases.  Its .lit4 has
 * 0.1, 0.1, 0.04, 0.02, 0.8, 0.3, 0.3. */
#include "cxxrt.h"                          /* __rtti_si (GCC 14's typeinfo function) */
#include "game/menu/framework/T213_menu_item.h"            /* Item (base) */
#include "game/menu/framework/T218_color.h"                /* Color4f::toRGBA8 */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (the constructor) */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (car) */
#include "game/menu/framework/T259_menu_drawable.h"        /* Drawable::getScreenX / getScreenY */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar::maxSpeed */
#include "engine/pgl/G001_flare.h"                /* gfx_enable / disable, pgl_push / pop_matrix,
                                               pgl_translate, vu0_scale, ring_draw */
#include "game/menu/framework/T032_circle_gauge.h"         /* own translation unit */
#include "game/menu/arcade/arcade_control_internal.h"

namespace gt2k_menu {

/* The angle span of the colour-2 part: the value past 80%. */
#define CIRCLE_GAUGE_MARK 0.8f
/* The top speed of a full speed gauge, km/h. */
#define CIRCLE_GAUGE_SPEED_FULL 350.0f
/* T278's unit tag of km/h ('kmph'; ShowRoomCar::maxSpeed does not read it). */
#define UNIT_TAG_KMPH 0x6B6D7068

/* Item(name, layout) is the page item `name' (findHiddenItem).  Built by
 * four page functions of T021 / T023 with operator new(0x124); Item's
 * destructor (Item::~Item) if setKind throws. */
// FUNC 0x00115F70 gt2k_menu::CircleGauge::CircleGauge
CircleGauge::CircleGauge(MenuCarSpec *spec, const char *name, PageLayout *layout, int kind)
    : Item(name, layout)
{
    car = spec;
    value = 0.0f;
    target = 0.0f;
    state = 0;
    unk120 = 0;
    angleFull = 30.0f;
    angleZero = 230.0f;
    maxValue = 1.0f;
    hold = 1;
    setKind(kind);
}

/* Starts the animation. */
// FUNC 0x00116040 gt2k_menu::CircleGauge::virtual_6
void CircleGauge::virtual_6()
{
    hold = 0;
}

/* Sets the target from statistic `kind' of the car and restarts (hold,
 * state and value 0).  Other kinds keep the old target (the ratings:
 * CarSpec handlingRating / accelRating). */
// FUNC 0x00116048 gt2k_menu::CircleGauge::setKind
void CircleGauge::setKind(int kind)
{
    int rating;                             /* handling / acceleration: one variable (retail's $a2) */
    switch (kind) {
    case CIRCLE_GAUGE_HANDLING: {
        rating = car->car.spec.handlingRating;
        if (rating <= 0)
            rating = 7;
        target = (f32)rating * 0.1f;
        break;
    }
    case CIRCLE_GAUGE_SPEED: {
        int kmh = car->car.maxSpeed(UNIT_TAG_KMPH, 1);
        int v = (kmh > 0) ? kmh : 7;
        f32 t = (f32)v / CIRCLE_GAUGE_SPEED_FULL;
        target = t;
        if (1.0f < t)
            target = 1.0f;
        break;
    }
    case CIRCLE_GAUGE_ACCELERATION: {
        rating = car->car.spec.accelRating;
        if (rating == 0)
            rating = 7;
        target = (f32)rating * 0.1f;
        break;
    }
    }
    state = 0;
    value = 0.0f;
    hold = 0;
}

/* Animation step after Item's (Item::virtual_1): state 0 -> 1, 1 waits for
 * hold == 0 (value 0), 2 rises to 1.0, 3 falls to the target, 4 done.
 * (Jump table 0x0030DE30, 5 entries.) */
// FUNC 0x00116140 gt2k_menu::CircleGauge::virtual_1
void CircleGauge::virtual_1()
{
    Item::virtual_1();
    switch (state) {
    case 0:
        state = state + 1;
        /* fall through */
    case 1:
        if (hold == 0) {
            value = 0.0f;
            state = state + 1;
        }
        break;
    case 2:
        value = value + 0.04f;
        if (1.0f <= value) {
            value = 1.0f;
            state = state + 1;
        }
        break;
    case 3:
        value = value - 0.02f;
        if (value < target) {
            value = target;
            state = state + 1;
        }
        break;
    case 4:
        break;
    }
}

/* Draws the gauge at the item's screen position (translate) and size
 * (scale), antialiased: the shadow ring (colour 0, 4 larger, 8 degrees wider
 * at both ends, width 0.5), then if value > 0 the value ring (colour 1,
 * width 0.3), split at 80% with colour 2 beyond.  Each ring's inner and
 * outer colour are the same (two conversions). */
// FUNC 0x001161F8 gt2k_menu::CircleGauge::virtual_2
void CircleGauge::virtual_2()
{
    gfx_enable(PGL_CAP_ANTIALIAS);
    f32 zero  = angleZero;
    f32 span  = angleFull - zero;
    f32 shown = span * value + zero;
    f32 full  = span * maxValue + zero;
    f32 mark  = span * CIRCLE_GAUGE_MARK + zero;

    pgl_push_matrix();
    pgl_translate(getScreenX(), getScreenY(), 0.0f);
    vu0_scale(w + 4.0f, h + 4.0f, 1.0f);
    f32 shadowZero = angleZero + 8.0f;
    full = full - 8.0f;
    /* each ring's inner and outer colour: the same conversion, twice */
    ring_draw(color[0].toRGBA8(alphaRatio), color[0].toRGBA8(alphaRatio), 32, full, shadowZero, 0.5f);
    pgl_pop_matrix();

    if (0.0f < value) {
        pgl_push_matrix();
        pgl_translate(getScreenX(), getScreenY(), 0.0f);
        vu0_scale(w, h, 1.0f);
        if (mark < shown) {                 /* below 80% */
            ring_draw(color[1].toRGBA8(alphaRatio), color[1].toRGBA8(alphaRatio), 32, shown, angleZero,
                      0.3f);
        } else {
            ring_draw(color[1].toRGBA8(alphaRatio), color[1].toRGBA8(alphaRatio), 32, mark, angleZero,
                      0.3f);
            ring_draw(color[2].toRGBA8(alphaRatio), color[2].toRGBA8(alphaRatio), 32, shown, mark, 0.3f);
        }
        pgl_pop_matrix();
    }
    gfx_disable(PGL_CAP_ANTIALIAS);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base Item). */
// FUNC 0x00116470 __tfQ29gt2k_menu11CircleGauge
extern "C" void *__tfQ29gt2k_menu11CircleGauge(void)
{
    if (__tiQ29gt2k_menu11CircleGauge[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu11CircleGauge, "Q29gt2k_menu11CircleGauge",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu11CircleGauge;
}
#endif

// FUNC 0x001164C0 gt2k_menu::CircleGauge::~CircleGauge
/* implicit in game/menu/framework/T032_circle_gauge.h; egcs emits this copy: Item::~Item(this,
 * 0), then with flags bit 0 the class's Object::operator delete; no vptr
 * store. */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
