/* T194_race_display_event.cpp - RaceDisplayEventBase and the race display
 * events (0x00190198..0x001909E0); the classes are declared in
 * include/game/race/display/T194_race_display_event.h.
 *
 * A display event is a small object that travels through the race event
 * queue (RaceOrganization::eventQueue) as (type, one 32-bit word): post()
 * queues getEventType() / pack() with a delay of 2 frames, take() removes
 * the first event of the type and unpacks it, peek() reads the nth one and,
 * when the object is bound to a car (car >= 0), accepts it only if it is
 * for that car.
 *
 * Word layouts (bits 0..3 always the car, clamped to 0..15):
 *   lap time    (type 0xB):  lap 8..17 (0..999)
 *   check point (type 0xC):  check point 4..7 (0..15), lap 8..17
 *   difference  (abstract):  a signed 28-bit value 4..31 (+-0x7FFFFFF);
 *                            setTime / getTime map times above 0xFFFFFFE
 *                            to 0xFFFFFFF / RACE_TIME_INVALID
 *   speed       (type 0x11): speed * 10 in 4..17 (0..9999)
 *   information (type 0x12): an information code 4..7
 * pack() sets the bit fields of an uninitialised word: the other bits are
 * whatever the register held (the original's quirk, kept).
 *
 * Data (u050's data pass): .rodata (0x0032DDA0..0x0032DF89) is "bad_alloc"
 * (<new>), the six vtables, then the type names in reverse definition
 * order: the events of game/race/display/T194_race_display_event.h (T154's three
 * difference events included: the original header defined them) and
 * UnitArena<64> last (T153 read first, with its name).  No .sdata.
 */
// DATA_START 0x0032DDA0
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's T194 has it first) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user (GCC 14's __tf functions) */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, last (T153_race_course.h) */
#include "game/race/core/T153_race_course.h"          /* (UnitArena<64>: retail's type names) */
#define GAME_T194_EMITS_TYPEINFO            /* egcs generates __tf20RaceDisplayEventBase here */
#include "game/race/display/T194_race_display_event.h"   /* own */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::find / post / findNth */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (eventQueue) */
#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID */
#include "game/race/core/race_championship_internal.h"                           /* the unit's constants */

/* lo <= v <= hi (the original's clamp: < lo first, then > hi) */
static inline int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* The event words, as bit fields (bit 0 first). */
union LapTimeBits {                         /* RaceDisplayLapTimeEvent */
    int word;
    struct {
        u32 car : 4;
        u32     : 4;
        u32 lap : 10;
    } f;
};
union CheckPointBits {                      /* RaceDisplayCheckPointEvent */
    int word;
    struct {
        u32 car : 4;
        u32 checkPoint : 4;
        u32 lap : 10;
    } f;
};
union DiffBits {                            /* RaceDisplayDiffEvent */
    int word;
    struct {
        u32 car : 4;
        s32 diff : 28;
    } f;
};
union SpeedBits {                           /* RaceDisplaySpeedEvent */
    int word;
    struct {
        u32 car : 4;
        u32 speed : 14;                     /* tenths */
    } f;
};
union InformationBits {                     /* RaceDisplayInformationEvent */
    int word;
    struct {
        u32 car : 4;
        u32 information : 4;
    } f;
};

/* ======================================================================== */
/* RaceDisplayEventBase                                                     */
/* ======================================================================== */

// FUNC 0x00190198 RaceDisplayEventBase::RaceDisplayEventBase
//   car = -1 (any), the vptr.  Callers: the 4 event constructors.
RaceDisplayEventBase::RaceDisplayEventBase()
{
    car = -1;
}

// FUNC 0x001901B8 RaceDisplayEventBase::post
//   Queues (getEventType(), pack()) with a delay of 2 frames
//   (RaceEventQueue::post).  14 callers (RaceDisplay::virtual_0, the T303 /
//   T315 / T350 event posters, ...).
void RaceDisplayEventBase::post(RaceOrganization *org)
{
    int type = getEventType();
    int bits = pack();
    org->eventQueue.post(type, bits, DISPLAY_EVENT_DELAY);
}

