/* T023 - the arcade car select screen.  0x00109F20..0x0010C9E0.
 *
 * Methods of gt2k_menu::ArcadeControl (declared with the class in
 * T010_arcade_control.h; T018..T023 add more of its methods: isCarLocked and
 * miniCarImageName are also pointers to members for T021, .sdata 0x00350C90
 * / 0x00350C78) and gt2k_menu::TransitiveEX.
 *
 * runCarSelect (called from T010) opens the "ar/select_car" page, builds the
 * screen (buildCarSelect) and runs a small state machine (CAR_SELECT_*):
 *   1 car and colour (selectCarAndColor), 2 transmission (selectTransmission),
 *   3 racing_* / dri_* choice (selectRacing; skipped for the garage classes
 *   and on a dirt course), 4 nothing, 0 / 5 leave with result 0 / 1.
 * Everything is placed on a "cross_bar" (Cross) that scrolls sideways from
 * car to car (680 per car) and down from row to row (scrollToRow).
 *
 * The cars come from the product database or from a garage, after the
 * player's class: ARCADE_CLASS_HOME_GARAGE = getGameStatus()'s garage,
 * ARCADE_CLASS_GUEST_GARAGE = the guest's (getGameStatusHolder()->get()),
 * any other class (0..4, 7, 8) the ProductConfig cars of the category
 * getPlayerClassName(player).  A player argument of 2 means player 2,
 * anything else player 1.  The page itself is player 1's (the two-player
 * select is T021). */
#define U004_T023                           /* arcade_car_select_internal.h: this unit's declarations */
/* .rodata opens with the debug format (unreferenced; it was counted to T035). */
// DATA_START 0x0030A780
#include "game/menu/arcade/arcade_car_select_internal.h"
#include "game/menu/arcade/T023_arcade_car_select.h"

