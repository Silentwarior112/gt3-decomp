/* T178_course_env.h - CourseEnv, the environment block of a course file,
 * and the functions that hand it to the renderer (translation unit T178,
 * unit u059), 0x001A80E0..0x001A8D18.
 *
 * The block (0xF50 bytes, CourseData +0x80, T179; no pointers: relocating
 * it does nothing) holds the course's lights (four of 0x80 bytes at +0xD0
 * for the scene, four of 0x50 at +0xAD0), three fogs (the view, the
 * rear-view mirror, the road reflection pass: u045's reading of what the
 * first names call the shadow pass), the ambient colours, the weather, the
 * tyre smoke types (T173) and surface records (T332), the replay blur, the
 * vignette, the colour correction, the sun / lamp / flare / reflection
 * parameters of FlareClass (T159), the car shadow, the environment map and
 * the vision list flags.  Its users name the fields they read: u043..u045
 * (T125 / T130 / T131), u058 (T173), u060 (T180 / T181), u061 (T182 /
 * T183), u075, u092 / u094 (T383 / T395: u092's FogSource and u094's
 * StepPairSource are CourseEnv, h22).  flags: bits 0..3 lights A on, 0x20
 * rain, 0x40 the vignette, 0x80 the colour correction (read as a byte,
 * >> 7), bit 8 the replay depth-of-field blur (T131), 0x400 the sun, bits
 * 11..14 lights B on, 0x8000 the lamp light parameters (RacePS2Base::
 * addCarLamps; the cars' night model), 0x10000 / 0x20000 the road
 * reflection pass draws the cars / the lamps (u045), 0x40000 the smoke
 * fades with the distance, 0x80000 the cars' evening model (kind 3, u043),
 * 0x100000 a sub view gets a blank (cleared) environment map (u045; the
 * first reading "keeps the map" was wrong), 0x200000 the flare types
 * (addCourseLights), 0x400000 no lights in the split view (u045).
 *
 * The lights' four vectors are pgl_lightfv's PGL_LIGHT_POSITION (pos),
 * PGL_LIGHT_AMBIENT, PGL_LIGHT_DIFFUSE and PGL_LIGHT_SPECULAR (G001; u059's
 * p0 / p1 / p4 were the parameter numbers).  Without
 * runway lights a car is lit by lights A dimmed by the road's vertex values
 * u / v at its wheels (RunwayContact u / v, T153; T181 passes the mean of
 * car_contact_u / _v, its roadU / roadV): course_env_set_car_lights scales
 * each vector by D + (1 - D) v (B + (1 - B) u), B / D the floors below
 * (diffuseFloorU .. ambientFloorV: the levels at u = 0 / v = 0), and
 * course_env_car_body_light gives the body light (bodyLightU0 +
 * (bodyLightU1 - bodyLightU0) u) v.
 */
#ifndef GAME_T178_COURSE_ENV_H
#define GAME_T178_COURSE_ENV_H

#include "game/game.h"

class CourseData;                           /* T179 (pointers only: forward declarations) */
class FlareClass;                           /* T159 */
struct Vec3;                                /* T159 */
struct Weather;                             /* G001_flare.h */

/* ? name (u059's).  A fog: course_fog_apply sets gfx_set_fog_params(nearZ x
 * scale, farZ) and gfx_set_fog_color_f(color).  0x18 bytes (CourseEnv
 * +0x60, +0xA90, +0xC40). */
struct CourseFog {
    f32 color[3];                           /* 0x00 */
    f32 unk_0C;                             /* 0x0C */
    f32 nearZ;                              /* 0x10 scaled by course_fog_apply's factor */
    f32 farZ;                               /* 0x14 */
};
EGCS_SIZE(CourseFog, 0x18);

/* ? names (u059 / u060's CourseEnvLight).  A scene light (course_env_set_car_lights:
 * pgl_lightfv parameters 0xD, 0, 1, 4; T181 lights the cars with them by
 * the runway lights' type).  0x80 bytes. */
struct CourseEnvLightA {
    f32 pos[4];                             /* 0x00 PGL_LIGHT_POSITION (0xD) */
    f32 ambient[4];                         /* 0x10 PGL_LIGHT_AMBIENT (0; u059 p0) */
    f32 diffuse[4];                         /* 0x20 PGL_LIGHT_DIFFUSE (1; u059 p1) */
    f32 specular[4];                        /* 0x30 PGL_LIGHT_SPECULAR (4; u059 p4) */
    f32 glowSize;                           /* 0x40 (u060) */
    u8  unk_44[8];                          /* 0x44 */
    f32 exponent;                           /* 0x4C the spot exponent */
    f32 cutoff;                             /* 0x50 degrees, 180: no spot cone */
    f32 att[3];                             /* 0x54 constant, linear, quadratic attenuation */
    f32 color[4];                           /* 0x60 the car shadow's colour from this light (car_shadow_set_color, T181) */
    f32 glow[4];                            /* 0x70 the colour of its glow */
};
EGCS_SIZE(CourseEnvLightA, 0x80);

