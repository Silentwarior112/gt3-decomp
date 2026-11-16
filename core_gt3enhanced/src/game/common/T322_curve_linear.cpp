/* T322 - Curve lookup, linear scan (0x001D8FB8). */
#include "game/common/T322_curve_linear.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "game/common/T320_record_set.h"           /* Curve */
/* (retail's T322 has no .rodata: it saw no class) */

/* y at x: clamped to the end points; the segment found by a scan from the
 * start.  x0 / x1 hold the end points first and the segment's later (as in
 * curveLookup) */
// FUNC 0x001D8FB8 curveLookupLinear
f32 curveLookupLinear(const struct Curve *t, f32 x)
{
    const f32 *xs = t->x;
    u32 last = t->count - 1;
    f32 x0 = xs[0];
    f32 x1 = xs[last];
    f32 y0, y1;
    u32 i;

    if (x <= x0)
        return t->y[0];
    if (x1 <= x)
        return t->y[last];
    for (i = 1; i < last; i++) {
        if (x < xs[i])
            break;
    }
    x0 = xs[i - 1];
    x1 = xs[i];
    y0 = t->y[i - 1];
    y1 = t->y[i];
    return y0 + ((x - x0) * (y1 - y0)) / (x1 - x0);
}
