/* T225 - MenuItemArray (? name).  0x001C1520..0x001C1A18.
 *
 * The per-entry data of a menu list built at run time (T044
 * create_menu_garage / create_menu_garage_trade / create_menu_wheel, T091
 * create_menu_records, T209 create_name_button_list): one zeroed block on
 * the menu heap cut into arrays, count entries each.  Entry i is a copy of
 * a button record, its own label (0x80 chars), and copies of a layout item
 * and of its active item whose texts point at the label.  The block is
 * freed by a FreeAgent hung under the list. */
/* Retail's T225 has no .rodata: it saw no polymorphic class (the button
 * record from T244_button_data.h, not the button list). */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T229_page_layout.h"          /* MenuLayoutItem, MenuText */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData */
#include "game/menu/framework/T225_menu_item_array.h"
#include "game/menu/framework/menu_widgets_internal.h"

using namespace gt2k_menu;

/* The bytes of one entry: the record pointer, the record, two layout
 * items, two texts and the label. */
enum {
    ENTRY_SIZE = sizeof(ButtonData *) + 0x14C + 2 * 0x6C + 2 * 0x10 + 0x80   /* 0x2C8 */
};

/* The arrays are cut with a running pointer (retail keeps one register for
 * the chain of sums), and buffer is stored last. */
// FUNC 0x001C1520 MenuItemArray::MenuItemArray
MenuItemArray::MenuItemArray(int entries)
{
    u32 size = entries * ENTRY_SIZE;
    count = entries;
    if (size == 0)
        size = 0x10;
    u8 *block = (u8 *)menu_heap_current()->allocate(size, 0x10);
    for (u32 i = 0; i < size; i++)
        block[i] = 0;
    u8 *p = block;
    items = (ButtonData **)p;     p += count * 4;
    records = (ButtonData *)p;    p += count * 0x14C;
    rects = (MenuLayoutItem *)p;  p += count * 0x6C;
    subRects = (MenuLayoutItem *)p; p += count * 0x6C;
    texts = (MenuText *)p;        p += count * 0x10;
    subTexts = (MenuText *)p;     p += count * 0x10;
    names = (char (*)[0x80])p;
    buffer = block;
}

/* Entry i: records[i] = *tmpl, label = name ("(null)" if NULL), rects[i] =
 * *item; if the item has a text (flag 4) its copy points at the label; if
 * it has an active item (flag 8) that is copied the same way into
 * subRects[i] / subTexts[i] and rects[i].active points at the copy.  (The
 * record and item copies are inline word / doubleword loops, aligned or
 * not.) */
// FUNC 0x001C15F0 MenuItemArray::setEntry
void MenuItemArray::setEntry(int i, const ButtonData *tmpl, const MenuLayoutItem *item,
                             const char *name)
{
    records[i] = *tmpl;
    items[i] = &records[i];
    char *label = names[i];
    if (name != 0)
        strcpy(label, name);
    else
        strcpy(label, "(null)");    /* inlined 7-byte copy */

    rects[i] = *item;
    if (item->flags & 4) {          /* has a text */
        texts[i] = *item->text;
        rects[i].text = &texts[i];
        texts[i].text = label;
    }
    if (item->flags & 8) {          /* has an active item */
        const MenuLayoutItem *active = item->active;
        subRects[i] = *active;
        if (active->flags & 4) {
            subTexts[i] = *active->text;
            subRects[i].text = &subTexts[i];
            subTexts[i].text = label;
        }
        rects[i].active = &subRects[i];
    }
}
