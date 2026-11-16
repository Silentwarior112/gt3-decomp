/* T021 - the two-player arcade car select page ("ar/select_car_2p").
 * 0x001060B8..0x00108870.
 *
 * carSelect2P runs carSelect2PStep for player 1 and player 2 every frame.
 * Each player walks the states
 *   1/2  the class list ("mini_cars" over the classes) and the class logo
 *                                                          -> cancel: state 0
 *   3/4  the car list, data, labels and gauges over the 3D car;
 *        left / right = car, up / down = colour            -> cancel: state 1
 *   5/6  the transmission (auto_* / manu_* items)          -> cancel: state 3
 *   7/8  the "racing" / "dri" option (skipped on a dirt course and for the
 *        garage classes)                                   -> cancel: state 5
 *   9    ready; cancel goes back to 5 (dirt) or 7; when the other player is
 *        ready too -> 10
 *   10   done (*result = 1);   0: backed out (*result = 0).
 * When one player finishes, carSelect2PAbort closes what the other player's
 * state has open.  The odd states (1, 3, 5, 7) build their page and fall
 * into the even ones, which run the input of that page.
 *
 * Player 1's objects are the members the one-player car select (T023)
 * uses (carItem, carView, miniCars, colorChips, ...), player 2's the "2"
 * members; the items of player 2 are the layout's "_2p" ones.
 *
 * The includes give the original's type names (.rodata 0x0030A120..): egcs
 * writes them in reverse definition order, so the original saw the
 * GameStatus's garage lists first, then the Transitive headers
 * (OperatorHolder, List / Cell<OperatorHolder>, OperatorList), then
 * TransitiveEX (T023's, whose name only T021 and T023 have), and
 * SmartIndexT<int> last (ArcadeControl's members; T251_smart_index.h
 * instantiates it where it is read, so T010_arcade_control.h comes last). */
#include "game/save/T429_game_status.h"          /* first: the garage lists; getGameStatus, MenuStatus,
                                               GarageCar::getWheelModelName */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/arcade/T023_arcade_car_select.h"    /* TransitiveEX (not used: its type name); T216 */
#include "game/menu/arcade/T020_arcade_helpers.h"       /* arcade_fade_out */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car, menu_car_clear_loading */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T024_mini_cursor.h"          /* gt2k_menu::MiniCursor */
#include "game/menu/framework/T032_circle_gauge.h"         /* gt2k_menu::CircleGauge */
#include "game/menu/arcade/T035_select_box.h"           /* gt2k_menu::SelectBox (colorChips) */
#include "game/menu/framework/T258_menu_string.h"          /* gt2k_menu::String (powerText ... accelText) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "engine/io/T507_async_loader.h"         /* async_loader_create / destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl (last: SmartIndexT<int>) */
#include "game/menu/arcade/arcade_car_select_2p_internal.h"
/* Not used here: the original's menu car headers brought SimpleCameraBase
 * in, and egcs writes the string of its inline print() when it reads the
 * class: "(%f, %f, %f) %f %f %f\n" opens T021's .rodata (0x00309EF0), as it
 * does in other menu objects.  Without it the strings start 0x18 bytes
 * earlier in the section and lose retail's distance to the 16-aligned jump
 * tables.  (T022's .rodata, 0x0030A320, has no copy.)  No code references
 * the string, so the anchor below starts T021's .rodata range there (it
 * follows T020's last type name, Operator, 0x00309ED8). */
// DATA_START 0x00309EF0
#include "game/camera/T412_simple_camera_base.h"

