/* T181_car_lighting.h - CarRenderCtx, the lighting of one car for a frame
 * (translation unit T181, unit u060), 0x001AB7D0..0x001AC870.
 *
 * CarRenderCtx (u045's name): setup() weighs up to nine runway lights around
 * the light the car is at (the raw u of the road under wheel 0, RaceCarState
 * contact[0].u0, +0x14E8), keeps the four strongest as the GL lights
 * 0x65..0x68 of the body (apply) and as its shadow passes (applyShadowPass: the
 * shadow direction and colour of each), and up to six positional ones as
 * glows that drawGlows renders into the car's dynamic environment map.  A
 * course without runway lights uses its CourseEnv's lights A and car shadow,
 * scaled by the road's vertex values (u, v) under the wheels.  A stack
 * object of RacePS2Base::drawCars / drawCarReflections (T131). */
#ifndef GAME_T181_CAR_LIGHTING_H
#define GAME_T181_CAR_LIGHTING_H

#include "game/game.h"

class CourseData;                           /* T179_course_data.h (pointers only) */
class CourseEnv;                            /* T178_course_env.h */
class RaceCarState;                         /* T302_car_state.h */
class GSBuffer;                             /* T514_gs_buffer.h */
class DynamicEnvmapCamera;                  /* T277_dynamic_envmap_camera.h */

/* ======================================================================== */
/* CarRenderCtx                                                             */
/* ======================================================================== */

/* A light of the car, set up for one GL light unit 0x65.. (0x50 bytes).  The
 * pgl_lightfv parameters (G001 PGL_LIGHT_*): dir POSITION (w 0: a
 * direction), AMBIENT, DIFFUSE, SPECULAR (u060 p0 / p1 / p4). */
struct CarLight {
    f32 dir[4];                             /* 0x00 (parameter 0xD) the direction to the light */
    f32 ambient[4];                         /* 0x10 (parameter 0) the ambient colour */
    f32 diffuse[4];                         /* 0x20 (parameter 1) the diffuse colour */
    f32 specular[4];                        /* 0x30 (parameter 4) the specular colour */
    f32 shadowColor[4];                     /* 0x40 the colour of the car's shadow from it (car_shadow_set_color, T446; u060 color) */
};
EGCS_SIZE(CarLight, 0x50);

/* A runway light drawn as a glow into the environment map (0x28 bytes). */
struct CarGlow {
    f32 pos[4];                             /* 0x00 (w = 1) */
    f32 color[4];                           /* 0x10 */
    f32 intensity;                          /* 0x20 sqrt of the light's weight */
    f32 size;                               /* 0x24 */
};
EGCS_SIZE(CarGlow, 0x28);

/* Not polymorphic.  0x248 bytes of data; the stack slots of RacePS2Base::
 * drawCars / drawCarReflections are 0x250 (sp+0x90, the next local 16-aligned at
 * sp+0x2E0: u060's pad_248 is that alignment, nothing reads it). */
struct CarRenderCtx {
    /* The lights of the car of state s at time t (cd: the course). */
    void setup(CourseData *cd, RaceCarState *s, f32 t);     /* 0x001AB7D0 */
    /* The CourseEnv's ambient colour, then the GL lights (or the course's
     * lights A, course_env_set_car_lights, by the road values roadU / roadV). */
    void apply();                                           /* 0x001AC218 */
    /* Shadow pass i of the body: the shadow direction (light i's direction
     * x -dirScale) and colour, strength g_00352AC4 = 1; or applyDefault()
     * without runway lights.  (u060 applyPass.) */
    void applyShadowPass(int i);                            /* 0x001AC328 */
    /* The CourseEnv's car shadow colour, strength course_env_car_body_light(roadU, roadV). */
    void applyDefault();                                    /* 0x001AC3D0 */
    /* envBuf (the scene's environment map: gfx_frame_effect's target B) is
     * copied into envTex (its own VRAM: the draw buffer), then the glows
     * seen by cam are drawn over it.  The last argument (u045: the car is
     * the camera's target) is not used. */
    void drawGlows(GSBuffer *envTex, GSBuffer *envBuf, DynamicEnvmapCamera *cam,
                   int unused);                             /* 0x001AC5B8 */

    CarLight   lights[4];                   /* 0x000 */
    CarGlow    glows[6];                    /* 0x140 */
    int        passes;                      /* 0x230 the lights used (1 without runway lights) */
    int        glowCount;                   /* 0x234 */
    /* u045 envmap: then the glows are drawn into the car's environment map */
    int        lit;                         /* 0x238 runway lights are used */
    CourseEnv *env;                         /* 0x23C */
    /* Not positions: the mean of the road's vertex values under the four
     * wheels (RunwayContact u / v, car_contact_u / _v), the factors of
     * course_env_set_car_lights / course_env_car_body_light (T178; u060
     * avgX / avgZ). */
    f32        roadU;                       /* 0x240 the wheels' mean contact u (car_contact_u) */
    f32        roadV;                       /* 0x244 the wheels' mean contact v (car_contact_v) */
};
EGCS_SIZE(CarRenderCtx, 0x248);

#endif /* GAME_T181_CAR_LIGHTING_H */
