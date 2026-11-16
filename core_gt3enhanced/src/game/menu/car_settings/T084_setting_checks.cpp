/* T084 - the button checks of the car setting pages (gt2k_menu::
 * SettingControl, T080) and the regulation check of a car.
 * 0x001677A0..0x00169780.
 *
 * T084_static_init fills g_002AD058 with 23 SelectableFuncData {page,
 * checks, count} entries sorted by page (count g_00351994);
 * SettingControl::begin (T080, 0x0015CA98) gives it to its PageManager
 * (selectableFuncs).  ButtonList::setButtonChecks (T244) gives button i of
 * the page check i (the last one for the rest); a button is available while
 * check(ctx, its record) != 0:
 *   "chg/change_*" pages: GameStatus's setting car (MenuStatus::raceCar[0])
 *       owns the part of the button (CarInformation::hasPart; the part ids
 *       of T083's "set_*" actions);
 *   "set/adjast_*" [sic] pages: a setting of the setting car can be
 *       adjusted: the record of the fitted part gives it a range that is not
 *       a single value.
 * Every check returns 0 without a setting car.  None reads its arguments.
 * The check arrays are file-local constant data (below). */
/* Includes: retail's type names (.rodata 0x00322CE8..0x00322F08, reverse
 * definition order) are SmartIndexT<int>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * RegurationResult, the garage lists (GarageWheelList .. Cell<GarageCar>)
 * and Operator (a non-polymorphic base: last); no List<Button>, no
 * SettingControl (the original saw Button, not ButtonList, and not the
 * control).  So the GameStatus first, RegurationResult, the menu item
 * classes (T206: T216's operator classes, Button's base Cell<Button>), then
 * this unit's header (T211's PageManager: SmartIndexT<int>).  The headers
 * after it add no class name. */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getMenuStatus; (seen: the
                                               garage classes) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult (isOk, the inline destructor),
                                               __ti16RegurationResult */
#include "game/menu/framework/T206_button.h"               /* (seen: T216's operator classes, Cell<Button>) */
#include "game/menu/car_settings/T084_setting_checks.h"       /* car_meets_event_regulations, the checks, g_002AD058,
                                               g_00351994 (T211 SelectableFuncData; SmartIndexT<int>) */
#include "game/save/T427_menu_status.h"          /* MenuStatus::raceCar[0], event */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments: get*Record, getSuspensionPart,
                                               getLSDPart; the records; PART_*, TIRE_KIND_* */
#include "game/data/T483_car_information.h"      /* CarInformation::hasPart */
#include "game/data/T489_car_database.h"         /* check_regulation */
#include "libc.h"                           /* strcmp (0x0027EA40) */
#include "cxxrt.h"                          /* __rtti_user (0x0027C408) */
#include "game/menu/car_settings/setting_actions_internal.h"

using namespace gt2k_menu;

/* 1 if `car` meets the regulations of the current event (MenuStatus::event,
 * check_regulation of T489), 1 when there is no event ("", 0x00351990).  The
 * RegurationResult on the stack (sp+0x00) is destroyed by its inline
 * destructor (only the vptr store: 0x00310310 at +0x1C); the landing pad
 * does the same before rethrowing (__throw 0x00278268).  31 callers (T096's
 * check_set_parts, T082's setters, ...).
 *
 * car goes to its CarInformation base (+0x10) without the NULL test egcs
 * adds to a pointer conversion: through a reference, as here (egcs then
 * gives the listing's code).  (? the original took GarageCar &car, or
 * check_regulation a CarInformation &: both compile to this code.) */
// FUNC 0x001677A0 car_meets_event_regulations
int car_meets_event_regulations(GarageCar *car)
{
    const char *event = getGameStatus()->menu.event;
    if (strcmp(event, "") == 0)
        return 1;
    GarageCar &garageCar = *car;
    RegurationResult result = check_regulation(nameToCode(event), &garageCar);
    return result.isOk();
}

/* ------------------------------------------------------------------------ */
/* "chg/change_*": 1 if the setting car owns the part of the button.  One   */
/* function per button, named after the T083 "set_*" action of the part.   */
/* ------------------------------------------------------------------------ */

/* computer ("chg/change_eng_computer", g_00351998) */
// FUNC 0x00167840 setting_owns_cpu_off
int setting_owns_cpu_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_COMPUTER) != 0;
}

// FUNC 0x00167888 setting_owns_cpu_on
int setting_owns_cpu_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_COMPUTER_ROM) != 0;
}

/* muffler ("chg/change_eng_muffler", g_00322F08) */
// FUNC 0x001678D0 setting_owns_muf_normal
int setting_owns_muf_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_MUFFLER) != 0;
}

// FUNC 0x00167918 setting_owns_muf_sports
int setting_owns_muf_sports(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_MUFFLER_SPORTS) != 0;
}