namespace gt2k_menu {

/* The class and car logos of the page: 3/4 of their layout size; player 2's
 * is mirrored to the right half of the screen (at rest there).  The four
 * builders expand the same code (? an inline helper of the original). */
static inline void setupLogo2P(Item *logo, int player)
{
    logo->w = logo->w * 0.75f;
    logo->h = logo->h * 0.75f;
    if (player == 2) {
        MotionValue *x = &logo->x;
        x->set(MENU_SCREEN_W - x->value);
    }
}

/* The 3D car shown at player `player''s "car" placeholder item: the
 * placeholder's texture set as the environment image, the player's spec and
 * model buffer, camera mode 1 / 2 (2: the mirrored preset), the colour of the
 * colour index, the garage car's wheel (garage classes only), the
 * placeholder's draw order. */
// FUNC 0x001060B8 gt2k_menu::ArcadeControl::newCarModel
Car *ArcadeControl::newCarModel(const char *code, int player)
{
    SmartIndexT<int> *color = colorOf(this, player);
    MenuCarSpec *spec = specOf(this, player);
    Item **place = carItemOf(this, player);
    int cameraMode = (player != 2) ? 1 : 2;

    /* Object::operator new (the menu heap); Object::operator delete if the
     * constructor throws */
    /* implicit: operator new, operator delete (new-expression) */
    Car *car = new Car((*place)->texSet, spec, player == 2,
                       U003_CAR_BUFFER(this, player == 2));
    car->camera.setType(cameraMode);
    car->setCar(code);
    car->setColor(color->value);
    GarageCar *garage = garageCar(player);
    if (garage != NULL)
        car->setWheel(garage->getWheelModelName());
    car->z = (*place)->z;
    return car;
}

/* The input step of one selector page.  left / right move `sel'
 * (MENU_SE_LIST_MOVE, then onSelChange), up / down move `sel2' if there is
 * one (MENU_SE_CURSOR, then onSel2Change); a move that does not happen
 * sounds MENU_SE_REFUSED.  decide is refused (MENU_SE_REFUSED) when
 * selDisabled says the current value is disabled, else it ends the page with
 * *result = 1 (MENU_SE_DECIDE); cancel ends it with *result = 0
 * (MENU_SE_CANCEL).  Returns 1 when the page ended.  The callbacks are
 * pointers to members (egcs calls them through the vtable when their index
 * is positive; all the ones passed here are non-virtual); a NULL one (index
 * 0) is skipped.  decide is read again after the selDisabled call. */
// FUNC 0x00106210 gt2k_menu::ArcadeControl::selectInput
int ArcadeControl::selectInput(int player, int *result, MenuControl *pad, SmartIndexT<int> *sel,
                               ChangeFn onSelChange, CheckFn selDisabled, SmartIndexT<int> *sel2,
                               ChangeFn onSel2Change)
{
    int ended = 0;

    if (pad->left != 0 || pad->right != 0) {
        int moved = 0;
        if (pad->left != 0)
            moved = sel->move(-1);
        else if (pad->right != 0)
            moved = sel->move(1);
        if (moved != 0) {
            menu_play_se(MENU_SE_LIST_MOVE);
            if (onSelChange)
                (this->*onSelChange)(player);
        } else {
            menu_play_se(MENU_SE_REFUSED);
        }
    } else if (sel2 != NULL && (pad->up != 0 || pad->down != 0)) {
        int moved = 0;
        if (pad->up != 0)
            moved = sel2->move(-1);
        else if (pad->down != 0)
            moved = sel2->move(1);
        if (moved != 0) {
            menu_play_se(MENU_SE_CURSOR);
            if (onSel2Change)
                (this->*onSel2Change)(player);
        } else {
            menu_play_se(MENU_SE_REFUSED);
        }
    } else if (pad->decide != 0 || pad->cancel != 0) {
        if (pad->decide != 0 && selDisabled && (this->*selDisabled)(sel->value, player) != 0) {
            menu_play_se(MENU_SE_REFUSED);
        } else if (pad->decide != 0) {
            menu_play_se(MENU_SE_DECIDE);
            *result = 1;
            ended = 1;
        } else if (pad->cancel != 0) {
            menu_play_se(MENU_SE_CANCEL);
            *result = 0;
            ended = 1;
        }
    }
    return ended;
}

/* States 1/2: the class list ("mini_cars" / "mini_cars_2p", entries
 * "<class>_mini", mark "class_x") and the logo of the current class.  The
 * classes the course or the garages do not allow are marked, and the current
 * class is moved off a road class on a dirt course (to the dirt class) / off
 * the dirt class on a road course (to C). */
// FUNC 0x001064D8 gt2k_menu::ArcadeControl::openClassList
void ArcadeControl::openClassList(int player)
{
    Item **plogo = logoOf(this, player);
    MiniCursor **plist = miniClassesOf(this, player);
    SmartIndexT<int> *cls = classOf(this, player);
    const char *listName = (player != 2) ? "mini_cars" : "mini_cars_2p";
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];
    char name[64];

    *plist = createMiniCursor(listName, cls, &ArcadeControl::formatClassMini, player);
    (*plist)->setMark("class_x", layout);
    (*plist)->attachTo(layer);
    if (isDirtCourse()) {
        (*plist)->setMarked(ARCADE_CLASS_C, 1);
        (*plist)->setMarked(ARCADE_CLASS_B, 1);
        (*plist)->setMarked(ARCADE_CLASS_A, 1);
        (*plist)->setMarked(ARCADE_CLASS_S, 1);
        if (cls->value < ARCADE_CLASS_DIRT)
            cls->set(ARCADE_CLASS_DIRT);
    } else {
        (*plist)->setMarked(ARCADE_CLASS_DIRT, 1);
        if (cls->value == ARCADE_CLASS_DIRT)
            cls->set(ARCADE_CLASS_C);
    }
    if (getGameStatus()->getCarNum() <= 0)
        (*plist)->setMarked(ARCADE_CLASS_HOME_GARAGE, 1);
    if (getGameStatusHolder()->get()->getCarNum() <= 0)
        (*plist)->setMarked(ARCADE_CLASS_GUEST_GARAGE, 1);

    const char *label = getClassLabel(cls->value);
    sprintf(name, "%s_logo", label);
    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *plogo = new Item(name, layout);
    setupLogo2P(*plogo, player);
    (*plogo)->attachTo(layer);
}

// FUNC 0x00106770 gt2k_menu::ArcadeControl::closeClassList
void ArcadeControl::closeClassList(int player)
{
    MiniCursor **plist = miniClassesOf(this, player);
    arcade_fade_out(*plist);
    *plist = NULL;
    Item **plogo = logoOf(this, player);
    arcade_fade_out(*plogo);
    *plogo = NULL;
}

/* The class list's change callback (pointer to member 0x00350C58): a new
 * logo for the new class (the old one goes through arcade_fade_out; the pointer is
 * overwritten, not cleared first). */
