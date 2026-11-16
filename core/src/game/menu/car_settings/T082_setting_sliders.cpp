/* T082 - the slider functions of the car setting screen
 * (gt2k_menu::SettingControl, "menu/setting.mbl").  0x001638F8..0x00166F98.
 *
 * Pairs of {set, func} per slider element, the gear ratio helpers, and the
 * static initialiser of the MenuSliderFunc table g_002ABAB0 (see setting_sliders_internal.h).
 * Names: the element ("slider_<name>") each func is registered under.
 *
 * settingCar below is GameStatus's setting car (MenuStatus::raceCar[0]);
 * ctl->car is the SettingControl's working copy of it.  The settings are
 * CarSaveEquipments words (T482; Lupo 2's names), the ranges the fields of
 * the record of the fitted part: a {count, from, to} setting gives the
 * levels 1..count, a {min, max} one min..max (desc.max is exclusive: the
 * record's + 1).
 *
 * Forms the listing shows (matching pass, ref/matching/u035/notes.md): the
 * setting car is read through the inline GameStatus::getMenuStatus() (menu's
 * 0x307C8 added, then raceCar[0] at +0x540); a func returns
 * SliderDesc(...), built in the caller's buffer, per branch; the cars'
 * settings are read and written through their CarEquipments base (+0x10,
 * added once and kept for the record getter calls and the loads and
 * stores).  The original most likely called CarEquipments' inline setting
 * accessors here (getSetting* / setSetting* / getMaxLevel* / *Min / *Max:
 * T482 has out-of-line copies of them, in reverse order and never called);
 * the canonical header declares them out of line, so the base is a local
 * pointer or reference.
 *
 * Includes: retail's type names (.rodata 0x00322090..0x003222F8, after the
 * table's strings; reverse definition order) are SettingControl,
 * SmartIndexT<int>, List<Button>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * RegurationResult, the garage lists (GarageWheelList .. Cell<GarageCar>) and
 * Operator (a non-polymorphic base: last), the same as T081's: so the
 * GameStatus first, RegurationResult (no code here uses the class), the menu
 * item classes (T206: T216's operator classes, Button's base Cell<Button>),
 * List<Button>, SmartIndexT<int> after it (ButtonList's header: its
 * List<Button> base, then its `index' member, which instantiates
 * SmartIndexT<int>: T251_smart_index_template.h) and SettingControl last.
 * The headers after T080's add no class name. */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getMenuStatus; (seen: the
                                               garage classes) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#include "game/menu/framework/T206_button.h"               /* (seen: T216's operator classes, Cell<Button>) */
#include "game/menu/framework/menu_list.h"                 /* (seen: List<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* (seen: ButtonList: List<Button>, then SmartIndexT<int>) */
#include "game/menu/car_settings/T080_setting_control.h"      /* gt2k_menu::SettingControl (car, gearSliders); GarageCar */
#include "game/menu/car_settings/T082_setting_sliders.h"      /* the slider functions, g_002ABAB0, g_00351970,
                                               T082_static_init */
#include <stddef.h>                         /* NULL */
#include "game/menu/car_settings/T084_setting_checks.h"       /* car_meets_event_regulations (the regulation check) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuSliderFunc */
#include "game/menu/framework/T238_slider.h"               /* gt2k_menu::Slider, SliderDesc, SliderSetFn */
#include "game/car/physics/T294_gear_auto.h"            /* gear_auto_ratios, gear_auto_ranges (the auto gear ratios) */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus::raceCar[0] (the setting car) */
#include "game/data/T482_car_equipments.h"       /* CarEquipments, CarSaveEquipments (the settings), the part
                                               records, CarSpec, PART_LSD_AYC */
#include "game/data/T483_car_information.h"      /* CarInformation (GarageCar's base) */
#include "game/menu/car_settings/setting_sliders_internal.h"

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* Dampers: two stages (F1 / F2) of bound (B) and rebound (R) per axle       */
/* ------------------------------------------------------------------------ */

/* "slider_damper_f": bound and rebound together (the four front words). */
// FUNC 0x001638F8 setting_slider_set_damper_f
int setting_slider_set_damper_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.frontDamperF1B = value;
        eq.frontDamperF2B = value;
        eq.frontDamperF1R = value;
        eq.frontDamperF2R = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

