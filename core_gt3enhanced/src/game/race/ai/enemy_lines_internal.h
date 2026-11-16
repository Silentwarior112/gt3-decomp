/* enemy_lines_internal.h - unit u087 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001F8BA0..0x001FBCE8, second pass.
 *
 * Translation units (link order):
 *   T368  the course's enemy lines, the driving lines of the computer cars
 *         (CourseData +0x7C): preparing the loaded file (relocation, fixed
 *         point to float, the runway sector, course distance, road height /
 *         pitch / roll of every node), ground queries along a line, a point
 *         of a line at a course distance, a corner entry search
 *         (include/game/race/ai/T368_enemy_lines.h)
 *   T369  choosing a line: the fallback list of a line kind, the side lines
 *         (1 / 2) of a kind (T369_line_select.h)
 *   T370  line 4, the pit lane: node distance / heading, a pit slot's pose
 *         (T370_pit_line.h)
 *   T371  a line's segment at a course distance (straight / arc
 *         interpolation of position and height); line 5's arc test
 *         (T371_line_segment.h)
 *   T372  line 4: is a course distance inside the pit lane (T372_pit_lane.h)
 *   T373  the car box list (DynamicsConductor +0xBBB4): building the boxes
 *         from the cars, the collision step of a frame (up to 2 passes of
 *         predict / detect / advance to the first hit / respond)
 *         (T373_car_box_list.h)
 *   T374  one car's collision box (CarBox, 0x4F0): an octagon of 8 corners
 *         and 8 edges, the pose / sweep / bounding box, the impact feedback
 *         to the car, save / restore of its corners (T374_car_box.h)
 *
 * Every class is defined in its canonical header (include/game/): the lines
 * (LineNode, Line, EnemyLines, CourseLines: T368), the boxes (Vec2,
 * BoxVertex, CarBoxSave: T374; BoxEdge: T375; CarBox: T376; CarBoxList,
 * WallSeg: T167).  The car is RaceCarState (T302; the first pass's
 * RaceCarDynamics, the same object): its CarDynamics +0x10 (the box's
 * lengths, width, mass, yaw inertia), the frame time +0xEC0, its DynCarState
 * +0xF84 (the impact +0x4F0, the steering +0x523).  This header adds the
 * outside headers the sources need and what only this unit uses: the two
 * tables of T368 / T369 and the named constants of the literal pool.
 *
 * The world of the lines and the boxes is (x, y) horizontal, z up; the
 * runway queries take (x, height, -y).
 */
#ifndef U087_H
#define U087_H

#include "types.h"                          /* boot_sqrtf (sqrt.s) */

/* own translation units */
#include "game/race/ai/T368_enemy_lines.h"          /* LineNode, Line, EnemyLines, CourseLines */
#include "game/race/ai/T369_line_select.h"
#include "game/race/ai/T370_pit_line.h"
#include "game/race/ai/T371_line_segment.h"
#include "game/race/ai/T372_pit_lane.h"
#include "game/car/physics/T373_car_box_list.h"
#include "game/car/physics/T374_car_box.h"              /* Vec2, BoxVertex, CarBoxSave */

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, cosf, sincosf, sinf */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (getLapLength, getLapDistance, getContact),
                                               RunwayPos, RunwayContact */
#include "game/car/physics/T167_rigid_body.h"           /* CarBoxList, WallSeg */
#include "game/course/T179_course_data.h"          /* CourseData (runway, enemyLine) */
#include "game/sound/T288_car_sound.h"            /* DynCarState, car_crash_sound_add */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_no_controller */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (getCar, carCount) */
#include "game/car/physics/T347_ground_surface.h"       /* ground_surface_type */
#include "game/car/physics/T375_car_box_cars.h"         /* BoxEdge, the car against car tests */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox, the car against wall tests */
#include "game/course/T457_runway.h"               /* Runway (points: the wall records) */

/* ======================================================================== */
/* Constants (the literal pool)                                             */
/* ======================================================================== */

#define U087_PI           3.1415927f        /* 0x40490FDB */
#define U087_TWO_PI       6.2831855f        /* 0x40C90FDB */
#define U087_TEN_DEG      0.17453292f       /* 0x3E32B8C2: 10 degrees in rad, the arc trace step */
#define U087_FIXED_ONE    0.00024414062f    /* 0x39800000: 1 / 4096, the line file's fixed point */
#define U087_PROBE_HEIGHT 100.0f            /* the runway probes start 100 m above the point */
#define U087_YAW_RATE_MAX 62.831856f        /* 0x427B53D2: 20 pi rad/s */

/* The line kinds (EnemyLines::line[] indices) the code names. */
enum {
    LINE_SIDE_1  = 1,                       /* a side line of the racing line */
    LINE_SIDE_2  = 2,                       /* the other side line */
    LINE_PIT_IN  = 3,                       /* the way into the pit lane */
    LINE_PIT     = 4,                       /* the pit lane (distances run on past the lap) */
    LINE_ARCS    = 5,                       /* only its arcs are used (T369's side test) */
    LINE_RACING  = 6                        /* the racing line */
};

/* boxes_step's hit code: a box index, + 0x200 for a wall hit. */
#define BOX_HIT_WALL      0x200

/* BoxVertex::flags bit 1: resting on an edge of another box (T375). */
#define BOX_VERTEX_ON_CAR 0x2

/* ======================================================================== */
/* Data of the unit (single-unit globals, h16's rule)                       */
/* ======================================================================== */

/* T369's .data (defined there; not const: retail has it in .data, before
 * T369's frame info): line kind -> up to 6 lines to try, -1 ends:
 *   0 {-1}  1 {1, 6, 2}  2 {2, 6, 1}  3 {3}  4 {4}  5 {-1}  6 {6, 1, 2}  7 {7, 6} */
extern s8 g_002DA508[8][6];                 /* 0x002DA508 */
/* T368's .sdata (defined there): the order lines_prepare works through the
 * lines: 6, 0, 1, 2, 5, 3, 4, 7 (the racing line first: line 4 needs its
 * sectors). */
extern const u8 g_003529C0[8];              /* 0x003529C0 gp-0x5030 */

#endif /* U087_H */
