/* T185_error_event_view.h - ErrorEventView, the error messages of a race
 * (translation unit T185, unit u061), 0x001AE4E0..0x001AE880.
 *
 * The messages posted to the race event queue (event 0x17, its argument a
 * message string, T122) are shown at (x, y) one under the other, 120 frames
 * each.  They are list nodes in `entries' (one slot here), linked from
 * `head', the free nodes from `freeHead'.  RacePS2Base (T130, +0xBCD0)
 * holds it: its constructor, virtual_8 (reset), virtual_27 (update: only
 * drains the events) and virtual_46 (draw); tick and collect are not
 * referenced. */
#ifndef GAME_T185_ERROR_EVENT_VIEW_H
#define GAME_T185_ERROR_EVENT_VIEW_H

#include "game/game.h"

class RaceOrganization;                     /* T125_race_organization.h (pointers only) */
class GSBuffer;                             /* T514_gs_buffer.h */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __user_type_info at 0x003533D8 (gp-0x4618), no base. */
extern "C" const type_info &__tf14ErrorEventView(void);    /* 0x001AE840 */
#endif

/* One message: a list node indexed from the view, 8 bytes. */
struct ErrorEntry {
    s16         next;                       /* 0x0 the next index, -1: end */
    s16         timer;                      /* 0x2 frames left (signed: tick's test is sll 16; bnez) */
    const char *msg;                        /* 0x4 the argument of the event */
};
EGCS_SIZE(ErrorEntry, 0x8);

/* vtable 0x00330C90 (1 slot), typeinfo __tf14ErrorEventView 0x001AE840
 * (user).  0x18 bytes of data, egcs vptr +0x18: size 0x1C (RacePS2Base
 * gives it 0x20: the next member, RaceBGMPS2, is 8-aligned; u061 / u044 had
 * the 4 bytes as a pad, nothing reads them). */
class ErrorEventView {
public:
    ErrorEventView();                       /* 0x001AE4E0 setPosition(100, 100); the lists are not reset */
    virtual ~ErrorEventView();              /* [0] +0x0C 0x001AE520 */
    void reset();                           /* 0x001AE550 freeHead 0, head -1, entries[0].next -1 */
    void setPosition(int x, int y);         /* 0x001AE568 */
    void update(RaceOrganization *org);     /* 0x001AE578 drain(org) */
    /* One frame: the expired messages go back to the free list (no
     * callers).  Quirk: the walk goes on with the freed node's new next. */
    void tick();                            /* 0x001AE598 */
    /* The 0x17 events into the list (a message already shown restarts its
     * timer); when no node is free, the rest are drained (no callers). */
    void collect(RaceOrganization *org);    /* 0x001AE610 */
    void drain(RaceOrganization *org);      /* 0x001AE6B8 every 0x17 event removed */
    ErrorEntry *find(const char *msg);      /* 0x001AE6F8 the shown node of msg (the pointer), NULL */
    ErrorEntry *alloc();                    /* 0x001AE738 the first free node, pushed in front; NULL */
    /* The messages one under the other (18 pixels apart), font "sw_12",
     * colour 0.8, into buf. */
    void draw(GSBuffer *buf);               /* 0x001AE770 */

    ErrorEntry entries[1];                  /* 0x00 */
    int        head;                        /* 0x08 the first shown, -1: none */
    int        freeHead;                    /* 0x0C the first free, -1: none */
    int        x;                           /* 0x10 */
    int        y;                           /* 0x14 */
    /*         vptr                            0x18 */
};
EGCS_SIZE(ErrorEventView, 0x1C);

#endif /* GAME_T185_ERROR_EVENT_VIEW_H */
