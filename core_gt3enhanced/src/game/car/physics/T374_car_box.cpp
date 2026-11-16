/* T374 - one car's collision box (CarBox, 0x4F0 bytes): an octagon (a
 * rectangle with cut corners) in the car's frame, its pose over the frame
 * (pos at t, next at tEnd), the swept bounding box, the impact feedback
 * to the car, and save / restore of its corners. */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DE68
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/car/physics/T374_car_box.h"
#include "game/race/ai/enemy_lines_internal.h"

/* The box of car `index' (mass and inertia: the car's CarDynamics values;
 * the box keeps 1 / mass and 1 / (inertia x 10)).  Caller: boxes_init_car. */
// FUNC 0x001FAE38 car_box_init
void car_box_init(CarBox *b, int index, RaceCourse *course, RaceCarState *car, f32 front,
                  f32 rear, f32 width, f32 chamferFront, f32 chamferRear, f32 mass,
                  f32 inertia)
{
    f32 back = -rear;
    f32 halfWidth = width * 0.5f;
    f32 inertia10 = inertia * 10.0f;
    f32 radius2Max;
    int k;

    b->index = index;
    b->course = course;
    b->car = car;
    b->front = front;
    b->rear = rear;
    b->halfWidth = halfWidth;
    b->chamfer[0] = chamferFront;
    b->chamfer[1] = chamferRear;
    b->mass = mass;
    b->inertia = inertia10;
    /* the corners, counter-clockwise from the front right.  (79.5%: the
     * FP registers of the corner values and of the loop's lx / ly and
     * quotients differ; the order ids, fields, corners, 1 / x is the best
     * of 24 orders, the loop's dir / zero stores of 120, the final stores
     * of 120; the loop indexes b->v[k] / b->e[k] as retail's hoisted field
     * bases show, the two sqrts are the volatile form.) */
    b->v[0].local.x = halfWidth;
    b->v[0].local.y = front - chamferFront;
    b->v[1].local.x = halfWidth - chamferFront;
    b->v[1].local.y = front;
    b->v[2].local.x = -halfWidth + chamferFront;
    b->v[2].local.y = front;
    b->v[3].local.x = -halfWidth;
    b->v[3].local.y = front - chamferFront;
    b->v[4].local.x = -halfWidth;
    b->v[4].local.y = back + chamferRear;
    b->v[5].local.x = -halfWidth + chamferRear;
    b->v[5].local.y = back;
    b->v[6].local.x = halfWidth - chamferRear;
    b->v[6].local.y = back;
    b->v[7].local.x = halfWidth;
    b->v[7].local.y = back + chamferRear;
    b->invMass = 1.0f / mass;
    b->invInertia = 1.0f / inertia10;

    radius2Max = 0.0f;
    for (k = 0; k < 8; k++) {
        BoxVertex *v = &b->v[k];
        BoxVertex *vNext = &b->v[(k + 1) & 7];
        f32 lx = v->local.x;
        f32 ly = v->local.y;
        f32 len = boot_sqrtf_v(lx * lx + ly * ly);

        b->v[k].dir.x = lx / len;
        b->v[k].dir.y = ly / len;
        b->v[k].wall = NULL;
        b->e[k].f1C = 0;
        b->v[k].flags = 0;
        b->e[k].p0 = v->local;
        b->e[k].p1 = vNext->local;

        f32 dx = vNext->local.x - v->local.x;
        f32 dy = vNext->local.y - v->local.y;
        f32 negDx = -dx;
        f32 edgeLen = boot_sqrtf_v(dy * dy + negDx * negDx);
        b->e[k].n.x = dy / edgeLen;
        b->e[k].n.y = negDx / edgeLen;
        b->e[k].d = -b->e[k].n.x * v->local.x - b->e[k].n.y * v->local.y;

        f32 radius2 = v->local.x * v->local.x + v->local.y * v->local.y;
        if (radius2Max < radius2)
            radius2Max = radius2;
    }
    b->radius = boot_sqrtf(radius2Max);
    b->collidable = 1;
    b->initialized = 0;
    b->impactTimer = 0;
    b->impactPeak = 0.0f;
    b->soundTimer = 0;
    b->soundLevel = 0.0f;
}

