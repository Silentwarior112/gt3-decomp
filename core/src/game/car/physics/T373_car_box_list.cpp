/* T373 - the car box list (DynamicsConductor +0xBBB4): one CarBox per car,
 * built from the cars, and the collision step of a frame. */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DE40
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/car/physics/T373_car_box_list.h"
#include "game/race/ai/enemy_lines_internal.h"

/* Callers: boxes_init_car, boxes_step, boxes_step_car. */
// FUNC 0x001FA680 boxes_car
RaceCarState *boxes_car(CarBoxList *l, int i)
{
    return l->dc->getCar(i);
}

/* Caller: DynamicsConductor::virtual_21 (0x001F30D8). */
// FUNC 0x001FA6A0 boxes_init
void boxes_init(CarBoxList *l, DynamicsConductor *dc, RaceCourse *course)
{
    int i;

    l->course = course;
    l->dc = dc;
    l->count = dc->carCount;
    for (i = 0; i < l->count; i++)
        boxes_init_car(l, i);
    boxes_link(l);
}

/* Caller: DynamicsConductor::restoreSector. */
// FUNC 0x001FA710 boxes_reset_car
void boxes_reset_car(CarBoxList *l, int i)
{
    boxes_init_car(l, i);
    boxes_link(l);
}

/* Box i from car i: its CarDynamics' front / rear lengths, width (the corner
 * cuts 0.2 x width), mass and yaw inertia (the car fetched for each value).
 * Callers: boxes_init, boxes_reset_car. */
// FUNC 0x001FA740 boxes_init_car
void boxes_init_car(CarBoxList *l, int i)
{
    CarBox *box = &l->box[i];       /* (taken first: retail keeps it in $s2) */
    f32 front = boxes_car(l, i)->params.lenFront;
    f32 rear = boxes_car(l, i)->params.lenRear;
    f32 width = boxes_car(l, i)->params.width;
    f32 chamferFront = boxes_car(l, i)->params.width * 0.2f;
    f32 chamferRear = boxes_car(l, i)->params.width * 0.2f;
    f32 mass = boxes_car(l, i)->params.mass;
    f32 inertia = boxes_car(l, i)->params.yawInertia;
    RaceCarState *car = boxes_car(l, i);

    car_box_init(box, i, l->course, car, front, rear, width, chamferFront, chamferRear, mass,
                 inertia);
}

/* count = dc->carCount; every box gets the list and the pair reaches.
 * Callers: boxes_init, boxes_reset_car. */
// FUNC 0x001FA870 boxes_link
void boxes_link(CarBoxList *l)
{
    int i;

    l->count = l->dc->carCount;
    for (i = 0; i < l->count; i++)
        car_box_link(&l->box[i], l->box, l->count);
}

/* The end of a box's frame (the steering at rest, the walls, the state back
 * to the car, the impact feedback).  Inline: retail recomputes &box[i] from
 * its own copies of l and i after boxes_step_car's loop, where GCSE would
 * otherwise reuse the loop's register. */
static inline void boxes_finish(CarBoxList *l, int i, f32 t)
{
    CarBox *b = &l->box[i];

    car_box_steer_at_rest(b, t);
    car_box_push_out_walls(b);
    car_box_end(b);
    car_box_impact_feedback(b, boxes_car(l, i));
}

/* The collision step of all cars: the cars' states in (a car without a
 * controller is not collidable), then up to two passes of: the static
 * overlaps pushed apart, the resting contacts, the prediction, the
 * detection of the earliest hit (a wall hit coded box + 0x200, a car hit
 * the box), every box advanced to it, the hit responded to; then the rest of
 * the frame, the states back to the cars and the impact feedback.
 * Caller: DynamicsConductor::collideAllCars. */
