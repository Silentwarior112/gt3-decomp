/* raw_list_internal.h - unit u074 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001D3F70..0x001D5D60, second pass.
 *
 * Translation units (link order):
 *   T263  the list windows of the menu system: gt2k_menu::RawListItem (a
 *         window that can be an item of a list), RawListItemFixed, RawList
 *         (a window holding a linked list of RawListItems with a focus item,
 *         cursor navigation, automatic layout and scrolling) and its
 *         variants RawListFixed, RawListH / HFixed / HPos / HSize, RawListV /
 *         VFixed / VPos / VSize
 *   T274  gt2k_menu::RawScrollBar (constructor, destructor, typeinfo only)
 *
 * Every class is defined in its canonical header (include/game/T263, T274);
 * the rest of each class's boilerplate (className = slot 8, the layout
 * factory, create, canCreate) is in T272 (u073).  This header adds the
 * outside headers the sources need and what only this unit uses: named
 * constants, the interrupt helper and the typeinfo storage of the classes
 * only this unit builds.
 *
 * Names: the accessors at 0x001D5A70..0x001D5AE0 and 0x001D5B88..0x001D5BB8
 * are inline members emitted out of line.  index.txt names some of them from
 * the Lupo 2 map, which is one function off there (Lupo 2 emits these
 * accessors in the reverse order); the canonical header and the sources use
 * the names that fit the field each one touches, and each definition notes
 * the index.txt name.
 */
#ifndef U074_H
#define U074_H

#include "types.h"
#include <stddef.h>

/* own translation units */
#include "game/menu/framework/T263_raw_list.h"
#include "game/menu/framework/T274_raw_scroll_bar.h"

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"         /* intr_suspend (0x00269B00), intr_resume */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator delete, the tree links) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindow, RawWindowArgs; its typeinfo */
#include "game/menu/framework/T270_raw_animation.h"        /* gt2k_menu::RawAnimation (update, virtual_5 = the window) */
#include "game/menu/framework/T271_raw_anim_scroll.h"      /* gt2k_menu::RawAnimScroll::scrollTo */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* RAW_EVENT_* (the RawWindow events): game/menu/framework/T264_raw_window.h. */

/* RawListItem::moveDir (set on the list): the direction of the last focus
 * move, which scrollToFocus uses to keep room before / after the item.  A
 * wrap-around counts as a move the other way (first -> last is NEXT). */
enum {
    LIST_MOVE_PREV = -1,
    LIST_MOVE_NONE = 0,
    LIST_MOVE_NEXT = 1
};

/* menu_play_se's sound effects: T233's MENU_SE_CURSOR / MENU_SE_REFUSED. */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __si_type_info storage (3 words, .bss) of the classes only this unit
 * builds.  RawList's, RawListItemFixed's and RawListItem's are in
 * T263_raw_list.h, RawWindow's in T264_raw_window.h. */
extern "C" {
extern u32 __tiQ29gt2k_menu11RawListVPos[3];    /* 0x004B2740 */
extern u32 __tiQ29gt2k_menu8RawListH[3];        /* 0x004B2760 */
extern u32 __tiQ29gt2k_menu11RawListHPos[3];    /* 0x004B2770 */
extern u32 __tiQ29gt2k_menu8RawListV[3];        /* 0x004B2780 */
extern u32 __tiQ29gt2k_menu12RawListHSize[3];   /* 0x004B2790 */
extern u32 __tiQ29gt2k_menu13RawListVFixed[3];  /* 0x004B27A0 */
extern u32 __tiQ29gt2k_menu13RawListHFixed[3];  /* 0x004B27C0 */
extern u32 __tiQ29gt2k_menu12RawListFixed[3];   /* 0x004B27D0 */
extern u32 __tiQ29gt2k_menu12RawListVSize[3];   /* 0x004B27E0 */
extern u32 __tiQ29gt2k_menu12RawScrollBar[3];   /* 0x004B2800 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U074_H */
