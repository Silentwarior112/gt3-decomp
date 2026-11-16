/* T320 - the set-up of a Curve (0x001D8DA0..0x001D8E50).  The single
 * setters always succeed (1); curveSet clears bytes 2 and 3. */
#include "game/common/T320_record_set.h"    /* this translation unit */
#include "game/common/curves_internal.h"
/* (retail's T320 has no .rodata: it saw no class) */

// FUNC 0x001D8DA0 curveSetCount
int curveSetCount(struct Curve *c, int n)
{
    c->count = (s16)n;
    return 1;
}

// FUNC 0x001D8DB0 curveSetXs
int curveSetXs(struct Curve *c, f32 *xs)
{
    c->x = xs;
    return 1;
}

// FUNC 0x001D8DC0 curveSetYs
int curveSetYs(struct Curve *c, f32 *ys)
{
    c->y = ys;
    return 1;
}

/* all three setters are called; 1 if all of them returned non-zero.  mode
 * and flags ($a4 / $a5) are never read. */
// FUNC 0x001D8DD0 curveSet
int curveSet(struct Curve *c, int n, f32 *xs, f32 *ys, int mode, int flags)
{
    int ok;

    ok = curveSetCount(c, n);
    ok = (ok != 0) & (curveSetXs(c, xs) != 0);
    ok = (ok != 0) & (curveSetYs(c, ys) != 0);
    c->unk_2 = 0;
    c->unk_3 = 0;
    return ok;
}
