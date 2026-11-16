/* T298 - the ground contact and the suspension of a car, the pit stop jack
 * and the body's pitch / roll / heave motion (callers: car_step, T302, in
 * this order: car_ground_contact, car_susp_prepare, car_suspension_forces,
 * car_body_motion, car_ground_penetration). */
/* Retail T298's .rodata (0x0033C898, nothing references it: DATA_START):
 * "bad_alloc", "t9UnitArena1Ui64" (the race headers, as T296). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T298_suspension_body.h"
#include "game/car/physics/suspension_internal.h"
// DATA_START 0x0033C898

// FUNC 0x001E8AE0 surface_bump_wave
/* The bump pattern of a surface at (x, z): sin(k x) + cos(k z); kind 1
 * squares it (keeping the sign). */
f32 surface_bump_wave(int kind, f32 x, f32 z, f32 waveNumber)
{
    f32 wave = sinf(waveNumber * x) + cosf(waveNumber * z);

    switch (kind) {
    case 0:
        break;
    case 1: {
        f32 squared = wave * wave;
        if (wave < 0.0f)
            squared = -squared;
        wave = squared;
        break;
    }
    }
    return wave;
}

/* The bump height of a wheel on the surface it stands on (car_ground_contact):
 * the surface's bump wave x its amplitude, x the wheel's speed factor on
 * the surfaces whose bumps grow with the speed.  (Matching: an inline helper;
 * written in the loop, loop.c's second pass strength-reduces the contact
 * pointer, which retail computes from i every pass.) */
static inline f32 wheel_bump(GroundQuery *c, CarWheelState *w)
{
    u8 surface = w->surface;
    f32 amplitude = g_002D2CE0.bumpAmplitude[surface];
    if (amplitude == 0.0f)
        return 0.0f;
    f32 wave = surface_bump_wave(g_002D2CE0.bumpKind[surface], c->x, c->z,
                                 g_002D2CE0.bumpWaveNumber[surface]);
    if (g_002D2CE0.bumpSpeed[w->surface] != 0)
        wave *= w->speedFactor;
    return wave * amplitude;
}

// FUNC 0x001E8B68 car_ground_contact
/* Queries the road under the 4 wheels (RaceCourse::getContacts, course
 * axes (x, z, -y)); per wheel: the ground height, the surface and its
 * attribute bytes, the surface's bump height (x speedFactor on the surfaces
 * whose bumps grow with the speed); the runway sector of wheel 0; flag 0x10
 * when a wheel is on counted ground and the conductor allows it
 * (isPitEnabled); then the conductor's course-out rule gets the counts of
 * wheels on counted / off-course ground.  (Matching, 95.6%: left is the
 * surface's register in the bump block, $a1 in retail with its x 4 in $a0
 * (here $a0 / $v1), so the call's delay slot takes the kind load there.
 * Tried: int / u8 / u32 surface, locals for the kind and the wave number,
 * the helper's signatures, the surface as w->surface at each use.) */
void car_ground_contact(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    GroundQuery *contact = car->contact;
    int countedWheels = 0;
    int offCourseWheels = 0;
    RaceCourse *course = car->course;
    int i;

    /* (i only counts here: the loop runs down to 0, as retail's) */
    {
        GroundQuery *c = contact;
        CarWheelState *w = st->wheel;
        for (i = 0; i < 4; i++) {
            f32 wheelPos[3];                /* sp+0x0 (a local copy, not used further) */
            wheelPos[1] = st->pos[1] + w->offY;
            wheelPos[0] = st->pos[0] + w->offX;
            wheelPos[2] = st->pos[2] + w->offZ;
            c->x = wheelPos[0];
            c->y = wheelPos[2];
            c->z = -wheelPos[1];
            c++;
            w++;
        }
    }
    course->getContacts(contact);

    for (i = 0; i < 4; i++) {
        GroundQuery *c = &car->contact[i];
        CarWheelState *w = &st->wheel[i];
        if (c->offRoad) {
            w->bump = 0.0f;
            continue;
        }
        w->groundZ = w->groundZ2 = c->height;
        w->surface = ground_surface_type(c);
        w->b25 = c->unk_32;
        w->b26 = c->attr2;
        countedWheels += c->attr2 & ATTR2_COUNTED;
        if (ground_is_off_course(c) != 0)
            offCourseWheels++;
        w->bump = wheel_bump(c, w);         /* implicit: surface_bump_wave */
    }

    st->sector = contact[0].sector;
    st->flags = st->flags & ~DYN_FLAG_COUNTED_GROUND;
    if (countedWheels != 0) {
        if (car->dc->isPitEnabled() != 0)
            st->flags = st->flags | DYN_FLAG_COUNTED_GROUND;
    }
    car->dc->chargePenaltyBySurface(car, countedWheels, offCourseWheels);
}

