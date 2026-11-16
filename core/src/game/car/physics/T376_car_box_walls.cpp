/* T376 - the car collision boxes against the runway walls.
 *
 * A wall is a polyline of WallSeg vertices; RaceCourse::forEachPoint calls a
 * callback for every vertex (segment start) in a box of the plane.  The
 * box's plane is (x, y) = (wall.x, -wall.z).  The swept test
 * (car_box_sweep_walls) checks the corners' paths against the segments and
 * the wall vertices against the box's moving edges and keeps the earliest
 * crossing; car_box_respond_wall_vertex / _edge apply an impulse with a
 * restitution of 0.05 plus friction.
 */
/* Retail T376's .rodata (0x0033DEB8): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_box_internal.h"
#include "game/car/physics/T376_car_box_walls.h"
#include "car_box_vec2.h"
// DATA_START 0x0033DEB8

/* egcs "b |= true" on a 4-byte bool: (b | v) != 0 */
static inline int bool_or(int b, int v)
{
    return (b | v) != 0;
}

/* the vertex after w on its polyline */
static inline WallSeg *wall_next(WallSeg *w)
{
    WallSeg *n = w + 1;
    if (w->link < 0)
        n += w->link;
    return n;
}

/* the vertex before w on its polyline */
static inline WallSeg *wall_prev(WallSeg *w)
{
    WallSeg *p = w - 1;
    if (w->link > 0)
        p += w->link;
    return p;
}

/* the z of (ax, ay) x (bx, by) */
static inline f32 cross2(f32 ax, f32 ay, f32 bx, f32 by)
{
    return ax * by - ay * bx;
}

// FUNC 0x001FCF48 car_box_slide_walls
//   For each corner sliding along a wall (flags bit 0): when its position
//   has moved past the start (projection < 0) or the end of its segment,
//   the corner moves to the previous / next segment (wall, unit wallDir;
//   (1, 0) for a zero-length one).  When the box is still moving into the
//   wall (vel x wallDir >= 0) and the corner is on the inner side, the box is
//   moved back so the corner is on the segment, its velocity is projected
//   on the box's stored wall direction (wallDir +0x420), and the yaw rate
//   loses the corresponding spin (lever x the lost velocity x the mass, x
//   invInertia: an angular impulse).  Otherwise the
//   corner's bit 0 is cleared.  Caller: car_box_resolve_contacts (T374).
//   Retail's forms (ref/matching/u088/notes.md, follow-up 2026-10-17): the
//   flags are read before corner is set; the previous segment in its own
//   `prev' (`other' keeps one value); the update in both branches and the
//   clear in both else arms (cross-jumped into one in retail).
void car_box_slide_walls(CarBox *b)
{
    for (int i = 0; i < 8; i++) {
        u32 flags = b->v[i].flags;
        BoxVertex *corner = &b->v[i];
        if (!(flags & BOX_VERTEX_ON_WALL))
            continue;

        WallSeg *wall = b->v[i].wall;
        WallSeg *other = wall_next(wall);       /* the segment's end vertex */
        Vec2 rel;
        vec2_set(&rel, b->v[i].pos.x - wall->x, b->v[i].pos.y + wall->z);
        f32 along = rel.x * corner->wallDir.x + rel.y * corner->wallDir.y;

        if (along < 0.0f) {
            /* before the start: the previous segment, prev -> wall */
            WallSeg *prev = wall_prev(wall);
            Vec2 dir;
            vec2_set(&dir, wall->x - prev->x, -wall->z + prev->z);
            f32 len = boot_sqrtf_v(dir.x * dir.x + dir.y * dir.y);
            if (len != 0.0f) {
                f32 inv = 1.0f / len;
                dir.x = dir.x * inv;
                dir.y = dir.y * inv;
            } else {
                dir.y = 0.0f;
                dir.x = 1.0f;
            }
            corner->wallDir = dir;
            b->v[i].wall = prev;
            wall = prev;
            vec2_set(&rel, b->v[i].pos.x - wall->x, b->v[i].pos.y + wall->z);
            along = vec2_dot(&rel, &dir);
        } else if ((other->x - wall->x) * corner->wallDir.x -
                   (other->z - wall->z) * corner->wallDir.y < along) {
            /* past the end: the next segment, other -> other's next */
            WallSeg *after = wall_next(other);
            Vec2 dir;
            vec2_set(&dir, after->x - other->x, -after->z + other->z);
            f32 len = boot_sqrtf_v(dir.x * dir.x + dir.y * dir.y);
            if (len != 0.0f) {
                f32 inv = 1.0f / len;
                dir.x = dir.x * inv;
                dir.y = dir.y * inv;
            } else {
                dir.y = 0.0f;
                dir.x = 1.0f;
            }
            corner->wallDir = dir;
            b->v[i].wall = other;
            wall = other;
            vec2_set(&rel, b->v[i].pos.x - wall->x, b->v[i].pos.y + wall->z);
            along = vec2_dot(&rel, &dir);
        }

        if (0.0f <= vec2_cross(&b->vel, &b->v[i].wallDir)) {
          if (0.0f < vec2_cross(&rel, &b->v[i].wallDir)) {
            Vec2 push;
            vec2_set(&push, (wall->x + b->v[i].wallDir.x * along) - b->v[i].pos.x,
                     (-wall->z + b->v[i].wallDir.y * along) - b->v[i].pos.y);
            vec2_add(&b->pos, &b->pos, &push);

            Vec2 lost = b->vel;
            f32 velAlong = vec2_dot(&b->wallDir, &b->vel);
            b->vel.x = velAlong * b->wallDir.x;
            b->vel.y = velAlong * b->wallDir.y;
            vec2_sub(&lost, &lost, &b->vel);
            Vec2 lever = b->v[i].pos;
            vec2_sub(&lever, &lever, &b->pos);
            b->yawRate -= vec2_cross(&lever, &lost) * b->mass * b->invInertia;
          } else {
            b->v[i].flags &= ~(u32)BOX_VERTEX_ON_WALL;
          }
        } else {
            b->v[i].flags &= ~(u32)BOX_VERTEX_ON_WALL;
        }
    }
}

