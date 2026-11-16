/* car_motion_internal.h - unit u082 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001E9E80..0x001ED000.
 *
 * The per-car dynamics step of the race core and the conductor's car-to-car
 * passes.  Everything works on one car of the DynamicsConductor
 * (RaceCarState, 0x16DC bytes, DynamicsConductor::getCar) and mostly on its
 * run state DynCarState at +0xF84.
 *
 * Translation units (link order), each with its canonical header:
 *   T300  the tyre wear -> per-wheel grip factor (the WearCurve at dc +0x84),
 *         the per-wheel grip from the load, camber and surface, the per-wheel
 *         low speed factor / velocity angle, and the pre-step of a car.
 *         game/car/physics/T300_car_tyre_grip.h
 *   T301  the ground attitude from the 4 wheel heights, the wheel velocities
 *         (yaw rate x offsets), the car speed, the trip meter / condition,
 *         the top speed, the smoothed display speed, the axle speeds.
 *         game/car/physics/T301_car_motion.h
 *   T302  RaceCarState: the car's init, lap / finish queries, the finish
 *         action, the copy of the previous frame's values and their
 *         interpolators (the display runs between two dynamics frames), the
 *         per-frame step.  game/car/physics/T302_car_state.h
 *   T349  the car-to-car pass of the conductor: relative positions, the
 *         proximity code per pair (dc near), the computer drivers' traffic,
 *         the slipstream factor per pair (dc slip) and per car.
 *         game/race/dynamics/T349_car_pairs.h
 *   T303  the best lap of a car and the race display events of the section /
 *         lap times and differences (out-of-line copies of the T154 event
 *         classes' getEventType and typeinfo functions).
 *         game/race/dynamics/T303_race_time_events.h
 *   T304  SlowCarBoost (catch-up): DynamicsConductor::virtual_3 and the time
 *         scale of a car (dt, invDt).  game/race/dynamics/T304_slow_car_boost.h
 *
 * Every class, function and global the unit uses is declared in a canonical
 * header; this header only includes them and names a few numbers.  Offsets
 * in comments are the original (egcs) ones.
 */
#ifndef U082_H
#define U082_H

#include "types.h"
/* (No header here emits a type name or a literal: retail's T300, T301, T302
 * and T349 have no .rodata.  T303 includes T194 (the three T154 events'
 * names) and cxxrt.h (GCC 14's typeinfo copies) itself; T303 and T304 read
 * <new> and T153 with GAME_T153_EMITS_TYPEINFO before this header:
 * "bad_alloc" and UnitArena<64>'s name, as retail.) */

/* own translation units */
#include "game/car/physics/T300_car_tyre_grip.h"            /* car_wear_grip .. car_pre_step */
#include "game/car/physics/T301_car_motion.h"               /* car_ground_attitude .. car_speed_step */
#include "game/car/physics/T302_car_state.h"                /* RaceCarState, car_* (CAR_FINISH_*) */
#include "game/race/dynamics/T349_car_pairs.h"                /* CarPairInfo, dc_car_pairs */
#include "game/race/dynamics/T303_race_time_events.h"         /* the best lap, dc_post_*, the typeinfo copies */
#include "game/race/dynamics/T304_slow_car_boost.h"           /* SlowCarBoost, dc_setup_slow_car_boost */

