/* T083 - the page actions of the car setting screen (gt2k_menu::
 * SettingControl, T080).  0x00166F98..0x001677A0.
 *
 * T083_static_init fills g_002ACCC8 with 77 ExecuteFuncData {name, func}
 * entries sorted by name (count g_00351978); SettingControl::begin (T080,
 * 0x0015CA98) gives it to its PageManager (executeFuncs), ctx the
 * SettingControl; PageManager::callExecuteFunc (T211) calls func(ctx, rec,
 * index, layout) for a decided button whose exec name (ButtonData::exec) is
 * the key.  None of the actions reads index or layout.  The control works
 * on its own copy of the setting car (SettingControl::car); "commit_slider"
 * and the "set_*" actions copy it back to GameStatus's setting car
 * (MenuStatus::raceCar[0], GarageCar::assign), "retrieve_slider" copies the
 * other way. */
/* Includes: retail's type names (.rodata 0x00322880..0x00322AD0, reverse
 * definition order) are SettingControl, List<Button>, Cell<Button>,
 * OperatorList, List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SmartIndexT<int>, the garage lists (GarageWheelList .. Cell<GarageCar>)
 * and Operator (a non-polymorphic base: last); no RegurationResult.  So the
 * GameStatus first, then this unit's header (T211's PageManager:
 * SmartIndexT<int>), the menu item classes (T206: T216's operator classes,
 * Button's base Cell<Button>), ButtonList (List<Button>) and SettingControl
 * last.  The headers after T080's add no class name. */
/* Retail's .sdata: g_00351978 (the table count), read only by T080's code
 * (SettingControl::begin), so its address is anchored here (u033's data
 * pass, ref/data/u036/pending.md). */
// DATA_START 0x00351978
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::getMenuStatus; (seen: the
                                               garage classes) */
#include "game/menu/car_settings/T083_setting_actions.h"      /* the page actions, g_002ACCC8, g_00351978 (T211
                                               ExecuteFuncData; SmartIndexT<int>) */
#include "game/menu/framework/T206_button.h"               /* (seen: T216's operator classes, Cell<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* ButtonData::exec (List<Button>) */
#include "game/menu/car_settings/T080_setting_control.h"      /* SettingControl: car, request, changed; GarageCar::assign */
#include "game/data/T203_part_names.h"           /* setting_id_of_name */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_* */
#include "game/menu/framework/T238_slider.h"               /* g_003526B0 (Slider::hidePrev's flag) */
#include "game/save/T427_menu_status.h"          /* MenuStatus::raceCar[0] */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::fitPart */
#include "game/data/T483_car_information.h"      /* CarInformation::hasPart */
#include "game/menu/car_settings/setting_actions_internal.h"

using namespace gt2k_menu;

/* "commit_slider": the working copy (the slider values) becomes the
 * setting car; then g_003526B0 = 1, as Slider::hidePrev (T238) does, also
 * without a setting car. */
// FUNC 0x00166F98 setting_commit_slider
void setting_commit_slider(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    SettingControl *ctl = (SettingControl *)ctx;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        settingCar->assign(ctl->car);
        ctl->changed = 1;
    }
    g_003526B0 = 1;
}

/* "retrieve_slider": the working copy is reset from the setting car. */
// FUNC 0x00166FF0 setting_retrieve_slider
void setting_retrieve_slider(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    SettingControl *ctl = (SettingControl *)ctx;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL)
        ctl->car.assign(*settingCar);
}

/* Every "set_<category>_<part>": the part id of the button's exec name
 * (setting_id_of_name, T203).  If the working copy owns the part it is
 * fitted, the copy is committed to the setting car and the confirm sound
 * plays; a part it does not own only plays the refused sound.  Nothing
 * without a setting car.  (T096's check_set_parts has already refused a
 * part the event's regulations forbid.) */
// FUNC 0x00167038 setting_set_part
void setting_set_part(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    SettingControl *ctl = (SettingControl *)ctx;
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar != NULL) {
        int part = setting_id_of_name(rec->exec);
        if (ctl->car.hasPart(part)) {
            ctl->car.fitPart(part);
            settingCar->assign(ctl->car);
            ctl->changed = 1;
            menu_play_se(MENU_SE_CONFIRM);
        } else {
            menu_play_se(MENU_SE_REFUSED);
        }
    }
}

/* "go_load": setting_menu (T080) loads a setting from a memory card once
 * the pages are left. */
// FUNC 0x001670F8 setting_go_load
void setting_go_load(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    ((SettingControl *)ctx)->request = SettingControl::REQUEST_LOAD;
}

