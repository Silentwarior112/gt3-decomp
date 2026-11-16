/* T081 - the layout hooks of the car setting screen
 * (gt2k_menu::SettingControl, "menu/setting.mbl"), 0x0015D448..0x001638F8:
 * the texts of the setting values and of the fitted parts, the attach_* and
 * disable_* BoolItem predicates, "gear_setup", three more hooks and the
 * static initialiser of the hook table.  One object in the original; the
 * matching pass had it in two files, one per work unit (u033 up to
 * attach_tur_off, u034 from setting_attach_tur_stage1 on): merged here, in
 * address order.  Names: the hook (table entry of g_002A9080) each function
 * is registered under; see setting_hooks_internal.h.
 *
 * T081_static_init fills g_002A9080 with 161 {name, create, p8, pC, p10,
 * p14} entries sorted by name; SettingControl::begin (T080, 0x0015CA98)
 * hands the table to both layouts of its PageManager (PageLayout::setHooks).
 * Most entries are the CallbackTexts (create_callback_text) of the text_*
 * hooks and BoolItems (create_bool_item) shown while one of the predicates
 * holds:
 *   "attach_<category>_<part>": the image of the part fitted to the working
 *       copy of the setting car (SettingControl::car, a GarageCar at +0xE40),
 *       read with the part getters of its CarEquipments (the part ids of
 *       T482, PART_*);
 *   "disable_<setting>": the setting of the setting car cannot be adjusted
 *       (the T084 checks, inverted).
 *
 * Every text hook writes "----" when GameStatus has no setting car
 * (MenuStatus::raceCar[0]) or the setting cannot be adjusted on it (the T084
 * checks); the copy of "----" is inlined (5 bytes: lwl/lwr + lb, swl/swr +
 * sb), written strcpy here.  The values are the working copy's
 * (SettingControl::car, a GarageCar: its CarSaveEquipments fields).
 *
 * The setting car is read through GameStatus::getMenuStatus() (the listing
 * adds the 0x307C8 of `menu' before loading +0x540, as egcs does for the
 * inline accessor, not for `->menu.raceCar[0]').  The T084 checks take no
 * argument (h39): the car pointer is in $a0 when they are called only
 * because egcs loads it there for the NULL test (it does so for the (void)
 * call too).  The working copy's CarEquipments is a reference where the
 * listing keeps ctl + 0xE50 in a register (its CarInformation base at
 * GarageCar +0x10).  The operator new / delete of every `new' below is the
 * menu allocator (Object, 0x001BF3C0 / 0x001BF458); the landing pads in the
 * binary only free the object if its constructor throws.
 *
 * Compiled with egcs 991111b (-O2 -G8 -ffast-math -DBOOT_OLD_GCC;
 * ref/compile_flags.json), all 160 functions give the listing's instruction
 * words, and every data reference points where retail's does: the short
 * strings in .sdata 0x00351900.. ("%.1t%s" .. "blink", with the entry count
 * g_00351950 defined in its place before the static initialiser), the long
 * ones in .rodata 0x00320D90...  T081_static_init is written in an exact
 * form, not the original (see there).
 *
 * Includes: retail's type names (.rodata 0x00321AE8..0x00321D50, reverse
 * definition order) are SettingControl, SmartIndexT<int>, List<Button>,
 * Cell<Button>, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, RegurationResult, the garage lists (GarageWheelList
 * .. Cell<GarageCar>) and Operator (a non-polymorphic base: last): so the
 * GameStatus first, RegurationResult (no code here uses the class), the menu
 * item classes (T206: T216's operator classes, Button's base Cell<Button>),
 * List<Button>, SmartIndexT<int> after it (ButtonList's header: its
 * List<Button> base, then its `index' member, which instantiates
 * SmartIndexT<int>: T251_smart_index_template.h) and SettingControl last.
 * The headers after T080's add no class name. */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getMenuStatus; (seen: the
                                               garage classes) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#include "game/menu/framework/T206_button.h"               /* gt2k_menu::Button::getElement; (seen: T216's operator
                                               classes, Cell<Button>) */
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::ListIterator, List<Button> */
#include "game/menu/framework/T244_button_list.h"          /* (seen: ButtonList: List<Button>, then SmartIndexT<int>) */
#include "game/menu/car_settings/T080_setting_control.h"      /* gt2k_menu::SettingControl (car, pages, gearSliders,
                                               gearAutoLevel); T211 PageManager (buttonLists,
                                               popupLevel), T251 SmartIndexT<int>; T036_garage.h */
#include "game/menu/car_settings/T081_setting_hooks.h"        /* the hooks of this unit, g_002A9080, g_00351950,
                                               T081_static_init */
#include "libc.h"                           /* strcpy (core 0x0027EB84) */
#include <stddef.h>
#include "game/save/T427_menu_status.h"          /* MenuStatus::raceCar[0] (the setting car), event */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments: get*Record, get*Part, get*Name,
                                               isTurboByDefault, getName1, TransmissionRecord,
                                               TcsRecord, PART_*, TIRE_KIND_* */
