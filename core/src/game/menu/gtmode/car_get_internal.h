/* car_get_internal.h - unit u021 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0013C8F0..0x0013FF98, second pass.
 *
 * The prize car screen and the options menu (translation units in address
 * order):
 *   T107  gt2k_menu::CarsItem ("render_model" of menu/car_get.mbl): up to four
 *         cars of a prize set (TheCar: CarModel body + four MenuTireModel
 *         tyres) on the owner's turntable (T106); a roulette (CarRoulette,
 *         T106) stops on the car won (picked at random by the cars'
 *         weights), the others sink into the floor and the car won is lit
 *         and reflects "menu/prize_get.img".  The "car_get_message" item
 *         (CarGetMessageItem) shows once that is done.
 *   T061  gt2k_menu::CarGetControl (menu/car_get.mbl, page "car_get"): one
 *         screen per prize set a race gives (car_get_prizes: loads the cars'
 *         models, adds the car won to the garage), F_0013F508 the entry point
 *         (then plays the ending movies once), the "eventname" item
 *         (EventNameItem) and the hook table g_0049BE90.
 *   T062  gt2k_menu::OptionControl (menu/option.mbl, page "option"): the
 *         options menu, F_0013FE58 its entry point (a 1 MB menu heap on the
 *         stack); edits a copy of the game's GameOptions.
 *
 * Every class is defined in its canonical header (include/game/T107, T061,
 * T062 and the headers of the classes they use).  Since the data pass
 * (2026-10-17) each file includes the canonical headers it needs itself, in
 * the order that gives retail's type names (see there); this header has
 * what only this unit uses: constants and, for GCC 14, the typeinfo storage
 * of the unit's own classes.  T107's light vectors and T061's hook table are
 * file statics of their files (.bss).
 */
#ifndef U021_H
#define U021_H

#include "types.h"
#include "libc.h"                           /* memset, memalign, free */
#include <stddef.h>

#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user (GCC 14 only) */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* T061: the prize car screens' memory (car_get_prizes). */
enum {
    CAR_GET_MAX_CARS     = 4,               /* the cars of a prize set (CarsItem::cars) */
    CAR_MODEL_BUF_SIZE   = 0xC8000,         /* one car's model file */
    PRIZE_GET_IMG_SIZE   = 0x19000,         /* "menu/prize_get.img" */
    CAR_GET_HEAP_SIZE    = 0x80000,         /* the menu heap of one screen */
    GARAGE_MAX_CARS      = 200              /* a full garage: not_car_get instead */
};
/* car_model_path kinds (T446): 0 "cars/day/", 1 "cars/night/", 2
 * "menu/cars/", 3 "cars/eve/". */
enum { CAR_MODEL_MENU = 2 };
/* The BGM group of the prize car page (menu_bgm_play_if_changed). (? the
 * music of the car won) */
enum { BGM_GROUP_CAR_GET = 0x19 };

/* T062: the options menu's menu heap, on F_0013FE58's stack. */
enum { OPTION_HEAP_SIZE = 0x100000 };

/* T107: CarsItem::state, the show of virtual_1. */
enum {
    CARS_START     = 0,                     /* sets the first wait */
    CARS_WAIT      = 1,                     /* then the roulette starts (one car: no roulette) */
    CARS_SPIN      = 2,                     /* the roulette runs */
    CARS_STOPPING  = 3,                     /* until it has stopped on the car won */
    CARS_HIGHLIGHT = 4,                     /* the other cars are covered (highlight 1 -> 0) */
    CARS_SINK      = 5,                     /* the other cars sink into the floor */
    CARS_DONE      = 6
};
/* The frames of the waits. */
enum {
    CARS_WAIT_ONE_CAR = 210,                /* 3.5 s before a single car is shown as won */
    CARS_WAIT_FIRST   = 120,                /* 2 s before the roulette starts */
    CARS_SPIN_TIME    = 60                  /* the roulette runs 1 s before it is stopped */
};
/* MenuCarCamera::mode (T411) of the prize cars: showroom record 8,
 * "menu/showroom_prize.fbi" (showroom_index maps mode 9 to record 8). */
enum { MENU_CAMERA_MODE_PRIZE = 9 };

/* The showroom record (T215 ShowroomData): flag bit 0, "the record has
 * light 0"; values 0 the ambient colour, 4 / 8 / 12 / 16 light 0's
 * ambient, diffuse, specular colours and position (as T245 reads them). */
enum { SHOWROOM_FLAG_LIGHT0 = 0 };
enum {
    SHOWROOM_AMBIENT         = 0x00,
    SHOWROOM_LIGHT0_AMBIENT  = 0x04,
    SHOWROOM_LIGHT0_DIFFUSE  = 0x08,
    SHOWROOM_LIGHT0_SPECULAR = 0x0C,
    SHOWROOM_LIGHT0_POSITION = 0x10
};

/* The prize car scene's projection: 640 x 448 around the centre, near
 * 0.1, far 1000, a horizontal field of view of 40 degrees (focal length
 * 320 / tan(20 degrees)); the y scale 1.098 (F_00257EF0), x * 0.75 on a
 * 16:9 screen. */
#define CARS_HALF_FOV_RAD   0.34906587f     /* 20 degrees */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (__si_type_info 3 words, __user_type_info 2). */
extern "C" {
extern u32 __tiQ29gt2k_menu17CarGetMessageItem[3];        /* 0x004B1830 */
extern u32 __tiQ29gt2k_menu13CarGetControl[3];            /* 0x004B1840 */
extern u32 __tiQ29gt2k_menu13EventNameItem[3];            /* 0x004B1850 */
extern u32 __tiQ29gt2k_menu8CarsItem[3];                  /* 0x004B1860 */
extern u32 __tiQ29gt2k_menu13OptionControl[3];            /* 0x004B1870 */
extern u32 __tiQ29gt2k_menu6TheCar[2];                    /* 0x00353348 gp-0x46A8 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U021_H */
