/* T355 (0x001DE158..0x001DE318): the check of the tyre / car sound table
 * g_002D35B8 (TyreSoundTable) before tyre_sound_init (T333) and
 * car_sound_params_init (T288) build their curves from it: fixed point
 * counts, clamps and four "lower < upper" speed pairs. */
#include "game/car/physics/T355_setting_check.h"
#include "game/car/physics/car_param_internal.h"
// DATA_START 0x002D35B8

/* T355's .data (then its frame info at 0x002D37C0; only u078's T333 / T288
 * code reads the table, hence the anchor): rows [set][surface
 * class] (input1, n1, n2neg, n2pos, x1, y1, x2neg, y2neg, x2pos, y2pos,
 * y1Scale, y2negScale, y2posScale, input2, base, threshold, gain; unk_23
 * zero), then the car sound block. */
TyreSoundTable g_002D35B8 = {               /* 0x002D35B8 */
    {
        {   /* set 0: the squeal volume */
            { 1, 4, 4, 4, { 10, 86, 110, 135 }, { 0, 152, 204, 255 },
              { 9, 80, 148, 255 }, { 0, 100, 203, 203 }, { 9, 80, 148, 255 }, { 0, 100, 203, 203 },
              20, 30, 20, 0, 0, 3, 0 },
            { 1, 4, 4, 4, { 14, 30, 118, 127 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              4, 2, 2, 0, 7, 1, 30 },
            { 1, 4, 4, 4, { 14, 30, 118, 127 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              3, 4, 3, 0, 0, 10, 12 },
            { 1, 4, 4, 4, { 14, 30, 118, 127 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              15, 15, 15, 0, 0, 0, 0 },
        },
        {   /* set 1: the squeal pitch */
            { 1, 4, 4, 4, { 4, 30, 54, 83 }, { 0, 56, 116, 179 },
              { 4, 64, 168, 255 }, { 0, 36, 92, 147 }, { 0, 52, 184, 255 }, { 0, 28, 104, 147 },
              5, 8, 8, 0, 75, 3, 0 },
            { 1, 4, 4, 4, { 10, 62, 162, 255 }, { 0, 60, 196, 203 },
              { 12, 28, 76, 88 }, { 0, 32, 224, 255 }, { 12, 20, 48, 72 }, { 0, 52, 224, 255 },
              20, 8, 8, 0, 50, 10, 10 },
            { 1, 4, 4, 4, { 14, 46, 106, 255 }, { 0, 96, 192, 187 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              4, 3, 3, 0, 50, 30, 15 },
            { 1, 4, 4, 4, { 14, 30, 118, 127 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              15, 15, 15, 0, 50, 0, 0 },
        },
        {   /* set 2: the smoke */
            { 1, 4, 4, 4, { 104, 116, 124, 147 }, { 0, 40, 128, 151 },
              { 136, 144, 160, 180 }, { 0, 32, 224, 255 }, { 144, 152, 168, 188 }, { 0, 52, 224, 255 },
              3, 1, 1, 1, 30, 0, 0 },
            { 1, 4, 4, 4, { 28, 60, 236, 255 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              5, 5, 2, 1, 0, 1, 40 },
            { 1, 4, 4, 4, { 48, 88, 236, 255 }, { 0, 40, 232, 255 },
              { 84, 108, 136, 164 }, { 0, 32, 224, 255 }, { 56, 76, 100, 140 }, { 0, 52, 232, 255 },
              1, 1, 1, 1, 20, 10, 10 },
            { 1, 4, 4, 4, { 28, 60, 236, 255 }, { 0, 40, 232, 255 },
              { 12, 32, 112, 128 }, { 0, 32, 224, 255 }, { 12, 28, 60, 68 }, { 0, 52, 224, 255 },
              15, 15, 15, 0, 0, 0, 0 },
        },
    },
    {   /* src: whine, roll, noise, wind, road, engine, dirt, crash */
        30, 100, 50, 35, 3, 0,              /* whineLo .. unk_05 */
        50, 100, 255, 3, 15, 50, 100, 3,    /* rollLo .. rollMax */
        100,                                /* noiseGain */
        10, 1, 45, 25, 75, 5, 0,            /* windCoef .. unk_15 */
        20, 1, 32, 90, 150, 20,             /* roadCoef .. roadMax */
        13, 13, 20, 25,                     /* engine1C .. engine1F */
        5, 1, 5, 20, 43, 10,                /* dirtVolume .. unk_25 */
        60, 0,                              /* crashGain, unk_27 */
    },
};

// FUNC 0x001DE158 TyreSoundRow_fix
void TyreSoundRow_fix(TyreSoundRow *r, int clear)
{
    r->n1 = 4;
    r->n2neg = 4;
    r->n2pos = 4;
    if (clear) {
        r->y1[0] = 0;
        r->y2neg[0] = 0;
        r->y2pos[0] = 0;
    }
    if (!(r->threshold < 51))
        r->threshold = 50;
}

// FUNC 0x001DE198 setting_fix_pair
/* Makes *lo < *hi: raises *hi to *lo + 1, or (*lo == 0xFF) lowers *lo to
 * *hi - 1; with *lo == 0xFF and *hi == 0 it stores 0xFF into *lo, which
 * already holds it (retail stores the register it compared with 0xFF). */
void setting_fix_pair(u8 *lo, u8 *hi)
{
    if (*hi <= *lo) {
        if (*lo != 0xFF)
            *hi = *lo + 1;
        else if (*hi != 0)
            *lo = *hi - 1;
        else
            *lo = 0xFF;
    }
}

// FUNC 0x001DE1E0 TyreSoundTable_validate
/* The rows of the three sets (0 the squeal volume, its first y values
 * cleared; 1 the squeal pitch, base 50..200; 2 the smoke) of each surface
 * class, then the car sound block's speed ranges and whineAccel (0 -> 1). */
void TyreSoundTable_validate(TyreSoundTable *t)
{
    int i;
    CarSoundSource *src = &t->src;

    for (i = 0; i < 4; i++) {
        u8 base;

        TyreSoundRow_fix(&t->rows[0][i], 1);
        TyreSoundRow_fix(&t->rows[1][i], 0);
        base = t->rows[1][i].base;
        if (base < 50)
            t->rows[1][i].base = 50;
        else if (!(base < 201))
            t->rows[1][i].base = 200;
        TyreSoundRow_fix(&t->rows[2][i], 0);
    }
    setting_fix_pair(&src->rollSpeedLo, &src->rollSpeedHi);
    setting_fix_pair(&src->windSpeedLo, &src->windSpeedHi);
    setting_fix_pair(&src->roadSpeedLo, &src->roadSpeedHi);
    setting_fix_pair(&src->dirtSpeedLo, &src->dirtSpeedHi);
    if (src->whineAccel == 0)
        src->whineAccel = 1;
}
