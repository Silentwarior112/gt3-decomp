/* T033 - gt2k_menu::FlashItem.  0x00102E68..0x00103038.
 *
 * A menu Item with a counter at +0x104 that runs 0, 8, .. 240 and wraps;
 * +0x100 = 520.0 and +0x108 = 16.0 are only initialised here. */
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's typeinfo function) */
#include "game/menu/framework/T033_flash_item.h"

namespace gt2k_menu {

// FUNC 0x00102E68 gt2k_menu::FlashItem::FlashItem
FlashItem::FlashItem(MenuLayoutItem *res, PageLayout *layout)
    : Item(res, layout)
{
    unk_100 = 520.0f;
    unk_108 = 16.0f;
    counter = 0.0f;
}

/* Item's update, then the counter. */
// FUNC 0x00102EB8 gt2k_menu::FlashItem::virtual_1
void FlashItem::virtual_1()
{
    Item::virtual_1();
    counter = counter + 8.0f;
    if (240.0f < counter)
        counter = 0.0f;
}

// FUNC 0x00102F08 gt2k_menu::FlashItem::virtual_2
void FlashItem::virtual_2()
{
    Item::virtual_2();
}

} /* namespace gt2k_menu */

/* The hook of the layout type "flash_item" (g_00292A18, T030; no direct
 * callers).  Object::operator new (0x001BF3C0, the menu heap, 0x10C bytes),
 * released with Object::operator delete (0x001BF458) if the constructor
 * throws. */
// FUNC 0x00102F28 create_flash_item
gt2k_menu::Transitive *create_flash_item(void *ctx, gt2k_menu::PageLayout *layout,
                                         gt2k_menu::Object *parent,
                                         gt2k_menu::MenuLayoutItem *item,
                                         int p8, int pC, int p10, int p14)
{
    return new gt2k_menu::FlashItem(item, layout); /* implicit: Object::operator delete (if the constructor throws) */
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x00102FA0 __tfQ29gt2k_menu9FlashItem
extern "C" void *__tfQ29gt2k_menu9FlashItem(void)
{
    if (__tiQ29gt2k_menu9FlashItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu9FlashItem, "Q29gt2k_menu9FlashItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9FlashItem;
}
#endif

// FUNC 0x00102FF0 gt2k_menu::FlashItem::~FlashItem
/* implicit in game/menu/framework/T033_flash_item.h; egcs emits this copy (slot [0]):
 * Item::~Item (flags 0), then Object::operator delete with flags & 1; no
 * vptr store. */
/* implicit: ~Item (base destructor), Object::operator delete (deleting destructor) */
