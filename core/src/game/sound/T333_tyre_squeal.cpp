/* T333 - the tyre squeal and smoke curves: 0x001DE5E0..0x001DF148.
 *
 * tyre_sound_init (from DynamicsConductor::virtual_20) builds, for each of
 * the 4 surface classes, three curve sets from the table g_002D35B8: set 0
 * gives the squeal volume, set 1 its pitch, set 2 the smoke.  Each set has
 * two curves: curve 1 over the slip angle or the lateral slip speed, curve 2
 * over the wheel's slip ratio or the longitudinal slip speed; the result is
 * sqrt(c1^2 + c2^2), plus a term in the wheel's speedFactor above a
 * threshold.
 *
 * tyre_sound_update (from car_step, T302) evaluates them per wheel; on the
 * kerbs (?) (surfaces 1 and 13) the volume and pitch follow a fixed formula
 * instead.  The smoke of the wheels on the loose classes is summed into the
 * car's dirt (CarCondition::addDirt, x 8 when the course kind is 0).
 *
 * Data (retail): .rodata 0x0033C410..0x0033C469: <new>'s "bad_alloc"
 * (unreferenced: DATA_START), the switch's jump table 0x0033C420, then
 * UnitArena<64>'s type name (T153 with GAME_T153_EMITS_TYPEINFO; no race
 * display names: the original did not see RaceSolitaire / the displays);
 * .lit4 0x0035024C; .bss 0x0049C990..0x0049CFD4, the curve sets and the
 * fixed squeal (file statics: retail has them in .bss). */
// DATA_START 0x0033C410
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (seen: UnitArena<64>) */
#include "game/car/physics/car_engine_internal.h"
#include "game/sound/T333_tyre_squeal.h"

/* .bss, per surface class: the squeal volume, its pitch and the smoke.
 * Three arrays (retail builds each address with its own lui / addiu; one
 * [3][4] array lets CSE derive the others from the first). */
static TyreCurveSet g_0049C990[4];          /* 0x0049C990 the squeal volume */
static TyreCurveSet g_0049CBA0[4];          /* 0x0049CBA0 the squeal pitch */
static TyreCurveSet g_0049CDB0[4];          /* 0x0049CDB0 the smoke */
/* The squeal on surfaces 1 / 13: volume, speed lo, pitch lo, speed hi,
 * pitch hi (from CarSoundSource dirt*). */
static f32 g_0049CFC0[5];                   /* 0x0049CFC0 */

/* Curve 1 of a set: n1 points (x1, y1), scaled.  curveSet's mode is 0 or 2
 * (not read there). */
// FUNC 0x001DE5E0 tyre_curve1_build
void tyre_curve1_build(TyreCurveSet *s, const TyreSoundRow *r, int mode, f32 xScale, f32 yScale)
{
    Curve *c = &s->c1;
    f32 *xs = s->x1;
    f32 *ys = s->y1;
    const u8 *y = r->y1;
    const u8 *x = r->x1;
    int n = r->n1;
    for (int i = 0; i < n; i++) {
        xs[i] = (f32)x[i] * xScale;
        ys[i] = (f32)y[i] * yScale;
    }
    curveSet(c, n, xs, ys, mode == 0 ? 0 : 2, 0);
}

/* Curve 2 of a set: the n2neg negative points (stored outwards, so read
 * backwards and negated), then the n2pos positive ones, each side with its
 * own y scale. */
// FUNC 0x001DE680 tyre_curve2_build
void tyre_curve2_build(TyreCurveSet *s, const TyreSoundRow *r, int mode, f32 xScale, f32 yNegScale, f32 yPosScale)
{
    Curve *c = &s->c2;
    f32 *xs = s->x2;
    f32 *ys = s->y2;
    const u8 *yPos = r->y2pos;
    const u8 *xNeg = r->x2neg;
    const u8 *xPos = r->x2pos;
    const u8 *yNeg = r->y2neg;
    int nNeg = r->n2neg;
    int nPos = r->n2pos;
    int i;

    for (i = 0; i < nNeg; i++) {
        xs[i] = -(f32)xNeg[nNeg - 1 - i] * xScale;
        ys[i] = (f32)yNeg[nNeg - 1 - i] * yNegScale;
    }
    for (i = 0; i < nPos; i++) {
        xs[nNeg + i] = (f32)xPos[i] * xScale;
        ys[nNeg + i] = (f32)yPos[i] * yPosScale;
    }
    curveSet(c, nNeg + nPos, xs, ys, mode == 0 ? 0 : 2, 0);
}

/* base = row base / 100; with t = the row's threshold km/h in m/s:
 * threshold = t / 50, gain = (row gain / 100 x 50) / (27.78 - t)
 * (27.78 m/s = 100 km/h). */
