/* T036 - the GT-mode page hooks (0x0011C9C8..0x00125B50): the create
 * functions (make_*) and text callbacks (text_*) of the garage, car shop,
 * status and licence pages, the tuning shop's price texts, the event list and
 * regulation pages, the car spec sheet and the race prizes; T036_static_init,
 * which fills the hook table; DecayItem; and the copies egcs emits at the end
 * of the unit for the classes whose vtables it writes here.  See garage_internal.h and
 * game/menu/gtmode/T036_garage.h for how the hooks are reached (the MenuHook table
 * g_00296B90); each comment gives the entry name(s).
 *
 * One translation unit, as in retail; the work units u010..u013 held it in
 * four files until the T036 merge (ref/matching/u010/notes.md, "Merge
 * (T036)").
 *
 * The garage / shop car is MenuStatus::car, its 3D view MenuStatus::carItem;
 * the ridden car GameStatus::getRunningCar(); the current race / event
 * MenuStatus::event, the current list ctl->getButtonList() and its focused
 * event the name of its focused record (ButtonList::focusData()); the car on
 * show menu_car_spec() (T232).  Text callbacks write into buf (0x80 bytes;
 * size is not read).  A `new' of a menu object is Object::operator new (the
 * current menu heap), with Object::operator delete in the landing pad if the
 * constructor throws. */

/* The includes, in the order of retail T036's class definitions (data
 * layout): egcs writes a unit's type names in the reverse order of the class
 * definitions, and retail's run (.rodata 0x00311C70..0x00311EE6, after the
 * vtables) is DecayItem, List<Button>, Cell<Button>, ItemText,
 * SmartIndexT<int>, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, RegurationResult, GarageWheelList, List<GarageWheel>,
 * GarageWheel, Cell<GarageWheel>, GarageCarList, List<GarageCar>, GarageCar,
 * Cell<GarageCar>, Operator (a non-polymorphic base: last).  So the
 * GameStatus' garage first, then RegurationResult, the Transitive classes,
 * SmartIndexT<int>, ItemText, Button, ButtonList, the rest, DecayItem last. */
/* T036's .rodata (0x00310570) opens with "(%f, %f, %f) %f %f %f\n", the
 * string of SimpleCameraBase::print, an inline member egcs writes in every
 * unit that reads the class (u003, u064): without it the strings and jump
 * tables after it are 0x18 bytes early and the 16-aligned jump tables 8 bytes
 * off.  Nothing references it: DATA_START below. */
#include "game/camera/T412_simple_camera_base.h"
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus; GarageCar
                                               (Cell<GarageCar>, CarInformation), the garage lists */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hooks' result, start);
                                               Operator, OperatorHolder, OperatorList */
#include "game/common/T251_smart_index.h"          /* (data layout only) SmartIndexT<int>, instantiated
                                               where the header is read */
#include "game/menu/gtmode/T036_item_text.h"            /* (data layout only) gt2k_menu::ItemText */
#include "game/menu/framework/T206_button.h"               /* gt2k_menu::Button (its Cell<Button> base) */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList, ButtonData (List<Button>) */
#include "game/menu/gtmode/T036_garage.h"               /* the hooks, g_00296B90, g_00351278, T036_static_init */
#include "engine/pgl/G001_flare.h"                /* fmt_sprintf ("%.1t" / "%.2t" / "%.3t": an int with 1 / 2 / 3
                                               decimals) */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen (0x00269A68) */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/gtmode/T039_gtmode_messages.h"      /* g_003512C8 (the GTMODE texts: "purchased") */
#include "game/menu/gtmode/T040_oil_change_cylinder.h"  /* gt2k_menu::OilChangeCylinder */
#include "game/menu/gtmode/T041_event_status.h"         /* gt2k_menu::EventStatusObject, EventStatusButtonObject */
#include "game/menu/gtmode/T042_event_hider.h"          /* gt2k_menu::EventHider */
#include "game/menu/gtmode/T043_gtmode_control.h"       /* gt2k_menu::GtmodeControl (pm, prevPower, oilCylinder) */
#include "game/menu/gtmode/T045_gtmode_page_checks.h"   /* __tf16RegurationResult, __ti16RegurationResult */
#include "game/data/T203_part_names.h"           /* part_id_of_name */
#include "game/menu/framework/T205_blink.h"                /* create_blink */
#include "game/menu/framework/T210_arrow.h"                /* gt2k_menu::Arrow */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (popupLevel, buttonLists, setFreeze) */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, MenuTexture, __tfQ29gt2k_menu4Item */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete, z) */
#include "game/menu/framework/T226_menu_image_holder.h"    /* MenuImageHolder */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuLayer, MenuHook(Fn) */
#include "game/menu/framework/T230_scroll_bar.h"           /* gt2k_menu::create_scroll_bar */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec, menu_car_spec */
#include "game/menu/framework/T234_bool_item.h"            /* gt2k_menu::create_bool_item */
#include "game/menu/framework/T235_color_box.h"            /* gt2k_menu::ColorBoxSetFn, create_color_box */
#include "game/menu/framework/T239_callback_text.h"        /* gt2k_menu::CallbackTextFn, create_callback_text */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "game/menu/framework/T249_mpeg_image.h"           /* gt2k_menu::MpegImage */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar::power / torque */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition (odometerDistance, dirt, oilDirtiness) */
#include "game/save/T420_license_records.h"      /* LicenseRecords, LicenseMedal */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus (car, carItem, color, parts, event) */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments, the PART_* ids */
#include "game/data/T483_car_information.h"      /* CarInformation, __tf14CarInformation */
#include "game/data/T484_race_database.h"        /* RaceDataBase, RaceEvent, RaceTime */
#include "game/data/T489_car_database.h"         /* check_regulation */
#include "game/common/T494_text_utils.h"           /* format_number, format_time, str_cut_last_underscore */
#include "game/menu/framework/T495_menu_list.h"            /* pCell, __tfQ29gt2k_menu5pCell */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::Cell<T> */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user, __rtti_class,
                                               __class_type_info::base_info (the GCC 14 __tf functions) */
#endif
/* own translation unit, last: DecayItem (retail T036 defined it after every
 * other polymorphic class) */
#include "game/menu/gtmode/T036_decay_item.h"
#include "game/menu/gtmode/garage_internal.h"                           /* the TU's constants and helpers */

// DATA_START 0x00310570

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* The garage, car shop, status and licence pages                           */
/* ------------------------------------------------------------------------ */

/* "mpeg": a MpegImage of the item. */
// FUNC 0x0011C9C8 make_mpeg
Transitive *make_mpeg(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                      int p8, int pC, int p10, int p14)
{
    return new MpegImage(item, layout);     /* Object::operator new (0x120); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
}

/* "user_carname": name 1 of the ridden car, "" without one. */
// FUNC 0x0011CA40 text_user_carname
void text_user_carname(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car != NULL)
        strcpy(buf, car->getName1());
    else
        buf[0] = '\0';
}

/* "user_carname_full": names 1 and 2 of the ridden car, "" without one. */
// FUNC 0x0011CA90 text_user_carname_full
void text_user_carname_full(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car != NULL) {
        sprintf(buf, "%s%s", car->getName1(), car->getName2());
        return;
    }
    buf[0] = '\0';
}

/* "user_day" (once): the day counter. */
// FUNC 0x0011CB18 text_user_day
void text_user_day(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->day);
}

/* "user_day_unit": an Item of the item showing the day counter, with the
 * item's own text as the printf format; NULL if the item has no text (the
 * text pointer is read without PageLayout::getText's flag test). */
// FUNC 0x0011CB60 make_user_day_unit
Transitive *make_user_day_unit(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                               int p8, int pC, int p10, int p14)
{
    char text[0x40];                        /* sp+0x00 */

    if (item->text != NULL && item->text->text != NULL) {
        Item *dayItem = new Item(item, layout); /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
        sprintf(text, item->text->text, getGameStatus()->day);
        dayItem->setText(text);
        return dayItem;
    }
    return 0;
}

/* "user_money": the money with thousands separators. */
// FUNC 0x0011CC20 text_user_money
void text_user_money(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, getGameStatus()->money);
}

/* "focused_carname": name 1 of the garage / shop car. */
// FUNC 0x0011CC50 text_focused_carname
void text_focused_carname(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, getGameStatus()->getMenuStatus()->car->getName1());
}

/* "car_buy_price": the garage / shop car's price, "----" for a car the
 * dealer does not sell. */
// FUNC 0x0011CC98 text_car_buy_price
void text_car_buy_price(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    if (car->getShopFlags() & SHOP_FLAG_NOT_SOLD) {
        strcpy(buf, "----");
        return;
    }
    format_number(buf, car->getPrice());
}

/* "car_sell_price": a quarter of the price (signed division, as
 * CarInformation::getSellPrice, written out), "----" for a car the used car
 * shop does not trade. */
// FUNC 0x0011CD28 text_car_sell_price
void text_car_sell_price(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    if ((car->getShopFlags() >> 1) & 1) {   /* SHOP_FLAG_NO_USED */
        strcpy(buf, "----");
        return;
    }
    format_number(buf, car->getPrice() / 4);
}

/* "car_kilometrage": the garage / shop car's odometer (tenths of a km or a
 * mile) and the distance unit. */
// FUNC 0x0011CDD0 text_car_kilometrage
void text_car_kilometrage(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    fmt_sprintf(buf, "%.1t%s", car->condition.odometerDistance(), g_002F59D8.distanceName());
}

/* "car_current_torque" and "disp_ps_befor": the POWER of the ridden car
 * (the "torque" entry shows the power too), "" without one. */
// FUNC 0x0011CE48 text_current_power
void text_current_power(GtmodeControl *ctl, char *buf, int size)
{
    int power;                              /* sp+0x0 */
    int rpm;                                /* sp+0x4 */

    GarageCar *car = getGameStatus()->getRunningCar();
    if (car != NULL) {
        car->getPower(&power, &rpm);
        const char *unit = g_002F59D8.powerName();
        sprintf(buf, "%d %s", power, unit);
        return;
    }
    buf[0] = '\0';
}

/* "car_previous_torque": the power before the part was fitted
 * (GtmodeControl::prevPower). */
// FUNC 0x0011CEB8 text_previous_power
void text_previous_power(GtmodeControl *ctl, char *buf, int size)
{
    int power = ctl->prevPower;
    const char *unit = g_002F59D8.powerName();
    sprintf(buf, "%d %s", power, unit);
}

/* "disp_ps_after" (once): the power of a copy of the ridden car with the
 * focused part fitted (the part named by the current list's focused button,
 * part_id_of_name); "----" if the car cannot take that part, "" without a
 * car, a list or a focused button. */
// FUNC 0x0011CF08 text_ps_after
void text_ps_after(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden == NULL) {
        buf[0] = '\0';
        return;
    }
    /* GarageCar() 0x0023AEA0; its inline destructor runs on every return:
     * GarageCar's vptrs, ~CarInformation (flags 0), then Cell<GarageCar>'s
     * vptr and ~pCell (flags 2). */
    /* implicit: ~CarInformation, ~pCell (the inline ~GarageCar of car) */
    GarageCar car;                          /* sp+0x000 (0x230 bytes) */
    car.assign(*ridden);
    ButtonList *list = ctl->getButtonList();
    if (list == NULL) {
        buf[0] = '\0';
        return;
    }
    ButtonData *focused = list->focusData();
    if (focused == NULL) {
        buf[0] = '\0';
        return;
    }
    int part = part_id_of_name(focused->name);
    if (car.partExists(part) == 0) {
        strcpy(buf, "----");
        return;
    }
    car.fitPart(part);
    int power;                              /* sp+0x230 */
    int rpm;                                /* sp+0x234 */
    car.getPower(&power, &rpm);
    const char *unit = g_002F59D8.powerName();
    sprintf(buf, "%d %s", power, unit);
}

/* "car_drive_type": the garage / shop car's drive train ("FR", "FF", ...). */
// FUNC 0x0011D188 text_car_drive_type
void text_car_drive_type(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, getGameStatus()->getMenuStatus()->car->getDriveTrainText());
}

/* "car_max_power": "<power><unit>/<rpm>rpm" of the car on show (the spec
 * sheet's figures, catalogue 0). */
// FUNC 0x0011D1D0 text_car_max_power
void text_car_max_power(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0 */

    int power = menu_car_spec()->car.power(&rpm, UNIT_TAG_PS, 0);
    const char *unit = g_002F59D8.powerName();
    sprintf(buf, "%d%s/%drpm", power, unit, rpm);
}