// FUNC 0x00167960 setting_owns_muf_semi_racing
int setting_owns_muf_semi_racing(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_MUFFLER_SEMI_RACING) != 0;
}

// FUNC 0x001679A8 setting_owns_muf_racing
int setting_owns_muf_racing(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_MUFFLER_RACING) != 0;
}

/* turbine kit ("chg/change_trb_turbine", g_00322F18) */
// FUNC 0x001679F0 setting_owns_tur_off
int setting_owns_tur_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TURBINE) != 0;
}

// FUNC 0x00167A38 setting_owns_tur_stage1
int setting_owns_tur_stage1(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TURBINE_STAGE1) != 0;
}

// FUNC 0x00167A80 setting_owns_tur_stage2
int setting_owns_tur_stage2(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TURBINE_STAGE2) != 0;
}

// FUNC 0x00167AC8 setting_owns_tur_stage3
int setting_owns_tur_stage3(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TURBINE_STAGE3) != 0;
}

// FUNC 0x00167B10 setting_owns_tur_stage4
int setting_owns_tur_stage4(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TURBINE_STAGE4) != 0;
}

/* intercooler ("chg/change_trb_inter", g_00322F30) */
// FUNC 0x00167B58 setting_owns_coo_normal
int setting_owns_coo_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_INTERCOOLER) != 0;
}

// FUNC 0x00167BA0 setting_owns_coo_1
int setting_owns_coo_1(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_INTERCOOLER_SPORTS) != 0;
}

// FUNC 0x00167BE8 setting_owns_coo_2
int setting_owns_coo_2(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_INTERCOOLER_RACING) != 0;
}

/* NA tune ("chg/change_eng_na", g_00322F40) */
// FUNC 0x00167C30 setting_owns_nat_off
int setting_owns_nat_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_NA_TUNE_NONE) != 0;
}

// FUNC 0x00167C78 setting_owns_nat_stage1
int setting_owns_nat_stage1(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_NA_TUNE_STAGE1) != 0;
}

// FUNC 0x00167CC0 setting_owns_nat_stage2
int setting_owns_nat_stage2(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_NA_TUNE_STAGE2) != 0;
}

// FUNC 0x00167D08 setting_owns_nat_stage3
int setting_owns_nat_stage3(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_NA_TUNE_STAGE3) != 0;
}

/* suspension ("chg/change_sus_kit", g_00322F50) */
// FUNC 0x00167D50 setting_owns_sus_normal
int setting_owns_sus_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_SUSPENSION) != 0;
}

// FUNC 0x00167D98 setting_owns_sus_sports
int setting_owns_sus_sports(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_SUSPENSION_SPORTS) != 0;
}

// FUNC 0x00167DE0 setting_owns_sus_semi_racing
int setting_owns_sus_semi_racing(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_SUSPENSION_SEMI_RACING) != 0;
}

// FUNC 0x00167E28 setting_owns_sus_racing
int setting_owns_sus_racing(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_SUSPENSION_FULL_CUSTOM) != 0;
}

/* brake controller ("chg/change_brk_balance", g_003519A8) */
// FUNC 0x00167E70 setting_owns_brc_off
int setting_owns_brc_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_BRAKE_CONTROLLER_NONE) != 0;
}

// FUNC 0x00167EB8 setting_owns_brc_on
int setting_owns_brc_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_BRAKE_CONTROLLER) != 0;
}

/* brakes ("chg/change_brk_kit", g_003519A0) */
// FUNC 0x00167F00 setting_owns_brk_normal
int setting_owns_brk_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_BRAKE) != 0;
}

// FUNC 0x00167F48 setting_owns_brk_racing
int setting_owns_brk_racing(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_BRAKE_RACING) != 0;
}

/* transmission ("chg/change_drv_mission", g_00322F60) */
// FUNC 0x00167F90 setting_owns_msn_normal
int setting_owns_msn_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_GEAR) != 0;
}

// FUNC 0x00167FD8 setting_owns_msn_close
int setting_owns_msn_close(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_GEAR_CLOSE) != 0;
}

// FUNC 0x00168020 setting_owns_msn_super_close
int setting_owns_msn_super_close(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_GEAR_SUPER_CLOSE) != 0;
}

// FUNC 0x00168068 setting_owns_msn_variable
int setting_owns_msn_variable(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_GEAR_FULL_CUSTOM) != 0;
}

/* clutch ("chg/change_drv_clutch", g_00322F70) */
// FUNC 0x001680B0 setting_owns_clt_normal
int setting_owns_clt_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_CLUTCH) != 0;
}

// FUNC 0x001680F8 setting_owns_clt_big
int setting_owns_clt_big(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_CLUTCH_SPORTS) != 0;
}

