/* T096 - the checks of the car setting pages.  0x001279C8..0x00128338.
 *
 * T096_static_init fills g_0029AEA0 with 75 {exec name, check} entries (76
 * with the enhanced engine's check_cvt) sorted by name; gt2k_menu::SettingControl::begin (T080) gives it to its
 * PageManager (validFuncs, ctx the SettingControl; not read by the checks).
 * For a decided button the PageManager calls the check of its exec name
 * with the button's record (ButtonData, T244: the setting's name at +0x0C0):
 * NULL accepts it, a name opens that message page as a popup instead.  The
 * car of the setting pages is MenuStatus::raceCar[0] (T427).
 *
 * The rest of the unit is T096's out-of-line copies of GarageCar's inline
 * functions (T036_garage.h), emitted with T096's copies of the GarageCar
 * vtables (0x00312FE8 / 0x00312FF8 / 0x00313018); those vtables use T036's
 * functions, so these have no references.
 *
 * Data: .data the table g_0029AEA0 (zeros) and the frame info; .sdata ""
 * and g_003512DC; .rodata 0x00312A30..0x00313276: the functions' and the
 * initialiser's strings, the GarageCar / Cell<GarageCar> vtables, then the
 * type names (reverse definition order): Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SettingControl, SmartIndexT<int>, RegurationResult, the garage's eight,
 * Operator.  So the GameStatus, RegurationResult, SmartIndexT<int> (T251),
 * SettingControl (T080: its header reads the operator classes only after
 * the class), then Button (T206); no ButtonList (no List<Button>), no
 * <new>. */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus, GarageCar */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/menu/car_settings/T080_setting_control.h"      /* (seen: SettingControl; T036_garage.h: the operator
                                               classes, GarageCar's type_info) */
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>) */
#include "game/menu/car_settings/T096_setting_checks.h"
#include "game/menu/framework/T211_page_manager.h"         /* ValidFuncData */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData (exec) */
#include "game/save/T427_menu_status.h"          /* MenuStatus::raceCar, event */
#include "game/data/T482_car_equipments.h"       /* GarageCar::fitPart (CarEquipments) */
#include "game/data/T483_car_information.h"      /* CarInformation, __tf14CarInformation */
#include "game/menu/framework/T495_menu_list.h"            /* pCell, __tfQ29gt2k_menu5pCell */
#include "game/data/T203_part_names.h"           /* setting_id_of_name */
#include "game/menu/car_settings/T084_setting_checks.h"       /* car_meets_event_regulations */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_REFUSED */
#include "game/data/T489_car_database.h"         /* race_dirt_tire_only */
#include "runtime/newlib/G007_stdio.h"                /* strcmp */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class (GCC 14's __tf) */
#include "game/menu/gtmode/gtmode_main_internal.h"
#include "game/enhanced.h"                  /* ENH_MENU_ARCADE_DIRT_TYRES */
#include "game/menu/car_settings/T080_setting_entry.h"        /* the enhanced engine: setting_menu_course */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseKind */
#include "libc.h"                           /* printf (the enhanced engine's log line) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* {&__tiQ29gt2k_menut4Cell1Z9GarageCar, public, offset 0}, {&__ti14CarInformation,
 * public, offset 0x10} (words 0x004B1350, 0x40000000, 0x004B2A50, 0x40000010):
 * T096's copy (T036's, g_00311EA0, is declared in garage_internal.h). */
extern const __class_type_info::base_info g_00313230[2];   /* 0x00313230 (T096) */
#endif

using namespace gt2k_menu;

/* "set_*": refuses a setting change that would break the regulations of
 * the current race.  A copy of the setting car (a GarageCar on the stack,
 * GarageCar::assign) gets the setting of the record's exec name (T203
 * setting_id_of_name, a part id) fitted, and the copy is checked
 * (car_meets_event_regulations, T084); the refused sound and
 * "not_to_change" if it fails.
 * NULL without a setting car.  The copy's inline destructor runs on every
 * path (and in the landing pad). */
// FUNC 0x001279C8 check_set_parts
const char *check_set_parts(void *ctx, ButtonData *rec)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL)
        return NULL;
    int part = setting_id_of_name(rec->exec);
    /* implicit: ~CarInformation, ~pCell (the inline ~GarageCar of car) */
    GarageCar car;                          /* sp+0x000 */
    car.assign(*settingCar);
    car.fitPart(part);
    if (car_meets_event_regulations(&car) == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "not_to_change";
    }
    return NULL;
}

