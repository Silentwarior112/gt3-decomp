/* setting_hooks_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, units u033
 * (0x0015D448..0x0015FC70) and u034 (0x0015FC70..0x001638F8): translation
 * unit T081, one source file (T081_setting_hooks.cpp; the matching pass had
 * it in two parts, one per work unit).
 *
 * T081 (0x0015D448..0x001638F8, 160 functions): the layout hooks of the car
 * setting screen (gt2k_menu::SettingControl, T080, "menu/setting.mbl";
 * T081_setting_hooks.h).  T081_static_init (.ctors entry 0x0028F0A0) fills
 * the hook table g_002A9080 (161 MenuHooks {name, create, p8, pC, p10, p14},
 * sorted by name; count g_00351950); SettingControl::begin (T080) gives it to
 * both layouts of its PageManager (PageLayout::setHooks, T229; ctx the
 * SettingControl).  None of the functions has a direct caller: each is the
 * p8 of a table entry (or a create function of its own), and the function
 * names are the entries' hook names:
 *   text_*          p8 of create_callback_text (T239), called fn(ctl, buf,
 *                   0x80): the value of a setting / the name of a fitted part
 *                   of the working copy, "----" when there is no setting car
 *                   or the setting cannot be adjusted on it;
 *   attach_*        p8 of create_bool_item (T234): the item (the image of a
 *                   part) is shown while the part fitted to the working copy
 *                   is that part;
 *   disable_*       p8 of create_bool_item: the setting cannot be adjusted
 *                   (the T084 checks inverted);
 *   gear_chart_car  p8 of create_gear_chart (T237);
 *   "gear_setup", a hook that creates nothing but finds the gear ratio
 *   sliders of the current button list; "turbine_zero_name", "user_carname",
 *   "car_current_power".
 *
 * Two cars are involved: GameStatus's setting car (MenuStatus::raceCar[0],
 * the car of the setting pages; NULL when there is none: then every hook
 * gives "----" / 0) and SettingControl::car, the working copy of it (copied
 * by SettingControl::mainLoop, T080), whose CarSaveEquipments holds the
 * setting values the texts show and whose fitted parts the attach_*
 * predicates read.  The adjustability checks (T084) and the gear counts read
 * the GameStatus car.
 *
 * Every class is defined in a canonical header; the source includes the
 * headers it needs itself, in the order that gives retail's type names (the
 * data pass, ref/data/u033/notes.md); this header has the unit's constants
 * only. */
#ifndef U033_H
#define U033_H

#include "types.h"

/* ======================================================================== */
/* Constants (only used here)                                               */
/* ======================================================================== */

/* The toe settings are stored biased: TOE_ZERO is 0 (CarSaveEquipments
 * frontToe / rearToe); the texts show (toe - TOE_ZERO) x 5 tenths of a
 * degree ("%.1t"). */
#define TOE_ZERO 0x80

/* RaceEvent::needTire of a race on dirt tyres only: T484's NEED_TIRE_DIRT.
 * The tyre kinds 0..9 (TiresRecord::kind; the part ids of the fitted tyres
 * are PART_FRONT_TIRE / PART_REAR_TIRE + kind): T482's TIRE_KIND_*. */

#endif /* U033_H */
