/* T122_race_event_queue.h - RaceEventQueue (translation unit T122, unit u042),
 * 0x00179468..0x00179878.
 *
 * The race's event queue (RaceOrganization +0x84): up to 256 events {type,
 * timer, arg} in posting order, the pending count of each type and the
 * count.  An event with a timer expires after that many update() calls (one
 * per frame, RaceBase::virtual_27); timer 0: it stays until taken.  When the
 * queue is full, post() drops the event of the largest type (lower types are
 * more important) unless it is more important than the new one, else the new
 * event is lost.  The queue is edited with interrupts off.
 *
 * The event types are RACE_EVENT_* below (the sweep collected them from the
 * units' local copies).
 *
 * The element is named RaceQueueEvent here: u042 called it RaceEvent, which
 * is T484's race record (h11).
 */
#ifndef GAME_T122_RACE_EVENT_QUEUE_H
#define GAME_T122_RACE_EVENT_QUEUE_H

#include "game/game.h"

class type_info;

/* The typeinfo function (vtable entry 0), declared before the class, for
 * GCC 14 only: egcs generates it itself (a declaration it sees stops it
 * emitting the function; u042 matching). */
#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
const type_info &__tf14RaceEventQueue(void);    /* 0x00179838 (__user_type_info 0x00353370) */
}
#endif

/* One queued event.  Size 8 (u042 RaceEvent). */
struct RaceQueueEvent {
    s16 type;                               /* 0x0 lower = more important; counts[type] */
    u16 timer;                              /* 0x2 frames until it expires, 0 = never */
    int arg;                                /* 0x4 */
};
EGCS_SIZE(RaceQueueEvent, 0x8);

/* RaceQueueEvent::type: the event types posted / taken by the race (counts[]
 * has room for 0..RACE_EVENT_TYPES - 1; no use of 0 or 6 is known).
 *
 * The enhanced engine appends Lupo 2's new events after GT3's last (0x17),
 * so GT3's numbers (replays, i.LINK, every post / take) stay: Lupo 2
 * inserted its disqualification event at 0x0B and moved GT3's 0x0B..0x17
 * up by one (race_logic.md C3, hud.md C25); here it is 0x18. */
