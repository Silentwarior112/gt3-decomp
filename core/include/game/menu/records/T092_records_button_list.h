/* T092_records_button_list.h - gt2k_menu::RecordsButtonList and
 * gt2k_menu::RecordsButton, the list of places of the records screen; CdClock
 * (translation unit T092, unit u040), 0x001735A0..0x00174350.
 *
 * create_menu_records (T091) builds the list.  It owns seven labels
 * (RecordsLabels: rank, its shadow, name, time, date, car, car picture)
 * placed like the layout's "record_*" items relative to the layout's first
 * item; every RecordsButton draws its place by moving the labels onto
 * itself and drawing them with its own texts.  Deciding on a place opens a
 * panel page ("can_not_delete" when the records refuse to delete it,
 * "select_panel_code" while the pad bit 0x10000 is held, else
 * "select_panel_normal") on a copy of the button's record (g_0049C0E0).
 *
 * gt2k_menu::MenuItemRecord (u040 / u066's name of the 0x14C-byte record:
 * name +0x00, page +0x100) is T244's ButtonData (link +0x100): a typedef
 * here. */
#ifndef GAME_T092_RECORDS_BUTTON_LIST_H
#define GAME_T092_RECORDS_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList; ButtonData (MenuItemRecord) */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button; T213's Item (RecordsLabel::item) */
#include "engine/pdi/T519_pdicdvd.h"              /* struct CdClock */

/* struct CdClock (a date as the CD clock gives it): T519_pdicdvd.h. */

namespace gt2k_menu {
class ControlBase;                          /* T247 (pointers only) */
class MenuControl;                          /* T214 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class RecordsControl;                       /* T090 */
class RecordsMenuArg;                       /* T089 */
class RecordsButtonList;

/* The 0x14C-byte record of a list entry (see above). */
typedef ButtonData MenuItemRecord;

/* (? name) One label of a line: an Item and its offset from the centre of
 * the layout's first item.  Not polymorphic; 0x108 bytes. */
struct RecordsLabel {
    RecordsLabel() : item() {}              /* inline (in RecordsLabels()): Item(NULL, NULL) */
    /* Moves the label to (offX + x, offY + y), sets its text and draws it
     * (Item::virtual_2, called directly).  The text after the floats: egcs
     * expands the arguments in declared order, and retail's last call in
     * RecordsButton::virtual_2 has the text's %lo in the jal's delay slot
     * (u040 notes). */
    void draw(f32 x, f32 y, const char *text);  /* 0x00173F58 */
    void setOrigin(f32 x, f32 y);           /* 0x00173FB8 offX / offY = the item's centre - (x, y) */

    Item item;                              /* 0x000 */
    f32  offX;                              /* 0x100 */
    f32  offY;                              /* 0x104 */
};
EGCS_SIZE(gt2k_menu::RecordsLabel, 0x108);

/* RecordsButton is defined before RecordsButtonList: egcs emits a unit's
 * vtables, type names and __class_type_info base lists in the reverse
 * order of the classes' definitions, and retail T092 has
 * RecordsButtonList's first (vtables 0x00325708 / 0x00325720 before
 * 0x00325790 / 0x003257B0, names 0x00325848 / 0x00325868, base lists
 * .sdata 0x00351CC0 / 0x00351CC8). */
/* Bases: Button (Item at +0x00, Cell<Button> at +0x100).  vtables
 * 0x003257B0 (18 slots: [0], [2] its own, the rest Button's) and 0x00325790
 * (the Cell part, this-delta -0x100, 3 slots: [0] the destructor, [1] / [2]
 * pCell's).  typeinfo __tfQ29gt2k_menu13RecordsButton 0x00174298
 * (__class_type_info, one public base: Button, base list 0x00351CC8; node
 * 0x004B1D90).  Size 0x170 (operator new in createButton).  One place. */
class RecordsButton : public Button {
public:
    /* Button(list, controlType, layout, linkMenu, elem, relative 1, depth
     * index, offPoint), list, index, number = "%d" of index + 1.  offPoint
     * is declared after layout (as T047's TopButton): egcs expands the
     * arguments in declared order, and createButton's retail sets $f12
     * between $a2 and $a3 (u040 notes). */
    RecordsButton(RecordsButtonList *list, int controlType, PageLayout *layout,
                  f32 offPoint, ButtonData *linkMenu, MenuLayoutItem *elem,
                  int index);               /* 0x00173C78 */
    /* [0] +0x0C 0x001742F0 ~RecordsButton: implicit (virtual through
     * Button); T092's copy, the one in the vtables. */
    /* Button's draw frame (beginDraw, isClipped, drawItem ... drawFocus,
     * endDraw); in between the list's labels with this place's number
     * (shadow first), name, time, date ("\x7F\x7F/\x7F\x7F/\x7F\x7F" without
     * one; the packed date through a CdClock), and if the records show cars
     * the car name and an empty car picture text. */
    virtual void virtual_2();               /* [2] +0x1C 0x00173D20 */
    int getIndex() { return index; }        /* 0x00174290 (inline: T092's copy, no callers) */

