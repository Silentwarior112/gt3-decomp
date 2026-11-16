/* T181_car_lighting.cpp - CarRenderCtx, the lighting of one car for a frame
 * (0x001AB7D0..0x001AC870); the class is declared in
 * include/game/car/model/T181_car_lighting.h.
 *
 * setup() looks at up to nine runway lights around the light the car is at
 * (the raw u of the road triangle under wheel 0, RaceCarState
 * contact[0].u0, from four before it), weighs each (directional lights: 1;
 * the others by the attenuation of their CourseEnv light A and its spot
 * cone), sorts them by weight and keeps the four strongest as GL lights
 * 0x65..0x68 (apply) and as the car's shadow passes (applyShadowPass), and up to
 * six positional ones as glows that drawGlows renders into the car's
 * dynamic environment map.  A course without runway lights uses the
 * CourseEnv's lights A and car shadow, by the mean of the road's vertex
 * values u / v under the wheels (course_env_set_car_lights / course_env_car_body_light).  Callers:
 * RacePS2Base::drawCars / virtual_51 (u045), with a CarRenderCtx on the
 * stack.
 */
/* Retail T181's .rodata (0x00330BA8): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/model/race_car_model_internal.h"
#include "game/car/model/T181_car_lighting.h"         /* CarRenderCtx, CarLight, CarGlow */
#include "game/course/T178_course_env.h"           /* CourseEnv, CourseEnvLightA, course_env_set_car_lights, course_env_car_body_light */
#include "game/course/T179_course_data.h"          /* CourseData (runway, env) */
#include "game/car/model/T446_car_model.h"            /* car_shadow_set_dir / _color, g_00352AC4 (the shadow strength) */
#include "game/effects/T277_dynamic_envmap_camera.h" /* DynamicEnvmapCamera::applyView */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState and its car_* accessors */
#include "game/course/T457_runway.h"               /* Runway, RunwayLight */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer::setTargetB / setDrawBuffer */
#include "engine/pgl/G001_flare.h"                /* the pgl_* / gfx_* calls, strip_*, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* expf, powf */
// DATA_START 0x00330BA8

/* One runway light considered by setup (0x14 bytes, sp+0x00 x 9). */
struct CarLightCandidate {
    RunwayLight *light;                     /* 0x00 */
    f32 weight;                             /* 0x04 */
    f32 dir[3];                             /* 0x08 the unit vector to the light */
};

/* setup's vector stores: statement macros (their loop notes keep each
 * group's address arithmetic in it, as retail has; written out, sched1
 * hoists all of it and reload spills the addresses). */
#define CAR_V4_COPY(d, s)                                                      \
    do { (d)[0] = (s)[0]; (d)[1] = (s)[1]; (d)[2] = (s)[2]; (d)[3] = (s)[3]; } while (0)
#define CAR_V4_SCALE(d, s, k)                                                  \
    do {                                                                       \
        (d)[0] = (s)[0] * (k); (d)[1] = (s)[1] * (k);                          \
        (d)[2] = (s)[2] * (k); (d)[3] = (s)[3] * (k);                          \
    } while (0)
#define CAR_V4_SET(d, a, b, c, e)                                              \
    do { (d)[0] = (a); (d)[1] = (b); (d)[2] = (c); (d)[3] = (e); } while (0)

