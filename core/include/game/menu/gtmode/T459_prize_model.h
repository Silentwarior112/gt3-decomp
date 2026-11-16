/* T459_prize_model.h - PrizeModel (? name, as in u019): the .prm trophy model
 * file and its pose (translation unit T459, unit u103).
 *
 * The file has two series of key matrices (one per frame), applied as
 * PrizeModelSetContext::root then ::body around the embedded T451 model,
 * lights (up to 8 set) and per light a planar shadow: the model drawn
 * flattened onto a horizontal plane along the light's direction.  The
 * animation runs at `speed' keys per 1/60 s from startFrame to endFrame + 1,
 * then goes on from loopFrame; between two keys the matrices are
 * interpolated (matrix_interpolate, T461).  The file stores offsets relative
 * to `self'.
 *
 * PrizeModelSetContext (the pose, below) has three methods defined in this
 * translation unit: applyRoot 0x002296E0, applyBody 0x00229738, copy
 * 0x00229798.  It is defined here, not in T460_prize_base.h (which includes
 * this header where the class was): retail's T459 and T461 have its type
 * name "20PrizeModelSetContext" (0x003413F0, 0x00341488) but not
 * CallBackWhenPrizeRendering's, so they saw this header, not T460's (u103's
 * data pass).
 */
#ifndef GAME_T459_PRIZE_MODEL_H
#define GAME_T459_PRIZE_MODEL_H

#include "game/game.h"

struct ModelSet;                                /* T451_model_set.h */
struct PrizeMatrix;

/* ? name.  s on the diagonal, 0 elsewhere (PrizeMatrix's constructor: 1;
 * T459's prize_model_reset expands it with 1).  Declared before PrizeMatrix,
 * defined after it: the matrix's inline constructor is compiled before the
 * body, so it calls it (PrizeBase::PrizeBase, the only unit that constructs
 * the matrices: two jal 0x0022A3B0), and T460 emits the copy 0x0022A3B0 with
 * its deferred inline functions, last: after PrizeModelSetContext's
 * destructor and typeinfo function, as retail (egcs writes the deferred
 * functions in the reverse of the order it saved them; this body is saved
 * before PrizeModelSetContext is complete).  Written column by column (the
 * store order of the copies: T459's prize_model_reset, T460's 0x0022A3B0). */
inline void prize_matrix_set_diagonal(struct PrizeMatrix *mat, f32 s);    /* 0x0022A3B0 */

/* ? name.  A 4x4 matrix, m[row * 4 + col] (row 3: the translation). */
typedef struct PrizeMatrix {
    f32 m[16];                                  /* 0x00 */
#ifdef __cplusplus
    /* The identity.  Inline in the original (u103's data pass): expanded
     * into PrizeModelSetContext's constructor (PrizeBase::PrizeBase), where
     * it calls prize_matrix_set_diagonal. */
    PrizeMatrix() { prize_matrix_set_diagonal(this, 1.0f); }
#endif
} PrizeMatrix;
EGCS_SIZE(PrizeMatrix, 0x40);

inline void prize_matrix_set_diagonal(PrizeMatrix *mat, f32 s)
{
    f32 *m = mat->m;

    m[0] = s;
    m[4] = 0.0f;
    m[8] = 0.0f;
    m[12] = 0.0f;
    m[1] = 0.0f;
    m[5] = s;
    m[9] = 0.0f;
    m[13] = 0.0f;
    m[2] = 0.0f;
    m[6] = 0.0f;
    m[10] = s;
    m[14] = 0.0f;
    m[3] = 0.0f;
    m[7] = 0.0f;
    m[11] = 0.0f;
    m[15] = s;
}

/* ? name.  A light of a prize model, 0x40 bytes: four parameters given to
 * pgl_lightfv (G001's GsLight pnames: 13 position, 1 diffuse, 0 ambient,
 * 4 specular).  The position is also the direction of the shadow
 * (prize_shadow_matrix: a directional light, w = 0). */
typedef struct PrizeLight {
    f32 dir[4];                                 /* 0x00 pname 13: the position (u103: ? rename position) */
    f32 p1[4];                                  /* 0x10 pname 1: the diffuse colour (? rename diffuse) */
    f32 p0[4];                                  /* 0x20 pname 0: the ambient colour (? rename ambient) */
    f32 p4[4];                                  /* 0x30 pname 4: the specular colour (? rename specular) */
} PrizeLight;
EGCS_SIZE(PrizeLight, 0x40);

