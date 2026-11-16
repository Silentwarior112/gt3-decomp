/* T045 - the ValidFuncData checks of the GT mode pages, and RegurationResult.
 * 0x0011BC18..0x0011C9C8.
 *
 * g_00296750 (filled by T045_static_init, sorted by name) maps the links and
 * exec names of the GT mode buttons to checks; PageManager::callValidFunc
 * (T211) calls one with the GtmodeControl (not used) and the button's
 * record.  A check returns NULL to go on, or the page to open as a popup
 * instead (a message); most play MENU_SE_REFUSED first.  The string a check
 * reads is the record's name (a licence letter, a part, a wheel, a course).
 * The buying checks compare with the credits, a 64-bit GameStatus::money.
 *
 * Includes: retail's type names (.rodata 0x00310320..0x00310568, reverse
 * definition order) are List<Button>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SmartIndexT<int>, RegurationResult, the garage lists (GarageWheelList ..
 * Cell<GarageCar>) and Operator (T044's without its two classes): so the
 * GameStatus first, RegurationResult before SmartIndexT<int> (which this
 * file's header brings through T211), the menu item classes (T206: T216's
 * operator classes, Button's base Cell<Button>), ButtonList's List<Button>;
 * the other headers add no class.  (The "(%f, %f, %f) %f %f %f\n" at
 * 0x00310570, after the type names, is the next file's: an inline
 * function's literal, written when its header is read.) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (money, menu, licences,
                                               garage), MenuStatus, GarageCar */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult (check_go_race) */
#include "game/common/T251_smart_index.h"          /* (SmartIndexT<int>: its type name, see above) */
#include "game/menu/framework/T206_button.h"               /* (Cell<Button>, T216's operator classes) */
#include "game/menu/framework/T244_button_list.h"          /* ButtonData (List<Button>) */
#include "game/menu/gtmode/T045_gtmode_page_checks.h"   /* the checks, LicenseEntry, ValidFuncData g_00296750 */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getShopFlags, PART_FRONT_TIRE_DIRT */
#include "game/data/T483_car_information.h"      /* CarInformation::hasPart / getPrice / getPartPrice /
                                               partExists */
#include "game/data/T484_race_database.h"        /* RaceDataBase, COURSE_KIND_DIRT */
#include "game/data/T489_car_database.h"         /* check_regulation */
#include "game/common/T494_text_utils.h"           /* str_cut_last_underscore */
#include "game/data/T435_product_config.h"       /* product_config, servicePrice */
#include "game/data/T203_part_names.h"           /* part_id_of_name */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "runtime/newlib/G007_stdio.h"                /* strcmp */
#include "game/menu/gtmode/gtmode_control_internal.h"

using namespace gt2k_menu;

/* "00cmn/cd_buy_sure": buying the dealer's car (MenuStatus::car): sold
 * there, the credits, a free garage slot. */
// FUNC 0x0011BC18 check_cd_buy_sure
const char *check_cd_buy_sure(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    s64 money = getGameStatus()->money;
    if ((car->getShopFlags() >> CAR_SHOP_BIT_NOT_AT_DEALER) & 1) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/not_for_buy";
    }
    if (money < car->getPrice()) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    if (getGameStatus()->getCarNum() >= GARAGE_MAX_CARS) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/max_car";
    }
    return NULL;
}

/* "hm/hm_buy_sure": the same at the used car shop (flag bit 1). */
// FUNC 0x0011BCE0 check_hm_buy_sure
const char *check_hm_buy_sure(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    s64 money = getGameStatus()->money;
    if ((car->getShopFlags() >> CAR_SHOP_BIT_NOT_USED) & 1) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/not_for_buy";
    }
    if (money < car->getPrice()) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    if (getGameStatus()->getCarNum() >= GARAGE_MAX_CARS) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/max_car";
    }
    return NULL;
}

/* "hm/hm_sell_sure": MenuStatus::car may be sold at the used car shop. */
// FUNC 0x0011BDA8 check_hm_sell_sure
const char *check_hm_sell_sure(void *ctx, ButtonData *rec)
{
    if ((getGameStatus()->getMenuStatus()->car->getShopFlags() >> CAR_SHOP_BIT_NOT_USED) & 1) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/not_for_sell";
    }
    return NULL;
}

/* "at/at", "go_machine_test", "hm/hm_fr", "mt/mt", "ts/ts", "ts/ts_bs",
 * "ts/ts_dr", "ts/ts_eg", "ts/ts_ot", "ts/ts_tr": a car is ridden. */
