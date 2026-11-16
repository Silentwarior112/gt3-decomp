/* menu_draw_state_internal.h - unit u073 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001D1AE0..0x001D3F70, second pass.
 *
 * Translation units (link order):
 *   T261  the menu drawing state: the screen buffer being drawn, the clip
 *         rectangle (a 640 x 448 virtual screen, centre + size) turned into
 *         a GS scissor, the drawing offset / scale, a debug message, two
 *         640 x 224 GSBuffers (include/game/menu/framework/T261_menu_draw_state.h)
 *   T262  the menu BGM starters, only when the "menu BGM" option is on
 *         (T262_menu_bgm.h)
 *   T270  gt2k_menu::RawAnimation, an animation driving one RawWindow
 *         (T270_raw_animation.h)
 *   T271  gt2k_menu::RawAnimScroll, moves its window to a target in steps of
 *         a quarter of the distance (T271_raw_anim_scroll.h)
 *   T275  gt2k_menu::RawCheckBox, a list item with an on / off picture
 *         (T275_raw_check_box.h)
 *   T272  the class registry entries of the 16 Raw* window classes: the
 *         class name (slot 8), the layout factories, the static registrars
 *         (T272_raw_window_classes.h)
 *   T273  gt2k_menu::RawLabel (T273_raw_label.h)
 *
 * Every class and function is declared in its canonical header
 * (include/game/); this header adds what only this unit uses: the check
 * box's index values and the typeinfo storage of the unit's own classes.
 * The include set decides each object's .rodata (type names): T261 and T262
 * include their own headers instead of this one (retail's T261 has no
 * .rodata; T262's is only the game status's garage-list names, without the
 * menu operator classes this header brings).  T270, T271, T275, T272, T273
 * include this header: retail's names of those five are exactly what it
 * brings (the operator classes), plus each file's own class.
 */
#ifndef U073_H
#define U073_H

#include "types.h"
#include <stddef.h>
#include <stdarg.h>

/* own translation units */
#include "game/menu/framework/T261_menu_draw_state.h"
#include "game/menu/framework/T262_menu_bgm.h"
#include "game/menu/framework/T270_raw_animation.h"
#include "game/menu/framework/T271_raw_anim_scroll.h"
#include "game/menu/framework/T272_raw_window_classes.h"
#include "game/menu/framework/T273_raw_label.h"
#include "game/menu/framework/T275_raw_check_box.h"

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, gfx_set_scissor, gs_set_clear_color,
                                               gs_clear_buffers (PGL_CLEAR_COLOR), pgl_translate,
                                               vu0_scale */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (decide) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete, the base of
                                               RawAnimation) */
#include "game/menu/framework/T263_raw_list.h"             /* gt2k_menu::RawListItem(Fixed), RawList and its variants */
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindow, RawWindowArgs */
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass, RawWindowRegister */
#include "game/menu/framework/T274_raw_scroll_bar.h"       /* gt2k_menu::RawScrollBar */
/* T262_menu_bgm.cpp alone includes T429_game_status.h and T464_bgm.h (their
 * type names are only in T262's .rodata); it does not include this header. */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (width, psm, displayHeight, allocVram, freeVram) */
#include "engine/pdistd/T535_vsprintf.h"             /* vsprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* ======================================================================== */
/* T275 - RawCheckBox                                                       */
/* ======================================================================== */
/* The check box's images[2][2] index values. */
enum { CHECK_OFF = 0, CHECK_ON = 1 };

/* ======================================================================== */
/* RTTI: the typeinfo storage of this unit's own classes                    */
/* ======================================================================== */
/* egcs generates the __tf functions and their nodes itself; the C
 * equivalents in this unit and these declarations are for the modern
 * compiler only.  __si_type_info nodes (3 words, .bss), only used here. */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu12RawAnimation[3];   /* 0x004B2700 */
extern u32 __tiQ29gt2k_menu13RawAnimScroll[3];  /* 0x004B2710 */
extern u32 __tiQ29gt2k_menu11RawCheckBox[3];    /* 0x004B2720 */
extern u32 __tiQ29gt2k_menu8RawLabel[3];        /* 0x004B2730 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U073_H */
