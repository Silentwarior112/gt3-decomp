/* T209_name_button_list.h - a factory of ButtonLists labelled by a callback
 * (translation unit T209, unit u063), 0x001B5E88..0x001B60B0.
 */
#ifndef GAME_T209_NAME_BUTTON_LIST_H
#define GAME_T209_NAME_BUTTON_LIST_H

#include "game/game.h"

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct LinkPage;                            /* T241: count +0x104, items +0x108 (u063's MenuListElement) */
}

/* NULL if the layout has no item (PageLayout::firstPageItem).  Otherwise
 * `count' entries (a MenuItemArray, T225), each a copy of the page's first
 * record (page->items[0]) with the layout's first item and the label
 * name(buf, 0x80, i); a ButtonList (relative 1, spacing 4.0) centred in rect
 * (y turned up: 448 - centre); a FreeAgent (T236) holding the entries' block
 * hangs under the list.  `parent' is not used.  One caller: T044's
 * "menu_regulated_car" factory (0x0011AB78, u009). */
gt2k_menu::ButtonList *create_name_button_list(gt2k_menu::LinkPage *page,
                                               gt2k_menu::MenuLayoutItem *rect, void *parent,
                                               int controlType, int loop,
                                               gt2k_menu::PageLayout *layout, int index, int count,
                                               void (*name)(char *buf, int size, int i)); /* 0x001B5E88 */

#endif /* GAME_T209_NAME_BUTTON_LIST_H */
