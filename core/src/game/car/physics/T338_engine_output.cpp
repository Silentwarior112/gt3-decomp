/* T338 - the engine's output per frame: 0x001E1768..0x001E1B28. */
#include "game/car/physics/car_engine_internal.h"
#include "game/car/physics/T338_engine_output.h"

/* The rev limiter (cuts at the redline, back 500 rpm below), then the
 * torque load x (engineTorque + frictionTorque) - frictionTorque (u082: engine_torque_at and
 * engine_scale of the frame: the engine's torque and its friction, ?)
 * through the power limit at the engine speed.  Callers: engine_update. */
// FUNC 0x001E1768 engine_output
f32 engine_output(RaceCarState *car, f32 load)
{
    DynCarState *st = &car->st;
    CarEngine *e = &car->params.engine;
    f32 torque = 0.0f;

    /* the state read into an int first: retail reloads limiterOn after the
     * update (the same field read twice as one byte load lets GCSE carry
     * the stored values instead) */
    int on = st->limiterOn;
    if (on == 0) {
        if (st->rpm >= e->redline)
            st->limiterOn = 1;
    } else {
        if (st->rpm < e->redline - 500)
            st->limiterOn = 0;
    }
    if (st->limiterOn == 0)
        torque = load * (st->engineTorque + st->frictionTorque);
    return engine_power_limit(e, torque - st->frictionTorque, st->engineSpeed);
}

/* Without a turbo the throttle goes straight through.  With one: the spool
 * of each stage follows turboThrottle x rpm / (the boost factor at the
 * engine rpm) at its rate x dt; the turbo throttle becomes throttle x the
 * boost factor at the spools, the load passed on is its share of the full
 * factor.  Also the blow-off (blowOffState: armed / fired; blowOffLevel its level, a
 * lift below 0.75 from above 0.8 turbo throttle), turboFactor (the average
 * of itself and the turbo throttle + the lift-off part), the larger spool
 * and the engine sound level [2], the turbo's ((throttlePrev - 0.75)^2 x
 * 128 with the torque; 1 when there is no torque or no boost).  Returns the
 * torque.  Callers: car_drive_torque (T344), car_engine_speed (T296). */
// FUNC 0x001E17F0 engine_update
f32 engine_update(RaceCarState *car, f32 throttle)
{
    CarEngine *e = &car->params.engine;
    DynCarState *st = &car->st;
    f32 dt = car->dt;
    f32 load = throttle;

    if (e->turboMax == 0.0f) {
        st->turboThrottle = throttle;
        st->engineSnd[2] = 0;
        st->throttlePrev = throttle;
    } else {
        f32 boost[2];                       /* sp+0x00 */
        f32 full = turbo_boost_factor(e, st->rpm, st->rpm, boost);
        st->throttlePrev = st->turboThrottle;
        f32 target = (st->turboThrottle * (f32)st->rpm) / full;
        for (int i = 0; i < 2; i++) {
            if (e->turboBoost[i] != 0.0f) {
                st->turboSpool[i] += (int)((target - (f32)st->turboSpool[i]) * e->turboRate[i] * dt);
                if (st->turboSpool[i] < 0)
                    st->turboSpool[i] = 0;
            }
        }
        f32 out = throttle * turbo_boost_factor(e, st->turboSpool[0], st->turboSpool[1], boost);
        f32 liftK = 0.5f - (f32)st->rpm * 0.00024414062f * 0.25f;   /* 0.5 - rpm / 16384 */
        if (liftK < 0.0625f)
            liftK = 0.0625f;
        u8 blowOff = st->blowOffState;
        st->blowOffLevel = 0.0f;
        st->turboFactor = (st->turboFactor + (out + (1.0f - throttle) * liftK)) * 0.5f;
        switch (blowOff) {
        case BLOWOFF_READY:
            if (throttle < 0.75f) {
                f32 level = st->turboThrottle - 0.8f;
                if (level < st->blowOffLevel)
                    level = st->blowOffLevel;
                st->blowOffState = BLOWOFF_FIRED;
                st->blowOffLevel = level * 0.00390625f;              /* / 256 */
            }
            break;
        case BLOWOFF_FIRED:
            if (0.9f < throttle)
                st->blowOffState = BLOWOFF_READY;
            break;
        }
        load = 1.0f;
        st->turboThrottle = out;
        if (!(full <= out))
            load = out / full;
        st->turboSpoolMax = st->turboSpool[0];
        if (st->turboSpoolMax < st->turboSpool[1])
            st->turboSpoolMax = st->turboSpool[1];
    }

    f32 torque = engine_output(car, load);
    if (0.0f < e->turboMax) {
        f32 d = st->throttlePrev - 0.75f;
        if (d <= 0.0f || torque <= 0.0f) {
            st->engineSnd[2] = 1;
        } else {
            f32 m;
            if (588.0f <= torque)
                m = d;
            else
                m = d * torque * 0.0017006802f;                 /* / 588 (60 kgm) */
            int level = (int)(m * m * 128.0f);
            if (!(level < 0x80))
                level = 0x7F;
            if (!(0 < level))
                level = 1;
            st->engineSnd[2] = level;
        }
    }
    return torque;
}
