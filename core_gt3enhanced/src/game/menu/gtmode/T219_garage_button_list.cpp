/* T219 - gt2k_menu::GarageButtonList and gt2k_menu::GarageButton.
 * 0x001BD710..0x001BF390.
 *
 * The car list of the garage page ("hm/hm_gr_panel", built by T044
 * create_menu_garage / create_menu_garage_trade): one GarageButton per garage
 * car.  A button draws the "mark" item next to the current car, a colour chip
 * in the car's colour and one info column chosen by a SmartIndexT<int>
 * (GarageInfoMode: power, price, maker, distance, drive, aspiration) that L2 /
 * R2 of the page control's first pad move; a CallbackItem shows the matching
 * column header.  The list sorts its buttons with one of nine qsort
 * comparators (g_002C8DF8[sortType]).  With a page, START opens it on the
 * focused car; cancel puts the current car back as the menu's car. */
/* The original object saw these classes before its own (typeinfo names come
 * in reverse definition order): Q29gt2k_menu16GarageButtonList and
 * Q29gt2k_menu12GarageButton are the first type names of T219's .rodata
 * (0x00334638 / 0x00334658, the __tf functions' references), then
 * SmartIndexT<int>, List<Button>, Cell<Button>, T216's operator classes, the
 * garage's, Operator (0x00334678..0x003348A6). */
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus, MenuStatus (T427) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x00334678 / 0x00334698). */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList, ButtonData */
#include "game/menu/gtmode/T219_garage_button_list.h"
#include "game/menu/framework/T218_color.h"                /* Color4f, color_compare_hsv */
#include "engine/pgl/G001_flare.h"                /* fmt_sprintf (%t fixed point) */
#include "runtime/newlib/G007_stdio.h"                /* qsort, strcmp, strcpy */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setResource, setText), MenuTexture */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (cancel), CONTROL_START (T474) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, operator new / delete) */
#include "game/menu/framework/T221_callback_item.h"        /* gt2k_menu::CallbackItem (the info column header) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (control1) */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition::odometerDistance */
#include "game/menu/gtmode/T036_garage.h"               /* GarageCar (getColor, getPower), CarInformation (T483),
                                               CarEquipments (T482) */
#include "game/common/T494_text_utils.h"           /* format_number */
#include "game/menu/framework/T495_menu_list.h"            /* gt2k_menu::pCell / pList (count, at, unlinkAll, append) */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/framework/menu_list.h"                 /* gt2k_menu::Cell<T>, List<T>, ListIterator<T> */
#include "game/menu/framework/operator_internal.h"                           /* GarageSortType, GarageInfoMode, g_002C8DF8 */