#if ENH_MENU_ARCADE_DIRT_TYRES
/* The enhanced engine (new, Lupo 2 L2 0x00152A80 isDirtTireOnly;
 * menus_b.md 5): whether the race of event `event' allows dirt tyres only.
 * GT3 asks the race record (race_dirt_tire_only).  Lupo 2 also refuses
 * other tyres in an arcade event ("A...") whose course is a dirt course
 * (kind 2): the arcade races of GT3's free run ("ATAF0001") and of a garage
 * car on a dirt course do not say so in their record.  Lupo 2 reads the
 * course from its menu state (MenuStatus +0x84); GT3's arcade does not keep
 * it there, so the course is the one setting_menu runs for (T080).  The
 * race record still counts for an arcade event (Lupo 2 tests only the
 * course): no GT3 refusal is lost. */
static int setting_dirt_tire_only(const char *event)
{
    if (event[0] == 'A') {
        u64 course = setting_menu_course();
        if (course != 0) {
            RaceDataBase db;
            if (db.getCourseKind(course) == 2)
                return 1;
        }
    }
    return race_dirt_tire_only(event);
}
#endif

/* "chg/change_tir_front", "chg/change_tir_rear": the tyres cannot be
 * changed in a race whose rule is dirt tyres only (race_dirt_tire_only,
 * T489): the refused sound and "dirt_tire_only".  NULL without a setting
 * car or without a race (MenuStatus::event ""; "" is gp-0x6718,
 * 0x003512D8).  The enhanced engine: also in an arcade race on a dirt
 * course (setting_dirt_tire_only; Lupo 2 L2 0x0015DB18). */
// FUNC 0x00127B30 check_change_tire
const char *check_change_tire(void *ctx, ButtonData *rec)
{
    if (getGameStatus()->getMenuStatus()->raceCar[0] == NULL)
        return NULL;
    const char *event = getGameStatus()->getMenuStatus()->event;
    if (strcmp(event, "") == 0)
        return NULL;
#if ENH_MENU_ARCADE_DIRT_TYRES
    if (setting_dirt_tire_only(event) != 0) {
#else
    if (race_dirt_tire_only(event) != 0) {
#endif
        menu_play_se(MENU_SE_REFUSED);
        return "dirt_tire_only";
    }
    return NULL;
}

/* The enhanced engine (new, Lupo 2 L2 0x0015DB88 check_cvt; menus_b.md 1):
 * the transmission page cannot be opened for a CVT (CarEquipments::isCVT of
 * the setting car: 2 forward gears), "your_car_is_CVT".  Lupo 2 registers it
 * under its Concept layout's "set/adjast_gear"; here it is GT3's button,
 * the link "set/adjast_drv_gear_ratio" of menu/setting.mbl.  GT3's layout
 * has no "your_car_is_CVT" page: without it the check opens GT3's own
 * refusal "not_to_change" with the refused sound (a missing page would
 * leave the PageManager without a page).  NULL for every other car: no GT3
 * gearbox has 2 gears. */
const char *check_cvt(void *ctx, ButtonData *rec)
{
    GarageCar *settingCar = getGameStatus()->getMenuStatus()->raceCar[0];
    if (settingCar == NULL || settingCar->isCVT() == 0)
        return NULL;
    if (static_cast<SettingControl *>(ctx)->pages.findPage("your_car_is_CVT") != NULL) {
        printf("menus: CVT: the transmission page refused (your_car_is_CVT)\n");
        return "your_car_is_CVT";
    }
    printf("menus: CVT: the transmission page refused (no your_car_is_CVT page: not_to_change)\n");
    menu_play_se(MENU_SE_REFUSED);
    return "not_to_change";
}

/* The table's entry count, .sdata 0x003512DC (a constant word, 75), read by
 * SettingControl::begin (T080).  Defined here, after the checks and before
 * the initialiser: its place in .sdata, after check_change_tire's ""
 * (0x003512D8), shows it. */
int g_003512DC = sizeof(g_0029AEA0) / sizeof(g_0029AEA0[0]);    /* 0x003512DC */

/* The check table (.data 0x0029AEA0..0x0029B0F8, zero in the original:
 * T096_static_init fills it).  No initialiser: egcs writes it as zeros at
 * the end of the file's .data, before the frame info. */
gt2k_menu::ValidFuncData g_0029AEA0[76];    /* 0x0029AEA0 (75 in GT3; the enhanced engine adds
                                               check_cvt) */

/* One table entry: the inline constructor of the original's ValidFuncData
 * (see T096_static_init). */
static inline void set_check(ValidFuncData *e, const char *name,
                             const char *(*func)(void *ctx, ButtonData *rec))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006 / u008 / u009's
 * form): the constructor on a copy of the cursor, then the cursor and the
 * count of the elements left stepped. */
