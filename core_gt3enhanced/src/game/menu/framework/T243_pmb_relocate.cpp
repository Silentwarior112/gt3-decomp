/* T243 - relocation of a loaded .pmb layout file (0x001C7BB8..0x001C7FE0):
 * pages -> their items and layers -> the layers' items and object items
 * (and each item's type, active item, texture record, vertex colours and
 * text), then the textures and their TEX1 images.  Every pointer is stored
 * as an offset from the file start (PageLayout::readFile, T229).  The
 * records are T229's: MenuLayoutData, MenuPage, MenuLayer, MenuLayoutItem
 * (u069's first pass called them MenuArchiveFile, MenuLayer, MenuGroup,
 * MenuResource).  RELOC: menu_links_internal.h.
 *
 * Data: none in retail but the frame info (no .rodata, no type name). */
#include "game/menu/framework/T229_page_layout.h"          /* the records, ITEM_*, relocate_archive */
#include "game/menu/framework/T213_menu_texture.h"         /* gt2k_menu::MenuTexture (not Item's header: retail's
                                               T243 has no type name) */
#include "engine/pgl/G001_flare.h"                /* tex1_relocate, Tex1Image */
#include "game/menu/framework/menu_links_internal.h"                           /* RELOC */

namespace gt2k_menu {

/* The type record of an object item: its name (the hook's). */
// FUNC 0x001C7BB8 gt2k_menu::relocate_item_type
void relocate_item_type(void *base, MenuItemType *t)
{
    long off;

    RELOC(base, off, t->name);
}

// FUNC 0x001C7BC8 gt2k_menu::relocate_text
void relocate_text(void *base, MenuText *t)
{
    long off;

    RELOC(base, off, t->text);
    RELOC(base, off, t->font);
}

/* By the item's flags: the type record (flags & 0x30000 == 0x10000), the
 * active item (recursively), the texture record, the vertex colours, the
 * text.  Exact (and relocate_layer / _page / _archive) with RELOC's one
 * `off' local (menu_links_internal.h): retail keeps the flags in $a2 and loads the text's
 * offset into $v0; in the others it loads a list's offset into $a1 and
 * sign-extends into $v0.  An inline helper with a `long' parameter tied
 * the two (95.0 / 99.1 / 99.1 / 97.7%). */
// FUNC 0x001C7BF0 gt2k_menu::relocate_item
void relocate_item(void *base, MenuLayoutItem *item)
{
    long off;

    if ((item->flags & ITEM_TYPE_MASK) == ITEM_HAS_TYPE) {
        RELOC(base, off, item->type);
        relocate_item_type(base, item->type);
    }
    if (item->flags & ITEM_HAS_ACTIVE) {
        RELOC(base, off, item->active);
        relocate_item(base, item->active);
    }
    if (item->flags & ITEM_HAS_TEXTURE)
        RELOC(base, off, item->uv);
    if (item->flags & ITEM_HAS_COLORS)
        RELOC(base, off, item->colors);
    if (item->flags & ITEM_HAS_TEXT) {
        RELOC(base, off, item->text);
        relocate_text(base, item->text);
    }
}

/* A layer: its plain items, then its object items. */
// FUNC 0x001C7CE0 gt2k_menu::relocate_layer
void relocate_layer(void *base, MenuLayer *l)
{
    long off;

    RELOC(base, off, l->items);
    for (int i = 0; i < l->itemCount; i++) {
        RELOC(base, off, l->items[i]);
        relocate_item(base, l->items[i]);
    }
    RELOC(base, off, l->objects);
    for (int i = 0; i < l->objectCount; i++) {
        RELOC(base, off, l->objects[i]);
        relocate_item(base, l->objects[i]);
    }
}

/* A page: its own items, then its layers. */
// FUNC 0x001C7DC8 gt2k_menu::relocate_page
void relocate_page(void *base, MenuPage *p)
{
    long off;

    RELOC(base, off, p->items);
    for (int i = 0; i < p->itemCount; i++) {
        RELOC(base, off, p->items[i]);
        relocate_item(base, p->items[i]);
    }
    RELOC(base, off, p->layers);
    for (int i = 0; i < p->layerCount; i++) {
        RELOC(base, off, p->layers[i]);
        relocate_layer(base, p->layers[i]);
    }
}

/* The pages, then the textures: data relocated, and a "Tex1" image (magic
 * read byte by byte, little-endian 0x31786554) relocated inside by
 * tex1_relocate.  Caller PageLayout::readFile (0x001C2448). */
// FUNC 0x001C7EB0 gt2k_menu::relocate_archive
void relocate_archive(void *base, MenuLayoutData *data)
{
    long off;

    RELOC(base, off, data->pages);
    for (int i = 0; i < data->pageCount; i++) {
        RELOC(base, off, data->pages[i]);
        relocate_page(base, data->pages[i]);
    }
    RELOC(base, off, data->textures);
    for (int i = 0; i < data->textureCount; i++) {
        RELOC(base, off, data->textures[i].data);
        const u8 *image = (const u8 *)data->textures[i].data;
        u32 magic = image[0] | (image[1] << 8) | (image[2] << 16) | (image[3] << 24);
        if (magic == 0x31786554)            /* "Tex1" */
            tex1_relocate((Tex1Image *)data->textures[i].data);
    }
}

} /* namespace gt2k_menu */
