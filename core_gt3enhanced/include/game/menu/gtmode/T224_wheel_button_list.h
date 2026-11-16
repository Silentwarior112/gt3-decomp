/* T224_wheel_button_list.h - gt2k_menu::WheelButtonList, gt2k_menu::WheelButton
 * (translation unit T224, unit u067); MenuElement.
 *
 * The wheel list of the garage: one WheelButton per wheel the player owns
 * (GameStatus wheel i), each showing a picture of its wheel ("default_wheel"
 * for the car's own).  The list's "mark" picture is drawn over the wheel
 * fitted to the garage car, the focus starts there, and moving the focus
 * shows the focused wheel on the 3D car (MenuStatus::carItem). */
#ifndef GAME_T224_WHEEL_BUTTON_LIST_H
#define GAME_T224_WHEEL_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* MenuLayoutItem (MenuElement) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, held by value */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList, ButtonData */

/* u067's MenuElement is the layout item of T229: the same 0x6C-byte record
 * (name, flags, x / y / z / w / h, type, active, uv, colors, text).  (u009's
 * MenuElement, with count +0x104 and items +0x108, is another record: T241's
 * LinkPage.) */
typedef gt2k_menu::MenuLayoutItem MenuElement;

namespace gt2k_menu {
class MenuControl;                          /* T214 (pointers / references only) */
/* MenuResource: declared by T244 (T235's layout item). */

/* WheelButton is defined before WheelButtonList (u067 matching): egcs
 * writes the typeinfo names of a unit's classes in the reverse order of
 * their definition, and retail T224's .rodata has WheelButtonList's name
 * first (0x00334FB0, right after the vtables), then WheelButton's
 * (0x00334FD0); both __tf functions refer to them. */

/* vtables 0x00334F18 (18 slots: [1] / [2] its own, the others Button's and
 * its bases') and 0x00334EF8 (Cell<Button> part, this-delta -0x100, 3
 * slots).  typeinfo __tfQ29gt2k_menu11WheelButton 0x001C1458
 * (__class_type_info 0x004B2580, base list 0x003525F8: Button, public, +0).
 * Size 0x268 (operator new in createButton 0x001C0F00). */
class WheelButton : public Button {
public:
    /* offPoint is declared fourth, as in Button's constructor (T206): EABI
     * passes it in $f12 wherever it is declared, but the declaration orders
     * the expansion of the call's arguments, and retail's createButton
     * 0x001C0EC8 sets $f12 before $a3 (exact only so). */
    WheelButton(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
                ButtonData *linkMenu, MenuLayoutItem *elem, Item *mark,
                int wheelIndex);            /* 0x001C10B8 Button(..., relative 1, depth wheelIndex) */
    /*  [0] +0x0C 0x001C14B0 the implicit destructor: wheel.~Item, ~Button */
    virtual void virtual_1();               /* [1] +0x14 0x001C1280 Button's update, then the wheel's */
    virtual void virtual_2();               /* [2] +0x1C 0x001C12B0 the button, the mark (fitted wheel),
                                               the focus, the wheel picture */
    /* Inline in the original (no callers; 0x001C1450 is egcs's copy). */
    int getWheelIndex() { return wheelIndex; }  /* 0x001C1450 */

    int   wheelIndex;                       /* 0x160 GameStatus wheel index */
    Item  wheel;                            /* 0x164 picture of the wheel (its name, else "default_wheel") */
    Item *mark;                             /* 0x264 the list's mark */
};
EGCS_SIZE(gt2k_menu::WheelButton, 0x268);

/* vtables 0x00334E88 (13 slots: [1] ButtonList's, [2] Drawable's, [3] / [4]
 * Object's, [6..9] / [11] ButtonList's) and 0x00334E70 (List<Button> part,
 * this-delta -0xB0, 2 slots).  typeinfo __tfQ29gt2k_menu15WheelButtonList
 * 0x001C1388 (__class_type_info 0x004B2570, base list 0x003525F0: ButtonList,
 * public, +0).  Size 0x228 (operator new in T044 0x0011A800).
 * The layout items are MenuResource here, as in the base ButtonList (T244):
 * T235's name of T229's MenuLayoutItem (see the T229 header); WheelButton
 * takes them as MenuLayoutItem, as its base Button (T206) does. */
class WheelButtonList : public ButtonList {
public:
    WheelButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                    int count, MenuResource *elem, MenuResource *rects,
                    int defaultIndex);      /* 0x001C0D18 elem: the list's own rectangle; the focus
                                               starts on the fitted wheel, else defaultIndex */
    /*  [0] +0x0C 0x001C13E0 the implicit destructor: mark.~Item, ~ButtonList */
    virtual Button *createButton(MenuResource *elem, ButtonData *data, f32 offPoint,
                                 int index); /* [5] +0x34 0x001C0EC8 a WheelButton (wheel index =
                                               index), attached to the list */
    virtual int  getCursor();               /* [10] +0x5C 0x001C0F90 the focused button's wheelIndex */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x001C0FC0 ButtonList's; a move
                                               shows the focused wheel on the car */

    Item mark;                              /* 0x128 picture "mark" (on the fitted wheel) */
};
EGCS_SIZE(gt2k_menu::WheelButtonList, 0x228);

/* 1 if GameStatus wheel `wheelIndex' is the one fitted to the garage car
 * (both unnamed, or the same name). */
int wheel_is_current(int wheelIndex);       /* 0x001C11C0 */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tfQ29gt2k_menu15WheelButtonList(void);  /* 0x001C1388 */
void *__tfQ29gt2k_menu11WheelButton(void);      /* 0x001C1458 */
}
#endif

#endif /* GAME_T224_WHEEL_BUTTON_LIST_H */
