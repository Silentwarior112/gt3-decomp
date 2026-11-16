/* trophy_get_internal.h - unit u020 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0013A8D0..0x0013C8F0, second pass.
 *
 * Three translation units of the gt2k_menu prize screens:
 *   T060  gt2k_menu::TrophyGetControl ("menu/trophy_get.mbl"): the
 *         championship cup screen of trophy_get (called by T053 once a
 *         championship is won), plus its item factory table, RTTI and the
 *         unreferenced copies of the PrizeCounter / PrizeControlBase /
 *         ControlBaseWithPageManager destructors and typeinfo functions.
 *   T105  gt2k_menu::NotCarGetControl ("menu/not_get_car.mbl"): the page
 *         not_car_get shows instead of a prize car when the garage is full
 *         (T061 car_get_prizes, >= 200 cars).
 *   T106  the prize car presentation helpers of T107's CarsItem and T061's
 *         CarGetControl: CarRoulette, CarTurntable, the floor drawing and
 *         gt2k_menu::CarGetMenuCamera.
 *
 * Every class is defined in its canonical header (include/game/T060, T105,
 * T106 and the headers of the classes they use).  Since the data pass
 * (2026-10-17) each source includes the canonical headers it needs itself,
 * in the order that gives retail's type names (each file's comment lists
 * them; ref/data/u020/notes.md), and this header last: a few constants and,
 * for GCC 14, the typeinfo storage of the unit's own classes.  One section
 * per translation unit (#define U020_T060 / U020_T105 / U020_T106 before
 * the include).
 */
#ifndef U020_H
#define U020_H

#include "types.h"
#include "libc.h"                           /* memalign, free */
#include <stddef.h>

/* ======================================================================== */
/* T060                                                                     */
/* ======================================================================== */
#ifdef U020_T060
/* The menu heap of the screen (memalign(0x40, size), menu_heap_push). */
enum { TROPHY_GET_HEAP_SIZE = 0x100000 };   /* 1 MB */

/* GameStatus::largeTrophy: championship cup n (0..9) is bit 6 + n (the
 * licence trophies, T059, are the bits below). */
enum { LARGE_TROPHY_CUP0 = 0x40 };

/* The music of the cup screen (menu_bgm_play_if_changed; T464: groups
 * 0x12..0x22 are the ADS stream groups 0..0x10).  (? name; u019 names 0x17
 * BGM_PRIZE_FANFARE and 0x21 BGM_LICENSE_GET.) */
enum { BGM_TROPHY_GET = 0x18 };

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; for GCC 14 only.  The
 * __si_type_info storage (3 words, .bss) of TrophyGetControl (the bases'
 * nodes are declared in T056, T059 and T099). */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user */
extern "C" {
extern u32 __tiQ29gt2k_menu16TrophyGetControl[3];         /* 0x004B16E0 */
}
#endif
#endif /* U020_T060 */

/* ======================================================================== */
/* T105                                                                     */
/* ======================================================================== */
#ifdef U020_T105
/* The menu heap of the page (memalign(0x40, size), menu_heap_push). */
enum { NOT_CAR_GET_HEAP_SIZE = 0x40000 };   /* 256 KB */

#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
extern "C" {
extern u32 __tiQ29gt2k_menu16NotCarGetControl[3];         /* 0x004B1810 */
}
#endif
#endif /* U020_T105 */

/* ======================================================================== */
/* T106                                                                     */
/* ======================================================================== */
#ifdef U020_T106
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
extern "C" {
extern u32 __tiQ29gt2k_menu16CarGetMenuCamera[3];         /* 0x004B1820 */
}
#endif
#endif /* U020_T106 */

#endif /* U020_H */
