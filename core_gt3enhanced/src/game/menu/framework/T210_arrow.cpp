/* T210 - gt2k_menu::Arrow.  0x001B60B0..0x001B6250.
 *
 * A scroll arrow of a ButtonList: dir 0 ("arrow_inc") is hidden when the
 * list's index stops at its end and is on the last entry, dir 1
 * ("arrow_dec") when it stops at its beginning and is on the first entry.
 *
 * Data (retail .rodata 0x00333608..0x003337AE): Arrow's vtable, then the
 * type names Arrow, SmartIndexT<int>, List<Button>, Cell<Button>, the
 * operator list classes (Item's), Operator: the original read the Button
 * header (Item, Cell<Button>) and ButtonList's (List<Button>,
 * SmartIndexT<int>) before Arrow's. */
#include "game/menu/framework/T206_button.h"               /* (data layout only) gt2k_menu::Button: Cell<Button> */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x00333688 / 0x00333668). */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (index: SmartIndexT<int>) */
#include "game/menu/framework/T210_arrow.h"
#include "game/menu/framework/menu_buttons_internal.h"

namespace gt2k_menu {

enum { ARROW_INC = 0, ARROW_DEC = 1 };      /* Arrow::dir */

/* One caller (T036 0x001213A0).  Item(elem, layout), then dir and the
 * list. */
// FUNC 0x001B60B0 gt2k_menu::Arrow::Arrow
Arrow::Arrow(int dir_, ButtonList *list_, MenuLayoutItem *elem, PageLayout *layout)
    : Item(elem, layout)
{
    dir = dir_;
    list = list_;
}

/* Item's update, then: hidden while the cursor cannot move further in the
 * arrow's direction (a clamped index on its last / first entry).  The
 * switch (? the original's shape: its index is a copy of dir, which the
 * DEC test uses, retail's `daddu $t0, $a2') then the DEC test. */
// FUNC 0x001B6110 gt2k_menu::Arrow::virtual_1
void Arrow::virtual_1()
{
    Item::virtual_1();
    SmartIndexT<int> *idx = &list->index;
    switch (dir) {
    case ARROW_INC:
        if (idx->atEnd()) {
            visible = 0;
            return;
        }
        break;
    }
    if (dir == ARROW_DEC && idx->atBegin()) {
        visible = 0;
        return;
    }
    visible = 1;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001B61B8 __tfQ29gt2k_menu5Arrow
extern "C" void *__tfQ29gt2k_menu5Arrow(void)
{
    if (__tiQ29gt2k_menu5Arrow[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu5Arrow, "Q29gt2k_menu5Arrow",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu5Arrow;
}
#endif

// FUNC 0x001B6208 gt2k_menu::Arrow::~Arrow
/* the implicit destructor (game/menu/framework/T210_arrow.h): Item::~Item(0) without a
 * vptr store, operator delete if flags & 1 */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
