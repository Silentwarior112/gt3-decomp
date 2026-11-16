/* T319 - angle helpers (0x001D8A80..0x001D8DA0), radians. */
#include "game/common/T319_angle_math.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "engine/media/G002_mpeg_frameq.h"          /* sinf, cosf */
/* (retail's T319 has no .rodata: it saw no class) */

#define T319_PI        3.1415927f           /* pi */
#define T319_TWO_PI    6.2831855f           /* 2 pi, one turn */
#define T319_THREE_PI  9.424778f            /* 3 pi: beyond it wrapAngle needs more than one turn */

// FUNC 0x001D8A80 eulerToAxes
void eulerToAxes(f32 *axisY, f32 *axisX, f32 *axisZ, f32 rx, f32 ry, f32 rz)
{
    f32 sx = sinf(rx);
    f32 cx = cosf(rx);
    f32 sy = sinf(ry);
    f32 cy = cosf(ry);
    f32 sz = sinf(rz);
    f32 cz = cosf(rz);

    axisX[0] = cz * cy;
    axisX[1] = sz * cy;
    axisX[2] = -sy;
    axisY[0] = cz * sy * sx - sz * cx;
    axisY[1] = sz * sy * sx + cz * cx;
    axisY[2] = cy * sx;
    axisZ[0] = cz * sy * cx + sz * sx;
    axisZ[1] = sz * sy * cx - cz * sx;
    axisZ[2] = cy * cx;
}

/* a moved by whole turns towards 0: turns = (int)((|a| + pi) / 2pi) */
// FUNC 0x001D8BA8 wrapAngleFar
f32 wrapAngleFar(f32 a)
{
    f32 turns = (f32)(int)((boot_fabsf(a) + T319_PI) / T319_TWO_PI);

    if (a < 0.0f)
        return a + turns * T319_TWO_PI;
    return a + turns * -T319_TWO_PI;
}

/* a in [-pi, pi) */
// FUNC 0x001D8C00 wrapAngle
f32 wrapAngle(f32 a)
{
    if (a < -T319_PI) {
        if (a < -T319_THREE_PI)
            return wrapAngleFar(a);
        a += T319_TWO_PI;
    } else if (T319_PI <= a) {
        if (T319_THREE_PI < a)
            return wrapAngleFar(a);
        a -= T319_TWO_PI;
    }
    return a;
}

/* a - b in [-pi, pi) */
// FUNC 0x001D8C80 angleDiff
f32 angleDiff(f32 a, f32 b)
{
    f32 diff;

    a = wrapAngle(a);
    b = wrapAngle(b);
    diff = a - b;
    if (T319_PI <= diff)
        diff -= T319_TWO_PI;
    else if (diff < -T319_PI)
        diff += T319_TWO_PI;
    return diff;
}

/* wa * a + wb * b with a and b taken on the same side of the turn
 * (no callers) */
// FUNC 0x001D8CF8 angleBlend
f32 angleBlend(f32 wa, f32 a, f32 wb, f32 b)
{
    f32 diff;

    a = wrapAngle(a);
    b = wrapAngle(b);
    diff = a - b;
    if (T319_PI <= diff)
        b += T319_TWO_PI;
    else if (diff < -T319_PI)
        a += T319_TWO_PI;
    return wrapAngle(wa * a + wb * b);
}