// FUNC 0x001FA8F8 boxes_step
void boxes_step(CarBoxList *l)
{
    f32 hitTime = 1.0f;
    int i, j, pass;

    for (i = 0; i < l->count; i++) {
        car_box_begin(&l->box[i], boxes_car(l, i)->dt);
        car_box_set_collidable(&l->box[i], car_no_controller(boxes_car(l, i)) == 0);
    }

    pass = 0;
    do {
        f32 earliest;
        int hit;

        for (i = 0; i < l->count; i++) {
            for (j = 0; j < l->count; j++) {
                if (i != j)
                    car_box_push_out_car(&l->box[i], j);
            }
        }
        for (i = 0; i < l->count; i++)
            car_box_resolve_contacts(&l->box[i]);
        for (i = 0; i < l->count; i++)
            car_box_predict(&l->box[i]);

        hit = -1;
        earliest = 2.0f;
        for (i = 0; i < l->count; i++) {
            car_box_clear_hits(&l->box[i]);
            if (car_box_sweep_walls(&l->box[i]) != 0) {
                if (l->box[i].hitTime < earliest) {
                    earliest = l->box[i].hitTime;
                    hit = i + BOX_HIT_WALL;
                }
            }
            for (j = 0; j < l->count; j++) {
                if (i != j)
                    car_box_sweep_car(&l->box[i], j);
            }
            if (l->box[i].carHit != 0) {
                if (l->box[i].hitTime < earliest) {
                    earliest = l->box[i].hitTime;
                    hit = i;
                }
            }
        }
        /* BUG (as compiled): the box is the stale loop counter i (== count
         * here, one past the last box), so box[count] gets the call count
         * times and no car's soft-wall crossing (crossedSoft) is checked in
         * this step.  With 8 cars box[8] lies past the list (DynamicsConductor
         * +0xE340, its rules) and its softHit (+0x400) past the conductor. */
        for (j = 0; j < l->count; j++)
            car_box_check_soft_wall(&l->box[i], earliest);

        if (hit < 0)
            break;
        hitTime = earliest;
        for (i = 0; i < l->count; i++)
            car_box_advance(&l->box[i], earliest);
        if (hit & BOX_HIT_WALL) {
            if (l->box[hit & ~BOX_HIT_WALL].wallVertexHit != 0)
                car_box_respond_wall_vertex(&l->box[hit & ~BOX_HIT_WALL]);
            else
                car_box_respond_wall_edge(&l->box[hit & ~BOX_HIT_WALL]);
        } else {
            car_box_respond_car(&l->box[hit]);
        }
        pass++;
    } while (pass < 2);

    /* implicit: car_box_steer_at_rest, car_box_push_out_walls, car_box_end,
     * car_box_impact_feedback (boxes_finish) */
    for (i = 0; i < l->count; i++)
        boxes_finish(l, i, hitTime);
}

/* The collision step of car i alone: walls only (not collidable with the
 * other cars), up to two hits.  Caller: DynamicsConductor::collideWalls. */
// FUNC 0x001FACB8 boxes_step_car
void boxes_step_car(CarBoxList *l, int i)
{
    f32 hitTime = 1.0f;
    int hits = 0;

    car_box_begin(&l->box[i], boxes_car(l, i)->dt);
    car_box_set_collidable(&l->box[i], 0);
    do {
        car_box_resolve_contacts(&l->box[i]);
        car_box_predict(&l->box[i]);
        car_box_clear_hits(&l->box[i]);
        if (car_box_sweep_walls(&l->box[i]) == 0) {
            car_box_check_soft_wall(&l->box[i], 2.0f);
            break;
        }
        hitTime = l->box[i].hitTime;
        car_box_check_soft_wall(&l->box[i], hitTime);
        car_box_advance(&l->box[i], hitTime);
        if (l->box[i].wallVertexHit != 0)
            car_box_respond_wall_vertex(&l->box[i]);
        else
            car_box_respond_wall_edge(&l->box[i]);
        hits++;
    } while (hits < 2);
    /* implicit: car_box_steer_at_rest, car_box_push_out_walls, car_box_end,
     * car_box_impact_feedback (boxes_finish) */
    boxes_finish(l, i, hitTime);
}