/* "go_save": ... saves the setting car's setting. */
// FUNC 0x00167108 setting_go_save
void setting_go_save(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    ((SettingControl *)ctx)->request = SettingControl::REQUEST_SAVE;
}

/* The table, .data 0x002ACCC8..0x002ACF30 (77 entries of 8 bytes), zero in
 * the image: T083_static_init fills it at start-up.  A C++ global without an
 * initialiser: zeros at the end of the file's .data, before the frame info
 * (as T048's g_00295380, T049's g_002959E8). */
gt2k_menu::ExecuteFuncData g_002ACCC8[77];  /* 0x002ACCC8 */

/* The table's entry count, .sdata 0x00351978 (a constant word, 77), read by
 * SettingControl::begin (T080).  Defined here, after the actions and before
 * the initialiser: its place in .sdata, right before the initialiser's
 * "go_load" (0x00351980), shows it. */
const int g_00351978 = sizeof(g_002ACCC8) / sizeof(g_002ACCC8[0]);

/* One table entry: the inline constructor of the original's ExecuteFuncData
 * (see T083_static_init). */
static inline void set_action(ExecuteFuncData *e, const char *name,
                              void (*func)(void *ctx, ButtonData *rec, int index,
                                           PageLayout *layout))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006 / u013's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped. */
#define ACTION(name, func)                                                    \
    { ExecuteFuncData *e = base; set_action(e, name, func); }                 \
    base++;                                                                   \
    left--

/* Static initialiser of the action table g_002ACCC8 (.ctors entry
 * 0x0028F0A8), sorted by name; every "set_*" entry is setting_set_part.
 * Compiler-generated in the original: the table was defined here with
 * ExecuteFuncData objects built by an inline constructor, which egcs builds
 * at run time (cp/init.c build_vec_init).  Written out the way
 * build_vec_init expands it (a walking cursor, a copy of it per element, the
 * dead count of the elements left): an exact form, not the original; it
 * stores the same 77 entries.  The comments give the part id
 * setting_id_of_name (T203) maps each name to (what the T084 checks test). */
