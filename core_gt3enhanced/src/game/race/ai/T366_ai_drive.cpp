/* T366 (0x001F7078..0x001F8638): the computer driver's driving: the braking
 * point, the throttle for a target speed, the look-ahead point, the steering,
 * the recovery logic and the copy of the outputs into the car.
 * Called by car_input_ai (T358: ai_drive, ai_idle), car_input_ai_apply
 * (T342: ai_apply_outputs) and ai_update_gear (T363: ai_brake_factor). */
/* Retail T366's .rodata (0x0033DD00): <new>'s "bad_alloc" (unreferenced:
 * DATA_START), ai_drive's double 0.01 and jump table, the type name
 * "t9UnitArena1Ui64". */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/ai/T366_ai_drive.h"
#include "game/race/ai/ai_internal.h"
#include "game/enhanced.h"                  /* ENH_AI_TORQUE_HANDICAP, _START_LOOKAHEAD, _PIT_EXIT, _RECOVERY,
                                               _SLIP_STEER */
#include "game/race/modes/race_pod_rules.h" /* the Pod race's stop area (pod_rules_*) */
// DATA_START 0x0033DD00

/* The enhanced engine: AIDriver.unk_AE (T361) is Lupo 2's Pod stop state
 * (AIDriver +0xAE there too; race_logic.md A10, only in the Pod race). */
enum {
    POD_STOP_NONE     = 0,
    POD_STOP_APPROACH = 1,                  /* braking for the stop point, then 10 km/h for the last 20 m */
    POD_STOP_STOPPED  = 2                   /* within 0.5 m and under 1 km/h: held while the stop is pending */
};

/* Where in the stop area a computer car stops, by (rank - 1) / 2: the
 * leaders at its far end (Lupo 2's table L2 0x002DF128). */
static const f32 podStopShare[4] = { 0.75f, 0.5f, 0.25f, 0.0f };

/* The corner index, 0 past the last one. */
// FUNC 0x001F7078 ai_corner_index
int ai_corner_index(int idx, int count)
{
    return (idx != count) ? idx : 0;
}

/* Steps ai->corner along the course distance (wrapping at half a lap), then,
 * heading for a slower corner, compares the braking distance (v^2 - vt^2)
 * x max(unk_E34, 1 / (2 mu g)) (mu: the front tyre's grip on the corner's
 * surface at the static load, adjusted for the pitch, the node's brakeAdj,
 * the driver's brakeScale and the caution) with the distance to the corner
 * node: past it the state becomes braking (3 / 4); within margin + 50 m of it
 * brakeState 1 and, with brakeScale > 1, a throttle factor that runs down to
 * 0 over the margin.  Returns that factor times throttleScale (not on
 * line 3; 1 on line 4).
 *
 * The enhanced engine:
 * - race_logic.md A1 (ENH_AI_TORQUE_HANDICAP): a throttle scale s under 1
 *   becomes the pedal that gives s of the engine's full-throttle net
 *   torque, (s x engineTorque + frictionTorque) / (engineTorque +
 *   frictionTorque), so the engine braking no longer handicaps a car more
 *   than its rules say (Lupo 2 L2 0x0020134C..0x00201398);
 * - race_logic.md A10 (the Pod race only, pod_rules_on): a computer car
 *   with a stop pending brakes for a point in the stop area when that point
 *   lies in the segment it is driving (the leaders stop at the far end, by
 *   rank); past its braking point it hands over to ai_drive (unk_AE 1)
 *   (L2 0x00200DC8..0x00200F9C, 0x00201200..0x00201244, 0x00201330). */