// FUNC 0x001FD368 car_box_sweep_wall_cb
//   The RaceCourse::forEachPoint callback of car_box_sweep_walls for the
//   segment w -> next.  Each free corner (flags 0) whose path pos -> next
//   goes from the outer side (> 0) to the inner side (< 0) of the segment,
//   within it, crosses at s = d0 / (d0 - d1) (1 when equal); the earliest
//   crossing is kept: on a solid wall in the corner (t, hit, wall, wallDir:
//   the unit segment direction, (1, 0) when zero), on a non-solid one
//   (flags bit 1) in softT / softHit.  When the wall vertex is a tested one
//   (flags bit 0) the vertex itself is swept against the box's moving edges
//   (corner pos -> next): the earliest crossing goes to the edge before it
//   (edge (i + 7) & 7: t, wall, hit) or, non-solid, to softT / softHit.
//   Retail's forms (ref/matching/u088/notes.md, follow-up 2026-10-17): one
//   counter i for both loops, the first counting up (loop.c reverses it);
//   the side tests through cross2 (its arguments are computed in order);
//   edge before rel in the second loop.  w->flags is read again after the
//   first loop (PRE's copy, zero-extended, keeps the test after the loop).
void car_box_sweep_wall_cb(WallSeg *w, CarBox *b)
{
    /* the segment a -> c in the box plane */
    WallSeg *end = wall_next(w);
    f32 ax = w->x;
    f32 ay = -w->z;
    f32 cx = end->x;
    f32 cy = -end->z;
    f32 segX = cx - ax;
    f32 segY = cy - ay;
    int soft = w->flags & WALL_SOFT;

    BoxVertex *corner = b->v;
    int i;
    for (i = 0; i < 8; i++, corner++) {
        if (corner->flags != 0)
            continue;
        f32 nextX = corner->next.x;
        f32 nextY = corner->next.y;
        f32 side1 = cross2(segX, segY, nextX - ax, nextY - ay);
        if (0.0f <= side1)
            continue;                       /* the end of the path is not inside */
        f32 posX = corner->pos.x;
        f32 posY = corner->pos.y;
        f32 relX = posX - ax;
        f32 relY = posY - ay;
        f32 side0 = cross2(segX, segY, relX, relY);
        if (side0 <= 0.0f)
            continue;                       /* the start is not outside */
        f32 pathX = nextX - posX;
        f32 pathY = nextY - posY;
        if (cross2(pathX, pathY, relX, relY) < 0.0f)
            continue;                       /* the path passes beyond a */
        if (0.0f < cross2(pathX, pathY, posX - cx, posY - cy))
            continue;                       /* the path passes beyond c */
        f32 den = side0 - side1;
        f32 s = 1.0f;
        if (den != 0.0f)
            s = side0 / den;
        if (!(s < corner->t))
            continue;
        if (soft != 0) {
            if (!(s < b->softT))
                continue;
            b->softT = s;
            b->softHit = 1;
        } else {
            f32 len = boot_sqrtf_v(segX * segX + segY * segY);
            corner->t = s;
            corner->hit = 1;
            corner->wall = w;
            if (len == 0.0f) {
                corner->wallDir.y = 0.0f;
                corner->wallDir.x = 1.0f;
            } else {
                corner->wallDir.x = segX / len;
                corner->wallDir.y = segY / len;
            }
        }
    }

    if (!(w->flags & WALL_TEST_VERTEX))
        return;

    /* the wall vertex a against the moving edges prev -> cur */
    BoxVertex *prev = &b->v[7];
    BoxVertex *cur = b->v;
    for (i = 0; i < 8; i++, prev = cur, cur++) {
        f32 side1 = cross2(cur->next.x - prev->next.x, cur->next.y - prev->next.y,
                           ax - prev->next.x, ay - prev->next.y);
        if (side1 <= 0.0f)
            continue;                       /* not inside the edge at the end */
        f32 edgeX = cur->pos.x - prev->pos.x;
        f32 edgeY = cur->pos.y - prev->pos.y;
        f32 relX = ax - prev->pos.x;
        f32 relY = ay - prev->pos.y;
        f32 side0 = cross2(edgeX, edgeY, relX, relY);
        if (0.0f <= side0)
            continue;                       /* not outside it at the start */
        f32 movePrevX = prev->next.x - prev->pos.x;   /* the motion of prev */
        f32 movePrevY = prev->next.y - prev->pos.y;
        f32 moveCurX = cur->next.x - cur->pos.x;      /* the motion of cur */
        f32 moveCurY = cur->next.y - cur->pos.y;
        f32 turnPrev = cross2(movePrevX, movePrevY, edgeX, edgeY);
        f32 sidePrev = cross2(movePrevX, movePrevY, relX, relY);
        f32 sideCur = cross2(moveCurX, moveCurY, ax - cur->pos.x, ay - cur->pos.y);
        if (turnPrev < 0.0f) {
            if (0.0f < sidePrev)
                continue;
            if (0.0f < sideCur)
                continue;
        }
        if (cross2(moveCurX, moveCurY, edgeX, edgeY) < 0.0f) {
            if (sidePrev < 0.0f)
                continue;
            if (sideCur < 0.0f)
                continue;
        }
        if (sidePrev < 0.0f)
            continue;
        if (0.0f < sideCur)
            continue;
        f32 den = side0 - side1;
        f32 s = 1.0f;
        if (den != 0.0f)
            s = side0 / den;
        if (soft != 0) {
            if (!(s < b->softT))
                continue;
            b->softT = s;
            b->softHit = 1;
        } else {
            BoxEdge *edge = &b->e[(i + 7) & 7];
            if (!(s < edge->t))
                continue;
            edge->t = s;
            edge->hit = 1;
            edge->wall = w;
        }
    }
}

