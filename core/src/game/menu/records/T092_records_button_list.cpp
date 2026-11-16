/* T092 - gt2k_menu::RecordsButtonList and gt2k_menu::RecordsButton: the list
 * of places of the records screen (game/menu/records/T092_records_button_list.h).
 * 0x001735A0..0x00174350.
 *
 * The list owns seven labels (RecordsLabels: rank, its shadow, name, time,
 * date, car, car picture), placed like the layout's "record_*" items
 * relative to the layout's first item; every RecordsButton draws its place
 * by moving the labels onto itself and drawing them with its own texts.
 * Deciding on a place opens a panel page ("can_not_delete", "select_panel_code"
 * while R1 is held, else "select_panel_normal") on a copy of the button's
 * record. */
#include "libc.h"                           /* strcpy */
/* Retail's .rodata (0x00325640..0x00325AF8): the literals, the vtables, then
 * the type names in the reverse order of the classes' definitions:
 * RecordsButtonList, RecordsButton, RecordsControl, RecordsMenuArg,
 * SmartIndexT<int>, List / Cell<Button>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, the GameStatus's garage lists,
 * Operator. */
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#include "game/save/T429_game_status.h"          /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/framework/T206_button.h"               /* base gt2k_menu::Button (Cell<Button>) */
/* ButtonList: its List<Button> base, then its SmartIndexT<int> index member
 * (retail 0x003258C8 / 0x003258E8). */
#include "game/menu/framework/T244_button_list.h"          /* base gt2k_menu::ButtonList (List<Button>, SmartIndexT<int>) */
#include "game/menu/records/T089_records_menu_args.h"    /* RecordsMenuArg */
#include "game/menu/records/T090_records_control.h"      /* RecordsControl */
#include "game/menu/records/T092_records_button_list.h"
#include "game/menu/framework/T213_menu_item.h"            /* Item (setResource, setText, virtual_2) */
#include "game/menu/framework/T214_menu_control.h"         /* MenuControl, MENU_BUTTON_R1 */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (firstPageItem), MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T247_control_base.h"         /* ControlBase::getControl1 */
#include "engine/input/T474_control.h"              /* Control::getButtons */
#include "game/common/T494_text_utils.h"           /* format_date */
#include "engine/pdi/T519_pdicdvd.h"              /* pdicdvd_time_to_clock_local */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/framework/menu_list.h"                 /* ListIterator<Button> */
#ifndef BOOT_OLD_GCC                        /* egcs generates the __tf functions itself */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_class, __class_type_info::base_info */
/* .sdata: the base lists of the two __class_type_infos, {base, public,
 * offset 0}: RecordsButtonList's base ButtonList (__tiQ29gt2k_menu10ButtonList
 * 0x004B2650), RecordsButton's base Button (__tiQ29gt2k_menu6Button
 * 0x004B2450); flag word 0x40000000. */
extern const __class_type_info::base_info g_00351CC0[1];   /* 0x00351CC0 gp-0x5D30 */
extern const __class_type_info::base_info g_00351CC8[1];   /* 0x00351CC8 gp-0x5D28 */
#endif

/* The record of the panel being opened (openPanel): .bss, a file static. */
static gt2k_menu::MenuItemRecord g_0049C0E0;    /* 0x0049C0E0 */