/* "car_max_torque": "<torque>.<1 digit><unit>/<rpm>rpm"; the torque comes in
 * tenths. */
// FUNC 0x0011D240 text_car_max_torque
void text_car_max_torque(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0 */

    int tenths = menu_car_spec()->car.torque(&rpm, UNIT_TAG_DKGM, 0);
    /* implicit: fptodp (float to double conversion) */
    /* (tenths * 0.1f: a float multiply, then fptodp, 0x0027AFA0, the
     * soft-float __extendsfdf2, before torqueName: the format is loaded
     * first, the argument conversion is expanded in order.) */
    sprintf(buf, "%.1f%s/%drpm", tenths * 0.1f, g_002F59D8.torqueName(), rpm);
}

/* "car_weight": a fixed placeholder (leaf function). */
// FUNC 0x0011D2D0 text_car_weight
void text_car_weight(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, "550t");
}

/* "goods_car_price": an Item of the item with the price of the car named by
 * the item (up to its last '_'): "Cr. <price>", or "----" for a car the
 * dealer does not sell; NULL for an unknown car.  The CarInformation is
 * destroyed (~CarInformation, flags 2) before the Item is made; the text
 * buffer reuses its stack slot. */
// FUNC 0x0011D2F8 make_goods_car_price
Transitive *make_goods_car_price(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                 int p8, int pC, int p10, int p14)
{
    char code[0x40];                        /* sp+0x00, then the formatted price */
    int  price;
    int  notSold;

    str_cut_last_underscore(code, item->name);
    {
        CarInformation info;                /* sp+0x40 */
        if (info.loadCar(nameToCode(code)) == 0)
            return 0;
        price   = info.getPrice();
        notSold = info.getShopFlags() & SHOP_FLAG_NOT_SOLD;
    }
    Item *priceItem = new Item(item, layout);   /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    if (notSold) {
        priceItem->setText("----");
    } else {
        char text[0x80];                    /* sp+0x40 (? size: at most the CarInformation's 0x218) */
        format_number(code, price);
        sprintf(text, "Cr. %s", code);
        priceItem->setText(text);
    }
    return priceItem;
}

/* "status_complete": the game's completion in tenths of a percent, at most
 * 99.9 unless it is exactly 1.0.  (int) is cvt.w.s, which truncates on the
 * EE. */
// FUNC 0x0011D460 text_status_complete
void text_status_complete(GtmodeControl *ctl, char *buf, int size)
{
    f32 ratio = getGameStatus()->getCompletionRatio();
    if (ratio == 1.0f) {
        strcpy(buf, "100.0 %");
        return;
    }
    int tenths = (int)(ratio * 1000.0f);
    if (tenths >= 1000)
        tenths = 999;
    fmt_sprintf(buf, "%.1t %%", tenths);
}

/* "status_ave_prize_car": the cars won (not an average). */
// FUNC 0x0011D4F8 text_status_ave_prize_car
void text_status_ave_prize_car(GtmodeControl *ctl, char *buf, int size)
{
    int num = getGameStatus()->getPresentNum();
    sprintf(buf, "%d", num);
}

/* "status_ttl_wins". */
// FUNC 0x0011D530 text_status_ttl_wins
void text_status_ttl_wins(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->winnerNum);
}

/* "status_avg_win": the percentage of races won. */
// FUNC 0x0011D578 text_status_avg_win
void text_status_avg_win(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d %%", getGameStatus()->getWinRate());
}

/* "status_ttl_races". */
// FUNC 0x0011D5C8 text_status_ttl_races
void text_status_ttl_races(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->battleNum);
}

/* "status_no_cars": the cars in the garage. */
// FUNC 0x0011D610 text_status_no_cars
void text_status_no_cars(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->getCarNum());
}

/* "status_prize_money": the prize money won. */
// FUNC 0x0011D660 text_status_prize_money
void text_status_prize_money(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, getGameStatus()->totalPrize);
}

/* "status_no_lc_g": the licence tests passed with gold. */
// FUNC 0x0011D690 text_status_no_lc_g
void text_status_no_lc_g(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->licences.countMedal(LICENSE_MEDAL_GOLD));
}

/* "status_no_lc_s": ... with silver. */
// FUNC 0x0011D6E8 text_status_no_lc_s
void text_status_no_lc_s(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->licences.countMedal(LICENSE_MEDAL_SILVER));
}

/* "status_no_lc_b": ... with bronze. */
// FUNC 0x0011D740 text_status_no_lc_b
void text_status_no_lc_b(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->licences.countMedal(LICENSE_MEDAL_BRONZE));
}

/* "status_no_trophy". */
// FUNC 0x0011D798 text_status_no_trophy
void text_status_no_trophy(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "%d", getGameStatus()->trophyNum);
}

/* "status_ttl_car_price": the total price of the garage's cars. */
// FUNC 0x0011D7E0 text_status_ttl_car_price
void text_status_ttl_car_price(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, getGameStatus()->getTotalCarPrice());
}

/* "status_ttl_milage": the total distance / 50 (unsigned: tenths of a km)
 * and the distance unit.  Unlike CarCondition::odometerDistance it never
 * divides by 80 for miles, so an imperial setting labels km as miles. */
// FUNC 0x0011D818 text_status_ttl_milage
void text_status_ttl_milage(GtmodeControl *ctl, char *buf, int size)
{
    fmt_sprintf(buf, "%.1t %s", getGameStatus()->distance / 50, g_002F59D8.distanceName());
}

/* "license_mark": the mark of the highest licence passed (a hidden item),
 * placed on the item; NULL if none.  The switch is unsigned (sltiu 5): no
 * mark for -1.  The art names do not follow the licence order. */
// FUNC 0x0011D890 make_license_mark
Transitive *make_license_mark(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    const char *name = NULL;
    switch (getGameStatus()->licences.highestPassed()) {
    case LICENSE_B:  name = "lc_mark2"; break;
    case LICENSE_A:  name = "lc_mark1"; break;
    case LICENSE_IB: name = "lc_mark4"; break;
    case LICENSE_IA: name = "lc_mark3"; break;
    case LICENSE_S:  name = "lc_mark5"; break;
    }
    MenuLayoutItem *mark;
    if (name == NULL || (mark = layout->findHiddenItem(name)) == NULL)
        return 0;
    MenuTexture *tex = layout->getTexture(mark);
    Item *markItem = new Item(mark, tex);   /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    markItem->setRect(item);
    return markItem;
}

/* "license_mark_rally": the hidden "lc_mark6" placed on the item once the R
 * (rally) licence is passed (all 8 tests); NULL before. */
// FUNC 0x0011D9D0 make_license_mark_rally
Transitive *make_license_mark_rally(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                    int p8, int pC, int p10, int p14)
{
    const char *name = NULL;
    if (getGameStatus()->licences.isPassed(LICENSE_R, 0) != 0)
        name = "lc_mark6";
    MenuLayoutItem *mark;
    if (name == NULL || (mark = layout->findHiddenItem(name)) == NULL)
        return 0;
    MenuTexture *tex = layout->getTexture(mark);
    Item *markItem = new Item(mark, tex);   /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    markItem->setRect(item);
    return markItem;
}

/* "user_license": the licence card of the highest licence passed, at the
 * hidden item's own place (item is not used).  The jump table has 8 entries
 * (-1..6): 5 ("license_d") and 6 (no card) are dead, highestPassed() returns
 * -1..4. */
// FUNC 0x0011DAC0 make_user_license
Transitive *make_user_license(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    const char *name = NULL;
    switch (getGameStatus()->licences.highestPassed()) {
    case -1:         name = "license_no"; break;
    case LICENSE_B:  name = "license_b";  break;
    case LICENSE_A:  name = "license_a";  break;
    case LICENSE_IB: name = "license_ib"; break;
    case LICENSE_IA: name = "license_ia"; break;
    case LICENSE_S:  name = "license_s";  break;
    case LICENSE_R:  name = "license_d";  break;
    case 6:          break;
    }
    MenuLayoutItem *card;
    if (name == NULL || (card = layout->findHiddenItem(name)) == NULL)
        return 0;
    MenuTexture *tex = layout->getTexture(card);
    return new Item(card, tex);             /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
}

/* "need_license": the mark of the licence the focused event needs, placed on
 * the item.  The event code is the name of the current list's focused
 * button with a final '0' made '1' (no NULL test of the list or the button,
 * none for an empty name: code[-1] is read then).  NULL if the event needs
 * none; the licence byte is not range-checked.  The RaceDataBase is
 * destroyed (flags 2) after the Item is made. */
// FUNC 0x0011DBF8 make_need_license
Transitive *make_need_license(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    GtmodeControl *ctl = static_cast<GtmodeControl *>(ctx);
    char code[0x40];                        /* sp+0x00 */

    strcpy(code, ctl->getButtonList()->focusData()->name);
    char *last = &code[strlen(code) - 1];
    if (*last == '0')
        *last = *last + 1;
    RaceDataBase db;                        /* sp+0x40 */
    db.select(nameToCode(code));
    u8 license = db.event->needLicense;
    if (license == 0)
        return 0;
    const char *marks[7] = {                /* sp+0x70, copied from 0x00310738 */
        "license_unlimited", "lc_mark_b", "lc_mark_a", "lc_mark_ib",
        "lc_mark_ia", "lc_mark_s", "lc_mark_r",
    };
    const char *mark = marks[license];      /* (read before the new, as retail) */
    /* implicit: Object::operator delete (if the constructor throws) */
    return new Item(mark, layout, item);    /* Object::operator new; operator delete */
}

/* "disp_license_cup_<licence><n>" (test = licence << 4 | n - 1, licences b 0,
 * a 1, ib 2, ia 3, s 4, r 5): the cup won in that test, placed on the item;
 * NULL if none.  The conditional new keeps egcs's "new in progress" flags
 * for the landing pad (operator delete only while Item() runs). */
// FUNC 0x0011DDA0 make_license_cup
Transitive *make_license_cup(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int test, int pC, int p10, int p14)
{
    const char *name = NULL;
    int licence = test >> LICENSE_CUP_SHIFT;
    int n = test & LICENSE_CUP_TEST_MASK;
    switch (getGameStatus()->licences.getMedal(licence, n)) {
    case LICENSE_MEDAL_GOLD:   name = "cup_gold";   break;
    case LICENSE_MEDAL_SILVER: name = "cup_silver"; break;
    case LICENSE_MEDAL_BRONZE: name = "cup_bronze"; break;
    }
    /* Object::operator new; operator delete */
    return name != NULL ? new Item(name, layout, item) : 0;  /* implicit: Object::operator delete (the conditional new's cleanup, twice) */
}

/* The gold (0), silver (1) or bronze (2) time of the licence test named by
 * item (up to its last '_'), as an Item text (0 ms for another medal).  The
 * RaceDataBase is destroyed (flags 2) before the time is formatted. */
// FUNC 0x0011DF08 make_license_time
Item *make_license_time(PageLayout *layout, MenuLayoutItem *item, int medal)
{
    char text[0x40];                        /* sp+0x00: the code, then the time */
    int  ms;

    str_cut_last_underscore(text, item->name);
    ms = 0;
    {
        RaceDataBase db;                    /* sp+0x40 */
        db.select(nameToCode(text));
        switch (medal) {
        case LICENSE_MEDAL_GOLD:   ms = RACE_TIME_MS(db.event->time[0]); break;
        case LICENSE_MEDAL_SILVER: ms = RACE_TIME_MS(db.event->time[1]); break;
        case LICENSE_MEDAL_BRONZE: ms = RACE_TIME_MS(db.event->time[2]); break;
        }
    }
    format_time(text, ms);
    /* implicit: operator new, operator delete (new-expression) */
    Item *timeItem = new Item(item, layout);    /* Object::operator new; operator delete */
    timeItem->setText(text);
    return timeItem;
}

/* "disp_time_gold". */
// FUNC 0x0011E0A8 make_time_gold
Transitive *make_time_gold(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                           int p8, int pC, int p10, int p14)
{
    return make_license_time(layout, item, LICENSE_MEDAL_GOLD);
}

/* "disp_time_silver". */
// FUNC 0x0011E0D0 make_time_silver
Transitive *make_time_silver(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14)
{
    return make_license_time(layout, item, LICENSE_MEDAL_SILVER);
}