namespace gt2k_menu {

/* No callers.  0 for the garage classes and the preview class.  The last two
 * tests grouped: written as one chain, fold merges the first two into a
 * range test (cls - 5 < 2) that retail does not have. */
// FUNC 0x00109F20 gt2k_menu::ArcadeControl::F_00109F20
int ArcadeControl::F_00109F20(int player)
{
    int cls = classOf(this, player)->value;

    return cls != ARCADE_CLASS_HOME_GARAGE &&
           (cls != ARCADE_CLASS_GUEST_GARAGE && cls != ARCADE_CLASS_PREVIEW);
}

/* Number of cars the player's class offers; -1 for an unknown class. */
// FUNC 0x00109F58 gt2k_menu::ArcadeControl::carCount
int ArcadeControl::carCount(int player)
{
    switch (classOf(this, player)->value) {
    case ARCADE_CLASS_C: case ARCADE_CLASS_B: case ARCADE_CLASS_A: case ARCADE_CLASS_S:
    case ARCADE_CLASS_DIRT: case ARCADE_CLASS_COMPULSORY: case ARCADE_CLASS_PREVIEW:
    case ARCADE_CLASS_POD:                  /* the enhanced engine: the Pod race's cars */ {
        ProductConfig *config = product_config();
        return config->carCount(getPlayerClassName(player));
    }
    case ARCADE_CLASS_HOME_GARAGE:
        return getGameStatus()->getCarNum();
    case ARCADE_CLASS_GUEST_GARAGE:
        return getGameStatusHolder()->get()->getCarNum();
    }
    return -1;
}

/* Code of car `car' (the current one when car < 0): the database code, or
 * the garage car's name; NULL for an unknown class. */
// FUNC 0x0010A028 gt2k_menu::ArcadeControl::carCode
const char *ArcadeControl::carCode(int player, int car)
{
    SmartIndexT<int> *cls = classOf(this, player);
    SmartCarIndex *current = carOf(this, player);

    if (car < 0)
        car = current->value;
    switch (cls->value) {
    case ARCADE_CLASS_C: case ARCADE_CLASS_B: case ARCADE_CLASS_A: case ARCADE_CLASS_S:
    case ARCADE_CLASS_DIRT: case ARCADE_CLASS_COMPULSORY: case ARCADE_CLASS_PREVIEW:
    case ARCADE_CLASS_POD:                  /* the enhanced engine: the Pod race's cars */ {
        ProductConfig *config = product_config();
        return config->carCode(car, getPlayerClassName(player));
    }
    case ARCADE_CLASS_HOME_GARAGE:
        return getGameStatus()->getCar(car)->getName();
    case ARCADE_CLASS_GUEST_GARAGE:
        return getGameStatusHolder()->get()->getCar(car)->getName();
    }
    return NULL;
}

/* Loads the current car into the player's MenuCarSpec: a database car by
 * its code, a garage car as its CarInformation. */
// FUNC 0x0010A128 gt2k_menu::ArcadeControl::loadCarInfo
void ArcadeControl::loadCarInfo(int player)
{
    int cls = classOf(this, player)->value;
    int car = carOf(this, player)->value;
    MenuCarSpec *spec = specOf(this, player);

    switch (cls) {
    case ARCADE_CLASS_C: case ARCADE_CLASS_B: case ARCADE_CLASS_A: case ARCADE_CLASS_S:
    case ARCADE_CLASS_DIRT: case ARCADE_CLASS_COMPULSORY: case ARCADE_CLASS_PREVIEW:
    case ARCADE_CLASS_POD:                  /* the enhanced engine: the Pod race's cars */ {
        ProductConfig *config = product_config();
        spec->setCar(config->carCode(car, getPlayerClassName(player)));
        break;
    }
    case ARCADE_CLASS_HOME_GARAGE:
        spec->setCar(garageCarInfo(getGameStatus()->getCar(car)));
        break;
    case ARCADE_CLASS_GUEST_GARAGE:
        spec->setCar(garageCarInfo(getGameStatusHolder()->get()->getCar(car)));
        break;
    }
}

/* The car's name, which names its pictures (the logo item, and with
 * "_mini" its mini car icon): CarEquipments::getShortName of the car `code'
 * loaded into a local CarEquipments; "" if there is no such car.  The EH
 * landing pads run ~CarEquipments. */
// FUNC 0x0010A250 gt2k_menu::ArcadeControl::carName
const char *ArcadeControl::carName(const char *code)
{
    CarEquipments equip;

    if (equip.setCar(nameToCode(code)) != 0)
        return equip.getShortName();
    return "";
}

/* 1 if car `car' cannot be chosen (CheckFn; also T021's, through the
 * pointer to member at 0x00350C90).  Garage cars are always allowed, except
 * on a dirt course: then only those owning the dirt tyres.  Database cars:
 * ProductConfig::carSelectable. */
// FUNC 0x0010A2F0 gt2k_menu::ArcadeControl::isCarLocked
int ArcadeControl::isCarLocked(int car, int player)
{
    SmartIndexT<int> *cls = classOf(this, player);
    int dirt = isDirtCourse();

    if (cls->value == ARCADE_CLASS_HOME_GARAGE) {
        if (dirt != 0)
            return getGameStatus()->getCar(car)->hasPart(PART_FRONT_TIRE_DIRT) == 0;
        return 0;
    } else if (cls->value == ARCADE_CLASS_GUEST_GARAGE) {
        if (dirt != 0)
            return getGameStatusHolder()->get()->getCar(car)->hasPart(PART_FRONT_TIRE_DIRT) == 0;
        return 0;
    } else {
        ProductConfig *config = product_config();
        return config->carSelectable(car, getPlayerClassName(player)) ^ 1;
    }
}

/* The current garage car; NULL for database cars (and unknown classes). */
// FUNC 0x0010A3E0 gt2k_menu::ArcadeControl::garageCar
GarageCar *ArcadeControl::garageCar(int player)
{
    int cls = classOf(this, player)->value;
    int car = carOf(this, player)->value;

    switch (cls) {
    case ARCADE_CLASS_C: case ARCADE_CLASS_B: case ARCADE_CLASS_A: case ARCADE_CLASS_S:
    case ARCADE_CLASS_DIRT: case ARCADE_CLASS_COMPULSORY: case ARCADE_CLASS_PREVIEW:
    case ARCADE_CLASS_POD:                  /* the enhanced engine: the Pod race's cars */
        return NULL;
    case ARCADE_CLASS_HOME_GARAGE:
        return getGameStatus()->getCar(car);
    case ARCADE_CLASS_GUEST_GARAGE:
        return getGameStatusHolder()->get()->getCar(car);
    }
    return NULL;
}

/* The enhanced engine (new, Lupo 2 L2 0x0010CDE8 isCarCVT; menus_a.md 39):
 * whether the player's current car is a CVT (CarEquipments::isCVT, a
 * transmission with 2 forward gears).  A garage car is asked itself (its
 * fitted transmission); a data base car through a CarEquipments of its code
 * (Lupo 2 builds every car that way).  0 when there is no car.  No GT3 car
 * is a CVT.  Callers: updateTransmissionButtons, selectTransmission (1P,
 * also the compulsory page) and T021's onTransmissionChange /
 * transmissionInput (2P). */
int ArcadeControl::isCarCVT(int player)
{
    GarageCar *gcar = garageCar(player);
    if (gcar != NULL)
        return gcar->isCVT();
    const char *code = carCode(player, -1);
    if (code == NULL)
        return 0;
    CarEquipments equip;
    if (equip.setCar(nameToCode(code)) == 0)
        return 0;
    return equip.isCVT();
}

/* A garage car keeps its colour (range [c, c + 1)); a database car offers
 * all its colours (at least one), starting with the first. */
// FUNC 0x0010A480 gt2k_menu::ArcadeControl::setupColorIndex
void ArcadeControl::setupColorIndex(MenuCarSpec *spec, int cls, SmartIndexT<int> *color)
{
    if (cls == ARCADE_CLASS_HOME_GARAGE || cls == ARCADE_CLASS_GUEST_GARAGE) {
        int colorIndex = spec->getEquipments()->colorIndex;

        color->setRange(colorIndex, colorIndex + 1);
        color->set(colorIndex);
    } else {
        int count = spec->getColorCount();

        if (count == 0)
            color->setRange(0, 1);
        else
            color->setRange(0, count);
        color->set(0);
    }
}

// FUNC 0x0010A528 gt2k_menu::ArcadeControl::resetColorIndex
void ArcadeControl::resetColorIndex(int player)
{
    if (player != ARCADE_PLAYER_2)
        setupColorIndex(&car1Spec, class1.value, &color1);
    else
        setupColorIndex(&car2Spec, class2.value, &color2);
}

/* Scrolls the cross bar to option row `row' (0 the car, 1 transmission,
 * 2 racing / dri, 3 the unused opt1d row of F_0010C5A0).  Retail's constant
 * (.lit4 0x0034FA20) is 0x43066667, 134.40001, one step above 134.4f
 * (0x43066666): a folded product, perhaps 448 * 0.3 (0.3 of F_001051B0's
 * 448); written as its value. */
// FUNC 0x0010A570 gt2k_menu::ArcadeControl::scrollToRow
void ArcadeControl::scrollToRow(int row)
{
    if (row == 0)
        crossBar->targetY = crossBarY;
    else
        crossBar->targetY = crossBarY - F_001051B0(row) - 134.40001f;
}

/* Shows the current colour on the car and renews the colour name (the old
 * one goes through arcade_fade_out). */
// FUNC 0x0010A5C8 gt2k_menu::ArcadeControl::applyColor
void ArcadeControl::applyColor()
{
    carView->setColor(color1.value);
    arcade_fade_out(colorName);
    createColorName();
}

/* Player 1's 3D car: the hidden "car" item's texture set as its environment
 * image, player 1's spec and model buffer, the current colour, a garage
 * car's wheel, the "car" item's draw order; the compulsory mode's camera
 * preset on the compulsory page (T018 calls this too). */
// FUNC 0x0010A608 gt2k_menu::ArcadeControl::createCarView
Car *ArcadeControl::createCarView(const char *code)
{
    /* Object::operator new (0x720 bytes, the menu heap); Object::operator
     * delete if the constructor throws */
    /* implicit: operator new, operator delete (new-expression) */
    Car *car = new Car(carItem->texSet, &car1Spec, 0, carBuffer1);
    GarageCar *garage;

    car->setCar(code);
    car->setColor(color1.value);
    garage = garageCar(0);
    if (garage != NULL)
        car->setWheel(garage->getWheelModelName());
    car->z = carItem->z;
    if (mode == ARCADE_COMPULSORY)
        car->camera.setType(CAR_CAMERA_COMPULSORY);
    else
        car->camera.setType(CAR_CAMERA_ARCADE);
    return car;
}

/* The colour chips of the player's car ("color_chip", "color_chip_2p"),
 * over its colour index. */
// FUNC 0x0010A700 gt2k_menu::ArcadeControl::createColorChips
SelectBox *ArcadeControl::createColorChips(int player)
{
    SmartIndexT<int> *color = colorOf(this, player);
    MenuCarSpec *spec = specOf(this, player);
    const char *image;
    PageLayout *layout;
    u32 chipColors[64];

    if (player != ARCADE_PLAYER_2)
        image = "color_chip";
    else
        image = "color_chip_2p";
    layout = pages.current;
    spec->getColors(chipColors, 0);
    /* Object::operator new (0x420 bytes); Object::operator delete if the
     * constructor throws */
    /* implicit: operator new, operator delete (new-expression) */
    return new SelectBox(image, color, chipColors, layout);
}

/* "ff", "fr", "four_wd", "mr" or "rr" (+ "_2p" for player 2) after the
 * drive train. */
// FUNC 0x0010A7D0 gt2k_menu::ArcadeControl::createDriveTrainIcon
Item *ArcadeControl::createDriveTrainIcon(int player)
{
    PageLayout *layout = pages.current;
    const char *image;
    char name2P[32];
    /* Two calls (cross-jumped into one jal, the spec's address set in each
     * arm): a ?: of the two addresses would be a movn. */
    int drive = (player != ARCADE_PLAYER_2) ? car1Spec.car.driveName() : car2Spec.car.driveName();

    /* FR has its own case (its test is left in retail as a dead li after
     * jump2 merged its body into the default's); "fr" is the first string
     * of the object's .sdata. */
    switch (drive) {
    case DRIVE_FR:
        image = "fr";
        break;
    case DRIVE_FF:
        image = "ff";
        break;
    case DRIVE_4WD:
        image = "four_wd";
        break;
    case DRIVE_MR:
        image = "mr";
        break;
    case DRIVE_RR:
        image = "rr";
        break;
    default:
        image = "fr";
        break;
    }
    /* Object::operator new (0x100 bytes); Object::operator delete if the
     * constructor throws */
    /* implicit: operator new, operator delete (new-expression) */
    if (player != ARCADE_PLAYER_2)
        return new Item(image, layout);
    sprintf(name2P, "%s_2p", image);
    return new Item(name2P, layout);
}

/* The spec Strings: F0 (the characters shown) 0, 0.25 s, then F0 + 16 / s
 * for 2 s (the text types itself out).  Static: retail has it after the
 * typeinfo names, at the end of T023's .rodata, where egcs writes a const
 * with internal linkage (deferred to the end of the file). */
static const OperatorCode g_0030B1E0[] = {  /* 0x0030B1E0 */
    U004_OP(OPC_SET_8C, 0,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_WAIT,   0,  0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_ADD_8C, 0, 16.0f, 0.0f, 0.0f, 0.0f, 2.0f, 1),
    U004_OP(OPC_REMOVE, 0,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1)
};

/* "<power><unit>/<rpm>" (the units of g_002F59D8), or "----" when the
 * power is 0; typed out by g_0030B1E0. */
// FUNC 0x0010A938 gt2k_menu::ArcadeControl::createPowerText
String *ArcadeControl::createPowerText(int player)
{
    MenuCarSpec *spec = specOf(this, player);
    PageLayout *layout = pages.current;
    String *text = createLabel((player != ARCADE_PLAYER_2) ? "ps" : "ps_2p", layout, 24.0f);
    int power;
    char buf[32];

    text->align = 12;
    power = spec->getEquipments()->getMaxPower();
    if (power != 0) {
        const char *rpm = spec->getEquipments()->getMaxPowerRpmText();
        /* the unit before the call: retail loads the format after it */
        const char *unit = g_002F59D8.powerName();

        sprintf(buf, "%d%s/%s", power, unit, rpm);
    } else {
        sprintf(buf, "----");
    }
    text->setText(buf);
    text->start(g_0030B1E0);
    return text;
}

/* "<torque><unit>/<rpm>", or "----".  fmt_sprintf's %.2t prints the integer
 * with a decimal point before its last two digits. */
// FUNC 0x0010AA40 gt2k_menu::ArcadeControl::createTorqueText
String *ArcadeControl::createTorqueText(int player)
{
    MenuCarSpec *spec = specOf(this, player);
    PageLayout *layout = pages.current;
    String *text = createLabel((player != ARCADE_PLAYER_2) ? "kgm" : "kgm_2p", layout, 24.0f);
    int torque;
    char buf[32];

    text->align = 12;
    torque = spec->getEquipments()->getMaxTorque();
    if (torque != 0) {
        const char *rpm = spec->getEquipments()->getMaxTorqueRpmText();
        /* (the format loaded before torqueName's call, unlike createPowerText) */
        fmt_sprintf(buf, "%.2t%s/%s", torque, g_002F59D8.torqueName(), rpm);
    } else {
        sprintf(buf, "----");
    }
    text->setText(buf);
    text->start(g_0030B1E0);
    return text;
}

/* "handling", "speed", "acceleration". */
// FUNC 0x0010AB58 gt2k_menu::ArcadeControl::createSpecLabel
String *ArcadeControl::createSpecLabel(const char *name)
{
    String *label = createLabel(name, pages.current, 24.0f);

    label->align = 12;
    label->start(g_0030B1E0);
    return label;
}

/* "color_name" with the name of player 1's current colour as its text (none
 * if the name starts with '-'), on the cross bar, in the current layer;
 * Object::operator new (0x100 bytes). */
// FUNC 0x0010ABA8 gt2k_menu::ArcadeControl::createColorName
void ArcadeControl::createColorName()
{
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Object *layer = pm->layers[pm->popupLevel];
    const char *name;

    colorName = new Item("color_name", layout); /* implicit: Object::operator delete (if the constructor throws) */
    name = car1Spec.getColorName(color1.value);
    if (name != NULL && name[0] == '-')
        name = NULL;
    colorName->setText(name);
    attachToBar(colorName, crossBar);
    colorName->attachTo(layer);
}

/* A dimension text: colours (0, 0, 0, 0), alpha in at 1 / s for 0.5 s, 3 s,
 * out again, then kill(). */
const OperatorCode g_0030A898[] = {         /* 0x0030A898 */
    U004_OP(OPC_SET_COLORS,  0, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0),
    U004_OP(OPC_FADE_COLORS, 0, 0.0f, 0.0f, 0.0f,  1.0f, 0.5f, 1),
    U004_OP(OPC_WAIT,        0, 3.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1),
    U004_OP(OPC_FADE_COLORS, 0, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 1),
    U004_OP(OPC_RELEASE,     0, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};

/* A dimension text: at (100, 250), 21 x 17, rising 40 / s for 8 s. */
const OperatorCode g_0030A988[] = {         /* 0x0030A988 */
    U004_OP(OPC_SET_RECT, 0, 100.0f, 250.0f, 21.0f, 17.0f, 0.0f, 0),
    U004_OP(OPC_MOVE,     0,   0.0f, -40.0f,  8.0f,  0.0f, 0.0f, 1),
    U004_OP(OPC_REMOVE,   0,   0.0f,   0.0f,  0.0f,  0.0f, 0.0f, 1)
};

/* A String child of the spec panel: font "sw_20#1", fading in and out
 * (g_0030A898) while it rises (g_0030A988), z 4. */
// FUNC 0x0010AC90 gt2k_menu::specPanelAddText
void specPanelAddText(TransitiveEX *panel, const char *text)
{
    /* Object::operator new (0x114 bytes); Object::operator delete if the
     * constructor throws */
    /* implicit: operator new, operator delete (new-expression) */
    String *line = new String(text, "sw_20#1");

    line->start(g_0030A898);
    line->start(g_0030A988);
    line->z = 4.0f;
    line->attachTo(panel);
}

/* The four dimensions, called in turn by the panel's script g_0030AA58
 * (opCall records 0x0030AA88 / 0x0030AAE8 / 0x0030AB48 / 0x0030ABA8:
 * length, height, width, wheel base).  The values are read as s16. */
// FUNC 0x0010AD48 gt2k_menu::specPanelShowLength
void specPanelShowLength(TransitiveEX *panel)
{
    char buf[32];

    sprintf(buf, "Length\n%dmm", panel->info->car.spec.length);
    specPanelAddText(panel, buf);
}

// FUNC 0x0010AD90 gt2k_menu::specPanelShowWidth
void specPanelShowWidth(TransitiveEX *panel)
{
    char buf[32];

    sprintf(buf, "Width\n%dmm", panel->info->car.spec.width);
    specPanelAddText(panel, buf);
}

// FUNC 0x0010ADD8 gt2k_menu::specPanelShowHeight
void specPanelShowHeight(TransitiveEX *panel)
{
    char buf[32];

    sprintf(buf, "Height\n%dmm", panel->info->car.spec.height);
    specPanelAddText(panel, buf);
}

// FUNC 0x0010AE20 gt2k_menu::specPanelShowWheelBase
void specPanelShowWheelBase(TransitiveEX *panel)
{
    char buf[32];

    sprintf(buf, "WheelBase\n%dmm", panel->info->car.spec.wheelBase);
    specPanelAddText(panel, buf);
}

/* The spec panel, forever: 1 s, length, 1 s, height, 1 s, width, 1 s,
 * wheel base, 0.5 s, again. */
const OperatorCode g_0030AA58[] = {         /* 0x0030AA58 */
    U004_OP(OPC_WAIT,    0,                           1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_CALL,    (int)specPanelShowLength,    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_WAIT,    0,                           1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_CALL,    (int)specPanelShowHeight,    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_WAIT,    0,                           1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_CALL,    (int)specPanelShowWidth,     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_WAIT,    0,                           1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_CALL,    (int)specPanelShowWheelBase, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_WAIT,    0,                           0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_RESTART, 0,                           0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0)
};

/* The dimension panel of player 1's car; its script adds the texts. */
// FUNC 0x0010AE68 gt2k_menu::ArcadeControl::createSpecPanel
void ArcadeControl::createSpecPanel()
{
    Object *layer = pages.layers[pages.popupLevel];

    /* Object::operator new (0xC8 bytes), the inline constructors:
     * Transitive::Transitive(), info, the vtable */
    /* implicit: Transitive (base constructor, inline TransitiveEX ctor) */
    specPanel = new TransitiveEX(&car1Spec); /* implicit: Object::operator delete (if the constructor throws) */
    specPanel->start(g_0030AA58);
    specPanel->attachTo(layer);
}

/* "<car name>_mini", the picture of mini car `car' (NameFn of
 * createMiniCursor: the pointer to member at 0x00350D90; also T021's,
 * 0x00350C78). */
// FUNC 0x0010AF20 gt2k_menu::ArcadeControl::miniCarImageName
void ArcadeControl::miniCarImageName(char *buf, int car, int player)
{
    const char *name = carName(carCode(player, car));

    sprintf(buf, "%s_mini", name);
}

/* The current car changed (selectCarAndColor): reload it, reset the colour,
 * replace the Car view and every element that shows the car (the old ones
 * go through arcade_fade_out, the old chip list is frozen first; the old Car view
 * and spec panel are killed).  Object::operator new for each new Item and
 * gauge. */
// FUNC 0x0010AF78 gt2k_menu::ArcadeControl::onCarChanged
void ArcadeControl::onCarChanged()
{
    SmartIndexT<int> *car = &car1;          /* retail $s1, before carCode's call */
    const char *code = carCode(0, -1);
    PageManager *pm = &pages;
    PageLayout *layout = pm->current;
    Object *layer = pm->layers[pm->popupLevel];
    const char *name;

    loadCarInfo(0);
    resetColorIndex(0);
    color1.set(0);
    applyColor();
    carView->kill();
    carView = createCarView(code);
    carView->attachTo(layer);
    crossBar->wrapTargetX(car1.value);
    updateArrows(car);

    arcade_fade_out(logo);
    name = carName(code);
    logo = new Item(name, layout); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(logo, crossBar);
    logo->attachTo(layer);

    arcade_fade_out(colorSelection);
    colorSelection = new Item("color_selection", layout); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(colorSelection, crossBar);
    colorSelection->attachTo(layer);

    arcade_fade_out(colorChips);
    colorChips->frozen = 1;
    colorChips = createColorChips(0);
    attachToBar(colorChips, crossBar);
    colorChips->attachTo(layer);

    arcade_fade_out(driveTrain);
    driveTrain = createDriveTrainIcon(0);
    attachToBar(driveTrain, crossBar);
    driveTrain->attachTo(layer);

    arcade_fade_out(colorName);
    createColorName();

    arcade_fade_out(powerText);
    powerText = createPowerText(0);
    attachToBar(powerText, crossBar);
    powerText->attachTo(layer);

    arcade_fade_out(torqueText);
    torqueText = createTorqueText(0);
    attachToBar(torqueText, crossBar);
    torqueText->attachTo(layer);

    arcade_fade_out(handlingText);
    handlingText = createSpecLabel("handling");
    attachToBar(handlingText, crossBar);
    handlingText->attachTo(layer);

    arcade_fade_out(speedText);
    speedText = createSpecLabel("speed");
    attachToBar(speedText, crossBar);
    speedText->attachTo(layer);

    arcade_fade_out(accelText);
    accelText = createSpecLabel("acceleration");
    attachToBar(accelText, crossBar);
    accelText->attachTo(layer);

    arcade_fade_out(handlingGauge);
    handlingGauge = new CircleGauge(&car1Spec, "handling_gauge", layout, 0); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(handlingGauge, crossBar);
    handlingGauge->attachTo(layer);

    arcade_fade_out(speedGauge);
    speedGauge = new CircleGauge(&car1Spec, "speed_gauge", layout, 1); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(speedGauge, crossBar);
    speedGauge->attachTo(layer);

    arcade_fade_out(accelGauge);
    accelGauge = new CircleGauge(&car1Spec, "acceleration_gauge", layout, 2); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(accelGauge, crossBar);
    accelGauge->attachTo(layer);

    specPanel->kill();
    createSpecPanel();
    startCarViewMotion(0);
}

/* The Car view: colours (1, 1, 1, 0), alpha in at 1 / s for 1 s. */
const OperatorCode g_0030ACA0[] = {         /* 0x0030ACA0 */
    U004_OP(OPC_SET_COLORS,  0, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_FADE_COLORS, 0, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1),
    U004_OP(OPC_REMOVE,      0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1)
};

/* The Car view fades in (g_0030ACA0); player 2's for T018 / T021. */
// FUNC 0x0010B428 gt2k_menu::ArcadeControl::startCarViewMotion
void ArcadeControl::startCarViewMotion(int player)
{
    ((player != ARCADE_PLAYER_2) ? carView : carView2)->start(g_0030ACA0);
}

/* auto_yes + manu_no for automatic (0), auto_no + manu_yes for manual.
 * The enhanced engine (Lupo 2 L2 0x0010E3B0): a CVT shows automatic. */
// FUNC 0x0010B460 gt2k_menu::ArcadeControl::updateTransmissionButtons
void ArcadeControl::updateTransmissionButtons()
{
    if (transmission1.value == 0 || isCarCVT(0) != 0) {
        autoYes->visible = 1;
        autoNo->visible  = 0;
        manuYes->visible = 0;
        manuNo->visible  = 1;
    } else {
        autoYes->visible = 0;
        autoNo->visible  = 1;
        manuYes->visible = 1;
        manuNo->visible  = 0;
    }
}

/* racing_yes + dri_no for 0, racing_no + dri_yes for 1. */
// FUNC 0x0010B4B8 gt2k_menu::ArcadeControl::updateRacingButtons
void ArcadeControl::updateRacingButtons()
{
    if (racing1.value == 0) {
        racingYes->visible = 1;
        racingNo->visible  = 0;
        driYes->visible    = 0;
        driNo->visible     = 1;
    } else {
        racingYes->visible = 0;
        racingNo->visible  = 1;
        driYes->visible    = 1;
        driNo->visible     = 0;
    }
}

/* Builds the whole screen in the current layer: the cross bar, the hidden
 * "car" item and the Car view, arrow_l / arrow_r, the car's logo, the mini
 * cars (the locked ones marked "x"), the colour chips, drive train, colour
 * name, specs, gauges, the dimension panel, the blur, and the auto / manu
 * (row 1) and racing / dri (row 2) buttons (x 220 / 420, y relative to the
 * bar).  Object::operator new for every element (Object::operator delete if
 * a constructor throws). */
// FUNC 0x0010B510 gt2k_menu::ArcadeControl::buildCarSelect
void ArcadeControl::buildCarSelect()
{
    const char *code = carCode(0, -1);
    PageLayout *layout;
    Object *layer;
    const char *name;
    Cross *bar;
    PageManager *pm;
    int i;

    loadCarInfo(0);
    pm = &pages;
    layout = pm->current;
    layer  = pm->layers[pm->popupLevel];
    resetColorIndex(0);

    crossBar = new Cross(0); /* implicit: Object::operator delete (if the constructor throws) */
    crossBar->setResource("cross_bar", layout);
    bar = crossBar;
    /* MotionValue::get(): retail reads y through &bar->y (twice: the
     * targetY store may alias it) */
    bar->targetX = bar->x.get();
    bar->targetY = bar->y.get();
    crossBarY = bar->y.get();
    bar->attachTo(layer);
    crossBar->jumpX(car1.value);

    carItem = new Item("car", layout); /* implicit: Object::operator delete (if the constructor throws) */
    carItem->visible = 0;
    carItem->attachTo(layer);

    carView = createCarView(code);
    carView->attachTo(layer);

    createArrows(&car1, layout, layer);

    name = carName(code);
    logo = new Item(name, layout); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(logo, crossBar);
    logo->attachTo(layer);

    miniCars = createMiniCursor("mini_cars", &car1, &ArcadeControl::miniCarImageName, 0);
    miniCars->setMark("x", layout);
    for (i = 0; i < car1.size(); i++) {
        if (isCarLocked(i, 0) != 0)
            miniCars->setMarked(i, 1);
    }
    attachToBar(miniCars, crossBar);
    miniCars->attachTo(layer);

    colorSelection = new Item("color_selection", layout); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(colorSelection, crossBar);
    colorSelection->attachTo(layer);

    colorChips = createColorChips(0);
    attachToBar(colorChips, crossBar);
    colorChips->attachTo(layer);

    driveTrain = createDriveTrainIcon(0);
    attachToBar(driveTrain, crossBar);
    driveTrain->attachTo(layer);

    /* The first colour name runs no script (its fade-in stays put) until
     * selectCarAndColor's applyColor replaces it: sleep()'s store. */
    createColorName();
    colorName->active = 0;

    powerText = createPowerText(0);
    attachToBar(powerText, crossBar);
    powerText->attachTo(layer);

    torqueText = createTorqueText(0);
    attachToBar(torqueText, crossBar);
    torqueText->attachTo(layer);

    handlingText = createSpecLabel("handling");
    attachToBar(handlingText, crossBar);
    handlingText->attachTo(layer);

    speedText = createSpecLabel("speed");
    attachToBar(speedText, crossBar);
    speedText->attachTo(layer);

    accelText = createSpecLabel("acceleration");
    attachToBar(accelText, crossBar);
    accelText->attachTo(layer);

    handlingGauge = new CircleGauge(&car1Spec, "handling_gauge", layout, 0); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(handlingGauge, crossBar);
    handlingGauge->attachTo(layer);

    speedGauge = new CircleGauge(&car1Spec, "speed_gauge", layout, 1); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(speedGauge, crossBar);
    speedGauge->attachTo(layer);

    accelGauge = new CircleGauge(&car1Spec, "acceleration_gauge", layout, 2); /* implicit: Object::operator delete (if the constructor throws) */
    attachToBar(accelGauge, crossBar);
    accelGauge->attachTo(layer);

    createSpecPanel();
    createBlur();

    /* row 1: transmission */
    autoNo = new Item("auto_no", layout); /* implicit: Object::operator delete (if the constructor throws) */
    autoNo->x.set(220.0f);
    autoNo->y.set(F_001051B0(1));
    autoNo->anchor = crossBar;
    autoNo->anchorType = ANCHOR_Y;
    autoNo->attachTo(layer);

    autoYes = new Item("auto_yes", layout); /* implicit: Object::operator delete (if the constructor throws) */
    autoYes->x.set(220.0f);
    autoYes->y.set(F_001051B0(1));
    autoYes->anchor = crossBar;
    autoYes->anchorType = ANCHOR_Y;
    autoYes->attachTo(layer);

    manuNo = new Item("manu_no", layout); /* implicit: Object::operator delete (if the constructor throws) */
    manuNo->x.set(420.0f);
    manuNo->y.set(F_001051B0(1));
    manuNo->anchor = crossBar;
    manuNo->anchorType = ANCHOR_Y;
    manuNo->attachTo(layer);

    manuYes = new Item("manu_yes", layout); /* implicit: Object::operator delete (if the constructor throws) */
    manuYes->x.set(420.0f);
    manuYes->y.set(F_001051B0(1));
    manuYes->anchor = crossBar;
    manuYes->anchorType = ANCHOR_Y;
    manuYes->attachTo(layer);

    updateTransmissionButtons();

    /* row 2: racing / dri */
    racingNo = new Item("racing_no", layout); /* implicit: Object::operator delete (if the constructor throws) */
    racingNo->x.set(220.0f);
    racingNo->y.set(F_001051B0(2));
    racingNo->anchor = crossBar;
    racingNo->anchorType = ANCHOR_Y;
    racingNo->attachTo(layer);

    racingYes = new Item("racing_yes", layout); /* implicit: Object::operator delete (if the constructor throws) */
    racingYes->x.set(220.0f);
    racingYes->y.set(F_001051B0(2));
    racingYes->anchor = crossBar;
    racingYes->anchorType = ANCHOR_Y;
    racingYes->attachTo(layer);

    driNo = new Item("dri_no", layout); /* implicit: Object::operator delete (if the constructor throws) */
    driNo->x.set(420.0f);
    driNo->y.set(F_001051B0(2));
    driNo->anchor = crossBar;
    driNo->anchorType = ANCHOR_Y;
    driNo->attachTo(layer);

    driYes = new Item("dri_yes", layout); /* implicit: Object::operator delete (if the constructor throws) */
    driYes->x.set(420.0f);
    driYes->y.set(F_001051B0(2));
    driYes->anchor = crossBar;
    driYes->anchorType = ANCHOR_Y;
    driYes->attachTo(layer);

    updateRacingButtons();
}

/* Forgets every element (the objects themselves belong to the layer), in
 * buildCarSelect's order (sched2 puts the last store, where the base
 * register dies, first). */
// FUNC 0x0010BF48 gt2k_menu::ArcadeControl::clearCarSelect
void ArcadeControl::clearCarSelect()
{
    crossBar = NULL;
    carItem  = NULL;
    carView  = NULL;
    miniCars = NULL;
    clearArrows();
    logo           = NULL;
    colorSelection = NULL;
    colorChips     = NULL;
    driveTrain     = NULL;
    colorName      = NULL;
    powerText      = NULL;
    torqueText     = NULL;
    handlingText   = NULL;
    speedText      = NULL;
    accelText      = NULL;
    handlingGauge  = NULL;
    speedGauge     = NULL;
    accelGauge     = NULL;
    specPanel      = NULL;
    blur           = NULL;
    autoNo         = NULL;
    autoYes        = NULL;
    manuNo         = NULL;
    manuYes        = NULL;
    racingNo       = NULL;
    racingYes      = NULL;
    driNo          = NULL;
    driYes         = NULL;
}

/* The mini cars: alpha out at 2 / s for 0.5 s (hideMiniCars) / in
 * (showMiniCars). */
const OperatorCode g_0030AD90[] = {         /* 0x0030AD90 */
    U004_OP(OPC_FADE_ALPHA, 0, -2.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_REMOVE,     0,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1)
};

const OperatorCode g_0030ADF0[] = {         /* 0x0030ADF0 */
    U004_OP(OPC_FADE_ALPHA, 0, 2.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1),
    U004_OP(OPC_REMOVE,     0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1)
};

/* Entering the transmission row: the mini cars fade out (g_0030AD90). */
// FUNC 0x0010BFD8 gt2k_menu::ArcadeControl::hideMiniCars
void ArcadeControl::hideMiniCars()
{
    miniCars->start(g_0030AD90);
}

/* Back to the car row: they fade in again (g_0030ADF0). */
// FUNC 0x0010C000 gt2k_menu::ArcadeControl::showMiniCars
void ArcadeControl::showMiniCars()
{
    miniCars->start(g_0030ADF0);
}

/* A Blur2D at the origin, size 0 (no blur), just in front of the Car
 * view. */
// FUNC 0x0010C028 gt2k_menu::ArcadeControl::createBlur
void ArcadeControl::createBlur()
{
    Object *layer = pages.layers[pages.popupLevel];
    Blur2D *b;

    /* Object::operator new (0xC4 bytes), the inline constructor:
     * Transitive::Transitive() and the Blur2D vtable */
    /* implicit: Transitive (base constructor, inline Blur2D ctor) */
    blur = new Blur2D; /* implicit: Object::operator delete (if the constructor throws) */
    b = blur;
    b->x.set(0.0f);
    b->y.set(0.0f);
    b->w = 0.0f;
    b->h = 0.0f;
    b->z = carView->z + 0.5f;
    b->attachTo(layer);
}

/* The blur: from size 0 at the origin, w / h grow by 4 / s for 0.5 s
 * (blurIn); shrink again, then size 0 at the origin (blurOut). */
const OperatorCode g_0030AE50[] = {         /* 0x0030AE50 */
    U004_OP(OPC_SET_RECT, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_GROW,     0, 4.0f, 4.0f, 0.5f, 0.0f, 0.0f, 1),
    U004_OP(OPC_REMOVE,   0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1)
};

const OperatorCode g_0030AEE0[] = {         /* 0x0030AEE0 */
    U004_OP(OPC_GROW,     0, -4.0f, -4.0f, 0.5f, 0.0f, 0.0f, 1),
    U004_OP(OPC_SET_RECT, 0,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0),
    U004_OP(OPC_REMOVE,   0,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 1)
};

/* The blur radius grows to 2 (g_0030AE50): the screen behind the option
 * rows blurs. */
// FUNC 0x0010C108 gt2k_menu::ArcadeControl::blurIn
void ArcadeControl::blurIn()
{
    blur->clearOperators();
    blur->start(g_0030AE50);
}

/* ... and shrinks back to 0 (g_0030AEE0). */
// FUNC 0x0010C140 gt2k_menu::ArcadeControl::blurOut
void ArcadeControl::blurOut()
{
    blur->clearOperators();
    blur->start(g_0030AEE0);
}

/* State 1: left / right change the car (MENU_SE_LIST_MOVE, or
 * MENU_SE_REFUSED if it cannot move), up / down the colour (MENU_SE_CURSOR /
 * MENU_SE_REFUSED); decide on a locked car is refused, on any other car it
 * returns 1 (MENU_SE_DECIDE); cancel returns 0 (MENU_SE_CANCEL), except in
 * the i.LINK mode.  The moves go through the indices' vtables (slot 3).
 * Returns `result' unchanged when the screen times out. */
// FUNC 0x0010C178 gt2k_menu::ArcadeControl::selectCarAndColor
int ArcadeControl::selectCarAndColor(int result)
{
    int noCancel;

    scrollToRow(0);
    applyColor();
    noCancel = (mode == ARCADE_I_LINK);
    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;

            if (control1.getLeft() != 0)
                moved = moveIndex(&car1, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&car1, 1);
            if (moved != 0) {
                menu_play_se(MENU_SE_LIST_MOVE);
                onCarChanged();
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getUp() != 0 || control1.getDown() != 0) {
            int moved = 0;

            if (control1.getUp() != 0)
                moved = moveIndex(&color1, -1);
            else if (control1.getDown() != 0)
                moved = moveIndex(&color1, 1);
            if (moved != 0) {
                applyColor();
                menu_play_se(MENU_SE_CURSOR);
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0 && isCarLocked(car1.value, 0) != 0) {
                menu_play_se(MENU_SE_REFUSED);
            } else if (control1.getDecide() != 0) {
                menu_play_se(MENU_SE_DECIDE);
                result = 1;
                break;
            } else if (control1.getCancel() != 0 && !noCancel) {
                menu_play_se(MENU_SE_CANCEL);
                result = 0;
                break;
            }
        }
        waitFrame();
    } while (timedOut == 0);
    return result;
}

/* State 3: left / right choose racing or dri (row 2). */
// FUNC 0x0010C360 gt2k_menu::ArcadeControl::selectRacing
int ArcadeControl::selectRacing(int result)
{
    scrollToRow(2);
    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;

            if (control1.getLeft() != 0)
                moved = moveIndex(&racing1, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&racing1, 1);
            if (moved != 0) {
                updateRacingButtons();
                menu_play_se(MENU_SE_CURSOR);
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0) {
                menu_play_se(MENU_SE_DECIDE);
                result = 1;
                break;
            } else if (control1.getCancel() != 0) {
                menu_play_se(MENU_SE_CANCEL);
                result = 0;
                break;
            }
        }
        waitFrame();
    } while (timedOut == 0);
    return result;
}

