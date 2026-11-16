/* T041 - gt2k_menu::EventStatusObject and gt2k_menu::EventStatusButtonObject,
 * the event result marks of the GT-mode race menus.  0x00117C58..0x001190D0.
 *
 * Event names end in a two-digit number ("<base>NN").  EventStatusObject
 * finds the events <base>NN, <base>NN+1, ... that the event data base has
 * and draws a row of "frame" pictures (or a frame, "frame_arrow" pictures
 * and a frame) centred on its x, each with the "mark_<place + 1>" picture
 * of that event's best place (GameStatus::events, EventResults::getPlace;
 * 8: never raced, no mark).  EventStatusButtonObject draws, at an event
 * button's top-left corner, the mark of its place and the picture named
 * after the event's course, bright on the focused button, dimmed
 * otherwise.
 *
 * Quirk (both classes): marks[place] is read for every place but 8; a place
 * 6 or 7 would read frame / frameArrow (+0x12C / +0x130) - the races have
 * six cars, so getPlace gives 0..5. */
/* Retail's .rodata (0x0030EFE8..0x0030F2B8): "frame_arrow", the vtables,
 * then the type names in the reverse order of the classes' definitions:
 * EventStatusButtonObject, EventStatusObject, Cell<Button>, OperatorList,
 * List / Cell<OperatorHolder>, OperatorHolder, the GameStatus's garage
 * lists, Operator (a base: last).  No List<Button> or SmartIndexT<int>:
 * the original saw Button, not ButtonList (T244_button_list.h). */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::events; T425 getPlace */
#include "game/menu/framework/T206_button.h"               /* Button (posX / posY, focused, linkMenu) */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData (the button's record) */
#include "game/data/T435_product_config.h"       /* product_config, hasEvent */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T484_race_database.h"        /* RaceDataBase */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen */
#include "runtime/newlib/G007_stdio.h"                /* strcpy, sscanf */
#include "game/menu/framework/T229_page_layout.h"          /* MenuLayoutItem (z) */
/* Own header last (gtmode_buttons_internal.h adds no class): EventStatusButtonObject's and
 * EventStatusObject's type names come first after the vtables (retail
 * 0x0030F088). */
#include "game/menu/gtmode/T041_event_status.h"
#include "game/menu/gtmode/gtmode_buttons_internal.h"

