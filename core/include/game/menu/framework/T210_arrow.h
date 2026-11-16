/* T210_arrow.h - gt2k_menu::Arrow (translation unit T210, unit u063),
 * 0x001B60B0..0x001B6250.
 *
 * A scroll arrow of a ButtonList ("arrow_inc" / "arrow_dec" of the layout,
 * u011): hidden while the list's index stops at its end and is on the last
 * entry (dir 0, inc) or stops at its beginning and is on the first entry
 * (dir 1, dec).
 */
#ifndef GAME_T210_ARROW_H
#define GAME_T210_ARROW_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x00333608 (8 slots: [2] / [4] Item's, [3] Object's, [5] Image's,
 * [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu5Arrow 0x001B61B8
 * (__si_type_info 0x004B24C0, base Item).  Size 0x108. */
class Arrow : public Item {
public:
    Arrow(int dir, ButtonList *list, MenuLayoutItem *elem, PageLayout *layout); /* 0x001B60B0 Item(elem,
                                               layout); one caller */
    /*  [0] +0x0C 0x001B6208 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001B6110 Item's, then visible from the
                                               list's index */

    int         dir;                        /* 0x100 0 "arrow_inc" (forwards), 1 "arrow_dec" */
    ButtonList *list;                       /* 0x104 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::Arrow, 0x108);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu5Arrow(void);      /* 0x001B61B8 */
#endif

#endif /* GAME_T210_ARROW_H */
