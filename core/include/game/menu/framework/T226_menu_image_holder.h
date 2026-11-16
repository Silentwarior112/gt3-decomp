/* T226_menu_image_holder.h - MenuImageHolder (translation unit T226, unit u067).
 *
 * (? name) The texture of a layout item, held on the stack by T036
 * make_car_render (the car view factory): a menu-heap copy unpacked from gzip
 * (and its TEX1 offsets relocated) when the item's flag 0x40000 is set,
 * else the texture data itself.  handOver() hangs a FreeAgent (T236) owning
 * the copy under the object that uses it.  Not polymorphic. */
#ifndef GAME_T226_MENU_IMAGE_HOLDER_H
#define GAME_T226_MENU_IMAGE_HOLDER_H

#include "game/game.h"

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */
}
struct Tex1Image;                           /* G001 (pointers only) */

/* Size 0xC (a local of T036 make_car_render). */
struct MenuImageHolder {
    MenuImageHolder(gt2k_menu::MenuLayoutItem *item, gt2k_menu::MenuTexture *tex); /* 0x001C1A18 */
    ~MenuImageHolder();                     /* 0x001C1AB8 (empty) */
    void handOver(gt2k_menu::Object *owner); /* 0x001C1AE0 a heap copy is freed with owner */

    /* Typed (u067 matching): the constructor loads tex->data (void *) before
     * it stores data, which egcs lets happen only when the two have different
     * alias sets (a void * field kept them in source order). */
    Tex1Image *data;                        /* 0x00 the texture data (TEX1) */
    int   unpacked;                         /* 0x04 1: data is a menu-heap copy */
    int   handedOver;                       /* 0x08 set by handOver */
};
EGCS_SIZE(MenuImageHolder, 0xC);

#endif /* GAME_T226_MENU_IMAGE_HOLDER_H */
