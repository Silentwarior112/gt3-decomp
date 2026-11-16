/* page_layout_internal.h - unit u068 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001C2318..0x001C5228, second pass.
 *
 * Translation units (link order):
 *   T229  gt2k_menu::PageLayout: a loaded menu layout file (pages, their
 *         layers of items, the textures), the element factories ("hooks")
 *         that build a page's objects, and the page slide transitions
 *         (include/game/menu/framework/T229_page_layout.h)
 *   T230  gt2k_menu::ScrollBar: the bar (and its "pinch") beside a ButtonList
 *         (T230_scroll_bar.h)
 *   T231  gt2k_menu::TextBox: the text field of the on-screen keyboard
 *         (characters kept as one int each, 2-byte codes for SJIS)
 *         (T231_text_box.h)
 *   T232  MenuCarSpec: the car on show in the menus (its spec record, its
 *         CarEquipments, its colours) (T232_menu_car_spec.h)
 *   T268  gt2k_menu::FreeButton: a button that opens a sub list in place
 *         (T268_free_button.h)
 *   T269  gt2k_menu::MultiButton: a button with a list of choices
 *         (T269_multi_button.h)
 *   T233  MenuSound: the menu music of a page; the menu sound effects
 *         (T233_menu_sound.h)
 *   T234  gt2k_menu::BoolItem: an item shown while a test function says so
 *         (T234_bool_item.h)
 *
 * Every class is defined in its canonical header (include/game/), which the
 * sources include themselves; this header holds what only this unit uses
 * and needs no class: named constants, the externs of the statics of T231 /
 * T232 / T268 (defined in those files), and the typeinfo storage and base
 * lists of the unit's own classes (GCC 14).
 *
 * No class headers here: egcs writes the type names of every class a unit
 * sees (with only inline virtual functions) into its .rodata, and retail's
 * files saw different sets (each source's comment lists them): T232 none,
 * T233 only the garage classes (the GameStatus), T231 / T234 no Button,
 * T229 / T230 / T268 / T269 Button's.
 */
#ifndef U068_H
#define U068_H

#include "types.h"
#include "libc.h"                           /* strcpy (core 0x0027EB84), strlen (0x00269A68), free
                                               (0x0026C380) */
#include <stddef.h>
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __rtti_si, __rtti_user,
                                               __class_type_info::base_info */
#endif

namespace gt2k_menu {
class TextBox;                              /* T231_text_box.h */
}
struct MenuCarSpec;                         /* T232_menu_car_spec.h */

/* ======================================================================== */
/* T229 - the page transitions (T229 .rodata, only used here)               */
/* ======================================================================== */
/* The page transitions (two OperatorCode instructions each: the scripts
 * page_slide_in 0x00335720, page_slide_out_down / _up / _right / _left
 * 0x00335798 / 0x003357F8 / 0x00335858 / 0x003358B8) are function-local
 * statics of openTransition / closeTransition: retail has each function's
 * between its neighbours' jump tables.  The inputMode of the PageLayout picks
 * the side (openTransition places the layer on the opposite side first). */

/* PageLayout::inputMode: the side the next layer slides in from (the menu
 * screen's y grows downwards); the old one leaves by the opposite side. */
enum {
    PAGE_SLIDE_NONE        = 0,             /* no slide (closeTransition: kill() at once) */
    PAGE_SLIDE_FROM_TOP    = 1,             /* in from y -448, out to y +448 */
    PAGE_SLIDE_FROM_BOTTOM = 2,             /* in from y +448, out to y -448 */
    PAGE_SLIDE_FROM_LEFT   = 3,             /* in from x -640, out to x +640 */
    PAGE_SLIDE_FROM_RIGHT  = 4              /* in from x +640, out to x -640 */
};

/* Item::anchorType: T259's ANCHOR_*. */

/* MenuLayoutItem::flags: T229's ITEM_*. */

/* The 640 x 448 virtual screen of the menus: T259's MENU_SCREEN_*. */

/* ======================================================================== */
/* T230 / T231 values                                                       */
/* ======================================================================== */
/* The quads drawn without a picture (pgl_color: RGBA8, R in the low
 * byte, alpha 0x80). */
