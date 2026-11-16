/* T459 - PrizeModel (? name): the .prm trophy model file and its pose.
 *
 * The file has two series of key matrices (one per frame), applied as
 * PrizeModelSetContext::root then ::body around the embedded T451 model,
 * lights (up to 8 set) and per light a planar shadow: the model drawn
 * flattened onto a horizontal plane along the light's direction.  The
 * animation runs at speed keys per 1/60 s from startFrame to endFrame + 1,
 * then goes on from loopFrame; between two keys the matrices are
 * interpolated (T461's matrix_interpolate).
 */
/* .rodata (0x003413C0..0x00341407): g_003413C0, g_003413D0 (below), then
 * the type name "20PrizeModelSetContext" (T459_prize_model.h; retail's T459
 * did not see CallBackWhenPrizeRendering, so not T460_prize_base.h). */
#include "game/menu/gtmode/prize_model_internal.h"
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, PGL_* */
#include "engine/gfx/T451_model_set.h"            /* modelset_relocate */
#include "game/menu/gtmode/T459_prize_model.h"          /* own; PrizeModelSetContext */
#include "game/common/T461_matrix_interpolate.h"   /* matrix_interpolate */

/* prize_model_step: how many times one frame may wrap past the end. */
#define PRIZE_MAX_WRAPS 21

// FUNC 0x00229520 prize_matrix_copy
PrizeMatrix *prize_matrix_copy(PrizeMatrix *dst, const PrizeMatrix *src)
{
    const f32 *from = src->m;
    f32 *to = dst->m;
    int left;

    for (left = 15; left >= 0; left--)
        *to++ = *from++;
    return dst;
}

// FUNC 0x00229558 prize_light_set
//   Sets the four parameters of light `light' (PGL_CAP_LIGHT0 + i) and
//   enables it.
void prize_light_set(const PrizeLight *l, int light)
{
    pgl_lightfv(light, PGL_LIGHT_POSITION, l->dir);
    pgl_lightfv(light, PGL_LIGHT_DIFFUSE, l->p1);
    pgl_lightfv(light, PGL_LIGHT_AMBIENT, l->p0);
    pgl_lightfv(light, PGL_LIGHT_SPECULAR, l->p4);
    gfx_enable(light);
}

// FUNC 0x002295D0 prize_shadow_matrix
//   Multiplies the current matrix by the shadow projection of the light
//   (row vectors: x' = x + y * kx, y' = the plane height, z' = z + y * kz)
//   with k = -dir.xz / dir.y; k = 0 (straight down) when mode != 0 or
//   |dir.y| < 0.0001.  0 (nothing done, but the prim buffer space is taken)
//   when dir.y is 0.
int prize_shadow_matrix(const PrizeLight *l, int mode, f32 y)
{
    f32 *m = (f32 *)gfx_alloc_tail(4);          /* one quadword-aligned 4x4 */
    f32 dirY = l->dir[1];
    f32 kx, kz;

    if (dirY == 0.0f)
        return 0;
    if (mode != 0 || (dirY < 0.0001f && -0.0001f < dirY)) {
        kx = 0.0f;
        kz = 0.0f;
    } else {
        kx = -l->dir[0] / dirY;
        kz = -l->dir[2] / dirY;
    }
    m[4] = kx;
    m[13] = y;
    m[6] = kz;
    m[0] = 1.0f;
    m[8] = 0.0f;
    m[12] = 0.0f;
    m[1] = 0.0f;
    m[5] = 0.0f;
    m[9] = 0.0f;
    m[2] = 0.0f;
    m[10] = 1.0f;
    m[14] = 0.0f;
    m[3] = 0.0f;
    m[7] = 0.0f;
    m[11] = 0.0f;
    m[15] = 1.0f;
    pgl_mult_matrix(m);
    return 1;
}

// FUNC 0x002296E0 PrizeModelSetContext::applyRoot
//   Multiplies the current matrix by root (through a prim buffer copy).
void PrizeModelSetContext::applyRoot()
{
    f32 *m = (f32 *)gfx_alloc_tail(4);
    const f32 *from = root.m;
    f32 *to = m;
    int left;

    for (left = 15; left >= 0; left--)
        *to++ = *from++;
    pgl_mult_matrix(m);
}

// FUNC 0x00229738 PrizeModelSetContext::applyBody
void PrizeModelSetContext::applyBody()
{
    f32 *m = (f32 *)gfx_alloc_tail(4);
    const f32 *from = body.m;
    f32 *to = m;
    int left;

    for (left = 15; left >= 0; left--)
        *to++ = *from++;
    pgl_mult_matrix(m);
}

// FUNC 0x00229798 PrizeModelSetContext::copy
//   (no caller)
void PrizeModelSetContext::copy(const PrizeModelSetContext &src)
{
    time = src.time;
    prize_matrix_copy(&root, &src.root);
    prize_matrix_copy(&body, &src.body);
}

// FUNC 0x002297E0 prize_matrix_blend
//   out = wa * a + wb * b, element by element (matrix_interpolate's
//   fallback).
void prize_matrix_blend(PrizeMatrix *out, const PrizeMatrix *a, const PrizeMatrix *b,
                        f32 wa, f32 wb)
{
    const f32 *pa = a->m;
    const f32 *pb = b->m;
    f32 *to = out->m;
    int left;

    for (left = 15; left >= 0; left--)
        *to++ = wa * *pa++ + wb * *pb++;
}