/* ? names (u059's).  A light of course_env_set_course_lights (pgl_lightfv parameters 0xD, 0,
 * 1, 4); B light 0's pos is also the gadgets' shadow direction
 * (RigidBodyManager::SetLightVector, T131).  0x50 bytes. */
struct CourseEnvLightB {
    f32 pos[4];                             /* 0x00 PGL_LIGHT_POSITION */
    f32 ambient[4];                         /* 0x10 PGL_LIGHT_AMBIENT (u059 p0) */
    f32 diffuse[4];                         /* 0x20 PGL_LIGHT_DIFFUSE (u059 p1) */
    f32 specular[4];                        /* 0x30 PGL_LIGHT_SPECULAR (u059 p4) */
    u8  unk_40[0x10];                       /* 0x40 */
};
EGCS_SIZE(CourseEnvLightB, 0x50);

/* ? name (u059's).  A flare type: FlareClass::setFlareType(i, v[0..2] x
 * v[3] (r, g, b x level), v[4..13] = glowScale, size, haloAlpha,
 * exponent, att0, att1, att2, far, threshold, period; v[14..15] not read
 * here).
 * 0x40 bytes. */
struct CourseEnvFlareType {
    f32 v[16];                              /* 0x00 */
};
EGCS_SIZE(CourseEnvFlareType, 0x40);

/* ? names (u058's).  A tyre smoke type (RaceSmoke, T173: CourseEnv
 * +0x4D0 + i x 0x44).  T332's CourseEnv_initSurfaceSmoke fills types 0..10 from
 * +0x0C (its records at CourseEnv +0x4DC are these, seen from density:
 * h17's SurfaceParam). */
struct SmokeType {
    f32 r;                                  /* 0x00 */
    f32 g;                                  /* 0x04 */
    f32 b;                                  /* 0x08 */
    f32 density;                            /* 0x0C < 0.01: none; the alpha scale */
    f32 size;                               /* 0x10 the start size (x rate) */
    f32 grow;                               /* 0x14 size / s */
    f32 rise;                               /* 0x18 the start vertical speed */
    f32 spin;                               /* 0x1C degrees / s */
    f32 fade;                               /* 0x20 alpha / s */
    u8  unk_24[8];                          /* 0x24 */
    f32 gravity;                            /* 0x2C x 9.81 */
    f32 alphaScale;                         /* 0x30 the end of the alpha scale ramp */
    u8  unk_34[0xC];                        /* 0x34 */
    u32 period;                             /* 0x40 the emission counter's period; the fade spread */
};
EGCS_SIZE(SmokeType, 0x44);