/* 1..the front bound levels; the value is the first bound stage. */
// FUNC 0x00163970 setting_slider_damper_f
SliderDesc setting_slider_damper_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->damperFCount + 1,
                          eq->frontDamperF1B, 1,
                          setting_slider_set_damper_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_damper_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_damper_r" */
// FUNC 0x00163A20 setting_slider_set_damper_r
int setting_slider_set_damper_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.rearDamperF1B = value;
        eq.rearDamperF2B = value;
        eq.rearDamperF1R = value;
        eq.rearDamperF2R = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00163A98 setting_slider_damper_r
SliderDesc setting_slider_damper_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->damperRCount + 1,
                          eq->rearDamperF1B, 1,
                          setting_slider_set_damper_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_damper_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_bound_f": both bound stages. */
// FUNC 0x00163B48 setting_slider_set_bound_f
int setting_slider_set_bound_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.frontDamperF1B = value;
        eq.frontDamperF2B = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00163BB8 setting_slider_bound_f
SliderDesc setting_slider_bound_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->damperFCount + 1,
                          eq->frontDamperF1B, 1,
                          setting_slider_set_bound_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_bound_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_bound_r" */
// FUNC 0x00163C68 setting_slider_set_bound_r
int setting_slider_set_bound_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.rearDamperF1B = value;
        eq.rearDamperF2B = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00163CD8 setting_slider_bound_r
SliderDesc setting_slider_bound_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->damperRCount + 1,
                          eq->rearDamperF1B, 1,
                          setting_slider_set_bound_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_bound_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_rebound_f": both rebound stages. */
// FUNC 0x00163D88 setting_slider_set_rebound_f
int setting_slider_set_rebound_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.frontDamperF1R = value;
        eq.frontDamperF2R = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00163DF8 setting_slider_rebound_f
SliderDesc setting_slider_rebound_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->reboundFCount + 1,
                          eq->frontDamperF1R, 1,
                          setting_slider_set_rebound_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_rebound_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_rebound_r" */
// FUNC 0x00163EA8 setting_slider_set_rebound_r
int setting_slider_set_rebound_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        eq.rearDamperF1R = value;
        eq.rearDamperF2R = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00163F18 setting_slider_rebound_r
