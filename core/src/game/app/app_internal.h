/* app_internal.h - unit u001 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00100140..0x001036D0, second pass.
 *
 * Translation units (link order):
 *   T000  main and the top-level game modes (races, demos, replays)
 *   T001  the double packet buffer object g_0028F550
 *   T002  GameObject, the render thread and the boot-screen fade
 *   T003  GameObjectList / GameObjectController (priority list of objects)
 *   T004  GameStatusHolder, the holder of the guest gt2k_game::GameStatus
 *   T005  TransitionObject and its four screen transitions
 *   T006  file_resolve_retry
 *   T009  RaceIOPLoadModules (the IOP module load script)
 *   T007  game_boot / game_init, race_begin / race_end
 *   T008  build switches (constant-returning stubs)
 *   T033  gt2k_menu::FlashItem
 *   T016  the "ar/select_ending" page hooks
 *   T017  gt2k_menu::SmartSkipIndex / SmartCarIndex / SmartCourseIndex
 *
 * Every class is defined in its canonical header (include/game/).  Since the
 * data pass (ref/data/u001/notes.md) each source includes the canonical
 * headers it needs itself, in the order that gives the original's data (the
 * type names egcs writes for the classes a file sees), and defines its own
 * data; this header only has the named constants several of them use.  It
 * must not include headers with classes.
 */
#ifndef U001_H
#define U001_H

#include "types.h"

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */
/* The screen and the menus' virtual screen: 640 x 448. */
enum { SCREEN_W = 640, SCREEN_H = 448 };

/* GameStatus::menu.nextMode: what main does next (jump table 0x003082B0;
 * 0, 10, 14..23 and values >= 27 do nothing). */
enum {
    MAIN_MODE_NONE          = 0,            /* nothing (an explicit case: the table starts at 0) */
    MAIN_MODE_ARCADE        = 1,            /* main_arcade */
    MAIN_MODE_CHAMPIONSHIP  = 2,            /* main_championship(0) */
    MAIN_MODE_GTMODE_RACE   = 3,            /* main_gtmode_race */
    MAIN_MODE_GTMODE_RACE2  = 4,            /* main_gtmode_race2 (the same code) */
    MAIN_MODE_MENU_DEMO     = 5,            /* run_menu_demo_race */
    MAIN_MODE_LICENSE_TEST  = 6,            /* main_license_test */
    MAIN_MODE_MACHINE_TEST  = 7,            /* main_machine_test */
    MAIN_MODE_FREE_RUN      = 8,            /* main_free_run */
    MAIN_MODE_OPTIONS       = 9,            /* F_0013FE58 (T062) */
    MAIN_MODE_CARD_REPLAY   = 11,           /* play_saved_replay(0): a replay of the memory card */
    MAIN_MODE_DEMO_REPLAY   = 12,           /* play_saved_replay(1): a demo replay of the disc */
    MAIN_MODE_LOGGER        = 13,           /* logger_main (T055) */
    MAIN_MODE_FILE_UTILITY  = 24,           /* the memory card file utility */
    MAIN_MODE_GAME_SAVE     = 25,           /* save the game */
    MAIN_MODE_GAME_LOAD     = 26            /* load the game (as the autoload) */
};

/* The work buffer main gives the file utility and the game save. */
enum { MC_WORK_ALIGN = 0x40, MC_WORK_SIZE = 0xD00000 };

/* GameStatus::menu.runMode as T000 sets it before entering a mode (named
 * after those modes; the default after MenuStatus() is 8). */
enum {
    RUN_MODE_TIME_TRIAL   = 0,              /* run_time_trial (arcade) */
    RUN_MODE_SPLIT_BATTLE = 1,              /* run_split_battle (arcade, 2 players) */
    RUN_MODE_GTMODE       = 2,              /* run_gtmode_race / run_championship */
    RUN_MODE_LINK_BATTLE  = 3,              /* the 8-car i.LINK battle (main_arcade) */
    RUN_MODE_LICENSE_TEST = 4,              /* run_license_test */
    RUN_MODE_MACHINE_TEST = 5,              /* run_machine_test */
    RUN_MODE_ARCADE_RACE  = 6,              /* run_arcade_race (arcade single race) */
    RUN_MODE_FREE_RUN     = 7               /* run_free_run (arcade, and main_free_run) */
};

/* main_arcade's states (jump table 0x00308340).  The menu state maps
 * MenuArgs::selection 1..5 (single race, compulsory, time attack, two
 * players, i.LINK) to the five race states (table 0x00308360; any other
 * selection stays in the menu state). */
enum {
    ARCADE_STATE_EXIT         = 0,
    ARCADE_STATE_MENU         = 1,          /* arcade_menu */
    ARCADE_STATE_SINGLE_RACE  = 2,          /* run_arcade_race */
    ARCADE_STATE_TIME_TRIAL   = 3,          /* run_time_trial */
    ARCADE_STATE_FREE_RUN     = 4,          /* run_free_run(args, 0) */
    ARCADE_STATE_SPLIT_BATTLE = 5,          /* run_split_battle(args, 1) */
    ARCADE_STATE_LINK_BATTLE  = 6           /* RaceLinkBattle of the i.LINK nodes */
};
/* The i.LINK battle: up to 8 nodes, a start count of 10 s. */
enum { LINK_BATTLE_SLOTS = 8, LINK_BATTLE_START_COUNT = 10 };

/* RacePS2Base::virtual_41's `replay' (RaceBase::replayMode): RACE_LIVE /
 * RACE_REPLAY / RACE_DEMO, and RaceBase::endCode's RACE_END_DEMO_TIME_UP
 * (T129_race_base.h). */

/* race_begin's control record pool (AutomobileControlRecord::allocBuffers):
 * buffers of 0x20000 bytes, 2 of them (8 for the i.LINK battle). */
enum { CONTROL_RECORD_SIZE = 0x20000, CONTROL_RECORDS = 2 };

/* The race object pool of game_init (race_pool_assign). */
enum { RACE_POOL_SIZE = 0x22000 };

#endif /* U001_H */