#include "game/data/T483_car_information.h"      /* CarInformation::hasPart */
#include "game/data/T484_race_database.h"        /* RaceDataBase, RaceEvent::needTire */
#include "game/menu/car_settings/T084_setting_checks.h"       /* can_adjust_spring_rate .. can_adjust_vcd (can a setting be adjusted) */
#include "game/data/messages/T093_msg_setting.h"          /* g_002B1940, the SETTING texts (the units) */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* fmt_sprintf: Polyphony's printf with %t, an int
                                               printed as a fixed-point number with `precision'
                                               decimals ("%.1t" tenths, "%1.2t" hundredths,
                                               "%1.3t" thousandths; "%t" a plain integer) */
#include "game/menu/framework/T205_blink.h"                /* create_blink ("blink") */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (the "turbine_zero_name" picture) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hooks' result) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuHook(Fn) */
#include "game/menu/framework/T234_bool_item.h"            /* gt2k_menu::create_bool_item */
#include "game/menu/framework/T238_slider.h"               /* gt2k_menu::Slider::showPrev */
#include "game/menu/car_settings/T237_gear_chart.h"           /* gt2k_menu::create_gear_chart */
#include "game/menu/framework/T239_callback_text.h"        /* gt2k_menu::CallbackTextFn, create_callback_text */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList (its List<Button>) */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 (powerName) */
#include "game/menu/car_settings/setting_hooks_internal.h"

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* Suspension                                                               */
/* ------------------------------------------------------------------------ */

/* hook "spring_rate_f": "<rate>kgf/mm" (tenths). */
// FUNC 0x0015D448 text_spring_rate_f
void text_spring_rate_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_spring_rate()) {
        fmt_sprintf(buf, "%.1t%s", ctl->car.frontSpringrate, g_002B1940[SETTING_UNIT_KGF_MM]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_rate_r" */
// FUNC 0x0015D4E0 text_spring_rate_r
void text_spring_rate_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_spring_rate()) {
        fmt_sprintf(buf, "%.1t%s", ctl->car.rearSpringrate, g_002B1940[SETTING_UNIT_KGF_MM]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_height_f": ride height, "<height>mm". */
// FUNC 0x0015D578 text_spring_height_f
void text_spring_height_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_ride_height()) {
        fmt_sprintf(buf, "%t%s", ctl->car.frontRideheight, g_002B1940[SETTING_UNIT_MM]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_height_r" */
// FUNC 0x0015D610 text_spring_height_r
void text_spring_height_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_ride_height()) {
        fmt_sprintf(buf, "%t%s", ctl->car.rearRideheight, g_002B1940[SETTING_UNIT_MM]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_camber_f": "<camber><degree sign>" (tenths). */
// FUNC 0x0015D6A8 text_spring_camber_f
void text_spring_camber_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_camber()) {
        fmt_sprintf(buf, "%.1t%s", ctl->car.frontCamber, g_002B1940[SETTING_UNIT_DEGREE]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_camber_r" */
// FUNC 0x0015D740 text_spring_camber_r
void text_spring_camber_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_camber()) {
        fmt_sprintf(buf, "%.1t%s", ctl->car.rearCamber, g_002B1940[SETTING_UNIT_DEGREE]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_toe_f": the toe is stored biased by TOE_ZERO; shown as
 * (toe - TOE_ZERO) * 5 tenths with the degree sign. */
// FUNC 0x0015D7D8 text_spring_toe_f
void text_spring_toe_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_toe()) {
        fmt_sprintf(buf, "%.1t%s", (ctl->car.frontToe - TOE_ZERO) * 5, g_002B1940[SETTING_UNIT_DEGREE]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_toe_r" */
// FUNC 0x0015D880 text_spring_toe_r
void text_spring_toe_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_toe()) {
        fmt_sprintf(buf, "%.1t%s", (ctl->car.rearToe - TOE_ZERO) * 5, g_002B1940[SETTING_UNIT_DEGREE]);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_stabilizer_f": "Level <n>". */
// FUNC 0x0015D928 text_spring_stabilizer_f
void text_spring_stabilizer_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_stabiliser()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.frontStabilizer);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_stabilizer_r" */
// FUNC 0x0015D9C0 text_spring_stabilizer_r
void text_spring_stabilizer_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_stabiliser()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.rearStabilizer);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_brake_f": the brake balance controller's front level,
 * "Level <n>". */
// FUNC 0x0015DA58 text_spring_brake_f
void text_spring_brake_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_brake_balance()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.frontABS);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_brake_r" */
// FUNC 0x0015DAF0 text_spring_brake_r
void text_spring_brake_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_brake_balance()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.rearABS);
        return;
    }
    strcpy(buf, "----");
}

/* ------------------------------------------------------------------------ */
/* LSD                                                                      */
/* ------------------------------------------------------------------------ */
/* The "%d" texts pass an extra "" (0x00351928) that the format does not
 * use (as if each text had a unit). */

/* hook "lsd_int_f": initial torque. */
// FUNC 0x0015DB88 text_lsd_int_f
void text_lsd_int_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_initial()) {
        fmt_sprintf(buf, "%d", ctl->car.frontLSDparam, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "lsd_int_r": "0" (plain sprintf) when the LSD fitted is the AYC
 * controller (PART_LSD_AYC: it uses rearLSDparam as its own level, see
 * "elec_aycc"). */
// FUNC 0x0015DC20 text_lsd_int_r
void text_lsd_int_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_initial()) {
        CarEquipments &equipments = ctl->car;   /* ctl + 0xE50 */
        if (equipments.getLSDPart() == PART_LSD_AYC) {
            sprintf(buf, "%d", 0);
            return;
        }
        fmt_sprintf(buf, "%d", equipments.rearLSDparam, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "lsd_acl_f": acceleration sensitivity. */
// FUNC 0x0015DCE8 text_lsd_acl_f
void text_lsd_acl_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_accel()) {
        fmt_sprintf(buf, "%d", ctl->car.frontLSDparam2, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "lsd_acl_r" */
// FUNC 0x0015DD80 text_lsd_acl_r
void text_lsd_acl_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_accel()) {
        fmt_sprintf(buf, "%d", ctl->car.rearLSDparam2, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "lsd_dec_f": deceleration sensitivity. */
// FUNC 0x0015DE18 text_lsd_dec_f
void text_lsd_dec_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_decel()) {
        fmt_sprintf(buf, "%d", ctl->car.frontLSDparam3, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "lsd_dec_r" */
// FUNC 0x0015DEB0 text_lsd_dec_r
void text_lsd_dec_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_lsd_decel()) {
        fmt_sprintf(buf, "%d", ctl->car.rearLSDparam3, "");
        return;
    }
    strcpy(buf, "----");
}

/* ------------------------------------------------------------------------ */
/* Dampers                                                                  */
/* ------------------------------------------------------------------------ */
/* "Level <n>".  The damper texts (the sports / semi racing suspensions'
 * single damper level) and the bound texts (the racing suspension) show the
 * same field, the first-stage bound F1B, under different checks. */

/* hook "spring_damper_f" */
// FUNC 0x0015DF48 text_spring_damper_f
void text_spring_damper_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_damper()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.frontDamperF1B);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_damper_r" */
// FUNC 0x0015DFE0 text_spring_damper_r
void text_spring_damper_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_damper()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.rearDamperF1B);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_bound_f" */
// FUNC 0x0015E078 text_spring_bound_f
void text_spring_bound_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_bound_rebound()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.frontDamperF1B);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_bound_r" */
// FUNC 0x0015E110 text_spring_bound_r
void text_spring_bound_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_bound_rebound()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.rearDamperF1B);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_rebound_f": the first-stage rebound F1R. */
// FUNC 0x0015E1A8 text_spring_rebound_f
void text_spring_rebound_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_bound_rebound()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.frontDamperF1R);
        return;
    }
    strcpy(buf, "----");
}

/* hook "spring_rebound_r" */
// FUNC 0x0015E240 text_spring_rebound_r
void text_spring_rebound_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_bound_rebound()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.rearDamperF1R);
        return;
    }
    strcpy(buf, "----");
}

/* ------------------------------------------------------------------------ */
/* Gear ratios                                                              */
/* ------------------------------------------------------------------------ */
/* "<ratio>" in thousandths.  gearRatio[0] is the reverse gear,
 * gearRatio[1..gears] the forward ones (TransmissionRecord / CarSpec): ratio
 * N is shown while the transmission of the GameStatus car (not the working
 * copy; the record is not checked for NULL) has at least N gears.  Ratio 0
 * and the final ratio test gears >= 0, always true.  The gear count goes
 * through an int: egcs then keeps that test (lbu + bltz, as the listing
 * has), where a direct compare of the u8 is folded away. */

/* hook "gear_ratio_0" */
// FUNC 0x0015E2D8 text_gear_ratio_0
void text_gear_ratio_0(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 0) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[0]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_1" */
// FUNC 0x0015E370 text_gear_ratio_1
void text_gear_ratio_1(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 1) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[1]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_2" */
// FUNC 0x0015E408 text_gear_ratio_2
void text_gear_ratio_2(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 2) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[2]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_3" */
// FUNC 0x0015E4A0 text_gear_ratio_3
void text_gear_ratio_3(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 3) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[3]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_4" */
// FUNC 0x0015E538 text_gear_ratio_4
void text_gear_ratio_4(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 4) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[4]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_5" */
// FUNC 0x0015E5D0 text_gear_ratio_5
void text_gear_ratio_5(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 5) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[5]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_6" */
// FUNC 0x0015E668 text_gear_ratio_6
void text_gear_ratio_6(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 6) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[6]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_7" */
// FUNC 0x0015E700 text_gear_ratio_7
void text_gear_ratio_7(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 7) {
            fmt_sprintf(buf, "%1.3t", ctl->car.gearRatio[7]);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_final" */
// FUNC 0x0015E798 text_gear_ratio_final
void text_gear_ratio_final(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int gears = settingCar->getGearRecord()->gears;
        if (gears >= 0) {
            fmt_sprintf(buf, "%1.3t", ctl->car.finalGearRatio);
            return;
        }
    }
    strcpy(buf, "----");
}

/* hook "gear_ratio_auto": the auto level, "Level <n>" (plain sprintf), when
 * the working copy's transmission has a range of them (maxSpeedMin !=
 * maxSpeedMax; the record is fetched twice, not checked for NULL). */
// FUNC 0x0015E830 text_gear_ratio_auto
void text_gear_ratio_auto(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &equipments = ctl->car;   /* ctl + 0xE50 */
        u8 levelMin = equipments.getGearRecord()->maxSpeedMin;
        if (levelMin != equipments.getGearRecord()->maxSpeedMax) {
            sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], equipments.maxSpeed);
            return;
        }
    }
    strcpy(buf, "----");
}

/* ------------------------------------------------------------------------ */
/* Aero, electronic controllers                                             */
/* ------------------------------------------------------------------------ */

/* hook "aero_f": the downforce, "<clF>" in hundredths (extra "" argument). */
// FUNC 0x0015E8E0 text_aero_f
void text_aero_f(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_aero()) {
        fmt_sprintf(buf, "%1.2t", ctl->car.clF, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "aero_r" */
// FUNC 0x0015E978 text_aero_r
void text_aero_r(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_aero()) {
        fmt_sprintf(buf, "%1.2t", ctl->car.clR, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "elec_aycc": the AYC controller's level is the rear LSD initial
 * torque field (rearLSDparam). */
// FUNC 0x0015EA10 text_elec_aycc
void text_elec_aycc(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_ayc()) {
        fmt_sprintf(buf, "%d", ctl->car.rearLSDparam, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "elec_ascc": "Level <settingASCC - 1>". */
// FUNC 0x0015EAA8 text_elec_ascc
void text_elec_ascc(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_asm()) {
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL], ctl->car.settingASCC - 1);
        return;
    }
    strcpy(buf, "----");
}

/* hook "elec_tcsc": the level reversed, "Level <count - settingTCSC>"
 * (TcsRecord of the working copy, not checked for NULL). */
// FUNC 0x0015EB48 text_elec_tcsc
void text_elec_tcsc(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_tcs()) {
        CarEquipments &equipments = ctl->car;   /* ctl + 0xE50 */
        fmt_sprintf(buf, "%s %d", g_002B1940[SETTING_UNIT_LEVEL],
                    equipments.getTCSCRecord()->count - equipments.settingTCSC);
        return;
    }
    strcpy(buf, "----");
}

/* hook "elec_vcdc": the variable centre differential, "<param4WD> %"
 * (extra "" argument). */
// FUNC 0x0015EC10 text_elec_vcdc
void text_elec_vcdc(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && can_adjust_vcd()) {
        fmt_sprintf(buf, "%d %%", ctl->car.param4WD, "");
        return;
    }
    strcpy(buf, "----");
}

/* hook "gear_chart": p8 of create_gear_chart (T237): the car whose gears
 * the GearChart draws, the working copy; NULL when there is no setting car. */
// FUNC 0x0015ECA8 gear_chart_car
GarageCar *gear_chart_car(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return NULL;
    return &ctl->car;
}

/* ------------------------------------------------------------------------ */
/* Names of the fitted parts                                                */
/* ------------------------------------------------------------------------ */
/* The name of the working copy's part of the hook's category (CarEquipments
 * get*Name, T482: the short names of getPartName), "----" when there is no
 * setting car. */

/* hook "parts_eng_computer" */
// FUNC 0x0015ECE8 text_parts_eng_computer
void text_parts_eng_computer(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getComputerName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_muffler" */
// FUNC 0x0015ED68 text_parts_eng_muffler
void text_parts_eng_muffler(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getMufflerName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_turbine" */
// FUNC 0x0015EDE8 text_parts_eng_turbine
void text_parts_eng_turbine(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getTurbineName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_inter_cooler" */
// FUNC 0x0015EE68 text_parts_eng_inter_cooler
void text_parts_eng_inter_cooler(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getIntercoolerName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_displacement" */
// FUNC 0x0015EEE8 text_parts_eng_displacement
void text_parts_eng_displacement(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getDisplacementName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_na_tune" */
// FUNC 0x0015EF68 text_parts_eng_na_tune
void text_parts_eng_na_tune(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getNATuneName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_port_polish" */
// FUNC 0x0015EFE8 text_parts_eng_port_polish
void text_parts_eng_port_polish(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getPortpolishName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_eng_engine_balance" */
// FUNC 0x0015F068 text_parts_eng_engine_balance
void text_parts_eng_engine_balance(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getEnginebalanceName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_sus_s_kit": the suspension kit. */
// FUNC 0x0015F0E8 text_parts_sus_s_kit
void text_parts_sus_s_kit(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getSuspensionName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_sus_b_kit": the brakes. */
// FUNC 0x0015F168 text_parts_sus_b_kit
void text_parts_sus_b_kit(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getBrakeName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_sus_b_balance": the brake balance controller. */
// FUNC 0x0015F1E8 text_parts_sus_b_balance
void text_parts_sus_b_balance(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getBrakecontrollerName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_drv_mission": the transmission. */
// FUNC 0x0015F268 text_parts_drv_mission
void text_parts_drv_mission(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getGearName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_drv_clutch" */
// FUNC 0x0015F2E8 text_parts_drv_clutch
void text_parts_drv_clutch(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getClutchName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_drv_fly": the flywheel. */
// FUNC 0x0015F368 text_parts_drv_fly
void text_parts_drv_fly(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getFlywheelName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_drv_lsd" */
// FUNC 0x0015F3E8 text_parts_drv_lsd
void text_parts_drv_lsd(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getLSDName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_drv_shaft": the propeller shaft. */
// FUNC 0x0015F468 text_parts_drv_shaft
void text_parts_drv_shaft(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getPropellershaftName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_oth_asm_s": the active stability management (ASCC). */
// FUNC 0x0015F4E8 text_parts_oth_asm_s
void text_parts_oth_asm_s(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getASCCName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_oth_tcs_s": the traction control (TCSC). */
// FUNC 0x0015F568 text_parts_oth_tcs_s
void text_parts_oth_tcs_s(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getTCSCName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_oth_vcd_s": the drive train (the variable centre
 * differential). */
// FUNC 0x0015F5E8 text_parts_oth_vcd_s
void text_parts_oth_vcd_s(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getDrivetrainName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_oth_weight_r": the weight reduction. */
// FUNC 0x0015F668 text_parts_oth_weight_r
void text_parts_oth_weight_r(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        strcpy(buf, ctl->car.getLightweightName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_tir_f": the front tyres' name.  In a race on dirt tyres only
 * (the current event, MenuStatus::event, looked up in a RaceDataBase;
 * RaceEvent::needTire NEED_TIRE_DIRT) the full name of the dirt
 * tyres (PART_FRONT_TIRE_DIRT) if the working copy owns them, else "----".
 * The RaceDataBase is destroyed (its inline destructor, flags 2) before the
 * tyre name is read. */
// FUNC 0x0015F6E8 text_parts_tir_f
void text_parts_tir_f(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        {
            const char *event = getGameStatus()->getMenuStatus()->event;
            RaceDataBase races;                 /* sp+0x00 */
            if (races.select(nameToCode(event)) && races.event->needTire == NEED_TIRE_DIRT) {
                if (ctl->car.hasPart(PART_FRONT_TIRE_DIRT))
                    strcpy(buf, CarEquipments::getPartFullName(PART_FRONT_TIRE_DIRT));
                else
                    strcpy(buf, "----");
                return;
            }
        }
        strcpy(buf, ctl->car.getFronttireName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_tir_r": as "parts_tir_f" with the rear tyres; the dirt race
 * case also tests and names PART_FRONT_TIRE_DIRT (0x42), not
 * PART_REAR_TIRE_DIRT (0x4D): both full names are "dirt tyres"
 * (PTEXT_TIRE_DIRT), so only the ownership test differs. */
// FUNC 0x0015F848 text_parts_tir_r
void text_parts_tir_r(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        {
            const char *event = getGameStatus()->getMenuStatus()->event;
            RaceDataBase races;                 /* sp+0x00 */
            if (races.select(nameToCode(event)) && races.event->needTire == NEED_TIRE_DIRT) {
                if (ctl->car.hasPart(PART_FRONT_TIRE_DIRT))
                    strcpy(buf, CarEquipments::getPartFullName(PART_FRONT_TIRE_DIRT));
                else
                    strcpy(buf, "----");
                return;
            }
        }
        strcpy(buf, ctl->car.getReartireName());
        return;
    }
    strcpy(buf, "----");
}

/* hook "parts_oth_ayc_s": whether the AYC controller is fitted (the LSD
 * part PART_LSD_AYC): the short names of the ASCC part ids, PART_ASCC
 * (PTEXT_EQUIP, "fitted") or PART_ASCC_NONE (PTEXT_NOTHING, "none").  (Two
 * calls in the source: egcs merges their tails, leaving the part id in the
 * branch's delay slot.) */
// FUNC 0x0015F9A8 text_parts_oth_ayc_s
void text_parts_oth_ayc_s(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        if (ctl->car.getLSDPart() == PART_LSD_AYC)
            strcpy(buf, CarEquipments::getPartName(PART_ASCC));
        else
            strcpy(buf, CarEquipments::getPartName(PART_ASCC_NONE));
        return;
    }
    strcpy(buf, "----");
}

/* ------------------------------------------------------------------------ */
/* "attach_*" predicates (p8 of create_bool_item).  0 without a setting car  */
/* (GameStatus's raceCar[0]), else whether the working copy's part of the    */
/* category is the one of the image.  (The first ones are named after their */
/* hook, the rest keep their setting_ prefix.)                              */
/* ------------------------------------------------------------------------ */

/* hook "attach_cpu_off": the stock computer. */
// FUNC 0x0015FA40 attach_cpu_off
int attach_cpu_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getComputerPart() == PART_COMPUTER;
}

/* hook "attach_cpu_on": the tuned ROM. */
// FUNC 0x0015FA90 attach_cpu_on
int attach_cpu_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getComputerPart() == PART_COMPUTER_ROM;
}

/* hook "attach_muf_normal" */
// FUNC 0x0015FAE0 attach_muf_normal
int attach_muf_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getMufflerPart() == PART_MUFFLER;
}

/* hook "attach_muf_sports" */
// FUNC 0x0015FB30 attach_muf_sports
int attach_muf_sports(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getMufflerPart() == PART_MUFFLER_SPORTS;
}

/* hook "attach_muf_semi_racing" */
// FUNC 0x0015FB80 attach_muf_semi_racing
int attach_muf_semi_racing(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getMufflerPart() == PART_MUFFLER_SEMI_RACING;
}

/* hook "attach_muf_racing" */
// FUNC 0x0015FBD0 attach_muf_racing
int attach_muf_racing(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getMufflerPart() == PART_MUFFLER_RACING;
}

/* turbine kit ("tur") */

/* hook "attach_tur_off": no turbine kit (kind 0; stages 1..4 follow). */
// FUNC 0x0015FC20 attach_tur_off
int attach_tur_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTurbinePart() == PART_TURBINE;
}

// FUNC 0x0015FC70 setting_attach_tur_stage1
int setting_attach_tur_stage1(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTurbinePart() == PART_TURBINE_STAGE1;
}

// FUNC 0x0015FCC0 setting_attach_tur_stage2
int setting_attach_tur_stage2(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTurbinePart() == PART_TURBINE_STAGE2;
}

// FUNC 0x0015FD10 setting_attach_tur_stage3
int setting_attach_tur_stage3(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTurbinePart() == PART_TURBINE_STAGE3;
}

// FUNC 0x0015FD60 setting_attach_tur_stage4
int setting_attach_tur_stage4(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTurbinePart() == PART_TURBINE_STAGE4;
}

/* intercooler ("coo"; getIntercoolerPart is PART_NONE unless the car is a
 * turbo in stock form) */

// FUNC 0x0015FDB0 setting_attach_coo_normal
int setting_attach_coo_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getIntercoolerPart() == PART_INTERCOOLER;
}

// FUNC 0x0015FE00 setting_attach_coo_1
int setting_attach_coo_1(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getIntercoolerPart() == PART_INTERCOOLER_SPORTS;
}

// FUNC 0x0015FE50 setting_attach_coo_2
int setting_attach_coo_2(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getIntercoolerPart() == PART_INTERCOOLER_RACING;
}

/* NA tune ("nat") */

// FUNC 0x0015FEA0 setting_attach_nat_off
int setting_attach_nat_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getNATunePart() == PART_NA_TUNE_NONE;
}

// FUNC 0x0015FEF0 setting_attach_nat_stage1
int setting_attach_nat_stage1(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getNATunePart() == PART_NA_TUNE_STAGE1;
}

// FUNC 0x0015FF40 setting_attach_nat_stage2
int setting_attach_nat_stage2(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getNATunePart() == PART_NA_TUNE_STAGE2;
}

// FUNC 0x0015FF90 setting_attach_nat_stage3
int setting_attach_nat_stage3(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getNATunePart() == PART_NA_TUNE_STAGE3;
}

/* suspension ("sus"; the hook's "racing" is the full custom kit) */

// FUNC 0x0015FFE0 setting_attach_sus_normal
int setting_attach_sus_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getSuspensionPart() == PART_SUSPENSION;
}

// FUNC 0x00160030 setting_attach_sus_sports
int setting_attach_sus_sports(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getSuspensionPart() == PART_SUSPENSION_SPORTS;
}

// FUNC 0x00160080 setting_attach_sus_semi_racing
int setting_attach_sus_semi_racing(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getSuspensionPart() == PART_SUSPENSION_SEMI_RACING;
}

// FUNC 0x001600D0 setting_attach_sus_racing
int setting_attach_sus_racing(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getSuspensionPart() == PART_SUSPENSION_FULL_CUSTOM;
}

/* brakes ("brk") */

// FUNC 0x00160120 setting_attach_brk_normal
int setting_attach_brk_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getBrakePart() == PART_BRAKE;
}

// FUNC 0x00160168 setting_attach_brk_racing
int setting_attach_brk_racing(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getBrakePart() == PART_BRAKE_RACING;
}

/* brake balance controller ("brc") */

// FUNC 0x001601B8 setting_attach_brc_off
int setting_attach_brc_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getBrakecontrollerPart() == PART_BRAKE_CONTROLLER_NONE;
}

// FUNC 0x00160208 setting_attach_brc_on
int setting_attach_brc_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getBrakecontrollerPart() == PART_BRAKE_CONTROLLER;
}

/* transmission ("msn"; "variable" is the full custom gearbox) */

// FUNC 0x00160258 setting_attach_msn_normal
int setting_attach_msn_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getGearPart() == PART_GEAR;
}

// FUNC 0x001602A8 setting_attach_msn_close
int setting_attach_msn_close(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getGearPart() == PART_GEAR_CLOSE;
}

// FUNC 0x001602F8 setting_attach_msn_super_close
int setting_attach_msn_super_close(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getGearPart() == PART_GEAR_SUPER_CLOSE;
}

// FUNC 0x00160348 setting_attach_msn_variable
int setting_attach_msn_variable(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getGearPart() == PART_GEAR_FULL_CUSTOM;
}

/* clutch ("clt"; "big" is the sports clutch) */

// FUNC 0x00160398 setting_attach_clt_normal
int setting_attach_clt_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getClutchPart() == PART_CLUTCH;
}

// FUNC 0x001603E8 setting_attach_clt_big
int setting_attach_clt_big(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getClutchPart() == PART_CLUTCH_SPORTS;
}

// FUNC 0x00160438 setting_attach_clt_twin
int setting_attach_clt_twin(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getClutchPart() == PART_CLUTCH_TWIN;
}

// FUNC 0x00160488 setting_attach_clt_triple
int setting_attach_clt_triple(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getClutchPart() == PART_CLUTCH_TRIPLE;
}

/* flywheel ("fly"; the hooks' light / cr-mo / light cr-mo are the sports,
 * semi racing and racing flywheels of T482) */

// FUNC 0x001604D8 setting_attach_fly_normal
int setting_attach_fly_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFlywheelPart() == PART_FLYWHEEL;
}

// FUNC 0x00160528 setting_attach_fly_light
int setting_attach_fly_light(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFlywheelPart() == PART_FLYWHEEL_SPORTS;
}

// FUNC 0x00160578 setting_attach_fly_crmo
int setting_attach_fly_crmo(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFlywheelPart() == PART_FLYWHEEL_SEMI_RACING;
}

// FUNC 0x001605C8 setting_attach_fly_light_crmo
int setting_attach_fly_light_crmo(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFlywheelPart() == PART_FLYWHEEL_RACING;
}

/* LSD ("variable" is the full custom LSD; "aycc" the AYC controller) */

// FUNC 0x00160618 setting_attach_lsd_normal
int setting_attach_lsd_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD;
}

// FUNC 0x00160668 setting_attach_lsd_2way
int setting_attach_lsd_2way(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_2WAY;
}

// FUNC 0x001606B8 setting_attach_lsd_15way
int setting_attach_lsd_15way(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_1_5WAY;
}

// FUNC 0x00160708 setting_attach_lsd_1way
int setting_attach_lsd_1way(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_1WAY;
}

// FUNC 0x00160758 setting_attach_lsd_variable
int setting_attach_lsd_variable(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_FULL_CUSTOM;
}

// FUNC 0x001607A8 setting_attach_lsd_aycc
int setting_attach_lsd_aycc(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_AYC;
}

/* propeller shaft ("shf"; "propeller" is the carbon shaft;
 * getPropellershaftPart is PART_NONE for FF, MR and RR cars) */

// FUNC 0x001607F8 setting_attach_shf_normal
int setting_attach_shf_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getPropellershaftPart() == PART_PROPELLER_SHAFT;
}

// FUNC 0x00160848 setting_attach_shf_propeller
int setting_attach_shf_propeller(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getPropellershaftPart() == PART_PROPELLER_SHAFT_CARBON;
}

/* front tyres: PART_FRONT_TIRE + the tyre kind (no hook for the dirt
 * tyres, kind 0xC) */

// FUNC 0x00160898 setting_attach_tir_f_normal
int setting_attach_tir_f_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_NORMAL;
}

// FUNC 0x001608E8 setting_attach_tir_f_sports
int setting_attach_tir_f_sports(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_SPORTS;
}

// FUNC 0x00160938 setting_attach_tir_f_racing_sh
int setting_attach_tir_f_racing_sh(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_SH;
}

// FUNC 0x00160988 setting_attach_tir_f_racing_h
int setting_attach_tir_f_racing_h(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_H;
}

// FUNC 0x001609D8 setting_attach_tir_f_racing_mh
int setting_attach_tir_f_racing_mh(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_MH;
}

// FUNC 0x00160A28 setting_attach_tir_f_racing_m
int setting_attach_tir_f_racing_m(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_M;
}

// FUNC 0x00160A78 setting_attach_tir_f_racing_ms
int setting_attach_tir_f_racing_ms(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_MS;
}

// FUNC 0x00160AC8 setting_attach_tir_f_racing_s
int setting_attach_tir_f_racing_s(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_S;
}

// FUNC 0x00160B18 setting_attach_tir_f_racing_ss
int setting_attach_tir_f_racing_ss(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_RACING_SS;
}

// FUNC 0x00160B68 setting_attach_tir_f_control
int setting_attach_tir_f_control(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getFronttirePart() == PART_FRONT_TIRE + TIRE_KIND_CONTROL;
}

/* rear tyres: PART_REAR_TIRE + the tyre kind */

// FUNC 0x00160BB8 setting_attach_tir_r_normal
int setting_attach_tir_r_normal(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_NORMAL;
}

// FUNC 0x00160C08 setting_attach_tir_r_sports
int setting_attach_tir_r_sports(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_SPORTS;
}

// FUNC 0x00160C58 setting_attach_tir_r_racing_sh
int setting_attach_tir_r_racing_sh(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_SH;
}

// FUNC 0x00160CA8 setting_attach_tir_r_racing_h
int setting_attach_tir_r_racing_h(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_H;
}

// FUNC 0x00160CF8 setting_attach_tir_r_racing_mh
int setting_attach_tir_r_racing_mh(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_MH;
}

// FUNC 0x00160D48 setting_attach_tir_r_racing_m
int setting_attach_tir_r_racing_m(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_M;
}

// FUNC 0x00160D98 setting_attach_tir_r_racing_ms
int setting_attach_tir_r_racing_ms(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_MS;
}

// FUNC 0x00160DE8 setting_attach_tir_r_racing_s
int setting_attach_tir_r_racing_s(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_S;
}

// FUNC 0x00160E38 setting_attach_tir_r_racing_ss
int setting_attach_tir_r_racing_ss(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_RACING_SS;
}

// FUNC 0x00160E88 setting_attach_tir_r_control
int setting_attach_tir_r_control(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getReartirePart() == PART_REAR_TIRE + TIRE_KIND_CONTROL;
}

/* AYC: the AYC controller is an LSD part (PART_LSD_AYC, as
 * "attach_lsd_aycc") */

/* The only predicate that is 1 without a setting car (the "AYC off" image is
 * the default). */
// FUNC 0x00160ED8 setting_attach_ayc_off
int setting_attach_ayc_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 1;
    return ctl->car.getLSDPart() != PART_LSD_AYC;
}

// FUNC 0x00160F28 setting_attach_ayc_on
int setting_attach_ayc_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getLSDPart() == PART_LSD_AYC;
}

/* ASM ("sms": the active stability management, T482's ASCC) */

// FUNC 0x00160F78 setting_attach_sms_off
int setting_attach_sms_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getASCCPart() == PART_ASCC_NONE;
}

// FUNC 0x00160FC8 setting_attach_sms_on
int setting_attach_sms_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getASCCPart() == PART_ASCC;
}

/* TCS (T482's TCSC) */

// FUNC 0x00161018 setting_attach_tcs_off
int setting_attach_tcs_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTCSCPart() == PART_TCSC_NONE;
}

// FUNC 0x00161068 setting_attach_tcs_on
int setting_attach_tcs_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getTCSCPart() == PART_TCSC;
}

/* VCD: the variable centre differential, a drive train part */

// FUNC 0x001610B8 setting_attach_vcd_off
int setting_attach_vcd_off(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getDrivetrainPart() == PART_DRIVETRAIN;
}

// FUNC 0x00161108 setting_attach_vcd_on
int setting_attach_vcd_on(SettingControl *ctl)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    return ctl->car.getDrivetrainPart() == PART_DRIVETRAIN_VCD;
}

/* ------------------------------------------------------------------------ */
/* "gear_setup"                                                             */
/* ------------------------------------------------------------------------ */
/* Placed on the gear ratio page: creates nothing (NULL), but finds the
 * buttons of the current button list (pages.buttonLists[pages.popupLevel])
 * by the name of their layout item and keeps them in the SettingControl:
 * the sliders "slider_gear_ratio_1".."_7" and "slider_gear_ratio_final"
 * (dynamic_cast from Button: NULL if the button of that name is not a
 * Slider) and the button "slider_gear_auto_level".  The ratio sliders 1..7
 * do not draw their initial value ("pinch_prev": showPrev 0); the final
 * drive slider keeps it.  The item is fetched again (getElement) for every
 * comparison; a NULL button list is converted to a NULL List<Button>, whose
 * head is read anyway.  The stores of the first loop run from [7] down to
 * [0] in the binary (egcs reverses the loop; the same stores).
 *
 * The name tests sit in a `do { } while (0)' block left by `break' at the
 * first match, each ratio slider clearing its own showPrev (the inline
 * Slider::setShowPrev, 0x001C6C60): the listing shows that inner loop.
 * egcs moved the first test with its `break' (an exit within the loop's
 * first 30 insns) behind the others (stmt.c expand_end_loop), so retail
 * jumps to "slider_gear_ratio_1" from the loop top and falls from it into
 * the showPrev test, which jump2 shares among the seven ratio sliders
 * (cross-jumped tails).  The block's loop notes end CSE's extended block, so
 * the button is tested for NULL again before every dynamic_cast, and loop.c
 * moves the two typeinfo addresses only to the block's preheader (`lui' at
 * the top of each pass), not out of the outer loop.  A plain if / else chain
 * gave 59..64%.
 * &gearSliders[0] is kept in a register for the stores (`gear').  unk1094 is
 * cleared before unk10B8: the two independent stores come out in the
 * reverse order (retail stores 0x10B8 first; written the other way round
 * they come out 0x1094 first). */
// FUNC 0x00161158 setting_create_gear_setup
Transitive *setting_create_gear_setup(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    SettingControl *ctl = (SettingControl *)ctx;
    Slider **gear = ctl->gearSliders;

    for (int i = 0; i < 8; i++)
        gear[i] = NULL;
    ctl->gearAutoLevel = NULL;

    ListIterator<Button> it(ctl->pages.buttonLists[ctl->pages.popupLevel]);
    Button *button;
    while ((button = it.next()) != NULL) {
        do {
            if (str_equal(button->getElement()->name, "slider_gear_ratio_1")) {
                gear[0] = dynamic_cast<Slider *>(button);
                if (gear[0] != NULL)
                    gear[0]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_2")) {
                gear[1] = dynamic_cast<Slider *>(button);
                if (gear[1] != NULL)
                    gear[1]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_3")) {
                gear[2] = dynamic_cast<Slider *>(button);
                if (gear[2] != NULL)
                    gear[2]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_4")) {
                gear[3] = dynamic_cast<Slider *>(button);
                if (gear[3] != NULL)
                    gear[3]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_5")) {
                gear[4] = dynamic_cast<Slider *>(button);
                if (gear[4] != NULL)
                    gear[4]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_6")) {
                gear[5] = dynamic_cast<Slider *>(button);
                if (gear[5] != NULL)
                    gear[5]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_7")) {
                gear[6] = dynamic_cast<Slider *>(button);
                if (gear[6] != NULL)
                    gear[6]->setShowPrev(0);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_ratio_final")) {
                gear[7] = dynamic_cast<Slider *>(button);
                break;
            }
            if (str_equal(button->getElement()->name, "slider_gear_auto_level")) {
                ctl->gearAutoLevel = button;
                break;
            }
        } while (0);
    }

    /* An empty loop of 8 iterations in the binary (counter 7 down to 0, no
     * body; egcs pads the short loop with nops).  ? a loop over the sliders
     * whose body was compiled out. */
    for (int i = 0; i < 8; i++)
        ;

    ctl->unk1094 = 0;
    ctl->unk10B8 = 0;
    return NULL;
}

/* ------------------------------------------------------------------------ */
/* "disable_*" predicates (p8 of create_bool_item): the setting of the      */
/* setting car cannot be adjusted.  The listing inverts the T084 check with */
/* xori 1, egcs's `!' of a bool: the checks return 0 / 1 (bool in the       */
/* original; T084 declares them int), so `^ 1' is that `!'.                */
/* ------------------------------------------------------------------------ */

// FUNC 0x00161530 setting_disable_spring_rate
int setting_disable_spring_rate(SettingControl *ctl)
{
    return can_adjust_spring_rate() ^ 1;
}

// FUNC 0x00161550 setting_disable_ride_height
int setting_disable_ride_height(SettingControl *ctl)
{
    return can_adjust_ride_height() ^ 1;
}

// FUNC 0x00161570 setting_disable_damper
int setting_disable_damper(SettingControl *ctl)
{
    return can_adjust_damper() ^ 1;
}

/* Registered as both "disable_bound" and "disable_rebound". */
// FUNC 0x00161590 setting_disable_bound
int setting_disable_bound(SettingControl *ctl)
{
    return can_adjust_bound_rebound() ^ 1;
}

// FUNC 0x001615B0 setting_disable_camber
int setting_disable_camber(SettingControl *ctl)
{
    return can_adjust_camber() ^ 1;
}

// FUNC 0x001615D0 setting_disable_toe
int setting_disable_toe(SettingControl *ctl)
{
    return can_adjust_toe() ^ 1;
}

// FUNC 0x001615F0 setting_disable_stabilizer
int setting_disable_stabilizer(SettingControl *ctl)
{
    return can_adjust_stabiliser() ^ 1;
}

// FUNC 0x00161610 setting_disable_brake_balance
int setting_disable_brake_balance(SettingControl *ctl)
{
    return can_adjust_brake_balance() ^ 1;
}

// FUNC 0x00161630 setting_disable_lsd_initial
int setting_disable_lsd_initial(SettingControl *ctl)
{
    return can_adjust_lsd_initial() ^ 1;
}

// FUNC 0x00161650 setting_disable_lsd_accel
int setting_disable_lsd_accel(SettingControl *ctl)
{
    return can_adjust_lsd_accel() ^ 1;
}

// FUNC 0x00161670 setting_disable_lsd_decrease
int setting_disable_lsd_decrease(SettingControl *ctl)
{
    return can_adjust_lsd_decel() ^ 1;
}

// FUNC 0x00161690 setting_disable_gear_ratio
int setting_disable_gear_ratio(SettingControl *ctl)
{
    return can_adjust_final_gear() ^ 1;
}

// FUNC 0x001616B0 setting_disable_aero
int setting_disable_aero(SettingControl *ctl)
{
    return can_adjust_aero() ^ 1;
}

// FUNC 0x001616D0 setting_disable_ayc
int setting_disable_ayc(SettingControl *ctl)
{
    return can_adjust_ayc() ^ 1;
}

// FUNC 0x001616F0 setting_disable_asm
int setting_disable_asm(SettingControl *ctl)
{
    return can_adjust_asm() ^ 1;
}

// FUNC 0x00161710 setting_disable_tcs
int setting_disable_tcs(SettingControl *ctl)
{
    return can_adjust_tcs() ^ 1;
}

// FUNC 0x00161730 setting_disable_vcd
int setting_disable_vcd(SettingControl *ctl)
{
    return can_adjust_vcd() ^ 1;
}

/* ------------------------------------------------------------------------ */
/* More hooks                                                               */
/* ------------------------------------------------------------------------ */

/* "turbine_zero_name": the name picture of turbine kit stage 0, an Item of
 * the layout's hidden picture "name_normal" if the setting car is a turbo
 * in stock form (isTurboByDefault: its stage 0 is the stock turbo), else
 * "name_none" (also without a setting car), placed on the hook's item. */
// FUNC 0x00161750 setting_create_turbine_zero_name
Transitive *setting_create_turbine_zero_name(void *ctx, PageLayout *layout, Object *parent,
                                             MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL && settingCar->isTurboByDefault())
        return new Item("name_normal", layout, item); /* implicit: Object::operator delete (if the constructor throws) */
    return new Item("name_none", layout, item); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "user_carname" (p8 of create_callback_text): name 1 of the setting car
 * (CarEquipments::getName1), "" without one. */
// FUNC 0x00161840 setting_text_user_carname
void setting_text_user_carname(SettingControl *ctl, char *buf, int size)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL)
        strcpy(buf, settingCar->getName1());
    else
        buf[0] = '\0';
}

