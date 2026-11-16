/* T042_event_hider.h - gt2k_menu::EventHider, the cover over the closed
 * events of a GT-mode race list (translation unit T042, unit u008),
 * 0x00119200..0x00119778.
 *
 * A picture drawn over every button of a ButtonList whose event is not
 * open (isOpen(the button's record name) == 0, T036 F_00120F30): moved to
 * the button's position, grey 0x80 on the focused button, else 0x40
 * (alpha 0x80).
 *
 * gt2k_menu::ListIterator<T> (the inline walk over a List<T>, nine units)
 * is defined in menu_list.h with the Cell / List templates; this header
 * includes it. */
#ifndef GAME_T042_EVENT_HIDER_H
#define GAME_T042_EVENT_HIDER_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::ListIterator<T> */
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* vtable 0x0030F590 (8 slots: [1] Item's 0x001B9F98, [3] Object's, [4]
 * Item's, [5] Image's, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu10EventHider 0x001196E0 (si, base Item; storage
 * 0x004B1580).  Size 0x108 (operator new in T036 F_00120F80).
 *
 * The destructor is implicit in the original: 0x00119730 does not store the
 * vptr (egcs stores it in every user-declared destructor; the destructor it
 * generates does not when no class of the hierarchy has two bases: T047's
 * implicit ones, under Button / ButtonList, do), only ~Item and the
 * deleting part. */
class EventHider : public Item {
public:
    EventHider(MenuLayoutItem *res, MenuTexture *tex, ButtonList *list,
               int (*isOpen)(const char *event));   /* 0x00119200 Item(res, tex) */
    /*  [0] +0x0C 0x00119730 ~EventHider: implicit (see above) */
    virtual void virtual_2();               /* [2] +0x1C 0x00119258 draw: Item's once per closed button */

    ButtonList *list;                       /* 0x100 */
    int (*isOpen)(const char *event);       /* 0x104 called with Button::linkMenu (its name) */
};
EGCS_SIZE(gt2k_menu::EventHider, 0x108);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu10EventHider(void);       /* 0x001196E0 */
#endif

#endif /* GAME_T042_EVENT_HIDER_H */
