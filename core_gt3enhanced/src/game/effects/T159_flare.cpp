/* T159_flare.cpp - FlareClass (0x0019AD00..0x0019C368): lens flares and
 * glows of the course's flare sources and the cars' lamps, the sun glare
 * and the reflections of the lamps on the road, on G001's flare library
 * (FlareWork = the first 0x7C bytes of the object).
 *
 * Per frame (RacePS2Base::drawView / virtual_47): setNoOcclusionTest,
 * begin(screen) sets up G001 (flare_init), empties the lists and keeps the
 * projection matrix; the course submits its flare sources (T195 ->
 * addSource) and the cars their lamps (RaceCarModel::addLamps ->
 * addLight), each projected with the current VU0 matrix, culled against the
 * screen and given an occlusion query (flare_query_screen, unless
 * noOcclusion); flush() reads the query results back (flare_readback);
 * draw() draws the sun glare, the lamps and the sources with distance
 * attenuation 1 / (att0 + att1 d + att2 d^2), each glow with its query's
 * result.  The reflections are a separate pass (RacePS2Base::drawLampReflections:
 * beginReflections, RaceCarModel::drawLampReflections -> drawReflection,
 * endReflections).  See game/effects/T159_flare.h. */
/* T159's own include set (not race_display_draw_internal.h): retail T159's .rodata has only its
 * strings, the reflection UVs, the vtable and "10FlareClass" (then <new>'s
 * "bad_alloc"); the race display headers would add their type names. */
#include "types.h"
#include "vu0asm.h"                         /* VU0_ACC */
#include "libc.h"                           /* free */
#include "cxxrt.h"                          /* type_info, __rtti_user (GCC 14's __tf10FlareClass) */
#include "game/effects/T159_flare.h"                /* own: FlareClass, Vec3 / Vec4, CarLamp, FlareSource ... */
#include "engine/pgl/G001_flare.h"                /* the pgl library, the flare library (flare_*), Tex1Image,
                                               VifModel, strip_batch_*, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* sinf */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (wide), g_002F4250 (VramAllocator) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 / flushReleased */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "game/enhanced.h"                  /* ENH_RENDER_FLARE_NO_FOG */

/* ======================================================================== */
/* T159: FlareClass helpers                                                 */
/* ======================================================================== */

/* The first 0x7C bytes of a FlareClass are G001's FlareWork (defined by
 * u113, forward-declared in G001_flare.h): every flare_* call takes `this'. */
static inline FlareWork *flare_work(FlareClass *fc)
{
    return (FlareWork *)fc;
}

/* A lamp with |sizeA - sizeB| below this is round, else elongated
 * (gp-0x7C28 / gp-0x7C24 = 0x3C23D70A). */
#define FLARE_ROUND_EPS 0.01f
#define FLARE_MAX_ENTRIES 100               /* lights[] / sources[] */
#define FLARE_TWO_PI 6.2831855f             /* gp-0x7C20 (0x40C90FDB) */

