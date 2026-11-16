/* T301 - the car's attitude, velocities and speed: 0x001EA4D8..0x001EAA88.
 *
 * car_ground_attitude derives a ground position / pitch / roll from the
 * four wheel heights (the "ground" frame); car_local_speed projects the
 * world velocity on that frame (forward, lateral) and car_wheel_velocities
 * adds the yaw rate times each wheel's offset; car_wheel_spin turns the
 * wheel velocity into the tread speed and the axle speeds; car_speed_step
 * is the speed part of the frame (trip meter, condition, top speed). */
#include "game/car/physics/car_motion_internal.h"
#include "game/car/physics/T301_car_motion.h"

/* From DynCar_place (T330) and car_step.  The left / right heights are
 * taken at the centre between the axles (rearRatio of the way from the
 * front wheel to the rear one). */
// FUNC 0x001EA4D8 car_ground_attitude
void car_ground_attitude(RaceCarState *car)
{
    DynCarState *st = &car->st;
    CarParams *p = &car->params;
    f32 ratio = p->rearRatio;
    f32 x = st->pos[0];
    f32 y = st->pos[1];
    f32 h[4];

    for (int i = 0; i < 4; i++)
        h[i] = st->wheel[i].groundZ2;
    st->groundPos[0] = x;
    st->groundPos[1] = y;
    f32 left = h[0] + (h[2] - h[0]) * ratio;
    f32 right = h[1] + (h[3] - h[1]) * ratio;
    st->groundPos[2] = (left + right) * 0.5f;
    st->groundPitch = atan2f(((h[0] + h[1]) - (h[2] + h[3])) * 0.5f, p->wheelbase);
    st->groundRoll = atan2f(left - right, p->track);
    eulerToAxes(st->groundRot[0], st->groundRot[1], st->groundRot[2],
                st->groundPitch, st->groundRoll, st->yaw);
}

/* wheel velocity = car velocity (forward, lateral) + yaw rate x the
 * wheel's offset (half tracks, axle distances). */
// FUNC 0x001EA5F8 car_wheel_velocities
void car_wheel_velocities(RaceCarState *car)
{
    CarParams *p = &car->params;
    DynCarState *st = &car->st;
    f32 yawRate = st->yawRate;
    f32 dvY[4];                                 /* the yaw rate's part of velY */
    f32 dvX[4];                                 /* of velX */

    dvY[0] = dvY[1] = -p->frontDist * yawRate;
    dvY[2] = dvY[3] = p->rearDist * yawRate;
    dvX[0] = -p->halfTrack[0] * yawRate;
    dvX[1] = -dvX[0];
    dvX[2] = -p->halfTrack[1] * yawRate;
    dvX[3] = -dvX[2];
    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        w->velX = st->speedX + dvX[i];
        w->velY = st->speedY + dvY[i];
    }
}

/* speedX / speedY = the world velocity along the ground frame's first two
 * rows. */
// FUNC 0x001EA6B0 car_local_speed
void car_local_speed(RaceCarState *car)
{
    DynCarState *st = &car->st;
    f32 speedX = 0.0f;
    f32 speedY = 0.0f;

    for (int i = 0; i < 3; i++) {
        f32 v = st->vel[i];
        speedX += st->groundRot[0][i] * v;
        speedY += st->groundRot[1][i] * v;
    }
    st->speedX = speedX;
    st->speedY = speedY;
    car_wheel_velocities(car);
}

/* The tread speed of every wheel with no DynWheel.flags bit set (its
 * velocity along the steered wheel); an axle's speed follows the mean tread
 * speed of its two wheels when one of them was set or its differential type
 * is 0. */
// FUNC 0x001EA720 car_wheel_spin
void car_wheel_spin(RaceCarState *car)
{
    DynCarState *st = &car->st;
    int axleSet[2];
    int i;

    axleSet[1] = 0;
    axleSet[0] = 0;
    for (i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        if (w->flags == 0) {
            f32 steer = w->steer;
            f32 along = cosf(steer) * w->velX;
            f32 across = sinf(steer) * w->velY;
            axleSet[i >> 1] = 1;
            w->spin = along - across;
        }
    }
    for (i = 0; i < 2; i++) {
        if (axleSet[i] != 0 || car->params.gear.diffType[i] == 0) {
            f32 mean = (st->wheel[i * 2].spin + st->wheel[i * 2 + 1].spin) * 0.5f;
            st->axleSpeed[i] = car->params.gear.tyreRadiusInv[i] * mean;
        }
    }
}

/* speedView follows speed at dt x 111.1 m/s (400 km/h) per second. */
// FUNC 0x001EA870 car_speed_view
void car_speed_view(RaceCarState *car)
{
    f32 target = car->speed;
    f32 view = car->speedView;
    f32 step = car->dt * KMH_400;

    if (view < target) {
        car->speedView = view + step;
        if (target < car->speedView)
            car->speedView = target;
    } else {
        car->speedView = view - step;
        if (car->speedView < target)
            car->speedView = target;
    }
}

/* From DynCar_initialize (T330).  Copies the condition (unaligned
 * doubleword copies) and clears tripStep. */
// FUNC 0x001EA8C0 car_set_condition
void car_set_condition(RaceCarState *car, const CarCondition *c)
{
    car->st.condition = *c;
    car->st.tripStep = 0.0f;
}

/* Adds this frame's distance to the condition (addOdometer keeps the
 * remainder in tripStep) and to the trip meter (modulo 8,000,000 m). */
// FUNC 0x001EA908 car_trip
void car_trip(RaceCarState *car)
{
    f32 speed = car->speed;
    DynCarState *st = &car->st;

    if (!(speed < 0.0f)) {
        speed *= car->dt;                      /* the distance of this frame */
        st->tripStep = st->condition.addOdometer(st->tripStep + speed);
        f32 trip = st->trip + speed;
        st->trip = trip;
        if (8e6f <= trip)
            st->trip = trip - 8e6f;
    }
}

/* From RaceDisplay::virtual_2.  The trip in the speed unit (x 0.625 for
 * miles). */
// FUNC 0x001EA988 car_trip_display
f32 car_trip_display(RaceCarState *car)
{
    f32 trip = car->st.trip;
    if (g_002F59D8.system == UNIT_IMPERIAL)
        trip *= 0.625f;
    return trip;
}

// FUNC 0x001EA9B8 car_speed_step
void car_speed_step(RaceCarState *car)
{
    DynCarState *st = &car->st;

    car_local_speed(car);
    car->speed = st->speedX;
    car_trip(car);
    if (car->speed < 0.0f)
        car->speed = -car->speed;
    f32 dist = g_002D2B00.distance(car);
    st->condition.addEngineMileage(dist);
    st->condition.addOilMileage(dist);
    if (st->finished == 0) {
        if (car->topSpeed < car->speed)
            car->topSpeed = car->speed;
    }
    car_speed_view(car);
    car_wheel_spin(car);
}
