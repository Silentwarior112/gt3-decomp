/* T225_menu_item_array.h - MenuItemArray (translation unit T225, unit u067).
 *
 * (? name) The per-entry data of a menu list built at run time (T044, T091,
 * T209): one zeroed block on the menu heap cut into arrays of `count'
 * entries.  Entry i is a copy of a button record, its own label (0x80
 * chars), and copies of a layout item and of its active item whose texts
 * point at the label.  The block is freed by a FreeAgent (T236) hung under
 * the list.  Not polymorphic, no destructor. */
#ifndef GAME_T225_MENU_ITEM_ARRAY_H
#define GAME_T225_MENU_ITEM_ARRAY_H

#include "game/game.h"

namespace gt2k_menu {
struct ButtonData;                          /* T244 (pointers only): the 0x14C-byte button record */
struct MenuLayoutItem;                      /* T229 */
struct MenuText;                            /* T229 */
}

/* Size 0x24.  The block: count * 0x2C8 bytes (0x10 if count is 0). */
struct MenuItemArray {
    MenuItemArray(int count);               /* 0x001C1520 */
    void setEntry(int i, const gt2k_menu::ButtonData *tmpl, const gt2k_menu::MenuLayoutItem *item,
                  const char *name);        /* 0x001C15F0 records[i] = *tmpl, the label = name
                                               ("(null)"), rects[i] = *item (and its active item,
                                               flag 8, into subRects[i]); texts on the label */
    /* Inline accessors (? names).  u009's T044 factories read the three
     * through them: create_menu_wheel (0x0011A700) keeps &entries in a
     * register for them as retail does, and an inline call among the
     * arguments makes egcs evaluate items before the other arguments of the
     * list's constructor (the garage lists, as retail).  T091 and T209 read
     * the fields directly. */
    gt2k_menu::ButtonData **getItems() { return items; }
    gt2k_menu::MenuLayoutItem *getRects() { return rects; }
    void *getBuffer() { return buffer; }

    gt2k_menu::ButtonData     **items;      /* 0x00 count: entry i -> &records[i] */
    gt2k_menu::ButtonData      *records;    /* 0x04 count * 0x14C */
    gt2k_menu::MenuLayoutItem  *rects;      /* 0x08 count * 0x6C */
    gt2k_menu::MenuLayoutItem  *subRects;   /* 0x0C count * 0x6C the active items */
    gt2k_menu::MenuText        *texts;      /* 0x10 count * 0x10 */
    gt2k_menu::MenuText        *subTexts;   /* 0x14 count * 0x10 */
    char                      (*names)[0x80]; /* 0x18 count * 0x80 the labels */
    void                       *buffer;     /* 0x1C the block (menu heap) */
    int                         count;      /* 0x20 */
};
EGCS_SIZE(MenuItemArray, 0x24);

#endif /* GAME_T225_MENU_ITEM_ARRAY_H */
