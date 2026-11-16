/* race_pause_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u057,
 * 0x0019F9F8..0x001A2EC8, second pass: translation units T167, T168, T169.
 *
 *   T167  RigidBodyManager: the loose course objects ("gadgets": cones,
 *         barrels ...) of a race.  A pool of 100 rigid bodies, each with a
 *         shape (up to 8 contact points, inertia, mass, bounding radius, a
 *         model); a 4th-order symplectic integrator (Forest-Ruth / Yoshida
 *         coefficients) of the Euler-angle rotation and Euler's equations,
 *         gravity, ground contact with an impulse + friction response,
 *         collisions with the cars (5-plane boxes rebuilt every frame from
 *         the car box list) and with the runway walls; drawing with a planar
 *         shadow.  The body matrix of the gadget being processed lives in
 *         the VU0 registers vf20..vf23 (VU0 macro mode) across the calls of
 *         Gadget::step, its transpose in vf24..vf27
 *         (include/game/car/physics/T167_rigid_body.h).
 *   T168  PauseBase: the base of the pause menus, a frame timeout
 *         (T168_pause_base.h).
 *   T169  RacePause, the race pause menu (continue / exit with an exit
 *         confirmation), its window PauseMenu and the typeinfo functions of
 *         T199's RaceMenuSelectWindow / RaceMenuWindow (T169_race_pause.h).
 *
 * Every shared class is defined in its canonical header (include/game/):
 * RigidBodyManager, GadgetShape, GadgetDesc, CarBoxList, WallSeg (T167),
 * PauseBase (T168), PauseMenu, RacePause (T169), the menu classes (T197 /
 * T198 / T199 / T220), the vectors (T159), Matrix44 (T131), CarBox (T376),
 * RaceCourse (T153), CourseData (T179).  This header is T167's only (the
 * data pass: T168 and T169 include the canonical headers their originals
 * saw themselves): the outside headers T167 needs and what only T167 uses:
 * the rigid body itself (Gadget, the canonical header only forward-declares
 * it), the wall callback's context, the vector helpers, the declarations
 * of T167's tables and the named constants of the literal pool.
 *
 * Coordinates: the gadgets live in the render world (x, y up, z); a car box
 * (T376: (x, y) horizontal, z up) maps to it as (x, y, z) -> (x, z, -y).
 */
#ifndef U057_H
#define U057_H

/* T167's include set only (T168 / T169 include their own headers): retail's
 * T167 .rodata opens with <new>'s "bad_alloc" (0x0032FEC8, u056's finding:
 * its inline what() literal is written where <new> is read, before the
 * file's own tables) and has two type names, 16RigidBodyManager's and,
 * after it in the section (so instantiated later), UnitArena<64>'s: the
 * T167 header first, then T153 with GAME_T153_EMITS_TYPEINFO.  None of the
 * other headers below makes the file see a class with a type name.  egcs's
 * <new> and cxxrt.h conflict: cxxrt.h for GCC 14 only (DATA.md). */
#include <new>                              /* retail's "bad_alloc" (T167's DATA_START) */
#include "types.h"                          /* boot_sqrtf (sqrt.s) */
#include "libc.h"                           /* memset */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_user (the GCC 14 __tf16RigidBodyManager) */
#endif

/* own translation unit */
#include "game/car/physics/T167_rigid_body.h"           /* RigidBodyManager, GadgetShape, GadgetDesc, CarBoxList,
                                               WallSeg; Vec3 / Vec4 (T159), CarBox (T376) */

/* outside the unit */
#define GAME_T153_EMITS_TYPEINFO            /* retail's T167 has UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (getPlane, forEachPoint) */
#include "engine/pgl/G001_flare.h"                /* pgl_* matrix stack, vu0_*, proj_shift_depth, box_visible,
                                               gfx_depth_mask */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, sincosf */
#include "game/race/core/T131_race_ps2_render.h"      /* Matrix44 */
#include "game/course/T179_course_data.h"          /* CourseData (runway) */
#include "game/sound/T288_car_sound.h"            /* car_report_crash */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_no_controller */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox */
#include "engine/gfx/T451_model_set.h"            /* ModelSet, modelset_draw_part */
#include "game/course/T457_runway.h"               /* Runway, runway_is_neighbour, RunwayPointFunc */

/* ======================================================================== */
/* T167: constants (the literal pool)                                       */
/* ======================================================================== */