// FUNC 0x001AB7D0 CarRenderCtx::setup
//   See the file comment.  The runway lights from lightIndex - 4 (wrapped)
//   on, at most 9.  A light's weight: 1 / (att[0] + att[1] d + att[2] d^2)
//   (d: its distance from the car's view position at t), and for a spot
//   (cutoff != 180) times powf(c, exponent) with c = max(dot(the unit
//   vector to the light, light->dir), 0), or 0 when c is below
//   pgl_cos_deg(cutoff).  The sort is a bubble sort, descending.
//   Directional lights keep their CourseEnv light's colours (their pos is
//   the direction), the others are scaled by their weight (the shadow
//   colour's rgb is not, its alpha is).  A glow's intensity is
//   sqrt(weight).  Without runway lights: one pass, and the means of
//   car_contact_u / _v (the road's u / v) at fraction 0.
//   Matching (99.9%): the spot's dot product: retail puts the first partial
//   sum in $f3 and the result in $f12; as one expression ours loads the
//   three directions into other registers (99.5%), so it is written as two
//   statements (the partial sum then lands in $f12).  Tried: 12 shapes of
//   the dot (temporaries, association, an inline dot3, operand orders).
void CarRenderCtx::setup(CourseData *cd, RaceCarState *s, f32 t)
{
    CourseEnv *courseEnv = cd->env;
    Runway *runway = cd->runway;
    env = courseEnv;
    if (runway->hasLights != 0) {
        CarLightCandidate cand[CAR_LIGHT_CANDIDATES];   /* sp+0x00 */
        int n = 0;                                      /* sp+0xE0 */
        f32 carX = car_view_x(s, t);
        f32 carY = car_view_y(s, t);
        f32 carZ = car_view_z(s, t);
        int total = runway->lightCount;
        /* lh: the u16 is read signed */
        int index = (s16)s->contact[0].u0;
        index -= CAR_LIGHT_LOOK_BACK;
        if (!(index > -1))
            index += total;
        /* i: one counter for all four loops (retail: s5 throughout) */
        int i = (total < CAR_LIGHT_CANDIDATES + 1) ? total : CAR_LIGHT_CANDIDATES;
        while (i > 0) {
            RunwayLight *light = &runway->lights[index];
            int type = light->type;
            cand[n].light = light;
            if (light->radius == 0.0f) {
                cand[n].weight = 1.0f;
            } else {
                f32 dx = light->pos[0] - carX;
                f32 dy = light->pos[1] - carY;
                f32 dz = light->pos[2] - carZ;
                f32 dist2 = dx * dx + dy * dy + dz * dz;
                f32 invDist = ee_rsqrt(1.0f, dist2);
                f32 weight = 1.0f / (courseEnv->lightA[type].att[0]
                                     + courseEnv->lightA[type].att[1] * (dist2 * invDist)
                                     + courseEnv->lightA[type].att[2] * dist2);
                dx *= invDist;
                dy *= invDist;
                dz *= invDist;
                if (courseEnv->lightA[type].cutoff != SPOT_NO_CONE) {
                    f32 cosCutoff = pgl_cos_deg(courseEnv->lightA[type].cutoff);
                    f32 dot = dx * light->dir[0] + dy * light->dir[1];
                    dot += dz * light->dir[2];
                    /* (-ffast-math: max.s) */
                    dot = (dot > 0.0f) ? dot : 0.0f;
                    if (cosCutoff <= dot)
                        weight *= powf(dot, courseEnv->lightA[type].exponent);
                    else
                        weight = 0.0f;
                }
                cand[n].weight = weight;
                cand[n].dir[0] = dx;
                cand[n].dir[1] = dy;
                cand[n].dir[2] = dz;
            }
            i--;
            index++;
            if (!(index < total))
                index -= total;
            n++;
        }

        /* bubble sort by weight, the strongest first */
        for (i = n - 1; i > 0; i--) {
            for (int j = 0; j < i; j++) {
                if (cand[j].weight < cand[j + 1].weight) {
                    do {                    /* (a statement macro: retail aligns the label after it) */
                        CarLightCandidate swap = cand[j];   /* sp+0xC0 */
                        cand[j] = cand[j + 1];
                        cand[j + 1] = swap;
                    } while (0);
                }
            }
        }

        /* the loop reads passes back (loop.c keeps it in a hoisted register:
         * retail's last spill slot) */
        passes = (n < CAR_LIGHT_MAX + 1) ? n : CAR_LIGHT_MAX;
        for (i = 0; i < passes; i++) {
            RunwayLight *light = cand[i].light;
            int type = light->type;
            if (light->radius == 0.0f) {
                lights[i].dir[0] = light->pos[0];
                lights[i].dir[1] = light->pos[1];
                lights[i].dir[2] = light->pos[2];
                lights[i].dir[3] = 0.0f;
                CAR_V4_COPY(lights[i].ambient, courseEnv->lightA[type].ambient);
                CAR_V4_COPY(lights[i].diffuse, courseEnv->lightA[type].diffuse);
                CAR_V4_COPY(lights[i].specular, courseEnv->lightA[type].specular);
                CAR_V4_COPY(lights[i].shadowColor, courseEnv->lightA[type].color);
            } else {
                f32 x = cand[i].dir[0];
                f32 y = cand[i].dir[1];
                f32 z = cand[i].dir[2];
                f32 weight = cand[i].weight;
                CAR_V4_SET(lights[i].dir, x, y, z, 0.0f);
                CAR_V4_SCALE(lights[i].ambient, courseEnv->lightA[type].ambient, weight);
                CAR_V4_SCALE(lights[i].diffuse, courseEnv->lightA[type].diffuse, weight);
                CAR_V4_SCALE(lights[i].specular, courseEnv->lightA[type].specular, weight);
                CAR_V4_SET(lights[i].shadowColor, courseEnv->lightA[type].color[0], courseEnv->lightA[type].color[1],
                           courseEnv->lightA[type].color[2], courseEnv->lightA[type].color[3] * weight);
            }
        }

        glowCount = 0;
        for (i = 0; i < n && glowCount < CAR_GLOW_MAX; i++) {
            RunwayLight *light = cand[i].light;
            int type = light->type;
            if (light->radius != 0.0f) {
                f32 intensity = boot_sqrtf_v(cand[i].weight);  /* sqrt.s before the glow's loads */
                glows[glowCount].intensity = intensity;
                glows[glowCount].size = courseEnv->lightA[type].glowSize;
                glows[glowCount].pos[0] = light->pos[0];
                glows[glowCount].pos[1] = light->pos[1];
                glows[glowCount].pos[2] = light->pos[2];
                glows[glowCount].pos[3] = 1.0f;
                CAR_V4_COPY(glows[glowCount].color, courseEnv->lightA[type].glow);
                glowCount++;
            }
        }
        lit = 1;
        return;
    }

    /* no runway lights: the course's own, by the road's values under the
     * wheels */
    f32 sumU = 0.0f;
    passes = 1;
    lit = 0;
    f32 sumV = 0.0f;
    for (int wheel = 0; wheel < 4; wheel++) {
        sumV += car_contact_v(s, wheel, 0.0f);
        sumU += car_contact_u(s, wheel, 0.0f);
    }
    roadV = sumV * 0.25f;
    roadU = sumU * 0.25f;
}

