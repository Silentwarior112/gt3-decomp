/* button_list_internal.h - unit u070 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001C7FE0..0x001CB3F0, second pass.
 *
 * Translation units (link order):
 *   T244  gt2k_menu::ButtonList and its base List<Button>
 *         (include/game/menu/framework/T244_button_list.h).
 *   T245  CarRender: the 3D car of the menu screens
 *         (include/game/car/model/T245_car_render.h).
 *
 * Every class is defined in its canonical header (include/game/); each source
 * includes the canonical headers it needs itself, in the order that gives the
 * original's type names (u070 data pass).  This header has only what the
 * unit's own files declare for each other: the jitter table of
 * CarRender::draw (defined in T245) and the RTTI storage of T244's typeinfo
 * functions (GCC 14).
 */
#ifndef U070_H
#define U070_H

#include "types.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __rtti_si, __rtti_user,
                                               __class_type_info::base_info */
#endif

/* ======================================================================== */
/* T244 constants (only used here)                                          */
/* ======================================================================== */

/* gt2k_menu::ButtonData::flags: T244's BUTTON_*. */

/* menu_play_se numbers used here: T233's MENU_SE_REFUSED / MENU_SE_LIST_MOVE (the
 * pages' own cursors use MENU_SE_CURSOR). */

/* ======================================================================== */
/* T245 - the jitter table                                                  */
/* ======================================================================== */

/* 0x002CCFC0 (T245 .data, defined there): per pass count n (1..5) a table of
 * n sub-pixel offsets {x, y} for the jittered passes of CarRender::draw
 * (entry 0 is NULL).  Writable data in the original (.data, not .rodata). */
extern const f32 *g_002CCFC0[6];

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; this is for GCC 14 only.
 * 0x00336CA8: the __class_type_info base list of ButtonList (.rodata), both
 * public: {Drawable 0x004B26F0, +0}, {List<Button> 0x004B14E0, +0xB0}. */
extern const __class_type_info::base_info g_00336CA8[2];
#endif /* !BOOT_OLD_GCC */

#endif /* U070_H */