/* VU0 macro mode (the current matrix lives in vf28..vf31: the x, y, z
 * columns and the translation; vu0_load_identity_regs, pgl_translate ...).
 * The original's VU0 code is asm (VU0 / MMI instructions egcs never emits):
 * the operands move through $2 / $3 with unaligned loads and stores (ldl /
 * ldr / lwl / lwr + pcpyld + qmtc2.ni in, qmfc2.ni + pcpyud + sdl / sdr /
 * swl / swr out) addressed as memory operands (retail folds the field
 * offsets into them: `ldl $3, 0x27($s0)' for &light->dir), all volatile
 * statements; a `vnop' (vu0_sync) waits for VU0 before most of them.  The
 * results are read back as volatile locals: retail reloads every component
 * at each use, in source order (drawReflection's lamp.x * lamp.x is two
 * lwc1).  GCC 14 (#else): the same transforms through an aligned temporary
 * (lqc2 / sqc2, same values). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define VU0_IN_V3(p)                                                           \
    "ldl $3, 7+" p "\n\tldr $3, " p "\n\tlwl $2, 11+" p "\n\tlwr $2, 8+" p "\n\t" \
    "pcpyld $2, $2, $3\n\tqmtc2.ni $2, $vf16\n\t"
#define VU0_IN_V4(p)                                                           \
    "ldl $3, 7+" p "\n\tldr $3, " p "\n\tldl $2, 15+" p "\n\tldr $2, 8+" p "\n\t" \
    "pcpyld $2, $2, $3\n\tqmtc2.ni $2, $vf16\n\t"
#define VU0_OUT_V3(p)                                                          \
    "qmfc2.ni $2, $vf16\n\tpcpyud $3, $2, $2\n\t"                              \
    "sdl $2, 7+" p "\n\tsdr $2, " p "\n\tswl $3, 11+" p "\n\tswr $3, 8+" p
#define VU0_OUT_V4(p)                                                          \
    "qmfc2.ni $2, $vf16\n\tpcpyud $3, $2, $2\n\t"                              \
    "sdl $2, 7+" p "\n\tsdr $2, " p "\n\tsdl $3, 15+" p "\n\tsdr $3, 8+" p
#define VU0_POINT                                                              \
    "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"                              \
    "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"                              \
    "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf16z\n\t"                              \
    "vmaddw.xyzw  $vf16, $vf31, $vf0w\n\t"
#define VU0_VEC4                                                               \
    "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"                              \
    "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"                              \
    "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf16z\n\t"                              \
    "vmaddw.xyzw  $vf16, $vf31, $vf16w\n\t"
#define VU0_DIR                                                                \
    "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"                              \
    "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"                              \
    "vmaddz.xyzw  $vf16, $vf30, $vf16z\n\t"

/* Waits for VU0. */
static inline void vu0_sync(void)
{
    __asm__ __volatile__("vnop");
}
/* out.xyz = M * (in.xyz, 1). */
static inline void vu0_xform_point(Vec3 *out, const Vec3 *in)
{
    __asm__ __volatile__(VU0_IN_V3("%1") VU0_POINT VU0_OUT_V3("%0")
                         : "=m"(*out) : "m"(*in) : "$2", "$3");
}
/* out = M * in (all four components). */
static inline void vu0_xform_vec4(Vec4 *out, const Vec4 *in)
{
    __asm__ __volatile__(VU0_IN_V4("%1") VU0_VEC4 VU0_OUT_V4("%0")
                         : "=m"(*out) : "m"(*in) : "$2", "$3");
}
/* out.xyz = M3x3 * in.xyz (a direction, no translation). */
static inline void vu0_xform_dir(Vec3 *out, const Vec3 *in)
{
    __asm__ __volatile__(VU0_IN_V3("%1") VU0_DIR VU0_OUT_V3("%0")
                         : "=m"(*out) : "m"(*in) : "$2", "$3");
}
/* out = vf31, the translation of the current matrix. */
static inline void vu0_get_translation(Vec4 *out)
{
    __asm__ __volatile__("qmfc2.ni $2, $vf31\n\tpcpyud $3, $2, $2\n\t"
                         "sdl $2, 7+%0\n\tsdr $2, %0\n\tsdl $3, 15+%0\n\tsdr $3, 8+%0"
                         : "=m"(*out) : : "$2", "$3");
}
#else
static inline void vu0_sync(void)
{
    __asm__ volatile("vnop");
}
static inline void vu0_xform_point(Vec3 *out, const Vec3 *in)
{
    f32 v[4] __attribute__((aligned(16)));
    v[0] = in->x; v[1] = in->y; v[2] = in->z; v[3] = 0.0f;
    __asm__ volatile(
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf31, $vf0w\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2];
}
static inline void vu0_xform_vec4(Vec4 *out, const Vec4 *in)
{
    f32 v[4] __attribute__((aligned(16)));
    v[0] = in->x; v[1] = in->y; v[2] = in->z; v[3] = in->w;
    __asm__ volatile(
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf31, $vf16w\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2]; out->w = v[3];
}
static inline void vu0_xform_dir(Vec3 *out, const Vec3 *in)
{
    f32 v[4] __attribute__((aligned(16)));
    v[0] = in->x; v[1] = in->y; v[2] = in->z; v[3] = 0.0f;
    __asm__ volatile(
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"
        "vmaddz.xyzw  $vf16, $vf30, $vf16z\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2];
}
static inline void vu0_get_translation(Vec4 *out)
{
    f32 v[4] __attribute__((aligned(16)));
    __asm__ volatile(
        "sqc2         $vf31, 0(%0)\n\t"
        : : "r"(v) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2]; out->w = v[3];
}
#endif

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft).  A volatile asm: none of the
 * image's 27 rsqrt.s is in a delay slot and the code around them stays in
 * source order (u032 found the same). */
static inline f32 ee_rsqrt(f32 num, f32 sq)
{
    f32 r;
    __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
}



/* The occlusion query's second argument (flare_query_screen's size). */
#define FLARE_QUERY_SIZE 10.0f

// FUNC 0x0019AD00 FlareClass::setNoOcclusionTest
//   Callers: RacePS2Base::drawView / virtual_47 (before begin).
//   Set: everything that passes the culling is drawn unoccluded (glow flags
//   + 2), without an occlusion query.
void FlareClass::setNoOcclusionTest(int on)
{
    noOcclusion = on;
}

