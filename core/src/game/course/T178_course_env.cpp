/* T178_course_env.cpp - CourseEnv, the environment block of a course file
 * (T178), u059.
 *
 * The block (0xF50 bytes, CourseData +0x80) holds the course's lights (four
 * of 0x80 bytes at +0xD0 for the scene and the cars, four of 0x50 at +0xAD0
 * for the course), the fogs, the ambient colours, the weather, the sun /
 * lamp / flare / reflection parameters handed to FlareClass (T159), the car
 * shadow and a few blend factors.  Two debug helpers dump it to / load it
 * from the host (unreferenced).
 *
 * Data (retail): .data 0x002C2670 (the two light model vectors, then the
 * frame info), .lit4 0x0034FF3C; .rodata 0x00330A70..0x00330A91, nothing
 * the code references: <new>'s "bad_alloc", then the type name
 * "t9UnitArena1Ui64" (T153 with GAME_T153_EMITS_TYPEINFO; no other class
 * without a key function seen).
 */
// DATA_START 0x00330A70
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (T153 suppresses it by default) */
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> (no use) */
#include "game/course/T178_course_env.h"           /* this translation unit */
#include "game/effects/T159_flare.h"                /* FlareClass (setSun .. setReflectionParams), Vec3 */
#include "game/car/model/T446_car_model.h"            /* car_shadow_set_dir */
#include "engine/io/T505_host_files.h"           /* host_read_file, host_write_file */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, weather_set, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* sinf, cosf, atan2f */
#include "game/sound/car_sound_internal.h"                           /* g_002C2670, g_002C2680 */

/* The four GL lights the course uses: PGL_CAP_LIGHT0 + 0..3. */
#define COURSE_LIGHT_COUNT      4
#define DEG2RAD_PI              3.1415927f  /* (x * pi) / 180 */
#define RAD2DEG                 57.295776f
/* course_env_host_dump's host file: the block zero-padded to 0x1350 bytes. */
#define COURSE_ENV_DUMP_SIZE    0x1350

// FUNC 0x001A80E0 F_001A80E0
//   The relocation of the block (CourseData +0x80, course_data_relocate): it has no
//   pointers, nothing to do.
void F_001A80E0(CourseEnv *env)
{
}

// FUNC 0x001A80E8 F_001A80E8
//   Empty (RaceBase::virtual_8).
void F_001A80E8(CourseEnv *env, CourseData *cd)
{
}

// FUNC 0x001A80F0 course_env_lights_off
//   Switches the four lights off (RacePS2Base::virtual_35).
void course_env_lights_off(CourseEnv *env)
{
    gfx_disable(PGL_CAP_LIGHT0 + 0);
    gfx_disable(PGL_CAP_LIGHT0 + 1);
    gfx_disable(PGL_CAP_LIGHT0 + 2);
    gfx_disable(PGL_CAP_LIGHT0 + 3);
}

// FUNC 0x001A8128 F_001A8128
//   Empty (RaceBase::virtual_29).
void F_001A8128(CourseEnv *env, CourseData *cd)
{
}

// FUNC 0x001A8130 course_env_apply
//   Sets up the lights B, the car shadow's direction, the fog (scale 1) and
//   the environment map (RacePS2Base::drawView / drawSubView / drawMirror).
void course_env_apply(CourseEnv *env)
{
    course_env_set_course_lights(env);
    course_env_set_shadow_dir(env);
    course_fog_apply(&env->fog, 1.0f);
    course_env_set_envmap(env);
}

