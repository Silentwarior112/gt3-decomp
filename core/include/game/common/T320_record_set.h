/* T320_record_set.h - the set-up of a Curve, the piecewise-linear table of
 * the dynamics (translation unit T320, unit u075).
 *
 * The Curve record is defined here (planned in T284_car_param.h, u077 having
 * the fullest copy; the first pass's T320Record / CurveTable in u075): this
 * is its set-up, T321..T323 its lookups.  The single setters always succeed
 * (1); curveSet calls all three, clears bytes 2 and 3 and returns 1 if all
 * of them did. */
#ifndef GAME_T320_RECORD_SET_H
#define GAME_T320_RECORD_SET_H

#include "game/game.h"

/* A piecewise-linear table y(x) over count points x[] / y[] (x ascending):
 * every curve of the dynamics (T279, T291, T296, T328, T333 hold them).  Not
 * polymorphic.  Size 0xC (CarEngine's rpm[] follows it at +0xC; the curve of
 * SurfaceTable g_002D2CE0: {8, 0x002D2CA0, 0x002D2CC0} in the file). */
typedef struct Curve {
    s16  count;                             /* 0x0 points (lh 0x001D8E54, sh 0x001D8DA0) */
    u8   unk_2;                             /* 0x2 cleared by curveSet */
    u8   unk_3;                             /* 0x3 cleared by curveSet */
    f32 *x;                                 /* 0x4 */
    f32 *y;                                 /* 0x8 */
} Curve;
EGCS_SIZE(Curve, 0xC);

int  curveSetCount(struct Curve *c, int n); /* 0x001D8DA0 count = (s16)n; 1 (T320_setUnk0) */
int  curveSetXs(struct Curve *c, f32 *xs);  /* 0x001D8DB0 1 (T320_setUnk4) */
int  curveSetYs(struct Curve *c, f32 *ys);  /* 0x001D8DC0 1 (T320_setUnk8) */
/* n points, xs ascending.  Every call (11, in u076 / u077 / u078) also
 * passes $a4 / $a5 (0, 0; T333: mode 0 or 2); the code never reads them. */
int  curveSet(struct Curve *c, int n, f32 *xs, f32 *ys, int mode, int flags);  /* 0x001D8DD0 (T320_set) */

#endif /* GAME_T320_RECORD_SET_H */