SliderDesc setting_slider_rebound_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->reboundRCount + 1,
                          eq->rearDamperF1R, 1,
                          setting_slider_set_rebound_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_rebound_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* Brake balance (the brake controller's levels)                            */
/* ------------------------------------------------------------------------ */

/* "slider_brake_f" */
// FUNC 0x00163FC8 setting_slider_set_brake_f
int setting_slider_set_brake_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontABS = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164030 setting_slider_brake_f
SliderDesc setting_slider_brake_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getBrakecontrollerRecord()->countF + 1,
                          eq->frontABS, 1,
                          setting_slider_set_brake_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_brake_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_brake_r" */
// FUNC 0x001640E0 setting_slider_set_brake_r
int setting_slider_set_brake_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearABS = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164148 setting_slider_brake_r
SliderDesc setting_slider_brake_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getBrakecontrollerRecord()->countR + 1,
                          eq->rearABS, 1,
                          setting_slider_set_brake_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_brake_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* Camber, ride height, spring rate, stabilisers, toe                       */
/* ------------------------------------------------------------------------ */

/* "slider_camber_f" */
// FUNC 0x001641F8 setting_slider_set_camber_f
int setting_slider_set_camber_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontCamber = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164260 setting_slider_camber_f
SliderDesc setting_slider_camber_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->camberFMin, eq->getSuspensionRecord()->camberFMax + 1,
                          eq->frontCamber, 1,
                          setting_slider_set_camber_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_camber_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_camber_r" */
// FUNC 0x00164320 setting_slider_set_camber_r
int setting_slider_set_camber_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearCamber = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164388 setting_slider_camber_r
SliderDesc setting_slider_camber_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->camberRMin, eq->getSuspensionRecord()->camberRMax + 1,
                          eq->rearCamber, 1,
                          setting_slider_set_camber_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_camber_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_height_f": ride height. */
// FUNC 0x00164448 setting_slider_set_height_f
int setting_slider_set_height_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontRideheight = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x001644B0 setting_slider_height_f
SliderDesc setting_slider_height_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->heightFMin, eq->getSuspensionRecord()->heightFMax + 1,
                          eq->frontRideheight, 1,
                          setting_slider_set_height_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_height_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_height_r" */
// FUNC 0x00164570 setting_slider_set_height_r
int setting_slider_set_height_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearRideheight = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x001645D8 setting_slider_height_r
SliderDesc setting_slider_height_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->heightRMin, eq->getSuspensionRecord()->heightRMax + 1,
                          eq->rearRideheight, 1,
                          setting_slider_set_height_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_height_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_rate_f": spring rate. */
// FUNC 0x00164698 setting_slider_set_rate_f
int setting_slider_set_rate_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontSpringrate = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164700 setting_slider_rate_f
SliderDesc setting_slider_rate_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->springRateFMin, eq->getSuspensionRecord()->springRateFMax + 1,
                          eq->frontSpringrate, 1,
                          setting_slider_set_rate_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_rate_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_rate_r" */
// FUNC 0x001647C0 setting_slider_set_rate_r
int setting_slider_set_rate_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearSpringrate = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164828 setting_slider_rate_r
SliderDesc setting_slider_rate_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->springRateRMin, eq->getSuspensionRecord()->springRateRMax + 1,
                          eq->rearSpringrate, 1,
                          setting_slider_set_rate_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_rate_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_stabilizer_f" */
// FUNC 0x001648E8 setting_slider_set_stabilizer_f
int setting_slider_set_stabilizer_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontStabilizer = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

/* 1..the front stabiliser levels. */
// FUNC 0x00164950 setting_slider_stabilizer_f
SliderDesc setting_slider_stabilizer_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->stabilizerFCount + 1,
                          eq->frontStabilizer, 1,
                          setting_slider_set_stabilizer_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_stabilizer_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_stabilizer_r" */
// FUNC 0x00164A00 setting_slider_set_stabilizer_r
int setting_slider_set_stabilizer_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearStabilizer = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164A68 setting_slider_stabilizer_r
SliderDesc setting_slider_stabilizer_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getSuspensionRecord()->stabilizerRCount + 1,
                          eq->rearStabilizer, 1,
                          setting_slider_set_stabilizer_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_stabilizer_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_toe_f" (the setting is stored biased: 0x80 = no toe, u033's
 * TOE_ZERO). */
// FUNC 0x00164B18 setting_slider_set_toe_f
int setting_slider_set_toe_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontToe = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164B80 setting_slider_toe_f
SliderDesc setting_slider_toe_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->toeFMin, eq->getSuspensionRecord()->toeFMax + 1,
                          eq->frontToe, 1,
                          setting_slider_set_toe_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_toe_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_toe_r" */
// FUNC 0x00164C40 setting_slider_set_toe_r
int setting_slider_set_toe_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearToe = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164CA8 setting_slider_toe_r
SliderDesc setting_slider_toe_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getSuspensionRecord()->toeRMin, eq->getSuspensionRecord()->toeRMax + 1,
                          eq->rearToe, 1,
                          setting_slider_set_toe_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_toe_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* LSD: initial torque (param), acceleration (param2), deceleration        */
/* (param3) sensitivity                                                      */
/* ------------------------------------------------------------------------ */

/* "slider_lsd_init_f" */
// FUNC 0x00164D68 setting_slider_set_lsd_init_f
int setting_slider_set_lsd_init_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontLSDparam = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164DD0 setting_slider_lsd_init_f
SliderDesc setting_slider_lsd_init_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->initialFMin, eq->getLSDRecord()->initialFMax + 1,
                          eq->frontLSDparam, 1,
                          setting_slider_set_lsd_init_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_init_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_lsd_accl_f" */
// FUNC 0x00164E90 setting_slider_set_lsd_accl_f
int setting_slider_set_lsd_accl_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontLSDparam2 = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00164EF8 setting_slider_lsd_accl_f
SliderDesc setting_slider_lsd_accl_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->accelFMin, eq->getLSDRecord()->accelFMax + 1,
                          eq->frontLSDparam2, 1,
                          setting_slider_set_lsd_accl_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_accl_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_lsd_accl_r" */
