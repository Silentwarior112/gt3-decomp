/* T183 - race_draw_vignette (? name: the ring looks like a vignette).
 * 0x001AD818..0x001ADA68.
 *
 * A ring of 32 segments (a strip of 64 vertices, one pair per angle) between
 * the course environment's vignetteRadius0 and vignetteRadius1, colour
 * vignetteColor0 at the first radius and vignetteColor1 at the second,
 * drawn over the screen in an orthographic -320..320 x -224..224 view with
 * alpha blending, no culling, no depth test. */
/* Retail has no .rodata here (no literal, no type name): the file sees no
 * class with a type name. */
#include "game/effects/T183_race_vignette.h"
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, vu0_load_identity_regs, pack_rgba_floats_to_u32 */
#include "engine/media/G002_mpeg_frameq.h"          /* sinf, cosf */
#include "game/course/T178_course_env.h"           /* CourseEnv (the vignette) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/effects/replay_effects_internal.h"                           /* BLEND_ALPHA */

enum { VIGNETTE_SEGMENTS = 32 };

/* Caller: RacePS2Base::drawView (replays, course flag 0x40). */
// FUNC 0x001AD818 race_draw_vignette
void race_draw_vignette(GSBuffer *buf, CourseEnv *env)
{
    buf->setDrawBuffer();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    pgl_ortho(-320.0f, 320.0f, -224.0f, 224.0f, -1.0f, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();

    f32 radius0 = env->vignetteRadius0;
    f32 radius1 = env->vignetteRadius1;
    u32 color0 = pack_rgba_floats_to_u32(env->vignetteColor0[0], env->vignetteColor0[1],
                                         env->vignetteColor0[2], env->vignetteColor0[3], 255.0f);
    u32 color1 = pack_rgba_floats_to_u32(env->vignetteColor1[0], env->vignetteColor1[1],
                                         env->vignetteColor1[2], env->vignetteColor1[3], 255.0f);
    u32 *colors = (u32 *)gfx_alloc_tail(0x10); /* 32 x 2 colours */
    f32 *xyz = (f32 *)gfx_alloc_tail(0x30);    /* 32 x 2 vertices (x, y, z) */

    u32 *col = colors;
    f32 *v = xyz;
    for (int i = 0; i < VIGNETTE_SEGMENTS; i++) {
        /* the last pair closes the ring at angle 0 exactly */
        f32 angle = (i != VIGNETTE_SEGMENTS - 1) ? (f32)i * 6.2831855f : 0.0f;
        angle = angle / 31.0f;
        f32 s = sinf(angle);
        f32 c = cosf(angle);
        *col++ = color0;                /* the inner vertex, */
        *col++ = color1;                /* the outer one */
        *v++ = c * radius0;             /* x */
        v[2] = c * radius1;
        *v++ = s * radius0;             /* y */
        v[2] = s * radius1;
        *v = 0.0f;                      /* z */
        v += 3;
        *v++ = 0.0f;
    }
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_FOG);
    gfx_blend_func(BLEND_ALPHA, 1.0f);
    pgl_draw_strip(xyz, NULL, colors, NULL, VIGNETTE_SEGMENTS * 2);
    gfx_enable(PGL_CAP_CULL_FACE);
}