// FUNC 0x001E8DD0 susp_damper_force
/* The damper force at a stroke speed (clamped to +-1): bump (speed < 0)
 * base + gain x sqrt(-speed - ofs), rebound -base - gain x sqrt(speed -
 * ofs). */
f32 susp_damper_force(SuspParam *s, f32 speed)
{
    if (speed < 0.0f) {
        if (speed < -1.0f)
            speed = -1.0f;
        return s->bumpBase + s->bumpGain * boot_sqrtf_v(-speed - s->bumpOfs);
    }
    if (speed > 1.0f)
        speed = 1.0f;
    f32 root = boot_sqrtf_v(speed - s->reboundOfs);
    return -s->reboundBase - s->reboundGain * root;
}

// FUNC 0x001E8E58 susp_wheel_force
/* The suspension force of a wheel at the gap `gap': beyond strokeMax the
 * wheel hangs (no load, the force -unsprungWeight); else the spring (the
 * stroke held at strokeMin or more), the bump stop below bumpStart, the
 * damper (with extra bump stop damping); the wheel's load = force +
 * unsprungWeight (>= 0).  Returns the spring + damper force.  (The wheel
 * index is passed but not used.) */
f32 susp_wheel_force(CarDynState *st, CarWheelState *w, SuspParam *s, int wheel, f32 gap)
{
    if (s->strokeMax < gap) {
        w->stroke = s->strokeMax;
        w->load = 0.0f;
        w->strokeSpeed = 0.0f;
        w->springForce = -s->unsprungWeight;
        w->damperForce = 0.0f;
        return -s->unsprungWeight;
    }

    f32 stroke = (gap < s->strokeMin) ? s->strokeMin : gap;
    f32 intoBumpStop = stroke - s->bumpStart;
    f32 spring = -stroke * s->spring;
    if (intoBumpStop < 0.0f)
        spring += intoBumpStop * intoBumpStop * s->bumpStop;

    f32 speed = st->invDt * (gap - w->stroke);
    w->strokeSpeed = speed;
    f32 damper = susp_damper_force(s, speed);
    if (speed < 0.0f && intoBumpStop < 0.0f) {
        if (speed < -1.0f)
            speed = -1.0f;
        damper -= s->bumpStopDamp * speed;
    }
    w->damperForce = damper;
    f32 force = spring + damper;
    w->stroke = stroke;
    w->springForce = spring;
    f32 load = force + s->unsprungWeight;
    w->load = load;
    if (load < 0.0f)
        w->load = 0.0f;
    return force;
}

// FUNC 0x001E8FD0 susp_antiroll_force
/* The anti-roll bar force between the wheels a and b: antiRoll x d x |d|,
 * d = the stroke difference. */
f32 susp_antiroll_force(CarWheelState *a, CarWheelState *b, SuspParam *s)
{
    f32 d = a->stroke - b->stroke;
    if (0.0f <= d)
        return s->antiRoll * d * d;
    return -s->antiRoll * d * d;
}

// FUNC 0x001E9010 car_wheel_gaps
/* The gap of each wheel: (the body height at the wheel - the ground - the
 * bump) x groundCos + (the axle's gapOffset - its tyre radius). */
