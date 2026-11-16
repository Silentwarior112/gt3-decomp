/* T213_menu_texture.h - gt2k_menu::MenuTexture, MenuUV: the texture records
 * of a layout file (plain records; split from T213_menu_item.h, which
 * includes this header where they were).
 *
 * Their own header so that a file that needs the records but not Item (T243,
 * the .pmb relocation) does not read Item's header: that one brings the
 * operator list classes, whose type names egcs writes into every includer,
 * and retail's T243 has none (u069's data pass). */
#ifndef GAME_T213_MENU_TEXTURE_H
#define GAME_T213_MENU_TEXTURE_H

#include "game/game.h"

namespace gt2k_menu {

/* A texture entry of a layout file (MenuLayoutData::textures, T229), 8
 * bytes; PageLayout::getTexture returns &textures[item->uv->texture]. */
struct MenuTexture {
    u32   size;                             /* 0x0 the unpacked size */
    void *data;                             /* 0x4 TEX1 image, gzipped when the item flag 0x40000 is set */
};

/* The texture record of a layout item (MenuLayoutItem::uv): the texture's
 * index, then the UV extents (? three corners: u0 / v0 top left, u1 at +0x0C
 * and v1 at +0x18).  setUVExtents makes an Image of type 2 (TILED) when the
 * extents are not the whole texture.  Its size is not known. */
struct MenuUV {
    int texture;                            /* 0x00 index into the file's textures (T229 getTexture) */
    f32 u0;                                 /* 0x04 */
    f32 v0;                                 /* 0x08 */
    f32 u1;                                 /* 0x0C */
    f32 unk10;                              /* 0x10 */
    f32 unk14;                              /* 0x14 */
    f32 v1;                                 /* 0x18 */
};

} /* namespace gt2k_menu */

#endif /* GAME_T213_MENU_TEXTURE_H */