// FUNC 0x001FD720 car_box_sweep_walls
//   Resets the corners' and edges' crossings (hit 0, t 2.0), visits the walls
//   in the bounding box of the corners' paths (car_box_sweep_wall_cb), and
//   keeps the earliest solid crossing: a free corner (index i) or an edge
//   (index 8 + j).  hitTime = t + (tEnd - t) * s; a corner hit sets
//   wallVertexHit, wallVertex, wallSeg, wallDir and hitWall, an edge hit
//   wallEdgeHit, wallEdge (j) and hitWall; hitFront / hitRear by the index.
//   Returns 1 when a crossing was found.  Callers: boxes_step, boxes_step_car (T373).
int car_box_sweep_walls(CarBox *b)
{
    BoxVertex *corner = b->v;
    BoxEdge *edge = b->e;
    f32 maxX = corner->pos.x;
    f32 maxY = corner->pos.y;
    f32 minX = maxX;
    f32 minY = maxY;
    for (int i = 0; i < 8; i++) {
        f32 x = corner->pos.x;
        f32 y = corner->pos.y;
        if (x < minX) minX = x;
        if (maxX < x) maxX = x;
        if (y < minY) minY = y;
        if (maxY < y) maxY = y;
        if (corner->next.x < minX) minX = corner->next.x;
        if (maxX < corner->next.x) maxX = corner->next.x;
        if (corner->next.y < minY) minY = corner->next.y;
        if (maxY < corner->next.y) maxY = corner->next.y;
        corner->hit = 0;
        corner->t = BOX_NO_HIT;
        edge->hit = 0;
        corner++;
        edge->t = BOX_NO_HIT;
        edge++;
    }
    /* RaceCourse::forEachPoint (0x0018FD00); the course frame: z = -y */
    b->course->forEachPoint((RunwayPointFunc)car_box_sweep_wall_cb, b, b->sector,
                            minX, -maxY, maxX, -minY);

    f32 best = BOX_NO_HIT;
    int hit = -1;                           /* a corner i, or an edge 8 + j */
    corner = b->v;
    edge = b->e;
    for (int i = 0; i < 8; i++, corner++, edge++) {
        if (corner->hit != 0 && corner->flags == 0 && corner->t < best) {
            best = corner->t;
            hit = i;
        }
        if (edge->hit != 0 && edge->t < best) {
            best = edge->t;
            hit = i + 8;
        }
    }
    if (hit < 0)
        return 0;

    b->hitTime = (b->tEnd - b->t) * best + b->t;
    if (hit < 8) {
        b->wallVertexHit = 1;
        b->wallVertex = hit;
        b->wallSeg = b->v[hit].wall;
        b->wallDir = b->v[hit].wallDir;
        b->hitWall = bool_or(b->hitWall, 1);
        if (b->wallVertex < BOX_HALF)
            b->hitFront = bool_or(b->hitFront, 1);
        else
            b->hitRear = bool_or(b->hitRear, 1);
    } else {
        b->wallEdgeHit = 1;
        b->wallEdge = hit - 8;
        b->hitWall = bool_or(b->hitWall, 1);
        /* quirk: the code tests hit (8..15), not the edge, so neither flag is
         * ever set here */
        if (hit < BOX_FRONT_END)
            b->hitFront = bool_or(b->hitFront, 1);
        if ((u32)(hit - BOX_REAR_START) < 3)
            b->hitRear = bool_or(b->hitRear, 1);
    }
    return 1;
}

