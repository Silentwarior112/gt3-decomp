/* T374_car_box.h - one car's collision box (translation unit T374, unit
 * u087), 0x001FAE38..0x001FBCE8.
 *
 * CarBox (0x4F0, T376_car_box_walls.h; u083 CarAux, u085 GridSlot) is an
 * octagon in the car's frame: 8 corners (BoxVertex, counter-clockwise from
 * the front right; local x across the car, y along it, front +) and 8 edges
 * (BoxEdge, T375).  T374 builds it, moves its pose over the frame (pos at t,
 * next at tEnd), sweeps its bounding box, feeds the frame's impact back to
 * the car and saves / restores its corners for the replay sectors
 * (DynamicsConductor::saveSector / restoreSector, T306).  The hit tests and
 * their resolution are T375 (the cars) and T376 (the walls).
 *
 * The CarBox frame flags read by the getters: hitFront +0x434, hitRear
 * +0x438, hitWall +0x43C, hitCar +0x440, crossedSoft +0x444 (u088's names;
 * u087 f434..f444).  Front / rear: the corner (or the other box's edge)
 * index below 4 / 4..6. */
#ifndef GAME_T374_CAR_BOX_H
#define GAME_T374_CAR_BOX_H

#include "game/game.h"

struct CarBox;                              /* T376_car_box_walls.h (pointers only) */
struct WallSeg;                             /* T167_rigid_body.h: a 16-byte record of Runway +0x3C */
class RaceCourse;                           /* T153_race_course.h */
class RaceCarState;                         /* T302_car_state.h (u087: RaceCarDynamics, T154) */

/* A point or direction of the box plane. */
struct Vec2 {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
};
EGCS_SIZE(Vec2, 0x8);

/* A corner of the box, 0x44 bytes (CarBox +0x8 + k x 0x44). */
struct BoxVertex {
    /* Inline and empty: CarBox's constructor (0x001F42E0) loops over v[8]
     * with nothing in the loop. */
    BoxVertex() {}

    Vec2     local;                         /* 0x00 in the car's frame */
    Vec2     dir;                           /* 0x08 local / |local| */
    Vec2     pos;                           /* 0x10 world, at the current pose */
    Vec2     next;                          /* 0x18 world, at the predicted pose */
    u32      flags;                         /* 0x20 bit 0: sliding along `wall'; */
                                            /* bit 1: resting on edge otherEdge of box otherCar */
                                            /* (cleared when the box stops being collidable) */
    int      hit;                           /* 0x24 a wall segment crosses its path (u088) */
    f32      t;                             /* 0x28 the crossing's part of the step (2.0: none) */
    WallSeg *wall;                          /* 0x2C the wall segment (its start vertex); */
                                            /* car_box_save stores it as an index of Runway +0x3C */
    Vec2     wallDir;                       /* 0x30 the unit direction of that segment */
    u8       unk_38[4];                     /* 0x38 */
    int      otherCar;                      /* 0x3C the box it rests on (flags bit 1) */
    int      otherEdge;                     /* 0x40 that box's edge */
};
EGCS_SIZE(BoxVertex, 0x44);

/* The saved corners of a box (car_box_save / car_box_load), 0x228 bytes. */
struct CarBoxSave {
    int       skip;                         /* 0x0 non-zero: nothing to restore */
    int       size;                         /* 0x4 car_box_save_size() */
    BoxVertex v[8];                         /* 0x8 (wall: the index of a 16-byte record of Runway +0x3C) */
};
EGCS_SIZE(CarBoxSave, 0x228);

/* The box of car `index': the corners from the lengths (cut by the
 * chamfers), the edges, the radius, 1 / mass, 1 / (inertia x 10). */
void car_box_init(CarBox *b, int index, RaceCourse *course, RaceCarState *car, f32 front,
                  f32 rear, f32 width, f32 chamferFront, f32 chamferRear, f32 mass,
                  f32 inertia);                                                 /* 0x001FAE38 */
/* The other boxes and the pair reaches (radius sums and their squares). */
void car_box_link(CarBox *b, CarBox *boxes, int n);                             /* 0x001FB0D8 */
/* The frame's start: pose and velocity from the car's state, the frame
 * flags cleared, the corners from the last prediction (the first time:
 * placed at the pose). */
