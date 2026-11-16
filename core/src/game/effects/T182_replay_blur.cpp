/* T182 - ReplayBlur: the depth of field of the replays.  0x001AC870..0x001AD818.
 *
 * Thin-lens model: with focal length f, focus distance s, f-number N and a
 * circle of confusion c (film metres), the image is c-blurred at
 *     far  = s f^2 / (f^2 - N c (s - f))   (behind the focus)
 *     near = s f^2 / (f^2 + N c (s - f))   (in front of it).
 * c runs over 2^(k / steps) pixels (expf(k ln2 / steps)) times the film
 * height of one pixel.  For each k, level k / steps of the downsampled chain
 * is drawn over the screen at the depth of that distance, depth-tested
 * against the frame's depth buffer, with the alpha of the fraction of the
 * octave.
 *
 * The far side is drawn with float depths (pgl_set_sprite_depth): a level
 * covers what lies beyond its distance.  For the near side drawZ inverts the
 * depth buffer (invertDepth: Z' = 0xFFFFFF - Z), so that the same depth test
 * covers what lies in front of the distance, and draws with GS Z values it
 * computes itself (gs_set_sprite_zf of 0xFFFFFF - Z), then inverts it back. */
/* Retail's .rodata is the level table alone (0x00330BD0..0x00330C20): no
 * literal, no type name (the "bad_alloc" and UnitArena<64>'s name in front
 * of it are T181's), so the file sees no class with a type name. */
#include "game/effects/T182_replay_blur.h"
#include "libgraph.h"                       /* SCE_GS_PSMCT24 / PSMZ24 */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, gs_*, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* expf, logf */
#include "game/race/core/T131_race_ps2_render.h"      /* Matrix44 */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230 (RenderContext, GSBufferPair), g_002F4D68 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/effects/replay_effects_internal.h"                           /* BlurRect, g_00330BD0, u061_rsqrtf, BLEND_* */

/* The levels of the chain (.rodata 0x00330BD0): each half the previous one
 * in one direction, alternately, inside ReplayBlur::levels (640 x 224). */
const BlurRect g_00330BD0[BLUR_LEVELS] = {  /* 0x00330BD0 */
    {   0,   0, 320, 224 },
    { 320,   0, 320, 112 },
    { 320, 112, 160, 112 },
    { 480, 112, 160,  56 },
    { 480, 168, 160,  56 },
};

/* Callers: RacePS2Base's constructor (0x0017D070).  levels is constructed
 * first; ~GSBuffer(levels, 0) on the exception path. */
// FUNC 0x001AC870 ReplayBlur::ReplayBlur
ReplayBlur::ReplayBlur()
    : levels()                              /* GSBuffer::GSBuffer */ /* implicit: GSBuffer (member constructors), ~GSBuffer (exception cleanup) */
{
    setFilm(35.0f, 1.333f);
    fNumber = 2.0f;
    focalLength = 0.035f;
    focus = 10.0f;
    farMax = 8.0f;
    nearMax = 4.0f;
    farSteps = 2;
    nearSteps = 1;
    zFar = 0.1f;
    zNear = 1000.0f;
}

/* The film: its diagonal (mm) and aspect give its height in metres.  The
 * original is one rsqrt.s (fs / sqrt(ft)).  Callers: the constructor and
 * RacePS2Base::drawView (env +0x934, 1.333). */
// FUNC 0x001AC948 ReplayBlur::setFilm
void ReplayBlur::setFilm(f32 diagMm, f32 aspect)
{
    filmDiag = diagMm;
    filmHeight = u061_rsqrtf(diagMm * 0.001f, aspect * aspect + 1.0f);
}

/* The clip distances and the focal length from the projection matrix: for
 * a GL-style projection m[3][2] / (m[2][2] + 1) is the far clip and
 * m[3][2] / (m[2][2] - 1) the near one; m[1][1] is 2 f / film height.
 * Caller: RacePS2Base::drawView. */
// FUNC 0x001AC970 ReplayBlur::setFromProjection
void ReplayBlur::setFromProjection()
{
    Matrix44 proj;                          /* sp+0x00 */

    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_get_matrix(&proj.m[0][0]);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    zFar = proj.m[3][2] / (proj.m[2][2] + 1.0f);
    zNear = proj.m[3][2] / (proj.m[2][2] - 1.0f);
    focalLength = filmHeight * 0.5f * proj.m[1][1];
}

