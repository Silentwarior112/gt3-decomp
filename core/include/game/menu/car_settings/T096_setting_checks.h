/* T096_setting_checks.h - the checks of the car setting pages (translation
 * unit T096, unit u013), 0x001279C8..0x00128338.
 *
 * g_0029AEA0 is the ValidFuncData table (T211) that SettingControl (T080)
 * gives its PageManager (the first pass's handlers[3], PageManager +0xB24):
 * 75 {exec name, check} entries sorted by name.  A check gets the decided
 * button's record and returns NULL to accept it or the page to open as a
 * popup instead: "set_*" (check_set_parts 0x001279C8) refuses a part the
 * current race's regulations do not allow ("not_to_change"),
 * "chg/change_tir_front" / "chg/change_tir_rear" (check_change_tire
 * 0x00127B30) a tyre change in a dirt-tyre race ("dirt_tire_only").  The
 * checks are used only through the table.  The rest of the unit is T096's
 * unreferenced copies of GarageCar's inline functions (T036).
 *
 * PageItem (u013, u036) is the button record of T244: name at +0x000, the
 * exec / check name ("set_tcs_on") at +0x0C0, the link at +0x100. */
#ifndef GAME_T096_SETTING_CHECKS_H
#define GAME_T096_SETTING_CHECKS_H

#include "game/game.h"
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::ValidFuncData (g_0029AEA0) */

namespace gt2k_menu {
struct ButtonData;                          /* T244, 0x14C bytes (pointers only) */
}

typedef gt2k_menu::ButtonData PageItem;     /* u013, u036 (T244_button_list.h for its members) */

/* The checks, sorted by name (T096 .data), and their count (T096 .sdata,
 * read by SettingControl::begin, T080). */
extern gt2k_menu::ValidFuncData g_0029AEA0[75];    /* 0x0029AEA0 */
extern int g_003512DC;                      /* 0x003512DC gp-0x6714 its entries (75) */

/* The static initialiser of the table (.ctors entry 0x0028F0CC; the
 * compiler's _GLOBAL_$I$ function in the original). */
void T096_static_init(void);                /* 0x00127BB0 */

#endif /* GAME_T096_SETTING_CHECKS_H */
