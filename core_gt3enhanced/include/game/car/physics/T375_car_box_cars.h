/* T375_car_box_cars.h - BoxEdge and the car against car collisions of the
 * car boxes (translation unit T375, unit u088), 0x001FBCE8..0x001FCF48.
 *
 * The collision boxes of the cars (CarBox, T376; the list and the frame step
 * are T373 / T374 in u087): an eight-corner polygon in the plane (x, y), z
 * up.  The other box of a test is b->boxes[other].  The swept test
 * (car_box_sweep_car) takes the other box's corners into this box's local
 * frame at both ends of the step and finds the earliest crossing of one of
 * this box's edges; car_box_respond_car then applies an impulse with a
 * restitution of 0.5 and pushes the two boxes apart.  A rotation `rot' is a
 * row-major 2x2 {cos, -sin, sin, cos} of a box's yaw: world = rot * local.
 */
#ifndef GAME_T375_CAR_BOX_CARS_H
#define GAME_T375_CAR_BOX_CARS_H

#include "game/game.h"
#include "game/car/physics/T374_car_box.h"              /* Vec2 (held by value) */

struct CarBox;                              /* T376 (pointers only) */
struct WallSeg;                             /* T167 */

/* An edge of a car box, corner k to corner k + 1 (CarBox::e[k]); p0 is also
 * the local position of corner k.  0x2C bytes. */
struct BoxEdge {
    /* Inline and empty: CarBox's constructor (0x001F42E0) loops over e[8]
     * with nothing in the loop. */
    BoxEdge() {}

    Vec2     p0;                            /* 0x00 local corner k */
    Vec2     p1;                            /* 0x08 local corner k + 1 */
    Vec2     n;                             /* 0x10 the outward normal (dy, -dx) / length */
    f32      d;                             /* 0x18 -(n . p0) */
    int      f1C;                           /* 0x1C */
    int      hit;                           /* 0x20 a wall vertex crosses it (car_box_sweep_walls) */
    f32      t;                             /* 0x24 the crossing's part of the step (2.0: none) */
    WallSeg *wall;                          /* 0x28 that wall vertex */
};
EGCS_SIZE(BoxEdge, 0x2C);

/* Corners resting on an edge of another box (flags bit 1): released when
 * they leave the edge, the boxes pushed apart when inside, the approaching
 * velocity along the edge normal shared half and half (T374). */
void car_box_resolve_car_contacts(CarBox *b);                           /* 0x001FBCE8 */
/* The swept test against box `other'; 1: a hit (carHit, hitTime, carEdge,
 * carOther, carVertex, carContact) earlier than the ones found so far. */
int  car_box_sweep_car(CarBox *b, int other);                           /* 0x001FBF50 */
/* The response to the hit car_box_sweep_car stored: the impulse, both
 * boxes' impact, the separation, the velocity and yaw-rate changes, the
 * other corner marked as resting on the edge, a hit of both cars (T288). */
void car_box_respond_car(CarBox *b);                                    /* 0x001FC5C0 */
/* The impulse of the stored car hit (restitution 0.5): *imp = j n; returns
 * j.  rotA / rotB: the rotations of this box and the other (no callers
 * outside T375). */
f32  car_box_car_impulse(CarBox *b, Vec2 *imp, const f32 *rotA, const f32 *rotB);  /* 0x001FC840 */
/* the other corner still inside the edge line: both boxes moved apart by
 * half the depth (pos only; no callers outside T375) */
void car_box_separate_car(CarBox *b, const f32 *rotA, const f32 *rotB); /* 0x001FCB20 */
/* The static overlap with box `other': the deepest corner of it inside this
 * box moves the two boxes apart, half each (T373 boxes_step). */
void car_box_push_out_car(CarBox *b, int other);                        /* 0x001FCC98 */

#endif /* GAME_T375_CAR_BOX_CARS_H */
