/* option_items_internal.h - unit u014 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00128D98..0x0012C300, second pass.
 *
 * Four translation units of the gt2k_menu option and prize screens:
 *   T098  HandicapItem: the 2-player handicap balance bar (option menu).
 *   T099  PrizeCounter: the rolling prize-money counter, and the items that
 *         show it - PrizeCounterControlItem ("count"), PrizeCounterShadowItem
 *         ("count_shadow"), PrizeCounterRemainItem ("get_money_disp") - with
 *         the hooks the prize screens register under those names.
 *   T100  CalibrationItem: one bar of the USB wheel calibration page.
 *   T101  KeyConfigItem: the button assignment page of one controller.
 * The option items are built by the T064 hooks (option_create_handicap,
 * option_create_calib_bar, option_create_key_config_1, ...) for
 * gt2k_menu::OptionControl (T062).
 *
 * Every class is defined in its canonical header (include/game/T098..T101
 * and the headers of the classes they use); this header adds what only this
 * unit uses: the key configuration constants, the declarations of T101's
 * tables (.data) and T100's counters (.sdata), and the typeinfo storage of
 * the unit's own classes.  It includes no class header: each file includes
 * the canonical headers it needs itself, in the order that gives retail's
 * type names (u014's data pass: egcs writes the names of the classes a file
 * sees in the reverse order of their definition; each file's comment lists
 * retail's), and this header last.
 */
#ifndef U014_H
#define U014_H

#include "types.h"
#include <stddef.h>
#include "game/game.h"                      /* EGCS_SIZE */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The menu sound effects: T233's MENU_SE_*. */

/* Control::type (T474): bits 0-4 the PS pad type (PS_PAD_*), bits 5-7 the
 * USB type (USB_WHEEL_GTFORCE: KeyConfigItem mode 2, USB_WHEEL_C294: mode 3,
 * the "jaguar_*" pictures). */

/* The pad manager slots of the USB wheels (PadManager::attach, T475). */
enum { PAD_SLOT_USB1 = 10, PAD_SLOT_USB2 = 11 };

/* CalibrationItem::axis: what the bar measures (the hook's p8). */
enum {
    CALIB_STEERING = 0,                     /* the steering range (channel 0) */
    CALIB_CENTRE   = 1,                     /* the steering centre (channel 0), the "pinch" marker */
    CALIB_PEDAL1   = 2,                     /* channel 1: axis1Min / axis1Max */
    CALIB_PEDAL2   = 3                      /* channel 2: axis2Min / axis2Max */
};

/* KeyConfigItem::mode: the controller kind, the index of its table in
 * g_0029BE98 and the mode of ControllerConfig's key tables (T426's
 * CONFIG_DIGITAL .. CONFIG_WHEEL_C294), or: */
enum {
    KEYCFG_NONE    = 5                      /* nothing usable connected */
};

/* The row functions (rowFunc, ControllerConfig's func) and the button ids
 * (indexes of g_0029BD80, KeyConfigItem::buttons; the tables' options). */
enum { KEY_FUNC_DEFAULT = 10 };             /* the "default" row: resets the mode's keys */
enum {
    KEY_ANA_L_LR    = 0x01,                 /* the left stick's x axis */
    KEY_ANA_R_LR    = 0x02,                 /* the right stick's x axis */
    KEY_ANA_L_L     = 0x05,                 /* left stick left (0x06 right) */
    KEY_ANA_R_L     = 0x09,                 /* right stick left (0x0A right) */
    KEY_USB_L1      = 0x15,                 /* usb_l1 .. usb_r3: 0x15 .. 0x1A */
    KEY_USB_L2      = 0x16,
    KEY_USB_L3      = 0x17,
    KEY_USB_R1      = 0x18,
    KEY_USB_R2      = 0x19,
    KEY_USB_R3      = 0x1A,
    KEY_DEFAULT     = 0x22,                 /* "default_item" (only on an editable page) */
    KEY_JAGUAR_L1   = 0x23,                 /* jaguar_l1 .. jaguar_r3: 0x23 .. 0x28 */
    KEY_JAGUAR_L2   = 0x24,
    KEY_JAGUAR_L3   = 0x25,
    KEY_JAGUAR_R1   = 0x26,
    KEY_JAGUAR_R2   = 0x27,
    KEY_JAGUAR_R3   = 0x28,
    KEY_BUTTON_COUNT = 41
};

/* ======================================================================== */
/* T101 .data: the key configuration tables (defined in T101)              */
/* ======================================================================== */
namespace gt2k_menu {

/* One row of a mode's table: the button ids it offers. */
struct KeyRow {
    const int *options;                     /* 0x0 */
    int        count;                       /* 0x4 */
};
EGCS_SIZE(gt2k_menu::KeyRow, 0x8);

} /* namespace gt2k_menu */

extern const char *g_0029BD80[KEY_BUTTON_COUNT]; /* 0x0029BD80 button pictures: "cross_lr", "ana_l_lr",
                                               ..., "usb_l1" .. "usb_r3", "default_item", "jaguar_l1" .. */
extern const char *g_0029BE28[10];          /* 0x0029BE28 row labels, pad page, player 1: "steering_1" ..
                                               "change_view_1", "default_1" */
extern const char *g_0029BE50[10];          /* 0x0029BE50 the same for player 2 ("steering_2" ..) */
extern const char *g_0029BE78[7];           /* 0x0029BE78 row labels, USB page: "back" .. "change_view",
                                               "default_1" */
/* Per mode 0..4 (4 = 3's table; mode 5 has none): its rows (T101's static
 * const tables, .rodata 0x003145B0 ..).  Digital pad 8 buttons a row,
 * analog 16 (8 more stick directions), GT Force 6, type 3 wheel 10.  The
 * row functions g_003146C0 (pad page) / g_003146E8 (USB page) are T101's
 * static const tables too (internal linkage: retail has them after the
 * type names, with the rows). */
extern const gt2k_menu::KeyRow *g_0029BE98[5];  /* 0x0029BE98 */

/* ======================================================================== */
/* T100 .sdata (defined in T100: zeros after "pinch")                       */
/* ======================================================================== */
extern int g_003512F8;      /* 0x003512F8 gp-0x66F8 pjoy_get_event_count of the port when the first
                               CalibrationItem was built */
extern int g_003512FC;      /* 0x003512FC gp-0x66F4 CalibrationItems alive */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __si_type_info storage (3 words, .bss) of the Item subclasses only
 * this unit builds (PrizeCounter's is declared in T099_prize_counter.h). */
extern "C" {
extern u32 __tiQ29gt2k_menu12HandicapItem[3];             /* 0x004B1650 */
extern u32 __tiQ29gt2k_menu22PrizeCounterRemainItem[3];   /* 0x004B1690 */
extern u32 __tiQ29gt2k_menu23PrizeCounterControlItem[3];  /* 0x004B16B0 */
extern u32 __tiQ29gt2k_menu22PrizeCounterShadowItem[3];   /* 0x004B16D0 */
extern u32 __tiQ29gt2k_menu15CalibrationItem[3];          /* 0x004B1700 */
extern u32 __tiQ29gt2k_menu13KeyConfigItem[3];            /* 0x004B1710 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U014_H */
