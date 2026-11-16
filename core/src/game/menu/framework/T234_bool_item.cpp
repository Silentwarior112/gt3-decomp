/* T234 - gt2k_menu::BoolItem.  0x001C4FD0..0x001C5228.
 *
 * Hook create_bool_item: an Item drawn only while test(ctx) (p8) is
 * non-zero; with pC == 1 the test is made once, when the item is built.
 *
 * Data (retail .rodata 0x003362F8..0x00336430): BoolItem's vtable, then the
 * type names BoolItem, the operator list classes (Item's), Operator. */
#include "game/menu/framework/page_layout_internal.h"
#include "game/menu/framework/T234_bool_item.h"
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (virtual_2) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hook's result) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (getTexture) */

namespace gt2k_menu {

/* value stays unset unless once. */
// FUNC 0x001C4FD0 gt2k_menu::BoolItem::BoolItem
BoolItem::BoolItem(int (*test_)(void *ctx), void *ctx_, MenuLayoutItem *item, MenuTexture *tex,
                   int once_)
    : Item(item, tex), test(test_), ctx(ctx_), once(once_)
{
    if (once_ != 0)
        value = test_(ctx_);
}

// FUNC 0x001C5078 gt2k_menu::BoolItem::virtual_2
void BoolItem::virtual_2()
{
    if (once != 0) {
        if (value != 0)
            Item::virtual_2();
    } else if (test(ctx) != 0) {
        Item::virtual_2();
    }
}

/* Hook: ctx = the hook context, p8 = test, pC = mode (1: tested once). */
// FUNC 0x001C50D8 gt2k_menu::create_bool_item
Transitive *create_bool_item(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14)
{
    int (*test)(void *ctx) = (int (*)(void *))p8;
    MenuTexture *tex = layout->getTexture(item);
    int once = (pC == 1);                   /* pC: the mode */
    return new BoolItem(test, ctx, item, tex, once);  /* implicit: Object::operator delete (if the constructor throws) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001C5190 __tfQ29gt2k_menu8BoolItem
extern "C" void *__tfQ29gt2k_menu8BoolItem(void)
{
    if (__tiQ29gt2k_menu8BoolItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu8BoolItem, "Q29gt2k_menu8BoolItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu8BoolItem;
}
#endif

// FUNC 0x001C51E0 gt2k_menu::BoolItem::~BoolItem
/* implicit (game/menu/framework/T234_bool_item.h); egcs emits this copy: ~Item, operator
 * delete (0x001BF458) if flags & 1. */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
