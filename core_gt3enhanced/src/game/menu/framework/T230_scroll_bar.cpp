/* T230 - gt2k_menu::ScrollBar.  0x001C33D8..0x001C3828.
 *
 * The scroll bar of a ButtonList (hook create_scroll_bar): its own picture,
 * and over it the "pinch" (the hidden item of the layout, else a magenta
 * quad) sized and placed as the visible part of the list's contents.  Drawn
 * only while the contents are longer than the bar.
 *
 * Data (retail .rodata 0x00335A58..0x00335C00): ScrollBar's vtable, then the
 * type names ScrollBar, SmartIndexT<int>, List<Button>, Cell<Button>, the
 * operator list classes (Item's), Operator: the original read the Button
 * header, then ButtonList's (its List<Button> base, then the
 * SmartIndexT<int> index member).  .sdata 0x00352620: "pinch". */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T206_button.h"               /* data layout only: Item's classes, Cell<Button> */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (getTotalSize, getSize,
                                               scrollOffset, count) */
#include "game/menu/framework/T230_scroll_bar.h"
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setResource, virtual_2) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hook's result) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (findHiddenItem, getTexture) */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable::setRect */
#include "engine/pgl/G001_flare.h"                /* pgl_color */
#include "game/common/T494_text_utils.h"           /* draw_rect */

namespace gt2k_menu {

/* pinchItem starts empty (Item(NULL, NULL)) and gets the hidden item
 * "pinch" if the layout has one. */
// FUNC 0x001C33D8 gt2k_menu::ScrollBar::ScrollBar
ScrollBar::ScrollBar(ButtonList *target, PageLayout *layout, MenuLayoutItem *item, MenuTexture *tex)
    : Item(item, tex), list(target)
{
    pinch = layout->findHiddenItem("pinch");
    if (pinch != 0)
        pinchItem.setResource(pinch, layout->getTexture(pinch));
}

/* The bar runs along x for a horizontal list (controlType != 0), along y
 * else; x / y are centres.  The pinch is the list's own extent (its w or h)
 * scaled by bar / contents, moved by the list's scroll position.  Retail
 * shows ButtonList's inline getTotalSize / getSize (getSize's controlType
 * load is not shared with the test after it) and Drawable's inline setRect
 * (the pinch's w / h stored through &pinchItem). */
// FUNC 0x001C34C8 gt2k_menu::ScrollBar::virtual_2
void ScrollBar::virtual_2()
{
    f32 contents = list->getTotalSize();   /* implicit: totalHeight, totalWidth (inline getTotalSize) */
    if (contents <= 0.0f)
        return;
    f32 barLength = (list->controlType == 0) ? h : w;
    if (contents <= barLength)
        return;

    Item::virtual_2();

    /* The bar's length over the contents' length, and the list's scroll
     * position on that scale (each arm calls scrollOffset: jump2 merges
     * the arms from the division on, as in retail). */
    f32 scale, offset;
    if (list->controlType == 0) {
        f32 pos = list->y.value;
        scale = h / contents;
        offset = -(pos + list->scrollOffset()) * scale;
    } else {
        f32 pos = list->x.value;
        scale = w / contents;
        offset = -(pos + list->scrollOffset()) * scale;
    }

    f32 span = list->getSize() * scale;
    f32 pinchX, pinchY, pinchW, pinchH;
    if (list->controlType == 0) {
        pinchX = x.value;
        pinchY = (y.value - h * 0.5f) + span * 0.5f + offset;
        pinchW = w;
        pinchH = span;
    } else {
        pinchX = (x.value - w * 0.5f) + span * 0.5f + offset;
        pinchY = y.value;
        pinchW = span;
        pinchH = h;
    }

    if (pinch != 0) {
        pinchItem.Drawable::setRect(pinchX, pinchY, pinchW, pinchH);
        pinchItem.virtual_2();
    } else {
        pgl_color(SCROLL_BAR_PINCH_COLOR);
        draw_rect(pinchX, pinchY, pinchW + 1.0f, pinchH + 1.0f);
    }
}

/* Hook: a ScrollBar of the list getList(ctx) (p8), NULL if there is no
 * list or it has no buttons. */
// FUNC 0x001C36B0 gt2k_menu::create_scroll_bar
Transitive *create_scroll_bar(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    ButtonList *(*getList)(void *ctx) = (ButtonList *(*)(void *))p8;
    MenuTexture *tex = layout->getTexture(item);
    ButtonList *list = getList(ctx);
    if (list == 0)
        return 0;
    if (list->count() <= 0)
        return 0;
    return new ScrollBar(list, layout, item, tex);  /* implicit: Object::operator delete (if the constructor throws) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C3780 __tfQ29gt2k_menu9ScrollBar
extern "C" void *__tfQ29gt2k_menu9ScrollBar(void)
{
    if (__tiQ29gt2k_menu9ScrollBar[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9ScrollBar, "Q29gt2k_menu9ScrollBar",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9ScrollBar;
}
#endif

// FUNC 0x001C37D0 gt2k_menu::ScrollBar::~ScrollBar
/* implicit (game/menu/framework/T230_scroll_bar.h); egcs emits this copy: pinchItem's ~Item,
 * then the base's ~Item; operator delete (0x001BF458) if flags & 1. */
/* implicit: ~Item (member and base destructors), operator delete (deleting destructor) */
