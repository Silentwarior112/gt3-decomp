/* ai_internal.h - unit u086 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001F5FD8..0x001F8BA0, second pass.
 *
 * The computer driver of a race car: AIDriver (T361), the 0xBC-byte object
 * at RaceCarState +0xEC8 (u077 / u085 DynSub_EC8, u079 / u082 Transmission:
 * typedefs of it).  It follows one of the course's enemy lines (CourseLines,
 * DynamicsConductor +0xBBA8, T368: the nodes LineNode are its corners),
 * looks ahead along it, steers at the look-ahead point, brakes for the next
 * corner from the tyre grip, holds the corner speeds, picks a gear and moves
 * to another line around the cars near it.
 *
 * Translation units (link order):
 *   T362  the driver parameters: the 0x20-byte CourseDriveParam (T361) turned
 *         into AIParams, the race rules' handicaps, the downforce ratio
 *         (include/game/race/ai/T362_ai_params.h)
 *   T363  the gear for the target speed (T363_ai_gear.h)
 *   T364  the corner speeds (grip, radius, roll, downforce) and the corner
 *         bookkeeping along the line (T364_ai_corner_speed.h)
 *   T365  the cars around (eight zones), the closing-speed caution, the line
 *         changes they cause (T365_ai_traffic.h)
 *   T366  the braking point, the throttle for a target speed, the look-ahead
 *         point, the steering, the recovery (backing up / turning round) and
 *         the copy of the outputs into the car (T366_ai_drive.h)
 *   T367  set-up, the line tables, the line changes, the start revving
 *         (T367_ai_line.h)
 *
 * Every function takes the AIDriver ($a0) and reaches the car through
 * ai->car (RaceCarState, T302): its CarDynamics `params' (T279), the frame
 * time `dt', the run state `st' (DynCarState, T288 = CarRunState, T366).
 * Every class is defined in its canonical header; this header adds the
 * outside headers and what only this unit uses: the line table g_002DA030,
 * the unit conversions and the named values of the AIDriver bytes.
 */
#ifndef U086_H
#define U086_H

#include "types.h"

/* own translation units */
#include "game/race/ai/T362_ai_params.h"
#include "game/race/ai/T363_ai_gear.h"
#include "game/race/ai/T364_ai_corner_speed.h"
#include "game/race/ai/T365_ai_traffic.h"
#include "game/race/ai/T366_ai_drive.h"
#include "game/race/ai/T367_ai_line.h"

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, cosf, sinf */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getLapLength */
#include "game/race/display/T154_race_display_entry.h"   /* F_001927C8, F_001927D0, F_001927D8 (empty; not the
                                               display classes: retail's u086 files did not see them) */
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics (u086 dyn) */
#include "game/car/physics/T284_car_param.h"            /* CarDynamics_getTyre */
#include "game/sound/T288_car_sound.h"            /* DynCarState (u086 CarRunState) */
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData (u086 TyreGrip) */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_set_brake */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (wear, mode, lines, boxes, rules, clock) */
#include "game/common/T319_angle_math.h"           /* angleDiff */
#include "game/common/T320_record_set.h"           /* Curve (TyreData.loadCurve) */
#include "game/common/T321_curve_lookup.h"         /* curveLookup */
#include "game/race/dynamics/T326_race_rule_options.h"    /* RaceRuleOptions (enemySkillA..C2) */
#include "game/car/physics/T335_pad_vibration.h"        /* DynWheel (u086 CarWheelState) */
#include "game/car/physics/T341_gear_mode.h"            /* car_mode_still (ctrlMode 8) */
#include "game/car/physics/T343_gear_select.h"          /* car_gear_for_speed */
#include "game/car/physics/T356_brake_assist.h"         /* TcsParams */
#include "game/race/dynamics/T361_drive_params.h"         /* AIDriver, CourseDriveParam, ai_drive_param */
#include "game/race/ai/T368_enemy_lines.h"          /* CourseLines, EnemyLines, Line, LineNode */
#include "game/race/ai/T369_line_select.h"          /* lines_select, lines_side_line, lines_side_line_other */
#include "game/race/ai/T371_line_segment.h"         /* line_point_at, line_point_at_linear */
#include "game/car/physics/T374_car_box.h"              /* car_box_get_zero, car_box_hit_front, car_box_hit_rear */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* km/h -> m/s (the literal pool's 0x3E8E38E4).  The other speeds are kept as
 * the code's literals, the km/h in a comment (20.0f * KMH_TO_MS would not be
 * the same float). */
#define KMH_TO_MS           0.2777778f
#define DEG_TO_RAD          0.017453292f

/* The enemy lines the driver uses (T368): 6 the racing line, 1 / 2 its side
 * lines, 3 the way into the pit lane, 4 the pit lane. */
enum {
    LINE_PIT_ENTRY = 3,
    LINE_PIT_LANE  = 4,
    LINE_RACING    = 6
};

/* AIDriver.state / nextKind: the kind of the corner (LineNode.kind of its
 * node), then braking for the next one (ai_brake_factor). */
enum {
    AI_STRAIGHT    = 0,                     /* full throttle up to the top speed */
    AI_CORNER_POS  = 1,                     /* an arc, radius > 0 */
    AI_CORNER_NEG  = 2,                     /* an arc, radius < 0 */
    AI_BRAKE       = 3,                     /* braking from a straight */
    AI_BRAKE_CORNER = 4                     /* braking inside a corner (for a slower one) */
};

/* AIDriver.brakeState (ai_brake_factor, ai_drive), read by ai_avoid_traffic:
 * no line change in 1, 2 and 5; 3 / 4 forbid one side. */
enum {
    BRAKE_STATE_NONE     = 0,
    BRAKE_STATE_NEAR     = 1,               /* within margin + ai_brake_margin of the braking point */
    BRAKE_STATE_BRAKING  = 2,               /* past it (state 3 / 4) */
    BRAKE_STATE_FULL_NEG = 3,               /* full throttle in a kind 2 corner (radius < 0): not to the - side line */
    BRAKE_STATE_FULL_POS = 4,               /* full throttle in a kind 1 corner (radius > 0): not to the + side line */
    BRAKE_STATE_PART     = 5                /* part throttle in a corner */
};

/* AIDriver.recover: the way back to the line (ai_drive). */
enum {
    AI_ON_LINE     = 0,
    AI_BACKING_UP  = 1,                     /* reverses until the heading error is below 10 deg */
    AI_TURNING     = 2                      /* forward at 50 km/h, full lock beyond 80 deg */
};

/* DynCarState.ctrlMode / flags: CTRL_* / DYN_FLAG_* (T288). */

/* ======================================================================== */
/* Data of this unit                                                        */
/* ======================================================================== */

/* 0x002DA030 {6, 2, 1}: the lines ai_nearest_line tries (defined in T367;
 * retail has it in .data: not const). */
extern s32 g_002DA030[3];                   /* 0x002DA030 */

#endif /* U086_H */