/* "car_current_power" (p8 of create_callback_text): "<power> <unit>" of the
 * working copy (GarageCar::getPower; its rpm is not shown; the power unit of
 * the unit settings, T518); "" without a setting car. */
// FUNC 0x00161898 setting_text_car_current_power
void setting_text_car_current_power(SettingControl *ctl, char *buf, int size)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        int power, rpm;                     /* sp+0x0, sp+0x4 */
        ctl->car.getPower(&power, &rpm);
        sprintf(buf, "%d %s", power, g_002F59D8.powerName());
    } else {
        buf[0] = '\0';
    }
}

/* ------------------------------------------------------------------------ */
/* The hook table                                                           */
/* ------------------------------------------------------------------------ */
/* The table, .data 0x002A9080..0x002A9F98 (161 entries of 0x18 bytes): no
 * initialiser, so egcs writes its zeros in .data in front of the frame info
 * (retail's are zero too: the static initialiser below fills it at boot; as
 * T036's g_00296B90, T064's g_002A0AD0). */
MenuHook g_002A9080[161];                   /* 0x002A9080 */

/* The table's entry count, .sdata 0x00351950: a constant word (161), read
 * by SettingControl::begin (T080).  Defined here, before the initialiser,
 * where the original defined it: its place in .sdata, between "%d %s" of
 * car_current_power and the table's short names ("aero_f", ...), shows it. */
