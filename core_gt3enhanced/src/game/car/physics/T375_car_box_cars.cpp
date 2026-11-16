/* T375 - the car collision boxes: car against car.
 *
 * The other box of a test is b->boxes[other].  The swept test
 * (car_box_sweep_car) moves the other box's corners into this box's local
 * frame at both ends of the step and finds the earliest crossing of one of
 * this box's edges; car_box_respond_car then applies an impulse with a
 * restitution of 0.5 and pushes the two boxes apart.
 */
/* Retail T375's .rodata (0x0033DE90): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_box_internal.h"
#include "game/car/physics/T375_car_box_cars.h"
#include "car_box_vec2.h"
// DATA_START 0x0033DE90

// FUNC 0x001FBCE8 car_box_resolve_car_contacts
//   For each corner resting on an edge of another box (flags bit 1): when
//   the corner has left the edge's extent the flag is cleared; when it is
//   inside the other box (on the inner side) both boxes are pushed apart by
//   half the penetration each; then (also for a corner on the outer side)
//   the approaching relative velocity along the edge normal is shared out
//   half and half.  Caller: car_box_resolve_contacts (T374).
//   (rel and edge through vec2_set: retail computes both components before
//   it stores either; the velocity share dv reuses edge's slot.)
void car_box_resolve_car_contacts(CarBox *b)
{
    if (b->collidable == 0)
        return;

    for (int i = 0; i < 8; i++) {
        BoxVertex *corner = &b->v[i];
        u32 flags = corner->flags;
        if (!(flags & BOX_VERTEX_ON_CAR))
            continue;

        CarBox *otherBox = &b->boxes[corner->otherCar];
        BoxVertex *edgeStart = &otherBox->v[corner->otherEdge];
        BoxVertex *edgeEnd = &otherBox->v[(corner->otherEdge + 1) & 7];

        Vec2 rel, edge;
        const Vec2 *start = &edgeStart->pos;
        vec2_set(&rel, corner->pos.x - start->x, corner->pos.y - start->y);
        vec2_set(&edge, edgeEnd->pos.x - start->x, edgeEnd->pos.y - start->y);
        f32 invLen2 = 1.0f / vec2_dot(&edge, &edge);
        f32 along = vec2_dot(&rel, &edge) * invLen2;

        if (along < 0.0f || 1.0f < along) {
            corner->flags = flags & ~(u32)BOX_VERTEX_ON_CAR;
            continue;
        }

        if (0.0f < vec2_cross(&rel, &edge)) {
            /* on the outer side: released (the velocity is still shared) */
            corner->flags = flags & ~(u32)BOX_VERTEX_ON_CAR;
        } else {
            Vec2 push;
            push.x = (edge.x * along - rel.x) * 0.5f;
            push.y = (edge.y * along - rel.y) * 0.5f;
            vec2_add(&b->pos, &b->pos, &push);
            vec2_sub(&otherBox->pos, &otherBox->pos, &push);
        }

        Vec2 relVel = b->vel;
        vec2_sub(&relVel, &relVel, &otherBox->vel);
        if (vec2_cross(&relVel, &edge) < 0.0f) {
            f32 velAlong = vec2_dot(&relVel, &edge) * invLen2;
            edge.x = (edge.x * velAlong - relVel.x) * 0.5f;
            edge.y = (edge.y * velAlong - relVel.y) * 0.5f;
            vec2_add(&b->vel, &b->vel, &edge);
            vec2_sub(&otherBox->vel, &otherBox->vel, &edge);
        }
    }
}