// FUNC 0x0019AD08 FlareClass::addSource
//   Caller: FlareGroup::submit.  Projects the source with its
//   type's size and far distance; on screen, it joins the list (at most
//   100) with its view position, its directional factor and whether its
//   occlusion query was issued (facing away: none).
void FlareClass::addSource(FlareSource *src)
{
    Vec3 view;                              /* sp+0x00 */
    Vec3 screen;                            /* sp+0x10 */
    FlareType *type;
    FlareSourceEntry *entry;
    f32 facing;
    int n = nSources;

    if (n >= FLARE_MAX_ENTRIES)
        return;
    type = &types[src->type];
    entry = &sources[n];
    if (!project(&view, &screen, &src->pos, type->size, type->far))
        return;
    facing = directionalFactor(&view, &src->dir, type->exponent);
    if (facing < 0.0f)
        entry->queried = 0;
    else if (noOcclusion != 0)
        entry->queried = 1;
    else
        entry->queried = query(&screen, 1.0f, type->threshold);
    entry->view.x = view.x;
    entry->view.y = view.y;
    entry->view.z = view.z;
    entry->facing = facing;
    entry->src = src;
    nSources = nSources + 1;
}

// FUNC 0x0019AE30 FlareClass::project
//   view = M * pos (VU0, a point); screen = the projected x, y
//   (proj.m[0][0], m[1][1], divided by -z) and -z.  0 when behind the
//   camera (z >= 0), farther than far, or when the size x size square
//   around the point is off the -1..1 screen.  The half sizes are also
//   stored to the stack frame and never read (dead stores of the
//   original: a local array).  Matching (88.3%, the same with 991111-01):
//   retail returns the z tests' 0 straight to the epilogue (bc1tl with v0 =
//   0 in the slot) and the culling tests' 0 through a separate, 8-aligned
//   `v0 = 0' block; egcs merges every `return 0' here.  Tried: if chains,
//   one || test, a do { } while (0) with breaks, a result variable,
//   positive tests, the z tests combined.
int FlareClass::project(Vec3 *view, Vec3 *screen, const Vec3 *pos, f32 size, f32 far)
{
    f32 z, invDepth, sx, sy;
    f32 half[2];                            /* sp+0x0 (stored, read back from registers) */

    vu0_sync();
    vu0_xform_point(view, pos);
    z = view->z;
    invDepth = -1.0f / z;
    if (0.0f <= z)
        return 0;
    if (z < -far)
        return 0;
    sx = view->x * proj.m[0][0] * invDepth;
    screen->x = sx;
    sy = view->y * proj.m[1][1] * invDepth;
    invDepth = invDepth * size;
    screen->y = sy;
    screen->z = -view->z;
    half[0] = proj.m[0][0] * invDepth;
    half[1] = proj.m[1][1] * invDepth;
    if (sx - half[0] > 1.0f)
        return 0;
    if (sx + half[0] < -1.0f)
        return 0;
    if (sy - half[1] > 1.0f)
        return 0;
    if (sy + half[1] < -1.0f)
        return 0;
    return 1;
}

// FUNC 0x0019AF68 FlareClass::directionalFactor
//   n = (int)(exponent + 0.5); 0: 1.0.  Else the cosine between
//   M3x3 * dir (VU0) and the view position (dot / |view|, rsqrt.s): < 0
//   gives -1.0 (facing away), else its n-th power.
f32 FlareClass::directionalFactor(const Vec3 *view, const Vec3 *dir, f32 exponent)
{
    volatile Vec3 viewDir;                  /* sp+0x0 (volatile: see the VU0 helpers) */
    f32 len, cosine, factor;
    int i, n = (int)(exponent + 0.5f);

    if (n == 0)
        return 1.0f;
    vu0_sync();
    vu0_xform_dir((Vec3 *)&viewDir, dir);
    len = view->x * view->x + view->y * view->y + view->z * view->z;
    factor = ee_rsqrt(viewDir.x * view->x + viewDir.y * view->y + viewDir.z * view->z, len);
    if (factor < 0.0f)
        return -1.0f;
    cosine = factor;
    for (i = 0; i < n - 1; i++)
        factor *= cosine;
    return factor;
}

// FUNC 0x0019B070 FlareClass::query
//   G001's occlusion query flare_query_screen at the screen position (x,
//   y, depth), scale twice the size, size 10.0; the threshold argument
//   ($f13) is overwritten with 10.0 before the call (ignored).  1 when the
//   query was issued (its box on screen; u055 isVisible): whether there is
//   a query, not whether the point is hidden (the query's result decides
//   that when the glow is drawn: flare_draw_glow flags & 1).
int FlareClass::query(const Vec3 *screen, f32 size, f32 threshold)
{
    return flare_query_screen(flare_work(this), &screen->x, size + size, FLARE_QUERY_SIZE) != 0;
}

