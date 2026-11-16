/* T269 - gt2k_menu::MultiButton.  0x001C4978..0x001C4D78.
 *
 * Made by ButtonList::createMultiButton (T244) for a button record named in
 * the layout's multi-button table: the table's function gives the changed
 * callback and the initial choice.  The button carries a ButtonList of the
 * record's sub-entries (the choices), placed at the button every frame;
 * every input frame the callback gets the current choice, then the input
 * goes to the sub list.
 *
 * Data (retail .rodata 0x00335FA0..0x003361C0): MultiButton's two vtables,
 * then the type names MultiButton, SmartIndexT<int>, List<Button>,
 * Cell<Button>, the operator list classes (Item's), Operator, as T268.
 * .sdata 0x00352660: the base list of MultiButton's __class_type_info
 * (g_00352660, egcs's own). */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Item, Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (the sub list), ButtonData */
#include "game/menu/framework/T269_multi_button.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (decide) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (findPageItem), MenuLayoutItem,
                                               MultiButtonInfo */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable (getX .. getH), MENU_SCREEN_W / _H */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue::set */

namespace gt2k_menu {

/* loop is not used.  The sub list's buttons are placed relative to it
 * (scrolling) when the page item of the first choice lies outside the
 * 640 x 448 screen (y measured up from the bottom); the item is not checked
 * for NULL.  offScreen is set before the findPageItem call (retail keeps it
 * in its own $s5); the rectangle through Drawable's inline getX() .. getH(),
 * as in FreeButton. */
// FUNC 0x001C4978 gt2k_menu::MultiButton::MultiButton
MultiButton::MultiButton(ButtonList *owner_, int controlType_, int loop, PageLayout *layout_,
                         f32 offPoint_, ButtonData *linkMenu_, MenuLayoutItem *elem,
                         ButtonData **items, int count, int subControlType, int subLoop,
                         int relative_, int depth_, void *ctx_, const MultiButtonInfo &info)
    : Button(owner_, controlType_, layout_, offPoint_, linkMenu_, elem, relative_, depth_)
{
    changed = info.changed;
    initialIndex = info.index;
    ctx = ctx_;

    int offScreen = 0;
    MenuLayoutItem *first = layout_->findPageItem(items[0]->name);
    f32 halfW = first->w * 0.5f;
    f32 centreX = first->x + halfW;
    f32 halfH = first->h * 0.5f;
    f32 centreY = first->y + halfH;
    f32 screenY = MENU_SCREEN_H - centreY;
    if (centreX + halfW < 0.0f || MENU_SCREEN_W < centreX - halfW ||
        screenY + halfH < 0.0f || MENU_SCREEN_H < screenY - halfH)
        offScreen = 1;

    subList = new ButtonList(subControlType, subLoop, layout_, getX(), getY(), getW(), getH(), /* implicit: operator new, operator delete (new-expression) */
                             SUB_LIST_SPACING, items, count, offScreen, initialIndex, 0);
    subList->refresh();
    subList->attachTo(this);
}

/* Both vtables, then ~Button; operator delete (0x001BF458) if flags & 1. */
// FUNC 0x001C4B70 gt2k_menu::MultiButton::~MultiButton
MultiButton::~MultiButton()
{
    /* implicit: ~Button (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001C4BD0 gt2k_menu::MultiButton::virtual_9
void MultiButton::virtual_9(int visible_)
{
    Button::virtual_9(visible_);
    if (subList != 0)
        subList->hideButtons(visible_ ^ 1);
}

// FUNC 0x001C4C18 gt2k_menu::MultiButton::virtual_10
void MultiButton::virtual_10(void *ctx_, void (*func)(void *ctx, ButtonData *data, int index,
                                                      ButtonList *list, int a4))
{
    subList->setDecideFunc(ctx_, func);
}

// FUNC 0x001C4C38 gt2k_menu::MultiButton::virtual_1
void MultiButton::virtual_1()
{
    Button::virtual_1();
    subList->x.set(posX);
    subList->y.set(posY);
}

/* Returns 1 unless decide was pressed. */
// FUNC 0x001C4C88 gt2k_menu::MultiButton::virtual_11
int MultiButton::virtual_11(const MenuControl &ctl)
{
    changed(ctx, subList->index.value);
    subList->doControl(ctl);
    return ctl.decide == 0;
}

// FUNC 0x001C4CF0 gt2k_menu::MultiButton::virtual_12
void MultiButton::virtual_12()
{
    subList->decide();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, base Button). */
// FUNC 0x001C4D20 __tfQ29gt2k_menu11MultiButton
extern "C" void *__tfQ29gt2k_menu11MultiButton(void)
{
    if (__tiQ29gt2k_menu11MultiButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu11MultiButton, "Q29gt2k_menu11MultiButton", g_00352660, 1);
    }
    return __tiQ29gt2k_menu11MultiButton;
}
#endif
