/* T322_curve_linear.h - Curve lookup, linear scan (translation unit T322,
 * unit u075).
 *
 * The Curve record is defined in T320_record_set.h (set up by its
 * curveSet). */
#ifndef GAME_T322_CURVE_LINEAR_H
#define GAME_T322_CURVE_LINEAR_H

#include "game/game.h"

struct Curve;                               /* T320 (pointers only) */

/* ys at x, linear between the points, clamped to the end points (the
 * segment found by a scan from the start). */
f32 curveLookupLinear(const struct Curve *t, f32 x);   /* 0x001D8FB8 */

#endif /* GAME_T322_CURVE_LINEAR_H */
