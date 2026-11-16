/* gtmode_main_internal.h - unit u013 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001258E8..0x00128D98, second pass.
 *
 * Translation units (link order), each with its canonical header:
 *   (T036's last 9 functions, 0x001258E8..0x00125B50: DecayItem and the
 *        copies egcs emits at the end of T036, are in the whole T036 source,
 *        src/game/menu/gtmode/T036_garage.cpp, since the T036 merge)
 *   T037 the GT-mode page actions ("buy_car", "go_race", "sort_price", ...):
 *        the ExecuteFuncData table g_0029A180 (54 entries, sorted by name,
 *        filled by T037_static_init) (T037_gtmode_actions.h), and
 *        gt2k_menu::TumbleCar, the car wash animation (T037_tumble_car.h)
 *   T046 gt2k_menu::GtmodeRender, the renderer of GtmodeControl
 *        (T046_gtmode_render.h)
 *   T038 gtmode_main (called by main): runs GtmodeControl until the player
 *        leaves the GT mode; handles the trade and the "save at home" modes
 *        (T038_gtmode_main.h)
 *   T039 GT2KMSGGTMODE::Message, the "GTMODE" real-time text table
 *        (T039_gtmode_messages.h)
 *   T050 set_country: country-dependent settings (T050_country.h)
 *   T096 the ValidFuncData table g_0029AEA0 of the car setting pages
 *        (SettingControl, T080): 75 checks (T096_setting_checks.h)
 *   T097 gt2k_menu::NameEntryControl and name_entry (T097_name_entry.h)
 *   T051 play_movie, gt2k_menu::MovieControl / MovieRender
 *        (T051_movie_player.h)
 *
 * How the T037 actions and T096 checks are reached: a control's begin()
 * gives its PageManager (T211) the tables with itself as their context
 * (executeFuncs / validFuncs); for a decided button the PageManager looks the
 * button record's exec name (ButtonData +0xC0) up and calls
 *     func(ctx, rec, index, layout)   /   func(ctx, rec)
 * A button record starts with its name (a race / car / wheel / part /
 * course code), which most actions use as their string argument.  A check
 * returns NULL to go on, or the name of the message page to open instead.
 *
 * Every class is defined in a canonical header.  Each source includes the
 * canonical headers it needs itself, in the order that gives the original's
 * type names (u013's data pass: egcs writes the type names of the classes a
 * file sees in the reverse order of their definitions; each file's comment
 * lists the original's).  This header has only the unit's constants and,
 * for GCC 14, the type_info storage of the unit's classes. */
#ifndef U013_H
#define U013_H

