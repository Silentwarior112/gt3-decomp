/* T219_garage_button_list.h - gt2k_menu::GarageButtonList,
 * gt2k_menu::GarageButton, gt2k_menu::PicData (translation unit T219, unit
 * u066), 0x001BD710..0x001BF390.
 *
 * The car list of the garage ("hm/hm_gr_panel"): one button per garage car
 * with an info column (power, price, maker, distance, drive, aspiration: the
 * shared SmartIndexT infoMode, moved by L2 / R2 of the page control's first
 * pad, ControlBase::control1.l2 / r2), a colour chip and a mark on the current
 * car, sorted by one of nine orders.
 *
 * File-local (static) in T219, so not declared here: garage_info_mode
 * 0x001BD710 (a CallbackItem select function: infoMode->value) and the qsort
 * comparators of the sort orders, the table g_002C8DF8 (T219 .data) by
 * sortType: 0 cmp_index 0x001BEBD8 (garage index), 1 cmp_name 0x001BEF80,
 * 2 cmp_maker 0x001BEB70, 3 cmp_color 0x001BEC30 (T218 color_compare_hsv),
 * 4 cmp_price 0x001BEAA8, 5 cmp_distance 0x001BEF00, 6 cmp_power 0x001BEA78,
 * 7 cmp_drive 0x001BEFE8, 8 cmp_aspiration 0x001BF050.  The record handed to
 * the page opened (g_0049C840, a ButtonData copy, .bss) is only used here.
 */
#ifndef GAME_T219_GARAGE_BUTTON_LIST_H
#define GAME_T219_GARAGE_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item, held by value */

class GarageCar;                            /* T036 (pointers only) */
namespace gt2k_game {
class GameStatus;                           /* T429 */
}

namespace gt2k_menu {
template <class T> class SmartIndexT;       /* T251 (pointers only) */
class ControlBase;                          /* T247 */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */
struct ButtonData;                          /* T244 */

/* u066's PicData (and u008's) is the 0x6C-byte layout item of T229: x / y
 * (top left), z, w, h at +0x44..+0x54. */
typedef MenuLayoutItem PicData;

/* vtables 0x003345A0 (18 slots: [0] / [2] its own, the others Button's and
 * its bases') and 0x00334580 (the Cell<Button> part, this-delta -0x100, 3
 * slots).  typeinfo __tfQ29gt2k_menu12GarageButton 0x001BF2B8
 * (__class_type_info 0x004B2510, base list 0x003525E0: {Button, public, +0}).
 * Size 0x3BC (operator new in GarageButtonList::createButton). */
class GarageButton : public Button {
public:
    /* Button(owner, controlType, layout, offPoint, linkMenu, pic, relative 1,
     * depth index), then the car's power and its rpm.  offPoint is declared
     * after layout, as in Button's constructor (EABI passes it in $f12
     * wherever it is declared; the declaration orders the expansion of
     * createButton's arguments: positions 0..4 give retail's code, u066). */
    GarageButton(gt2k_game::GameStatus *gs, ButtonList *owner, int controlType,
                 PageLayout *layout, f32 offPoint, ButtonData *linkMenu, MenuLayoutItem *pic,
                 Item *mark, Item *colorItem, int index,
                 SmartIndexT<int> *infoMode);   /* 0x001BDFF8 */
    /*  [0] +0x0C the implicit destructor 0x001BF310 (data pass: T219's frame
     *  info has it after __tf, as egcs emits a synthesized member; the
     *  members infoA / info, Button::~Button, Object::operator delete) */
    /*  [1] +0x14 Button::virtual_1 0x001B4008 */
    /* Button's draw steps with, between them: the mark left of the current
     * car's button, the colour chip at the left edge, the info column
     * (infoMode: 0 power, 1 price, 2 maker, 3 distance, 4 drive, 5
     * aspiration; infoA while focused). */
    virtual void virtual_2();               /* [2] +0x1C 0x001BE1F0 */
    /* info / infoA: Item::setResource(item, texture) each. */
    void setInfoResources(MenuLayoutItem *info, MenuTexture *infoTex,
                          MenuLayoutItem *infoA, MenuTexture *infoATex);  /* 0x001BE148 (F_001BE148) */
    GarageCar *car();                       /* 0x001BE198 gs->F_00219460(index) */
    f32  xAtPercent(f32 percent);           /* 0x001BE1B8 (F_001BE1B8) x at percent of the width (from the left edge) */
    /* Inline (T219 writes the copies with the vtable, no callers: the frame
     * info has getPower 0x001BF2A8, then getIndex 0x001BF2B0, i.e. declared
     * in this order reversed). */
    int  getIndex() { return index; }       /* 0x001BF2B0 */
    int  getPower() { return power; }       /* 0x001BF2A8 */