// FUNC 0x00164FB8 setting_slider_set_lsd_accl_r
int setting_slider_set_lsd_accl_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearLSDparam2 = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00165020 setting_slider_lsd_accl_r
SliderDesc setting_slider_lsd_accl_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->accelRMin, eq->getLSDRecord()->accelRMax + 1,
                          eq->rearLSDparam2, 1,
                          setting_slider_set_lsd_accl_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_accl_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_lsd_decl_f" */
// FUNC 0x001650E0 setting_slider_set_lsd_decl_f
int setting_slider_set_lsd_decl_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.frontLSDparam3 = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00165148 setting_slider_lsd_decl_f
SliderDesc setting_slider_lsd_decl_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->decelFMin, eq->getLSDRecord()->decelFMax + 1,
                          eq->frontLSDparam3, 1,
                          setting_slider_set_lsd_decl_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_decl_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_lsd_decl_r" */
// FUNC 0x00165208 setting_slider_set_lsd_decl_r
int setting_slider_set_lsd_decl_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearLSDparam3 = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00165270 setting_slider_lsd_decl_r
SliderDesc setting_slider_lsd_decl_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->decelRMin, eq->getLSDRecord()->decelRMax + 1,
                          eq->rearLSDparam3, 1,
                          setting_slider_set_lsd_decl_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_decl_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_lsd_init_r" (the same setting as "slider_elec_aycc"). */
// FUNC 0x00165330 setting_slider_set_lsd_init_r
int setting_slider_set_lsd_init_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearLSDparam = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

/* No range when the working copy's LSD is the AYC: the rear initial torque
 * is then the AYC level ("slider_elec_aycc"). */
// FUNC 0x00165398 setting_slider_lsd_init_r
SliderDesc setting_slider_lsd_init_r(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && ctl->car.getLSDPart() != PART_LSD_AYC) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->initialRMin, eq->getLSDRecord()->initialRMax + 1,
                          eq->rearLSDparam, 1,
                          setting_slider_set_lsd_init_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_lsd_init_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* Gear ratios                                                              */
/* ------------------------------------------------------------------------ */
/* The ratios are x 1000 (CarSpec::gearRatio: [0] reverse, [1..gears]
 * forward).  The auto setting level (CarSaveEquipments::maxSpeed) x 10 is
 * the top speed in km/h T294 sets the ratios for.  The gear sliders read and
 * write the working copy; only the gear-count test reads the setting car. */

/* The gear ratios of auto setting level `level' for `car', into
 * ratios[0..7]. */
// FUNC 0x00165470 setting_gear_auto_ratios
void setting_gear_auto_ratios(GarageCar *car, int level, s16 *ratios)
{
    CarSpec spec;                           /* sp+0x000 (0x3E2 bytes, 0x3F0 on the stack) */

    car->getSpec(&spec);
    gear_auto_ratios(&spec, level * 10, ratios, 1);   /* clearAuto */
}

/* The range of gear `gear' of `car' at its auto level and the final gear
 * the level was set with: the whole scale lo[top gear]..hi[1] (*min,
 * *max), the gear's own lo[gear]..hi[gear] (*limitMin, *limitMax). */
// FUNC 0x001654C8 setting_gear_range
void setting_gear_range(GarageCar *car, int gear, int *min, int *max,
                        int *limitMin, int *limitMax)
{
    CarSpec spec;                           /* sp+0x000 */
    s16 lo[8];                              /* sp+0x3F0 */
    s16 hi[8];                              /* sp+0x400 */

    CarEquipments &eq = *car;
    eq.getSpec(&spec);
    gear_auto_ranges(&spec, eq.maxSpeed * 10, (s16)eq.lastFinalGearRatio, lo, hi);
    *min      = lo[eq.getGearRecord()->gears];
    *max      = hi[1];
    *limitMin = lo[gear];
    *limitMax = hi[gear];
}

/* "slider_gear_ratio_0": the reverse gear.  Slider N is offered when the
 * setting car's transmission has N gears or more: for N = 0 the test
 * (gears >= 0 on the u8, lbu + slti 0) is always true.  Each gear function
 * has its gear number in a variable, as the listing shows: egcs folds a
 * literal `gears >= 0' away at the tree level (call kept, test gone), and in
 * setting_slider_gear_ratio_1 the gear variable's 1 is the constant CSE
 * reuses for desc.step, which changes that function's registers. */