// FUNC 0x001067E0 gt2k_menu::ArcadeControl::onClassChange
void ArcadeControl::onClassChange(int player)
{
    Item **plogo = logoOf(this, player);
    SmartIndexT<int> *cls = classOf(this, player);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];
    char name[64];

    arcade_fade_out(*plogo);
    const char *label = getClassLabel(cls->value);
    sprintf(name, "%s_logo", label);
    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *plogo = new Item(name, layout);
    setupLogo2P(*plogo, player);
    (*plogo)->attachTo(layer);
}

/* State 2: left / right pick the class; the disabled classes refuse decide. */
// FUNC 0x00106928 gt2k_menu::ArcadeControl::classListInput
int ArcadeControl::classListInput(int *result, int player)
{
    /* The NULL onSel2Change (the 9th argument, on the stack) is a temporary
     * cleared by memset. */
    /* implicit: memset (temporary: the NULL pointer-to-member argument) */
    int ended;
    if (player != 2)
        ended = selectInput(player, result, &control1, &class1,
                            &ArcadeControl::onClassChange, &ArcadeControl::isClassDisabled,
                            NULL, 0);
    else
        ended = selectInput(2, result, &control2, &class2,
                            &ArcadeControl::onClassChange, &ArcadeControl::isClassDisabled,
                            NULL, 0);
    return ended;
}

/* Entering state 3: the car index covers the cars of the chosen class
 * (clamped, from 0); the car's logo, the hidden "car" placeholder and the 3D
 * car at it. */
// FUNC 0x00106A28 gt2k_menu::ArcadeControl::openCarView
void ArcadeControl::openCarView(int player)
{
    SmartCarIndex *car = carOf(this, player);
    Item **plogo = logoOf(this, player);
    Item **place = carItemOf(this, player);
    Car **pview = carViewOf(this, player);

    car->setRange(0, carCount(player));
    car->set(0);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];
    car->clamp = 1;
    const char *code = carCode(player, -1);
    loadCarInfo(player);
    resetColorIndex(player);

    const char *logoName = carName(code);   /* called before operator new */
    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *plogo = new Item(logoName, layout);
    setupLogo2P(*plogo, player);
    (*plogo)->attachTo(layer);

    *place = new Item("car", layout);
    (*place)->visible = 0;
    (*place)->attachTo(layer);

    *pview = newCarModel(code, player);
    (*pview)->attachTo(layer);
}

/* States 3/4: the car list ("mini_cars" / "mini_cars_2p" with mark "car_x",
 * entries named by miniCarImageName; the cars isCarLocked refuses are
 * marked), the colour chips, the drive train icon, the power / torque texts,
 * the handling / speed / acceleration labels and their gauges. */
// FUNC 0x00106C58 gt2k_menu::ArcadeControl::openCarList
void ArcadeControl::openCarList(int player)
{
    MiniCursor **plist = miniCarsOf(this, player);
    SmartCarIndex *car = carOf(this, player);
    const char *listName = (player != 2) ? "mini_cars" : "mini_cars_2p";
    MenuCarSpec *spec = specOf(this, player);
    SelectBox **pchips = colorChipsOf(this, player);
    Item **pdriveTrain = driveTrainOf(this, player);
    String **ppower = powerTextOf(this, player);
    String **ptorque = torqueTextOf(this, player);
    String **phandling = handlingTextOf(this, player);
    String **pspeed = speedTextOf(this, player);
    String **paccel = accelTextOf(this, player);
    CircleGauge **phandlingGauge = handlingGaugeOf(this, player);
    CircleGauge **pspeedGauge = speedGaugeOf(this, player);
    CircleGauge **paccelGauge = accelGaugeOf(this, player);
    const char *handlingName = (player != 2) ? "handling" : "handling_2p";
    const char *speedName = (player != 2) ? "speed" : "speed_2p";
    const char *accelName = (player != 2) ? "acceleration" : "acceleration_2p";
    const char *handlingGaugeName = (player != 2) ? "handling_gauge" : "handling_gauge_2p";
    const char *speedGaugeName = (player != 2) ? "speed_gauge" : "speed_gauge_2p";
    const char *accelGaugeName = (player != 2) ? "acceleration_gauge" : "acceleration_gauge_2p";
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    loadCarInfo(player);
    resetColorIndex(player);
    *plist = createMiniCursor(listName, car, &ArcadeControl::miniCarImageName, player);
    (*plist)->setMark("car_x", layout);
    for (int i = 0; i < car->size(); i++) {
        if (isCarLocked(i, player) != 0)
            (*plist)->setMarked(i, 1);
    }
    (*plist)->attachTo(layer);

    *pchips = createColorChips(player);
    (*pchips)->attachTo(layer);
    *pdriveTrain = createDriveTrainIcon(player);
    (*pdriveTrain)->attachTo(layer);
    *ppower = createPowerText(player);
    (*ppower)->attachTo(layer);
    *ptorque = createTorqueText(player);
    (*ptorque)->attachTo(layer);
    *phandling = createSpecLabel(handlingName);
    (*phandling)->attachTo(layer);
    *pspeed = createSpecLabel(speedName);
    (*pspeed)->attachTo(layer);
    *paccel = createSpecLabel(accelName);
    (*paccel)->attachTo(layer);
    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *phandlingGauge = new CircleGauge(spec, handlingGaugeName, layout, 0);  /* kind 0 handling */
    (*phandlingGauge)->attachTo(layer);
    *pspeedGauge = new CircleGauge(spec, speedGaugeName, layout, 1);        /* 1 top speed */
    (*pspeedGauge)->attachTo(layer);
    *paccelGauge = new CircleGauge(spec, accelGaugeName, layout, 2);        /* 2 acceleration */
    (*paccelGauge)->attachTo(layer);
}