#define CHECK(name, func)                                                     \
    { ValidFuncData *e = base; set_check(e, name, func); }                    \
    base++;                                                                   \
    left--

/* Static initialiser (.ctors entry 0x0028F0CC): the check table, sorted by
 * name.  Compiler-generated in the original: the table was defined here
 * with ValidFuncData objects built by an inline constructor, which egcs
 * builds at run time (cp/init.c build_vec_init).  Written out the way
 * build_vec_init expands it (a walking cursor, a copy of it per element,
 * the dead count of the elements left): an exact form, not the original; it
 * stores the same 75 entries. */
// FUNC 0x00127BB0 T096_static_init
void T096_static_init(void)
{
    ValidFuncData *base = g_0029AEA0;
    int left = 75;                          /* build_vec_init's iterator: elements - 1 */

    CHECK("chg/change_tir_front", check_change_tire);
    CHECK("chg/change_tir_rear",  check_change_tire);
    CHECK("set/adjast_drv_gear_ratio", check_cvt);    /* the enhanced engine (Lupo 2: check_cvt) */
    CHECK("set_ayc_off",          check_set_parts);
    CHECK("set_ayc_on",           check_set_parts);
    CHECK("set_brc_off",          check_set_parts);
    CHECK("set_brc_on",           check_set_parts);
    CHECK("set_brk_normal",       check_set_parts);
    CHECK("set_brk_racing",       check_set_parts);
    CHECK("set_clt_big",          check_set_parts);
    CHECK("set_clt_normal",       check_set_parts);
    CHECK("set_clt_triple",       check_set_parts);
    CHECK("set_clt_twin",         check_set_parts);
    CHECK("set_coo_1",            check_set_parts);
    CHECK("set_coo_2",            check_set_parts);
    CHECK("set_coo_normal",       check_set_parts);
    CHECK("set_cpu_off",          check_set_parts);
    CHECK("set_cpu_on",           check_set_parts);
    CHECK("set_fly_crmo",         check_set_parts);
    CHECK("set_fly_light",        check_set_parts);
    CHECK("set_fly_light_crmo",   check_set_parts);
    CHECK("set_fly_normal",       check_set_parts);
    CHECK("set_lsd_15way",        check_set_parts);
    CHECK("set_lsd_1way",         check_set_parts);
    CHECK("set_lsd_2way",         check_set_parts);
    CHECK("set_lsd_normal",       check_set_parts);
    CHECK("set_lsd_variable",     check_set_parts);
    CHECK("set_msn_close",        check_set_parts);
    CHECK("set_msn_normal",       check_set_parts);
    CHECK("set_msn_super_close",  check_set_parts);
    CHECK("set_msn_variable",     check_set_parts);
    CHECK("set_muf_normal",       check_set_parts);
    CHECK("set_muf_racing",       check_set_parts);
    CHECK("set_muf_semi_racing",  check_set_parts);
    CHECK("set_muf_sports",       check_set_parts);
    CHECK("set_nat_off",          check_set_parts);
    CHECK("set_nat_stage1",       check_set_parts);
    CHECK("set_nat_stage2",       check_set_parts);
    CHECK("set_nat_stage3",       check_set_parts);
    CHECK("set_shf_normal",       check_set_parts);
    CHECK("set_shf_propeller",    check_set_parts);
    CHECK("set_sms_off",          check_set_parts);
    CHECK("set_sms_on",           check_set_parts);
    CHECK("set_sus_normal",       check_set_parts);
    CHECK("set_sus_racing",       check_set_parts);
    CHECK("set_sus_semi_racing",  check_set_parts);
    CHECK("set_sus_sports",       check_set_parts);
    CHECK("set_tcs_off",          check_set_parts);
    CHECK("set_tcs_on",           check_set_parts);
    CHECK("set_tir_f_control",    check_set_parts);
    CHECK("set_tir_f_normal",     check_set_parts);
    CHECK("set_tir_f_racing_h",   check_set_parts);
    CHECK("set_tir_f_racing_m",   check_set_parts);
    CHECK("set_tir_f_racing_mh",  check_set_parts);
    CHECK("set_tir_f_racing_ms",  check_set_parts);
    CHECK("set_tir_f_racing_s",   check_set_parts);
    CHECK("set_tir_f_racing_sh",  check_set_parts);
    CHECK("set_tir_f_racing_ss",  check_set_parts);
    CHECK("set_tir_f_sports",     check_set_parts);
    CHECK("set_tir_r_control",    check_set_parts);
    CHECK("set_tir_r_normal",     check_set_parts);
    CHECK("set_tir_r_racing_h",   check_set_parts);
    CHECK("set_tir_r_racing_m",   check_set_parts);
    CHECK("set_tir_r_racing_mh",  check_set_parts);
    CHECK("set_tir_r_racing_ms",  check_set_parts);
    CHECK("set_tir_r_racing_s",   check_set_parts);
    CHECK("set_tir_r_racing_sh",  check_set_parts);
    CHECK("set_tir_r_racing_ss",  check_set_parts);
    CHECK("set_tir_r_sports",     check_set_parts);
    CHECK("set_tur_off",          check_set_parts);
    CHECK("set_tur_stage1",       check_set_parts);
    CHECK("set_tur_stage2",       check_set_parts);
    CHECK("set_tur_stage3",       check_set_parts);
    CHECK("set_tur_stage4",       check_set_parts);
    CHECK("set_vcd_off",          check_set_parts);
    CHECK("set_vcd_on",           check_set_parts);
}