// FUNC 0x001DE7C0 tyre_curve_extra
void tyre_curve_extra(TyreCurveSet *s, const TyreSoundRow *r)
{
    s->base = (f32)r->base * 0.01f;
    f32 speed = (f32)r->threshold * 0.2777778f;        /* km/h -> m/s */
    s->threshold = speed / 50.000004f;
    s->gain = ((f32)r->gain * 0.01f * 50.000004f) / (27.777779f - speed);
}

/* Callers: DynamicsConductor::virtual_20.  The three sets of each class
 * are written out (each with its own literals in the binary).  The x scales:
 * curve 1's input in 1/9 m/s (lateral slip) or 0.2 degree (slip angle),
 * curve 2's in 1/18 m/s (longitudinal slip) or 1/255 (the slip ratio). */
// FUNC 0x001DE830 tyre_sound_init
void tyre_sound_init(void)
{
    TyreSoundTable *table = &g_002D35B8;
    const TyreSoundRow *row;
    TyreCurveSet *set;

    TyreSoundTable_validate(table);
    for (int cls = 0; cls < 4; cls++) {
        set = &g_0049C990[cls];
        memset(set, 0, sizeof(TyreCurveSet));
        row = &table->rows[TYRE_SET_VOLUME][cls];
        tyre_curve1_build(set, row, 0, row->input1 == 0 ? 0.0034906585f : 0.11111112f,
                          (f32)row->y1Scale / 2550.0f);
        tyre_curve2_build(set, row, 0, row->input2 == 0 ? 0.003921569f : 0.05555556f,
                          (f32)row->y2negScale / 2550.0f, (f32)row->y2posScale / 2550.0f);
        tyre_curve_extra(set, row);

        set = &g_0049CBA0[cls];
        memset(set, 0, sizeof(TyreCurveSet));
        row = &table->rows[TYRE_SET_PITCH][cls];
        tyre_curve1_build(set, row, 0, row->input1 == 0 ? 0.0034906585f : 0.11111112f,
                          (f32)row->y1Scale / 2550.0f);
        tyre_curve2_build(set, row, 0, row->input2 == 0 ? 0.003921569f : 0.05555556f,
                          (f32)row->y2negScale / 2550.0f, (f32)row->y2posScale / 2550.0f);
        tyre_curve_extra(set, row);

        set = &g_0049CDB0[cls];
        memset(set, 0, sizeof(TyreCurveSet));
        row = &table->rows[TYRE_SET_SMOKE][cls];
        tyre_curve1_build(set, row, 0, row->input1 == 0 ? 0.0034906585f : 0.11111112f,
                          (f32)row->y1Scale / 2550.0f);
        tyre_curve2_build(set, row, 0, row->input2 == 0 ? 0.003921569f : 0.05555556f,
                          (f32)row->y2negScale / 2550.0f, (f32)row->y2posScale / 2550.0f);
        tyre_curve_extra(set, row);
    }
    car_sound_params_init(table);

    /* the fixed squeal of surfaces 1 / 13: volume, speed lo, pitch lo,
     * speed hi, pitch hi (the speeds x 10 km/h in m/s) */
    const CarSoundSource *src = &table->src;
    g_0049CFC0[4] = (f32)src->dirtPitchHi * 0.1f;
    g_0049CFC0[3] = (f32)src->dirtSpeedHi * 2.777778f;
    g_0049CFC0[0] = (f32)src->dirtVolume * 0.1f;
    g_0049CFC0[1] = (f32)src->dirtSpeedLo * 2.777778f;
    g_0049CFC0[2] = (f32)src->dirtPitchLo * 0.1f;
}

/* Per wheel: contactTimer is 6 after a frame with load and counts down in
 * the air; at 0 the wheel is silent (smoke 0, volume 0, pitch 1).
 * Surface classes (jump table 0x0033C420): 2..5 -> 1, 6 and 12..14 -> 2,
 * else 0.  The inputs: the slip angle |tyreForce.slipAngle|, the lateral and
 * longitudinal slip speeds (the wheel's velocity turned by its steering; the
 * tread speed off the longitudinal one) and the slip ratio. */