// FUNC 0x0019B098 FlareClass::FlareClass
//   Caller: RacePS2Base::RacePS2Base (member +0x9830).  The image pointers
//   zeroed, the two GSBuffers constructed (GSBuffer::GSBuffer), the
//   results pointer = (this + 0x13F) & ~0x3F, the three files read
//   (blocking, unpacked, from the gt3.vol device) and relocated, G001's disc
//   and rays models built, the light parameters and the 8 flare types set
//   to their defaults.  The EH landing pads destroy the GSBuffers
//   (~GSBuffer, flags 0).
FlareClass::FlareClass()
    : glareImg(NULL), flareBin(NULL), modelTex2(NULL), reflImg(NULL)
{
    /* implicit: GSBuffer (member constructors), ~GSBuffer (exception cleanup) */
    int i;

    results = (u32 *)(((int)resultsBuf + 0x3F) & ~0x3F);
    glareImg = (Tex1Image *)file_read_blocking("race/glare.img", NULL, 0, 1, &vol_device_object);
    tex1_relocate(glareImg);
    flareBin = (VifModel *)file_read_blocking("race/flare.bin", NULL, 0, 1, &vol_device_object);
    vif_model_relocate(flareBin);
    reflImg = (Tex1Image *)file_read_blocking("race/reflection.img", NULL, 0, 1, &vol_device_object);
    tex1_relocate(reflImg);
    flare_build_disc();
    flare_build_rays();
    results[0] = 0;
    sunVisible = 0;
    sunType = 0;
    setLightParams(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1000.0f, 0.0f);
    for (i = 0; i < 8; i++) {
        setFlareType(i, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                     0.0f, 0.0f, 1000.0f, 0.0f, 0.0f);
        types[i].phase = 0.0f;
    }
}

// FUNC 0x0019B2B8 FlareClass::~FlareClass
//   Frees the files (reflImg and glareImg unconditionally, flareBin and
//   modelTex2 when set), then the GSBuffers (~GSBuffer flags 2: workBuf, then
//   occBuf); flags & 1: operator delete.  Callers: RacePS2Base::RacePS2Base
//   (EH), RacePS2Base::~RacePS2Base.
FlareClass::~FlareClass()
{
    free(reflImg);
    if (modelTex2 != NULL)
        free(modelTex2);
    if (flareBin != NULL)
        free(flareBin);
    free(glareImg);
    /* implicit: ~GSBuffer (member destructors), operator delete (deleting destructor) */
}

// FUNC 0x0019B358 FlareClass::begin
//   Callers: RacePS2Base::drawView / virtual_47.  The VRAM of
//   the occlusion result strip (64 x 8, PSMCT32) and of the reduction work
//   buffer (64 x 64), flare_init for the screen buffer (aspect 0.911, 1.214
//   on the wide screen; the models' textures glareImg / modelTex2), the lists
//   emptied, the projection matrix kept (pgl_get_matrix in matrix mode
//   PROJECTION).
void FlareClass::begin(GSBuffer &screen)
{
    f32 aspect;
    int screenVram, screenW;

    /* implicit: allocVram (GSBuffer's inline allocVram(psm, w, h), twice:
     * retail keeps the buffer's address in a register for the three
     * stores, the inline member's this) */
    occBuf.allocVram(0, 0x40, 8);
    aspect = 0.911f;
    workBuf.allocVram(0, 0x40, 0x40);
#if ENH_RENDER_FLARE_NO_FOG
    /* Lupo 2 (L2 0x00198A04, rendering.md 10): fog off before the flare
     * buffers are set up */
    gfx_disable(PGL_CAP_FOG);
#endif
    if (g_002F424C != 0)
        aspect = 1.214f;
    screenVram = screen.addr;
    screenW = screen.width;
    flare_init(flare_work(this), aspect, screenVram, screenW, screen.displayHeight(), occBuf.addr,
               8, workBuf.addr, 0x40, 0x40, glareImg, modelTex2);
    sunVisible = 0;
    nLights = 0;
    nSources = 0;
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_get_matrix(&proj.m[0][0]);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    unk21A4 = proj.m[1][1];
}

// FUNC 0x0019B460 FlareClass::setLightParams
//   Callers: the constructor, T178's course_env_set_lamps (the course's
//   values).  glowScale is stored * 8, threshold * 128.
void FlareClass::setLightParams(f32 r, f32 g, f32 b, f32 glowScale, f32 size, f32 haloAlpha,
                                f32 att0, f32 att1, f32 att2, f32 far, f32 threshold)
{
    params.r = r;
    params.g = g;
    params.b = b;
    params.glowScale = glowScale * 8.0f;
    params.size = size;
    params.haloAlpha = haloAlpha;
    params.att0 = att0;
    params.att1 = att1;
    params.att2 = att2;
    params.far = far;
    params.threshold = threshold * 128.0f;
}