// FUNC 0x00168140 setting_owns_clt_twin
int setting_owns_clt_twin(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_CLUTCH_TWIN) != 0;
}

// FUNC 0x00168188 setting_owns_clt_triple
int setting_owns_clt_triple(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_CLUTCH_TRIPLE) != 0;
}

/* flywheel ("chg/change_drv_flywheel", g_00322F80) */
// FUNC 0x001681D0 setting_owns_fly_normal
int setting_owns_fly_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FLYWHEEL) != 0;
}

// FUNC 0x00168218 setting_owns_fly_light
int setting_owns_fly_light(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FLYWHEEL_SPORTS) != 0;
}

// FUNC 0x00168260 setting_owns_fly_crmo
int setting_owns_fly_crmo(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FLYWHEEL_SEMI_RACING) != 0;
}

// FUNC 0x001682A8 setting_owns_fly_light_crmo
int setting_owns_fly_light_crmo(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FLYWHEEL_RACING) != 0;
}

/* LSD ("chg/change_drv_lsd", g_00322F90; its sixth item is the AYC) */
// FUNC 0x001682F0 setting_owns_lsd_normal
int setting_owns_lsd_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD) != 0;
}

// FUNC 0x00168338 setting_owns_lsd_2way
int setting_owns_lsd_2way(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_2WAY) != 0;
}

// FUNC 0x00168380 setting_owns_lsd_15way
int setting_owns_lsd_15way(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_1_5WAY) != 0;
}

// FUNC 0x001683C8 setting_owns_lsd_1way
int setting_owns_lsd_1way(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_1WAY) != 0;
}

// FUNC 0x00168410 setting_owns_lsd_variable
int setting_owns_lsd_variable(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_FULL_CUSTOM) != 0;
}

// FUNC 0x00168458 setting_owns_lsd_aycc
int setting_owns_lsd_aycc(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_AYC) != 0;
}

/* propeller shaft ("chg/change_drv_shaft", g_003519B0) */
// FUNC 0x001684A0 setting_owns_shf_normal
int setting_owns_shf_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_PROPELLER_SHAFT) != 0;
}

// FUNC 0x001684E8 setting_owns_shf_propeller
int setting_owns_shf_propeller(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_PROPELLER_SHAFT_CARBON) != 0;
}

/* tyres ("chg/change_tir_front" g_00322FA8 and "chg/change_tir_rear"
 * g_00322FD0: the rear page tests the FRONT tyre parts too) */
// FUNC 0x00168530 setting_owns_tir_f_normal
int setting_owns_tir_f_normal(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_NORMAL) != 0;
}

// FUNC 0x00168578 setting_owns_tir_f_control
int setting_owns_tir_f_control(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_CONTROL) != 0;
}

// FUNC 0x001685C0 setting_owns_tir_f_sports
int setting_owns_tir_f_sports(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_SPORTS) != 0;
}

// FUNC 0x00168608 setting_owns_tir_f_racing_sh
int setting_owns_tir_f_racing_sh(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_SH) != 0;
}

// FUNC 0x00168650 setting_owns_tir_f_racing_h
int setting_owns_tir_f_racing_h(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_H) != 0;
}

// FUNC 0x00168698 setting_owns_tir_f_racing_mh
int setting_owns_tir_f_racing_mh(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_MH) != 0;
}

// FUNC 0x001686E0 setting_owns_tir_f_racing_m
int setting_owns_tir_f_racing_m(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_M) != 0;
}

// FUNC 0x00168728 setting_owns_tir_f_racing_ms
int setting_owns_tir_f_racing_ms(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_MS) != 0;
}

// FUNC 0x00168770 setting_owns_tir_f_racing_s
int setting_owns_tir_f_racing_s(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_S) != 0;
}

// FUNC 0x001687B8 setting_owns_tir_f_racing_ss
int setting_owns_tir_f_racing_ss(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_FRONT_TIRE + TIRE_KIND_RACING_SS) != 0;
}

/* AYC ("chg/change_oth_aycc", g_003519B8): the AYC page's own copies of
 * PART_LSD / PART_LSD_AYC */
// FUNC 0x00168800 setting_owns_ayc_off
int setting_owns_ayc_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD) != 0;
}

// FUNC 0x00168848 setting_owns_ayc_on
int setting_owns_ayc_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_LSD_AYC) != 0;
}

/* ASM ("chg/change_oth_asm", g_003519C0) */
// FUNC 0x00168890 setting_owns_asm_off
int setting_owns_asm_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_ASCC_NONE) != 0;
}

// FUNC 0x001688D8 setting_owns_asm_on
int setting_owns_asm_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_ASCC) != 0;
}

/* TCS ("chg/change_oth_tcsc", g_003519C8) */
// FUNC 0x00168920 setting_owns_tcs_off
int setting_owns_tcs_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TCSC_NONE) != 0;
}