namespace gt2k_menu {

/* The column header's select function (a CallbackItem callback, ctx the
 * list): the info column. */
// FUNC 0x001BD710 garage_info_mode
static int garage_info_mode(void *ctx)
{
    return ((GarageButtonList *)ctx)->infoMode->value;
}

/* frame gives the list's rectangle (x, y bottom left, y up: the centre is at
 * 448 - (y + h / 2)).  select: the button to focus; 0 or out of range: the
 * menu's car, else the current car (getRunningCar), else 0. */
// FUNC 0x001BD720 gt2k_menu::GarageButtonList::GarageButtonList
GarageButtonList::GarageButtonList(gt2k_game::GameStatus *gs_, const char *page_,
                                   int controlType_, int loop_, PageLayout *layout_,
                                   ButtonData **items, int count, MenuLayoutItem *frame,
                                   MenuLayoutItem *pics, int select, int sortType_,
                                   SmartIndexT<int> *infoMode_)
    : ButtonList(controlType_, loop_, layout_),
      gs(gs_), page(page_),
      owner((ControlBase *)layout_->owner),   /* the page manager's owner: the page control */
      mark(), color(), /* implicit: Item (member constructors), ~Item (exception cleanup) */
      infoMode(infoMode_), sortType(sortType_)
{
    /* The members mark and color are Item() (0x001B98C0), given their layout
     * pictures below; the landing pads destroy what is built (Item::~Item
     * 0x001B9C90, ButtonList::~ButtonList 0x001C8DA8) and free the
     * CallbackItem's block (Object::operator delete 0x001BF458) if its
     * constructor throws. */
    setRect(frame->x + frame->w * 0.5f, 448.0f - (frame->y + frame->h * 0.5f), frame->w, frame->h);
    ButtonList::createButtons(items, count, pics);  /* a direct call in the original */
    resetIndex();

    if (select == 0 || index.outOfRange(select)) {
        GarageCar *car;
        int i;

        do {
            select = -1;
            car = getGameStatus()->getMenuStatus()->car;
            for (i = 0; i < gs->getCarNum(); i++)
                select = (gs->getCar(i) == car) ? i : select;
            if (select != -1)
                break;
            select = 0;
            car = gs->getRunningCar();
            if (car != NULL) {
                for (i = 0; i < gs->getCarNum(); i++)
                    select = (gs->getCar(i) == car) ? i : select;
            }
        } while (0);
    }
    index.set(select);
    mark.setResource("mark", layout_);
    color.setResource("color", layout_);

    /* the column header: one child Item per info column (Object::operator new) */
    CallbackItem *header = new CallbackItem(GARAGE_INFO_COUNT, garage_info_mode, this); /* implicit: operator new, operator delete (new-expression) */
    MenuLayoutItem *powerItem = layout_->findHiddenItem("power");
    header->setItem(powerItem, layout_, GARAGE_INFO_POWER);
    if (powerItem != NULL)
        header->z = powerItem->z;
    header->setItem(layout_->findHiddenItem("price"), layout_, GARAGE_INFO_PRICE);
    header->setItem(layout_->findHiddenItem("maker"), layout_, GARAGE_INFO_MAKER);
    header->setItem(layout_->findHiddenItem("distance"), layout_, GARAGE_INFO_DISTANCE);
    header->setItem(layout_->findHiddenItem("drive"), layout_, GARAGE_INFO_DRIVE);
    header->setItem(layout_->findHiddenItem("aspiration"), layout_, GARAGE_INFO_ASPIRATION);
    header->attachTo(this);

    MenuLayoutItem *info = layout_->findHiddenItem("info");
    MenuTexture *infoTex = NULL;
    if (info != NULL)
        infoTex = layout_->getTexture(info);
    MenuLayoutItem *infoA = layout_->findHiddenItem("info_a");
    MenuTexture *infoATex = NULL;
    if (infoA != NULL)
        infoATex = layout_->getTexture(infoA);

    ListIterator<Button> it(this);
    Button *button;
    while ((button = it.next()) != NULL)
        static_cast<GarageButton *>(button)->setInfoResources(info, infoTex, infoA, infoATex);
}

/* ButtonList::refresh, then the buttons sorted by sortType. */
// FUNC 0x001BDBE8 gt2k_menu::GarageButtonList::refresh
void GarageButtonList::refresh()
{
    ButtonList::refresh();
    sortButtons(g_002C8DF8[sortType]);
}

/* A GarageButton for garage car garageIndex (the entry's index), attached to
 * the list. */
// FUNC 0x001BDC30 gt2k_menu::GarageButtonList::createButton
Button *GarageButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                       int garageIndex)
{
    /* 0x3BC bytes from Object::operator new; Object::operator delete in the
     * landing pad if the constructor throws. */
    GarageButton *button = new GarageButton(gs, this, controlType, layout, offPoint, data, /* implicit: operator new, operator delete (new-expression) */
                                            elem, &mark, &color, garageIndex, infoMode);
    button->attachTo(this);
    return button;
}