// FUNC 0x0011BE00 check_ride_car
const char *check_ride_car(void *ctx, ButtonData *rec)
{
    if (getGameStatus()->getRunningCar() == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    }
    return NULL;
}

/* "at/ws/ws" (the wheel shop): the ridden car takes other wheels. */
// FUNC 0x0011BE40 check_ws
const char *check_ws(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    }
    if ((car->getShopFlags() >> CAR_SHOP_BIT_FIXED_WHEELS) & 1) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/not_for_change_wheel";
    }
    return NULL;
}

/* "hm/hm_wheel": the same for MenuStatus::car. */
// FUNC 0x0011BEA8 check_hm_wheel
const char *check_hm_wheel(void *ctx, ButtonData *rec)
{
    if ((getGameStatus()->getMenuStatus()->car->getShopFlags() >> CAR_SHOP_BIT_FIXED_WHEELS) & 1) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/not_for_change_wheel";
    }
    return NULL;
}

/* "ts/ts_buy_sure1" .. "5": buying the part rec->name for the ridden car:
 * not owned yet, made for the car, the credits. */
// FUNC 0x0011BF00 check_ts_buy_sure
const char *check_ts_buy_sure(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    }
    int part = part_id_of_name(rec->name);
    if (car->hasPart(part) != 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "ts/already_you_have";
    }
    if (car->partExists(part) == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "ts/not_exist";
    }
    int price = car->getPartPrice(part);
    if (getGameStatus()->money < price) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    return NULL;
}

/* "ts/ts_buy_sure_all_tire": the set of racing tyres (front kinds 2 .. 8,
 * part ids 0x3A .. 0x40): the price of those the ridden car can have and does
 * not own; nothing to buy, or not the credits, refuses. */
// FUNC 0x0011BFE0 check_ts_buy_sure_all_tire
const char *check_ts_buy_sure_all_tire(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    }
    int total = 0;
    for (int part = PART_RACING_TIRE_FIRST; part < PART_RACING_TIRE_END; part++) {
        if (car->partExists(part) != 0 && car->hasPart(part) == 0)
            total += car->getPartPrice(part);
    }
    if (total == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "ts/already_you_have_all_tires";
    }
    if (getGameStatus()->money < total) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    return NULL;
}

/* "00cmn/ws_buy_sure": buying the wheel rec->name (cut at its last '_'):
 * not owned yet, the credits for the ProductConfig price "wheel". */
// FUNC 0x0011C0C8 check_ws_buy_sure
const char *check_ws_buy_sure(void *ctx, ButtonData *rec)
{
    char wheel[0x40];

    str_cut_last_underscore(wheel, rec->name);
    if (getGameStatus()->hasWheel(wheel) != 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/already_you_have";
    }
    int price = product_config()->servicePrice("wheel");
    if (getGameStatus()->money < price) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    return NULL;
}

/* The licences in the order they are taken, with the message of each when
 * it is missing (T045 .data 0x002966B0, terminated by {NULL, -1}).  Defined
 * here, between check_ws_buy_sure and check_license: its names lie in .sdata
 * between "wheel" and check_license's "d", its messages in .rodata between
 * "00cmn/already_you_have" and "lc/no_last_exam". */
LicenseEntry g_002966B0[6] = {              /* 0x002966B0 */
    { "b",  0, NULL },
    { "a",  1, "lc/no_license_a" },
    { "ib", 2, "lc/no_license_ib" },
    { "ia", 3, "lc/no_license_ia" },
    { "s",  4, "lc/no_license_s" },
    { NULL, -1, NULL }
};

/* "lc/lc_a", "lc/lc_b", "lc/lc_d", "lc/lc_ia", "lc/lc_ib", "lc/lc_s":
 * rec->name is a licence letter.  Licence d is always open.  Else every
 * licence before it in g_002966B0 (b, a, ib, ia, s) must be complete (all
 * 8 tests); if one is not, the message of rec's entry ("lc/no_license_a"
 * ...; NULL for b or an unknown letter: no sound then) is returned. */
// FUNC 0x0011C160 check_license
const char *check_license(void *ctx, ButtonData *rec)
{
    if (strcmp(rec->name, "d") == 0)
        return NULL;
    const char *message = NULL;
    int i;
    for (i = 0; g_002966B0[i].name != NULL; i++)
        if (strcmp(rec->name, g_002966B0[i].name) == 0)
            message = g_002966B0[i].message;
    for (i = 0; g_002966B0[i].name != NULL && strcmp(rec->name, g_002966B0[i].name) != 0; i++) {
        if (getGameStatus()->licences.isPassed(g_002966B0[i].license, 0) == 0) {
            if (message != NULL)
                menu_play_se(MENU_SE_REFUSED);
            return message;
        }
    }
    return NULL;
}

