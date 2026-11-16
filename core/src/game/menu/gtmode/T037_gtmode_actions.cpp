/* T037 - the GT-mode page actions, and gt2k_menu::TumbleCar, the car wash.
 * 0x00125B50..0x00127368.
 *
 * T037_static_init fills g_0029A180 with 54 {exec name, action} entries
 * sorted by name; GtmodeControl::begin (T043) gives it to its PageManager
 * (setExecuteFuncData, ctx the GtmodeControl), whose callExecuteFunc (T211)
 * calls func(ctx, rec, index, layout) for a decided button with that exec
 * name.  rec is the button's record (ButtonData, T244): its name (+0x000)
 * is the race / test / car / wheel / part / course code most actions read;
 * index and layout are not used.  The go_* actions set MenuStatus::nextMode,
 * the mode main() (T000) runs once the menu is left; the others buy, sell,
 * ride, tune and wash cars and set the garage's sort order.
 *
 * "The shop car" is MenuStatus::car, "the ridden car" GameStatus::
 * getRunningCar(), "the current list" ctl->getButtonList() (T043, inline).
 * The GameStatus is fetched again (getGameStatus) for nearly every access,
 * as the original does.
 *
 * Includes: the original's type names (.rodata 0x003122F0..0x00312586,
 * reverse definition order) are TumbleCar, UnitArena<64>, RegurationResult,
 * RecordsMenuArg, List<Button>, Cell<Button>, OperatorList,
 * List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * SmartIndexT<int>, the garage's eight (GarageWheelList .. Cell<GarageCar>),
 * Operator (a non-polymorphic base: always last): so the GameStatus first,
 * then T251, the operator classes (T216), Button (T206), ButtonList (T244),
 * RecordsMenuArg (T089), RegurationResult, T153's UnitArena<64>, the other
 * headers (no names), and TumbleCar last.  The section opens with two
 * unreferenced strings (DATA_START; u010's pass found them): T412's
 * SimpleCameraBase::print literal "(%f, %f, %f) %f %f %f\n" (0x00311EE8) and
 * <new>'s "bad_alloc" (0x00311F00), in that order: T412 was read first. */
// DATA_START 0x00311EE8
#include "game/camera/T412_simple_camera_base.h"   /* (seen: SimpleCameraBase::print's literal, first) */
#include <new>                              /* (seen: "bad_alloc") */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus, GarageCar */
#include "game/common/T251_smart_index.h"          /* SmartIndexT<int>::set (GtmodeControl::infoMode) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen: the operator classes) */
#include "game/menu/framework/T206_button.h"               /* (seen: Cell<Button>) */
#include "game/menu/framework/T244_button_list.h"          /* ButtonList::getCursor, ButtonData */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/menu/gtmode/T043_gtmode_control.h"       /* GtmodeControl (pm, sortType, infoMode, oilCylinder, mode) */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/gtmode/T040_oil_change_cylinder.h"  /* OilChangeCylinder::start */
#include "game/menu/framework/T202_menu_sound_timer.h"     /* menu_sound_after */
#include "game/data/T203_part_names.h"           /* part_id_of_name */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car */
#include "game/menu/framework/T220_object.h"               /* Object::attachTo */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* menu_car_spec, MenuCarSpec::setCar */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se, MENU_SE_* */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition::setDirt / changeOil */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig::servicePrice */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments, PART_* */
#include "game/data/T483_car_information.h"      /* CarInformation */
#include "game/save/T491_garage_wheel.h"         /* GarageWheel */
#include "game/common/T494_text_utils.h"           /* str_cut_last_underscore */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14's __tf) */
#include "game/menu/gtmode/T037_gtmode_actions.h"       /* the actions, g_0029A180, g_00351294 */
#include "game/menu/gtmode/T037_tumble_car.h"           /* TumbleCar: the last class defined */
#include "game/menu/gtmode/gtmode_main_internal.h"

using namespace gt2k_menu;

/* The racing tyres "buy_all_tires" buys: front tyre kinds 2 .. 8, racing
 * super hard .. super soft (T482's tyre kinds). */