/* Both sides, far first: the far side with float depths, then the depth
 * inverted and the near side with GS Z values: for a distance z the NDC
 * depth is (A z + B) / z with A = (far + near) / (far - near), B = -2 far
 * near / (far - near), the GS Z (1 - ndc) 2^23, inverted: 0xFFFFFF - it.
 * The levels go into `work'.  Caller: drawFrameDepth (itself uncalled). */
// FUNC 0x001ACA00 ReplayBlur::drawZ
void ReplayBlur::drawZ(GSBuffer *dst, GSBuffer *src, GSBuffer *depth, GSBuffer *work)
{
    int k;

    levels.copy(work);
    buildLevels(src);
    dst->setDrawBuffer();
    depth->setZBuffer();
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_depth_mask(0);
    gfx_disable(PGL_CAP_FOG);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_ALPHA, 1.0f);

    f32 pixel = filmHeight / (f32)src->height;          /* f23: the film height of one pixel */
    f32 ndcA = (zFar + zNear) / (zFar - zNear);         /* f25 */
    f32 ndcB = (zFar * zNear * -2.0f) / (zFar - zNear); /* f24 */
    f32 lensDist = focus - focalLength;                 /* f22 */
    f32 f2 = focalLength * focalLength;                 /* f21 */

    /* behind the focus */
    k = 0;
    for (;;) {
        f32 coc = expf(((f32)k * BLUR_LN2) / (f32)farSteps);
        if (farMax < coc)
            break;
        int steps = farSteps;
        int next = k + 1;
        int frac = next % steps;
        f32 alpha;
        if (frac == 0)
            alpha = 1.0f;
        else
            alpha = (f32)frac / (f32)steps;
        f32 dist = (f2 * focus) / (f2 - lensDist * coc * pixel * fNumber);
        if (zFar < dist || dist < 0.0f)
            break;
        pgl_set_sprite_depth(dist);
        drawLevel(k / farSteps, alpha);
        k = next;
    }
    invertDepth(depth);                      /* the depth inverted */

    dst->setDrawBuffer();
    depth->setZBuffer();
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_ALPHA, 1.0f);

    /* in front of it */
    k = 0;
    for (;;) {
        f32 coc = expf(((f32)k * BLUR_LN2) / (f32)nearSteps);
        if (nearMax < coc)
            break;
        int steps = nearSteps;
        int next = k + 1;
        int frac = next % steps;
        f32 alpha;
        if (frac == 0)
            alpha = 1.0f;
        else
            alpha = (f32)frac / (f32)steps;
        f32 dist = (f2 * focus) / (f2 + lensDist * coc * pixel * fNumber);
        if (dist < zNear)
            break;
        gs_set_sprite_zf(0xFFFFFF - (int)((1.0f - (ndcA * dist + ndcB) / dist) * 8388608.0f), 0xFF);
        drawLevel(k / nearSteps, alpha);
        k = next;
    }
    invertDepth(depth);                      /* and back */
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_BLEND);
}

/* No callers: the depth from the current frame copy (copied into a PSMZ24
 * view of g_002F4230.bufB38), src drawn over dst (blitField), and the
 * levels in the frame copy's VRAM (+0x23000 when that is dst's), 320 x 224. */
// FUNC 0x001ACE00 ReplayBlur::drawFrameDepth
void ReplayBlur::drawFrameDepth(GSBuffer *dst, GSBuffer *src, int mode, f32 alpha)
{
    GSBuffer depth(&g_002F4D68, SCE_GS_PSMZ24);         /* sp+0x00 */

    copyFrameTo(&depth);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    dst->blitField(src, mode, alpha);
    GSBufferPair *pair = &g_002F4230.unkAC4;
    int addr = pair->buf[pair->cur].addr;
    if (addr == dst->addr)
        addr += 0x23000;
    GSBuffer *lv = &levels;
    lv->setAddr(addr);
    lv->width = 320;
    lv->height = 224;
    drawZ(dst, src, &depth, lv);
}