namespace gt2k_menu {

/* ButtonList(controlType, loop, layout), the two vptrs, owner = the
 * layout's owner (the RecordsControl), control, records, the labels; the
 * list centred in rect; the buttons of the entries (ButtonList::
 * createButtons, called directly) and the cursor (resetIndex); the labels'
 * pictures from the layout and their offsets from the centre of the
 * layout's first item.  `index' is not used.  The landing pads destroy the
 * labels (Item::~Item, flags 2, last first) and the ButtonList (flags 0).
 * Caller: create_menu_records. */
// FUNC 0x001735A0 gt2k_menu::RecordsButtonList::RecordsButtonList
RecordsButtonList::RecordsButtonList(int controlType, int loop, PageLayout *layout,
                                     ButtonData **items, int count, MenuLayoutItem *rect,
                                     MenuLayoutItem *rects, int index, RecordsControl *control,
                                     RecordsMenuArg *records)
    : ButtonList(controlType, loop, layout),
      owner(static_cast<ControlBase *>(layout->owner)), ctl(control), arg(records),
      labels()                              /* RecordsLabels::RecordsLabels (out of line) */
{
    f32 height = rect->h;
    f32 width = rect->w;
    f32 centreX = rect->x + width * 0.5f;    /* both centres before the stores: */
    f32 centreY = 448.0f - (rect->y + height * 0.5f);   /* retail loads h, w, y, x */
    /* implicit: RecordsLabels (labels' constructor), ~Item (the labels' Items
     * destroyed if the body throws) */
    x.set(centreX);
    y.set(centreY);
    w = width;
    h = height;
    ButtonList::createButtons(items, count, rects);
    resetIndex();

    MenuLayoutItem *firstItem = layout->firstPageItem();
    f32 originX = firstItem->x + firstItem->w * 0.5f;
    f32 originY = 448.0f - (firstItem->y + firstItem->h * 0.5f);
    labels.rank.item.setResource("record_rank", layout);
    labels.rankShadow.item.setResource("record_rank_shadow", layout);
    labels.name.item.setResource("record_name", layout);
    labels.data.item.setResource("record_data", layout);
    labels.date.item.setResource("record_date", layout);
    labels.car.item.setResource("record_car", layout);
    labels.carPict.item.setResource("record_car_pict", layout);
    labels.rank.setOrigin(originX, originY);
    labels.rankShadow.setOrigin(originX, originY);
    labels.name.setOrigin(originX, originY);
    labels.data.setOrigin(originX, originY);
    labels.date.setOrigin(originX, originY);
    labels.car.setOrigin(originX, originY);
    labels.carPict.setOrigin(originX, originY);

    /* An inlined walk over the buttons whose body was empty (or optimised
     * away): it only steps through the cells (the iterator at sp+0). */
    ListIterator<Button> buttons(this);
    while (buttons.next() != NULL) {
    }
}

/* index.txt: virtual_5.  A RecordsButton (0x170 bytes) for entry `index',
 * appended to this list's children.  The landing pad frees the block
 * (Object::operator delete). */
// FUNC 0x001738E8 gt2k_menu::RecordsButtonList::createButton
Button *RecordsButtonList::createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                        int index)
{
    /* Object::operator new (0x170); the landing pad: Object::operator delete */
    /* implicit: operator new, operator delete (Object's: the new expression
     * and its landing pad) */
    RecordsButton *button = new RecordsButton(this, controlType, layout, offPoint, data, elem,
                                              index);
    button->attachTo(this);
    return button;
}

/* Opens the panel page `page' on the focused RecordsButton: the control
 * remembers its place, the button's record is copied into g_0049C0E0 with
 * `page' as its link, the panel sound, then decideButton on the copy (a2 0:
 * the list played its own sound).  Nothing when the focused button is not a
 * RecordsButton.  Caller: decide. */
// FUNC 0x001739B0 gt2k_menu::RecordsButtonList::openPanel
void RecordsButtonList::openPanel(const char *page)
{
    RecordsButton *button = dynamic_cast<RecordsButton *>(focusButton());

    if (button != NULL) {
        int place = button->index;          /* read before ctl (retail's order) */
        ctl->index = place;
        g_0049C0E0 = *button->linkMenu;     /* 0x14C-byte struct copy */
        strcpy(g_0049C0E0.link, page);
        menu_play_se(MENU_SE_CONFIRM);
        decideButton(&g_0049C0E0, 0);
    }
}

/* The buttons of mask held on a pad (? an inline of the original, as
 * u005's T027): through the inline call the mask is a pseudo set before the
 * virtual getButtons call (retail keeps 0x10000, R1, in $s0 across it), and
 * through ControlBase::getControl1() the call is virtual (lw 0xC0, the
 * vptr of the owner's first MenuControl). */
static inline u32 heldButtons(Control *pad, u32 mask)
{
    return pad->getButtons() & mask;
}