enum {
    PART_RACING_TIRE_FIRST = PART_FRONT_TIRE + 2,  /* 0x3A */
    PART_RACING_TIRE_END   = PART_FRONT_TIRE + 9   /* 0x41, exclusive */
};

/* "set_car_code": the dealer's car rec->name becomes the focused car, and
 * the menu car spec (T232) shows it. */
// FUNC 0x00125B50 action_set_car_code
void action_set_car_code(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->setFocusCar(rec->name);
    menu_car_spec()->setCar(rec->name);
}

/* "set_race_code": the race rec->name; go_race will run it as a single race. */
// FUNC 0x00125BA0 action_set_race_code
void action_set_race_code(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->setEvent(rec->name);
    getGameStatus()->getMenuStatus()->raceMode = MAIN_MODE_GTMODE_RACE2;
}

/* "set_race_code_champ": the race rec->name with its last character + 1
 * (the championship's code of the series); go_race will run the
 * championship. */
// FUNC 0x00125BF8 action_set_race_code_champ
void action_set_race_code_champ(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    char code[0x40];                        /* sp+0x00 */

    strcpy(code, rec->name);
    code[strlen(code) - 1]++;
    getGameStatus()->getMenuStatus()->setEvent(code);
    getGameStatus()->getMenuStatus()->raceMode = MAIN_MODE_CHAMPIONSHIP;
}

/* "set_race_code_spot": the race rec->name; go_race will run it as a spot
 * race. */
// FUNC 0x00125C68 action_set_race_code_spot
void action_set_race_code_spot(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->setEvent(rec->name);
    getGameStatus()->getMenuStatus()->raceMode = MAIN_MODE_GTMODE_RACE;
}

/* "set_race_code_info": the race rec->name cut at its last '_' (raceMode is
 * left as it is). */
// FUNC 0x00125CC0 action_set_race_code_info
void action_set_race_code_info(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    char code[0x40];                        /* sp+0x00 */

    str_cut_last_underscore(code, rec->name);
    getGameStatus()->getMenuStatus()->setEvent(code);
}

/* "clear_race_code" ("" is gp-0x6760, 0x00351290). */
// FUNC 0x00125CF8 action_clear_race_code
void action_clear_race_code(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->setEvent("");
}

// FUNC 0x00125D30 action_clear_focus_car
void action_clear_focus_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->clearFocusCar();
}

/* "set_garage_car": the garage car at the current list's cursor becomes the
 * shop car (MenuStatus::setCar) and the selected garage index. */
// FUNC 0x00125D60 action_set_garage_car
void action_set_garage_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    int cursor = ctl->getButtonList()->getCursor();
    GarageCar *car = getGameStatus()->getCar(cursor);
    getGameStatus()->getMenuStatus()->setCar(car);
    getGameStatus()->getMenuStatus()->garageIndex = cursor;
}

/* "set_garage_car_trade": the same from the guest garage (the
 * GameStatusHolder's status, T004); the garage index is not changed. */
// FUNC 0x00125DF0 action_set_garage_car_trade
void action_set_garage_car_trade(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    int cursor = ctl->getButtonList()->getCursor();
    GarageCar *car = getGameStatusHolder()->get()->getCar(cursor);
    getGameStatus()->getMenuStatus()->setCar(car);
}

/* "buy_car": buys the shop car in the colour chosen in car_color_box; the
 * new car is first in the garage (garageIndex 0); the bought sound after
 * half a second. */
// FUNC 0x00125E68 action_buy_car
void action_buy_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    int color = getGameStatus()->getMenuStatus()->color;
    gt2k_game::GameStatus *status = getGameStatus();
    status->buyCar(car->getName(), color);
    getGameStatus()->getMenuStatus()->garageIndex = 0;
    menu_sound_after(&ctl->render.root, MENU_SE_BOUGHT, MENU_SE_DELAY_FRAMES);
}