// FUNC 0x001FD9D0 car_box_check_soft_wall
//   crossedSoft = 1 when a non-solid wall was crossed before time t.
//   Callers: boxes_step, boxes_step_car (T373).
void car_box_check_soft_wall(CarBox *b, f32 t)
{
    if (b->softHit != 0) {
        if ((b->tEnd - b->t) * b->softT + b->t < t)
            b->crossedSoft = 1;
    }
}

// FUNC 0x001FDA10 car_box_push_wall_cb
//   The callback of car_box_push_out_walls for the segment w -> n: for the
//   16 pairs (corner i, previous corner) and (corner i, next corner) of
//   free corners (flags 0), at the predicted pose (next), when the corner is
//   inside the wall line and the pair's segment straddles it within the
//   wall: beyond either end of the wall the predicted pose moves the wall
//   end out of the pair's segment; otherwise the corner is moved onto the
//   wall line (when off by more than 0.5, the pose moves along the wall
//   instead, to the end the pair is on); in the last case the corner then
//   slides along this wall (flags bit 0, wall, unit wallDir).  The corners'
//   predicted positions are recomputed (car_box_place_next_corners) after every move.
//   Retail's forms (ref/matching/u088/notes.md, follow-up 2026-10-17): v set
//   first; segLen2 written out in the loop (GCSE hoists the two squares,
//   not their sum); the partner's next chosen per side; s and invLen as
//   if / else; the slide push set in each arm (cross-jumped in retail, so
//   push is read back from the stack).  segLen2 is tested twice.
void car_box_push_wall_cb(WallSeg *w, CarBox *b)
{
    BoxVertex *v = b->v;
    /* the segment a -> c in the box plane */
    WallSeg *end = wall_next(w);
    f32 ax = w->x;
    f32 ay = -w->z;
    f32 cx = end->x;
    f32 cy = -end->z;
    f32 segX = cx - ax;
    f32 segY = cy - ay;

    for (int pair = 0; pair < 16; pair++) {
        int i = pair & 7;
        if (v[i].flags != 0)
            continue;
        int withPrev = pair < 8;            /* 0..7: corner i and the one before it */
        const Vec2 *partnerNext;
        if (withPrev)
            partnerNext = &v[(pair + 7) & 7].next;
        else
            partnerNext = &v[(pair + 1) & 7].next;
        BoxVertex *corner = &v[i];

        f32 cornerX = corner->next.x;
        f32 cornerY = corner->next.y;
        f32 relX = cornerX - ax;
        f32 relY = cornerY - ay;
        if (0.0f <= cross2(segX, segY, relX, relY))
            continue;                       /* the corner is outside the wall line */
        f32 partnerX = partnerNext->x;
        f32 partnerY = partnerNext->y;
        f32 fromStartX = partnerX - ax;
        f32 fromStartY = partnerY - ay;
        if (cross2(segX, segY, fromStartX, fromStartY) <= 0.0f)
            continue;                       /* the partner is inside too */
        f32 pairX = cornerX - partnerX;
        f32 pairY = cornerY - partnerY;
        if (cross2(pairX, pairY, fromStartX, fromStartY) < 0.0f)
            continue;
        f32 fromEndX = partnerX - cx;
        f32 fromEndY = partnerY - cy;
        if (0.0f < cross2(pairX, pairY, fromEndX, fromEndY))
            continue;

        f32 along = relX * segX + relY * segY;
        f32 segLen2 = segX * segX + segY * segY;
        Vec2 push;
        if (along < 0.0f) {
            /* before the wall's start: a out of partner -> corner */
            f32 pairLen2 = pairX * pairX + pairY * pairY;
            f32 s;
            if (pairLen2 != 0.0f)
                s = (-fromStartX * pairX - fromStartY * pairY) / pairLen2;
            else
                s = 0.0f;
            push.x = s * pairX + fromStartX;
            push.y = s * pairY + fromStartY;
            vec2_sub(&b->next, &b->next, &push);
            car_box_place_next_corners(b);
            continue;
        }
        if (segLen2 < along) {
            /* past the wall's end: c out of partner -> corner */
            f32 pairLen2 = pairX * pairX + pairY * pairY;
            f32 s;
            if (pairLen2 != 0.0f)
                s = (-fromEndX * pairX - fromEndY * pairY) / pairLen2;
            else
                s = 0.0f;
            push.x = s * pairX + fromEndX;
            push.y = s * pairY + fromEndY;
            vec2_sub(&b->next, &b->next, &push);
            car_box_place_next_corners(b);
            continue;
        }

        f32 invLen;
        if (segLen2 != 0.0f)
            invLen = u088_rsqrtf(1.0f, segLen2);
        else
            invLen = 1.0f;
        if (segLen2 != 0.0f)
            along = along / segLen2;
        push.x = along * segX - relX;       /* the corner onto the wall line */
        push.y = along * segY - relY;
        if (WALL_PUSH_MAX2 < push.x * push.x + push.y * push.y) {
            /* too far: the pose moves along the wall instead */
            if (withPrev)
                vec2_set(&push, (1.0f - along) * segX, (1.0f - along) * segY);
            else
                vec2_set(&push, -along * segX, -along * segY);
            vec2_add(&b->next, &b->next, &push);
            car_box_place_next_corners(b);
        } else {
            vec2_add(&b->next, &b->next, &push);
            car_box_place_next_corners(b);
            corner->wall = w;
            corner->wallDir.x = segX * invLen;
            corner->wallDir.y = segY * invLen;
            corner->flags |= BOX_VERTEX_ON_WALL;
        }
    }
}

