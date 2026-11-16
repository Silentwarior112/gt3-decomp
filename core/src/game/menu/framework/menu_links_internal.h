/* menu_links_internal.h - unit u069 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001C5228..0x001C7FE0, second pass.
 *
 * Translation units (link order):
 *   T235  gt2k_menu::ColorBox: a column of colour swatches (the body colour
 *         choice) with a blinking cursor, moved with up / down
 *   T236  gt2k_menu::FreeAgent: an Object that owns a menu heap block
 *   T237  gt2k_menu::GearChart: the speed / rpm chart of the gear ratios
 *   T238  gt2k_menu::Slider: a Button with a "pinch" knob on a value range
 *         (SliderDesc), left / right (L1 / R1: x10) change the value
 *   T239  gt2k_menu::CallbackText: an Item whose text a callback writes
 *   T240  gt2k_menu::PageHistory: a ring of 32 page names (the "back" stack)
 *   T241  gt2k_menu::LinkManager (link_manager.cxx): the page records of a
 *         loaded .mbl file, the import page, the .pmb layout file names
 *   T242  relocation of a loaded .mbl file (offsets -> pointers)
 *   T243  relocation of a loaded .pmb layout file (pages, layers, items,
 *         textures)
 *
 * Every class is defined in its canonical header (include/game/T235..T241);
 * this header adds what only this unit uses: the .mbl header LinkFile, the
 * T241 searches, the T242 / T243 relocation steps, the typeinfo storage and
 * base list of the unit's own classes (GCC 14).
 *
 * Data: each source includes the canonical headers it needs itself, in the
 * order that gives retail's type names (each file's comment lists them), and
 * this header last.  The headers included here bring no type name and no
 * literal (tools/hdrnames.py), so they change no file's data.
 */
#ifndef U069_H
#define U069_H

#include "types.h"
#include "libc.h"                           /* strcpy, strcmp, strlen, free */
#include <stddef.h>
#include "game/menu/framework/T229_page_layout.h"          /* MenuItemType, MenuText, MenuLayoutItem, MenuLayer,
                                               MenuPage, MenuLayoutData (T243) */
#include "game/menu/framework/T241_link_manager.h"         /* gt2k_menu::LinkPage */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __rtti_si, __rtti_user,
                                               __class_type_info::base_info */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The menu sound effects these classes play: T233's MENU_SE_CURSOR / _REFUSED /
 * _VALUE. */

/* MenuLayoutItem::flags bits the T243 relocation tests: T229's ITEM_*. */

namespace gt2k_menu {

/* T235 / T240's moveIndex (idx->move(delta) through the vtable): in the two
 * sources (it needs SmartIndexT<int>, whose type name only they have). */

/* ======================================================================== */
/* T235 / T237 / T239 - the page hook factories                             */
/* ======================================================================== */
/* The three create functions (create_color_box, create_gear_chart,
 * create_callback_text) are declared in T235 / T237 / T239 with MenuHookFn's
 * signature, as are their p8 types ColorBoxGetFn / GearChartCarFn. */

/* ======================================================================== */
/* T241 / T242 - the link file (.mbl)                                       */
/* ======================================================================== */
/* The header of a loaded .mbl file (forward-declared in T241): the pages,
 * sorted by name (binary search, signed characters). */
struct LinkFile {
    int        count;                       /* 0x00 */
    LinkPage **pages;                       /* 0x04 */
};
EGCS_SIZE(gt2k_menu::LinkFile, 0x8);

/* T241, no callers outside it. */
ButtonData *find_button(ButtonData **list, int count, const char *name); /* 0x001C78B8 depth first,
                                               the sub lists too; NULL if none */
LinkPage  **bsearch_page(LinkPage **pages, int count, const char *name); /* 0x001C7988 NULL if absent */

/* T242 / T243: a pointer field of the file becomes base + its offset,
 * through `off', one `long' local per relocation function (as u099's
 * GCF_REL for the .gcf file).  Retail loads the offset into one register
 * and sign-extends it (dsll32 / dsra32, the DImode `off') into another
 * before the 32-bit add, except where CSE still has the plain load (a
 * function's first relocation): `off' is set more than once, so it is not
 * local to a block and global-alloc gives it its own register, as retail
 * (u069 revisit, 2026-10-10: relocate_item / _layer / _page / _archive
 * exact; an inline helper with a `long' parameter, a fresh pseudo per
 * call, tied the two and stayed at 95.0..99.1%). */
#define RELOC(base, off, field) \
    ((off) = (long)(int)(field), (field) = (__typeof__(field))((u8 *)(base) + (off)))

/* T242: every pointer of the file is an offset from its start. */
void relocate_button(void *base, ButtonData *b);    /* 0x001C7A20 the sub list, recursively */
void relocate_link_page(void *base, LinkPage *p);   /* 0x001C7AA8 */
void relocate_link_file(void *base, LinkFile *f);   /* 0x001C7B30 (LinkManager::load) */

/* ======================================================================== */
/* T243 - the layout file (.pmb); relocate_archive is declared in T229      */
/* ======================================================================== */
void relocate_item_type(void *base, MenuItemType *t);  /* 0x001C7BB8 its name */
void relocate_text(void *base, MenuText *t);           /* 0x001C7BC8 */
void relocate_item(void *base, MenuLayoutItem *item);  /* 0x001C7BF0 the active item recursively */
void relocate_layer(void *base, MenuLayer *l);         /* 0x001C7CE0 */
void relocate_page(void *base, MenuPage *p);           /* 0x001C7DC8 */

} /* namespace gt2k_menu */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (.bss) of the classes only this unit builds:
 * __si_type_info = 3 words, __class_type_info = 4. */
extern "C" {
extern u32 __tiQ29gt2k_menu8ColorBox[3];        /* 0x004B2600 */
extern u32 __tiQ29gt2k_menu9FreeAgent[3];       /* 0x004B2610 */
extern u32 __tiQ29gt2k_menu9GearChart[3];       /* 0x004B2620 */
extern u32 __tiQ29gt2k_menu6Slider[4];          /* 0x004B2630 */
extern u32 __tiQ29gt2k_menu12CallbackText[3];   /* 0x004B2640 */
}

/* The __class_type_info base list of Slider (.sdata): {Button, public +0}. */
extern const __class_type_info::base_info g_003526C0[1];   /* 0x003526C0 */
#endif /* !BOOT_OLD_GCC */

#endif /* U069_H */
