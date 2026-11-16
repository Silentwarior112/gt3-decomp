/* gtmode_control_internal.h - unit u009 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00119778..0x0011C9C8, second pass: the GT mode menu's controller and its
 * page tables.
 *
 * Translation units (link order), each with its canonical header:
 *   T043  gt2k_menu::GtmodeControl: the controller gtmode_main (T038) runs:
 *         the pages of "menu/gt_mode.mbl", the root page over the title
 *         movie, the garage trade page (T043_gtmode_control.h)
 *   T044  the MenuFuncData table g_00295FB8: the "menu_*" button list
 *         factories of the pages (garage, wheels, dealer, tuning shops,
 *         regulated cars, map, top, ride), gt2k_menu::DealerButtonList and
 *         gt2k_menu::CarGoodsButtonList (T044_gtmode_menu_factories.h)
 *   T045  the ValidFuncData table g_00296750: the checks run before a button
 *         acts (credits, ridden car, licences, regulations; NULL = go on,
 *         else the message page to open), and RegurationResult's out-of-line
 *         destructor and typeinfo (T045_gtmode_page_checks.h)
 *
 * GtmodeControl::begin gives the five FuncData tables (selectable T049, menu
 * T044, execute T037, valid T045, focus T048) to its PageManager, with
 * itself as their context, and the hook table g_00296B90 (T036) to both of
 * its page layouts.
 *
 * Every class is defined in a canonical header.  Each source includes the
 * canonical headers it needs itself, in the order that gives the original's
 * type names (u009's data pass, ref/data/u009/notes.md); this header has the
 * unit's constants and the type_info storage of its classes only (no header
 * here may define a class: egcs would write its type name into every
 * source). */
#ifndef U009_H
#define U009_H

#include "types.h"
#include "game/data/T482_car_equipments.h"       /* PART_FRONT_TIRE (PART_RACING_TIRE_*) */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __class_type_info::base_info (g_00351090) */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The menus' virtual screen (T259 MENU_SCREEN_W / MENU_SCREEN_H: 640 x 448)
 * is also the size of the title movie. */

/* The frames per second of the idle count (the title page's idle time is in
 * seconds: ProductConfig::idleSeconds). */
enum { FRAMES_PER_SECOND = 60 };

/* GameStatus::menu.nextMode, the mode main() runs next (u001's names, T000:
 * the ones this unit tests). */
enum {
    MAIN_MODE_ARCADE        = 1,            /* main_arcade */
    MAIN_MODE_CHAMPIONSHIP  = 2,            /* main_championship */
    MAIN_MODE_GTMODE_RACE   = 3,            /* main_gtmode_race */
    MAIN_MODE_GTMODE_RACE2  = 4,            /* main_gtmode_race2 */
    MAIN_MODE_OPTIONS       = 9,            /* the options (T062) */
    MAIN_MODE_CARD_REPLAY   = 11,           /* play_saved_replay(0) */
    MAIN_MODE_FILE_UTILITY  = 24,           /* the memory card file utility */
    MAIN_MODE_GAME_SAVE     = 25,           /* save the game */
    MAIN_MODE_GAME_LOAD     = 26            /* load the game */
};

/* GtmodeControl::mode: how the root page was entered (its cursor, T048
 * cursor_from_owner), from the mode main() came back from. */
enum {
    ROOT_ENTRY_DEFAULT      = 0,            /* the first time, after an arcade race, others */
    ROOT_ENTRY_FROM_PAGE    = 1,            /* a page asked for the root page (requestRoot) */
    ROOT_ENTRY_CARD_REPLAY  = 2,            /* MAIN_MODE_CARD_REPLAY */
    ROOT_ENTRY_GAME_LOAD    = 3,            /* MAIN_MODE_GAME_LOAD */
    ROOT_ENTRY_GAME_SAVE    = 4,            /* MAIN_MODE_GAME_SAVE */
    ROOT_ENTRY_FILE_UTILITY = 5,            /* MAIN_MODE_FILE_UTILITY */
    ROOT_ENTRY_OPTIONS      = 6             /* MAIN_MODE_OPTIONS */
};

/* GtmodeControl::infoMode: the garage's info column, in [0, 6), at 2. */
enum { GARAGE_INFO_MODES = 6, GARAGE_INFO_MODE_DEFAULT = 2 };

/* The menu sound effects of the checks: T233's MENU_SE_REFUSED / MENU_SE_CONFIRM
 * (7, before the "ride this car?" popup). */

/* The bits of the car record's shop flags (CarEquipments::getShopFlags). */
enum {
    CAR_SHOP_BIT_NOT_AT_DEALER = 0,         /* not sold by the dealers */
    CAR_SHOP_BIT_NOT_USED      = 1,         /* not bought / sold at the used car shop */
    CAR_SHOP_BIT_FIXED_WHEELS  = 2          /* the wheels cannot be changed */
};

/* The garage's slots (GameStatus::cars[200]). */
enum { GARAGE_MAX_CARS = 200 };

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* The racing tyres a tuning shop sells as a set ("ts/ts_buy_sure_all_tire"):
 * front tyre kinds 2 .. 8, racing super hard .. super soft (T482). */
enum {
    PART_RACING_TIRE_FIRST = PART_FRONT_TIRE + 2,  /* 0x3A */
    PART_RACING_TIRE_END   = PART_FRONT_TIRE + 9   /* 0x41, exclusive */
};

/* ======================================================================== */
/* Type_info storage of the unit's classes (.bss), filled by their __tf     */
/* functions; the base list of the two __class_type_infos (.sdata)          */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC                        /* egcs generates the __tf functions itself */
extern "C" {
extern u32 __tiQ29gt2k_menu13GtmodeControl[3];          /* 0x004B1590 __si_type_info */
extern u32 __tiQ29gt2k_menu16DealerButtonList[4];       /* 0x004B15A0 __class_type_info */
extern u32 __tiQ29gt2k_menu18CarGoodsButtonList[4];     /* 0x004B15B0 __class_type_info */
}
/* {base ButtonList (__tiQ29gt2k_menu10ButtonList 0x004B2650), public,
 * offset 0}: the one base of DealerButtonList and of CarGoodsButtonList. */
extern const __class_type_info::base_info g_00351090[1];   /* 0x00351090 */
#endif /* !BOOT_OLD_GCC */

#endif /* U009_H */
