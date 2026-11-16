/* setting_actions_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u036,
 * 0x00166F98..0x00169780, second pass.
 *
 * Translation units (link order), both of the car setting screen
 * (gt2k_menu::SettingControl, T080, "menu/setting.mbl"; ctx of every hook is
 * the SettingControl):
 *   T083  the ExecuteFuncData table g_002ACCC8 (game/menu/car_settings/T083_setting_actions.h):
 *         "commit_slider", "go_load", "go_save", "retrieve_slider" and the 73
 *         "set_<category>_<part>" actions, which fit an owned part to the
 *         control's working copy of the setting car.
 *   T084  car_meets_event_regulations (does a car meet the regulations of the current event)
 *         and the SelectableFuncData table g_002AD058
 *         (game/menu/car_settings/T084_setting_checks.h): one availability check per button of
 *         a setting page, "chg/change_*" pages "the setting car owns part N",
 *         "set/adjast_*" pages "this setting of the setting car can be
 *         adjusted"; then T084's unreferenced copies of RegurationResult's
 *         inline destructor and typeinfo function (T045).
 *
 * Two cars are involved: GameStatus's setting car (MenuStatus::raceCar[0],
 * NULL when there is none) and SettingControl::car, the control's working
 * copy of it (+0xE40).  The setting car is read through the inline
 * GameStatus::getMenuStatus() (the listing adds menu's 0x307C8, then loads
 * raceCar[0] at +0x540).
 *
 * Every shared class is defined in a canonical header.  Each source includes
 * the canonical headers it needs itself, in the order that gives retail's
 * type names (the data pass, ref/data/u036/notes.md); this header keeps the
 * outside headers that define no class and the unit's notes. */
#ifndef U036_H
#define U036_H

#include "types.h"
#include <stddef.h>

/* Menu sound effects: T233's MENU_SE_REFUSED (a "set_*" part the car does not
 * own) and MENU_SE_CONFIRM (a part fitted). */

/* Tyre kinds 0..9 (TIRE_KIND_NORMAL .. TIRE_KIND_CONTROL): T482. */

#endif /* U036_H */
