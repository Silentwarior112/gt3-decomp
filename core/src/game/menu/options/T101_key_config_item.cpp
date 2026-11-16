/* T101 - gt2k_menu::KeyConfigItem.  0x0012AB90..0x0012C300.
 *
 * The button assignment page of one controller, built by the T064 hooks
 * (option_create_key_config_1 ..., Object::operator new 0x1180), e.g. for
 * player 1's pad KeyConfigItem(owner, item, tex, &owner->control1,
 * &owner->pages, layout, 1, 0, 1, 0).  Each row offers the buttons of
 * g_0029BE98[mode][row]; three of them are drawn around the choice,
 * scrolling sideways inside the row label (menu_clip_column).  Choices are
 * written straight into the owner's ControllerConfig; the page can only be
 * left (checkExit) while no two rows use the same button or overlapping
 * stick inputs (hasConflict).
 * Button ids (picture names g_0029BD80): 0 cross_lr, 1 / 2 the left /
 * right stick's x axis, 3..10 the stick directions (ana_l_u, _d, _l, _r,
 * ana_r_u, ...), 11 maru, batu, sankaku, sikaku, r1, r2, l1, l2, 19 start,
 * 20 usb_steer, 0x15..0x1A usb_l1..usb_r3, usb_accel, usb_brake, usb_nil,
 * 30..33 the wheel's sankaku, batu, sikaku, maru, 0x22 default_item,
 * 0x23..0x28 jaguar_l1..jaguar_r3. */
/* Retail T101's data: .data 0x0029BD80 the tables below, then the frame
 * info; .rodata 0x00314120 the tables' long strings, the vtables
 * (KeyConfigItem, SmartIndexT<int>), the type names KeyConfigItem,
 * OperatorList .. Cell<OperatorHolder>, the garage's, SmartIndexT<int>,
 * Operator, then the static const tables (0x003145B0 .. 0x003147C8); .sdata
 * 0x00351300 the tables' short strings, "bad" .. "arrow_r", the three small
 * static const tables.  The type names in reverse definition order:
 * SmartIndexT<int> (T251) first, the GameStatus (the garage), then the menu
 * classes (T213, through T101's header); no prize counter (option_items_internal.h used to
 * bring it: "%s Cr. %s", "Cr. %s"). */
#include "game/common/T251_smart_index.h"          /* (SmartIndexT<int>'s type name, first) */
#include "game/save/T429_game_status.h"          /* (the garage's type names) */
#include "game/menu/options/T101_key_config_item.h"
#include "game/menu/options/T062_option_control.h"       /* gt2k_menu::OptionControl (owner) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (noCancel, exitRequest) */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (pad) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (findHiddenItem) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T259_menu_drawable.h"        /* setRectVertices */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_column, menu_clip_full */
#include "game/save/T426_controller_config.h"    /* ControllerConfig */
#include "game/save/T428_game_options.h"         /* GameOptions (config) */
#include "engine/input/T474_control.h"              /* Control (rawUSBType, rawPSType) */
#include "engine/pgl/G001_flare.h"                /* gfx_enable, gfx_blend_func, gfx_alloc_tail, gfx_vsync,
                                               pgl_color4f, pgl_draw_strip */
#ifndef BOOT_OLD_GCC
#include "game/common/T017_smart_index.h"          /* __tiQ29gt2k_menut11SmartIndexT1Zi (SmartIndexT<int>'s node) */
#endif
#include "game/menu/options/option_items_internal.h"

/* ------------------------------------------------------------------------ */
/* The tables                                                               */
/* ------------------------------------------------------------------------ */

/* Nothing in the code references the tables' strings (only the tables do):
 * the file's .rodata and .sdata start with them. */
// DATA_START 0x00314120
// DATA_START 0x00351300

/* A mode's rows (g_0029BE98) and the row functions: static const, so egcs
 * writes them at the end of the unit (after the type names), those already
 * referenced first (the rows, from g_0029BE98, then the row functions, from
 * the constructor), then the button lists the rows point at, each in
 * definition order; the lists of 8 bytes or less go to .sdata. */