/* Leaving state 4.  The list is killed at once, the rest goes through
 * arcade_fade_out. */
// FUNC 0x00107158 gt2k_menu::ArcadeControl::closeCarList
void ArcadeControl::closeCarList(int player)
{
    MiniCursor **plist = miniCarsOf(this, player);
    SelectBox **pchips = colorChipsOf(this, player);
    Item **pdriveTrain = driveTrainOf(this, player);
    String **ppower = powerTextOf(this, player);
    String **ptorque = torqueTextOf(this, player);
    String **phandling = handlingTextOf(this, player);
    String **pspeed = speedTextOf(this, player);
    String **paccel = accelTextOf(this, player);
    CircleGauge **phandlingGauge = handlingGaugeOf(this, player);
    CircleGauge **pspeedGauge = speedGaugeOf(this, player);
    CircleGauge **paccelGauge = accelGaugeOf(this, player);

    (*plist)->kill();
    *plist = NULL;
    arcade_fade_out(*pchips);
    *pchips = NULL;
    arcade_fade_out(*pdriveTrain);
    *pdriveTrain = NULL;
    arcade_fade_out(*ppower);
    *ppower = NULL;
    arcade_fade_out(*ptorque);
    *ptorque = NULL;
    arcade_fade_out(*phandling);
    *phandling = NULL;
    arcade_fade_out(*pspeed);
    *pspeed = NULL;
    arcade_fade_out(*paccel);
    *paccel = NULL;
    arcade_fade_out(*phandlingGauge);
    *phandlingGauge = NULL;
    arcade_fade_out(*pspeedGauge);
    *pspeedGauge = NULL;
    arcade_fade_out(*paccelGauge);
    *paccelGauge = NULL;
}

/* Removes what openCarView made. */
// FUNC 0x001072F0 gt2k_menu::ArcadeControl::closeCarView
void ArcadeControl::closeCarView(int player)
{
    Item **place = carItemOf(this, player);
    Item **plogo = logoOf(this, player);
    Car **pview = carViewOf(this, player);

    arcade_fade_out(*plogo);
    *plogo = NULL;
    arcade_fade_out(*place);
    *place = NULL;
    arcade_fade_out(*pview);
    *pview = NULL;
}

/* The car list's change callback (pointer to member 0x00350C88): colour 0,
 * a new logo, a new 3D car (the old one killed at once; the other player's
 * car, if any, is put 0.5 before it in the draw order) and new chips, icon,
 * texts, labels and gauges. */
// FUNC 0x00107370 gt2k_menu::ArcadeControl::onCarChange
void ArcadeControl::onCarChange(int player)
{
    Item **plogo = logoOf(this, player);
    Car **pview = carViewOf(this, player);
    Car **pother = (player == 2) ? &carView : &carView2;
    MenuCarSpec *spec = specOf(this, player);
    SmartIndexT<int> *color = colorOf(this, player);
    SelectBox **pchips = colorChipsOf(this, player);
    Item **pdriveTrain = driveTrainOf(this, player);
    String **ppower = powerTextOf(this, player);
    String **ptorque = torqueTextOf(this, player);
    String **phandling = handlingTextOf(this, player);
    String **pspeed = speedTextOf(this, player);
    String **paccel = accelTextOf(this, player);
    CircleGauge **phandlingGauge = handlingGaugeOf(this, player);
    CircleGauge **pspeedGauge = speedGaugeOf(this, player);
    CircleGauge **paccelGauge = accelGaugeOf(this, player);
    const char *handlingName = (player != 2) ? "handling" : "handling_2p";
    const char *speedName = (player != 2) ? "speed" : "speed_2p";
    const char *accelName = (player != 2) ? "acceleration" : "acceleration_2p";
    const char *handlingGaugeName = (player != 2) ? "handling_gauge" : "handling_gauge_2p";
    const char *speedGaugeName = (player != 2) ? "speed_gauge" : "speed_gauge_2p";
    const char *accelGaugeName = (player != 2) ? "acceleration_gauge" : "acceleration_gauge_2p";
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    const char *code = carCode(player, -1);
    loadCarInfo(player);
    color->set(0);
    resetColorIndex(player);

    arcade_fade_out(*plogo);
    const char *logoName = carName(code);   /* called before operator new */
    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *plogo = new Item(logoName, layout);
    setupLogo2P(*plogo, player);
    (*plogo)->attachTo(layer);

    (*pview)->kill();
    *pview = newCarModel(code, player);
    (*pview)->attachTo(layer);
    if (*pother != NULL)
        (*pother)->z = (*pview)->z - 0.5f;

    arcade_fade_out(*pchips);
    *pchips = createColorChips(player);
    (*pchips)->attachTo(layer);
    arcade_fade_out(*pdriveTrain);
    *pdriveTrain = createDriveTrainIcon(player);
    (*pdriveTrain)->attachTo(layer);
    arcade_fade_out(*ppower);
    *ppower = createPowerText(player);
    (*ppower)->attachTo(layer);
    arcade_fade_out(*ptorque);
    *ptorque = createTorqueText(player);
    (*ptorque)->attachTo(layer);
    arcade_fade_out(*phandling);
    *phandling = createSpecLabel(handlingName);
    (*phandling)->attachTo(layer);
    arcade_fade_out(*pspeed);
    *pspeed = createSpecLabel(speedName);
    (*pspeed)->attachTo(layer);
    arcade_fade_out(*paccel);
    *paccel = createSpecLabel(accelName);
    (*paccel)->attachTo(layer);
    arcade_fade_out(*phandlingGauge);
    *phandlingGauge = new CircleGauge(spec, handlingGaugeName, layout, 0);
    (*phandlingGauge)->attachTo(layer);
    arcade_fade_out(*pspeedGauge);
    *pspeedGauge = new CircleGauge(spec, speedGaugeName, layout, 1);
    (*pspeedGauge)->attachTo(layer);
    arcade_fade_out(*paccelGauge);
    *paccelGauge = new CircleGauge(spec, accelGaugeName, layout, 2);
    (*paccelGauge)->attachTo(layer);
}

