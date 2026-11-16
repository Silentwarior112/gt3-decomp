/* T047 - gt2k_menu::TopButtonList and gt2k_menu::TopButton (the GT-mode top
 * menu's list).  0x001168B8..0x00116E00.
 *
 * A ButtonList that creates TopButtons; a TopButton is drawn with
 * alphaRatio = 1 - |posY - list y| / (1.8 h) and not at all once that is
 * <= 0, so the buttons fade out away from the list's centre line.  The
 * list's scrollTarget keeps the focused button centred when the buttons do
 * not fit. */
/* Retail's .rodata (0x0030E130..0x0030E4F0): "(%f, %f, %f) %f %f %f\n", the
 * literal of SimpleCameraBase's inline print() (written where its header is
 * read: the original read it first), the vtables, then the type names in
 * the reverse order of the classes' definitions: TopButtonList, TopButton,
 * SmartIndexT<int>, List / Cell<Button>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, the GameStatus's garage lists,
 * Operator (a base: last).  Nothing references the literal: the anchor
 * below gives the file its first item (u007's data pass found it). */
// DATA_START 0x0030E130
#include "game/camera/T412_simple_camera_base.h"   /* (seen, not used: brings that literal only) */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail's names: SmartIndexT 0x0030E2C0, then List<Button> 0x0030E2E0). */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
/* Own header last (gtmode_buttons_internal.h adds no class): TopButtonList's and TopButton's
 * type names come first after the vtables (retail 0x0030E288). */
#include "game/menu/gtmode/T047_top_button_list.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

namespace gt2k_menu {

// FUNC 0x001168B8 gt2k_menu::TopButtonList::TopButtonList
/* One caller (T044 create_menu_top, relative = 1).  relative is not read;
 * createButtons is called directly (not through the vtable).  (The floats
 * come before relative and index, as the caller's argument order shows:
 * u009 notes; this constructor's code is the same either way.) */
TopButtonList::TopButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                             int count, f32 x, f32 y, f32 w, f32 h, int relative,
                             int index)
    : ButtonList(controlType, loop, layout)
{
    this->x.set(x);
    this->y.set(y);
    this->w = w;
    this->h = h;
    ButtonList::createButtons(items, count, 0);
    resetIndex();
    this->index.set(index);
}

// FUNC 0x001169C8 gt2k_menu::TopButtonList::createButton
/* ButtonList::createButton with a TopButton (Object::operator new(0x160);
 * operator delete if its constructor throws), attached under this list. */
Button *TopButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                    int index)
{
    /* implicit: operator new, operator delete (new-expression) */
    TopButton *button = new TopButton(this, controlType, layout, offPoint, data, elem,
                                      relative, index);
    button->attachTo(this);
    return button;
}

// FUNC 0x00116A90 gt2k_menu::TopButtonList::scrollTarget
/* The scroll target along the list's axis (w and the widths for a
 * horizontal list, h and the heights for a vertical one).  If the buttons
 * are longer than the list, the focused button is centred; otherwise a
 * horizontal list is centred and a vertical one starts at the top.  The
 * length after the focus is computed and not used.  The vertical case
 * (controlType 0) is tested first, in both selections: retail's block order
 * and branch sense. */
f32 TopButtonList::scrollTarget()
{
    f32 size, total, before, offset;

    if (controlType == 0) {
        size = h;
        total = totalHeight();
        before = heightBeforeFocus();
        heightAfterFocus();                     /* result unused */
    } else {
        size = w;
        total = totalWidth();
        before = widthBeforeFocus();
        widthAfterFocus();                      /* result unused */
    }
    if (size < total) {
        Button *focus = focusButton();
        f32 focusSize;
        if (focus != 0)
            focusSize = (controlType == 0) ? focus->h : focus->w;
        else
            focusSize = 0.0f;
        offset = size * 0.5f - (before + focusSize * 0.5f);
    } else {
        offset = 0.0f;
        if (controlType != 0)
            offset = (size - total) * 0.5f;
    }
    return offset - size * 0.5f;
}

// FUNC 0x00116BB0 gt2k_menu::TopButton::TopButton
/* Button(...), then the two TopButton vtables.  offPoint is declared among
 * the first four parameters (see the header). */
TopButton::TopButton(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
                     ButtonData *linkMenu, MenuLayoutItem *elem, int relative, int depth)
    : Button(owner, controlType, layout, offPoint, linkMenu, elem, relative, depth)
{
}

// FUNC 0x00116BF0 gt2k_menu::TopButton::virtual_2
/* Button::virtual_2 (0x001B44E8) without its `visible' test; drawItem /
 * drawFocus only while the button is less than 1.8 heights from the
 * owner's y, its alphaRatio following that distance unless it is decided. */
void TopButton::virtual_2()
{
    beginDraw();
    if (isClipped() == 0) {
        /* boot_fabsf (types.h): the abs.s stays between the subtraction and
         * the loads of h and 1.8, as retail has it. */
        f32 dist = boot_fabsf(posY - owner->y.value) / (h * 1.8f);
        if (dist < 1.0f) {
            if (decide == 0)
                alphaRatio = 1.0f - dist;
            drawItem();
            drawFocus();
        }
    }
    endDraw();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00116C90 __tfQ29gt2k_menu13TopButtonList
//   Builds the __class_type_info __tiQ29gt2k_menu13TopButtonList (one base,
//   ButtonList: __tfQ29gt2k_menu10ButtonList first; base list g_00350FD0) on
//   the first call.
extern "C" void *__tfQ29gt2k_menu13TopButtonList(void)
{
    if (__tiQ29gt2k_menu13TopButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu13TopButtonList, "Q29gt2k_menu13TopButtonList",
                     g_00350FD0, 1);
    }
    return __tiQ29gt2k_menu13TopButtonList;
}
#endif

// FUNC 0x00116CE8 gt2k_menu::TopButtonList::~TopButtonList (_$_Q29gt2k_menu13TopButtonList)
/* the implicit destructor (game/menu/gtmode/T047_top_button_list.h; egcs writes it after
 * the class's __tf): both TopButtonList vtables, ~ButtonList(0), then if
 * (flags & 1) Object's operator delete */
/* implicit: ~ButtonList (base destructor), operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00116D48 __tfQ29gt2k_menu9TopButton
//   Builds the __class_type_info __tiQ29gt2k_menu9TopButton (one base,
//   Button: __tfQ29gt2k_menu6Button first; base list g_00350FD8) on the
//   first call.
extern "C" void *__tfQ29gt2k_menu9TopButton(void)
{
    if (__tiQ29gt2k_menu9TopButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu9TopButton, "Q29gt2k_menu9TopButton", g_00350FD8, 1);
    }
    return __tiQ29gt2k_menu9TopButton;
}
#endif

// FUNC 0x00116DA0 gt2k_menu::TopButton::~TopButton (_$_Q29gt2k_menu9TopButton)
/* the implicit destructor (game/menu/gtmode/T047_top_button_list.h): both TopButton
 * vtables, ~Button(0), then if (flags & 1) Object's operator delete */
/* implicit: ~Button (base destructor), operator delete (deleting destructor) */
