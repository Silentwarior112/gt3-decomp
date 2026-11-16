/* T242 - relocation of a loaded .mbl file (0x001C7A20..0x001C7BB8): every
 * pointer of the file is stored as an offset from its start; base + offset
 * gives the pointer (LinkManager::load).  RELOC: menu_links_internal.h.
 *
 * Data: none in retail but the frame info (no .rodata, no type name). */
#include "game/menu/framework/T241_link_manager.h"         /* gt2k_menu::LinkPage */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData */
#include "game/menu/framework/menu_links_internal.h"                           /* LinkFile, RELOC */

namespace gt2k_menu {

/* Recursive: the sub list. */
// FUNC 0x001C7A20 gt2k_menu::relocate_button
void relocate_button(void *base, ButtonData *b)
{
    long off;

    RELOC(base, off, b->subItems);
    for (int i = 0; i < b->subCount; i++) {
        RELOC(base, off, b->subItems[i]);
        relocate_button(base, b->subItems[i]);
    }
}

// FUNC 0x001C7AA8 gt2k_menu::relocate_link_page
void relocate_link_page(void *base, LinkPage *p)
{
    long off;

    RELOC(base, off, p->items);
    for (int i = 0; i < p->count; i++) {
        RELOC(base, off, p->items[i]);
        relocate_button(base, p->items[i]);
    }
}

/* Caller: LinkManager::load (base == f). */
// FUNC 0x001C7B30 gt2k_menu::relocate_link_file
void relocate_link_file(void *base, LinkFile *f)
{
    long off;

    RELOC(base, off, f->pages);
    for (int i = 0; i < f->count; i++) {
        RELOC(base, off, f->pages[i]);
        relocate_link_page(base, f->pages[i]);
    }
}

} /* namespace gt2k_menu */
