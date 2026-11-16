/* T194_race_display_event.h - RaceDisplayEventBase and the race display
 * events (translation unit T194, unit u050), 0x00190198..0x001909E0.
 *
 * A display event is a small object that travels through the race event
 * queue (RaceOrganization +0x84, RaceEventQueue T122) as (type, one 32-bit
 * word): post() queues getEventType() / pack() with a delay of 2 frames,
 * take() removes the first event of the type and unpacks it, peek() reads
 * the nth one and, when the object is bound to a car (car >= 0), accepts it
 * only if it is for that car.  The race display (T154) polls them every
 * frame; the conductors (T303, T315, ...) and the race modes post them.
 *
 * Word layouts (bits 0..3 always the car, clamped to 0..15):
 *   lap time    (type 0xB):  lap 8..17 (0..999)
 *   check point (type 0xC):  check point 4..7 (0..15), lap 8..17
 *   difference  (abstract):  a signed 28-bit value 4..31 (+-0x7FFFFFF);
 *                            setTime / getTime map times above 0xFFFFFFE to
 *                            0xFFFFFFF / RACE_TIME_INVALID (T128)
 *   speed       (type 0x11): speed * 10 in 4..17 (0..9999)
 *   information (type 0x12): an information code 4..7
 * The difference events with a type (0xE general time, 0xF time
 * difference, 0x10 lap difference) are T154's (defined here, inline only:
 * see below).  pack() sets the bit fields
 * of an uninitialised word: the bits it does not set are whatever the
 * register held.
 *
 * Names: u050 (the owner) and the Lupo 2 symbols (getSpeed / setSpeed,
 * getCheckPoint / setCheckPoint); slots 1 / 2 (index: virtual_1 / _2) are
 * pack / unpack. */
#ifndef GAME_T194_RACE_DISPLAY_EVENT_H
#define GAME_T194_RACE_DISPLAY_EVENT_H

#include "game/game.h"

class RaceOrganization;                     /* T125_race_organization.h (pointers only) */
class type_info;

/* The typeinfo functions (vtable entry 0).  egcs generates the derived
 * classes' itself (T194 emits their vtables); an earlier user declaration
 * would make egcs use it and generate nothing (u045), and egcs rejects one
 * after the class.  RaceDisplayEventBase's stays declared for egcs too: with
 * only pure virtual functions it has no key function, and egcs would write
 * its type name into every object that includes this header (retail has it
 * in 6).  GCC 14 has no __tf functions: the build defines them in T194 (and
 * calls them from T154 / T303 / T315). */
#if !defined(BOOT_OLD_GCC) || !defined(GAME_T194_EMITS_TYPEINFO)
/* (T194 itself defines GAME_T194_EMITS_TYPEINFO: egcs generates it there.) */
extern "C" const type_info &__tf20RaceDisplayEventBase(void);   /* 0x001909A0 __user_type_info 0x00353398 */
#endif
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf23RaceDisplayLapTimeEvent(void);       /* 0x00190950 __si_type_info 0x004B2150 */
const type_info &__tf26RaceDisplayCheckPointEvent(void);    /* 0x001908E8 __si_type_info 0x004B21B0 */
const type_info &__tf20RaceDisplayDiffEvent(void);          /* 0x00190880 __si_type_info 0x004B2180 */
const type_info &__tf21RaceDisplaySpeedEvent(void);         /* 0x00190830 __si_type_info 0x004B2160 */
const type_info &__tf27RaceDisplayInformationEvent(void);   /* 0x001907C8 __si_type_info 0x004B21A0 */
/* T154's difference events (defined below; egcs generates theirs) */
const type_info &__tf27RaceDisplayGeneralTimeEvent(void);   /* 0x001956E0 (T303 0x001ECAA8) */
const type_info &__tf23RaceDisplayLapDiffEvent(void);       /* 0x00195738 (T303 0x001ECB00) */
const type_info &__tf24RaceDisplayTimeDiffEvent(void);      /* 0x00195790 (T303 0x001ECB58, T315 0x001F26F8) */
}
#endif

/* vtable 0x0032DE50 (3 slots, all pure), typeinfo __tf20RaceDisplayEventBase
 * 0x001909A0 (user, no bases).  4 bytes of data, vptr +0x4; 8 bytes.  No
 * virtual destructor (the events live on the stack). */