/* The garage index of the focused button, 0 if none. */
// FUNC 0x001BDD10 gt2k_menu::GarageButtonList::getCursor
int GarageButtonList::getCursor()
{
    Button *button = focusButton();
    if (button != NULL)
        return static_cast<GarageButton *>(button)->index;
    return 0;
}

} /* namespace gt2k_menu */

/* The record handed to the page opened: a file static (the original's .bss,
 * 0x0049C840..0x0049C98C). */
static gt2k_menu::ButtonData g_0049C840;     /* 0x0049C840 */

namespace gt2k_menu {

/* Opens `page`: a copy of the focused button's record with link = page, the
 * cursor sound, then ButtonList::decideButton. */
// FUNC 0x001BDD48 gt2k_menu::GarageButtonList::openPage
void GarageButtonList::openPage()
{
    Button *button = focusButton();
    if (button != NULL) {
        g_0049C840 = *button->linkMenu;
        strcpy(g_0049C840.link, page);
        menu_play_se(MENU_SE_CURSOR);
        decideButton(&g_0049C840, 0);
    }
}

/* L2 / R2 of the page control's first pad move the info column (the cursor
 * sound if it moved; R2 overrides L2's result).  With a page, START makes the
 * focused car the menu's car and opens the page.  Otherwise cancel puts the
 * current car back as the menu's car (else garage car 0), then
 * ButtonList::doControl. */
// FUNC 0x001BDE80 gt2k_menu::GarageButtonList::doControl
void GarageButtonList::doControl(const MenuControl &ctl)
{
    if (count() == 0)                               /* pList::count of the buttons */
        return;

    int moved = 0;
    if (owner->control1.l2 != 0)
        moved = infoMode->move(-1);
    if (owner->control1.r2 != 0)
        moved = infoMode->move(1);
    if (moved != 0)
        menu_play_se(MENU_SE_CURSOR);

    if (page != NULL && (ctl.getButtonsOn() & CONTROL_START) != 0) {
        GarageCar *focusCar = gs->getCar(index.value);
        if (focusCar != NULL)
            gs->menu.setCar(focusCar);
        openPage();
        return;
    }
    if (ctl.cancel != 0) {
        GarageCar *car = gs->getRunningCar();
        if (car != NULL) {
            gs->menu.setCar(car);
        } else {
            car = gs->getCar(0);
            if (car != NULL)
                gs->menu.setCar(car);
        }
    }
    ButtonList::doControl(ctl);
}

/* Button(owner, controlType, layout, offPoint, linkMenu, pic, relative 1,
 * depth garageIndex) (ButtonList::createButton passes its `relative'), the
 * members, then the car's power and its rpm. */
// FUNC 0x001BDFF8 gt2k_menu::GarageButton::GarageButton
GarageButton::GarageButton(gt2k_game::GameStatus *gs_, ButtonList *owner_, int controlType_,
                           PageLayout *layout_, f32 offPoint_, ButtonData *linkMenu_,
                           MenuLayoutItem *pic, Item *mark_, Item *colorItem_, int garageIndex,
                           SmartIndexT<int> *infoMode_)
    : Button(owner_, controlType_, layout_, offPoint_, linkMenu_, pic, 1, garageIndex),
      gs(gs_), index(garageIndex), mark(mark_), colorItem(colorItem_),
      info(), infoA(), infoMode(infoMode_)
{
    /* info / infoA are Item() (0x001B98C0); the landing pads run Item::~Item
     * and Button::~Button (0x001B3E60) on what is built. */
    gs->getCar(index)->getPower(&power, &rpm);
}

/* The two info items' pictures: Item::setResource(item, texture) each. */
// FUNC 0x001BE148 gt2k_menu::GarageButton::setInfoResources
void GarageButton::setInfoResources(MenuLayoutItem *infoItem, MenuTexture *infoTex,
                              MenuLayoutItem *infoAItem, MenuTexture *infoATex)
{
    info.setResource(infoItem, infoTex);
    infoA.setResource(infoAItem, infoATex);
}

// FUNC 0x001BE198 gt2k_menu::GarageButton::car
GarageCar *GarageButton::car()
{
    return gs->getCar(index);
}

/* The x at percent (0..100) of the width from the left edge.  No callers. */
// FUNC 0x001BE1B8 gt2k_menu::GarageButton::xAtPercent
f32 GarageButton::xAtPercent(f32 percent)
{
    return (x.value - w * 0.5f) + (w * percent) / 100.0f;
}

/* Button's draw steps (beginDraw, isClipped, drawItem ... drawFocus, endDraw)
 * with, between them: on the current car's button the mark (its right edge
 * 164 left of the button's), the colour chip (8 wide, in the car's colour) at
 * the left edge, and the info column text centred 80 left of the right edge
 * ("info_a" while focused).  The Items are drawn by their virtual_2. */
// FUNC 0x001BE1F0 gt2k_menu::GarageButton::virtual_2
void GarageButton::virtual_2()
{
    GarageCar *garageCar = car();
    beginDraw();
    if (isClipped() == 0) {
        drawItem();
        if (garageCar == gs->getRunningCar()) {
            mark->x.set(x.get() + w * 0.5f - mark->w * 0.5f - 164.0f);
            mark->y.set(y.get());
            mark->virtual_2();
        }

        /* the car's colour, alpha at least 0x80 (GS: opaque); Drawable::setColor
         * (inline, T259: the four corners), which Item::setColor(u32), the
         * text colour, hides */
        colorItem->Drawable::setColor(garageCar->getColor(0x80000000));
        colorItem->w = 8.0f;
        colorItem->h = h;
        colorItem->x.set(x.get() - w * 0.5f + 4.0f);
        colorItem->y.set(y.get());
        colorItem->virtual_2();

        Item *label = (focused == 0) ? &info : &infoA;
        label->x.set(x.get() + w * 0.5f - 80.0f);
        label->y.set(y.get());
        switch (infoMode->value) {
        case GARAGE_INFO_POWER: {
            const char *unit = g_002F59D8.powerName();
            sprintf(text, "%d%s", power, unit);
            label->setText(text);
            break;
        }
        case GARAGE_INFO_PRICE:
            if ((garageCar->getShopFlags() >> 1) & 1) {
                label->setText("-");                /* shop flag bit 1 (u009: not at the used car
                                                       shop): no sell price */
            } else {
                /* the sell price, getPrice() / 4 (CarInformation::getSellPrice's
                 * formula, written out) */
                format_number(text, garageCar->getPrice() / 4);
                label->setText(text);
            }
            break;
        case GARAGE_INFO_MAKER:
            label->setText(garageCar->getMakerDisplayName()); /* the maker's name (record +0x104) */
            break;
        case GARAGE_INFO_DISTANCE: {
            /* tenths of a km / mile: %.1t prints them with one decimal */
            char distText[0x40];
            fmt_sprintf(distText, "%.1t%s", garageCar->condition.odometerDistance(),
                        g_002F59D8.distanceName());
            label->setText(distText);
            break;
        }
        case GARAGE_INFO_DRIVE:
            label->setText(garageCar->getDriveTrainText());
            break;
        case GARAGE_INFO_ASPIRATION:
            label->setText(garageCar->getAspirationText());
            break;
        }
        label->virtual_2();
        drawFocus();
    }
    endDraw();
}

/* ------------------------------------------------------------------------
 * The sort comparators (g_002C8DF8), over GarageButton pointers.  The
 * numeric ones sort in decreasing order and evaluate their keys twice.
 * ------------------------------------------------------------------------ */
static inline GarageButton *button_of(const void *elem)
{
    return *(GarageButton *const *)elem;
}

/* g_002C8DF8[GARAGE_SORT_POWER] */
// FUNC 0x001BEA78 cmp_power
static int cmp_power(const void *a, const void *b)
{
    int powerA = button_of(a)->power;
    int powerB = button_of(b)->power;
    if (powerA > powerB)
        return -1;
    return powerA < powerB;
}

/* g_002C8DF8[GARAGE_SORT_PRICE]: the sell price (getPrice() / 4). */
// FUNC 0x001BEAA8 cmp_price
static int cmp_price(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    if (carA->getPrice() / 4 > carB->getPrice() / 4)
        return -1;
    if (carA->getPrice() / 4 < carB->getPrice() / 4)
        return 1;
    return 0;
}

/* g_002C8DF8[GARAGE_SORT_MAKER]: the maker's names (CarEquipments::getMakerDisplayName). */
// FUNC 0x001BEB70 cmp_maker
static int cmp_maker(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    return strcmp(carA->getMakerDisplayName(), carB->getMakerDisplayName());
}

/* g_002C8DF8[GARAGE_SORT_INDEX] */
// FUNC 0x001BEBD8 cmp_index
static int cmp_index(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    int indexA = carA->index;
    int indexB = carB->index;
    if (indexA > indexB)
        return -1;
    return indexA < indexB;
}

/* g_002C8DF8[GARAGE_SORT_COLOR]: color_compare_hsv with the arguments
 * swapped (b's colour first). */
// FUNC 0x001BEC30 cmp_color
static int cmp_color(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    Color4f colorA(carA->getColor(0x80000000));
    Color4f colorB(carB->getColor(0x80000000));
    return color_compare_hsv(&colorB, &colorA);
}

/* g_002C8DF8[GARAGE_SORT_DISTANCE] (unsigned) */
// FUNC 0x001BEF00 cmp_distance
static int cmp_distance(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    if (carA->condition.odometerDistance() > carB->condition.odometerDistance())
        return -1;
    return carA->condition.odometerDistance() < carB->condition.odometerDistance();
}

/* g_002C8DF8[GARAGE_SORT_NAME]: the first part of the car's name. */
// FUNC 0x001BEF80 cmp_name
static int cmp_name(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    return strcmp(carA->getName1(), carB->getName1());
}

/* g_002C8DF8[GARAGE_SORT_DRIVE] */
// FUNC 0x001BEFE8 cmp_drive
static int cmp_drive(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    return strcmp(carA->getDriveTrainText(), carB->getDriveTrainText());
}

/* g_002C8DF8[GARAGE_SORT_ASPIRATION] */
// FUNC 0x001BF050 cmp_aspiration
static int cmp_aspiration(const void *a, const void *b)
{
    GarageButton *buttonA = button_of(a);
    GarageButton *buttonB = button_of(b);
    GarageCar *carA = buttonA->car();
    GarageCar *carB = buttonB->car();
    return strcmp(carA->getAspirationText(), carB->getAspirationText());
}

} /* namespace gt2k_menu */

