/* dynamics_conductor_internal.h - unit u083 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001ED000..0x001F05F8, second pass.
 *
 * Translation units (link order):
 *   T305  DynamicsConductor: the running order (slots 1 / 2, car_compare),
 *         the lap / distance gap table, the class-inline accessors, the base
 *         answers of several slots, the typeinfo function.
 *         game/race/dynamics/T305_dynamics_conductor.h (the class: its other members are
 *         T290, T303, T304, T306..T308, T317, T324, T352)
 *   T350  the pit lane state machine of a car (car_update_pit) and its
 *         information events.  game/race/dynamics/T350_pit_lane.h
 *   T351  a car snapshot: a RaceCarState copied back over a car, its curves
 *         relinked.  game/race/dynamics/T351_car_snapshot.h
 *   T306  DynamicsConductor::saveSector / restoreSector: a car into / out of
 *         a data logger OneLapGhostInfo (T313)
 *   T307  the lap counter: checkpoints, logger sections, the slot 30 line,
 *         the start / finish line; the per-frame car update; the battles'
 *         checkpoint time report
 *   T359  car_stopped_in_box: "the car stands still in the box" (licenses)
 *   T352  the end of a license test: failure / target time
 *   T308  DynamicsConductor: the lap count (slot 29), lap times, the
 *         battles' checkpoint time table (recordGate), the race clock (slot
 *         13), checkpoint / lap end handlers (slots 15..18), the record checks
 *   T309  DynamicsConductorBattle2P.  game/race/dynamics/T309_dynamics_conductor_battle2p.h
 *   T310  DynamicsConductorBattleMP.  game/race/dynamics/T310_dynamics_conductor_battlemp.h
 * T306, T307, T308, T352 and T359 have no header of their own: their
 * DynamicsConductor members are declared in T305's class.
 *
 * Every class, function and global the unit uses is declared in a canonical
 * header; this header includes them (but T129 / T194, which T308 / T350
 * include themselves: see below) and adds what only this unit uses: the
 * T359 function (called only by T352), the stack record of
 * noseDistance and the named values of the pit lane code.  Offsets in
 * comments are the original (egcs) ones.
 *
 * Times: the race clock (DynamicsConductor::clock) counts 1/3 ms, 50 a frame
 * (60 Hz); "ms" values are clock / 3.  A crossing inside a frame is
 * interpolated: frac = (int)(fraction x 50) is added to the clock.
 */
#ifndef U083_H
#define U083_H

#include "types.h"
#include "libc.h"                           /* memset */
#ifndef BOOT_OLD_GCC                        /* (types.h defines it) */
#include "cxxrt.h"                          /* __rtti_user, __rtti_si (GCC 14's typeinfo functions;
                                               egcs generates them itself, and its <new> conflicts
                                               with cxxrt.h) */
#endif

/* own translation units */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor, CarPairStates, car_effective_lap, car_compare */
#include "game/race/dynamics/T309_dynamics_conductor_battle2p.h"  /* DynamicsConductorBattle2P */
#include "game/race/dynamics/T310_dynamics_conductor_battlemp.h"  /* DynamicsConductorBattleMP */
#include "game/race/dynamics/T350_pit_lane.h"             /* dc_post_pit_in / _out, car_in_pit_lane, car_update_pit */
#include "game/race/dynamics/T351_car_snapshot.h"         /* CarLinks, car_relink_curves, car_copy, car_restore */

/* outside the unit.  None of these writes a type name or a literal
 * (tools/hdrnames.py): the type names are each file's own choice.  Retail's
 * .rodata: T305, T306, T307, T352, T308, T309 and T310 have "bad_alloc" and
 * UnitArena<64>'s name (they read <new> and T153 with
 * GAME_T153_EMITS_TYPEINFO first), T350 also the T194 events' names (it
 * includes T194), T308 RaceCameraBase's (it includes T129); T351 and T359
 * have no .rodata.  So T129_race_base.h and T194_race_display_event.h are
 * not here. */