/* ? name (u059's).  Not polymorphic, 0xF50 bytes (course_env_host_dump dumps it). */
class CourseEnv {
public:
    u8                 unk_000[0x10];       /* 0x000 */
    int                flags;               /* 0x010 (see above; signed: u059's light loops test
                                               (flags >> (i + 11)) & 1 with srav) */
    u8                 unk_014[0xC];        /* 0x014 */
    f32                farClip;             /* 0x020 the view's far clip (T131 setFarClip; u045 f20); the
                                               sun is placed at farClip / 2 (u059 sunScale) */
    f32                ambScale;            /* 0x024 */
    /* the sky's rotations about y / x in degrees (u075: envmapDrawScene's
     * vu0_rotate_y / vu0_rotate_x before envSky; u059 unk_028 / unk_048) */
    f32                skyRotY;             /* 0x028 the sky's rotation about y */
    u8                 unk_02C[0xC];        /* 0x02C */
    f32                amb[4];              /* 0x038 F_001A8208: x ambScale */
    f32                skyRotX;             /* 0x048 the sky's rotation about x */
    f32                brakeGlowScale;      /* 0x04C RaceEntryCar::drawTires' scale: x car_brake_heat_sq, the heat brake glow (u060; u045 f4C, u059 unk_04C) */
    f32                texLodSize;          /* 0x050 modelset_set_tex_lod_size (T131; u045 f50) */
    f32                carLodDist;          /* 0x054 the cars' CarModel lodDist, x 0.5 for the target (T131;
                                               u045 f54) */
    f32                fade;                /* 0x058 F_001A8618 */
    u8                 unk_05C[4];          /* 0x05C */
    CourseFog          fog;                 /* 0x060 the view's (course_env_apply) */
    u8                 unk_078[0x38];       /* 0x078 */
    int                weatherDrops;        /* 0x0B0 weather_init's count (T130; u044 unkB0) */
    f32                weatherSize;         /* 0x0B4 weather_set's (F_001A8678; u059 p0B4[6]) */
    f32                weatherSpreadXZ;     /* 0x0B8 */
    f32                weatherSpreadY;      /* 0x0BC */
    f32                weatherAhead;        /* 0x0C0 */
    f32                weatherVelXZ;        /* 0x0C4 */
    f32                weatherVelY;         /* 0x0C8 */
    u8                 unk_0CC[4];          /* 0x0CC */
    CourseEnvLightA    lightA[4];           /* 0x0D0 (u060 lights) */
    u8                 unk_2D0[0x200];      /* 0x2D0 */
    SmokeType          smoke[16];           /* 0x4D0 (u058) */
    f32                smokeMinIntensity;   /* 0x910 (u058 minIntensity) */
    int                smokeDivX;           /* 0x914 the smoke's horizontal resolution divisor (1..64) (u058 divX) */
    int                smokeBlend;          /* 0x918 0x48: additive, unsorted (else 0x44: alpha) (u058 blend) */
    int                smokeDivY;           /* 0x91C the vertical one (u058 divY) */
    u8                 unk_920[0x10];       /* 0x920 */
    f32                blurFNumber;         /* 0x930 the replay blur's f-number (ReplayBlur +0x08; u045 f930) */
    f32                blurFilmDiag;        /* 0x934 its film diagonal, mm (ReplayBlur::setFilm; u045 f934) */
    f32                blurFocusMin;        /* 0x938 the least focus distance (u045 f938) */
    u8                 unk_93C[0x14];       /* 0x93C */
    f32                vignetteRadius0;     /* 0x950 (? the outer radius of the ring) (T183; u061 radius0) */
    f32                vignetteRadius1;     /* 0x954 (? the inner radius) (u061 radius1) */
    f32                vignetteColor0[4];   /* 0x958 at radius0 */
    f32                vignetteColor1[4];   /* 0x968 at radius1 */
    u8                 unk_978[8];          /* 0x978 */
    f32                correctionColor[3];  /* 0x980 the colour correction's r, g, b (T383; u092 unk980..988) */
    f32                correctionLevel[2];  /* 0x98C its lo / hi (T395's StepPair; u094 init[2]) */
    f32                correctionSaturation;    /* 0x994 (T383; u092 unk994) */
    u8                 unk_998[0x28];       /* 0x998 */
    /* the sun: FlareClass::setSun (the glare) and drawSunDisc (the disc);
     * u059 sun9C0 .. sun9D8, sun9E8 */
    f32                sunColor[3];         /* 0x9C0 the sun's colour: setSun's and drawSunDisc's r, g, b */
    f32                sunQueryScale;       /* 0x9CC setSun's queryScale (the occlusion query) */
    f32                sunGlareScale;       /* 0x9D0 setSun's scale, the glare (x farClip / 2) */
    f32                sunDiscScale;        /* 0x9D4 drawSunDisc's scale, the disc (x 10) */
    f32                sunDiscAlpha;        /* 0x9D8 drawSunDisc's a, the disc's alpha */
    f32                sunPitch;            /* 0x9DC degrees */
    f32                sunYaw;              /* 0x9E0 degrees */
    int                sunType;             /* 0x9E4 FlareClass::sunType (the glare's model) */
    f32                sunQuerySize;        /* 0x9E8 setSun's querySize */
    u8                 unk_9EC[4];          /* 0x9EC */
    f32                lampColor[3];        /* 0x9F0 FlareClass::setLightParams' r, g, b (x lampLevel; u059 flare9F0) */
    f32                lampLevel;           /* 0x9FC (u059 flare9FC) */
    f32                lampParams[8];       /* 0xA00 setLightParams' glowScale, size, haloAlpha, att0, att1,
                                               att2, far, threshold (u059 flareA00) */
    u8                 unk_A20[0x40];       /* 0xA20 */
    f32                carShadowColor[3];   /* 0xA60 car_shadow_set_color (T181; u059 / u060 colorA60) */
    f32                carShadowAlpha;      /* 0xA6C (u059 / u060 alphaA6C) */
    f32                dirPitch;            /* 0xA70 degrees: car_shadow_set_dir (course_env_set_shadow_dir) */
    f32                dirYaw;              /* 0xA74 degrees */
    f32                dirScale;            /* 0xA78 (T181: the body's light direction x -dirScale) */
    u8                 unk_A7C[0x14];       /* 0xA7C */
    CourseFog          mirrorFog;           /* 0xA90 the rear-view mirror's (RacePS2Base::drawMirror; u045 subA90) */
    u8                 unk_AA8[8];          /* 0xAA8 */
    /* (u059 fAB0 .. fACC) */
    f32                bodyLightU1;         /* 0xAB0 course_env_car_body_light's body light at u = 1 (x v) */
    f32                diffuseFloorU;       /* 0xAB4 course_env_set_car_lights: the diffuse's level at u = 0 (B) */
    f32                bodyLightU0;         /* 0xAB8 the body light at u = 0 */
    f32                diffuseFloorV;       /* 0xABC the diffuse's level at v = 0 (D) */
    f32                specularFloorU;      /* 0xAC0 the specular's at u = 0 */
    f32                specularFloorV;      /* 0xAC4 the specular's at v = 0 */
    f32                ambientFloorU;       /* 0xAC8 the ambient's at u = 0 */
    f32                ambientFloorV;       /* 0xACC the ambient's at v = 0 */
    CourseEnvLightB    lightB[4];           /* 0xAD0 (u045 subAD0) */
    u8                 unk_C10[0x10];       /* 0xC10 */
    /* The road reflection pass (RacePS2Base::drawReflections ..
     * drawLampReflections, T131; u045 / u059 shadowBlend .. shadowFog: the
     * first names called it the shadow pass). */
    int                reflBlend;           /* 0xC20 the reflection pass's gfx_blend_func (T131; u045 fC20) */
    int                reflW;               /* 0xC24 the reflection buffer's size */
    int                reflH;               /* 0xC28 */
    int                reflBlur;            /* 0xC2C passes of the reflection's blur */
    u8                 unk_C30[0x10];       /* 0xC30 */
    CourseFog          reflFog;             /* 0xC40 the reflection pass's (RacePS2Base::drawReflections; u045 subC40) */
    u8                 unk_C58[8];          /* 0xC58 */
    CourseEnvFlareType flareTypes[8];       /* 0xC60 */
    f32                reflColor[3];        /* 0xE60 FlareClass::setReflectionParams' r, g, b (x reflLevel;
                                               u059 reflE60) */
    f32                reflLevel;           /* 0xE6C (u059 reflE6C) */
    f32                reflShape[4];        /* 0xE70 its width0, width1, maxLength, top (u059 reflE70) */
    u8                 unk_E80[0x20];       /* 0xE80 */
    f32                blurScale;           /* 0xEA0 the wheels' spin blur: time scale (T180, u060) */
    f32                blurExp;             /* 0xEA4 the speed exponent (u060) */
    u8                 unk_EA8[0x18];       /* 0xEA8 */
    f32                envmapParams[2];     /* 0xEC0 pgl_set_envmap_params (course_env_set_envmap; u059 fEC0 / fEC4) */
    u8                 unk_EC8[0x28];       /* 0xEC8 */
    f32                ambient[4];          /* 0xEF0 pgl_light_modelfv(0) of the cars (T181) */
    u8                 unk_F00[0x30];       /* 0xF00 */
    int                visionMain;          /* 0xF30 the vision list flags: the main view (T131; u045 fF30) */
    int                visionSub;           /* 0xF34 a sub view (u045 fF34) */
    int                visionMirror;        /* 0xF38 the rear-view mirror (u045 fF38) */
    u8                 unk_F3C[0x14];       /* 0xF3C */
};
EGCS_SIZE(CourseEnv, 0xF50);

