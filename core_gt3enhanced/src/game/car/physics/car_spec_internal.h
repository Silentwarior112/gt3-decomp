/* car_spec_internal.h - unit u076 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001D93C0..0x001DB220.
 *
 * Translation units (link order), each with its canonical header:
 *   T278  ShowRoomCar: a car set up outside a race (spec record, physics
 *         block, engine spec sheet) and the figures the menus show.
 *         game/car/model/T278_show_room_car.h
 *   T279  ValuateAutomobilePerformance, SelectMeterScale (the speedometer
 *         and tachometer scales); CarDynamics.  game/race/display/T279_meter_scale.h
 *   T280  CarCondition: odometer, engine and oil mileages, dirt.
 *         game/car/physics/T280_car_condition.h
 *   T324  DynamicsConductor slots 34..40 (the rule queries, the course-out
 *         rule) and the collision rule of License / TimeTrial.
 *         game/race/dynamics/T324_dynamics_conductor_rules.h (the class: T305)
 *   T325  no code: the dynamics tuning bytes DynParams g_002D28B0 (its
 *         .data and frame info only; T325_dyn_params.cpp).
 *         game/car/physics/T325_misc.h
 *   T281  battle-mode presets of two RaceRuleOptions groups; the default
 *         rules g_002D2988.  game/race/dynamics/T281_battle_rule_presets.h
 *   T282  SetupCarSpec: the CarSpec record of a CarEquipments.
 *         game/car/physics/T282_car_spec.h
 *   T326  RaceRuleOptions from the race (RaceDataBase).
 *         game/race/dynamics/T326_race_rule_options.h
 *   T327  WearCurve, built from RaceRuleOptions.  game/car/physics/T327_wear_curve.h
 *   T283  ConditionModel g_002D2B00: the CarCondition thresholds and the
 *         power factor of a condition; DynParams.
 *         game/car/physics/T283_condition_model.h
 *   T328  the rolling resistance curve and the per-surface table
 *         g_002D2CE0, from DynParams g_002D28B0.  game/car/physics/T328_surface_tables.h
 *   T329  the slipstream factors and their per-frame update.
 *         game/car/physics/T329_slipstream.h
 *
 * Every class, function and global the unit uses is declared in a canonical
 * header; this header only includes them.  Offsets in comments are the
 * original (egcs) ones.
 */
#ifndef U076_H
#define U076_H

#include "types.h"
#include <stddef.h>
/* No <new>: its inline bad_alloc::what() puts "bad_alloc" into every
 * object's .rodata; retail's T278 / T279 objects have none (0x0033C2D0..). */

/* own translation units */
#include "game/car/model/T278_show_room_car.h"            /* ShowRoomCar */
#include "game/race/display/T279_meter_scale.h"              /* CarDynamics, ValuateAutomobilePerformance, SelectMeterScale */
#include "game/car/physics/T280_car_condition.h"            /* CarCondition */
#include "game/race/dynamics/T281_battle_rule_presets.h"      /* rules_boost_preset, rules_tire_wear_preset, g_002D2988 */
#include "game/car/physics/T282_car_spec.h"                 /* SetupCarSpec, g_00352910 / g_00352918 */
#include "game/car/physics/T283_condition_model.h"          /* ConditionModel g_002D2B00, DynParams */
#include "game/race/dynamics/T324_dynamics_conductor_rules.h" /* F_001D9FE0 */
#include "game/race/dynamics/T326_race_rule_options.h"        /* RaceRuleOptions */
#include "game/car/physics/T327_wear_curve.h"               /* WearCurve */
#include "game/car/physics/T328_surface_tables.h"           /* SurfaceTable g_002D2CE0, g_002D2CA0 / g_002D2CC0, surface_tables_init */
#include "game/car/physics/T329_slipstream.h"               /* g_00352920..g_00352930, slipstream_init, car_slipstream_update */

/* outside the unit */
#include "game/car/physics/T284_car_param.h"                /* CarDynamics_init */
#include "game/car/physics/T291_engine.h"                   /* CarEngine, EngineSpecSheet, engine_spec_sheet */
#include "game/car/physics/T297_wheel_steer.h"              /* wheel_steer_angle */
#include "game/car/physics/T298_suspension_body.h"          /* wheel_camber, wheel_lateral_pos */
#include "game/car/physics/T302_car_state.h"                /* RaceCarState (DynCarState st, T288: u076's CarRunState,
                                                   T366's typedef of it) */
#include "game/race/dynamics/T305_dynamics_conductor.h"       /* DynamicsConductor (the T324 members), CourseLines lines */
#include "game/common/T320_record_set.h"               /* Curve, curveSet */
#include "game/car/physics/T325_misc.h"                     /* DynParams g_002D28B0 */
#include "game/race/ai/T372_pit_lane.h"                 /* pit_lane_contains */
#include "game/data/T481_car_data_base.h"            /* CarDataBase g_002EBD88, nameToCode */
#include "game/data/T482_car_equipments.h"           /* CarEquipments, CarSpec, CARDB_*, TIRE_KIND_*, LAYOUT_* */
#include "game/data/T484_race_database.h"            /* RaceDataBase, RaceEvent (u076's RaceSetup / RaceRecord) */
#include "game/data/T518_unit_settings.h"            /* UnitSettings g_002F59D8, UNIT_* */
#include "runtime/newlib/G007_stdio.h"                    /* memset */

/* ======================================================================== */
/* Names for the numbers (only this unit uses them)                         */
/* ======================================================================== */

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* DynamicsConductor::chargePenaltyBySurface's wheel counts: all four. */
enum { WHEELS_ALL = 4 };

/* The course-out levels of slots 36 / 37 (DynamicsConductor::virtual_36 /
 * virtual_37). */
enum {
    COURSE_OUT_NONE    = 0,
    COURSE_OUT_COUNT   = 1,                     /* DynCarState.courseOut counts up */
    COURSE_OUT_AT_ONCE = 2                      /* courseOut = 0xFF */
};

/* DynCarState.fault: FAULT_*; issueCrashEvent's kind: CRASH_KIND_* (T288). */

#endif /* U076_H */