/* State 2: left / right choose automatic or manual (row 1). */
// FUNC 0x0010C480 gt2k_menu::ArcadeControl::selectTransmission
int ArcadeControl::selectTransmission(int result)
{
    scrollToRow(1);
    /* The enhanced engine (Lupo 2 L2 0x0010F828; menus_a.md 39): a CVT can
     * only choose automatic (the range [0, 1) at 0), any other car both. */
    if (isCarCVT(0) != 0) {
        transmission1.set(0);
        transmission1.setRange(0, 1);
    } else {
        transmission1.setRange(0, 2);
    }
    updateTransmissionButtons();
    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;

            if (control1.getLeft() != 0)
                moved = moveIndex(&transmission1, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&transmission1, 1);
            if (moved != 0) {
                updateTransmissionButtons();
                menu_play_se(MENU_SE_CURSOR);
            } else {
                menu_play_se(MENU_SE_REFUSED);
            }
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0) {
                menu_play_se(MENU_SE_DECIDE);
                result = 1;
                break;
            } else if (control1.getCancel() != 0) {
                menu_play_se(MENU_SE_CANCEL);
                result = 0;
                break;
            }
        }
        waitFrame();
    } while (timedOut == 0);
    return result;
}

/* The enhanced engine (new; Lupo 2's 1P assist step, L2 0x0010BC18..
 * 0x0010C0E0; menus_a.md 17): player 1's assist level, left / right, from
 * the layout's items (T021: openAssist / assistInput / closeAssist).  1 and
 * the level is player 1's choice (ArcadeControl::finish gives it to the
 * options) when decided, 0 on cancel; `result' unchanged when the screen
 * times out.  Caller: runCarSelect, only when hasAssistStep(1). */