namespace gt2k_menu {

// FUNC 0x00117C58 gt2k_menu::EventStatusObject::EventStatusObject
/* One caller (T036 make_event_status).  Item(res, NULL); the marks and
 * frames (Object::operator new(0x100) each; operator delete if an Item
 * constructor throws) get this item's y; `first' is read with sscanf from
 * the last two characters of linkMenu.  No arrow frames when the name's
 * 4th and 3rd last characters are "00".  The item's y is read with
 * MotionValue::get() (retail: through &this->y). */
EventStatusObject::EventStatusObject(MenuLayoutItem *res, const char *linkMenu,
                                     PageLayout *layout)
    : Item(res, (MenuTexture *)0)
{
    char base[0x40];                        /* sp+0x00 "mark_N", then the name without its number */
    char name[0x40];                        /* sp+0x40 "<base>NN" */

    this->linkMenu = linkMenu;
    first = 0;
    end = 0;
    count = 0;
    arrows = 0;
    for (int i = 0; i < 6; i++) {
        sprintf(base, "mark_%d", i + 1);
        /* implicit: operator new, operator delete (new-expression) */
        marks[i] = new Item(base, layout);
        marks[i]->y.set(y.get());
    }
    frame = new Item("frame", layout);
    frame->y.set(y.get());
    frameArrow = new Item("frame_arrow", layout);
    frameArrow->y.set(y.get());

    if (this->linkMenu != 0) {
        int len = strlen(this->linkMenu);
        char *digits;
        int number;

        strcpy(base, this->linkMenu);
        digits = base + (len - 2);
        sscanf(digits, "%02d", &first);
        number = first;
        *digits = 0;                        /* base = the name without its number */
        while (number < EVENT_NUMBER_LIMIT) {
            sprintf(name, "%s%02d", base, number);
            if (product_config()->hasEvent(name) == 0)
                break;
            number++;
        }
        end = number;
        count = number - first;
        if (this->linkMenu[len - 4] == '0' && this->linkMenu[len - 3] == this->linkMenu[len - 4])
            arrows = 0;
        else
            arrows = 1;
    }
}

// FUNC 0x00117F20 gt2k_menu::EventStatusObject::~EventStatusObject
/* Each pointer is cleared before its object is deleted (the marks only when
 * they are set).  Then ~Item(0) and, if (flags & 1), Object's operator
 * delete. */
EventStatusObject::~EventStatusObject()
{
    Item *item;

    for (int i = 0; i < 6; i++) {
        if (marks[i] != 0) {
            item = marks[i];
            marks[i] = 0;
            delete item;
        }
    }
    item = frame;
    frame = 0;
    delete item;
    item = frameArrow;
    frameArrow = 0;
    delete item;
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00118010 gt2k_menu::EventStatusObject::virtual_1
/* The marks' and frames' updates (not Item::virtual_1). */
void EventStatusObject::virtual_1()
{
    for (int i = 0; i < 6; i++) {
        if (marks[i] != 0)
            marks[i]->virtual_1();
    }
    if (frame != 0)
        frame->virtual_1();
    if (frameArrow != 0)
        frameArrow->virtual_1();
}

// FUNC 0x001180B8 gt2k_menu::EventStatusObject::virtual_2
/* Draw.  The frame / frameArrow items are moved and drawn once per event;
 * this item itself is not drawn.  With arrows: frame, (count - 2) arrow
 * frames, frame, total width 2 frame + (count - 1) arrow (as computed); the
 * mark of an arrow frame is moved back by half the width difference.
 * Without: count frames 8 apart, total width count * frame + count * 8 / 2
 * (as computed: half the gaps). */
void EventStatusObject::virtual_2()
{
    char base[0x40];                        /* sp+0x00 the name without its number */
    char name[0x40];                        /* sp+0x40 "<base>NN" */
    EventResults *results;
    int place;

    if (linkMenu == 0 || count == 0 || frame == 0 || frameArrow == 0)
        return;
    results = &getGameStatus()->events;
    strcpy(base, linkMenu);
    base[strlen(base) - 2] = 0;

    if (arrows != 0) {
        f32 total = frame->w + frame->w + frameArrow->w * (f32)(count - 1);
        Item *mark = 0;
        f32 x, shift;

        x = getScreenX() - total * 0.5f;
        x += frame->w * 0.5f;
        place = results->getPlace(linkMenu);
        if (place != EVENT_PLACE_NONE)
            mark = marks[place];
        frame->x.set(x);
        frame->virtual_2();
        if (mark != 0) {
            mark->x.set(x);
            mark->virtual_2();
        }
        x += frame->w * 1.5f;
        shift = (frameArrow->w - frame->w) * 0.5f;
        for (int number = first + 1; number < end; number++) {
            Item *box = (number == end - 1) ? frame : frameArrow;
            Item *boxMark = 0;

            x += box->w * 0.5f;
            sprintf(name, "%s%02d", base, number);
            place = results->getPlace(name);
            if (place != EVENT_PLACE_NONE)
                boxMark = marks[place];
            box->x.set(x);
            box->virtual_2();
            if (boxMark != 0) {
                f32 markX = x;
                if (number != end - 1)
                    markX = x - shift;
                boxMark->x.set(markX);
                boxMark->virtual_2();
            }
            x += box->w * 0.5f;
        }
    } else {
        f32 gap = EVENT_FRAME_GAP;           /* a local: retail keeps it in $f22 */
        f32 n = (f32)count;
        f32 total = frame->w * n + n * gap * 0.5f;
        f32 x = getScreenX() - total * 0.5f;

        for (int number = first; number < end; number++) {
            Item *boxMark = 0;

            x += frame->w * 0.5f;
            sprintf(name, "%s%02d", base, number);
            place = results->getPlace(name);
            if (place != EVENT_PLACE_NONE)
                boxMark = marks[place];
            frame->x.set(x);
            frame->virtual_2();
            if (boxMark != 0) {
                boxMark->x.set(x);
                boxMark->virtual_2();
            }
            x += frame->w * 0.5f;
            x += gap;
        }
    }
}

// FUNC 0x001184B0 gt2k_menu::EventStatusButtonObject::EventStatusButtonObject
/* One caller (T036 make_event_status_button).  Transitive(); the mark of
 * the best place of the button's event (its record's name), then the
 * picture named by the event's course (a RaceDataBase on the stack:
 * select(nameToCode(event)), F_002389C8(course), the course record's
 * name0A; destroyed with flags 2), then z = res->z.  Object::operator
 * new(0x100) for each Item (operator delete if its constructor throws). */
EventStatusButtonObject::EventStatusButtonObject(MenuLayoutItem *res, Button *button,
                                                 PageLayout *layout)
    : Transitive()
{
    const char *event;
    int place;

    this->button = button;
    mark = 0;
    picture = 0;
    event = button->linkMenu->name;
    place = getGameStatus()->events.getPlace(event);
    if (place != EVENT_PLACE_NONE) {
        char markName[0x40];                /* sp+0x00 (the RaceDataBase reuses the slot) */
        sprintf(markName, "mark_%d", place + 1);
        /* implicit: operator new, operator delete (new-expression) */
        mark = new Item(markName, layout);
    }
    {
        RaceDataBase db;                    /* sp+0x00 */
        if (db.select(nameToCode(event)) != 0) {
            const char *courseName = db.F_002389C8(db.course);
            picture = new Item(courseName, layout);
        }
    }
    z = res->z;
}

// FUNC 0x00118660 gt2k_menu::EventStatusButtonObject::~EventStatusButtonObject
/* Clears each pointer, then deletes its object.  Then ~Transitive(0) and,
 * if (flags & 1), Object's operator delete. */
EventStatusButtonObject::~EventStatusButtonObject()
{
    Item *item;

    item = mark;
    mark = 0;
    delete item;
    item = picture;
    picture = 0;
    delete item;
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00118700 gt2k_menu::EventStatusButtonObject::virtual_1
/* The mark's and the picture's updates. */
void EventStatusButtonObject::virtual_1()
{
    if (mark != 0)
        mark->virtual_1();
    if (picture != 0)
        picture->virtual_1();
}

// FUNC 0x00118760 gt2k_menu::EventStatusButtonObject::virtual_2
/* Draw.  Each picture's own position is an offset from the button's
 * top-left corner (read with MotionValue::get(): retail reads it through
 * &item->x / &item->y): it is moved there, coloured grey 0x80 when the button is
 * focused, else 0x40 (alpha 0x80; Drawable::setColor inline), drawn, and
 * its offset put back. */
void EventStatusButtonObject::virtual_2()
{
    Item *item = mark;

    if (item != 0) {
        Button *b = button;
        f32 offX = item->x.get();
        f32 offY = item->y.get();

        item->x.set(b->posX - b->w * 0.5f + offX);
        item->y.set(b->posY - b->h * 0.5f + offY);
        if (b->focused != 0)
            item->Drawable::setColor(MENU_GREY_FOCUSED);
        else
            item->Drawable::setColor(MENU_GREY_UNFOCUSED);
        item->virtual_2();
        mark->x.set(offX);
        mark->y.set(offY);
    }
    item = picture;
    if (item != 0) {
        Button *b = button;
        f32 offX = item->x.get();
        f32 offY = item->y.get();

        item->x.set(b->posX - b->w * 0.5f + offX);
        item->y.set(b->posY - b->h * 0.5f + offY);
        if (b->focused != 0)
            item->Drawable::setColor(MENU_GREY_FOCUSED);
        else
            item->Drawable::setColor(MENU_GREY_UNFOCUSED);
        item->virtual_2();
        picture->x.set(offX);
        picture->y.set(offY);
    }
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
// FUNC 0x00119030 __tfQ29gt2k_menu23EventStatusButtonObject
//   Builds the __si_type_info __tiQ29gt2k_menu23EventStatusButtonObject (base
//   Transitive: __tfQ29gt2k_menu10Transitive first) on the first call.
extern "C" void *__tfQ29gt2k_menu23EventStatusButtonObject(void)
{
    if (__tiQ29gt2k_menu23EventStatusButtonObject[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu23EventStatusButtonObject,
                  "Q29gt2k_menu23EventStatusButtonObject",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu23EventStatusButtonObject;
}

// FUNC 0x00119080 __tfQ29gt2k_menu17EventStatusObject
//   Builds the __si_type_info __tiQ29gt2k_menu17EventStatusObject (base Item:
//   __tfQ29gt2k_menu4Item first) on the first call.
extern "C" void *__tfQ29gt2k_menu17EventStatusObject(void)
{
    if (__tiQ29gt2k_menu17EventStatusObject[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu17EventStatusObject, "Q29gt2k_menu17EventStatusObject",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu17EventStatusObject;
}
#endif
