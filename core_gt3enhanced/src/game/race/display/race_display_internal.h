/* race_display_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, translation
 * unit T154 (include/game/race/display/T154_race_display.h), 0x001909E0..0x00195B88: work
 * units u051 (0x001909E0..0x00193040) and u052 (0x00193040..0x00195B88, with
 * T155 after it), one object as in the original (src/game/race/display/T154_race_display.cpp).
 *
 *   T154  RaceDisplay, the head-up display of a race: its constructor and
 *         destructor, the shared resources (race/<country>/display.imgs and
 *         font/menu.fnt, reference counted), virtual_0 (the set-up for a
 *         race), virtual_2 (the per-frame update), the display mode / car /
 *         meter helpers, the strip helpers of the display, the countdown /
 *         time limit texts, the car warning, the event pollers the races call
 *         every frame; the display event pollers (lap / check point / lap and
 *         time difference / speed / information / replay view mode), the draw
 *         (virtual_3), the finish, start and lap time messages, the layout
 *         loader (the race mode's racedetail.db records of g_002F00C0 -> the
 *         display objects), the two meter layouts g_002BC080 / g_002BC120 and
 *         their static initializer, and the out-of-line copies of the inline
 *         members (the T154 difference events, RaceDisplay's accessors,
 *         RaceMeterBase, RaceMiniMap, RaceOnboardSpeedmeter) with the
 *         typeinfo functions.
 *
 * Every class is defined in its canonical header (include/game/); the source
 * includes the headers it needs itself (in the order that gives retail's
 * type names, see its include list); this header keeps the named constants
 * for the race events, sound effects, texts, colours and layout records the
 * code uses.
 */
#ifndef U051_H
#define U051_H

#include "types.h"
#include "game/enhanced.h"                  /* ENH_HUD_* (ref/enhanced/survey/hud.md), ENH_RACE_TIME_UP_ENDS */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */
/* RACE_EVENT_BGM's code at a car's finish (? the goal music). */
enum { BGM_CODE_FINISH = 0x13 };

/* RaceDisplay::replay (virtual_0's argument). */
enum {
    RD_LIVE       = 0,                      /* a race */
    RD_REPLAY     = 1,                      /* a replay (also 3) */
    RD_DEMO       = 2,                      /* a demo */
    RD_REPLAY_ALT = 3                       /* ? another replay mode, shown as RD_REPLAY */
};

/* The two split screen battle modes (race mode ids 7 / 8: g_002F424C picks
 * 8, side by side).  (u32)(mode - RD_MODE_SPLIT_A) < 2 tests for them. */
enum {
    RD_MODE_SPLIT_A = 7,                    /* one above the other: 448 / 2 lines high */
    RD_MODE_SPLIT_B = 8                     /* side by side (16:9): 853 / 2 wide */
};
/* ? which mode: the race mode whose display shows every item (virtual_3). */
enum { RD_RACE_MODE_SHOW_ALL = 0xF };

/* The sound effects race_se_play (T176) plays: T469's SE_COUNT_D (each
 * second of the last 3 of the countdown) and SE_COUNT_S (the start). */

/* RACEDISPLAY texts: g_002C43E8[RACEDISPLAY_*] (T186). */

/* RaceDisplayInformationEvent::information (type 0x12), RD_INFO_*: in
 * game/race/display/T194_race_display_event.h (the enhanced engine moved
 * them there, with the Pod's appended code). */

/* DynamicsConductor::getStartingFormat (slot 8): the base returns
 * rules.launchSpeed == 0 (0x001F2B80), the free run / time trial
 * conductors 2, free practice 3. */
enum {
    START_FORMAT_ROLLING       = 0,         /* a rolling start (launchSpeed != 0) */
    START_FORMAT_STANDING      = 1,         /* a standing start */
    START_FORMAT_FREE_RUN      = 2,         /* DynamicsConductorFreeRun / TimeTrial */
    START_FORMAT_FREE_PRACTICE = 3          /* DynamicsConductorFreePractice */
};

/* The display items, RD_SHOW_*: in game/race/display/T154_race_display.h
 * (the enhanced engine moved them there: race/core's HUD toggle sets
 * RaceDisplay.showMask). */

/* RaceDetailRecord::type: the display object a layout record places
 * (RaceDisplay::setLayout). */