// FUNC 0x001FDE08 car_box_push_out_walls
//   Visits the walls in the bounding box of the corners' predicted
//   positions with car_box_push_wall_cb.  Quirk: the pointer is advanced
//   after each test, so corner 0 is tested twice and corner 7 not at all.
//   Callers: boxes_step, boxes_step_car (T373).
void car_box_push_out_walls(CarBox *b)
{
    BoxVertex *corner = b->v;
    f32 maxX = corner->next.x;
    f32 maxY = corner->next.y;
    f32 minX = maxX;
    f32 minY = maxY;
    for (int i = 1; i < 8; i++, corner++) {
        f32 x = corner->next.x;
        f32 y = corner->next.y;
        if (x < minX) minX = x;
        if (maxX < x) maxX = x;
        if (y < minY) minY = y;
        if (maxY < y) maxY = y;
    }
    /* RaceCourse::forEachPoint (0x0018FD00); the course frame: z = -y */
    b->course->forEachPoint((RunwayPointFunc)car_box_push_wall_cb, b, b->sector,
                            minX, -maxY, maxX, -minY);
}

// FUNC 0x001FDEC0 car_box_respond_wall_vertex
//   The response to a corner hitting a wall (wallVertex): the contact
//   velocity of the corner (v - w * perp(r)), the normal impulse
//   (car_box_wall_vertex_impulse), the friction (car_box_friction), the
//   position fix (car_box_project_wall_vertex), then the velocity, the
//   vertical velocity (not reversed) and the yaw rate (from the normal
//   impulse only).  The corner then slides along the wall (flags bit 0).
//   Callers: boxes_step, boxes_step_car (T373).
void car_box_respond_wall_vertex(CarBox *b)
{
    Vec2 imp;
    f32 rot[4];
    Vec2 contactVel, perp;
    f32 friction[3];

    rot[0] = cosf(b->yaw);
    f32 sinYaw = sinf(b->yaw);
    rot[2] = sinYaw;
    rot[3] = rot[0];
    rot[1] = -sinYaw;

    rot_perp(&perp, rot, &b->e[b->wallVertex].p0);
    contactVel.x = b->vel.x - b->yawRate * perp.x;
    contactVel.y = b->vel.y - b->yawRate * perp.y;
    car_box_wall_vertex_impulse(b, &imp, rot, &contactVel);
    car_box_friction(b, &imp, &contactVel, friction);

    Vec2 localImp;                          /* the normal impulse in the local frame */
    rot_apply_t(&localImp, rot, &imp);
    imp.x += friction[0];
    imp.y += friction[1];
    car_box_project_wall_vertex(b, rot);

    f32 invMass = b->invMass;
    b->vel.x += imp.x * invMass;
    b->v[b->wallVertex].flags |= BOX_VERTEX_ON_WALL;
    b->vel.y += imp.y * invMass;
    /* the vertical velocity may be reduced by the friction, not reversed */
    int wasNegative = b->velZ < 0.0f;
    b->velZ += friction[2] * invMass;
    if (wasNegative ? 0.0f < b->velZ : b->velZ < 0.0f)
        b->velZ = 0.0f;
    b->yawRate += vec2_cross(&b->e[b->wallVertex].p0, &localImp) * b->invInertia;
}

