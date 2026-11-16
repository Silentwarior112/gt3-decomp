/* T321_curve_lookup.h - Curve lookup, binary search (translation unit T321,
 * unit u075).
 *
 * The Curve record is defined in T320_record_set.h (set up by its
 * curveSet); count is read as a halfword (lh, 0x001D8E54). */
#ifndef GAME_T321_CURVE_LOOKUP_H
#define GAME_T321_CURVE_LOOKUP_H

#include "game/game.h"

struct Curve;                               /* T320 (pointers only) */

/* ys at x, linear between the points, clamped to the end points: a binary
 * search narrows the segment to fewer than 4 points (an exact hit returns
 * that point), then a linear scan. */
f32 curveLookup(const struct Curve *t, f32 x);     /* 0x001D8E50 */

#endif /* GAME_T321_CURVE_LOOKUP_H */