/* "buy_guest_car": buys a copy of the shop car, a car of the guest garage. */
// FUNC 0x00125F08 action_buy_guest_car
void action_buy_guest_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    getGameStatus()->buyUsedCar(car);
    getGameStatus()->getMenuStatus()->garageIndex = 0;
}

/* "ride_car": the selected garage car becomes the ridden car. */
// FUNC 0x00125F60 action_ride_car
void action_ride_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    int garageIndex = getGameStatus()->getMenuStatus()->garageIndex;
    getGameStatus()->setRunningCarIndex(garageIndex);
}

/* "sell_car": sells the selected garage car. */
// FUNC 0x00125FA8 action_sell_car
void action_sell_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    int garageIndex = getGameStatus()->getMenuStatus()->garageIndex;
    getGameStatus()->sellCar(garageIndex);
}

/* "set_focus_wheel": the wheel at the current list's cursor. */
// FUNC 0x00125FF0 action_set_focus_wheel
void action_set_focus_wheel(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    int cursor = ctl->getButtonList()->getCursor();
    getGameStatus()->getMenuStatus()->wheelIndex = cursor;
}

/* "change_wheel": fits the selected garage wheel to the shop car (an
 * unnamed one is the car's own wheel: code 0); the wheel sound after half a
 * second. */
// FUNC 0x00126048 action_change_wheel
void action_change_wheel(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    int wheelIndex = getGameStatus()->getMenuStatus()->wheelIndex;
    GarageWheel *wheel = getGameStatus()->getWheel(wheelIndex);
    if (wheel->name[0] == '\0')
        car->wheel = 0;
    else
        car->wheel = wheel->getCode();
    menu_sound_after(&ctl->render.root, MENU_SE_WHEEL_FIT, MENU_SE_DELAY_FRAMES);
}

/* "set_focus_wheel_code": the wheel shop's wheel, rec->name cut at its last
 * '_'. */
// FUNC 0x001260E8 action_set_focus_wheel_code
void action_set_focus_wheel_code(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    char code[0x40];                        /* sp+0x00 */

    str_cut_last_underscore(code, rec->name);
    strcpy(getGameStatus()->getMenuStatus()->wheelName, code);
}

/* "buy_wheel": buys the wheel shop's wheel (into the garage's wheels). */
// FUNC 0x00126120 action_buy_wheel
void action_buy_wheel(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    char *wheelName = getGameStatus()->getMenuStatus()->wheelName;
    getGameStatus()->buyWheel(wheelName);
}

/* "fit_wheel": fits the wheel shop's wheel to the ridden car (the wheel
 * sound after half a second) and shows it on the 3D car. */
// FUNC 0x00126160 action_fit_wheel
void action_fit_wheel(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    GarageCar *car = getGameStatus()->getRunningCar();
    char *wheelName = getGameStatus()->getMenuStatus()->wheelName;
    car->wheel = nameToCode(wheelName);
    menu_sound_after(&ctl->render.root, MENU_SE_WHEEL_FIT, MENU_SE_DELAY_FRAMES);
    getGameStatus()->getMenuStatus()->carItem->setWheel(wheelName);
}

// FUNC 0x001261F0 action_go_arcade_menu
void action_go_arcade_menu(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_ARCADE;
}

/* "go_sitemap": leave the root page for the site map ("map"). */
// FUNC 0x00126220 action_go_sitemap
void action_go_sitemap(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->mapRequest = 1;                    /* the expanded GtmodeControl::requestMap */
}

/* "go_top": leave the page for the root page. */
// FUNC 0x00126230 action_go_top
void action_go_top(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->rootRequest = 1;                   /* the expanded GtmodeControl::requestRoot */
}

/* "go_race": the mode the last set_race_code* chose; a day passes. */
// FUNC 0x00126240 action_go_race
void action_go_race(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    menu->nextMode = menu->raceMode;
    getGameStatus()->day++;
}

/* "go_preview": the menu demo race. */
// FUNC 0x00126280 action_go_preview
void action_go_preview(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_MENU_DEMO;
}

