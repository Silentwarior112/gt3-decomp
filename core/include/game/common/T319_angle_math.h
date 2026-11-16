/* T319_angle_math.h - angle helpers (translation unit T319, unit u075).
 *
 * Radians.  The sines and cosines are G002's (0x0026A5F0 / 0x0026A1A0). */
#ifndef GAME_T319_ANGLE_MATH_H
#define GAME_T319_ANGLE_MATH_H

#include "game/game.h"

/* The rotation rz * ry * rx: its y, x and z columns (the dynamics units'
 * three rows of a body's rotation from pitch / roll / yaw). */
void eulerToAxes(f32 *axisY, f32 *axisX, f32 *axisZ, f32 rx, f32 ry, f32 rz);  /* 0x001D8A80 */
/* a moved by whole turns towards 0: n = (int)((|a| + pi) / 2pi) turns
 * (wrapAngle's case beyond 3 pi). */
f32  wrapAngleFar(f32 a);                   /* 0x001D8BA8 */
/* a in [-pi, pi). */
f32  wrapAngle(f32 a);                      /* 0x001D8C00 */
/* a - b in [-pi, pi). */
f32  angleDiff(f32 a, f32 b);               /* 0x001D8C80 */
/* wa * a + wb * b with a and b taken on the same side of the turn, wrapped
 * (no callers). */
f32  angleBlend(f32 wa, f32 a, f32 wb, f32 b);  /* 0x001D8CF8 */

#endif /* GAME_T319_ANGLE_MATH_H */