void car_wheel_gaps(CarDynState *st, CarParams *p, f32 groundCos)
{
    f32 offset[4];                          /* sp+0x0 */
    f32 front = -p->gear.tyreRadius[0] + p->susp[0].gapOffset;
    f32 rear = -p->gear.tyreRadius[1] + p->susp[1].gapOffset;
    f32 z = st->pos[2];
    int i;

    offset[0] = offset[1] = front;
    offset[2] = offset[3] = rear;
    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        st->wheelGap[i] = (((z + w->offZ) - w->groundZ) - w->bump) * groundCos + offset[i];
    }
}

// FUNC 0x001E90A0 car_wheel_ground_bits
/* Sets bit i of bits for each wheel whose gap is below its axle's
 * strokeMin (the wheel is pressed into the ground). */
int car_wheel_ground_bits(CarDynState *st, CarParams *p, int bits)
{
    int bit = 1;
    int i;

    for (i = 0; i < 4; i++) {
        if (st->wheelGap[i] < p->susp[i >> 1].strokeMin)
            bits |= bit;
        bit <<= 1;
    }
    return bits;
}

// FUNC 0x001E90F8 pit_jack_lift
/* The jack's lift of a car in its pit stop, by the pit timer (300 frames
 * at the box, counting down): the first 30 frames phase 1, rising to 0.3;
 * down to 30 frames phase 2, 0.3; the last 30 phase 0, 0 (the car comes
 * down). */
f32 pit_jack_lift(int timer, int *phase)
{
    if (timer >= 270) {
        *phase = 1;
        return ((f32)(300 - timer) * 0.29999998f) / 30.0f;
    }
    if (timer >= 30) {
        *phase = 2;
        return 0.29999998f;
    }
    *phase = 0;
    return 0.0f;
}

// FUNC 0x001E9160 car_hold_lift
/* Holds the car at least `lift' above the ground point (groundPos): then
 * clears the vertical speed, pitch, roll and their rates.  1 if it did. */
int car_hold_lift(CarDynState *st, f32 lift)
{
    f32 z = st->groundPos[2] + lift;
    if (z < st->pos[2])
        return 0;
    st->pos[2] = z;
    st->vel[2] = st->pitchRate = st->rollRate = st->pitch = st->roll = 0.0f;
    return 1;
}

// FUNC 0x001E91A8 car_susp_prepare
/* The ground's cosine to the body (the ground normal's z / the body's up
 * z), the wheel gaps and the ground bits. */
void car_susp_prepare(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;

    f32 groundCos = st->groundRot[2][2] / st->rot[2][2];
    st->groundCos = groundCos;
    car_wheel_gaps(st, p, groundCos);
    st->groundBits = (u8)car_wheel_ground_bits(st, p, 0);
}

// FUNC 0x001E9218 wheel_camber
/* The camber of a wheel: the axle's camber + camberGain x stroke; on the
 * front axle the steering adds t = casterCamber x steer x the wheel's
 * steering share (picked by the sign of t; + for wheel 0, - for wheel 1). */
f32 wheel_camber(CarParams *p, int wheel, f32 steer, f32 stroke)
{
    int axle = wheel >> 1;
    f32 camber = p->camber[axle] + p->camberGain[axle] * stroke;

    if (wheel < 2) {
        f32 caster = p->casterCamber;
        if (caster != 0.0f) {
            f32 t = steer * caster;
            if (0.0f <= t) {
                if (wheel == 0)
                    camber += p->steerGain[0] * t;
                else
                    camber -= p->steerGain[1] * t;
            } else {
                if (wheel == 0)
                    camber += p->steerGain[1] * t;
                else
                    camber -= p->steerGain[0] * t;
            }
        }
    }
    return camber;
}

