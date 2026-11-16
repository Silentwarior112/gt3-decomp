/* T230_scroll_bar.h - gt2k_menu::ScrollBar (translation unit T230, unit u068).
 *
 * The scroll bar of a ButtonList (hook create_scroll_bar, used by the garage
 * screens, u010 / u012): its own picture, and over it the "pinch" (the
 * layout's hidden item, else a quad of colour 0x80FF00FF) sized and placed
 * as the visible part of the list's contents.  Drawn only while the
 * contents are longer than the bar (along x for a horizontal list). */
#ifndef GAME_T230_SCROLL_BAR_H
#define GAME_T230_SCROLL_BAR_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class ButtonList;                           /* T244 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* vtable 0x00335A58 (8 slots: [1] / [4] Item's, [3] Object's, [5] ImageGZ's,
 * [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu9ScrollBar 0x001C3780
 * (si, base Item; __si_type_info 0x004B25B0).  Size 0x208 (operator new in
 * create_scroll_bar 0x001C3714). */
class ScrollBar : public Item {
public:
    ScrollBar(ButtonList *list, PageLayout *layout, MenuLayoutItem *item,
              MenuTexture *tex);            /* 0x001C33D8 pinch: the hidden item "pinch" */
    /*  [0] +0x0C 0x001C37D0 the implicit destructor: only ~Item of its Item member and of its base, no vptr store */
    virtual void virtual_2();               /* [2] +0x1C 0x001C34C8 the bar and the pinch */

    ButtonList     *list;                   /* 0x100 */
    MenuLayoutItem *pinch;                  /* 0x104 hidden item "pinch"; NULL: a plain quad */
    Item            pinchItem;              /* 0x108 */
};
EGCS_SIZE(gt2k_menu::ScrollBar, 0x208);

/* The scroll bar hook (a MenuHookFn, T229): p8 gives the list,
 * ButtonList *(*)(void *ctx). */
Transitive *create_scroll_bar(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14);  /* 0x001C36B0 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu9ScrollBar(void);  /* 0x001C3780 */
#endif

#endif /* GAME_T230_SCROLL_BAR_H */
