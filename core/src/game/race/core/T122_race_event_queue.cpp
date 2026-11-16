/* T122_race_event_queue.cpp - RaceEventQueue (0x00179468..0x00179878).
 *
 * The race's event queue (RaceOrganization +0x84): up to 256 events {type,
 * timer, arg} in posting order, the pending count of each type, and the
 * count.  Events with a timer expire after that many update() calls (one per
 * frame, RaceBase::virtual_27).  When the queue is full, posting drops the
 * event of the largest type (lower types are more important) unless it is
 * more important than the new one; otherwise the new event is lost.  The
 * queue is edited with interrupts off.
 */
/* Retail's .rodata (0x00327418..0x00327438): the vtable and the class's type
 * name only (no "bad_alloc", no other class seen): the own header and the
 * outside declarations. */
#include "game/race/core/T122_race_event_queue.h"
#include "libc.h"                           /* memset */
#include "eekernel.h"                       /* EI */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_user */
#include "runtime/cxx/T548_tinfo.h"
#endif

enum { EVENT_QUEUE_SIZE = 0x100 };          /* RaceEventQueue::events[] */

// FUNC 0x00179468 RaceEventQueue::RaceEventQueue
//   Caller: RaceOrganization's constructor.
RaceEventQueue::RaceEventQueue()
{
    clear();
}

// FUNC 0x001794A0 RaceEventQueue::~RaceEventQueue
//   (slot 0.)  Only the vptr store and, with flags bit 0, operator delete.
RaceEventQueue::~RaceEventQueue()
{
    /* implicit: operator delete (flags & 1) */
}

// FUNC 0x001794D0 RaceEventQueue::clear
void RaceEventQueue::clear()
{
    count = 0;
    memset(counts, 0, sizeof(counts));
}

// FUNC 0x00179500 RaceEventQueue::remove
//   Removes the event at index, keeping the order of the others.
void RaceEventQueue::remove(int index)
{
    int wasOn = intr_suspend();
    counts[events[index].type]--;
    count--;
    for (int i = index; i < count; i++)
        events[i] = events[i + 1];
    if (wasOn != 0)
        EI();
}

// FUNC 0x001795A0 RaceEventQueue::update
//   Counts the timers down; an event whose timer reaches 0 is removed.
void RaceEventQueue::update()
{
    int wasOn = intr_suspend();
    for (int i = 0; i < count; i++) {
        RaceQueueEvent *e = &events[i];
        if (e->timer != 0) {
            e->timer--;
            if (e->timer == 0)
                remove(i--);                /* the next event moved to i */
        }
    }
    if (wasOn != 0)
        EI();
}

// FUNC 0x00179640 RaceEventQueue::find
//   17 callers.  The first event of `type': 1 and its arg (when arg !=
//   NULL); take != 0 also removes it.  0 when there is none.
int RaceEventQueue::find(int type, int *arg, int take)
{
    if (counts[type] == 0)
        return 0;
    int n = count;
    for (int i = 0; i < n; i++) {
        RaceQueueEvent *e = &events[i];
        if (e->type == type) {
            if (arg != 0)
                *arg = e->arg;
            if (take != 0)
                remove(i);
            return 1;
        }
    }
    return 0;
}

// FUNC 0x001796C0 RaceEventQueue::post
//   52 callers.  Appends an event; delay = its timer (0: it stays until
//   taken).
void RaceEventQueue::post(int type, int arg, int delay)
{
    if (count == EVENT_QUEUE_SIZE) {
        int worst = 0;
        int worstType = -1;
        for (int i = 0; i < count; i++) {   /* (count is 0x100 here: a register bound, as retail) */
            if (worstType < events[i].type) {
                worstType = events[i].type;
                worst = i;
            }
        }
        if (worstType < type)
            return;                         /* full of more important events: dropped */
        remove(worst);
    }
    counts[type]++;
    /* Matching (u042): 98.1%; retail keeps arg in $s3 and delay in $s2
     * (ours the other way round: a global-alloc tie; all 24 orders of the
     * four stores tried). */
    int n = count;
    events[n].type = type;
    events[n].timer = delay;
    events[n].arg = arg;
    count = n + 1;
}

// FUNC 0x00179798 RaceEventQueue::findNth
//   The nth (0 = first) event of `type': 1 and its arg (when arg != NULL).
int RaceEventQueue::findNth(int type, int *arg, int nth)
{
    if (nth < counts[type]) {
        for (int i = 0; i < count; i++) {
            RaceQueueEvent *e = &events[i];
            if (e->type == type) {
                if (nth == 0) {
                    if (arg != 0)
                        *arg = e->arg;
                    return 1;
                }
                nth--;
            }
        }
    }
    return 0;
}

// FUNC 0x00179808 RaceEventQueue::getCount
//   No callers (RaceOrganization::getEventCount has its own copy).
u8 RaceEventQueue::getCount(int type)
{
    return counts[type];
}

// FUNC 0x00179818 RaceEventQueue::reset
//   No callers.
void RaceEventQueue::reset()
{
    clear();
}

#ifndef BOOT_OLD_GCC
// FUNC 0x00179838 __tf14RaceEventQueue
//   Builds the __user_type_info __ti14RaceEventQueue on the first call.
extern "C" const type_info &__tf14RaceEventQueue(void)
{
    if (__ti14RaceEventQueue[0] == 0)
        __rtti_user(__ti14RaceEventQueue, "14RaceEventQueue");
    return *(const type_info *)__ti14RaceEventQueue;
}
#endif