// FUNC 0x00168968 setting_owns_tcs_on
int setting_owns_tcs_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_TCSC) != 0;
}

/* VCD ("chg/change_oth_vcdc", g_003519D0) */
// FUNC 0x001689B0 setting_owns_vcd_off
int setting_owns_vcd_off(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_DRIVETRAIN) != 0;
}

// FUNC 0x001689F8 setting_owns_vcd_on
int setting_owns_vcd_on(void *ctx, ButtonData *data)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->hasPart(PART_DRIVETRAIN_VCD) != 0;
}

/* ------------------------------------------------------------------------ */
/* Can a setting of the setting car be adjusted?  The record of the fitted  */
/* part has two different limits (min / max, or more than one level).       */
/* T081 (u033 / u034) calls these directly; the "set/adjast_*" tables call  */
/* them through the wrappers below.  The record getter is called again for  */
/* every byte read.                                                         */
/* The two-pair checks test "both pairs equal" and return 0 inside nested   */
/* ifs, 1 after them (the listing's bnel to a shared "return 1" and the     */
/* beq / 0 / 1 tail); the first value of a pair is read into a local before */
/* the second getter call (call, lbu, call, lbu).  `return a != b` for the  */
/* last pair gives a store-flag instead.                                    */
/* ------------------------------------------------------------------------ */

/* spring rate: the front or the rear limits differ */
// FUNC 0x00168A40 can_adjust_spring_rate
int can_adjust_spring_rate(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getSuspensionRecord()->springRateFMin;
    if (frontMin == settingCar->getSuspensionRecord()->springRateFMax) {
        u8 rearMin = settingCar->getSuspensionRecord()->springRateRMin;
        if (rearMin == settingCar->getSuspensionRecord()->springRateRMax)
            return 0;
    }
    return 1;
}

/* ride height (16-bit limits) */
// FUNC 0x00168AD0 can_adjust_ride_height
int can_adjust_ride_height(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u16 frontMin = settingCar->getSuspensionRecord()->heightFMin;
    if (frontMin == settingCar->getSuspensionRecord()->heightFMax) {
        u16 rearMin = settingCar->getSuspensionRecord()->heightRMin;
        if (rearMin == settingCar->getSuspensionRecord()->heightRMax)
            return 0;
    }
    return 1;
}

/* damper: only the sports and semi racing suspensions (not the normal one,
 * not the full custom one, which has bound / rebound instead).  The `||`
 * in one test gives the listing's beq to the shared "return 0" and the
 * bne / 1 / 0 tail; two separate tests give a store-flag. */
// FUNC 0x00168B60 can_adjust_damper
int can_adjust_damper(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    if (settingCar->getSuspensionPart() == PART_SUSPENSION
        || settingCar->getSuspensionPart() == PART_SUSPENSION_FULL_CUSTOM)
        return 0;
    return 1;
}

/* bound / rebound: only the full custom ("set_sus_racing") suspension; the
 * "set/adjast_sus" table uses it for both buttons */
// FUNC 0x00168BC8 can_adjust_bound_rebound
int can_adjust_bound_rebound(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->getSuspensionPart() == PART_SUSPENSION_FULL_CUSTOM;
}

/* camber */
// FUNC 0x00168C10 can_adjust_camber
int can_adjust_camber(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getSuspensionRecord()->camberFMin;
    if (frontMin == settingCar->getSuspensionRecord()->camberFMax) {
        u8 rearMin = settingCar->getSuspensionRecord()->camberRMin;
        if (rearMin == settingCar->getSuspensionRecord()->camberRMax)
            return 0;
    }
    return 1;
}

/* toe */
// FUNC 0x00168CA0 can_adjust_toe
int can_adjust_toe(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getSuspensionRecord()->toeFMin;
    if (frontMin == settingCar->getSuspensionRecord()->toeFMax) {
        u8 rearMin = settingCar->getSuspensionRecord()->toeRMin;
        if (rearMin == settingCar->getSuspensionRecord()->toeRMax)
            return 0;
    }
    return 1;
}

/* stabiliser: unless both level counts are 1 (egcs compares the rear count
 * with the register holding the front one, known to be 1 there) */
// FUNC 0x00168D30 can_adjust_stabiliser
int can_adjust_stabiliser(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    if (settingCar->getSuspensionRecord()->stabilizerFCount == 1) {
        if (settingCar->getSuspensionRecord()->stabilizerRCount == 1)
            return 0;
    }
    return 1;
}

/* brake balance ("set/adjast_brk"): unless both level counts of the brake
 * controller are 1 */
