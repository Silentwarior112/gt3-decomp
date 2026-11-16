/* T373_car_box_list.h - the car box list: one collision box per car and the
 * collision step of a frame (translation unit T373, unit u087),
 * 0x001FA680..0x001FAE38.
 *
 * CarBoxList (T167_rigid_body.h; u083 CarAuxSet, u085 CourseGrid +0xC) is
 * DynamicsConductor +0xBBB4: {dc, count, course, CarBox box[8]} (+0x0, +0x4,
 * +0x8, +0xC; CarBox 0x4F0, T376_car_box_walls.h).  boxes_step runs all cars:
 * the cars' states in, up to two passes of predict / detect the earliest hit
 * (T375 / T376) / advance everyone to it / resolve it, then the rest of the
 * frame, the states back to the cars and the impact feedback (T374).
 * boxes_step_car does one car alone (walls only). */
#ifndef GAME_T373_CAR_BOX_LIST_H
#define GAME_T373_CAR_BOX_LIST_H

#include "game/game.h"

struct CarBoxList;                          /* T167_rigid_body.h (pointers only) */
class DynamicsConductor;                    /* T305_dynamics_conductor.h */
class RaceCourse;                           /* T153_race_course.h */
class RaceCarState;                         /* T302_car_state.h (u087: RaceCarDynamics, T154) */

/* DynamicsConductor::getCar(l->dc, i) (F_001F2B68). */
RaceCarState *boxes_car(CarBoxList *l, int i);                                  /* 0x001FA680 */
/* course, dc, count = dc->carCount, every box (boxes_init_car), boxes_link
 * (DynamicsConductor::virtual_21). */
void boxes_init(CarBoxList *l, DynamicsConductor *dc, RaceCourse *course);      /* 0x001FA6A0 */
/* Box i built again from car i, then boxes_link (DynamicsConductor::
 * restoreSector, F_001EE6D8). */
void boxes_reset_car(CarBoxList *l, int i);                                     /* 0x001FA710 */
/* Box i from car i: its front / rear lengths, width (corner cuts 0.2 x
 * width), mass and yaw inertia (car_box_init). */
void boxes_init_car(CarBoxList *l, int i);                                      /* 0x001FA740 */
/* count = dc->carCount; every box gets the list and the pair reaches. */
void boxes_link(CarBoxList *l);                                                 /* 0x001FA870 */
/* The collision step of all cars (DynamicsConductor::collideAllCars).  A car
 * without a controller (car_no_controller) is not collidable.  As compiled,
 * the soft-wall check of each pass runs count times on the stale box[count]
 * instead of on every box (a bug: no car's crossedSoft is set here). */
void boxes_step(CarBoxList *l);                                                 /* 0x001FA8F8 */
/* The collision step of car i alone, not collidable with the other cars
 * (collideWalls). */
void boxes_step_car(CarBoxList *l, int i);                                      /* 0x001FACB8 */

#endif /* GAME_T373_CAR_BOX_LIST_H */
