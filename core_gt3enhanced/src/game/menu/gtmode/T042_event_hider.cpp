/* T042 - gt2k_menu::EventHider, the cover over the closed events of a
 * GT-mode race list.  0x00119200..0x00119778.
 *
 * An item drawn over every button of a ButtonList whose event is not open
 * (isOpen(the button's record name) == 0; T036 event_unlocked): moved to
 * the button's position, grey 0x80 on the focused button, else 0x40 (alpha
 * 0x80). */
/* Retail's .rodata (0x0030F590..0x0030F826): the vtable, then the type names
 * in the reverse order of the classes' definitions: EventHider,
 * SmartIndexT<int>, List / Cell<Button>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, the GameStatus's garage lists,
 * Operator (a base: last). */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/framework/T206_button.h"               /* Button (posX / posY, linkMenu) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail's names: SmartIndexT 0x0030F5F8, then List<Button> 0x0030F618). */
#include "game/menu/framework/T244_button_list.h"          /* ButtonList::focusButton */
/* Own header last (gtmode_buttons_internal.h adds no class): EventHider's type name comes
 * first after the vtable (retail 0x0030F5D8). */
#include "game/menu/gtmode/T042_event_hider.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

namespace gt2k_menu {

// FUNC 0x00119200 gt2k_menu::EventHider::EventHider
/* One caller (T036 make_event_hide, isOpen = event_unlocked). */
EventHider::EventHider(MenuLayoutItem *res, MenuTexture *tex, ButtonList *list,
                       int (*isOpen)(const char *event))
    : Item(res, tex)
{
    this->list = list;
    this->isOpen = isOpen;
}

// FUNC 0x00119258 gt2k_menu::EventHider::virtual_2
/* Draw: this item (Item::virtual_2) once per closed button, at its position
 * (Drawable::setColor inline for the grey).  The walk takes the list by
 * reference (retail converts it to its List<Button> base without a null
 * check). */
void EventHider::virtual_2()
{
    Button *focus = list->focusButton();
    ListIterator<Button> it(*list);
    Button *button;

    while ((button = it.next()) != 0) {
        if (isOpen(button->linkMenu->name) != 0)
            continue;
        x.set(button->posX);
        y.set(button->posY);
        if (button == focus)
            Drawable::setColor(MENU_GREY_FOCUSED);
        else
            Drawable::setColor(MENU_GREY_UNFOCUSED);
        Item::virtual_2();
    }
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x001196E0 __tfQ29gt2k_menu10EventHider
//   Builds the __si_type_info __tiQ29gt2k_menu10EventHider (base Item:
//   __tfQ29gt2k_menu4Item first) on the first call.
extern "C" void *__tfQ29gt2k_menu10EventHider(void)
{
    if (__tiQ29gt2k_menu10EventHider[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu10EventHider, "Q29gt2k_menu10EventHider",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu10EventHider;
}
#endif

// FUNC 0x00119730 gt2k_menu::EventHider::~EventHider (_$_Q29gt2k_menu10EventHider)
/* the implicit destructor (game/menu/gtmode/T042_event_hider.h): ~Item(0) without a
 * vptr store, Object's operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