// FUNC 0x001A8178 course_env_set_envmap
//   The environment map: basis (0, 1, 0) / (1, 0, 0), envmapParams.
void course_env_set_envmap(CourseEnv *env)
{
    pgl_envmap_set_basis(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    pgl_set_envmap_params(env->envmapParams[0], env->envmapParams[1]);
}

// FUNC 0x001A81C8 course_fog_apply
//   The fog: gfx_set_fog_params(nearZ * scale, farZ), its colour.
void course_fog_apply(const CourseFog *fog, f32 scale)
{
    gfx_set_fog_params(fog->nearZ * scale, fog->farZ);
    gfx_set_fog_color_f(fog->color[0], fog->color[1], fog->color[2]);
}

// FUNC 0x001A8208 F_001A8208
//   Called by envmapRender (T276): pgl_set_const_color(0, 0, 0, 0), pgl_set_const_vector(amb *
//   ambScale) (G001's lighting mode 5 colour and vector).
void F_001A8208(const CourseEnv *env)
{
    pgl_set_const_color(0.0f, 0.0f, 0.0f, 0.0f);
    f32 k = env->ambScale;
    pgl_set_const_vector(env->amb[0] * k, env->amb[1] * k, env->amb[2] * k, env->amb[3] * k);
}

/* The light model vectors (.data: not const in the original; written in
 * definition order before the frame info). */
f32 g_002C2670[4] = { 0.0f, 0.0f, 0.0f, 1.0f };     /* 0x002C2670 PGL_LIGHTMODEL_AMBIENT */
f32 g_002C2680[4] = { 0.0f, 2.0f, 0.0f, 1.0f };     /* 0x002C2680 PGL_LIGHTMODEL_EYE */

// FUNC 0x001A8268 course_env_set_course_lights
//   The light model (ambient {0, 0, 0, 1}, the eye at {0, 2, 0, 1}) and
//   lights B: light PGL_CAP_LIGHT0 + i is enabled with its four vectors
//   when flags bit 11 + i is set, else disabled.
void course_env_set_course_lights(const CourseEnv *env)
{
    pgl_light_modelfv(PGL_LIGHTMODEL_AMBIENT, g_002C2670);
    pgl_light_modelfv(PGL_LIGHTMODEL_EYE, g_002C2680);
    int light = PGL_CAP_LIGHT0;
    for (int i = 0; i < COURSE_LIGHT_COUNT; i++, light++) {
        const CourseEnvLightB *l = &env->lightB[i];
        if ((env->flags >> (i + 11)) & 1) {
            gfx_enable(light);
            pgl_lightfv(light, PGL_LIGHT_POSITION, l->pos);
            pgl_lightfv(light, PGL_LIGHT_AMBIENT, l->ambient);
            pgl_lightfv(light, PGL_LIGHT_DIFFUSE, l->diffuse);
            pgl_lightfv(light, PGL_LIGHT_SPECULAR, l->specular);
        } else {
            gfx_disable(light);
        }
    }
}

// FUNC 0x001A8360 F_001A8360
//   out.xyz = in.xyz * (((1 - (1 - a) * b) * d * c + (1 - d)) * e);
//   out.w = in.w.  (course_env_set_car_lights: a, c the road's u / v, b, d one minus the
//   floors.)  k is built in two statements (retail ties the products to d,
//   not to the 1.0); the copy loop runs on its own pointers, out[3] = in[3]
//   from the arguments.
void F_001A8360(f32 *out, const f32 *in, f32 a, f32 b, f32 c, f32 d, f32 e)
{
    f32 k = 1.0f - (1.0f - a) * b;
    k = k * d * c + (1.0f - d);
    k *= e;
    f32 *o = out;
    const f32 *s = in;
    for (int i = 2; i >= 0; i--)
        *o++ = *s++ * k;
    out[3] = in[3];
}

// FUNC 0x001A83C8 course_env_set_car_lights
//   Called by CarRenderCtx::apply (T181) without runway lights, with the
//   mean of the road's vertex values u / v under the car's wheels: lights
//   A, light PGL_CAP_LIGHT0 + i for flags bit i, each colour vector scaled
//   by F_001A8360 with the floors at u = 0 (ambientFloorU ambient, diffuseFloorU diffuse,
//   specularFloorU specular) and at v = 0 (ambientFloorV, diffuseFloorV, specularFloorV).
void course_env_set_car_lights(const CourseEnv *env, f32 u, f32 v)
{
    f32 color[4];
    f32 diffuseU = 1.0f - env->diffuseFloorU;
    f32 ambientU = 1.0f - env->ambientFloorU;
    f32 specularU = 1.0f - env->specularFloorU;
    f32 diffuseV = 1.0f - env->diffuseFloorV;
    f32 ambientV = 1.0f - env->ambientFloorV;
    f32 specularV = 1.0f - env->specularFloorV;
    int light = PGL_CAP_LIGHT0;
    for (int i = 0; i < COURSE_LIGHT_COUNT; i++, light++) {
        const CourseEnvLightA *l = &env->lightA[i];
        if ((env->flags >> i) & 1) {
            gfx_enable(light);
            pgl_lightfv(light, PGL_LIGHT_POSITION, l->pos);
            F_001A8360(color, l->ambient, u, ambientU, v, ambientV, 1.0f);
            pgl_lightfv(light, PGL_LIGHT_AMBIENT, color);
            F_001A8360(color, l->diffuse, u, diffuseU, v, diffuseV, 1.0f);
            pgl_lightfv(light, PGL_LIGHT_DIFFUSE, color);
            F_001A8360(color, l->specular, u, specularU, v, specularV, 1.0f);
            pgl_lightfv(light, PGL_LIGHT_SPECULAR, color);
        } else {
            gfx_disable(light);
        }
    }
}

// FUNC 0x001A8588 course_env_car_body_light
//   The car body's light level at the road's u / v: (u * (bodyLightU1 - bodyLightU0) +
//   bodyLightU0) * v (called by CarRenderCtx::applyDefault, T181).
f32 course_env_car_body_light(const CourseEnv *env, f32 u, f32 v)
{
    f32 atZero = env->bodyLightU0;
    return (u * (env->bodyLightU1 - atZero) + atZero) * v;
}

// FUNC 0x001A85A8 F_001A85A8
//   Unreferenced.  pgl_set_const_vector(1, 1, 1, the course_env_car_body_light level *
//   carShadowAlpha), pgl_set_const_color(carShadowColor, 0).  The level is a
//   variable set twice (retail ties the last product to the alpha, not to
//   the level).
void F_001A85A8(const CourseEnv *env, f32 u, f32 v)
{
    f32 atZero = env->bodyLightU0;
    f32 level = (env->bodyLightU1 - atZero) * u + atZero;
    level *= v;
    pgl_set_const_vector(1.0f, 1.0f, 1.0f, level * env->carShadowAlpha);
    pgl_set_const_color(env->carShadowColor[0], env->carShadowColor[1], env->carShadowColor[2], 0.0f);
}

// FUNC 0x001A8618 F_001A8618
//   When fade is not 0: pgl_set_const_vector(1, 1, 1, fade), pgl_set_const_color(0, 0, 0, 0),
//   returns 1; else 0 (RacePS2Base::drawCars).
int F_001A8618(const CourseEnv *env)
{
    f32 fade = env->fade;
    int faded = 0;
    if (fade != 0.0f) {
        pgl_set_const_vector(1.0f, 1.0f, 1.0f, fade);
        pgl_set_const_color(0.0f, 0.0f, 0.0f, 0.0f);
        faded = 1;
    }
    return faded;
}

// FUNC 0x001A8678 F_001A8678
//   weather_set(w, the six weather floats at +0xB4) (RacePS2Base::virtual_8).
void F_001A8678(const CourseEnv *env, struct Weather *w)
{
    weather_set(w, env->weatherSize, env->weatherSpreadXZ, env->weatherSpreadY,
                env->weatherAhead, env->weatherVelXZ, env->weatherVelY);
}

// FUNC 0x001A86B0 angles_to_dir
//   The unit vector of two angles in degrees:
//   x = sin(-yaw) cos(-pitch), y = -sin(-pitch), z = cos(-yaw) cos(-pitch).
void angles_to_dir(Vec3 *out, f32 pitch, f32 yaw)
{
    f32 p = (-pitch * DEG2RAD_PI) / 180.0f;
    f32 sinP = sinf(p);
    f32 cosP = cosf(p);
    f32 y = (-yaw * DEG2RAD_PI) / 180.0f;
    f32 sinY = sinf(y);
    f32 cosY = cosf(y);
    out->x = sinY * cosP;
    out->y = -sinP;
    out->z = cosY * cosP;
}

// FUNC 0x001A8780 dir_to_angles
//   Unreferenced.  The inverse of angles_to_dir: *pitch = atan2(y, |xz|),
//   *yaw = -atan2(x, z), in degrees.  sqrt.s as a volatile asm (no errno
//   call; x and z stay in the argument registers).
void dir_to_angles(const Vec3 *v, f32 *pitch, f32 *yaw)
{
    f32 x = v->x;
    f32 z = v->z;
    f32 horiz = boot_sqrtf_v(x * x + z * z);
    *yaw = -atan2f(x, z) * RAD2DEG;
    *pitch = atan2f(v->y, horiz) * RAD2DEG;
}

// FUNC 0x001A8810 course_env_set_sun
//   The sun glare (flags 0x400): its direction times farClip / 2, the glare
//   scale times the same; fc->sunType = sunType (RacePS2Base::drawView /
//   47).
void course_env_set_sun(const CourseEnv *env, FlareClass *fc)
{
    if (env->flags & 0x400) {
        Vec3 dir;
        angles_to_dir(&dir, env->sunPitch, env->sunYaw);
        f32 k = env->farClip * 0.5f;
        dir.x = dir.x * k;
        dir.y = dir.y * k;
        dir.z = dir.z * k;
        fc->sunType = env->sunType;
        fc->setSun(&dir, env->sunQueryScale, env->sunQuerySize, env->sunGlareScale * k,
                   env->sunColor[0], env->sunColor[1], env->sunColor[2]);
    }
}

// FUNC 0x001A88C0 course_env_draw_sun_disc
//   The sun disc (flags 0x400): the sun direction times 10 to
//   FlareClass::drawSunDisc with the sun's colour, alpha sunDiscAlpha and scale
//   sunDiscScale * 10 (RacePS2Base::drawView / drawSubView).
void course_env_draw_sun_disc(const CourseEnv *env, FlareClass *fc)
{
    if (env->flags & 0x400) {
        Vec3 dir;
        angles_to_dir(&dir, env->sunPitch, env->sunYaw);
        dir.x = dir.x * 10.0f;
        dir.y = dir.y * 10.0f;
        dir.z = dir.z * 10.0f;
        fc->drawSunDisc(&dir, env->sunColor[0], env->sunColor[1], env->sunColor[2],
                       env->sunDiscAlpha, env->sunDiscScale * 10.0f);
    }
}

// FUNC 0x001A8958 course_env_set_shadow_dir
//   car_shadow_set_dir(the direction of dirPitch / dirYaw times dirScale).
void course_env_set_shadow_dir(const CourseEnv *env)
{
    Vec3 dir;
    angles_to_dir(&dir, env->dirPitch, env->dirYaw);
    f32 k = env->dirScale;
    car_shadow_set_dir(dir.x * k, dir.y * k, dir.z * k);
}

// FUNC 0x001A89A8 course_env_set_lamps
//   FlareClass::setLightParams(lampColor * lampLevel, lampParams[0..7])
//   (RacePS2Base::addCarLamps).
void course_env_set_lamps(const CourseEnv *env, FlareClass *fc)
{
    f32 k = env->lampLevel;
    fc->setLightParams(env->lampColor[0] * k, env->lampColor[1] * k, env->lampColor[2] * k,
                       env->lampParams[0], env->lampParams[1], env->lampParams[2],
                       env->lampParams[3], env->lampParams[4], env->lampParams[5],
                       env->lampParams[6], env->lampParams[7]);
}

// FUNC 0x001A8A10 course_env_set_flare_types
//   The eight flare types: FlareClass::setFlareType(i, rgb * v[3],
//   v[4..13]) (RacePS2Base::addCourseLights).  Every argument indexes the
//   table: retail strength-reduces each of the 14 fields to a pointer of
//   its own (and saves the caller-saved ones around the call).
void course_env_set_flare_types(const CourseEnv *env, FlareClass *fc)
{
    for (int i = 0; i < 8; i++)
        fc->setFlareType(i, env->flareTypes[i].v[0] * env->flareTypes[i].v[3], env->flareTypes[i].v[1] * env->flareTypes[i].v[3], env->flareTypes[i].v[2] * env->flareTypes[i].v[3], env->flareTypes[i].v[4], env->flareTypes[i].v[5], env->flareTypes[i].v[6], env->flareTypes[i].v[7],
                         env->flareTypes[i].v[8], env->flareTypes[i].v[9], env->flareTypes[i].v[10], env->flareTypes[i].v[11], env->flareTypes[i].v[12], env->flareTypes[i].v[13]);
}

// FUNC 0x001A8B88 course_env_set_reflection
//   FlareClass::setReflectionParams(reflColor * reflLevel, reflShape[0..3])
//   (RacePS2Base::drawLampReflections).
void course_env_set_reflection(const CourseEnv *env, FlareClass *fc)
{
    f32 k = env->reflLevel;
    fc->setReflectionParams(env->reflColor[0] * k, env->reflColor[1] * k, env->reflColor[2] * k,
                            env->reflShape[0], env->reflShape[1], env->reflShape[2],
                            env->reflShape[3]);
}

// FUNC 0x001A8BD8 course_env_host_dump
//   Unreferenced debug helper: writes the block, zero-padded to 0x1350
//   bytes, to the host file `path'.  The clearing loop counts up; egcs
//   reverses it (one store: it clears from the end).
void course_env_host_dump(const CourseEnv *env, const char *path)
{
    union {
        u8        bytes[COURSE_ENV_DUMP_SIZE];
        CourseEnv env;
    } file;
    for (int i = 0; i < COURSE_ENV_DUMP_SIZE; i++)
        file.bytes[i] = 0;
    file.env = *env;
    host_write_file(path, &file, COURSE_ENV_DUMP_SIZE);
}

// FUNC 0x001A8CE8 course_env_host_load
//   Unreferenced debug helper: reads the host file `path' over the block
//   (all of it); 1 when it was read.
int course_env_host_load(CourseEnv *env, const char *path)
{
    return host_read_file(path, env, 0, 0) != 0;
}
