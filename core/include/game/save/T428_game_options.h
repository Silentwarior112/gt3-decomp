/* T428_game_options.h - GameOptions, the game options of the save data
 * (translation unit T428, unit u097), 0x00218250..0x002185D8.  (? class
 * name, as in u014 / u021; no RTTI, so the namespace is not known either.)
 *
 * GameStatus+0x2FF38, 0x888 bytes; 8 more in MenuStatus+0x600 (T427: the
 * i.LINK battle nodes').  The
 * defaults, copy, load and save.  The options screen (u022, T064) names most
 * words through its item names ("wide_mode", "vib1", "gear_auto",
 * "slider_race_laps", "assist_typeN_on_1p", "assist_asm", ...).  Word 0 lives
 * in g_002F424C while the game runs: apply() copies GameStatus's value there,
 * fetch() copies it back before a save.
 *
 * The same class is declared as gt2k_game::GameOptions by 13 units (u016,
 * u017, u044, ...; T053 in the pass-2 plan): see notes.md of group h07. */
#ifndef GAME_T428_GAME_OPTIONS_H
#define GAME_T428_GAME_OPTIONS_H

#include "game/game.h"
#include "game/save/T430_music_play_list.h"          /* MusicPlayList (by value) */
#include "game/save/T426_controller_config.h"        /* ControllerConfig (by value) */
#include "game/menu/records/T089_split_battle_option.h"      /* SplitBattleOption (by value) */

/* No vtable.  Size 0x888 (getSize 0x638 + ControllerConfig::getSize;
 * MenuStatus's options[8] stride).  8-byte aligned: operator= copies the
 * first 0x638 bytes with ld / sd. */
class GameOptions {
public:
    GameOptions();                              /* 0x00218250 config's constructor, then setDefaults() */
    /* The first 0x638 bytes as one block, config with its operator=, then
     * apply(). */
    GameOptions &operator=(const GameOptions &o); /* 0x00218288 */
    void setDefaults();                         /* 0x00218318 */
    void reset();                               /* 0x00218460 setDefaults(), apply() */
    void setWide(int wide);                     /* 0x00218488 wide = w, apply() */
    static int getSize();                       /* 0x002184A8 0x888 */
    const u8 *load(const u8 *src);              /* 0x002184C8 */
    u8 *save(u8 *dst);                          /* 0x00218518 fetch() first */
    static void apply();                        /* 0x00218568 g_002F424C = GameStatus's wide */
    static void fetch();                        /* 0x002185A0 GameStatus's setWide(g_002F424C) */

    int              wide;                      /* 0x000 default 0 ("wide_mode": the 16:9 screen) */
    int              menuBgm;                   /* 0x004 default 1 */
    int              menuSe;                    /* 0x008 default 1 */
    int              raceBgm;                   /* 0x00C default 1 (u044: used when the race is live) */
    int              raceSe;                    /* 0x010 default 1 */
    int              vibration1p;               /* 0x014 default 1 ("vib1"; -> the race entry +0x20F8) */
    int              vibration2p;               /* 0x018 default 1 ("vib2") */
    int              raceLaps;                  /* 0x01C default 2 (1..100; -> RaceOption+0x0, T115) */
    bool             tireDamage;                /* 0x020 default 1 (-> RaceOption+0x4, an int there; a bool:
                                                   see SplitBattleOption, T089) */
    int              gearAuto;                  /* 0x024 default 1 ("gear_auto"; ^ 1 -> the race entry +0x20E4) */
    int              ghost;                     /* 0x028 default 1 (non-zero: no "ghost_dlg" question) */
    /* The split screen battle options, copied field by field into
     * RaceOption+0x8 (T115).  Defaults: raceLaps 2 (1..100), tireDamage 0, handicap 0
     * (-150..151 by 5 m; > 0 player 2's bar shows it, < 0 player 1's), boost 1
     * (0 none, 1 low, 2 high). */
    SplitBattleOption splitBattle;              /* 0x02C..0x03B */
    int              steeringAssist1p;          /* 0x03C default 0 (0..4 = "assist_type1".."assist_type5"; -> the race entry +0x20EC) */
    int              forceFeedback1p;           /* 0x040 default 1 ("force_feedback"; RaceInput: player 1's wheel force) */
    int              steeringAssist2p;          /* 0x044 default 0 */
    int              forceFeedback2p;           /* 0x048 default 1 */
    int              displayFlags;              /* 0x04C default 0 (the replay display items, bit i shown; ? bit 0 meters, 1 car name, 2 times, 3 map, ...; u052, u061) */
    int              replaySplit;               /* 0x050 default 1 (? 2P replays are shown split screen; u046, u051, u061, u092) */
    int              replayBgm;                 /* 0x054 default 1 */
    int              replaySe;                  /* 0x058 default 1 */
    int              assistAsm;                 /* 0x05C default 1 ("assist_asm"; -> the race entry +0x20F0) */
    int              assistTcs;                 /* 0x060 default 1 ("assist_tcs"; -> the race entry +0x20F4) */
    u8               unk_064[0x6C];             /* 0x064 default 0 */
    int              viewMode;                  /* 0x0D0 default 0 (the replay display mode = the camera kind the views start with, 0..3) */
    int              target[16];                /* 0x0D4 default F_0020EA58(i < 4 ? i : 0)[2]: [viewMode] the camera target (u092: variant) */
    f32              replaySoundLevel;          /* 0x114 default 0.1 (-> CameraManager+0x9630, 0..1: the car sound level of the replay, g_003521E4; camera mode 1 steps it by 0.1) */
    f32              replayCutStyle;            /* 0x118 default 0 (-> CameraManager+0x9634, 0..1: >= 0.5 the other cut style of camera mode 1) */
    int              onboardMount[2];           /* 0x11C default 0: the onboard camera mount of views 0 / 1 */
    u8               unk_124[0x100];            /* 0x124 default 0 */
    /* music: filled by 0x0021ABB0 in setDefaults; GameOptions() does not
     * construct it (0x0021ABB0 is a method, not T430's constructor: see
     * ref/pass2/units/u097/notes.md). */
    u8               unk_224[4];                /* 0x224 never set (? alignment: music is 8-aligned); saved with the first 0x638 bytes */
    MusicPlayList    music;                     /* 0x228 (T430) */
    ControllerConfig config;                    /* 0x638 (T426) */
} __attribute__((aligned(8)));
EGCS_SIZE(GameOptions, 0x888);

#endif /* GAME_T428_GAME_OPTIONS_H */
