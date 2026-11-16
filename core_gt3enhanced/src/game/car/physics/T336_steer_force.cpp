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
#include "game/enhanced.h"                  /* ENH_PHYSICS_FORCE_FEEDBACK */

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

#if ENH_PHYSICS_FORCE_FEEDBACK
/* ---- the enhanced engine: Lupo 2's force feedback (physics.md 20) -------
 *
 * Two parameter sets, chosen per frame by the car's controller: [1] for a
 * Driving Force Pro (USB wheel type 4, USB_WHEEL_DFP), [0] for every other
 * wheel.  The bytes are Lupo 2's (L2 0x002DED94, 0x20 bytes a set; its
 * bytes +0x0E / +0x0F, the DFP's input prediction, are T342's
 * g_dfpSteerPredict).  Set 0 is GT3's tuning but for the gain (120 x 0.02 =
 * 2.4, GT3 2.0) and no slip term limit (both have a 0 slip gain); set 1 is
 * softer (gain 0.8, the square-root curve - responseInit mode 2, n 100 -
 * no filter, no steering rate term, the ground-normal term a quarter of
 * set 0's).  The per-wheel factor (the wheel* bytes) is 0 in both sets, so
 * it is off as in Lupo 2; it is kept for a tuning that sets it. */
typedef struct SteerForceBytes {
    u8 gain;                                /* x 0.02: the front lateral forces / the front sprung weight */
    u8 curveMode;                           /* that through this response curve (responseInit) */
    u8 curveN;
    u8 lateralGain;                         /* x 0.1: the front tyres' ground-normal term */
    u8 filter;                              /* STEER_FILTER_* */
    u8 rateGain;                            /* x 0.01 / 4096: the steering rate */
    u8 steerAngleGain;                      /* x 0.01 / steerAngleLimit: the steering angle */
    u8 steerAngleLimit;                     /* 0.1 deg */
    u8 slipGain;                            /* x 0.01 / slipLimit: the front slip angles */
    u8 slipLimit;                           /* 0.1 deg */
    u8 wheelBase;                           /* the per-wheel factor on each front lateral force: */
    u8 wheelSlip;                           /*   (base + slip (1 - |slip angle| / slipRef) + long x the */
    u8 wheelSlipRef;                        /*   longitudinal force / half the front sprung weight) / (base */
    u8 wheelLong;                           /*   + slip), within -1..2; on when base + slip and slipRef > 0 */
} SteerForceBytes;

static SteerForceBytes g_steerForceBytes[2] = {
    { 120, 0, 100, 200, 1, 10, 0, 50, 0, 0, 0, 0, 0, 0 },     /* the wheels (GT Force and the others) */
    {  40, 2, 100,  50, 0,  0, 0, 50, 0, 0, 0, 0, 0, 0 }      /* the Driving Force Pro */
};

/* The sets as steer_force_update uses them (Lupo 2 L2 0x004A9BA8). */
typedef struct SteerForceSet {
    f32           lateralGain;
    f32           gain;
    ResponseCurve curve;
    f32           rateGain;
    f32           angleGain;
    f32           angleLimit;
    f32           slipGain;
    f32           slipLimit;
    u8            filter;
    u8            wheelOn;                  /* the per-wheel factor is on */
    f32           wheelBase;
    f32           wheelSlip;
    f32           wheelSlipInv;             /* 1 / slipRef, rad */
    f32           wheelLong;
} SteerForceSet;

static SteerForceSet g_steerForceSet[2];

/* A response curve as Lupo 2's responseInit sets it (L2 0x001FE968): its
 * square-root mode 2 has c = 1 + (100 - n) / 100 (n 0..100; GT3's T354: c
 * = n / 10, n 10..100), so the DFP's n 100 is y = sqrt(u).  The other
 * modes go to responseInit (T354); responseEval is the same in both. */