// FUNC 0x001AC218 CarRenderCtx::apply
//   The CourseEnv's ambient colour (pgl_light_modelfv); with the runway
//   lights the GL lights PGL_CAP_LIGHT0.. (gfx_enable, then pgl_lightfv
//   POSITION, AMBIENT, DIFFUSE, SPECULAR) and the rest of the eight
//   disabled (gfx_disable), else the course's lights A (course_env_set_car_lights by the
//   road values).
void CarRenderCtx::apply()
{
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, env->ambient);
    if (lit != 0) {
        int unit = PGL_CAP_LIGHT0;
        int i = 0;
        for (; i < passes; i++, unit++) {
            gfx_enable(unit);
            pgl_lightfv(unit, PGL_LIGHT_POSITION, lights[i].dir);
            pgl_lightfv(unit, PGL_LIGHT_AMBIENT, lights[i].ambient);
            pgl_lightfv(unit, PGL_LIGHT_DIFFUSE, lights[i].diffuse);
            pgl_lightfv(unit, PGL_LIGHT_SPECULAR, lights[i].specular);
        }
        for (; i < CAR_LIGHT_UNITS; i++, unit++)
            gfx_disable(unit);
    } else {
        course_env_set_car_lights(env, roadU, roadV);
    }
}

// FUNC 0x001AC328 CarRenderCtx::applyShadowPass
//   Shadow pass i of the body (before RaceEntryCar::drawShadow, the
//   body's drawShadow): with the runway lights, the shadow direction
//   (car_shadow_set_dir: light i's direction x -env->dirScale) and colour
//   (car_shadow_set_color), the shadow strength g_00352AC4 = 1; else
//   applyDefault().
void CarRenderCtx::applyShadowPass(int i)
{
    if (lit != 0) {
        f32 k = -env->dirScale;
        car_shadow_set_dir(lights[i].dir[0] * k, lights[i].dir[1] * k, lights[i].dir[2] * k);
        car_shadow_set_color(lights[i].shadowColor[0], lights[i].shadowColor[1], lights[i].shadowColor[2], lights[i].shadowColor[3]);
        g_00352AC4 = 1.0f;
    } else {
        applyDefault();
    }
}

// FUNC 0x001AC3D0 CarRenderCtx::applyDefault
//   The CourseEnv's car shadow colour (car_shadow_set_color), the shadow
//   strength g_00352AC4 = course_env_car_body_light by the road values.
void CarRenderCtx::applyDefault()
{
    CourseEnv *e = env;
    car_shadow_set_color(e->carShadowColor[0], e->carShadowColor[1], e->carShadowColor[2],
                         e->carShadowAlpha);
    g_00352AC4 = course_env_car_body_light(env, roadU, roadV);
}

