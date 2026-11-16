/* T206_button.h - gt2k_menu::Button (translation unit T206, unit u063),
 * 0x001B3BE8..0x001B4900.
 *
 * The button of a ButtonList (T244): an Item (its picture) and a list cell.
 * It keeps its position along the list (offPoint); each frame update works
 * out its screen position (posX, posY) from the list's position and scroll
 * offset, and the draw places the Item there for the time of the draw
 * (beginDraw / endDraw), clipped to the list.  A focused button draws its
 * "active" picture (or a grey quad) and the list's cursor picture over it,
 * blinking; a button whose check function says it is not available is drawn
 * half-toned, with the list's "disable" picture over it.
 *
 * Also emitted here (with the Cell part's vtable): Cell<Button>::~Cell
 * 0x001B47C0, the instance of the template destructor
 * (menu_list.h).
 */
#ifndef GAME_T206_BUTTON_H
#define GAME_T206_BUTTON_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/menu_list.h"                  /* base gt2k_menu::Cell<T> */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
struct ButtonData;                          /* T244: the 0x14C-byte record of an entry */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T206_button.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* Item at +0x00, Cell<Button> at +0x100 (the __class_type_info base list
 * 0x00332C78: {Item, public +0}, {Cell<Button>, public +0x100}).  vtables
 * 0x00332B60 (18 slots, vptr +0x1C: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's) and 0x00332B40
 * (the Cell part, 3 slots, this-delta -0x100, vptr +0x108).  typeinfo
 * __tfQ29gt2k_menu6Button 0x001B4850 (__class_type_info 0x004B2450).  Size
 * 0x160 (operator new in T244 createButton; the data of MapButton,
 * WheelShopButton, GarageButton, WheelButton start at +0x160). */
class Button : public Item, public Cell<Button> {
public:
    /* Item(elem, layout), then: an element with flag 8 gets its "active"
     * picture (elem->active); z goes up by 100 per layout level and down by
     * 0.001 per depth unit.  offPoint is declared fourth (u008 / u063
     * matching): EABI passes it in $f12 wherever it is declared, but the
     * declaration orders the expansion of a call's arguments, and retail's
     * callers (ButtonList::createButton 0x001C84A8, exact only so) set
     * $f12 before the integer arguments; positions 0..3 give the same
     * code, every constructor that calls this one is unchanged. */
    Button(ButtonList *owner, int controlType, PageLayout *layout, f32 offPoint,
           ButtonData *linkMenu, MenuLayoutItem *elem, int relative, int depth);  /* 0x001B3C50 */
    virtual ~Button();                      /* [0] +0x0C 0x001B3E60 deletes activeItem */
    virtual void virtual_1();               /* [1] +0x14 0x001B4008 update: activeItem's and Item's, the
                                               blink of the focus frame, posX / posY */
    virtual void virtual_2();               /* [2] +0x1C 0x001B44E8 draw (when visible and not clipped) */
    virtual void virtual_8();               /* [8] +0x4C 0x001B4548 empty */
    virtual void virtual_9(int visible);    /* [9] +0x54 0x001B4550 visible = v */
    /* Empty: the buttons with a list of their own (T268, T269) pass the
     * list's decide callback on (ButtonList::setDecideFunc; func is T244's
     * ButtonDecideFunc, spelled out to keep T244 out of this header). */
    virtual void virtual_10(void *ctx, void (*func)(void *ctx, ButtonData *data, int index,
                                                    ButtonList *list, int a4)); /* [10] +0x5C 0x001B4558 */
    /* One frame of input given to the focused button (ButtonList::doControl);
     * 1: the list goes on with it. */
    virtual int  virtual_11(const MenuControl &ctl); /* [11] +0x64 0x001B4560 returns 1 */
    virtual void virtual_12();              /* [12] +0x6C 0x001B4568 empty */
    virtual void virtual_13();              /* [13] +0x74 0x001B4570 empty: the focus comes (setFocus) */
    virtual void virtual_14();              /* [14] +0x7C 0x001B4578 empty: the focus goes */
    virtual void virtual_15();              /* [15] +0x84 0x001B4580 the decide effect (a DecideEffect,
                                               T222; waits a vsync at a time until it is done) */
    virtual void virtual_16();              /* [16] +0x8C 0x001B4660 blinks for 30 frames (alpha 0.4 .. 1) */
    virtual void virtual_17();              /* [17] +0x94 0x001B4718 fades for 30 frames (alpha 1 .. 0.2) */

    void placeOffPoint();                   /* 0x001B3BE8 relative: dx (horizontal list) or dy = offPoint */
    /* check(ctx, linkMenu) says whether the button is available. */
    void setCheck(int (*fn)(void *ctx, ButtonData *data), void *ctx);  /* 0x001B3F08 */
    int  isAvailable();                     /* 0x001B3F18 1 without a check */
    MenuLayoutItem *getElement();           /* 0x001B3F50 */
    /* virtual_13 / 14 on a change; a focused button shows the list's help
     * layers (ButtonList::showHelpLayer / 2, the record's +0x80 / +0x40). */
    void setFocus(int focus);               /* 0x001B3F58 */
    void beginDraw();                       /* 0x001B4130 clip to the list, move the Item to posX / posY */
    void drawItem();                        /* 0x001B41F8 */
    void drawFocus();                       /* 0x001B4288 */
    void endDraw();                         /* 0x001B4478 */
    int  isClipped();                       /* 0x001B44C0 */
    f32  getPosY();                         /* 0x001B47E8 (inline accessors: out-of-line copies,
                                               no callers) */
    f32  getPosX();                         /* 0x001B47F0 */
    Item *getActiveItem();                  /* 0x001B47F8 */
    void setDecide();                       /* 0x001B4800 */
    ButtonData *getLinkMenu() const;        /* 0x001B4810 */
    void setOffPoint(f32 p);                /* 0x001B4818 and placeOffPoint() */
    int  isDisableFocus() const;            /* 0x001B4838 */
    void setDisableFocus(bool b);           /* 0x001B4840 */
    int  isFocused() const;                 /* 0x001B4848 */

    /*  Cell<Button>                           0x100 (next, prev, vptr +0x108) */
    int             controlType;            /* 0x10C the owner's: 0 vertical list, else horizontal */
    PageLayout     *layout;                 /* 0x110 */
    ButtonList     *owner;                  /* 0x114 */
    ButtonData     *linkMenu;               /* 0x118 the entry's record (name at +0, help layers at
                                               +0x40 / +0x80) */
    MenuLayoutItem *element;                /* 0x11C */
    Item           *activeItem;             /* 0x120 picture drawn while focused (element flag 8) */
    f32             offPoint;               /* 0x124 offset along the list */
    f32             baseZ;                  /* 0x128 z before the depth adjustment */
    int             depth;                  /* 0x12C */
    int             focused;                /* 0x130 */
    int             relative;               /* 0x134 positioned relative to the owner (and clipped by it) */
    int             disableFocus;           /* 0x138 the cursor skips it */
    int             decide;                 /* 0x13C setDecide */
    int           (*check)(void *ctx, ButtonData *data);  /* 0x140 NULL: always available */
    void           *checkCtx;               /* 0x144 */
    f32             blink;                  /* 0x148 grey level of the focus frame (0.5) */
    int             effectRunning;          /* 0x14C during virtual_16 / virtual_17 */
    f32             savedX;                 /* 0x150 dx.value while drawn */
    f32             savedY;                 /* 0x154 */
    f32             posX;                   /* 0x158 position on the screen (update) */
    f32             posY;                   /* 0x15C */
};

/* The inline accessors 0x001B47E8..0x001B4848: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline int Button::isFocused() const { return focused; }  /* 0x001B4848 */
inline void Button::setDisableFocus(bool b) { disableFocus = b; }  /* 0x001B4840 */
inline int Button::isDisableFocus() const { return disableFocus; }  /* 0x001B4838 */
/* 0x001B4818 */
inline void Button::setOffPoint(f32 p)
{
    offPoint = p;
    placeOffPoint();
}
inline ButtonData *Button::getLinkMenu() const { return linkMenu; }  /* 0x001B4810 */
inline void Button::setDecide() { decide = 1; }  /* 0x001B4800 */
inline Item *Button::getActiveItem() { return activeItem; }  /* 0x001B47F8 */
inline f32 Button::getPosX() { return posX; }  /* 0x001B47F0 */
inline f32 Button::getPosY() { return posY; }  /* 0x001B47E8 */

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::Button, 0x160);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu6Button(void);                        /* 0x001B4850 */
void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu6Button(void);    /* 0x001B48B0 (__si_type_info 0x004B14C0) */
extern u32 __tiQ29gt2k_menu6Button[4];      /* 0x004B2450 its __class_type_info (4 words), filled by
                                               __tfQ29gt2k_menu6Button */
}
#endif

#endif /* GAME_T206_BUTTON_H */