// FUNC 0x00229820 prize_model_relocate
//   Relocates a loaded file to its address (nothing when it is there) and
//   its embedded model; the speed becomes keys per 1/60 s.
void prize_model_relocate(PrizeModel *m)
{
    int delta = (u8 *)m - (u8 *)m->self;

    if (delta == 0)
        return;
    m->self = m;
    if (m->model != NULL) {
        u103_reloc(m->model, delta);
        modelset_relocate(m->model);
    }
    if (m->rootKeys != NULL)
        u103_reloc(m->rootKeys, delta);
    if (m->bodyKeys != NULL)
        u103_reloc(m->bodyKeys, delta);
    if (m->lights != NULL)
        u103_reloc(m->lights, delta);
    m->speed = m->speed / 60.0f;
}

// FUNC 0x002298C0 prize_model_step
//   One frame: time += speed; past endFrame + 1 it goes on from loopFrame
//   (the overshoot kept; after PRIZE_MAX_WRAPS wraps it stays at
//   endFrame).  Then the pose.  (Also called by MiniPrize::stepAnimation,
//   T145's copy.)
void prize_model_step(PrizeModel *m, PrizeModelSetContext *ctx)
{
    int wraps = 0;
    f32 end;

    ctx->time = ctx->time + m->speed;
    end = m->endFrame;
    while (end + 1.0f <= ctx->time) {
        f32 over = ctx->time - (end + 1.0f);
        if (over >= 0.0) {                      /* compared as double (__cmpdf2) */  /* implicit: fptodp, dpcmp */
            ctx->time = m->loopFrame + over;
            end = m->endFrame;
        }
        wraps++;
        if (!(wraps < PRIZE_MAX_WRAPS)) {
            ctx->time = end;
            break;
        }
    }
    prize_model_pose(m, ctx, matrix_interpolate);
}

// FUNC 0x002299A0 prize_model_loop
//   time = loopFrame and the pose (callers: T145's
//   MiniPrize::restartAnimation / resetAnimation).
void prize_model_loop(PrizeModel *m, PrizeModelSetContext *ctx)
{
    ctx->time = m->loopFrame;
    prize_model_pose(m, ctx, matrix_interpolate);
}

// FUNC 0x002299D0 prize_model_pose
//   The matrices at ctx->time: key = (int)time and the next one (loopFrame
//   after endFrame), lerp(out, keys[key], keys[next], key + 1 - time,
//   time - key); key 0 only when the flags say the series is fixed.
//   Matching (98.7%): in the body lerp call retail sets $f12 before the
//   last argument address (a scheduling tie; the root call matches).
void prize_model_pose(PrizeModel *m, PrizeModelSetContext *ctx, PrizeMatrixLerp lerp)
{
    f32 time = ctx->time;
    int key = (int)time;
    int lastKey = (int)m->endFrame;
    int following = key + 1;
    f32 wNext = time - (f32)key;
    f32 wKey = (f32)following - time;
    int next;

    if (key == lastKey)
        next = (int)m->loopFrame;
    else
        next = following;
    if (m->rootKeys != NULL) {
        if (m->flags & 1)                       /* root: key 0 only */
            prize_matrix_copy(&ctx->root, m->rootKeys);
        else
            lerp(&ctx->root, &m->rootKeys[key], &m->rootKeys[next], wKey, wNext);
    }
    if (m->bodyKeys != NULL) {
        if (m->flags & 2)                       /* body: key 0 only */
            prize_matrix_copy(&ctx->body, m->bodyKeys);
        else
            lerp(&ctx->body, &m->bodyKeys[key], &m->bodyKeys[next], wKey, wNext);
    }
}

// FUNC 0x00229B10 prize_model_reset
//   time = startFrame, both matrices identity.
void prize_model_reset(PrizeModel *m, PrizeModelSetContext *ctx)
{
    ctx->time = m->startFrame;
    prize_matrix_set_diagonal(&ctx->root, 1.0f);
    prize_matrix_set_diagonal(&ctx->body, 1.0f);
}

// FUNC 0x00229BA8 prize_model_shadow
//   The shadow matrix of light i (0 when there is no such light).
int prize_model_shadow(PrizeModel *m, int i, int mode, f32 y)
{
    if (i >= m->lightCount)
        return 0;
    return prize_shadow_matrix(&m->lights[i], mode, y);
}

/* The light model's ambient colour (black) and the gfx ids of the lights
 * 0..7, PGL_CAP_LIGHT0 + 0..7 (external linkage: prize_model_internal.h's externs). */
const f32 g_003413C0[4] = { 0.0f, 0.0f, 0.0f, 1.0f };                    /* 0x003413C0 */
const int g_003413D0[8] = { 101, 102, 103, 104, 105, 106, 107, 108 };     /* 0x003413D0 */

// FUNC 0x00229BE8 prize_model_lights
//   The light model (global ambient black), then lights 101..108
//   (PGL_CAP_LIGHT0 + 0..7): the model's lights, the others disabled.
void prize_model_lights(PrizeModel *m)
{
    const int *lightId;
    int i;

    gfx_init_lights_material_spr();
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, g_003413C0);
    lightId = g_003413D0;
    for (i = 0; i < 8; i++) {
        if (i < m->lightCount)
            prize_light_set(&m->lights[i], *lightId);
        else
            gfx_disable(*lightId);
        lightId++;
    }
}