/* outside the unit */
#include "game/race/core/T121_race_entry.h"               /* RaceEntryBase / RaceEntryCar (lapTime, timeRecord, topSpeed) */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization (recordSectionTime, lapRecord) */
#include "game/race/core/T128_race_lap_record.h"          /* LapRecord, RACE_TIME_INVALID */
#include "game/car/physics/T280_car_condition.h"            /* CarCondition */
#include "game/car/physics/T283_condition_model.h"          /* ConditionModel g_002D2B00 (u082 ConditionTable) */
#include "game/car/physics/T284_car_param.h"                /* CarDynamics_getTyre */
#include "game/sound/T288_car_sound.h"                /* DynCarState, car_sound_engine_levels */
#include "game/car/physics/T289_car_mode.h"                 /* car_silence_hidden */
#include "game/car/physics/T291_engine.h"                   /* CarEngine, engine_torque_at, engine_scale */
#include "game/car/physics/T296_car_drivetrain.h"           /* TyreData */
#include "game/car/physics/T298_suspension_body.h"          /* CarParams (CarDynamics), the body step functions */
#include "game/car/physics/T299_wheel_offsets.h"            /* car_wheel_planar_offsets, car_wheel_offsets */
#include "game/race/dynamics/T305_dynamics_conductor.h"       /* DynamicsConductor */
#include "game/common/T319_angle_math.h"               /* eulerToAxes */
#include "game/common/T320_record_set.h"               /* Curve */
#include "game/common/T322_curve_linear.h"             /* curveLookupLinear */
#include "game/car/physics/T327_wear_curve.h"               /* WearCurve */
#include "game/car/physics/T329_slipstream.h"               /* car_slipstream_update */
#include "game/car/physics/T331_dyn_car_message.h"          /* DynCar_tickMessage */
#include "game/sound/T333_tyre_squeal.h"              /* tyre_sound_update */
#include "game/effects/T334_tyre_smoke.h"               /* F_001DF148, tyre_smoke_emit */
#include "game/car/physics/T335_pad_vibration.h"            /* DynWheel, pad_vibration_update */
#include "game/car/physics/T336_steer_force.h"              /* steer_force_update */
#include "game/car/physics/T337_attitude_lag.h"             /* car_attitude_lag_update */
#include "game/car/physics/T341_gear_mode.h"                /* car_mode_computer, car_mode_stop_brake, car_mode_stop */
#include "game/car/physics/T348_wheel_speed_factor.h"       /* car_wheel_speed_factors (CarDynState *) */
#include "game/race/dynamics/T361_drive_params.h"             /* AIDriver */
#include "game/race/ai/T362_ai_params.h"                /* ai_apply_rules, ai_reduce_pace, F_001F6548 */
#include "game/race/ai/T365_ai_traffic.h"               /* ai_clear_traffic, ai_note_car, ai_note_closing */
#include "game/race/ai/T367_ai_line.h"                  /* ai_construct */
#include "game/data/T518_unit_settings.h"            /* UnitSettings g_002F59D8, UNIT_IMPERIAL */
#include "engine/media/G002_mpeg_frameq.h"              /* atan2f, cosf, sinf */
#include "runtime/newlib/G007_stdio.h"                    /* memset */

/* ======================================================================== */
/* Names for the numbers (only this unit uses them)                         */
/* ======================================================================== */

/* DynCarState.driver / ctrlMode / flags: DRIVER_* / CTRL_* / DYN_FLAG_*
 * (T288). */

/* DynWheel.wearLevel (T335): the segments of the wear curve. */
enum {
    WEAR_LEVEL_UNWORN = -0x80,                  /* below the curve's x0 */
    WEAR_LEVEL_X0     = -0x3F,                  /* at x0 (-0x3F..0 between x0 and 0) */
    WEAR_LEVEL_X1     = 0x20,                   /* 0..0x20 between 0 and x1 */
    WEAR_LEVEL_X2     = 0x40,                   /* 0x20..0x40 between x1 and x2 */
    WEAR_LEVEL_MAX    = 0x7F                    /* 0x40..0x7F between x2 and x3, then worn out */
};

/* Speeds and angles. */
#define KMH_1        0.2777778f                 /* 1 km/h in m/s */
#define KMH_10       2.777778f                  /* 10 km/h */
#define KMH_100      27.777779f                 /* 100 km/h */
#define KMH_400      111.111115f                /* 400 km/h */
#define TWO_PI       6.2831855f
#define FPS          60.0f                      /* the dynamics frames per second at time scale 1 */

#endif /* U082_H */