/* The other boxes and the pair reaches (the sum of the radii, squared).
 * Caller: boxes_link. */
// FUNC 0x001FB0D8 car_box_link
void car_box_link(CarBox *b, CarBox *boxes, int n)
{
    int j;

    b->boxes = boxes;
    for (j = 0; j < n; j++) {
        f32 reach = b->radius + boxes[j].radius;
        b->reach[j] = reach;
        b->reach2[j] = reach * reach;
    }
}

/* The frame's start: the pose and velocity from the car's state, the
 * frame's hit flags and impact cleared; the corners from the last frame's
 * prediction (the first time: placed at the pose).  Callers: boxes_step,
 * boxes_step_car. */
// FUNC 0x001FB140 car_box_begin
void car_box_begin(CarBox *b, f32 dt)
{
    b->dt = dt;
    b->tEnd = 1.0f;
    b->pos.x = b->statePos[0];
    b->pos.y = b->statePos[1];
    b->posZ = b->statePos[2];
    b->yaw = b->stateYaw;
    b->vel.x = b->stateVel[0];
    b->vel.y = b->stateVel[1];
    b->velZ = b->stateVel[2];
    b->yawRate = b->stateYawRate;
    b->hitFront = 0;
    b->hitRear = 0;
    b->hitCar = 0;
    b->hitWall = 0;
    b->crossedSoft = 0;
    b->impact = 0.0f;
    b->t = 0.0f;
    if (b->initialized != 0) {
        car_box_commit_corners(b);
    } else {
        b->initialized = 1;
        car_box_place_corners(b);
    }
}

/* Switching collisions off releases every corner resting on another box
 * (flags bit 1).  Callers: boxes_step, boxes_step_car. */
// FUNC 0x001FB1F0 car_box_set_collidable
void car_box_set_collidable(CarBox *b, int on)
{
    if (b->collidable != 0 && on == 0) {
        int k;
        for (k = 0; k < 8; k++)
            b->v[k].flags &= ~BOX_VERTEX_ON_CAR;
    }
    b->collidable = on;
}

/* The resting contacts: on the other boxes (T375), then along the walls
 * (T376).  Callers: boxes_step, boxes_step_car. */
// FUNC 0x001FB238 car_box_resolve_contacts
void car_box_resolve_contacts(CarBox *b)
{
    car_box_resolve_car_contacts(b);
    car_box_slide_walls(b);
}

/* The pose at tEnd from the velocity over the rest of the frame, the swept
 * bounds, the predicted corners.  Callers: boxes_step, boxes_step_car,
 * car_box_steer_at_rest. */
// FUNC 0x001FB268 car_box_predict
void car_box_predict(CarBox *b)
{
    f32 span = (b->tEnd - b->t) * b->dt;

    b->move.x = b->vel.x * span;
    b->move.y = b->vel.y * span;
    b->moveZ = b->velZ * span;
    b->moveYaw = b->yawRate * span;
    b->next.x = b->pos.x + b->move.x;
    b->next.y = b->pos.y + b->move.y;
    b->nextZ = b->posZ + b->moveZ;
    b->nextYaw = b->yaw + b->moveYaw;
    car_box_sweep_bounds(b);
    car_box_place_next_corners(b);
}

/* Before the hit tests.  Callers: boxes_step, boxes_step_car. */
// FUNC 0x001FB308 car_box_clear_hits
void car_box_clear_hits(CarBox *b)
{
    b->carHit = 0;
    b->wallVertexHit = 0;
    b->wallEdgeHit = 0;
    b->softHit = 0;
    b->softT = 2.0f;
}

/* Moves the pose to time t (that part of the move between t and tEnd), the
 * corners with it; move becomes the part done.  Callers: boxes_step,
 * boxes_step_car. */