/* "go_license": the licence test rec->name; a day passes. */
// FUNC 0x001262B0 action_go_license
void action_go_license(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    menu->setEvent(rec->name);
    menu->nextMode = MAIN_MODE_LICENSE_TEST;
    getGameStatus()->day++;
}

/* "go_machine_test": the machine test rec->name; a day passes. */
// FUNC 0x00126308 action_go_machine_test
void action_go_machine_test(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    menu->setEvent(rec->name);
    menu->nextMode = MAIN_MODE_MACHINE_TEST;
    getGameStatus()->day++;
}

/* "go_free_run": the race "ATAF0001" on the course rec->name (MenuStatus::
 * str84); a day passes. */
// FUNC 0x00126360 action_go_free_run
void action_go_free_run(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    menu->setEvent("ATAF0001");
    menu->nextMode = MAIN_MODE_FREE_RUN;
    menu->setStr84(rec->name);
    getGameStatus()->day++;
}

// FUNC 0x001263D0 action_go_replay_theater
void action_go_replay_theater(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_CARD_REPLAY;
}

// FUNC 0x00126400 action_go_demonstration
void action_go_demonstration(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_DEMO_REPLAY;
}

// FUNC 0x00126430 action_go_logger
void action_go_logger(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_LOGGER;
}

// FUNC 0x00126460 action_go_load_game_menu
void action_go_load_game_menu(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_GAME_LOAD;
}

// FUNC 0x00126490 action_go_save_game_menu
void action_go_save_game_menu(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_GAME_SAVE;
}

// FUNC 0x001264C0 action_go_file_operation
void action_go_file_operation(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_FILE_UTILITY;
}

// FUNC 0x001264F0 action_go_option
void action_go_option(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_OPTIONS;
}

// FUNC 0x00126520 action_go_setting
void action_go_setting(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_SETTING;
}

/* "go_trade": handled by gtmode_main (T038). */
// FUNC 0x00126550 action_go_trade
void action_go_trade(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_GTMODE_TRADE;
}

/* "go_save_in_home": handled by gtmode_main (T038). */
// FUNC 0x00126580 action_go_save_in_home
void action_go_save_in_home(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    getGameStatus()->getMenuStatus()->nextMode = MAIN_MODE_GTMODE_SAVE;
}

/* "set_focus_parts": the part named rec->name (T203; PART_NONE if
 * unknown). */
// FUNC 0x001265B0 action_set_focus_parts
void action_set_focus_parts(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    menu->parts = part_id_of_name(rec->name);
}

/* "buy_parts": buys the focused part for the ridden car (not fitted). */
// FUNC 0x001265F0 action_buy_parts
void action_buy_parts(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    int part = getGameStatus()->getMenuStatus()->parts;
    car->addPart(part);
    getGameStatus()->incMoney(-car->getPartPrice(part));
}

/* "buy_all_tires": buys every racing tyre (front, super hard .. super soft)
 * the ridden car can have and does not have yet. */
// FUNC 0x00126680 action_buy_all_tires
void action_buy_all_tires(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    for (int part = PART_RACING_TIRE_FIRST; part < PART_RACING_TIRE_END; part++) {
        if (car->partExists(part) != 0 && car->hasPart(part) == 0) {
            car->addPart(part);
            getGameStatus()->incMoney(-car->getPartPrice(part));
        }
    }
}

/* "fit_parts": fits the focused part (an owned one: no payment) to the
 * ridden car, after keeping its power for "car_previous_torque".  A front
 * tyre of kind 0..9 (normal .. control) also fits the rear tyre of the
 * same kind (a compare chain in the original; not the dirt tyres). */
