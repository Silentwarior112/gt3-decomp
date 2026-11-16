/* T336 - the steering force (force-feedback wheel): 0x001DFF40..0x001E0668.
 *
 * steer_force_init (from input_tuning_init, T342) sets the gains (.sdata
 * g_00352934..g_0035294C) and the response curve g_0049D050 from the input
 * tuning bytes g_002D4628 +0x34..+0x3E.  steer_force_update (per car, from
 * car_step, T302) sums: the front wheels' lateral forces through the curve,
 * the steering rate, the clamped steering angle term (InputTuning's
 * steerAngleGain / steerAngleLimit: DynCarState +0x41C is the steering
 * angle) and the
 * clamped front slip angles; a filter (0..3) on that; plus the lateral
 * term of the front tyres; below 2 km/h (0.5556 m/s) the steering angle
 * alone (clamped to 5 degrees) / 10 degrees.  Clamped to +-1, x 127, stored
 * as 0x80 - force in vib.steer.
 *
 * Data (retail): .rodata 0x0033C4C0..0x0033C4C9, <new>'s "bad_alloc" only
 * (unreferenced: DATA_START; no UnitArena<64> name: T153 read without it);
 * .sdata 0x00352934..0x00352950, the gains (values: retail's data);
 * .lit4 0x003502E0; .bss 0x0049D050, the response curve (a file static). */
// DATA_START 0x0033C4C0
#include <new>                              /* (unused: "bad_alloc") */
#include "game/car/physics/car_engine_internal.h"
#include "game/car/physics/T336_steer_force.h"

/* .sdata: the gains steer_force_init sets from g_002D4628 (car_engine_internal.h). */
f32 g_00352934 = 0.0f;                      /* 0x00352934 */
f32 g_00352938 = 1.0f;                      /* 0x00352938 */
f32 g_0035293C = 0.0f;                      /* 0x0035293C */
f32 g_00352940 = 0.0f;                      /* 0x00352940 */
f32 g_00352944 = 0.0f;                      /* 0x00352944 */
f32 g_00352948 = 0.0f;                      /* 0x00352948 */
f32 g_0035294C = 0.0f;                      /* 0x0035294C */

/* .bss: the steering force's response curve, set from
 * g_002D4628.forceCurveMode / forceCurveN. */
static ResponseCurve g_0049D050;            /* 0x0049D050 */

// FUNC 0x001DFF40 steer_force_init
void steer_force_init(void)
{
    InputTuning *tuning = &g_002D4628;

    g_00352938 = (f32)tuning->forceGain * 0.01f;
    g_00352934 = (f32)tuning->lateralGain * 0.1f;
    responseInit(&g_0049D050, tuning->forceCurveMode, tuning->forceCurveN);
    g_0035293C = (f32)tuning->rateGain * 0.01f * 0.00024414062f;
    f32 limit = (f32)tuning->steerAngleLimit * 0.0017453292f;     /* 0.1 degree in rad */
    g_00352944 = limit;
    if (tuning->steerAngleGain == 0)
        g_00352940 = 0.0f;
    else
        g_00352940 = ((f32)tuning->steerAngleGain * 0.01f) / limit;
    limit = (f32)tuning->slipLimit * 0.0017453292f;
    g_0035294C = limit;
    if (tuning->slipGain == 0)
        g_00352948 = 0.0f;
    else
        g_00352948 = ((f32)tuning->slipGain * 0.01f) / limit;
}

/* x clamped to -limit..limit (an inline: the slip loop keeps a copy of
 * the limit, as retail does). */
static inline f32 steer_clamp(f32 x, f32 limit)
{
    if (limit < x) {
        x = limit;
    } else {
        f32 lo = -limit;
        if (x < lo)
            x = lo;
    }
    return x;
}

/* Filters (InputTuning.filter): 0 none; 1 the average with the last value;
 * 2 the smaller in size of this and the last value when they have the same
 * sign side; 3 a peak hold that decays by dt x 8 after 7 frames.  ($f0
 * holds force x 127 at the return; no caller reads it.)  One variable
 * (force) is the aligning term and then the force: retail keeps both in
 * $f12, the argument register of responseEval. */