// FUNC 0x00168DA8 can_adjust_brake_balance
int can_adjust_brake_balance(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    if (settingCar->getBrakecontrollerRecord()->countF == 1) {
        if (settingCar->getBrakecontrollerRecord()->countR == 1)
            return 0;
    }
    return 1;
}

/* LSD initial torque */
// FUNC 0x00168E20 can_adjust_lsd_initial
int can_adjust_lsd_initial(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getLSDRecord()->initialFMin;
    if (frontMin == settingCar->getLSDRecord()->initialFMax) {
        u8 rearMin = settingCar->getLSDRecord()->initialRMin;
        if (rearMin == settingCar->getLSDRecord()->initialRMax)
            return 0;
    }
    return 1;
}

/* LSD acceleration sensitivity */
// FUNC 0x00168EB0 can_adjust_lsd_accel
int can_adjust_lsd_accel(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getLSDRecord()->accelFMin;
    if (frontMin == settingCar->getLSDRecord()->accelFMax) {
        u8 rearMin = settingCar->getLSDRecord()->accelRMin;
        if (rearMin == settingCar->getLSDRecord()->accelRMax)
            return 0;
    }
    return 1;
}

/* LSD deceleration sensitivity */
// FUNC 0x00168F40 can_adjust_lsd_decel
int can_adjust_lsd_decel(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getLSDRecord()->decelFMin;
    if (frontMin == settingCar->getLSDRecord()->decelFMax) {
        u8 rearMin = settingCar->getLSDRecord()->decelRMin;
        if (rearMin == settingCar->getLSDRecord()->decelRMax)
            return 0;
    }
    return 1;
}

/* gear ratios: the transmission's final gear range is not a single value
 * (finalGearRatioMin, +0x26, read first) */
// FUNC 0x00168FD0 can_adjust_final_gear
int can_adjust_final_gear(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u16 finalMin = settingCar->getGearRecord()->finalGearRatioMin;
    return finalMin != settingCar->getGearRecord()->finalGearRatioMax;
}

/* aero (downforce; the racing modification's limits) */
// FUNC 0x00169038 can_adjust_aero
int can_adjust_aero(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 frontMin = settingCar->getRacingmodifyRecord()->aeroFMin;
    if (frontMin == settingCar->getRacingmodifyRecord()->aeroFMax) {
        u8 rearMin = settingCar->getRacingmodifyRecord()->aeroRMin;
        if (rearMin == settingCar->getRacingmodifyRecord()->aeroRMax)
            return 0;
    }
    return 1;
}

/* AYC: the LSD fitted is the AYC controller */
// FUNC 0x001690C8 can_adjust_ayc
int can_adjust_ayc(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->getLSDPart() == PART_LSD_AYC;
}

/* ASM: the ASCC record's first setting has more than one level */
// FUNC 0x00169110 can_adjust_asm
int can_adjust_asm(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->getASCCRecord()->count0 != 1;
}

/* TCS: the TCSC record has more than one level */
// FUNC 0x00169158 can_adjust_tcs
int can_adjust_tcs(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    return settingCar->getTCSCRecord()->count != 1;
}

/* VCD: the drive train's front / rear torque split range */
// FUNC 0x001691A0 can_adjust_vcd
int can_adjust_vcd(void)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return 0;
    u8 vcdMin = settingCar->getDrivetrainRecord()->vcdMin;
    return vcdMin != settingCar->getDrivetrainRecord()->vcdMax;
}

/* ------------------------------------------------------------------------ */
/* "set/adjast_*": the table entries, out-of-line wrappers (a call, not a   */
/* tail jump) of the checks above.                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x00169208 setting_adjust_spring
int setting_adjust_spring(void *ctx, ButtonData *data)
{
    return can_adjust_spring_rate();
}

// FUNC 0x00169228 setting_adjust_height
int setting_adjust_height(void *ctx, ButtonData *data)
{
    return can_adjust_ride_height();
}

// FUNC 0x00169248 setting_adjust_damper
int setting_adjust_damper(void *ctx, ButtonData *data)
{
    return can_adjust_damper();
}

/* in g_00322FF8 twice (bound and rebound) */
// FUNC 0x00169268 setting_adjust_bound_rebound
int setting_adjust_bound_rebound(void *ctx, ButtonData *data)
{
    return can_adjust_bound_rebound();
}

// FUNC 0x00169288 setting_adjust_camber
int setting_adjust_camber(void *ctx, ButtonData *data)
{
    return can_adjust_camber();
}

// FUNC 0x001692A8 setting_adjust_toe
int setting_adjust_toe(void *ctx, ButtonData *data)
{
    return can_adjust_toe();
}

// FUNC 0x001692C8 setting_adjust_stabiliser
int setting_adjust_stabiliser(void *ctx, ButtonData *data)
{
    return can_adjust_stabiliser();
}