class RaceDisplayEventBase {
public:
    RaceDisplayEventBase();                 /* 0x00190198 car = -1 */
    virtual int  getEventType() const = 0;  /* [0] +0x0C the queue's event type */
    virtual int  pack() const = 0;          /* [1] +0x14 the event word (index: virtual_1) */
    virtual void unpack(int bits) = 0;      /* [2] +0x1C (index: virtual_2) */

    /* Queues (getEventType(), pack()) with a delay of 2 frames
     * (RaceEventQueue F_001796C0). */
    void post(RaceOrganization *org);       /* 0x001901B8 */
    /* Removes the first event of the type and unpacks it: 1, or 0 if there
     * is none.  No callers. */
    int  take(RaceOrganization *org);       /* 0x00190230 */
    /* The nth event of the type, unpacked; when bound to a car (car >= 0)
     * and the event is another car's, car is restored and 0 returned. */
    int  peek(RaceOrganization *org, int nth);  /* 0x001902B0 */

    int car;                                /* 0x0 the car (0..15), -1: any */
    /*  vptr                                   0x4 */
};
EGCS_SIZE(RaceDisplayEventBase, 0x8);

/* vtable 0x0032DE30, typeinfo __tf23RaceDisplayLapTimeEvent 0x00190950 (si,
 * base RaceDisplayEventBase).  Type 0xB.  0xC bytes. */
class RaceDisplayLapTimeEvent : public RaceDisplayEventBase {
public:
    RaceDisplayLapTimeEvent();              /* 0x00190350 lap = 0 */
    /* (The event classes' getEventType and accessors are inline, u050's data
     * pass: retail's T194 has their copies at the end, each class's before
     * its __tf, in the reverse of their declaration order; T194 has the key
     * functions.) */
    void setLap(int v) { lap = v; }         /* 0x00190948 no callers */
    int  getLap() { return lap; }           /* 0x00190940 no callers */
    virtual int  getEventType() const { return 0xB; }   /* [0] +0x0C 0x00190938 RACE_EVENT_LAP_TIME */
    virtual int  pack() const;              /* [1] +0x14 0x00190388 car, lap 8..17 */
    virtual void unpack(int bits);          /* [2] +0x1C 0x001903F0 */

    int lap;                                /* 0x8 0..999 */
};
EGCS_SIZE(RaceDisplayLapTimeEvent, 0xC);

/* vtable 0x0032DE10, typeinfo __tf26RaceDisplayCheckPointEvent 0x001908E8
 * (si, base RaceDisplayLapTimeEvent).  Type 0xC.  0x10 bytes. */
class RaceDisplayCheckPointEvent : public RaceDisplayLapTimeEvent {
public:
    RaceDisplayCheckPointEvent();           /* 0x00190408 checkPoint = 0 */
    void setCheckPoint(int v) { checkPoint = v; }           /* 0x001908E0 */
    int  getCheckPoint() const { return checkPoint; }       /* 0x001908D8 */
    virtual int  getEventType() const { return 0xC; }       /* [0] +0x0C 0x001908D0 RACE_EVENT_CHECK_POINT */
    virtual int  pack() const;              /* [1] +0x14 0x00190440 car, check point 4..7, lap 8..17 */
    virtual void unpack(int bits);          /* [2] +0x1C 0x001904D8 */

    int checkPoint;                         /* 0xC 0..15 (the section of the lap) */
};
EGCS_SIZE(RaceDisplayCheckPointEvent, 0x10);

/* vtable 0x0032DDF0 (slot 0 pure), typeinfo __tf20RaceDisplayDiffEvent
 * 0x00190880 (si, base RaceDisplayEventBase).  The base of T154's
 * RaceDisplayGeneralTimeEvent (0xE), TimeDiffEvent (0xF) and LapDiffEvent
 * (0x10), whose vtables keep these slots 1 / 2.  0xC bytes. */
class RaceDisplayDiffEvent : public RaceDisplayEventBase {
public:
    RaceDisplayDiffEvent();                 /* 0x00190500 diff = 0 */
    virtual int  pack() const;              /* [1] +0x14 0x00190538 car, diff clamped to +-0x7FFFFFF in 4..31 */
    virtual void unpack(int bits);          /* [2] +0x1C 0x001905B0 diff sign-extended */
    void setTime(u32 t);                    /* 0x001905C8 0xFFFFFFF above 0xFFFFFFE (unsigned) */
    u32  getTime();                         /* 0x001905E8 RACE_TIME_INVALID above 0xFFFFFFE (unsigned) */

