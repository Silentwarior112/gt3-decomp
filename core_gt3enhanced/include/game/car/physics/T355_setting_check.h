/* T355_setting_check.h - the tyre and car sound table g_002D35B8 and its
 * validation (translation unit T355, unit u077), 0x001DE158..0x001DE318.
 *
 * g_002D35B8 is T355's .data (0x208 bytes up to its .eh_frame at 0x002D37C0).
 * TyreSoundTable_validate (u077 CarSettingBlock_validate; u078 calls it
 * F_001DE1E0 "prepares it") fixes it up before tyre_sound_init (T333) builds
 * the squeal / smoke curves and car_sound_params_init (T288) the car sound
 * parameters: fixed point counts, clamps, and four "lower < upper" speed
 * pairs.  u077 read the table as a car setting block (CarSettingBlock of
 * SettingRec); the layout is u078's TyreSoundTable. */
#ifndef GAME_T355_SETTING_CHECK_H
#define GAME_T355_SETTING_CHECK_H

#include "game/game.h"

/* One surface class of a curve set source (u077 SettingRec).  Not
 * polymorphic.  Size 0x28 (the array stride). */
class TyreSoundRow {
public:
    u8 input1;                              /* 0x00 curve 1's input: 0 the slip angle (x 0.2 degree), else the
                                               lateral slip (x 1/9 m/s) */
    u8 n1;                                  /* 0x01 points of curve 1 (forced to 4) */
    u8 n2neg;                               /* 0x02 points of curve 2 below 0 (forced to 4) */
    u8 n2pos;                               /* 0x03 points of curve 2 above 0 (forced to 4) */
    u8 x1[4];                               /* 0x04 */
    u8 y1[4];                               /* 0x08 ([0] cleared in the volume rows) */
    u8 x2neg[4];                            /* 0x0C (negated, stored outwards) */
    u8 y2neg[4];                            /* 0x10 ([0] cleared in the volume rows) */
    u8 x2pos[4];                            /* 0x14 */
    u8 y2pos[4];                            /* 0x18 ([0] cleared in the volume rows) */
    u8 y1Scale;                             /* 0x1C / 2550 */
    u8 y2negScale;                          /* 0x1D / 2550 */
    u8 y2posScale;                          /* 0x1E / 2550 */
    u8 input2;                              /* 0x1F curve 2's input: 0 wheel +0x68 (x 1/255), else the
                                               longitudinal slip (x 1/18 m/s) */
    u8 base;                                /* 0x20 x 0.01 -> TyreCurveSet.base (pitch rows: 50..200) */
    u8 threshold;                           /* 0x21 km/h, at most 50 -> TyreCurveSet.threshold */
    u8 gain;                                /* 0x22 x 0.01 -> TyreCurveSet.gain */
    u8 unk_23[5];                           /* 0x23 */
};
EGCS_SIZE(TyreSoundRow, 0x28);

/* The car sound block of the table (CarSoundParams' source, T288).  Not
 * polymorphic.  Size 0x28. */
class CarSoundSource {
public:
    u8 whineLo;                             /* 0x00 x 0.01 */
    u8 whineHi;                             /* 0x01 x 0.01 */
    u8 whineAccel;                          /* 0x02 x 9.8 (0 -> 1) */
    u8 whinePitchTop;                       /* 0x03 x 0.1 at 10000 rpm */
    u8 whinePitchBase;                      /* 0x04 x 0.1 */
    u8 unk_05;                              /* 0x05 */
    u8 rollLo;                              /* 0x06 x 0.01 */
    u8 rollHi;                              /* 0x07 x 0.01 */
    u8 rollGain;                            /* 0x08 x 0.01 */
    u8 rollSpeedLo;                         /* 0x09 x 10 km/h (x 2.777778 m/s), < rollSpeedHi */
    u8 rollSpeedHi;                         /* 0x0A x 10 km/h */
    u8 rollPitchLo;                         /* 0x0B x 0.01 */
    u8 rollPitchHi;                         /* 0x0C x 0.01 */
    u8 rollMax;                             /* 0x0D x 0.1 */
    u8 noiseGain;                           /* 0x0E x 0.1 */
    u8 windCoef;                            /* 0x0F x 0.001, squared */
    u8 windSpeedLo;                         /* 0x10 x 10 km/h, < windSpeedHi */
    u8 windSpeedHi;                         /* 0x11 x 10 km/h */
    u8 windPitchLo;                         /* 0x12 x 100 */
    u8 windPitchHi;                         /* 0x13 x 100 */
    u8 windMax;                             /* 0x14 x 0.1 */
    u8 unk_15;                              /* 0x15 */
    u8 roadCoef;                            /* 0x16 x 0.001, squared */
    u8 roadSpeedLo;                         /* 0x17 x 10 km/h, < roadSpeedHi */
    u8 roadSpeedHi;                         /* 0x18 x 10 km/h */
    u8 roadPitchLo;                         /* 0x19 x 0.01 */
    u8 roadPitchHi;                         /* 0x1A x 0.01 */
    u8 roadMax;                             /* 0x1B x 0.1 */
    u8 engine1C;                            /* 0x1C x 0.01 */
    u8 engine1D;                            /* 0x1D x 0.01 */
    u8 engine1E;                            /* 0x1E x 0.01 */
    u8 engine1F;                            /* 0x1F x 0.1 */
    u8 dirtVolume;                          /* 0x20 x 0.1: the fixed squeal of surfaces 1 / 13 (g_0049CFC0; ? kerbs) */
    u8 dirtSpeedLo;                         /* 0x21 x 10 km/h, < dirtSpeedHi */
    u8 dirtPitchLo;                         /* 0x22 x 0.1 */
    u8 dirtSpeedHi;                         /* 0x23 x 10 km/h */
    u8 dirtPitchHi;                         /* 0x24 x 0.1 */
    u8 unk_25;                              /* 0x25 */
    u8 crashGain;                           /* 0x26 x 0.01 */
    u8 unk_27;                              /* 0x27 */
};
EGCS_SIZE(CarSoundSource, 0x28);

/* [set][surface class]: set 0 the squeal volume, 1 the squeal pitch, 2 the
 * smoke; tyre_sound_update's surface classes: 1 (surfaces 2..5), 2 (6,
 * 12..14), 0 the others.  Not polymorphic.  Size 0x208 (u077 CarSettingBlock). */
class TyreSoundTable {
public:
    TyreSoundRow   rows[3][4];              /* 0x000 */
    CarSoundSource src;                     /* 0x1E0 */
};
EGCS_SIZE(TyreSoundTable, 0x208);

extern TyreSoundTable g_002D35B8;           /* 0x002D35B8 */

/* n1 = n2neg = n2pos = 4; with clear, y1[0] = y2neg[0] = y2pos[0] = 0;
 * threshold at most 50. */
void TyreSoundRow_fix(TyreSoundRow *r, int clear);          /* 0x001DE158 (u077 SettingRec_fix) */
/* Makes *lo < *hi: raises *hi to *lo + 1, or (*lo == 0xFF) lowers *lo to
 * *hi - 1; with *lo == 0xFF and *hi == 0 it stores 0xFF (unchanged). */
void setting_fix_pair(u8 *lo, u8 *hi);                      /* 0x001DE198 */
void TyreSoundTable_validate(TyreSoundTable *t);            /* 0x001DE1E0 (u077 CarSettingBlock_validate) */

#endif /* GAME_T355_SETTING_CHECK_H */
