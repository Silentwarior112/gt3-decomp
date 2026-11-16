/* control_record_internal.h - unit u085 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001F2A10..0x001F5FD8.
 *
 * Translation units (link order):
 *   T317  DynamicsConductor: the base constructor, the per-race set-up of the
 *         cars (slot 22 initialize, "DynamicsConductor::initialize()"), the
 *         start type / start speed rules (slots 23..25), the frame step of
 *         all the cars (update) and the accessors of the course, the rules,
 *         the enemy lines and the collision boxes; CarBox's constructor
 *   T353  CarInput::clear (one frame of a car's input; u085's first pass
 *         called it ControlFrame)
 *   T318  AutomobileControlRecord: Recorder (a bit-stream codec of the input
 *         frames: run lengths and changed fields, Elias-gamma style codes)
 *         and Manager (one recorder over a whole buffer of the pool or over
 *         0x8000-byte lap slots)
 *   T360  CarPairStates, DynamicsConductor +0xBA34 (Lupo 2's "race
 *         watcher"): an 8x8 state machine over the ordered pairs of cars
 *   T361  the course drive parameters of a drive class (CourseDriveParam)
 *
 * The classes and functions are declared in the canonical headers: the own
 * ones (T318, T361), T305 (DynamicsConductor with T317's members and
 * CarPairStates), T342 (CarInput) and those of the units they use.  The car
 * is RaceCarState (T302; u085's DynCar is a typedef of it in T330): +0xEC4
 * interpBias (the first pass's stepAcc), +0xEC8 its computer driver
 * (AIDriver = DynSub_EC8), +0xF84 its run state (DynCarState = DynState).
 * The conductor's +0xBBA8 (the first pass's CourseGrid) is CourseLines lines
 * (T368) followed by CarBoxList boxes (T167) with CarBox box[8] (T376; the
 * first pass's GridSlot).  This header keeps what is local to the unit: the
 * global only this unit reads and named constants.
 *
 * Data (u085's data pass, ref/data/u085/notes.md): none of the headers below
 * writes a type name or a literal.  Retail's T317, T318, T360 and T361 each
 * open their .rodata with <new>'s "bad_alloc" and have UnitArena<64>'s type
 * name (T317 RaceCameraBase's too; T353 has no .rodata): each of them reads
 * <new> and T153 with GAME_T153_EMITS_TYPEINFO itself, before this header
 * (T318 after its own class header), and anchors the string with
 * DATA_START.  egcs's <new> and cxxrt.h conflict, so cxxrt.h is read for
 * GCC 14 only.
 */
#ifndef U085_H
#define U085_H

#include "types.h"
#include "libc.h"                           /* memset, memcpy, malloc, free */
#ifndef BOOT_OLD_GCC                        /* (types.h defines it) */
#include "cxxrt.h"                          /* __rtti_user (GCC 14's typeinfo copies, T318; egcs generates
                                               them itself, and its <new> conflicts with cxxrt.h) */
#endif
#include <stddef.h>