// FUNC 0x00190230 RaceDisplayEventBase::take
//   No callers.  Removes the first event of the type (RaceEventQueue::find
//   with take = 1) and unpacks it: 1, or 0 if there is none.
int RaceDisplayEventBase::take(RaceOrganization *org)
{
    int bits;                               /* sp+0x00 */
    int type = getEventType();
    if (org->eventQueue.find(type, &bits, 1) != 0) {
        unpack(bits);
        return 1;
    }
    return 0;
}

// FUNC 0x001902B0 RaceDisplayEventBase::peek
//   The nth event of the type (RaceEventQueue::findNth), unpacked; when
//   this event was bound to a car (car >= 0) and the event is another
//   car's, the car is restored and 0 returned.  Callers: the RaceDisplay
//   pollers (T154: pollLapTimeEvents, pollCheckPointEvents,
//   pollLapDiffEvents, pollTimeDiffEvents, pollSpeedEvents,
//   pollInformationEvents).
int RaceDisplayEventBase::peek(RaceOrganization *org, int nth)
{
    int bits;                               /* sp+0x00 */
    int type = getEventType();
    if (org->eventQueue.findNth(type, &bits, nth) != 0) {
        int boundCar = car;
        unpack(bits);
        if (boundCar < 0 || car == boundCar)
            return 1;
        car = boundCar;
        return 0;
    }
    return 0;
}

/* ======================================================================== */
/* RaceDisplayLapTimeEvent                                                  */
/* ======================================================================== */

// FUNC 0x00190350 RaceDisplayLapTimeEvent::RaceDisplayLapTimeEvent
//   RaceDisplayEventBase::RaceDisplayEventBase, the vptr, lap 0.  Callers:
//   RaceDisplayCheckPointEvent's constructor, RaceDisplay::
//   pollLapTimeEvents (T154), dc_post_lap_time (T303).
RaceDisplayLapTimeEvent::RaceDisplayLapTimeEvent()
    : RaceDisplayEventBase()
{
    lap = 0;
}

// FUNC 0x00190388 RaceDisplayLapTimeEvent::pack
//   (index: virtual_1.)  car in 0..3, lap in 8..17; bits 4..7 and 18..31
//   uninitialised.
int RaceDisplayLapTimeEvent::pack() const
{
    LapTimeBits bits;                       /* not initialised (quirk) */
    bits.f.car = clampi(car, 0, EVENT_CAR_MAX);
    bits.f.lap = clampi(lap, 0, EVENT_LAP_MAX);
    return bits.word;
}

// FUNC 0x001903F0 RaceDisplayLapTimeEvent::unpack
//   (index: virtual_2.)
void RaceDisplayLapTimeEvent::unpack(int word)
{
    LapTimeBits bits;
    bits.word = word;
    car = bits.f.car;
    lap = bits.f.lap;
}

/* ======================================================================== */
/* RaceDisplayCheckPointEvent                                               */
/* ======================================================================== */

// FUNC 0x00190408 RaceDisplayCheckPointEvent::RaceDisplayCheckPointEvent
//   RaceDisplayLapTimeEvent::RaceDisplayLapTimeEvent, the vptr, check
//   point 0.  Callers: RaceDisplay::pollCheckPointEvents (T154),
//   dc_post_section_time (T303).
RaceDisplayCheckPointEvent::RaceDisplayCheckPointEvent()
    : RaceDisplayLapTimeEvent()
{
    checkPoint = 0;
}

// FUNC 0x00190440 RaceDisplayCheckPointEvent::pack
//   (index: virtual_1.)  car in 0..3, the check point in 4..7, lap in
//   8..17; bits 18..31 uninitialised.
int RaceDisplayCheckPointEvent::pack() const
{
    CheckPointBits bits;                    /* not initialised (quirk) */
    bits.f.car = clampi(car, 0, EVENT_CAR_MAX);
    bits.f.checkPoint = clampi(checkPoint, 0, EVENT_CHECK_POINT_MAX);
    bits.f.lap = clampi(lap, 0, EVENT_LAP_MAX);
    return bits.word;
}

