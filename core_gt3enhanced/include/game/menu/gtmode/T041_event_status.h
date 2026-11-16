/* T041_event_status.h - gt2k_menu::EventStatusObject and
 * gt2k_menu::EventStatusButtonObject, the event result marks of the GT-mode
 * race menus (translation unit T041, unit u008), 0x00117C58..0x001190D0.
 *
 * Event names end in a two-digit number ("<base>NN").  An EventStatusObject
 * finds the events <base>NN, <base>NN+1, ... the event data base (T435
 * F_0021D7A0) has and draws a row of "frame" pictures (or a frame,
 * "frame_arrow" pictures and a frame) centred on its x, each with the
 * picture "mark_<result + 1>" of that event's result (T425 F_00217080 on
 * GameStatus+0x23510; 8: none).  An EventStatusButtonObject draws, at an
 * event button's top-left corner, the mark of its result and the picture
 * the race data base (T484) names for it (the name of the event's course),
 * bright on the focused button, dimmed otherwise. */
#ifndef GAME_T041_EVENT_STATUS_H
#define GAME_T041_EVENT_STATUS_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

namespace gt2k_menu {
class Button;                               /* T206 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x0030F040 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu17EventStatusObject 0x00119080 (si, base Item; storage
 * 0x004B1560).  Size 0x134 (operator new in T036 F_00121228). */
class EventStatusObject : public Item {
public:
    /* Item(res, NULL); the marks and frames at this item's y; first from
     * the last two characters of linkMenu (sscanf "%02d"). */
    EventStatusObject(MenuLayoutItem *res, const char *linkMenu,
                      PageLayout *layout);  /* 0x00117C58 */
    virtual ~EventStatusObject();           /* [0] +0x0C 0x00117F20 deletes the marks and frames */
    virtual void virtual_1();               /* [1] +0x14 0x00118010 update: the marks' and frames'
                                               (not Item's) */
    virtual void virtual_2();               /* [2] +0x1C 0x001180B8 draw: a frame and its mark per event
                                               (this item itself is not drawn) */

    const char *linkMenu;                   /* 0x100 the first event's name */
    int   first;                            /* 0x104 its number (the last two digits) */
    int   end;                              /* 0x108 the first number that is not an event */
    int   count;                            /* 0x10C end - first */
    /* arrows: 0 if the name's 4th and 3rd last characters are "00", else
     * arrow frames between the first and the last frame. */
    int   arrows;                           /* 0x110 */
    Item *marks[6];                         /* 0x114 "mark_1" .. "mark_6", by result */
    Item *frame;                            /* 0x12C "frame" */
    Item *frameArrow;                       /* 0x130 "frame_arrow" */
};
EGCS_SIZE(gt2k_menu::EventStatusObject, 0x134);

/* vtable 0x0030EFF8 (8 slots: [3] Object's, [4]..[7] Transitive's).
 * typeinfo __tfQ29gt2k_menu23EventStatusButtonObject 0x00119030 (si, base
 * Transitive; storage 0x004B1570).  Size 0xD0 (operator new in T036
 * F_001212E0). */
class EventStatusButtonObject : public Transitive {
public:
    /* Transitive(); the mark of the result of button's event
     * (button->linkMenu), the race data base's picture for it (the hidden
     * item named RaceDataBase::F_002389C8(course), the course record's
     * name0A), z = res->z. */
    EventStatusButtonObject(MenuLayoutItem *res, Button *button,
                            PageLayout *layout);    /* 0x001184B0 */
    virtual ~EventStatusButtonObject();     /* [0] +0x0C 0x00118660 deletes mark and picture */
    virtual void virtual_1();               /* [1] +0x14 0x00118700 update: mark's and picture's */
    virtual void virtual_2();               /* [2] +0x1C 0x00118760 draw: each picture at its offset from
                                               the button's top-left corner, grey 0x80 on the focused
                                               button, else 0x40 (alpha 0x80) */

    Button *button;                         /* 0xC4 */
    Item   *mark;                           /* 0xC8 "mark_<result + 1>", NULL without a result */
    Item   *picture;                        /* 0xCC the race data base's picture, NULL if none */
};
EGCS_SIZE(gt2k_menu::EventStatusButtonObject, 0xD0);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu23EventStatusButtonObject(void);  /* 0x00119030 */
void *__tfQ29gt2k_menu17EventStatusObject(void);        /* 0x00119080 */
}
#endif

#endif /* GAME_T041_EVENT_STATUS_H */