static const int g_00351408[1] = { 0 };     /* 0x00351408 digital, steering: cross_lr */
static const int g_00314708[8] = {          /* 0x00314708 digital: maru .. l2 */
    11, 12, 13, 14, 15, 16, 17, 18
};
static const int g_00351410[2] = { KEY_ANA_L_LR, KEY_ANA_R_LR };   /* 0x00351410 analog, steering */
static const int g_00314728[16] = {         /* 0x00314728 analog: maru .. l2, the stick directions */
    11, 12, 13, 14, 15, 16, 17, 18, 3, 4, 5, 6, 7, 8, 9, 10
};
static const int g_00314768[8] = {          /* 0x00314768 analog, rear view / change view: maru .. l2 */
    11, 12, 13, 14, 15, 16, 17, 18
};
static const int g_00314788[6] = {          /* 0x00314788 GT Force: usb_l1, usb_r1, usb_l2, usb_r2, usb_r3, usb_nil */
    KEY_USB_L1, KEY_USB_R1, KEY_USB_L2, KEY_USB_R2, KEY_USB_R3, 0x1D
};
static const int g_003147A0[10] = {         /* 0x003147A0 type 3 wheel: its sankaku, batu, sikaku, maru,
                                               usb_l1, usb_r1, usb_l2, usb_r2, usb_l3, usb_r3 */
    0x1E, 0x1F, 0x20, 0x21, KEY_USB_L1, KEY_USB_R1, KEY_USB_L2, KEY_USB_R2, KEY_USB_L3, KEY_USB_R3
};
static const int g_00351418[1] = { KEY_DEFAULT };  /* 0x00351418 the default row: default_item */

static const gt2k_menu::KeyRow g_003145B0[10] = {  /* 0x003145B0 mode 0, digital pad */
    { g_00351408, 1 },
    { g_00314708, 8 }, { g_00314708, 8 }, { g_00314708, 8 }, { g_00314708, 8 },
    { g_00314708, 8 }, { g_00314708, 8 }, { g_00314708, 8 }, { g_00314708, 8 },
    { g_00351418, 1 }
};
static const gt2k_menu::KeyRow g_00314600[10] = {  /* 0x00314600 mode 1, analog pad */
    { g_00351410, 2 },
    { g_00314728, 16 }, { g_00314728, 16 }, { g_00314728, 16 },
    { g_00314728, 16 }, { g_00314728, 16 }, { g_00314728, 16 },
    { g_00314768, 8 }, { g_00314768, 8 },
    { g_00351418, 1 }
};
static const gt2k_menu::KeyRow g_00314650[7] = {   /* 0x00314650 mode 2, GT Force */
    { g_00314788, 6 }, { g_00314788, 6 }, { g_00314788, 6 },
    { g_00314788, 6 }, { g_00314788, 6 }, { g_00314788, 6 },
    { g_00351418, 1 }
};
static const gt2k_menu::KeyRow g_00314688[7] = {   /* 0x00314688 modes 3 and 4, type 3 wheel */
    { g_003147A0, 10 }, { g_003147A0, 10 }, { g_003147A0, 10 },
    { g_003147A0, 10 }, { g_003147A0, 10 }, { g_003147A0, 10 },
    { g_00351418, 1 }
};
/* The row functions (ControllerConfig's func; KEY_FUNC_DEFAULT the
 * "default" row). */
static const int g_003146C0[10] = {         /* 0x003146C0 pad page: steering .. change view, default */
    0, 1, 2, 3, 4, 5, 6, 7, 8, KEY_FUNC_DEFAULT
};
static const int g_003146E8[7] = {          /* 0x003146E8 USB page: back .. change view, default */
    3, 4, 5, 6, 7, 8, KEY_FUNC_DEFAULT
};

/* .data, in address order; their strings are the file's first .rodata
 * (9 bytes or more) and .sdata items, in first-use order ("sankaku",
 * "batu", "sikaku", "maru" and "default_1" once). */