// FUNC 0x001FB328 car_box_advance
void car_box_advance(CarBox *b, f32 t)
{
    f32 t0 = b->t;
    f32 span = b->tEnd - t0;
    f32 part;

    if (span != 0.0f)
        part = (t - t0) / span;
    else
        part = 0.0f;
    f32 moveX = b->move.x * part;
    f32 moveY = b->move.y * part;
    f32 moveZ = b->moveZ * part;
    f32 moveYaw = b->moveYaw * part;
    b->t = t;
    b->pos.x = b->pos.x + moveX;
    b->pos.y = b->pos.y + moveY;
    b->posZ = b->posZ + moveZ;
    b->yaw = b->yaw + moveYaw;
    b->move.x = moveX;
    b->move.y = moveY;
    b->moveZ = moveZ;
    b->moveYaw = moveYaw;
    car_box_place_corners(b);
}

/* The predicted corners become the current ones.  Caller: car_box_begin. */
// FUNC 0x001FB3E8 car_box_commit_corners
void car_box_commit_corners(CarBox *b)
{
    int k;

    for (k = 7; k >= 0; k--)
        b->v[7 - k].pos = b->v[7 - k].next;
}

/* The corners' world positions at the pose.  Callers: car_box_begin,
 * car_box_advance, car_box_project_wall_vertex. */
// FUNC 0x001FB420 car_box_place_corners
void car_box_place_corners(CarBox *b)
{
    f32 s, c;                       /* sp+0x0, sp+0x4 */
    int k;

    /* (an index loop: loop.c's four pointer givs; a pointer loop is 40%) */
    sincosf(b->yaw, &s, &c);
    for (k = 0; k < 8; k++) {
        b->v[k].pos.x = (c * b->v[k].local.x - s * b->v[k].local.y) + b->pos.x;
        b->v[k].pos.y = (s * b->v[k].local.x + c * b->v[k].local.y) + b->pos.y;
    }
}

/* The corners' world positions at the predicted pose.  Callers:
 * car_box_predict, car_box_push_wall_cb. */
// FUNC 0x001FB4D8 car_box_place_next_corners
void car_box_place_next_corners(CarBox *b)
{
    f32 s, c;                       /* sp+0x0, sp+0x4 */
    int k;

    sincosf(b->nextYaw, &s, &c);
    for (k = 0; k < 8; k++) {
        b->v[k].next.x = (c * b->v[k].local.x - s * b->v[k].local.y) + b->next.x;
        b->v[k].next.y = (s * b->v[k].local.x + c * b->v[k].local.y) + b->next.y;
    }
}

/* The (x, y) bounds of the move from pos to next, grown by the radius.
 * Caller: car_box_predict. */
// FUNC 0x001FB590 car_box_sweep_bounds
void car_box_sweep_bounds(CarBox *b)
{
    f32 radius = b->radius;

    /* (the stores in both arms: cross-jumped after sched1, they stay
     * before the next test's loads) */
    if (b->move.x < 0.0f) {
        b->bbMin[0] = b->next.x - radius;
        b->bbMax[0] = b->pos.x + radius;
    } else {
        b->bbMin[0] = b->pos.x - radius;
        b->bbMax[0] = b->next.x + radius;
    }
    if (b->move.y < 0.0f) {
        b->bbMin[1] = b->next.y - radius;
        b->bbMax[1] = b->pos.y + radius;
    } else {
        b->bbMin[1] = b->pos.y - radius;
        b->bbMax[1] = b->next.y + radius;
    }
    b->bbSize[0] = b->bbMax[0] - b->bbMin[0];
    b->bbSize[1] = b->bbMax[1] - b->bbMin[1];
}

/* An angle 8 turns or more from 0 (the turns counted to the nearest)
 * brought back by those whole turns; nearer ones are kept.  Caller:
 * car_box_end. */
// FUNC 0x001FB628 wrap_angle
f32 wrap_angle(f32 a)
{
    int turns = (int)((boot_fabsf(a) + U087_PI) / U087_TWO_PI);

    if (turns < 8)
        return a;
    f32 n = (f32)turns;             /* (converted once, before the test) */
    if (a < 0.0f)
        return a + n * U087_TWO_PI;
    return a + n * -U087_TWO_PI;
}