enum RaceEventType {
    RACE_EVENT_END            = 0x01,   /* ends the race frame: RaceBase::virtual_27 runs virtual_33
                                           (the result sequence is over, a demo ends, ...) */
    RACE_EVENT_COUNTDOWN      = 0x02,   /* the start sequence begins (RaceOrganization::
                                           updateStartAndTimer: a start action with a count) */
    RACE_EVENT_GO             = 0x03,   /* the race has started: posted by RaceBase::start
                                           (then the race music); the display's start */
    RACE_EVENT_START          = 0x04,   /* the countdown reached 0 / the driver has the car (the
                                           conductors): RaceBase::virtual_27 calls start */
    RACE_EVENT_GOAL           = 0x05,   /* a car reached the goal (arg: the car): RaceResultBase::
                                           update, RaceDisplay::virtual_16 */
    RACE_EVENT_TIME_UP        = 0x07,   /* the time limit expired */
    RACE_EVENT_STOP_TIMER     = 0x08,   /* stops the time limit (RaceOrganization::updateRanksAndEvents) */
    RACE_EVENT_LICENSE_FAILED = 0x09,   /* the licence test is failed (arg: the car; T305 / T314):
                                           RaceLicenseDisplay takes it, else RaceBase::update ends the
                                           race after the frame (virtual_33) */
    RACE_EVENT_DISQUALIFIED   = 0x0A,   /* DynamicsConductorTimeTrial: car 0 is disqualified
                                           (RaceTimeTrialDisplay's disqualification sequence) */
    RACE_EVENT_LAP_TIME       = 0x0B,   /* RaceDisplayLapTimeEvent (T194) */
    RACE_EVENT_CHECK_POINT    = 0x0C,   /* RaceDisplayCheckPointEvent (T194) */
    RACE_EVENT_LOGGER_SECTOR  = 0x0D,   /* a data logger sector (arg: the car's index) */
    RACE_EVENT_GENERAL_TIME   = 0x0E,   /* RaceDisplayGeneralTimeEvent (T154) */
    RACE_EVENT_TIME_DIFF      = 0x0F,   /* RaceDisplayTimeDiffEvent (T154) */
    RACE_EVENT_LAP_DIFF       = 0x10,   /* RaceDisplayLapDiffEvent (T154) */
    RACE_EVENT_SPEED          = 0x11,   /* RaceDisplaySpeedEvent (T194) */
    RACE_EVENT_INFORMATION    = 0x12,   /* RaceDisplayInformationEvent (T194) */
    RACE_EVENT_LAP_RECORD     = 0x13,   /* a new lap record (DynamicsConductor::checkLapRecord, T308):
                                           RacePauseWithRetry disables the retry */
    RACE_EVENT_BGM            = 0x14,   /* the BGM (arg: a code; -2 pause, -3 resume, -1 stop, else
                                           play: RaceBase::virtual_27) */
    RACE_EVENT_VIEW_MODE      = 0x15,   /* the replay view mode changed (CameraManager::update T383,
                                           updateMode1 T390): arg (style << 16) | (mode << 8) | view;
                                           RaceDisplay::pollViewModeEvent */
    RACE_EVENT_RECORD_FULL    = 0x16,   /* a player's replay record is full (arg: the player;
                                           RaceDisplay::showReplayRecorded) */
    RACE_EVENT_ERROR          = 0x17,   /* an error / debug message (arg: a const char *):
                                           ErrorEventView (T185) */
    /* The enhanced engine (ref/enhanced/INTERFACES.md): one car is
     * disqualified (arg: the car), Lupo 2's 0x0B.  Posted by the Pod
     * race's lap check when a lap does not count (L2 0x001F7B20; race /
     * physics: the Pod rules); taken by the HUD (SimpleExamDisplay: its own
     * player car, hud.md C20) and the result board (the car placed last, not
     * running; menus_b.md 14).  The least important type: a full queue drops
     * it first. */
    RACE_EVENT_CAR_DISQUALIFIED = 0x18,
    RACE_EVENT_TYPES            = 0x19  /* the types counts[] has room for (GT3 0x18) */
};

/* vtable 0x00327418 (1 slot).  typeinfo __tf14RaceEventQueue 0x00179838.
 * vptr +0x81C (after the data).  Size 0x820 (RaceOrganization +0x84, then
 * +0x8A4).  The enhanced engine: counts[] has room for the appended type,
 * so count is at +0x81C, the vptr at +0x820 and the size 0x824 (Lupo 2's
 * queue grew the same word); RaceOrganization keeps its size. */
class RaceEventQueue {
public:
    RaceEventQueue();                       /* 0x00179468 clear() */
    virtual ~RaceEventQueue();              /* [0] +0x0C 0x001794A0 */
    void clear();                           /* 0x001794D0 count = 0, counts cleared */
    void remove(int index);                 /* 0x00179500 keeps the order of the others */
    void update();                          /* 0x001795A0 counts the timers down, drops the expired */
    /* The first event of `type': 1 and its arg (arg != NULL); take != 0
     * removes it.  0 when there is none. */
    int  find(int type, int *arg, int take);    /* 0x00179640 */
    void post(int type, int arg, int delay);    /* 0x001796C0 delay: the timer */
    /* The nth (0 = first) event of `type': 1 and its arg. */
    int  findNth(int type, int *arg, int nth);  /* 0x00179798 */
    u8   getCount(int type);                /* 0x00179808 counts[type] (no callers) */
    void reset();                           /* 0x00179818 clear() (no callers) */

    RaceQueueEvent events[256];             /* 0x000 */
    u8             counts[RACE_EVENT_TYPES];    /* 0x800 the pending events per type (GT3 0x18) */
    int            count;                   /* 0x81C (GT3 0x818) */
    /*  vptr                                   0x820 (GT3 0x81C) */
};
EGCS_SIZE(RaceEventQueue, 0x824);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti14RaceEventQueue[2];     /* 0x00353370 gp-0x4680 (u042: g_00353370) */
#endif

#endif /* GAME_T122_RACE_EVENT_QUEUE_H */
