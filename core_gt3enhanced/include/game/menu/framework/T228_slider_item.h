/* T228_slider_item.h - gt2k_menu::SliderItem (translation unit T228, unit u067).
 *
 * A bar (the layout item's picture) with a knob, the layout's hidden item
 * "pinch".  getter(ctx, &min, &end, &value) gives the setting at every draw;
 * the knob is drawn at step value - min of end - min along the bar (T238
 * Slider::knobX).  Built by the hook create_slider_item (the option screens,
 * u022: p8 is the getter). */
#ifndef GAME_T228_SLIDER_ITEM_H
#define GAME_T228_SLIDER_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* The setting a SliderItem shows: the range [min, end) and the value. */
typedef void (*SliderGetter)(void *ctx, int *min, int *end, int *value);

/* vtable 0x00335588 (8 slots: [1] / [4] Item's, [3] Object's, [5] ImageGZ's,
 * [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu10SliderItem 0x001C2270
 * (si, base Item; __si_type_info 0x004B25A0).  Size 0x208 (operator new in
 * create_slider_item). */
class SliderItem : public Item {
public:
    SliderItem(SliderGetter getter, void *ctx, PageLayout *layout, MenuLayoutItem *item,
               MenuTexture *tex);           /* 0x001C1FC8 the knob: the hidden item "pinch" */
    /*  [0] +0x0C 0x001C22C0 the implicit destructor: only ~Item of its Item member and of its base, no vptr store */
    virtual void virtual_2();               /* [2] +0x1C 0x001C20F0 the bar, then the knob at the value */

    SliderGetter getter;                    /* 0x100 */
    void        *ctx;                       /* 0x104 */
    Item         pinch;                     /* 0x108 the knob */
};
EGCS_SIZE(gt2k_menu::SliderItem, 0x208);

/* The "slider" hook (a MenuHookFn, T229): p8 is the SliderGetter, ctx (the
 * hook context) its context; parent, pC..p14 are not used. */
Transitive *create_slider_item(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                               int p8, int pC, int p10, int p14);  /* 0x001C21C0 a SliderItem */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu10SliderItem(void);    /* 0x001C2270 */
#endif

#endif /* GAME_T228_SLIDER_ITEM_H */
