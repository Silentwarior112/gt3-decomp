/* T018 - the arcade "ar/select_compulsory" page.  0x00103B70..0x00104EE0.
 *
 * Member functions of gt2k_menu::ArcadeControl (T010), the page of
 * ARCADE_COMPULSORY (runCompulsory).  It shows the cars "tt01".."ttNN" of
 * the time trials, picked with car1 (player 1's car index, clamped): per car
 * a background, a logo, a description, power / weight / tyre figures and
 * the three best times of its time trial (TimeTrialRecords course
 * car1.value), the strip of mini pictures (a MiniCursor) and the automatic /
 * manual pictures of the transmission row.  Most items are placed relative
 * to the cross bar (attachToBar), which scrolls one screen per car.
 *
 * Item pictures are the hidden items of the page's layout (pages.current)
 * and the items go under the current layer (pages.layers[popupLevel]).
 * Every `new' of a menu object uses Object::operator new (the menu heap) and
 * frees with Object::operator delete if the constructor throws.
 *
 * Data: .rodata 0x00309658..0x00309946 opens with "(%f, %f, %f) %f %f %f\n",
 * the string of SimpleCameraBase::print (an inline member: egcs writes it
 * where the header is read; the original saw the class, ? through a menu car
 * header), then the functions' strings and the type names SmartIndexT<int>,
 * OperatorList, List<OperatorHolder>, OperatorHolder, Cell<OperatorHolder>,
 * GarageWheelList, List<GarageWheel>, GarageWheel, Cell<GarageWheel>,
 * GarageCarList, List<GarageCar>, GarageCar, Cell<GarageCar>, Operator
 * (reverse definition order): the GameStatus (garage) before the menu
 * headers, T010 (SmartIndexT<int>) after them. */
/* .rodata opens with the debug format the pose dump prints (nothing
 * references it before T018's own strings; it was counted to T034). */
// DATA_START 0x00309658
#include "types.h"
#include <stddef.h>
#include "game/camera/T412_simple_camera_base.h"   /* (data layout only) */
#include "game/save/T429_game_status.h"          /* getGameStatus */
#include "game/save/T417_time_trial_records.h"   /* TimeTrialRecords::getTime */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive */
#include "game/menu/framework/T024_mini_cursor.h"          /* gt2k_menu::MiniCursor */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car (ArcadeControl::carView) */
#include "game/menu/framework/T255_cross.h"                /* gt2k_menu::Cross */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable, ANCHOR_* */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (ArcadeControl::car1Spec) */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar::power (MenuCarSpec::car) */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/common/T494_text_utils.h"           /* format_time */
#include "engine/io/T507_async_loader.h"         /* async_loader_create / _destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/arcade/T010_arcade_control.h"       /* gt2k_menu::ArcadeControl; SmartIndexT<int> (T251) */
#include "game/menu/arcade/T018_select_compulsory.h"
#include "game/menu/arcade/T020_arcade_helpers.h"       /* arcade_fade_out, F_001051B0 */
#include "game/menu/arcade/arcade_select_internal.h"                           /* UNIT_TAG_PS */

