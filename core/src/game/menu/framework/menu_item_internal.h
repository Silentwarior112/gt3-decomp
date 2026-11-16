/* menu_item_internal.h - unit u065 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001B9318..0x001BBC58, second pass.
 *
 * Translation units (link order):
 *   T213  gt2k_menu::Item: the image / text item of a menu page, built from a
 *         layout item (MenuLayoutItem) of a PageLayout (texture, vertex
 *         colours, text)
 *   T214  gt2k_menu::MenuControl: one pad's menu input (auto-repeat
 *         directions, diagonals, decide / cancel, shoulder buttons)
 *   T215  the showroom tables: 11 "menu/showroom_*.fbi" records
 *   T216  gt2k_menu::Transitive: a Drawable driven by operator scripts
 *         (OperatorHolder, the List / Cell<OperatorHolder> instances)
 *
 * Every class is defined in its canonical header (include/game/T213..T216
 * and the menu library's), which the sources include themselves; this
 * header holds what only this unit uses and needs no class: Item's text
 * flags, the raw pad constants MenuControl uses, the T215 helpers, the
 * functions of T216 nobody calls and the typeinfo storage / base list of
 * the classes only this unit builds (GCC 14).  The declarations that need
 * a class (T213's and T216's operator scripts, F_001BB758) are in the
 * sources that define them.
 *
 * No class headers here: egcs writes the type names of every class a unit
 * sees (with only inline virtual functions) into its .rodata, and retail's
 * T214 has only MenuControl's, T215 none (the operator list classes of
 * T213_menu_item.h / T216_menu_transitive.h are in T213's and T216's
 * only).
 */
#ifndef U065_H
#define U065_H

#include "types.h"
#include "libc.h"                           /* strlen, strcpy, memalign */
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user */
#include "eekernel.h"                       /* EI() */
#include <stddef.h>
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __class_type_info::base_info */
#endif

struct ShowroomData;                        /* T215_showroom_data.h */

/* ======================================================================== */
/* T213 - Item's data (.rodata / .sdata, only used here)                    */
/* ======================================================================== */
namespace gt2k_menu {

/* The menu's virtual screen: T259's MENU_SCREEN_*. */

/* MenuLayoutItem::flags: T229's ITEM_*. */

/* MenuText::flags. */
enum {
    TEXT_ANCHOR_MASK = 0x0F,                /* 0..8: 3 x 3, row by row from the top left */
    TEXT_MODE_MASK   = 0xF0,
    TEXT_SCROLL      = 0x10,                /* scrolls right to left inside the item */
    TEXT_TYPEWRITER  = 0x20                 /* one more character a frame */
};

/* The face of a text without one (MenuText::font NULL or ""), and the
 * fallback for an unknown one: T213 .sdata 0x00352570. */
#define ITEM_DEFAULT_FONT "sw_16"

/* Drawable::drawFlags: T259's DRAW_*; Rectangle::frameType: T256's FRAME_*. */

/* Font::setAlign: G001 font_set_align's FONT_ALIGN_* (G001_flare.h). */

/* T213's operator scripts g_00333D80 / g_00333E08 (.rodata) and the vertex
 * order g_00352568 (.sdata) are declared and defined in T213. */

/* ======================================================================== */
/* T216 - only used here (or not at all)                                    */
/* ======================================================================== */
/* T216's close script g_003341E8 (.rodata) and F_001BB758 (an operator
 * list lookup nobody calls) are declared in T216. */
/* No callers: forward to Operator::operator new / delete (0x001BD1A8 /
 * 0x001BD200).  (? OperatorHolder's operator new[] / delete[]: Object and
 * Operator have the same new / new[] / delete / delete[] quartet, new[] and
 * delete[] forwarding; but these forward to Operator's, not to
 * OperatorHolder's own operator new 0x001BB430 / delete 0x001BB4A8.) */
void *F_001BB488(size_t size);              /* 0x001BB488 */
void  F_001BB4D8(void *p);                  /* 0x001BB4D8 */

} /* namespace gt2k_menu */

/* T214 reads the raw PS pad bits (Control::curButtons.raw[0], PADRAW_*) and
 * the channels (CHAN_RSTICK_Y, CHAN_PRESS_*) T474 defines. */

/* MenuControl::dirMask: the eight directions updateKeys tells apart. */
enum {
    DIR_UP_LEFT    = 0x01,
    DIR_UP_RIGHT   = 0x02,
    DIR_DOWN_LEFT  = 0x04,
    DIR_DOWN_RIGHT = 0x08,
    DIR_UP         = 0x10,
    DIR_DOWN       = 0x20,
    DIR_LEFT       = 0x40,
    DIR_RIGHT      = 0x80
};

/* updateKeys: a direction counts once held for DIR_HOLD_FRAMES frames; the
 * auto-repeat (Control::getRepeat) starts after REPEAT_DELAY frames and
 * repeats every REPEAT_INTERVAL frames, every frame for a d-pad direction
 * pressed with PRESSURE_FULL or more; upPress .. rightPress: a pressure
 * above PRESSURE_FULL. */
enum {
    DIR_HOLD_FRAMES = 5,
    REPEAT_DELAY    = 30,
    REPEAT_INTERVAL = 3,
    PRESSURE_FULL   = 250
};

/* ======================================================================== */
/* T215 - the per-record accessors and the loader (only used here)          */
/* ======================================================================== */
enum { SHOWROOM_RECORDS = 11 };             /* the files, one record each */

int  showroom_record_flag(ShowroomData *rec, int bit);     /* 0x001BB1D8 flag `bit' of rec */
f32  showroom_record_value(ShowroomData *rec, int i);       /* 0x001BB1E8 value i of rec */
f32 *showroom_record_value_ptr(ShowroomData *rec, int i);   /* 0x001BB1F8 &value i of rec */
void showroom_read_files(ShowroomData *table);              /* 0x001BB2E0 reads the 11 files into table */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (.bss / .sbss) of the classes only this unit builds
 * (Item's and Transitive's are in T213 / T216): __si_type_info = 3 words,
 * __class_type_info = 4, __user_type_info = 2. */
extern "C" {
extern u32 __tiQ29gt2k_menu11MenuControl[3];                        /* 0x004B24F0 */
extern u32 __tiQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder[3]; /* 0x004B1320 */
extern u32 __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder[3]; /* 0x004B1220 */
extern u32 __tiQ29gt2k_menu14OperatorHolder[4];                     /* 0x004B1300 */
extern u32 __tiQ29gt2k_menu8Operator[2];                            /* 0x00353300 gp-0x46F0 */
}

/* The __class_type_info base list of OperatorHolder (.rodata), both public:
 * {Operator, +0}, {Cell<OperatorHolder>, +0x28}. */
extern const __class_type_info::base_info g_00334380[2];           /* 0x00334380 */
#endif /* !BOOT_OLD_GCC */

#endif /* U065_H */