static void steer_force_curve(ResponseCurve *r, int mode, int n)
{
    f32 c, c2, a;

    if (mode != 2) {
        responseInit(r, mode, n);
        return;
    }
    r->mode = mode;
    if (n < 0)
        n = 0;
    else if (n > 100)
        n = 100;
    c = (f32)(100 - n) * 0.01f + 1.0f;
    c2 = c * c;
    r->c = c;
    a = -((c2 - 1.0f) * (c2 - 1.0f)) / (c2 * 4.0f);
    r->a = a;
    r->b = -c * boot_sqrtf_v(-a);
}

/* Lupo 2 L2 0x001F6990 (in Lupo 2's degree and gain constants). */
static void steer_force_init_sets(void)
{
    int i;

    for (i = 0; i < 2; i++) {
        SteerForceBytes *b = &g_steerForceBytes[i];
        SteerForceSet *f = &g_steerForceSet[i];
        f32 limit;

        f->gain = (f32)b->gain * 0.02f;
        f->lateralGain = (f32)b->lateralGain * 0.099999994f;
        steer_force_curve(&f->curve, b->curveMode, b->curveN);
        f->rateGain = (f32)b->rateGain * 0.01f * 0.00024414062f;
        limit = (f32)b->steerAngleLimit * 0.0017453289f;
        f->angleLimit = limit;
        if (b->steerAngleGain == 0)
            f->angleGain = 0.0f;
        else
            f->angleGain = ((f32)b->steerAngleGain * 0.01f) / limit;
        limit = (f32)b->slipLimit * 0.0017453289f;
        f->slipLimit = limit;
        if (b->slipGain == 0)
            f->slipGain = 0.0f;
        else
            f->slipGain = ((f32)b->slipGain * 0.01f) / limit;
        f->filter = b->filter;
        f->wheelOn = 0;
        if (b->wheelBase + b->wheelSlip != 0 && b->wheelSlipRef != 0) {
            f32 sum = (f32)b->wheelBase + (f32)b->wheelSlip;

            f->wheelSlipInv = 1.0f / ((f32)b->wheelSlipRef * 0.01745329f);
            f->wheelLong = (f32)b->wheelLong / sum;
            f->wheelBase = (f32)b->wheelBase / sum;
            f->wheelSlip = (f32)b->wheelSlip / sum;
            f->wheelOn = 1;
        }
    }
}
#endif

/* The enhanced engine, ENH_PHYSICS_FORCE_FEEDBACK (physics.md 20; Lupo 2
 * L2 0x001F6990): Lupo 2's two parameter sets instead. */
