/* T235_color_box.h - gt2k_menu::ColorBox (translation unit T235, unit
 * u069), 0x001C5228..0x001C5D40; the first-pass name gt2k_menu::MenuResource
 * of the layout item (the gear chart data GearChartData, placed here by the
 * plan, is in T235_gear_chart_data.h, included below).
 *
 * ColorBox: a colour choice (the body colours of a car).  `count' swatches,
 * copies of the Item tinted with colors[i], drawn downwards h + 2.5 apart
 * above the Item's place (swatch i at y - (count - i) * (h + 2.5)), and a
 * blinking marker left of the swatch under the cursor.  Up / down move the
 * cursor while no popup is open; every frame the cursor goes to the setColor
 * callback.  Built by the page hook create_color_box (0x001C5B80; operator
 * new(0x130)). */
#ifndef GAME_T235_COLOR_BOX_H
#define GAME_T235_COLOR_BOX_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuLayoutItem (MenuResource) */
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int> (ColorBox::index) */
#include "game/menu/framework/T235_gear_chart_data.h"      /* GearChartData (a plain record; its own header since
                                               u069's data pass, included here where it was) */

namespace gt2k_menu {
class ControlBase;                          /* T247 (pointers only) */
class PageManager;                          /* T211 */
class Object;                               /* T220 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuTexture;                         /* T213 */

/* MenuResource (u002, u032, u065, u069) is the layout item of T229: the same
 * 0x6C-byte record (name, flags, x / y / z / w / h, type, active, uv,
 * colors, text; T243 relocate_item 0x001C7BF0 relocates it).  Item (T213),
 * Button (T206) and PageLayout (T229) take it as MenuLayoutItem, and so do
 * the h14 headers; the name stays as this alias (T244 repeats the typedef). */
typedef MenuLayoutItem MenuResource;
EGCS_SIZE(gt2k_menu::MenuResource, 0x6C);

/* Called every frame with the cursor (the colour index). */
typedef void ColorBoxSetFn(ControlBase *owner, int index);
/* The colours (RGBA8) and their count (car_color_box_colors, T036). */
typedef const u32 *ColorBoxGetFn(int *count);

/* vtable 0x00336430 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's 0x001BB9F8 /
 * 0x001BBA00).  typeinfo __tfQ29gt2k_menu8ColorBox 0x001C5C68 (si, base
 * Item; __si_type_info 0x004B2600).  Size 0x130 (operator new in
 * create_color_box). */
class ColorBox : public Item {
public:
    ColorBox(ControlBase *owner, const u32 *colors, int count, PageManager *pages,
             ColorBoxSetFn *setColor, MenuLayoutItem *res, MenuTexture *tex); /* 0x001C5228 cursor 0
                                               in [0, count), clamped; blink 1.0 */
    /*  [0] +0x0C 0x001C5CB8 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001C5340 update (nothing while
                                               pages->popupLevel > 0): Transitive's (not Item's), the
                                               blink, up / down of pad 1 move the cursor, then
                                               setColor(owner, cursor) */
    virtual void virtual_2();               /* [2] +0x1C 0x001C5418 draw: the swatches and the marker */

    ControlBase      *owner;                /* 0x100 */
    const u32        *colors;               /* 0x104 RGBA8 (R in the low byte) */
    int               count;                /* 0x108 */
    SmartIndexT<int>  index;                /* 0x10C the cursor */
    PageManager      *pages;                /* 0x124 owner->pageManager */
    ColorBoxSetFn    *setColor;             /* 0x128 */
    f32               blink;                /* 0x12C the marker's brightness, 1.0 down to 0 */
};
EGCS_SIZE(gt2k_menu::ColorBox, 0x130);

/* The hook of "car_color_box" (a MenuHookFn, T229; no direct callers): ctx is
 * the screen's controller (the owner), p8 a ColorBoxGetFn, pC a
 * ColorBoxSetFn; z from the item.  (u012 / u069 declared it locally before
 * the sweep.) */
Transitive *create_color_box(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14);  /* 0x001C5B80 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu8ColorBox(void);      /* 0x001C5C68 */
}
#endif

#endif /* GAME_T235_COLOR_BOX_H */