// FUNC 0x001E92A8 wheel_lateral_pos
/* The y of a wheel's contact point: +-half track (even wheels negative),
 * less (1 - cos(atan2(stroke, swingArm))) x swingArm.  (The steering is
 * passed but not used.) */
f32 wheel_lateral_pos(CarParams *p, int wheel, f32 steer, f32 stroke)
{
    int axle = wheel >> 1;
    f32 arm = p->swingArm[axle];
    f32 y;

    if (axle == 0)
        y = p->halfTrack[0];
    else
        y = p->halfTrack[1];
    if (arm != 0.0f)
        y -= (1.0f - cosf(atan2f(stroke, arm))) * arm;
    if ((wheel & 1) == 0)
        y = -y;
    return y;
}

// FUNC 0x001E9348 car_suspension_forces
/* The suspension force, track position and camber of each wheel (the
 * stroke from the axle's static stroke), the anti-roll bars, then the
 * vertical force of each corner: spring -+ anti-roll - half the axle's
 * aerodynamic lift, with the load transfer of the longitudinal force
 * (longTransfer) and of the lateral acceleration (latTransfer).  (Matching:
 * the anti-roll bars' wheels and axles are locals set at the start, as retail
 * computes their addresses there; the lift terms written in place.) */
void car_suspension_forces(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    CarWheelState *w0 = &st->wheel[0];
    CarWheelState *w1 = &st->wheel[1];
    CarWheelState *w2 = &st->wheel[2];
    CarWheelState *w3 = &st->wheel[3];
    SuspParam *front = &p->susp[0];
    SuspParam *rear = &p->susp[1];
    int i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        SuspParam *s = &p->susp[i >> 1];
        susp_wheel_force(st, w, s, i, st->wheelGap[i]);
        f32 fromStatic = w->stroke - s->staticStroke;
        w->lateralPos = wheel_lateral_pos(p, i, st->steer, fromStatic);
        w->camber = wheel_camber(p, i, st->steer, fromStatic);
    }

    f32 antiRollFront = susp_antiroll_force(w0, w1, front);
    f32 antiRollRear = susp_antiroll_force(w2, w3, rear);
    f32 longTransfer = p->longTransfer * (-st->unk_4D0 + st->longForce);
    f32 latTransfer = p->latTransfer * -st->accelY;

    st->wheelForce[0] = st->wheel[0].springForce - antiRollFront - st->lift[0] * 0.5f + longTransfer + latTransfer;
    st->wheelForce[1] = st->wheel[1].springForce + antiRollFront - st->lift[0] * 0.5f + longTransfer - latTransfer;
    st->wheelForce[2] = st->wheel[2].springForce - antiRollRear - st->lift[1] * 0.5f - longTransfer + latTransfer;
    st->wheelForce[3] = st->wheel[3].springForce + antiRollRear - st->lift[1] * 0.5f - longTransfer - latTransfer;
}

// FUNC 0x001E9528 car_pit_jack
/* While the pit timer runs: the jack holds the car up (pit_jack_lift /
 * car_hold_lift); in phase 2 the yaw turns toward the pit box's heading
 * (DynamicsConductor::pitNodeAngle of pitBox) by at most 30 degrees a second;
 * then the body matrix.  1 if the jack holds the car (car_body_motion is
 * skipped then).  (Matching: the hold test as an if around the jack's work
 * with return 1 inside, then return 0: retail reloads pitTimer for the call,
 * which the two early returns do not give.) */
int car_pit_jack(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    f32 dt = car->dt;
    int phase;

    if (st->pitTimer == 0)
        return 0;
    if (car_hold_lift(st, pit_jack_lift(st->pitTimer, &phase)) != 0) {
        if (phase == 2) {
            f32 heading = car->dc->pitNodeAngle(st->pitBox);
            f32 diff = angleDiff(heading, st->yaw);
            if (diff != 0.0f) {
                f32 step = dt * (30.0f * U081_DEG);
                if (step < diff)
                    st->yaw = st->yaw + step;
                else if (diff < -step)
                    st->yaw = st->yaw - step;
                else
                    st->yaw = heading;
            }
        }
        eulerToAxes(st->rot[0], st->rot[1], st->rot[2], st->pitch, st->roll, st->yaw);
        return 1;
    }
    return 0;
}

