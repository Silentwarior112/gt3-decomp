/* menu_buttons_internal.h - unit u063 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001B2C20..0x001B6250, second pass.
 *
 * Translation units (link order):
 *   T203  two sorted {name, id} tables of car part names (built by a static
 *         initialiser) and their binary-search lookups
 *   T204  the menu heap stack: four pdistd::Allocator objects, push / pop
 *   T205  the "blink" page hook (an Item running a blink script)
 *   T206  gt2k_menu::Button (the button of a ButtonList) and Cell<Button>'s
 *         destructor instance
 *   T207  gt2k_menu::MapButtonList / MapButton (a free pointer moved over a
 *         map of buttons, with a zoom), MagScrBegin / MagScrEnd (the zoom
 *         transform pushed around the map)
 *   T208  gt2k_menu::WheelShopButtonList / WheelShopButton (the wheel shop)
 *   T209  a factory building a ButtonList from a name callback
 *   T210  gt2k_menu::Arrow (a scroll arrow shown while the list can move)
 *
 * Every class is defined in its canonical header (include/game/T203..T210).
 * Each source includes the canonical headers it needs itself, in the order
 * that gives the original's type names (data pass: the type names in an
 * object's .rodata are those of the classes the file saw, in reverse
 * definition order; each source's comment lists retail's).  This header
 * adds only what no class header has and defines no class egcs writes a
 * type name for: T203's {name, id} record and its search, and (GCC 14
 * only) the typeinfo storage and base lists of the unit's own classes.
 */
#ifndef U063_H
#define U063_H

#include "types.h"
#include "game/game.h"
#include <stddef.h>

/* ======================================================================== */
/* T203 - part name tables (only used here)                                 */
/* ======================================================================== */
/* One entry of the T203 tables: a part (or "set_*" setting) name and its
 * part id.  The tables are sorted by name (strcmp order) for the binary
 * search; the static initialiser builds them at start-up through the inline
 * constructor (T203's tables are static arrays of these). */
struct PartName {
    PartName(const char *n, int i) : name(n), id(i) {}
    const char *name;                       /* 0x0 */
    int         id;                         /* 0x4 */
};
EGCS_SIZE(PartName, 8);

enum {
    PART_NAME_COUNT    = 0x35,              /* g_0049C638 */
    SETTING_NAME_COUNT = 0x49,              /* g_0049C3F0 */
    PART_ID_UNKNOWN    = 0x58               /* what the lookups return for an unknown name */
};

/* Binary search by name; the entry, NULL if absent. */
PartName *find_part_name(PartName *table, int count, const char *name);    /* 0x001B2C90 */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __rtti_si, __class_type_info::base_info */

/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (.bss) of the classes only this unit builds:
 * __class_type_info = 4 words, __si_type_info = 3. */
extern "C" {
extern u32 __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button[3];  /* 0x004B14C0 */
extern u32 __tiQ29gt2k_menu13MapButtonList[4];              /* 0x004B2460 */
extern u32 __tiQ29gt2k_menu11MagScrBegin[3];                /* 0x004B2470 */
extern u32 __tiQ29gt2k_menu9MapButton[4];                   /* 0x004B2480 */
extern u32 __tiQ29gt2k_menu9MagScrEnd[3];                   /* 0x004B2490 */
extern u32 __tiQ29gt2k_menu19WheelShopButtonList[4];        /* 0x004B24A0 */
extern u32 __tiQ29gt2k_menu15WheelShopButton[4];            /* 0x004B24B0 */
extern u32 __tiQ29gt2k_menu5Arrow[3];                       /* 0x004B24C0 */
}

/* The __class_type_info base lists (.rodata / .sdata), all public.  egcs
 * writes them itself (anonymous labels, after the class's type name). */
extern const __class_type_info::base_info g_00332C78[2];   /* 0x00332C78 Button: {Item, +0},
                                                               {Cell<Button>, +0x100} */
extern const __class_type_info::base_info g_00352520[1];   /* 0x00352520 MapButtonList: {ButtonList, +0} */
extern const __class_type_info::base_info g_00352528[1];   /* 0x00352528 MapButton: {Button, +0} */
extern const __class_type_info::base_info g_00352530[1];   /* 0x00352530 WheelShopButtonList: {ButtonList, +0} */
extern const __class_type_info::base_info g_00352538[1];   /* 0x00352538 WheelShopButton: {Button, +0} */
#endif /* !BOOT_OLD_GCC */

#endif /* U063_H */