/* The colour callback (pointer to member 0x00350C98). */
// FUNC 0x00107978 gt2k_menu::ArcadeControl::onColorChange
void ArcadeControl::onColorChange(int player)
{
    Car **pview = carViewOf(this, player);
    SmartIndexT<int> *color = colorOf(this, player);
    (*pview)->setColor(color->value);
}

/* State 4: left / right = car (isCarLocked refuses decide), up / down =
 * colour. */
// FUNC 0x001079B8 gt2k_menu::ArcadeControl::carListInput
int ArcadeControl::carListInput(int *result, int player)
{
    int ended;
    if (player != 2)
        ended = selectInput(player, result, &control1, &car1,
                            &ArcadeControl::onCarChange, &ArcadeControl::isCarLocked,
                            &color1, &ArcadeControl::onColorChange);
    else
        ended = selectInput(2, result, &control2, &car2,
                            &ArcadeControl::onCarChange, &ArcadeControl::isCarLocked,
                            &color2, &ArcadeControl::onColorChange);
    return ended;
}

/* States 5/6: the transmission choice, two items per answer. */
// FUNC 0x00107A70 gt2k_menu::ArcadeControl::openTransmission
void ArcadeControl::openTransmission(int player)
{
    Item **pautoNo = autoNoOf(this, player);
    Item **pautoYes = autoYesOf(this, player);
    Item **pmanuNo = manuNoOf(this, player);
    Item **pmanuYes = manuYesOf(this, player);
    const char *autoNoName = (player != 2) ? "auto_no" : "auto_no_2p";
    const char *autoYesName = (player != 2) ? "auto_yes" : "auto_yes_2p";
    const char *manuNoName = (player != 2) ? "manu_no" : "manu_no_2p";
    const char *manuYesName = (player != 2) ? "manu_yes" : "manu_yes_2p";
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *pautoNo = new Item(autoNoName, layout);
    (*pautoNo)->attachTo(layer);
    *pautoYes = new Item(autoYesName, layout);
    (*pautoYes)->attachTo(layer);
    *pmanuNo = new Item(manuNoName, layout);
    (*pmanuNo)->attachTo(layer);
    *pmanuYes = new Item(manuYesName, layout);
    (*pmanuYes)->attachTo(layer);
    onTransmissionChange(player);
}

/* Change callback (pointer to member 0x00350CB0): 0 (automatic) shows
 * auto_yes + manu_no, anything else (manual) auto_no + manu_yes.  The
 * enhanced engine (Lupo 2 L2 0x0010B338; menus_a.md 39): a CVT shows
 * automatic. */
// FUNC 0x00107CD8 gt2k_menu::ArcadeControl::onTransmissionChange
void ArcadeControl::onTransmissionChange(int player)
{
    SmartIndexT<int> *transmission = transmissionOf(this, player);
    Item **pautoNo = autoNoOf(this, player);
    Item **pautoYes = autoYesOf(this, player);
    Item **pmanuNo = manuNoOf(this, player);
    Item **pmanuYes = manuYesOf(this, player);

    if (transmission->value == 0 || isCarCVT(player) != 0) {
        (*pautoYes)->visible = 1;
        (*pautoNo)->visible = 0;
        (*pmanuYes)->visible = 0;
        (*pmanuNo)->visible = 1;
    } else {
        (*pautoYes)->visible = 0;
        (*pautoNo)->visible = 1;
        (*pmanuYes)->visible = 1;
        (*pmanuNo)->visible = 0;
    }
}

/* State 6: left / right; no check, no second selector.  The enhanced
 * engine (Lupo 2 L2 0x0010B3D8; menus_a.md 39): for a CVT the choice is
 * automatic only (the range [0, 1) at 0), else both. */