void car_box_begin(CarBox *b, f32 dt);                                          /* 0x001FB140 */
/* Switching collisions off clears bit 1 of every corner's flags. */
void car_box_set_collidable(CarBox *b, int on);                                 /* 0x001FB1F0 */
/* The resting contacts: T375's car contacts (F_001FBCE8), then T376's wall
 * slides (F_001FCF48).  Callers: boxes_step, boxes_step_car. */
void car_box_resolve_contacts(CarBox *b);                                       /* 0x001FB238 (F_001FB238) */
/* The pose at tEnd from the velocity, the swept bounds, the predicted
 * corners. */
void car_box_predict(CarBox *b);                                                /* 0x001FB268 */
/* Before the hit tests: carHit = wallVertexHit = wallEdgeHit = softHit = 0,
 * softT = 2.0. */
void car_box_clear_hits(CarBox *b);                                             /* 0x001FB308 */
/* Moves the pose (and the corners) to time t. */
void car_box_advance(CarBox *b, f32 t);                                         /* 0x001FB328 */
/* The predicted corners become the current ones. */
void car_box_commit_corners(CarBox *b);                                         /* 0x001FB3E8 */
/* The corners' world positions at the pose. */
void car_box_place_corners(CarBox *b);                                          /* 0x001FB420 */
/* The corners' world positions at the predicted pose. */
void car_box_place_next_corners(CarBox *b);                                     /* 0x001FB4D8 */
/* The (x, y) bounds of the move from pos to next, grown by the radius. */
void car_box_sweep_bounds(CarBox *b);                                           /* 0x001FB590 */
/* An angle 8 turns or more from 0 brought back by whole turns. */
f32  wrap_angle(f32 a);                                                         /* 0x001FB628 */
/* The yaw rate held to +-20 pi rad/s. */
f32  clamp_yaw_rate(f32 w);                                                     /* 0x001FB698 */
/* The frame's end: the predicted pose and the velocity back to the car's
 * state. */
void car_box_end(CarBox *b);                                                    /* 0x001FB6D8 */
/* A (nearly) stopped car hit early in the frame (t < 0.1) turns with its
 * steering, and is predicted again. */
void car_box_steer_at_rest(CarBox *b, f32 t);                                   /* 0x001FB740 */
/* Moves pos and the corners' pos by (dx, dy) (T375). */
void car_box_translate(CarBox *b, f32 dx, f32 dy);                              /* 0x001FB7E8 */
/* hitWall || hitCar (car_update_pit, DynamicsConductor::updateCar). */
int  car_box_has_hit(CarBox *b);                                                /* 0x001FB838 (car_box_hit_43C_440) */
int  car_box_hit_wall(CarBox *b);                                               /* 0x001FB860 (car_box_get_43C) */
int  car_box_hit_car(CarBox *b);                                                /* 0x001FB868 (car_box_get_440; no callers) */
/* Always 0 (ai_drive: the first of its three box flags). */
/* Always 0: ai_drive's boxZero transitions (to recover 1 from 0, its part
 * of 2 -> 1) are dead code in this build (u086). */
int  car_box_get_zero(CarBox *b);                                               /* 0x001FB870 */
int  car_box_hit_front(CarBox *b);                                              /* 0x001FB878 (car_box_get_434; ai_drive) */
int  car_box_hit_rear(CarBox *b);                                               /* 0x001FB880 (car_box_get_438; ai_drive) */
int  car_box_crossed_soft(CarBox *b);                                           /* 0x001FB888 (car_box_get_444; no callers) */
/* The frame's impact (capped at 1) to the car: a crash sound request
 * (car_crash_sound_add with kind -1) held 60 frames, and car->impact held
 * 24 frames at the peak. */
void car_box_impact_feedback(CarBox *b, RaceCarState *car);                     /* 0x001FB890 */
/* Saves the 8 corners into buf (the wall pointers as record indices);
 * returns the end of the record (DynamicsConductor::saveSector). */
void *car_box_save(CarBox *b, CarBoxSave *buf);                                 /* 0x001FBA08 */
/* Restores them, keeping the box's own local / dir; a skip record is passed
 * over.  Returns the end of the record (DynamicsConductor::restoreSector). */
void *car_box_load(CarBox *b, CarBoxSave *buf);                                 /* 0x001FBB50 */
/* 0x228, sizeof(CarBoxSave) (b is not read). */
int  car_box_save_size(CarBox *b);                                              /* 0x001FBCE0 */

#endif /* GAME_T374_CAR_BOX_H */
