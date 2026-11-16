/* T268 - gt2k_menu::FreeButton.  0x001C4550..0x001C4978.
 *
 * Made by ButtonList::createFreeButton (T244) for a button record with
 * sub-entries: the button carries a ButtonList of them (a child object,
 * placed at the button's screen position every frame), which gets the input
 * while the button does.  When the button loses the focus the sub list's
 * focus is switched off; when it gets it back, the sub list is scrolled back
 * to the position saved by the update (three statics, shared by every
 * FreeButton) if its cursor had moved or its count did not grow.
 *
 * Data (retail .rodata 0x00335D80..0x00335FA0): FreeButton's two vtables
 * (the Cell<Button> part's first), then the type names FreeButton,
 * SmartIndexT<int>, List<Button>, Cell<Button>, the operator list classes
 * (Item's), Operator: the Button header, then ButtonList's.  .sdata
 * 0x00352648..0x00352660: the three statics below, then (8-aligned) the
 * base list of FreeButton's __class_type_info (g_00352658, egcs's own). */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Item, Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (the sub list), ButtonData */
#include "game/menu/framework/T268_free_button.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (getPrev / getNext) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable (getX .. getH) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue::set */
#include "game/menu/framework/T495_menu_list.h"            /* gt2k_menu::pList::count */

/* Defined with an initialiser: retail has them in .sdata (zero) in front of
 * the base list egcs writes with the typeinfo function at the end. */
f32 free_button_saved_position = 0.0f;      /* 0x00352648 */
int free_button_saved_count = 0;            /* 0x0035264C */
int free_button_cursor_moved = 0;           /* 0x00352650 */

namespace gt2k_menu {

/* loop (the owner's) is not used.  The sub list: (subControlType, subLoop)
 * from the record's flags bits 4 / 5, the record's sub-entries, the button's
 * rectangle, spacing 4, the cursor at 0.  depth 0 (the first button) starts
 * with the focus.  The calls of virtual_13 / virtual_14 are direct (in the
 * constructor).  The rectangle through Drawable's inline getX() .. getH()
 * (as T267's ConcurrentButton): the inline calls are evaluated first, so the
 * stack argument is stored last (retail's jal delay slot). */
// FUNC 0x001C4550 gt2k_menu::FreeButton::FreeButton
FreeButton::FreeButton(ButtonList *owner_, int controlType_, int loop, PageLayout *layout_,
                       f32 offPoint_, ButtonData *linkMenu_, MenuLayoutItem *elem,
                       ButtonData **items, int count, int subControlType, int subLoop,
                       int relative_, int depth_)
    : Button(owner_, controlType_, layout_, offPoint_, linkMenu_, elem, relative_, depth_)
{
    subList = new ButtonList(subControlType, subLoop, layout_, getX(), getY(), getW(), getH(), /* implicit: operator new, operator delete (new-expression) */
                             SUB_LIST_SPACING, items, count, relative_, 0, 0);
    subList->attachTo(this);
    if (depth_ == 0)
        FreeButton::virtual_13();
    else
        FreeButton::virtual_14();
    free_button_saved_position = 0.0f;
    free_button_saved_count = 0;
    free_button_cursor_moved = 0;
}

/* Both vtables, then ~Button; operator delete (0x001BF458) if flags & 1.
 * The sub list is a child object: the tree deletes it. */
// FUNC 0x001C46B0 gt2k_menu::FreeButton::~FreeButton
FreeButton::~FreeButton()
{
    /* implicit: ~Button (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001C4710 gt2k_menu::FreeButton::virtual_9
void FreeButton::virtual_9(int visible_)
{
    Button::virtual_9(visible_);
    if (subList != 0)
        subList->hideButtons(visible_ ^ 1);
}

// FUNC 0x001C4758 gt2k_menu::FreeButton::virtual_10
void FreeButton::virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                    ButtonList *list, int a4))
{
    subList->setDecideFunc(ctx, func);
}

/* Moves the sub list to the button; while its focus is on, saves its
 * position and count.  The test is ButtonList::isEnabledFocus() expanded
 * inline (focusDisabled ^ 1, the xori of 0x001C47B4). */
// FUNC 0x001C4778 gt2k_menu::FreeButton::virtual_1
void FreeButton::virtual_1()
{
    Button::virtual_1();
    ButtonList *sub = subList;
    sub->x.set(posX);
    sub->y.set(posY);
    if ((sub->focusDisabled ^ 1) != 0) {
        free_button_saved_position = sub->focusPosition();
        free_button_saved_count = subList->count();
    }
}

/* The input goes to the sub list; a move along it sets free_button_cursor_moved. */
// FUNC 0x001C47F0 gt2k_menu::FreeButton::virtual_11
int FreeButton::virtual_11(const MenuControl &ctl)
{
    subList->doControl(ctl);
    if (ctl.getPrev(subList->controlType) != 0 || ctl.getNext(subList->controlType) != 0)
        free_button_cursor_moved = 1;
    return 1;
}

// FUNC 0x001C4870 gt2k_menu::FreeButton::virtual_12
void FreeButton::virtual_12()
{
    subList->decide();
}

/* The focus comes: scroll back (the cursor moved, or the count is not above
 * the saved one), then the sub list's focus on. */
// FUNC 0x001C48A0 gt2k_menu::FreeButton::virtual_13
void FreeButton::virtual_13()
{
    if (free_button_cursor_moved != 0 || subList->count() <= free_button_saved_count)
        subList->setCursorAt(free_button_saved_position);
    subList->enableFocus();
    free_button_cursor_moved = 0;
}

/* The focus goes: the sub list's focus off. */
// FUNC 0x001C4900 gt2k_menu::FreeButton::virtual_14
void FreeButton::virtual_14()
{
    subList->disableFocus();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, base Button). */
// FUNC 0x001C4920 __tfQ29gt2k_menu10FreeButton
extern "C" void *__tfQ29gt2k_menu10FreeButton(void)
{
    if (__tiQ29gt2k_menu10FreeButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu10FreeButton, "Q29gt2k_menu10FreeButton", g_00352658, 1);
    }
    return __tiQ29gt2k_menu10FreeButton;
}
#endif