const char *g_0029BD80[KEY_BUTTON_COUNT] = {    /* 0x0029BD80 */
    "cross_lr", "ana_l_lr", "ana_r_lr",
    "ana_l_u", "ana_l_d", "ana_l_l", "ana_l_r", "ana_r_u", "ana_r_d", "ana_r_l", "ana_r_r",
    "maru", "batu", "sankaku", "sikaku", "r1", "r2", "l1", "l2", "start",
    "usb_steer", "usb_l1", "usb_l2", "usb_l3", "usb_r1", "usb_r2", "usb_r3",
    "usb_accel", "usb_brake", "usb_nil",
    "sankaku", "batu", "sikaku", "maru",
    "default_item",
    "jaguar_l1", "jaguar_l2", "jaguar_l3", "jaguar_r1", "jaguar_r2", "jaguar_r3"
};
const char *g_0029BE28[10] = {              /* 0x0029BE28 */
    "steering_1", "acceleration_1", "brake_1", "back_1", "side_brake_1", "shift_up_1",
    "shift_down_1", "rear_view_1", "change_view_1", "default_1"
};
const char *g_0029BE50[10] = {              /* 0x0029BE50 */
    "steering_2", "acceleration_2", "brake_2", "back_2", "side_brake_2", "shift_up_2",
    "shift_down_2", "rear_view_2", "change_view_2", "default_2"
};
const char *g_0029BE78[7] = {               /* 0x0029BE78 */
    "back", "side_brake", "shift_up", "shift_down", "rear_view", "change_view", "default_1"
};
const gt2k_menu::KeyRow *g_0029BE98[5] = {  /* 0x0029BE98 */
    g_003145B0, g_00314600, g_00314650, g_00314688, g_00314688
};

