/* T167_rigid_body.h - RigidBodyManager: the loose course objects
 * ("gadgets": cones, barrels ...) of a race, and the records it shares with
 * the course and the car collision code (translation unit T167, unit
 * u057), 0x0019F9F8..0x001A2590.
 *
 * A pool of 100 rigid bodies (Gadget, 0x50 bytes, T167's own), each with a
 * shape (GadgetShape: up to 8 contact points, inertia, mass, bounding
 * radius, a model); a 4th-order symplectic integrator of the Euler-angle
 * rotation and Euler's equations, gravity, ground contact with an impulse
 * and friction response, collisions with the cars (5-plane boxes rebuilt
 * every frame from the car box list) and with the runway walls (WallSeg
 * polylines), drawing with a planar shadow.  RacePS2Base (T130, +0xBCA4)
 * holds the manager; its virtual_8 sets the statics g_00352174 (the shapes:
 * the course objects, CourseData +0x90 + 4), g_00352178 (the car boxes,
 * DynamicsConductor +0xBBB4) and g_0035217C (the course), T167's .sdata
 * (declared at the end; the plan had put them in T130).
 * RaceCourse::setupRigidBodies (T153) adds the runway's 0x20-byte objects
 * (GadgetDesc).
 *
 * Here too, because the plan put them in T167:
 *   - CarBoxList, the car box list of the car collisions (T373..T376, u087 /
 *     u088): DynamicsConductor +0xBBB4 (T305);
 *   - WallSeg, a vertex of a runway wall polyline (Runway +0x3C, T457);
 *   - GadgetShape is also u059's CourseObj (T179: the objects of CourseData
 *     +0x90, the first of which T179 copies into g_002C04B8).
 */
#ifndef GAME_T167_RIGID_BODY_H
#define GAME_T167_RIGID_BODY_H

#include "game/game.h"
#include "game/effects/T159_flare.h"                /* Vec3, Vec4 (held by value) */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox (held by value) */

class Gadget;                               /* T167's own (pointers only: forward declarations) */
class ModelSet;                             /* T451 */
class RaceCourse;                           /* T153 */
class DynamicsConductor;                    /* T305 */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" const type_info &__tf16RigidBodyManager(void);  /* 0x001A2550 (user) */
#endif

/* ? name (u057's).  A vertex of a runway wall polyline, 16 bytes (the
 * records of Runway +0x3C); the wall runs to the next vertex, this + 1, or
 * this + 1 + link when link < 0 (the last vertex of a loop).  The car boxes
 * use x and -z as their plane (T376). */
struct WallSeg {
    f32 x;                                  /* 0x00 */
    f32 y;                                  /* 0x04 */
    f32 z;                                  /* 0x08 */
    s16 link;                               /* 0x0C */
    u16 flags;                              /* 0x0E WALL_* */
};
EGCS_SIZE(WallSeg, 0x10);

/* WallSeg::flags (u088's names; the sweep moved u057 / u088's copies here). */
#define WALL_TEST_VERTEX    0x1             /* the vertex is swept against the car box edges (T376) */
#define WALL_SOFT           0x2             /* not solid: the car boxes keep only its crossing time, the
                                               gadgets ignore it */

/* ? name (u057's; u085 / u089: the grid slots of CourseGrid).  One collision
 * box per car: DynamicsConductor +0xBBB4 (T305; u085's CourseGrid +0xC,
 * u083's CarAuxSet), built and stepped by T373 (boxes_init, boxes_step).
 * RigidBodyManager reads count and the boxes' state through g_00352178. */
struct CarBoxList {
    DynamicsConductor *dc;                  /* 0x000 */
    int                count;               /* 0x004 dc->carCount */
    RaceCourse        *course;              /* 0x008 */
    CarBox             box[8];              /* 0x00C (u057 cars[count], u089 cars) */
};
EGCS_SIZE(CarBoxList, 0x278C);

/* ? name (u057's; u059 CourseObj).  The shape of a gadget, 0xB0 bytes: the
 * objects of the course file (CourseData +0x90, T179; their model is an
 * offset from the object until T179 relocates it) and the template
 * g_002C04B8 (a copy of the first object). */
struct GadgetShape {
    int       count;                        /* 0x00 contact points (<= 8) */
    Vec3      inertia;                      /* 0x04 Ix, Iy, Iz */
    Vec4      points[8];                    /* 0x10 the contact points (body frame, w = 1) */
    f32       radius;                       /* 0x90 the bounding sphere */
    f32       mass;                         /* 0x94 */
    f32       invMass;                      /* 0x98 */
    u8        unk_9C[0x10];                 /* 0x9C */
    ModelSet *model;                        /* 0xAC (RigidBodyManager::GetModel; T179 relocates it) */
};
EGCS_SIZE(GadgetShape, 0xB0);

/* ? name (u057's).  A rigid body of the runway's object list (0x20 bytes
 * each, Runway +0x54; RaceCourse::setupRigidBodies). */
struct GadgetDesc {
    int  shape;                             /* 0x00 index of the shapes (g_00352174) */
    u8   unk_04[4];                         /* 0x04 */
    Vec3 pos;                               /* 0x08 */
    Vec3 rot;                               /* 0x14 the Euler angles (rad) */
};
EGCS_SIZE(GadgetDesc, 0x20);

/* vtable 0x0032FF58 (1 slot), typeinfo __tf16RigidBodyManager 0x001A2550
 * (user).  0x14 bytes of data, egcs vptr +0x14 (the constructor's store):
 * size 0x18, as egcs puts the vptr after every data member (u057 had 0x1C
 * with a pad at +0x18, which nothing reads; RacePS2Base +0xBCBC..+0xBCC0
 * after it, at +0xBCA4, is RacePS2Base's). */