    int diff;                               /* 0x8 a time (ms) or a difference (ms, laps) */
};
EGCS_SIZE(RaceDisplayDiffEvent, 0xC);

/* ======================================================================== */
/* T154's difference events (types 0xF, 0x10, 0xE)                          */
/* ======================================================================== */

/* Defined here, after RaceDisplayDiffEvent and before the speed event: the
 * original's display event header held them (egcs writes the type names of
 * the classes without a key function in the reverse of their definition
 * order: retail's T194 .rodata has Information, Speed, GeneralTime,
 * LapDiff, TimeDiff, DiffEvent, CheckPoint, LapTime, EventBase; T154's
 * GeneralTime, LapDiff, TimeDiff, EventBase after RaceDisplay's; T303 /
 * T315 / T316 the same four).  Inline only: their vtables (slots 1 / 2
 * RaceDisplayDiffEvent's), getEventType and typeinfo functions are emitted
 * in every unit that builds one (T154, T303, T315; the code stores T154's
 * vtables), in the reverse of this order.  0xC bytes each.  (Before the
 * T154 merge: T154_race_display_events.h, u082 / u084.) */

/* vtable 0x0032E1B8 (copies 0x0033C938 T303, 0x0033D840 T315), typeinfo
 * __tf24RaceDisplayTimeDiffEvent 0x00195790 (si, base RaceDisplayDiffEvent).
 * A difference in ms (to the reference lap, the car ahead). */
class RaceDisplayTimeDiffEvent : public RaceDisplayDiffEvent {
public:
    RaceDisplayTimeDiffEvent() {}
    virtual int getEventType() const { return 0xF; }    /* [0] +0x0C 0x00195788 (T303 0x001ECB50,
                                                           T315 0x001F26F0) */
};
EGCS_SIZE(RaceDisplayTimeDiffEvent, 0xC);

/* vtable 0x0032E198 (copy 0x0033C918, T303), typeinfo
 * __tf23RaceDisplayLapDiffEvent 0x00195738 (si, base RaceDisplayDiffEvent).
 * A difference in laps. */
class RaceDisplayLapDiffEvent : public RaceDisplayDiffEvent {
public:
    RaceDisplayLapDiffEvent() {}
    virtual int getEventType() const { return 0x10; }   /* [0] +0x0C 0x00195730 (T303 0x001ECAF8) */
};
EGCS_SIZE(RaceDisplayLapDiffEvent, 0xC);

/* vtable 0x0032E178 (copy 0x0033C8F8, T303), typeinfo
 * __tf27RaceDisplayGeneralTimeEvent 0x001956E0 (si, base
 * RaceDisplayDiffEvent).  A section / lap time (diff: the time, setTime). */
class RaceDisplayGeneralTimeEvent : public RaceDisplayDiffEvent {
public:
    RaceDisplayGeneralTimeEvent() {}
    virtual int getEventType() const { return 0xE; }    /* [0] +0x0C 0x001956D8 (T303 0x001ECAA0) */
};
EGCS_SIZE(RaceDisplayGeneralTimeEvent, 0xC);

/* vtable 0x0032DDD0, typeinfo __tf21RaceDisplaySpeedEvent 0x00190830 (si,
 * base RaceDisplayEventBase).  Type 0x11.  0xC bytes. */
class RaceDisplaySpeedEvent : public RaceDisplayEventBase {
public:
    RaceDisplaySpeedEvent();                /* 0x00190608 speed = 0 */
    void  setSpeed(float v) { speed = v; }                  /* 0x00190828 */
    float getSpeed() const { return speed; }                /* 0x00190820 */
    virtual int  getEventType() const { return 0x11; }      /* [0] +0x0C 0x00190818 RACE_EVENT_SPEED */
    virtual int  pack() const;              /* [1] +0x14 0x00190640 car, (int)(speed * 10) clamped to 0..9999 */
    virtual void unpack(int bits);          /* [2] +0x1C 0x001906C0 the field times 0.1 */