// FUNC 0x0019B4B8 FlareClass::setFlareType
//   Callers: the constructor, T178's course_env_set_flare_types (the course's
//   types).  glowScale is stored * 10, threshold * 128; rate = 1 / period
//   (0: 0, steady).
void FlareClass::setFlareType(int i, f32 r, f32 g, f32 b, f32 glowScale, f32 size,
                              f32 haloAlpha, f32 exponent, f32 att0, f32 att1, f32 att2,
                              f32 far, f32 threshold, f32 period)
{
    types[i].r = r;
    types[i].g = g;
    types[i].b = b;
    types[i].glowScale = glowScale * 10.0f;
    types[i].size = size;
    types[i].exponent = exponent;
    types[i].haloAlpha = haloAlpha;
    types[i].att0 = att0;
    types[i].att1 = att1;
    types[i].att2 = att2;
    types[i].far = far;
    types[i].threshold = threshold * 128.0f;
    if (period == 0.0f)
        types[i].rate = 0.0f;
    else
        types[i].rate = 1.0f / period;
}

// FUNC 0x0019B568 FlareClass::drawSunDisc
//   The sun disc: every register passes through to G001's
//   flare_draw_sun_disc(this, pos, r, g, b, a, scale).  Caller: T178's
//   course_env_draw_sun_disc (the sun direction * 10).
void FlareClass::drawSunDisc(const Vec3 *pos, f32 r, f32 g, f32 b, f32 a, f32 scale)
{
    flare_draw_sun_disc(flare_work(this), &pos->x, r, g, b, a, scale);
}

// FUNC 0x0019B588 FlareClass::setSun
//   Caller: T178's course_env_set_sun (it sets sunType itself).  Keeps the
//   sun's direction and its glare's scale and colour for draw; $f12 / $f13
//   are left untouched for flare_query_sun (its scale and size): the
//   sun's occlusion query.  sunVisible = it was issued.
void FlareClass::setSun(const Vec3 *pos, f32 queryScale, f32 querySize, f32 scale, f32 r,
                        f32 g, f32 b)
{
    sun[0] = scale;
    sunPos.x = pos->x;
    sunPos.y = pos->y;
    sunPos.z = pos->z;
    sun[1] = r;
    sun[2] = g;
    sun[3] = b;
    sunVisible = flare_query_sun(flare_work(this), &sunPos.x, queryScale, querySize) != 0;
}

// FUNC 0x0019B5E8 FlareClass::spotFactor
//   As directionalFactor, with dir already in view space.
f32 FlareClass::spotFactor(const Vec3 *view, const Vec3 *dir, f32 exponent)
{
    f32 cosine, factor;
    int i, n = (int)(exponent + 0.5f);

    if (n == 0)
        return 1.0f;
    factor = ee_rsqrt(dir->x * view->x + dir->y * view->y + dir->z * view->z,
                      view->x * view->x + view->y * view->y + view->z * view->z);
    if (factor < 0.0f)
        return -1.0f;
    cosine = factor;
    for (i = 0; i < n - 1; i++)
        factor *= cosine;
    return factor;
}

// FUNC 0x0019B6B0 FlareClass::addLight
//   Caller: RaceCarModel::addLamps (T180: the car's lamps, 0x80
//   bytes each).  Projects light->flarePos with params.size * max(sizeA,
//   sizeB); on screen, it joins the list (at most 100).  An elongated lamp
//   (|sizeA - sizeB| >= 0.01) gets its projected axis (M * axis, VU0 with
//   w) and loses the spot test; a round one has the axis (1, 1).  The spot
//   factor (exponent: light->exponent as unsigned) is computed only when
//   `directional'; otherwise it is -1 and the lamp gets no occlusion query
//   (visible 0).  Matching (86.7%, the same with 991111-01): retail
//   recomputes sizeA - sizeB for the axis scale (GCSE reuses the round
//   test's difference here, which also turns the in-place neg.s into a
//   copy) and keeps light in $s0, entry in $s1 (ours the other way round).
//   Tried: the round test as an inline helper, before / after the size
//   loads, the explicit field reloads, declaration orders.
void FlareClass::addLight(CarLamp *light, f32 intensity, int directional)
{
    Vec3 view;                              /* sp+0x00 */
    Vec3 screen;                            /* sp+0x10 */
    volatile Vec4 xf;                       /* sp+0x20 the projected axis, then direction */
    FlareLightParams *p;
    FlareLightEntry *entry;
    f32 size, sizeA, sizeB, diff, axisScale, spot;
    int round, n = nLights;

    if (n >= FLARE_MAX_ENTRIES)
        return;
    sizeA = light->sizeA;
    sizeB = light->sizeB;
    p = &params;
    size = p->size;
    entry = &lights[n];
    if (sizeA < sizeB)
        size = size * sizeB;
    else
        size = size * sizeA;
    if (!project(&view, &screen, &light->flarePos, size, p->far))
        return;
    sizeA = light->sizeA;
    sizeB = light->sizeB;
    diff = sizeA - sizeB;
    if (diff < 0.0f)
        diff = -diff;
    round = diff < FLARE_ROUND_EPS;
    if (!round) {
        directional = 0;
        vu0_sync();
        vu0_xform_vec4((Vec4 *)&xf, &light->axis);
        axisScale = xf.x * xf.x + xf.y * xf.y;
        if (axisScale == 0.0f) {
            entry->axisX = sizeA;
            entry->axisY = 0.0f;
        } else {
            axisScale = (sizeA - sizeB) + ee_rsqrt(sizeB, axisScale);
            entry->axisY = xf.y * axisScale;
            entry->axisX = xf.x * axisScale;
        }
    } else {
        entry->axisY = 1.0f;
        entry->axisX = 1.0f;
    }
    /* overwrites xf.xyz (xf.w keeps the axis' w) */
    vu0_sync();
    vu0_xform_dir((Vec3 *)&xf, (const Vec3 *)&light->dir);
    spot = -1.0f;
    if (directional != 0)
        spot = spotFactor(&view, (const Vec3 *)&xf, (f32)light->exponent);
    if (spot < 0.0f)
        entry->queried = 0;
    else if (noOcclusion != 0)
        entry->queried = 1;
    else
        entry->queried = query(&screen, 1.0f, p->threshold);
    vu0_sync();
    vu0_xform_point(&entry->view, &light->pos);
    entry->light = light;
    entry->intensity = spot * intensity;
    nLights = nLights + 1;
    entry->depth = xf.z * -0.5f;
}

