/* T173_race_smoke.h - RaceSmoke, the tyre smoke / dust particles of a race
 * (translation unit T173, unit u058), 0x001A45A8..0x001A6138.
 *
 * 256 particles of 0x40 bytes in one memalign'ed block, on two lists
 * (active, free) linked by s16 indices.  RacePS2Base holds it (+0xBC30,
 * T130) and hands the course a pointer (RaceCourse::smoke, T153);
 * RacePS2Base::virtual_8 resets it and gives it the course's smoke texture
 * (CourseData::smokeTex) and environment (CourseData::env, T178: 16 smoke
 * types of 0x44 bytes, the intensity floor, the resolution divisors, the
 * blend mode).  Per frame: tyre_smoke_emit (T334) emits one particle per
 * wheel through RaceCourse::emitSmoke (emit), RacePS2Base::virtual_28 moves
 * them (update: spin, growth, fade, rise and gravity; the faded ones and
 * the ones fallen under their ground plane are freed), RacePS2Base::
 * drawView / drawSubView draw them (render: at the full resolution, or into a
 * reduced offscreen buffer that is then added over the frame), drawMirror
 * draws them straight (draw(1)).
 *
 * draw transforms the particles to view space with the VU0 matrix
 * (vf28..vf31), culls them against the depth range and the screen, sorts
 * them back to front (unless the blend is additive) and writes one GIF
 * packet: a camera-facing quad per particle (strips of RGBAQ / ST / XYZF,
 * projected with vf20..vf27), its texture one of the four quarters of the
 * smoke texture.  The small particles (size <= 3) are split by their
 * ground plane: the part above is drawn as is, the part below is projected
 * onto the ground (along the view ray, its spread clamped to twice the
 * size) and fades to alpha 0.
 *
 * Names: u058's (u044 / u045 / u050 used the same ones for what they
 * declared: reset, setTexture, update, render, draw, emit, texture, env).
 */
#ifndef GAME_T173_RACE_SMOKE_H
#define GAME_T173_RACE_SMOKE_H

#include "game/game.h"
#include "game/effects/T159_flare.h"                /* Vec3, Vec4 (held by value) */

struct Tex1Image;                           /* G001 (pointers only: forward declarations) */
struct Matrix44;                            /* T131 */
class CourseEnv;                            /* T178 */
class SmokeSource;                          /* T334 */
class GSBuffer;                             /* T514 */

/* A particle, 0x40 bytes.  Not polymorphic. */
struct SmokeParticle {
    s16  next;                              /* 0x00 the next of its list (active / free), -1: none */
    u8   flags;                             /* 0x02 bits 0..2: the variant (g_00330730 / g_00330750),
                                               bit 5: spins the other way (SmokeSource::flags bit 0) */
    u8   type;                              /* 0x03 index of CourseEnv::smoke */
    u32  color;                             /* 0x04 the type's rgb x the intensity (x 128), alpha 0 */
    f32  size;                              /* 0x08 half the side of its quad */
    f32  alpha;                             /* 0x0C <= 0: freed */
    f32  alphaScale;                        /* 0x10 ramped from alpha to 1 by SmokeType::alphaScale */
    f32  angle;                             /* 0x14 degrees, [0, 360] */
    f32  vy;                                /* 0x18 the vertical speed */
    u16  fadeRate;                          /* 0x1C x 1/256: the fade's factor (random by SmokeType::period) */
    u8   growRate;                          /* 0x1E x 1/128: the growth's factor (0.9 .. 1.1) */
    u8   unk_1F;                            /* 0x1F random 0..63 (emit; nothing in T173 reads it) */
    Vec3 pos;                               /* 0x20 course coordinates */
    f32  ground;                            /* 0x2C the plane's y under pos: below it (top), freed */
    Vec4 plane;                             /* 0x30 the ground plane (normal, d) */
};
EGCS_SIZE(SmokeParticle, 0x40);

/* A visible particle (draw's stack array, 256), 0x10 bytes. */
struct SmokeVis {
    Vec3 pos;                               /* 0x0 view space */
    int  index;                             /* 0xC of RaceSmoke::particles */
};
EGCS_SIZE(SmokeVis, 0x10);

/* Where an edge of a clipped quad crosses the ground plane (on the stack of
 * drawClippedQuad, 0x20 apart), 0x14 bytes. */
struct SmokeEdge {
    Vec3 pos;                               /* 0x00 view space */
    f32  uv[2];                             /* 0x0C */
};
EGCS_SIZE(SmokeEdge, 0x14);

/* Not polymorphic.  Size 0x60 (RacePS2Base +0xBC30 .. +0xBC90). */
class RaceSmoke {
public:
    /* The particle block (memalign(0x40, 0x4000)), no texture, reset().
     * Caller: RacePS2Base's constructor. */
    RaceSmoke();                            /* 0x001A45A8 */
    /* Frees the particle block. */
    ~RaceSmoke();                           /* 0x001A45E8 */
    /* No active particle, the free list 0 .. 255, the emission counters
     * cleared, the VU0 random seed 0.1.  Callers: RacePS2Base::virtual_8,
     * gt2k_menu::LoggerControl::runPage (T055), the constructor. */
    void reset();                           /* 0x001A4630 */
    /* NULL: nothing is drawn.  Caller: RacePS2Base::virtual_8. */
    void setTexture(Tex1Image *tex);        /* 0x001A46C0 texture = tex */
    /* One particle of the smoke type of src->kind (none when the type's
     * density is < 0.01 or src->rate <= 0.001): from the free list, or the
     * weakest active one is reused.  Caller: RaceCourse::emitSmoke (T153). */
    void emit(const SmokeSource *src);      /* 0x001A46C8 */
    /* The active particle with the smallest alpha x alphaScale (0 if none). */
    int  findWeakest();                     /* 0x001A4A40 */
    /* One step of dt for every active particle.  Caller: RacePS2Base::
     * virtual_28. */
    void update(f32 dt);                    /* 0x001A4A98 */
    /* draw(mode) at the full resolution when both divisors of the course
     * are 1; else draw(1) into a reduced offscreen colour buffer (with
     * depth's Z stretched into a reduced depth buffer), then that buffer
     * added over (0, 0) .. depth's size.  Callers: RacePS2Base::drawView
     * / 47 (T131). */
    void render(GSBuffer *depth, int mode); /* 0x001A4CC8 */
    /* Nothing without a texture.  mode 0: the particles of size > 3 as
     * plain quads, the others clipped at their ground; 1: all plain; other
     * modes: no quads.  Callers: RacePS2Base::drawMirror (T131), render. */
    void draw(int mode);                    /* 0x001A4F28 */
    /* A GIF REGLIST strip of n vertices (positions in view space, their
     * uvs): rgba0, then rgba1 from the third vertex. */
    void emitStrip(const Vec3 *pos, const f32 (*uv)[2], u32 rgba0, u32 rgba1,
                   int n);                  /* 0x001A5438 */
    /* The particle's camera-facing quad. */
    void drawQuad(const SmokeVis *v);       /* 0x001A55E0 */
    /* A quad in loop order drawn as a strip (corners 2 and 3 swapped in q). */
    void drawLoopQuad(Vec3 *q, u32 rgba, int uvSet);    /* 0x001A56F0 */
    /* The particle's quad split by its ground plane. */
    void drawClippedQuad(const SmokeVis *v);            /* 0x001A5760 */
    /* The particle's colour with its alpha (x the fog factor with
     * CourseEnv::flags 0x40000), at most 0x80. */
    u32  color(const SmokeVis *v);          /* 0x001A5B50 */
    /* in moved along the view ray onto clipPlane. */
    void projectToPlane(Vec3 *out, const Vec3 *in);     /* 0x001A5C08 */
    /* Where the edge clipQuad[i] -> clipQuad[(i + 1) & 3] meets clipPlane,
     * with its uv; returns the edge fraction. */
    f32  edgeCross(SmokeEdge *out, int i);  /* 0x001A5C70 */
    /* v pulled towards e->pos to a distance of max if it is farther. */
    void clampLength(const SmokeEdge *e, Vec3 *v, f32 max);     /* 0x001A5D70 */
    /* A polygon in loop order: first, second, then the corners
     * verts[(from + 1) & 3 ..] up to (to + 1) & 3 (excluded) with their
     * clipUV.  Returns its vertex count. */
    int  buildPolygon(int from, int to, Vec3 *pos, f32 (*uv)[2],
                      const SmokeEdge *second, const SmokeEdge *first,
                      const Vec3 *verts);   /* 0x001A5DF0 */
    /* Loop order 0, 1, .., n - 1 into strip order 0, 1, n - 1, 2, .. (n >= 4). */
    void toStrip(int n, Vec3 *pos, f32 (*uv)[2]);       /* 0x001A5EF0 */
    /* Quicksort of v[lo..hi] by the view z, ascending (the farthest first). */
    void sortByDepth(SmokeVis *v, int lo, int hi);      /* 0x001A5FB8 */

    SmokeParticle *particles;               /* 0x00 [256] (memalign 0x40, 0x4000 bytes) */
    s16            activeHead;              /* 0x04 -1: none */
    s16            freeHead;                /* 0x06 -1: none */
    u32            emitCount[11];           /* 0x08 per smoke type: counts the emissions, wraps after
                                               SmokeType::period (nothing reads it) */
    Tex1Image     *texture;                 /* 0x34 NULL: not drawn */
    CourseEnv     *env;                     /* 0x38 the course's (smoke types, divisors, blend) */
    const f32    (*clipUV)[2];              /* 0x3C the uvs of the quad being clipped (g_003307F0) */
    Vec3          *clipQuad;                /* 0x40 the quad being clipped (drawClippedQuad's stack) */
    Vec4          *clipPlane;               /* 0x44 its ground plane in view space (the same) */
    Matrix44      *planeMatrix;             /* 0x48 the inverse transpose of the view matrix (draw's stack) */
    u64           *pkStart;                 /* 0x4C the GIF packet (gfx_open_direct) */
    u32           *pkCur;                   /* 0x50 its write pointer */
    f32            fog[3];                  /* 0x54 pgl_get_fog_constants: the fog's offset, scale and start */
};
EGCS_SIZE(RaceSmoke, 0x60);

/* T173's tables. */
extern const int g_003306D0[24];            /* 0x003306D0 the smoke type of a SmokeSource::kind (the surface): 0..5 */
extern const f32 g_00330730[8];             /* 0x00330730 the alpha of a variant (x the source's rate) */
extern const f32 g_00330750[8];             /* 0x00330750 the spin of a variant (x the type's spin / size) */
/* The uvs of a quad's corners for each of the four images (the vertical
 * quarters of the texture), in strip order and in loop order. */
extern const f32 g_00330770[4][4][2];       /* 0x00330770 strip order (drawQuad, drawLoopQuad) */
extern const f32 g_003307F0[4][4][2];       /* 0x003307F0 loop order (drawClippedQuad) */

#endif /* GAME_T173_RACE_SMOKE_H */