// FUNC 0x001692E8 setting_adjust_brake_balance
int setting_adjust_brake_balance(void *ctx, ButtonData *data)
{
    return can_adjust_brake_balance();
}

// FUNC 0x00169308 setting_adjust_lsd_initial
int setting_adjust_lsd_initial(void *ctx, ButtonData *data)
{
    return can_adjust_lsd_initial();
}

// FUNC 0x00169328 setting_adjust_lsd_accel
int setting_adjust_lsd_accel(void *ctx, ButtonData *data)
{
    return can_adjust_lsd_accel();
}

// FUNC 0x00169348 setting_adjust_lsd_decel
int setting_adjust_lsd_decel(void *ctx, ButtonData *data)
{
    return can_adjust_lsd_decel();
}

// FUNC 0x00169368 setting_adjust_gear_ratio
int setting_adjust_gear_ratio(void *ctx, ButtonData *data)
{
    return can_adjust_final_gear();
}

// FUNC 0x00169388 setting_adjust_aero
int setting_adjust_aero(void *ctx, ButtonData *data)
{
    return can_adjust_aero();
}

// FUNC 0x001693A8 setting_adjust_ayc
int setting_adjust_ayc(void *ctx, ButtonData *data)
{
    return can_adjust_ayc();
}

// FUNC 0x001693C8 setting_adjust_asm
int setting_adjust_asm(void *ctx, ButtonData *data)
{
    return can_adjust_asm();
}

// FUNC 0x001693E8 setting_adjust_tcs
int setting_adjust_tcs(void *ctx, ButtonData *data)
{
    return can_adjust_tcs();
}

// FUNC 0x00169408 setting_adjust_vcd
int setting_adjust_vcd(void *ctx, ButtonData *data)
{
    return can_adjust_vcd();
}

/* ------------------------------------------------------------------------ */
/* The page table                                                           */
/* ------------------------------------------------------------------------ */

/* The check lists of the pages: .sdata 0x00351998..0x003519E0 (the lists of
 * one or two checks, 8 bytes each under -G8: g_003519D8's one word is
 * followed by 4 zero bytes, arrays being 8-aligned) and .rodata
 * 0x00322F08..0x00323040 (the longer ones).  File-local consts: retail has
 * them after T084's "" and table count (.sdata) and after its type names
 * (.rodata), where egcs writes a const with internal linkage at the end of
 * the unit (u008's T049, u014's T101; one with an extern declaration would
 * come at its definition).  Written in address order, which is one
 * definition order for both sections (egcs's end-of-unit pass writes the
 * referenced tables in definition order, each to .sdata or .rodata by its
 * size): the setting menu's order of the parts, engine, suspension, brakes,
 * drive train, tyres, others, then the "set/adjast_*" lists. */
typedef int (*SettingCheckFunc)(void *ctx, ButtonData *data);  /* (the address scanner
                                               reads the definitions below only so) */