// FUNC 0x00126730 action_fit_parts
void action_fit_parts(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    int power;                              /* sp+0x0 */
    int rpm;                                /* sp+0x4 */

    GarageCar *car = getGameStatus()->getRunningCar();
    car->getPower(&power, &rpm);
    ctl->prevPower = power;
    int part = getGameStatus()->getMenuStatus()->parts;
    car->fitPart(part);
    if (part == PART_FRONT_TIRE + 0)
        car->fitPart(PART_REAR_TIRE + 0);
    else if (part == PART_FRONT_TIRE + 1)
        car->fitPart(PART_REAR_TIRE + 1);
    else if (part == PART_FRONT_TIRE + 2)
        car->fitPart(PART_REAR_TIRE + 2);
    else if (part == PART_FRONT_TIRE + 3)
        car->fitPart(PART_REAR_TIRE + 3);
    else if (part == PART_FRONT_TIRE + 4)
        car->fitPart(PART_REAR_TIRE + 4);
    else if (part == PART_FRONT_TIRE + 5)
        car->fitPart(PART_REAR_TIRE + 5);
    else if (part == PART_FRONT_TIRE + 6)
        car->fitPart(PART_REAR_TIRE + 6);
    else if (part == PART_FRONT_TIRE + 7)
        car->fitPart(PART_REAR_TIRE + 7);
    else if (part == PART_FRONT_TIRE + 8)
        car->fitPart(PART_REAR_TIRE + 8);
    else if (part == PART_FRONT_TIRE + 9)
        car->fitPart(PART_REAR_TIRE + 9);
}

/* "buy_fit_parts": buys the focused part and fits it, after keeping the
 * ridden car's power (no rear tyre here, unlike fit_parts). */
// FUNC 0x001268B8 action_buy_fit_parts
void action_buy_fit_parts(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    int power;                              /* sp+0x0 */
    int rpm;                                /* sp+0x4 */

    GarageCar *car = getGameStatus()->getRunningCar();
    car->getPower(&power, &rpm);
    ctl->prevPower = power;
    int part = getGameStatus()->getMenuStatus()->parts;
    car->addPart(part);
    getGameStatus()->incMoney(-car->getPartPrice(part));
    car->fitPart(part);
}

/* The garage's sort orders (GtmodeControl::sortType) and, with some, the
 * info column that shows the sort key (infoMode.set). */
// FUNC 0x00126968 action_sort_color
void action_sort_color(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_COLOR;
}

// FUNC 0x00126978 action_sort_date
void action_sort_date(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_DATE;
}

// FUNC 0x00126980 action_sort_distance
void action_sort_distance(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_DISTANCE;
    ctl->infoMode.set(GARAGE_INFO_DISTANCE);
}

// FUNC 0x001269A8 action_sort_maker
void action_sort_maker(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_MAKER;
    ctl->infoMode.set(GARAGE_INFO_MAKER);
}

// FUNC 0x001269D0 action_sort_name
void action_sort_name(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_NAME;
}

// FUNC 0x001269E0 action_sort_power
void action_sort_power(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_POWER;
    ctl->infoMode.set(GARAGE_INFO_POWER);
}

// FUNC 0x00126A08 action_sort_price
void action_sort_price(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_PRICE;
    ctl->infoMode.set(GARAGE_INFO_PRICE);
}

// FUNC 0x00126A30 action_sort_drive
void action_sort_drive(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_DRIVE;
    ctl->infoMode.set(GARAGE_INFO_DRIVE);
}

// FUNC 0x00126A58 action_sort_aspiration
void action_sort_aspiration(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->sortType = GARAGE_SORT_ASPIRATION;
    ctl->infoMode.set(GARAGE_INFO_ASPIRATION);
}

/* "wash_car": the car wash animation (a TumbleCar under the PageManager's
 * root, Object::operator new; Object::operator delete in the landing pad if
 * the inline constructor throws), pays the "car_wash" service (T435) and
 * cleans the shop car (dirt 0). */
// FUNC 0x00126A80 action_wash_car
void action_wash_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    PageManager *pages = &ctl->pm;
    Car *carItem = getGameStatus()->getMenuStatus()->carItem;
    RootObject *root = pages->root;
    f32 *spin = &carItem->camera.tumbleY.force;
    /* implicit: operator new, Object, operator delete (new-expression, the inline TumbleCar constructor) */
    TumbleCar *tumble = new TumbleCar(carItem, spin, pages);   /* 0x34 bytes */
    tumble->attachTo(root);
    int price = product_config()->servicePrice("car_wash");
    getGameStatus()->incMoney(-price);
    getGameStatus()->getMenuStatus()->car->condition.setDirt(0.0f);
}

