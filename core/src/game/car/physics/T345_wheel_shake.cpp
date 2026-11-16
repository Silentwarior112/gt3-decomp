/* T345 - a per-wheel damped oscillation in -1..1 (DynWheel.shake /
 * shakeVel, ? the wheel's shake or judder): a spring of stiffness 100 and
 * damping 5, driven by DynCarState.accelX x 5 and by random kicks
 * (vu0_rand). */
#include "game/car/physics/T345_wheel_shake.h"
#include "game/car/physics/drivetrain_internal.h"

/* The constants, small data of this object in the original (gp-relative
 * loads, 0x00352994..0x003529AF: retail loads even 5.0 / 20.0 / 100.0 from
 * memory, so they were variables, not literals). */
f32 g_00352994 = 5.0f;                      /* 0x00352994 accelX x this */
f32 g_00352998 = 5.0f;                      /* 0x00352998 the velX kick's gain */
f32 g_0035299C = 0.02f;                     /* 0x0035299C its probability factor */
f32 g_003529A0 = 20.0f;                     /* 0x003529A0 the spin kick's gain */
f32 g_003529A4 = 0.04f;                     /* 0x003529A4 its probability factor */
f32 g_003529A8 = 100.0f;                    /* 0x003529A8 the spring's stiffness */
f32 g_003529AC = 5.0f;                      /* 0x003529AC its damping */

/* Called by DynamicsConductor::update (0x001F39D8).  Each wheel: a kick of
 * (1 - x) x 5 x velX with the probability ~0.02 / (1 -+ x) (by velX's sign);
 * on a surface whose tyre index is non-zero, rolling forward on the ground,
 * a kick of (1 - x) x 20 x spin with the probability ~0.04 / (1 - x).  The
 * position is held to -1..1, its speed zeroed there. */
// FUNC 0x001E6A38 car_wheel_shake
void car_wheel_shake(RaceCarState *car)
{
    CarRunState *st = &car->st;
    int i;

    for (i = 0; i < 4; i++) {
        f32 force = car->st.accelX * g_00352994;
        CarWheelState *w = &st->wheel[i];
        f32 x = w->shake;
        f32 v = w->shakeVel;
        f32 room;
        f32 dt;

        if (0.0f <= w->velX)
            room = 1.0f - x;
        else
            room = x + 1.0f;
        if (vu0_rand() * room < g_0035299C)
            force = force + (1.0f - x) * g_00352998 * w->velX;
        {
            int t = car->params.tyreIndex[i >> 1][w->surface];   /* (an int: retail's signed blez) */

            if (t > 0 && 0.0f < w->velX && 0.0f < w->load) {
                room = 1.0f - x;
                if (vu0_rand() * room < g_003529A4)
                    force = force + room * g_003529A0 * w->spin;
            }
        }
        dt = car->dt;
        force = force - x * g_003529A8;
        force = force - v * g_003529AC;
        v = v + force * dt;
        x = x + v * dt;
        if (1.0f < x) {
            x = 1.0f;
            v = 0.0f;
        } else if (x < -1.0f) {
            v = 0.0f;
            x = -1.0f;
        }
        w->shake = x;
        w->shakeVel = v;
    }
}
