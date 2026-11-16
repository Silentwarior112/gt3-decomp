/* T185 - ErrorEventView: the error messages posted to the race event queue
 * (RACE_EVENT_ERROR, 0x17, its argument a message string), each shown for
 * 120 frames.  0x001AE4E0..0x001AE880.
 *
 * The messages are list nodes in `entries' (one slot), linked from `head';
 * the free nodes from `freeHead'.  In the game only reset, update (which
 * just drains the events) and draw are called: tick and collect are
 * unreferenced, so after reset the list stays empty. */
/* Retail's .rodata (0x00330C80..0x00330CC9): "bad_alloc" (<new>'s inline
 * what(), written when the header is read: DATA_START), the vtable, then
 * the type names ErrorEventView, UnitArena<64> (reverse definition order). */
// DATA_START 0x00330C80
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include <new>
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/race/display/T185_error_event_view.h"
#include "engine/pgl/G001_flare.h"                /* gfx_disable, PGL_CAP_* */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find, RACE_EVENT_ERROR */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_user */
#endif
#include "game/effects/replay_effects_internal.h"                           /* ERROR_SHOW_FRAMES, ERROR_ENTRIES */

/* Caller: RacePS2Base's constructor.  (100, 100); the lists are not reset
 * here (reset is called by RacePS2Base::virtual_8). */
// FUNC 0x001AE4E0 ErrorEventView::ErrorEventView
ErrorEventView::ErrorEventView()
{
    setPosition(100, 100);
}

// FUNC 0x001AE520 ErrorEventView::~ErrorEventView
/* Its vtable, then operator delete when flags & 1.  Callers:
 * RacePS2Base's constructor (exception path), RacePS2Base::virtual_0. */
ErrorEventView::~ErrorEventView()
{
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x001AE550 ErrorEventView::reset
void ErrorEventView::reset()
{
    int i;

    freeHead = 0;
    head = -1;
    /* The free list through every node (a loop of none with one node). */
    for (i = 0; i < ERROR_ENTRIES - 1; i++)
        entries[i].next = i + 1;
    entries[i].next = -1;
}

// FUNC 0x001AE568 ErrorEventView::setPosition
void ErrorEventView::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}

/* Caller: RacePS2Base::virtual_27 (per frame). */
// FUNC 0x001AE578 ErrorEventView::update
void ErrorEventView::update(RaceOrganization *org)
{
    drain(org);
}

/* No callers.  One frame: the expired messages go back to the free list.
 * Quirk: the walk continues with the freed node's new `next' (the old free
 * head), not with its successor in the shown list. */
// FUNC 0x001AE598 ErrorEventView::tick
void ErrorEventView::tick()
{
    int i = head;
    int prev = -1;

    while (i >= 0) {
        ErrorEntry *e = &entries[i];
        e->timer = e->timer - 1;
        if (e->timer == 0) {
            if (prev >= 0)
                entries[prev].next = e->next;
            else
                head = e->next;
            e->next = freeHead;
            freeHead = i;
        } else {
            prev = i;
        }
        i = entries[i].next;
    }
}

/* No callers.  The error events into the list (a message already shown
 * restarts its timer); when no node is free, the rest are drained. */
// FUNC 0x001AE610 ErrorEventView::collect
void ErrorEventView::collect(RaceOrganization *org)
{
    int arg;                                            /* sp+0x00 */

    while (org->eventQueue.find(RACE_EVENT_ERROR, &arg, 1) != 0) {
        const char *msg = (const char *)arg;
        ErrorEntry *e = find(msg);
        if (e == NULL) {
            e = alloc();
            if (e == NULL) {
                drain(org);
                return;
            }
        }
        e->msg = msg;
        e->timer = ERROR_SHOW_FRAMES;
    }
}

/* Removes every error event. */
// FUNC 0x001AE6B8 ErrorEventView::drain
void ErrorEventView::drain(RaceOrganization *org)
{
    int arg;                                            /* sp+0x00 */

    while (org->eventQueue.find(RACE_EVENT_ERROR, &arg, 1) != 0)
        ;
}

/* The shown node of msg (the same pointer), NULL. */
// FUNC 0x001AE6F8 ErrorEventView::find
ErrorEntry *ErrorEventView::find(const char *msg)
{
    for (int i = head; i >= 0; i = entries[i].next) {
        if (msg == entries[i].msg)
            return &entries[i];
    }
    return NULL;
}

/* The first free node, pushed in front of the shown list. */
// FUNC 0x001AE738 ErrorEventView::alloc
ErrorEntry *ErrorEventView::alloc()
{
    int i = freeHead;
    if (i < 0)
        return NULL;
    freeHead = entries[i].next;
    entries[i].next = head;
    head = i;
    return &entries[i];
}

/* Caller: RacePS2Base::virtual_46.  The messages one under the other (18
 * pixels apart) in the system font, style "sw_12", grey 0.8, no depth
 * test, no fog. */
// FUNC 0x001AE770 ErrorEventView::draw
void ErrorEventView::draw(GSBuffer *buf)
{
    buf->setDrawBuffer();
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_FOG);
    g_002F1968.setStyle("sw_12");
    g_002F1968.setColor(0.8f, 0.8f, 0.8f, 0.8f);
    int i = head;
    int px = x;
    int py = y;
    while (i >= 0) {
        ErrorEntry *e = &entries[i];
        g_002F1968.setPos(px, py);
        py += 18;
        g_002F1968.print(e->msg);             /* the message as the format */
        i = e->next;
    }
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001AE840 __tf14ErrorEventView
/* __user_type_info __ti14ErrorEventView (gp-0x4618): no bases. */
extern "C" const type_info &__tf14ErrorEventView(void)
{
    if (__ti14ErrorEventView[0] == 0)
        __rtti_user(__ti14ErrorEventView, "14ErrorEventView");
    return *(const type_info *)__ti14ErrorEventView;
}
#endif
