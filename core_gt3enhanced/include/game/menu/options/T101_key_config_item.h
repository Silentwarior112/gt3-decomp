/* T101_key_config_item.h - gt2k_menu::KeyConfigItem, the button assignment
 * page of one controller (translation unit T101, unit u014),
 * 0x0012AB90..0x0012C300.
 *
 * Built by the option page hooks (T064 option_create_key_config_1 ...,
 * operator new 0x1180), e.g. for player 1's pad KeyConfigItem(owner, item, tex, &owner->control1,
 * &owner->pages, layout, 1, 0, 1, 0).  A row per function (steering,
 * accelerate, brake, back, side brake, shift up, shift down, rear view,
 * change view; the USB wheel page only the last six) and a "default" row;
 * each row offers the buttons of g_0029BE98[mode][row] (T101 .data), three
 * of them drawn around the choice, scrolling sideways inside the row label.
 * The choices go straight into the owner's ControllerConfig (T426,
 * setKey); the page can only be left (checkExit) while no two rows use the
 * same button or overlapping stick inputs (hasConflict).  mode: 0 digital
 * pad, 1 analog pad, 2 / 3 USB wheels (3 shows the "jaguar_*" pictures), 5
 * none.  Row functions (rowFunc): 0 steering .. 8 change view, 10 default.
 * Button ids (picture names g_0029BD80): 0 cross_lr, 1 / 2 the sticks' x
 * axes, 3..10 the stick directions, 11.. maru, batu, sankaku, sikaku, r1,
 * r2, l1, l2, start, 0x15..0x1A usb_l1..usb_r3, 0x22 default_item,
 * 0x23..0x28 jaguar_l1..jaguar_r3.  OptionControl (T062) keeps the page of
 * each player and calls handleInput / checkExit every frame. */
#ifndef GAME_T101_KEY_CONFIG_ITEM_H
#define GAME_T101_KEY_CONFIG_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value (labels, ...) */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> (cursor, select) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (scroll) */

namespace gt2k_menu {
class OptionControl;                        /* T062 (pointers only) */
class MenuControl;                          /* T214 */
class PageManager;                          /* T211 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
struct MenuTexture;                         /* T213 */

/* vtable 0x00314340 (8 slots: [3] .. [7] as Item's).  typeinfo
 * __tfQ29gt2k_menu13KeyConfigItem 0x0012C270 (si, base Item; storage
 * 0x004B1710; the unit also has a copy of SmartIndexT<int>'s vtable,
 * 0x00314388).  Size 0x1180 (operator new in the T064 hooks). */
class KeyConfigItem : public Item {
public:
    /* Item(res, tex); the members in order (blink 0, wait 3); the rows of
     * the page (isUsb), the 41 button pictures that exist in the layout
     * (default_item only when editable), the mode (updateMode), the row
     * labels, and when editable "bad", "warning", "arrow_l", "arrow_r" and
     * pages->noCancel = 1; the selection; registered with the owner
     * (attachKeyConfig1 / attachKeyConfig2); a vsync. */
    KeyConfigItem(OptionControl *owner, MenuLayoutItem *res, MenuTexture *tex, MenuControl *pad,
                  PageManager *pages, PageLayout *layout, int mode, int port, int editable,
                  int usb);                 /* 0x0012AB90 */
    /* Deletes the button pictures, leaves the owner
     * (detachKeyConfig1 / detachKeyConfig2), pages->noCancel = 0. */
    virtual ~KeyConfigItem();               /* [0] +0x0C 0x0012B120 */
    /* update: Item's, the blink (0.5 -> 0, again), the rows' scroll eases
     * to their choice (rate 0.3). */
    virtual void virtual_1();               /* [1] +0x14 0x0012B6A8 */
    /* draw: nothing without a controller; the rows (drawRow), one frame
     * late when not editable; "warning" while a row conflicts. */
    virtual void virtual_2();               /* [2] +0x1C 0x0012BE20 */

    int  isUsb();                           /* 0x0012B2A8 usb */
    int  rowFunction(int row);              /* 0x0012B2B0 rowFunc[row] */
    /* Every row's choices from the mode's table, the choice from the
     * owner's current assignment (the default row: 0). */
    void resetSelection();                  /* 0x0012B2F0 */
    int  selectedOption(int row);           /* 0x0012B528 the button id chosen (0 without a controller) */
    /* The mode of the controller now connected (the USB page: the owner's
     * control of the port; the pad page: pad); none: mode 5, after
     * resetting a conflicting assignment to the defaults (resetKeys). */
    void updateMode();                      /* 0x0012B580 */
    int  hasConflict(int row);              /* 0x0012B768 */
    int  anyConflict();                     /* 0x0012B898 */
    void drawRow(int row);                  /* 0x0012B918 */
    /* A frame of input (OptionControl::mainLoop): up / down the row,
     * left / right its choice, decide on the default row resets the mode's
     * assignment; then the rows are written back. */
    void handleInput();                     /* 0x0012BF08 */
    /* Decide (not on the default row) or cancel leave the page
     * (pages->exitRequest) when this page and, on the pad page, the other
     * player's (if it has a controller) are free of conflicts; else menu
     * sound 4.  Nothing for the first `wait' frames. */
    void checkExit(KeyConfigItem *other);   /* 0x0012C158 */
    int  getPort();                         /* 0x0012C268 (no callers) */

    int              port;                  /* 0x100 0 player 1, 1 player 2 */
    OptionControl   *owner;                 /* 0x104 */
    int              mode;                  /* 0x108 */
    int              usb;                   /* 0x10C 1: the USB wheel page */
    MenuControl     *pad;                   /* 0x110 the pad that edits this page */
    PageManager     *pages;                 /* 0x114 &owner->pages */
    int              rows;                  /* 0x118 10 (pad) or 7 (USB) */
    const int       *rowFunc;               /* 0x11C g_003146C0 (pad) or g_003146E8 (USB) */
    Item            *buttons[41];           /* 0x120 one per button id, NULL if the layout has none */
    Item             labels[10];            /* 0x1C4 the row labels */
    int              defaultRow;            /* 0xBC4 9 (pad) or 6 (USB) */
    SmartIndexT<int> cursor;                /* 0xBC8 the current row (wraps) */
    SmartIndexT<int> select[10];            /* 0xBE0 the choice of each row (clamps) */
    f32              selectPos[10];         /* 0xCD0 (f32)select[i].value */
    MotionValue      scroll[10];            /* 0xCF8 follows selectPos */
    f32              blink;                 /* 0xD70 0.5 -> 0, then again */
    Item             arrowL;                /* 0xD74 "arrow_l" */
    Item             arrowR;                /* 0xE74 "arrow_r" */
    Item             bad;                   /* 0xF74 "bad" over a conflicting row */
    Item             warning;               /* 0x1074 "warning" while any row conflicts */
    int              wait;                  /* 0x1174 frames before checkExit acts (3) */
    int              hideDelay;             /* 0x1178 frames before the rows are drawn */
    int              editable;              /* 0x117C */
};
EGCS_SIZE(gt2k_menu::KeyConfigItem, 0x1180);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu13KeyConfigItem(void);    /* 0x0012C270 */
#endif

#endif /* GAME_T101_KEY_CONFIG_ITEM_H */
