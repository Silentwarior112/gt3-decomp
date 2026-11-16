/* T208_wheel_shop_button_list.h - gt2k_menu::WheelShopButtonList,
 * gt2k_menu::WheelShopButton (translation unit T208, unit u063),
 * 0x001B56D0..0x001B5E88.
 *
 * The wheel shop list: one button per wheel, each drawn with the wheel's
 * picture (named after the entry, cut at its last '_') and the list's
 * "sold_out" picture over it when the player has that wheel already
 * (GameStatus::hasWheel).  The list
 * starts on the wheel of the car being ridden, and whenever the focus moves
 * (or once at the start) it shows the focused wheel on the shop's 3D car
 * (MenuStatus car, a gt2k_menu::Car: Car::setWheel).
 */
#ifndef GAME_T208_WHEEL_SHOP_BUTTON_LIST_H
#define GAME_T208_WHEEL_SHOP_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, held by value */

namespace gt2k_menu {
class MenuControl;                          /* T214 (pointers / references only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct ButtonData;                          /* T244 */

/* vtables 0x003331B0 (18 slots: [1] / [2] its own, the others Button's and
 * its bases') and 0x00333190 (the Cell<Button> part, this-delta -0x100).
 * typeinfo __tfQ29gt2k_menu15WheelShopButton 0x001B5DC0 (__class_type_info
 * 0x004B24B0, base list 0x00352538: {Button, public, +0}).  Size 0x2A4. */
class WheelShopButton : public Button {
public:
    /* Button(...), then the wheel picture: an empty Item given the hidden
     * item named after linkMenu cut at its last '_'.  offPoint is declared
     * among the first four parameters (as T047's TopButton): EABI passes it
     * in $f12 wherever it is, but the declaration orders the expansion of
     * createButton's new-expression, and retail sets $f12 first. */
    WheelShopButton(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
                    ButtonData *linkMenu, MenuLayoutItem *elem, int relative, int depth,
                    Item *soldOut);         /* 0x001B5AF8 */
    /*  [0] +0x0C 0x001B5E18 the implicit destructor (vptrs, wheel, ~Button) */
    virtual void virtual_1();               /* [1] +0x14 0x001B5BE8 Button's, then the wheel's */
    /* Button's draw without its visible test, the wheel (and "sold_out")
     * between the picture and the focus frame. */
    virtual void virtual_2();               /* [2] +0x1C 0x001B5C18 */

    Item *soldOut;                          /* 0x160 the list's soldOut */
    Item  wheel;                            /* 0x164 picture of the wheel */
    char  wheelName[0x40];                  /* 0x264 linkMenu's name cut at its last '_' */
};

/* vtables 0x00333120 (13 slots: [2] Drawable's, [3] / [4] Object's, [6..11]
 * ButtonList's) and 0x00333108 (the List<Button> part, this-delta -0xB0, 2
 * slots).  typeinfo __tfQ29gt2k_menu19WheelShopButtonList 0x001B5CF8
 * (__class_type_info 0x004B24A0, base list 0x00352530: {ButtonList, public,
 * +0}).  Size 0x22C. */
class WheelShopButtonList : public ButtonList {
public:
    /* ButtonList(controlType, loop, layout), the "sold_out" picture, the
     * rectangle (x, y the centre), the buttons; the index goes to the
     * button of the ridden car's wheel; MenuStatus car = NULL (the page sets
     * it).  One caller (T044 0x0011A8E0), which also passes relative (1,
     * $a6) and index ($a7) as ButtonList's building constructor takes them;
     * neither is read (the sweep added the two, u009's TODO).  The floats
     * come before relative and index: the caller sets $a7 after $f12..$f15
     * (EABI numbers the int and float registers apart, so only the
     * caller's argument order shows it; u009 matching, which made that
     * caller exact; this constructor's code is the same either way). */
    WheelShopButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                        int count, f32 x, f32 y, f32 w, f32 h,
                        int relative, int index);   /* 0x001B56D0 */
    /*  [0] +0x0C 0x001B5D50 the implicit destructor (vptrs, soldOut, ~ButtonList) */
    virtual void virtual_1();               /* [1] +0x14 0x001B59B0 ButtonList's, then soldOut's */
    /* A WheelShopButton (relative = unkF8, depth = index) sharing soldOut. */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index);     /* [5] +0x34 0x001B58E0 */
    /* ButtonList's (with buttons); then, once (refresh) or when the focus
     * moved, the focused wheel goes on the shop's car. */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x001B59E0 */

    Item soldOut;                           /* 0x128 picture "sold_out" */
    int  refresh;                           /* 0x228 1: show the focused wheel on the car */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::WheelShopButtonList, 0x22C);
EGCS_SIZE(gt2k_menu::WheelShopButton, 0x2A4);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu19WheelShopButtonList(void);  /* 0x001B5CF8 */
void *__tfQ29gt2k_menu15WheelShopButton(void);      /* 0x001B5DC0 */
}
#endif

#endif /* GAME_T208_WHEEL_SHOP_BUTTON_LIST_H */