// FUNC 0x001DEBB0 tyre_sound_update
void tyre_sound_update(RaceCarState *car)
{
    DynCarState *st = &car->st;
    CarCondition *cond = &st->condition;
    f32 dirt = 0.0f;

    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];

        if (w->load != 0.0f) {
            w->contactTimer = 6;
        } else {
            if (w->contactTimer == 0)
                goto silent;
            w->contactTimer = w->contactTimer - 1;
        }
        if (w->contactTimer == 0) {
        silent:
            w->squealVolume = 0.0f;
            w->squealPitch = 1.0f;
            w->smoke = 0.0f;
            continue;
        }

        int cls = TYRE_CLASS_ROAD;
        switch (w->surface) {
        case 2: case 3: case 4: case 5:
            cls = TYRE_CLASS_LOOSE;
            break;
        case 6: case 12: case 13: case 14:
            cls = TYRE_CLASS_OTHER;
            break;
        }

        f32 slipAngle = st->tyreForce[i].slipAngle;
        if (slipAngle < 0.0f)
            slipAngle = -slipAngle;

        f32 steer = w->steer;
        f32 latSlip = cosf(steer) * w->velY + sinf(steer) * w->velX;
        if (latSlip < 0.0f)
            latSlip = -latSlip;

        f32 steer2 = w->steer;
        f32 slipRatio = w->slip;
        f32 lonSpeed = cosf(steer2) * w->velX - sinf(steer2) * w->velY;
        f32 tread = w->spin;
        /* quirk: the sign is tested before the tread speed is taken off */
        f32 lonSlip = lonSpeed - tread;
        if (lonSpeed < 0.0f)
            lonSlip = -lonSlip;

        /* c1: curve 1's input, its value, then the speed over the threshold;
         * c2: curve 2's input and value (retail keeps each in one register:
         * matching, u078) */
        f32 c1, c2, v;

        if (w->surface != SURFACE_KERB && w->surface != SURFACE_KERB_REMAPPED) {
            /* the volume */
            c1 = g_002D35B8.rows[TYRE_SET_VOLUME][cls].input1 == 0 ? slipAngle : latSlip;
            c1 = curveLookupLinear(&g_0049C990[cls].c1, c1);
            c2 = g_002D35B8.rows[TYRE_SET_VOLUME][cls].input2 == 0 ? slipRatio : lonSlip;
            c2 = curveLookupLinear(&g_0049C990[cls].c2, c2);
            v = boot_sqrtf_v(c2 * c2 + c1 * c1);
            c1 = w->speedFactor - g_0049C990[cls].threshold;
            if (0.0f < c1)
                v += c1 * g_0049C990[cls].gain;
            if (v != 0.0f)
                v += g_0049C990[cls].base;
            w->squealVolume = v;

            /* the pitch (base always added) */
            c1 = g_002D35B8.rows[TYRE_SET_PITCH][cls].input1 == 0 ? slipAngle : latSlip;
            c1 = curveLookupLinear(&g_0049CBA0[cls].c1, c1);
            c2 = g_002D35B8.rows[TYRE_SET_PITCH][cls].input2 == 0 ? slipRatio : lonSlip;
            c2 = curveLookupLinear(&g_0049CBA0[cls].c2, c2);
            v = boot_sqrtf_v(c2 * c2 + c1 * c1);
            c1 = w->speedFactor - g_0049CBA0[cls].threshold;
            f32 pitch = g_0049CBA0[cls].base + v;
            if (0.0f < c1)
                pitch += c1 * g_0049CBA0[cls].gain;
            w->squealPitch = pitch;
        } else {
            /* the kerb (?): volume by the load (from 1/4 of 2450) and the
             * tread speed (from 5 km/h), pitch over the tread speed */
            f32 loadK = w->load / 2450.0f - 0.25f;
            if (loadK < 0.0f)
                loadK = 0.0f;
            else if (1.0f < loadK)
                loadK = 1.0f;
            f32 speed = tread;
            if (speed < 0.0f)
                speed = -tread;
            f32 speedK = (speed - 1.388889f) * 0.144f;
            if (speedK < 0.0f)
                speedK = 0.0f;
            else if (1.0f < speedK)
                speedK = 1.0f;
            w->squealVolume = speedK * loadK * g_0049CFC0[0];
            f32 pitch;
            if (speed <= g_0049CFC0[1])
                pitch = g_0049CFC0[2];
            else if (g_0049CFC0[3] <= speed)
                pitch = g_0049CFC0[4];
            else
                pitch = g_0049CFC0[2] + ((speed - g_0049CFC0[1]) * (g_0049CFC0[4] - g_0049CFC0[2]))
                                        / (g_0049CFC0[3] - g_0049CFC0[1]);
            w->squealPitch = pitch;
        }

        /* the smoke, at most 1 */
        c1 = g_002D35B8.rows[TYRE_SET_SMOKE][cls].input1 == 0 ? slipAngle : latSlip;
        c1 = curveLookupLinear(&g_0049CDB0[cls].c1, c1);
        c2 = g_002D35B8.rows[TYRE_SET_SMOKE][cls].input2 == 0 ? slipRatio : lonSlip;
        c2 = curveLookupLinear(&g_0049CDB0[cls].c2, c2);
        v = boot_sqrtf_v(c2 * c2 + c1 * c1);
        c1 = w->speedFactor - g_0049CDB0[cls].threshold;
        if (0.0f < c1)
            v += c1 * g_0049CDB0[cls].gain;
        if (v != 0.0f)
            v += g_0049CDB0[cls].base;
        if (1.0f < v)
            v = 1.0f;
        w->smoke = v;
        if (cls != TYRE_CLASS_ROAD)
            dirt += v;
    }
    if (car->dc->getCourseKind() == 0)
        dirt *= 8.0f;
    cond->addDirt(dirt * 0.001f * car->dt);
}
