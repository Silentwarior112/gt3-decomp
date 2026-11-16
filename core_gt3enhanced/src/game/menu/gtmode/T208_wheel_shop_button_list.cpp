/* T208 - gt2k_menu::WheelShopButtonList and gt2k_menu::WheelShopButton.
 * 0x001B56D0..0x001B5E88.
 *
 * The wheel shop list: one button per wheel, each drawn with the wheel's
 * picture (named after the entry, cut at its last '_') and the list's
 * "sold_out" picture over it when the player already has that wheel
 * (GameStatus::hasWheel).  The list starts on the wheel of the car being
 * ridden, and whenever the focus moves (or once at the start) it shows the
 * focused wheel on the shop's 3D car (MenuStatus::carItem).
 *
 * Data (retail): .rodata 0x003330E0..0x003334BE: "(%f, %f, %f) %f %f %f\n"
 * (SimpleCameraBase::print's, an inline function's literal written where
 * its header is read: unreferenced, it opens the file's .rodata),
 * "sold_out", the two classes' vtables, then the type names
 * WheelShopButtonList, WheelShopButton, SmartIndexT<int>, List<Button>,
 * Cell<Button>, the operator list classes (Item's), the garage lists
 * (GameStatus's), Operator: reverse definition order, so the original read
 * the GameStatus first, then the Button header (Item, Cell<Button>), then
 * ButtonList's (List<Button>, SmartIndexT<int>).  .sdata
 * 0x00352530..0x00352540: the two base lists. */
// DATA_START 0x003330E0
#include "game/camera/T412_simple_camera_base.h"   /* (data layout only) SimpleCameraBase::print's literal */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus (hasWheel, getRunningCar),
                                               MenuStatus::carItem, GarageCar::getWheelModelName */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Item, Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x003332B0 / 0x00333290). */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList, ButtonData */
#include "game/menu/gtmode/T208_wheel_shop_button_list.h"
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car::setWheel (MenuStatus::carItem) */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (getPrev / getNext) */
#include "game/common/T494_text_utils.h"           /* str_equal, str_cut_last_underscore */
#include "game/menu/framework/menu_buttons_internal.h"

namespace gt2k_menu {

/* One caller.  ButtonList(controlType, loop, layout), the "sold_out"
 * picture, the rectangle, the buttons; the index goes to the button of the
 * ridden car's wheel; the shop's car is cleared (the page sets it).
 * relative_ and index_ are not read.  (The float parameters come before
 * relative_ and index_, as the caller's argument order shows: u009 notes;
 * this constructor's code is the same either way.) */
// FUNC 0x001B56D0 gt2k_menu::WheelShopButtonList::WheelShopButtonList
WheelShopButtonList::WheelShopButtonList(int controlType_, int loop_, PageLayout *layout_,
                                         ButtonData **items, int count,
                                         f32 centreX, f32 centreY, f32 width, f32 height,
                                         int relative_, int index_)
    : ButtonList(controlType_, loop_, layout_), soldOut("sold_out", layout_) /* implicit: Item (member constructor), ~Item (exception cleanup) */
                                            /* Item(name, layout); ~Item on unwinding */
{
    x.set(centreX);
    y.set(centreY);
    w = width;
    h = height;
    refresh = 1;
    ButtonList::createButtons(items, count, 0);     /* a direct call in the original */
    resetIndex();

    GarageCar *car = getGameStatus()->getRunningCar();
    if (car != 0) {
        const char *carWheel = car->getWheelModelName();
        ListIterator<Button> it(this);      /* (declared before the buffer: retail's frame) */
        char wheel[0x40];
        int i = 0;
        Button *b;
        while ((b = it.next()) != 0) {
            str_cut_last_underscore(wheel, b->linkMenu->name);
            if (str_equal(wheel, carWheel) != 0)
                break;
            i++;
        }
        if (b != 0)
            index.set(i);
        /* Left (92.2%): retail keeps this loop's `b test' entry with only
         * the iterator's test moved to the bottom; here stmt.c's
         * expand_end_loop moves the test and the body up to the break (it
         * looks 30 RTL insns ahead for the last exit; ours reaches the
         * break in 24, retail's needed more than 30: four more temporaries
         * before the cut give retail's code).  No natural form found
         * (accessors, temporaries, helpers, for / while forms). */
    }
    getGameStatus()->getMenuStatus()->carItem = 0;
}

/* Creates a WheelShopButton (0x2A4 bytes) sharing the list's "sold_out"
 * picture; depth = index.  Exact since the virtual is declared
 * createButton(elem, data, offPoint, index) (T244, every override and caller
 * changed by u070; before, 98.1%: `index' / `elem' in $s2 / $s1, retail
 * $s1 / $s2). */
// FUNC 0x001B58E0 gt2k_menu::WheelShopButtonList::createButton
Button *WheelShopButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                          int index_)
{
    WheelShopButton *b = new WheelShopButton(this, controlType, layout, offPoint, data, elem, /* implicit: operator new, operator delete (new-expression) */
                                             relative, index_, &soldOut);
                                            /* Object::operator new (0x2A4 bytes) */
    b->attachTo(this);
    return b;
}