namespace gt2k_menu {

/* selectCompulsory's states. */
enum {
    COMPULSORY_CANCELLED    = 0,
    COMPULSORY_SELECT_CAR   = 1,
    COMPULSORY_TRANSMISSION = 2,
    COMPULSORY_DONE         = 3
};

/* The NameFn of the mini cursor (createMiniCursor), passed as the member
 * pointer constant 0x00350BF0 {delta 0, index -1, 0x00103B70}; `arg' is
 * not used. */
// FUNC 0x00103B70 gt2k_menu::ArcadeControl::compulsoryMiniName
void ArcadeControl::compulsoryMiniName(char *buf, int i, int arg)
{
    sprintf(buf, "tt%02d_mini", i + 1);
}

/* "tt%02d_doc" and "arrow", both relative to the cross bar. */
// FUNC 0x00103B98 gt2k_menu::ArcadeControl::compulsoryCreateDoc
void ArcadeControl::compulsoryCreateDoc()
{
    char name[0x40];
    PageManager *pm = &pages;
    Transitive *layer = pm->layers[pm->popupLevel];

    sprintf(name, "tt%02d_doc", car1.value + 1);
    doc = new Item(name, pm->current);    /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    attachToBar(doc, crossBar);
    doc->attachTo(layer);

    arrow = new Item("arrow", pm->current);
    attachToBar(arrow, crossBar);
    arrow->attachTo(layer);
}

/* Power ("<value> <unit>": ShowRoomCar::power in the user's unit, the
 * catalogue figure if the car has one), weight ("<kg> KG") and tyre items.
 * Quirk: "tire01" is created and placed but never attached to the layer. */
// FUNC 0x00103CC0 gt2k_menu::ArcadeControl::compulsoryCreateSpec
void ArcadeControl::compulsoryCreateSpec()
{
    char text[0x40];
    int rpm;
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    power = new Item("power", layout);     /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    attachToBar(power, crossBar);
    power->attachTo(layer);

    powerValue = new Item("power_value", layout);
    attachToBar(powerValue, crossBar);
    ShowRoomCar *car = &car1Spec.car;
    int ps = car->power(&rpm, UNIT_TAG_PS, 1);
    sprintf(text, "%d %s", ps, g_002F59D8.powerName());
    powerValue->setText(text);
    powerValue->attachTo(layer);

    weight = new Item("weight", layout);
    attachToBar(weight, crossBar);
    weight->attachTo(layer);

    weightValue = new Item("weight_value", layout);
    attachToBar(weightValue, crossBar);
    sprintf(text, "%d %s", car->spec.weight, "KG");
    weightValue->setText(text);
    weightValue->attachTo(layer);

    tire = new Item("tire", layout);
    attachToBar(tire, crossBar);
    tire->attachTo(layer);

    tire01 = new Item("tire01", layout);
    attachToBar(tire01, crossBar);
}

/* "record" and the 1st .. 3rd times of the car's time trial.  "--:--:--"
 * for a time of 0; an empty place holds
 * 359999999, which format_time prints as dashes itself (and reset() puts
 * three preset records in every course). */
// FUNC 0x00103FC8 gt2k_menu::ArcadeControl::compulsoryCreateRecords
void ArcadeControl::compulsoryCreateRecords()
{
    char text[0x40];
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];
    TimeTrialRecords *records = getGameStatus()->getTimeTrialRecords();
    u32 time;

    record = new Item("record", layout);   /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    attachToBar(record, crossBar);
    record->attachTo(layer);

    first = new Item("first", layout);
    attachToBar(first, crossBar);
    first->attachTo(layer);

    firstTime = new Item("first_time", layout);
    time = records->getTime(car1.value, 0);
    if (time == 0) {
        firstTime->setText("--:--:--");
    } else {
        format_time(text, time);
        firstTime->setText(text);
    }
    attachToBar(firstTime, crossBar);
    firstTime->attachTo(layer);

    second = new Item("second", layout);
    attachToBar(second, crossBar);
    second->attachTo(layer);

    secondTime = new Item("second_time", layout);
    time = records->getTime(car1.value, 1);
    if (time == 0) {
        secondTime->setText("--:--:--");
    } else {
        format_time(text, time);
        secondTime->setText(text);
    }
    attachToBar(secondTime, crossBar);
    secondTime->attachTo(layer);

    third = new Item("third", layout);
    attachToBar(third, crossBar);
    third->attachTo(layer);

    thirdTime = new Item("third_time", layout);
    time = records->getTime(car1.value, 2);
    if (time == 0) {
        thirdTime->setText("--:--:--");
    } else {
        format_time(text, time);
        thirdTime->setText(text);
    }
    attachToBar(thirdTime, crossBar);
    thirdTime->attachTo(layer);
}

/* After car1 moved: loads the new car, replaces its view, logo,
 * description, spec and records (the old items fade out and die through
 * arcade_fade_out; the old view and background are killed at once), scrolls the
 * cross bar to the car's screen and updates the arrows. */