// FUNC 0x001F7088 ai_brake_factor
f32 ai_brake_factor(AIDriver *ai)
{
    f32 factor = 1.0f;
    RaceCarState *car = ai->car;
    CourseLines *lines = ai->lines;
    DynCarState *st = &car->st;
    CarDynamics *dyn = &car->params;
    AIParams *p;
    int count, changed;
    LineNode *nodes;
    f32 lap;

    ai->brakeState = BRAKE_STATE_NONE;
    if (ai->line == LINE_PIT_LANE)
        return factor;
    count = ai_corner_count(ai, ai->line);
    nodes = ai_corners(ai, ai->line);
    lap = lines->course->getLapLength();   /* F_0018FA10 */
    changed = 0;
    if (ai->corner == 0 && lap * 0.5f < st->lapDistance) {
        ai->corner = count;                 /* before the line: still the last lap's end */
        changed = 1;
    } else if (ai->corner < count && nodes[ai->corner].dist <= st->lapDistance) {
        changed = 1;
        ai->corner = ai->corner + 1;
    } else if (ai->corner == count && st->lapDistance < lap * 0.5f) {
        ai->corner = 0;                     /* across the line */
        changed = 1;
    } else if (ai->corner != 0 && st->lapDistance < nodes[ai->corner - 1].dist) {
        changed = 1;                        /* backwards */
        ai->corner = ai->corner - 1;
    }
    if (changed != 0)
        ai_next_corner(ai);

    p = &ai->params;
    /* (The braking states' else last: retail jumps there from this test, the
     * braking point's own brakeState = 2 falls into it.) */
    if (ai->state != AI_BRAKE && ai->state != AI_BRAKE_CORNER) {
        int nextKind = ai->nextKind;
        f32 nextSpeed = ai->nextSpeed;
        int podStop = 0;                    /* Lupo 2: braking for the Pod's stop point instead */

        /* Lupo 2 (L2 0x00200DC8): the Pod race's stop, for a car the
         * computer drives (driver 1 / 2, or not the driver's mode) */
        if (pod_rules_on(ai->conductor) && ai->unk_AE == POD_STOP_NONE
            && (st->driver == DRIVER_AUTO || st->driver == DRIVER_COMPUTER || st->ctrlMode != CTRL_DRIVER)
            && pod_rules_pending(ai->conductor, st->carIndex) > 0) {
            f32 start, length, target;

            pod_rules_area(ai->conductor, &start, &length);
            target = start + length * podStopShare[(st->rank - 1) / 2];
            if (ai->line != LINE_PIT_LANE) {
                /* the point lies in the segment the car is in, ahead of it */
                if (ai->corner == count) {
                    if (nodes[count - 1].dist < target && st->lapDistance < target)
                        podStop = 1;
                    else if (target < nodes[0].dist && st->lapDistance < target)
                        podStop = 1;
                } else if (ai->corner == 0) {
                    if (target < nodes[0].dist && st->lapDistance < target)
                        podStop = 1;
                } else if (nodes[ai->corner - 1].dist < target && target < nodes[ai->corner].dist
                           && st->lapDistance < target) {
                    podStop = 1;
                }
            }
            if (podStop != 0) {
                f32 point[3];               /* x, y, height */
                RunwayPos q;

                nextSpeed = 0.0f;
                line_point_at(lines, ai->line, ai->corner, target, &point[0], &point[1], &point[2]);
                q.x = point[0];             /* the runway's axes: (x, height, -y) */
                q.y = point[2];
                q.z = -point[1];
                q.sector = st->sector;
                pod_rules_set_stop_point(ai->conductor, st->carIndex, point[0], point[1],
                                         ai->conductor->course->getLapDistance(&q));
                nextKind = AI_BRAKE;        /* (Lupo 2 sets the kind register to 3: not a straight) */
            }
        }
        if (nextKind != AI_STRAIGHT && st->speedX > nextSpeed) {
            f32 v = st->speedX;
            f32 vt = nextSpeed;
            f32 dv2 = (v + vt) * (v - vt);
            TyreData *tyre;
            LineNode *node;
            s8 surface;
            f32 grip, g, perDv2, gripLimit, adj, dx, dy, d;

            surface = nodes[ai_corner_index(ai->corner, count)].surface;
            grip = CarDynamics_getTyre(dyn, 0, surface)->surfaceGrip[surface];
            tyre = CarDynamics_getTyre(dyn, 0, surface);
            /* The grip at half the weight per front tyre width
             * (gear.tyreWidth[0]), the slip curve's peak, the camber's. */
            grip *= curveLookup(&tyre->loadCurve, (dyn->weight * 0.5f) / dyn->gear.tyreWidth[0]);
            grip *= tyre->peakPos;
            grip *= dyn->camberGrip[0];
            if (st->flags & DYN_FLAG_COUNTED_GROUND)
                g = grip;
            else
                g = grip * (st->wheel[0].wearGrip + st->wheel[1].wearGrip) * 0.5f;
            /* The braking distance per (m/s)^2: the brakes' (unk_E34: mass
             * over the brake force) or the grip's 1 / (2 mu g), the longer. */
            perDv2 = dyn->unk_E34;
            gripLimit = 1.0f / ((g + g) * 9.8f);
            if (perDv2 < gripLimit)
                perDv2 = gripLimit;
            perDv2 *= 1.0f - sinf(nodes[ai_corner_index(ai->corner, count)].pitch);
            d = dv2 * perDv2;               /* the braking distance */
            /* In double: (f32)brakeAdj * 0.01 + 1.0 (0x0033DD10, __muldf3 /
             * __adddf3 / __truncdfsf2). */
            adj = (f32)((f32)nodes[ai_corner_index(ai->corner, count)].brakeAdj * 0.01 + 1.0);  /* implicit: fptodp, dpmul, dpadd, dptofp (the double arithmetic) */
            if (adj < 0.1f)
                adj = 0.1f;
            d *= adj;
            if (p->brakeScale < 1.0f)
                d *= p->brakeScale;         /* a late braker */
            if (ai->lineTimer == 0 && 0.0f < ai->caution)
                d += ai->caution * 20.0f;   /* a car closing in ahead: up to 20 m earlier */
            if (podStop != 0) {
                /* Lupo 2: the distance to the Pod's stop point (L2 0x00201218) */
                f32 x, y, at;

                pod_rules_get_stop_point(ai->conductor, st->carIndex, &x, &y, &at);
                dx = x - st->pos[0];
                dy = y - st->pos[1];
            } else {
                node = &nodes[ai_corner_index(ai->corner, count)];
                dx = node->x - st->pos[0];
                dy = node->y - st->pos[1];
            }
            {
                f32 toNode = boot_sqrtf_v(dx * dx + dy * dy);

                if (d < toNode) {
                    f32 margin = 0.0f;
                    f32 rest;

                    if (1.0f < p->brakeScale)
                        margin = d * (p->brakeScale - 1.0f);    /* a careful one */
                    rest = toNode - d;
                    if (rest < margin + ai_brake_margin(ai))
                        ai->brakeState = BRAKE_STATE_NEAR;
                    if (0.0f < margin && rest < margin)
                        factor = rest / margin;
                } else {
                    if (ai->state != AI_STRAIGHT)
                        ai->state = AI_BRAKE_CORNER;
                    else
                        ai->state = AI_BRAKE;
                    ai->brakeState = BRAKE_STATE_BRAKING;
                    if (podStop != 0)
                        ai->unk_AE = POD_STOP_APPROACH;     /* Lupo 2 (L2 0x0020133C) */
                }
            }
        }
    } else {
        ai->brakeState = BRAKE_STATE_BRAKING;
    }
    if (ai->line != LINE_PIT_ENTRY) {
#if ENH_AI_TORQUE_HANDICAP
        /* Lupo 2: the handicap as a share of the net engine torque (L2
         * 0x00201350) */
        f32 scale = p->throttleScale;

        if (scale < 1.0f) {
            f32 drive = st->engineTorque;
            f32 drag = st->frictionTorque;
            f32 sum = drive + drag;

            if (sum != 0.0f)
                scale = (scale * drive + drag) / sum;
        }
        factor *= scale;
#else
        factor *= p->throttleScale;
#endif
    }
    return factor;
}