/* "disp_time_bronze". */
// FUNC 0x0011E0F8 make_time_bronze
Transitive *make_time_bronze(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14)
{
    return make_license_time(layout, item, LICENSE_MEDAL_BRONZE);
}

/* "license_car_name": "<name 1> <name 2>" of entry car 0 of the licence
 * test named by the item (up to its last '_'), "error" if it has none, as an
 * Item text.  ~CarEquipments and ~RaceDataBase (flags 2) run before the
 * Item is made. */
// FUNC 0x0011E120 make_license_car_name
Transitive *make_license_car_name(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                  int p8, int pC, int p10, int p14)
{
    char text[0x80];                        /* sp+0x00 */
    char code[0x40];                        /* sp+0x80 */

    str_cut_last_underscore(code, item->name);
    {
        RaceDataBase db;                    /* sp+0xC0 */
        db.select(nameToCode(code));
        CarEquipments car;                  /* sp+0xF0 */
        if (db.setupEntryCar(0, &car) != 0) {
            sprintf(text, "%s %s", car.getName1(), car.getName2());
        } else {
            strcpy(text, "error");
        }
    }
    Item *nameItem = new Item(item, layout);    /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    nameItem->setText(text);
    return nameItem;
}

/* "license_launch_speed": the start speed of the licence test and the speed
 * unit, as an Item text.  A CarEquipments is built and destroyed but not
 * used (the code of license_car_name, copied). */
// FUNC 0x0011E2C0 make_license_launch_speed
Transitive *make_license_launch_speed(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                      int p8, int pC, int p10, int p14)
{
    char text[0x80];                        /* sp+0x00 */
    char code[0x40];                        /* sp+0x80 */

    str_cut_last_underscore(code, item->name);
    {
        RaceDataBase db;                    /* sp+0xC0 */
        db.select(nameToCode(code));
        CarEquipments unused;               /* sp+0xF0 */
        sprintf(text, "%d%s", db.event->launchSpeed, g_002F59D8.speedName());
    }
    Item *speedItem = new Item(item, layout);   /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    speedItem->setText(text);
    return speedItem;
}

/* The car_render_* hooks' body: a gt2k_menu::Car (0x720 bytes) showing car
 * in camera preset view, placed on the item (Item::setRect's code), its
 * model fade from the car's dirt, its wheel getWheelModelName(), its
 * colour; remembered as MenuStatus::carItem.  The item's texture goes
 * through a MenuImageHolder on the stack (~MenuImageHolder, flags 2, at the
 * end); its data is the Car's environment image.  parent is not used. */
// FUNC 0x0011E428 make_car_render
Car *make_car_render(GarageCar *car, PageLayout *layout, Object *parent, MenuLayoutItem *item, int view)
{
    MenuImageHolder texture(item, layout->getTexture(item));    /* sp+0x00 */
    /* Object::operator new; operator delete if Car() throws */
    Car *carItem = new Car(holder_image(texture), menu_car_spec(), 0, 0); /* implicit: Object::operator delete (if the constructor throws) */
    carItem->setDirt(car_condition(*car)->dirt);
    texture.handOver(carItem);
    const char *wheel = car->getWheelModelName();
    carItem->setCameraMode(view);
    /* Drawable::setRect: the centre (y up from the bottom) and the size;
     * item's h, w, y and x are read before the first store, z after. */
    carItem->setRect(item->x + item->w * 0.5f, (f32)MENU_SCREEN_H - (item->y + item->h * 0.5f),
                     item->w, item->h);
    carItem->z = item->z;
    carItem->setCar(car->getName());
    carItem->setWheel(wheel);
    carItem->updateViewport();              /* the viewport from the rectangle (y and h halved) */
    carItem->setColor(car->getColorIndex());
    getGameStatus()->getMenuStatus()->carItem = carItem;
    return carItem;
}

/* "car_render_car_wash" (8), "car_render_mine" / "_mine_side" (5),
 * "car_render_oil_change" (10), "car_render_wheel_shop" (7): the ridden car,
 * which also becomes MenuStatus::car (MenuStatus::setCar already puts it in
 * the MenuCarSpec; it is done again).  Without one: carItem = NULL, NULL. */
// FUNC 0x0011E630 make_car_render_current
Transitive *make_car_render_current(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                    int view, int pC, int p10, int p14)
{
    GarageCar *car = getGameStatus()->getRunningCar();
    if (car == NULL) {
        getGameStatus()->getMenuStatus()->carItem = 0;
        return 0;
    }
    getGameStatus()->getMenuStatus()->setCar(car);
    menu_car_spec()->setCar(static_cast<CarInformation *>(car));
    return make_car_render(car, layout, parent, item, view);
}

/* "car_render_wheel" (6): MenuStatus::car (the code of
 * make_car_render_focused). */
// FUNC 0x0011E700 make_car_render_wheel
Transitive *make_car_render_wheel(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                  int view, int pC, int p10, int p14)
{
    return make_car_render(getGameStatus()->getMenuStatus()->car, layout, parent, item, view);
}

/* "car_render_from" / "car_render_shop" (4), "car_render_garage" (5):
 * MenuStatus::car. */
// FUNC 0x0011E770 make_car_render_focused
Transitive *make_car_render_focused(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                    int view, int pC, int p10, int p14)
{
    return make_car_render(getGameStatus()->getMenuStatus()->car, layout, parent, item, view);
}

/* "car_render_buy" (11): MenuStatus::car painted in the colour chosen in the
 * colour box, without the mirror pass, at z 300. */
// FUNC 0x0011E7E0 make_car_render_buy
Transitive *make_car_render_buy(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                int view, int pC, int p10, int p14)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    car->setColorIndex(getGameStatus()->getMenuStatus()->color);
    Car *carItem = make_car_render(car, layout, parent, item, view);
    carItem->drawMirror = 0;
    carItem->z = 300.0f;
    return carItem;
}

/* "car_logo": the hidden item named after MenuStatus::car's code, at its own
 * place (item is not used); NULL if there is none. */
// FUNC 0x0011E888 make_car_logo
Transitive *make_car_logo(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                          int p8, int pC, int p10, int p14)
{
    const char *code = getGameStatus()->getMenuStatus()->car->getName();
    if (code == NULL)
        return 0;
    MenuLayer *hidden = layout->getHiddenLayer();
    MenuLayoutItem *logo;
    if (hidden == NULL || (logo = PageLayout::findItemIn(code, hidden)) == NULL)
        return 0;
    MenuTexture *tex = layout->getTexture(logo);
    return new Item(logo, tex);             /* Object::operator new; operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
}

/* The colours car_color_box_colors hands to the ColorBox (T036 .bss, a file
 * static).  32 words: the original's size is not in the code (only the base
 * is referenced, the count is the car's colour count), but T036's .bss has
 * 0x80 bytes up to T097's g_0049BCA0, the next object's .bss in link order
 * (T037 .. T096 have none). */
static u32 g_0049BC20[32];                  /* 0x0049BC20 */

/* "car_color_box" (the ColorBox getter, p8): the colours of the car on show
 * with alpha 0x80, in the static table g_0049BC20, and their count. */
// FUNC 0x0011E958 car_color_box_colors
u32 *car_color_box_colors(int *count)
{
    *count = menu_car_spec()->getColorCount();
    menu_car_spec()->getColors(g_0049BC20, COLOR_BOX_ALPHA);
    return g_0049BC20;
}

/* "car_color_box" (the setter, pC): repaints the 3D car (if any) and
 * remembers the colour. */
// FUNC 0x0011E9A8 car_color_box_select
void car_color_box_select(GtmodeControl *ctl, int color)
{
    Car *carItem = getGameStatus()->getMenuStatus()->carItem;
    if (carItem != NULL)
        carItem->setColor(color);
    getGameStatus()->getMenuStatus()->color = color;
}

/* "car_color_name": the name of the garage / shop car's colour, " " for a
 * name starting with '-'.  getColorName may return NULL (out of range): the
 * first byte is read regardless. */
// FUNC 0x0011EA08 text_car_color_name
void text_car_color_name(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    int colorIndex = car->colorIndex;
    const char *name = menu_car_spec()->getColorName(colorIndex);
    if (name[0] == '-') {
        strcpy(buf, " ");
        return;
    }
    strcpy(buf, name);
}

/* "color_name_box": the name of the colour chosen in the colour box (the
 * same NULL quirk). */
// FUNC 0x0011EA90 text_color_name_box
void text_color_name_box(GtmodeControl *ctl, char *buf, int size)
{
    int color = getGameStatus()->getMenuStatus()->color;
    const char *name = menu_car_spec()->getColorName(color);
    if (name[0] == '-') {
        strcpy(buf, " ");
        return;
    }
    strcpy(buf, name);
}

/* "scroll_bar" (p8 of create_scroll_bar): the current list. */
// FUNC 0x0011EB18 scroll_bar_list
ButtonList *scroll_bar_list(GtmodeControl *ctl)
{
    return ctl->getButtonList();
}

/* "focused_partsname": the full name of the focused part (MenuStatus::parts). */
// FUNC 0x0011EB30 text_focused_partsname
void text_focused_partsname(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, CarEquipments::getPartFullName(getGameStatus()->getMenuStatus()->parts));
}

/* "focused_wheel_name": a placeholder, EUC-JP "ほげほげホイール" ("hoge-hoge
 * wheel"), given to sprintf as the format (0x00310788). */
// FUNC 0x0011EB78 text_focused_wheel_name
void text_focused_wheel_name(GtmodeControl *ctl, char *buf, int size)
{
    sprintf(buf, "\xA4\xDB\xA4\xB2\xA4\xDB\xA4\xB2\xA5\xDB\xA5\xA4\xA1\xBC\xA5\xEB");
}

/* ------------------------------------------------------------------------ */
/* "price_<part>": the tuning shop                                          */
/* ------------------------------------------------------------------------ */
/* Each body is T036_PRICE_TEXT (T036_garage.h): 52 texts,
 * 0x0011EBA0..0x00120DC0 (the work units u010 / u011 meet after
 * "price_nat_2"). */

/* "price_muf_sports". */
// FUNC 0x0011EBA0 text_price_muf_sports
void text_price_muf_sports(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_MUFFLER_SPORTS);
}

/* "price_muf_semi_racing". */
// FUNC 0x0011EC48 text_price_muf_semi_racing
void text_price_muf_semi_racing(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_MUFFLER_SEMI_RACING);
}

/* "price_muf_racing". */
// FUNC 0x0011ECF0 text_price_muf_racing
void text_price_muf_racing(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_MUFFLER_RACING);
}

/* "price_cpu_on": the tuned ROM. */
// FUNC 0x0011ED98 text_price_cpu_on
void text_price_cpu_on(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_COMPUTER_ROM);
}

/* "price_ptp_on": the port polish. */
// FUNC 0x0011EE40 text_price_ptp_on
void text_price_ptp_on(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_PORT_POLISH);
}

/* "price_egb_on": the engine balance tune. */
// FUNC 0x0011EEE8 text_price_egb_on
void text_price_egb_on(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_ENGINE_BALANCE);
}

/* "price_nat_1": NA tune stage 1. */
// FUNC 0x0011EF90 text_price_nat_1
void text_price_nat_1(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_NA_TUNE_STAGE1);
}

/* "price_nat_2": NA tune stage 2. */
// FUNC 0x0011F038 text_price_nat_2
void text_price_nat_2(GtmodeControl *ctl, char *buf, int size)
{
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
    T036_PRICE_TEXT(buf, PART_NA_TUNE_STAGE2);
}