// FUNC 0x00104390 gt2k_menu::ArcadeControl::compulsoryChangeCar
void ArcadeControl::compulsoryChangeCar()
{
    char name[0x40];
    const char *code = carCode(0, -1);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    car1Spec.setCar(code);
    carView->kill();
    carView = createCarView(code);
    carView->attachTo(layer);
    crossBar->wrapTargetX(car1.value);

    arcade_fade_out(logo);
    sprintf(name, "tt%02d_logo", car1.value + 1);
    logo = new Item(name, layout);          /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    attachToBar(logo, crossBar);
    logo->attachTo(layer);

    arcade_fade_out(doc);
    arcade_fade_out(arrow);
    compulsoryCreateDoc();

    arcade_fade_out(power);
    arcade_fade_out(powerValue);
    arcade_fade_out(weight);
    arcade_fade_out(weightValue);
    arcade_fade_out(tire);
    arcade_fade_out(tire01);
    compulsoryCreateSpec();

    arcade_fade_out(record);
    arcade_fade_out(first);
    arcade_fade_out(firstTime);
    arcade_fade_out(second);
    arcade_fade_out(secondTime);
    arcade_fade_out(third);
    arcade_fade_out(thirdTime);
    compulsoryCreateRecords();

    sprintf(name, "tt%02d_bg", car1.value + 1);
    Item *oldBg = bg;
    bg = new Item(name, layout);
    bg->attachTo(layer);
    oldBg->kill();

    updateArrows(&car1);
    startCarViewMotion(0);
}

/* Builds the page: car1 over the cars of the class (clamped), color1 over
 * [0, 1), the cross bar (its targets and crossBarY from its layout position, then at rest on the car's screen),
 * the background, the arrows, the hidden "car" placeholder, the car view,
 * logo, description, spec, records, the mini cursor, the blur and the four
 * automatic / manual pictures (x 220 / 420, y the transmission row
 * F_001051B0(1), anchored to the bar in y only). */
// FUNC 0x001045D0 gt2k_menu::ArcadeControl::compulsoryOpen
void ArcadeControl::compulsoryOpen()
{
    char name[0x40];
    SmartIndexT<int> *index = &car1;

    index->setRange(0, carCount(0));
    index->set(0);
    index->clamp = 1;
    const char *code = carCode(0, -1);
    car1Spec.setCar(code);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];
    color1.setRange(0, 1);
    color1.set(0);

    crossBar = new Cross(0);                /* Object::operator new / delete */ /* implicit: operator new, operator delete (new-expression) */
    crossBar->setResource("cross_bar", layout);
    crossBar->targetX = crossBar->x.get();
    crossBar->targetY = crossBar->y.get();
    crossBarY = crossBar->y.get();
    crossBar->attachTo(layer);
    crossBar->jumpX(car1.value);

    sprintf(name, "tt%02d_bg", car1.value + 1);
    bg = new Item(name, layout);
    bg->attachTo(layer);

    createArrows(index, layout, layer);

    carItem = new Item("car", layout);
    carItem->visible = 0;
    carItem->attachTo(layer);

    carView = createCarView(code);
    carView->attachTo(layer);

    sprintf(name, "tt%02d_logo", car1.value + 1);
    logo = new Item(name, layout);
    attachToBar(logo, crossBar);
    logo->attachTo(layer);

    compulsoryCreateDoc();
    compulsoryCreateSpec();
    compulsoryCreateRecords();

    miniCars = createMiniCursor("mini_cars", index, &ArcadeControl::compulsoryMiniName, 0);
    attachToBar(miniCars, crossBar);
    miniCars->attachTo(layer);

    createBlur();

    autoNo = new Item("auto_no", layout);
    autoNo->x.set(220.0f);
    autoNo->y.set(F_001051B0(1));
    autoNo->anchor = crossBar;
    autoNo->anchorType = ANCHOR_Y;
    autoNo->attachTo(layer);

    autoYes = new Item("auto_yes", layout);
    autoYes->x.set(220.0f);
    autoYes->y.set(F_001051B0(1));
    autoYes->anchor = crossBar;
    autoYes->anchorType = ANCHOR_Y;
    autoYes->attachTo(layer);

    manuNo = new Item("manu_no", layout);
    manuNo->x.set(420.0f);
    manuNo->y.set(F_001051B0(1));
    manuNo->anchor = crossBar;
    manuNo->anchorType = ANCHOR_Y;
    manuNo->attachTo(layer);

    manuYes = new Item("manu_yes", layout);
    manuYes->x.set(420.0f);
    manuYes->y.set(F_001051B0(1));
    manuYes->anchor = crossBar;
    manuYes->anchorType = ANCHOR_Y;
    manuYes->attachTo(layer);

    updateTransmissionButtons();
}