const int g_00351950 = sizeof(g_002A9080) / sizeof(g_002A9080[0]);

/* One table entry: the inline constructor of the original's MenuHook (see
 * T081_static_init); the stores in member order.  Not a function of the
 * binary. */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* The library hooks take fewer or typed parameters (p8 as what it is): the
 * table holds them as MenuHookFn.  The callbacks go into p8, an int. */
#define HOOK_BOOL_ITEM      create_bool_item        /* T234 p8 a predicate, pC 0: every draw */
#define HOOK_CALLBACK_TEXT  create_callback_text    /* T239 p8 a text, pC 0: every draw */
#define HOOK_GEAR_CHART     create_gear_chart       /* T237 p8 the car */
#define HOOK_BLINK          create_blink            /* T205 */

/* One element as egcs's build_vec_init expands it: the constructor called
 * on a copy of the cursor (`this'), then the cursor and the count of the
 * elements left stepped (pC, p10 and p14 are 0 in every entry).  (No
 * do-while wrapper: its loop notes would change the code.) */
#define ENTRY(name, create, p8)                                               \
    { MenuHook *e = base; set_entry(e, name, create, p8, 0, 0, 0); }        \
    base++;                                                                   \
    left--

/* Static initialiser of the hook table g_002A9080 (.ctors entry
 * 0x0028F0A0): a compiler-generated _GLOBAL_$I$ function in the original.
 * The names are sorted (PageLayout::findHook binary-searches them); entry i
 * is at 0x002A9080 + 0x18 * i.  "disable_bound" and "disable_rebound" share
 * one predicate.
 *
 * The original defined the table here, with MenuHook objects built by a
 * constructor, e.g. MenuHook("blink", create_blink, 0, 0, 0, 0): egcs builds
 * such a table at run time, in this function (cp/init.c build_vec_init),
 * and leaves .data zero.  Written that way (a MenuHook with an inline
 * constructor and the table definition; build/scratch/u034/gen.py "ctor")
 * the function compiles to exactly these instructions.  The decomp does not
 * define the table (it stays at its original address), so the function is
 * written out: an exact form, not the original (as u006's T030 and u012's
 * T036 initialisers).  It spells out what build_vec_init expands: the
 * array's address as the result value (`table', dead) and as the cursor,
 * the iterator (`left', dead: flow deletes both), and per element a copy of
 * the cursor; without `table' 58.9%, indexing the table (the second pass's
 * form) 9.9%.  A plain `p->name = ...; p++' walk stores the same 161 x 6
 * words. */