// FUNC 0x001FE100 car_box_wall_vertex_impulse
//   The normal impulse of a corner hit: n = perp(wallDir) = (-dir.y, dir.x),
//   the effective mass from invMass and the corner's lever (in the local
//   frame), j = -1.05 (v . n) / k (0 when k is 0); *imp = j n.  |j| / 100000
//   goes to impact and the car gets a crash (kind 0, part wallVertex).
void car_box_wall_vertex_impulse(CarBox *b, Vec2 *imp, const f32 *rot, const Vec2 *v)
{
    Vec2 n, localN, perp;
    n.x = -b->wallDir.y;
    n.y = b->wallDir.x;
    rot_apply_t(&localN, rot, &n);
    f32 spin = -vec2_cross(&b->e[b->wallVertex].p0, &localN) * b->invInertia;
    rot_perp(&perp, rot, &b->e[b->wallVertex].p0);
    f32 den = b->invMass + vec2_dot(&perp, &n) * spin;
    f32 j;
    if (den != 0.0f)
        j = ((v->x * n.x + v->y * n.y) * WALL_IMPULSE_GAIN) / den;
    else
        j = 0.0f;
    imp->x = j * n.x;
    imp->y = j * n.y;
    f32 strength = j < 0.0f ? -j : j;
    b->impact += strength / WALL_IMPACT_SCALE;
    car_report_crash(b->car, CRASH_KIND_WALL, b->wallVertex, strength);
}