// FUNC 0x001FBF50 car_box_sweep_car
//   The swept test against box `other' (both collidable, the same sector or
//   neighbouring ones): the bounding boxes must overlap and the closest
//   approach of the relative motion be within reach2[other].  Each corner of
//   the other box (whose flags are 0) is taken into this box's local frame
//   at the start (pos / yaw) and at the end (next / nextYaw) of the step;
//   a corner going from the outside (>= 0) to the inside (<= 0) of an edge's
//   line, within the edge, gives a hit at time t + (tEnd - t) * s, kept when
//   in [0, 1] and the earliest.  Unless an earlier car or wall hit is
//   already recorded, the hit is stored (carEdge / carOther / carVertex /
//   carContact, hitTime, carHit) and the front / rear flags of both boxes
//   are set.  Returns 1 when stored.  Caller: boxes_step (T373).
int car_box_sweep_car(CarBox *b, int other)
{
    if (b->collidable == 0)
        return 0;
    CarBox *otherBox = &b->boxes[other];
    if (otherBox->collidable == 0)
        return 0;
    if (b->sector != otherBox->sector &&
        runway_is_neighbour(b->course->data->runway, b->sector, otherBox->sector) == 0)
        return 0;

    f32 overlapX = otherBox->bbMax[0] - b->bbMin[0];
    f32 overlapY = otherBox->bbMax[1] - b->bbMin[1];
    if (overlapX < 0.0f)
        return 0;
    if (overlapY < 0.0f)
        return 0;
    if (otherBox->bbSize[0] + b->bbSize[0] < overlapX)
        return 0;
    if (otherBox->bbSize[1] + b->bbSize[1] < overlapY)
        return 0;

    /* the closest approach of the relative motion */
    Vec2 relPos, relMove, closest;
    relMove.x = otherBox->move.x - b->move.x;
    relMove.y = otherBox->move.y - b->move.y;
    f32 moveLen2 = relMove.x * relMove.x + relMove.y * relMove.y;
    relPos.x = otherBox->pos.x - b->pos.x;
    relPos.y = otherBox->pos.y - b->pos.y;
    f32 approach = -relPos.x * relMove.x - relPos.y * relMove.y;
    if (approach <= 0.0f) {
        closest = relPos;
    } else if (moveLen2 <= approach) {
        closest.x = relPos.x + relMove.x;
        closest.y = relPos.y + relMove.y;
    } else {
        f32 k = approach / moveLen2;
        closest.x = relPos.x + relMove.x * k;
        closest.y = relPos.y + relMove.y * k;
    }
    if (b->reach2[other] < closest.x * closest.x + closest.y * closest.y)
        return 0;

    /* the other box's pose relative to this one, at both ends */
    Vec2 relNext;
    relNext.x = otherBox->next.x - b->next.x;
    relNext.y = otherBox->next.y - b->next.y;
    f32 relYaw0 = otherBox->yaw - b->yaw;
    f32 relYaw1 = otherBox->nextYaw - b->nextYaw;
    f32 sin0 = sinf(-b->yaw);
    f32 cos0 = cosf(-b->yaw);
    f32 sin1 = sinf(-b->nextYaw);
    f32 cos1 = cosf(-b->nextYaw);
    Vec2 origin0, origin1;                  /* the other box's centre in this box's frame */
    origin0.x = cos0 * relPos.x - sin0 * relPos.y;
    origin0.y = sin0 * relPos.x + cos0 * relPos.y;
    origin1.x = cos1 * relNext.x - sin1 * relNext.y;
    origin1.y = sin1 * relNext.x + cos1 * relNext.y;
    f32 relSin0 = sinf(relYaw0);
    f32 relCos0 = cosf(relYaw0);
    f32 relSin1 = sinf(relYaw1);
    f32 relCos1 = cosf(relYaw1);

    f32 best = BOX_NO_HIT;
    int bestVertex = -1;
    int bestEdge = -1;
    Vec2 contact;
    for (int i = 0; i < 8; i++) {
        if (otherBox->v[i].flags != 0)
            continue;
        const Vec2 *local = &otherBox->e[i].p0;     /* the corner in its box's frame */
        Vec2 q0, q1;                                /* in this box's frame, at both ends */
        q0.x = (relCos0 * local->x - relSin0 * local->y) + origin0.x;
        q0.y = (relSin0 * local->x + relCos0 * local->y) + origin0.y;
        q1.x = (relCos1 * local->x - relSin1 * local->y) + origin1.x;
        q1.y = (relSin1 * local->x + relCos1 * local->y) + origin1.y;
        for (int j = 0; j < 8; j++) {
            f32 dist1 = q1.x * b->e[j].n.x + q1.y * b->e[j].n.y + b->e[j].d;
            f32 dist0 = q0.x * b->e[j].n.x + q0.y * b->e[j].n.y + b->e[j].d;
            if (0.0f < dist1)
                continue;
            if (dist0 < 0.0f)
                continue;
            Vec2 toStart, toEnd, path;
            toStart.x = b->e[j].p0.x - q0.x;
            toStart.y = b->e[j].p0.y - q0.y;
            toEnd.x = b->e[j].p1.x - q0.x;
            toEnd.y = b->e[j].p1.y - q0.y;
            path.x = q1.x - q0.x;
            path.y = q1.y - q0.y;
            f32 sideStart = toStart.x * path.y - toStart.y * path.x;
            f32 sideEnd = toEnd.x * path.y - toEnd.y * path.x;
            if (sideStart < 0.0f && sideEnd < 0.0f)
                continue;
            if (0.0f < sideStart && 0.0f < sideEnd)
                continue;
            f32 den = dist0 - dist1;
            f32 s = 1.0f;
            if (den != 0.0f)
                s = dist0 / den;
            f32 time = (b->tEnd - b->t) * s + b->t;
            int outside = 0;
            if (time < 0.0f || 1.0f < time)
                outside = 1;
            if (outside != 0)
                continue;
            if (time < best) {
                best = time;
                bestVertex = i;
                bestEdge = j;
                contact.x = q0.x + path.x * s;
                contact.y = q0.y + path.y * s;
            }
        }
    }

    if (bestVertex < 0)
        return 0;
    if ((b->carHit != 0 || b->wallVertexHit != 0) && b->hitTime < best)
        return 0;

    b->carEdge = bestEdge;
    b->hitTime = best;
    b->carOther = other;
    b->carVertex = bestVertex;
    b->carHit = 1;
    b->carContact = contact;
    b->hitCar = 1;
    if (b->carEdge < BOX_FRONT_END)
        b->hitFront = 1;
    else if ((u32)(b->carEdge - BOX_REAR_START) < 3)
        b->hitRear = 1;
    int front = b->carVertex < BOX_HALF;    /* (retail reads carVertex before the store) */
    otherBox->hitCar = 1;
    if (front)
        otherBox->hitFront = 1;
    else
        otherBox->hitRear = 1;
    return 1;
}

