/* T207_map_button_list.h - gt2k_menu::MapButtonList, MapButton, MagScrBegin,
 * MagScrEnd (translation unit T207, unit u063), 0x001B4900..0x001B56D0.
 *
 * A map of buttons placed freely on the screen.  The d-pad moves the
 * selection to the button that lies best in the pushed direction
 * (findInDirection: cos^2 of the angle over the distance); the Pointer
 * picture (T223) glides to it, the button under the pointer gets the focus,
 * and the view (viewX, viewY) follows the pointer while the zoom goes to 30 /
 * the selected button's width, kept in [0.5, 2].  MagScrBegin / MagScrEnd (z
 * -1 and 20) bracket the map's drawing with the zoom transform (and the same
 * scale / offset for the system font).  In this build nothing makes a
 * MagScrBegin or MagScrEnd (their constructors have no callers and no table
 * entries, their vtables no other reference), so magBegin stays NULL and the
 * map is drawn unzoomed: the zoom only moves the DecideEffect (MapButton).
 */
#ifndef GAME_T207_MAP_BUTTON_LIST_H
#define GAME_T207_MAP_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue, held by value */

namespace gt2k_menu {
class Pointer;                              /* T223 (pointers only) */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct ButtonData;                          /* T244 */
class MagScrBegin;

/* vtable 0x00332EF8 (5 slots: [1] / [3] / [4] Object's).  typeinfo
 * __tfQ29gt2k_menu11MagScrBegin 0x001B5638 (__si_type_info 0x004B2470, base
 * Object).  Size 0x2C.  Unused in this build: nothing makes one. */
class MagScrBegin : public Object {
public:
    MagScrBegin();                          /* 0x001B5248 zoom 1, centre (320, 224), z -1 */
    /*  [0] +0x0C 0x001B5688 the implicit destructor: only ~Object, no vptr store */
    /* Pushes scale(zoom) * translate(320 / zoom - x, 224 / zoom - y): (x, y)
     * comes to the screen centre; the system font gets the same. */
    virtual void virtual_2();               /* [2] +0x1C 0x001B52C0 */
    void set(f32 zoom, f32 x, f32 y);       /* 0x001B52B0 */

    f32 zoom;                               /* 0x20 */
    f32 x;                                  /* 0x24 centre of the view */
    f32 y;                                  /* 0x28 */
};
/* vtable 0x00332EC8 (5 slots: [1] / [3] / [4] Object's).  typeinfo
 * __tfQ29gt2k_menu9MagScrEnd 0x001B55A0 (__si_type_info 0x004B2490, base
 * Object).  Size 0x20 (no data).  Unused in this build, as MagScrBegin. */
class MagScrEnd : public Object {
public:
    MagScrEnd();                            /* 0x001B5390 z 20 */
    /*  [0] +0x0C 0x001B55F0 the implicit destructor: only ~Object, no vptr store */
    virtual void virtual_2();               /* [2] +0x1C 0x001B53D0 resets the font, pops the matrix */
};
/* vtables 0x00332E30 (18 slots: [15] its own, the others Button's and its
 * bases') and 0x00332E10 (the Cell<Button> part, this-delta -0x100).
 * typeinfo __tfQ29gt2k_menu9MapButton 0x001B54E8 (__class_type_info
 * 0x004B2480, base list 0x00352528: {Button, public, +0}).  Size 0x16C. */
class MapButton : public Button {
public:
    /* Button(owner, 0, layout, linkMenu, elem, not relative, depth, 0.0). */
    MapButton(ButtonList *owner, PageLayout *layout, ButtonData *linkMenu, MenuLayoutItem *elem,
              int depth, MotionValue *viewX, MotionValue *viewY, MotionValue *zoom); /* 0x001B50A8 */
    /*  [0] +0x0C 0x001B5540 the implicit destructor (vptrs, ~Button) */
    virtual void virtual_15();              /* [15] +0x84 0x001B5148 Button's decide effect, bound to the
                                               list's zoom and view (DecideEffect::setView) */

