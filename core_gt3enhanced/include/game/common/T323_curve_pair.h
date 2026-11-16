/* T323_curve_pair.h - two Curves on the same xs, one scan (translation unit
 * T323, unit u075).
 *
 * The Curve record is defined in T320_record_set.h (set up by its
 * curveSet). */
#ifndef GAME_T323_CURVE_PAIR_H
#define GAME_T323_CURVE_PAIR_H

#include "game/game.h"

struct Curve;                               /* T320 (pointers only) */

/* *y1 = t's ys at x, *y2 = t2's ys at x, both on t's xs and count (t2's xs
 * are not read), clamped to the end points; returns 1. */
int curveLookupPair(const struct Curve *t, const struct Curve *t2, f32 *y1, f32 *y2, f32 x); /* 0x001D9098 */

#endif /* GAME_T323_CURVE_PAIR_H */