/* "oil_change": the page's oil change animation (GtmodeControl::oilCylinder,
 * if any), pays the "oil_change" service and changes the shop car's oil. */
// FUNC 0x00126B98 action_oil_change
void action_oil_change(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    if (ctl->oilCylinder != NULL)
        ctl->oilCylinder->start();
    int price = product_config()->servicePrice("oil_change");
    getGameStatus()->incMoney(-price);
    getGameStatus()->getMenuStatus()->car->condition.changeOil();
}

/* "set_focus_replay_theater": the root page will be entered as after the
 * replay theater (its cursor there, T048). */
// FUNC 0x00126C18 action_set_focus_replay_theater
void action_set_focus_replay_theater(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    GtmodeControl *ctl = (GtmodeControl *)ctx;
    ctl->mode = ROOT_ENTRY_CARD_REPLAY;
}

/* "check_ride_car": nothing (T045's check of the same name does the work). */
// FUNC 0x00126C28 action_check_ride_car
void action_check_ride_car(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
}

/* The table's entry count, .sdata 0x00351294 (a constant word, 54), read by
 * GtmodeControl::begin (T043).  Defined here, after the actions and before
 * the initialiser: its place in .sdata, after clear_race_code's "" and
 * before the initialiser's "buy_car" (0x00351298), shows it. */
int g_00351294 = sizeof(g_0029A180) / sizeof(g_0029A180[0]);    /* 0x00351294 */

/* The action table (.data 0x0029A180..0x0029A330, zero in the original:
 * T037_static_init fills it).  No initialiser: egcs writes it as zeros at
 * the end of the file's .data, before the frame info. */
gt2k_menu::ExecuteFuncData g_0029A180[54];  /* 0x0029A180 */

/* One table entry: the inline constructor of the original's ExecuteFuncData
 * (see T037_static_init). */
static inline void set_action(ExecuteFuncData *e, const char *name,
                              void (*func)(void *ctx, ButtonData *rec, int index,
                                           PageLayout *layout))
{
    e->name = name;
    e->func = func;
}

/* One element as egcs's build_vec_init expands it (u006 / u008 / u009's
 * form): the constructor on a copy of the cursor, then the cursor and the
 * count of the elements left stepped. */
#define ACTION(name, func)                                                    \
    { ExecuteFuncData *e = base; set_action(e, name, func); }                 \
    base++;                                                                   \
    left--

/* Static initialiser (.ctors entry 0x0028F04C): the action table, sorted by
 * name.  Compiler-generated in the original: the table was defined here
 * with ExecuteFuncData objects built by an inline constructor, which egcs
 * builds at run time (cp/init.c build_vec_init; retail's .data
 * 0x0029A180..0x0029A330 is zero).  Written out the way build_vec_init
 * expands it (a walking cursor, a copy of it per element, the dead count of
 * the elements left): an exact form, not the original; it stores the same
 * 54 entries. */
