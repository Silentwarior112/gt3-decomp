/* T294 (0x001E28A8..0x001E2CC8): the automatic gear ratios and the
 * adjustable range of every gear. */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T294_gear_auto.h"

/* The auto ratio steps of 5, 6, 7 gears (.data: not const), each 200 / the
 * byte: [1] / [0] the step at level 150 / 400, [3] / [2] its growth, [5] /
 * [4] the 1st gear's extra step. */
u8 g_002D43C8[3][6] = {                     /* 0x002D43C8 */
    { 155, 165, 188, 190, 190, 200 },       /* 5 gears */
    { 165, 175, 190, 192, 190, 200 },       /* 6 gears */
    { 170, 180, 192, 194, 190, 200 }        /* 7 gears */
};

/* The ratios (x 1000) of the "auto" gear setting of level `level' (the top
 * speed in km/h, 150..400) into ratios[0..gearCount]: the top gear reaches
 * CarSpec.autoGearRpm at `level' (on the driven tyre), each lower gear is a
 * step longer (the steps of the gear count's row of g_002D43C8,
 * interpolated by the level), the 1st gear one more step; the reverse is (3
 * x 1st + 2nd) / 4.  clearAuto != 0 also clears CarSpec.autoGear. */
// FUNC 0x001E28A8 gear_auto_ratios
void gear_auto_ratios(CarSpec *spec, int level, s16 *ratios, int clearAuto)
{
    u8  driveType;                  /* sp+0x00 */
    u8  drivenAxle;                 /* sp+0x01 */
    f32 awdSplit;                   /* sp+0x04 */
    f32 awdTorque;                  /* sp+0x08 */
    f32 radius;                     /* sp+0x0C */
    f32 width;                      /* sp+0x10 */
    f32 sidewall;                   /* sp+0x14 */
    const u8 *steps;
    int gears;
    int i;
    f32 levelPos;
    f32 speed;
    f32 final;
    f32 ratio;
    f32 step;
    f32 stepGrowth;
    f32 firstStep;
    f32 atTop, atBottom;

    if (clearAuto != 0)
        spec->autoGear = 0;
    drivetrain_layout(spec, &driveType, &drivenAxle, &awdSplit, &awdTorque);
    tyre_dimensions(spec, drivenAxle, &radius, &width, &sidewall);

    gears = spec->gearCount;
    i = (gears > 4) ? gears : 5;                        /* (i: also the loop's counter below) */
    steps = g_002D43C8[i - 5];
    speed = (f32)level * 0.2777778f;                    /* km/h -> m/s */
    levelPos = (f32)(level - 150) / 250.0f;             /* 0 at 150 km/h, 1 at 400 */
    /* implicit: soft-float; a double multiply: fptodp, dpmul (0.001 at 0x0033C630), dptofp */
    final = (f32)spec->finalGear * 0.001;

    atTop = 200.0f / (f32)steps[1];
    atBottom = 200.0f / (f32)steps[0];
    step = atTop + levelPos * (atBottom - atTop);
    /* the top gear's ratio: the engine at autoGearRpm (rad/s) x the radius
     * / the speed (m/s) / the final gear */
    ratio = (f32)(spec->autoGearRpm * 100) * 0.10471976f * radius / speed;
    ratio /= final;                                     /* (a statement of its own: retail's registers) */
    atTop = 200.0f / (f32)steps[3];
    atBottom = 200.0f / (f32)steps[2];
    stepGrowth = atTop + levelPos * (atBottom - atTop);
    atTop = 200.0f / (f32)steps[5];
    atBottom = 200.0f / (f32)steps[4];
    firstStep = atTop + levelPos * (atBottom - atTop);

    for (i = gears; i > 0; i--) {
        if (i == 1)
            ratio *= firstStep;
        ratios[i] = (s16)(int)(ratio * 1000.0f);
        ratio *= step;
        step *= stepGrowth;
    }
    ratios[0] = (s16)((ratios[1] * 3 + ratios[2]) / 4);
}

/* The range of every gear from the ratios of gear_auto_ratios: lo[i] / hi[i + 1]
 * are 1/3 and 2/3 of the way between gears i and i + 1; the ends are 7/8 of
 * the top gear, 7/8 and 9/8 of the reverse, 9/8 of the 1st. */
// FUNC 0x001E2B30 gear_ranges
void gear_ranges(int gears, s16 *ratios, s16 *lo, s16 *hi)
{
    int i;

    hi[1] = (s16)((ratios[1] * 9) / 8);
    for (i = 1; i < gears; i++) {
        lo[i] = (s16)((ratios[i] * 2 + ratios[i + 1]) / 3);
        hi[i + 1] = (s16)((ratios[i] + ratios[i + 1] * 2) / 3);
    }
    lo[gears] = (s16)((ratios[gears] * 7) / 8);
    lo[0] = (s16)((ratios[0] * 7) / 8);
    hi[0] = (s16)((ratios[0] * 9) / 8);
}

/* The adjustable range of every gear at auto level `level' with the final
 * gear `final' (put into the spec meanwhile). */
// FUNC 0x001E2C58 gear_auto_ranges
void gear_auto_ranges(CarSpec *spec, int level, s16 final, s16 *lo, s16 *hi)
{
    s16 ratios[8];                  /* sp+0x00 */
    s16 savedFinal = spec->finalGear;

    spec->finalGear = final;
    gear_auto_ratios(spec, level, ratios, 0);
    gear_ranges(spec->gearCount, ratios, lo, hi);
    spec->finalGear = savedFinal;
}