// FUNC 0x00161930 T081_static_init
void T081_static_init(void)
{
    MenuHook *table = g_002A9080;           /* build_vec_init's result value (dead) */
    MenuHook *base = g_002A9080;            /* its cursor */
    int left = 160;                         /* its iterator: elements - 1 (dead) */

    ENTRY("aero_f",                    HOOK_CALLBACK_TEXT, (int)text_aero_f);
    ENTRY("aero_r",                    HOOK_CALLBACK_TEXT, (int)text_aero_r);
    ENTRY("attach_ayc_off",            HOOK_BOOL_ITEM, (int)setting_attach_ayc_off);
    ENTRY("attach_ayc_on",             HOOK_BOOL_ITEM, (int)setting_attach_ayc_on);
    ENTRY("attach_brc_off",            HOOK_BOOL_ITEM, (int)setting_attach_brc_off);
    ENTRY("attach_brc_on",             HOOK_BOOL_ITEM, (int)setting_attach_brc_on);
    ENTRY("attach_brk_normal",         HOOK_BOOL_ITEM, (int)setting_attach_brk_normal);
    ENTRY("attach_brk_racing",         HOOK_BOOL_ITEM, (int)setting_attach_brk_racing);
    ENTRY("attach_clt_big",            HOOK_BOOL_ITEM, (int)setting_attach_clt_big);
    ENTRY("attach_clt_normal",         HOOK_BOOL_ITEM, (int)setting_attach_clt_normal);
    ENTRY("attach_clt_triple",         HOOK_BOOL_ITEM, (int)setting_attach_clt_triple);
    ENTRY("attach_clt_twin",           HOOK_BOOL_ITEM, (int)setting_attach_clt_twin);
    ENTRY("attach_coo_1",              HOOK_BOOL_ITEM, (int)setting_attach_coo_1);
    ENTRY("attach_coo_2",              HOOK_BOOL_ITEM, (int)setting_attach_coo_2);
    ENTRY("attach_coo_normal",         HOOK_BOOL_ITEM, (int)setting_attach_coo_normal);
    ENTRY("attach_cpu_off",            HOOK_BOOL_ITEM, (int)attach_cpu_off);
    ENTRY("attach_cpu_on",             HOOK_BOOL_ITEM, (int)attach_cpu_on);
    ENTRY("attach_fly_crmo",           HOOK_BOOL_ITEM, (int)setting_attach_fly_crmo);
    ENTRY("attach_fly_light",          HOOK_BOOL_ITEM, (int)setting_attach_fly_light);
    ENTRY("attach_fly_light_crmo",     HOOK_BOOL_ITEM, (int)setting_attach_fly_light_crmo);
    ENTRY("attach_fly_normal",         HOOK_BOOL_ITEM, (int)setting_attach_fly_normal);
    ENTRY("attach_lsd_15way",          HOOK_BOOL_ITEM, (int)setting_attach_lsd_15way);
    ENTRY("attach_lsd_1way",           HOOK_BOOL_ITEM, (int)setting_attach_lsd_1way);
    ENTRY("attach_lsd_2way",           HOOK_BOOL_ITEM, (int)setting_attach_lsd_2way);
    ENTRY("attach_lsd_aycc",           HOOK_BOOL_ITEM, (int)setting_attach_lsd_aycc);
    ENTRY("attach_lsd_normal",         HOOK_BOOL_ITEM, (int)setting_attach_lsd_normal);
    ENTRY("attach_lsd_variable",       HOOK_BOOL_ITEM, (int)setting_attach_lsd_variable);
    ENTRY("attach_msn_close",          HOOK_BOOL_ITEM, (int)setting_attach_msn_close);
    ENTRY("attach_msn_normal",         HOOK_BOOL_ITEM, (int)setting_attach_msn_normal);
    ENTRY("attach_msn_super_close",    HOOK_BOOL_ITEM, (int)setting_attach_msn_super_close);
    ENTRY("attach_msn_variable",       HOOK_BOOL_ITEM, (int)setting_attach_msn_variable);
    ENTRY("attach_muf_normal",         HOOK_BOOL_ITEM, (int)attach_muf_normal);
    ENTRY("attach_muf_racing",         HOOK_BOOL_ITEM, (int)attach_muf_racing);
    ENTRY("attach_muf_semi_racing",    HOOK_BOOL_ITEM, (int)attach_muf_semi_racing);
    ENTRY("attach_muf_sports",         HOOK_BOOL_ITEM, (int)attach_muf_sports);
    ENTRY("attach_nat_off",            HOOK_BOOL_ITEM, (int)setting_attach_nat_off);
    ENTRY("attach_nat_stage1",         HOOK_BOOL_ITEM, (int)setting_attach_nat_stage1);
    ENTRY("attach_nat_stage2",         HOOK_BOOL_ITEM, (int)setting_attach_nat_stage2);
    ENTRY("attach_nat_stage3",         HOOK_BOOL_ITEM, (int)setting_attach_nat_stage3);
    ENTRY("attach_shf_normal",         HOOK_BOOL_ITEM, (int)setting_attach_shf_normal);
    ENTRY("attach_shf_propeller",      HOOK_BOOL_ITEM, (int)setting_attach_shf_propeller);
    ENTRY("attach_sms_off",            HOOK_BOOL_ITEM, (int)setting_attach_sms_off);
    ENTRY("attach_sms_on",             HOOK_BOOL_ITEM, (int)setting_attach_sms_on);
    ENTRY("attach_sus_normal",         HOOK_BOOL_ITEM, (int)setting_attach_sus_normal);
    ENTRY("attach_sus_racing",         HOOK_BOOL_ITEM, (int)setting_attach_sus_racing);
    ENTRY("attach_sus_semi_racing",    HOOK_BOOL_ITEM, (int)setting_attach_sus_semi_racing);
    ENTRY("attach_sus_sports",         HOOK_BOOL_ITEM, (int)setting_attach_sus_sports);
    ENTRY("attach_tcs_off",            HOOK_BOOL_ITEM, (int)setting_attach_tcs_off);
    ENTRY("attach_tcs_on",             HOOK_BOOL_ITEM, (int)setting_attach_tcs_on);
    ENTRY("attach_tir_f_control",      HOOK_BOOL_ITEM, (int)setting_attach_tir_f_control);
    ENTRY("attach_tir_f_normal",       HOOK_BOOL_ITEM, (int)setting_attach_tir_f_normal);
    ENTRY("attach_tir_f_racing_h",     HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_h);
    ENTRY("attach_tir_f_racing_m",     HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_m);
    ENTRY("attach_tir_f_racing_mh",    HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_mh);
    ENTRY("attach_tir_f_racing_ms",    HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_ms);
    ENTRY("attach_tir_f_racing_s",     HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_s);
    ENTRY("attach_tir_f_racing_sh",    HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_sh);
    ENTRY("attach_tir_f_racing_ss",    HOOK_BOOL_ITEM, (int)setting_attach_tir_f_racing_ss);
    ENTRY("attach_tir_f_sports",       HOOK_BOOL_ITEM, (int)setting_attach_tir_f_sports);
    ENTRY("attach_tir_r_control",      HOOK_BOOL_ITEM, (int)setting_attach_tir_r_control);
    ENTRY("attach_tir_r_normal",       HOOK_BOOL_ITEM, (int)setting_attach_tir_r_normal);
    ENTRY("attach_tir_r_racing_h",     HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_h);
    ENTRY("attach_tir_r_racing_m",     HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_m);
    ENTRY("attach_tir_r_racing_mh",    HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_mh);
    ENTRY("attach_tir_r_racing_ms",    HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_ms);
    ENTRY("attach_tir_r_racing_s",     HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_s);
    ENTRY("attach_tir_r_racing_sh",    HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_sh);
    ENTRY("attach_tir_r_racing_ss",    HOOK_BOOL_ITEM, (int)setting_attach_tir_r_racing_ss);
    ENTRY("attach_tir_r_sports",       HOOK_BOOL_ITEM, (int)setting_attach_tir_r_sports);
    ENTRY("attach_tur_off",            HOOK_BOOL_ITEM, (int)attach_tur_off);
    ENTRY("attach_tur_stage1",         HOOK_BOOL_ITEM, (int)setting_attach_tur_stage1);
    ENTRY("attach_tur_stage2",         HOOK_BOOL_ITEM, (int)setting_attach_tur_stage2);
    ENTRY("attach_tur_stage3",         HOOK_BOOL_ITEM, (int)setting_attach_tur_stage3);
    ENTRY("attach_tur_stage4",         HOOK_BOOL_ITEM, (int)setting_attach_tur_stage4);
    ENTRY("attach_vcd_off",            HOOK_BOOL_ITEM, (int)setting_attach_vcd_off);
    ENTRY("attach_vcd_on",             HOOK_BOOL_ITEM, (int)setting_attach_vcd_on);
    ENTRY("blink",                     HOOK_BLINK, 0);
    ENTRY("car_current_power",         HOOK_CALLBACK_TEXT, (int)setting_text_car_current_power);
    ENTRY("disable_aero",              HOOK_BOOL_ITEM, (int)setting_disable_aero);
    ENTRY("disable_asm",               HOOK_BOOL_ITEM, (int)setting_disable_asm);
    ENTRY("disable_ayc",               HOOK_BOOL_ITEM, (int)setting_disable_ayc);
    ENTRY("disable_bound",             HOOK_BOOL_ITEM, (int)setting_disable_bound);
    ENTRY("disable_brake_balance",     HOOK_BOOL_ITEM, (int)setting_disable_brake_balance);
    ENTRY("disable_camber",            HOOK_BOOL_ITEM, (int)setting_disable_camber);
    ENTRY("disable_damper",            HOOK_BOOL_ITEM, (int)setting_disable_damper);
    ENTRY("disable_gear_ratio",        HOOK_BOOL_ITEM, (int)setting_disable_gear_ratio);
    ENTRY("disable_lsd_accel",         HOOK_BOOL_ITEM, (int)setting_disable_lsd_accel);
    ENTRY("disable_lsd_decrease",      HOOK_BOOL_ITEM, (int)setting_disable_lsd_decrease);
    ENTRY("disable_lsd_initial",       HOOK_BOOL_ITEM, (int)setting_disable_lsd_initial);
    ENTRY("disable_rebound",           HOOK_BOOL_ITEM, (int)setting_disable_bound);
    ENTRY("disable_ride_height",       HOOK_BOOL_ITEM, (int)setting_disable_ride_height);
    ENTRY("disable_spring_rate",       HOOK_BOOL_ITEM, (int)setting_disable_spring_rate);
    ENTRY("disable_stabilizer",        HOOK_BOOL_ITEM, (int)setting_disable_stabilizer);
    ENTRY("disable_tcs",               HOOK_BOOL_ITEM, (int)setting_disable_tcs);
    ENTRY("disable_toe",               HOOK_BOOL_ITEM, (int)setting_disable_toe);
    ENTRY("disable_vcd",               HOOK_BOOL_ITEM, (int)setting_disable_vcd);
    ENTRY("elec_ascc",                 HOOK_CALLBACK_TEXT, (int)text_elec_ascc);
    ENTRY("elec_aycc",                 HOOK_CALLBACK_TEXT, (int)text_elec_aycc);
    ENTRY("elec_tcsc",                 HOOK_CALLBACK_TEXT, (int)text_elec_tcsc);
    ENTRY("elec_vcdc",                 HOOK_CALLBACK_TEXT, (int)text_elec_vcdc);
    ENTRY("gear_chart",                HOOK_GEAR_CHART, (int)gear_chart_car);
    ENTRY("gear_ratio_0",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_0);
    ENTRY("gear_ratio_1",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_1);
    ENTRY("gear_ratio_2",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_2);
    ENTRY("gear_ratio_3",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_3);
    ENTRY("gear_ratio_4",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_4);
    ENTRY("gear_ratio_5",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_5);
    ENTRY("gear_ratio_6",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_6);
    ENTRY("gear_ratio_7",              HOOK_CALLBACK_TEXT, (int)text_gear_ratio_7);
    ENTRY("gear_ratio_auto",           HOOK_CALLBACK_TEXT, (int)text_gear_ratio_auto);
    ENTRY("gear_ratio_final",          HOOK_CALLBACK_TEXT, (int)text_gear_ratio_final);
    ENTRY("gear_setup",                setting_create_gear_setup, 0);
    ENTRY("lsd_acl_f",                 HOOK_CALLBACK_TEXT, (int)text_lsd_acl_f);
    ENTRY("lsd_acl_r",                 HOOK_CALLBACK_TEXT, (int)text_lsd_acl_r);
    ENTRY("lsd_dec_f",                 HOOK_CALLBACK_TEXT, (int)text_lsd_dec_f);
    ENTRY("lsd_dec_r",                 HOOK_CALLBACK_TEXT, (int)text_lsd_dec_r);
    ENTRY("lsd_int_f",                 HOOK_CALLBACK_TEXT, (int)text_lsd_int_f);
    ENTRY("lsd_int_r",                 HOOK_CALLBACK_TEXT, (int)text_lsd_int_r);
    ENTRY("parts_drv_clutch",          HOOK_CALLBACK_TEXT, (int)text_parts_drv_clutch);
    ENTRY("parts_drv_fly",             HOOK_CALLBACK_TEXT, (int)text_parts_drv_fly);
    ENTRY("parts_drv_lsd",             HOOK_CALLBACK_TEXT, (int)text_parts_drv_lsd);
    ENTRY("parts_drv_mission",         HOOK_CALLBACK_TEXT, (int)text_parts_drv_mission);
    ENTRY("parts_drv_shaft",           HOOK_CALLBACK_TEXT, (int)text_parts_drv_shaft);
    ENTRY("parts_eng_computer",        HOOK_CALLBACK_TEXT, (int)text_parts_eng_computer);
    ENTRY("parts_eng_displacement",    HOOK_CALLBACK_TEXT, (int)text_parts_eng_displacement);
    ENTRY("parts_eng_engine_balance",  HOOK_CALLBACK_TEXT, (int)text_parts_eng_engine_balance);
    ENTRY("parts_eng_inter_cooler",    HOOK_CALLBACK_TEXT, (int)text_parts_eng_inter_cooler);
    ENTRY("parts_eng_muffler",         HOOK_CALLBACK_TEXT, (int)text_parts_eng_muffler);
    ENTRY("parts_eng_na_tune",         HOOK_CALLBACK_TEXT, (int)text_parts_eng_na_tune);
    ENTRY("parts_eng_port_polish",     HOOK_CALLBACK_TEXT, (int)text_parts_eng_port_polish);
    ENTRY("parts_eng_turbine",         HOOK_CALLBACK_TEXT, (int)text_parts_eng_turbine);
    ENTRY("parts_oth_asm_s",           HOOK_CALLBACK_TEXT, (int)text_parts_oth_asm_s);
    ENTRY("parts_oth_ayc_s",           HOOK_CALLBACK_TEXT, (int)text_parts_oth_ayc_s);
    ENTRY("parts_oth_tcs_s",           HOOK_CALLBACK_TEXT, (int)text_parts_oth_tcs_s);
    ENTRY("parts_oth_vcd_s",           HOOK_CALLBACK_TEXT, (int)text_parts_oth_vcd_s);
    ENTRY("parts_oth_weight_r",        HOOK_CALLBACK_TEXT, (int)text_parts_oth_weight_r);
    ENTRY("parts_sus_b_balance",       HOOK_CALLBACK_TEXT, (int)text_parts_sus_b_balance);
    ENTRY("parts_sus_b_kit",           HOOK_CALLBACK_TEXT, (int)text_parts_sus_b_kit);
    ENTRY("parts_sus_s_kit",           HOOK_CALLBACK_TEXT, (int)text_parts_sus_s_kit);
    ENTRY("parts_tir_f",               HOOK_CALLBACK_TEXT, (int)text_parts_tir_f);
    ENTRY("parts_tir_r",               HOOK_CALLBACK_TEXT, (int)text_parts_tir_r);
    ENTRY("spring_bound_f",            HOOK_CALLBACK_TEXT, (int)text_spring_bound_f);
    ENTRY("spring_bound_r",            HOOK_CALLBACK_TEXT, (int)text_spring_bound_r);
    ENTRY("spring_brake_f",            HOOK_CALLBACK_TEXT, (int)text_spring_brake_f);
    ENTRY("spring_brake_r",            HOOK_CALLBACK_TEXT, (int)text_spring_brake_r);
    ENTRY("spring_camber_f",           HOOK_CALLBACK_TEXT, (int)text_spring_camber_f);
    ENTRY("spring_camber_r",           HOOK_CALLBACK_TEXT, (int)text_spring_camber_r);
    ENTRY("spring_damper_f",           HOOK_CALLBACK_TEXT, (int)text_spring_damper_f);
    ENTRY("spring_damper_r",           HOOK_CALLBACK_TEXT, (int)text_spring_damper_r);
    ENTRY("spring_height_f",           HOOK_CALLBACK_TEXT, (int)text_spring_height_f);
    ENTRY("spring_height_r",           HOOK_CALLBACK_TEXT, (int)text_spring_height_r);
    ENTRY("spring_rate_f",             HOOK_CALLBACK_TEXT, (int)text_spring_rate_f);
    ENTRY("spring_rate_r",             HOOK_CALLBACK_TEXT, (int)text_spring_rate_r);
    ENTRY("spring_rebound_f",          HOOK_CALLBACK_TEXT, (int)text_spring_rebound_f);
    ENTRY("spring_rebound_r",          HOOK_CALLBACK_TEXT, (int)text_spring_rebound_r);
    ENTRY("spring_stabilizer_f",       HOOK_CALLBACK_TEXT, (int)text_spring_stabilizer_f);
    ENTRY("spring_stabilizer_r",       HOOK_CALLBACK_TEXT, (int)text_spring_stabilizer_r);
    ENTRY("spring_toe_f",              HOOK_CALLBACK_TEXT, (int)text_spring_toe_f);
    ENTRY("spring_toe_r",              HOOK_CALLBACK_TEXT, (int)text_spring_toe_r);
    ENTRY("turbine_zero_name",         setting_create_turbine_zero_name, 0);
    ENTRY("user_carname",              HOOK_CALLBACK_TEXT, (int)setting_text_user_carname);
}