/* "price_nat_3": NA tune stage 3. */
// FUNC 0x0011F0E0 text_price_nat_3
void text_price_nat_3(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_NA_TUNE_STAGE3);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_dsp_on": the displacement up. */
// FUNC 0x0011F188 text_price_dsp_on
void text_price_dsp_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_DISPLACEMENT_UP);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_tbk_1" .. "price_tbk_4": turbo kit stages 1 .. 4. */
// FUNC 0x0011F230 text_price_tbk_1
void text_price_tbk_1(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_TURBINE_STAGE1);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F2D8 text_price_tbk_2
void text_price_tbk_2(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_TURBINE_STAGE2);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F380 text_price_tbk_3
void text_price_tbk_3(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_TURBINE_STAGE3);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F428 text_price_tbk_4
void text_price_tbk_4(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_TURBINE_STAGE4);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_icl_sports" / "price_icl_racing_big": the intercoolers. */
// FUNC 0x0011F4D0 text_price_icl_sports
void text_price_icl_sports(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_INTERCOOLER_SPORTS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F578 text_price_icl_racing_big
void text_price_icl_racing_big(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_INTERCOOLER_RACING);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_sus_sports" / "_semi_racing" / "_racing": the suspensions
 * ("racing" is the full custom kit). */
// FUNC 0x0011F620 text_price_sus_sports
void text_price_sus_sports(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_SUSPENSION_SPORTS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F6C8 text_price_sus_semi_racing
void text_price_sus_semi_racing(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_SUSPENSION_SEMI_RACING);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011F770 text_price_sus_racing
void text_price_sus_racing(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_SUSPENSION_FULL_CUSTOM);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_bkt_racing": the racing brake kit. */
// FUNC 0x0011F818 text_price_bkt_racing
void text_price_bkt_racing(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_BRAKE_RACING);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_bbl_on": the brake balance controller. */
// FUNC 0x0011F8C0 text_price_bbl_on
void text_price_bbl_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_BRAKE_CONTROLLER);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_tms_close" / "_super_close" / "_variable": the transmissions
 * ("variable" is the full custom gearbox). */
// FUNC 0x0011F968 text_price_tms_close
void text_price_tms_close(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_GEAR_CLOSE);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FA10 text_price_tms_super_close
void text_price_tms_super_close(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_GEAR_SUPER_CLOSE);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FAB8 text_price_tms_variable
void text_price_tms_variable(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_GEAR_FULL_CUSTOM);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_clt_high" / "_twin" / "_triple": the clutches (sports, twin
 * plate, triple plate). */
// FUNC 0x0011FB60 text_price_clt_high
void text_price_clt_high(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_CLUTCH_SPORTS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FC08 text_price_clt_twin
void text_price_clt_twin(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_CLUTCH_TWIN);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FCB0 text_price_clt_triple
void text_price_clt_triple(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_CLUTCH_TRIPLE);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_fly_light" / "_cr_mo" / "_light_cr_mo": the flywheels (sports,
 * semi-racing, racing). */
// FUNC 0x0011FD58 text_price_fly_light
void text_price_fly_light(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FLYWHEEL_SPORTS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FE00 text_price_fly_cr_mo
void text_price_fly_cr_mo(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FLYWHEEL_SEMI_RACING);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x0011FEA8 text_price_fly_light_cr_mo
void text_price_fly_light_cr_mo(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FLYWHEEL_RACING);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_prp_on": the carbon propeller shaft. */
// FUNC 0x0011FF50 text_price_prp_on
void text_price_prp_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_PROPELLER_SHAFT_CARBON);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_lsd_1" / "_15" / "_2" / "_variable": the limited-slip
 * differentials (1-way, 1.5-way, 2-way, full custom; the part ids go 1, 2,
 * 1.5). */
// FUNC 0x0011FFF8 text_price_lsd_1
void text_price_lsd_1(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LSD_1WAY);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001200A0 text_price_lsd_15
void text_price_lsd_15(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LSD_1_5WAY);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120148 text_price_lsd_2
void text_price_lsd_2(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LSD_2WAY);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001201F0 text_price_lsd_variable
void text_price_lsd_variable(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LSD_FULL_CUSTOM);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_tir_*": the tyres, by the front tyre's part id (T483's addPart of
 * 0x38..0x41 also owns the rear ids 0x43..0x4C). */
// FUNC 0x00120298 text_price_tir_control
void text_price_tir_control(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_CONTROL);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120340 text_price_tir_dirt
void text_price_tir_dirt(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE_DIRT);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001203E8 text_price_tir_sports
void text_price_tir_sports(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_SPORTS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120490 text_price_tir_super_hard
void text_price_tir_super_hard(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_SH);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120538 text_price_tir_hard
void text_price_tir_hard(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_H);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001205E0 text_price_tir_medium_hard
void text_price_tir_medium_hard(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_MH);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120688 text_price_tir_medium
void text_price_tir_medium(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_M);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120730 text_price_tir_medium_soft
void text_price_tir_medium_soft(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_MS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001207D8 text_price_tir_soft
void text_price_tir_soft(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_S);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120880 text_price_tir_super_soft
void text_price_tir_super_soft(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_FRONT_TIRE + TIRE_KIND_RACING_SS);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_wtr_1" .. "price_wtr_3": weight reduction stages 1 .. 3. */
// FUNC 0x00120928 text_price_wtr_1
void text_price_wtr_1(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LIGHTWEIGHT_STAGE1);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x001209D0 text_price_wtr_2
void text_price_wtr_2(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LIGHTWEIGHT_STAGE2);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

// FUNC 0x00120A78 text_price_wtr_3
void text_price_wtr_3(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LIGHTWEIGHT_STAGE3);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_ayc_on": the active yaw control. */
// FUNC 0x00120B20 text_price_ayc_on
void text_price_ayc_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_LSD_AYC);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_tcs_on": the traction control. */
// FUNC 0x00120BC8 text_price_tcs_on
void text_price_tcs_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_TCSC);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_sms_on": the stability management (ASCC). */
// FUNC 0x00120C70 text_price_sms_on
void text_price_sms_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_ASCC);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* "price_vcd_on": the variable centre differential. */
// FUNC 0x00120D18 text_price_vcd_on
void text_price_vcd_on(GtmodeControl *ctl, char *buf, int size)
{
    T036_PRICE_TEXT(buf, PART_DRIVETRAIN_VCD);
    /* implicit: getGameStatus, getRunningCar, hasPart, getPartPrice, format_number, strcpy (T036_PRICE_TEXT) */
}

/* ------------------------------------------------------------------------ */
/* "price_<service>": the pit services, priced by name in the product      */
/* database (ProductConfig::servicePrice, x100 for COUNTRY_JP)              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00120DC0 text_price_oil_change
void text_price_oil_change(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, product_config()->servicePrice("oil_change"));
}

// FUNC 0x00120E00 text_price_car_wash
void text_price_car_wash(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, product_config()->servicePrice("car_wash"));
}

// FUNC 0x00120E40 text_price_wheel
void text_price_wheel(GtmodeControl *ctl, char *buf, int size)
{
    format_number(buf, product_config()->servicePrice("wheel"));
}

/* ------------------------------------------------------------------------ */
/* The event list and regulation pages                                      */
/* ------------------------------------------------------------------------ */

/* "race_item": the hidden item named after MenuStatus::event (the current
 * race), as an Item with its texture; NULL if there is none. */
// FUNC 0x00120E80 make_race_item
Transitive *make_race_item(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                           int p8, int pC, int p10, int p14)
{
    MenuLayoutItem *race = layout->findHiddenItem(getGameStatus()->menu.event);
    if (race == NULL)
        return NULL;
    MenuTexture *tex = layout->getTexture(race);    /* before Object::operator new */
    return new Item(race, tex);             /* Object::operator new (0x100); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
}

/* Whether an event is open: GameStatus::isLicenseSPassed() (1 once licence
 * 4, S, is passed, else 0) is at least the event's requirement in the
 * product database (ProductConfig::eventValue).  EventHider::isOpen of
 * make_event_hide; make_event_flyer / make_event_text call it. */
// FUNC 0x00120F30 event_unlocked
int event_unlocked(const char *event)
{
    int level = getGameStatus()->isLicenseSPassed();
    return level >= product_config()->eventValue(event);
}

/* "event_hide": an EventHider over the current list, which covers its
 * closed events with the item's picture; NULL without a list. */
// FUNC 0x00120F80 make_event_hide
Transitive *make_event_hide(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list == NULL)
        return NULL;
    MenuTexture *tex = layout->getTexture(item);    /* before Object::operator new */
    /* implicit: Object::operator delete (if the constructor throws) */
    return new EventHider(item, tex, list, event_unlocked);    /* Object::operator new (0x108);
                                                                   operator delete */
}

/* "event_flyer": the item, or the hidden "hide_plate" while the focused
 * event is closed. */
// FUNC 0x00121038 make_event_flyer
Transitive *make_event_flyer(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        ButtonData *focused = list->focusData();
        if (focused != NULL && event_unlocked(focused->name) == 0)
            return new Item("hide_plate", layout);  /* Object::operator new (0x100); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    }
    MenuTexture *tex = layout->getTexture(item);    /* before Object::operator new */
    return new Item(item, tex); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "event_text": the same with "hide_text". */
// FUNC 0x00121130 make_event_text
Transitive *make_event_text(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        ButtonData *focused = list->focusData();
        if (focused != NULL && event_unlocked(focused->name) == 0)
            return new Item("hide_text", layout);   /* Object::operator new (0x100); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    }
    MenuTexture *tex = layout->getTexture(item);    /* before Object::operator new */
    return new Item(item, tex); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "event_status": the result marks of the focused event and the ones after
 * it; NULL without a list or a focused record. */
// FUNC 0x00121228 make_event_status
Transitive *make_event_status(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                              int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        ButtonData *focused = list->focusData();
        if (focused != NULL)
            /* implicit: Object::operator delete (if the constructor throws) */
            return new EventStatusObject(item, focused->name, layout);  /* Object::operator new (0x134);
                                                                           operator delete */
    }
    return NULL;    /* one `return NULL' after the landing pad's __throw, as retail */
}

/* "event_status_button01" .. "11" (cell 1 .. 11): the status of the button
 * in cell `cell' of the current list (cells count from 0 = the head, which
 * no entry uses); NULL without a list or such a cell. */
// FUNC 0x001212E0 make_event_status_button
Transitive *make_event_status_button(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int cell, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        /* the link is the Button's Cell<Button> base (+0x100): NULL stays NULL */
        Button *button = static_cast<Button *>(static_cast<Cell<Button> *>(list->at(cell)));
        if (button != NULL)
            /* implicit: Object::operator delete (if the constructor throws) */
            return new EventStatusButtonObject(item, button, layout);  /* Object::operator new (0xD0);
                                                                          operator delete */
    }
    return NULL;
}

/* "arrow_dec" (dec 1) / "arrow_inc" (0): a scroll arrow of the current
 * list; NULL without a list. */
// FUNC 0x001213A0 make_arrow
Transitive *make_arrow(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                       int dec, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL)
        return new Arrow(dec, list, item, layout);  /* Object::operator new (0x108); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    return NULL;
}

/* "event_course_doc": the hidden picture "<course>_doc", <course> the name
 * of the focused event's course (RaceDataBase::F_002389C8, the course
 * record's name0A), placed on the item; NULL without a focused event or for
 * an unknown race.  RaceDataBase::course (+0x20) is read directly. */
// FUNC 0x00121450 make_event_course_doc
Transitive *make_event_course_doc(void *ctx, PageLayout *layout, Object *parent,
                                  MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        ButtonData *focused = list->focusData();
        if (focused != NULL) {
            RaceDataBase db;                    /* sp+0x00 */
            if (db.select(nameToCode(focused->name)) != 0) {
                /* the course name first: retail calls F_002389C8 before it
                 * computes name and the format (an argument call is not
                 * expanded first here) */
                const char *course = db.F_002389C8(db.course);
                /* sp+0x30: 0x40 bytes up to the register save area at
                 * sp+0x70, the size of a layout item's name
                 * (MenuLayoutItem::name) */
                char name[0x40];
                sprintf(name, "%s_doc", course);
                /* implicit: Object::operator delete (if the constructor throws) */
                return new Item(name, layout, item);    /* Object::operator new (0x100);
                                                           operator delete; then ~RaceDataBase */
            }
        }                                       /* ~RaceDataBase */
    }
    return NULL;
}

/* "event_course_logo": the hidden picture "<course>" itself. */
// FUNC 0x00121598 make_event_course_logo
Transitive *make_event_course_logo(void *ctx, PageLayout *layout, Object *parent,
                                   MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ButtonList *list = ((GtmodeControl *)ctx)->getButtonList();
    if (list != NULL) {
        ButtonData *focused = list->focusData();
        if (focused != NULL) {
            RaceDataBase db;                    /* sp+0x00 */
            if (db.select(nameToCode(focused->name)) != 0) {
                const char *course = db.F_002389C8(db.course);  /* before Object::operator new */
                /* implicit: Object::operator delete (if the constructor throws) */
                return new Item(course, layout, item);  /* Object::operator new (0x100);
                                                           operator delete; then ~RaceDataBase */
            }
        }                                       /* ~RaceDataBase */
    }
    return NULL;
}

/* "regulation_license": the icon of the licence MenuStatus::event needs
 * (RaceEvent::needLicense), placed on the item, its text red when the
 * player has not passed that licence; NULL for an unknown race. */