// FUNC 0x001FE288 car_box_project_wall_vertex
//   When the hit corner (pos) is on the inner side of the wall line
//   (wallSeg, wallDir), moves the box so that it is on the line (the foot
//   of the corner on it; parameter 1 for a zero direction) and recomputes
//   the corners (car_box_place_corners).  rot is passed by the caller but
//   not used.
void car_box_project_wall_vertex(CarBox *b, const f32 *rot)
{
    WallSeg *wall = b->wallSeg;
    f32 dirY = b->wallDir.y;
    f32 dirX = b->wallDir.x;
    Vec2 start;
    start.x = wall->x;
    start.y = -wall->z;
    Vec2 corner = b->v[b->wallVertex].pos;
    Vec2 rel;
    rel.x = corner.x - start.x;
    rel.y = corner.y - start.y;
    if (0.0f < rel.x * dirY - rel.y * dirX) {
        f32 dirLen2 = dirX * dirX + dirY * dirY;
        f32 along;
        if (dirLen2 != 0.0f)
            along = (rel.x * dirX + rel.y * dirY) / dirLen2;
        else
            along = 1.0f;
        Vec2 push;
        const Vec2 *dir = &b->wallDir;     /* (retail reads wallDir.y again here) */
        push.x = (along * dir->x + start.x) - corner.x;
        push.y = (along * dir->y + start.y) - corner.y;
        b->pos.x += push.x;
        b->pos.y += push.y;
        car_box_place_corners(b);
    }
}