// FUNC 0x001904D8 RaceDisplayCheckPointEvent::unpack
//   (index: virtual_2.)
void RaceDisplayCheckPointEvent::unpack(int word)
{
    CheckPointBits bits;
    bits.word = word;
    car = bits.f.car;
    checkPoint = bits.f.checkPoint;
    lap = bits.f.lap;
}

/* ======================================================================== */
/* RaceDisplayDiffEvent                                                     */
/* ======================================================================== */

// FUNC 0x00190500 RaceDisplayDiffEvent::RaceDisplayDiffEvent
//   RaceDisplayEventBase::RaceDisplayEventBase, the vptr, diff 0.  13
//   callers (the T154 difference events and their users, T303 / T315).
RaceDisplayDiffEvent::RaceDisplayDiffEvent()
    : RaceDisplayEventBase()
{
    diff = 0;
}

// FUNC 0x00190538 RaceDisplayDiffEvent::pack
//   (index: virtual_1; also the slot of RaceDisplayGeneralTimeEvent /
//   LapDiffEvent / TimeDiffEvent.)  car in 0..3, the value clamped to
//   +-0x7FFFFFF in 4..31 (here the whole word is set).
int RaceDisplayDiffEvent::pack() const
{
    DiffBits bits;                          /* not initialised (all bits set below) */
    bits.f.car = clampi(car, 0, EVENT_CAR_MAX);
    bits.f.diff = clampi(diff, -EVENT_DIFF_MAX, EVENT_DIFF_MAX);
    return bits.word;
}

// FUNC 0x001905B0 RaceDisplayDiffEvent::unpack
//   (index: virtual_2.)  The value sign-extended (sra 4).
void RaceDisplayDiffEvent::unpack(int word)
{
    DiffBits bits;
    bits.word = word;
    car = bits.f.car;
    diff = bits.f.diff;
}

// FUNC 0x001905C8 RaceDisplayDiffEvent::setTime
//   A time (unsigned), 0xFFFFFFF if above 0xFFFFFFE (the largest the word
//   can carry).  Callers: dc_post_section_time / dc_post_lap_time (T303).
void RaceDisplayDiffEvent::setTime(u32 t)
{
    if (t > EVENT_TIME_MAX)
        t = EVENT_TIME_SATURATED;
    diff = t;
}

// FUNC 0x001905E8 RaceDisplayDiffEvent::getTime
//   The time; RACE_TIME_INVALID for a value above 0xFFFFFFE (unsigned).
//   Callers: RaceDisplay::pollLapTimeEvents / pollCheckPointEvents (T154).
u32 RaceDisplayDiffEvent::getTime()
{
    u32 t = diff;
    return (t > EVENT_TIME_MAX) ? RACE_TIME_INVALID : t;
}

/* ======================================================================== */
/* RaceDisplaySpeedEvent                                                    */
/* ======================================================================== */

// FUNC 0x00190608 RaceDisplaySpeedEvent::RaceDisplaySpeedEvent
//   RaceDisplayEventBase::RaceDisplayEventBase, the vptr, speed 0.
//   Callers: RaceDisplay::pollSpeedEvents (T154), dc_post_speed (T303).
RaceDisplaySpeedEvent::RaceDisplaySpeedEvent()
    : RaceDisplayEventBase()
{
    speed = 0.0f;
}

// FUNC 0x00190640 RaceDisplaySpeedEvent::pack
//   (index: virtual_1.)  car in 0..3, (int)(speed * 10) (truncated)
//   clamped to 0..9999 in 4..17; bits 18..31 uninitialised.
int RaceDisplaySpeedEvent::pack() const
{
    SpeedBits bits;                         /* not initialised (quirk) */
    bits.f.car = clampi(car, 0, EVENT_CAR_MAX);
    bits.f.speed = clampi((int)(speed * 10.0f), 0, EVENT_SPEED_MAX);
    return bits.word;
}

// FUNC 0x001906C0 RaceDisplaySpeedEvent::unpack
//   (index: virtual_2.)  The field converted as an unsigned value, times
//   0.1.
void RaceDisplaySpeedEvent::unpack(int word)
{
    SpeedBits bits;
    bits.word = word;
    car = bits.f.car;
    speed = (f32)(u32)bits.f.speed * 0.1f;
}