// FUNC 0x001216D0 make_regulation_license
Transitive *make_regulation_license(void *ctx, PageLayout *layout, Object *parent,
                                    MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    const char *race = getGameStatus()->menu.event;
    RaceDataBase db;                        /* sp+0x00 */
    if (db.select(nameToCode(race)) == 0)
        return NULL;                        /* ~RaceDataBase */
    u8 need = db.event->needLicense;
    const char *icons[7] = {                /* sp+0x30, copied from the template at 0x003107F0 */
        "license_unlimited", "license_b", "license_a", "license_ib",
        "license_ia", "license_s", "license_dirt",
    };
    /* read before Object::operator new, and before the range check of the
     * switch (garbage for need >= 7) */
    const char *iconName = icons[need]; /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(iconName, layout, item);  /* Object::operator new (0x100); operator delete */
    int passed = 0;
    switch (need) {                         /* jump table 0x00310810 */
    case 0:                                 /* none */
        passed = 1;
        break;
    case 1:
        passed = getGameStatus()->licences.isPassed(LICENSE_B, 0);
        break;
    case 2:
        passed = getGameStatus()->licences.isPassed(LICENSE_A, 0);
        break;
    case 3:
        passed = getGameStatus()->licences.isPassed(LICENSE_IB, 0);
        break;
    case 4:
        passed = getGameStatus()->licences.isPassed(LICENSE_IA, 0);
        break;
    case 5:
        passed = getGameStatus()->licences.isPassed(LICENSE_S, 0);
        break;
    case 6:                                 /* the rally licence ("license_dirt") */
        passed = getGameStatus()->licences.isPassed(LICENSE_R, 0);
        break;
    }
    if (passed == 0)
        icon->textColor = REGULATION_FAIL_COLOR;    /* Item::setColor, expanded */
    return icon;                            /* after ~RaceDataBase */
}

/* ------------------------------------------------------------------------ */
/* "regulation_*": the regulation icons of the current race                 */
/* ------------------------------------------------------------------------ */
/* Each one selects MenuStatus::event in a RaceDataBase on the stack (sp+0x00;
 * select's result is not checked: for an unknown race `event' stays NULL, as
 * clear() left it, and the rule is read through NULL), makes the icon of the
 * race's rule placed on the item (Item(name, layout, pos): the hidden item
 * `name', centred on item), and colours it REGULATION_FAIL_COLOR
 * (Item::textColor: what Item::setColor sets, expanded as one store) when
 * the ridden car breaks the rule or no car is ridden.  The icon name tables
 * are copied to the stack from .rodata templates; the rule byte indexes them
 * without a range check.  The rule is read before the table is copied, the
 * icon's name before operator new (retail loads it into the call's delay
 * slot; `new Item(icons[rule], ...)' reads it after the call).  The
 * RaceDataBase is destroyed (~RaceDataBase, flags 2) on every way out. */

/* "regulation_power": always the "power_unlimited" icon (no power rule is
 * checked); red only without a ridden car. */
// FUNC 0x001218F0 make_regulation_power
Transitive *make_regulation_power(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                  int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    char unused[0x80];                      /* ? never touched: the frame (0xF0) has 0x80 bytes at
                                               sp+0x30, where the other hooks keep their icon table;
                                               egcs gives 0x70 without it.  A leftover */
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item("power_unlimited", layout, item);    /* Object::operator new (0x100);
                                                                   operator delete */
    if (getGameStatus()->getRunningCar() == NULL)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "regulation_tuning": needTuning 0 any, 1 racing (carType != 0), 2 tuned
 * (CarEquipments::isTuned), 3 normal (neither). */
// FUNC 0x001219F0 make_regulation_tuning
Transitive *make_regulation_tuning(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                   int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    u8 rule = db.event->needTuning;
    const char *icons[4] = {                /* sp+0x30, copied from 0x00310888 */
        "tuning_unlimited", "tuning_racing", "tuning_tuned", "tuning_normal",
    };
    const char *name = icons[rule];
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(name, layout, item);  /* Object::operator new; operator delete */
    int ok = 0;
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden != NULL) {
        /* `if (...) ok = 1': egcs turns each into a movz / movn, as in
         * the listing */
        switch (rule) {
        case NEED_TUNING_ANY:
            ok = 1;
            break;
        case NEED_TUNING_RACING:
            if (ridden->carType != 0)
                ok = 1;
            break;
        case NEED_TUNING_TUNED:
            if (ridden->isTuned() != 0)
                ok = 1;
            break;
        case NEED_TUNING_NORMAL: {
            /* the CarEquipments base (+0x10) taken once, before carType's
             * load (retail: addiu a0,a0,0x10 in the case's branch slot,
             * lhu 0x10(a0), then isTuned with the same a0) */
            CarEquipments *eq = ridden;
            if (eq->carType == 0 && eq->isTuned() == 0)
                ok = 1;
            break;
        }
        }
    }
    if (!ok)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "regulation_dr_type": the drive train rule (jump table 0x00310910, 6
 * cases); the rule's numbers (1 FF, 2 FR, ...) are not the drive train codes
 * of CarEquipments::getDriveTrain (0 FR, 1 FF, ...). */
// FUNC 0x00121BB8 make_regulation_dr_type
Transitive *make_regulation_dr_type(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                    int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    u8 rule = db.event->needDrivetrain;
    const char *icons[6] = {                /* sp+0x30, copied from 0x003108F8 */
        "drive_unlimited", "drive_ff", "drive_fr", "drive_4wd", "drive_mr", "drive_rr",
    };
    const char *name = icons[rule];
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(name, layout, item);  /* Object::operator new; operator delete */
    int ok = 0;
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden != NULL) {
        switch (rule) {
        case NEED_DRIVETRAIN_ANY:
            ok = 1;
            break;
        case NEED_DRIVETRAIN_FF:
            ok = ridden->getDriveTrain() == LAYOUT_FF;
            break;
        case NEED_DRIVETRAIN_FR:
            ok = ridden->getDriveTrain() == LAYOUT_FR;
            break;
        case NEED_DRIVETRAIN_4WD:
            ok = ridden->getDriveTrain() == LAYOUT_4WD;
            break;
        case NEED_DRIVETRAIN_MR:
            ok = ridden->getDriveTrain() == LAYOUT_MR;
            break;
        case NEED_DRIVETRAIN_RR:
            ok = ridden->getDriveTrain() == LAYOUT_RR;
            break;
        }
    }
    if (!ok)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "regulation_aspiration": needAspiration 0 any, 1 NA, 2 turbo
 * (CarEquipments::isTurbo). */
// FUNC 0x00121D98 make_regulation_aspiration
Transitive *make_regulation_aspiration(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    u8 rule = db.event->needAspiration;
    const char *icons[3] = {                /* sp+0x30, copied from 0x00310968 */
        "aspiration_unlimited", "aspiration_na", "aspiration_turbo",
    };
    const char *name = icons[rule];
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(name, layout, item);  /* Object::operator new; operator delete */
    int ok = 0;
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden != NULL) {
        switch (rule) {
        case NEED_ASPIRATION_ANY:
            ok = 1;
            break;
        case NEED_ASPIRATION_NA:
            if (ridden->isTurbo() == 0)
                ok = 1;
            break;
        case NEED_ASPIRATION_TURBO:
            if (ridden->isTurbo() != 0)
                ok = 1;
            break;
        }
    }
    if (!ok)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "regulation_tire": the icon of needTire; the check is the whole
 * check_regulation (T489) of the ridden car, of which only flags[0] (the
 * tyres) is used: red when it is 1 (`ok' is flags[0] ^ 1, an xori; nothing
 * else is ever stored there).  The result (sp+0x60) is destroyed inline:
 * only its vptr is stored (_vt$16RegurationResult 0x00310310, T045's copy). */
// FUNC 0x00121F38 make_regulation_tire
Transitive *make_regulation_tire(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                 int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    u8 rule = db.event->needTire;
    const char *icons[10] = {               /* sp+0x30, copied from 0x00310988 */
        "tire_0", "tire_1", "tire_2", "tire_3", "tire_4",
        "tire_5", "tire_6", "tire_7", "tire_8", "tire_dirt",
    };
    const char *name = icons[rule];
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(name, layout, item);  /* Object::operator new; operator delete */
    int ok = 0;
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden != NULL) {
        /* the CarInformation base before nameToCode (retail: addiu s0,v0,0x10
         * after the NULL test), the race code before the result's address */
        CarInformation *info = ridden;
        u64 code = nameToCode(event);
        RegurationResult result = check_regulation(code, info);
        ok = result.flags[0];               /* loaded into ok's register, then xori */
        ok ^= 1;
    }
    if (!ok)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "regulation_car": "car_limited" when the race has a regulated car list
 * (red unless the ridden car's code is one of them), else "car_unlimited"
 * (red only without a ridden car). */
// FUNC 0x001220E8 make_regulation_car
Transitive *make_regulation_car(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    int count = db.getNumRegulatedCars();
    const char *icons[2] = {                /* sp+0x30, copied from 0x00351218 (.sdata) */
        "car_unlimited", "car_limited",
    };
    const char *name = icons[count != 0];
    /* implicit: Object::operator delete (if the constructor throws) */
    Item *icon = new Item(name, layout, item);  /* Object::operator new; operator delete */
    int ok = 0;
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden != NULL) {
        u64 code = ridden->carCode;
        ok = 1;
        if (count != 0) {
            int i;
            for (i = 0; i < count; i++)
                if (code == db.getRegulatedCar(i))
                    break;
            ok = i < count;
        }
    }
    if (!ok)
        icon->textColor = REGULATION_FAIL_COLOR;
    return icon;
}

/* "violated_license": the hidden item named after the licence the race
 * needs (needLicense; "error" for none, "DIRT" for 6, the rally licence),
 * placed on the item; never red. */
// FUNC 0x00122260 make_violated_license
Transitive *make_violated_license(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                  int p8, int pC, int p10, int p14)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    u8 rule = db.event->needLicense;
    const char *icons[7] = {                /* sp+0x30, copied from 0x003109D0 */
        "error", "B", "A", "IB", "IA", "S", "DIRT",
    };
    const char *name = icons[rule];
    /* implicit: Object::operator delete (if the constructor throws) */
    return new Item(name, layout, item);    /* Object::operator new; operator delete */
}

/* ------------------------------------------------------------------------ */
/* "spec_*": the car spec sheet (CallbackText callbacks)                    */
/* ------------------------------------------------------------------------ */
/* Most read the catalogue data of MenuStatus::car (its CarEquipments, the
 * GarageCar's CarInformation base at +0x10; no NULL check); the ratios, the
 * treads, the weight and "spec_max_torque_real" read the car on show,
 * menu_car_spec() (T232): its ShowRoomCar `car' (T278).  The MenuStatus is
 * GameStatus::getMenuStatus() (retail adds its offset 0x307C8, then loads
 * car at +0x538).  The sprintf texts take the car into a local first: the
 * format's address is then computed after the car's load and before the
 * getters (in an s-register across them), as retail has it. */

/* "spec_aspiration" */
// FUNC 0x001223A8 text_spec_aspiration
void text_spec_aspiration(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, getGameStatus()->getMenuStatus()->car->getAspirationText());
}

/* "spec_displacement" and "spec_displacement_real" */
// FUNC 0x001223F0 text_spec_displacement
void text_spec_displacement(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%s", car->getDisplacementText());
}

/* "spec_drive_type": "FR", "FF", "4WD", "MR" or "RR". */
// FUNC 0x00122450 text_spec_drive_type
void text_spec_drive_type(GtmodeControl *ctl, char *buf, int size)
{
    strcpy(buf, getGameStatus()->getMenuStatus()->car->getDriveTrainText());
}

/* "spec_engine_type": the two words of the engine type. */
// FUNC 0x00122498 text_spec_engine_type
void text_spec_engine_type(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%s %s", car->getEngineTypeText1(), car->getEngineTypeText2());
}

/* "spec_height" */
// FUNC 0x00122518 text_spec_height
void text_spec_height(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%d mm", car->getHeight());
}

/* "spec_length" */
// FUNC 0x00122578 text_spec_length
void text_spec_length(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%d mm", car->getLength());
}

/* "spec_max_power": "<power><unit>/<rpm text>"; " " when the catalogue has
 * no power (0).  The getter is called again for the text.  (" " is the one
 * literal 0x003511B8 that text_car_color_name uses first.) */
// FUNC 0x001225D8 text_spec_max_power
void text_spec_max_power(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    if (car->getMaxPower() == 0) {
        strcpy(buf, " ");                   /* expanded as a 2-byte copy */
        return;
    }
    sprintf(buf, "%d%s/%s", car->getMaxPower(), g_002F59D8.powerName(), car->getMaxPowerRpmText());
}

/* "spec_max_power_real": the power of the car as it is
 * (GarageCar::getPower) and its rpm. */
// FUNC 0x001226A0 text_spec_max_power_real
void text_spec_max_power_real(GtmodeControl *ctl, char *buf, int size)
{
    int power;                              /* sp+0x0 */
    int rpm;                                /* sp+0x4 */

    getGameStatus()->getMenuStatus()->car->getPower(&power, &rpm);
    sprintf(buf, "%d%s/%drpm", power, g_002F59D8.powerName(), rpm);
}

/* "spec_max_torque": "<torque, 2 decimals><unit>/<rpm text>"; " " when the
 * catalogue has no torque (0). */
// FUNC 0x00122720 text_spec_max_torque
void text_spec_max_torque(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    if (car->getMaxTorque() == 0) {
        strcpy(buf, " ");                   /* expanded as a 2-byte copy */
        return;
    }
    fmt_sprintf(buf, "%.2t%s/%s", car->getMaxTorque(), g_002F59D8.torqueName(),
                car->getMaxTorqueRpmText());
}

/* "spec_max_torque_real": the computed torque of the car on show (in
 * tenths) and its rpm; the code of text_car_max_torque. */
// FUNC 0x001227E8 text_spec_max_torque_real
void text_spec_max_torque_real(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0 */

    int tenths = menu_car_spec()->car.torque(&rpm, UNIT_TAG_DKGM, 0);
    /* implicit: fptodp (float to double conversion) */
    /* (tenths * 0.1f: a float multiply, then fptodp, 0x0027AFA0, the
     * soft-float __extendsfdf2, before torqueName: the format is loaded
     * first, the argument conversion is expanded in order; the form of
     * text_car_max_torque.) */
    sprintf(buf, "%.1f%s/%drpm", tenths * 0.1f, g_002F59D8.torqueName(), rpm);
}

/* "spec_p_w_ratio": weight / catalogue power, 3 decimals (kg per power
 * unit); the float is truncated (cvt.w.s). */
// FUNC 0x00122878 text_spec_p_w_ratio
void text_spec_p_w_ratio(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0, not used */

    int power = menu_car_spec()->car.power(&rpm, UNIT_TAG_PS, 1);
    s16 weight = menu_car_spec()->car.spec.weight;
    fmt_sprintf(buf, "%.3t", (int)((f32)weight / (f32)power * 1000.0f));
}

/* "spec_p_w_ratio_real": the same with the computed power. */
// FUNC 0x00122908 text_spec_p_w_ratio_real
void text_spec_p_w_ratio_real(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0, not used */

    int power = menu_car_spec()->car.power(&rpm, UNIT_TAG_PS, 0);
    s16 weight = menu_car_spec()->car.spec.weight;
    fmt_sprintf(buf, "%.3t", (int)((f32)weight / (f32)power * 1000.0f));
}

/* "spec_t_w_ratio": weight / catalogue torque (tenths) * 0.1, 3 decimals:
 * weight / (10 x torque) as written (? the 0.1 was probably meant to undo
 * the tenths the other way round: weight / torque * 10). */
// FUNC 0x00122998 text_spec_t_w_ratio
void text_spec_t_w_ratio(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0, not used */

    int tenths = menu_car_spec()->car.torque(&rpm, UNIT_TAG_DKGM, 1);
    s16 weight = menu_car_spec()->car.spec.weight;
    fmt_sprintf(buf, "%.3t", (int)((f32)weight / (f32)tenths * 0.1f * 1000.0f));
}

/* "spec_t_w_ratio_real": the same with the computed torque. */
// FUNC 0x00122A30 text_spec_t_w_ratio_real
void text_spec_t_w_ratio_real(GtmodeControl *ctl, char *buf, int size)
{
    int rpm;                                /* sp+0x0, not used */

    int tenths = menu_car_spec()->car.torque(&rpm, UNIT_TAG_DKGM, 0);
    s16 weight = menu_car_spec()->car.spec.weight;
    fmt_sprintf(buf, "%.3t", (int)((f32)weight / (f32)tenths * 0.1f * 1000.0f));
}

/* "spec_tread_f", "spec_tread_r", "spec_weight": the car on show is fetched
 * before the sprintf (the format is loaded after the call, unlike the
 * catalogue texts', where the getter call is inside the arguments). */
// FUNC 0x00122AC8 text_spec_tread_f
void text_spec_tread_f(GtmodeControl *ctl, char *buf, int size)
{
    MenuCarSpec *spec = menu_car_spec();
    sprintf(buf, "%d mm", spec->car.spec.treadFront);
}

// FUNC 0x00122B00 text_spec_tread_r
void text_spec_tread_r(GtmodeControl *ctl, char *buf, int size)
{
    MenuCarSpec *spec = menu_car_spec();
    sprintf(buf, "%d mm", spec->car.spec.treadRear);
}

// FUNC 0x00122B38 text_spec_weight
void text_spec_weight(GtmodeControl *ctl, char *buf, int size)
{
    MenuCarSpec *spec = menu_car_spec();
    sprintf(buf, "%d kg", spec->car.spec.weight);
}

/* "spec_wheel_base" */
// FUNC 0x00122B70 text_spec_wheel_base
void text_spec_wheel_base(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%d mm", car->getWheelBase());
}

/* "spec_width" */
// FUNC 0x00122BD0 text_spec_width
void text_spec_width(GtmodeControl *ctl, char *buf, int size)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    sprintf(buf, "%d mm", car->getWidth());
}