/* The functions of the block (u059's names; the others keep their F_ names).
 * Callers in parentheses. */
void F_001A80E0(CourseEnv *env);                            /* 0x001A80E0 the relocation: nothing (T179) */
void F_001A80E8(CourseEnv *env, CourseData *cd);            /* 0x001A80E8 empty (RaceBase::virtual_8) */
/* gfx_disable(0x65..0x68): the four lights off (RacePS2Base::virtual_35). */
void course_env_lights_off(CourseEnv *env);                            /* 0x001A80F0 */
void F_001A8128(CourseEnv *env, CourseData *cd);            /* 0x001A8128 empty (RaceBase::virtual_29) */
/* Lights B, the light direction, the fog (scale 1), course_env_set_envmap
 * (RacePS2Base::drawView / drawSubView / drawMirror). */
void course_env_apply(CourseEnv *env);                            /* 0x001A8130 */
/* pgl_envmap_set_basis(0, 1, 0, 1, 0, 0), pgl_set_envmap_params
 * (envmapParams) (also u115). */
void course_env_set_envmap(CourseEnv *env);                            /* 0x001A8178 */
/* A fog (nearZ x scale) (T131: the mirror's, the reflection pass's). */
void course_fog_apply(const CourseFog *fog, f32 scale);           /* 0x001A81C8 */
/* pgl_set_const_color(0, 0, 0, 0), pgl_set_const_vector(amb x ambScale) (u075's
 * F_001D83A8, u117). */