#include "types.h"
#include <stddef.h>

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* MenuStatus::nextMode, the mode main() (T000) runs once gtmode_main
 * returns (u001's names; 0x1D / 0x1E are handled by gtmode_main itself). */
enum {
    MAIN_MODE_ARCADE        = 1,            /* main_arcade */
    MAIN_MODE_CHAMPIONSHIP  = 2,            /* main_championship */
    MAIN_MODE_GTMODE_RACE   = 3,            /* main_gtmode_race (a spot race) */
    MAIN_MODE_GTMODE_RACE2  = 4,            /* main_gtmode_race2 (the same code; a single race) */
    MAIN_MODE_MENU_DEMO     = 5,            /* run_menu_demo_race ("go_preview") */
    MAIN_MODE_LICENSE_TEST  = 6,            /* main_license_test */
    MAIN_MODE_MACHINE_TEST  = 7,            /* main_machine_test */
    MAIN_MODE_FREE_RUN      = 8,            /* main_free_run */
    MAIN_MODE_OPTIONS       = 9,            /* the options (T062) */
    MAIN_MODE_SETTING       = 10,           /* "go_setting" (? main() has no case for 10, T000) */
    MAIN_MODE_CARD_REPLAY   = 11,           /* play_saved_replay(0): "go_replay_theater" */
    MAIN_MODE_DEMO_REPLAY   = 12,           /* play_saved_replay(1): "go_demonstration" */
    MAIN_MODE_LOGGER        = 13,           /* logger_main (T055) */
    MAIN_MODE_FILE_UTILITY  = 24,           /* the memory card file utility */
    MAIN_MODE_GAME_SAVE     = 25,           /* save the game */
    MAIN_MODE_GAME_LOAD     = 26,           /* load the game */
    MAIN_MODE_GTMODE_TRADE  = 29,           /* "go_trade": gtmode_main's guest garage */
    MAIN_MODE_GTMODE_SAVE   = 30            /* "go_save_in_home": gtmode_main's game save */
};

/* The menu heaps (menu_heap_push) and the memory card work buffer. */
enum {
    MENU_HEAP_ALIGN       = 0x40,
    GTMODE_HEAP_SIZE      = 0x400000,       /* 4 MB: gtmode_main's GtmodeControl */
    GTMODE_SAVE_WORK_SIZE = 0xD00000,       /* 13 MB: the "save at home" work buffer */
    NAME_ENTRY_HEAP_SIZE  = 0x20000,        /* 128 KB: name_entry */
    MOVIE_HEAP_SIZE       = 0x100000        /* 1 MB: play_movie */
};

/* GtmodeControl::sortType: the garage's sort orders (the comparators of
 * GarageButtonList, T219 g_002C8DF8): the enum GarageSortOrder
 * (GARAGE_SORT_DATE ..) of game/menu/gtmode/T043_gtmode_control.h (u009 made the member
 * that type).  GtmodeControl::infoMode: the info column the order shows
 * (GarageButton::virtual_2, T219). */
enum {
    GARAGE_INFO_POWER      = 0,
    GARAGE_INFO_PRICE      = 1,
    GARAGE_INFO_MAKER      = 2,
    GARAGE_INFO_DISTANCE   = 3,
    GARAGE_INFO_DRIVE      = 4,
    GARAGE_INFO_ASPIRATION = 5
};

/* GtmodeControl::mode: the root page entered from the replay theater (T043
 * mainLoop: as after MAIN_MODE_CARD_REPLAY). */
enum { ROOT_ENTRY_CARD_REPLAY = 2 };

/* The menu sound effects (menu_play_se / menu_sound_after, T233 / T202): T233's
 * MENU_SE_* (MENU_SE_BOUGHT, MENU_SE_CAR_WASH, MENU_SE_REFUSED, MENU_SE_WHEEL_FIT). */
/* menu_sound_after's delay of the shop sounds: half a second. */
enum { MENU_SE_DELAY_FRAMES = 30 };

/* (The racing tyres "buy_all_tires" buys, PART_RACING_TIRE_FIRST / _END, are
 * T037's: they are defined from T482's PART_FRONT_TIRE there.) */

/* The course regions of course_set_region (T179: its folder table g_002C2AF0 holds
 * "courses/US" at 0 and 2, "courses/JP", "courses/UK", "courses/EU" at 1, 3,
 * 4). */
enum { COURSE_REGION_JP = 1, COURSE_REGION_US = 2, COURSE_REGION_UK = 3, COURSE_REGION_EU = 4 };

/* T050's switches have a case for country 7, which does what the default
 * does (? a country not in T516's list). */
enum { COUNTRY_OTHER = 7 };

/* The player's name on the name entry page (GameStatus::lastInputedName). */
enum { NAME_ENTRY_MAX_LEN = 15 };

/* (The two one-entry tables of the name entry page, g_0049BCA0 / g_00353208,
 * are T097's file statics: .bss / .sbss.) */

/* ======================================================================== */
/* Type_info storage of the unit's classes (.bss), filled by their __tf     */
/* functions (GCC 14 only: egcs generates them itself)                      */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu9TumbleCar[3];              /* 0x004B15E0 __si_type_info */
extern u32 __tiQ29gt2k_menu12GtmodeRender[3];           /* 0x004B15F0 __si_type_info */
extern u32 __tiQ213GT2KMSGGTMODE7Message[3];            /* 0x004B1600 __si_type_info */
extern u32 __tiQ29gt2k_menu16NameEntryControl[3];       /* 0x004B1620 __si_type_info */
extern u32 __tiQ29gt2k_menu11MovieRender[3];            /* 0x004B1630 __si_type_info */
extern u32 __tiQ29gt2k_menu12MovieControl[3];           /* 0x004B1640 __si_type_info */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U013_H */
