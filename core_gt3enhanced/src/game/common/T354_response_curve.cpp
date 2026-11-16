/* T354 - a response curve (0x001D91D8..0x001D93C0).
 *
 * y = sign(x) * f(|x|):
 *   mode 1: f(u) = c (u - a)^2 + b, c = n / 100 (n 1..100), a and b put
 *           (0, 0) and (1, 1) on it;
 *   mode 2: f(u) = c sqrt(u - a) + b, c = n / 10 (n 10..100), through
 *           (0, 0) (and (1, 1));
 *   mode 3: f(u) = c u^8 + b u, c = n / 100 (n 0..100), b = 1 - c (the
 *           enhanced engine: Lupo 2's, L2 0x001FE968 / 0x001FEB00;
 *           menus_a.md 38, physics.md 18; no GT3 curve uses it);
 *   else    f(u) = u.
 * The two sqrt.s of the original are R5900 encodings (source in ft) the
 * listing shows as "c1 0x10044" / "c1 0x20084".
 */
#include "game/common/T354_response_curve.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "game/enhanced.h"                      /* ENH_PHYSICS_WHEEL_THROTTLE */
/* (retail's T354 has no .rodata: it saw no class) */

enum {
    RESPONSE_QUADRATIC = 1,                 /* ResponseCurve::mode */
    RESPONSE_SQRT      = 2,
    RESPONSE_POWER8    = 3                  /* the enhanced engine: Lupo 2's c u^8 + b u (menus_a.md 38) */
};

/* Computed in the fields (CSE keeps the stored values: no reloads); local
 * scale / offset variables give other FPU registers.  The sqrt.s is
 * boot_sqrtf_v, a scheduling barrier as in the original. */
// FUNC 0x001D91D8 responseInit
void responseInit(ResponseCurve *r, int mode, int n)
{
    f32 root;

    r->mode = mode;
    switch (mode) {
    case RESPONSE_QUADRATIC:
#if ENH_PHYSICS_WHEEL_THROTTLE
        /* Lupo 2 (L2 0x001FE9A8): n 0..100, c at least 0.001 (GT3: n 1..100);
         * the same curve for n >= 1, no GT3 curve has n <= 0 */
        if (n <= 0)
            n = 0;
        else if (n > 100)
            n = 100;
        r->c = (f32)n * 0.01f;
        if (r->c < 0.001f)
            r->c = 0.001f;
#else
        if (n <= 0)
            n = 1;
        else if (n > 100)
            n = 100;
        r->c = (f32)n * 0.01f;
#endif
        r->a = -(1.0f - r->c) / (r->c + r->c);
        r->b = -r->c * r->a * r->a;
        break;
    case RESPONSE_SQRT:
#if ENH_PHYSICS_WHEEL_THROTTLE
        /* Lupo 2 (L2 0x001FEA20): c = 1 + (100 - n) / 100 for n 0..100, so
         * the curve runs from linear (n 100) to c 2 (n 0); GT3's c = n / 10
         * (1..10).  No GT3 data uses mode 2 here (T336 builds its own
         * mode-2 curve), so GT3's cars do not change. */
        if (n < 0)
            n = 0;
        else if (n > 100)
            n = 100;
        r->c = (f32)(100 - n) * 0.01f + 1.0f;
#else
        if (n < 10)
            n = 10;
        else if (n > 100)
            n = 100;
        r->c = (f32)n * 0.1f;
#endif
        r->a = -((r->c * r->c - 1.0f) * (r->c * r->c - 1.0f)) / ((r->c * r->c) * 4.0f);
        root = boot_sqrtf_v(-r->a);     /* before -c (retail negates c after the sqrt) */
        r->b = -r->c * root;
        break;
    case RESPONSE_POWER8:
        /* Lupo 2 (L2 0x001FEAA8): c = n / 100 (n 0..100), b = 1 - c, so
         * (1, 1) is on it; a 0.  No GT3 curve has mode 3 (physics.md 18:
         * the wheel throttle's, under ENH_PHYSICS_WHEEL_THROTTLE). */
        if (n <= 0)
            n = 0;
        else if (n > 100)
            n = 100;
        r->a = 0.0f;
        r->c = (f32)n * 0.01f;
        r->b = 1.0f - r->c;
        break;
    default:
        r->a = 0.0f;
        r->b = 0.0f;
        r->c = 0.0f;
        break;
    }
}

/* *slope (if not NULL) = f'(|x|); mode 2 keeps sqrt(u - a) >= 0.01 there.
 * u becomes u - a (and its sqrt) in place: retail keeps both in u's register */
// FUNC 0x001D92E0 responseEval
f32 responseEval(const ResponseCurve *r, f32 x, f32 *slope)
{
    f32 u, y;

    u = (x < 0.0f) ? -x : x;
    switch (r->mode) {
    case RESPONSE_QUADRATIC:
        u -= r->a;
        y = r->c * u * u + r->b;
        if (slope != NULL)
            *slope = (r->c + r->c) * u;
        break;
    case RESPONSE_SQRT:
        u -= r->a;
        u = boot_sqrtf_v(u);
        y = r->c * u + r->b;
        if (slope != NULL) {
            if (u < 0.01f)
                u = 0.01f;
            *slope = r->c / (u + u);
        }
        break;
    case RESPONSE_POWER8: {
        /* Lupo 2 (L2 0x001FEBC0..0x001FEBFC, in its order of products):
         * y = c u^8 + b u, slope 8 c u^7 + b. */
        f32 u2 = u * u;
        f32 u4 = u2 * u2;
        y = r->c * u4 * u4 + r->b * u;
        if (slope != NULL)
            *slope = r->c * 8.0f * u4 * (u * u2) + r->b;
        break;
    }
    default:
        y = u;
        if (slope != NULL)
            *slope = 1.0f;
        break;
    }
    if (x < 0.0f)
        y = -y;
    return y;
}