class RigidBodyManager {
public:
    /* The pool (new Gadget[100], each next = 0: Gadget's inline
     * constructor), capacity 100, maxShape 0. */
    RigidBodyManager();                     /* 0x001A1F78 */
    virtual ~RigidBodyManager();            /* [0] +0x0C 0x001A1FE8 */
    /* No live body; all on the free list, maxShape -1 (RacePS2Base::
     * virtual_8). */
    void    reset();                        /* 0x001A2040 */
    /* A body of the runway object d, if one is free; 1 when added
     * (RaceCourse::setupRigidBodies). */
    int     add(const GadgetDesc *d);       /* 0x001A20A8 */
    /* Shapes 0..maxShape get the inertia and mass of g_002C04B8 (no
     * callers). */
    void    resetShapeMass();               /* 0x001A2108 */
    Gadget *alloc();                        /* 0x001A2188 NULL when none is free */
    void    release(Gadget *g, Gadget *prev);           /* 0x001A21B8 */
    /* Per frame (RacePS2Base): the car boxes, then every body (removed
     * when it leaves the road). */
    void    update(f32 dt);                 /* 0x001A21E0 */
    /* The light direction of the shadows (RacePS2Base: CourseEnv +0xAD0,
     * the first B light, T178). */
    static void SetLightVector(const Vec3 *v);          /* 0x001A2278 */
    /* The 5-plane boxes of the cars (g_002C0568) from g_00352178. */
    void    updateCarBoxes();               /* 0x001A22A0 */
    void    draw();                         /* 0x001A24B0 */
    /* The static accessors, inline (defined below the statics): T130's
     * RacePS2Base::virtual_8 stores the statics without a call, T167's own
     * code reads them directly; egcs emits the out-of-line copies at the
     * end of T167, before __tf16RigidBodyManager, as T198 / T199 do theirs.
     * The names but GetLightVector / GetModel / GetGadgetShape (lupo2) are
     * u057's. */
    static Vec3        *GetLightVector();               /* 0x001A24E8 */
    static ModelSet    *GetModel(int i);                /* 0x001A24F8 shape i's model (u057 void *) */
    static GadgetShape *GetGadgetShape(int i);          /* 0x001A2510 */
    static void         SetGadgetShapes(GadgetShape **s);   /* 0x001A2528 g_00352174 */
    static CarBoxList  *GetCarBoxes();                  /* 0x001A2530 */
    static void         SetCarBoxes(CarBoxList *l);     /* 0x001A2538 g_00352178 */
    static RaceCourse  *GetCourse();                    /* 0x001A2540 */
    static void         SetCourse(RaceCourse *c);       /* 0x001A2548 g_0035217C */

    Gadget *pool;                           /* 0x00 new Gadget[capacity] */
    int     capacity;                       /* 0x04 100 */
    Gadget *active;                         /* 0x08 the live list */
    Gadget *freeList;                       /* 0x0C */
    int     maxShape;                       /* 0x10 the largest shape index added (-1: none) */
    /*      vptr                               0x14 */
};
EGCS_SIZE(RigidBodyManager, 0x18);

/* The template shape (T167's .data): RigidBodyManager::resetShapeMass reads
 * its inertia and mass; T179 (u059) copies the course's first object into
 * it. */
extern GadgetShape g_002C04B8;              /* 0x002C04B8 */
/* The statics of the Get / Set accessors (T167's .sdata, after the FLT_MAX
 * at 0x00352170); RacePS2Base::virtual_8 sets them (u057's names). */
extern GadgetShape **g_00352174;            /* 0x00352174 gp-0x587C the shapes: course->data +0x90, + 4 */
extern CarBoxList   *g_00352178;            /* 0x00352178 gp-0x5878 the car boxes (the conductor's +0xBBB4) */
extern RaceCourse   *g_0035217C;            /* 0x0035217C gp-0x5874 the course */
/* The light direction of the shadows (T167's .data, (0, 1, 0) at start):
 * RigidBodyManager::SetLightVector / GetLightVector, Gadget::shadowMatrix. */
extern Vec3          g_002C04A8;            /* 0x002C04A8 */

/* Defined in the reverse of retail's order (0x001A24E8 GetLightVector ..
 * 0x001A2548 SetCourse): egcs writes out-of-class inline members out of
 * line last-defined first (T167's frame info, ref/data/u057/notes.md). */
inline void         RigidBodyManager::SetCourse(RaceCourse *c)  { g_0035217C = c; }
inline RaceCourse  *RigidBodyManager::GetCourse()               { return g_0035217C; }
inline void         RigidBodyManager::SetCarBoxes(CarBoxList *l) { g_00352178 = l; }
inline CarBoxList  *RigidBodyManager::GetCarBoxes()             { return g_00352178; }
inline void         RigidBodyManager::SetGadgetShapes(GadgetShape **s) { g_00352174 = s; }
inline GadgetShape *RigidBodyManager::GetGadgetShape(int i)     { return g_00352174[i]; }
inline ModelSet    *RigidBodyManager::GetModel(int i)           { return g_00352174[i]->model; }
inline Vec3        *RigidBodyManager::GetLightVector()          { return &g_002C04A8; }

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti16RigidBodyManager[2];   /* 0x003533B8 gp-0x4638 __user_type_info (u057 g_003533B8) */
#endif

#endif /* GAME_T167_RIGID_BODY_H */