/* The last test of each licence (T045 .data 0x002966F8, terminated by
 * {NULL, -1}).  Defined here, after check_license: its names follow "d" in
 * .sdata. */
LicenseEntry g_002966F8[7] = {              /* 0x002966F8 */
    { "L0B0008", 0, NULL },
    { "L0A0008", 1, NULL },
    { "LIB0008", 2, NULL },
    { "LIA0008", 3, NULL },
    { "L0S0008", 4, NULL },
    { "L0R0008", 5, NULL },
    { NULL, -1, NULL }
};

/* "go_license": rec->name is a licence test; the last test of a licence
 * (g_002966F8: L0B0008, L0A0008, LIB0008, LIA0008, L0S0008, L0R0008) needs
 * the licence's first 7 tests passed. */
// FUNC 0x0011C258 check_go_license
const char *check_go_license(void *ctx, ButtonData *rec)
{
    for (int i = 0; g_002966F8[i].name != NULL; i++) {
        if (strcmp(g_002966F8[i].name, rec->name) == 0) {
            if (getGameStatus()->licences.isPassed(g_002966F8[i].license, 1) != 0)
                return NULL;
            menu_play_se(MENU_SE_REFUSED);
            return "lc/no_last_exam";
        }
    }
    return NULL;
}

/* "go_race": the ridden car meets the regulations of the current event
 * (check_regulation by value into a local of the else block: its inline
 * destructor, the vptr store, runs on every return, and the block's end
 * puts the rethrow ahead of the `return NULL' as retail has it). */
// FUNC 0x0011C308 check_go_race
const char *check_go_race(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    } else {
        RegurationResult result = check_regulation(nameToCode(getGameStatus()->getMenuStatus()->event),
                                                   car);
        if (result.isOk() == 0) {
            menu_play_se(MENU_SE_REFUSED);
            if (result.flags[1] != 0)       /* the licence */
                return "00cmn/license_violation";
            return "00cmn/regulation_violation";
        }
    }
    return NULL;
}

/* "go_free_run": rec->name is a course; a dirt course (its kind in the race
 * "ATAF0001") needs dirt tyres (PART_FRONT_TIRE_DIRT) on the ridden car.
 * (~RaceDataBase on every return and on an exception.) */
// FUNC 0x0011C3F8 check_go_free_run
const char *check_go_free_run(void *ctx, ButtonData *rec)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/ride_car";
    }
    RaceDataBase race;
    race.select(nameToCode("ATAF0001"));
    if (race.getCourseKind(nameToCode(rec->name)) == COURSE_KIND_DIRT &&
        car->hasPart(PART_FRONT_TIRE_DIRT) == 0) {
        menu_play_se(MENU_SE_REFUSED);
        return "hm/need_dirt_tire";
    }
    return NULL;
}

/* "oil_change": the credits for the ProductConfig price "oil_change". */
// FUNC 0x0011C4F8 check_oil_change
const char *check_oil_change(void *ctx, ButtonData *rec)
{
    int price = product_config()->servicePrice("oil_change");
    if (getGameStatus()->money < price) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    return NULL;
}

/* "wash_car": the credits for the price "car_wash". */
// FUNC 0x0011C558 check_wash_car
const char *check_wash_car(void *ctx, ButtonData *rec)
{
    int price = product_config()->servicePrice("car_wash");
    if (getGameStatus()->money < price) {
        menu_play_se(MENU_SE_REFUSED);
        return "00cmn/no_money";
    }
    return NULL;
}

/* "check_ride_car": with a car ridden, asks first ("00cmn/cd_ride_sure");
 * else rides the selected garage car (MenuStatus::garageIndex) at once. */
// FUNC 0x0011C5B8 check_ride_sure
const char *check_ride_sure(void *ctx, ButtonData *rec)
{
    if (getGameStatus()->getRunningCar() != NULL) {
        menu_play_se(MENU_SE_CONFIRM);
        return "00cmn/cd_ride_sure";
    }
    int garageIndex = getGameStatus()->getMenuStatus()->garageIndex;
    getGameStatus()->setRunningCarIndex(garageIndex);
    return NULL;
}

/* The table's entry count, .sdata 0x00351100 (34), read by
 * GtmodeControl::begin (T043).  Defined here, after the checks, as T048's
 * g_00350FF4: its place in .sdata, after the checks' short strings and
 * before the table's short names ("at/at" 0x00351108 ..), shows it.  Not
 * const, as T037's g_00351294 (see T044's g_00351088). */