    f32 speed;                              /* 0x8 0..999.9 in 0.1 steps (the speed unit's) */
};
EGCS_SIZE(RaceDisplaySpeedEvent, 0xC);

/* vtable 0x0032DDB0, typeinfo __tf27RaceDisplayInformationEvent 0x001907C8
 * (si, base RaceDisplayEventBase).  Type 0x12.  0xC bytes. */
class RaceDisplayInformationEvent : public RaceDisplayEventBase {
public:
    RaceDisplayInformationEvent();          /* 0x00190710 information = 0 */
    void setInformation(int v) { information = v; }         /* 0x001907C0 no callers */
    int  getInformation() { return information; }          /* 0x001907B8 no callers */
    virtual int  getEventType() const { return 0x12; }      /* [0] +0x0C 0x001907B0 RACE_EVENT_INFORMATION */
    virtual int  pack() const;              /* [1] +0x14 0x00190748 car, the low 4 bits of information (not
                                               clamped) */
    virtual void unpack(int bits);          /* [2] +0x1C 0x00190798 */

    int information;                        /* 0x8 0..15 (u084: 2 a new record, 3 the start; u052: 1..6) */
};
EGCS_SIZE(RaceDisplayInformationEvent, 0xC);

/* RaceDisplayInformationEvent::information (type 0x12): the RACEDISPLAY
 * text keys they show (RaceDisplay::pollInformationEvents; u052, moved here
 * from race_display_internal.h for the enhanced engine's posters).  The
 * conductors keep names of their own for 2 / 3 (dc_modes_internal.h). */
enum {
    RD_INFO_PIT_IN         = 1,             /* "<maker> has pitted in" (u083 dc_post_pit_in) */
    RD_INFO_POLE_POSITION  = 2,             /* "<maker> is on provisional pole", with its icon */
    RD_INFO_START_QUALIFY  = 3,             /* "start the qualifying attack" */
    RD_INFO_READY_QUALIFY  = 4,             /* "attack after one warm-up lap" (virtual_0: ReadyQualify) */
    RD_INFO_ENDURANCE_FINAL = 5,            /* "final lap" (the player's car only) */
    RD_INFO_START          = 6,             /* put_start_display and SE_COUNT_S */
    /* The enhanced engine (ref/enhanced/INTERFACES.md): appended after
     * GT3's last code, the number Lupo 2 gives it too.  The player's Pod
     * sat down in the stop area: "it sat down!" in blue in the warning slot
     * for 2 s with the start beep (hud.md C18, RACEDISPLAY 57; Lupo 2's
     * pollInformationEvents L2 0x00191E00 case 7).  Posted by the Pod
     * stop-area rule (L2 0x001F7708..0x001F7D98). */
    RD_INFO_POD_SAT_DOWN   = 7
};

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
/* The type_info nodes (u050 / u052 / u082 / u084 g_<address>): a
 * __user_type_info {vptr, name}, a __si_type_info {vptr, name, base} (the
 * nodes are 16 bytes apart in .bss; the plan had also put
 * __ti20RaceDisplayDiffEvent, the base of T154's time / lap difference
 * events, in T151). */
extern "C" {
extern u32 __ti20RaceDisplayEventBase[2];           /* 0x00353398 gp-0x4658 __user_type_info */
extern u32 __ti23RaceDisplayLapTimeEvent[3];        /* 0x004B2150 */
extern u32 __ti21RaceDisplaySpeedEvent[3];          /* 0x004B2160 */
extern u32 __ti20RaceDisplayDiffEvent[3];           /* 0x004B2180 */
extern u32 __ti27RaceDisplayInformationEvent[3];    /* 0x004B21A0 */
extern u32 __ti26RaceDisplayCheckPointEvent[3];     /* 0x004B21B0 */
/* T154's difference events' (base __ti20RaceDisplayDiffEvent) */
extern u32 __ti24RaceDisplayTimeDiffEvent[3];       /* 0x004B2170 */
extern u32 __ti27RaceDisplayGeneralTimeEvent[3];    /* 0x004B2190 */
extern u32 __ti23RaceDisplayLapDiffEvent[3];        /* 0x004B21C0 */
}
#endif

#endif /* GAME_T194_RACE_DISPLAY_EVENT_H */