/* ======================================================================== */
/* RaceDisplayInformationEvent                                              */
/* ======================================================================== */

// FUNC 0x00190710 RaceDisplayInformationEvent::RaceDisplayInformationEvent
//   RaceDisplayEventBase::RaceDisplayEventBase, the vptr, information 0.
//   Callers: RaceDisplay::virtual_0 / pollInformationEvents (T154),
//   dc_post_pit_in / dc_post_pit_out (T350), DynamicsConductorFreePractice::
//   virtual_16 / virtual_17 (T315).
RaceDisplayInformationEvent::RaceDisplayInformationEvent()
    : RaceDisplayEventBase()
{
    information = 0;
}

// FUNC 0x00190748 RaceDisplayInformationEvent::pack
//   (index: virtual_1.)  car in 0..3, the low 4 bits of the information in
//   4..7 (not clamped); bits 8..31 uninitialised.
int RaceDisplayInformationEvent::pack() const
{
    InformationBits bits;                   /* not initialised (quirk) */
    bits.f.car = clampi(car, 0, EVENT_CAR_MAX);
    bits.f.information = information;
    return bits.word;
}

// FUNC 0x00190798 RaceDisplayInformationEvent::unpack
//   (index: virtual_2.)
void RaceDisplayInformationEvent::unpack(int word)
{
    InformationBits bits;
    bits.word = word;
    car = bits.f.car;
    information = bits.f.information;
}

/* Inline in game/race/display/T194_race_display_event.h (declared in the reverse of
 * this order); egcs emits these copies at the end of the unit, before
 * the class's typeinfo function (this unit defines the class's key
 * function, pack). */
// FUNC 0x001907B0 RaceDisplayInformationEvent::getEventType
// FUNC 0x001907B8 RaceDisplayInformationEvent::getInformation
//   No callers.
// FUNC 0x001907C0 RaceDisplayInformationEvent::setInformation
//   No callers.

#ifndef BOOT_OLD_GCC
// FUNC 0x001907C8 __tf27RaceDisplayInformationEvent
//   The __si_type_info __ti27RaceDisplayInformationEvent, base
//   __ti20RaceDisplayEventBase (gp-0x4658).
extern "C" const type_info &__tf27RaceDisplayInformationEvent(void)
{
    if (__ti27RaceDisplayInformationEvent[0] == 0) {
        __tf20RaceDisplayEventBase();
        __rtti_si(__ti27RaceDisplayInformationEvent, "27RaceDisplayInformationEvent",
                  (const type_info *)__ti20RaceDisplayEventBase);
    }
    return *(const type_info *)__ti27RaceDisplayInformationEvent;
}
#endif

/* ======================================================================== */
/* RaceDisplaySpeedEvent: accessors                                         */
/* ======================================================================== */

/* Inline in game/race/display/T194_race_display_event.h (declared in the reverse of
 * this order); egcs emits these copies at the end of the unit, before
 * the class's typeinfo function (this unit defines the class's key
 * function, pack). */
// FUNC 0x00190818 RaceDisplaySpeedEvent::getEventType
// FUNC 0x00190820 RaceDisplaySpeedEvent::getSpeed
// FUNC 0x00190828 RaceDisplaySpeedEvent::setSpeed

#ifndef BOOT_OLD_GCC
// FUNC 0x00190830 __tf21RaceDisplaySpeedEvent
//   The __si_type_info __ti21RaceDisplaySpeedEvent, base
//   __ti20RaceDisplayEventBase.
extern "C" const type_info &__tf21RaceDisplaySpeedEvent(void)
{
    if (__ti21RaceDisplaySpeedEvent[0] == 0) {
        __tf20RaceDisplayEventBase();
        __rtti_si(__ti21RaceDisplaySpeedEvent, "21RaceDisplaySpeedEvent",
                  (const type_info *)__ti20RaceDisplayEventBase);
    }
    return *(const type_info *)__ti21RaceDisplaySpeedEvent;
}

