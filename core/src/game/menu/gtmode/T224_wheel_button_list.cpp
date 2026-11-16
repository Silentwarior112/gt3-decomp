/* T224 - gt2k_menu::WheelButtonList, gt2k_menu::WheelButton.
 * 0x001C0D18..0x001C1520.
 *
 * The wheel list of the garage: one WheelButton per wheel the player owns
 * (GameStatus::getWheel(i)), each showing a picture of its wheel
 * ("default_wheel" for the car's own).  The list's "mark" picture is drawn
 * over the wheel fitted to the garage car, the focus starts there, and
 * moving the focus left / right (or up / down) shows the focused wheel on
 * the 3D car (MenuStatus::carItem). */
/* Retail's .rodata (0x00334E48..0x0033521E) opens with
 * SimpleCameraBase::print's literal "(%f, %f, %f) %f %f %f\n" (T412's inline
 * body, written where the header is read: the original saw the camera
 * classes), then "default_wheel", the vtables and the type names
 * WheelButtonList, WheelButton, SmartIndexT<int>, List / Cell<Button>,
 * OperatorList, List / Cell<OperatorHolder>, OperatorHolder, the
 * GameStatus's garage lists, Operator: the GameStatus was read before the
 * menu classes, its own header last. */
// DATA_START 0x00334E48
#include "game/camera/T412_simple_camera_base.h"   /* (seen: its inline print's literal) */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (getWheel, MenuStatus car /
                                               carItem), GarageCar, GarageWheel */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Cell<Button>, T213 Item, T216's
                                               operator classes) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x00335010 / 0x00334FF0). */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList (List<Button>, SmartIndexT<int>) */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car::setWheel */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl (getPrev / getNext) */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout, MenuLayoutItem */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::isDefaultWheel */
#include "game/save/T491_garage_wheel.h"         /* GarageWheel (name, getName) */
#include "game/common/T494_text_utils.h"           /* str_equal */
#include "game/menu/gtmode/T224_wheel_button_list.h"
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

/* elem: the list's own layout item (its rectangle; the centre y measured
 * down from the top of the 448-line screen); rects: the button items.  The
 * focus goes to the first wheel that is the car's, else to defaultIndex.
 * The rectangle goes through Drawable::setRect (retail evaluates its four
 * arguments before the first store); the found wheel goes into the
 * parameter itself (retail loads defaultIndex from the stack at entry: a
 * parameter that is assigned gets its register there). */
// FUNC 0x001C0D18 gt2k_menu::WheelButtonList::WheelButtonList
WheelButtonList::WheelButtonList(int controlType, int loop, PageLayout *layout,
                                 ButtonData **items, int count, MenuResource *elem,
                                 MenuResource *rects, int defaultIndex)
    : ButtonList(controlType, loop, layout), mark() /* implicit: Item (member constructor), ~Item (exception cleanup) */
{
    setRect(elem->x + elem->w * 0.5f, MENU_SCREEN_H - (elem->y + elem->h * 0.5f), elem->w, elem->h);
    ButtonList::createButtons(items, count, rects);
    resetIndex();

    for (int i = 0; i < count; i++) {
        if (wheel_is_current(i) != 0) {
            defaultIndex = i;
            break;
        }
    }
    index.set(defaultIndex);
    mark.setResource("mark", layout);
}

/* The button of record `data' (its wheel index = its index in the list),
 * hung under the list. */
// FUNC 0x001C0EC8 gt2k_menu::WheelButtonList::createButton
Button *WheelButtonList::createButton(MenuResource *elem, ButtonData *data, f32 offPoint,
                                      int buttonIndex)
{
    WheelButton *button = new WheelButton(this, controlType, layout, offPoint, data, elem, /* implicit: operator new, operator delete (new-expression) */
                                          &mark, buttonIndex);  /* 0x268 bytes */
    button->attachTo(this);
    return button;
}

/* The focused button's wheel index (no NULL check). */
// FUNC 0x001C0F90 gt2k_menu::WheelButtonList::getCursor
int WheelButtonList::getCursor()
{
    return static_cast<WheelButton *>(focusButton())->wheelIndex;
}

/* Input: the list's own; then, when the focus may have moved (left / right,
 * or up / down for a vertical list), the focused wheel goes onto the car
 * view.  An unnamed wheel (the car's own) shows the car model's own wheel
 * (GarageCar::getModelName).  GameStatus::getMenuStatus(): retail keeps
 * &menu (0x307C8) in a register for both reads. */
// FUNC 0x001C0FC0 gt2k_menu::WheelButtonList::doControl
void WheelButtonList::doControl(const MenuControl &ctl)
{
    if (count() != 0) {
        ButtonList::doControl(ctl);
        Car *car = getGameStatus()->getMenuStatus()->carItem;
        if (car != 0 && (ctl.getPrev(controlType) != 0 || ctl.getNext(controlType) != 0)) {
            int cursor = index.value;
            GarageWheel *wheel = getGameStatus()->getWheel(cursor);
            if (wheel != 0) {
                const char *name = wheel->getName();
                if (name != 0)
                    car->setWheel(name);
                else
                    car->setWheel(getGameStatus()->getMenuStatus()->car->getModelName());
            }
        }
    }
}