// FUNC 0x001FC5C0 car_box_respond_car
//   The response to the hit stored by car_box_sweep_car: the impulse (see
//   car_box_car_impulse), its size / 15000 added to both boxes' impact, the
//   positional separation, the velocity and yaw-rate changes of both boxes
//   (this box at its contact point, the other at its corner), the other
//   corner marked as resting on this box's edge (flags bit 1), and a crash
//   of both cars (kind 1: this car's part edge + 8, the other's part its
//   corner).  Caller: boxes_step (T373).
void car_box_respond_car(CarBox *b)
{
    CarBox *otherBox = &b->boxes[b->carOther];
    BoxVertex *otherCorner = &otherBox->v[b->carVertex];
    Vec2 imp;
    f32 rotA[4], rotB[4];

    rotA[0] = cosf(b->yaw);
    f32 sinA = sinf(b->yaw);
    rotA[2] = sinA;
    rotA[3] = rotA[0];
    rotA[1] = -sinA;
    rotB[0] = cosf(otherBox->yaw);
    f32 sinB = sinf(otherBox->yaw);
    rotB[2] = sinB;
    rotB[3] = rotB[0];
    rotB[1] = -sinB;

    f32 j = car_box_car_impulse(b, &imp, rotA, rotB);
    f32 strength = (j < 0.0f ? -j : j) / CAR_IMPACT_SCALE;
    b->impact += strength;
    otherBox->impact += strength;
    car_box_separate_car(b, rotA, rotB);

    /* this box: the impulse in its local frame, at the contact point */
    Vec2 localImp;
    localImp.x = rotA[0] * imp.x + rotA[2] * imp.y;
    localImp.y = rotA[1] * imp.x + rotA[3] * imp.y;
    b->vel.x += imp.x * b->invMass;
    b->vel.y += imp.y * b->invMass;
    b->yawRate += (b->carContact.x * localImp.y - b->carContact.y * localImp.x) * b->invInertia;
    otherCorner->flags |= BOX_VERTEX_ON_CAR;
    otherCorner->otherCar = b->index;
    otherCorner->otherEdge = b->carEdge;

    /* the other box: the opposite impulse at its corner */
    otherBox->vel.x -= imp.x * otherBox->invMass;
    otherBox->vel.y -= imp.y * otherBox->invMass;
    Vec2 otherImp;
    otherImp.x = rotB[0] * imp.x + rotB[2] * imp.y;
    otherImp.y = rotB[1] * imp.x + rotB[3] * imp.y;
    otherBox->yawRate -= (otherCorner->local.x * otherImp.y - otherCorner->local.y * otherImp.x) *
                         otherBox->invInertia;

    /* (the enhanced engine: the Pod's reaction first, as at the head of Lupo
     * 2's car_report_crash, L2 0x001DE7A0) */
    pod_crash_reaction(b->car, CRASH_KIND_CAR, b->carEdge + CRASH_PART_EDGE);
    car_report_crash(b->car, CRASH_KIND_CAR, b->carEdge + CRASH_PART_EDGE, strength);
    pod_crash_reaction(otherBox->car, CRASH_KIND_CAR, b->carVertex);
    car_report_crash(otherBox->car, CRASH_KIND_CAR, b->carVertex, strength);
}