/* index.txt: virtual_11.  The player decided on a place: "can_not_delete"
 * when the records refuse to delete it (asked through the control's
 * records), else "select_panel_code" while R1 is held on the owner's first
 * pad (Control::getButtons, slot 4), else "select_panel_normal".  Nothing in
 * an empty list or without a focused RecordsButton. */
// FUNC 0x00173B40 gt2k_menu::RecordsButtonList::decide
void RecordsButtonList::decide()
{
    if (count() != 0) {
        RecordsButton *button = dynamic_cast<RecordsButton *>(focusButton());

        if (button != NULL) {
            if (ctl->arg->deleteEnable(button->index) == 0)
                openPanel("can_not_delete");
            else if (heldButtons(owner->getControl1(), MENU_BUTTON_R1))
                openPanel("select_panel_code");
            else
                openPanel("select_panel_normal");
        }
    }
}

/* index.txt: virtual_12.  ButtonList's (called directly). */
// FUNC 0x00173C58 gt2k_menu::RecordsButtonList::doControl
void RecordsButtonList::doControl(const MenuControl &control)
{
    ButtonList::doControl(control);
}

/* Button(list, controlType, layout, offPoint, linkMenu, elem, relative 1,
 * depth index), list, index, the two vptrs, number = "%d" of index + 1.
 * The landing pad destroys the Button (flags 0).  Caller: createButton. */
// FUNC 0x00173C78 gt2k_menu::RecordsButton::RecordsButton
RecordsButton::RecordsButton(RecordsButtonList *buttonList, int controlType,
                             PageLayout *layout, f32 offPoint, ButtonData *linkMenu,
                             MenuLayoutItem *elem, int place)
    : Button(buttonList, controlType, layout, offPoint, linkMenu, elem, 1, place),
      list(buttonList), index(place)
{
    sprintf(number, "%d", place + 1);
}

/* index.txt: virtual_2 (draw).  Button's draw frame (beginDraw, isClipped,
 * drawItem ... drawFocus, endDraw); in between the list's labels are moved
 * onto this button and drawn with this place's texts: the number (shadow
 * first), the name, the time, the date ("\x7F\x7F/\x7F\x7F/\x7F\x7F" when
 * there is none; the packed date through a CdClock in local time), and when
 * the records show cars the car name and an empty text for the car
 * picture. */
// FUNC 0x00173D20 gt2k_menu::RecordsButton::virtual_2
void RecordsButton::virtual_2()
{
    char timeText[0x40];                    /* sp+0x00 */
    char dateText[0x40];                    /* sp+0x40 */
    char carText[0x80];                     /* sp+0x80 */
    CdClock clock;                          /* sp+0x100 */

    beginDraw();
    if (isClipped() == 0) {
        drawItem();
        RecordsButtonList *buttonList = list;
        f32 posX = x.value;
        f32 posY = y.value;
        RecordsMenuArg *records = buttonList->arg;
        RecordsLabels *labels = &buttonList->labels;   /* retail: $s5, the date / car
                                                          labels after the branch from it */

        labels->rankShadow.draw(posX, posY, number);
        labels->rank.draw(posX, posY, number);
        labels->name.draw(posX, posY, records->getName(index));
        labels->data.draw(posX, posY, records->getTime(index, timeText));
        u32 date = records->getDate(index);
        if (date == 0) {
            strcpy(dateText, "\177\177/\177\177/\177\177");   /* 9-byte copy from 0x003256F8 */
        } else {
            pdicdvd_time_to_clock_local(&clock, (u64)date);
            format_date(dateText,
                        (clock.day >> 4) * 10 + (clock.day & 0xF),
                        (clock.month >> 4) * 10 + (clock.month & 0xF),
                        (clock.year >> 4) * 10 + (clock.year & 0xF),
                        0);
        }
        labels->date.draw(posX, posY, dateText);
        if (records->isDrawCarName() != 0) {
            labels->car.draw(posX, posY, records->getCarName(index, carText));
            labels->carPict.draw(posX, posY, "");    /* "" at 0x00351CB8 */
        }
        drawFocus();
    }
    endDraw();
}

