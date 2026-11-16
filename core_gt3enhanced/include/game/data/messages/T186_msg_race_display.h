/* T186_msg_race_display.h - GT2KMSGRACEDISPLAY::Message, the "RACEDISPLAY"
 * real-time text table (56 entries: the words of the race display),
 * translation unit T186, unit u061, 0x001AE880..0x001AE990.  The global
 * instance is g_002C44C8 (T186 .data; keys 0x002C4308, texts 0x002C43E8). */
#ifndef GAME_T186_MSG_RACE_DISPLAY_H
#define GAME_T186_MSG_RACE_DISPLAY_H

#include "game/game.h"
#include "engine/text/T517_realtime_text.h"            /* base GT2KRealTimeText */

namespace GT2KMSGRACEDISPLAY {

/* vtable 0x00331208 (1 slot).  typeinfo __tfQ218GT2KMSGRACEDISPLAY7Message
 * 0x001AE940 (si, base GT2KRealTimeText; storage 0x004B23B0).  No data of
 * its own: size 0x18 (GT2KRealTimeText's vptr +0x14). */
class Message : public GT2KRealTimeText {
public:
    Message();                                  /* 0x001AE880 GT2KRealTimeText("RACEDISPLAY",
                                                   keys, texts, 0x38) */
    virtual ~Message();                         /* [0] +0x0C 0x001AE8D0 */
};

} /* namespace GT2KMSGRACEDISPLAY */

EGCS_SIZE(GT2KMSGRACEDISPLAY::Message, 0x18);

/* The RACEDISPLAY texts (the current language's; the defaults are English),
 * by key (0x002C4308): the indices of g_002C43E8 (the sweep moved u041 /
 * u047 / u048 / u049 / u051 / u052 / u055's local copies here). 
 * The users read single entries at their own addresses: u041 g_002C4478
 * ([36]), g_002C44AC .. g_002C44B8 ([49..52]), u054 g_002C4478[0], u047
 * g_002C43F4 / g_002C43F8 ([3], [4]), u049 g_002C4400 / g_002C4404 ([6],
 * [7]); they are g_002C43E8[i] (one array of T186's .data). */