// FUNC 0x001FC840 car_box_car_impulse
//   The impulse of the stored car hit: n = this box's edge normal in the
//   world, the contact velocities of both boxes (with their yaw rates:
//   v - w * perp(r)), the effective mass along n from both masses and
//   inertias, j = -1.5 (v_rel . n) / k (0 when k is 0); *imp = j n.
//   Returns j.  rotA / rotB: the rotations of this box and the other.
//   The arms rA / rB and perpA / perpB are the same vectors: retail computes
//   them twice (for the velocities and again for the effective mass), each
//   in its own stack slot.
f32 car_box_car_impulse(CarBox *b, Vec2 *imp, const f32 *rotA, const f32 *rotB)
{
    Vec2 n, velA, velB, rA, rB;
    CarBox *otherBox = &b->boxes[b->carOther];

    rot_apply(&n, rotA, &b->e[b->carEdge].n);
    rot_perp(&rA, rotA, &b->carContact);
    velA.x = b->vel.x - b->yawRate * rA.x;
    velA.y = b->vel.y - b->yawRate * rA.y;
    rot_perp(&rB, rotB, &otherBox->e[b->carVertex].p0);
    velB.x = otherBox->vel.x - otherBox->yawRate * rB.x;
    velB.y = otherBox->vel.y - otherBox->yawRate * rB.y;

    f32 spinA = -vec2_cross(&b->carContact, &b->e[b->carEdge].n) * b->invInertia;
    Vec2 perpA;
    rot_perp(&perpA, rotA, &b->carContact);
    Vec2 otherN;
    rot_apply_t(&otherN, rotB, &n);
    f32 spinB = -vec2_cross(&otherBox->e[b->carVertex].p0, &otherN) * otherBox->invInertia;
    Vec2 perpB;
    rot_perp(&perpB, rotB, &otherBox->e[b->carVertex].p0);
    Vec2 massTerm;
    massTerm.x = spinA * perpA.x + spinB * perpB.x;
    massTerm.y = spinA * perpA.y + spinB * perpB.y;
    f32 den = b->invMass + otherBox->invMass + vec2_dot(&massTerm, &n);
    Vec2 relVel;
    vec2_sub(&relVel, &velA, &velB);

    f32 j;
    if (den != 0.0f)
        j = (vec2_dot(&relVel, &n) * CAR_IMPULSE_GAIN) / den;
    else
        j = 0.0f;
    imp->x = j * n.x;
    imp->y = j * n.y;
    return j;
}

