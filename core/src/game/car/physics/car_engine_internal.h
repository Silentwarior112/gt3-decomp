/* car_engine_internal.h - unit u078 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001DE5E0..0x001E1B28.
 *
 * The per-car "feel" code of the race core: everything here works on one car
 * of the DynamicsConductor (RaceCarState, T302), mostly on its run state
 * DynCarState (RaceCarState +0xF84, T288).
 *
 * Translation units (link order), each with its canonical header:
 *   T333  the tyre squeal / smoke curves and their per-wheel evaluation.
 *         game/sound/T333_tyre_squeal.h
 *   T334  the tyre smoke: one SmokeSource per wheel to the course.
 *         game/effects/T334_tyre_smoke.h
 *   T335  the pad's big vibration motor; DynWheel.
 *         game/car/physics/T335_pad_vibration.h
 *   T288  the car sound parameters g_0049CFD8 and their evaluators, the
 *         crash sound; DynCarState, PadVibration, TyreForce.
 *         game/sound/T288_car_sound.h
 *   T336  the steering force (force-feedback wheel) and the pad send.
 *         game/car/physics/T336_steer_force.h
 *   T289  the display level of a car (car_display_level: ghost cars) and the sound
 *         reset it guards.  game/car/physics/T289_car_mode.h
 *   T337  the lagged yaw / pitch.  game/car/physics/T337_attitude_lag.h
 *   T290  DynamicsConductor::virtual_7 and the data logger's sector
 *         records.  game/debug/T290_logger_sectors.h
 *   T291  CarEngine: torque curve, turbo stages, power limit, spec sheet.
 *         game/car/physics/T291_engine.h
 *   T338  the engine output per frame.  game/car/physics/T338_engine_output.h
 *
 * Every shared class, function and global is declared in a canonical
 * header; this header includes them and keeps what only this unit uses: the
 * externs of T336 / T337's .sdata (defined in those files), engine_output's
 * prototype and names for the numbers the code tests.
 * Type names (u078's data pass): none of the headers below writes one (T153
 * suppresses UnitArena<64>'s by default).  The files whose retail .rodata
 * has "bad_alloc" / UnitArena<64>'s name / the race displays' names include
 * <new>, T153 with GAME_T153_EMITS_TYPEINFO or T154 themselves, before this
 * header (each file's comment lists its retail data).
 * Offsets in comments are the original (egcs) ones.
 */
#ifndef U078_H
#define U078_H

#include "types.h"
#include <stddef.h>

/* own translation units */
#include "game/sound/T333_tyre_squeal.h"              /* TyreCurveSet g_0049C990 / g_0049CBA0 / g_0049CDB0, g_0049CFC0,
                                                   tyre_curve*, tyre_sound_* */
#include "game/effects/T334_tyre_smoke.h"               /* SmokeSource, F_001DF148, tyre_smoke_emit */
#include "game/car/physics/T335_pad_vibration.h"            /* DynWheel, pad_vibration_update */
#include "game/sound/T288_car_sound.h"                /* DynCarState, PadVibration, CarSoundParams g_0049CFD8, car_sound_*,
                                                   car_crash_sound_*, car_report_crash */
#include "game/car/physics/T336_steer_force.h"              /* steer_force_init / update, pad_vibration_send */
#include "game/car/physics/T289_car_mode.h"                 /* car_display_level, car_silence_hidden, CAR_SHOW_* */
#include "game/car/physics/T337_attitude_lag.h"             /* car_attitude_lag_reset / update */
#include "game/debug/T290_logger_sectors.h"           /* dc_section_*, car_logger_active, car_sector_*, dc_sector_restart,
                                                   dc_logger_start */
#include "game/car/physics/T291_engine.h"                   /* CarEngine, EngineSpecSheet, engine_*, turbo_* */
#include "game/car/physics/T338_engine_output.h"            /* engine_update */