/* The one in use (RacePS2Base::drawView, replays with course flag 0x100):
 * the near side first, from the most blurred level (nearSteps *
 * log2(nearMax + 1)) down to 0, each drawn at the depth of the level drawn
 * before (0 for the first; its own depth is kept for the next); then src
 * over dst (blitField: mode, alpha) at the depth of the last one, then the
 * far side as in drawZ. */
// FUNC 0x001ACF38 ReplayBlur::draw
void ReplayBlur::draw(GSBuffer *dst, GSBuffer *src, GSBuffer *depth, GSBuffer *work,
                      int mode, f32 alpha)
{
    int k;
    f32 lastDist;

    levels.copy(work);
    buildLevels(src);
    lastDist = 0.0f;
    dst->setDrawBuffer();
    depth->setZBuffer();
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_depth_mask(0);
    gfx_disable(PGL_CAP_FOG);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_ALPHA, 1.0f);

    f32 f2 = focalLength * focalLength;                 /* f22 */
    f32 pixel = filmHeight / (f32)src->height;          /* f24 */
    f32 lensDist = focus - focalLength;                 /* f23 */

    /* in front of the focus */
    for (k = nearSteps * (int)(logf(nearMax + 1.0f) / BLUR_LN2); k >= 0; k--) {
        f32 coc = expf(((f32)k * BLUR_LN2) / (f32)nearSteps);
        int steps = nearSteps;
        f32 levelAlpha = (f32)(steps - k % steps) / (f32)steps;
        f32 dist = (f2 * focus) / (f2 + lensDist * coc * pixel * fNumber);
        if (!(dist < zNear)) {
            pgl_set_sprite_depth(lastDist);
            lastDist = dist;
            drawLevel(k / nearSteps, levelAlpha);
        }
    }
    gfx_disable(PGL_CAP_BLEND);
    pgl_set_sprite_depth(lastDist);

    dst->blitField(src, mode, alpha);
    dst->setDrawBuffer();
    depth->setZBuffer();
    gfx_depth_mask(0);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_ALPHA, 1.0f);

    /* behind it */
    k = 0;
    for (;;) {
        f32 coc = expf(((f32)k * BLUR_LN2) / (f32)farSteps);
        if (farMax < coc)
            break;
        int steps = farSteps;
        int next = k + 1;
        int frac = next % steps;
        f32 levelAlpha;
        if (frac == 0)
            levelAlpha = 1.0f;
        else
            levelAlpha = (f32)frac / (f32)steps;
        f32 dist = (f2 * focus) / (f2 - lensDist * coc * pixel * fNumber);
        if (zFar < dist || dist < 0.0f)
            break;
        pgl_set_sprite_depth(dist);
        drawLevel(k / farSteps, levelAlpha);
        k = next;
    }
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_BLEND);
}

/* The current frame copy (g_002F4230.unkAC4) drawn 640 x 448 into dst,
 * nearest filtering, no test, no blending.  Caller: drawFrameDepth. */
// FUNC 0x001AD2F0 ReplayBlur::copyFrameTo
void ReplayBlur::copyFrameTo(GSBuffer *dst)
{
    GSBufferPair *pair = &g_002F4230.unkAC4;
    pair->buf[pair->cur].setTexture();
    dst->setDrawBuffer();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    pgl_set_sprite_depth(0.0f);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gs_draw_textured_sprite(0, 0, SCREEN_W, SCREEN_H, 0, 0, SCREEN_W, SCREEN_H);
}

/* Inverts the depth buffer: seen as a PSMCT24 colour buffer, a white
 * 640 x 448 rectangle is drawn into it with blend (Cs - Cd) * FIX (FIX 1.0:
 * 128), which gives 0xFF - Cd per byte, i.e. Z' = 0xFFFFFF - Z; then psm
 * back to PSMZ24.  drawZ calls it around its near side.  (u061 invertDepth.) */
// FUNC 0x001AD3B0 ReplayBlur::invertDepth
void ReplayBlur::invertDepth(GSBuffer *depth)
{
    depth->psm = SCE_GS_PSMCT24;
    depth->setDrawBuffer();
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_SUBTRACT, 1.0f);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gs_draw_flat_sprite_strips(0, 0, SCREEN_W, SCREEN_H);
    depth->psm = SCE_GS_PSMZ24;
}