// FUNC 0x001AC420 car_glow_draw
//   A glow of the environment map: dir is the light's direction in the
//   camera's frame (xyz, unit) and its angular size (w).  Centre c =
//   dir.xy / |dir + (0, 0, 1)| (sphere map), a fan of 10 segments (a strip
//   of 21 vertices: rim, centre, rim, ...; gfx_alloc_tail, strip_draw) of
//   radius dir.w, the rim turned by 36 degrees each time from (0, dir.w);
//   colour (color.rgb, color.a * alpha) at the centre, alpha 0 at the rim
//   (pack_rgba_floats_to_u32, scale 255).
void car_glow_draw(const f32 *dir, const f32 *color, f32 alpha)
{
    f32 x = dir[0];
    f32 y = dir[1];
    f32 z = dir[2] + 1.0f;
    f32 inv = ee_rsqrt(1.0f, x * x + y * y + z * z);
    f32 centerX = x * inv;
    f32 centerY = y * inv;
    f32 rimX = 0.0f;
    u32 centerColor = pack_rgba_floats_to_u32(color[0], color[1], color[2], color[3] * alpha, 255.0f);
    u32 rimColor = pack_rgba_floats_to_u32(color[0], color[1], color[2], 0.0f, 255.0f);
    f32 *verts = (f32 *)gfx_alloc_tail(0x10);   /* 21 x (x, y, z) */
    u32 *colors = (u32 *)gfx_alloc_tail(6);     /* 21 colours */
    f32 rimY = dir[3];
    f32 sn = GLOW_SIN;                      /* (in locals: retail multiplies them as the */
    f32 cs = GLOW_COS;                      /* first operands; .lit4 has sin first) */
    f32 *v = verts;
    u32 *c = colors;
    for (int k = 0; k < GLOW_SEGMENTS; k++) {
        *v++ = centerX + rimX;
        *v++ = centerY + rimY;
        *v++ = 0.0f;
        *c++ = rimColor;
        *v++ = centerX;
        *v++ = centerY;
        *v++ = 0.0f;
        *c++ = centerColor;
        f32 nextX = cs * rimX - sn * rimY;
        f32 nextY = sn * rimX + cs * rimY;
        rimX = nextX;
        rimY = nextY;
    }
    verts[0x3E] = 0.0f;                     /* +0xF8 */
    colors[GLOW_SEGMENTS * 2] = rimColor;
    verts[0x3C] = centerX + rimX;           /* +0xF0 */
    verts[0x3D] = centerY + rimY;           /* +0xF4 */
    strip_draw(verts, NULL, colors, NULL, GLOW_SEGMENTS * 2 + 1);
}

// FUNC 0x001AC5B8 CarRenderCtx::drawGlows
//   Renders the glows into the car's environment map: saves the GS state
//   (pgl_push_attrib 0x8C7), an orthographic projection of +-0.9 (near -1,
//   far 1), the camera's view (DynamicEnvmapCamera::applyView) mirrored by
//   vu0_scale(-1, 1, -1); envBuf as target B (GSBuffer::setTargetB) is
//   copied into envTex, the draw buffer (setDrawBuffer; gfx_frame_effect
//   PGL_FX_COPY_B_TO_A, 1.0); then RGB writes only, no depth test, alpha
//   test, fog or texture, alpha blending ((Cs - Cd) As + Cd), no culling,
//   PGL_CAP_VU1_PROG_33E.  The glows' positions are transformed
//   (vu0_apply_matrix) and normalised (w = size / distance), then drawn with
//   car_glow_draw in one strip batch; the state is restored
//   (PGL_CAP_VU1_PROG_33E stays on).  The last argument (u045: the car is
//   the camera's target) is not used.  Caller: RacePS2Base::drawCars
//   (u045).
void CarRenderCtx::drawGlows(GSBuffer *envTex, GSBuffer *envBuf, DynamicEnvmapCamera *cam,
                             int unused)
{
    f32 view[CAR_GLOW_MAX][4];              /* sp+0x00 */
    int i;

    pgl_push_attrib(GLOW_ATTRIBS);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    vu0_load_identity_regs();
    pgl_ortho(-GLOW_ORTHO, GLOW_ORTHO, -GLOW_ORTHO, GLOW_ORTHO, -1.0f, 1.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    vu0_load_identity_regs();
    vu0_scale(-1.0f, 1.0f, -1.0f);
    cam->applyView();
    envBuf->setTargetB();
    envTex->setDrawBuffer();
    gfx_frame_effect(PGL_FX_COPY_B_TO_A, 1.0f);
    gs_flush_dirty_regs();
    gfx_set_frame_write_mask(FRAME_WRITE_RGB);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_FOG);
    gfx_disable(PGL_CAP_TEXTURE);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 0.0f);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    for (i = 0; i < glowCount; i++) {
        f32 *v = view[i];
        vu0_apply_matrix(v, glows[i].pos);
        f32 inv = ee_rsqrt(1.0f, v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
        view[i][3] = glows[i].size * inv;
    }
    vu0_load_identity_regs();
    strip_batch_begin();
    for (i = 0; i < glowCount; i++)
        car_glow_draw(view[i], glows[i].color, glows[i].intensity);
    strip_batch_end();
    gfx_set_frame_write_mask(FRAME_WRITE_RGBA);
    gfx_disable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_CULL_FACE);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    pgl_pop_attrib();
    gs_flush_dirty_regs();
    gfx_texflush();
}
