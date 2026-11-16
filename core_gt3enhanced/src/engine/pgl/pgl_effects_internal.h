/* pgl_effects_internal.h - unit u113 of the GT3 main executable, 0x00245C10..0x00249B28.
 *
 * The first stretch of the no-FDE graphics library (G001, plain C, the
 * Polyphony "pgl" GS / VU0 driver also linked into the bootstrap).  This
 * unit covers its effect helpers:
 *   vu0_math.c   VU0 atan2, the VU0 and LCG random generators
 *   particles.c  weather particles (a box of drops wrapped around
 *                          the camera, drawn as frame-buffer refraction quads)
 *   flare.c      sun / lens-flare: VIF1 packets of the sun disc
 *                          models, the occlusion test (z-buffer reduction
 *                          into a 1x1 texel) and the glare sprites
 *   tex1.cpp     the 'Tex1' texture set library (relocation, VRAM
 *                          linking, CLUT animation, upload; C++ like the
 *                          bootstrap's src/tex1.cpp, C linkage), the texture
 *                          override switches, the VifModel relocation and
 *                          the ring of CircleGauge (the same stretch)
 *
 * The shared G001 types (GsContext, VifModel, Tex1Image, GsTexRegs ...) and
 * the prototypes of the library come from the canonical engine/pgl/G001_flare.h;
 * this header keeps what only this unit uses.
 */
#ifndef U113_H
#define U113_H

#include "types.h"
#include "eekernel.h"        /* FlushCache, DMAC_GIF, WRITEBACK_DCACHE */
#include "engine/pgl/G001_flare.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Helpers and constants                                                    */
/* ======================================================================== */

/* The bits of a float (mfc1) and back (mtc1). */
static inline u32 u113_f2u(f32 f) { union { f32 f; u32 u; } c; c.f = f; return c.u; }
static inline f32 u113_u2f(u32 u) { union { f32 f; u32 u; } c; c.u = u; return c.f; }

/* The scratchpad context pointer (bootstrap gs_internal.h; u114 / u116 /
 * u118 have the same): loaded whole into a register where retail does,
 * again where it reloads it (egcs: a one-instruction asm; GCC 14 the
 * constant). */
#ifdef BOOT_OLD_GCC
#define GS_SPR_CTX ({ GsContext *ctx_; __asm__("li %0, 0x70003000" : "=r"(ctx_)); ctx_; })
#else
#define GS_SPR_CTX GS_CTX
#endif

/* The draw buffer GsContext.tgtA as retail reads it: through a union, i.e.
 * in alias set 0, so a load of its offsets stays behind every earlier store
 * (flare_project loads yoff only after the *sx store).  pgl_font_internal.h has the same
 * view (font_set_start, font_set_pen); what union the original had is not
 * known, this one has the header's layout.  GCC 14 builds without strict
 * aliasing: the same code there. */
typedef union GsTargetView { GsRenderTarget t; } GsTargetView;
#define GS_TGT_A(ctx) (((GsTargetView *)&(ctx)->tgtA)->t)

/* sqrt.s without the errno path that egcs adds to __builtin_sqrtf outside
 * -ffast-math, and in its source position (volatile): weather_draw. */