/* The heading of `line' at course distance dist: the direction from its
 * point at dist to the point 0.5 m further, pushed on 0.5 m at a time (up to
 * 10 tries) until the two are at least 0.2 m apart; yaw 0 looks along +y,
 * as the car's (line 4: line_point_at_linear).  Lupo 2 L2 0x00201668, new
 * in the enhanced engine (race_logic.md A6, ENH_AI_RECOVERY).  Caller:
 * ai_drive. */
f32 ai_line_heading(CourseLines *lines, int line, int corner, f32 dist)
{
    f32 p0[3], p1[3];                       /* x, y, height */
    f32 dx, dy;
    int tries = 0;

    if (line != LINE_PIT_LANE)
        line_point_at(lines, line, corner, dist, &p0[0], &p0[1], &p0[2]);
    else
        line_point_at_linear(lines, LINE_PIT_LANE, corner, &p0[0], &p0[1], &p0[2], dist);
    do {
        dist += 0.5f;
        if (line != LINE_PIT_LANE)
            line_point_at(lines, line, corner, dist, &p1[0], &p1[1], &p1[2]);
        else
            line_point_at_linear(lines, LINE_PIT_LANE, corner, &p1[0], &p1[1], &p1[2], dist);
        dx = p1[0] - p0[0];
        dy = p1[1] - p0[1];
        tries++;
        if (0.2f <= boot_sqrtf(dx * dx + dy * dy))
            break;
    } while (tries < 10);
    return atan2f(-dx, dy);
}

#if ENH_AI_RECOVERY
/* ai_drive's recovery test of Lupo 2 (L2 0x00202038..0x00202078): 1 when
 * the car's yaw is more than 60 deg off the line's own direction at its
 * lap distance.  The difference is not wrapped to +-pi, as in Lupo 2 (near
 * the +-pi seam it reads as off). */
static int ai_line_off(AIDriver *ai, CourseLines *lines, DynCarState *st)
{
    f32 off = ai_line_heading(lines, ai->line, ai->corner, st->lapDistance) - st->yaw;

    if (off < 0.0f)
        off = -off;
    return 1.0471976f < off;
}
#endif

/* base + gain x (speed - target), the difference held to +-10 km/h; 0..1. */
// FUNC 0x001F7520 ai_speed_throttle
f32 ai_speed_throttle(f32 speed, f32 target, f32 base, f32 gain)
{
    f32 diff = speed - target;
    f32 throttle;

    if (diff < -2.777778f)                  /* 10 km/h */
        diff = -2.777778f;
    else if (2.777778f < diff)
        diff = 2.777778f;
    throttle = base + gain * diff;
    if (1.0f < throttle)
        return 1.0f;
    if (throttle < 0.0f)
        throttle = 0.0f;
    return throttle;
}