// FUNC 0x001655B0 setting_slider_set_gear_ratio_0
int setting_slider_set_gear_ratio_0(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 0;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

/* The whole scale with the gear's own range as the inner range. */
// FUNC 0x00165618 setting_slider_gear_ratio_0
SliderDesc setting_slider_gear_ratio_0(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 0;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_0, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_0, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_1" */
// FUNC 0x00165710 setting_slider_set_gear_ratio_1
int setting_slider_set_gear_ratio_1(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 1;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165770 setting_slider_gear_ratio_1
SliderDesc setting_slider_gear_ratio_1(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 1;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_1, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_1, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_2" */
// FUNC 0x00165860 setting_slider_set_gear_ratio_2
int setting_slider_set_gear_ratio_2(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 2;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x001658C8 setting_slider_gear_ratio_2
SliderDesc setting_slider_gear_ratio_2(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 2;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_2, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_2, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_3" */
// FUNC 0x001659C0 setting_slider_set_gear_ratio_3
int setting_slider_set_gear_ratio_3(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 3;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165A28 setting_slider_gear_ratio_3
SliderDesc setting_slider_gear_ratio_3(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 3;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_3, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_3, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_4" */
// FUNC 0x00165B20 setting_slider_set_gear_ratio_4
int setting_slider_set_gear_ratio_4(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 4;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165B88 setting_slider_gear_ratio_4
SliderDesc setting_slider_gear_ratio_4(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 4;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_4, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_4, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_5" */
// FUNC 0x00165C80 setting_slider_set_gear_ratio_5
int setting_slider_set_gear_ratio_5(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 5;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165CE8 setting_slider_gear_ratio_5
SliderDesc setting_slider_gear_ratio_5(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 5;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_5, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_5, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_6" */
// FUNC 0x00165DE0 setting_slider_set_gear_ratio_6
int setting_slider_set_gear_ratio_6(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 6;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165E48 setting_slider_gear_ratio_6
SliderDesc setting_slider_gear_ratio_6(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 6;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_6, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_6, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_7" */
// FUNC 0x00165F40 setting_slider_set_gear_ratio_7
int setting_slider_set_gear_ratio_7(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 7;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear)
        ctl->car.gearRatio[gear] = value;
    return 1;
}

// FUNC 0x00165FA8 setting_slider_gear_ratio_7
SliderDesc setting_slider_gear_ratio_7(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int gear = 7;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL && settingCar->getGearRecord()->gears >= gear) {
        int min, max, limitMin, limitMax;   /* sp+0x0, +0x4, +0x8, +0xC */
        setting_gear_range(&ctl->car, gear, &min, &max, &limitMin, &limitMax);
        return SliderDesc(min, max + 1, ctl->car.gearRatio[gear], 1,
                          setting_slider_set_gear_ratio_7, SLIDER_MODE_IMMEDIATE,
                          limitMin, limitMax + 1);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_7, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_gear_ratio_final" */
// FUNC 0x001660A0 setting_slider_set_gear_ratio_final
int setting_slider_set_gear_ratio_final(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL)
        ctl->car.finalGearRatio = value;
    return 1;
}

/* Range and value from the working copy: finalGearRatioMin ..
 * finalGearRatioMax, without the + 1 of the other sliders (max is
 * exclusive, so the record's maximum itself cannot be chosen). */
// FUNC 0x001660F0 setting_slider_gear_ratio_final
SliderDesc setting_slider_gear_ratio_final(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        return SliderDesc(eq.getGearRecord()->finalGearRatioMin, eq.getGearRecord()->finalGearRatioMax,
                          eq.finalGearRatio, 1,
                          setting_slider_set_gear_ratio_final, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_ratio_final, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* A new auto setting level: every gear slider of the page (gear i at
 * gearSliders[i - 1], i = 1..gears) gets the new ranges (Slider::setRange)
 * and the ratio of the level (Slider::setValue, which commits it through
 * setting_slider_set_gear_ratio_<i>).  The ratios are those of `level'; the
 * ranges come from the working copy's maxSpeed (the caller has just stored
 * `level' there) and the final gear of the previous level. */
// FUNC 0x001661B0 setting_gear_apply_auto_level
void setting_gear_apply_auto_level(SettingControl *ctl, int level)
{
    s16 ratios[8];                          /* sp+0x00 */
    s16 lo[8];                              /* sp+0x10 */
    s16 hi[8];                              /* sp+0x20 */
    CarSpec spec;                           /* sp+0x30 */

    setting_gear_auto_ratios(&ctl->car, level, ratios);
    CarEquipments &eq = ctl->car;
    eq.getSpec(&spec);
    gear_auto_ranges(&spec, eq.maxSpeed * 10, (s16)eq.lastFinalGearRatio, lo, hi);
    int gears = eq.getGearRecord()->gears;
    /* An empty loop of 8 passes: egcs compiles this one to the binary's
     * counter 7..0 with the four nops (checked with ee-gcc 2.9).  ? A loop
     * over the 8 gears whose body was compiled out (a debug print); u034's
     * gear_setup (0x001614D8) ends with the same loop. */
    for (int i = 0; i < 8; i++)
        EMPTY_LOOP_BODY();
    for (int gear = 1; gear <= gears; gear++) {
        /* the slider pointer is read again for each call */
        if (ctl->gearSliders[gear - 1] != NULL) {
            ctl->gearSliders[gear - 1]->setRange(lo[gears], hi[1] + 1, lo[gear], hi[gear] + 1);
            ctl->gearSliders[gear - 1]->setValue(ratios[gear]);
        }
    }
}

/* "slider_gear_auto_level": stores the level, moves the gear sliders to its
 * ratios, then remembers the final gear the level was set with. */
// FUNC 0x001662F0 setting_slider_set_gear_auto_level
int setting_slider_set_gear_auto_level(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;

    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return 0;
    CarEquipments &eq = ctl->car;
    eq.maxSpeed = value;
    setting_gear_apply_auto_level(ctl, value);
    eq.lastFinalGearRatio = eq.finalGearRatio;
    return 1;
}

/* Range and value from the working copy: maxSpeedMin..maxSpeedMax. */
// FUNC 0x00166368 setting_slider_gear_auto_level
SliderDesc setting_slider_gear_auto_level(void *ctx)
{
    SettingControl *ctl = (SettingControl *)ctx;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        CarEquipments &eq = ctl->car;
        return SliderDesc(eq.getGearRecord()->maxSpeedMin, eq.getGearRecord()->maxSpeedMax + 1,
                          eq.maxSpeed, 1,
                          setting_slider_set_gear_auto_level, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_gear_auto_level, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* Aero (downforce: the racing modification's clF / clR)                    */
/* ------------------------------------------------------------------------ */

/* "slider_aero_f" */
// FUNC 0x00166430 setting_slider_set_aero_f
int setting_slider_set_aero_f(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.clF = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00166498 setting_slider_aero_f
SliderDesc setting_slider_aero_f(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getRacingmodifyRecord()->aeroFMin, eq->getRacingmodifyRecord()->aeroFMax + 1,
                          eq->clF, 1,
                          setting_slider_set_aero_f, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_aero_f, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_aero_r" */
// FUNC 0x00166558 setting_slider_set_aero_r
int setting_slider_set_aero_r(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.clR = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x001665C0 setting_slider_aero_r
SliderDesc setting_slider_aero_r(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getRacingmodifyRecord()->aeroRMin, eq->getRacingmodifyRecord()->aeroRMax + 1,
                          eq->clR, 1,
                          setting_slider_set_aero_r, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_aero_r, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* Electronic controllers                                                   */
/* ------------------------------------------------------------------------ */

/* "slider_elec_aycc": the AYC level is the rear LSD initial torque. */
// FUNC 0x00166680 setting_slider_set_elec_aycc
int setting_slider_set_elec_aycc(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.rearLSDparam = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

/* The LSD record's rear initial range (no AYC test, unlike lsd_init_r). */
// FUNC 0x001666E8 setting_slider_elec_aycc
SliderDesc setting_slider_elec_aycc(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getLSDRecord()->initialRMin, eq->getLSDRecord()->initialRMax + 1,
                          eq->rearLSDparam, 1,
                          setting_slider_set_elec_aycc, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_elec_aycc, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_elec_ascc" */
// FUNC 0x001667A8 setting_slider_set_elec_ascc
int setting_slider_set_elec_ascc(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.settingASCC = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

/* 1..the ASCC record's first setting's levels (count0). */
// FUNC 0x00166810 setting_slider_elec_ascc
SliderDesc setting_slider_elec_ascc(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getASCCRecord()->count0 + 1,
                          eq->settingASCC, 1,
                          setting_slider_set_elec_ascc, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_elec_ascc, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_elec_tcsc": the slider is reversed, the stored level is
 * count + 1 - value (count from the setting car's TCSC record).  egcs's fold
 * turns `count - (value - 1)' into (count + 1) - value; retail subtracts
 * value - 1, which is what `count + 1 - value' folds to.  The working copy's
 * address is taken before the record call (retail keeps it in $s0 across
 * it) and passed to the check, the store goes through ctl. */
// FUNC 0x001668C0 setting_slider_set_elec_tcsc
int setting_slider_set_elec_tcsc(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        GarageCar *car = &ctl->car;
        ctl->car.settingTCSC = settingCar->getTCSCRecord()->count + 1 - value;
        accepted = car_meets_event_regulations(car);
    }
    return accepted;
}

/* 1..count, showing count + 1 - settingTCSC (the same fold as the setter). */
// FUNC 0x00166940 setting_slider_elec_tcsc
SliderDesc setting_slider_elec_tcsc(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(1, eq->getTCSCRecord()->count + 1,
                          eq->getTCSCRecord()->count + 1 - eq->settingTCSC, 1,
                          setting_slider_set_elec_tcsc, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_elec_tcsc, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* "slider_elec_vcdc": the 4WD torque split (the drive train's VCD). */
// FUNC 0x00166A08 setting_slider_set_elec_vcdc
int setting_slider_set_elec_vcdc(void *ctx, int value, int commit)
{
    SettingControl *ctl = (SettingControl *)ctx;
    int accepted = 0;

    if (getGameStatus()->getMenuStatus()->raceCar[0] != NULL) {
        ctl->car.param4WD = value;
        accepted = car_meets_event_regulations(&ctl->car);
    }
    return accepted;
}

// FUNC 0x00166A70 setting_slider_elec_vcdc
SliderDesc setting_slider_elec_vcdc(void *ctx)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];

    if (settingCar != NULL) {
        CarEquipments *eq = settingCar;
        return SliderDesc(eq->getDrivetrainRecord()->vcdMin, eq->getDrivetrainRecord()->vcdMax + 1,
                          eq->param4WD, 1,
                          setting_slider_set_elec_vcdc, SLIDER_MODE_IMMEDIATE, 0, 0);
    }
    return SliderDesc(0, 1, 0, 1, setting_slider_set_elec_vcdc, SLIDER_MODE_IMMEDIATE, 0, 0);
}

/* ------------------------------------------------------------------------ */
/* The table                                                                */
/* ------------------------------------------------------------------------ */

/* The table, .data 0x002ABAB0..0x002ABBF0 (40 entries of 8 bytes): no
 * initialiser, so egcs writes its zeros in .data in front of the frame info
 * (retail's are zero too: T082_static_init fills it at boot; as T081's
 * g_002A9080). */
MenuSliderFunc g_002ABAB0[40];              /* 0x002ABAB0 */

/* The table's entry count, .sdata 0x00351970: a constant word (40), read by
 * SettingControl::begin (T080); defined here, before the initialiser (T082's
 * only small data).  Only T080's code reads it, so T082's .sdata starts
 * unreferenced (the anchor gives the word to T082's range, not T081's). */
// DATA_START 0x00351970
const int g_00351970 = sizeof(g_002ABAB0) / sizeof(g_002ABAB0[0]);

/* One table entry: the inline constructor of the original's MenuSliderFunc
 * (see T082_static_init). */
static inline void set_slider(MenuSliderFunc *e, const char *name, SliderDesc (*func)(void *ctx))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006's form, as u022's
 * T065): the constructor on a copy of the cursor, then the cursor and the
 * count of the elements left stepped. */
#define SLIDER(name, func)                                                    \
    { MenuSliderFunc *e = base; set_slider(e, name, func); }                  \
    base++;                                                                   \
    left--

/* Static initialiser of g_002ABAB0 (.ctors entry 0x0028F0A4): a
 * compiler-generated _GLOBAL_$I$ function in the original, which defined
 * the table here with MenuSliderFunc objects built by an inline constructor
 * (built at run time by cp/init.c build_vec_init; retail's .data is zero).
 * The decomp defines the table without an initialiser (above) and writes
 * the function out the way build_vec_init expands it (a
 * walking cursor, a copy of it per element, the dead count of the elements
 * left): an exact form, not the original; it stores the same 40 entries
 * (strings 0x00321D50..0x00322080). */
// FUNC 0x00166B30 T082_static_init
void T082_static_init(void)
{
    MenuSliderFunc *base = g_002ABAB0;
    int left = 39;                          /* build_vec_init's iterator: elements - 1 */

    SLIDER("slider_aero_f",           setting_slider_aero_f);
    SLIDER("slider_aero_r",           setting_slider_aero_r);
    SLIDER("slider_bound_f",          setting_slider_bound_f);
    SLIDER("slider_bound_r",          setting_slider_bound_r);
    SLIDER("slider_brake_f",          setting_slider_brake_f);
    SLIDER("slider_brake_r",          setting_slider_brake_r);
    SLIDER("slider_camber_f",         setting_slider_camber_f);
    SLIDER("slider_camber_r",         setting_slider_camber_r);
    SLIDER("slider_damper_f",         setting_slider_damper_f);
    SLIDER("slider_damper_r",         setting_slider_damper_r);
    SLIDER("slider_elec_ascc",        setting_slider_elec_ascc);
    SLIDER("slider_elec_aycc",        setting_slider_elec_aycc);
    SLIDER("slider_elec_tcsc",        setting_slider_elec_tcsc);
    SLIDER("slider_elec_vcdc",        setting_slider_elec_vcdc);
    SLIDER("slider_gear_auto_level",  setting_slider_gear_auto_level);
    SLIDER("slider_gear_ratio_0",     setting_slider_gear_ratio_0);
    SLIDER("slider_gear_ratio_1",     setting_slider_gear_ratio_1);
    SLIDER("slider_gear_ratio_2",     setting_slider_gear_ratio_2);
    SLIDER("slider_gear_ratio_3",     setting_slider_gear_ratio_3);
    SLIDER("slider_gear_ratio_4",     setting_slider_gear_ratio_4);
    SLIDER("slider_gear_ratio_5",     setting_slider_gear_ratio_5);
    SLIDER("slider_gear_ratio_6",     setting_slider_gear_ratio_6);
    SLIDER("slider_gear_ratio_7",     setting_slider_gear_ratio_7);
    SLIDER("slider_gear_ratio_final", setting_slider_gear_ratio_final);
    SLIDER("slider_height_f",         setting_slider_height_f);
    SLIDER("slider_height_r",         setting_slider_height_r);
    SLIDER("slider_lsd_accl_f",       setting_slider_lsd_accl_f);
    SLIDER("slider_lsd_accl_r",       setting_slider_lsd_accl_r);
    SLIDER("slider_lsd_decl_f",       setting_slider_lsd_decl_f);
    SLIDER("slider_lsd_decl_r",       setting_slider_lsd_decl_r);
    SLIDER("slider_lsd_init_f",       setting_slider_lsd_init_f);
    SLIDER("slider_lsd_init_r",       setting_slider_lsd_init_r);
    SLIDER("slider_rate_f",           setting_slider_rate_f);
    SLIDER("slider_rate_r",           setting_slider_rate_r);
    SLIDER("slider_rebound_f",        setting_slider_rebound_f);
    SLIDER("slider_rebound_r",        setting_slider_rebound_r);
    SLIDER("slider_stabilizer_f",     setting_slider_stabilizer_f);
    SLIDER("slider_stabilizer_r",     setting_slider_stabilizer_r);
    SLIDER("slider_toe_f",            setting_slider_toe_f);
    SLIDER("slider_toe_r",            setting_slider_toe_r);
}