// FUNC 0x001B59B0 gt2k_menu::WheelShopButtonList::virtual_1
void WheelShopButtonList::virtual_1()
{
    ButtonList::virtual_1();
    soldOut.Item::virtual_1();
}

/* Input (ButtonList's), then, if the shop's car exists and this is the
 * first time or the focus moved along the list, the focused wheel goes on
 * the car. */
// FUNC 0x001B59E0 gt2k_menu::WheelShopButtonList::doControl
void WheelShopButtonList::doControl(const MenuControl &ctl)
{
    if (count() != 0) {
        ButtonList::doControl(ctl);
        Car *car = getGameStatus()->getMenuStatus()->carItem;
        if (car != 0 &&
            (refresh != 0 || ctl.getPrev(controlType) != 0 || ctl.getNext(controlType) != 0)) {
            Button *b = static_cast<Button *>(static_cast<Cell<Button> *>(at(index.value)));
            WheelShopButton *wb = dynamic_cast<WheelShopButton *>(b);
            if (wb != 0) {
                char wheel[0x40];
                str_cut_last_underscore(wheel, wb->linkMenu->name);
                car->setWheel(wheel);
            }
            refresh = 0;
        }
    }
}

/* Button(...) with the same arguments, then the wheel picture: an empty
 * Item given the hidden item named after the entry cut at its last '_'. */
// FUNC 0x001B5AF8 gt2k_menu::WheelShopButton::WheelShopButton
WheelShopButton::WheelShopButton(ButtonList *owner_, int controlType_, PageLayout *layout_,
                                 f32 offPoint_, ButtonData *linkMenu_, MenuLayoutItem *elem,
                                 int relative_, int depth_, Item *soldOut_)
    : Button(owner_, controlType_, layout_, offPoint_, linkMenu_, elem, relative_, depth_),
      soldOut(soldOut_), wheel()           /* Item(0, 0): no item, no texture yet */
{
    str_cut_last_underscore(wheelName, linkMenu_->name);
    wheel.setResource(wheelName, layout_);
}

// FUNC 0x001B5BE8 gt2k_menu::WheelShopButton::virtual_1
void WheelShopButton::virtual_1()
{
    Button::virtual_1();
    wheel.Item::virtual_1();
}

/* Button::virtual_2 without its `visible` test, with the wheel picture (and
 * the "sold_out" one over a wheel the player has) between the picture and
 * the focus frame. */
// FUNC 0x001B5C18 gt2k_menu::WheelShopButton::virtual_2
void WheelShopButton::virtual_2()
{
    beginDraw();
    if (isClipped() == 0) {
        drawItem();
        f32 atX = posX;
        f32 atY = posY;
        wheel.x.set(atX);
        wheel.y.set(atY);
        wheel.Item::virtual_2();
        if (getGameStatus()->hasWheel(wheelName) != 0) {
            Item *sold = soldOut;
            sold->x.set(atX);
            sold->y.set(atY);
            sold->virtual_2();
        }
        drawFocus();
    }
    endDraw();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001B5CF8 __tfQ29gt2k_menu19WheelShopButtonList
extern "C" void *__tfQ29gt2k_menu19WheelShopButtonList(void)
{
    if (__tiQ29gt2k_menu19WheelShopButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu19WheelShopButtonList,
                     "Q29gt2k_menu19WheelShopButtonList", g_00352530, 1);
    }
    return __tiQ29gt2k_menu19WheelShopButtonList;
}
#endif

namespace gt2k_menu {

// FUNC 0x001B5D50 gt2k_menu::WheelShopButtonList::~WheelShopButtonList
/* the implicit destructor (game/menu/gtmode/T208_wheel_shop_button_list.h): both vptrs,
 * soldOut.~Item (flags 2), ButtonList::~ButtonList(0), Object::operator
 * delete if flags & 1 */
/* implicit: ~Item (member destructor), ~ButtonList (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x001B5DC0 __tfQ29gt2k_menu15WheelShopButton
extern "C" void *__tfQ29gt2k_menu15WheelShopButton(void)
{
    if (__tiQ29gt2k_menu15WheelShopButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu15WheelShopButton, "Q29gt2k_menu15WheelShopButton",
                     g_00352538, 1);
    }
    return __tiQ29gt2k_menu15WheelShopButton;
}
#endif

namespace gt2k_menu {

// FUNC 0x001B5E18 gt2k_menu::WheelShopButton::~WheelShopButton
/* the implicit destructor (game/menu/gtmode/T208_wheel_shop_button_list.h): both vptrs,
 * wheel.~Item (flags 2), Button::~Button(0), Object::operator delete if
 * flags & 1 */
/* implicit: ~Item (member destructor), ~Button (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */
