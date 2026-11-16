/* T159_flare.h - FlareClass: lens flares, light glows and road reflections
 * (translation unit T159, unit u055), 0x0019AD00..0x0019C368; and the small
 * vector types Vec3 / Vec4.
 *
 * Per frame (RacePS2Base::drawView / drawSubView, T131): begin(screen)
 * empties the lists and keeps the projection matrix; the course submits its
 * flare sources (T195: FlareGroup::submit -> addSource) and the cars their
 * lamps (T180: RaceCarModel::addLamps -> addLight), each projected with the
 * current VU0 matrix (vf28..vf31), culled against the screen and tested for
 * occlusion by G001 (unless noOcclusion); flush() reads the occlusion
 * results back; draw() draws the sun glare, the lights and the sources with
 * distance attenuation 1 / (att0 + att1 d + att2 d^2).  The reflections on
 * the road are a separate pass (RacePS2Base::drawLampReflections: beginReflections,
 * drawReflection per lamp from RaceCarModel::drawLampReflections,
 * endReflections).  Files: race/glare.img, race/flare.bin,
 * race/reflection.img.  The course environment (T178, course_env_set_sun ..
 * course_env_set_reflection) sets the sun, the light parameters, the 8 flare types and the
 * reflection parameters.
 */
#ifndef GAME_T159_FLARE_H
#define GAME_T159_FLARE_H

#include "game/game.h"
#include "engine/gfx/T514_gs_buffer.h"                /* GSBuffer (members occBuf, workBuf) */
#include "game/race/core/T131_race_ps2_render.h"          /* Matrix44 (member proj) */

struct Tex1Image;                               /* G001 (pointers only) */
struct VifModel;                                /* G001 (pointers only) */
class type_info;                                /* T548 */

/* A 3-vector (12 bytes, 4-aligned: the VU0 code moves it through GPRs with
 * unaligned ldl / ldr). */
struct Vec3 {
    f32 x;                                      /* 0x0 */
    f32 y;                                      /* 0x4 */
    f32 z;                                      /* 0x8 */
};
EGCS_SIZE(Vec3, 0xC);

/* A 4-vector (16 bytes, 4-aligned: not a quadword type). */
struct Vec4 {
    f32 x;                                      /* 0x0 */
    f32 y;                                      /* 0x4 */
    f32 z;                                      /* 0x8 */
    f32 w;                                      /* 0xC */
};
EGCS_SIZE(Vec4, 0x10);

/* ? name (u055 / h27; u055 first called it CourseLight).  A car lamp record,
 * 0x80 bytes: the arrays lampsA / B / C of a car model file (CarModelInfo,
 * T455) walked by RaceCarModel::addLamps / drawLampReflections (T180) into
 * addLight / drawReflection. */
struct CarLamp {
    Vec4 axis;                                  /* 0x00 the long axis of an elongated light (M * axis, with w) */
    u8   unk_10[0x10];                          /* 0x10 */
    Vec4 dir;                                   /* 0x20 the direction it shines to */
    Vec3 pos;                                   /* 0x30 the lamp (FlareLightEntry::view, the reflection) */
    u8   unk_3C[4];                             /* 0x3C */
    f32  sizeA;                                 /* 0x40 */
    f32  sizeB;                                 /* 0x44 (sizeA == sizeB within 0.01: round) */
    u8   unk_48[4];                             /* 0x48 */
    Vec3 flarePos;                              /* 0x4C the glow's position */
    f32  r;                                     /* 0x58 */
    f32  g;                                     /* 0x5C */
    f32  b;                                     /* 0x60 */
    u32  exponent;                              /* 0x64 the spot exponent (converted as unsigned) */
    u32  flags;                                 /* 0x68 bit 0: lit by the body's lamp state fraction (list C) */
    /* The enhanced engine (rendering.md 4): Lupo 2's blink fields, read for
     * the lamps of list A (RaceCarModel::addLamps / drawLampReflections, T180;
     * L2 0x001A8BF4 / 0x001A8C14).  0 in every GT3 car model (checked:
     * cars/day, night, eve, menu/cars), so GT3's lamps stay steady. */
    u32  blink;                                 /* 0x6C 0 steady; 1 / 2 the blink pattern (LAMP_BLINK_*, T180);
                                                   bit 0x80 (LAMP_BLINK_ALWAYS): steady, lit even with the lamps off */
    f32  blinkOffset;                           /* 0x70 s added to the pattern's phase */
    u8   unk_74[0xC];                           /* 0x74 */
};
EGCS_SIZE(CarLamp, 0x80);

/* ? name.  A flare source of the course's flare section (T195), 0x20 bytes. */
struct FlareSource {
    int  type;                                  /* 0x00 index into FlareClass::types (0..7) */
    Vec3 pos;                                   /* 0x04 */
    Vec3 dir;                                   /* 0x10 the direction it shines to */
    f32  phase;                                 /* 0x1C flicker phase (cycles) */
};
EGCS_SIZE(FlareSource, 0x20);

/* A flare source projected this frame (FlareClass::sources), 0x18 bytes. */
struct FlareSourceEntry {
    Vec3         view;                          /* 0x00 view-space position */
    f32          facing;                        /* 0x0C directionalFactor (< 0: not visible) */
    int          queried;                       /* 0x10 its occlusion query was issued (u055 visible) */
    FlareSource *src;                           /* 0x14 */
};
EGCS_SIZE(FlareSourceEntry, 0x18);

/* A light projected this frame (FlareClass::lights), 0x24 bytes. */
struct FlareLightEntry {
    Vec3         view;                          /* 0x00 view-space position of light->pos */
    CarLamp *light;                         /* 0x0C */
    int          queried;                       /* 0x10 its occlusion query was issued (u055 visible) */
    f32          intensity;                     /* 0x14 the spot factor times the caller's intensity */
    f32          axisX;                         /* 0x18 the projected axis (elongated lights; else 1, 1) */
    f32          axisY;                         /* 0x1C */
    f32          depth;                         /* 0x20 -0.5 * the view z of light->dir: G001's `near' */
};
EGCS_SIZE(FlareLightEntry, 0x24);

/* The light parameters (FlareClass::params, setLightParams), 0x30 bytes.
 * draw() gives flare_draw_glow (G001) the glow model's scale glowScale *
 * the lamp's sizeA, the halo alpha haloAlpha and the halo scale size *
 * sizeA. */
struct FlareLightParams {
    f32 r;                                      /* 0x00 */
    f32 g;                                      /* 0x04 */
    f32 b;                                      /* 0x08 */
    f32 glowScale;                              /* 0x0C the argument * 8: the glow model's scale (u055 unk0C) */
    f32 size;                                   /* 0x10 the halo's scale; also the culling size (addLight) */
    f32 haloAlpha;                              /* 0x14 the halo's (and the streak's) alpha (u055 unk14) */
    u8  unk_18[4];                              /* 0x18 */
    f32 att0;                                   /* 0x1C 1 / (att0 + att1 d + att2 d^2) */
    f32 att1;                                   /* 0x20 */
    f32 att2;                                   /* 0x24 */
    f32 far;                                    /* 0x28 */
    f32 threshold;                              /* 0x2C arg * 128 (query ignores it) */
};
EGCS_SIZE(FlareLightParams, 0x30);

/* A flare type (FlareClass::types[8], setFlareType), 0x38 bytes.  As
 * FlareLightParams: draw() gives flare_draw_glow glowScale, haloAlpha and size. */
struct FlareType {
    f32 r;                                      /* 0x00 */
    f32 g;                                      /* 0x04 */
    f32 b;                                      /* 0x08 */
    f32 glowScale;                              /* 0x0C the argument * 10: the glow model's scale (u055 unk0C) */
    f32 size;                                   /* 0x10 the halo's scale; also the culling size (addSource) */
    f32 haloAlpha;                              /* 0x14 (u055 unk14) */
    f32 exponent;                               /* 0x18 the directional exponent */
    f32 att0;                                   /* 0x1C */
    f32 att1;                                   /* 0x20 */
    f32 att2;                                   /* 0x24 */
    f32 far;                                    /* 0x28 */
    f32 threshold;                              /* 0x2C arg * 128 (query ignores it) */
    f32 rate;                                   /* 0x30 1 / period; 0: steady */
    f32 phase;                                  /* 0x34 0..1, advanced by update */
};
EGCS_SIZE(FlareType, 0x38);

/* Declared before the class, for GCC 14 only: egcs declares a polymorphic
 * class's __tf<mangled> itself (C++ linkage) when the class is complete and
 * rejects a later extern "C" declaration; an earlier one would be the
 * declaration it uses, and it would then never synthesize the function
 * (u042 / u055). */
#ifndef BOOT_OLD_GCC
extern "C" {
/* The __user_type_info gp-0x4648 (0x003533A8), name "10FlareClass". */
const type_info &__tf10FlareClass(void);        /* 0x0019C328 */
}
#endif

/* vtable 0x0032F220 (1 slot), typeinfo __tf10FlareClass 0x0019C328 (user).
 * The first polymorphic class of its hierarchy: vptr +0x23FC, after the
 * data.  Size 0x2400: a member of RacePS2Base (T130, +0x9830), built by its
 * constructor 0x0017D070.  The first 0x7C bytes are the work area of G001's
 * flare library (FlareWork, u113: flare_init .. flare_draw_sun_disc take
 * `this'): the occlusion queries of the frame are drawn into occBuf (one
 * result texel each), read back into results by flush, and each glow drawn
 * with its query's result.
 *   queries    FlareWork::queries: flare_readback restarts it, every query
 *             and every occluded draw counts one; flush keeps it in queryCount.
 *   flareBin  the glow model of the lamps and sources (relocated by
 *             vif_model_relocate); NULL: flare_sun_model_b.
 *   results    the read-back occlusion results (64 x 8 RGBA32, 0x800 bytes).
 *   sunType   the sun glare's model: 0 flare_sun_model_a (textured with
 *             glareImg, made resident first), 1 flare_rays_model, 2
 *             flare_disc_model. */
class FlareClass {
public:
    /* The images zeroed, the GSBuffers constructed, results = (resultsBuf
     * + 0x3F) & ~0x3F, the three files read (blocking, gt3.vol) and
     * relocated, G001's disc and rays models built, the default light
     * parameters and flare types. */
    FlareClass();                               /* 0x0019B098 */
    /* Frees reflImg, modelTex2, flareBin, glareImg; then the GSBuffers.
     * Callers: RacePS2Base's constructor (EH), RacePS2Base::virtual_0. */
    virtual ~FlareClass();                      /* [0] +0x0C 0x0019B2B8 */

    /* Set: no occlusion queries; whatever passes the culling is drawn
     * unoccluded (flare_draw_glow flags + 2). */
    void setNoOcclusionTest(int on);            /* 0x0019AD00 */
    /* Projected with its type's size and far distance; on screen it joins
     * the list (at most 100). */
    void addSource(FlareSource *src);           /* 0x0019AD08 */
    /* view = M * pos (VU0); screen = the projected x, y and -z.  0 when
     * behind, farther than far, or the size x size square is off screen. */
    int  project(Vec3 *view, Vec3 *screen, const Vec3 *pos, f32 size, f32 far);   /* 0x0019AE30 */
    /* cos(M3x3 * dir, view) ^ (int)(exponent + 0.5); < 0: -1; exponent 0: 1. */
    f32  directionalFactor(const Vec3 *view, const Vec3 *dir, f32 exponent);      /* 0x0019AF68 */
    /* G001's occlusion query flare_query_screen(screen, size * 2, 10.0) (the
     * threshold is not used): 1 when it was issued (its box on screen); the
     * query's result decides whether the glow is hidden when it is drawn.
     * (u055 query.) */
    int  query(const Vec3 *screen, f32 size, f32 threshold);                      /* 0x0019B070 */
    /* The two work buffers' VRAM, flare_init for the screen buffer (aspect
     * 0.911, 1.214 on the wide screen), the lists emptied, proj kept. */
    void begin(GSBuffer &screen);               /* 0x0019B358 */
    void setLightParams(f32 r, f32 g, f32 b, f32 glowScale, f32 size, f32 haloAlpha, f32 att0,
                        f32 att1, f32 att2, f32 far, f32 threshold);  /* 0x0019B460 */
    void setFlareType(int i, f32 r, f32 g, f32 b, f32 glowScale, f32 size, f32 haloAlpha,
                      f32 exponent, f32 att0, f32 att1, f32 att2, f32 far,
                      f32 threshold, f32 period);   /* 0x0019B4B8 */
    /* The sun disc: G001's flare_draw_sun_disc(this, pos, r, g, b, a, scale)
     * (the arguments pass through).  Caller: course_env_draw_sun_disc (T178).  (u055
     * drawSunDisc.) */
    void drawSunDisc(const Vec3 *pos, f32 r, f32 g, f32 b, f32 a, f32 scale);  /* 0x0019B568 */
    /* The sun glare: sunPos, its scale and colour (sun[0..3]) kept for draw;
     * queryScale / querySize ($f12 / $f13) pass through to the occlusion
     * query flare_query_sun (sunVisible: it was issued). */
    void setSun(const Vec3 *pos, f32 queryScale, f32 querySize, f32 scale, f32 r, f32 g,
                f32 b);                         /* 0x0019B588 */
    /* As directionalFactor, dir already in view space. */
    f32  spotFactor(const Vec3 *view, const Vec3 *dir, f32 exponent);    /* 0x0019B5E8 */
    /* light->flarePos projected with params.size * max(sizeA, sizeB); the
     * spot factor only when directional (an elongated light is not).  (The
     * float before the int: the same registers, and RaceCarModel::addLamps,
     * u060, sets $f12 before $a2 only so.) */
    void addLight(CarLamp *light, f32 intensity, int directional);   /* 0x0019B6B0 */
    /* workBuf freed; the occlusion results read back into results
     * (flare_readback) when anything was queried or listed; occBuf freed. */
    void flush();                               /* 0x0019B9B8 */
    /* The sun glare, then the lamps' glows (or streaks) and the sources'. */
    void draw();                                /* 0x0019BA38 */
    void update(f32 dt);                        /* 0x0019BE98 the types' flicker phases */
    void setReflectionParams(f32 r, f32 g, f32 b, f32 width0, f32 width1,
                             f32 maxLength, f32 top);   /* 0x0019BEF0 */
    void beginReflections();                    /* 0x0019BF28 */
    void drawReflection(CarLamp *light, f32 intensity);  /* 0x0019BFE0 */
    void endReflections();                      /* 0x0019C2D0 */
    int  F_0019C318();                          /* 0x0019C318 returns 0 (no callers) */
    void setSunType(int type);                  /* 0x0019C320 (no callers: T178 stores it) */

    u8               unk_000[8];                /* 0x000 G001's FlareWork: occVram, occHeight */
    int              queries;                   /* 0x008 FlareWork::queries: the frame's occlusion queries (u055 queries) */
    u8               unk_00C[0x70];             /* 0x00C the rest of FlareWork (0x7C bytes) */
    Tex1Image       *glareImg;                  /* 0x07C race/glare.img (FlareWork tex[0]: the models' texture 1) */
    VifModel        *flareBin;                  /* 0x080 race/flare.bin, the glow model (NULL: flare_sun_model_b) */
    Tex1Image       *modelTex2;                 /* 0x084 FlareWork tex[1], the models' texture 2 (? never set; u055 modelTex2) */
    Tex1Image       *reflImg;                   /* 0x088 race/reflection.img */
    GSBuffer         occBuf;                    /* 0x08C 64 x 8: the occlusion result strip (flare_init occVram; u055 occBuf) */
    GSBuffer         workBuf;                   /* 0x0C4 64 x 64: the reduction work buffer (workVram; u055 workBuf) */
    u32             *results;                   /* 0x0FC the read-back results, 64-byte aligned in resultsBuf (u055 results) */
    u8               resultsBuf[0x900];         /* 0x100 */
    int              queryCount;                /* 0xA00 the queries of the frame (queries), kept by flush (u055 queryCount) */
    Vec3             sunPos;                    /* 0xA04 the sun's direction (setSun) */
    f32              sun[4];                    /* 0xA10 the sun glare's scale, r, g, b (flare_draw_sun) */
    int              sunVisible;                /* 0xA20 its occlusion query was issued */
    int              sunType;                   /* 0xA24 the sun glare's model (see above) */
    int              noOcclusion;               /* 0xA28 */
    int              nLights;                   /* 0xA2C */
    FlareLightEntry  lights[100];               /* 0xA30 */
    int              nSources;                  /* 0x1840 */
    FlareSourceEntry sources[100];              /* 0x1844 */
    f32              unk21A4;                   /* 0x21A4 proj.m[1][1] */
    Matrix44         proj;                      /* 0x21A8 the projection matrix of the frame */
    FlareLightParams params;                    /* 0x21E8 */
    u8               unk_2218[8];               /* 0x2218 */
    FlareType        types[8];                  /* 0x2220 */
    f32              reflR;                     /* 0x23E0 arg * 128 */
    f32              reflG;                     /* 0x23E4 */
    f32              reflB;                     /* 0x23E8 */
    f32              reflWidth0;                /* 0x23EC the width facing the camera */
    f32              reflWidth1;                /* 0x23F0 the width seen from the side */
    f32              reflMaxLength;             /* 0x23F4 */
    f32              reflTop;                   /* 0x23F8 */
    /*               vptr                          0x23FC */
};
EGCS_SIZE(FlareClass, 0x2400);

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
extern "C" u32 __ti10FlareClass[2];             /* 0x003533A8 gp-0x4648 the __user_type_info */
#endif

#endif /* GAME_T159_FLARE_H */
