/* T321 - Curve lookup, binary search (0x001D8E50). */
#include "game/common/T321_curve_lookup.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "game/common/T320_record_set.h"           /* Curve */
/* (retail's T321 has no .rodata: it saw no class) */

/* y at x: clamped to the end points; a binary search narrows the segment
 * to fewer than 4 points (an exact hit returns that point), then a linear
 * scan.  x0 / x1 hold the end points first and the segment's later (one
 * variable each: retail keeps both uses in one FPU register) */
// FUNC 0x001D8E50 curveLookup
f32 curveLookup(const struct Curve *t, f32 x)
{
    const f32 *xs = t->x;
    u32 last = t->count - 1;
    f32 x0 = xs[0];
    f32 x1 = xs[last];
    f32 y0, y1;
    u32 lo, hi, mid, i;

    if (x <= x0)
        return t->y[0];
    if (x1 <= x)
        return t->y[last];
    hi = last;
    lo = 0;
    while (hi - lo >= 4) {
        mid = (hi + lo) >> 1;
        if (x < xs[mid])
            hi = mid;
        else if (x == xs[mid])
            return t->y[mid];
        else
            lo = mid;
    }
    for (i = lo; i < hi; i++) {
        if (x < xs[i])
            break;
    }
    x0 = xs[i - 1];
    x1 = xs[i];
    y0 = t->y[i - 1];
    y1 = t->y[i];
    return y0 + ((y1 - y0) * (x - x0)) / (x1 - x0);
}