/* ? name.  The .prm file. */
typedef struct PrizeModel {
    u8                 unk_00[4];               /* 0x00 */
    struct PrizeModel *self;                    /* 0x04 the address the offsets are for */
    u8                 unk_08[4];               /* 0x08 */
    int                size;                    /* 0x0C PrizeBase::getSize */
    f32                startFrame;              /* 0x10 */
    f32                endFrame;                /* 0x14 the last key */
    f32                loopFrame;               /* 0x18 where it goes on after the end */
    f32                speed;                   /* 0x1C keys per second; per 1/60 s once relocated */
    u16                lightCount;              /* 0x20 (up to 8 are set) */
    u16                unk_22;                  /* 0x22 */
    PrizeMatrix       *rootKeys;                /* 0x24 the keys of PrizeModelSetContext::root */
    PrizeMatrix       *bodyKeys;                /* 0x28 the keys of PrizeModelSetContext::body */
    struct ModelSet   *model;                   /* 0x2C the embedded model (part 0 the trophy, part 1 drawn
                                                   flattened as the shadows: PrizeBase::draw) */
    PrizeLight        *lights;                  /* 0x30 [lightCount] */
    u16                flags;                   /* 0x34 1: root is key 0 only, 2: body is key 0 only */
    u16                unk_36;                  /* 0x36 */
    f32                focus;                   /* 0x38 PrizeBase::getFocus */
    f32                eyeDelta;                /* 0x3C PrizeBase::getEyeDelta */
} PrizeModel;

/* How prize_model_pose combines two keys: out = a weighted wa : wb with b. */
typedef void (*PrizeMatrixLerp)(PrizeMatrix *out, const PrizeMatrix *a, const PrizeMatrix *b,
                                f32 wa, f32 wb);

PrizeMatrix *prize_matrix_copy(PrizeMatrix *dst, const PrizeMatrix *src);  /* 0x00229520 returns dst */
/* Sets the four parameters of light `light' (a gfx_enable id, 0x65 + the
 * light) and enables it. */
void prize_light_set(const PrizeLight *l, int light);                       /* 0x00229558 */
/* Multiplies the current matrix by the light's shadow projection onto the
 * plane at height y (mode != 0: straight down).  0: dir.y is 0 (nothing
 * done). */
int  prize_shadow_matrix(const PrizeLight *l, int mode, f32 y);             /* 0x002295D0 */
/* out = wa * a + wb * b, element by element. */
void prize_matrix_blend(PrizeMatrix *out, const PrizeMatrix *a, const PrizeMatrix *b,
                        f32 wa, f32 wb);                                    /* 0x002297E0 */
/* Relocates the file in place (nothing when it is there) and its model;
 * the speed becomes keys per 1/60 s. */
void prize_model_relocate(PrizeModel *m);                                   /* 0x00229820 */
/* One frame: time += speed (past endFrame + 1 on from loopFrame), then the
 * pose. */
void prize_model_step(PrizeModel *m, struct PrizeModelSetContext *ctx);     /* 0x002298C0 */
/* time = loopFrame, then the pose. */
void prize_model_loop(PrizeModel *m, struct PrizeModelSetContext *ctx);     /* 0x002299A0 */
/* The matrices at ctx->time (key 0 only for a series the flags fix). */
void prize_model_pose(PrizeModel *m, struct PrizeModelSetContext *ctx,
                      PrizeMatrixLerp lerp);                                /* 0x002299D0 */
/* time = startFrame, both matrices identity. */
void prize_model_reset(PrizeModel *m, struct PrizeModelSetContext *ctx);    /* 0x00229B10 */
/* The shadow matrix of light i (0: no such light). */
int  prize_model_shadow(PrizeModel *m, int i, int mode, f32 y);             /* 0x00229BA8 */
/* The light model (global ambient black), lights 101..108 (gfx ids of the
 * lights 0..7): the model's, the others disabled. */
void prize_model_lights(PrizeModel *m);                                     /* 0x00229BE8 */

/* vtable 0x00341430 (1 slot), typeinfo __tf20PrizeModelSetContext 0x0022A370
 * (both T460's: PrizeBase::PrizeBase is the only unit that constructs one).
 * The pose of a prize model.  0x84 bytes of data, egcs vptr +0x84; 0x88
 * bytes (PrizeBase +0x04).  No key function: every unit that sees the class
 * has its type name. */
class PrizeModelSetContext {
public:
    /* inline: expanded in PrizeBase::PrizeBase (root and body: PrizeMatrix's
     * constructor, the identity) */
    PrizeModelSetContext() { time = 0.0f; }
    /* Inline in the original: ~PrizeBase expands it (the vptr store only)
     * and nothing calls 0x0022A340, the copy egcs emits for the vtable. */
    virtual ~PrizeModelSetContext() {}          /* [0] +0x0C 0x0022A340 */
    /* Multiply the current matrix by root / body (through a prim buffer
     * copy).  Defined in T459. */
    void applyRoot();                           /* 0x002296E0 */
    void applyBody();                           /* 0x00229738 */
    void copy(const PrizeModelSetContext &src); /* 0x00229798 (T459) */

    PrizeMatrix root;                           /* 0x00 (? name) drawn first, also under the shadows */
    PrizeMatrix body;                           /* 0x40 (? name) after the shadow projection */
    f32         time;                           /* 0x80 the current key (fractional) */
    /*          vptr                               0x84 */
};
EGCS_SIZE(PrizeModelSetContext, 0x88);

#endif /* GAME_T459_PRIZE_MODEL_H */
