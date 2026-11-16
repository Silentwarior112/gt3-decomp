/* T292 (0x001E1B28..0x001E2670): the drivetrain block of a CarSpec, and the
 * gear chart of the setting screen. */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T292_gear_calc.h"

/* The differential letters (index 0..7), not terminated: .sdata (-G8). */
const char g_00352968[8] = { 'f', 'r', 'n', 'm', 'h', 'v', 'y', 't' };  /* 0x00352968 */

/* The differential code of an axle: its letter in "frnmhvyt" -> 0..7; any
 * other value (already an index) is returned unchanged. */
// FUNC 0x001E1B28 diff_code_to_index
s8 diff_code_to_index(s8 code)
{
    int i;
    s8 index = code;

    for (i = 0; i < 8; i++) {
        if (g_00352968[i] == code) {
            index = i;
            break;
        }
    }
    return index;
}

/* No callers. */
// FUNC 0x001E1B80 diff_codes_to_index
void diff_codes_to_index(CarSpec *spec)
{
    unsigned i;

    for (i = 0; i < 2; i++)
        spec->diffCode[i] = diff_code_to_index((s8)spec->diffCode[i]);
}

/* The inverse: an index 0..7 -> its letter (no callers). */
// FUNC 0x001E1BC8 diff_codes_to_letters
void diff_codes_to_letters(CarSpec *spec)
{
    unsigned i;

    for (i = 0; i < 2; i++)
        spec->diffCode[i] = (spec->diffCode[i] < 8) ? (u8)g_00352968[spec->diffCode[i]] : spec->diffCode[i];
}

/* With the "auto" gear setting on, the ratios of the spec are replaced by
 * the ones of its level (0xFF: 255 km/h, else level x 10 km/h).  The second
 * argument is not read (gear_calc passes its own 4th argument). */
// FUNC 0x001E1C10 gear_auto_apply
int gear_auto_apply(CarSpec *spec, int unused)
{
    int level;
    int i;

    if (spec->autoGear == 0)
        return 0;
    if (spec->autoGearLevel == 0)
        return 0;
    if (spec->autoGearLevel == 0xFF)
        level = 0xFF;
    else
        level = spec->autoGearLevel * 10;
    for (i = 0; i < 8; i++)
        spec->gearRatio[i] = -1;
    gear_auto_ratios(spec, level, spec->gearRatio, 0);
    return 1;
}

/* The drivetrain block of a spec (e: the engine of the same spec, for its
 * idle rpm). */