void F_001A8208(const CourseEnv *env);                      /* 0x001A8208 */
/* The light model and lights B: light 0x65 + i with its four vectors
 * when flags bit 11 + i, else off. */
void course_env_set_course_lights(const CourseEnv *env);                      /* 0x001A8268 */
/* out.xyz = in.xyz x (((1 - (1 - a) b) d c + (1 - d)) e), out.w = in.w. */
void F_001A8360(f32 *out, const f32 *in, f32 a, f32 b, f32 c, f32 d, f32 e);   /* 0x001A8360 */
/* Lights A (flags bits 0..3), each colour vector scaled by F_001A8360
 * with the floors diffuseFloorU .. ambientFloorV and the road's vertex values u / v under the
 * car (see above; T181's CarRenderCtx::apply without runway lights). */
void course_env_set_car_lights(const CourseEnv *env, f32 u, f32 v);        /* 0x001A83C8 */
/* (u x (bodyLightU1 - bodyLightU0) + bodyLightU0) x v: the body light at the road's vertex
 * values (T181's CarRenderCtx::applyDefault). */
f32  course_env_car_body_light(const CourseEnv *env, f32 u, f32 v);        /* 0x001A8588 */
/* Unreferenced: pgl_set_const_vector(1, 1, 1, course_env_car_body_light's light x carShadowAlpha),
 * pgl_set_const_color(carShadowColor, 0). */
void F_001A85A8(const CourseEnv *env, f32 u, f32 v);        /* 0x001A85A8 */
/* fade != 0: pgl_set_const_vector(1, 1, 1, fade), pgl_set_const_color(0, 0, 0, 0), 1; else 0
 * (RacePS2Base::drawCars). */
int  F_001A8618(const CourseEnv *env);                      /* 0x001A8618 */
/* weather_set(w, weatherSize .. weatherVelY) (RacePS2Base::virtual_8, its
 * weather; the first pass's `void *buf'). */
void F_001A8678(const CourseEnv *env, struct Weather *w);   /* 0x001A8678 */
/* The unit vector of pitch / yaw (degrees); dir_to_angles the inverse
 * (unreferenced). */
void angles_to_dir(Vec3 *out, f32 pitch, f32 yaw);             /* 0x001A86B0 */
void dir_to_angles(const Vec3 *v, f32 *pitch, f32 *yaw);       /* 0x001A8780 */
/* flags 0x400: FlareClass::setSun with the sun's direction x farClip / 2,
 * fc->sunType (RacePS2Base::drawView / drawSubView). */
void course_env_set_sun(const CourseEnv *env, FlareClass *fc);      /* 0x001A8810 */
/* flags 0x400: the sun disc, FlareClass::drawSunDisc with the sun's
 * direction x 10 (RacePS2Base::drawView / drawSubView). */
void course_env_draw_sun_disc(const CourseEnv *env, FlareClass *fc);      /* 0x001A88C0 */
/* car_shadow_set_dir(the direction of dirPitch / dirYaw x dirScale). */
void course_env_set_shadow_dir(const CourseEnv *env);                      /* 0x001A8958 */
/* FlareClass::setLightParams (RacePS2Base::addCarLamps). */
void course_env_set_lamps(const CourseEnv *env, FlareClass *fc);      /* 0x001A89A8 */
/* FlareClass::setFlareType of the eight flare types (addCourseLights). */
void course_env_set_flare_types(const CourseEnv *env, FlareClass *fc);      /* 0x001A8A10 */
/* FlareClass::setReflectionParams (drawLampReflections). */
void course_env_set_reflection(const CourseEnv *env, FlareClass *fc);      /* 0x001A8B88 */
/* Debug helpers (unreferenced): the block to the host file `path'
 * (zero-padded to 0x1350 bytes); read back (1: read). */
void course_env_host_dump(const CourseEnv *env, const char *path);    /* 0x001A8BD8 */
int  course_env_host_load(CourseEnv *env, const char *path);          /* 0x001A8CE8 */

#endif /* GAME_T178_COURSE_ENV_H */