/* The comparators by GarageSortType (initialised data: .data
 * 0x002C8DF8..0x002C8E1C, before the frame info). */
int (*g_002C8DF8[GARAGE_SORT_COUNT])(const void *, const void *) = {   /* 0x002C8DF8 */
    gt2k_menu::cmp_index,                   /* GARAGE_SORT_INDEX */
    gt2k_menu::cmp_name,                    /* GARAGE_SORT_NAME */
    gt2k_menu::cmp_maker,                   /* GARAGE_SORT_MAKER */
    gt2k_menu::cmp_color,                   /* GARAGE_SORT_COLOR */
    gt2k_menu::cmp_price,                   /* GARAGE_SORT_PRICE */
    gt2k_menu::cmp_distance,                /* GARAGE_SORT_DISTANCE */
    gt2k_menu::cmp_power,                   /* GARAGE_SORT_POWER */
    gt2k_menu::cmp_drive,                   /* GARAGE_SORT_DRIVE */
    gt2k_menu::cmp_aspiration,              /* GARAGE_SORT_ASPIRATION */
};

namespace gt2k_menu {

/* List<Button> slot 1 (pList::sort; the secondary vtable, this-delta -0xB0):
 * the buttons into a stack array (VLA), the list emptied, the array qsorted,
 * the buttons appended again in order. */
// FUNC 0x001BF0B8 gt2k_menu::GarageButtonList::sort
void GarageButtonList::sort(int (*cmp)(const void *, const void *))
{
    pList *list = this;
    int n = list->count();
    Button **buttons = (Button **)__builtin_alloca(n * sizeof(Button *));
    int i;

    for (i = 0; i < n; i++)
        buttons[i] = static_cast<Button *>(static_cast<Cell<Button> *>(list->at(i)));
    list->unlinkAll();
    qsort(buttons, n, sizeof(Button *), cmp);
    for (i = 0; i < n; i++)
        list->append(buttons[i]);
}

/* The end of the object, egcs's out-of-line copies with the vtables (the
 * classes in reverse definition order; per class its inline members in
 * reverse declaration order, __tf, the implicit destructor): the frame info
 * 0x002C8E1C has them in this order. */
// FUNC 0x001BF1C0 gt2k_menu::GarageButtonList::getInfoMode (getInfoMode__Q29gt2k_menu16GarageButtonList)
/* inline in game/menu/gtmode/T219_garage_button_list.h; egcs emits this copy (no
 * callers): infoMode->value */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001BF1D0 __tfQ29gt2k_menu16GarageButtonList
extern "C" void *__tfQ29gt2k_menu16GarageButtonList(void)
{
    if (__tiQ29gt2k_menu16GarageButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu16GarageButtonList, "Q29gt2k_menu16GarageButtonList",
                     g_003525D8, 1);
    }
    return __tiQ29gt2k_menu16GarageButtonList;
}
#endif