/* ------------------------------------------------------------------------ */
/* "race_prize_1" .. "race_prize_6" (CallbackText callbacks)                */
/* ------------------------------------------------------------------------ */
/* The prize money of place 1..6 of MenuStatus::event, with thousands
 * separators (RaceDataBase at sp+0x00, ~RaceDataBase with flags 2; select's
 * result is not checked).  Six copies of one body, the place the only
 * difference. */

// FUNC 0x00122C30 text_race_prize_1
void text_race_prize_1(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(0));
}

// FUNC 0x00122CD8 text_race_prize_2
void text_race_prize_2(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(1));
}

// FUNC 0x00122D80 text_race_prize_3
void text_race_prize_3(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(2));
}

// FUNC 0x00122E28 text_race_prize_4
void text_race_prize_4(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(3));
}

// FUNC 0x00122ED0 text_race_prize_5
void text_race_prize_5(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(4));
}

// FUNC 0x00122F78 text_race_prize_6
void text_race_prize_6(GtmodeControl *ctl, char *buf, int size)
{
    const char *event = getGameStatus()->getMenuStatus()->event;
    RaceDataBase db;
    db.select(nameToCode(event));
    format_number(buf, db.getPrize(5));
}

/* ------------------------------------------------------------------------ */
/* Other hooks                                                              */
/* ------------------------------------------------------------------------ */

/* "ignore_control": no object; freezes the page's input
 * (PageManager::setFreeze(1)); the OilChangeCylinder (T040) unfreezes it
 * when its refill is over. */
// FUNC 0x00123020 make_ignore_control
Transitive *make_ignore_control(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                                int p8, int pC, int p10, int p14)
{
    GtmodeControl *ctl = static_cast<GtmodeControl *>(ctx);
    ctl->pm.setFreeze(1);
    return NULL;
}

/* "oil_change_cylinder": the oil change animation on the item, as dirty as
 * the oil of MenuStatus::car (CarCondition::oilDirtiness of its
 * CarInformation, +0x204 of the GarageCar); also kept in ctl->oilCylinder
 * for "oil_change" to start (T037).  The constructor's result ($v0, this)
 * is both stored and returned; returned from a local (retail's return block,
 * `b epilogue', sits after the landing pad's __throw: a return at the end,
 * not a `return ctl->oilCylinder = new ...'). */
// FUNC 0x00123048 make_oil_change_cylinder
Transitive *make_oil_change_cylinder(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    GtmodeControl *ctl = static_cast<GtmodeControl *>(ctx);
    PageManager *pages = &ctl->pm;          /* taken first ($s0, in the first call's delay slot) */
    f32 dirt = getGameStatus()->getMenuStatus()->car->condition.oilDirtiness();
    /* implicit: Object::operator delete (if the constructor throws) */
    OilChangeCylinder *cyl = new OilChangeCylinder(pages, dirt, item, layout);   /* Object::operator
                                                                         new (0x124); operator delete */
    ctl->oilCylinder = cyl;
    return cyl;
}

/* DecayItem's fade (game/menu/gtmode/T036_decay_item.h): opFadeAlpha, alphaRatio -1 /
 * 1.5 per second for 1.5 s, then opRelease (the item is killed); result 1
 * ends the frame.  Words: 25, 0, 0, 0, -0.6666667 (0xBF2AAAAB), 1.5, 0 x 5,
 * 1; 6, 0 x 10, 1.  Here, where retail's .rodata has it: after
 * text_spec_max_torque's "%.2t%s/%s", before the hook table's names. */