namespace gt2k_menu {

/* The members in order: labels[10] (Item(NULL, NULL), a loop), cursor and
 * select[10] (SmartIndexT<int>'s inline constructor, the vtable 0x0030D998),
 * scroll[10] (MotionValue(), a loop), blink, arrowL, arrowR, bad, warning
 * (Item(NULL, NULL)), then wait and editable.  The landing pads destroy
 * the members built so far (labels through their virtual destructor).
 * The isUsb() choices test !isUsb() (retail's movz / fall-through to the
 * pad values); one loop variable for the buttons and the labels (retail's
 * $s3 in both).  Exact since SmartIndexT's `wrapped' is a bool (T251,
 * u069): with an int it was 99.7%, sched2 putting value's `sw $zero' before
 * the vtable's `addiu %lo' (retail after). */
// FUNC 0x0012AB90 gt2k_menu::KeyConfigItem::KeyConfigItem
KeyConfigItem::KeyConfigItem(OptionControl *owner, MenuLayoutItem *res, MenuTexture *tex,
                             MenuControl *pad, PageManager *pages, PageLayout *layout, int mode,
                             int port, int editable, int usb)
    : Item(res, tex), port(port), owner(owner), mode(mode), usb(usb), pad(pad), pages(pages),
      blink(0.0f), wait(3), editable(editable)
{
    rows = !isUsb() ? 10 : 7;
    rowFunc = !isUsb() ? g_003146C0 : g_003146E8;
    defaultRow = !isUsb() ? 9 : 6;
    cursor.set(0);
    cursor.setRange(0, rows);
    cursor.setClamp(0);                     /* the row cursor wraps */
    int i;
    for (i = 0; i < KEY_BUTTON_COUNT; i++) {
        if (editable != 0 || i != KEY_DEFAULT) {   /* "default_item" only when editable */
            MenuLayoutItem *picture = layout->findHiddenItem(g_0029BD80[i]);
            if (picture != NULL)
                /* implicit: operator new, operator delete (new-expression) */
                buttons[i] = new Item(picture, layout);    /* Object::operator new(0x100) */
            else
                buttons[i] = NULL;
        } else {
            buttons[KEY_DEFAULT] = NULL;
        }
    }
    updateMode();
    for (i = 0; i < rows; i++) {
        const char *name;
        if (isUsb())
            name = g_0029BE78[i];
        else if (this->port == 0)
            name = g_0029BE28[i];
        else
            name = g_0029BE50[i];
        if (editable != 0 || i != defaultRow)
            labels[i].setResource(name, layout);
    }
    if (this->editable != 0) {
        bad.setResource("bad", layout);
        warning.setResource("warning", layout);
        arrowL.setResource("arrow_l", layout);
        arrowR.setResource("arrow_r", layout);
        hideDelay = 0;
        this->pages->noCancel = 1;
    } else {
        hideDelay = 1;
    }
    resetSelection();
    if (isUsb() || this->port == 0)
        this->owner->attachKeyConfig1(this);    /* 0x0013FC98 */
    else if (this->port == 1)
        this->owner->attachKeyConfig2(this);    /* 0x0013FCD0 */
    gfx_vsync(1);
}

// FUNC 0x0012B120 gt2k_menu::KeyConfigItem::~KeyConfigItem
KeyConfigItem::~KeyConfigItem()
{
    /* the vptr is stored first */
    for (int id = 0; id < KEY_BUTTON_COUNT; id++) {
        if (buttons[id] != NULL)
            delete buttons[id];             /* virtual, flags 3 */
    }
    if (isUsb() || port == 0)
        owner->detachKeyConfig1();          /* 0x0013FCB0 */
    else if (port == 1)
        owner->detachKeyConfig2();          /* 0x0013FCE8 */
    pages->noCancel = 0;
    /* then the members: warning, bad, arrowR, arrowL (~Item, flags 2),
     * labels[9..0] (their virtual destructor, flags 0); Item::~Item(this,
     * 0); Object::operator delete if flags & 1 */
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x0012B2A8 gt2k_menu::KeyConfigItem::isUsb
int KeyConfigItem::isUsb()
{
    return usb;
}

/* The function of a row (KEY_FUNC_DEFAULT for the "default" row). */
// FUNC 0x0012B2B0 gt2k_menu::KeyConfigItem::rowFunction
int KeyConfigItem::rowFunction(int row)
{
    isUsb();                                /* called, result unused (? a leftover choice of
                                               the table) */
    return rowFunc[row];
}

/* Every row's choices from the mode's table, the choice from the owner's
 * current assignment (none found: unchanged; the default row: 0); the
 * scroll positions jump there. */
// FUNC 0x0012B2F0 gt2k_menu::KeyConfigItem::resetSelection
void KeyConfigItem::resetSelection()
{
    if (mode == KEYCFG_NONE)
        return;
    for (int row = 0; row < rows; row++) {
        select[row].setRange(0, g_0029BE98[mode][row].count);
        select[row].setClamp(1);            /* the choices stop at the ends */
        if (row < defaultRow) {
            ControllerConfig *config = &owner->options.config;
            int choice;
            for (choice = 0; choice < g_0029BE98[mode][row].count; choice++) {
                if (g_0029BE98[mode][row].options[choice] ==
                    config->getKey(mode, port, rowFunction(row)))
                    break;
            }
            if (choice < g_0029BE98[mode][row].count)
                select[row].set(choice);
        } else {
            select[row].set(0);
        }
        SmartIndexT<int> *choice = &select[row];
        selectPos[row] = (f32)choice->value;
        scroll[row].set(selectPos[row]);
    }
}

/* The button id chosen on a row (0 without a controller). */
// FUNC 0x0012B528 gt2k_menu::KeyConfigItem::selectedOption
int KeyConfigItem::selectedOption(int row)
{
    if (mode == KEYCFG_NONE)
        return 0;
    SmartIndexT<int> *choice = &select[row];
    return g_0029BE98[mode][row].options[choice->value];
}

/* The mode from the controller now connected: the USB page reads the USB
 * type of the owner's control of the port, the pad page the PS type of
 * its own pad.  Nothing usable: mode 5, after resetting the old mode's
 * assignment to the defaults if it was left conflicting.  A new mode
 * reloads the rows.  Control::rawUSBType / rawPSType are inline (T474). */
// FUNC 0x0012B580 gt2k_menu::KeyConfigItem::updateMode
void KeyConfigItem::updateMode()
{
    MenuControl *control;
    if (port != 1)
        control = &owner->control1;
    else
        control = &owner->control2;
    int oldMode = mode;
    if (isUsb()) {
        switch (control->rawUSBType()) {
        case USB_WHEEL_GTFORCE:
            mode = CONFIG_WHEEL_GTFORCE;
            break;
        case USB_WHEEL_C294:
            mode = CONFIG_WHEEL_C294;
            break;
        default:
            if (mode != KEYCFG_NONE) {
                if (anyConflict())
                    owner->options.config.resetKeys(mode, port);
            }
            mode = KEYCFG_NONE;
            break;
        }
    } else {
        switch (pad->rawPSType()) {
        case PS_PAD_DIGITAL:
            mode = CONFIG_DIGITAL;
            break;
        case PS_PAD_DUALSHOCK:
        case PS_PAD_DUALSHOCK2:
            mode = CONFIG_ANALOG;
            break;
        default:                            /* the same as the USB page's (cross-jumped) */
            if (mode != KEYCFG_NONE) {
                if (anyConflict())
                    owner->options.config.resetKeys(mode, port);
            }
            mode = KEYCFG_NONE;
            break;
        }
    }
    if (oldMode != mode)
        resetSelection();
}

/* update: Item's, the blink (0.5 -> 0, again), each row's scroll eases to
 * its choice (rate 0.3). */
// FUNC 0x0012B6A8 gt2k_menu::KeyConfigItem::virtual_1
void KeyConfigItem::virtual_1()
{
    Item::virtual_1();
    blink = blink - 0.016666668f;           /* 1 / 60 */
    if (blink < 0.0f)
        blink = 0.5f;
    for (int row = 0; row < rows; row++) {
        SmartIndexT<int> *choice = &select[row];
        selectPos[row] = (f32)choice->value;
        scroll[row].approach(selectPos[row], 0.3f);
    }
}

/* 1 if another assignable row uses the same button as `row', or one of
 * them uses a stick's x axis (1 / 2) and the other a horizontal direction
 * of the same stick (5, 6 / 9, 10). */
// FUNC 0x0012B768 gt2k_menu::KeyConfigItem::hasConflict
int KeyConfigItem::hasConflict(int row)
{
    int button = selectedOption(row);
    /* the tests on button alone are written in the loop: loop.c hoists them
     * into the preheader (after the rows test), as retail has them */
    for (int other = 0; other < rows; other++) {
        if (rowFunction(other) == KEY_FUNC_DEFAULT)
            continue;
        if (other == row)
            continue;
        int otherButton = selectedOption(other);
        if (button == otherButton)
            return 1;
        if (button == KEY_ANA_L_LR && (u32)(otherButton - KEY_ANA_L_L) < 2)
            return 1;
        if (button == KEY_ANA_R_LR && (u32)(otherButton - KEY_ANA_R_L) < 2)
            return 1;
        if ((u32)(button - KEY_ANA_L_L) < 2 && otherButton == KEY_ANA_L_LR)    /* 5 or 6 */
            return 1;
        if ((u32)(button - KEY_ANA_R_L) < 2 && otherButton == KEY_ANA_R_LR)    /* 9 or 10 */
            return 1;
    }
    return 0;
}

// FUNC 0x0012B898 gt2k_menu::KeyConfigItem::anyConflict
int KeyConfigItem::anyConflict()
{
    for (int row = 0; row < rows; row++) {
        if (rowFunction(row) == KEY_FUNC_DEFAULT)
            continue;
        if (hasConflict(row))
            return 1;
    }
    return 0;
}

/* (? names) drawRow reads the row's SmartIndexT through these: retail has
 * a fresh copy of &select[row] (index + this) per read, and two separate
 * loop invariants for the two value reads of the choice loop. */
static inline int indexValue(SmartIndexT<int> *index)
{
    return index->value;
}
static inline int indexBegin(SmartIndexT<int> *index)
{
    return index->begin;
}
static inline int indexEnd(SmartIndexT<int> *index)
{
    return index->end;
}

/* One row: its label; when editable the cursor quad (current row, additive,
 * grey level blink) and "bad" over a conflict; the chosen button and its
 * neighbours clipped to the label; the arrows while there is more on
 * either side.  The label is read through its inline accessors (x.get(),
 * getW(): each a copy of &labels[row]), the arrows' alphas through
 * getColor(i) (four copies of blink, as retail).
 * Not exact (87%): the choice loop's six invariant addresses (&scroll[row],
 * &select[row] twice, &labels[row], its x and y) come out of loop.c in
 * another order, combine makes a madd of row * 12 + this, and two of them
 * are spilled where retail spills the label's x / y; tried: the statement
 * order of the loop body, scroll[row].value, split conditions, the
 * reference compiler 991111-01 (the same). */
// FUNC 0x0012B918 gt2k_menu::KeyConfigItem::drawRow
void KeyConfigItem::drawRow(int row)
{
    if (mode == KEYCFG_NONE)
        return;
    labels[row].virtual_2();                /* virtual call (slot 2) */
    if (editable != 0) {
        if (cursor.value == row) {
            gfx_enable(PGL_CAP_BLEND);
            gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
            PrimVertex *quad = (PrimVertex *)gfx_alloc_tail(3);    /* 4 vertices */
            setRectVertices(quad, labels[row].x.get(), labels[row].y.get(), labels[row].getW(),
                            labels[row].getH());
            pgl_color4f(blink, blink, blink, 1.0f);
            pgl_draw_strip(quad, 0, 0, 0, 4);
        }
        if (editable != 0 && hasConflict(row)) {
            bad.setX(labels[row].x.get());
            bad.setY(labels[row].y.get());
            bad.virtual_2();
        }
    }
    f32 anchorY = 0.0f;
    f32 anchorX = 0.0f;
    if (anchor != NULL) {
        anchorX = anchor->x.value;
        anchorY = anchor->y.value;
    }
    menu_clip_column(labels[row].x.get() + anchorX, labels[row].getW());
    for (int choice = 0; choice < g_0029BE98[mode][row].count; choice++) {
        if (choice < indexValue(&select[row]) - 1 || indexValue(&select[row]) + 1 < choice)
            continue;
        int button = g_0029BE98[mode][row].options[choice];
        f32 bx = labels[row].x.get() + anchorX + labels[row].getW() * ((f32)choice - scroll[row].get());
        f32 by = labels[row].y.get() + anchorY;
        if (isUsb() && mode == CONFIG_WHEEL_C294) { /* the type 3 wheel's own pictures */
            if (button == KEY_USB_L1)
                button = KEY_JAGUAR_L1;
            else if (button == KEY_USB_L2)
                button = KEY_JAGUAR_L2;
            else if (button == KEY_USB_L3)
                button = KEY_JAGUAR_L3;
            else if (button == KEY_USB_R1)
                button = KEY_JAGUAR_R1;
            else if (button == KEY_USB_R2)
                button = KEY_JAGUAR_R2;
            else if (button == KEY_USB_R3)
                button = KEY_JAGUAR_R3;
        }
        Item *picture = buttons[button];
        if (picture != NULL) {
            picture->setX(bx);
            picture->setY(by);
            picture->virtual_2();           /* virtual call (slot 2) */
        }
    }
    menu_clip_full();
    if (editable != 0 && cursor.value == row) {
        if (indexEnd(&select[row]) - indexBegin(&select[row]) >= 2) {
            f32 labelX = labels[row].x.get();
            f32 labelY = labels[row].y.get();
            f32 labelW = labels[row].getW();
            if (indexBegin(&select[row]) < indexValue(&select[row])) {
                arrowL.setX(labelX - labelW * 0.5f - arrowL.getW());
                arrowL.setY(labelY);
                arrowL.getColor(0)->a = blink;
                arrowL.getColor(1)->a = blink;
                arrowL.getColor(2)->a = blink;
                arrowL.getColor(3)->a = blink;
                arrowL.virtual_2();
            }
            if (indexValue(&select[row]) < indexEnd(&select[row]) - 1) {
                /* sic: arrowL's width (0xDAC), not arrowR's */
                arrowR.setX(labelX + labelW * 0.5f + arrowL.getW());
                arrowR.setY(labelY);
                arrowR.getColor(0)->a = blink;
                arrowR.getColor(1)->a = blink;
                arrowR.getColor(2)->a = blink;
                arrowR.getColor(3)->a = blink;
                arrowR.virtual_2();
            }
        }
    }
}

/* draw: Item's; nothing more without a pad, or on the USB page without a
 * wheel; the rows one frame late when not editable; "warning" while a row
 * conflicts (pads: only for the types the pad page knows). */
// FUNC 0x0012BE20 gt2k_menu::KeyConfigItem::virtual_2
void KeyConfigItem::virtual_2()
{
    Item::virtual_2();
    if (pad->status == 0)
        return;
    if (isUsb() && mode == KEYCFG_NONE)
        return;
    if (hideDelay > 0) {
        hideDelay--;
        return;
    }
    for (int row = 0; row < rows; row++)
        drawRow(row);
    if (anyConflict()) {
        if (isUsb() || pad->rawPSType() == PS_PAD_DIGITAL ||
            pad->rawPSType() == PS_PAD_DUALSHOCK || pad->rawPSType() == PS_PAD_DUALSHOCK2)
            warning.virtual_2();
    }
}

/* (? name) The moves go through a pointer: retail makes the virtual call
 * (slot 3) for cursor too, and forms &select[i] as index + this, the
 * address order of an inline function's argument. */
static inline int moveIndex(SmartIndexT<int> *index, int delta)
{
    return index->move(delta);
}

/* A frame of input (OptionControl::mainLoop): up / down move the row,
 * left / right the choice of the row, decide on the default row resets the
 * mode's assignment; then every assignable row is written back. */
// FUNC 0x0012BF08 gt2k_menu::KeyConfigItem::handleInput
void KeyConfigItem::handleInput()
{
    if (port == 1 && !isUsb() && pad->status == 0) {
        updateMode();
        return;
    }
    if (wait > 0)
        wait--;
    updateMode();
    if (editable == 0)
        return;
    int moved = 0;
    if (pad->getPrev(0))
        moved = moveIndex(&cursor, -1);
    else if (pad->getNext(0))
        moved = moveIndex(&cursor, 1);
    else if (pad->getPrev(1))
        moved = moveIndex(&select[cursor.value], -1);
    else if (pad->getNext(1))
        moved = moveIndex(&select[cursor.value], 1);
    else if (pad->decide != 0 && cursor.value == defaultRow && mode != KEYCFG_NONE) {
        owner->options.config.resetKeys(mode, port);
        resetSelection();
        menu_play_se(MENU_SE_CURSOR);
    }
    if (moved) {
        blink = 0.5f;
        menu_play_se(MENU_SE_CURSOR);
    }
    for (int row = 0; row < rows; row++) {
        ControllerConfig *config = &owner->options.config;
        if (rowFunction(row) != KEY_FUNC_DEFAULT && mode != KEYCFG_NONE) {
            int func = rowFunction(row);
            config->setKey(mode, port, func, selectedOption(row));
        }
    }
}

/* Decide (not on the default row) or cancel leave the page
 * (pages->exitRequest) when this page - and on the pad page the other
 * player's, if it has a controller - is free of conflicts; otherwise
 * MENU_SE_REFUSED.  Nothing for the first frames (wait). */
// FUNC 0x0012C158 gt2k_menu::KeyConfigItem::checkExit
void KeyConfigItem::checkExit(KeyConfigItem *other)
{
    if (wait > 0)
        return;
    int canLeave = 0;
    if (isUsb()) {
        if (mode == KEYCFG_NONE || !anyConflict())
            canLeave = 1;
    } else if (!anyConflict() && mode != KEYCFG_NONE) {
        canLeave = 1;
        if (other != NULL && other->mode != KEYCFG_NONE)
            canLeave = other->anyConflict() == 0;
    }
    if (pad->decide != 0 && cursor.value != defaultRow) {
        if (canLeave)
            pages->exitRequest = 1;
        else
            menu_play_se(MENU_SE_REFUSED);
    } else if (pad->cancel != 0) {
        if (canLeave)
            pages->exitRequest = 1;
        else
            menu_play_se(MENU_SE_REFUSED);
    }
}

/* No callers in the binary. */
// FUNC 0x0012C268 gt2k_menu::KeyConfigItem::getPort
int KeyConfigItem::getPort()
{
    return port;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0012C270 __tfQ29gt2k_menu13KeyConfigItem
extern "C" void *__tfQ29gt2k_menu13KeyConfigItem(void)
{
    if (__tiQ29gt2k_menu13KeyConfigItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu13KeyConfigItem, "Q29gt2k_menu13KeyConfigItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu13KeyConfigItem;
}

/* Compiler-generated typeinfo function of the template instance
 * SmartIndexT<int> (__user_type_info, no bases): T101's copy, never
 * referenced (the vtable copy 0x00314388 has no users either; the linker
 * kept T017's 0x00103690).  Weak so that the copies can be linked. */
// FUNC 0x0012C2C0 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif /* !BOOT_OLD_GCC */
