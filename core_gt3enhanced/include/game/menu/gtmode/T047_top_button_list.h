/* T047_top_button_list.h - gt2k_menu::TopButtonList / gt2k_menu::TopButton,
 * the button list of the GT-mode top menu (translation unit T047, unit
 * u008), 0x001168B8..0x00116E00.
 *
 * A ButtonList (T244) that makes TopButtons: a TopButton fades with its
 * distance from the list's centre line (alphaRatio = 1 - |posY - list y| /
 * (1.8 h), not drawn past it), and the list's scroll target keeps the
 * focused button centred when the buttons do not fit (else a horizontal
 * list is centred, a vertical one starts at the top). */
#ifndef GAME_T047_TOP_BUTTON_LIST_H
#define GAME_T047_TOP_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */

namespace gt2k_menu {
class PageLayout;                           /* T229 (pointers only) */
struct MenuLayoutItem;                      /* T229 */
struct ButtonData;                          /* T244 */

/* TopButton is defined before TopButtonList: egcs writes the type names in
 * reverse definition order, and retail's .rodata has TopButtonList's
 * (0x0030E288) before TopButton's (0x0030E2A8). */

/* Bases: Button (Item at +0x00, Cell<Button> at +0x100).  vtables
 * 0x0030E1F0 (18 slots: Button's but [2]) and 0x0030E1D0 (the Cell part, 3
 * slots, this-delta -0x100).  typeinfo __tfQ29gt2k_menu9TopButton 0x00116D48
 * (class, base list g_00350FD8 {Button, public +0}; storage 0x004B1540).  No
 * data of its own: size 0x160 (operator new in TopButtonList::createButton). */
class TopButton : public Button {
public:
    /* Button(owner, controlType, layout, linkMenu, elem, relative, depth,
     * offPoint).  The float is declared among the first four parameters
     * (EABI passes it in $f12 wherever it is; its place decides the order in
     * which a call expands the arguments): retail's createButton copies it
     * into $f12 before the integer arguments, which only positions 0..3 give
     * (3 chosen, ? the original's). */
    TopButton(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
              ButtonData *linkMenu, MenuLayoutItem *elem, int relative,
              int depth);                   /* 0x00116BB0 Button(...) */
    /*  [0] +0x0C 0x00116DA0 ~TopButton: implicit (see TopButtonList) */
    /* Button::virtual_2 without its visible test; drawItem / drawFocus only
     * within 1.8 heights of the owner's y, alphaRatio by that distance
     * unless decided. */
    virtual void virtual_2();               /* [2] +0x1C 0x00116BF0 */
};
EGCS_SIZE(gt2k_menu::TopButton, 0x160);

/* Bases: ButtonList (Drawable at +0x00, List<Button> at +0xB0).  vtables
 * 0x0030E160 (13 slots: [1] / [6] / [8]..[12] ButtonList's) and 0x0030E148
 * (the List<Button> part, 2 slots, this-delta -0xB0).  typeinfo
 * __tfQ29gt2k_menu13TopButtonList 0x00116C90 (class, base list g_00350FD0
 * {ButtonList, public +0}; storage 0x004B1530).  No data of its own: size
 * 0x128 (operator new in T044 F_0011B468). */
class TopButtonList : public ButtonList {
public:
    /* ButtonList(controlType, loop, layout); the centre (x, y) and size,
     * the buttons of items (createButtons, called directly), the cursor on
     * index.  relative is not read (T044 passes 1).  The floats come
     * before relative and index: T044's call sets $a7 after $f12..$f15
     * (EABI numbers the int and float registers apart; u009 matching, which
     * made that caller exact; this constructor's code is the same). */
    TopButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                  int count, f32 x, f32 y, f32 w, f32 h, int relative,
                  int index);               /* 0x001168B8 */
    /*  [0] +0x0C 0x00116CE8 ~TopButtonList: implicit.  The destructors of
     * both classes are implicit in the original: retail's frame info
     * (0x00295060) has each one after its class's __tf (the end-of-unit
     * group: __tf, then the implicit members), where a user-declared one
     * would come at its definition.  They still set both vtables: egcs
     * skips that in an empty destructor only without multiple inheritance
     * in the hierarchy (ButtonList and Button have two bases). */
    /* A TopButton (owner this, the list's controlType, layout and relative,
     * depth index), linked under the list. */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index); /* [5] +0x34 0x001169C8 */
    virtual f32  scrollTarget();            /* [7] +0x44 0x00116A90 */
};
EGCS_SIZE(gt2k_menu::TopButtonList, 0x128);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13TopButtonList(void);    /* 0x00116C90 */
void *__tfQ29gt2k_menu9TopButton(void);         /* 0x00116D48 */
}
#endif

#endif /* GAME_T047_TOP_BUTTON_LIST_H */
