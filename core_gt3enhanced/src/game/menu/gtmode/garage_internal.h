/* garage_internal.h - unit u010 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0011C9C8..0x0011F0E0, second pass; the header of the whole translation
 * unit T036 (T036_garage.cpp).
 *
 * Translation unit T036 (0x0011C9C8..0x00125B50): the page hooks of the
 * GT-mode menu.  It was split over the work units u010 (its first 63
 * functions), u011 (the next 58), u012 (38, up to T036_static_init) and u013
 * (its last 9, DecayItem and the copies egcs emits); it is one source file
 * again, in this folder (the T036 merge).  The functions have no direct
 * callers: T036_static_init puts them in the MenuHook table g_00296B90,
 * GtmodeControl::begin (T043) gives the table to the PageManager's layouts
 * (PageLayout::setHooks, ctx the GtmodeControl), and
 * PageLayout::createObject (T229) calls an entry's create function for an
 * object item whose type is the entry's name.  The TU holds
 *   - create functions (make_*, MenuHookFn): Items, the 3D car (gt2k_menu::
 *     Car), a MpegImage, the event list's and the regulation pages' objects;
 *   - text callbacks (text_*) of create_callback_text (T239, the entry's p8):
 *     fn(ctl, buf, 0x80), once if the entry's pC is 1, else at every draw;
 *   - the colour getter / setter of "car_color_box" (T235), the list getter
 *     of "scroll_bar" (T230 create_scroll_bar), the test of "is_ride_car"
 *     (T234 create_bool_item), event_unlocked (EventHider::isOpen);
 *   - T036_static_init (0x00123270), which fills the table (221 entries,
 *     sorted by name);
 *   - DecayItem::virtual_4, and what egcs emits at the end of the unit for
 *     the classes whose vtables it writes: DecayItem's typeinfo function and
 *     destructor, RegurationResult's, GarageCar's and Cell<GarageCar>'s.
 * The hooks are declared in game/menu/gtmode/T036_garage.h, DecayItem in
 * game/menu/gtmode/T036_decay_item.h; T036_garage.cpp includes the canonical headers
 * itself, in the order of retail T036's class definitions (its type names),
 * and this header last: the TU's own constants and what only it uses.
 */
#ifndef U010_H
#define U010_H

#include "types.h"

/* ======================================================================== */
/* Constants (only used here)                                               */
/* ======================================================================== */
/* The licences LICENSE_B .. LICENSE_R: T420's LicenseIndex.  The tyre kinds
 * (part id PART_FRONT_TIRE + kind): T482's TIRE_KIND_*.  The rules of a race
 * record (RaceEvent::need*: NEED_*) are T484's; CarEquipments::getDriveTrain's
 * layouts (another order than the rule's) T482's LAYOUT_*. */

/* ShowRoomCar::power / torque's unit tag argument (not read by them). */
enum { UNIT_TAG_PS = 0x7073 /* 'ps' */, UNIT_TAG_DKGM = 0x646B676D /* 'dkgm' */ };

/* The menu's virtual screen height: T259's MENU_SCREEN_H. */

/* Hook p8 of "disp_license_cup_*": licence << 4 | test. */
enum { LICENSE_CUP_SHIFT = 4, LICENSE_CUP_TEST_MASK = 0xF };

/* CarEquipments::getShopFlags (the car record's flags, T482). */
enum {
    SHOP_FLAG_NOT_SOLD = 0x1,               /* not sold by the dealer: no buy price */
    SHOP_FLAG_NO_USED  = 0x2                /* not traded at the used car shop: no sell price */
};

/* ColorBox colours: RGBA8 with alpha 0x80 (opaque on the GS). */
#define COLOR_BOX_ALPHA 0x80000000u

/* The text colour of a regulation icon whose rule the player / the ridden
 * car breaks: RGBA8 (R in the low byte) R 0xFF, G 0x40, B 0x40, alpha 0x80
 * (opaque on the GS): red. */
#define REGULATION_FAIL_COLOR 0x804040FFu

/* ======================================================================== */
/* Declarations, data and helpers (only used here)                          */
/* ======================================================================== */
/* A text callback of this TU as the hook table holds it (T036_garage.h's
 * text_* signature; create_callback_text calls it with the GtmodeControl). */
typedef void GtmodeTextFn(gt2k_menu::GtmodeControl *ctl, char *buf, int size);

/* Stand-ins for inline accessors of the original that the canonical headers
 * declare out of line (their out-of-line copies have no callers).  Made
 * inline there, egcs would move CarInformation's copy to the end of u108's
 * T483 object: not done in this pass (u108 matching).  make_car_render
 * expands them: the inline `this' of getCondition / getColorIndex (the
 * GarageCar's CarInformation base, car + 0x10) is one GCSE expression,
 * kept in $s7 from the dirt to the colour; the image accessor is expanded
 * in the argument list before the real call menu_car_spec() (retail loads
 * texture.data before calling it).  A reference parameter, like `this',
 * converts without a NULL test.  (CarEquipments::getColorIndex 0x002376B0
 * is class-inline in game/data/T482_car_equipments.h since the T482 merge: the
 * stand-in car_color_index is gone.) */
/* CarInformation::getCondition (0x002382A0): &condition. */
static inline CarCondition *car_condition(CarInformation &info)
{
    return &info.condition;
}
/* The texture data of a MenuImageHolder (T226), a TEX1 image (? an inline
 * member of the original; T226 declares none). */
static inline Tex1Image *holder_image(MenuImageHolder &holder)
{
    return (Tex1Image *)holder.data;
}

/* A race time in ms (RaceDataBase::getGoldTime's arithmetic, written out in
 * make_license_time).  A macro: retail addresses each field from the event
 * (lbu 0x1C4 / 0x1C5, lhu 0x1C6); an inline function taking the RaceTime by
 * reference keeps its address in a register. */
#define RACE_TIME_MS(t) ((t).min * 60000 + (t).sec * 1000 + (t).ms)

/* The gold (0), silver (1) or bronze (2) time of the licence test named by
 * item (up to its last '_'), as an Item text.  The body of the three
 * make_time_* hooks; not declared in T036_garage.h (h33: only this TU uses
 * it). */
gt2k_menu::Item *make_license_time(gt2k_menu::PageLayout *layout,
                                   gt2k_menu::MenuLayoutItem *item, int medal);  /* 0x0011DF08 */

/* ======================================================================== */
/* The base list of GarageCar's __class_type_info (the GCC 14 build's       */
/* __tf9GarageCar; egcs generates it itself)                                */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* {&__tiQ29gt2k_menut4Cell1Z9GarageCar, public, offset 0}, {&__ti14CarInformation,
 * public, offset 0x10} (words 0x004B1350, 0x40000000, 0x004B2A50, 0x40000010):
 * T036's copy (.rodata, after the type name "9GarageCar"). */
extern const __class_type_info::base_info g_00311EA0[2];   /* 0x00311EA0 */
#endif /* !BOOT_OLD_GCC */

#endif /* U010_H */