// FUNC 0x00107D70 gt2k_menu::ArcadeControl::transmissionInput
int ArcadeControl::transmissionInput(int *result, int player)
{
    /* The NULL CheckFn is the .sdata constant 0x00350CB8, the NULL
     * onSel2Change a stack temporary cleared by memset. */
    /* implicit: memset (temporary: the NULL pointer-to-member argument) */
    int ended;
    SmartIndexT<int> *transmission = transmissionOf(this, player);
    if (isCarCVT(player) != 0) {
        transmission->setRange(0, 1);
        transmission->set(0);
    } else {
        transmission->setRange(0, 2);
    }
    if (player != 2)
        ended = selectInput(player, result, &control1, &transmission1,
                            &ArcadeControl::onTransmissionChange, 0, NULL, 0);
    else
        ended = selectInput(2, result, &control2, &transmission2,
                            &ArcadeControl::onTransmissionChange, 0, NULL, 0);
    return ended;
}

// FUNC 0x00107E70 gt2k_menu::ArcadeControl::closeTransmission
void ArcadeControl::closeTransmission(int player)
{
    Item **pautoYes = autoYesOf(this, player);
    Item **pautoNo = autoNoOf(this, player);
    Item **pmanuNo = manuNoOf(this, player);
    Item **pmanuYes = manuYesOf(this, player);

    arcade_fade_out(*pautoNo);
    *pautoNo = NULL;
    arcade_fade_out(*pautoYes);
    *pautoYes = NULL;
    arcade_fade_out(*pmanuNo);
    *pmanuNo = NULL;
    arcade_fade_out(*pmanuYes);
    *pmanuYes = NULL;
}

/* States 7/8: the "racing" / "dri" choice (racing1 / racing2: 0 "racing",
 * 1 "dri"; MenuArgs::opt1b = racing1 == 1, T010).  The page shows it after
 * the transmission and skips it on a dirt course and for the garage classes
 * (? the tyres / set-up of a data base car). */
// FUNC 0x00107F10 gt2k_menu::ArcadeControl::openRacing
void ArcadeControl::openRacing(int player)
{
    Item **pracingNo = racingNoOf(this, player);
    Item **pracingYes = racingYesOf(this, player);
    Item **pdriNo = driNoOf(this, player);
    Item **pdriYes = driYesOf(this, player);
    const char *racingNoName = (player != 2) ? "racing_no" : "racing_no_2p";
    const char *racingYesName = (player != 2) ? "racing_yes" : "racing_yes_2p";
    const char *driNoName = (player != 2) ? "dri_no" : "dri_no_2p";
    const char *driYesName = (player != 2) ? "dri_yes" : "dri_yes_2p";
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    /* Object::operator new (the menu heap) */
   /* implicit: operator new, operator delete (new-expression) */
    *pracingNo = new Item(racingNoName, layout);
    (*pracingNo)->attachTo(layer);
    *pracingYes = new Item(racingYesName, layout);
    (*pracingYes)->attachTo(layer);
    *pdriNo = new Item(driNoName, layout);
    (*pdriNo)->attachTo(layer);
    *pdriYes = new Item(driYesName, layout);
    (*pdriYes)->attachTo(layer);
    onRacingChange(player);
}

/* Change callback (pointer to member 0x00350CD0): 0 shows racing_yes +
 * dri_no, anything else racing_no + dri_yes. */
// FUNC 0x00108178 gt2k_menu::ArcadeControl::onRacingChange
void ArcadeControl::onRacingChange(int player)
{
    SmartIndexT<int> *racing = racingOf(this, player);
    Item **pracingNo = racingNoOf(this, player);
    Item **pracingYes = racingYesOf(this, player);
    Item **pdriNo = driNoOf(this, player);
    Item **pdriYes = driYesOf(this, player);

    if (racing->value == 0) {
        (*pracingYes)->visible = 1;
        (*pracingNo)->visible = 0;
        (*pdriYes)->visible = 0;
        (*pdriNo)->visible = 1;
    } else {
        (*pracingYes)->visible = 0;
        (*pracingNo)->visible = 1;
        (*pdriYes)->visible = 1;
        (*pdriNo)->visible = 0;
    }
}

/* State 8: as transmissionInput. */
// FUNC 0x00108210 gt2k_menu::ArcadeControl::racingInput
int ArcadeControl::racingInput(int *result, int player)
{
    /* The NULL CheckFn 0x00350CB8; the NULL onSel2Change cleared by memset. */
    /* implicit: memset (temporary: the NULL pointer-to-member argument) */
    int ended;
    if (player != 2)
        ended = selectInput(player, result, &control1, &racing1,
                            &ArcadeControl::onRacingChange, 0, NULL, 0);
    else
        ended = selectInput(2, result, &control2, &racing2,
                            &ArcadeControl::onRacingChange, 0, NULL, 0);
    return ended;
}

// FUNC 0x00108310 gt2k_menu::ArcadeControl::closeRacing
void ArcadeControl::closeRacing(int player)
{
    Item **pracingYes = racingYesOf(this, player);
    Item **pracingNo = racingNoOf(this, player);
    Item **pdriNo = driNoOf(this, player);
    Item **pdriYes = driYesOf(this, player);

    arcade_fade_out(*pracingNo);
    *pracingNo = NULL;
    arcade_fade_out(*pracingYes);
    *pracingYes = NULL;
    arcade_fade_out(*pdriNo);
    *pdriNo = NULL;
    arcade_fade_out(*pdriYes);
    *pdriYes = NULL;
}