int ArcadeControl::selectAssist(int result)
{
    openAssist(1);
    do {
        if (assistInput(&result, 1) != 0) {
            if (result != 0)
                assistChosen[0] = 1;
            break;
        }
        waitFrame();
    } while (timedOut == 0);
    closeAssist(1);
    return result;
}

/* No callers (state 4 of runCarSelect does nothing): the same loop for
 * row 3 and opt1d, with no buttons to update. */
// FUNC 0x0010C5A0 gt2k_menu::ArcadeControl::F_0010C5A0
int ArcadeControl::F_0010C5A0(int result)
{
    scrollToRow(3);
    do {
        if (control1.getLeft() != 0 || control1.getRight() != 0) {
            int moved = 0;

            if (control1.getLeft() != 0)
                moved = moveIndex(&opt1d, -1);
            else if (control1.getRight() != 0)
                moved = moveIndex(&opt1d, 1);
            if (moved != 0)
                menu_play_se(MENU_SE_CURSOR);
            else
                menu_play_se(MENU_SE_REFUSED);
        } else if (control1.getDecide() != 0 || control1.getCancel() != 0) {
            if (control1.getDecide() != 0) {
                menu_play_se(MENU_SE_DECIDE);
                result = 1;
                break;
            } else if (control1.getCancel() != 0) {
                menu_play_se(MENU_SE_CANCEL);
                result = 0;
                break;
            }
        }
        waitFrame();
    } while (timedOut == 0);
    return result;
}

