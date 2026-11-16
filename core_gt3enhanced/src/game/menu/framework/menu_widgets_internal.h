/* menu_widgets_internal.h - unit u067 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001BF390..0x001C2318, second pass.
 *
 * Translation units (link order):
 *   T220  gt2k_menu::Object (the node of the menu object tree: links, the
 *         class allocator on the menu heap, the dying states) and
 *         gt2k_menu::RootObject (the root of a tree: updates, draws (also
 *         z-sorted), deletes, counts the nodes under it)
 *   T221  gt2k_menu::CallbackItem (shows one of N items, the index given by
 *         a callback)
 *   T222  gt2k_menu::DecideEffect (the flash over a decided button)
 *   T223  gt2k_menu::Pointer (the map pointer: eases to a target, drawn
 *         through a zoomed view)
 *   T224  gt2k_menu::WheelButtonList / WheelButton (the wheel list of the
 *         garage: a "mark" on the fitted wheel, the focused wheel shown on
 *         the 3D car)
 *   T225  MenuItemArray (the per-entry data of a list built at run time)
 *   T226  MenuImageHolder (a layout item's texture, unpacked when gzipped)
 *   T227  run_text_input (the text input page loop)
 *   T267  gt2k_menu::ConcurrentButton (a button carrying a sub ButtonList)
 *   T228  gt2k_menu::SliderItem (a slider bar with a "pinch" knob)
 *
 * Every class is defined in its canonical header (include/game/T220..T228,
 * T267).  No class headers here: each source includes the headers it uses
 * itself, in the order that gives retail's .rodata (egcs writes the type
 * names of the classes a file sees in the reverse order of their
 * definition; each source's comment lists retail's names).  This header
 * holds only what GCC 14 needs besides: the typeinfo storage and base lists
 * of the unit's own classes (egcs generates them itself).  T222 defines the
 * decide effect's script (internal linkage, so no declaration here).
 */
#ifndef U067_H
#define U067_H

#include "types.h"
#include <stddef.h>

/* The menus' virtual screen: T259's MENU_SCREEN_* (640 x 448, its centre
 * (320, 224)). */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __rtti_si, __rtti_user,
                                               __class_type_info::base_info */
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (.bss) of the classes only this unit builds:
 * __class_type_info = 4 words, __si_type_info = 3.  (Object's
 * __user_type_info 0x003533F0 is in T220_object.h, Transitive's / Item's /
 * Button's in T216 / T213 / T206.) */
extern "C" {
extern u32 __tiQ29gt2k_menu10RootObject[3];             /* 0x004B2530 */
extern u32 __tiQ29gt2k_menu12CallbackItem[3];           /* 0x004B2540 */
extern u32 __tiQ29gt2k_menu12DecideEffect[3];           /* 0x004B2550 */
extern u32 __tiQ29gt2k_menu7Pointer[3];                 /* 0x004B2560 */
extern u32 __tiQ29gt2k_menu15WheelButtonList[4];        /* 0x004B2570 */
extern u32 __tiQ29gt2k_menu11WheelButton[4];            /* 0x004B2580 */
extern u32 __tiQ29gt2k_menu16ConcurrentButton[4];       /* 0x004B2590 */
extern u32 __tiQ29gt2k_menu10SliderItem[3];             /* 0x004B25A0 */
}

/* The __class_type_info base lists (.sdata), all {base, public, +0}. */
extern const __class_type_info::base_info g_003525F0[1];   /* 0x003525F0 WheelButtonList: {ButtonList} */
extern const __class_type_info::base_info g_003525F8[1];   /* 0x003525F8 WheelButton: {Button} */
extern const __class_type_info::base_info g_00352608[1];   /* 0x00352608 ConcurrentButton: {Button} */
#endif /* !BOOT_OLD_GCC */

#endif /* U067_H */
