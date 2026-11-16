/* T376_car_box_walls.h - CarBox and the car against wall collisions of the
 * car boxes (translation unit T376, unit u088), 0x001FCF48..0x001FE768.
 *
 * The same boxes against the runway walls (WallSeg polylines, T167): the
 * swept test of the corners against the wall segments and of the wall
 * vertices against the box edges (RaceCourse::forEachPoint callbacks, cast
 * to RunwayPointFunc: they take a CarBox * context), the
 * impulse responses (restitution 0.05, with friction), the push-out of the
 * predicted pose and the sliding of corners along a wall.  The wall vertices
 * are in the course frame, where the box's y is -z.  The list of boxes and
 * the frame step are T373 / T374 (u087).
 */
#ifndef GAME_T376_CAR_BOX_WALLS_H
#define GAME_T376_CAR_BOX_WALLS_H

#include "game/game.h"
#include "game/car/physics/T374_car_box.h"              /* Vec2, BoxVertex (held by value) */
#include "game/car/physics/T375_car_box_cars.h"         /* BoxEdge (held by value) */

class RaceCarState;                         /* T302 (pointers only) */
class RaceCourse;                           /* T153 */
struct WallSeg;                             /* T167 */

/* ? name (u057, u087).  The collision box of a car: 8 corners, 8 edges, the
 * pose at the start of the step (pos), at its end (next), the swept bounding
 * box, the hits found and the car's state copied in and out (T374).  One per
 * entry in the box list (DynamicsConductor +0xBBB4 + 0xC, T373).  0x4F0
 * bytes. */
struct CarBox {
    /* The two empty loops over v[8] / e[8] (BoxVertex / BoxEdge have empty
     * inline constructors).  Defined in T317, after DynamicsConductor's
     * constructor, which calls it for boxes.box[8] (0x001F2A8C): an inline
     * body in the class would be inlined there. */
    CarBox();                               /* 0x001F42E0 (T317) */

    f32           invMass;                  /* 0x000 */
    f32           invInertia;               /* 0x004 1 / inertia */
    BoxVertex     v[8];                     /* 0x008 the corners */
    BoxEdge       e[8];                     /* 0x228 the edges */
    f32           radius;                   /* 0x388 the farthest corner */
    f32           t;                        /* 0x38C the part of the frame done (0..tEnd) */
    Vec2          pos;                      /* 0x390 */
    f32           posZ;                     /* 0x398 */
    f32           yaw;                      /* 0x39C */
    f32           tEnd;                     /* 0x3A0 1.0 */
    Vec2          next;                     /* 0x3A4 the pose at tEnd */
    f32           nextZ;                    /* 0x3AC */
    f32           nextYaw;                  /* 0x3B0 */
    Vec2          move;                     /* 0x3B4 next - pos */
    f32           moveZ;                    /* 0x3BC */
    f32           moveYaw;                  /* 0x3C0 */
    Vec2          vel;                      /* 0x3C4 */
    f32           velZ;                     /* 0x3CC */
    f32           yawRate;                  /* 0x3D0 */
    f32           bbMin[2];                 /* 0x3D4 the swept bounding box (x, y) */
    f32           bbMax[2];                 /* 0x3DC */
    f32           bbSize[2];                /* 0x3E4 */
    int           initialized;              /* 0x3EC the corners have world positions */
    int           carHit;                   /* 0x3F0 a car hit was found (car_box_sweep_car) */
    int           wallVertexHit;            /* 0x3F4 a corner hits a wall (car_box_sweep_walls) */
    int           wallEdgeHit;              /* 0x3F8 a wall vertex hits an edge (car_box_sweep_walls) */
    f32           hitTime;                  /* 0x3FC the time of the first hit */
    int           softHit;                  /* 0x400 a corner crosses a non-solid wall */
    f32           softT;                    /* 0x404 its part of the step (2.0 before the test) */
    int           carEdge;                  /* 0x408 the car hit: this box's edge */
    int           carOther;                 /* 0x40C the other box */
    int           carVertex;                /* 0x410 its corner */
    Vec2          carContact;               /* 0x414 the contact point (this box's local frame) */
    int           wallVertex;               /* 0x41C the wall hit: the corner */
    Vec2          wallDir;                  /* 0x420 the wall's unit direction */
    WallSeg      *wallSeg;                  /* 0x428 the wall's start vertex */
    int           wallEdge;                 /* 0x42C the wall vertex hit: the edge */
    int           collidable;               /* 0x430 */
    int           hitFront;                 /* 0x434 bool, cleared each frame (? corners / edges 0..2) */
    int           hitRear;                  /* 0x438 bool (? corners / edges 4..6) */
    int           hitWall;                  /* 0x43C bool */
    int           hitCar;                   /* 0x440 bool */
    int           crossedSoft;              /* 0x444 a non-solid wall was crossed (car_box_check_soft_wall) */
    f32           impact;                   /* 0x448 the frame's collision strength */
    int           index;                    /* 0x44C the car (the entry) */
    RaceCarState *car;                      /* 0x450 (u087: RaceCarDynamics, the same object) */
    f32           statePos[3];              /* 0x454 the car's state, in and out (T374) */
    f32           stateVel[3];              /* 0x460 */
    f32           stateYaw;                 /* 0x46C radians (reset when |yaw| > 32 pi) */
    f32           stateYawRate;             /* 0x470 */
    int           sector;                   /* 0x474 the runway sector */
    RaceCourse   *course;                   /* 0x478 */
    f32           front;                    /* 0x47C the box: local length forwards */
    f32           rear;                     /* 0x480 backwards */
    f32           halfWidth;                /* 0x484 */
    f32           mass;                     /* 0x488 the car's mass (car_box_init; invMass = 1 / it; u087 /
                                               u088 friction): car_box_friction caps the friction
                                               impulse at it x the slip, the wall slide's spin is
                                               lever x (it x the lost velocity) */
    f32           inertia;                  /* 0x48C the car's yaw inertia x 10 */
    f32           chamfer[2];               /* 0x490 the cut of the front / rear corners (0.2 x width) */
    CarBox       *boxes;                    /* 0x498 the list's box[0] */
    f32           reach[8];                 /* 0x49C radius + boxes[j].radius */
    f32           reach2[8];                /* 0x4BC its square */
    f32           dt;                       /* 0x4DC the frame time */
    int           impactTimer;              /* 0x4E0 frames the car's impact is held */
    f32           impactPeak;               /* 0x4E4 */
    int           soundTimer;               /* 0x4E8 */
    f32           soundLevel;               /* 0x4EC */
};
EGCS_SIZE(CarBox, 0x4F0);