/* |w| below 20 pi rad/s, else +-20 pi.  Caller: car_box_end. */
// FUNC 0x001FB698 clamp_yaw_rate
f32 clamp_yaw_rate(f32 w)
{
    if (boot_fabsf(w) < U087_YAW_RATE_MAX)
        return w;
    return (w < 0.0f) ? -U087_YAW_RATE_MAX : U087_YAW_RATE_MAX;
}

/* The frame's end: the predicted pose and the velocity back to the car's
 * state.  Callers: boxes_step, boxes_step_car. */
// FUNC 0x001FB6D8 car_box_end
void car_box_end(CarBox *b)
{
    b->statePos[0] = b->next.x;
    b->statePos[1] = b->next.y;
    b->statePos[2] = b->nextZ;
    b->stateVel[0] = b->vel.x;
    b->stateVel[1] = b->vel.y;
    b->stateVel[2] = b->velZ;
    b->stateYaw = wrap_angle(b->nextYaw);
    b->stateYawRate = clamp_yaw_rate(b->yawRate);
}

/* A (nearly) stopped car (|vx|, |vy| < 1 m/s) that was hit early in the
 * frame (t < 0.1) turns with its steering: yaw rate + steering / 100 x 20 x
 * dt, then predicted again.  Callers: boxes_step, boxes_step_car. */
// FUNC 0x001FB740 car_box_steer_at_rest
void car_box_steer_at_rest(CarBox *b, f32 t)
{
    if (boot_fabsf(b->vel.x) < 1.0f && boot_fabsf(b->vel.y) < 1.0f && t < 0.1f) {
        b->yawRate = b->yawRate + ((f32)b->car->st.steering / 100.0f) * 20.0f * b->dt;
        car_box_predict(b);
    }
}

/* Moves pos and the corners' pos by (dx, dy).  Caller: car_box_push_out_car. */
// FUNC 0x001FB7E8 car_box_translate
void car_box_translate(CarBox *b, f32 dx, f32 dy)
{
    int k;

    b->pos.x = b->pos.x + dx;
    b->pos.y = b->pos.y + dy;
    for (k = 7; k >= 0; k--) {
        BoxVertex *v = &b->v[7 - k];
        v->pos.x = v->pos.x + dx;
        v->pos.y = v->pos.y + dy;
    }
}

/* Callers: car_update_pit, DynamicsConductor::updateCar. */
// FUNC 0x001FB838 car_box_has_hit
int car_box_has_hit(CarBox *b)
{
    int hit = 0;

    if (b->hitWall != 0 || b->hitCar != 0)
        hit = 1;
    return hit;
}

/* Caller: car_update_pit. */
// FUNC 0x001FB860 car_box_hit_wall
int car_box_hit_wall(CarBox *b)
{
    return b->hitWall;
}

/* (no callers) */
// FUNC 0x001FB868 car_box_hit_car
int car_box_hit_car(CarBox *b)
{
    return b->hitCar;
}

/* Caller: ai_drive. */
// FUNC 0x001FB870 car_box_get_zero
int car_box_get_zero(CarBox *b)
{
    return 0;
}

/* Caller: ai_drive. */
// FUNC 0x001FB878 car_box_hit_front
int car_box_hit_front(CarBox *b)
{
    return b->hitFront;
}

/* Caller: ai_drive. */
// FUNC 0x001FB880 car_box_hit_rear
int car_box_hit_rear(CarBox *b)
{
    return b->hitRear;
}

/* (no callers) */
// FUNC 0x001FB888 car_box_crossed_soft
int car_box_crossed_soft(CarBox *b)
{
    return b->crossedSoft;
}

/* The frame's collision strength (impact, capped at 1) to the car: a crash
 * sound request held 60 frames (car_crash_sound_add with kind -1, which
 * does nothing: the request is a no-op every frame) and the car's impact
 * (DynCarState +0x4F0) held 24 frames at the peak.  Callers: boxes_step,
 * boxes_step_car. */