/* The arcade car select page (6 callers in T010).  `result' comes in from
 * the previous page (nonzero: arriving forwards) and goes out as 1 = car
 * chosen, 0 = back.  The car index starts on MenuStatus::lastArcadeCar,
 * stops at the ends, and is stored back there.  The background movie is
 * g_00292930[3] ("stream/menu_bg2.pss"). */
// FUNC 0x0010C6B8 gt2k_menu::ArcadeControl::runCarSelect
int ArcadeControl::runCarSelect(int result)
{
    PageManager *pm = &pages;
    SmartCarIndex *car = &car1;
    int state;
    int lastCar;
    int done;

    pm->setPage("ar/select_car", NULL);
    pm->startPage(NULL);
    async_loader_create(0);
    g_002CDC10.start(&g_00292930[3]);
    lastCar = getGameStatus()->getMenuStatus()->lastArcadeCar;
    car->setRange(0, carCount(0));
    car->set(lastCar);
    car->clamp = 1;
    /* after the clamp store: with state = 1 known earlier, CSE stores
     * state's register there (retail loads 1 anew) */
    state = CAR_SELECT_CAR;
    buildCarSelect();
    transitionIn();
    if (result != 0 && mode == ARCADE_SINGLE_RACE)
        showHistoryClass();
    gfx_vsync(10);
    done = 0;
    do {
        switch (state) {
        case CAR_SELECT_BACK:
            result = 0;
            done = 1;
            break;
        case CAR_SELECT_CAR:
            result = selectCarAndColor(result);
            if (result != 0)
                state++;
            else
                state--;
            break;
        case CAR_SELECT_TRANSMISSION:
            hideMiniCars();
            if (result != 0)
                blurIn();
            result = selectTransmission(result);
            if (result != 0) {
                /* no racing / dri choice on a dirt course or for a garage car
                 * (one condition: two separate ifs on the class fold into a
                 * range test retail does not have) */
                if (isDirtCourse() != 0 || class1.value == ARCADE_CLASS_HOME_GARAGE ||
                    class1.value == ARCADE_CLASS_GUEST_GARAGE)
                    state += 2;
                else
                    state += 1;
            } else {
                state--;
                showMiniCars();
                blurOut();
            }
            break;
        case CAR_SELECT_RACING:
            result = selectRacing(result);
            if (result != 0)
                state++;
            else
                state--;
            break;
        case CAR_SELECT_OPT_D:
            /* The enhanced engine (Lupo 2; menus_a.md 17): the assist step
             * when the layout has it (GT3's has not: straight on, as GT3);
             * back goes to the racing step, or to the transmission where
             * the racing step was skipped. */
            if (hasAssistStep(1)) {
                result = selectAssist(result);
                if (result == 0) {
                    if (isDirtCourse() != 0 || class1.value == ARCADE_CLASS_HOME_GARAGE ||
                        class1.value == ARCADE_CLASS_GUEST_GARAGE)
                        state = CAR_SELECT_TRANSMISSION;
                    else
                        state = CAR_SELECT_RACING;
                    break;
                }
            }
            state++;
            break;
        case CAR_SELECT_DONE:
            result = 1;
            done = 1;
            break;
        }
        waitFrame();
        /* (both tests at the bottom: `while (timedOut == 0 && done == 0)'
         * gets the done test threaded to the loop top) */
        if (timedOut != 0)
            break;
    } while (done == 0);

    if (result == 0 && mode == ARCADE_SINGLE_RACE && preview == 0)
        hideHistoryClass();
    transitionOut();
    clearCarSelect();
    async_loader_destroy();
    getGameStatus()->getMenuStatus()->lastArcadeCar = car1.value;
    pm->closeAndExit();
    gfx_vsync(2);
    return result;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0010C948 __tfQ29gt2k_menu12TransitiveEX
extern "C" void *__tfQ29gt2k_menu12TransitiveEX(void)
{
    if (__tiQ29gt2k_menu12TransitiveEX[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu12TransitiveEX, "Q29gt2k_menu12TransitiveEX",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu12TransitiveEX;
}
#endif

// FUNC 0x0010C998 gt2k_menu::TransitiveEX::~TransitiveEX
/* implicit in game/menu/arcade/T023_arcade_car_select.h; egcs emits this copy (slot
 * [0]): Transitive::~Transitive (flags 0), then Object::operator delete
 * with flags & 1; no vptr store. */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