// FUNC 0x001E1C98 gear_calc
void gear_calc(CarSpec *spec, GearCalc *g, EngineCurve *e, int a3)
{
    f32 axleInertia[2];             /* sp+0x00 the axles' own inertia (unk_0D2 x 0.01) */
    unsigned axle;
    unsigned i;
    int gear;

    drivetrain_layout(spec, &g->driveType, &g->drivenAxle, &g->awdSplit, &g->awdTorque);
    for (axle = 0; axle < 2; axle++)
        g->diffType[axle] = diff_code_to_index((s8)spec->diffCode[axle]);   /* lb */

    /* a driven axle has at least differential 2 (open), an undriven one 0 */
    switch (g->driveType) {
    case DRIVE_REAR:
        if (g->diffType[0] != 0)
            g->diffType[0] = 0;
        if (g->diffType[1] == 0)
            g->diffType[1] = 2;
        break;
    case DRIVE_FRONT:
        if (g->diffType[0] == 0)
            g->diffType[0] = 2;
        if (g->diffType[1] != 0)
            g->diffType[1] = 0;
        break;
    case 2: case 3: case 4: case 5: case 6: case 7:     /* the 4WD kinds */
        if (g->diffType[0] == 0)
            g->diffType[0] = 2;
        if (g->diffType[1] == 0)
            g->diffType[1] = 2;
        break;
    }

    /* the LSD parameters of each differential type */
    for (i = 0; i < 2; i++) {
        switch (g->diffType[i]) {
        case 0: case 1: case 2:
            g->diffA[i] = 0.0f;
            g->diffB[i] = 0.0f;
            g->diffC[i] = 0.0f;
            break;
        case 3: case 4:
            g->diffA[i] = (f32)spec->diffInitial[i] * 9.8f;
            g->diffB[i] = (f32)spec->diffAccel[i] * 0.01f;
            g->diffC[i] = (f32)spec->diffDecel[i] * 0.01f;
            break;
        case 5:
            g->diffA[i] = (f32)spec->diffInitial[i] * (f32)spec->diffInitial[i];
            g->diffB[i] = (f32)spec->diffAccel[i] * 0.1f * 9.8f;
            g->diffC[i] = 0.0f;
            break;
        case 6:
            g->diffA[i] = (f32)spec->diffInitial[i] * 9.8f;
            g->diffB[i] = (f32)spec->diffAccel[i] * 0.01f;
            g->diffC[i] = (f32)spec->diffDecel[i] * 0.01f;
            break;
        case 7:
            g->diffA[i] = (f32)spec->diffInitial[i] * 0.01f;
            g->diffB[i] = (f32)spec->diffAccel[i] * 0.01f;
            g->diffC[i] = (f32)spec->diffDecel[i] * 0.01f;
            break;
        }
    }

    gear_auto_apply(spec, a3);

    g->gearCount = spec->gearCount;
    for (gear = 0; gear <= g->gearCount; gear++) {
        /* empty in the binary (a count-down loop of nops) */
    }
    for (gear = 0; gear <= g->gearCount; gear++)
        g->ratio[gear] = (f32)spec->gearRatio[gear] * 0.001f * ((f32)spec->finalGear * 0.001f);
    g->ratio[0] = -g->ratio[0];
    for (gear = g->gearCount + 1; gear < 8; gear++) {
        /* empty in the binary */
    }

    tyre_dimensions(spec, 0, &g->tyreRadius[0], &g->tyreWidth[0], &g->tyreSidewall[0]);
    tyre_dimensions(spec, 1, &g->tyreRadius[1], &g->tyreWidth[1], &g->tyreSidewall[1]);
    if (g->tyreRadius[0] != 0.0f)
        g->tyreRadiusInv[0] = 1.0f / g->tyreRadius[0];
    if (g->tyreRadius[1] != 0.0f)
        g->tyreRadiusInv[1] = 1.0f / g->tyreRadius[1];

    {
        int j;
        int k;
        int m;

        for (j = 0; j <= 1; j++)
            axleInertia[j] = (f32)spec->unk_0D2[j] * 0.01f;
        for (k = 0; k <= 1; k++) {
            f32 inertia = axleInertia[k];
            if (inertia != 0.0f) {
                f32 radiusInv = g->tyreRadiusInv[k];
                g->invInertia[k] = g->tyreRadius[k] / inertia;
                g->axleMass[k] = inertia * radiusInv * radiusInv;
            }
        }
        g->inertia = (f32)spec->flywheel * 0.001f;
        if (spec->flywheel != 0)
            g->inertiaInv = 1.0f / g->inertia;
        /* (98.7%: retail keeps the conversion of unk_0CE[m] apart from the
         * first product, $f2 / $f0, and the hoisted addresses' spill slots
         * differ; about 20 forms of this loop tried, see
         * ref/matching/u079/notes.md) */
        for (m = 0; m <= 1; m++)
            g->axleInertia[m] = (f32)spec->unk_0CE[m] * 0.01f * ((f32)spec->finalGear * 0.001f)
                              + (axleInertia[m] + axleInertia[m]);
    }
    g->invAxleInertia = 1.0f / (g->axleInertia[0] + g->axleInertia[1]);
    g->idleRpm = (u16)(spec->idleRpm * 10);
    g->revLimit = (u16)(spec->revLimit * 100);
    g->clutchTorque = (f32)spec->unk_0CD * 9.8f;

    for (gear = 0; gear <= g->gearCount; gear++)
        g->ratioPerRadius[gear] = g->ratio[gear] / g->tyreRadius[g->drivenAxle];
    if (g->idleRpm - 500 < e->idle)
        g->idleRpm = (u16)(e->idle + 500);

    /* The shift points: the rpm in gear + 1 at the speed of the rev limit
     * in `gear', less the rpm of 10 km/h in gear + 1; at least the idle rpm
     * + 500.  0.10471976 = 2 pi / 60 (rpm -> rad/s). */
    for (gear = 1; gear < g->gearCount; gear++) {
        f32 rpmNext = (f32)g->revLimit * 0.10471976f * g->ratio[gear + 1] / g->ratio[gear]
                    * 60.0f / 6.2831855f;
        /* 10 km/h in m/s: 0x4031C71D (10.0f / 3.6f; the first pass had
         * 2.7777777f, one ulp lower) */
        f32 margin = g->ratioPerRadius[gear + 1] * 2.7777779f * 60.0f / 6.2831855f;
        int lowest = g->idleRpm + 500;
        int rpm = (int)rpmNext - (int)margin;
        if (rpm < lowest)
            rpm = lowest;
        g->shiftRpm[gear - 1] = (u16)rpm;
    }
}

