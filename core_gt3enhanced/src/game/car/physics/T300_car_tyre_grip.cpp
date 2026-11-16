/* T300 - tyre wear and wheel grip: 0x001E9E80..0x001EA4D8.
 *
 * car_wear_grip maps each wheel's wear (DynWheel.wear) through the race's
 * wear curve (DynamicsConductor::wear, four points) to a grip factor and a
 * wear level byte; car_wheel_grip combines the load curve of the tyre (over
 * the load per tyre width), the wear, the axle grip, the surface and a
 * camber curve into the wheel's grip; car_pre_step is the first part of a
 * car's frame. */
#include "game/car/physics/car_motion_internal.h"
#include "game/car/physics/T300_car_tyre_grip.h"

/* Returns 0 (nothing done) when the curve's last wear x3 is 0 or the car
 * has DYN_FLAG_COUNTED_GROUND.  The wear is clamped to x3.  Between the
 * points the grip loss runs linearly from y(i) to y(i + 1); wearLevel is
 * 0x40..0x7F between x2 and x3, 0x20..0x40 between x1 and x2, 0..0x20
 * between 0 and x1, 0..-0x3F between x0 and 0 and -0x80 below x0.  The two
 * upper segments are computed in double (the literal 1.0).  The tyre's
 * surfaceGrip[11] is the floor of the wear grip. */
// FUNC 0x001E9E80 car_wear_grip
int car_wear_grip(DynCarState *st, CarParams *p, WearCurve *wc)
{
    int enabled = 1;
    if (wc->x3 == 0.0f)
        enabled = 0;
    if (enabled == 0)
        return 0;
    if (st->flags & DYN_FLAG_COUNTED_GROUND)
        return 0;

    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        f32 wear = w->wear;
        if (wc->x3 <= wear) {
            w->wear = wc->x3;
            w->wearGrip = 1.0f - wc->y3;
            w->wearLevel = WEAR_LEVEL_MAX;
        } else if (wc->x2 <= wear) {
            f32 r = (wear - wc->x2) / (wc->x3 - wc->x2);
            /* implicit: double: fptodp, dpsub, dptofp (the soft-float runtime) */
            w->wearGrip = 1.0 - wc->y2 - r * (wc->y3 - wc->y2);
            if (1.0f <= r)
                w->wearLevel = WEAR_LEVEL_MAX;
            else
                w->wearLevel = (int)(r * 63.9f) + WEAR_LEVEL_X2;
        } else if (wc->x1 <= wear) {
            f32 r = (wear - wc->x1) / (wc->x2 - wc->x1);
            w->wearGrip = 1.0 - wc->y1 - r * (wc->y2 - wc->y1);
            if (1.0f <= r)
                w->wearLevel = WEAR_LEVEL_X2;
            else
                w->wearLevel = (int)(r * 31.9f) + WEAR_LEVEL_X1;
        } else if (0.0f <= wear) {
            f32 r = wear / wc->x1;
            w->wearGrip = 1.0f - wc->y1 * r;
            if (1.0f <= r)
                w->wearLevel = WEAR_LEVEL_X1;
            else
                w->wearLevel = (int)(r * 31.9f);
        } else if (wc->x0 <= wear) {
            f32 r = -wear / -wc->x0;
            w->wearGrip = 1.0f - wc->y0 * r;
            if (1.0f <= r)
                w->wearLevel = WEAR_LEVEL_X0;
            else
                w->wearLevel = -(int)(r * 63.9f);
        } else {
            w->wearLevel = WEAR_LEVEL_UNWORN;
            w->wearGrip = 1.0f;
        }
        TyreData *tyre = CarDynamics_getTyre(p, i, w->surface);
        if (w->wearGrip < tyre->surfaceGrip[11])    /* [11]: the wear floor */
            w->wearGrip = tyre->surfaceGrip[11];
    }
    return 1;
}

/* load / the tyre width of the wheel's axle (also CarDynamics_init, T284). */
// FUNC 0x001EA1C0 tyre_load_per_width
f32 tyre_load_per_width(CarParams *p, int wheel, f32 load)
{
    return load / p->gear.tyreWidth[wheel >> 1];
}

/* grip = load x loadCurve(load / width) [x wearGrip] x axleGrip x the
 * tyre's surface factor x camberCurve(+-(roll - groundRoll) + camber): the
 * body's roll on the ground counts with one sign on the left wheels (0, 2)
 * and the other on the right ones. */
// FUNC 0x001EA1E8 car_wheel_grip
void car_wheel_grip(RaceCarState *car)
{
    DynCarState *st = &car->st;
    CarParams *p = &car->params;
    int wearOn = car_wear_grip(st, p, &car->dc->wear);
    f32 sideRoll[2];
    f32 roll = st->roll - st->groundRoll;

    sideRoll[1] = roll;
    sideRoll[0] = -roll;
    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        TyreData *tyre = CarDynamics_getTyre(p, i, w->surface);
        f32 load = w->load;
        f32 perWidth = tyre_load_per_width(p, i, load);
        f32 grip = load * curveLookupLinear(&tyre->loadCurve, perWidth);
        if (wearOn)
            grip *= w->wearGrip;
        int axle = i >> 1;
        grip *= p->axleGrip[axle];
        f32 camberAngle = sideRoll[i & 1] + p->camber[axle];
        f32 camberGrip = curveLookupLinear(&p->camberCurve[axle], camberAngle);
        w->grip = grip * tyre->surfaceGrip[w->surface] * camberGrip;
    }
}

/* *factor = 0 below 1 km/h, 1 from 10 km/h, linear between (0.4 per m/s);
 * returns atan2f(-vy, vx), or 0 when the factor is 0. */
// FUNC 0x001EA348 wheel_low_speed_factor
f32 wheel_low_speed_factor(f32 *factor, f32 vx, f32 vy)
{
    f32 k = boot_sqrtf_v(vy * vy + vx * vx);   /* the speed, then the factor (one register) */

    if (KMH_10 <= k)
        k = 1.0f;
    else if (k <= KMH_1)
        k = 0.0f;
    else
        k = (k - KMH_1) * 0.4f;
    *factor = k;
    if (k == 0.0f)
        return 0.0f;
    return atan2f(-vy, vx);
}

// FUNC 0x001EA3D8 car_wheel_low_speed_factors
void car_wheel_low_speed_factors(RaceCarState *car)
{
    for (int i = 0; i < 4; i++) {
        DynWheel *w = &car->st.wheel[i];
        w->velAngle = wheel_low_speed_factor(&w->lowSpeedFactor, w->velX, w->velY);
    }
}

/* From DynamicsConductor::update (T317): stopped, the pit stop countdown,
 * the message timer, the frame time, the slipstream, the engine values,
 * the grip and the low speed factors. */
// FUNC 0x001EA428 car_pre_step
void car_pre_step(RaceCarState *car)
{
    DynCarState *st = &car->st;

    st->stopped = car_is_held(car) != 0;
    if (st->pitTimer != 0)
        st->pitTimer = st->pitTimer - 1;
    DynCar_tickMessage(car);
    car_time_scale(car);
    car_slipstream_update(car);
    st->engineTorque = engine_torque_at(&car->params.engine, &st->condition, st->engineSpeed);
    st->frictionTorque = engine_scale(&car->params.engine, &st->condition, st->engineSpeed);
    car_wheel_grip(car);
    car_wheel_low_speed_factors(car);
}
