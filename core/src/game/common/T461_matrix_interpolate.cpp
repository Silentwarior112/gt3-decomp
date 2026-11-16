/* T461 - matrix interpolation (the prize model's key frames, T459).
 *
 * out = a "weighted" wa : wb between two 4x4 matrices.  The rotation part:
 * the relative rotation of the two (columns normalised) as a quaternion
 * (axis, half angle), its angle scaled by wb / (wa + wb), composed with a's
 * rotation.  Row 3 and column 3 are interpolated linearly.  When there is
 * no relative rotation axis (or wa + wb is 0) the whole matrix is blended
 * linearly (prize_matrix_blend).
 */
/* .rodata (0x00341488..0x0034149E, referenced by nothing): the type name
 * "20PrizeModelSetContext" (T459_prize_model.h), the second copy after
 * T460's. */
// DATA_START 0x00341488
#include "game/menu/gtmode/prize_model_internal.h"
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, cosf, sinf */
#include "sdk/libcdvd/G004_libcdvd.h"              /* sqrt */
#include "game/common/T461_matrix_interpolate.h"
#include "game/menu/gtmode/T459_prize_model.h"          /* prize_matrix_blend; PrizeModelSetContext */

// FUNC 0x0022A3F8 matrix_slerp_rotation
//   0: wa + wb == 0 or no rotation axis (out untouched).
//   Matching (82.3%): structure and schedule are close; left are register
//   numbers (the quaternion's FPU registers, the loop counters) and the
//   rotation loop's dot products, where retail copies the destination
//   pointer as an inline function's argument would (a helper for both dot
//   products gives that copy but loses elsewhere: 79-83%).
int matrix_slerp_rotation(MatrixSlerp *s, PrizeMatrix *out, f32 wa, f32 wb)
{
    f32 rot[3][3];                              /* sp+0x00 */
    f32 axis[3];                                /* sp+0x30 */
    f32 weightA = wa + wb;
    f32 axisLen, cosHalf, halfAngle, sinAngle, qx, qy, qz, qw;
    const f32 *ma;
    int row, col, k;

    if (weightA == 0.0f)
        return 0;
    weightA = wa / weightA;

    /* rot = the relative rotation: rot[i][j] = rb[i] . ra[j] */
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 3; col++) {
            rot[row][col] = 0.0f;
            for (k = 0; k < 3; k++)
                rot[row][col] += s->rb[row][k] * s->ra[col][k];
        }
    }
    axis[0] = rot[2][1] - rot[1][2];
    axis[1] = rot[0][2] - rot[2][0];
    axis[2] = rot[1][0] - rot[0][1];
    axisLen = (f32)sqrt((double)((axis[0] * axis[0] + axis[1] * axis[1]) + axis[2] * axis[2]));  /* implicit: fptodp, dptofp (float <-> double) */
    if (axisLen == 0.0f)
        return 0;
    for (k = 0; k < 3; k++)
        axis[k] = axis[k] / axisLen;

    /* cos of the half angle: sqrt(trace + 1) / 2 */
    cosHalf = boot_sqrtf_v(((rot[0][0] + rot[1][1]) + rot[2][2]) + 1.0f) * 0.5f;
    if (cosHalf <= -1.0f)
        halfAngle = 3.1415927f;
    else if (1.0f <= cosHalf)
        halfAngle = 0.0f;
    else if (cosHalf == 0.0f)
        halfAngle = 1.5707964f;
    else
        halfAngle = atan2f(boot_sqrtf_v(1.0f - cosHalf * cosHalf), cosHalf);
    halfAngle = halfAngle * (1.0f - weightA);
    sinAngle = sinf(halfAngle);
    qx = axis[0] * sinAngle;
    qy = axis[1] * sinAngle;
    qz = axis[2] * sinAngle;
    qw = cosf(halfAngle);

    /* rot = the rotation of the quaternion (qx, qy, qz, qw) */
    {
        f32 yy = qy * qy, xx = qx * qx, zz = qz * qz;
        f32 wx = qw * qx, xy = qx * qy, yz = qy * qz;
        f32 xxyy = xx + yy;
        f32 yw = qy * qw, zzxx = zz + xx, zw = qz * qw, xz = qx * qz;
        f32 yyzz = yy + zz;

        rot[0][0] = 1.0f - (yyzz + yyzz);
        rot[1][1] = 1.0f - (zzxx + zzxx);
        rot[0][1] = (xy - zw) + (xy - zw);
        rot[0][2] = (xz + yw) + (xz + yw);
        rot[1][0] = (xy + zw) + (xy + zw);
        rot[1][2] = (yz - wx) + (yz - wx);
        rot[2][0] = (xz - yw) + (xz - yw);
        rot[2][1] = (yz + wx) + (yz + wx);
        rot[2][2] = 1.0f - (xxyy + xxyy);
    }

    /* out: column i < 3 of row j < 3 = rot[i] . a's row j; row 3 and
     * column 3 linearly */
    ma = s->a->m;
    for (col = 0; col < 4; col++) {
        for (row = 0; row < 4; row++) {
            if (col == 3 || row == 3) {
                out->m[row * 4 + col] = ma[row * 4 + col] * weightA + s->b->m[row * 4 + col] * (1.0f - weightA);
            } else {
                out->m[row * 4 + col] = 0.0f;
                for (k = 0; k < 3; k++)
                    out->m[row * 4 + col] += rot[col][k] * ma[row * 4 + k];
            }
        }
    }
    return 1;
}

// FUNC 0x0022A7F8 matrix_interpolate
//   Matching (77.0%): ra / rb as row pointers held in registers ($t2 / $t3)
//   and the sums by index are retail's; left are register numbers (wa / wb
//   in $f21 / $f20 swapped, the column counter) and with them the order of
//   the three loads in the normalising branch.
//   The PrizeMatrixLerp of prize_model_step / prize_model_loop (T459).
void matrix_interpolate(PrizeMatrix *out, const PrizeMatrix *a, const PrizeMatrix *b,
                        f32 wa, f32 wb)
{
    MatrixSlerp s;                              /* sp+0x00 */
    f32 (*ra)[3] = s.ra;
    f32 (*rb)[3] = s.rb;
    int col, left;

    s.a = a;
    s.b = b;
    for (col = 0; col < 3; col++) {
        f32 len = 0.0f;

        for (left = 0; left < 3; left++)
            len += a->m[col + left * 4] * a->m[col + left * 4];
        len = boot_sqrtf(len);
        if (0.0f < len) {
            ra[col][1] = a->m[col + 4] / len;
            ra[col][2] = a->m[col + 8] / len;
            ra[col][0] = a->m[col] / len;
        } else {
            ra[col][2] = a->m[col + 8];
            ra[col][0] = a->m[col];
            ra[col][1] = a->m[col + 4];
        }
    }
    for (col = 0; col < 3; col++) {
        f32 len = 0.0f;

        for (left = 0; left < 3; left++)
            len += b->m[col + left * 4] * b->m[col + left * 4];
        len = boot_sqrtf(len);
        if (0.0f < len) {
            rb[col][1] = b->m[col + 4] / len;
            rb[col][2] = b->m[col + 8] / len;
            rb[col][0] = b->m[col] / len;
        } else {
            rb[col][2] = b->m[col + 8];
            rb[col][0] = b->m[col];
            rb[col][1] = b->m[col + 4];
        }
    }

    if (matrix_slerp_rotation(&s, out, wa, wb) != 1)
        prize_matrix_blend(out, a, b, wa, wb);
}
