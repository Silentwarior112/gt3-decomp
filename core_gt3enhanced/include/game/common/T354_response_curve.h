/* T354_response_curve.h - ResponseCurve, the shaping curve of the controller
 * inputs (translation unit T354, unit u075).
 *
 * y = sign(x) * f(|x|):
 *   mode 1: f(u) = c (u - a)^2 + b, c = n / 100 (n 1..100), a and b put (0, 0)
 *           and (1, 1) on it;
 *   mode 2: f(u) = c sqrt(u - a) + b, c = n / 10 (n 10..100), through (0, 0)
 *           and (1, 1);
 *   mode 3: f(u) = c u^8 + b u, c = n / 100 (n 0..100), b = 1 - c (the
 *           enhanced engine: Lupo 2's, menus_a.md 38 / physics.md 18; no
 *           GT3 curve uses it);
 *   else    f(u) = u.
 * The users: the steering-force curve 0x0049D050 (u078, SteerCurve there) and
 * the throttle / brake curves of the wheel controller 0x0049D060 / 0x0049D070
 * (u079, InputCurve there). */
#ifndef GAME_T354_RESPONSE_CURVE_H
#define GAME_T354_RESPONSE_CURVE_H

#include "game/game.h"

/* 16 bytes (the .bss curves 0x0049D050, 0x0049D060, 0x0049D070 follow each
 * other). */
typedef struct ResponseCurve {
    int mode;                               /* 0x0 1: quadratic, 2: square root, 3: c u^8 + b u (the enhanced
                                               engine), else linear */
    f32 a;                                  /* 0x4 the x offset */
    f32 b;                                  /* 0x8 the y offset */
    f32 c;                                  /* 0xC the scale */
} ResponseCurve;
EGCS_SIZE(ResponseCurve, 0x10);

/* n: the shape (clamped as above); other modes: a = b = c = 0. */
void responseInit(ResponseCurve *r, int mode, int n);          /* 0x001D91D8 */
/* sign(x) * f(|x|); *slope (if not NULL) = f'(|x|) (mode 2: sqrt(u - a)
 * taken as at least 0.01 there). */
f32  responseEval(const ResponseCurve *r, f32 x, f32 *slope);  /* 0x001D92E0 */

#endif /* GAME_T354_RESPONSE_CURVE_H */