#undef CHECK

// FUNC 0x001281F8 gt2k_menu::Cell<GarageCar>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (T096's, unreferenced;
 * T036's is 0x00125A10): the vptr (+0x8) = 0x00311C50, then
 * gt2k_menu::pCell::~pCell(this, flags), which does the delete. */
/* implicit: ~pCell (base destructor, the inline ~Cell in the header) */

#ifndef BOOT_OLD_GCC
/* T096's copy of __tf9GarageCar (unreferenced; T036's is 0x00125A38): the
 * same, with T096's copy of the base list (g_00313230).  Weak: T036's
 * definition wins at link time. */
// FUNC 0x00128220 __tf9GarageCar
extern "C" __attribute__((weak)) void *__tf9GarageCar(void)
{
    if (__ti9GarageCar[0] == 0) {                       /* _name not set yet */
        __tf14CarInformation();                         /* the bases' nodes first */
        __tfQ29gt2k_menut4Cell1Z9GarageCar();
        __rtti_class(__ti9GarageCar, "9GarageCar", g_00313230, 2);
    }
    return __ti9GarageCar;
}
#endif

// FUNC 0x00128280 GarageCar::~GarageCar
/* inline in game/menu/gtmode/T036_garage.h; egcs emits this copy (T096's, unreferenced;
 * T036's is 0x00125A98): the vptrs +0x1EC = 0x00311C20 and +0x8 =
 * 0x00311C30, CarInformation::~CarInformation(this + 0x10, 0), then
 * Cell<GarageCar>'s inline destructor: the vptr 0x00311C50,
 * gt2k_menu::pCell::~pCell(this, flags), which does the delete. */
/* implicit: ~CarInformation, ~pCell (base destructors, the inline ~GarageCar in the header) */

#ifndef BOOT_OLD_GCC
/* T096's copy of __tfQ29gt2k_menut4Cell1Z9GarageCar (unreferenced; T036's
 * is 0x00125B00; the base pCell's node is __tiQ29gt2k_menu5pCell gp-0x4538).
 * Weak: T036's definition wins at link time. */
// FUNC 0x001282E8 __tfQ29gt2k_menut4Cell1Z9GarageCar
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void)
{
    if (__tiQ29gt2k_menut4Cell1Z9GarageCar[0] == 0) {   /* _name not set yet */
        __tfQ29gt2k_menu5pCell();                       /* the base's node first */
        __rtti_si(__tiQ29gt2k_menut4Cell1Z9GarageCar, "Q29gt2k_menut4Cell1Z9GarageCar",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z9GarageCar;
}
#endif