// FUNC 0x001E9640 car_drive_pitch_heave
/* The pitch moment (*pitch) and the heave force (*heave) of the wheels'
 * drive / brake forces (x the tyre width, x unk_130 for the moment). */
void car_drive_pitch_heave(RaceCarDynamics *car, f32 *pitch, f32 *heave)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    f32 heaveForce = 0.0f;
    f32 moment = heaveForce;
    int i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        f32 force = st->tyreForce[i].cosSteer * (w->driveForce + w->brakeForce)
                  * p->gear.tyreWidth[i >> 1];
        moment += force * p->unk_130[i >> 1];
        if (i < 2)
            heaveForce += force / p->frontDist;
        else
            heaveForce -= force / p->rearDist;
    }
    *pitch = moment;
    *heave = heaveForce;
}

// FUNC 0x001E96F0 car_body_motion
/* Unless the pit jack holds the car: the pitch and roll rates from the
 * corner forces (within +-pi/2), pitch and roll (within +-pi/4, the rate
 * then 0), the body matrix, the heave acceleration and the velocity along
 * the body's up axis, and gravity.  (Matching, 95.3%: left is the gravity
 * term of the heave: retail loads 9.8 and rot[2][2] and multiplies them
 * before the last three loads of the sum (wheel[3].damperForce, drive[1],
 * invSprungMass), which sched1's priorities never do here; one .lit4
 * reference follows from it.  Tried: the term first, as a local, a gravity
 * local, the heave / dv orders, the sum split; 991111-01 gives the same.) */
void car_body_motion(RaceCarDynamics *car)
{
    f32 dt = car->dt;
    CarDynState *st = &car->st;
    f32 drive[2];                           /* sp+0x0: [0] pitch moment, [1] heave force */

    car_drive_pitch_heave(car, &drive[0], &drive[1]);
    CarParams *p = &car->params;
    if (car_pit_jack(car) != 0)
        return;

    f32 force0 = st->wheelForce[0];
    f32 force1 = st->wheelForce[1];
    f32 damper0 = st->wheel[0].damperForce;
    f32 damper1 = st->wheel[1].damperForce;
    f32 force2 = st->wheelForce[2];
    f32 force3 = st->wheelForce[3];
    f32 damper2 = st->wheel[2].damperForce;
    f32 damper3 = st->wheel[3].damperForce;

    f32 rate = st->pitchRate
             + ((((force0 + force1 + damper0 + damper1) * p->pitchFront)
                 - ((force2 + force3 + damper2 + damper3) * p->pitchRear)) + drive[0]) * dt;
    if (rate > U081_HALF_PI)
        rate = U081_HALF_PI;
    else if (rate < -U081_HALF_PI)
        rate = -U081_HALF_PI;
    f32 angle = st->pitch + rate * dt;
    st->pitch = angle;
    if (angle < -U081_QUARTER_PI) {
        st->pitch = -U081_QUARTER_PI;
        rate = 0.0f;
    } else if (angle > U081_QUARTER_PI) {
        st->pitch = U081_QUARTER_PI;
        rate = 0.0f;
    }
    st->pitchRate = rate;

    rate = st->rollRate
         + (((((force0 - force1) + damper0) - damper1) * p->rollFront)
            + ((((force2 - force3) + damper2) - damper3) * p->rollRear)) * dt;
    if (rate > U081_HALF_PI)
        rate = U081_HALF_PI;
    else if (rate < -U081_HALF_PI)
        rate = -U081_HALF_PI;
    angle = st->roll + rate * dt;
    st->roll = angle;
    if (angle < -U081_QUARTER_PI) {
        st->roll = -U081_QUARTER_PI;
        rate = 0.0f;
    } else if (angle > U081_QUARTER_PI) {
        st->roll = U081_QUARTER_PI;
        rate = 0.0f;
    }
    st->rollRate = rate;
    eulerToAxes(st->rot[0], st->rot[1], st->rot[2], st->pitch, st->roll, st->yaw);

    f32 upAccel = (st->wheelForce[0] + st->wheelForce[1] + st->wheelForce[2] + st->wheelForce[3]
                   + st->wheel[0].damperForce + st->wheel[1].damperForce
                   + st->wheel[2].damperForce + st->wheel[3].damperForce + drive[1])
                * p->invSprungMass;
    st->heaveAccel = upAccel - st->rot[2][2] * U081_G;
    f32 dv = upAccel * dt;
    int axis;
    for (axis = 0; axis < 3; axis++)
        st->vel[axis] += st->rot[2][axis] * dv;
    st->vel[2] = st->vel[2] - dt * U081_G;
}

