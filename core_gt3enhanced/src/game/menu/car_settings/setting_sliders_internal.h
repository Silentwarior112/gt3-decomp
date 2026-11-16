/* setting_sliders_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u035,
 * 0x001638F8..0x00166F98, second pass.
 *
 * Translation unit T082 (whole): the slider functions of the car setting
 * screen (gt2k_menu::SettingControl, T080, "menu/setting.mbl";
 * T082_setting_sliders.h).  T082_static_init fills the MenuSliderFunc table
 * g_002ABAB0 (40 {name, func} entries sorted by name, count g_00351970);
 * SettingControl::begin (T080) hands it to both layouts of its PageManager
 * (PageLayout::setSliderFuncs, ctx the SettingControl).
 * ButtonList::createSlider (T244) builds a Slider (T238) for an element
 * "slider_<name>" on the SliderDesc func(ctx) returns (by value: the
 * caller's buffer is the hidden $a0 and comes back in $v0, ctx is $a1);
 * the Slider calls desc.set(ctx, value, commit) on every move (commit 0)
 * and from Slider::setValue (commit 1).
 *
 * Two cars are involved: GameStatus's setting car (MenuStatus::raceCar[0],
 * the car of the setting pages; NULL when there is none) and
 * SettingControl::car, the working copy of it (T080).  A func takes the
 * range from the record of the part fitted to the setting car and, except
 * the gear sliders, the current value from the setting car too; without a
 * setting car it returns {0, 1, 0, 1, set, 0, 0, 0}.  A set function writes
 * the working copy and returns car_meets_event_regulations of it (T084: the regulations of
 * the current event still hold), 0 without a setting car; the gear setters
 * return 1.  Every SliderDesc here is mode 0 (SLIDER_MODE_IMMEDIATE): a 0
 * from set refuses the value, so the knob cannot leave the regulations.
 *
 * Every class is defined in a canonical header; T082 includes the ones it
 * needs itself, in the order that gives retail's type names (see there).
 * This header keeps the unit's constants. */
#ifndef U035_H
#define U035_H

#include "types.h"

/* SliderDesc::mode: T238's SLIDER_MODE_IMMEDIATE (0). */

/* The body of the empty loop setting_gear_apply_auto_level keeps (u141's
 * SPIN_BODY): egcs keeps an empty loop (counted down, with the four nops of
 * the R5900 short-loop padding), GCC 14 would delete it, so there it gets an
 * empty volatile asm. */
#ifdef BOOT_OLD_GCC
#define EMPTY_LOOP_BODY()
#else
#define EMPTY_LOOP_BODY()   __asm__ __volatile__("")
#endif

#endif /* U035_H */
