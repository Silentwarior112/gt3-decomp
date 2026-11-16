/* car_box_internal.h - unit u088 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001FBCE8..0x001FED38, second pass.
 *
 * Translation units (link order):
 *   T375  the car collision boxes (CarBox, 0x4F0 bytes; T373 / T374 in
 *         u087): car against car.  A swept test of the other box's corners
 *         against this box's edges (the earliest hit of the frame), its
 *         impulse response (restitution 0.5), the overlap push-outs, and the
 *         resolution of corners resting on another box
 *         (include/game/car/physics/T375_car_box_cars.h)
 *   T376  the same boxes against the runway walls: the swept test of the
 *         corners against the wall segments and of the wall vertices
 *         against the box edges (RaceCourse::forEachPoint callbacks), the
 *         impulse responses (restitution 0.05, with friction), the push-out
 *         of the predicted pose and the sliding of corners along a wall
 *         (T376_car_box_walls.h)
 *   T397  VariableCameraBase: the base of the race cameras (OnBoardCamera,
 *         PasslineCamera, AutomaticCamera, ...): a two-entry history of the
 *         camera values, accessors, the frame update wrapper
 *         (T397_variable_camera_base.h)
 *
 * Every class is defined in its canonical header (include/game/): Vec2,
 * BoxVertex (T374), BoxEdge (T375), CarBox (T376), WallSeg (T167), RaceCourse
 * (T153), CourseData (T179), CameraBase (T410), VariableCameraBase (T397),
 * CameraProjection (T383).  This header adds the outside headers the sources
 * need and the named constants only this unit uses.  T375 and T376 also
 * include car_box_vec2.h, their 2D vector helpers (inline; not in this
 * header, which T397 includes too).
 *
 * Coordinates of the boxes: the plane is (x, y) with z up; the runway wall
 * vertices (WallSeg) are in the course frame, where the box's y is -z.  A
 * rotation "rot" is a row-major 2x2 matrix {cos, -sin, sin, cos} of a box's
 * yaw: world = rot * local.
 */
#ifndef U088_H
#define U088_H

#include "types.h"                          /* boot_sqrtf (sqrt.s) */
#include "libc.h"                           /* printf */
#ifndef BOOT_OLD_GCC                        /* (types.h defines it; egcs's <new>, which every u088
                                               source reads first for "bad_alloc", conflicts with it) */
#include "cxxrt.h"                          /* type_info, __rtti_si (T397's GCC 14 typeinfo copy) */
#endif

/* own translation units */
#include "game/car/physics/T375_car_box_cars.h"         /* BoxEdge, the car against car tests */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox, the car against wall tests */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase */

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* gfx_tex_param, pgl_push_matrix / pop / frustum /
                                               load / mult, vu0_invert_rigid, vu0_apply_matrix */
#include "engine/media/G002_mpeg_frameq.h"          /* cosf, sinf, logf, intr_suspend */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (data, forEachPoint) */
#include "game/car/physics/T167_rigid_body.h"           /* WallSeg */
#include "game/course/T179_course_data.h"          /* CourseData (runway) */
#include "game/sound/T288_car_sound.h"            /* car_report_crash */
#include "game/car/physics/T302_car_state.h"            /* pod_crash_reaction (the enhanced engine) */
#include "game/car/physics/T374_car_box.h"              /* Vec2, BoxVertex, car_box_place_corners,
                                               car_box_place_next_corners, car_box_translate */
#include "game/camera/T383_camera_projection.h"    /* CameraProjection (not the camera manager's header: its
                                               RaceCameraBase name would go into T397's .rodata) */
#include "game/camera/T410_camera_base.h"          /* CameraBase, __tf10CameraBase, __ti10CameraBase */
#include "game/course/T457_runway.h"               /* runway_is_neighbour, RunwayPointFunc */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* BoxVertex::flags */
#define BOX_VERTEX_ON_WALL  0x1             /* sliding along `wall' (T376) */
#define BOX_VERTEX_ON_CAR   0x2             /* resting on edge otherEdge of box otherCar (T375) */

/* WallSeg::flags: WALL_* (T167). */

/* car_report_crash's kind and part: CRASH_KIND_*, CRASH_PART_EDGE (T288). */

/* The corner / edge index ranges of the front and the rear of a box (the
 * corners run counter-clockwise from the front right, T374). */
#define BOX_FRONT_END       3               /* corners / edges 0..2: the front */
#define BOX_REAR_START      4               /* corners / edges 4..6: the rear */
#define BOX_HALF            4               /* a corner below 4: the front half */

/* "no crossing" for the parts of the step (t, softT, the best so far) */
#define BOX_NO_HIT          2.0f

/* The impulses: -(1 + restitution) and the impact scales. */
#define CAR_IMPULSE_GAIN    (-1.5f)         /* car against car: restitution 0.5 */
#define WALL_IMPULSE_GAIN   (-1.05f)        /* against a wall: restitution 0.05 */
#define CAR_IMPACT_SCALE    15000.0f        /* |j| / 15000 to both boxes' impact (and the crash) */
#define WALL_IMPACT_SCALE   100000.0f       /* |j| / 100000 to impact */

/* The wall push-out (car_box_push_wall_cb): a corner further than 0.5 from
 * the wall line (squared) moves the pose along the wall instead. */
#define WALL_PUSH_MAX2      0.25f

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft) (T376's 1 / |v|).  A volatile asm as
 * in the other units (u032 icon_rsqrt, u055, u056, u057, u061: rsqrt.s
 * behaves like one; T376 is built without -ffast-math, where egcs would
 * emit it for num / sqrtf), a macro so that no inline body shifts the
 * unit's label numbers; num / sqrt under GCC 14. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define u088_rsqrtf(num, sq)                                                  \
    ({ f32 u088_rsqrt_r_;                                                     \
       __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(u088_rsqrt_r_)        \
                            : "f"((f32)(num)), "f"((f32)(sq)));               \
       u088_rsqrt_r_; })
#else
#define u088_rsqrtf(num, sq)  ((f32)(num) / boot_sqrtf((f32)(sq)))
#endif

/* VariableCameraBase::setProjection's zoom for the texture LOD (1.428148,
 * 0x3FB6CD8E: 1 / tan(35 degrees), a 70 degree field of view). */
#define CAMERA_DEFAULT_ZOOM 1.428148f
/* setLodBias's constants: 1 / ln 2 (0x3FB8AA3B) and -ln 2 (0xBF317218). */
#define INV_LN2             1.442695f
#define NEG_LN2             (-0.6931472f)

#endif /* U088_H */