static const SettingCheckFunc g_00351998[2] = {   /* 0x00351998 "chg/change_eng_computer" */
    setting_owns_cpu_off, setting_owns_cpu_on
};
static const SettingCheckFunc g_00322F08[4] = {   /* 0x00322F08 "chg/change_eng_muffler" */
    setting_owns_muf_normal, setting_owns_muf_sports, setting_owns_muf_semi_racing,
    setting_owns_muf_racing
};
static const SettingCheckFunc g_00322F18[5] = {   /* 0x00322F18 "chg/change_trb_turbine" */
    setting_owns_tur_off, setting_owns_tur_stage1, setting_owns_tur_stage2,
    setting_owns_tur_stage3, setting_owns_tur_stage4
};
static const SettingCheckFunc g_00322F30[3] = {   /* 0x00322F30 "chg/change_trb_inter" */
    setting_owns_coo_normal, setting_owns_coo_1, setting_owns_coo_2
};
static const SettingCheckFunc g_00322F40[4] = {   /* 0x00322F40 "chg/change_eng_na" */
    setting_owns_nat_off, setting_owns_nat_stage1, setting_owns_nat_stage2,
    setting_owns_nat_stage3
};
static const SettingCheckFunc g_00322F50[4] = {   /* 0x00322F50 "chg/change_sus_kit" */
    setting_owns_sus_normal, setting_owns_sus_sports, setting_owns_sus_semi_racing,
    setting_owns_sus_racing
};
static const SettingCheckFunc g_003519A0[2] = {   /* 0x003519A0 "chg/change_brk_kit" */
    setting_owns_brk_normal, setting_owns_brk_racing
};
static const SettingCheckFunc g_003519A8[2] = {   /* 0x003519A8 "chg/change_brk_balance" */
    setting_owns_brc_off, setting_owns_brc_on
};
static const SettingCheckFunc g_00322F60[4] = {   /* 0x00322F60 "chg/change_drv_mission" */
    setting_owns_msn_normal, setting_owns_msn_close, setting_owns_msn_super_close,
    setting_owns_msn_variable
};
static const SettingCheckFunc g_00322F70[4] = {   /* 0x00322F70 "chg/change_drv_clutch" */
    setting_owns_clt_normal, setting_owns_clt_big, setting_owns_clt_twin,
    setting_owns_clt_triple
};
static const SettingCheckFunc g_00322F80[4] = {   /* 0x00322F80 "chg/change_drv_flywheel" */
    setting_owns_fly_normal, setting_owns_fly_light, setting_owns_fly_crmo,
    setting_owns_fly_light_crmo
};
/* (1way before 15way and 2way here, not in the functions' order) */
static const SettingCheckFunc g_00322F90[6] = {   /* 0x00322F90 "chg/change_drv_lsd" */
    setting_owns_lsd_normal, setting_owns_lsd_1way, setting_owns_lsd_15way,
    setting_owns_lsd_2way, setting_owns_lsd_variable, setting_owns_lsd_aycc
};
static const SettingCheckFunc g_003519B0[2] = {   /* 0x003519B0 "chg/change_drv_shaft" */
    setting_owns_shf_normal, setting_owns_shf_propeller
};
static const SettingCheckFunc g_00322FA8[10] = {  /* 0x00322FA8 "chg/change_tir_front" */
    setting_owns_tir_f_normal, setting_owns_tir_f_control, setting_owns_tir_f_sports,
    setting_owns_tir_f_racing_sh, setting_owns_tir_f_racing_h, setting_owns_tir_f_racing_mh,
    setting_owns_tir_f_racing_m, setting_owns_tir_f_racing_ms, setting_owns_tir_f_racing_s,
    setting_owns_tir_f_racing_ss
};
/* the rear tyre page: the same ten front-tyre checks */
static const SettingCheckFunc g_00322FD0[10] = {  /* 0x00322FD0 "chg/change_tir_rear" */
    setting_owns_tir_f_normal, setting_owns_tir_f_control, setting_owns_tir_f_sports,
    setting_owns_tir_f_racing_sh, setting_owns_tir_f_racing_h, setting_owns_tir_f_racing_mh,
    setting_owns_tir_f_racing_m, setting_owns_tir_f_racing_ms, setting_owns_tir_f_racing_s,
    setting_owns_tir_f_racing_ss
};
static const SettingCheckFunc g_003519B8[2] = {   /* 0x003519B8 "chg/change_oth_aycc" */
    setting_owns_ayc_off, setting_owns_ayc_on
};
static const SettingCheckFunc g_003519C0[2] = {   /* 0x003519C0 "chg/change_oth_asm" */
    setting_owns_asm_off, setting_owns_asm_on
};
static const SettingCheckFunc g_003519C8[2] = {   /* 0x003519C8 "chg/change_oth_tcsc" */
    setting_owns_tcs_off, setting_owns_tcs_on
};
static const SettingCheckFunc g_003519D0[2] = {   /* 0x003519D0 "chg/change_oth_vcdc" */
    setting_owns_vcd_off, setting_owns_vcd_on
};
/* bound / rebound twice */
static const SettingCheckFunc g_00322FF8[8] = {   /* 0x00322FF8 "set/adjast_sus" */
    setting_adjust_spring, setting_adjust_height, setting_adjust_damper,
    setting_adjust_bound_rebound, setting_adjust_bound_rebound, setting_adjust_camber,
    setting_adjust_toe, setting_adjust_stabiliser
};
static const SettingCheckFunc g_003519D8[1] = {   /* 0x003519D8 "set/adjast_brk" */
    setting_adjust_brake_balance
};
static const SettingCheckFunc g_00323018[4] = {   /* 0x00323018 "set/adjast_drv" */
    setting_adjust_lsd_initial, setting_adjust_lsd_accel, setting_adjust_lsd_decel,
    setting_adjust_gear_ratio
};
static const SettingCheckFunc g_00323028[5] = {   /* 0x00323028 "set/adjast_oth" */
    setting_adjust_aero, setting_adjust_ayc, setting_adjust_asm, setting_adjust_tcs,
    setting_adjust_vcd
};

/* The table, .data 0x002AD058..0x002AD16C (23 entries of 0xC bytes), zero in
 * the image: T084_static_init fills it at start-up.  A C++ global without an
 * initialiser: zeros at the end of the file's .data, before the frame info
 * (as T049's g_002959E8). */
SelectableFuncData g_002AD058[23];          /* 0x002AD058 */

/* The table's entry count, .sdata 0x00351994 (a constant word, 23), read by
 * SettingControl::begin (T080).  Its place in .sdata, after
 * car_meets_event_regulations's "" (0x00351990) and before the check lists
 * (0x00351998 .., written at the end of the unit), is where egcs writes an
 * extern-declared const int: at its definition. */
