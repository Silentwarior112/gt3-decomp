/* T040 - gt2k_menu::OilChangeCylinder, the oil change animation of the
 * GT-mode shop.  0x00116E00..0x00117320.
 *
 * After start(): wait 60 frames, play sound 6 and drain the oil (level 1 ->
 * 0 by 0.01 a frame), wait 60 frames, make it clean (dirt = 0), play sound 6
 * and refill it (0 -> 1), then PageManager::setFreeze(0) and back to idle.
 * The oil is the picture `oil' ("dirty"), drawn with its bottom edge fixed
 * and level times the full height, its vertex colours mixed between the
 * pictures `clean' and `dirty' by dirt. */
#include "game/menu/framework/T211_page_manager.h"         /* PageManager::setFreeze */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_OIL */
/* Own header last (gtmode_buttons_internal.h adds no class): OilChangeCylinder's type name
 * comes first after the vtable (retail 0x0030E550), then the operator
 * lists' and SmartIndexT<int>'s; T040 sees no garage or button list
 * (retail has none of their names). */
#include "game/menu/gtmode/T040_oil_change_cylinder.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

namespace gt2k_menu {

// FUNC 0x00116E00 gt2k_menu::OilChangeCylinder::OilChangeCylinder
/* One caller (T036 make_oil_change_cylinder).  The three pictures come from
 * the layout's hidden layer (Object::operator new(0x100) each; operator
 * delete if an Item constructor throws). */
OilChangeCylinder::OilChangeCylinder(PageManager *pages, f32 dirt, MenuLayoutItem *res,
                                     PageLayout *layout)
    : Item(res, layout)
{
    this->pages = pages;
    this->dirt = dirt;
    started = 0;
    state = OIL_IDLE;
    timer = 0;
    level = 1.0f;
    /* implicit: operator new, operator delete (new-expression) */
    dirty = new Item("dirty", layout);
    clean = new Item("clean", layout);
    oil = new Item("dirty", layout);
}

// FUNC 0x00116F70 gt2k_menu::OilChangeCylinder::~OilChangeCylinder
/* Deletes `dirty' and `clean' only: `oil' (+0x10C) is never deleted (a leak
 * in the original).  Then ~Item(0) and, if (flags & 1), Object's operator
 * delete. */
OilChangeCylinder::~OilChangeCylinder()
{
    delete dirty;
    delete clean;
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00117008 gt2k_menu::OilChangeCylinder::start
/* Starts the animation (T037 action_oil_change). */
void OilChangeCylinder::start()
{
    started = 1;
}

// FUNC 0x00117018 gt2k_menu::OilChangeCylinder::virtual_1
/* Item's update, oil's, then one step of the animation (each step stores
 * the next state as state + 1). */
void OilChangeCylinder::virtual_1()
{
    int s;

    Item::virtual_1();
    oil->virtual_1();
    s = state;
    switch (s) {                            /* jump table 0x0030E4F0, unsigned < 6 */
    case OIL_IDLE:
        if (started != 0) {
            state = s + 1;
            timer = OIL_WAIT_FRAMES;
        }
        break;
    case OIL_WAIT_DRAIN:
        if (--timer < 0) {
            state = s + 1;
            menu_play_se(MENU_SE_OIL);
        }
        break;
    case OIL_DRAINING:
        level = level - OIL_LEVEL_STEP;
        if (level < 0.0f) {
            level = 0.0f;
            timer = OIL_WAIT_FRAMES;
            state = s + 1;
        }
        break;
    case OIL_WAIT_REFILL:
        if (--timer < 0) {
            dirt = 0.0f;
            state = s + 1;
            menu_play_se(MENU_SE_OIL);
        }
        break;
    case OIL_REFILLING:
        level = level + OIL_LEVEL_STEP;
        if (1.0f < level) {
            level = 1.0f;
            state = s + 1;
            pages->setFreeze(0);
        }
        break;
    case OIL_DONE:
        started = 0;
        state = OIL_IDLE;
        break;
    }
}

/* a * (1 - t) + b * t, channel by channel, into a default-constructed
 * colour (retail's four dead 0.502 stores at sp+0x10), returned by value
 * (the copy at sp+0).  An exact form, ? the original's: retail copies the
 * result channel by channel (float stores of the computed registers), which
 * `return c;' does not give (egcs copies the 16 bytes with ldl / ldr); a
 * user-defined copy constructor in Color4f gives it too, but it also drops
 * an exception region that retail has (T052 run_arcade_race's table entry
 * 0x0012F960..0x0012FE90 at 0x0034B468), so the original's Color4f had none. */
static inline Color4f mix_color(const Color4f &a, const Color4f &b, f32 t)
{
    Color4f c;

    c.r = a.r * (1.0f - t) + b.r * t;
    c.g = a.g * (1.0f - t) + b.g * t;
    c.b = a.b * (1.0f - t) + b.b * t;
    c.a = a.a * (1.0f - t) + b.a * t;
    return Color4f(c.r, c.g, c.b, c.a);
}

// FUNC 0x00117170 gt2k_menu::OilChangeCylinder::virtual_2
/* Draw.  oil colour k = clean colour k * (1 - dirt) + dirty colour k * dirt
 * (dirt is read again for each corner: the colour stores may alias it); oil
 * keeps dirty's bottom edge and is level times dirty's height; then oil is
 * drawn and this item over it. */
void OilChangeCylinder::virtual_2()
{
    for (int k = 0; k < 4; k++)
        oil->color[k] = mix_color(clean->color[k], dirty->color[k], dirt);
    f32 fullH = dirty->h;
    f32 oilH = fullH * level;               /* once: retail multiplies once for both */
    oil->y.set(dirty->y.value + fullH * 0.5f - oilH * 0.5f);
    oil->h = oilH;
    oil->virtual_2();
    Item::virtual_2();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001172D0 __tfQ29gt2k_menu17OilChangeCylinder
//   Builds the __si_type_info __tiQ29gt2k_menu17OilChangeCylinder (base Item:
//   __tfQ29gt2k_menu4Item first) on the first call.
extern "C" void *__tfQ29gt2k_menu17OilChangeCylinder(void)
{
    if (__tiQ29gt2k_menu17OilChangeCylinder[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu17OilChangeCylinder, "Q29gt2k_menu17OilChangeCylinder",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu17OilChangeCylinder;
}
#endif
