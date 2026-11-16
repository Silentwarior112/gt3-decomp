/* car_param_internal.h - unit u077 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001DB220..0x001DE5E0, second pass.
 *
 * Translation units (link order):
 *   T284  CarDynamics_init and its helpers: the car's physics parameter block
 *         (CarDynamics, T279) from its CarSpec (T482)
 *         (include/game/car/physics/T284_car_param.h)
 *   T330  the set-up of one car of the DynamicsConductor (RaceCarState, T302)
 *         (T330_dyn_car_init.h)
 *   T331  the timed per-car message (T331_dyn_car_message.h)
 *   T285  a 2-D float table lookup (T285_table.h)
 *   T332  the course's surface smoke types (T332_surface_params.h)
 *   T355  the check of the tyre / car sound table (T355_setting_check.h)
 *   T286  a level -> RGB colour ramp (T286_level_colour.h)
 *   T287  the tyre sound evaluators by surface class (T287_surface_grip.h)
 *
 * Every class is defined in its canonical header (include/game/); this header
 * adds the outside headers the sources need and the unit's named constants.
 * None of them emits a type name or a literal (tools/hdrnames.py): the type
 * names the originals have (UnitArena<64>, with "bad_alloc", in T284 / T330 /
 * T332 only) come from those three files' own `<new>' and T153 includes.
 */
#ifndef U077_H
#define U077_H

#include "types.h"
#include <stddef.h>

/* own translation units */
#include "game/car/physics/T284_car_param.h"
#include "game/car/physics/T285_table.h"
#include "game/race/display/T286_level_colour.h"
#include "game/car/physics/T287_surface_grip.h"
#include "game/car/physics/T330_dyn_car_init.h"
#include "game/car/physics/T331_dyn_car_message.h"
#include "game/car/physics/T332_surface_params.h"
#include "game/car/physics/T355_setting_check.h"

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, cosf, sinf */
#include "runtime/newlib/G007_stdio.h"                /* memset */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (input, steeringAssist..), RaceEntryBase (cars) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getContact, RunwayContact (GroundQuery) */
#include "game/course/T178_course_env.h"           /* CourseEnv, SmokeType */
#include "game/course/T179_course_data.h"          /* CourseData (env) */
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition::clear */
#include "game/sound/T288_car_sound.h"            /* DynCarState, car_sound_tyre_roll */
#include "game/car/physics/T291_engine.h"               /* CarEngine, engine_setup, engine_torque_at, .. */
#include "game/car/physics/T292_gear_calc.h"            /* GearCalc, gear_calc */
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData, tyre_peak_slip_angle */
#include "game/car/physics/T297_wheel_steer.h"          /* wheel_steer_angle */
#include "game/car/physics/T298_suspension_body.h"      /* wheel_camber, wheel_lateral_pos */
#include "game/car/physics/T299_wheel_offsets.h"        /* car_wheel_offsets */
#include "game/car/physics/T300_car_tyre_grip.h"        /* tyre_load_per_width */
#include "game/car/physics/T301_car_motion.h"           /* car_ground_attitude, car_set_condition */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_reset_best_lap */
#include "game/race/dynamics/T304_slow_car_boost.h"       /* car_time_scale */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (wear, lines, rules, slots 27 / 32 / 33 / 38) */
#include "game/common/T319_angle_math.h"           /* eulerToAxes */
#include "game/common/T320_record_set.h"           /* Curve, curveSet */
#include "game/common/T321_curve_lookup.h"         /* curveLookup */
#include "game/common/T322_curve_linear.h"         /* curveLookupLinear */
#include "game/car/physics/T325_misc.h"                 /* g_002D28B0 (DynParams) */
#include "game/race/dynamics/T326_race_rule_options.h"    /* RaceRuleOptions */
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve */
#include "game/car/physics/T328_surface_tables.h"       /* g_002D2CE0 (SurfaceTable) */
#include "game/car/physics/T329_slipstream.h"           /* g_00352920 */
#include "game/car/physics/T337_attitude_lag.h"         /* car_attitude_lag_reset */
#include "game/car/physics/T341_gear_mode.h"            /* car_mode_computer, car_mode_pit_box */
#include "game/car/physics/T342_car_input.h"            /* car_set_steer_assist */
#include "game/car/physics/T343_gear_select.h"          /* car_set_gear, car_gear_for_speed */
#include "game/car/physics/T347_ground_surface.h"       /* ground_surface_type */
#include "game/race/dynamics/T361_drive_params.h"         /* AIDriver, drive_class */
#include "game/race/ai/T362_ai_params.h"            /* ai_load_params, ai_apply_rules, F_001F6540, ai_set_downforce */
#include "game/race/ai/T363_ai_gear.h"              /* ai_select_gear */
#include "game/race/ai/T367_ai_line.h"              /* ai_reset */
#include "game/race/ai/T370_pit_line.h"             /* pit_slot_pose */
#include "game/data/T482_car_equipments.h"       /* CarSpec, CarSpecSuspension, CarSpecTyre */

/* ======================================================================== */
/* Named constants (the literal pool's values, 0x00350168..0x00350248)      */
/* ======================================================================== */

#define KMH_TO_MPS   0.2777778f             /* 1 / 3.6 (0x3E8E38E4) */
#define MPS_TO_KMH   3.6f                   /* 0x40666666 */
#define DEG2RAD      0.017453292f           /* pi / 180 (0x3C8EFA35) */
#define RPM_TO_RADS  0.10471976f            /* 2 pi / 60 (0x3DD67750) */
#define GRAVITY      9.8f                   /* m/s^2 (0x411CCCCD): kgf -> N */
#define TWO_PI       6.2831855f             /* 0x40C90FDB */
#define HALF_PI      1.5707964f             /* 0x3FC90FDB */
#define QUARTER_PI   0.7853982f             /* 0x3F490FDB */

/* DynCarState.msgTime: a message without a time-out (T331). */
#define MSG_NO_TIMEOUT 0xFF

#endif /* U077_H */