// FUNC 0x0019B9B8 FlareClass::flush
//   Callers: RacePS2Base::drawView / virtual_47 (after the
//   submissions).  Frees workBuf's VRAM; keeps the frame's query count; when
//   there is something to draw (queries, lamps or sources):
//   VramAllocator::flushReleased and flare_readback (the results into
//   results, the count restarted); then occBuf's VRAM.
void FlareClass::flush()
{
    workBuf.freeVram();
    queryCount = queries;
    if (queryCount == 0 && nLights == 0 && nSources == 0) {
        occBuf.freeVram();
        return;
    }
    g_002F4250.flushReleased();
    flare_readback(flare_work(this), results);
    occBuf.freeVram();
}

// FUNC 0x0019BA38 FlareClass::draw
//   Callers: RacePS2Base::drawView / virtual_47.  Nothing when
//   nothing was queried or listed.  The render state (PGL_CAP_VU1_PROG_33E
//   on; depth test, culling, alpha tests off; additive blending), the sun
//   glare (sunType 0: glareImg made resident; the model flare_sun_model_a,
//   or flare_rays_model / flare_disc_model for types 1 / 2), modelTex2 made
//   resident when there are lamps or sources, then the lamps (the model
//   flareBin, or flare_sun_model_b) and the sources, each attenuated by its
//   distance.  A lamp without its query and elongated is drawn as a streak
//   (flare_draw_streak, axes from its projected axis); everything else by
//   flare_draw_glow with flags = visible (draw the model) + 2 with
//   noOcclusion (unoccluded).  A flickering type (rate > 0) is modulated by
//   sin((type phase + source phase) * 2 pi) * 0.5 + 0.5.  Culling back on
//   at the end.  Matching (95.1%, the same with 991111-01): what is left is
//   local allocation in the lamp loop ($v0 / $v1 for the lamp and the query
//   flag, sizeA loaded before sizeB in retail) and the delay slot of the
//   round test's bnel.  The parameters are read through a pointer for the
//   attenuation, size and glow scale and directly for the colour and halo
//   alpha, as retail addresses them ($s0 = &params vs $s1 + 0x21E8).
void FlareClass::draw()
{
    int i, flags;

    if (queryCount == 0 && nLights == 0 && nSources == 0)
        return;
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD), 0.0f);  /* 0x48 */
    if (sunVisible != 0) {
        if (sunType == 0)
            g_002F4250.loadTex1(glareImg);
        if (sunVisible != 0) {
            VifModel *sunModel = flare_sun_model_a();
            switch (sunType) {
            case 1:
                sunModel = flare_rays_model();
                break;
            case 2:
                sunModel = flare_disc_model();
                break;
            }
            flare_draw_sun(flare_work(this), &sunPos.x, sunModel, sun[0], sun[1], sun[2], sun[3]);
        }
    }
    if (modelTex2 != NULL && (nLights != 0 || nSources != 0))
        g_002F4250.loadTex1(modelTex2);

    pgl_push_matrix();
    for (i = 0; i < nLights; i++) {
        FlareLightParams *p = &params;
        VifModel *model;
        FlareLightEntry *entry;
        CarLamp *light;
        f32 z, att, r, g, b, diff;
        int round;

        if (flareBin != NULL)
            model = flareBin;
        else
            model = flare_sun_model_b();
        entry = &lights[i];
        light = entry->light;
        z = -entry->view.z;
        att = 1.0f / (z * z * p->att2 + z * p->att1 + p->att0);
        r = light->r * params.r * att;
        g = light->g * params.g * att;
        b = light->b * params.b * att;
        if (entry->queried == 0) {
            diff = light->sizeA - light->sizeB;
            if (diff < 0.0f)
                diff = -diff;
            round = diff < FLARE_ROUND_EPS;
            if (!round) {
                f32 axes[4];                /* sp+0x00 the streak's 2x2 matrix */
                f32 k = ee_rsqrt(light->sizeB,
                                 entry->axisX * entry->axisX + entry->axisY * entry->axisY);
                axes[1] = entry->axisY * p->size;
                axes[0] = entry->axisX * p->size;
                axes[3] = axes[0] * k;
                axes[2] = -axes[1] * k;
                flare_draw_streak(flare_work(this), &entry->view.x, axes, model, entry->depth,
                                  r, g, b, params.haloAlpha);
                continue;
            }
        }
        flags = (entry->queried != 0) + (noOcclusion != 0 ? 2 : 0);
        flare_draw_glow(flare_work(this), flags, &entry->view.x, model, entry->depth, r, g, b,
                        entry->intensity, p->glowScale * light->sizeA, params.haloAlpha,
                        p->size * light->sizeA);
    }
    pgl_pop_matrix();

    pgl_push_matrix();
    for (i = 0; i < nSources; i++) {
        VifModel *model;
        FlareSourceEntry *entry;
        FlareSource *src;
        FlareType *type;
        f32 z, att;

        if (flareBin != NULL)
            model = flareBin;
        else
            model = flare_sun_model_b();
        entry = &sources[i];
        src = entry->src;
        z = -entry->view.z;
        type = &types[src->type];
        att = 1.0f / (z * z * type->att2 + z * type->att1 + type->att0);
        if (0.0f < type->rate)
            att *= sinf((type->phase + src->phase) * FLARE_TWO_PI) * 0.5f + 0.5f;
        flags = (entry->queried != 0) + (noOcclusion != 0 ? 2 : 0);
        flare_draw_glow(flare_work(this), flags, &entry->view.x, model, 0.5f, type->r * att,
                        type->g * att, type->b * att, entry->facing, type->glowScale, type->haloAlpha,
                        type->size);
    }
    pgl_pop_matrix();
    gfx_enable(PGL_CAP_CULL_FACE);
}