/* Forgets the page's items once the page is closed (the page owns them). */
// FUNC 0x00104B40 gt2k_menu::ArcadeControl::compulsoryClear
void ArcadeControl::compulsoryClear()
{
    crossBar = NULL;
    carItem = NULL;
    carView = NULL;
    bg = NULL;
    clearArrows();
    miniCars = NULL;
    logo = NULL;
    arrow = NULL;
    power = NULL;
    powerValue = NULL;
    weight = NULL;
    weightValue = NULL;
    tire = NULL;
    tire01 = NULL;
    record = NULL;
    first = NULL;
    firstTime = NULL;
    second = NULL;
    secondTime = NULL;
    third = NULL;
    thirdTime = NULL;
    doc = NULL;
    autoNo = NULL;
    autoYes = NULL;
    manuNo = NULL;
    manuYes = NULL;
}

/* The input loop of the page (on row 0): left / right move car1 (virtual
 * move, slot 3) and rebuild the car's items, or are refused; decide returns
 * 1; cancel returns 0 unless the mode is ARCADE_I_LINK (never on this page:
 * ? copied from the other selects).  On a time-out / exit request
 * (timedOut) the argument is returned. */
// FUNC 0x00104BC8 gt2k_menu::ArcadeControl::compulsorySelectCar
int ArcadeControl::compulsorySelectCar(int result)
{
    scrollToRow(0);
    SmartIndexT<int> *index = &car1;
    int noCancel = (mode == ARCADE_I_LINK);

    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;
            if (control1.getLeft() != 0)
                moved = index->move(-1);
            else if (control1.getRight() != 0)
                moved = index->move(1);
            if (moved != 0) {
                menu_play_se(MENU_SE_LIST_MOVE);
                compulsoryChangeCar();
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0) {
                menu_play_se(MENU_SE_DECIDE);
                result = 1;
                break;
            } else if (control1.getCancel() != 0 && noCancel == 0) {
                menu_play_se(MENU_SE_CANCEL);
                result = 0;
                break;
            }
        }
        waitFrame();
    } while (timedOut == 0);
    return result;
}

/* The page (runCompulsory): opens "ar/select_compulsory" (the background
 * movie suspended, a loader without a buffer), builds it, fades in, then
 * runs the car select (back: result 0) and the transmission row (back: to
 * the car select) until done (result 1) or timedOut; fades out, clears the
 * items, closes the page and returns the result. */
// FUNC 0x00104D08 gt2k_menu::ArcadeControl::selectCompulsory
int ArcadeControl::selectCompulsory(int result)
{
    int done = 0;
    int state = COMPULSORY_SELECT_CAR;

    pages.setPage("ar/select_compulsory", NULL);
    pages.startPage(NULL);
    g_002CDC10.suspend(1);
    async_loader_create(0);
    compulsoryOpen();
    transitionIn();
    gfx_vsync(10);

    do {
        switch (state) {
        case COMPULSORY_CANCELLED:
            result = 0;
            done = 1;
            break;
        case COMPULSORY_SELECT_CAR:
            result = compulsorySelectCar(result);
            if (result != 0)
                state = COMPULSORY_TRANSMISSION;
            else
                state = COMPULSORY_CANCELLED;
            break;
        case COMPULSORY_TRANSMISSION:
            hideMiniCars();
            if (result != 0)
                blurIn();
            result = selectTransmission(result);
            state = COMPULSORY_DONE;
            if (result == 0) {
                state = COMPULSORY_SELECT_CAR;
                showMiniCars();
                blurOut();
            }
            break;
        case COMPULSORY_DONE:
            result = 1;
            done = 1;
            break;
        }
        waitFrame();
        if (timedOut != 0)
            break;
    } while (done == 0);

    transitionOut();
    compulsoryClear();
    async_loader_destroy();
    pages.closeAndExit();
    gfx_vsync(2);
    g_002CDC10.suspend(0);
    return result;
}

} /* namespace gt2k_menu */
