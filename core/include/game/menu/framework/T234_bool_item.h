/* T234_bool_item.h - gt2k_menu::BoolItem (translation unit T234, unit u068).
 *
 * An Item drawn only while test(ctx) is non-zero (or, built with once, if
 * the test made when it was built said so).  Built by the hook
 * create_bool_item (the garage, option and setting screens: u012, u022,
 * u034). */
#ifndef GAME_T234_BOOL_ITEM_H
#define GAME_T234_BOOL_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* vtable 0x003362F8 (8 slots: [1] / [4] Item's, [3] Object's, [5] ImageGZ's,
 * [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu8BoolItem 0x001C5190
 * (si, base Item; __si_type_info 0x004B25F0).  Size 0x110 (operator new in
 * create_bool_item 0x001C5124). */
class BoolItem : public Item {
public:
    BoolItem(int (*test)(void *ctx), void *ctx, MenuLayoutItem *item, MenuTexture *tex,
             int once);                     /* 0x001C4FD0 once: value = test(ctx) now */
    /*  [0] +0x0C 0x001C51E0 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_2();               /* [2] +0x1C 0x001C5078 Item's while the test holds */

    int  (*test)(void *ctx);                /* 0x100 */
    void  *ctx;                             /* 0x104 */
    int    value;                           /* 0x108 test(ctx) at creation (once) */
    int    once;                            /* 0x10C 1: tested once */
};
EGCS_SIZE(gt2k_menu::BoolItem, 0x110);

/* The BoolItem hook (a MenuHookFn, T229): p8 the test, int (*)(void *ctx)
 * (called with the hook context), pC the mode (1: tested once, else at
 * every draw). */
Transitive *create_bool_item(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14);  /* 0x001C50D8 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu8BoolItem(void);   /* 0x001C5190 */
#endif

#endif /* GAME_T234_BOOL_ITEM_H */