/* outside the unit */
#include "game/car/physics/T302_car_state.h"                /* RaceCarState (CarDynamics params, DynCarState st, contact[4]) */
#include "game/race/display/T279_meter_scale.h"              /* CarDynamics (params.engine, gear, susp, sprungWeight, unk_E5C) */
#include "game/race/dynamics/T305_dynamics_conductor.h"       /* DynamicsConductor (getCar, getCourseKind, saveSector,
                                                   restoreSector, isDataLoggerSupported, issueCrashEvent) */
#include "game/race/dynamics/T313_dc_free_run.h"              /* OneLapGhostInfo, ghost_restart */
#include "game/race/core/T413_race_input.h"               /* RaceInput (u078's CarDynamics: rec, snapshots, hold) */
#include "game/race/dynamics/T318_control_record.h"           /* AutomobileControlRecord::Recorder::skip (u078's CarDynamicsCC) */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization (race, loggerMode, eventQueue,
                                                   getLoggerSectorCount / getLoggerSectorPosition) */
#include "game/race/core/T122_race_event_queue.h"         /* RaceEventQueue::post */
/* (RaceSolitaire::showGhost, T139_race_solitaire.h: T289 includes it itself,
 * after the race display header; it brings the race display classes' type
 * names, which only retail's T289 has.) */
#include "game/race/core/T153_race_course.h"              /* RaceCourse::emitSmoke, RunwayContact */
#include "game/car/physics/T280_car_condition.h"            /* CarCondition (addDirt, the constructor) */
#include "game/car/physics/T283_condition_model.h"          /* ConditionModel g_002D2B00 (u078's ConditionTable) */
#include "game/car/physics/T325_misc.h"                     /* DynParams g_002D28B0, the g_002D28B8 macro */
#include "game/common/T319_angle_math.h"               /* wrapAngle, angleDiff */
#include "game/common/T320_record_set.h"               /* Curve, curveSet */
#include "game/common/T321_curve_lookup.h"             /* curveLookup */
#include "game/common/T322_curve_linear.h"             /* curveLookupLinear */
#include "game/car/physics/T296_car_drivetrain.h"           /* car_driven_axle_speed */
#include "game/car/physics/T339_misc.h"                     /* InputTuning g_002D4628 (u078's SteerForceSource) */
#include "game/common/T354_response_curve.h"           /* ResponseCurve, responseInit, responseEval (u078's SteerCurve) */
#include "game/car/physics/T355_setting_check.h"            /* TyreSoundTable g_002D35B8, TyreSoundRow, CarSoundSource,
                                                   TyreSoundTable_validate */
#include "game/data/T482_car_equipments.h"           /* CarSpec (u078's EngineSpec: +0x6C..+0xAF) */
#include "engine/media/G002_mpeg_frameq.h"              /* cosf, sinf */
#include "runtime/newlib/G007_stdio.h"                    /* memset */

/* ======================================================================== */
/* The data of T336 / T337 (only this unit reads or writes them)            */
/* ======================================================================== */

/* (T336's .bss g_0049D050, the steering force's response curve, is a file
 * static of T336_steer_force.cpp.) */
/* T336 (.sdata): the gains steer_force_init sets from g_002D4628. */
extern f32 g_00352934;                      /* gp-0x50BC (=0) the front tyres' lateral term (lateralGain x 0.1) */
extern f32 g_00352938;                      /* gp-0x50B8 (=1.0) the front lateral forces (forceGain x 0.01) */
extern f32 g_0035293C;                      /* gp-0x50B4 (=0) the steering rate (rateGain x 0.01 / 4096) */
extern f32 g_00352940;                      /* gp-0x50B0 (=0) the steering angle term (InputTuning steerAngleGain), per rad */
extern f32 g_00352944;                      /* gp-0x50AC (=0) its clamp, rad (steerAngleLimit) */
extern f32 g_00352948;                      /* gp-0x50A8 (=0) the front slip angle term, per rad */
extern f32 g_0035294C;                      /* gp-0x50A4 (=0) the slip angle clamp, rad */
/* T337 (.sdata). */
extern f32 g_00352958;                      /* gp-0x5098 (=30.0) the lagged yaw's spring */
extern f32 g_0035295C;                      /* gp-0x5094 (=100.0) unreferenced */
extern f32 g_00352960;                      /* gp-0x5090 (=10.0) the lagged yaw's damping */
extern f32 g_00352964;                      /* gp-0x508C (=20.0) unreferenced */