// FUNC 0x001E9998 car_ground_penetration
/* When wheels were pressed into the ground: re-runs the gaps with the
 * wheel heights; for the wheels still below (low 4 bits) the body is
 * raised by half the smaller side / axle penetration and rolled / pitched
 * by atan2 of the difference over the track / wheelbase (the sides blend
 * front and rear by rearRatio).  The body matrix is rebuilt and the
 * vertical speed is held at least at the ground plane's (<= 0). */
void car_ground_penetration(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;

    if (st->groundBits == 0)
        return;

    car_wheel_heights(car);
    car_wheel_gaps(st, p, st->groundCos);
    int bits = car_wheel_ground_bits(st, p, st->groundBits << 4);
    st->groundBits = (u8)bits;

    if (bits & 0xF) {
        f32 lift = 0.0f;
        f32 blend = p->rearRatio;
        f32 depth[4];                       /* sp+0x0 */
        int i;
        for (i = 0; i < 4; i++) {
            depth[i] = p->susp[i >> 1].strokeMin - st->wheelGap[i];
            if (depth[i] < 0.0f)
                depth[i] = 0.0f;
        }
        f32 front = (depth[0] + depth[1]) * 0.5f;
        f32 rear = (depth[2] + depth[3]) * 0.5f;
        f32 left = depth[0] + (depth[2] - depth[0]) * blend;
        f32 right = depth[1] + (depth[3] - depth[1]) * blend;
        if (left != 0.0f || right != 0.0f) {
            if (right < left) {
                st->roll = st->roll + atan2f(left - right, p->track);
                lift = right * 0.5f;
            } else if (left < right) {
                st->roll = st->roll + atan2f(left - right, p->track);
                lift = left * 0.5f;
            } else {
                lift = left * 0.5f;
            }
        }
        if (front != 0.0f || rear != 0.0f) {
            f32 axleLift;
            if (rear < front) {
                st->pitch = st->pitch + atan2f(front - rear, p->wheelbase);
                axleLift = rear * 0.5f;
            } else if (front < rear) {
                st->pitch = st->pitch + atan2f(front - rear, p->wheelbase);
                axleLift = front * 0.5f;
            } else {
                axleLift = front * 0.5f;
            }
            lift += axleLift;
        }
        st->pos[2] = st->pos[2] + lift;
    }

    eulerToAxes(st->rot[0], st->rot[1], st->rot[2], st->pitch, st->roll, st->yaw);
    f32 along0 = 0.0f;
    f32 along1 = 0.0f;
    f32 rise0 = st->groundRot[0][2];
    f32 rise1 = st->groundRot[1][2];
    f32 vz = st->vel[2];
    int axis;
    for (axis = 0; axis < 3; axis++) {
        f32 v = st->vel[axis];
        along0 += st->groundRot[0][axis] * v;
        along1 += st->groundRot[1][axis] * v;
    }
    f32 minVz = along0 * rise0 + along1 * rise1;
    if (minVz > 0.0f)
        minVz = 0.0f;
    if (vz < minVz)
        st->vel[2] = minVz;
}