/* the own translation units */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (T317's members), CarPairStates (T360) */
#include "game/car/physics/T342_car_input.h"            /* CarInput (T353's clear), input_tuning_init, car_input_update */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord, g_003529B0..g_003529B8, g_002D8EE0 */
#include "game/race/dynamics/T361_drive_params.h"         /* CourseDriveParam, AIDriver (DynSub_EC8), drive_class,
                                               getDriveParam, ai_drive_param */

/* the classes */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, RaceResult */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar, RaceEntryBase */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue */
#include "game/race/core/T413_race_input.h"           /* RaceInput (enabled) */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseKind */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getGearPart, CarSpec, LAYOUT_* */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition */
#include "game/race/core/T153_race_course.h"          /* RaceCourse */
#include "game/course/T179_course_data.h"          /* CourseData (runway, driveParam) */
#include "game/course/T457_runway.h"               /* Runway, runway_marker_count, runway_marker_value */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_init, car_save_prev, car_view_update, car_step */
#include "game/sound/T288_car_sound.h"            /* DynCarState, car_crash_sound_clear */
#include "game/car/physics/T330_dyn_car_init.h"         /* DynCar_initialize (DynCar, DynState) */
#include "game/race/dynamics/T326_race_rule_options.h"    /* RaceRuleOptions */
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve */
#include "game/race/ai/T368_enemy_lines.h"          /* CourseLines, lines_init, lines_corner_entry_before, lines_get_point */
#include "game/race/ai/T369_line_select.h"          /* lines_select */
#include "game/race/ai/T370_pit_line.h"             /* pit_slot_node, pit_node_dist_wrapped, pit_node_dist, pit_node_angle */
#include "game/race/ai/T371_line_segment.h"         /* line_point_at, line_point_at_linear */
#include "game/car/physics/T167_rigid_body.h"           /* CarBoxList */
#include "game/car/physics/T373_car_box_list.h"         /* boxes_init, boxes_step, boxes_step_car */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox (its constructor is T317's) */
#include "engine/pdistd/T536_bit_stream_buffer.h"    /* BitStreamBuffer */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/dynamics/T313_dc_free_run.h"          /* OneLapGhostInfo (the head of a record slot) */

/* the functions of other units */
#include "game/car/physics/T282_car_spec.h"             /* SetupCarSpec */
#include "game/car/physics/T300_car_tyre_grip.h"        /* car_pre_step */
#include "game/car/physics/T297_wheel_steer.h"          /* car_wheel_steer_angles */
#include "game/car/physics/T296_car_drivetrain.h"       /* car_tyre_slip_ratios, car_drivetrain_step, car_tyre_wear,
                                               car_tyre_check */
#include "game/car/physics/T340_assist_control.h"       /* car_assists */
#include "game/car/physics/T295_tyre_heat.h"            /* car_brake_heat, brake_heat_setup */
#include "game/car/physics/T345_wheel_shake.h"          /* car_wheel_shake */
#include "game/car/physics/T336_steer_force.h"          /* pad_vibration_send */
#include "game/race/dynamics/T349_car_pairs.h"            /* dc_get_slip, dc_get_near, dc_clear_slip, dc_clear_near,
                                               dc_car_pairs */
#include "game/race/dynamics/T304_slow_car_boost.h"       /* dc_setup_slow_car_boost */
#include "game/car/physics/T346_conductor_car_boxes.h"  /* conductor_store_car_boxes, conductor_load_car_boxes */
#include "game/car/physics/T328_surface_tables.h"       /* surface_tables_init */
#include "game/car/physics/T332_surface_params.h"       /* CourseEnv_initSurfaceSmoke */
#include "game/sound/T333_tyre_squeal.h"          /* tyre_sound_init */
#include "game/car/physics/T298_suspension_body.h"      /* sl_active_suspension_init (the enhanced engine) */
#include "game/car/physics/T344_drivetrain.h"           /* hybrid_flow_reset (the enhanced engine) */
#include "game/car/physics/T292_gear_calc.h"            /* gearbox_is_electric (the enhanced engine) */

/* ======================================================================== */
/* T361's data                                                              */
/* ======================================================================== */

/* Four default records in front of g_002D9A08 that nothing reads (T361
 * defines both). */
extern CourseDriveParam g_002D9988[4];      /* 0x002D9988 */
/* The default drive parameters by course kind (0, 1, 2: dirt) for a course
 * whose data has none (getDriveParam).  Only this unit reads it (h20). */
extern CourseDriveParam g_002D9A08[3];      /* 0x002D9A08 */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* DynamicsConductor::mode: initialize's argument, RaceBase::replayMode
 * (T129: "0 race, 1 replay, 2 demo, 3 ?").  Non-zero: update steps the cars
 * at replaySpeed. */
enum {
    DC_MODE_RACE   = 0,
    DC_MODE_REPLAY = 1,                     /* the players' cars stay DRIVER_PAD (their inputs play back) */
    DC_MODE_DEMO   = 2,                     /* setFinishTimer runs only here; slot 23 gives the players' cars
                                               to the computer (getEntrantInfoRace) or DRIVER_AUTO (getEntrantInfoTrial) */
    DC_MODE_3      = 3                      /* ? (T129 too); the players' cars are DRIVER_AUTO */
};

/* DynCarState.driver: DRIVER_* (T288). */

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* DynCar_initialize's startType, as T317 chooses it (slots 24 / 25). */
enum {
    START_GRID      = 0,                    /* the start grid record */
    START_AT_POINT  = 1,                    /* startArg m along the racing line (License: rules.launchPoint) */
    START_ROLLING   = 2,                    /* behind the line (launchPoint < 0) at the launch speed */
    START_COURSE    = 3,                    /* the course drive parameters' start (record 0) */
    START_CORNER    = 4                     /* just before the last tight corner before the line, at the
                                               computer driver's corner speed (T330: AIDriver.nextSpeed) */
};

/* Enemy lines (T368 / T369). */
enum {
    LINE_PIT_LANE = 4,                      /* line 4: line_point_at_linear (its distances run past the lap) */
    LINE_RACING   = 6                       /* the racing line */
};

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* The longitudinal zone of the proximity code near[i][j] & 0xF
 * (car_near_code, T349): where car j is in car i's frame. */
enum {
    ZONE_NONE         = 0,                  /* more than 50 m away */
    ZONE_FAR_BEHIND   = 1,                  /* more than 4 x the rear gap behind */
    ZONE_BEHIND       = 2,
    ZONE_CLOSE_BEHIND = 3,                  /* within the rear gap behind (or alongside) */
    ZONE_CLOSE_AHEAD  = 4,                  /* within the front gap ahead */
    ZONE_AHEAD        = 5,
    ZONE_FAR_AHEAD    = 6                   /* more than 4 x the front gap ahead */
};

/* CarPairStates.state[i][j] (? names: what the states mean is read from
 * update's transitions only). */
enum {
    PAIR_NONE      = 0,
    PAIR_FOLLOWING = 1,                     /* ? i has been in j's slipstream for more than 60 frames */
    PAIR_CHASING   = 2,                     /* ? j is ahead of i (ZONE_AHEAD) */
    PAIR_CLOSE     = 3                      /* ? j is right ahead of i (ZONE_CLOSE_AHEAD) */
};

#endif /* U085_H */