/* ------------------------------------------------------------------------ */
/* The enhanced engine: the assist step (Lupo 2; menus_a.md 17)             */
/* ------------------------------------------------------------------------ */
/* Lupo 2's car selects let each player choose an assist level after the
 * transmission (and racing) step: 0 TCS + ASM, 1 TCS only, 2 none
 * (GameOptions::getAssist's presets).  The items come from the page's
 * layout; GT3's menu/arcade.mbl has none, and then the step is skipped
 * (hasAssistStep) and the options decide, as in GT3. */

/* The items' names (Lupo 2's strings L2 0x00335EF0..0x00335FC0). */
static const char *const g_assistTitleName[2] = { "assist_title", "assist_title_2p" };
static const char *const g_assistYesName[2][3] = {
    { "assist_0_yes", "assist_1_yes", "assist_2_yes" },
    { "assist_0_yes_2p", "assist_1_yes_2p", "assist_2_yes_2p" },
};
static const char *const g_assistNoName[2][3] = {
    { "assist_0_no", "assist_1_no", "assist_2_no" },
    { "assist_0_no_2p", "assist_1_no_2p", "assist_2_no_2p" },
};

/* The step exists for the player when the current page's layout has its
 * title item (in the "hidden" layer, where Item(name, layout) finds it). */
int ArcadeControl::hasAssistStep(int player)
{
    PageLayout *layout = pages.current;

    return layout != NULL && layout->findHiddenItem(g_assistTitleName[player == 2]) != NULL;
}

/* The title, then the "yes" and the "no" item of each level, in the current
 * layer (Lupo 2 L2 0x0010BC18; Lupo 2's "yes" items are a blinking Item
 * kind, plain Items here). */
void ArcadeControl::openAssist(int player)
{
    int p = (player == 2);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Transitive *layer = pm->layers[pm->popupLevel];

    printf("menus: assist step, player %d\n", p + 1);
    assistTitle[p] = new Item(g_assistTitleName[p], layout);
    assistTitle[p]->attachTo(layer);
    for (int i = 0; i < 3; i++) {
        assistYes[p][i] = new Item(g_assistYesName[p][i], layout);
        assistYes[p][i]->attachTo(layer);
    }
    for (int i = 0; i < 3; i++) {
        assistNo[p][i] = new Item(g_assistNoName[p][i], layout);
        assistNo[p][i]->attachTo(layer);
    }
    onAssistChange(player);
}

/* Change callback: only the chosen level's "yes" is shown (L2 0x0010BE60). */
void ArcadeControl::onAssistChange(int player)
{
    int p = (player == 2);
    SmartIndexT<int> *assist = (player != 2) ? &assist1 : &assist2;

    for (int i = 0; i < 3; i++)
        assistYes[p][i]->visible = 0;
    assistYes[p][assist->value]->visible = 1;
}

/* Left / right over the level (L2 0x0010BEE0). */
int ArcadeControl::assistInput(int *result, int player)
{
    if (player != 2)
        return selectInput(player, result, &control1, &assist1,
                           &ArcadeControl::onAssistChange, 0, NULL, 0);
    return selectInput(2, result, &control2, &assist2,
                       &ArcadeControl::onAssistChange, 0, NULL, 0);
}

/* L2 0x0010BFF0. */
void ArcadeControl::closeAssist(int player)
{
    int p = (player == 2);

    if (assistTitle[p] == NULL)
        return;
    arcade_fade_out(assistTitle[p]);
    assistTitle[p] = NULL;
    for (int i = 0; i < 3; i++) {
        arcade_fade_out(assistYes[p][i]);
        assistYes[p][i] = NULL;
        arcade_fade_out(assistNo[p][i]);
        assistNo[p][i] = NULL;
    }
}

/* The racing / dri step is not shown on a dirt course or for the garage
 * classes (the test of state 6 below, player 1's class for both players as
 * there). */
static inline int racing_step_skipped(ArcadeControl *ctl)
{
    return ctl->isDirtCourse() || ctl->class1.value == ARCADE_CLASS_HOME_GARAGE ||
           ctl->class1.value == ARCADE_CLASS_GUEST_GARAGE;
}

/* One frame of one player's state machine (see the top of the file).
 * Returns 1 when the page is over for both: state 0 (this player backed out,
 * *result = 0) or state 10 (*result = 1).
 * The enhanced engine: states 11 / 12, the assist step (open / input),
 * between the racing (or transmission) step and the wait of state 9, when
 * the layout has it (menus_a.md 17). */
