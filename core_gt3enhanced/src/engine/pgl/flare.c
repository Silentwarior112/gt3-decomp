/* flare.c - the lens-flare / sun-glare part of the graphics
 * library (G001), 0x002466F0..0x002487B8.  The work area is the start of
 * FlareClass (T159), which passes `this' to everything here.
 *
 * Models: VifModel packets drawn by the VU1 model program (MSCAL
 * GsContext.vu1Prog + 0x3C, vif_model_draw): two prebuilt ones in .data
 * (sun model a / b) and two built at start-up into .bss: a 14-triangle disc
 * (white centre, black rim) and 72 rays from the table g_00343D30.  Each
 * draw also hands the program a group of 4 occlusion results with an index
 * and an (r, g, b) colour.
 *
 * Occlusion (flare_occlusion_query): only the screen's alpha is written.
 * The alpha of a size x size box around the point is cleared, then a
 * square (or an octagon) of alpha 2.0 is drawn over it with the depth test
 * on: alpha is set where the flare is not hidden.  The box is copied from
 * the screen into the work buffer (64 x 64) and reduced by bilinear halving
 * sprites to one texel (flare_run_reduce: prebuilt DMA-call packets for
 * n < 18), which is then drawn, scaled by scale / size^2, into texel
 * (q % 64, q / 64) of the 64 x h result strip.  flare_readback reads the
 * strip back; vif_model_draw gives the VU1 program the result of query q as
 * the group &results[q & ~3] and the index q & 3 (flare_draw_result_disc).
 */
#include "engine/pgl/G001_flare.h"   /* this translation unit (G001) */
#include "vu0asm.h"
#include "engine/pgl/pgl_effects_internal.h"

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* The prebuilt sun models (flare_sun_model_a / _b): each a VIF1 packet for
 * the VU1 model program (the layout of flare_build_disc's: GIFtag, xyz,
 * strip table, masked ST, colours) in front of its VifModel header.  The
 * packets go out by DMA (vif_model_draw: a ref tag), so they are
 * qword-aligned. */
// DATA_START 0x002F5B40
static u32 g_002F5B40[36] __attribute__((aligned(16))) = {    /* 0x002F5B40 */
    VIF_UNPACK(VIF_V4_32, 1, 0xC0),
    0x00008004, 0x302E4000, 0x00000412, 0x00000000,     /* GIFtag NLOOP 4, EOP, PRE, PRIM 0x5C, ST RGBAQ XYZF2 */
    VIF_UNPACK(VIF_V3_32, 4, 0),                    /* xyz (1, 1, 0) (-1, 1, 0) (1, -1, 0) (-1, -1, 0) */
    0x3F800000, 0x3F800000, 0x00000000, 0xBF800000,
    0x3F800000, 0x00000000, 0x3F800000, 0xBF800000,
    0x00000000, 0xBF800000, 0xBF800000, 0x00000000,
    VIF_UNPACK(VIF_S_8, 2, 0x40), 0x00000601,          /* the strip table */
    VIF_UNPACK_M(VIF_V2_32, 4, 0x40),               /* ST (0, 0) (1, 0) (0, 1) (1, 1) */
    0x00000000, 0x00000000, 0x3F800000, 0x00000000,
    0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,
    VIF_UNPACK(VIF_V4_8, 4, 0x80),                  /* RGBA */
    0xFF808080, 0xFF808080, 0xFF808080, 0xFF808080,
    0x00000000, 0x00000000
};
VifModel g_002F5BD0 = {                 /* 0x002F5BD0 */
    NULL, NULL, 0, 0, 1, 4, 2, { { g_002F5B40, 9, 1 } }
};
/* (The array is two words longer than the part's 0x15 qwords: the zeros in
 * front of g_002F5D48.) */
static u32 g_002F5BF0[86] __attribute__((aligned(16))) = {    /* 0x002F5BF0 */
    VIF_UNPACK(VIF_V4_32, 1, 0xC0),
    0x0000800C, 0x302E4000, 0x00000412, 0x00000000,     /* GIFtag NLOOP 12, EOP, PRE, PRIM 0x5C, ST RGBAQ XYZF2 */
    VIF_UNPACK(VIF_V3_32, 12, 0),                   /* xyz */
    0x3F800000, 0x3F800000, 0x00000000, 0x00000000,
    0x3F800000, 0x00000000, 0x3F800000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x3F800000, 0xBF800000, 0x00000000, 0x00000000,
    0xBF800000, 0x00000000, 0x00000000, 0x3F800000,
    0x00000000, 0xBF800000, 0x3F800000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xBF800000,
    0x00000000, 0x00000000, 0x00000000, 0xBF800000,
    0x00000000, 0xBF800000, 0xBF800000, 0x00000000,
    VIF_UNPACK(VIF_S_8, 3, 0x40), 0x000C0C02,          /* the strip table */
    VIF_UNPACK_M(VIF_V2_32, 12, 0x40),              /* ST */
    0x00000000, 0x00000000, 0x3F800000, 0x00000000,
    0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,
    0x00000000, 0x00000000, 0x3F800000, 0x00000000,
    0x3F800000, 0x00000000, 0x00000000, 0x00000000,
    0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,
    0x3F800000, 0x00000000, 0x00000000, 0x00000000,
    VIF_UNPACK(VIF_V4_8, 12, 0x80),                 /* RGBA */
    0x80808080, 0x80808080, 0x80808080, 0x80808080,
    0x80808080, 0x80808080, 0x80808080, 0x80808080,
    0x80808080, 0x80808080, 0x80808080, 0x80808080,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000
};
VifModel g_002F5D48 = {                 /* 0x002F5D48 */
    NULL, NULL, 0, 0, 1, 12, 8, { { g_002F5BF0, 0x15, 2 } }
};
/* The rays: 72 triangles from the centre to p0 and p1 (x, y used, scaled by
 * 1/32 in flare_build_rays) with a grey level. */