/* The throttle that holds `target' (3/4 throttle at 2 km/h under it); full
 * brake when it is 0 and the car is faster; shiftTimer 12 for a part
 * throttle. */
// FUNC 0x001F7590 ai_hold_speed
void ai_hold_speed(AIDriver *ai, f32 target, f32 gain)
{
    DynCarState *st = &ai->car->st;
    f32 throttle = ai_speed_throttle(st->speedX, target - 0.5555556f /* 2 km/h */, 0.75f, gain);

    ai->throttle = throttle;
    if (throttle == 0.0f && target < st->speedX)
        ai->brake = 1.0f;
    if (ai->throttle != 1.0f && ai->throttle != 0.0f)
        ai->shiftTimer = 12;
}

/* The point `dist' m ahead on the line (line_point_at; line 4
 * line_point_at_linear), pushed 1 m further at a time (up to 9 m) until it is
 * at least 5 m from the car.  Returns the distance used; *dx, *dy: the point
 * relative to the car. */
// FUNC 0x001F7658 ai_look_ahead
f32 ai_look_ahead(CourseLines *lines, DynCarState *run, int line, int corner,
                  f32 *dx, f32 *dy, f32 dist)
{
    f32 point[3];                           /* sp+0x0: x, y, height */
    f32 x, y;
    int tries = 0;

    do {
        f32 at = run->lapDistance + dist;

        if (line != LINE_PIT_LANE)
            line_point_at(lines, line, corner, at, &point[0], &point[1], &point[2]);
        else
            line_point_at_linear(lines, LINE_PIT_LANE, corner, &point[0], &point[1], &point[2], at);
        x = point[0] - run->pos[0];
        y = point[1] - run->pos[1];
        if (5.0f <= boot_sqrtf(x * x + y * y))
            break;
        if (tries < 9)
            dist += 1.0f;
        tries++;
    } while (tries < 10);
    *dx = x;
    *dy = y;
    return dist;
}

/* Turns ai->steer toward `target' at steerRate rad/s. */
// FUNC 0x001F77B8 ai_steer_toward
void ai_steer_toward(AIDriver *ai, f32 target)
{
    f32 steer = ai->steer;
    f32 step = ai->params.steerRate * ai->car->dt;

    if (steer < target) {
        ai->steer = steer + step;
        if (target < steer + step)
            ai->steer = target;
    } else if (target < steer) {
        ai->steer = steer - step;
        if (steer - step < target)
            ai->steer = target;
    }
}

/* Stops the car: no throttle, full brake (not in ctrlMode 5) until it is
 * still (under 0.25 km/h and 3 deg/s), then car_mode_still (ctrlMode 8). */
// FUNC 0x001F7810 ai_stop
void ai_stop(AIDriver *ai, f32 steer)
{
    DynCarState *st = &ai->car->st;
    f32 speed, turnRate;
    int still;

    ai_reset_outputs(ai);
    ai_steer_toward(ai, steer);
    ai->throttle = 0.0f;
    {
        f32 vx = st->vel[0], vy = st->vel[1], vz = st->vel[2];
        f32 yaw = st->yawRate, roll = st->rollRate, pitch = st->pitchRate;

        speed = boot_sqrtf_v(vx * vx + vy * vy + vz * vz);
        turnRate = boot_sqrtf_v(yaw * yaw + roll * roll + pitch * pitch);
    }
    still = 0;
    if (speed < 0.06944445f && turnRate < 0.05235988f)  /* 0.25 km/h, 3 deg/s */
        still = 1;
    if (still != 0) {
        ai->brake = 0.0f;
        car_mode_still(ai->car);
    } else if (st->ctrlMode != CTRL_STOP_ROLL) {
        ai->brake = 1.0f;
    }
}

/* No throttle, no brake, the steering back to 0. */
// FUNC 0x001F7900 ai_idle
void ai_idle(AIDriver *ai)
{
    ai_reset_outputs(ai);
    ai_steer_toward(ai, 0.0f);
    ai->throttle = 0.0f;
    ai->brake = 0.0f;
}

/* The look-ahead distance: lookNear at rest to lookFar at
 * CourseDriveParam.lookSpeed x 2 km/h. */
// FUNC 0x001F7948 ai_look_distance
f32 ai_look_distance(AIParams *p, f32 speed)
{
    f32 t;

    if (speed <= 0.0f)
        return p->lookNear;
    t = p->lookInvSpeed * speed;
    if (1.0f <= t)
        return p->lookFar;
    return p->lookNear + t * (p->lookFar - p->lookNear);
}

/* Clears the outputs (the steering starts from the car's) and runs the line
 * blend down by the frame time. */
