/* T337 - a lagged copy of the car's yaw and pitch: 0x001E0740..0x001E0888.
 * Both take the run state (car +0xF84).
 *
 * Data (retail): .sdata 0x00352958..0x00352968 (four floats), .lit4
 * 0x0035030C; no .rodata (no <new>, no type name). */
#include "game/car/physics/car_engine_internal.h"
#include "game/car/physics/T337_attitude_lag.h"

/* .sdata (values: retail's data).  The yaw's spring and damping are read
 * by car_attitude_lag_update; nothing in the image reads 0x0035295C (100)
 * or 0x00352964 (20: the pitch's spring is the literal 20.0f). */
f32 g_00352958 = 30.0f;                     /* 0x00352958 */
f32 g_0035295C = 100.0f;                    /* 0x0035295C */
f32 g_00352960 = 10.0f;                     /* 0x00352960 */
f32 g_00352964 = 20.0f;                     /* 0x00352964 */

/* Callers: DynCar_initialize (T330). */
// FUNC 0x001E0740 car_attitude_lag_reset
void car_attitude_lag_reset(DynCarState *s)
{
    s->lagYaw = s->yaw;
    s->lagPitch = s->pitch;
    s->lagYawAcc = 0.0f;
    s->lagYawVel = 0.0f;
    s->lagYawDiff = 0.0f;
    s->lagPitchVel = 0.0f;
    s->lagPitchDiff = 0.0f;
}

/* At 1/60 s: the yaw follows with a spring (g_00352958) and damper
 * (g_00352960), the acceleration integrated as the average of this and the
 * last frame's; the pitch with a fixed spring 20 and damper 10.  Both
 * angles wrapped (wrapAngle); the remaining differences to the car's angles
 * in lagYawDiff / lagPitchDiff (read by car_lag_yaw / car_lag_pitch, T302).
 * Called by car_step (T302). */
// FUNC 0x001E0768 car_attitude_lag_update
void car_attitude_lag_update(DynCarState *s)
{
    f32 spring = g_00352958;
    f32 damping = g_00352960;
    f32 pitchSpring = 20.0f;
    f32 pitchDamping = 10.0f;
    f32 diff = angleDiff(s->yaw, s->lagYaw);
    f32 acc = diff * spring - s->lagYawVel * damping;
    f32 accSum = s->lagYawAcc + acc;
    s->lagYawAcc = acc;
    s->lagYawVel += accSum * 0.5f * 0.016666668f;
    s->lagYaw += s->lagYawVel * 0.016666668f;
    s->lagYaw = wrapAngle(s->lagYaw);
    s->lagYawDiff = angleDiff(s->yaw, s->lagYaw);

    diff = angleDiff(s->pitch, s->lagPitch);
    s->lagPitchVel += (diff * pitchSpring - s->lagPitchVel * pitchDamping) * 0.016666668f;
    s->lagPitch += s->lagPitchVel * 0.016666668f;
    s->lagPitch = wrapAngle(s->lagPitch);
    s->lagPitchDiff = angleDiff(s->pitch, s->lagPitch);
}
