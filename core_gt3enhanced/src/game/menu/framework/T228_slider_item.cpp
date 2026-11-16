/* T228 - gt2k_menu::SliderItem.  0x001C1FC8..0x001C2318.
 *
 * A bar (the layout item's texture) with a knob, the layout's hidden item
 * "pinch".  getter(ctx, &min, &end, &value) gives the setting each draw;
 * the knob is drawn at step value - min of end - min along the bar (T238
 * Slider::knobX). */
/* Retail's .rodata (0x00335588..0x0033570E): the vtable, then the type names
 * SliderItem, SmartIndexT<int>, Cell<Button>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, Operator: the slider's header (its
 * Button and SmartIndexDoubleRange), not the button list. */
#include "game/menu/framework/T238_slider.h"               /* gt2k_menu::Slider::knobX */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (findHiddenItem, getTexture), MenuLayoutItem */
#include "game/menu/framework/T228_slider_item.h"
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

// FUNC 0x001C1FC8 gt2k_menu::SliderItem::SliderItem
SliderItem::SliderItem(SliderGetter get, void *getCtx, PageLayout *layout, MenuLayoutItem *item,
                       MenuTexture *tex)
    : Item(), getter(get), ctx(getCtx), pinch()
{
    setResource(item, tex);
    MenuLayoutItem *knob = layout->findHiddenItem("pinch");
    if (knob != 0)
        pinch.setResource(knob, layout->getTexture(knob));
}

// FUNC 0x001C20F0 gt2k_menu::SliderItem::virtual_2
void SliderItem::virtual_2()
{
    int min, end, value;                    /* sp+0x0 / +0x4 / +0x8 */

    Item::virtual_2();
    getter(ctx, &min, &end, &value);
    int knobW = (int)pinch.getW();          /* cvt.w.s: truncates; the accessor: retail
                                               reads it through &pinch, kept in $s4 */
    f32 knobCentre = Slider::knobX(value - min, knobW, end - min, getScreenX(), w);
    pinch.x.set(knobCentre);
    pinch.y.set(y.value);
    pinch.Item::virtual_2();
}

/* The "slider" hook (a MenuHookFn, no direct callers): parent is not used,
 * p8 is the getter, ctx (the hook context) its context. */
// FUNC 0x001C21C0 gt2k_menu::create_slider_item
Transitive *create_slider_item(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                               int p8, int pC, int p10, int p14)
{
    SliderGetter getter = (SliderGetter)p8;
    MenuTexture *tex = layout->getTexture(item);
    return new SliderItem(getter, ctx, layout, item, tex);  /* 0x208 bytes */  /* implicit: Object::operator delete (if the constructor throws) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C2270 __tfQ29gt2k_menu10SliderItem
extern "C" void *__tfQ29gt2k_menu10SliderItem(void)
{
    if (__tiQ29gt2k_menu10SliderItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu10SliderItem, "Q29gt2k_menu10SliderItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu10SliderItem;
}
#endif

// FUNC 0x001C22C0 gt2k_menu::SliderItem::~SliderItem
/* the implicit destructor (game/menu/framework/T228_slider_item.h): pinch.~Item (flags 2),
 * Item::~Item (flags 0) without a vptr store, then Object::operator delete
 * if flags & 1 */
/* implicit: ~Item (member and base destructors), operator delete (deleting destructor) */