// FUNC 0x001F79A0 ai_reset_outputs
void ai_reset_outputs(AIDriver *ai)
{
    RaceCarState *car = ai->car;
    f32 blend = ai->lineBlend;

    ai->steer = car->st.steer;
    ai->throttle = 0.0f;
    ai->brake = 0.0f;
    ai->reverse = 0;
    ai->shiftTimer = 0;
    /* The code compares with the throttle it has just cleared (re-read):
     * 0, the blend's floor. */
    if (ai->throttle < blend) {
        f32 b = blend - car->dt;

        ai->lineBlend = b;
        if (b < ai->throttle)
            ai->lineBlend = ai->throttle;
    }
}

/* Copies the outputs into the car's run state; in a corner (state 1 / 2) the
 * throttle moves by at most the frame time a frame.  The enhanced engine
 * (Lupo 2 L2 0x00201A88): the steering display byte is the physics' with
 * ENH_PHYSICS_DISPLAY_FLAGS (physics.md 22 b: Lupo 2 writes it from the
 * steering angle there, not here); a Pod held in the stop area pulls the
 * side brake (race_logic.md A10). */
// FUNC 0x001F79F0 ai_apply_outputs
void ai_apply_outputs(AIDriver *ai)
{
    RaceCarState *car = ai->car;
    DynCarState *st = &car->st;
    f32 throttle = ai->throttle;

    if (ai->state == AI_CORNER_POS || ai->state == AI_CORNER_NEG) {
        f32 cur = st->throttle;
        f32 want = throttle;

        if (cur < want) {
            throttle = cur + car->dt;
            if (want < throttle)
                throttle = want;
        } else {
            throttle = cur - car->dt;
            if (throttle < want)
                throttle = want;
        }
    }
    st->throttleOut = throttle;
    st->throttle = throttle;
    st->steer = ai->steer;
    car_set_brake(car, ai->brake);
    st->ctrl.reverse = (s8)ai->reverse;
    if (ai->shiftTimer != 0)
        st->shiftTimer = (u8)ai->shiftTimer;
#if !ENH_PHYSICS_DISPLAY_FLAGS
    st->steering = (s8)(int)((ai->steer * 100.0f) / car->params.steerLock);
#endif
    /* Lupo 2: a Pod stopped in the stop area pulls the side brake, which
     * makes it sit down (L2 0x00201B24; race_logic.md A10) */
    if (ai->unk_AE == POD_STOP_STOPPED)
        st->handbrake = 1.0f;
}

/* A frame of driving.  ctrlMode 7: stop.  Otherwise: the look-ahead point
 * (5 m at the pit box, 20 m leaving the pits in the pit lane, 10 / 40 m when
 * recovering, else by the speed plus up to 40 m in the first 2 s of the
 * conductor's clock), blended with the old line after a change; the heading
 * error to it decides forward (drive 1) or reverse (drive -1) and the
 * recovery state; forward, the steering corrects the yaw rate toward the one
 * that reaches the point, limited by the tyres' peak slip angle (`slide' > 0
 * beyond it; on a course of kind 2 the steering is not cut back); the
 * throttle holds the speed of the state (full, corner, braking, the pit
 * lane), times the braking factor; from (rank x 100 + 5000) ms on, the
 * traffic logic.
 *
 * The enhanced engine (Lupo 2 L2 0x00201B50): leaving the pits, 5 m below
 * 30 km/h (A5, ENH_AI_PIT_EXIT); a computer car's start look-ahead in a
 * standing / slow rolling start up to 200 m over 5 s (A4,
 * ENH_AI_START_LOOKAHEAD); turning round, a front hit also backs up when
 * the car points more than 60 deg off the line's own direction (A6,
 * ENH_AI_RECOVERY); the yaw-rate gain eased by the front slip (A7,
 * ENH_AI_SLIP_STEER); in the Pod race the stop in the stop area (A10). */