namespace gt2k_menu {

// FUNC 0x001BF228 gt2k_menu::GarageButtonList::~GarageButtonList (_$_Q29gt2k_menu16GarageButtonList)
/* implicit in game/menu/gtmode/T219_garage_button_list.h; egcs emits this copy: both
 * vtables, then the members color / mark (Item::~Item, flags 2) and
 * ButtonList::~ButtonList(this, 0); if (flags & 1) Object::operator delete
 * (0x001BF458) */
/* implicit: ~Item, ~ButtonList, operator delete (the base / member
 * destructors, flags & 1) */

// FUNC 0x001BF2A8 gt2k_menu::GarageButton::getPower (getPower__Q29gt2k_menu12GarageButton)
/* inline in game/menu/gtmode/T219_garage_button_list.h; egcs emits this copy (no
 * callers): power */

// FUNC 0x001BF2B0 gt2k_menu::GarageButton::getIndex (getIndex__Q29gt2k_menu12GarageButton)
/* inline in game/menu/gtmode/T219_garage_button_list.h; egcs emits this copy (no
 * callers): index */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001BF2B8 __tfQ29gt2k_menu12GarageButton
extern "C" void *__tfQ29gt2k_menu12GarageButton(void)
{
    if (__tiQ29gt2k_menu12GarageButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu12GarageButton, "Q29gt2k_menu12GarageButton",
                     g_003525E0, 1);
    }
    return __tiQ29gt2k_menu12GarageButton;
}
#endif

// FUNC 0x001BF310 gt2k_menu::GarageButton::~GarageButton (_$_Q29gt2k_menu12GarageButton)
/* implicit in game/menu/gtmode/T219_garage_button_list.h; egcs emits this copy: both
 * vtables, then the members infoA / info (Item::~Item, flags 2) and
 * Button::~Button(this, 0); if (flags & 1) Object::operator delete
 * (0x001BF458) */
/* implicit: ~Item, ~Button, operator delete (the base / member destructors,
 * flags & 1) */