#define SCROLL_BAR_PINCH_COLOR 0x80FF00FFu  /* magenta: ScrollBar without its "pinch" item */
#define TEXT_BOX_CURSOR_COLOR  0x80FFFFFFu  /* white: TextBox without its "cursor" item */

enum {
    TEXT_BOX_MAX_CHARS   = 0x80,            /* TextBox::chars: 0x80 characters and the terminator */
    TEXT_BOX_REPEAT_WAIT = 30,              /* the cursor buttons: Control::getRepeat(mask, 30, 3) */
    TEXT_BOX_REPEAT_STEP = 3
};

/* menu_play_se (T233): a cursor move is MENU_SE_CURSOR. */

/* The text of TextBox::drawCursor / virtual_2 as a string: 0x80 two-byte
 * characters and the terminator.  The frames have room for 0x101..0x110
 * bytes (the saved registers start at sp+0x110). */
enum { TEXT_BOX_STRING_SIZE = TEXT_BOX_MAX_CHARS * 2 + 1 };

/* The current TextBox and the parameters of the next one (T231 .sdata,
 * defined there with zero initialisers). */
extern gt2k_menu::TextBox *text_box_current;      /* 0x00352628 gp-0x53C8 the TextBox being edited */
extern int                 text_box_max_length;   /* 0x0035262C gp-0x53C4 maximum length (<= 0: 0x80) */
extern const char         *text_box_initial_text; /* 0x00352630 gp-0x53C0 the initial text */
extern char               *text_box_output;       /* 0x00352634 gp-0x53BC gets the text at the end */

/* ======================================================================== */
/* T232 - MenuCarSpec                                                       */
/* ======================================================================== */
extern MenuCarSpec *menu_car_spec_instance; /* 0x00352644 gp-0x53AC the instance (menu_car_spec;
                                               T232 .sdata, defined there) */

/* CarEquipments::getFronttirePart (T482): the part id of the front dirt
 * tyres (0x38..0x41 the tyre kinds 0..9, 0x42 kind 0xC: dirt;
 * T482_car_equipments.h PART_FRONT_TIRE_DIRT). */
enum { PART_ID_FRONT_TIRE_DIRT = 0x42 };

/* ======================================================================== */
/* T233 - the menu music                                                    */
/* ======================================================================== */
/* MenuSound::bgm / the BGM group of a page (T464: 0..0x11 sequenced, 0x12..
 * 0x22 the ADS stream groups 0..0x10; past them nothing plays). */
enum { MENU_BGM_NONE = 0x23 };

/* se_play's SE_PAN_CENTRE / SE_PRIO_PROGRAM, as menu_play_se calls it: T469. */

/* ======================================================================== */
/* T268 - the FreeButton statics (shared by every FreeButton)               */
/* ======================================================================== */
/* T268 .sdata, defined there with zero initialisers. */
extern f32 free_button_saved_position;      /* 0x00352648 gp-0x53A8 the sub list's focusPosition() */
extern int free_button_saved_count;         /* 0x0035264C gp-0x53A4 the sub list's count() then */
extern int free_button_cursor_moved;        /* 0x00352650 gp-0x53A0 1: its cursor moved since */

/* The spacing of a sub list's buttons (FreeButton, MultiButton). */
#define SUB_LIST_SPACING 4.0f

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (.bss): __si_type_info = 3 words, __class_type_info
 * = 4. */
extern "C" {
extern u32 __tiQ29gt2k_menu9ScrollBar[3];   /* 0x004B25B0 */
extern u32 __tiQ29gt2k_menu7TextBox[3];     /* 0x004B25C0 */
extern u32 __tiQ29gt2k_menu10FreeButton[4]; /* 0x004B25D0 */
extern u32 __tiQ29gt2k_menu11MultiButton[4];    /* 0x004B25E0 */
extern u32 __tiQ29gt2k_menu8BoolItem[3];    /* 0x004B25F0 */
}

/* The __class_type_info base lists (.sdata): {&__tiQ29gt2k_menu6Button,
 * public +0}. */
extern const __class_type_info::base_info g_00352658[1];   /* 0x00352658 FreeButton */
extern const __class_type_info::base_info g_00352660[1];   /* 0x00352660 MultiButton */
#endif /* !BOOT_OLD_GCC */

#endif /* U068_H */