/* Button(owner, controlType, layout, offPoint, linkMenu, elem, relative 1,
 * depth = wheelIndex); the members as mem-initializers (retail stores
 * wheelIndex before the wheel Item is built, mark after it); the wheel
 * picture is the hidden item named after the wheel, or "default_wheel". */
// FUNC 0x001C10B8 gt2k_menu::WheelButton::WheelButton
WheelButton::WheelButton(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
                         ButtonData *linkMenu, MenuLayoutItem *elem, Item *listMark,
                         int wheelNo)
    : Button(owner, controlType, layout, offPoint, linkMenu, elem, 1, wheelNo),
      wheelIndex(wheelNo), wheel(), mark(listMark)
{
    const char *name = getGameStatus()->getWheel(wheelIndex)->getName();
    if (name == 0)
        name = "default_wheel";
    wheel.setResource(name, layout);
}

/* 1 if wheel `wheelIndex' of the garage is the one fitted to the garage car:
 * both unnamed (the car has its default wheel and the wheel is the car's
 * own), or both named the same. */
// FUNC 0x001C11C0 gt2k_menu::wheel_is_current
int wheel_is_current(int wheelIndex)
{
    GarageCar *car = getGameStatus()->getMenuStatus()->car;
    const char *carWheel;
    if (car->isDefaultWheel() != 0)
        carWheel = 0;
    else
        carWheel = car->getWheelModelName();

    GarageWheel *wheel = getGameStatus()->getWheel(wheelIndex);
    const char *name;
    if (wheel->name[0] != 0)
        name = wheel->getName();
    else
        name = 0;

    if (carWheel == 0) {
        if (name == 0)
            return 1;
    } else if (name != 0) {
        return str_equal(carWheel, name);
    }
    return 0;
}

// FUNC 0x001C1280 gt2k_menu::WheelButton::virtual_1
void WheelButton::virtual_1()
{
    Button::virtual_1();
    wheel.Item::virtual_1();
}

/* Draw: the button; if this is the fitted wheel the list's mark at the
 * button's position; the focus; the wheel picture there too. */
// FUNC 0x001C12B0 gt2k_menu::WheelButton::virtual_2
void WheelButton::virtual_2()
{
    beginDraw();
    if (isClipped() == 0) {
        drawItem();
        f32 px = posX;
        f32 py = posY;
        if (wheel_is_current(wheelIndex) != 0) {
            mark->x.set(px);
            mark->y.set(py);
            mark->virtual_2();
        }
        drawFocus();
        wheel.x.set(px);
        wheel.y.set(py);
        wheel.Item::virtual_2();
    }
    endDraw();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001C1388 __tfQ29gt2k_menu15WheelButtonList
extern "C" void *__tfQ29gt2k_menu15WheelButtonList(void)
{
    if (__tiQ29gt2k_menu15WheelButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu15WheelButtonList, "Q29gt2k_menu15WheelButtonList",
                     g_003525F0, 1);
    }
    return __tiQ29gt2k_menu15WheelButtonList;
}
#endif

/* The implicit destructor (game/menu/gtmode/T224_wheel_button_list.h): both vtables,
 * mark.~Item() (flags 2), ButtonList::~ButtonList (flags 0), then if
 * (flags & 1) the class operator delete.  egcs writes it after the class's
 * typeinfo function (the frame info's order). */
// FUNC 0x001C13E0 gt2k_menu::WheelButtonList::~WheelButtonList
/* implicit: ~Item (member destructor), ~ButtonList (base destructor), operator delete (deleting destructor) */

// FUNC 0x001C1450 gt2k_menu::WheelButton::getWheelIndex
/* inline in game/menu/gtmode/T224_wheel_button_list.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001C1458 __tfQ29gt2k_menu11WheelButton
extern "C" void *__tfQ29gt2k_menu11WheelButton(void)
{
    if (__tiQ29gt2k_menu11WheelButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu11WheelButton, "Q29gt2k_menu11WheelButton",
                     g_003525F8, 1);
    }
    return __tiQ29gt2k_menu11WheelButton;
}
#endif

/* The implicit destructor (game/menu/gtmode/T224_wheel_button_list.h): both vtables,
 * wheel.~Item() (flags 2), Button::~Button (flags 0), then if (flags & 1)
 * the class operator delete. */
// FUNC 0x001C14B0 gt2k_menu::WheelButton::~WheelButton
/* implicit: ~Item (member destructor), ~Button (base destructor), operator delete (deleting destructor) */
