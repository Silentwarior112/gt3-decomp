/* T033_flash_item.h - gt2k_menu::FlashItem, the "flash_item" layout object
 * of the arcade menu (translation unit T033, unit u001),
 * 0x00102E68..0x00103038.
 *
 * An Item of a layout item (T213) with a counter that runs 0, 8, .., 240
 * and wraps (+0x100 = 520 and +0x108 = 16 are only initialised).  Made by
 * create_flash_item, the hook of the type "flash_item" in the arcade hook
 * table g_00292A18 (T030). */
#ifndef GAME_T033_FLASH_ITEM_H
#define GAME_T033_FLASH_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x00309090 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu9FlashItem 0x00102FA0 (si, base Item; __si_type_info
 * 0x004B1440).  Size 0x10C (operator new in create_flash_item). */
class FlashItem : public Item {
public:
    FlashItem(MenuLayoutItem *res, PageLayout *layout);    /* 0x00102E68 Item(res, layout) */
    /* [0] +0x0C 0x00102FF0 ~FlashItem: implicit (no vptr store) */
    virtual void virtual_1();               /* [1] +0x14 0x00102EB8 Item's, then counter += 8 (0 past 240) */
    virtual void virtual_2();               /* [2] +0x1C 0x00102F08 Item's */

    f32 unk_100;                            /* 0x100 520 */
    f32 counter;                            /* 0x104 0, 8, .. 240 */
    f32 unk_108;                            /* 0x108 16 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::FlashItem, 0x10C);

/* new FlashItem(item, layout) (the menu heap): a gt2k_menu::MenuHookFn
 * (T229), of which it uses layout and item only. */
gt2k_menu::Transitive *create_flash_item(void *ctx, gt2k_menu::PageLayout *layout,
                                         gt2k_menu::Object *parent,
                                         gt2k_menu::MenuLayoutItem *item,
                                         int p8, int pC, int p10, int p14);  /* 0x00102F28 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu9FlashItem(void);     /* 0x00102FA0 */
extern u32 __tiQ29gt2k_menu9FlashItem[3];   /* 0x004B1440 (__si_type_info) */
}
#endif

#endif /* GAME_T033_FLASH_ITEM_H */
