/* T461_matrix_interpolate.h - matrix interpolation: the prize model's key
 * frames (translation unit T461, unit u103).
 *
 * out = a "weighted" wa : wb between two 4x4 matrices.  The rotation part:
 * the relative rotation of the two (columns normalised) as a quaternion
 * (axis, half angle), its angle scaled by wb / (wa + wb), composed with a's
 * rotation; row 3 and column 3 linearly.  Without a relative rotation axis
 * (or with wa + wb = 0) the whole matrix is blended linearly
 * (prize_matrix_blend, T459).
 */
#ifndef GAME_T461_MATRIX_INTERPOLATE_H
#define GAME_T461_MATRIX_INTERPOLATE_H

#include "game/game.h"

struct PrizeMatrix;                             /* T459_prize_model.h */

/* ? name.  matrix_interpolate's stack record (sp+0x00) for
 * matrix_slerp_rotation, 0x50 bytes. */
typedef struct MatrixSlerp {
    f32                       ra[3][3];         /* 0x00 row i: a's column i, normalised */
    f32                       rb[3][3];         /* 0x24 the same for b */
    const struct PrizeMatrix *a;                /* 0x48 */
    const struct PrizeMatrix *b;                /* 0x4C */
} MatrixSlerp;
EGCS_SIZE(MatrixSlerp, 0x50);

/* The rotation part of out (and row / column 3).  0: wa + wb == 0 or no
 * rotation axis (out untouched); 1: done. */
int  matrix_slerp_rotation(MatrixSlerp *s, struct PrizeMatrix *out, f32 wa, f32 wb);  /* 0x0022A3F8 */
/* The PrizeMatrixLerp of prize_model_step / prize_model_loop (T459). */
void matrix_interpolate(struct PrizeMatrix *out, const struct PrizeMatrix *a,
                        const struct PrizeMatrix *b, f32 wa, f32 wb);                /* 0x0022A7F8 */

#endif /* GAME_T461_MATRIX_INTERPOLATE_H */