// FUNC 0x0019BE98 FlareClass::update
//   Caller: RacePS2Base::virtual_28.  Advances the flicker phase
//   of the 8 types by rate * dt, wrapped once past 1.
void FlareClass::update(f32 dt)
{
    int i;

    for (i = 0; i < 8; i++) {
        types[i].phase = types[i].phase + types[i].rate * dt;
        if (types[i].phase > 1.0f)
            types[i].phase = types[i].phase - 1.0f;
    }
}

// FUNC 0x0019BEF0 FlareClass::setReflectionParams
//   Caller: T178's course_env_set_reflection.  The colour is stored * 128.
void FlareClass::setReflectionParams(f32 r, f32 g, f32 b, f32 width0, f32 width1,
                                     f32 maxLength, f32 top)
{
    reflR = r * 128.0f;
    reflG = g * 128.0f;
    reflB = b * 128.0f;
    reflWidth0 = width0;
    reflWidth1 = width1;
    reflMaxLength = maxLength;
    reflTop = top;
}

// FUNC 0x0019BF28 FlareClass::beginReflections
//   Caller: RacePS2Base::drawLampReflections.  The render state of the
//   reflection pass (alpha test off; blending, depth test, texturing on; no
//   depth writes; Cs * FIX + Cd with FIX 1.0, additive; culling off; TCC 1,
//   TFX MODULATE), race/reflection.img made resident and selected, then
//   strip_batch_begin with the identity VU0 matrix.
void FlareClass::beginReflections()
{
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_TEXTURE);
    gfx_depth_mask(0);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD), 1.0f);  /* 0x68 */
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    g_002F4250.loadTex1(reflImg);
    gfx_set_texture_regs(reflImg->tex);
    pgl_push_matrix();
    vu0_load_identity_regs();
    strip_batch_begin();
    pgl_pop_matrix();
}

/* The reflection quad's UVs (T159 .rodata, after the file names; extern:
 * egcs writes an internal const at the end of the unit). */
extern const f32 g_0032F200[4][2];
const f32 g_0032F200[4][2] __attribute__((aligned(16))) = {     /* 0x0032F200 */
    { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f },
};