/* The chain: src (its width rounded up to 64) into level 0, then each level
 * from the previous one (from level 1 on the source is levels itself),
 * bilinear, clamped to the source rectangle.
 *
 * Matching (u061): 93.9%.  The rectangle read field by field from
 * g_00330BD0[level] in the order x, y, w, h (loop.c reduces x / h to a
 * walking pointer, w / y keep base + level * 16, as retail; all 24 orders
 * tried).  Left: retail copies dstW / dstH into temporaries ($v0 / $v1)
 * before u0 + w / v0 + h at the loop top, and its walking pointer is $s5,
 * dstW $s4 (ours swapped).  Tried: w / h locals (block-scoped or not),
 * an inline adder, a rectangle struct, for / while loops, v before u; the
 * reference compiler 991111-01 gives the same 93.9%. */
// FUNC 0x001AD440 ReplayBlur::buildLevels
void ReplayBlur::buildLevels(GSBuffer *src)
{
    GSBuffer *chain = &levels;                          /* sp+0x00 */
    int dstX, dstY, dstW, dstH, level;

    chain->psm = SCE_GS_PSMCT24;
    chain->setField(PGL_FIELD_FRAME);
    chain->scanMask = PGL_SCANMASK_OFF;
    dstX = 0;                                           /* sp+0x04 */
    dstW = (src->width + 0x3F) & ~0x3F;
    dstH = src->height;
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    chain->setDrawBuffer();
    src->setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    level = 0;
    dstY = 0;
    do {
        /* the source: the rectangle drawn last (the whole src at first) */
        int u0 = dstX, v0 = dstY;
        int u1 = u0 + dstW, v1 = v0 + dstH;
        if (level == 1)
            chain->setTexture();
        dstX = g_00330BD0[level].x;
        dstY = g_00330BD0[level].y;
        dstW = g_00330BD0[level].w;
        dstH = g_00330BD0[level].h;
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
        level++;
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
        gfx_set_texture_region(PGL_REGION_U, u0, u1 - 1);
        gfx_set_texture_region(PGL_REGION_V, v0, v1 - 1);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
        gs_draw_textured_sprite_strips_f((f32)dstX, (f32)dstY, (f32)(dstX + dstW), (f32)(dstY + dstH),
                                         (f32)u0 + 0.5f, (f32)v0 + 0.5f, (f32)u1 + 0.5f, (f32)v1 + 0.5f);
        gfx_texflush();
    } while (level < BLUR_LEVELS);
}

/* Level `level' stretched over the screen in 10 strips of 64 x 448
 * (alpha 1: blending off, else on with that alpha).  Callers: drawZ, draw. */
// FUNC 0x001AD650 ReplayBlur::drawLevel
void ReplayBlur::drawLevel(int level, f32 alpha)
{
    if (level < BLUR_LEVELS) {
        levels.setTexture();
        int x = g_00330BD0[level].x;
        int y = g_00330BD0[level].y;
        int w = g_00330BD0[level].w;
        int h = g_00330BD0[level].h;
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
        int yEnd = y + h;
        gfx_set_texture_region(PGL_REGION_U, x, x + w - 1);
        gfx_set_texture_region(PGL_REGION_V, y, yEnd - 1);
        if (alpha == 1.0f) {
            gfx_disable(PGL_CAP_BLEND);
        } else {
            gfx_enable(PGL_CAP_BLEND);
            pgl_color4f(1.0f, 1.0f, 1.0f, alpha);
        }
        f32 stripU = (f32)w / 10.0f;
        f32 u0 = (f32)x - 0.5f;
        f32 x0 = 0.0f;
        f32 x1 = 64.0f;
        f32 u1 = u0 + stripU;
        int left = 9;
        do {
            left--;
            gs_draw_textured_sprite_f(x0, 0.0f, x1, (f32)SCREEN_H, u0, (f32)y - 0.5f, u1, (f32)yEnd - 0.5f);
            x0 = x1;
            u0 = u1;
            x1 += 64.0f;
            u1 += stripU;
        } while (left >= 0);
    }
}