// FUNC 0x001E0068 steer_force_update
void steer_force_update(RaceCarState *car)
{
    CarDynamics *params = &car->params;
    DynCarState *st = &car->st;
    f32 force = st->wheel[0].latForce + st->wheel[1].latForce;
    force = force / params->sprungWeight[0];
    if (g_002D4628.useX420 != 0)
        force *= st->unk_420;
    force = force * g_00352938;
    if (1.0f < force)
        force = 1.0f;
    else if (force < -1.0f)
        force = -1.0f;

    f32 out = 0.0f;
    force = responseEval(&g_0049D050, force, NULL)
             + (f32)((car->steerIn - car->steerInPrev) * 60) * g_0035293C;
    f32 steer = st->steer;
    if (g_00352944 < steer) {
        steer = g_00352944;
    } else {
        f32 lo = -g_00352944;
        if (steer < lo)
            steer = lo;
    }
    force += g_00352940 * steer;
    for (int i = 0; i < 2; i++)
        force += g_00352948 * steer_clamp(st->tyreForce[i].slipAngle, g_0035294C);

    switch (g_002D4628.filter) {
    case STEER_FILTER_NONE:
        out = force;
        st->steerForce = out;
        break;
    case STEER_FILTER_AVERAGE: {
        f32 sum = st->steerForce + force;
        st->steerForce = force;
        out = sum * 0.5f;
        break;
    }
    case STEER_FILTER_SMALLER: {
        f32 v = force;
        f32 last = st->steerForce;
        if (0.0f <= force) {
            if (force < last)
                v = last;
        } else {
            if (last < force)
                v = last;
        }
        out = v;
        st->steerForce = force;
        break;
    }
    case STEER_FILTER_PEAK_HOLD: {
        f32 held = st->steerForce;
        int frames;
        if (0.0f <= held) {
            frames = st->steerHold;
            if (frames < 60) {
                st->steerHold = frames + 1;
                frames = frames + 1;
            }
            if (frames >= 7) {
                f32 decayed = held - car->dt * 8.0f;
                st->steerForce = decayed;
                if (decayed < 0.0f) {
                    held = 0.0f;
                    st->steerForce = 0.0f;
                } else {
                    held = decayed;
                }
            }
            if (held < force || force < 0.0f) {
                held = force;
                st->steerForce = force;
                st->steerHold = 0;
            }
            out = held;
        } else {
            frames = st->steerHold;
            if (frames < 60) {
                st->steerHold = frames + 1;
                frames = frames + 1;
            }
            if (frames >= 7) {
                f32 decayed = held + car->dt * 8.0f;
                st->steerForce = decayed;
                if (0.0f < decayed) {
                    held = 0.0f;
                    st->steerForce = 0.0f;
                } else {
                    held = decayed;
                }
            }
            if (force < held || 0.0f < force) {
                st->steerForce = force;
                held = force;
                st->steerHold = 0;
            }
            out = held;
        }
        break;
    }
    }

    /* the front tyres' lateral term: the wheel's lateral axis (cos steer x
     * rot[1] - sin steer x rot[0]) . the ground normal in the car's axes
     * (n.x, -n.z, n.y: the runway's y is up), x the wheel's load */
    f32 lateral = 0.0f;
    for (int i = 0; i < 2; i++) {
        f32 c = cosf(st->wheel[i].steer);
        f32 s = sinf(st->wheel[i].steer);
        f32 axis[3];                        /* sp+0x00 */
        f32 normal[3];                      /* sp+0x10 */
        for (int k = 0; k < 3; k++)
            axis[k] = c * st->rot[1][k] - s * st->rot[0][k];
        normal[2] = car->contact[i].normal[1];
        normal[0] = car->contact[i].normal[0];
        normal[1] = -car->contact[i].normal[2];
        f32 dot = 0.0f;
        for (int k = 0; k < 3; k++)
            dot += axis[k] * normal[k];
        lateral += dot * st->wheel[i].load;
    }
    f32 springK = (params->susp[0].spring + 49000.0f) / 196000.0f;
    f32 speed2 = st->vel[0] * st->vel[0] + st->vel[1] * st->vel[1];
    lateral = lateral * springK;
    out = out + (lateral * g_00352934) / params->sprungWeight[0];
    if (boot_sqrtf_v(speed2) < 0.5555556f) {             /* 2 km/h */
        f32 a = st->steer;
        if (0.08726646f < a)                            /* 5 degrees */
            a = 0.08726646f;
        else if (a < -0.08726646f)
            a = -0.08726646f;
        out = a * 5.729578f;                            /* / 10 degrees */
    }
    if (1.0f < out)
        out = 1.0f;
    else if (out < -1.0f)
        out = -1.0f;
    car->vib.steer = -0x80 - (int)(out * 127.0f);
}

/* Sends the vibration block to the pad (RaceInput::sendVibration) unless the
 * driver is 1 or 2 or the computer drives (ctrlMode != 0).  Callers:
 * DynamicsConductor::update. */
// FUNC 0x001E0618 pad_vibration_send
void pad_vibration_send(RaceCarState *car)
{
    DynCarState *st = &car->st;
    s8 driver = st->driver;
    if (driver == 1)
        return;
    if (driver == 2)
        return;
    if (st->ctrlMode != CTRL_DRIVER)
        return;
    car->input->sendVibration(&car->vib);
}