// FUNC 0x0019BFE0 FlareClass::drawReflection
//   Caller: RaceCarModel::drawLampReflections (T180, the car's
//   lamps).  The lamp's position in view space is the translation of the
//   current matrix after pgl_translate (vf31).  Nothing when it is not in
//   front (z + 0.5 > 0) or faces away (the cosine between M * dir and the
//   position < 0).  Else a quad (4 x 12 bytes, z = depth) at the position
//   projected to the depth plane, its width between reflWidth1 (from the
//   side) and reflWidth0 (facing) times sizeA, from reflTop above to
//   min(the lamp's height, reflMaxLength) below; the colour lamp *
//   intensity * facing * refl, each clamped to 255, packed R | G << 8 | B
//   << 16 (alpha 0), with the UVs g_0032F200: strip_batch_add.  Matching
//   (95.9%, the same with 991111-01): retail loads reflTop before the
//   length test but subtracts after it, and computes left, top, bottom,
//   right in that order; 120 orders of the width / length / corner
//   statements (and 24 vertex orders, inline corners, an early reflTop
//   local) reach 95.9% at best.
void FlareClass::drawReflection(CarLamp *light, f32 intensity)
{
    volatile Vec4 lamp;                     /* sp+0x00 the lamp in view space */
    volatile Vec4 dir;                      /* sp+0x10 its direction in view space */
    Vec3 *quad;
    u32 *colors;
    u32 color;
    f32 depth, facing, k, x, y, width, length, left, right, top, bottom, r, g, b;

    pgl_push_matrix();
    pgl_translate(light->pos.x, light->pos.y, light->pos.z);
    vu0_sync();
    vu0_get_translation((Vec4 *)&lamp);
    depth = lamp.z + 0.5f;
    if (0.0f < depth) {
        pgl_pop_matrix();
        return;
    }
    vu0_xform_vec4((Vec4 *)&dir, &light->dir);
    facing = ee_rsqrt(dir.x * lamp.x + dir.y * lamp.y + dir.z * lamp.z,
                      lamp.x * lamp.x + lamp.y * lamp.y + lamp.z * lamp.z);
    if (facing < 0.0f) {
        pgl_pop_matrix();
        return;
    }
    intensity = intensity * facing;
    k = depth / lamp.z;
    x = lamp.x * k;
    y = lamp.y * k;
    quad = (Vec3 *)gfx_alloc_tail(3);
    colors = (u32 *)gfx_alloc_tail(1);
    vu0_load_identity_regs();
    width = ((reflWidth0 - reflWidth1) * facing + reflWidth1) * light->sizeA;
    top = y - reflTop;
    length = reflMaxLength;
    if (light->pos.y < length)
        length = light->pos.y;
    right = x + width;
    bottom = y + length;
    left = x - width;
    quad[0].x = right;
    quad[0].y = bottom;
    quad[0].z = depth;
    quad[1].x = left;
    quad[1].y = bottom;
    quad[1].z = depth;
    quad[2].x = right;
    quad[2].y = top;
    quad[2].z = depth;
    quad[3].x = left;
    quad[3].y = top;
    quad[3].z = depth;
    r = light->r * intensity * reflR;
    g = light->g * intensity * reflG;
    b = light->b * intensity * reflB;
    if (r > 255.0f)
        r = 255.0f;
    if (g > 255.0f)
        g = 255.0f;
    if (b > 255.0f)
        b = 255.0f;
    color = (u32)((int)r | ((int)g << 8) | ((int)b << 16));
    colors[0] = colors[1] = colors[2] = colors[3] = color;
    strip_batch_add(quad, NULL, colors, g_0032F200, 4, NULL);
    pgl_pop_matrix();
}

// FUNC 0x0019C2D0 FlareClass::endReflections
//   Caller: RacePS2Base::drawLampReflections.  strip_batch_end with the
//   identity VU0 matrix, depth writes back on, culling back on.
void FlareClass::endReflections()
{
    pgl_push_matrix();
    vu0_load_identity_regs();
    strip_batch_end();
    pgl_pop_matrix();
    gfx_depth_mask(1);
    gfx_enable(PGL_CAP_CULL_FACE);
}

// FUNC 0x0019C318 FlareClass::F_0019C318
//   No callers.  Returns 0 (? a getter of something never kept).
int FlareClass::F_0019C318()
{
    return 0;
}

// FUNC 0x0019C320 FlareClass::setSunType
//   No callers (T178's course_env_set_sun stores the word itself).
void FlareClass::setSunType(int type)
{
    sunType = type;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (egcs emits it with the vtable). */
// FUNC 0x0019C328 __tf10FlareClass
//   __user_type_info __ti10FlareClass (0x003533A8, gp-0x4648), name
//   0x0032F230.
extern "C" const type_info &__tf10FlareClass(void)
{
    if (__ti10FlareClass[0] == 0)
        __rtti_user(__ti10FlareClass, "10FlareClass");
    return *(const type_info *)__ti10FlareClass;
}
#endif