// FUNC 0x00126C30 T037_static_init
void T037_static_init(void)
{
    ExecuteFuncData *base = g_0029A180;
    int left = 53;                          /* build_vec_init's iterator: elements - 1 */

    ACTION("buy_all_tires",            action_buy_all_tires);
    ACTION("buy_car",                  action_buy_car);
    ACTION("buy_fit_parts",            action_buy_fit_parts);
    ACTION("buy_guest_car",            action_buy_guest_car);
    ACTION("buy_parts",                action_buy_parts);
    ACTION("buy_wheel",                action_buy_wheel);
    ACTION("change_wheel",             action_change_wheel);
    ACTION("check_ride_car",           action_check_ride_car);
    ACTION("clear_focus_car",          action_clear_focus_car);
    ACTION("clear_race_code",          action_clear_race_code);
    ACTION("fit_parts",                action_fit_parts);
    ACTION("fit_wheel",                action_fit_wheel);
    ACTION("go_arcade_menu",           action_go_arcade_menu);
    ACTION("go_demonstration",         action_go_demonstration);
    ACTION("go_file_operation",        action_go_file_operation);
    ACTION("go_free_run",              action_go_free_run);
    ACTION("go_license",               action_go_license);
    ACTION("go_load_game_menu",        action_go_load_game_menu);
    ACTION("go_logger",                action_go_logger);
    ACTION("go_machine_test",          action_go_machine_test);
    ACTION("go_option",                action_go_option);
    ACTION("go_preview",               action_go_preview);
    ACTION("go_race",                  action_go_race);
    ACTION("go_replay_theater",        action_go_replay_theater);
    ACTION("go_save_game_menu",        action_go_save_game_menu);
    ACTION("go_save_in_home",          action_go_save_in_home);
    ACTION("go_setting",               action_go_setting);
    ACTION("go_sitemap",               action_go_sitemap);
    ACTION("go_top",                   action_go_top);
    ACTION("go_trade",                 action_go_trade);
    ACTION("oil_change",               action_oil_change);
    ACTION("ride_car",                 action_ride_car);
    ACTION("sell_car",                 action_sell_car);
    ACTION("set_car_code",             action_set_car_code);
    ACTION("set_focus_parts",          action_set_focus_parts);
    ACTION("set_focus_replay_theater", action_set_focus_replay_theater);
    ACTION("set_focus_wheel",          action_set_focus_wheel);
    ACTION("set_focus_wheel_code",     action_set_focus_wheel_code);
    ACTION("set_garage_car",           action_set_garage_car);
    ACTION("set_garage_car_trade",     action_set_garage_car_trade);
    ACTION("set_race_code",            action_set_race_code);
    ACTION("set_race_code_champ",      action_set_race_code_champ);
    ACTION("set_race_code_info",       action_set_race_code_info);
    ACTION("set_race_code_spot",       action_set_race_code_spot);
    ACTION("sort_aspiration",          action_sort_aspiration);
    ACTION("sort_color",               action_sort_color);
    ACTION("sort_date",                action_sort_date);
    ACTION("sort_distance",            action_sort_distance);
    ACTION("sort_drive",               action_sort_drive);
    ACTION("sort_maker",               action_sort_maker);
    ACTION("sort_name",                action_sort_name);
    ACTION("sort_power",               action_sort_power);
    ACTION("sort_price",               action_sort_price);
    ACTION("wash_car",                 action_wash_car);
}

#undef ACTION


// FUNC 0x00127220 gt2k_menu::TumbleCar::virtual_1
/* Update (slot 1): inline in game/menu/gtmode/T037_tumble_car.h (the class has no key
 * function); egcs emits this copy for the vtable, at the end of the unit
 * with __tf and the implicit destructor. */
/* implicit: menu_play_se, setFreeze, kill (the inline body in game/menu/gtmode/T037_tumble_car.h) */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of TumbleCar (base Object, whose node is
 * __tiQ29gt2k_menu6Object gp-0x4600) on the first call. */
// FUNC 0x001272D0 __tfQ29gt2k_menu9TumbleCar
extern "C" void *__tfQ29gt2k_menu9TumbleCar(void)
{
    if (__tiQ29gt2k_menu9TumbleCar[0] == 0) {           /* _name not set yet */
        __tfQ29gt2k_menu6Object();                      /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu9TumbleCar, "Q29gt2k_menu9TumbleCar",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu9TumbleCar;
}
#endif

// FUNC 0x00127320 gt2k_menu::TumbleCar::~TumbleCar
/* implicit in game/menu/gtmode/T037_gtmode_actions.h; egcs emits this copy for the
 * vtable: no vptr store; gt2k_menu::Object::~Object(this, 0), then the
 * class Object::operator delete (0x001BF458) if flags & 1. */
/* implicit: ~Object (base destructor), operator delete (deleting destructor) */