// FUNC 0x00190880 __tf20RaceDisplayDiffEvent
//   The __si_type_info __ti20RaceDisplayDiffEvent, base
//   __ti20RaceDisplayEventBase.  Callers: the T154 difference events'
//   typeinfo functions and their copies in T303 / T315
//   (__tf27RaceDisplayGeneralTimeEvent, __tf23RaceDisplayLapDiffEvent,
//   __tf24RaceDisplayTimeDiffEvent).
extern "C" const type_info &__tf20RaceDisplayDiffEvent(void)
{
    if (__ti20RaceDisplayDiffEvent[0] == 0) {
        __tf20RaceDisplayEventBase();
        __rtti_si(__ti20RaceDisplayDiffEvent, "20RaceDisplayDiffEvent",
                  (const type_info *)__ti20RaceDisplayEventBase);
    }
    return *(const type_info *)__ti20RaceDisplayDiffEvent;
}
#endif

/* ======================================================================== */
/* RaceDisplayCheckPointEvent: accessors                                    */
/* ======================================================================== */

/* Inline in game/race/display/T194_race_display_event.h (declared in the reverse of
 * this order); egcs emits these copies at the end of the unit, before
 * the class's typeinfo function (this unit defines the class's key
 * function, pack). */
// FUNC 0x001908D0 RaceDisplayCheckPointEvent::getEventType
// FUNC 0x001908D8 RaceDisplayCheckPointEvent::getCheckPoint
// FUNC 0x001908E0 RaceDisplayCheckPointEvent::setCheckPoint

#ifndef BOOT_OLD_GCC
// FUNC 0x001908E8 __tf26RaceDisplayCheckPointEvent
//   The __si_type_info __ti26RaceDisplayCheckPointEvent, base
//   __ti23RaceDisplayLapTimeEvent.
extern "C" const type_info &__tf26RaceDisplayCheckPointEvent(void)
{
    if (__ti26RaceDisplayCheckPointEvent[0] == 0) {
        __tf23RaceDisplayLapTimeEvent();
        __rtti_si(__ti26RaceDisplayCheckPointEvent, "26RaceDisplayCheckPointEvent",
                  (const type_info *)__ti23RaceDisplayLapTimeEvent);
    }
    return *(const type_info *)__ti26RaceDisplayCheckPointEvent;
}
#endif

/* ======================================================================== */
/* RaceDisplayLapTimeEvent: accessors                                       */
/* ======================================================================== */

/* Inline in game/race/display/T194_race_display_event.h (declared in the reverse of
 * this order); egcs emits these copies at the end of the unit, before
 * the class's typeinfo function (this unit defines the class's key
 * function, pack). */
// FUNC 0x00190938 RaceDisplayLapTimeEvent::getEventType
// FUNC 0x00190940 RaceDisplayLapTimeEvent::getLap
//   No callers.
// FUNC 0x00190948 RaceDisplayLapTimeEvent::setLap
//   No callers.

#ifndef BOOT_OLD_GCC
// FUNC 0x00190950 __tf23RaceDisplayLapTimeEvent
//   The __si_type_info __ti23RaceDisplayLapTimeEvent, base
//   __ti20RaceDisplayEventBase.  Caller: __tf26RaceDisplayCheckPointEvent.
extern "C" const type_info &__tf23RaceDisplayLapTimeEvent(void)
{
    if (__ti23RaceDisplayLapTimeEvent[0] == 0) {
        __tf20RaceDisplayEventBase();
        __rtti_si(__ti23RaceDisplayLapTimeEvent, "23RaceDisplayLapTimeEvent",
                  (const type_info *)__ti20RaceDisplayEventBase);
    }
    return *(const type_info *)__ti23RaceDisplayLapTimeEvent;
}

// FUNC 0x001909A0 __tf20RaceDisplayEventBase
//   The __user_type_info __ti20RaceDisplayEventBase (gp-0x4658).  Callers:
//   the 4 derived classes' typeinfo functions.
extern "C" const type_info &__tf20RaceDisplayEventBase(void)
{
    if (__ti20RaceDisplayEventBase[0] == 0)
        __rtti_user(__ti20RaceDisplayEventBase, "20RaceDisplayEventBase");
    return *(const type_info *)__ti20RaceDisplayEventBase;
}
#endif