const FlareRayVert g_00343D30[72] = {   /* 0x00343D30 */
    { { 8.407244f, 30.875853f, 0.017418f, 1.0f }, { 29.874966f, -11.466752f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { -22.53565f, 22.718813f, 0.017418f, 1.0f }, { 24.867983f, 20.139105f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { -30.942894f, -8.15704f, 0.017418f, 1.0f }, { -5.006985f, 31.605856f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { -8.407244f, -30.875853f, 0.017418f, 1.0f }, { -29.874966f, 11.466752f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { 22.53565f, -22.718813f, 0.017418f, 1.0f }, { -24.867983f, -20.139105f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { 30.942894f, 8.15704f, 0.017418f, 1.0f }, { 5.006985f, -31.605856f, 0.017418f, 1.0f }, { 87, 87, 87 }, 128 },
    { { 10.792047f, 30.125267f, 0.030065f, 1.0f }, { 25.534002f, -19.287682f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { -20.693224f, 24.40882f, 0.030065f, 1.0f }, { 29.470625f, 12.469254f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { -31.48527f, -5.716447f, 0.030065f, 1.0f }, { 3.936622f, 31.756935f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { -10.792047f, -30.125267f, 0.030065f, 1.0f }, { -25.534002f, 19.287682f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { 20.693224f, -24.40882f, 0.030065f, 1.0f }, { -29.470625f, -12.469254f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { 31.48527f, 5.716447f, 0.030065f, 1.0f }, { -3.936622f, -31.756935f, 0.030065f, 1.0f }, { 118, 118, 118 }, 128 },
    { { -12.739069f, 29.355001f, 0.02812f, 1.0f }, { 31.819725f, 3.391915f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { -31.791712f, 3.645144f, 0.02812f, 1.0f }, { 12.972378f, 29.252647f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { -19.052643f, -25.709858f, 0.02812f, 1.0f }, { -18.847347f, 25.860733f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { 12.739069f, -29.355001f, 0.02812f, 1.0f }, { -31.819725f, -3.391915f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { 31.791712f, -3.645144f, 0.02812f, 1.0f }, { -12.972378f, -29.252647f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { 19.052643f, 25.709858f, 0.02812f, 1.0f }, { 18.847347f, -25.860733f, 0.02812f, 1.0f }, { 86, 86, 86 }, 128 },
    { { -11.375185f, 29.90995f, 0.030153f, 1.0f }, { 31.984882f, 0.983508f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { -31.59037f, 5.103777f, 0.030153f, 1.0f }, { 15.140698f, 28.191475f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { -20.215185f, -24.806173f, 0.030153f, 1.0f }, { -16.844185f, 27.207968f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { 11.375185f, -29.90995f, 0.030153f, 1.0f }, { -31.984882f, -0.983508f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { 31.59037f, -5.103777f, 0.030153f, 1.0f }, { -15.140698f, -28.191475f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { 20.215185f, 24.806173f, 0.030153f, 1.0f }, { 16.844185f, -27.207968f, 0.030153f, 1.0f }, { 65, 65, 65 }, 128 },
    { { 8.837571f, 30.755444f, 0.025949f, 1.0f }, { 29.663252f, -12.003814f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { -22.21621f, 23.031282f, 0.025949f, 1.0f }, { 25.227234f, 19.687222f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { -31.053782f, -7.724161f, 0.025949f, 1.0f }, { -4.436018f, 31.691036f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { -8.837571f, -30.755444f, 0.025949f, 1.0f }, { -29.663252f, 12.003814f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { 22.21621f, -23.031282f, 0.025949f, 1.0f }, { -25.227234f, -19.687222f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { 31.053782f, 7.724161f, 0.025949f, 1.0f }, { 4.436018f, -31.691036f, 0.025949f, 1.0f }, { 76, 76, 76 }, 128 },
    { { 13.160305f, 29.168585f, 0.024217f, 1.0f }, { 26.983088f, -17.202122f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { -18.680582f, 25.981451f, 0.024217f, 1.0f }, { 28.389017f, 14.766979f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { -31.840887f, -3.187134f, 0.024217f, 1.0f }, { 1.40593f, 31.9691f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { -13.160305f, -29.168585f, 0.024217f, 1.0f }, { -26.983088f, 17.202122f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { 18.680582f, -25.981451f, 0.024217f, 1.0f }, { -28.389017f, -14.766979f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { 31.840887f, 3.187134f, 0.024217f, 1.0f }, { -1.40593f, -31.9691f, 0.024217f, 1.0f }, { 122, 122, 122 }, 128 },
    { { 11.573747f, 29.83368f, 0.019116f, 1.0f }, { 28.788134f, -13.972952f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { -20.04985f, 24.939999f, 0.019116f, 1.0f }, { 26.494999f, 17.944778f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { -31.623596f, -4.89368f, 0.019116f, 1.0f }, { -2.293135f, 31.91773f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { -11.573747f, -29.83368f, 0.019116f, 1.0f }, { -28.788134f, 13.972952f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { 20.04985f, -24.939999f, 0.019116f, 1.0f }, { -26.494999f, -17.944778f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { 31.623596f, 4.89368f, 0.019116f, 1.0f }, { 2.293135f, -31.91773f, 0.019116f, 1.0f }, { 73, 73, 73 }, 128 },
    { { 18.77954f, 25.910015f, 0.024284f, 1.0f }, { 23.39734f, -21.830357f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { -13.048963f, 29.218567f, 0.024284f, 1.0f }, { 30.604313f, 9.347514f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { -31.8285f, 3.308549f, 0.024284f, 1.0f }, { 7.206972f, 31.17787f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { -18.77954f, -25.910015f, 0.024284f, 1.0f }, { -23.39734f, 21.830357f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { 13.048963f, -29.218567f, 0.024284f, 1.0f }, { -30.604313f, -9.347514f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { 31.8285f, -3.308549f, 0.024284f, 1.0f }, { -7.206972f, -31.17787f, 0.024284f, 1.0f }, { 120, 120, 120 }, 128 },
    { { 4.379858f, 31.698847f, 0.029573f, 1.0f }, { 31.100872f, -7.532313f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { -25.262077f, 19.64249f, 0.029573f, 1.0f }, { 22.07361f, 23.16799f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { -29.641935f, -12.056355f, 0.029573f, 1.0f }, { -9.027261f, 30.700302f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { -4.379858f, -31.698847f, 0.029573f, 1.0f }, { -31.100872f, 7.532313f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { 25.262077f, -19.64249f, 0.029573f, 1.0f }, { -22.07361f, -23.16799f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { 29.641935f, 12.056355f, 0.029573f, 1.0f }, { 9.027261f, -30.700302f, 0.029573f, 1.0f }, { 107, 107, 107 }, 128 },
    { { 19.402708f, 25.446709f, 0.015396f, 1.0f }, { 20.802877f, -24.315434f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { -12.336142f, 29.526592f, 0.015396f, 1.0f }, { 31.459223f, 5.858104f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { -31.73885f, 4.079884f, 0.015396f, 1.0f }, { 10.656344f, 30.173536f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { -19.402708f, -25.446709f, 0.015396f, 1.0f }, { -20.802877f, 24.315434f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { 12.336142f, -29.526592f, 0.015396f, 1.0f }, { -31.459223f, -5.858104f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { 31.73885f, -4.079884f, 0.015396f, 1.0f }, { -10.656344f, -30.173536f, 0.015396f, 1.0f }, { 110, 110, 110 }, 128 },
    { { 13.733026f, 28.903357f, 0.019286f, 1.0f }, { 28.449583f, -14.649954f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { -18.164528f, 26.344828f, 0.019286f, 1.0f }, { 26.912025f, 17.313086f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { -31.897554f, -2.558529f, 0.019286f, 1.0f }, { -1.53756f, 31.96304f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { -13.733026f, -28.903357f, 0.019286f, 1.0f }, { -28.449583f, 14.649954f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { 18.164528f, -26.344828f, 0.019286f, 1.0f }, { -26.912025f, -17.313086f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { 31.897554f, 2.558529f, 0.019286f, 1.0f }, { 1.53756f, -31.96304f, 0.019286f, 1.0f }, { 114, 114, 114 }, 128 },
    { { -10.846549f, 30.105686f, 0.021211f, 1.0f }, { 31.987257f, 0.902974f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
    { { -31.495564f, 5.659456f, 0.021211f, 1.0f }, { 15.21163f, 28.153265f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
    { { -20.649015f, -24.446232f, 0.021211f, 1.0f }, { -16.775627f, 27.25029f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
    { { 10.846549f, -30.105686f, 0.021211f, 1.0f }, { -31.987257f, -0.902974f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
    { { 31.495564f, -5.659456f, 0.021211f, 1.0f }, { -15.21163f, -28.153265f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
    { { 20.649015f, 24.446232f, 0.021211f, 1.0f }, { 16.775627f, -27.25029f, 0.021211f, 1.0f }, { 72, 72, 72 }, 128 },
};

/* The occlusion result group of an unoccluded draw (flare_draw_disc): four
 * 'fully visible' texels, sent by DMA (a ref tag in vif_model_draw). */
const u32 g_00344AB0[4] __attribute__((aligned(16))) = {    /* 0x00344AB0 */
    0xFF, 0xFF, 0xFF, 0xFF
};
/* The point of flare_query_point. */
const f32 g_00344AC0[4] = {             /* 0x00344AC0 */
    0.0f, 0.0f, 0.0f, 0.0f
};

/* The models built at start-up (FlareClass's constructor): the disc
 * (flare_build_disc) and the rays (flare_build_rays), VIF codes and
 * headers; the rays header has room for 4 parts. */
static u32 g_004A5240[0x118] __attribute__((aligned(16)));  /* 0x004A5240 */
static VifModel g_004A56A0;                                 /* 0x004A56A0 */
static u32 g_004A56C0[0x460] __attribute__((aligned(16)));  /* 0x004A56C0 */
static u32 g_004A6840[12];                                  /* 0x004A6840 */
VifModel *g_003530B0 = (VifModel *)g_004A6840;              /* 0x003530B0 */

// FUNC 0x002466F0 flare_sun_model_a
VifModel *flare_sun_model_a(void)
{
    return &g_002F5BD0;
}

// FUNC 0x00246700 flare_sun_model_b
/* FlareClass's model when race/flare.bin is not loaded. */
VifModel *flare_sun_model_b(void)
{
    return &g_002F5D48;
}

// FUNC 0x00246710 flare_build_disc
/* Builds the disc model into g_004A5240 / g_004A56A0 (FlareClass's
 * constructor): one VIF1 packet
 *   UNPACK V4-32 1 -> 0xC0: GIFtag NLOOP 28, EOP, PRE, PRIM 0x4C, regs ST RGBAQ XYZF2
 *   UNPACK S-8  8 -> 0x40: 7, then 6 x 7 (7 strips of 4 vertices)
 *   UNPACK V3-32 28 -> 0: 7 groups of rim(4i), centre, rim(4i + 2), rim(4i + 4)
 *                         with rim(k) = (cos, sin, 0) of k pi / 14 (the
 *                         last one exactly 0)
 *   UNPACK V4-8 28 -> 0x80: 0xFFFFFFFF for the centres (k % 4 == 1), else 0
 * padded to a qword.  14 triangles, 28 vertices.
 * Written with a running pointer (*p++: retail advances $s0 by 4 after
 * every word) and the vertex count in a variable (retail ORs 28 << 16 into
 * both UNPACK codes at run time).  93.2%: left are the strips loop's
 * p += 2, which egcs computes before the loop (retail after it, with an
 * alignment nop), the outer loop's induction order and the registers of
 * the header stores.  Tried: indexed strips, a strips pointer counting up,
 * the model through a pointer, five header store orders. */
void flare_build_disc(void)
{
    u32 *p = g_004A5240;
    f32 *xyz;
    u8 *strips;
    int i, j, k;
    int words;
    int n = 28;

    *p++ = VIF_UNPACK(VIF_V4_32, 1, 0xC0);
    *p++ = 0x801C;                          /* GIFtag NLOOP 28, EOP */
    *p++ = 0x30264000;                      /*        PRE, PRIM 0x4C, 3 regs */
    *p++ = 0x412;                           /*        ST, RGBAQ, XYZF2 */
    *p++ = 0;
    *p++ = VIF_UNPACK(VIF_S_8, 8, 0x40);
    strips = (u8 *)p;
    strips[0] = 7;
    strips += 7;
    for (k = 6; k >= 0; k--)
        *strips-- = 6;
    p += 2;
    *p = VIF_UNPACK(VIF_V3_32, n, 0);
    xyz = (f32 *)(p + 1);
    for (i = 0; i < 7; i++) {
        int rim = i * 4;

        for (j = 0; j < 3; j++, rim += 2) {
            f32 a;

            if (i * 2 + j != 14)
                a = (f32)rim * 3.1415927f / 14.0f;
            else
                a = 0.0f;
            *xyz++ = (f32)cos(a);
            *xyz++ = (f32)sin(a);
            *xyz++ = 0.0f;
            if (j == 0) {
                *xyz++ = 0.0f;
                *xyz++ = 0.0f;
                *xyz++ = 0.0f;
            }
        }
    }
    p = (u32 *)xyz;
    *p++ = VIF_UNPACK(VIF_V4_8, n, 0x80);
    for (k = 0; k < 28; k++)
        *p++ = ((k & 3) ^ 1) != 0 ? 0 : 0xFFFFFFFFu;
    while ((p - g_004A5240) & 3)
        *p++ = 0;
    words = p - g_004A5240;
    g_004A56A0.nParts = 1;
    g_004A56A0.nVerts = 0x1C;
    g_004A56A0.nPrims = 0xE;
    g_004A56A0.part[0].qwc = words / 4;
    g_004A56A0.part[0].tex = 0;
    g_004A56A0.mode = 0;
    g_004A56A0.scaleFlags = 0;
    g_004A56A0.part[0].data = g_004A5240;
}

// FUNC 0x002469E0 flare_disc_model
VifModel *flare_disc_model(void)
{
    return &g_004A56A0;
}

// FUNC 0x002469F0 flare_build_rays
/* Builds the rays model into g_004A56C0 / *g_003530B0 (FlareClass's
 * constructor): the 72 entries of g_00343D30 in parts of at most 21
 * triangles, each part
 *   UNPACK V4-32 1 -> 0xC0: GIFtag NLOOP 3n, EOP, PRE, PRIM 0x4C
 *   UNPACK S-8  n + 1 -> 0x40: n, then n x 3
 *   UNPACK V3-32 3n -> 0: per entry (0, 0, 0), p0.xy / 32, p1.xy / 32
 *   UNPACK V4-8 3n -> 0x80: per entry 0xFF000000 | b << 16 | g << 8 | r, 0, 0
 * padded to a qword (relative to the part's start).  72 triangles, 216
 * vertices.
 * The table is indexed in place (retail multiplies first + k by 0x30 in
 * every loop), 3n is one variable (retail shifts it once for both UNPACK
 * codes) and the colour words go out with *p++.  The table is const (it is
 * .rodata): its loads are then free to move over the float stores, and
 * the vertex loop issues all four before the first store, as retail
 * (93.4% while the C declared it non-const).  98.3%: left are the colour
 * loop's register pairing (retail ORs b and g, then r and 0xFF000000) and
 * the first header stores' order.  Tried: a ray pointer, loads into
 * locals first, u32 / struct views of the vertex stream, the 24 orders of
 * the colour terms (also with the const table, and as two OR pairs). */
void flare_build_rays(void)
{
    u32 *p = g_004A56C0;
    int part = 0;
    int first = 0;

    do {
        int n = 0x48 - first;
        u32 *start;
        u8 *strips;
        f32 *xyz;
        int k;
        int words;
        int nv;

        if (!(n < 0x16))
            n = 0x15;
        nv = n * 3;
        start = p;
        start[0] = VIF_UNPACK(VIF_V4_32, 1, 0xC0);
        start[1] = nv | 0x8000;        /* GIFtag NLOOP 3n, EOP */
        start[2] = 0x30264000;              /*        PRE, PRIM 0x4C, 3 regs */
        start[3] = 0x412;                   /*        ST, RGBAQ, XYZF2 */
        start[5] = VIF_UNPACK(VIF_S_8, n + 1, 0x40);
        start[4] = 0;
        strips = (u8 *)&start[6];
        strips[0] = (u8)n;
        for (k = 0; k < n; k++)
            strips[k + 1] = 3;
        p = (u32 *)strips + (n + 4) / 4;
        *p = VIF_UNPACK(VIF_V3_32, nv, 0);
        xyz = (f32 *)(p + 1);
        for (k = 0; k < n; k++) {
            *xyz++ = 0.0f;
            *xyz++ = 0.0f;
            *xyz++ = 0.0f;
            *xyz++ = g_00343D30[first + k].p0[0] * 0.03125f;
            *xyz++ = g_00343D30[first + k].p0[1] * 0.03125f;
            *xyz++ = 0.0f;
            *xyz++ = g_00343D30[first + k].p1[0] * 0.03125f;
            *xyz++ = g_00343D30[first + k].p1[1] * 0.03125f;
            *xyz++ = 0.0f;
        }
        p = (u32 *)xyz;
        *p++ = VIF_UNPACK(VIF_V4_8, nv, 0x80);
        for (k = 0; k < n; k++) {
            *p++ = (u32)g_00343D30[first + k].rgb[0] | ((u32)g_00343D30[first + k].rgb[1] << 8) | 0xFF000000 | ((u32)g_00343D30[first + k].rgb[2] << 16);
            *p++ = 0;
            *p++ = 0;
        }
        while ((p - start) & 3)
            *p++ = 0;
        words = p - start;
        g_003530B0->part[part].data = start;
        g_003530B0->part[part].qwc = words / 4;
        g_003530B0->part[part].tex = 0;
        part++;
        first += 21;
    } while (first < 0x48);
    g_003530B0->mode = 0;
    g_003530B0->scaleFlags = 0;
    g_003530B0->nParts = part;
    g_003530B0->nVerts = 0xD8;
    g_003530B0->nPrims = 0x48;
}

// FUNC 0x00246D28 flare_rays_model
VifModel *flare_rays_model(void)
{
    return g_003530B0;
}

// FUNC 0x00246D30 flare_project
/* Projects pos (x, y, z, loaded unaligned: ldl / ldr + lw, pcpyld) with
 * vf24..vf27: p = M (x, y, z, 1), Q = 1 / p.w, p = min(p, limits) * Q; the
 * screen point viewCenter + p * viewScale in 12.4 fixed point (vftoi4).
 * Returns the projected z (p.z after the divide); *sx = (X - xoff) / 16,
 * *sy = (Y - yoff) / 16 (/ 8 when rendering fields); GsContext.spriteZF =
 * F << 24 | Z (the 12.4 values >> 4; F from viewScale.w): the depth and fog
 * of the next sprites.
 * The asm is two blocks with "=m" outputs (retail stores straight to
 * 0x10($sp) and loads proj.z between them); the first also takes *pos as
 * an "m" input (pos is then referenced twice: global-alloc gives it $s0
 * before ctx, as retail).  The draw buffer offsets are read through the
 * union view GS_TGT_A (pgl_effects_internal.h): retail loads yoff only after the *sx
 * store.  Then *sx, *sy and the 12.4 depth / fog shifted in place (the
 * array stores are retail's), spriteZF last. */
f32 flare_project(const f32 *pos, f32 *sx, f32 *sy)
{
    f32 proj[4] __attribute__((aligned(16)));   /* sp+0x00 */
    s32 scr[4] __attribute__((aligned(16)));    /* sp+0x10 */
    GsContext *ctx = GS_SPR_CTX;
    f32 div = 16.0f;
    f32 z;

    if (ctx->halfHeight != 0)
        div = 8.0f;
    vu0_load_screen_regs();
    __asm__ volatile(
        "vnop\n\t"
        "ldl          $3, 7(%1)\n\t"
        "ldr          $3, 0(%1)\n\t"
        "lw           $2, 8(%1)\n\t"
        "pcpyld       $2, $2, $3\n\t"
        "qmtc2.ni     $2, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf27, $vf0w\n\t"
        "vdiv         " VU0_Q ", $vf0w, $vf16w\n\t"
        "vmini.xyzw   $vf16, $vf16, $vf21\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf16, $vf16, " VU0_Q "\n\t"
        "sqc2         $vf16, %0\n\t"
        : "=m"(proj) : "r"(pos), "m"(*pos) : "$2", "$3", "memory");
    z = proj[2];
    __asm__ volatile(
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf16, $vf16, $vf23\n\t"
        "vftoi4.xyzw  $vf16, $vf16\n\t"
        "sqc2         $vf16, %0\n\t"
        : "=m"(scr));
    *sx = (f32)(scr[0] - GS_TGT_A(ctx).xoff) * 0.0625f;
    *sy = (f32)(scr[1] - GS_TGT_A(ctx).yoff) / div;
    scr[2] >>= 4;
    scr[3] >>= 4;
    ctx->spriteZF = (scr[3] << 24) | scr[2];
    return z;
}

// FUNC 0x00246E60 flare_fill_octagon
/* A filled octagon of radius r around (cx, cy) as 5 rectangles (y scaled
 * by aspect): the centre column +-r sin(22.5), then +-r cos(45) and +-r
 * cos(22.5) on each side. */
void flare_fill_octagon(f32 cx, f32 cy, f32 r, f32 aspect)
{
    int outer = (int)(r * 0.923879f);       /* cos 22.5 */
    int mid = (int)(r * 0.707107f);         /* cos 45 */
    int inner = (int)(r * 0.382683f);       /* sin 22.5 */
    int x = (int)cx;
    int y = (int)cy;
    int outerY = (int)((f32)outer * aspect);
    int midY = (int)((f32)mid * aspect);
    int innerY = (int)((f32)inner * aspect);

    gs_draw_flat_sprite(x - inner, y - outerY, x + inner, y + outerY);
    gs_draw_flat_sprite(x - mid, y - midY, x - inner, y + midY);
    gs_draw_flat_sprite(x - outer, y - innerY, x - mid, y + innerY);
    gs_draw_flat_sprite(x + inner, y - midY, x + mid, y + midY);
    gs_draw_flat_sprite(x + mid, y - innerY, x + outer, y + innerY);
}

// FUNC 0x00246FC0 flare_set_target
/* Render target = the w x h PSMCT32 buffer at fbWordAddr, draw area the
 * whole of it, full-height (no field) rendering, no scan mask. */
void flare_set_target(u32 fbWordAddr, int w, int h)
{
    gfx_set_draw_buffer(fbWordAddr, 0, w, h);
    gfx_set_draw_area(0, 0, w, h);
    gfx_set_field_mode(0);
    gs_set_scanmask_mode(0);
}

// FUNC 0x00247020 flare_set_texture
/* Texture = the w x h PSMCT32 buffer at texWordAddr (buffer width rounded
 * up to 64), after a TEXFLUSH; clamp region = its size, both wrap modes
 * CLAMP (so the region is not used). */
void flare_set_texture(u32 texWordAddr, int w, int h)
{
    gfx_texflush();
    gfx_set_texture_buffer(0, texWordAddr, 0, (w + 0x3F) & ~0x3FU);    /* unsigned mask: lui / ori */
    gfx_set_texture_size(w, h);
    gfx_set_texture_region(0, 0, w);
    gfx_set_texture_region(1, 0, h);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
}

// FUNC 0x002470C0 flare_copy_visibility
/* Copies the screen's box (x0, y0)-(x1, y1) (y halved when rendering
 * fields) to the top left of the work buffer, at most its size, as one
 * textured sprite: depth test off, white, point sampling.  Only alpha is
 * written (the caller's colour mask): the visibility the query drew. */
void flare_copy_visibility(FlareWork *fw, int x0, int y0, int x1, int y1)
{
    int w = x1 - x0;
    int h = y1 - y0;
    int workW = fw->workWidth;
    int workH = fw->workHeight;

    if (workW < w)
        w = workW;
    if (workH < h)
        h = workH;
    if (GS_SPR_CTX->halfHeight != 0) {
        y0 = y0 / 2;
        y1 = y1 / 2;
    }
    flare_set_target(fw->workVram, workW, workH);
    flare_set_texture(fw->screenVram, fw->screenWidth, fw->screenHeight);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gs_draw_textured_sprite(0, 0, w, h, x0, y0, x1, y1);
}

// FUNC 0x002471D8 flare_reduce
/* Reduces the w x h box of the work buffer (clamped to its size) in place
 * to one texel: bilinear sprites, each halving an even size or taking an
 * odd one down by 1 (at least 1), sampling at texel centres (+0.5), until
 * both sizes are below 2.  Used for boxes of 18 or more (flare_run_reduce). */
void flare_reduce(FlareWork *fw, int w, int h)
{
    int workW = fw->workWidth;
    int workH = fw->workHeight;

    w = (workW < w) ? workW : w;
    h = (workH < h) ? workH : h;
    flare_set_target(fw->workVram, workW, workH);
    flare_set_texture(fw->workVram, workW, workH);
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    while (w >= 2 || h >= 2) {
        int nw = w >> 1;
        int nh = h >> 1;

        if (((nw << 1) ^ w) != 0)           /* odd: w - 1 */
            nw = nw << 1;
        if (((nh << 1) ^ h) != 0)
            nh = nh << 1;
        if (!(0 < nw))
            nw = 1;
        if (!(0 < nh))
            nh = 1;
        gfx_texflush();
        gs_draw_textured_sprite_f(0.0f, 0.0f, (f32)nw, (f32)nh, 0.5f, 0.5f,
                                  (f32)w + 0.5f, (f32)h + 0.5f);
        w = nw;
        h = nh;
    }
}

// FUNC 0x00247340 flare_build_reduce
/* Records the square reduction of an n x n box (n clamped to the work
 * width; the steps as in flare_reduce) into the packet buffer, closes it
 * with a DMA ret tag, moves it (backwards, word by word) to a block
 * allocated from the end of the packet buffer and rewinds the write
 * pointer; the state the recording changed is marked to be sent again
 * (FRAME and XYOFFSET dirty in both contexts, the VU1 matrix unsent).
 * Returns the block: a DMA call target. */
u32 *flare_build_reduce(FlareWork *fw, int n)
{
    GsContext *ctx = GS_SPR_CTX;
    int workW = fw->workWidth;
    int workH = fw->workHeight;
    u32 *save;
    u32 *start;
    u32 *end;
    u32 *dst;
    int words;

    n = (workW < n) ? workW : n;
    save = ctx->cur;
    flare_set_target(fw->workVram, workW, workH);
    flare_set_texture(fw->workVram, workW, workH);
    gs_flush_dirty_regs();
    start = ctx->cur;
    pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    if (!(n < 2)) {
        do {
            int half = n >> 1;

            if (((half << 1) ^ n) != 0)     /* odd: n - 1 */
                half = half << 1;
            gfx_texflush();
            gs_draw_textured_sprite_f(0.0f, 0.0f, (f32)half, (f32)half, 0.5f, 0.5f,
                                      (f32)n + 0.5f, (f32)n + 0.5f);
            n = half;
        } while (!(n < 2));
    }
    end = ctx->cur;
    end[0] = DMATAG_RET;
    end[1] = 0;
    end[2] = 0;
    end[3] = 0;
    end += 4;
    words = end - start;
    dst = (u32 *)gfx_alloc_tail(words / 4) + words;
    while (start < end) {
        end--;
        dst--;
        *dst = *end;
    }
    ctx->cur = save;
    ctx->dirty |= GS_DIRTY_REG_BOTH(GS_REG_FRAME) | GS_DIRTY_REG_BOTH(GS_REG_XYOFFSET);
    ctx->vu1MatrixSent = 0;
    return dst;
}

// FUNC 0x00247518 flare_run_reduce
/* w < 2: nothing.  w >= 18: flare_reduce (tail call).  Else the work
 * buffer as texture and a DMA call of the prebuilt packet reduce[w - 2]
 * (square: h is not used). */
void flare_run_reduce(FlareWork *fw, int w, int h)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p;
    u32 *pkt;

    if (w < 2)
        return;
    if (!(w < 0x12)) {
        flare_reduce(fw, w, h);
        return;
    }
    flare_set_texture(fw->workVram, fw->workWidth, fw->workHeight);
    gs_flush_dirty_regs();
    p = ctx->cur;
    pkt = fw->reduce[w - 2];
    ctx->cur = p + 4;
    p[0] = DMATAG_CALL;
    p[1] = (u32)pkt;
    p[3] = 0;           /* 3 before 2: retail's store order */
    p[2] = 0;
}

// FUNC 0x002475C8 flare_store_result
/* Draws the reduced texel (0, 0) of the work buffer, with colour
 * scale * w * h / 4 and blending Cs * As, into texel (q % 64, q / 64) of
 * the result strip (q = fw->queries). */
void flare_store_result(FlareWork *fw, f32 scale, int w, int h)
{
    f32 c = scale * (f32)(w * h) * 0.25f;
    int q;
    int qx, qy;

    flare_set_target(fw->occVram, 0x40, fw->occHeight);
    q = fw->queries;
    qy = q / 64;
    qx = q & 0x3F;
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_SRC_ALPHA, 0.0f);
    pgl_color4f(c, c, c, 1.0f);
    gs_draw_textured_sprite(qx, qy, qx + 1, qy + 1, 0, 0, 0, 0);
}

// FUNC 0x00247690 vif_model_draw
/* Queues model m for the VU1 model program (MSCAL vu1Prog + 0x3C; nothing
 * when vu1Prog is 0): STMASK 0xC0C0C0C0 and the matrices are synced first,
 * the statistics counters get the model's counts.  Per part: its texture's
 * GS registers (fw->tex[tex - 1]->tex) when tex != 0, then a DMA ref of the
 * part's packet with ITOP 0x2F7 and the MSCAL of the previous part
 * (deferred by one; 0 for the first); for the first two parts also a DMA
 * ref of the result group `results' (UNPACK V4-8 x4 -> 0xCB) and a cnt
 * qword with `index' (S-32 -> 0xCA) and (r, g, b) (V3-32 -> 0xC9).  When
 * the VU1 matrix was not sent yet pgl_send_matrix sends it to 0xC1 and
 * the MSCAL address goes 1 back; dirty GS registers go out through
 * pgl_send_dirty_regs to 0xD9 (address 5 back).  A final cnt tag carries
 * the last MSCAL. */
/* ? (r, g, b) as a colour: every caller passes one (glow r * a ..., the
 * sun's FlareClass.sun[1..3]); the VU1 program's use of 0xC9 is unread. */
/* m->part[i] is indexed in place (retail keeps i * 8 and m + 0x10 as loop
 * registers).  84.1%: retail spills fw, results, r, g, b to the stack and
 * keeps index in $fp; here index is spilled and b lives in $fp (global-alloc
 * priorities), which also moves the packet stores.  Tried: a part pointer,
 * while / goto loops, *(u32 *)& colour transfers. */
void vif_model_draw(FlareWork *fw, VifModel *m, const void *results, u32 index,
                    f32 r, f32 g, f32 b)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 mscal = 0;
    u32 entry;
    u32 i;
    u32 *p;

    if (ctx->stmask != 0xC0C0C0C0)
        gs_emit_stmask(ctx, 0xC0C0C0C0);
    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    if (ctx->vu1Prog == 0)
        return;
    entry = ctx->vu1Prog + FLARE_VU1_ENTRY;
    ctx->nVerts += m->nVerts;
    ctx->nPrims += m->nPrims;
    ctx->nParts += m->nParts;
    for (i = 0; i < m->nParts; i++) {

        if (m->part[i].tex != 0)
            gfx_set_texture_regs(fw->tex[m->part[i].tex - 1]->tex);
        p = ctx->cur;
        p[0] = DMATAG_REF(m->part[i].qwc);
        p[1] = (u32)m->part[i].data & 0x0FFFFFFF;
        p[2] = VIF_ITOP(FLARE_VU1_ITOP);
        p[3] = mscal != 0 ? VIF_MSCAL(mscal) : 0;
        ctx->cur = p + 4;
        mscal = entry;
        if (i < 2) {
            p += 4;
            p[0] = DMATAG_REF(1);
            p[1] = (u32)results & 0x0FFFFFFF;
            p[3] = VIF_UNPACK(VIF_V4_8, 4, 0xCB);
            p[4] = DMATAG_CNT(1);
            p[6] = VIF_UNPACK(VIF_S_32, 1, 0xCA);
            p[8] = VIF_UNPACK(VIF_V3_32, 1, 0xC9);
            p[9] = u113_f2u(r);
            p[10] = u113_f2u(g);
            p[11] = u113_f2u(b);
            ctx->cur = p + 12;
            p[2] = 0;
            p[5] = 0;
            p[7] = index;
        }
        if (ctx->vu1MatrixSent == 0) {
            pgl_send_matrix(ctx, 0xC1);
            mscal--;
            ctx->vu1MatrixSent = 1;
        }
        if (ctx->dirty & GS_DIRTY_ALL_CTX1_GEN) {
            pgl_send_dirty_regs(ctx, 0xD9);
            mscal -= 5;
        }
    }
    if (mscal != 0) {
        p = ctx->cur;
        ctx->cur = p + 4;
        p[0] = DMATAG_CNT(0);
        p[2] = VIF_ITOP(FLARE_VU1_ITOP);
        p[3] = VIF_MSCAL(mscal);
        p[1] = 0;
    }
}

// FUNC 0x00247948 flare_init
/* Per frame set-up (FlareClass::begin, with the screen buffer): the
 * buffers, aspect and textures stored, the 16 reduction packets (n =
 * 2..17) built with the GS state saved around (pgl_push_attrib /
 * pgl_pop_attrib), then texel (0, 0) of the work buffer cleared to red
 * (1, 0, 0, 0) with no blending, alpha or depth test, depth writes off.
 * ? The queries write only alpha into the work buffer, so a stored result
 * would be this red modulated by the reduced alpha. */
void flare_init(FlareWork *fw, f32 aspect, int screenVram, int screenW, int screenH,
                int occVram, int occH, int workVram, int workW,
                int workH, Tex1Image *tex0, Tex1Image *tex1)
{
    int i;

    fw->width = screenW;
    fw->height = screenH;
    fw->workVram = workVram;
    fw->workWidth = workW;
    fw->workHeight = workH;
    fw->queries = 0;
    fw->aspect = aspect;
    fw->screenVram = screenVram;
    fw->screenWidth = screenW;
    fw->screenHeight = screenH;
    fw->occVram = occVram;
    fw->occHeight = occH;
    fw->tex[0] = tex0;
    fw->tex[1] = tex1;
    pgl_push_attrib(FLARE_SAVED_ATTRIBS);
    for (i = 0; i < 0x10; i++)
        fw->reduce[i] = flare_build_reduce(fw, i + 2);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_depth_mask(0);
    flare_set_target(workVram, workW, workH);
    pgl_color4f(1.0f, 0.0f, 0.0f, 0.0f);
    gs_draw_flat_sprite(0, 0, 1, 1);
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    pgl_pop_attrib();
}

// FUNC 0x00247AA0 flare_occlusion_query
/* One occlusion query at screen point (sx, sy) for a size x size box
 * (integer size, the box centred on the rounded point).  Returns 0 when the
 * box is off the screen.  Else, writing alpha only: the box (at least
 * 2 x 2, + 2) cleared without depth test, then a square (octagon & 0xFF:
 * an octagon, counted as a circle: scale * 4 / pi) of alpha 2.0 drawn into
 * it with the depth test (the sprite depth was set by the caller); the box
 * clamped to the screen, copied to the work buffer, reduced to one texel
 * and stored to the result strip with scale; fw->queries++; returns 1.
 * 98.2%: left is the argument order of the flare_copy_visibility call
 * (retail loads $a0 first and puts x1 + 1 in the jal delay slot; here
 * $a0 goes last).  Tried: w / h before and after the colour mask, x1++,
 * the arguments from w / h. */
int flare_occlusion_query(FlareWork *fw, int octagon, f32 sx, f32 sy,
                          f32 size, f32 scale)
{
    int isize = (int)size;
    f32 fsize = (f32)isize;
    f32 half;
    f32 cx, cy;
    int x0, y0, x1, y1;
    int w, h;
    int isOctagon;

    sy = sy + 0.5f;
    sx = sx + 0.5f;
    half = fsize * 0.5f;
    cy = (f32)(int)sy;
    cx = (f32)(int)sx;
    isOctagon = octagon & 0xFF;
    y0 = (int)(cy - half);
    x0 = (int)(cx - half);
    x1 = x0 + isize;
    y1 = (int)((f32)y0 + fsize);
    if (x1 < 0 || fw->width < x0 || y1 < 0 || fw->height < y0)
        return 0;
    if (!(2 < x1 - x0))
        x1 = x0 + 2;
    if (!(2 < y1 - y0))
        y1 = y0 + 2;
    gfx_color_mask(0, 0, 0, 1);
    gfx_depth_mask(0);
    pgl_color(0);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gs_draw_flat_sprite(x0, y0, x1 + 2, y1 + 2);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    pgl_color4f(1.0f, 1.0f, 1.0f, 2.0f);
    if (isOctagon != 0) {
        flare_fill_octagon(cx, cy, half, fw->aspect);
        scale = scale * 1.2732406f;         /* 4 / pi */
    } else {
        gs_draw_flat_sprite(x0, y0, x1, y1);
    }
    if (!(-1 < x0))
        x0 = 0;
    if (fw->width - 1 < x1)
        x1 = fw->width - 1;
    if (fw->height - 1 < y1)
        y1 = fw->height - 1;
    if (!(-1 < y0))
        y0 = 0;
    pgl_push_attrib(FLARE_SAVED_ATTRIBS);
    w = x1 - x0;
    gfx_color_mask(0, 0, 0, 1);
    h = y1 - y0;
    flare_copy_visibility(fw, x0, y0, x1 + 1, y1 + 1);
    flare_run_reduce(fw, w, h);
    gfx_color_mask(1, 1, 1, 1);
    flare_store_result(fw, scale, w, h);
    pgl_pop_attrib();
    gfx_depth_mask(1);
    fw->queries = fw->queries + 1;
    return 1;
}

// FUNC 0x00247D50 flare_readback
/* Reads the 64 x occHeight result strip (PSMCT32) back into results (kept
 * for the draws; FlareClass::flush) and restarts the query count. */
void flare_readback(FlareWork *fw, u32 *results)
{
    fw->results = results;
    gs_download_image(fw->occVram, 0, 0x40, 0, 0, 0x40, fw->occHeight, results);
    fw->queries = 0;
}

// FUNC 0x00247DA0 flare_draw_result_disc
/* Draws m with the occlusion result of the next query (the 4-result group
 * &results[q & ~3] and index q & 3) and (r, g, b); fw->queries++. */
void flare_draw_result_disc(FlareWork *fw, VifModel *m, f32 r, f32 g, f32 b)
{
    s32 q = fw->queries;

    vif_model_draw(fw, m, fw->results + (q & ~3), q & 3, r, g, b);
    fw->queries = fw->queries + 1;
}

// FUNC 0x00247DF0 flare_draw_disc
/* Draws m unoccluded (the result group g_00344AB0, all 0xFF, index 0) with
 * (r, g, b) * intensity / 2. */
void flare_draw_disc(FlareWork *fw, VifModel *m, f32 r, f32 g, f32 b,
                     f32 intensity)
{
    intensity = intensity * 0.5f;
    vif_model_draw(fw, m, g_00344AB0, 0, r * intensity, g * intensity, b * intensity);
}

// FUNC 0x00247E18 flare_query_point
/* Occlusion query of the fixed point g_00344AC0 (skipped, 0, when its
 * projected z is outside -1..1; flare_project sets the sprite depth); size
 * at least 2; depth test on.  No references. */
int flare_query_point(FlareWork *fw, f32 scale, f32 size)
{
    f32 sx, sy;                             /* sp+0x00, sp+0x04 */
    f32 z;
    int isize;

    z = flare_project(g_00344AC0, &sx, &sy);
    if (z < -1.0f)
        return 0;
    if (1.0f < z)
        return 0;
    if (size < 2.0f)
        size = 2.0f;
    gfx_enable(PGL_CAP_DEPTH_TEST);
    isize = (int)size;
    return flare_occlusion_query(fw, 0, sx, sy, size, scale / (f32)(isize * isize));
}

// FUNC 0x00247EF0 flare_query_screen
/* Occlusion query at a point given in normalised screen coordinates
 * (-1..1, y up) with its depth: the sprites' depth = max(z - 0.2, 0)
 * (pgl_set_sprite_depth), size at least 2.  (FlareClass::isVisible.) */
int flare_query_screen(FlareWork *fw, const f32 *screen, f32 scale, f32 size)
{
    f32 x, y, z;
    int isize;

    x = (screen[0] * 0.5f + 0.5f) * (f32)fw->screenWidth;
    y = (0.5f - screen[1] * 0.5f) * (f32)fw->screenHeight;
    z = screen[2] - 0.2f;
    if (z < 0.0f)
        z = 0.0f;
    pgl_set_sprite_depth(z);
    if (size < 2.0f)
        size = 2.0f;
    isize = (int)size;
    return flare_occlusion_query(fw, 0, x, y, size, scale / (f32)(isize * isize));
}

// FUNC 0x00247FE8 flare_draw_glow
/* A glow at view position pos, pulled towards the eye by `near': the
 * current matrix = identity translated to pos * (d - near) / d (d = -pos.z),
 * additive blending, depth writes off.  flags & 1: m scaled by `scale',
 * either unoccluded with the depth test (flags & 2: flare_draw_disc with
 * (r, g, b) * a / 2) or with the next occlusion result and (r, g, b) * a.
 * Then, when haloA > 0, haloScale > 0 and d > 1, the disc model scaled by
 * haloScale with (r, g, b) * haloA / 2 (the matrix not pushed: it scales
 * the translated one). */
void flare_draw_glow(FlareWork *fw, int flags, const f32 *pos, VifModel *m,
                     f32 near, f32 r, f32 g, f32 b, f32 a, f32 scale,
                     f32 haloA, f32 haloScale)
{
    int halo = 0;
    int drawModel = flags & 1;
    int unoccluded;
    f32 d;
    f32 k;

    d = -pos[2];
    if (0.0f < haloA && 0.0f < haloScale && 1.0f < d)
        halo = 1;
    k = d - near;
    unoccluded = flags & 2;
    k = k / d;
    gfx_depth_mask(0);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_ADD, 0.0f);
    vu0_load_identity_regs();
    pgl_translate(pos[0] * k, pos[1] * k, pos[2] * k);
    if (drawModel != 0) {
        pgl_push_matrix();
        vu0_scale(scale, scale, scale);
        if (unoccluded != 0) {
            gfx_enable(PGL_CAP_DEPTH_TEST);
            flare_draw_disc(fw, m, r, g, b, a);
        } else {
            gfx_disable(PGL_CAP_DEPTH_TEST);
            flare_draw_result_disc(fw, m, r * a, g * a, b * a);
        }
        pgl_pop_matrix();
    }
    if (halo != 0) {
        vu0_scale(haloScale, haloScale, haloScale);
        gfx_enable(PGL_CAP_DEPTH_TEST);
        flare_draw_disc(fw, flare_disc_model(), r, g, b, haloA);
    }
    gfx_depth_mask(1);
}

// FUNC 0x002481F0 flare_draw_streak
/* An elongated glow (a light seen as a streak): when a > 0 and the light is
 * in front (d = -pos.z >= 0), the current matrix = rows (axes[0], axes[1],
 * 0, 0), (axes[2], axes[3], 0, 0), (0, 0, 1, 0), (pos * (d - near) / d, 1)
 * and the disc model is drawn with (r, g, b) * a / 2 (additive, depth
 * tested, no depth writes).  The matrix is pushed first; the early exit for
 * d < 0 skips the pop and the depth-mask reset: a push / pop imbalance, as
 * in the binary.  The 4th argument is not used. */
void flare_draw_streak(FlareWork *fw, const f32 *pos, const f32 *axes,
                       VifModel *unused, f32 near, f32 r, f32 g, f32 b, f32 a)
{
    f32 mtx[4][4] __attribute__((aligned(16)));     /* sp+0x00 */
    f32 pz, d, k;

    pgl_push_matrix();
    if (0.0f < a) {
        gfx_depth_mask(0);
        gfx_enable(PGL_CAP_BLEND);
        gfx_enable(PGL_CAP_DEPTH_TEST);
        gfx_blend_func(PGL_BLEND_ADD, 0.0f);
        pz = pos[2];
        d = -pz;
        if (d < 0.0f)
            return;                         /* without pgl_pop_matrix */
        k = (d - near) / d;
        mtx[0][0] = axes[0];        /* row by row (the scheduler makes retail's order) */
        mtx[0][1] = axes[1];
        mtx[0][2] = 0.0f;
        mtx[0][3] = 0.0f;
        mtx[1][0] = axes[2];
        mtx[1][1] = axes[3];
        mtx[1][2] = 0.0f;
        mtx[1][3] = 0.0f;
        mtx[2][0] = 0.0f;
        mtx[2][1] = 0.0f;
        mtx[2][2] = 1.0f;
        mtx[2][3] = 0.0f;
        mtx[3][0] = pos[0] * k;
        mtx[3][1] = pos[1] * k;
        mtx[3][2] = pz * k;
        mtx[3][3] = 1.0f;
        pgl_load_matrix(&mtx[0][0]);
        flare_draw_disc(fw, flare_disc_model(), r, g, b, a);
        gfx_depth_mask(1);
    }
    pgl_pop_matrix();
}

// FUNC 0x00248378 flare_query_sun
/* Occlusion query of the sun at pos, a direction: projected with the
 * current matrix's rotation only (vu0_reset_translation inside a push /
 * pop); skipped (0) when its projected z is outside -1..1; sprites at GS
 * depth 0, fog 0 (hidden by anything drawn); the integer size is used as
 * is.  (FlareClass::setSun.) */
int flare_query_sun(FlareWork *fw, const f32 *pos, f32 scale, f32 size)
{
    f32 sx, sy;                             /* sp+0x00, sp+0x04 */
    int isize = (int)size;
    f32 z;

    pgl_push_matrix();
    vu0_reset_translation();
    z = flare_project(pos, &sx, &sy);
    pgl_pop_matrix();
    if (z < -1.0f)
        return 0;
    if (1.0f < z)
        return 0;
    gs_set_sprite_zf(0, 0);
    return flare_occlusion_query(fw, 0, sx, sy, (f32)isize,
                                 scale / (f32)(isize * isize));
}

// FUNC 0x00248458 flare_draw_sun
/* The sun glare: model m as a billboard at the direction pos (translation
 * reset, translated to pos, rotation reset), scaled by `scale', additive,
 * no depth test or writes, with the next occlusion result and (r, g, b).
 * (FlareClass::draw: FlareClass.sun[0..3].) */
void flare_draw_sun(FlareWork *fw, const f32 *pos, VifModel *m, f32 scale,
                    f32 r, f32 g, f32 b)
{
    gfx_depth_mask(0);
    pgl_push_matrix();
    vu0_reset_translation();
    pgl_translate(pos[0], pos[1], pos[2]);
    vu0_reset_rotation();
    gfx_blend_func(PGL_BLEND_ADD, 0.0f);
    vu0_scale(scale, scale, scale);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    flare_draw_result_disc(fw, m, r, g, b);
    pgl_pop_matrix();
    gfx_depth_mask(1);
}

// FUNC 0x00248538 flare_draw_sun_disc
/* The sun disc: when scale > 0, the disc model as a billboard at the
 * direction pos, scaled by `scale', unoccluded with (r, g, b) * a / 2;
 * additive, no depth test, alpha test or depth writes, period adjust off
 * (back on at the end).  (FlareClass::drawSunDisc.) */
void flare_draw_sun_disc(FlareWork *fw, const f32 *pos, f32 r, f32 g, f32 b,
                         f32 a, f32 scale)
{
    if (scale <= 0.0f)
        return;
    gfx_depth_mask(0);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_blend_func(PGL_BLEND_ADD, 0.0f);
    pgl_push_matrix();
    vu0_reset_translation();
    pgl_translate(pos[0], pos[1], pos[2]);
    vu0_reset_rotation();
    vu0_scale(scale, scale, scale);
    flare_draw_disc(fw, flare_disc_model(), r, g, b, a);
    pgl_pop_matrix();
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_CULL_FACE);
}

// FUNC 0x00248680 vu0_sync_state
/* pgl_sync_matrices when a matrix changed. */
void vu0_sync_state(void)
{
    GsContext *ctx = GS_SPR_CTX;

    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    __asm__ volatile("vnop");
}

// FUNC 0x002486B8 vu0_load_screen_regs
/* After the matrix sync: vf23 = viewScale, vf22 = viewCenter, vf21 =
 * limits, vf20 = vBB0 (scratchpad 0x70003B80..0x70003BBF). */
void vu0_load_screen_regs(void)
{
    GsContext *ctx = GS_SPR_CTX;

    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    __asm__ volatile(
        "lqc2         $vf23, 0xB80(%0)\n\t"     /* viewScale */
        "lqc2         $vf22, 0xB90(%0)\n\t"     /* viewCenter */
        "lqc2         $vf21, 0xBA0(%0)\n\t"     /* limits */
        "lqc2         $vf20, 0xBB0(%0)\n\t"     /* vBB0 */
        : : "r"(ctx) : "memory");
}

// FUNC 0x00248700 vu0_load_screen_regs_nosync
/* The same loads without the sync ($v0 is left = the context). */
void vu0_load_screen_regs_nosync(void)
{
    __asm__ volatile(
        "lqc2         $vf23, 0xB80(%0)\n\t"     /* viewScale */
        "lqc2         $vf22, 0xB90(%0)\n\t"     /* viewCenter */
        "lqc2         $vf21, 0xBA0(%0)\n\t"     /* limits */
        "lqc2         $vf20, 0xBB0(%0)\n\t"     /* vBB0 */
        : : "r"(GS_CTX) : "memory");
}

// FUNC 0x00248720 vu0_load_modelview
/* vf20..vf23 = the MODELVIEW matrix: the current matrix (vf28..vf31) when
 * matrixMode is the modelview, else its parked copy at VU0 qwords 4..7
 * (vlqi through vi10).  ($v0 is left = 4.)  The name is from the first
 * pass: vu0_load_modelview would fit. */
void vu0_load_modelview(void)
{
    do {        /* a statement macro, as weather_draw's (the else label is 8-aligned) */
        if (GS_SPR_CTX->matrixMode == PGL_MTX_MODELVIEW) {
            __asm__ volatile(
                "vmove.xyzw   $vf20, $vf28\n\t"
                "vmove.xyzw   $vf21, $vf29\n\t"
                "vmove.xyzw   $vf22, $vf30\n\t"
                "vmove.xyzw   $vf23, $vf31\n\t");
        } else {
            __asm__ volatile(
                "ctc2.i       %0, $vi10\n\t"
                "vlqi.xyzw    $vf20, ($vi10++)\n\t"
                "vlqi.xyzw    $vf21, ($vi10++)\n\t"
                "vlqi.xyzw    $vf22, ($vi10++)\n\t"
                "vlqi.xyzw    $vf23, ($vi10++)\n\t"
                "vnop\n\t"
                "vnop\n\t"
                : : "r"(PGL_VU0_PARKED(PGL_MTX_MODELVIEW)));
        }
    } while (0);
}

// FUNC 0x00248768 vu0_load_projection
/* vf20..vf23 = the PROJECTION matrix: the current matrix when matrixMode
 * is the projection, else its parked copy at VU0 qwords 8..11.  ($v0 is
 * left = 8.) */
void vu0_load_projection(void)
{
    do {        /* a statement macro, as weather_draw's (the else label is 8-aligned) */
        if (GS_SPR_CTX->matrixMode == PGL_MTX_PROJECTION) {
            __asm__ volatile(
                "vmove.xyzw   $vf20, $vf28\n\t"
                "vmove.xyzw   $vf21, $vf29\n\t"
                "vmove.xyzw   $vf22, $vf30\n\t"
                "vmove.xyzw   $vf23, $vf31\n\t");
        } else {
            __asm__ volatile(
                "ctc2.i       %0, $vi10\n\t"
                "vlqi.xyzw    $vf20, ($vi10++)\n\t"
                "vlqi.xyzw    $vf21, ($vi10++)\n\t"
                "vlqi.xyzw    $vf22, ($vi10++)\n\t"
                "vlqi.xyzw    $vf23, ($vi10++)\n\t"
                "vnop\n\t"
                "vnop\n\t"
                : : "r"(PGL_VU0_PARKED(PGL_MTX_PROJECTION)));
        }
    } while (0);
}