static inline f32 u113_sqrtf(f32 x)
{
    f32 r;

    __asm__ volatile("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}

/* GsContext.matrixMode (PGL_MTX_*): the matrix held in vf28..vf31, the
 * others parked in VU0 data memory at qword 4 + 4 * mode. */
#define PGL_VU0_PARKED(mode)  (4 + 4 * (mode))

/* The gfx_enable switches (PGL_CAP_*), the gfx_tex_param pnames and values
 * (PGL_TEXPARAM_*, PGL_TFX_*, PGL_FILTER_*, PGL_WRAP_*), the blend modes
 * (PGL_BLEND_*) and the pgl_push_attrib groups (PGL_ATTR_*): G001_flare.h. */

/* GsContext.dirty: per-context register r is bit r (context 1) and 16 + r
 * (context 2); general register g is bit 32 + g. */
#define GS_DIRTY_ALL_CTX1_GEN   ((((1ull << GS_GEN_COUNT) - 1) << 32) | ((1ull << GS_REG_COUNT) - 1))

/* DMA tags and VIF codes: G001_flare.h.  The UNPACK formats (VIF_UNPACK:
 * FLG, addresses relative to TOPS, and USN set): */
enum { VIF_S_32 = 0x60, VIF_S_8 = 0x62, VIF_V2_32 = 0x64, VIF_V3_32 = 0x68, VIF_V4_32 = 0x6C, VIF_V4_8 = 0x6E };

/* The VU1 model program's entry and its data addresses (vif_model_draw). */
#define FLARE_VU1_ENTRY         0x3C    /* MSCAL GsContext.vu1Prog + 0x3C */
#define FLARE_VU1_ITOP          0x2F7

/* ======================================================================== */
/* vu0_math.c                                                     */
/* ======================================================================== */

int  lcg_rand_chance(f32 p);                /* 0x00245E10 (no references) */

extern u32 g_003530AC;                      /* gp-0x4944, the LCG state (init 0) */
extern const f32 g_00343CE8[8];             /* atan2 octant signs  { 1,-1,-1, 1,-1, 1, 1,-1} */
extern const f32 g_00343D08[10];            /* atan2 octant bases  { 0, pi/2, pi, pi/2, 0, -pi/2, -pi, -pi/2, 0, 0 } */

/* ======================================================================== */
/* particles.c                                                    */
/* ======================================================================== */

/* One weather particle (0x20 bytes; RacePS2Base.weatherDrops, malloc(0x6400):
 * room for 800). */
typedef struct WeatherDrop {
    f32 pos[4];         /* 0x00 x, y, z, 1 */
    f32 vel[4];         /* 0x10 x, y, z, 0 (added to pos every frame) */
} WeatherDrop;

/* The particle box (0x34 bytes, RacePS2Base.weather, malloc(0x40)). */
typedef struct Weather {
    f32 center[4];      /* 0x00 the camera point the box is wrapped around */
    f32 size;           /* 0x10 drop size (8.0) */
    f32 spreadXZ;       /* 0x14 box width x / z (32.0) */
    f32 spreadY;        /* 0x18 box height (8.0) */
    f32 ahead;          /* 0x1C distance of the box centre in front of the eye (16.0) */
    f32 velXZ;          /* 0x20 random x velocity range (0.5) */
    f32 velY;           /* 0x24 y velocity (0.01) */
    f32 velZ;           /* 0x28 random z velocity range (0.5) */
    WeatherDrop *drops; /* 0x2C */
    s32 count;          /* 0x30 */
} Weather;

void weather_drop_init(WeatherDrop *d, f32 spreadXZ, f32 spreadY, f32 velXZ,
                       f32 velY, f32 velZ);                     /* 0x00245E58 */
void weather_draw_drop(WeatherDrop *d, Weather *w, f32 period, f32 periodY,
                       f32 size);                               /* 0x002465B8 */

/* ======================================================================== */
/* flare.c                                                        */
/* ======================================================================== */

/* One vertex pair of the rays table g_00343D30 (0x30 bytes). */
typedef struct FlareRayVert {
    f32 p0[4];              /* 0x00 x, y used (scaled by 1/32) */
    f32 p1[4];              /* 0x10 x, y used */
    s32 rgb[3];             /* 0x20 r, g, b (0..255) */
    s32 a;                  /* 0x2C (128 in the table; not read) */
} FlareRayVert;

/* The flare work area (0x7C bytes, the start of FlareClass, T159, whose
 * methods pass `this'). */
typedef struct FlareWork {
    s32  occVram;           /* 0x00 VRAM word address of the 64 x h result strip */
    s32  occHeight;         /* 0x04 */
    s32  queries;           /* 0x08 occlusion queries issued / results drawn this frame (FlareClass::queries) */
    s32  screenVram;        /* 0x0C VRAM word address of the screen (FlareClass::begin: GSBuffer.addr);
                             *      the queries draw into its alpha and copy it from there */
    s32  screenWidth;       /* 0x10 GSBuffer.width */
    s32  screenHeight;      /* 0x14 GSBuffer::displayHeight() */
    s32  width;             /* 0x18 screen width (clip) */
    s32  height;            /* 0x1C screen height (clip) */
    s32  workVram;          /* 0x20 VRAM word address of the reduction buffer */
    s32  workWidth;         /* 0x24 */
    s32  workHeight;        /* 0x28 */
    Tex1Image *tex[2];      /* 0x2C the textures of VifPart.tex 1 / 2 (FlareClass glareImg, modelTex2) */
    f32  aspect;            /* 0x34 */
    u32 *results;           /* 0x38 the read-back result texels (RGBA32; red = visibility x scale) */
    u32 *reduce[16];        /* 0x3C prebuilt reduction packets (DMA call targets), n = 2..17 */
} FlareWork;

f32  flare_project(const f32 *pos, f32 *sx, f32 *sy);           /* 0x00246D30 */
void flare_fill_octagon(f32 cx, f32 cy, f32 r, f32 aspect);     /* 0x00246E60 */
void flare_set_target(u32 fbWordAddr, int w, int h);            /* 0x00246FC0 */
void flare_set_texture(u32 texWordAddr, int w, int h);          /* 0x00247020 */
void flare_copy_visibility(FlareWork *fw, int x0, int y0, int x1, int y1); /* 0x002470C0 */
void flare_reduce(FlareWork *fw, int w, int h);                 /* 0x002471D8 */
u32 *flare_build_reduce(FlareWork *fw, int n);                  /* 0x00247340 */
void flare_run_reduce(FlareWork *fw, int w, int h);             /* 0x00247518 */
void flare_store_result(FlareWork *fw, f32 scale, int w, int h); /* 0x002475C8 */
int  flare_occlusion_query(FlareWork *fw, int octagon, f32 sx, f32 sy,
                           f32 size, f32 scale);                /* 0x00247AA0 */
void flare_draw_result_disc(FlareWork *fw, VifModel *m, f32 r, f32 g,
                            f32 b);                             /* 0x00247DA0 */
void flare_draw_disc(FlareWork *fw, VifModel *m, f32 r, f32 g, f32 b,
                     f32 intensity);                            /* 0x00247DF0 */
int  flare_query_point(FlareWork *fw, f32 scale, f32 size);     /* 0x00247E18 */

/* The pgl_push_attrib groups the occlusion code saves (0x887). */
#define FLARE_SAVED_ATTRIBS (PGL_ATTR_SCISSOR | PGL_ATTR_VIEWPORT | PGL_ATTR_TARGET_B | \
                             PGL_ATTR_DEPTH | PGL_ATTR_DRAW_BUFFER)

/* (The models built at start-up, g_004A5240 / g_004A56A0 / g_004A56C0 /
 * g_004A6840, are file statics of flare.c: .bss.) */
extern VifModel     *g_003530B0;            /* 0x003530B0 gp-0x4940, = g_004A6840 (rays model header, .bss) */
extern VifModel      g_002F5BD0;            /* 0x002F5BD0 prebuilt sun model a (.data: 1 part, 4 vertices, texture 1) */
extern VifModel      g_002F5D48;            /* 0x002F5D48 prebuilt sun model b (.data: 1 part, 12 vertices, texture 2) */
extern const FlareRayVert g_00343D30[72];   /* 0x00343D30 rays vertex table (.rodata) */
extern const u32     g_00344AB0[4];         /* 0x00344AB0 0xFF x 4: four 'fully visible' results (flare_draw_disc) */
extern const f32     g_00344AC0[4];         /* 0x00344AC0 (0, 0, 0, 0): the point of flare_query_point */

/* ======================================================================== */
/* tex1.cpp - the 'Tex1' texture set (layout from the bootstrap's  */
/* include/startup_screen.h, same code)                                     */
/* ======================================================================== */

typedef struct ClutKey {
    f32  time;              /* 0x00 duration; negative = hold on this key */
    u32 *palette;           /* 0x04 nColors RGBA32 colours (relocated) */
} ClutKey;

typedef struct ClutAnimChannel {
    u16      id;            /* 0x00 */
    u16      mode;          /* 0x02 0 loop, 1 ping-pong, 2 once, else hold */
    u16      nColors;       /* 0x04 */
    u16      nKeys;         /* 0x06 */
    u32     *dest;          /* 0x08 destination CLUT colours (relocated) */
    ClutKey *keys;          /* 0x0C (relocated) */
    u16      cur;           /* 0x10 */
    u16      next;          /* 0x12 */
    f32      time;          /* 0x14 */
    f32      lerp;          /* 0x18 weight of keys[cur] */
    s16      hold;          /* 0x1C > 0: frozen */
    s8       dir;           /* 0x1E ping-pong direction */
    u8       dirty;         /* 0x1F */
} ClutAnimChannel;

/* Tex1Image.clutAnim */
typedef struct ClutAnimTable {
    u16              count; /* 0x00 */
    u16              pad02;
    ClutAnimChannel *ch;    /* 0x04 (relocated) */
} ClutAnimTable;

void clut_anim_channel_reset(ClutAnimChannel *ch);              /* 0x00248FF8 */
void clut_anim_channel_apply(ClutAnimChannel *ch);              /* 0x00249048 */
void clut_anim_table_relocate(ClutAnimTable *tbl, s32 delta);   /* 0x002495B8 */

/* The ring of gt2k_menu::CircleGauge (same file; its transformed point
 * g_004A6870 is a file static there, .bss) */
extern const f32 g_00344AD0[4];             /* 0x00344AD0 (1, 0, 0, 0) (.rodata) */

/* ======================================================================== */
/* Outside the unit                                                         */
/* ======================================================================== */

/* newlib (FlushCache 0x00285EE0: eekernel.h) */
double cos(double x);                       /* 0x00274F70 (newlib) */
double sin(double x);                       /* 0x00275078 (newlib) */

#ifdef __cplusplus
}
#endif

#endif /* U113_H */