const int g_00351994 = sizeof(g_002AD058) / sizeof(g_002AD058[0]);

/* One table entry: the inline constructor of the original's
 * SelectableFuncData (see T084_static_init); the fields in member order.
 * The lists are const; the table's member is not (T211), hence the cast (no
 * code). */
static inline void set_page(SelectableFuncData *e, const char *page,
                            const SettingCheckFunc *checks, int count)
{
    e->page = page;
    e->checks = (SettingCheckFunc *)checks;
    e->count = count;
}

/* One element as egcs's build_vec_init expands it (u006 / u008's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped. */
#define PAGE(page, checks, count)                                             \
    { SelectableFuncData *e = base; set_page(e, page, checks, count); }       \
    base++;                                                                   \
    left--

/* Static initialiser of g_002AD058 (.ctors entry 0x0028F0AC), sorted by page
 * name ("adjast" [sic]).  Compiler-generated in the original: the table was
 * defined here with SelectableFuncData objects built by an inline
 * constructor, which egcs builds at run time (cp/init.c build_vec_init).
 * Written out the way build_vec_init expands it (a walking cursor, a copy of
 * it per element, the dead count of the elements left): an exact form, not
 * the original; it stores the same 23 entries.  Note "chg/change_tir_rear"
 * gets the front tyre checks: g_00322FD0 holds the same ten functions as
 * g_00322FA8, so a rear tyre button is available when the FRONT tyre part is
 * owned (CarInformation::addPart gives both). */
// FUNC 0x00169428 T084_static_init
void T084_static_init(void)
{
    SelectableFuncData *base = g_002AD058;
    int left = 22;                          /* build_vec_init's iterator: elements - 1 */

    PAGE("chg/change_brk_balance",  g_003519A8, 2);
    PAGE("chg/change_brk_kit",      g_003519A0, 2);
    PAGE("chg/change_drv_clutch",   g_00322F70, 4);
    PAGE("chg/change_drv_flywheel", g_00322F80, 4);
    PAGE("chg/change_drv_lsd",      g_00322F90, 6);
    PAGE("chg/change_drv_mission",  g_00322F60, 4);
    PAGE("chg/change_drv_shaft",    g_003519B0, 2);
    PAGE("chg/change_eng_computer", g_00351998, 2);
    PAGE("chg/change_eng_muffler",  g_00322F08, 4);
    PAGE("chg/change_eng_na",       g_00322F40, 4);
    PAGE("chg/change_oth_asm",      g_003519C0, 2);
    PAGE("chg/change_oth_aycc",     g_003519B8, 2);
    PAGE("chg/change_oth_tcsc",     g_003519C8, 2);
    PAGE("chg/change_oth_vcdc",     g_003519D0, 2);
    PAGE("chg/change_sus_kit",      g_00322F50, 4);
    PAGE("chg/change_tir_front",    g_00322FA8, 10);
    PAGE("chg/change_tir_rear",     g_00322FD0, 10);
    PAGE("chg/change_trb_inter",    g_00322F30, 3);
    PAGE("chg/change_trb_turbine",  g_00322F18, 5);
    PAGE("set/adjast_brk",          g_003519D8, 1);
    PAGE("set/adjast_drv",          g_00323018, 4);
    PAGE("set/adjast_oth",          g_00323028, 5);
    PAGE("set/adjast_sus",          g_00322FF8, 8);
}

/* ------------------------------------------------------------------------ */
/* RegurationResult (T045): this TU's copies of its inline destructor and   */
/* typeinfo function, both unreferenced.                                    */
/* ------------------------------------------------------------------------ */

// FUNC 0x00169710 RegurationResult::~RegurationResult
/* inline in game/menu/gtmode/T045_gtmode_page_checks.h; egcs emits this copy (T084's,
 * unreferenced: the vtables use T045's 0x0011C958): the vptr (+0x1C) =
 * 0x00310310 (T045's vtable, not T084's copy 0x00322CD8: the weak vtables
 * resolve to the first), then ::operator delete (0x0027C0C0) if flags & 1. */
/* implicit: operator delete (flags & 1) */

#ifndef BOOT_OLD_GCC
/* T084's copy of RegurationResult's typeinfo function (unreferenced; T045's
 * is 0x0011C988).  Weak: the T045 definition wins at link time. */
// FUNC 0x00169740 __tf16RegurationResult
extern "C" __attribute__((weak)) void *__tf16RegurationResult(void)
{
    if (__ti16RegurationResult[0] == 0)                 /* _name not set yet */
        __rtti_user(__ti16RegurationResult, "16RegurationResult");
    return __ti16RegurationResult;
}
#endif
