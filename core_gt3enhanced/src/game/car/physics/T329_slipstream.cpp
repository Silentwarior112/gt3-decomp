/* T329 - the slipstream: a blend factor per car that rises while
 * DynCarState.slipTarget is non-zero and falls otherwise, scaling the drag
 * and lift forces of the frame (factors g_00352924 / g_00352928 at full
 * blend). */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T329_slipstream.h"

/* T329's .sdata: the factors before slipstream_init first runs (it sets
 * them from DynParams: 1.2, 0.5, 1.0, 2.0, 0.5 with the file's bytes). */
f32 g_00352920 = 0.8f;                      /* 0x00352920 airDensity / 10 */
f32 g_00352924 = 0.875f;                    /* 0x00352924 the drag factor in full slipstream */
f32 g_00352928 = 0.875f;                    /* 0x00352928 the lift factor */
f32 g_0035292C = 1.0f;                      /* 0x0035292C the blend's rise rate */
f32 g_00352930 = 0.25f;                     /* 0x00352930 the blend's fall rate */

/* The factors from DynParams airDensity .. slipFall (the defaults when a byte
 * is 0).  Not exact (87.5%): retail reloads airDensity for the second
 * division, which our GCSE merges with the first load
 * (ref/matching/u076/notes.md). */
// FUNC 0x001DB020 slipstream_init
void slipstream_init(void)
{
    f32 fbase = (f32)g_002D28B0.airDensity;

    g_00352920 = fbase * 0.1f;
    if (g_002D28B0.airDensity != 0) {
        g_00352928 = (f32)g_002D28B0.slipLift / (f32)g_002D28B0.airDensity;
        g_00352924 = (f32)g_002D28B0.slipDrag / fbase;
    } else {
        g_00352924 = 0.0f;
        g_00352928 = 0.0f;
    }
    if (g_002D28B0.slipRise != 0)
        g_0035292C = 10.0f / (f32)g_002D28B0.slipRise;
    else
        g_0035292C = 1.0f;
    if (g_002D28B0.slipFall != 0)
        g_00352930 = 10.0f / (f32)g_002D28B0.slipFall;
    else
        g_00352930 = 0.25f;
}

/* drag = -sign(v) x dragCoef x v^2 x lerp(1, g_00352924, blend), lift[i] =
 * liftCoef[i] x v^2 x lerp(1, g_00352928, blend), v = DynCarState.speedX
 * (v >= 0 counts as positive).  Not exact (90.9%): retail re-reads speedX
 * for the sign test, our GCSE keeps it in a register. */
// FUNC 0x001DB118 car_slipstream_update
void car_slipstream_update(RaceCarState *car)
{
    DynCarState *st = &car->st;
    CarDynamics *params = &car->params;
    f32 target = st->slipTarget;
    f32 speed = st->speedX;
    f32 dt = car->dt;
    f32 speed2 = speed * speed;
    f32 open, drag, liftScale;
    int i;

    if (target != 0.0f) {
        st->slipBlend += target * g_0035292C * dt;
        if (1.0f < st->slipBlend)
            st->slipBlend = 1.0f;
    } else {
        st->slipBlend -= g_00352930 * dt;
        if (st->slipBlend < 0.0f)
            st->slipBlend = 0.0f;
    }
    open = 1.0f - st->slipBlend;                /* the share of open air */
    drag = params->dragCoef * ((g_00352924 + (1.0f - g_00352924) * open) * speed2);
    if (0.0f <= st->speedX)
        drag = -drag;
    st->drag = drag;
    liftScale = (g_00352928 + (1.0f - g_00352928) * open) * speed2;
    for (i = 0; i < 2; i++)
        st->lift[i] = params->liftCoef[i] * liftScale;
}