// FUNC 0x001083B0 gt2k_menu::ArcadeControl::carSelect2PStep
int ArcadeControl::carSelect2PStep(int *state, int otherState, int *result, int player)
{
    int done = 0;

    switch (*state) {
    case 0:
        *result = 0;
        done = 1;
        break;
    case 1:
        openClassList(player);
        (*state)++;
        /* fall through */
    case 2:
        if (classListInput(result, player) == 0)
            break;
        closeClassList(player);
        if (*result == 0) {
            *state = 0;
            break;
        }
        (*state)++;
        openCarView(player);
        break;
    case 3:
        openCarList(player);
        (*state)++;
        /* fall through */
    case 4:
        if (carListInput(result, player) == 0)
            break;
        closeCarList(player);
        if (*result != 0) {
            (*state)++;
            break;
        }
        /* the enhanced engine: the Pod race has no class step (carSelect2P)
         * and backs out of the page instead */
        *state = (mode == ARCADE_POD_RACE) ? 0 : 1;
        closeCarView(player);
        break;
    case 5:
        openTransmission(player);
        (*state)++;
        /* fall through */
    case 6:
        if (transmissionInput(result, player) == 0)
            break;
        closeTransmission(player);
        if (*result == 0) {
            *state = 3;
            break;
        }
        /* Quirk: the class tested is player 1's (class1) for both
         * players. */
        if (isDirtCourse() || class1.value == ARCADE_CLASS_HOME_GARAGE ||
            class1.value == ARCADE_CLASS_GUEST_GARAGE)
            *state = hasAssistStep(player) ? 11 : 9;    /* the enhanced engine: 11 */
        else
            (*state)++;
        break;
    case 7:
        openRacing(player);
        (*state)++;
        /* fall through */
    case 8:
        if (racingInput(result, player) == 0)
            break;
        closeRacing(player);
        if (*result == 0)
            *state = 5;
        else if (hasAssistStep(player))     /* the enhanced engine */
            *state = 11;
        else
            (*state)++;
        break;
    case 9: {
        /* Quirk: cancel goes back to the racing option (7) for the garage
         * classes too, although they skipped it. */
        MenuControl *pad = controlOf(this, player);
        if (pad->cancel != 0) {
            if (hasAssistStep(player))      /* the enhanced engine: the assist step */
                *state = 11;
            else if (isDirtCourse())
                *state = 5;
            else
                *state = 7;
            menu_play_se(MENU_SE_CANCEL);
            break;
        }
        if (otherState == 9)
            (*state)++;
        break;
    }
    case 10:
        closeCarView(player);
        *result = 1;
        done = 1;
        break;
    case 11:                                /* the enhanced engine: the assist step */
        openAssist(player);
        (*state)++;
        /* fall through */
    case 12:
        if (assistInput(result, player) == 0)
            break;
        closeAssist(player);
        if (*result == 0) {
            *state = racing_step_skipped(this) ? 5 : 7;
            break;
        }
        assistChosen[player == 2] = 1;
        *state = 9;
        break;
    }
    return done;
}

/* Closes what `state' has open when the other player ended the page.  The
 * resting states 3, 5 and 7 (the setup of the next step not yet run) close
 * nothing, not even the car view of state 3 (? quirk: openCarView ran in
 * state 2). */
// FUNC 0x00108650 gt2k_menu::ArcadeControl::carSelect2PAbort
void ArcadeControl::carSelect2PAbort(int state, int player)
{
    switch (state) {
    case 1:
        break;
    case 2:
        closeClassList(player);
        break;
    case 3:
        break;
    case 4:
        closeCarList(player);
        closeCarView(player);
        break;
    case 5:
        break;
    case 6:
        closeTransmission(player);
        closeCarView(player);
        break;
    case 7:
        break;
    case 8:
        closeRacing(player);
        closeCarView(player);
        break;
    case 9:
        closeCarView(player);
        break;
    case 10:
        break;
    case 11:                                /* the enhanced engine (the assist step) */
        break;
    case 12:
        closeAssist(player);
        closeCarView(player);
        break;
    }
}

/* The page (runTwoPlayer, T010).  The movie g_00292930[3]
 * ("stream/menu_bg2.pss"), a loader without a buffer of its own for the cars
 * (async_loader_create(0)), both state machines from state 1.  Returns the
 * result of the player who ended it (1 = both chose, 0 = backed out);
 * `result' comes back unchanged if the page ends by timedOut (a lost link,
 * ControlBase's timeout). */
// FUNC 0x00108710 gt2k_menu::ArcadeControl::carSelect2P
int ArcadeControl::carSelect2P(int result)
{
    PageManager *pm = &pages;

    pm->setPage("ar/select_car_2p", NULL);
    pm->startPage(NULL);
    async_loader_create(0);
    g_002CDC10.start(&g_00292930[3]);
    menu_car_clear_loading();
    transitionIn();
    int state1 = 1;
    int state2 = 1;
    if (mode == ARCADE_POD_RACE) {
        /* The enhanced engine (menus_a.md 9 / 14): the Pod race's one class
         * needs no class step; both players start at their car list (what
         * state 2 does when its class is decided) */
        openCarView(1);
        openCarView(2);
        state1 = 3;
        state2 = 3;
    }
    gfx_vsync(10);

    int done;
    do {
        done = carSelect2PStep(&state1, state2, &result, 1);
        if (done != 0) {
            carSelect2PAbort(state2, 2);
        } else {
            done = carSelect2PStep(&state2, state1, &result, 2);
            if (done != 0)
                carSelect2PAbort(state1, 1);
        }
        waitFrame();
        if (timedOut != 0)
            break;
    } while (done == 0);

    transitionOut();
    async_loader_destroy();
    pm->closeAndExit();
    gfx_vsync(2);
    return result;
}

} /* namespace gt2k_menu */