#define RB_GRAVITY          9.81f           /* 0x411CF5C3 m/s^2 (gp-0x7B30) */
#define RB_TWO_PI           6.2831855f      /* 0x40C90FDB: the Euler angles wrap to [0, 2 pi) */
#define RB_SPIN_DAMPING     0.999f          /* 0x3F7FBE77: the angular velocity per step */
#define RB_SPIN_MAX         20.0f           /* rad/s, each body axis */
#define RB_RESTITUTION      0.25f           /* ground / wall impulses */
#define RB_REST_SPEED2      0.80999994f     /* 0x3F4F5C28 (0.9f squared): slower points do not bounce */
#define RB_FRICTION         0.5f            /* Coulomb: the tangential change is at most 0.5 j / m */
#define RB_CAR_IMPULSE_GAIN (-1.01f)        /* 0xBF8147AE: -(1 + 0.01) against a car */
#define RB_CAR_INERTIA      1.5f            /* the inertia scale of a car impulse */
#define RB_FACE_DEPTH_MAX   100000.0f       /* 0x47C35000: "no face" for pushOut / pushOutAlong */
#define RB_PENALTY_K        (-10.0f)        /* groundPenalty: the spring per metre of depth */
#define RB_PENALTY_DAMPING  (-0.999f)       /* 0xBF7FBE77: groundPenalty: of the point's velocity */
#define RB_YAW_WRAP         100.53097f      /* 0x42C90FDB: 32 pi, a car box yaw beyond it is reset */
#define RB_RAD_TO_DEG       57.295776f      /* 0x42652EE0 */
#define RB_SHADOW_DEPTH     0.005f          /* 0x3BA3D70A: the shadow's projection depth shift */
/* FLT_MAX (0x7F7FFFFF).  egcs cannot load it with li.s (the .lit4 pool):
 * it writes the constant into the object's .sdata (0x00352170, between
 * step's guard and the statics) and loads it with lui / lwc1, as retail
 * (wallImpulse).  (The first pass had it as a variable g_00352170.) */
#define RB_FLT_MAX          3.40282347e+38f

#define RB_POOL_SIZE        100             /* RigidBodyManager::capacity */
#define RB_RESTING_POINTS   3               /* contact counters at RB_CONTACT_RESTING: no motion */
#define RB_CONTACT_RESTING  2
#define RB_BOX_PLANES       5               /* the 4 sides and the top of a car box */
#define RB_MAX_CAR_BOXES    8               /* CarBoxList::box[8] */

/* WallSeg::flags: WALL_* (T167). */

/* modelset_draw_part's parts of a gadget's model (as T446's car models) */
#define GADGET_PART_BODY    0
#define GADGET_PART_SHADOW  1

/* car_report_crash (T288): what the car hit (CRASH_KIND_*, T288) and the
 * part.  u088 numbers the parts 0..7 corners, 8 + j edges: a gadget always
 * reports 9. */
#define CRASH_PART_GADGET   9               /* ? meaning (u088's numbering: edge 1) */

/* ======================================================================== */
/* T167: the rigid body (T167's own; the canonical header forward-declares it) */
/* ======================================================================== */

class Gadget;

/* The context of the wall callback (Gadget::step -> RaceCourse::forEachPoint
 * -> rb_wall_callback). */
struct WallCtx {
    const Vec3 *oldPos;                     /* 0x0 the position before the step */
    Gadget     *self;                       /* 0x4 */
    Vec3       *points;                     /* 0x8 the contact points (world) */
    int         hit;                        /* 0xC the first wall hit ends the search */
};

/* ? name (u057's).  One rigid body, 0x50 bytes (RigidBodyManager::pool[]). */
class Gadget {
public:
    Gadget() : next(NULL) {}                /* (inline: RigidBodyManager()) */

    void  init(const GadgetDesc *desc);     /* 0x0019FFC0 */
    void  clearContacts();                  /* 0x001A01D0 */
    int   findGround();                     /* 0x001A0200 1: on the road */
    int   step(f32 dt);                     /* 0x001A0258 0: off the road (removed) */
    void  loadMatrix();                     /* 0x001A08E8 vf20..vf23 = the body matrix */
    void  worldPoints(Vec3 *out);           /* 0x001A0940 */
    void  groundPenalty(Vec3 *acc, void *unused, const Vec3 *pts, int numPoints);  /* 0x001A09D8 (no callers) */
    void  groundContact(Vec3 *pts, int numPoints);  /* 0x001A0C48 */
    int   applyImpulse(int point, const Vec3 *normal);  /* 0x001A0D28 */
    void  collideCars(Vec3 *pts, int numPoints);    /* 0x001A10F8 */
    f32   carImpulse(int point, int box, const Vec3 *p, CarBox *car);  /* 0x001A1370 */
    int   pushOutAlong(int box, const Vec3 *p, const Vec3 *dir);       /* 0x001A16C0 (no callers) */
    int   pushOut(int box, const Vec3 *p);  /* 0x001A1808 */
    int   collideWall(const WallSeg *wall, const Vec3 *oldPos, Vec3 *pts);  /* 0x001A1930 */
    void  wallImpulse(Vec3 *pts, f32 dx, f32 dz);   /* 0x001A1B68 */
    void  rotationMatrix(Matrix44 *m);      /* 0x001A1C48 */
    void  shadowMatrix(Matrix44 *m);        /* 0x001A1C80 */
    void  draw();                           /* 0x001A1DD0 */
    int   isVisible();                      /* 0x001A1EF8 */