    gt2k_game::GameStatus *gs;              /* 0x160 */
    int               index;                /* 0x164 garage car index */
    Item             *mark;                 /* 0x168 the list's "mark" (current car) */
    Item             *colorItem;            /* 0x16C the list's "color" chip */
    Item              info;                 /* 0x170 the info text ("info") */
    Item              infoA;                /* 0x270 the info text when focused ("info_a") */
    SmartIndexT<int> *infoMode;             /* 0x370 the info column 0..5 */
    char              text[0x40];           /* 0x374 */
    int               power;                /* 0x3B4 GarageCar::F_0023B110 ('ps') */
    int               rpm;                  /* 0x3B8 */
};

/* vtables 0x00334510 (13 slots: [1] ButtonList's, [2] Drawable's, [3] / [4]
 * Object's, [6..8] / [11] ButtonList's) and 0x003344F8 (the List<Button>
 * part, this-delta -0xB0, 2 slots: [1] sort).  typeinfo
 * __tfQ29gt2k_menu16GarageButtonList 0x001BF1D0 (__class_type_info
 * 0x004B2520, base list 0x003525D8: {ButtonList, public, +0}).  Size 0x33C
 * (operator new in T044 F_0011A2B0 / F_0011A4E8, page "hm/hm_gr_panel"). */
class GarageButtonList : public ButtonList {
public:
    /* ButtonList(controlType, loop, layout), the "mark" / "color" pictures,
     * frame's rectangle, the buttons (pics: their elements), the cursor on
     * select (0 or out of range: the menu's car, else the current car, else
     * 0), sorted by sortType. */
    GarageButtonList(gt2k_game::GameStatus *gs, const char *page, int controlType, int loop,
                     PageLayout *layout, ButtonData **items, int count, MenuLayoutItem *frame,
                     MenuLayoutItem *pics, int select, int sortType,
                     SmartIndexT<int> *infoMode);   /* 0x001BD720 */
    /*  [0] +0x0C the implicit destructor 0x001BF228 (after __tf in T219's
     *  frame info: the members color / mark, ButtonList::~ButtonList,
     *  Object::operator delete) */
    /* A GarageButton for garage car index (sharing mark, color, infoMode;
     * relative 1 whatever the list's), attached to the list. */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index);     /* [5] +0x34 0x001BDC30 */
    virtual void refresh();                 /* [9] +0x54 0x001BDBE8 ButtonList's, then sortButtons by
                                               g_002C8DF8[sortType] */
    virtual int  getCursor();               /* [10] +0x5C 0x001BDD10 the focused button's garage index, 0 */
    /* The info column moves (owner->control1.l2 / r2); with a page, START
     * (ctl.getButtonsOn() & CONTROL_START) makes the focused car the menu's
     * car and opens the page; ctl.cancel puts the current car back (else
     * garage car 0); then ButtonList's. */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x001BDE80 */
    /* List<Button> slot 1 (pList::sort, the secondary vtable): qsort of the
     * buttons with cmp, then appended back. */
    virtual void sort(int (*cmp)(const void *, const void *));  /* 0x001BF0B8 */
    void openPage();                        /* 0x001BDD48 (F_001BDD48) opens `page' with the focused entry (a copy of
                                               its record, link = page; decideButton) */
    /* Inline (T219 writes the copy with the vtable, no callers). */
    int  getInfoMode() { return infoMode->value; }  /* 0x001BF1C0 */

    gt2k_game::GameStatus *gs;              /* 0x128 */
    const char       *page;                 /* 0x12C the page START opens, NULL: none */
    ControlBase      *owner;                /* 0x130 layout->owner: the page control */
    Item              mark;                 /* 0x134 "mark" */
    Item              color;                /* 0x234 "color" */
    SmartIndexT<int> *infoMode;             /* 0x334 */
    int               sortType;             /* 0x338 index in g_002C8DF8 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::GarageButton, 0x3BC);
EGCS_SIZE(gt2k_menu::GarageButtonList, 0x33C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu16GarageButtonList(void); /* 0x001BF1D0 */
void *__tfQ29gt2k_menu12GarageButton(void);     /* 0x001BF2B8 */
}
#endif

#endif /* GAME_T219_GARAGE_BUTTON_LIST_H */