// FUNC 0x001F7AE0 ai_drive
void ai_drive(AIDriver *ai)
{
    RaceCarState *car = ai->car;
    DynamicsConductor *dc = ai->conductor;
    CourseLines *lines = ai->lines;
    DynCarState *st = &car->st;
    CarDynamics *dyn = &car->params;
    AIParams *p;
    CarBoxList *boxes;
    f32 point[2][2];                        /* sp+0x0 the look-ahead point relative to the car: [0] on
                                             * the line, [1] on the old line (one array: retail's 0x0 / 0x8) */
    f32 brakeFactor, look, heading, headingAbs, alignLimit, speed, steer, slide;
    f32 negSinYaw, cosYaw;
    int braking, drive, boxZero, hitFront, hitRear, dir;

    ai_reset_outputs(ai);
    if (st->ctrlMode == CTRL_STOP) {
        ai_stop(ai, 0.0f);
        return;
    }
    brakeFactor = ai_brake_factor(ai);
    p = &ai->params;
    braking = 0;
    drive = 0;
    slide = 0.0f;
    /* The car's collision box (dc + 0xBBB4 + 0xC + carIndex x 0x4F0): the
     * frame's hits at its front / rear (the first query always gives 0). */
    boxes = dc->GetCollisionProcessor();
    boxZero = car_box_get_zero(&boxes->box[st->carIndex]);
    hitFront = car_box_hit_front(&boxes->box[st->carIndex]);
    hitRear = car_box_hit_rear(&boxes->box[st->carIndex]);
    negSinYaw = -sinf(st->yaw);
    cosYaw = cosf(st->yaw);

    if (st->ctrlMode == CTRL_PIT_BOX) {
        look = 5.0f;
    } else if (st->ctrlMode == CTRL_PIT_OUT && ai->inPitLane != 0) {
        look = 20.0f;
#if ENH_AI_PIT_EXIT
        if (st->speedX < 8.333334f)         /* Lupo 2: 5 m below 30 km/h (L2 0x00201CD0) */
            look = 5.0f;
#endif
    } else if (ai->recover != AI_ON_LINE) {
        look = 40.0f;
        if (ai->recover == AI_BACKING_UP)
            look = 10.0f;
    } else {
        int msLeft;

        look = ai_look_distance(p, st->speedX);
#if ENH_AI_START_LOOKAHEAD
        /* Lupo 2: a computer car in a standing or slow rolling start (under
         * 60 km/h), not on dirt nor in the Pod race, looks up to 200 m
         * further over the first 5 s (L2 0x00201D38..0x00201E24) */
        if (st->driver == DRIVER_COMPUTER && dc->isDirtCourse() == 0 && pod_rules_on(dc) == 0
            && dc->rules.launchSpeed < 60) {
            msLeft = 5000 - (int)(dc->clock / 3);
            if (msLeft > 0)
                look += ((f32)msLeft * 200.0f) / 5000.0f;
        } else
#endif
        {
            msLeft = 2000 - (int)(dc->clock / 3);   /* clock: 1/3 ms */
            if (msLeft > 0)
                look += ((f32)msLeft * 40.0f) / 2000.0f;
        }
    }
    look = ai_look_ahead(lines, st, ai->line, ai->corner, &point[0][0], &point[0][1], look);
    if (0.0f < ai->lineBlend) {
        ai_look_ahead(lines, st, ai->prevLine, ai->corner, &point[1][0], &point[1][1], look);
        point[0][0] += (point[1][0] - point[0][0]) * ai->lineBlend;
        point[0][1] += (point[1][1] - point[0][1]) * ai->lineBlend;
    }
    /* The heading error to the point (yaw 0 looks along +y). */
    heading = angleDiff(atan2f(-point[0][0], point[0][1]), st->yaw);
    headingAbs = heading;
    if (heading < 0.0f)
        headingAbs = -heading;

    alignLimit = 1.0471976f;                /* 60 deg */
    if (ai->recover == AI_BACKING_UP) {
        if (0.0f <= st->speedX)
            alignLimit = 0.17453292f;       /* 10 deg */
        else if (-8.333334f < st->speedX)   /* 30 km/h backwards: 10 .. 60 deg */
            alignLimit = 0.17453292f - (st->speedX * 0.87266463f) / 8.333334f;
    }
    speed = st->speedX;
    if (headingAbs < alignLimit || ai->recover == AI_TURNING) {
        if (-0.5555556f < speed)            /* 2 km/h */
            drive = 1;
        else
            braking = 1;
    } else if (0.5555556f < speed) {
        braking = 1;
    } else {
        drive = -1;
    }

    switch (ai->recover) {
    case AI_BACKING_UP:
        if (headingAbs < alignLimit) {
            if (0.8333334f < speed)         /* 3 km/h */
                ai->recover = AI_ON_LINE;
            else
                ai->recover = AI_TURNING;
        } else if (hitRear != 0) {
            ai->recover = AI_TURNING;
        }
        break;
    case AI_TURNING:
        if (headingAbs < 0.6981317f && 8.333334f < speed)  /* 40 deg, 30 km/h */
            ai->recover = AI_ON_LINE;
        else if (hitFront != 0 && (boxZero != 0 || 1.0471976f < headingAbs
#if ENH_AI_RECOVERY
                                   || ai_line_off(ai, lines, st)
#endif
                                   ))
            ai->recover = AI_BACKING_UP;
        break;
    default:
        if (1.3962634f < headingAbs)        /* 80 deg */
            ai->recover = AI_TURNING;
        else if (boxZero != 0)
            ai->recover = AI_BACKING_UP;
        else if (hitRear != 0 && 0.7853982f < headingAbs)
            ai->recover = AI_TURNING;
        break;
    }

    speed = st->speedX;
    dir = 0;
    if (0.5555556f < speed)
        dir = 1;
    else if (speed < -0.5555556f)
        dir = -1;

    if (dir == 0) {
        steer = 0.0f;
    } else {
        if (drive == 1) {
            u8 frSurface = st->wheel[1].surface;
            TyreData *flTyre = CarDynamics_getTyre(dyn, 0, st->wheel[0].surface);
            f32 maxSteer;

            steer = 0.0f;
            /* The front tyres' peak slip angle: the steering limit. */
            maxSteer = (flTyre->peakSlipAngle
                        + CarDynamics_getTyre(dyn, 1, frSurface)->peakSlipAngle) * 0.5f;
            if (ai->recover != AI_ON_LINE) {
                if (1.3962634f < heading)
                    steer = maxSteer;
                else if (heading < -1.3962634f)
                    steer = -maxSteer;
            }
            if (steer == 0.0f) {
                f32 dist2 = point[0][0] * point[0][0] + point[0][1] * point[0][1];
                f32 vx = st->vel[0];
                f32 vy = st->vel[1];
                f32 v = boot_sqrtf_v(vx * vx + vy * vy);
                f32 sinDir = negSinYaw;
                f32 cosDir;
                f32 wantRate, yawRate, gain;

                /* Moving (5 km/h on): the heading of the velocity instead of
                 * the yaw. */
                if (v < 1.388889f) {
                    cosDir = cosYaw;
                } else {
                    f32 velHeading = atan2f(-vx, vy);

                    sinDir = -sinf(velHeading);
                    cosDir = cosf(velHeading);
                }
                /* The yaw rate of the arc through the look-ahead point:
                 * 2 v x lateral / d^2. */
                wantRate = (v * -2.0f * (cosDir * point[0][0] - sinDir * point[0][1])) / dist2;
                if (st->speedX < 0.0f)
                    wantRate = -wantRate;
                yawRate = st->yawRate;
#if ENH_AI_SLIP_STEER
                /* Lupo 2: the gain eased by the front wheels' slip, x (peak -
                 * frontSlip) / peak held to +-1 (L2 0x0020234C..0x002023A8;
                 * frontSlip: physics.md 8, 0 until the physics writes it) */
                {
                    f32 ease = -(car->frontSlip - maxSteer) / maxSteer;

                    if (1.0f < ease)
                        ease = 1.0f;
                    else if (ease < -1.0f)
                        ease = -1.0f;
                    gain = p->steerGain * ease;
                }
#else
                gain = p->steerGain;
#endif
                if (0.0f < wantRate) {
                    if (yawRate < wantRate) {
                        steer += (wantRate - yawRate) * gain;
                        if (maxSteer < steer) {
                            slide = steer / maxSteer;
                            if (dc->isDirtCourse() == 0)
                                steer = maxSteer;
                        }
                    }
                } else if (wantRate < 0.0f && wantRate < yawRate) {
                    steer -= (yawRate - wantRate) * gain;
                    if (steer < -maxSteer) {
                        slide = -steer / maxSteer;
                        if (dc->isDirtCourse() == 0)
                            steer = -maxSteer;
                    }
                }
            }
            /* Plus the front wheels' velocity angle (the slip). */
            steer += (st->wheel[0].velAngle + st->wheel[1].velAngle) * 0.5f;
            if (0.7853982f < steer)         /* 45 deg */
                steer = 0.7853982f;
            else if (steer < -0.7853982f)
                steer = -0.7853982f;
        } else if (heading < -0.34906584f) {    /* 20 deg: 30 deg of lock */
            steer = -0.5235988f;
        } else if (0.34906584f < heading) {
            steer = 0.5235988f;
        } else {
            steer = heading * 1.5000001f;
        }
        if (dir == -1)
            steer = -steer;
    }

    if (st->ctrlMode == CTRL_STOP_ROLL || st->ctrlMode == CTRL_STOP_BRAKE) {
        ai_stop(ai, steer);
        return;
    }
    if (st->driver == 1 || st->driver == 2 || st->ctrlMode != CTRL_DRIVER)
        ai_steer_toward(ai, steer);

    if (drive == 1) {
        u8 state;
        f32 target, next, throttle;

        ai->inPitLane = 0;
        if (ai->line == LINE_PIT_LANE) {
            target = 5.555556f;             /* 20 km/h */
            state = AI_BRAKE;
            ai->inPitLane = 1;
            next = target;
        } else if (ai->line != LINE_PIT_ENTRY) {
            target = ai->segSpeed;
            next = ai->nextSpeed;
            state = ai->state;
            if (ai->unk_AE != POD_STOP_NONE) {
                /* Lupo 2 (L2 0x00202564..0x002026A8): the Pod race's stop -
                 * braking to the point (ai_brake_factor set it), 10 km/h for
                 * the last 20 m when slow, still within 0.5 m; held there
                 * while the stop is pending, then on */
                f32 stopSpeed = 0.0f;

                if (ai->unk_AE == POD_STOP_APPROACH) {
                    f32 x, y, at, d, half, len, speed;

                    pod_rules_get_stop_point(dc, st->carIndex, &x, &y, &at);
                    d = at - st->lapDistance;
                    len = dc->getLapLength();
                    half = len * 0.5f;
                    if (half < d)
                        d -= len;
                    else if (at < -half)    /* (Lupo 2 tests the point's distance here, not d: never) */
                        d += len;
                    speed = boot_sqrtf_v(st->speedX * st->speedX + st->speedY * st->speedY);
                    if (d < 0.5f) {
                        if (speed < 0.2777778f)             /* 1 km/h */
                            ai->unk_AE = POD_STOP_STOPPED;
                    } else if (speed < 2.777778f) {         /* 10 km/h */
                        if (d < 20.0f)
                            stopSpeed = 2.777778f;
                        else
                            ai->unk_AE = POD_STOP_NONE;     /* short of it: drive on to it */
                    }
                } else if (ai->unk_AE == POD_STOP_STOPPED) {
                    if (pod_rules_pending(dc, st->carIndex) == 0)
                        ai->unk_AE = POD_STOP_NONE;         /* it sat down: on */
                }
                if (ai->unk_AE != POD_STOP_NONE) {
                    target = stopSpeed;
                    next = stopSpeed;
                }
            }
            if (ai->recover == AI_TURNING)
                target = 13.888889f;        /* 50 km/h */
        } else {
            /* The way into the pit lane, along line 4's nodes: the pit
             * speed from node 2 to node n - 3. */
            f32 lap = lines->course->getLapLength();   /* F_0018FA10 */
            Line *pit = lines->data->line[LINE_PIT_LANE];
            int n = pit->count;
            LineNode *pitNodes = pit->node;
            f32 at = st->lapDistance;

            if (at < lap * 0.5f)
                at += lap;                  /* line 4 runs on past the lap */
            if (at < pitNodes[1].dist) {
                target = ai->segSpeed;
                state = ai->state;
                next = ai->nextSpeed;
            } else if (at < pitNodes[2].dist) {
                next = p->pitSpeed + 0.5555556f;
                target = ai->segSpeed;
                state = ai->state;
            } else {
                state = AI_CORNER_POS;
                if (at < pitNodes[n - 3].dist) {
                    target = p->pitSpeed + 0.5555556f;
                    ai->inPitLane = 1;
                    next = target;
                } else {
                    target = ai->segSpeed;
                    next = ai->nextSpeed;
                    state = ai->state;
                }
            }
        }

        if (ai->recover == AI_TURNING) {
            ai_hold_speed(ai, target, -0.5f);
            ai->holdCount = 0;
            throttle = ai->throttle;
        } else {
            switch (state) {
            case AI_BRAKE:
                ai_hold_speed(ai, next, -2.0f);
                ai->holdCount = 0;
                throttle = ai->throttle;
                break;
            case AI_BRAKE_CORNER:
                ai_hold_speed(ai, next, -1.0f);
                ai->holdCount++;
                throttle = ai->throttle;
                break;
            case AI_CORNER_POS:
            case AI_CORNER_NEG:
                if (!(st->flags & DYN_FLAG_COUNTED_GROUND) && !((int)ai->holdCount < p->bonusDelay))
                    target += p->cornerBonus;
                ai_hold_speed(ai, target, -0.5f);
                throttle = ai->throttle;
                /* Retail keeps the three stores apart (li; sb; b each); egcs
                 * cross-jumps them into one sb here, however the arms are
                 * written (ternaries, else-if chains, a switch, statement
                 * macros, a local: build/scratch/u086/drv_edits.py bs1..bs10). */
                if (ai->brakeState == BRAKE_STATE_NONE) {
                    if (throttle == 1.0f) {
                        if (state == AI_CORNER_POS)
                            ai->brakeState = BRAKE_STATE_FULL_POS;
                        else
                            ai->brakeState = BRAKE_STATE_FULL_NEG;
                    } else {
                        ai->brakeState = BRAKE_STATE_PART;
                    }
                }
                ai->holdCount++;
                break;
            case AI_STRAIGHT:
                ai->holdCount = 0;
                ai->throttle = 1.0f;
                throttle = 1.0f;
                break;
            default:
                throttle = ai->throttle;
                break;
            }
            if (ai->holdCount >= 6001)
                ai->holdCount = 6000;
        }
        ai->throttle = throttle * brakeFactor;
        if (st->rank * 100 + 5000 < (int)(dc->clock / 3))
            ai_avoid_traffic(ai);
    } else {
        if (drive == -1) {
            ai->reverse = 1;
            ai->throttle = 1.0f;
        } else {
            ai->throttle = 0.0f;
        }
        if (braking == 0)
            ai->brake = 0.0f;
        else
            ai->brake = 1.0f;
        ai->holdCount = 0;
    }

    /* Beyond the steering limit, in gear 2 and up: lift and brake. */
    if (0.0f < slide && !(st->gear < 2)) {
        f32 over = slide - 1.0f;
        f32 keep = 1.0f - p->slideThrottle * over;

        if (keep < 0.0f)
            keep = 0.0f;
        ai->throttle *= keep;
        over *= p->slideBrake;
        ai->brake += over;
        if (1.0f < ai->brake)
            ai->brake = 1.0f;
    }
}