/* T338: the rev limiter and the power limit on the torque of `load'; only
 * engine_update calls it (T338_engine_output.h leaves it out). */
f32 engine_output(RaceCarState *car, f32 load);     /* 0x001E1768 */

/* ======================================================================== */
/* The logger's sector records                                              */
/* ======================================================================== */

/* The sector records of a car are its RaceInput's snapshots (+0x12C), read
 * through RaceInput::getSnapshot / getFrames / getRecordMode / skip (T413's
 * inline accessors: the register choices of T290 match only with them). */

/* AutomobileControlRecord::Manager::mode (T318) of the RaceInput's record. */
enum {
    REC_MODE_ONE_SLOT = 2                   /* one slot: the sector starts count from sector 0's */
};

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* ======================================================================== */
/* Names for the numbers the code tests                                     */
/* ======================================================================== */

/* DynCarState.driver / ctrlMode: DRIVER_* / CTRL_* (T288). */

/* DynWheel.surface (ground_surface_type, T347) as this unit reads it.  1 and
 * 13 (1 with the contact's remap bit) are the kerbs (?): the squeal there is
 * a rumble over the tread speed, and the front wheels on surface 1 shake
 * the pad by load x speed. */
enum {
    SURFACE_KERB          = 1,
    SURFACE_KERB_REMAPPED = 13
};

/* The tyre sound tables' surface classes (TyreSoundTable rows, T355):
 * surfaces 2..5 are class 1, 6 and 12..14 class 2, the others class 0.  The
 * smoke of classes 1 and 2 adds to the car's dirt. */
enum {
    TYRE_CLASS_ROAD   = 0,
    TYRE_CLASS_LOOSE  = 1,                  /* surfaces 2..5 */
    TYRE_CLASS_OTHER  = 2                   /* surfaces 6, 12, 13, 14 */
};

/* The curve sets of a surface class: the TyreSoundTable rows (the built
 * sets are g_0049C990 / g_0049CBA0 / g_0049CDB0). */
enum {
    TYRE_SET_VOLUME = 0,
    TYRE_SET_PITCH  = 1,
    TYRE_SET_SMOKE  = 2
};

/* car_crash_sound_add's kind: CRASH_KIND_* (T288). */

/* InputTuning.filter (T339): steer_force_update's filter of the force. */
enum {
    STEER_FILTER_NONE      = 0,
    STEER_FILTER_AVERAGE   = 1,             /* with the last frame's */
    STEER_FILTER_SMALLER   = 2,             /* the smaller of this and the last on the same side */
    STEER_FILTER_PEAK_HOLD = 3              /* a peak hold, decaying after 7 frames */
};

/* CarEngine.turboType (T291): the boost below the full-boost rpm. */
enum {
    TURBO_CURVE_FALLING = 0,                /* boost - K (rpm - turboRpm)^2 */
    TURBO_CURVE_SQUARE  = 1,                /* K rpm^2 */
    TURBO_CURVE_LINEAR  = 2                 /* boost x rpm / turboRpm */
};

/* DynCarState.blowOffState (engine_update): the turbo's blow-off (?). */
enum {
    BLOWOFF_READY = 0,                      /* armed: a lift below 0.75 throttle fires it */
    BLOWOFF_FIRED = 1                       /* until the throttle is above 0.9 again */
};

#endif /* U078_H */