/* The gear chart lines: gear i from (speedLow, rpmLow) to (speedHigh,
 * rpmHigh), speed = rpm in rad/s / ratioPerRadius; the reverse gear (i = 0)
 * goes negative.  1st and the reverse start at 1000 rpm, gear i at the shift
 * rpm of i - 1 -> i; the top gear and the reverse end at the engine's
 * redline, the others at the rev limit. */
// FUNC 0x001E2450 gear_chart_fill
void gear_chart_fill(GearCalc *g, EngineCurve *e, GearChartData *out)
{
    int n = g->gearCount;
    int i;

    out->count = n;
    for (i = 0; i <= n; i++) {
        u16 rpmLow;
        u16 rpmHigh;
        f32 w;

        rpmLow = 1000;
        if ((unsigned)i >= 2)
            rpmLow = g->shiftRpm[i - 2];
        out->rpmLow[i] = rpmLow;
        w = (f32)rpmLow * 0.10471976f;
        if (i == 0)
            w = -w;
        out->speedLow[i] = w / g->ratioPerRadius[i];

        if (i == 0 || i == n)
            rpmHigh = e->redline;
        else
            rpmHigh = g->revLimit;
        out->rpmHigh[i] = rpmHigh;
        w = (f32)rpmHigh * 0.10471976f;
        if (i == 0)
            w = -w;
        out->speedHigh[i] = w / g->ratioPerRadius[i];
    }
}

/* GearChart (u069, T237): the chart of a spec.  The callers pass a third
 * argument 0 that is not read; the condition is built and not used (? the
 * original takes a condition, NULL for a fresh one, as gear_chart_with_sheet does, and
 * has no use for it: the select is optimised away, the constructor call is
 * not). */
// FUNC 0x001E2538 gear_chart_of_spec
void gear_chart_of_spec(CarSpec *spec, GearChartData *out, CarCondition *cond)
{
    GearCalc     gc;                /* sp+0x000 */
    EngineCurve  engine;            /* sp+0x0C0 */
    CarCondition fresh;             /* sp+0x180 */

    if (cond == NULL)
        cond = &fresh;              /* (no code: cond is not used) */
    engine_setup(&engine, spec, 0, 0);
    gear_calc(spec, &gc, &engine, 0);
    gear_chart_fill(&gc, &engine, out);
}

/* The same with the engine's spec sheet (with the condition, a fresh one
 * when cond is NULL); returns the maximum power as an int.  No callers. */
// FUNC 0x001E25B0 gear_chart_with_sheet
int gear_chart_with_sheet(CarSpec *spec, GearChartData *out, EngineSpecSheet *sheet, CarCondition *cond)
{
    GearCalc     gc;                /* sp+0x000 */
    EngineCurve  engine;            /* sp+0x0C0 */
    CarCondition fresh;             /* sp+0x180 */

    if (cond == NULL)
        cond = &fresh;
    engine_setup(&engine, spec, 0, 0);
    gear_calc(spec, &gc, &engine, 0);
    engine_spec_sheet(&engine, sheet, cond);
    gear_chart_fill(&gc, &engine, out);
    return (int)sheet->maxPower;
}
