/* T354 - a response curve (0x001D91D8..0x001D93C0).
 *
 * y = sign(x) * f(|x|):
 *   mode 1: f(u) = c (u - a)^2 + b, c = n / 100 (n 1..100), a and b put
 *           (0, 0) and (1, 1) on it;
 *   mode 2: f(u) = c sqrt(u - a) + b, c = n / 10 (n 10..100), through
 *           (0, 0) (and (1, 1));
 *   else    f(u) = u.
 * The two sqrt.s of the original are R5900 encodings (source in ft) the
 * listing shows as "c1 0x10044" / "c1 0x20084".
 */
#include "game/common/T354_response_curve.h"    /* this translation unit */
#include "game/common/curves_internal.h"
/* (retail's T354 has no .rodata: it saw no class) */

enum {
    RESPONSE_QUADRATIC = 1,                 /* ResponseCurve::mode */
    RESPONSE_SQRT      = 2
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
        if (n <= 0)
            n = 1;
        else if (n > 100)
            n = 100;
        r->c = (f32)n * 0.01f;
        r->a = -(1.0f - r->c) / (r->c + r->c);
        r->b = -r->c * r->a * r->a;
        break;
    case RESPONSE_SQRT:
        if (n < 10)
            n = 10;
        else if (n > 100)
            n = 100;
        r->c = (f32)n * 0.1f;
        r->a = -((r->c * r->c - 1.0f) * (r->c * r->c - 1.0f)) / ((r->c * r->c) * 4.0f);
        root = boot_sqrtf_v(-r->a);     /* before -c (retail negates c after the sqrt) */
        r->b = -r->c * root;
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