int g_00351100 = sizeof(g_00296750) / sizeof(g_00296750[0]);

/* The check table (.data 0x00296750, 34 entries): zero in the original's
 * data, filled by T045_static_init at boot.  No initialiser, so egcs writes
 * the zeros in .data after the two licence tables, in front of the frame
 * info, as retail has them. */
ValidFuncData g_00296750[34];               /* 0x00296750 */

/* One table entry: the inline constructor of the original's ValidFuncData
 * (see T045_static_init). */
static inline void set_valid(ValidFuncData *e, const char *name,
                             const char *(*func)(void *ctx, ButtonData *rec))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006 / u008's form): the
 * constructor on a copy of the cursor, then the cursor and the count of the
 * elements left stepped. */
#define VALID(name, func)                                                     \
    { ValidFuncData *e = base; set_valid(e, name, func); }                    \
    base++;                                                                   \
    left--

/* The static initialiser (.ctors 0x0028F058): the check table, sorted by
 * name.  Compiler-generated in the original, as T048's: the table was
 * defined here with ValidFuncData objects built by an inline constructor,
 * which egcs builds at run time (cp/init.c build_vec_init).  Written out the
 * way build_vec_init expands it (a walking cursor, a copy of it per element,
 * the dead count of the elements left): an exact form, not the original; it
 * stores the same 34 entries. */
// FUNC 0x0011C628 T045_static_init
void T045_static_init(void)
{
    ValidFuncData *base = g_00296750;
    int left = 33;                          /* build_vec_init's iterator: elements - 1 */

    VALID("00cmn/cd_buy_sure",         check_cd_buy_sure);
    VALID("00cmn/ws_buy_sure",         check_ws_buy_sure);
    VALID("at/at",                     check_ride_car);
    VALID("at/ws/ws",                  check_ws);
    VALID("check_ride_car",            check_ride_sure);
    VALID("go_free_run",               check_go_free_run);
    VALID("go_license",                check_go_license);
    VALID("go_machine_test",           check_ride_car);
    VALID("go_race",                   check_go_race);
    VALID("hm/hm_buy_sure",            check_hm_buy_sure);
    VALID("hm/hm_fr",                  check_ride_car);
    VALID("hm/hm_sell_sure",           check_hm_sell_sure);
    VALID("hm/hm_wheel",               check_hm_wheel);
    VALID("lc/lc_a",                   check_license);
    VALID("lc/lc_b",                   check_license);
    VALID("lc/lc_d",                   check_license);
    VALID("lc/lc_ia",                  check_license);
    VALID("lc/lc_ib",                  check_license);
    VALID("lc/lc_s",                   check_license);
    VALID("mt/mt",                     check_ride_car);
    VALID("oil_change",                check_oil_change);
    VALID("ts/ts",                     check_ride_car);
    VALID("ts/ts_bs",                  check_ride_car);
    VALID("ts/ts_buy_sure1",           check_ts_buy_sure);
    VALID("ts/ts_buy_sure2",           check_ts_buy_sure);
    VALID("ts/ts_buy_sure3",           check_ts_buy_sure);
    VALID("ts/ts_buy_sure4",           check_ts_buy_sure);
    VALID("ts/ts_buy_sure5",           check_ts_buy_sure);
    VALID("ts/ts_buy_sure_all_tire",   check_ts_buy_sure_all_tire);
    VALID("ts/ts_dr",                  check_ride_car);
    VALID("ts/ts_eg",                  check_ride_car);
    VALID("ts/ts_ot",                  check_ride_car);
    VALID("ts/ts_tr",                  check_ride_car);
    VALID("wash_car",                  check_wash_car);
}

// FUNC 0x0011C958 RegurationResult::~RegurationResult
/* inline in game/menu/gtmode/T045_gtmode_page_checks.h; egcs emits this copy (the one
 * in all four vtables): the vptr (+0x1C) = 0x00310310, then ::operator delete
 * (0x0027C0C0) if flags & 1. */
/* implicit: operator delete (deleting destructor, the inline ~RegurationResult in the header) */

#ifndef BOOT_OLD_GCC
/* Builds the __user_type_info __ti16RegurationResult (no bases) on the
 * first call. */
// FUNC 0x0011C988 __tf16RegurationResult
extern "C" void *__tf16RegurationResult(void)
{
    if (__ti16RegurationResult[0] == 0)
        __rtti_user(__ti16RegurationResult, "16RegurationResult");
    return __ti16RegurationResult;
}
#endif