const OperatorCode g_00310A00[2] = {        /* 0x00310A00 */
    { 25 /* opFadeAlpha */, 0, 0, { 0, 0, 0, 0 }, -0.6666667f, 1.5f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 6 /* opRelease */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

/* "slow_decay": the item as a DecayItem: it leaves by a fade of its own
 * (g_00310A00). */
// FUNC 0x00123110 make_slow_decay
Transitive *make_slow_decay(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14)
{
    /* implicit: Item (base constructor in the inline DecayItem constructor) */
    /* implicit: Object::operator delete (if the constructor throws) */
    return new DecayItem(item, layout);     /* Object::operator new (0x100); the inline constructor:
                                               Item(item, layout), then the vptr 0x00311BC8;
                                               operator delete */
}

/* "tuner_logo": the hidden item named after the brand of the ridden car
 * (CarEquipments::getBrandName), placed on the item; NULL without a ridden
 * car or a brand name. */
// FUNC 0x00123198 make_tuner_logo
Transitive *make_tuner_logo(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                            int p8, int pC, int p10, int p14)
{
    GarageCar *ridden = getGameStatus()->getRunningCar();
    if (ridden == NULL)
        return NULL;
    const char *brand = ridden->getBrandName();
    if (brand == NULL)
        return NULL;
    return new Item(brand, layout, item);   /* Object::operator new (0x100); operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
}

/* "is_ride_car" (p8 of create_bool_item, pC 1: tested once): 1 if a car is
 * ridden. */
// FUNC 0x00123248 is_ride_car
int is_ride_car(GtmodeControl *ctl)
{
    return getGameStatus()->getRunningCar() != NULL;
}

/* ------------------------------------------------------------------------ */
/* The hook table                                                           */
/* ------------------------------------------------------------------------ */
/* One element as egcs's build_vec_init expands it (see T036_static_init):
 * the constructor's stores in member order through a copy of the cursor
 * (`this'), then the cursor and the count of the elements left stepped.  (No
 * do-while wrapper: its loop notes would change the code.)  Not functions of
 * the binary; they only shorten the source. */
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

#define ENTRY(name, create, p8, pC)                                           \
    { MenuHook *e = base; set_entry(e, name, create, p8, pC, 0, 0); }       \
    base++;                                                                   \
    left--

/* An entry of a create function (p10 / p14 are 0 everywhere). */
#define HOOK(name, create, p8, pC)  ENTRY(name, create, p8, pC)

/* An entry of create_callback_text (T239): p8 the text callback, pC 1 when
 * the text is made once. */
#define TEXT(name, text, once)      ENTRY(name, create_callback_text, (int)(GtmodeTextFn *)text, once)

/* The hook table (T036 .data 0x00296B90..0x00298048, 221 entries of 0x18
 * bytes): zero in retail's image, T036_static_init fills it at boot.  No
 * initialiser: egcs writes an uninitialised C++ global as zeros in .data,
 * in front of the unit's frame info, where retail has it. */
MenuHook g_00296B90[221];                   /* 0x00296B90 */

/* The table's entry count (GtmodeControl::begin passes it to setHooks with
 * the table): an .sdata word after the literals of the functions above and
 * before the static initialiser's ("blink", "mpeg"), as retail has it
 * (0x00351278, then "blink" 0x00351280).  The original defined it after the
 * table; egcs writes an initialised variable where it is defined, the
 * initialiser's literals when it compiles the initialiser. */
int g_00351278 = sizeof(g_00296B90) / sizeof(g_00296B90[0]);  /* 0x00351278 */

/* Static initialiser of the hook table g_00296B90 (.ctors entry 0x0028F048).
 * Compiler-generated in the original (_GLOBAL_$I$...): the original defined
 * the table in this unit with MenuHook objects built by a constructor, e.g.
 * MenuHook("blink", create_blink, 0, 0, 0, 0); egcs builds such a table at
 * run time, in this function (cp/init.c build_vec_init), and leaves .data
 * zero.  Written that way (a MenuHook with an inline constructor and the
 * table definition) the function compiles to exactly these instructions
 * (build/scratch/u012/gen.py "ctor").  The decomp's MenuHook (T229) is a
 * plain struct shared by every hook table of the menus (T091 initialises
 * its table as an aggregate), and the table is defined above without an
 * initialiser (zeros, as retail's .data); so the function is written out: an
 * exact form, not the original.  It spells out what build_vec_init expands:
 * the array's address as the result value (`table', dead) and as the cursor,
 * the iterator (`left', dead: flow deletes both), and per element a copy of
 * the cursor; without any one of the three the 1326 stores come out with
 * other registers (43-55% instead of exact; u006's T030_static_init needs the
 * same form without `table').  A plain `p->name = ...; p++' walk stores the
 * same.  All six words of every entry are stored.  The names are sorted
 * (PageLayout::findHook binary-searches them); entry i is at 0x00296B90 +
 * 0x18 * i.  The library hooks (create_blink, create_scroll_bar,
 * create_bool_item, create_color_box, create_callback_text) take fewer or
 * typed parameters: cast to MenuHookFn.
 *
 * p8 of the entries that are not callbacks: make_arrow 1 dec / 0 inc;
 * make_car_render_* the camera preset (Car::setCameraMode); make_license_cup
 * licence << 4 | test - 1 (b 0, a 1, ib 2, ia 3, s 4, r 5);
 * make_event_status_button the cell of the list. */
// FUNC 0x00123270 T036_static_init
void T036_static_init(void)
{
    MenuHook *table = g_00296B90;           /* build_vec_init's result value (dead) */
    MenuHook *base = g_00296B90;            /* its cursor */
    int left = 220;                         /* its iterator: elements - 1 (dead) */

    HOOK("arrow_dec",              make_arrow,                 1, 0);
    HOOK("arrow_inc",              make_arrow,                 0, 0);
    HOOK("blink",                  create_blink, 0, 0);
    TEXT("car_buy_price",          text_car_buy_price,         0);
    HOOK("car_color_box",          create_color_box,
         (int)car_color_box_colors, (int)car_color_box_select);
    TEXT("car_color_name",         text_car_color_name,        0);
    TEXT("car_current_torque",     text_current_power,         0);
    TEXT("car_drive_type",         text_car_drive_type,        0);
    TEXT("car_kilometrage",        text_car_kilometrage,       0);
    HOOK("car_logo",               make_car_logo,              0, 0);
    TEXT("car_max_power",          text_car_max_power,         0);
    TEXT("car_max_torque",         text_car_max_torque,        0);
    TEXT("car_previous_torque",    text_previous_power,        0);
    HOOK("car_render_buy",         make_car_render_buy,        11, 0);
    HOOK("car_render_car_wash",    make_car_render_current,    8, 0);
    HOOK("car_render_from",        make_car_render_focused,    4, 0);
    HOOK("car_render_garage",      make_car_render_focused,    5, 0);
    HOOK("car_render_mine",        make_car_render_current,    5, 0);
    HOOK("car_render_mine_side",   make_car_render_current,    5, 0);
    HOOK("car_render_oil_change",  make_car_render_current,    10, 0);
    HOOK("car_render_shop",        make_car_render_focused,    4, 0);
    HOOK("car_render_wheel",       make_car_render_wheel,      6, 0);
    HOOK("car_render_wheel_shop",  make_car_render_current,    7, 0);
    TEXT("car_sell_price",         text_car_sell_price,        0);
    TEXT("car_weight",             text_car_weight,            0);
    TEXT("color_name_box",         text_color_name_box,        0);
    HOOK("disp_license_cup_a1",    make_license_cup,           0x10, 0);
    HOOK("disp_license_cup_a2",    make_license_cup,           0x11, 0);
    HOOK("disp_license_cup_a3",    make_license_cup,           0x12, 0);
    HOOK("disp_license_cup_a4",    make_license_cup,           0x13, 0);
    HOOK("disp_license_cup_a5",    make_license_cup,           0x14, 0);
    HOOK("disp_license_cup_a6",    make_license_cup,           0x15, 0);
    HOOK("disp_license_cup_a7",    make_license_cup,           0x16, 0);
    HOOK("disp_license_cup_a8",    make_license_cup,           0x17, 0);
    HOOK("disp_license_cup_b1",    make_license_cup,           0x00, 0);
    HOOK("disp_license_cup_b2",    make_license_cup,           0x01, 0);
    HOOK("disp_license_cup_b3",    make_license_cup,           0x02, 0);
    HOOK("disp_license_cup_b4",    make_license_cup,           0x03, 0);
    HOOK("disp_license_cup_b5",    make_license_cup,           0x04, 0);
    HOOK("disp_license_cup_b6",    make_license_cup,           0x05, 0);
    HOOK("disp_license_cup_b7",    make_license_cup,           0x06, 0);
    HOOK("disp_license_cup_b8",    make_license_cup,           0x07, 0);
    HOOK("disp_license_cup_ia1",   make_license_cup,           0x30, 0);
    HOOK("disp_license_cup_ia2",   make_license_cup,           0x31, 0);
    HOOK("disp_license_cup_ia3",   make_license_cup,           0x32, 0);
    HOOK("disp_license_cup_ia4",   make_license_cup,           0x33, 0);
    HOOK("disp_license_cup_ia5",   make_license_cup,           0x34, 0);
    HOOK("disp_license_cup_ia6",   make_license_cup,           0x35, 0);
    HOOK("disp_license_cup_ia7",   make_license_cup,           0x36, 0);
    HOOK("disp_license_cup_ia8",   make_license_cup,           0x37, 0);
    HOOK("disp_license_cup_ib1",   make_license_cup,           0x20, 0);
    HOOK("disp_license_cup_ib2",   make_license_cup,           0x21, 0);
    HOOK("disp_license_cup_ib3",   make_license_cup,           0x22, 0);
    HOOK("disp_license_cup_ib4",   make_license_cup,           0x23, 0);
    HOOK("disp_license_cup_ib5",   make_license_cup,           0x24, 0);
    HOOK("disp_license_cup_ib6",   make_license_cup,           0x25, 0);
    HOOK("disp_license_cup_ib7",   make_license_cup,           0x26, 0);
    HOOK("disp_license_cup_ib8",   make_license_cup,           0x27, 0);
    HOOK("disp_license_cup_r1",    make_license_cup,           0x50, 0);
    HOOK("disp_license_cup_r2",    make_license_cup,           0x51, 0);
    HOOK("disp_license_cup_r3",    make_license_cup,           0x52, 0);
    HOOK("disp_license_cup_r4",    make_license_cup,           0x53, 0);
    HOOK("disp_license_cup_r5",    make_license_cup,           0x54, 0);
    HOOK("disp_license_cup_r6",    make_license_cup,           0x55, 0);
    HOOK("disp_license_cup_r7",    make_license_cup,           0x56, 0);
    HOOK("disp_license_cup_r8",    make_license_cup,           0x57, 0);
    HOOK("disp_license_cup_s1",    make_license_cup,           0x40, 0);
    HOOK("disp_license_cup_s2",    make_license_cup,           0x41, 0);
    HOOK("disp_license_cup_s3",    make_license_cup,           0x42, 0);
    HOOK("disp_license_cup_s4",    make_license_cup,           0x43, 0);
    HOOK("disp_license_cup_s5",    make_license_cup,           0x44, 0);
    HOOK("disp_license_cup_s6",    make_license_cup,           0x45, 0);
    HOOK("disp_license_cup_s7",    make_license_cup,           0x46, 0);
    HOOK("disp_license_cup_s8",    make_license_cup,           0x47, 0);
    TEXT("disp_ps_after",          text_ps_after,              1);
    TEXT("disp_ps_befor",          text_current_power,         0);
    HOOK("disp_time_bronze",       make_time_bronze,           0, 0);
    HOOK("disp_time_gold",         make_time_gold,             0, 0);
    HOOK("disp_time_silver",       make_time_silver,           0, 0);
    HOOK("event_course_doc",       make_event_course_doc,      0, 0);
    HOOK("event_course_logo",      make_event_course_logo,     0, 0);
    HOOK("event_flyer",            make_event_flyer,           0, 0);
    HOOK("event_hide",             make_event_hide,            0, 0);
    HOOK("event_status",           make_event_status,          0, 0);
    HOOK("event_status_button01",  make_event_status_button,   1, 0);
    HOOK("event_status_button02",  make_event_status_button,   2, 0);
    HOOK("event_status_button03",  make_event_status_button,   3, 0);
    HOOK("event_status_button04",  make_event_status_button,   4, 0);
    HOOK("event_status_button05",  make_event_status_button,   5, 0);
    HOOK("event_status_button06",  make_event_status_button,   6, 0);
    HOOK("event_status_button07",  make_event_status_button,   7, 0);
    HOOK("event_status_button08",  make_event_status_button,   8, 0);
    HOOK("event_status_button09",  make_event_status_button,   9, 0);
    HOOK("event_status_button10",  make_event_status_button,   10, 0);
    HOOK("event_status_button11",  make_event_status_button,   11, 0);
    HOOK("event_text",             make_event_text,            0, 0);
    TEXT("focused_carname",        text_focused_carname,       0);
    TEXT("focused_partsname",      text_focused_partsname,     0);
    TEXT("focused_wheel_name",     text_focused_wheel_name,    0);
    HOOK("goods_car_price",        make_goods_car_price,       0, 0);
    HOOK("ignore_control",         make_ignore_control,        0, 0);
    HOOK("is_ride_car",            create_bool_item, (int)is_ride_car, 1);
    HOOK("license_car_name",       make_license_car_name,      0, 0);
    HOOK("license_launch_speed",   make_license_launch_speed,  0, 0);
    HOOK("license_mark",           make_license_mark,          0, 0);
    HOOK("license_mark_rally",     make_license_mark_rally,    0, 0);
    HOOK("mpeg",                   make_mpeg,                  0, 0);
    HOOK("need_license",           make_need_license,          0, 0);
    HOOK("oil_change_cylinder",    make_oil_change_cylinder,   0, 0);
    TEXT("price_ayc_on",           text_price_ayc_on,          0);
    TEXT("price_bbl_on",           text_price_bbl_on,          0);
    TEXT("price_bkt_racing",       text_price_bkt_racing,      0);
    TEXT("price_car_wash",         text_price_car_wash,        0);
    TEXT("price_clt_high",         text_price_clt_high,        0);
    TEXT("price_clt_triple",       text_price_clt_triple,      0);
    TEXT("price_clt_twin",         text_price_clt_twin,        0);
    TEXT("price_cpu_on",           text_price_cpu_on,          0);
    TEXT("price_dsp_on",           text_price_dsp_on,          0);
    TEXT("price_egb_on",           text_price_egb_on,          0);
    TEXT("price_fly_cr_mo",        text_price_fly_cr_mo,       0);
    TEXT("price_fly_light",        text_price_fly_light,       0);
    TEXT("price_fly_light_cr_mo",  text_price_fly_light_cr_mo, 0);
    TEXT("price_icl_racing_big",   text_price_icl_racing_big,  0);
    TEXT("price_icl_sports",       text_price_icl_sports,      0);
    TEXT("price_lsd_1",            text_price_lsd_1,           0);
    TEXT("price_lsd_15",           text_price_lsd_15,          0);
    TEXT("price_lsd_2",            text_price_lsd_2,           0);
    TEXT("price_lsd_variable",     text_price_lsd_variable,    0);
    TEXT("price_muf_racing",       text_price_muf_racing,      0);
    TEXT("price_muf_semi_racing",  text_price_muf_semi_racing, 0);
    TEXT("price_muf_sports",       text_price_muf_sports,      0);
    TEXT("price_nat_1",            text_price_nat_1,           0);
    TEXT("price_nat_2",            text_price_nat_2,           0);
    TEXT("price_nat_3",            text_price_nat_3,           0);
    TEXT("price_oil_change",       text_price_oil_change,      0);
    TEXT("price_prp_on",           text_price_prp_on,          0);
    TEXT("price_ptp_on",           text_price_ptp_on,          0);
    TEXT("price_sms_on",           text_price_sms_on,          0);
    TEXT("price_sus_racing",       text_price_sus_racing,      0);
    TEXT("price_sus_semi_racing",  text_price_sus_semi_racing, 0);
    TEXT("price_sus_sports",       text_price_sus_sports,      0);
    TEXT("price_tbk_1",            text_price_tbk_1,           0);
    TEXT("price_tbk_2",            text_price_tbk_2,           0);
    TEXT("price_tbk_3",            text_price_tbk_3,           0);
    TEXT("price_tbk_4",            text_price_tbk_4,           0);
    TEXT("price_tcs_on",           text_price_tcs_on,          0);
    TEXT("price_tir_control",      text_price_tir_control,     0);
    TEXT("price_tir_dirt",         text_price_tir_dirt,        0);
    TEXT("price_tir_hard",         text_price_tir_hard,        0);
    TEXT("price_tir_medium",       text_price_tir_medium,      0);
    TEXT("price_tir_medium_hard",  text_price_tir_medium_hard, 0);
    TEXT("price_tir_medium_soft",  text_price_tir_medium_soft, 0);
    TEXT("price_tir_soft",         text_price_tir_soft,        0);
    TEXT("price_tir_sports",       text_price_tir_sports,      0);
    TEXT("price_tir_super_hard",   text_price_tir_super_hard,  0);
    TEXT("price_tir_super_soft",   text_price_tir_super_soft,  0);
    TEXT("price_tms_close",        text_price_tms_close,       0);
    TEXT("price_tms_super_close",  text_price_tms_super_close, 0);
    TEXT("price_tms_variable",     text_price_tms_variable,    0);
    TEXT("price_vcd_on",           text_price_vcd_on,          0);
    TEXT("price_wheel",            text_price_wheel,           0);
    TEXT("price_wtr_1",            text_price_wtr_1,           0);
    TEXT("price_wtr_2",            text_price_wtr_2,           0);
    TEXT("price_wtr_3",            text_price_wtr_3,           0);
    HOOK("race_item",              make_race_item,             0, 0);
    TEXT("race_prize_1",           text_race_prize_1,          0);
    TEXT("race_prize_2",           text_race_prize_2,          0);
    TEXT("race_prize_3",           text_race_prize_3,          0);
    TEXT("race_prize_4",           text_race_prize_4,          0);
    TEXT("race_prize_5",           text_race_prize_5,          0);
    TEXT("race_prize_6",           text_race_prize_6,          0);
    HOOK("regulation_aspiration",  make_regulation_aspiration, 0, 0);
    HOOK("regulation_car",         make_regulation_car,        0, 0);
    HOOK("regulation_dr_type",     make_regulation_dr_type,    0, 0);
    HOOK("regulation_license",     make_regulation_license,    0, 0);
    HOOK("regulation_power",       make_regulation_power,      0, 0);
    HOOK("regulation_tire",        make_regulation_tire,       0, 0);
    HOOK("regulation_tuning",      make_regulation_tuning,     0, 0);
    HOOK("scroll_bar",             create_scroll_bar, (int)scroll_bar_list, 0);
    HOOK("slow_decay",             make_slow_decay,            0, 0);
    TEXT("spec_aspiration",        text_spec_aspiration,       0);
    TEXT("spec_displacement",      text_spec_displacement,     0);
    TEXT("spec_displacement_real", text_spec_displacement,     0);
    TEXT("spec_drive_type",        text_spec_drive_type,       0);
    TEXT("spec_engine_type",       text_spec_engine_type,      0);
    TEXT("spec_height",            text_spec_height,           0);
    TEXT("spec_length",            text_spec_length,           0);
    TEXT("spec_max_power",         text_spec_max_power,        0);
    TEXT("spec_max_power_real",    text_spec_max_power_real,   0);
    TEXT("spec_max_torque",        text_spec_max_torque,       0);
    TEXT("spec_max_torque_real",   text_spec_max_torque_real,  0);
    TEXT("spec_p_w_ratio",         text_spec_p_w_ratio,        0);
    TEXT("spec_p_w_ratio_real",    text_spec_p_w_ratio_real,   0);
    TEXT("spec_t_w_ratio",         text_spec_t_w_ratio,        0);
    TEXT("spec_t_w_ratio_real",    text_spec_t_w_ratio_real,   0);
    TEXT("spec_tread_f",           text_spec_tread_f,          0);
    TEXT("spec_tread_r",           text_spec_tread_r,          0);
    TEXT("spec_weight",            text_spec_weight,           0);
    TEXT("spec_wheel_base",        text_spec_wheel_base,       0);
    TEXT("spec_width",             text_spec_width,            0);
    TEXT("status_ave_prize_car",   text_status_ave_prize_car,  0);
    TEXT("status_avg_win",         text_status_avg_win,        0);
    TEXT("status_complete",        text_status_complete,       0);
    TEXT("status_no_cars",         text_status_no_cars,        0);
    TEXT("status_no_lc_b",         text_status_no_lc_b,        0);
    TEXT("status_no_lc_g",         text_status_no_lc_g,        0);
    TEXT("status_no_lc_s",         text_status_no_lc_s,        0);
    TEXT("status_no_trophy",       text_status_no_trophy,      0);
    TEXT("status_prize_money",     text_status_prize_money,    0);
    TEXT("status_ttl_car_price",   text_status_ttl_car_price,  0);
    TEXT("status_ttl_milage",      text_status_ttl_milage,     0);
    TEXT("status_ttl_races",       text_status_ttl_races,      0);
    TEXT("status_ttl_wins",        text_status_ttl_wins,       0);
    HOOK("tuner_logo",             make_tuner_logo,            0, 0);
    TEXT("user_carname",           text_user_carname,          0);
    TEXT("user_carname_full",      text_user_carname_full,     0);
    TEXT("user_day",               text_user_day,              1);
    HOOK("user_day_unit",          make_user_day_unit,         0, 0);
    HOOK("user_license",           make_user_license,          0, 0);
    TEXT("user_money",             text_user_money,            0);
    HOOK("violated_license",       make_violated_license,      0, 0);
}

/* ------------------------------------------------------------------------ */
/* DecayItem, and the copies egcs emits at the end of the unit              */
/* ------------------------------------------------------------------------ */
/* After DecayItem's method, the code egcs emits for the classes whose
 * vtables this unit writes (DecayItem, constructed by make_slow_decay;
 * GarageCar / Cell<GarageCar>, constructed by text_ps_after;
 * RegurationResult, returned by check_regulation in make_regulation_tire):
 * their inline / implicit destructors and typeinfo functions.  T096 has a second copy of the GarageCar ones and T045 the
 * RegurationResult ones; every GarageCar vtable uses T036's copies and every
 * RegurationResult vtable T045's, so 0x001259A0 / 0x001259D0 have no
 * references.  The type names after the vtables (0x00311C70..0x00311EE6)
 * are retail's: the include order at the top of the file (ref/data/u010/
 * notes.md).  egcs emits these copies in reverse class-definition order,
 * each class's inline members, its __tf, then its implicit members:
 * DecayItem's (virtual_4, __tf, ~), RegurationResult's (~, __tf),
 * Cell<GarageCar>::~Cell (instantiated right after GarageCar's class:
 * game/menu/gtmode/T036_garage_car.h), GarageCar's (__tf, ~), Cell<GarageCar>'s __tf. */

// FUNC 0x001258E8 gt2k_menu::DecayItem::virtual_4
/* inline in game/menu/gtmode/T036_decay_item.h (DecayItem has no key function); egcs
 * emits this copy for the vtable, first in the end-of-unit group: disappear
 * (slot 4), start(g_00310A00), the caller's script is not used. */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of DecayItem (base Item) on the first call. */
// FUNC 0x00125908 __tfQ29gt2k_menu9DecayItem
extern "C" void *__tfQ29gt2k_menu9DecayItem(void)
{
    if (__tiQ29gt2k_menu9DecayItem[0] == 0) {           /* _name not set yet */
        __tfQ29gt2k_menu4Item();                        /* the base's node first */
        __rtti_si(__tiQ29gt2k_menu9DecayItem, "Q29gt2k_menu9DecayItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu9DecayItem;
}
#endif

// FUNC 0x00125958 gt2k_menu::DecayItem::~DecayItem
/* implicit in game/menu/gtmode/T036_decay_item.h; egcs emits this copy for the vtable: no
 * vptr store; gt2k_menu::Item::~Item(this, 0), then the class
 * Object::operator delete (0x001BF458) if flags & 1. */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */

// FUNC 0x001259A0 RegurationResult::~RegurationResult
/* inline in game/menu/gtmode/T045_gtmode_page_checks.h; egcs emits this copy (T036's,
 * unreferenced: the vtables use T045's 0x0011C958): the vptr (+0x1C) =
 * 0x00310310 (T045's vtable, not T036's copy 0x00311C10: the weak vtables
 * resolve to the first), then ::operator delete (0x0027C0C0) if flags & 1. */
/* implicit: operator delete (deleting destructor, the inline ~RegurationResult in the header) */

#ifndef BOOT_OLD_GCC
/* T036's copy of RegurationResult's typeinfo function (unreferenced; T045's
 * is 0x0011C988).  Weak: the T045 definition wins at link time. */
// FUNC 0x001259D0 __tf16RegurationResult
extern "C" __attribute__((weak)) void *__tf16RegurationResult(void)
{
    if (__ti16RegurationResult[0] == 0)                 /* _name not set yet */
        __rtti_user(__ti16RegurationResult, "16RegurationResult");
    return __ti16RegurationResult;
}
#endif

// FUNC 0x00125A10 gt2k_menu::Cell<GarageCar>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (T036's, the one the
 * GarageCar vtables and T490's F_0023B2A0 use): the vptr (+0x8) =
 * 0x00311C50, then gt2k_menu::pCell::~pCell(this, flags), which does the
 * delete. */
/* implicit: ~pCell (base destructor, the inline ~Cell in the header) */

#ifndef BOOT_OLD_GCC
/* Builds the __class_type_info of GarageCar (bases Cell<GarageCar> at +0
 * and CarInformation at +0x10, base list g_00311EA0) on the first call; the
 * bases' nodes first, CarInformation's before Cell<GarageCar>'s. */
// FUNC 0x00125A38 __tf9GarageCar
extern "C" void *__tf9GarageCar(void)
{
    if (__ti9GarageCar[0] == 0) {                       /* _name not set yet */
        __tf14CarInformation();
        __tfQ29gt2k_menut4Cell1Z9GarageCar();
        __rtti_class(__ti9GarageCar, "9GarageCar", g_00311EA0, 2);
    }
    return __ti9GarageCar;
}
#endif

// FUNC 0x00125A98 GarageCar::~GarageCar
/* inline in game/menu/gtmode/T036_garage.h; egcs emits this copy (T036's, the one every
 * GarageCar vtable uses): the vptrs +0x1EC = 0x00311C20 and +0x8 =
 * 0x00311C30, CarInformation::~CarInformation(this + 0x10, 0), then
 * Cell<GarageCar>'s inline destructor: the vptr 0x00311C50,
 * gt2k_menu::pCell::~pCell(this, flags), which does the delete. */
/* implicit: ~CarInformation, ~pCell (base destructors, the inline ~GarageCar in the header) */

#ifndef BOOT_OLD_GCC
/* Builds the __si_type_info of Cell<GarageCar> (base pCell) on the first
 * call.  Callers: __tf9GarageCar and the other units' copies of it (T053,
 * T080, T096, T427, T429, T490 all call this one). */
// FUNC 0x00125B00 __tfQ29gt2k_menut4Cell1Z9GarageCar
extern "C" void *__tfQ29gt2k_menut4Cell1Z9GarageCar(void)
{
    if (__tiQ29gt2k_menut4Cell1Z9GarageCar[0] == 0) {  /* _name not set yet */
        __tfQ29gt2k_menu5pCell();                       /* the base's node first */
        __rtti_si(__tiQ29gt2k_menut4Cell1Z9GarageCar, "Q29gt2k_menut4Cell1Z9GarageCar",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1Z9GarageCar;
}
#endif