// FUNC 0x001FE3A0 car_box_respond_wall_edge
//   The response to a wall vertex hitting an edge (wallEdge): the lever r =
//   the vertex - pos, the contact velocity v + w * perp(r), the edge normal
//   in the world, the normal impulse (car_box_wall_edge_impulse), the
//   friction, then the velocity, the yaw rate (normal impulse only) and the
//   vertical velocity (not reversed).  Callers: boxes_step, boxes_step_car
//   (T373).
void car_box_respond_wall_edge(CarBox *b)
{
    Vec2 imp;
    f32 rot[4];
    Vec2 n, contactVel, lever;
    f32 friction[3];

    rot[0] = cosf(b->yaw);
    f32 sinYaw = sinf(b->yaw);
    rot[2] = sinYaw;
    rot[3] = rot[0];
    rot[1] = -sinYaw;

    BoxEdge *edge = &b->e[b->wallEdge];
    WallSeg *wall = edge->wall;
    f32 wx = wall->x;
    f32 wy = -wall->z;
    lever.x = wx - b->pos.x;
    lever.y = wy - b->pos.y;
    rot_apply(&n, rot, &edge->n);
    contactVel.x = b->vel.x - b->yawRate * lever.y;
    contactVel.y = b->vel.y + b->yawRate * lever.x;
    car_box_wall_edge_impulse(b, &imp, &contactVel, &n, &lever);
    car_box_friction(b, &imp, &contactVel, friction);

    f32 spin = vec2_cross(&lever, &imp) * b->invInertia;
    imp.x = imp.x + friction[0];
    imp.y = imp.y + friction[1];
    f32 invMass = b->invMass;
    b->yawRate += spin;
    b->vel.x += imp.x * invMass;
    b->vel.y += imp.y * invMass;
    /* the vertical velocity may be reduced by the friction, not reversed */
    int wasNegative = b->velZ < 0.0f;
    b->velZ += friction[2] * invMass;
    if (wasNegative ? 0.0f < b->velZ : b->velZ < 0.0f)
        b->velZ = 0.0f;
}

// FUNC 0x001FE578 car_box_wall_edge_impulse
//   The normal impulse of a wall vertex hit: k = invMass + (r x n)^2 x
//   invInertia, j = -1.05 (v . n) / k (0 when k is 0); *imp = j n.  |j| /
//   100000 goes to impact and the car gets a crash (kind 0, part wallEdge +
//   8).
void car_box_wall_edge_impulse(CarBox *b, Vec2 *imp, const Vec2 *v, const Vec2 *n,
                               const Vec2 *r)
{
    f32 nx = n->x;
    f32 ny = n->y;
    f32 leverCross = r->x * ny - r->y * nx;
    f32 den = b->invMass + leverCross * leverCross * b->invInertia;
    f32 j;
    if (den != 0.0f)
        j = ((v->x * nx + v->y * ny) * WALL_IMPULSE_GAIN) / den;
    else
        j = 0.0f;
    imp->x = j * nx;
    imp->y = j * n->y;
    f32 strength = j < 0.0f ? -j : j;
    b->impact += strength / WALL_IMPACT_SCALE;
    car_report_crash(b->car, CRASH_KIND_WALL, b->wallEdge + CRASH_PART_EDGE, strength);
}

// FUNC 0x001FE660 car_box_friction
//   The friction impulse out[3] (x, y, z) for the normal impulse imp and the
//   contact velocity v: the tangential velocity t = (v . imp / |imp|^2) imp
//   - v and the vertical one -velZ, scaled by min(0.5 |imp| / |(t, -velZ)|,
//   mass): Coulomb friction with a
//   coefficient of 0.5, at most the impulse that stops the slip; all 0 when
//   that vector is 0.
void car_box_friction(CarBox *b, const Vec2 *imp, const Vec2 *v, f32 *out)
{
    f32 impX = imp->x;
    f32 impY = imp->y;
    f32 impLen2 = impX * impX + impY * impY;
    f32 along;
    if (impLen2 != 0.0f)
        along = (v->x * impX + v->y * impY) / impLen2;
    else
        along = 0.0f;
    f32 slip[3];                            /* the velocity to take away */
    slip[0] = along * impX - v->x;
    slip[1] = along * impY - v->y;
    slip[2] = -b->velZ;
    f32 slipLen2 = slip[0] * slip[0] + slip[1] * slip[1] + slip[2] * slip[2];
    if (slipLen2 == 0.0f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
        return;
    }
    f32 invSlip = u088_rsqrtf(1.0f, slipLen2);
    f32 k = boot_sqrtf_v(impLen2) * 0.5f * invSlip;    /* the friction coefficient 0.5 */
    if (b->mass < k)
        k = b->mass;
    out[0] = k * slip[0];
    out[1] = k * slip[1];
    out[2] = k * slip[2];
}