    Gadget *next;                           /* 0x00 the active / free list */
    Vec3    rot;                            /* 0x04 Euler angles (rad, [0, 2 pi)) */
    Vec3    pos;                            /* 0x10 */
    Vec3    vel;                            /* 0x1C */
    Vec3    angVel;                         /* 0x28 (body frame, |w| <= 20) */
    Vec4    plane;                          /* 0x34 the ground plane under it (n, d) */
    s16     shape;                          /* 0x44 index into g_00352174 */
    s16     sector;                         /* 0x46 the runway sector (-1 off the road) */
    u8      contact[8];                     /* 0x48 per contact point: ++ while it is the
                                               deepest ground contact, 0 above the ground;
                                               resting while 3 of them are 2 */
};
static_assert(sizeof(Gadget) == 0x50, "Gadget");

/* The car state block of a CarBox (+0x454..+0x473: statePos, stateVel,
 * stateYaw, stateYawRate; T374's in / out record) as one record.  T167
 * reads it through its address: retail keeps car + 0x454 in a register
 * (carImpulse, updateCarBoxes), the original's state member reached through
 * an inline accessor (a view of the canonical CarBox's fields, same
 * layout). */
struct CarBoxState {
    f32 pos[3];                             /* 0x00 statePos */
    f32 vel[3];                             /* 0x0C stateVel */
    f32 yaw;                                /* 0x18 stateYaw */
    f32 yawRate;                            /* 0x1C stateYawRate */
};
static inline CarBoxState *car_box_state(CarBox *c)
{
    return (CarBoxState *)c->statePos;
}

/* The vector helpers (T167) */
void rb_cross(Vec3 *out, const Vec3 *a, const Vec3 *b);   /* 0x0019F9F8 */
f32  rb_dot(const Vec3 *a, const Vec3 *b);                /* 0x0019FA60 */
void rb_rotate(Vec3 *out, const Vec3 *in);                /* 0x0019FA90 by vf20..vf22 */
void rb_rotate_inverse(Vec3 *out, const Vec3 *in);        /* 0x0019FAD8 by vf24..vf26 */
void rb_rotation_between(Matrix44 *m, const Vec3 *from, const Vec3 *to);  /* 0x0019FB20 */
void rb_euler_matrix(Matrix44 *m, f32 rx, f32 ry, f32 rz);    /* 0x0019FDA0 */
void rb_matrix_to_euler(Vec3 *out, const Matrix44 *m);    /* 0x0019FEB0 */
void rb_wall_callback(const WallSeg *wall, void *ctx);        /* 0x0019FF80 */
void F_001A1C78(void);                                    /* 0x001A1C78 (empty, no callers) */

/* ======================================================================== */
/* T167: data (T167's own, defined in T167_rigid_body.cpp; with these      */
/* extern declarations the const tables have external linkage, so egcs    */
/* writes them where they are defined, first in T167's .rodata)            */
/* ======================================================================== */

/* The position steps d[k] of the integrator, built on the first call of
 * Gadget::step from the velocity steps c[k]: a function-local static with a
 * dynamic initialiser (egcs: `static f32 d[4]' in Gadget::step, its guard
 * g_0035216C).  The GCC 14 branch spells out the guard with these. */
extern int          g_0035216C;             /* 0x0035216C gp-0x5884 the init guard of g_0049C3E0 */
extern f32          g_0049C3E0[4];          /* 0x0049C3E0 .bss d: c0 / 2, (c0 + c1) / 2 (twice), c0 / 2 */
extern const f32    g_0032FEE8[4];          /* 0x0032FEE8 c: 1.3512071, -1.7024144, 1.3512071, 0 */
extern const Vec3   g_0032FED8;             /* 0x0032FED8 (0, 1, 0): the body's up */
/* The outward normals of a car box's faces (w 0): front (0, .05, -1), the
 * -x side (-1, .05, 0), rear (0, .05, 1), the +x side (1, .05, 0), top
 * (0, 1, 0), and the bottom (0, -1, 0), which updateCarBoxes never turns
 * into a plane (the box is open below). */
extern const Vec4   g_0032FEF8[6];          /* 0x0032FEF8 */
/* The planes of each car box (RigidBodyManager::updateCarBoxes): (n, d),
 * inside where n.p + d <= 0 for all five. */
extern Vec4         g_002C0568[RB_MAX_CAR_BOXES][RB_BOX_PLANES];  /* 0x002C0568 .data */

/* (T168 / T169's named values: PAUSE_RESULT_* in T168_pause_base.h,
 * PAUSE_FRAMES_PER_SECOND in T168_pause_base.cpp, PAUSE_BUTTON_SWITCH in
 * T169_race_pause.cpp.  Neither file includes this header: their own
 * include sets decide their .rodata.) */

#endif /* U057_H */