#include "game/menu/race_menu/T115_race_option.h"         /* RaceOption (RaceOrganization::option.splitBattle) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, RaceResult */
#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID, LapRecord */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getLapDistance, RunwayPos */
#include "game/car/physics/T167_rigid_body.h"           /* CarBoxList (DynamicsConductor::boxes) */
#include "game/race/display/T279_meter_scale.h"        /* CarDynamics (RaceCarState::params: lenFront, the curves) */
#include "game/race/dynamics/T281_battle_rule_presets.h"  /* rules_boost_preset, rules_tire_wear_preset */
#include "game/sound/T288_car_sound.h"            /* DynCarState (u083's CarRunState) */
#include "game/debug/T290_logger_sectors.h"       /* dc_section_value, car_logger_active, car_sector_begin / _end */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_race_done, car_finish */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_best_lap, dc_post_section_time / _lap_time, the time diffs */
#include "game/race/dynamics/T304_slow_car_boost.h"       /* SlowCarBoost */
#include "game/race/dynamics/T313_dc_free_run.h"          /* OneLapGhostInfo */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Recorder (the logger's frames) */
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve (x0: a new tyre's wear) */
#include "game/car/physics/T330_dyn_car_init.h"         /* DynCar_initTyreWear, DynCar_clearFaults, DynCar_resetState */
#include "game/car/physics/T331_dyn_car_message.h"      /* DynCar_setMessage */
#include "game/car/physics/T335_pad_vibration.h"        /* DynWheel (wear, needsService) */
#include "game/car/physics/T341_gear_mode.h"            /* car_mode_driver / car_mode_pit_in / car_mode_pit_box / car_mode_pit_out (ctrlMode) */
#include "game/race/dynamics/T361_drive_params.h"         /* AIDriver (RaceCarState::ai) */
#include "game/race/ai/T364_ai_corner_speed.h"      /* ai_find_corner */
#include "game/race/ai/T367_ai_line.h"              /* ai_init */
#include "game/race/ai/T368_enemy_lines.h"          /* CourseLines (DynamicsConductor::lines) */
#include "game/race/ai/T372_pit_lane.h"             /* pit_lane_contains */
#include "game/car/physics/T373_car_box_list.h"         /* boxes_reset_car */
#include "game/car/physics/T374_car_box.h"              /* car_box_has_hit / _hit_wall / _save / _load / _save_size, CarBoxSave */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox */
#include "game/race/core/T413_race_input.h"           /* RaceInput (rec.rec: the logger's recorder) */

/* ======================================================================== */
/* Named values                                                             */
/* ======================================================================== */

/* DynCarState.ctrlMode / flags: CTRL_* / DYN_FLAG_* (T288); the
 * DynCar_setMessage kinds: CARMSG_* (T331). */

/* Speeds of the pit lane / wrong-way tests, in m/s. */
#define KMH_5               1.3888889f      /* 5 km/h */
#define KMH_3               0.8333334f      /* 3 km/h */
#define KMH_1               0.2777778f      /* 1 km/h */
#define KMH_100             27.777779f      /* 100 km/h */

/* The pit stop's frames (5 s); the tyres are fitted at DynCar_initTyreWear
 * when PIT_TYRE_FRAME are left. */
#define PIT_STOP_FRAMES     300
#define PIT_TYRE_FRAME      0x3C

/* The race clock's ceiling: 0x405F7DFD / 3 ms = 99:59:59.999. */
#define RACE_CLOCK_MAX      0x405F7DFDu

/* RaceBase::virtual_26's id of the player's input (u084 / u085). */
#define INPUT_ID_PLAYER     0x100

/* ======================================================================== */
/* Local to the unit                                                        */
/* ======================================================================== */

/* (? layout) The stack record of DynamicsConductor::noseDistance: the
 * runway query point (axes x, z, -y) and its sector, then the same point in
 * the car's axes, which nothing reads (0x1C bytes in a 0x20-byte slot of a
 * 0x30-byte frame: not a RunwayContact, 0x34). */
struct RunwayPosWorld {
    RunwayPos pos;                          /* 0x00 */
    f32       world[3];                     /* 0x10 x, y, z */
};

/* (? name) The display values of a RaceCarState (T302: viewPos ..
 * lapDistancePrev, +0x158C..+0x16A4) as one record, with the lap counter's
 * two moves of lapDistancePrev.  Not the original form: retail (countLap
 * 0x001EE978) keeps car + 0x158C in a register and reaches lapDistancePrev
 * at +0x114 of it, the code of an inline member of a sub-object; the
 * original most likely had these values as a member object of RaceCarState,
 * which T302 keeps as flat members (u080 / u082 use them).  Same store, same
 * value (countLap 93.2 -> 96.7% with it).  countLap uses it under egcs
 * only; GCC 14 updates the member directly. */
struct CarViewBlock {
    f32 values[0x114 / 4];                  /* 0x000 viewPos .. lagPitchDiffPrev */
    f32 lapDistancePrev;                    /* 0x114 */

    void addLapDistance(f32 d) { lapDistancePrev += d; }
    void subLapDistance(f32 d) { lapDistancePrev -= d; }
};
EGCS_SIZE(CarViewBlock, 0x118);

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define CAR_VIEW_ADD_LAP_DISTANCE(c, d) (((CarViewBlock *)(c)->viewPos)->addLapDistance(d))
#define CAR_VIEW_SUB_LAP_DISTANCE(c, d) (((CarViewBlock *)(c)->viewPos)->subLapDistance(d))
#else
#define CAR_VIEW_ADD_LAP_DISTANCE(c, d) ((c)->lapDistancePrev += (d))
#define CAR_VIEW_SUB_LAP_DISTANCE(c, d) ((c)->lapDistancePrev -= (d))
#endif

/* T359.  1 when all four wheels' contacts are in the stop box (attr2 bit 2)
 * and the car moves at 1 km/h or less; *speed then gets the speed in km/h
 * (0..1).  Caller: licenseCheckStop (T352). */
int  car_stopped_in_box(RaceCarState *car, f32 *speed);     /* 0x001EF488 */

#endif /* U083_H */