// FUNC 0x001DFF40 steer_force_init
void steer_force_init(void)
{
#if ENH_PHYSICS_FORCE_FEEDBACK
    steer_force_init_sets();
#else
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
#endif
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

#if ENH_PHYSICS_FORCE_FEEDBACK
/* The enhanced engine (Lupo 2 L2 0x001F6C10): the force of the set the
 * car's controller selects.  While the computer drives (driver 1 / 2, or
 * ctrlMode not CTRL_DRIVER) the wheel is only pulled back to the centre,
 * by its own position (0x80 - the raw input / 32; pad_vibration_send now
 * sends it).  Otherwise as GT3 with the set's gains, curve and filter, the
 * per-wheel factor when the set has it, and no unk_420 factor (always 1).
 * Lupo 2 keeps the filter state and the force byte in the conductor (L2
 * 0x001F6BF0); here they stay in DynCarState / RaceCarState.vib. */
static void steer_force_update_sets(RaceCarState *car)
{
    CarDynamics *params = &car->params;
    DynCarState *st = &car->st;
    SteerForceSet *f;
    f32 force;
    f32 out = 0.0f;
    f32 steer;
    f32 lateral;
    f32 springK;
    f32 speed2;
    int i;

    f = &g_steerForceSet[((car->input->type >> USB_TYPE_SHIFT) & 7) == USB_WHEEL_DFP ? 1 : 0];
    if (st->driver == DRIVER_AUTO || st->driver == DRIVER_COMPUTER || st->ctrlMode != CTRL_DRIVER) {
        int v = 0x80 - car->steerIn / 32;

        if (v < 0)
            v = 0;
        else if (!(v < 0x100))
            v = 0xFF;
        car->vib.steer = (u8)v;
        return;
    }

    force = 0.0f;
    for (i = 0; i < 2; i++) {
        f32 lat = st->wheel[i].latForce;

        if (f->wheelOn != 0) {
            f32 slip = st->tyreForce[i].slipAngle;
            f32 k;

            if (slip < 0.0f)
                slip = -slip;
            k = f->wheelBase + (1.0f - slip * f->wheelSlipInv) * f->wheelSlip
                + ((st->wheel[i].driveForce + st->wheel[i].brakeForce) / (params->sprungWeight[0] * 0.5f)) * f->wheelLong;
            if (2.0f < k)
                k = 2.0f;
            else if (k < -1.0f)
                k = -1.0f;
            lat = lat * k;
        }
        force += lat;
    }
    force = force / params->sprungWeight[0];
    force = force * f->gain;
    if (1.0f < force)
        force = 1.0f;
    else if (force < -1.0f)
        force = -1.0f;
    force = responseEval(&f->curve, force, NULL)
             + (f32)((car->steerIn - car->steerInPrev) * 60) * f->rateGain;
    steer = st->steer;
    if (f->angleLimit < steer)
        steer = f->angleLimit;
    else if (steer < -f->angleLimit)
        steer = -f->angleLimit;
    force += f->angleGain * steer;
    for (i = 0; i < 2; i++)
        force += f->slipGain * steer_clamp(st->tyreForce[i].slipAngle, f->slipLimit);

    switch (f->filter) {
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

    lateral = 0.0f;
    for (i = 0; i < 2; i++) {
        f32 c = cosf(st->wheel[i].steer);
        f32 s = sinf(st->wheel[i].steer);
        f32 axis[3];
        f32 normal[3];
        f32 dot;
        int k;

        for (k = 0; k < 3; k++)
            axis[k] = c * st->rot[1][k] - s * st->rot[0][k];
        normal[0] = car->contact[i].normal[0];
        normal[1] = -car->contact[i].normal[2];
        normal[2] = car->contact[i].normal[1];
        dot = 0.0f;
        for (k = 0; k < 3; k++)
            dot += axis[k] * normal[k];
        lateral += dot * st->wheel[i].load;
    }
    springK = (params->susp[0].spring + 49000.0f) / 196000.0f;
    speed2 = st->vel[0] * st->vel[0] + st->vel[1] * st->vel[1];
    lateral = lateral * springK;
    out = out + (lateral * f->lateralGain) / params->sprungWeight[0];
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
#endif

/* Filters (InputTuning.filter): 0 none; 1 the average with the last value;
 * 2 the smaller in size of this and the last value when they have the same
 * sign side; 3 a peak hold that decays by dt x 8 after 7 frames.  ($f0
 * holds force x 127 at the return; no caller reads it.)  One variable
 * (force) is the aligning term and then the force: retail keeps both in
 * $f12, the argument register of responseEval. */
// FUNC 0x001E0068 steer_force_update
void steer_force_update(RaceCarState *car)
{
#if ENH_PHYSICS_FORCE_FEEDBACK
    steer_force_update_sets(car);
}
#else
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
#endif

/* Sends the vibration block to the pad (RaceInput::sendVibration) unless the
 * driver is 1 or 2 or the computer drives (ctrlMode != 0).  Callers:
 * DynamicsConductor::update.  The enhanced engine, ENH_PHYSICS_FORCE_FEEDBACK
 * (Lupo 2 L2 0x001F7458): always, so a wheel the computer has is held at
 * the centre (steer_force_update). */
// FUNC 0x001E0618 pad_vibration_send
void pad_vibration_send(RaceCarState *car)
{
#if ENH_PHYSICS_FORCE_FEEDBACK
    car->input->sendVibration(&car->vib);
}
#else
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
#endif
