/* T221 - gt2k_menu::CallbackItem.  0x001C0198..0x001C0640.
 *
 * A Transitive holding `count' Items (array on the menu heap).  Every frame
 * it updates all of them, asks callback(ctx) which one is current, takes
 * that one's z, and draws only that one. */
/* Retail's .rodata (0x00334940..0x00334A80): the vtable, then the type names
 * CallbackItem, OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * Operator (T216's operator classes). */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (getTexture, findHiddenItem), MenuLayoutItem */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture */
#include "game/menu/framework/T221_callback_item.h"
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

// FUNC 0x001C0198 gt2k_menu::CallbackItem::CallbackItem
CallbackItem::CallbackItem(int itemCount, int (*select)(void *ctx), void *selectCtx)
    : Transitive()
{
    count = itemCount;
    callback = select;
    ctx = selectCtx;
    current = -1;
    items = (Item **)menu_heap_current()->allocate(count * 4, 0x10);
    for (int i = 0; i < count; i++)
        items[i] = 0;
}

/* Deletes every item (virtual destructor, flags 3), frees the array on the
 * menu heap, then Transitive::~Transitive. */
// FUNC 0x001C0280 gt2k_menu::CallbackItem::~CallbackItem
CallbackItem::~CallbackItem()
{
    for (int i = 0; i < count; i++) {
        if (items[i] != 0) {
            delete items[i];
            items[i] = 0;
        }
    }
    menu_heap_current()->free(items);
    items = 0; /* implicit: operator delete (deleting destructor) */
}

/* items[index] = a new Item of item (its texture from layout); nothing if
 * item is NULL. */
// FUNC 0x001C0360 gt2k_menu::CallbackItem::setItem
void CallbackItem::setItem(MenuLayoutItem *item, PageLayout *layout, int index)
{
    if (item != 0) {
        MenuTexture *tex = layout->getTexture(item);
        items[index] = new Item(item, tex);     /* 0x100 bytes, Object::operator new */ /* implicit: operator new, operator delete (new-expression) */
    }
}

// FUNC 0x001C0408 gt2k_menu::CallbackItem::setItemByName
void CallbackItem::setItemByName(const char *name, PageLayout *layout, int index)
{
    setItem(layout->findHiddenItem(name), layout, index);
}

// FUNC 0x001C0460 gt2k_menu::CallbackItem::startScript
void CallbackItem::startScript(const OperatorCode *script)
{
    for (int i = 0; i < count; i++) {
        if (items[i] != 0)
            items[i]->start(script);
    }
}

/* Update: every item, then current = callback(ctx); the current item's z
 * becomes this one's. */
// FUNC 0x001C04D8 gt2k_menu::CallbackItem::virtual_1
void CallbackItem::virtual_1()
{
    for (int i = 0; i < count; i++) {
        if (items[i] != 0)
            items[i]->virtual_1();
    }
    current = callback(ctx);
    if (current >= 0 && current < count) {
        Item *item = items[current];
        if (item != 0)
            z = item->z;
    }
}

/* Draw: the current item only. */
// FUNC 0x001C0598 gt2k_menu::CallbackItem::virtual_2
void CallbackItem::virtual_2()
{
    if (current >= 0 && current < count) {
        Item *item = items[current];
        if (item != 0)
            item->virtual_2();
    }
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Transitive). */
// FUNC 0x001C05F0 __tfQ29gt2k_menu12CallbackItem
extern "C" void *__tfQ29gt2k_menu12CallbackItem(void)
{
    if (__tiQ29gt2k_menu12CallbackItem[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu12CallbackItem, "Q29gt2k_menu12CallbackItem",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu12CallbackItem;
}
#endif
