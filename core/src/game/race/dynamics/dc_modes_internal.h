/* dc_modes_internal.h - unit u084 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001F05F8..0x001F2A10, second pass.
 *
 * The race-mode subclasses of DynamicsConductor (the car dynamics / timing
 * conductor of a race, T305): each translation unit is one subclass, its
 * constructor, its virtual overrides and its typeinfo function.
 *
 * Translation units (link order), each with its canonical header:
 *   T311  DynamicsConductorMachineTest   the machine test (0-400 m, 0-1000 m,
 *         the max speed challenge; include/game/race/dynamics/T311_dc_machine_test.h)
 *   T312  DynamicsConductorLicense       the license tests (T312_dc_license.h)
 *   T313  DynamicsConductorFreeRun       the free run with the one-lap ghost;
 *         OneLapGhostInfo (T313_dc_free_run.h)
 *   T314  DynamicsConductorTimeTrial     a one-lap time trial, disqualified on
 *         a fault (T314_dc_time_trial.h)
 *   T315  DynamicsConductorFreePractice  the qualifying practice: the order by
 *         best lap, the difference to the race record, "provisional pole";
 *         the copies of RaceDisplayTimeDiffEvent's getEventType / typeinfo
 *         (T315_dc_free_practice.h)
 *   T316  DynamicsConductorSinglePlayer  the single player races: the default
 *         rules, the start of startAction 3 (T316_dc_single_player.h)
 *
 * Every class is defined in its canonical header; this header adds the
 * outside headers all six sources need and the constants only this unit
 * names.  Each source includes it first, then the headers only it needs
 * (T313: T129 RaceBase; T315 / T316: the T154 difference events), then its
 * own class header, last: egcs writes the type names of a unit's .rodata in
 * reverse definition order, and retail has the unit's own class name first,
 * then (T313) 14RaceCameraBase or (T315 / T316) the three T154 events and
 * 20RaceDisplayEventBase, then t9UnitArena1Ui64.  Each retail file's .rodata
 * opens with <new>'s "bad_alloc" (egcs writes the string of its inline
 * what() where the header is read, in front of everything; nothing
 * references it): each source reads <new> before this header and anchors
 * the string with DATA_START.  egcs's <new> and cxxrt.h conflict, so this
 * header reads cxxrt.h for GCC 14 only (the typeinfo copies).
 *
 * Times: the race clock (DynamicsConductor::clock) counts 1/3 ms, 50 a frame;
 * "ms" values are clock / 3.  RACE_TIME_INVALID (T128) is "no time".
 */
#ifndef U084_H
#define U084_H

/* UnitArena<64>'s type name in every unit (T153 declares its typeinfo
 * function, which stops egcs, unless this is set; only the name is written,
 * no function) */
#define GAME_T153_EMITS_TYPEINFO

#include "types.h"
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si (GCC 14's typeinfo copies) */
#endif

/* outside the unit */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar (sessionBest, order) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, RaceResult; T122 RaceEventQueue, T128 LapRecord */
#include "game/race/dynamics/T281_battle_rule_presets.h"  /* g_002D2988, the default rules */
#include "game/debug/T290_logger_sectors.h"       /* dc_section_count */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_best_lap, dc_post_* */
#include "game/car/physics/T330_dyn_car_init.h"         /* DynCar_clearFaults */
#include "game/car/physics/T341_gear_mode.h"            /* car_mode_driver / car_mode_computer: control modes 0 / 1 */
#include "game/car/physics/T342_car_input.h"            /* CarInput (the controller packet of slot 19) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (bestLap / lap records, rec, enabled, hold) */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* RaceBase::virtual_26's input ids (RaceEntryInformation::inputId, T121) */
enum {
    INPUT_GHOST  = 1,                       /* the second car's input: the free run's ghost */
    INPUT_PLAYER = 0x100                    /* 0x100 + player: player 0's */
};

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* RaceDisplayInformationEvent::information codes posted here: the messages of
 * RaceDisplay::pollInformationEvents (T154) */
enum {
    INFO_PROVISIONAL_POLE = 2,              /* "<maker> is on provisional pole" (with the car's icon) */
    INFO_QUALIFY_ATTACK   = 3               /* "start the qualifying attack" (the player's car) */
};

/* DynCarState.fault bits: FAULT_* (T288). */

/* car_finish (T302) modes used here */
enum {
    FINISH_DISQUALIFIED = 2,                /* (licenseFailed, T352, uses it too) */
    FINISH_RUN_ENDS     = 3
};

/* DynamicsConductorMachineTest::testKind: the rules' lap count of the test
 * course picks the test */
enum {
    MT_TEST_DEFAULT   = 0,                  /* a count of 0 or >= 4: as MT_TEST_MAX_SPEED */
    MT_TEST_400M      = 1,                  /* one line, 400 m: the line time */
    MT_TEST_1000M     = 2,                  /* lines at 400 m and 1000 m: the line times */
    MT_TEST_MAX_SPEED = 3                   /* lines at 400 m, 1000 m, 6845 m: the top speed */
};

/* The seconds of the finish timer of a time trial's end (setFinishTimer) */
#define TT_FINISH_SECONDS   3

/* The longest lap a lap record keeps for the ghost is RaceInput::maxFrames()
 * (T413): 36000 frames = 10 minutes at 60 Hz.  FreeRun's slot 17 compares
 * with the inline (retail: slt with the constant in a register, movz). */

/* RaceOrganization::startAction 3: a start without countdown
 * (RaceOrganization::updateStartAndTimer, T125; RaceBase::virtual_8 sets it
 * for a conductor whose getStartingFormat is 2).  SinglePlayer's slots 12 /
 * 24 handle it. */
#define START_ACTION_NO_COUNTDOWN   3

/* getStartingFormat results (RaceBase::virtual_8, T129: 0 / 3 startAction
 * -1, 1 the race record's, 2 START_ACTION_NO_COUNTDOWN) */
enum {
    START_FORMAT_NO_COUNTDOWN = 2,          /* FreeRun, TimeTrial */
    START_FORMAT_NONE         = 3           /* FreePractice: startAction -1 */
};

#endif /* U084_H */