/* Corners sliding along a wall (flags bit 0) move on to the next / previous
 * segment; a box still moving into the wall is put back on it, its velocity
 * projected and its yaw rate damped. */
void car_box_slide_walls(CarBox *b);                                    /* 0x001FCF48 */
/* the RaceCourse::forEachPoint callback of car_box_sweep_walls for the
 * segment w -> next (no callers outside T376) */
void car_box_sweep_wall_cb(WallSeg *w, CarBox *b);                      /* 0x001FD368 */
/* The swept test against the walls in the bounding box of the corners'
 * paths; keeps the earliest solid crossing (a corner or an edge).  1: a hit. */
int  car_box_sweep_walls(CarBox *b);                                    /* 0x001FD720 */
/* crossedSoft = 1 when a non-solid wall was crossed before time t */
void car_box_check_soft_wall(CarBox *b, f32 t);                         /* 0x001FD9D0 */
/* the callback of car_box_push_out_walls for the segment w -> next */
void car_box_push_wall_cb(WallSeg *w, CarBox *b);                       /* 0x001FDA10 */
/* The walls in the bounding box of the predicted corners push the predicted
 * pose out (car_box_push_wall_cb; corner 0 is tested twice, corner 7 not). */
void car_box_push_out_walls(CarBox *b);                                 /* 0x001FDE08 */
/* The response to a corner hitting a wall (wallVertex); the corner then
 * slides along it. */
void car_box_respond_wall_vertex(CarBox *b);                            /* 0x001FDEC0 */
/* The normal impulse of a corner hit (restitution 0.05): *imp = j n; |j| /
 * 100000 to impact, a hit of the car (T288).  v: the contact velocity. */
void car_box_wall_vertex_impulse(CarBox *b, Vec2 *imp, const f32 *rot, const Vec2 *v);  /* 0x001FE100 */
/* the hit corner back onto the wall line (rot is not used) */
void car_box_project_wall_vertex(CarBox *b, const f32 *rot);            /* 0x001FE288 */
/* The response to a wall vertex hitting an edge (wallEdge). */
void car_box_respond_wall_edge(CarBox *b);                              /* 0x001FE3A0 */
/* The normal impulse of a wall vertex hit: v the contact velocity, n the
 * edge normal, r the lever; *imp = j n. */
void car_box_wall_edge_impulse(CarBox *b, Vec2 *imp, const Vec2 *v, const Vec2 *n, const Vec2 *r);  /* 0x001FE578 */
/* The friction impulse out[3] (x, y, z) for the normal impulse imp and the
 * contact velocity v (no callers outside T376). */
void car_box_friction(CarBox *b, const Vec2 *imp, const Vec2 *v, f32 *out);  /* 0x001FE660 */

#endif /* GAME_T376_CAR_BOX_WALLS_H */