enum {
    RD_ITEM_NONE          = 0,              /* nothing (a case of its own in setLayout's switch) */
    RD_ITEM_RANK          = 1,              /* width / 100: the character scale */
    RD_ITEM_LAP           = 2,
    RD_ITEM_METERS        = 3,              /* subType < 0x40: layoutMeters(subType), else the back panel */
    RD_ITEM_BOOST         = 4,
    RD_ITEM_TIRE_WEAR     = 5,              /* when the car's tyres wear (hasTireWear) */
    RD_ITEM_WARNING       = 6,
    RD_ITEM_EVENTS        = 7,
    RD_ITEM_RECORD_TIME   = 8,              /* recordTimes[subType] */
    RD_ITEM_MAP           = 9,              /* width / 100, height / 100: the scales */
    RD_ITEM_MIRROR        = 10,
    RD_ITEM_TIME          = 11,             /* layoutTimeDisplay: subType picks the display */
    RD_ITEM_STEERING      = 12,
    RD_ITEM_AB_MONITOR    = 13,             /* the enhanced engine: subType 0 the accel / brake monitor,
                                               1 the battery gauge (Lupo 2; GT3's one subType 1 record is
                                               in no mode) */
    RD_ITEM_ONBOARD_TACHO = 14,             /* made activeTacho */
    RD_ITEM_REMOTE_TACHO  = 15,
    RD_ITEM_BATTLE_TACHO  = 16,
    RD_ITEM_SPEEDOMETER   = 17,
    RD_ITEM_DIGITAL_SPEED = 18,             /* width, height: the unit's offset */
    RD_ITEM_GEAR          = 19,             /* subType bit 0: the shift lamp's offset */
    RD_ITEM_ODOMETER      = 20,
    RD_ITEM_TRIP_METER    = 21,
    RD_ITEM_INDICATORS    = 22,
    RD_ITEM_CAR_NAME      = 23,
    RD_ITEM_REPLAY_MODE   = 24,
    RD_ITEM_EVENTS2       = 25,
    RD_ITEM_TITLE         = 26              /* subType 0: the title, else the mode message */
};
/* RaceDetailRecord::mode of a record shown in every layout variant. */
enum { RD_VARIANT_ALL = 0x7F };

/* layoutMeters' kinds (setLayout's RD_ITEM_METERS subType) and the back
 * panels they use (RaceBackPanel::setPanel: below 0x40 an image). */
enum {
    RD_METERS_ONBOARD = 0,                  /* the onboard tachometer, layout g_002BC080 */
    RD_METERS_REMOTE  = 1,                  /* the remote tachometer, layout g_002BC120 */
    RD_PANEL_IMAGES   = 0x40,               /* subTypes from here are back panels */
    RD_BACK_FRAME     = 0x40,               /* a frame 3 wide */
    RD_BACK_SHAPE     = 0x41                /* a filled shape (corners cut) */
};

/* Colours (0xAABBGGRR, alpha 0x80 = opaque). */
#if ENH_HUD_TEXT_LOOK
#define RD_COLOR_TEXT    0x80C0C0C0u        /* Lupo 2's grey (hud.md C12; L2 0x0018F43C, 0x00192520) */
#else
#define RD_COLOR_TEXT    0x80D0D0D0u        /* the title, the car name, "@start", "@finish" */
#endif
#define RD_COLOR_WARNING 0x800000F0u        /* red: "WRONG WAY" */
#define RD_COLOR_MESSAGE 0x801880D0u        /* amber: "AUTO DRIVE" (RaceValueDisplayBase's default too) */
#define RD_COLOR_WIN     0x801880D0u        /* amber: "PLAYER %d WIN!" */
#define RD_COLOR_EVENT   0x802BCC49u        /* the event board's messages (RaceEventDisplay's default) */
#define RD_COLOR_BEHIND  0x800000F0u        /* red: a difference >= 0 */
#define RD_COLOR_AHEAD   0x80FF8080u        /* blue: a difference < 0 */
#define RD_COLOR_OPAQUE  0x80FFFFFFu        /* pgl_color: opaque white */
/* The enhanced engine (hud.md C16, C18). */
#define RD_COLOR_TIME_UP 0x800000F0u        /* red: the time limit below 10 s, "GAME OVER" (Lupo 2) */
#define RD_COLOR_POD     0x80FF8080u        /* blue: "the Pod sat down" (L2 0x00191FD4) */

/* The enhanced engine: the time limit's offset (hud.md C16).  Lupo 2 shows
 * the limit 3 s low and "GAME OVER" when 3 s are left; it goes with the
 * race ending at the time limit (race.h ENH_RACE_TIME_UP_ENDS). */
#if ENH_HUD_TIME_LIMIT_LOOK && ENH_RACE_TIME_UP_ENDS
#define RD_TIME_LIMIT_OFFSET 3
#else
#define RD_TIME_LIMIT_OFFSET 0
#endif

/* The blinks of the mode message under the course name:
 * RaceMessageDisplay::setBlink(RD_MODE_MESSAGE_BLINKS), 999 blinks of 24
 * frames (blink 0x5DA8), i.e. for the whole intro. */
enum { RD_MODE_MESSAGE_BLINKS = 999 };

/* The event board's next-message offset of a difference (setOffset). */
enum {
    RD_DIFF_OFFSET_X   = 0x25,              /* +-37 with an icon */
    RD_DIFF_OFFSET_Y   = 0x19,              /* 25 */
    RD_DIFF_ICON_DX    = 0x3E,              /* the icon, +-62 from the board */
    RD_DIFF_ICON_DY    = 0x14               /* 20 lower */
};
/* A car icon's blink (RaceCarIconDisplay::setBlink(n): n blinks of 24
 * frames). */
enum { RD_BLINK_FRAMES = 24 };

/* The character of the font for a minus sign (the difference texts). */
enum { RD_CHAR_MINUS = 0x7F };

#endif /* U051_H */