// FUNC 0x00167118 T083_static_init
void T083_static_init(void)
{
    ExecuteFuncData *base = g_002ACCC8;
    int left = 76;                          /* build_vec_init's iterator: elements - 1 */

    ACTION("commit_slider",       setting_commit_slider);
    ACTION("go_load",             setting_go_load);
    ACTION("go_save",             setting_go_save);
    ACTION("retrieve_slider",     setting_retrieve_slider);
    ACTION("set_asm_off",         setting_set_part);   /* PART_ASCC_NONE */
    ACTION("set_asm_on",          setting_set_part);   /* PART_ASCC */
    ACTION("set_ayc_off",         setting_set_part);   /* PART_LSD */
    ACTION("set_ayc_on",          setting_set_part);   /* PART_LSD_AYC */
    ACTION("set_brc_off",         setting_set_part);   /* PART_BRAKE_CONTROLLER_NONE */
    ACTION("set_brc_on",          setting_set_part);   /* PART_BRAKE_CONTROLLER */
    ACTION("set_brk_normal",      setting_set_part);   /* PART_BRAKE */
    ACTION("set_brk_racing",      setting_set_part);   /* PART_BRAKE_RACING */
    ACTION("set_clt_big",         setting_set_part);   /* PART_CLUTCH_SPORTS */
    ACTION("set_clt_normal",      setting_set_part);   /* PART_CLUTCH */
    ACTION("set_clt_triple",      setting_set_part);   /* PART_CLUTCH_TRIPLE */
    ACTION("set_clt_twin",        setting_set_part);   /* PART_CLUTCH_TWIN */
    ACTION("set_coo_1",           setting_set_part);   /* PART_INTERCOOLER_SPORTS */
    ACTION("set_coo_2",           setting_set_part);   /* PART_INTERCOOLER_RACING */
    ACTION("set_coo_normal",      setting_set_part);   /* PART_INTERCOOLER */
    ACTION("set_cpu_off",         setting_set_part);   /* PART_COMPUTER */
    ACTION("set_cpu_on",          setting_set_part);   /* PART_COMPUTER_ROM */
    ACTION("set_fly_crmo",        setting_set_part);   /* PART_FLYWHEEL_SEMI_RACING */
    ACTION("set_fly_light",       setting_set_part);   /* PART_FLYWHEEL_SPORTS */
    ACTION("set_fly_light_crmo",  setting_set_part);   /* PART_FLYWHEEL_RACING */
    ACTION("set_fly_normal",      setting_set_part);   /* PART_FLYWHEEL */
    ACTION("set_lsd_15way",       setting_set_part);   /* PART_LSD_1_5WAY */
    ACTION("set_lsd_1way",        setting_set_part);   /* PART_LSD_1WAY */
    ACTION("set_lsd_2way",        setting_set_part);   /* PART_LSD_2WAY */
    ACTION("set_lsd_normal",      setting_set_part);   /* PART_LSD */
    ACTION("set_lsd_variable",    setting_set_part);   /* PART_LSD_FULL_CUSTOM */
    ACTION("set_msn_close",       setting_set_part);   /* PART_GEAR_CLOSE */
    ACTION("set_msn_normal",      setting_set_part);   /* PART_GEAR */
    ACTION("set_msn_super_close", setting_set_part);   /* PART_GEAR_SUPER_CLOSE */
    ACTION("set_msn_variable",    setting_set_part);   /* PART_GEAR_FULL_CUSTOM */
    ACTION("set_muf_normal",      setting_set_part);   /* PART_MUFFLER */
    ACTION("set_muf_racing",      setting_set_part);   /* PART_MUFFLER_RACING */
    ACTION("set_muf_semi_racing", setting_set_part);   /* PART_MUFFLER_SEMI_RACING */
    ACTION("set_muf_sports",      setting_set_part);   /* PART_MUFFLER_SPORTS */
    ACTION("set_nat_off",         setting_set_part);   /* PART_NA_TUNE_NONE */
    ACTION("set_nat_stage1",      setting_set_part);   /* PART_NA_TUNE_STAGE1 */
    ACTION("set_nat_stage2",      setting_set_part);   /* PART_NA_TUNE_STAGE2 */
    ACTION("set_nat_stage3",      setting_set_part);   /* PART_NA_TUNE_STAGE3 */
    ACTION("set_shf_normal",      setting_set_part);   /* PART_PROPELLER_SHAFT */
    ACTION("set_shf_propeller",   setting_set_part);   /* PART_PROPELLER_SHAFT_CARBON */
    ACTION("set_sus_normal",      setting_set_part);   /* PART_SUSPENSION */
    ACTION("set_sus_racing",      setting_set_part);   /* PART_SUSPENSION_FULL_CUSTOM */
    ACTION("set_sus_semi_racing", setting_set_part);   /* PART_SUSPENSION_SEMI_RACING */
    ACTION("set_sus_sports",      setting_set_part);   /* PART_SUSPENSION_SPORTS */
    ACTION("set_tcs_off",         setting_set_part);   /* PART_TCSC_NONE */
    ACTION("set_tcs_on",          setting_set_part);   /* PART_TCSC */
    ACTION("set_tir_f_control",   setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_CONTROL */
    ACTION("set_tir_f_normal",    setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_NORMAL */
    ACTION("set_tir_f_racing_h",  setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_H */
    ACTION("set_tir_f_racing_m",  setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_M */
    ACTION("set_tir_f_racing_mh", setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_MH */
    ACTION("set_tir_f_racing_ms", setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_MS */
    ACTION("set_tir_f_racing_s",  setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_S */
    ACTION("set_tir_f_racing_sh", setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_SH */
    ACTION("set_tir_f_racing_ss", setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_RACING_SS */
    ACTION("set_tir_f_sports",    setting_set_part);   /* PART_FRONT_TIRE + TIRE_KIND_SPORTS */
    ACTION("set_tir_r_control",   setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_CONTROL */
    ACTION("set_tir_r_normal",    setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_NORMAL */
    ACTION("set_tir_r_racing_h",  setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_H */
    ACTION("set_tir_r_racing_m",  setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_M */
    ACTION("set_tir_r_racing_mh", setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_MH */
    ACTION("set_tir_r_racing_ms", setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_MS */
    ACTION("set_tir_r_racing_s",  setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_S */
    ACTION("set_tir_r_racing_sh", setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_SH */
    ACTION("set_tir_r_racing_ss", setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_RACING_SS */
    ACTION("set_tir_r_sports",    setting_set_part);   /* PART_REAR_TIRE + TIRE_KIND_SPORTS */
    ACTION("set_tur_off",         setting_set_part);   /* PART_TURBINE */
    ACTION("set_tur_stage1",      setting_set_part);   /* PART_TURBINE_STAGE1 */
    ACTION("set_tur_stage2",      setting_set_part);   /* PART_TURBINE_STAGE2 */
    ACTION("set_tur_stage3",      setting_set_part);   /* PART_TURBINE_STAGE3 */
    ACTION("set_tur_stage4",      setting_set_part);   /* PART_TURBINE_STAGE4 */
    ACTION("set_vcd_off",         setting_set_part);   /* PART_DRIVETRAIN */
    ACTION("set_vcd_on",          setting_set_part);   /* PART_DRIVETRAIN_VCD */
}