// FUNC 0x001FCB20 car_box_separate_car
//   The other box's corner (at the current poses) against this box's edge
//   line: when it is still inside (distance > 0 along the inward side) the
//   boxes are moved apart by half the depth each along the edge normal.
//   Only pos changes (not the corners).
void car_box_separate_car(CarBox *b, const f32 *rotA, const f32 *rotB)
{
    CarBox *otherBox = &b->boxes[b->carOther];
    Vec2 corner, n, arm, push;

    rot_apply(&arm, rotB, &otherBox->e[b->carVertex].p0);
    corner.x = (otherBox->pos.x - b->pos.x) + arm.x;
    corner.y = (otherBox->pos.y - b->pos.y) + arm.y;
    rot_apply(&n, rotA, &b->e[b->carEdge].n);
    f32 depth = vec2_dot(&n, &corner) + b->e[b->carEdge].d;
    if (!(depth <= 0.0f)) {
        push.x = -depth * n.x * 0.5f;
        push.y = -depth * n.y * 0.5f;
        otherBox->pos.x += push.x;
        otherBox->pos.y += push.y;
        b->pos.x -= push.x;
        b->pos.y -= push.y;
    }
}

// FUNC 0x001FCC98 car_box_push_out_car
//   The static overlap with box `other' (collidable, same / neighbouring
//   sector, centres within reach2): for each corner of the other box that is
//   inside this box's polygon (left of every edge, corners' pos), the push
//   along the centre line d = o->pos - b->pos to the first edge it crosses;
//   the deepest push (projected on d) moves the other box by +1/2 and this
//   box by -1/2 of it (car_box_translate, corners included).  Caller:
//   boxes_step (T373).
void car_box_push_out_car(CarBox *b, int other)
{
    if (b->collidable == 0)
        return;
    CarBox *otherBox = &b->boxes[other];
    if (otherBox->collidable == 0)
        return;
    if (b->sector != otherBox->sector &&
        runway_is_neighbour(b->course->data->runway, b->sector, otherBox->sector) == 0)
        return;

    f32 centresX = otherBox->pos.x - b->pos.x;
    f32 centresY = otherBox->pos.y - b->pos.y;
    if (b->reach2[other] < centresX * centresX + centresY * centresY)
        return;

    f32 bestY = 0.0f;
    f32 bestX = 0.0f;
    f32 best = 0.0f;
    BoxVertex *corner = otherBox->v;
    for (int i = 0; i < 8; i++, corner++) {
        f32 qx = corner->pos.x;
        f32 qy = corner->pos.y;
        BoxVertex *prev = &b->v[7];
        BoxVertex *cur = b->v;
        int found = 0;
        f32 pushY = 0.0f, pushX = 0.0f, depth = 0.0f;
        int j;
        for (j = 0; j < 8; j++) {
            f32 edgeX = cur->pos.x - prev->pos.x;
            f32 edgeY = cur->pos.y - prev->pos.y;
            f32 relX = qx - prev->pos.x;
            f32 relY = qy - prev->pos.y;
            if (edgeX * relY - edgeY * relX < 0.0f)
                break;                      /* outside this edge */
            if (found == 0) {
                f32 sidePrev = centresX * relY - centresY * relX;
                if (!(sidePrev < 0.0f)) {
                    f32 curX = qx - cur->pos.x;
                    f32 curY = qy - cur->pos.y;
                    f32 sideCur = centresX * curY - centresY * curX;
                    if (!(0.0f < sideCur)) {
                        f32 den = sidePrev - sideCur;
                        f32 s;
                        if (den != 0.0f)
                            s = sidePrev / den;
                        else
                            s = 0.0f;
                        found = 1;
                        pushX = s * edgeX - relX;
                        pushY = s * edgeY - relY;
                        depth = pushX * centresX + pushY * centresY;
                    }
                }
            }
            prev = cur;
            cur++;
        }
        if (j == 8 && best < depth) {
            best = depth;
            bestX = pushX;
            bestY = pushY;
        }
    }

    if (0.0f < best) {
        bestX = bestX * 0.5f;
        bestY = bestY * 0.5f;
        car_box_translate(otherBox, bestX, bestY);
        car_box_translate(b, -bestX, -bestY);
    }
}