    /*                 Button                  0x000 .. 0x160 */
    RecordsButtonList *list;                /* 0x160 */
    int                index;               /* 0x164 the place */
    char               number[8];           /* 0x168 "%d" of index + 1 */
};
EGCS_SIZE(gt2k_menu::RecordsButton, 0x170);

/* (? name) The seven labels of a line, 0x738 bytes.  Defined after
 * RecordsButton, before RecordsButtonList: egcs emits the inline and
 * implicit members of a unit's classes at its end, the classes in the
 * reverse order of their definitions; per class the members declared inline
 * in reverse declaration order, then the typeinfo function, then the
 * implicit members.  Retail T092 has RecordsButtonList's getLabels, getArg,
 * __tf and destructor (0x00173FD8..0x00174108), then this constructor
 * (0x00174108), then RecordsButton's getIndex, __tf and destructor
 * (0x00174290..0x00174350); the frame info (0x002B15D0) in that order. */
struct RecordsLabels {
    /* RecordsLabels(): implicit (the seven labels' inline constructors,
     * Item::Item(NULL, NULL) each; the landing pads destroy the Items
     * already built); copy 0x00174108 in T092, which RecordsButtonList's
     * constructor calls (egcs does not inline the synthesized constructor;
     * an inline `RecordsLabels() {}' is expanded there). */

    RecordsLabel rank;                      /* 0x000 "record_rank" */
    RecordsLabel rankShadow;                /* 0x108 "record_rank_shadow" */
    RecordsLabel name;                      /* 0x210 "record_name" */
    RecordsLabel data;                      /* 0x318 "record_data" the time / speed */
    RecordsLabel date;                      /* 0x420 "record_date" */
    RecordsLabel car;                       /* 0x528 "record_car" */
    RecordsLabel carPict;                   /* 0x630 "record_car_pict" */
};
EGCS_SIZE(gt2k_menu::RecordsLabels, 0x738);

/* Bases: ButtonList (Drawable at +0x00, List<Button> at +0xB0).  vtables
 * 0x00325720 (13 slots: [0], [5], [11], [12] its own, the rest ButtonList's)
 * and 0x00325708 (the List<Button> part, this-delta -0xB0, 2 slots: [0] the
 * destructor, [1] pList's 0x0023C580).  typeinfo
 * __tfQ29gt2k_menu17RecordsButtonList 0x00173FE8 (__class_type_info, one
 * public base: ButtonList, base list 0x00351CC0; node 0x004B1DA0).  Size
 * 0x86C (operator new in create_menu_records). */
class RecordsButtonList : public ButtonList {
public:
    /* ButtonList(controlType, loop, layout); owner = layout's owner; the
     * list centred in rect; the buttons of items (ButtonList::createButtons,
     * called directly) and the cursor (resetIndex); the labels' images from
     * the layout and their offsets.  index is not used. */
    RecordsButtonList(int controlType, int loop, PageLayout *layout, ButtonData **items,
                      int count, MenuLayoutItem *rect, MenuLayoutItem *rects, int index,
                      RecordsControl *ctl, RecordsMenuArg *arg);  /* 0x001735A0 */
    /* [0] +0x0C 0x00174040 ~RecordsButtonList: implicit (virtual through
     * ButtonList): the labels, ~ButtonList; T092's copy, the one in the
     * vtables. */
    /* A RecordsButton (0x170 bytes) for entry index, appended to this
     * list's children. */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index);     /* [5] +0x34 0x001738E8 */
    /* The player decided on a place (see above); nothing in an empty list
     * or without a focused RecordsButton. */
    virtual void decide();                  /* [11] +0x64 0x00173B40 */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x00173C58 ButtonList's */

    /* The panel page `page' on the focused RecordsButton: ctl->index = its
     * place, its record copied into g_0049C0E0 with link = page, sound 7,
     * decideButton(&g_0049C0E0, 0). */
    void openPanel(const char *page);       /* 0x001739B0 */
    /* Inline (T092's copies, no callers; getArg declared first: its copy
     * comes second). */
    RecordsMenuArg *getArg() { return arg; }            /* 0x00173FE0 */
    RecordsLabels *getLabels() { return &labels; }      /* 0x00173FD8 */

    /*              ButtonList                 0x000 .. 0x128 */
    ControlBase    *owner;                  /* 0x128 the layout's owner (its pad 1: the 0x10000 bit) */
    RecordsControl *ctl;                    /* 0x12C */
    RecordsMenuArg *arg;                    /* 0x130 the records shown */
    RecordsLabels   labels;                 /* 0x134 */
};
EGCS_SIZE(gt2k_menu::RecordsButtonList, 0x86C);

} /* namespace gt2k_menu */

/* The record of the panel being opened (RecordsButtonList::openPanel):
 * a file static of T092 (.bss 0x0049C0E0). */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu17RecordsButtonList(void);    /* 0x00173FE8 */
void *__tfQ29gt2k_menu13RecordsButton(void);        /* 0x00174298 */
/* Their __class_type_info storage (4 words, .bss), filled by them.  (The
 * base lists, T092 .sdata 0x00351CC0 / 0x00351CC8, need T548's
 * __class_type_info::base_info: declared by the unit.) */
extern u32 __tiQ29gt2k_menu17RecordsButtonList[4];  /* 0x004B1DA0 */
extern u32 __tiQ29gt2k_menu13RecordsButton[4];      /* 0x004B1D90 */
}
#endif

#endif /* GAME_T092_RECORDS_BUTTON_LIST_H */
