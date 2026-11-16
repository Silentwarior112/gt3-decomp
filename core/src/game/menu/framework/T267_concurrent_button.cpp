/* T267 - gt2k_menu::ConcurrentButton.  0x001C1CC8..0x001C1FC8.
 *
 * A button of a ButtonList with a ButtonList of its own (built by T244
 * ButtonList::createConcurrentButton from the button record's sub list:
 * ButtonData subCount / subItems, flags bits 4 / 5).  The sub list is a
 * child of the button, sits on the button's rectangle and follows it;
 * visibility, input and the decision go to the sub list.  The button itself
 * never takes the focus. */
/* Retail's .rodata (0x00335368..0x00335586): the two vtables, then the type
 * names ConcurrentButton, SmartIndexT<int>, List / Cell<Button>,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, Operator. */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Cell<Button>, T216's operator
                                               classes) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member. */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (the sub list) */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout, MenuLayoutItem */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl */
#include "game/menu/framework/T267_concurrent_button.h"
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

/* `loop' (the owner's) is not read.  The rectangle through Drawable's
 * accessors (with the fields read in place, egcs stores the stack argument
 * first; retail's is in the jal delay slot). */
// FUNC 0x001C1CC8 gt2k_menu::ConcurrentButton::ConcurrentButton
ConcurrentButton::ConcurrentButton(ButtonList *owner, int controlType, int loop,
                                   PageLayout *layout, f32 offPoint, ButtonData *linkMenu,
                                   MenuLayoutItem *elem, ButtonData **items, int count,
                                   int listControlType, int listLoop, int relative, int depth)
    : Button(owner, controlType, layout, offPoint, linkMenu, elem, relative, depth)
{
    list = new ButtonList(listControlType, listLoop, layout, getX(), getY(), getW(), getH(), 4.0f, /* implicit: operator new, operator delete (new-expression) */
                          items, count, relative, 0, 0);    /* 0x128 bytes */
    list->attachTo(this);
    disableFocus = 1;
}

/* Both vtables, Button::~Button (flags 0), then if (flags & 1) the class
 * operator delete.  The sub list is a child object: the tree deletes it. */
// FUNC 0x001C1DF8 gt2k_menu::ConcurrentButton::~ConcurrentButton
ConcurrentButton::~ConcurrentButton()
{
    /* implicit: ~Button (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001C1E58 gt2k_menu::ConcurrentButton::virtual_9
void ConcurrentButton::virtual_9(int visible)
{
    Button::virtual_9(visible);
    if (list != 0)
        list->hideButtons(visible ^ 1);
}

// FUNC 0x001C1EA0 gt2k_menu::ConcurrentButton::virtual_10
void ConcurrentButton::virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                          ButtonList *list, int a4))
{
    list->setDecideFunc(ctx, func);
}

/* Update: the button, then the sub list goes to the button's position. */
// FUNC 0x001C1EC0 gt2k_menu::ConcurrentButton::virtual_1
void ConcurrentButton::virtual_1()
{
    Button::virtual_1();
    list->x.set(posX);
    list->y.set(posY);
}

/* Input goes to the sub list; always handled. */
// FUNC 0x001C1F10 gt2k_menu::ConcurrentButton::virtual_11
int ConcurrentButton::virtual_11(const MenuControl &ctl)
{
    list->doControl(ctl);
    return 1;
}

/* Decided: the sub list's decide (virtual). */
// FUNC 0x001C1F40 gt2k_menu::ConcurrentButton::virtual_12
void ConcurrentButton::virtual_12()
{
    list->decide();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001C1F70 __tfQ29gt2k_menu16ConcurrentButton
extern "C" void *__tfQ29gt2k_menu16ConcurrentButton(void)
{
    if (__tiQ29gt2k_menu16ConcurrentButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu16ConcurrentButton, "Q29gt2k_menu16ConcurrentButton",
                     g_00352608, 1);
    }
    return __tiQ29gt2k_menu16ConcurrentButton;
}
#endif