    MotionValue *viewX;                     /* 0x160 the list's viewX */
    MotionValue *viewY;                     /* 0x164 */
    MotionValue *zoom;                      /* 0x168 */
};
/* vtables 0x00332DA0 (13 slots: [2] Drawable's, [3] / [4] Object's, [6] /
 * [7] / [10] / [11] ButtonList's) and 0x00332D88 (the List<Button> part,
 * this-delta -0xB0, 2 slots: [1] pList::sort).  typeinfo
 * __tfQ29gt2k_menu13MapButtonList 0x001B5430 (__class_type_info 0x004B2460,
 * base list 0x00352520: {ButtonList, public, +0}).  Size 0x16C. */
class MapButtonList : public ButtonList {
public:
    /* ButtonList(0, 0, layout), the buttons, the index; the pointer is the
     * hidden item "pointer"; the view starts at the screen centre (320,
     * 224) with zoom 1.  One caller. */
    MapButtonList(ButtonData **items, int count, PageLayout *layout, int index); /* 0x001B4900 */
    /*  [0] +0x0C 0x001B5488 the implicit destructor (vptrs, ~ButtonList) */
    /* Update (ButtonList's is not called): the view follows the pointer
     * (rate 0.1), the zoom 30 / the selected button's width (rate 0.05),
     * clamped to [0.5, 2]; then magBegin->set(). */
    virtual void virtual_1();               /* [1] +0x14 0x001B4CB0 */
    /* A MapButton bound to the view (depth = index; offPoint unused). */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index);     /* [5] +0x34 0x001B4BF0 */
    virtual Button *focusButton();          /* [8] +0x4C 0x001B4DB8 the button under the pointer */
    /* The button at index.value gets the focus and becomes the selected
     * one; the pointer and the view jump to it. */
    virtual void refresh();                 /* [9] +0x54 0x001B4AE0 */
    /* The pointer's target is the selected button (the screen centre if
     * none), the focus goes to it once the pointer is there; the directions
     * move the selection (sound 3), decide calls decide(). */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x001B4F20 */

    int     isPointerOnTarget();            /* 0x001B4DC0 within 12 pixels */
    Button *findInDirection(f32 dirX, f32 dirY);    /* 0x001B4E10 NULL if none in front */
    void    moveSelection(f32 dirX, f32 dirY);      /* 0x001B4EF0 */

    Pointer     *pointer;                   /* 0x128 */
    Button      *focusedButton;             /* 0x12C the button under the pointer (focused) */
    Button      *selected;                  /* 0x130 the button the pointer moves to */
    f32          unk134;                    /* 0x134 (1.0) */
    f32          targetX;                   /* 0x138 */
    f32          targetY;                   /* 0x13C */
    MagScrBegin *magBegin;                  /* 0x140 NULL (the constructor's); nothing sets it (see
                                               the top) */
    void        *unk144;                    /* 0x144 NULL (the constructor's), never read here (? the
                                               MagScrEnd) */
    MotionValue  viewX;                     /* 0x148 follows the pointer */
    MotionValue  viewY;                     /* 0x154 */
    MotionValue  zoom;                      /* 0x160 30 / selected->w, in [0.5, 2] */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MapButtonList, 0x16C);
EGCS_SIZE(gt2k_menu::MapButton, 0x16C);
EGCS_SIZE(gt2k_menu::MagScrBegin, 0x2C);
EGCS_SIZE(gt2k_menu::MagScrEnd, 0x20);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu13MapButtonList(void);    /* 0x001B5430 */
void *__tfQ29gt2k_menu9MapButton(void);         /* 0x001B54E8 */
void *__tfQ29gt2k_menu9MagScrEnd(void);         /* 0x001B55A0 */
void *__tfQ29gt2k_menu11MagScrBegin(void);      /* 0x001B5638 */
}
#endif

#endif /* GAME_T207_MAP_BUTTON_LIST_H */