enum RaceDisplayText {
    RACEDISPLAY_TIME_LIMIT          = 0,    /* TimeLimit "Time Limit" */
    RACEDISPLAY_GAME_OVER           = 1,    /* GameOver "GAME OVER" */
    RACEDISPLAY_WRONG_WAY           = 2,    /* WrongWay "WRONG WAY" */
    RACEDISPLAY_PLEASE_CONNECT      = 3,    /* PleaseConnect: the i.LINK connection error */
    RACEDISPLAY_WAIT_OTHER          = 4,    /* WaitOther: waiting for the other side to answer */
    RACEDISPLAY_AUTO_DRIVE          = 5,    /* AutoDrive "AUTO DRIVE" */
    RACEDISPLAY_EXAM_FAIL           = 6,    /* ExamFail */
    RACEDISPLAY_EXAM_TIME_OVER      = 7,    /* ExamTimeOver */
    RACEDISPLAY_TIME_TRIAL_FAIL     = 8,    /* TimeTrialFail: the time trial's disqualification */
    RACEDISPLAY_PIT_IN              = 9,    /* PitIn "%s has pitted in" */
    RACEDISPLAY_POLE_POSITION       = 10,   /* PolePosition */
    RACEDISPLAY_START_QUALIFY       = 11,   /* StartQualify */
    RACEDISPLAY_READY_QUALIFY       = 12,   /* ReadyQualify */
    RACEDISPLAY_RECORD_END          = 13,   /* RecordEnd: the replay recording finished */
    RACEDISPLAY_ENDURANCE_FINAL     = 14,   /* EnduranceFinal: the final lap */
    RACEDISPLAY_ENDURANCE_LAPS      = 15,   /* EnduranceLaps */
    RACEDISPLAY_FREE_RUN_READY      = 16,   /* FreeRunReady: timing starts at the next control line */
    RACEDISPLAY_FREE_RUN_HELP       = 17,   /* FreeRunHelp: START ends the session */
    RACEDISPLAY_PLAYER_WIN          = 18,   /* PlayerWin "PLAYER %d WIN!" */
    RACEDISPLAY_QUALIFY_MODE        = 19,   /* QualifyMode "QUALIFY" */
    RACEDISPLAY_DEMO_MODE           = 20,   /* DemoMode "DEMONSTRATION" */
    RACEDISPLAY_LIVE_MODE           = 21,   /* LiveMode "LIVE" */
    RACEDISPLAY_REPLAY_MODE         = 22,   /* ReplayMode "REPLAY" */
    RACEDISPLAY_LAP_DIFF_INFO       = 23,   /* LapDiffInfo "%d Laps" */
    RACEDISPLAY_PP_TIME_LABEL       = 24,   /* PPTimeLabel "P.P." */
    RACEDISPLAY_TOTAL_TIME_LABEL    = 25,   /* TotalTimeLabel "Total" */
    RACEDISPLAY_BEST_TIME_LABEL     = 26,   /* BestTimeLabel "Best" */
    RACEDISPLAY_LAST_TIME_LABEL     = 27,   /* LastTimeLabel "Last" */
    RACEDISPLAY_LAP_TIME_LABEL      = 28,   /* LapTimeLabel "Lap" */
    RACEDISPLAY_RECORD_TIME_LABEL   = 29,   /* RecordTimeLabel "Record" */
    RACEDISPLAY_RECORD_LAP_LABEL    = 30,   /* RecordLapLabel "Record Lap" */
    RACEDISPLAY_TIME_LABEL          = 31,   /* TimeLabel "Time" */
    RACEDISPLAY_RECORD_SPEED_LABEL  = 32,   /* RecordSpeedLabel "Record" */
    RACEDISPLAY_SESSION_SPEED_LABEL = 33,   /* SessionSpeedLabel "Best" */
    RACEDISPLAY_FASTEST_TIME_LABEL  = 34,   /* FastestTimeLabel "Fastest" */
    RACEDISPLAY_TRIAL_TIME_LABEL    = 35,   /* TrialTimeLabel "Time" */
    RACEDISPLAY_LAP_COUNT_LABEL     = 36,   /* LapCountLabel "Lap" (0x002C4478) */
    RACEDISPLAY_RANK_1ST            = 37,   /* Rank1st .. Rank12th "1st" .. "12th": 37 + rank - 1 */
    RACEDISPLAY_MAX_SPEED           = 49,   /* MaxSpeed "Max Speed" (0x002C44AC) */
    RACEDISPLAY_NEW_RECORD          = 50,   /* NewRecord "NEW RECORD" */
    RACEDISPLAY_RECORD_TIME         = 51,   /* RecordTime "RECORD TIME" */
    RACEDISPLAY_RECORD_SPEED        = 52,   /* RecordSpeed "RECORD SPEED" */
    RACEDISPLAY_PRIZE_GOLD          = 53,   /* PrizeGold "GOLD" */
    RACEDISPLAY_PRIZE_SILVER        = 54,   /* PrizeSilver */
    RACEDISPLAY_PRIZE_BRONZE        = 55,   /* PrizeBronze */
    RACEDISPLAY_COUNT               = 56
};
extern const char *g_002C43E8[RACEDISPLAY_COUNT];  /* 0x002C43E8 */

/* The table (T186 .data).  Its static destructor / initialiser (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$, 0x001AE8F8 / 0x001AE920, .dtors 0x0028F224 /
 * .ctors 0x0028F144) are compiler-generated. */
extern GT2KMSGRACEDISPLAY::Message g_002C44C8;  /* 0x002C44C8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ218GT2KMSGRACEDISPLAY7Message(void);     /* 0x001AE940 */
#endif

#endif /* GAME_T186_MSG_RACE_DISPLAY_H */
