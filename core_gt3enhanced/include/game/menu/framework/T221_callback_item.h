/* T221_callback_item.h - gt2k_menu::CallbackItem (translation unit T221, unit
 * u067).
 *
 * A Transitive holding `count' Items (an array on the menu heap).  Every
 * frame it updates all of them, asks callback(ctx) which one is current,
 * takes that one's z and draws only that one.  Users: u006, u066. */
#ifndef GAME_T221_CALLBACK_ITEM_H
#define GAME_T221_CALLBACK_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {
class Item;                                 /* T213 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct OperatorCode;                        /* T217 */

/* vtable 0x00334940 (8 slots: [3] Object's, [4..7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu12CallbackItem 0x001C05F0 (si, base Transitive;
 * __si_type_info 0x004B2540).  Size 0xD8 (operator new in T030 0x00110098). */
class CallbackItem : public Transitive {
public:
    CallbackItem(int count, int (*callback)(void *ctx), void *ctx); /* 0x001C0198 current -1, the
                                               items array (NULLs) on the menu heap */
    virtual ~CallbackItem();                /* [0] +0x0C 0x001C0280 deletes the items, frees the array */
    virtual void virtual_1();               /* [1] +0x14 0x001C04D8 every item's update; current =
                                               callback(ctx), whose z this takes */
    virtual void virtual_2();               /* [2] +0x1C 0x001C0598 draws the current item only */

    void setItem(MenuLayoutItem *item, PageLayout *layout, int index);  /* 0x001C0360 items[index] =
                                               a new Item of item (nothing if NULL) */
    void setItemByName(const char *name, PageLayout *layout, int index); /* 0x001C0408 the hidden item
                                               `name' (findHiddenItem) */
    void startScript(const OperatorCode *script); /* 0x001C0460 Transitive::start(script) on every item */

    Item **items;                           /* 0xC4 count entries (menu heap) */
    int    count;                           /* 0xC8 */
    int  (*callback)(void *ctx);            /* 0xCC the index of the current item */
    void  *ctx;                             /* 0xD0 */
    int    current;                         /* 0xD4 the last callback result, -1 at first */
};
EGCS_SIZE(gt2k_menu::CallbackItem, 0xD8);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ29gt2k_menu12CallbackItem(void);  /* 0x001C05F0 */
#endif

#endif /* GAME_T221_CALLBACK_ITEM_H */
