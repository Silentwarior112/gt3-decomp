/* T323 - two Curves on the same xs (those of t), one scan (0x001D9098). */
#include "game/common/T323_curve_pair.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "game/common/T320_record_set.h"           /* Curve */
/* (retail's T323 has no .rodata: it saw no class) */

/* *y1 = t's y at x, *y2 = t2's y at x (t2's xs and count are not read);
 * returns 1.  y0 is reused for the second curve (retail: one FPU register) */
// FUNC 0x001D9098 curveLookupPair
int curveLookupPair(const struct Curve *t, const struct Curve *t2, f32 *y1, f32 *y2, f32 x)
{
    const f32 *xs = t->x;
    u32 last = t->count - 1;
    const f32 *ys = t->y;
    const f32 *ys2 = t2->y;
    u32 i;
    f32 dx, span, y0;

    if (x <= xs[0]) {
        *y1 = ys[0];
        *y2 = ys2[0];
        return 1;
    }
    if (xs[last] <= x) {
        *y1 = ys[last];
        *y2 = ys2[last];
        return 1;
    }
    for (i = 1; i < last; i++) {
        if (x < xs[i])
            break;
    }
    dx = x - xs[i - 1];
    span = xs[i] - xs[i - 1];
    y0 = ys[i - 1];
    *y1 = y0 + (dx * (ys[i] - y0)) / span;
    y0 = ys2[i - 1];
    *y2 = y0 + (dx * (ys2[i] - y0)) / span;
    return 1;
}