// FUNC 0x001FB890 car_box_impact_feedback
void car_box_impact_feedback(CarBox *b, RaceCarState *car)
{
    f32 level = b->impact;
    f32 impact;

    if (0.0f < level && (b->soundTimer == 0 || b->soundLevel < level)) {
        level = level + level;
        if (1.0f < level)
            level = 1.0f;
        car_crash_sound_add(car, level, -1, -1);
        impact = b->impact;
        b->soundTimer = 60;
        b->soundLevel = impact;
    } else {
        car_crash_sound_add(car, 0.0f, -1, -1);
        b->soundTimer = b->soundTimer - 1;
        if (b->soundTimer < 0)
            b->soundTimer = 0;
        impact = b->impact;
        b->soundLevel = b->soundLevel * 0.98f;
    }
    if (1.0f < impact) {
        b->impact = 1.0f;
        impact = 1.0f;
    }
    if (b->impactTimer > 0) {
        int left = b->impactTimer - 1;
        if (b->impactPeak < impact) {
            if (car->st.impact < impact) {
                car->st.impact = impact;
                impact = b->impact;
            }
            b->impactPeak = impact;
            b->impactTimer = 24;
            return;
        }
        b->impactTimer = left;
        if (left == 0)
            car->st.impact = 0.0f;
    } else if (0.0f < impact) {
        car->st.impact = impact;
        b->impactTimer = 24;
        b->impactPeak = b->impact;
    }
}

/* Saves the 8 corners (the wall pointers as indices of the runway's wall
 * records, Runway +0x3C); returns the end of the record.  Caller:
 * DynamicsConductor::saveSector. */
// FUNC 0x001FBA08 car_box_save
void *car_box_save(CarBox *b, CarBoxSave *buf)
{
    /* The record written through a cursor past its two words (retail
     * advances the buffer register itself: buf + 8, the end + 0x220 from
     * there; with buf->v CSE keeps buf and returns buf + 0x228). */
    int *p = (int *)buf;
    int k;

    p[1] = car_box_save_size(b);                    /* size */
    WallSeg *walls = b->course->data->runway->walls;
    p[0] = 0;                                       /* skip */
    p += 2;
    for (k = 0; k < 8; k++) {
        ((BoxVertex *)p)[k] = b->v[k];
        ((BoxVertex *)p)[k].wall = (WallSeg *)(((BoxVertex *)p)[k].wall - walls);
    }
    return (BoxVertex *)p + 8;
}

/* Restores the corners saved by car_box_save, keeping the box's own shape
 * (local, dir); returns the end of the record.  A skip record is passed
 * over by its size, counted from after the two words (8 bytes further than
 * the end car_box_save returns).  Caller: DynamicsConductor::restoreSector. */
// FUNC 0x001FBB50 car_box_load
void *car_box_load(CarBox *b, CarBoxSave *buf)
{
    /* (read through a cursor as car_box_save writes it) */
    int *p = (int *)buf;
    int k;
    int skip = p[0];
    int size = p[1];

    p += 2;
    if (skip != 0)
        return (u8 *)p + size;
    WallSeg *walls = b->course->data->runway->walls;
    for (k = 0; k < 8; k++) {
        Vec2 local = b->v[k].local;     /* sp+0x0 */
        Vec2 dir = b->v[k].dir;         /* sp+0x10 */
        b->v[k] = ((BoxVertex *)p)[k];
        b->v[k].wall = walls + (s32)((BoxVertex *)p)[k].wall;
        b->v[k].local = local;
        b->v[k].dir = dir;
    }
    return (BoxVertex *)p + 8;
}

/* sizeof(CarBoxSave); b is not read.  Callers: DynamicsConductor::saveSector,
 * car_box_save. */
// FUNC 0x001FBCE0 car_box_save_size
int car_box_save_size(CarBox *b)
{
    return 0x228;                   /* sizeof(CarBoxSave) */
}