/* Moves the label to (offX + x, offY + y), sets its text and draws it
 * (Item::virtual_2, called directly).  Caller: RecordsButton::virtual_2. */
// FUNC 0x00173F58 gt2k_menu::RecordsLabel::draw
void RecordsLabel::draw(f32 x, f32 y, const char *text)
{
    item.x.set(offX + x);
    item.y.set(offY + y);
    item.setText(text);
    item.Item::virtual_2();
}

/* The label's offset from (x, y).  Caller: the RecordsButtonList constructor. */
// FUNC 0x00173FB8 gt2k_menu::RecordsLabel::setOrigin
void RecordsLabel::setOrigin(f32 x, f32 y)
{
    offX = item.x.value - x;
    offY = item.y.value - y;
}

// FUNC 0x00173FD8 gt2k_menu::RecordsButtonList::getLabels
/* inline in game/menu/records/T092_records_button_list.h; egcs emits this copy (no
 * callers): &labels */

// FUNC 0x00173FE0 gt2k_menu::RecordsButtonList::getArg
/* inline in game/menu/records/T092_records_button_list.h; egcs emits this copy (no
 * callers): arg */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x00173FE8 __tfQ29gt2k_menu17RecordsButtonList
extern "C" void *__tfQ29gt2k_menu17RecordsButtonList(void)
{
    if (__tiQ29gt2k_menu17RecordsButtonList[0] == 0) {
        __tfQ29gt2k_menu10ButtonList();
        __rtti_class(__tiQ29gt2k_menu17RecordsButtonList, "Q29gt2k_menu17RecordsButtonList",
                     g_00351CC0, 1);
    }
    return __tiQ29gt2k_menu17RecordsButtonList;
}
#endif

namespace gt2k_menu {

/* index.txt: RecordsButtonList::~RecordsButtonList (slot 0 of both
 * vtables). */
// FUNC 0x00174040 gt2k_menu::RecordsButtonList::~RecordsButtonList
/* implicit in game/menu/records/T092_records_button_list.h; egcs emits this copy: the two
 * vptrs, the labels' inline destructor (the seven Items, last first,
 * Item::~Item flags 2), ButtonList::~ButtonList (flags 0), then
 * Object::operator delete (0x001BF458) when flags & 1 */
/* implicit: ~Item, ~ButtonList, operator delete (the base / member
 * destructors, flags & 1) */

/* Caller: the RecordsButtonList constructor. */
// FUNC 0x00174108 gt2k_menu::RecordsLabels::RecordsLabels
/* inline in game/menu/records/T092_records_button_list.h; egcs emits this copy: the seven
 * labels' inline constructors, Item::Item(NULL, NULL) (0x001B98C0) each; the
 * landing pads destroy the Items already built (Item::~Item, flags 2) */
/* implicit: Item, ~Item (the seven labels' inline constructors and their
 * landing pads) */

// FUNC 0x00174290 gt2k_menu::RecordsButton::getIndex
/* inline in game/menu/records/T092_records_button_list.h; egcs emits this copy (no
 * callers): index */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__class_type_info, one public base). */
// FUNC 0x00174298 __tfQ29gt2k_menu13RecordsButton
extern "C" void *__tfQ29gt2k_menu13RecordsButton(void)
{
    if (__tiQ29gt2k_menu13RecordsButton[0] == 0) {
        __tfQ29gt2k_menu6Button();
        __rtti_class(__tiQ29gt2k_menu13RecordsButton, "Q29gt2k_menu13RecordsButton",
                     g_00351CC8, 1);
    }
    return __tiQ29gt2k_menu13RecordsButton;
}
#endif

/* index.txt: RecordsButton::~RecordsButton (slot 0 of both vtables). */
// FUNC 0x001742F0 gt2k_menu::RecordsButton::~RecordsButton
/* implicit in game/menu/records/T092_records_button_list.h; egcs emits this copy: the two
 * vptrs, Button::~Button (flags 0), then Object::operator delete (0x001BF458)
 * when flags & 1 */
/* implicit: ~Button, operator delete (the base / member destructors, flags
 * & 1) */
