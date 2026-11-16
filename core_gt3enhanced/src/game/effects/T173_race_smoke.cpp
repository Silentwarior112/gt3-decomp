/* T173_race_smoke.cpp - RaceSmoke, the tyre smoke / dust particles of a
 * race (0x001A45A8..0x001A6138).
 *
 * 256 particles of 0x40 bytes in one memalign'ed block, on two lists
 * (active, free) linked by s16 indices.  The wheels emit them (emit, from
 * RaceCourse::emitSmoke) with a smoke type of the course environment
 * (CourseEnv::smoke, 16 types of 0x44 bytes); update moves them every
 * frame (spin, growth, fade, rise and gravity) and frees the faded ones and
 * the ones fallen under their ground plane.  render draws them, at the full
 * resolution or into a reduced offscreen buffer (CourseEnv::smokeDivX / Y,
 * 1..64) added over the frame; draw transforms them to view space with the
 * VU0 matrix (vf28..vf31), culls them against the depth range and the
 * screen, sorts them back to front (quicksort on the view z, unless the
 * blend is additive) and writes one GIF packet: a camera-facing quad per
 * particle (strips of RGBAQ / ST / XYZF, projected with vf20..vf27).  The
 * small particles (size <= 3) are split by their ground plane: the part
 * above is drawn as is, the part below is projected onto the ground (along
 * the view ray, its spread clamped to twice the size) and fades to alpha 0.
 */
#include "game/race/pause/replay_pause_internal.h"
#include "vu0asm.h"                         /* VU0_ACC, VU0_Q */
#include "libc.h"                           /* memalign, free */
#include "libgraph.h"                       /* SCE_GS_PSMCT32, SCE_GS_PSMZ24 */
#include "engine/pgl/G001_flare.h"                /* gfx_* / pgl_* / gs_* / vu0_* and their constants, Tex1Image */
#include "game/course/T178_course_env.h"           /* CourseEnv (smoke types, divisors, blend, flags) */
#include "game/effects/T334_tyre_smoke.h"           /* SmokeSource */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250, the VRAM allocator */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/effects/T173_race_smoke.h"           /* + T159 Vec3 / Vec4, T131 Matrix44 */

/* ======================================================================== */
/* T173: the smoke                                                          */
/* ======================================================================== */

enum {
    SMOKE_PARTICLES = 256,                  /* RaceSmoke::particles */
    SMOKE_TYPE_COUNTERS = 11,               /* RaceSmoke::emitCount */
    SMOKE_DIV_MAX   = 64                    /* CourseEnv::smokeDivX / Y: 1..64 (else 1) */
};

/* The blend modes (gfx_blend_func): CourseEnv::smokeBlend is one of the
 * first two. */
enum {
    /* Cs x As + Cd: additive, so the particles are drawn unsorted */
    SMOKE_BLEND_ADD     = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD),   /* 0x48 */
    /* (Cs - Cd) x As + Cd: the usual alpha blend, sorted back to front */
    SMOKE_BLEND_ALPHA   = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD),     /* 0x44 */
    /* Cs x FIX + Cd: render's offscreen buffer added over the frame */
    SMOKE_BLEND_COMPOSE = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD)   /* 0x68 */
};

/* render's saved GS state (pgl_push_attrib): 0x887. */
enum {
    SMOKE_RENDER_ATTRIBS = PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B
                         | PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR
};

/* The CourseEnv::flags bit of the smoke (T178): its alpha fades with the
 * distance like the fog. */
enum { ENV_SMOKE_FOG = 0x40000 };

/* GIF tags (EE User's Manual, GIF): NLOOP bits 0..14, EOP bit 15, FLG bits
 * 58..59 (0 PACKED, 1 REGLIST), NREG bits 60..63; the register descriptors
 * in the second word. */
#define GIF_TAG_EOP         0x8000ULL
#define GIF_TAG_REGLIST     (1ULL << 58)
#define GIF_TAG_NREG(n)     ((u64)(n) << 60)
#define GS_REG_PRIM         0x00ULL         /* the PRIM register's address (A+D) */
#define GS_PRIM_TRISTRIP    4               /* PRIM.PRIM: triangle strip */
/* The strip's register list: RGBAQ ST XYZF3 ST XYZF3 RGBAQ, then ST XYZF2
 * pairs (1 RGBAQ, 2 ST, 0xC XYZF3, 4 XYZF2; first descriptor in the low
 * nibble). */
#define SMOKE_STRIP_REGS    0x42424242421C2C21ULL
#define F32_ONE_BITS        0x3F800000      /* 1.0f: the Q of RGBAQ */

/* ------------------------------------------------------------------------ */
/* EE helpers                                                               */
/* ------------------------------------------------------------------------ */

/* The original's prefetches are `pref 0, <stack slot>': the pointer to the
 * next particle is stored to the stack and the asm operand is that stack
 * word ("m" of the pointer variable itself), so the prefetch is of the
 * stack, not of the particle.  Reproduced as such (no effect on the data). */
#define U058_PREF(var) __asm__ volatile("pref 0, %0" : : "m"(var))

/* VU0 macro mode.  The original moves the operands through the GPRs,
 * unaligned (ldl / ldr, lwl / lwr + pcpyld + qmtc2.ni in; qmfc2.ni + pcpyud
 * + sdl / sdr, swl / swr out): so do the egcs versions (as u057's T167
 * helpers; a Vec3's w lane is the sign of its z, unused).  GCC 14 goes
 * through an aligned temporary (the same values). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)

/* out = vf28..vf31 x (in, 1) (the current matrix: vmaddw vf31, vf0w); x, y,
 * z stored (draw's view-space positions). */
static inline void u058_xform_point(Vec3 *out, const Vec3 *in)
{
    u128 lo, hi;

    __asm__ volatile(
        "vnop\n\t"
        "ldl          %0, 7+%2\n\t"
        "ldr          %0, %2\n\t"
        "lwl          %1, 11+%2\n\t"
        "lwr          %1, 8+%2\n\t"
        "pcpyld       %1, %1, %0\n\t"
        "qmtc2.ni     %1, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf28, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf29, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf30, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf31, $vf0w\n\t"
        "qmfc2.ni     %1, $vf16\n\t"
        "pcpyud       %0, %1, %1"
        : "=&r"(lo), "=&r"(hi) : "m"(*in));
    __asm__ volatile(
        "sdl          %2, 7+%0\n\t"
        "sdr          %2, %0\n\t"
        "swl          %1, 11+%0\n\t"
        "swr          %1, 8+%0"
        : "=m"(*out) : "r"(lo), "r"(hi));
}

/* out = *pm x in, all four components: the matrix is loaded into
 * vf20..vf23 (which stay clobbered: emitStrip reloads them), vmaddw vf23,
 * vf16w.  The matrix pointer is read after the vnop (retail: the vnop,
 * then the lw of planeMatrix): it is passed by address. */
static inline void u058_xform_plane(Vec4 *out, const Vec4 *in, Matrix44 *const *pm)
{
    u128 lo, hi;
    const Matrix44 *m;

    __asm__ volatile("vnop");
    m = *pm;
    __asm__ volatile(
        "lqc2         $vf20, 0x00(%0)\n\t"
        "lqc2         $vf21, 0x10(%0)\n\t"
        "lqc2         $vf22, 0x20(%0)\n\t"
        "lqc2         $vf23, 0x30(%0)"
        : : "r"(m));
    __asm__ volatile(
        "ldl          %0, 7+%2\n\t"
        "ldr          %0, %2\n\t"
        "ldl          %1, 15+%2\n\t"
        "ldr          %1, 8+%2\n\t"
        "pcpyld       %1, %1, %0\n\t"
        "qmtc2.ni     %1, $vf16\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf23, $vf16w\n\t"
        "qmfc2.ni     %1, $vf16\n\t"
        "pcpyud       %0, %1, %1"
        : "=&r"(lo), "=&r"(hi) : "m"(*in));
    __asm__ volatile(
        "sdl          %2, 7+%0\n\t"
        "sdr          %2, %0\n\t"
        "sdl          %1, 15+%0\n\t"
        "sdr          %1, 8+%0"
        : "=m"(*out) : "r"(lo), "r"(hi));
}

#else /* GCC 14 */

/* out = vf28..vf31 x (in, 1) (the current matrix: vmaddw vf31, vf0w); x, y,
 * z stored (draw's view-space positions). */
static inline void u058_xform_point(Vec3 *out, const Vec3 *in)
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

/* out = *pm x in, all four components: the matrix is loaded into
 * vf20..vf23 (which stay clobbered: emitStrip reloads them), vmaddw vf23,
 * vf16w. */
static inline void u058_xform_plane(Vec4 *out, const Vec4 *in, Matrix44 *const *pm)
{
    f32 v[4] __attribute__((aligned(16)));
    const Matrix44 *m = *pm;

    v[0] = in->x; v[1] = in->y; v[2] = in->z; v[3] = in->w;
    __asm__ volatile(
        "vnop\n\t"
        "lqc2         $vf20, 0x00(%1)\n\t"
        "lqc2         $vf21, 0x10(%1)\n\t"
        "lqc2         $vf22, 0x20(%1)\n\t"
        "lqc2         $vf23, 0x30(%1)\n\t"
        "lqc2         $vf16, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf16x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf16y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf16z\n\t"
        "vmaddw.xyzw  $vf16, $vf23, $vf16w\n\t"
        "sqc2         $vf16, 0(%0)\n\t"
        : : "r"(v), "r"(m) : "memory");
    out->x = v[0]; out->y = v[1]; out->z = v[2]; out->w = v[3];
}

#endif

/* The screen projection of emitStrip: vf1 = vf24..vf27 x (p, 1) (the
 * projection); Q = 1 / w; vf1 = min(vf1, vf21) (the limits); vf1 = vf22 +
 * vf1 x Q x vf23 (viewCenter + p x viewScale); ftoi4.  vf20..vf23 are
 * loaded by vu0_load_screen_regs.  The vdiv is issued before the RGBAQ / ST
 * words of the vertex are written in the original; the order does not
 * matter. */
static inline void u058_project_vertex(s32 out[4], const Vec3 *p)
{
    f32 v[4] __attribute__((aligned(16)));
    s32 r[4] __attribute__((aligned(16)));

    v[0] = p->x; v[1] = p->y; v[2] = p->z; v[3] = 0.0f;
    __asm__ volatile(
        "lqc2         $vf1, 0(%0)\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        "vdiv         " VU0_Q ", $vf0w, $vf1w\n\t"
        "vmini.xyzw   $vf1, $vf1, $vf21\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf1, $vf1, " VU0_Q "\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
        "vftoi4.xyzw  $vf1, $vf1\n\t"
        "sqc2         $vf1, 0(%1)\n\t"
        : : "r"(v), "r"(r) : "memory");
    out[0] = r[0]; out[1] = r[1]; out[2] = r[2]; out[3] = r[3];
}

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft). */
static inline f32 u058_rsqrt(f32 num, f32 sq)
{
    f32 r;

    __asm__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
}

/* ------------------------------------------------------------------------ */
/* The tables (retail's .rodata 0x003306D0..0x00330870, all of T173's;     */
/* declared in T173_race_smoke.h, so written where they are defined)       */
/* ------------------------------------------------------------------------ */

/* The smoke type (CourseEnv::smoke index) of a SmokeSource::kind, the
 * wheel's surface. */
const int g_003306D0[24] = {               /* 0x003306D0 */
    0, 0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

/* The alpha of a particle variant (x the source's rate: emit). */
const f32 g_00330730[8] = {                 /* 0x00330730 */
    0.995322f, 0.00440771f, 0.118569f, 0.510142f,
    0.0352504f, 0.767226f, 0.276326f, 0.941884f
};

/* The spin of a particle variant (x the type's spin / size: update). */
const f32 g_00330750[8] = {                 /* 0x00330750 */
    0.00661152f, 1.0f, 0.726524f, 0.404117f,
    0.99552f, 0.177035f, 0.699705f, 0.0528541f
};

/* The uvs of a quad's corners for each of the four images (the vertical
 * quarters of the 256-texel wide texture, half a texel in, as 256ths: u
 * 1..63, 65..127, 129..191, 193..255; v 2..254), in strip order (drawQuad,
 * drawLoopQuad) ... */
const f32 g_00330770[4][4][2] = {           /* 0x00330770 */
    { { 0.00390625f, 0.0078125f }, { 0.24609375f, 0.0078125f },
      { 0.00390625f, 0.9921875f }, { 0.24609375f, 0.9921875f } },
    { { 0.25390625f, 0.0078125f }, { 0.49609375f, 0.0078125f },
      { 0.25390625f, 0.9921875f }, { 0.49609375f, 0.9921875f } },
    { { 0.50390625f, 0.0078125f }, { 0.74609375f, 0.0078125f },
      { 0.50390625f, 0.9921875f }, { 0.74609375f, 0.9921875f } },
    { { 0.75390625f, 0.0078125f }, { 0.99609375f, 0.0078125f },
      { 0.75390625f, 0.9921875f }, { 0.99609375f, 0.9921875f } }
};

/* ... and in loop order (drawClippedQuad's polygon). */
const f32 g_003307F0[4][4][2] = {           /* 0x003307F0 */
    { { 0.00390625f, 0.0078125f }, { 0.24609375f, 0.0078125f },
      { 0.24609375f, 0.9921875f }, { 0.00390625f, 0.9921875f } },
    { { 0.25390625f, 0.0078125f }, { 0.49609375f, 0.0078125f },
      { 0.49609375f, 0.9921875f }, { 0.25390625f, 0.9921875f } },
    { { 0.50390625f, 0.0078125f }, { 0.74609375f, 0.0078125f },
      { 0.74609375f, 0.9921875f }, { 0.50390625f, 0.9921875f } },
    { { 0.75390625f, 0.0078125f }, { 0.99609375f, 0.0078125f },
      { 0.99609375f, 0.9921875f }, { 0.75390625f, 0.9921875f } }
};

// FUNC 0x001A45A8 RaceSmoke::RaceSmoke
//   The particle block (memalign(0x40, 0x4000)), no texture, reset.  (env
//   is set by RacePS2Base::virtual_8.)  Caller: RacePS2Base's constructor.
RaceSmoke::RaceSmoke()
{
    void *block = memalign(0x40, SMOKE_PARTICLES * sizeof(SmokeParticle));

    texture = NULL;
    particles = (SmokeParticle *)block;
    reset();
}

// FUNC 0x001A45E8 RaceSmoke::~RaceSmoke
//   Frees the particle block (and the object with egcs flags bit 0:
//   operator delete).
RaceSmoke::~RaceSmoke()
{
    free(particles);
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x001A4630 RaceSmoke::reset
//   No active particle; the free list 0, 1, .., 255 (255's next = -1); the
//   11 emission counters cleared; the VU0 random seed 0.1 (vu0_rand_seed).
//   (Both loops count up in the source: loop.c reverses them, retail stores
//   from the last element down.)  Callers: gt2k_menu::LoggerControl::runPage
//   (F_00136838), RacePS2Base::virtual_8, the constructor.
void RaceSmoke::reset()
{
    int i;

    activeHead = -1;
    freeHead = 0;
    for (i = 0; i < SMOKE_PARTICLES - 1; i++)
        particles[i].next = i + 1;
    particles[SMOKE_PARTICLES - 1].next = -1;
    for (i = 0; i < SMOKE_TYPE_COUNTERS; i++)
        emitCount[i] = 0;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* The original called vu0_rand_seed with the seed only (a one-argument
     * prototype): $a0, which vu0_rand_seed falls through to store as the
     * LCG seed (G001), is this + 4, left over from the counter loop. */
    ((void (*)(f32))vu0_rand_seed)(0.1f);
#else
    vu0_rand_seed((u32)&activeHead, 0.1f);
#endif
}

// FUNC 0x001A46C0 RaceSmoke::setTexture
//   Caller: RacePS2Base::virtual_8.
void RaceSmoke::setTexture(Tex1Image *tex)
{
    texture = tex;
}

// FUNC 0x001A46C8 RaceSmoke::emit
//   One particle of the smoke type g_003306D0[src->kind]: nothing if the
//   type's density is < 0.01 or src->rate <= 0.001.  The type's counter
//   counts up and wraps after `period' (it is not otherwise used here).
//   The particle is taken from the free list (pushed on the active list),
//   or, with none free, the active one with the smallest alpha *
//   alphaScale is reused in place (findWeakest).  Its variant (0..7) is
//   random (vu0_rand); alpha = rate * g_00330730[variant], size = type size
//   * alpha, the colour = the type's rgb x (intensity ramped from
//   smokeMinIntensity to 1) with alpha 0 (pack_rgba_floats_to_u32), the
//   rise, angle, alphaScale ramp, fade and growth rates are random around
//   the type's values; the ground is the plane's y under pos.  Caller:
//   RaceCourse::emitSmoke (F_0018FDE0).
void RaceSmoke::emit(const SmokeSource *src)
{
    int type = g_003306D0[src->kind];
    int index;
    SmokeParticle *p;
    f32 rate;
    int variant;
    f32 intensity;
    f32 fade;

    if (env->smoke[type].density < 0.01f)
        return;
    if (env->smoke[type].period < ++emitCount[type])
        emitCount[type] = 0;
    rate = src->rate;
    if (rate <= 0.001f)
        return;

    if (freeHead < 0) {
        index = findWeakest();
    } else {
        index = freeHead;
        freeHead = particles[index].next;
        particles[index].next = activeHead;
        activeHead = index;
    }
    p = &particles[index];

    variant = (int)(vu0_rand() * 8.0f);
    if (!(variant < 8))
        variant = 7;
    p->flags = variant;
    rate *= g_00330730[variant];
    if (src->flags & 1)
        p->flags = variant | 0x20;
    p->type = type;

    intensity = src->intensity * (1.0f - env->smokeMinIntensity) + env->smokeMinIntensity;
    p->color = pack_rgba_floats_to_u32(env->smoke[type].r * intensity,
                                       env->smoke[type].g * intensity,
                                       env->smoke[type].b * intensity, 0.0f, 128.0f);
    p->alpha = rate;
    p->size = env->smoke[type].size * rate;
    p->vy = env->smoke[type].rise * ((vu0_rand() - 0.2f) * 0.3f + 1.0f);
    p->angle = vu0_rand() * 360.0f;
    p->alphaScale = rate + (1.0f - rate) * env->smoke[type].alphaScale;
    fade = vu0_rand() * ((f32)env->smoke[type].period + 0.2f) + 1.0f;
    if (255.0f <= fade)
        fade = 255.0f;
    p->fadeRate = (int)(fade * 256.0f);
    p->unk_1F = (int)(vu0_rand() * 64.0f);
    p->growRate = (int)(((vu0_rand() - 0.5f) * 0.2f + 1.0f) * 128.0f);

    p->pos.x = src->pos[0];
    p->pos.y = src->pos[1];
    p->pos.z = src->pos[2];
    p->plane.x = src->plane[0];
    p->plane.y = src->plane[1];
    p->plane.z = src->plane[2];
    p->plane.w = src->plane[3];
    p->ground = -(p->plane.x * p->pos.x + p->plane.z * p->pos.z + p->plane.w) / p->plane.y;
}

// FUNC 0x001A4A40 RaceSmoke::findWeakest
//   The active particle with the smallest alpha * alphaScale (below 1e7;
//   0 if none).
int RaceSmoke::findWeakest()
{
    int i = activeHead;
    f32 weakest = 1e7f;
    int found = 0;

    while (i >= 0) {
        SmokeParticle *p = &particles[i];
        int next = p->next;
        f32 strength = p->alpha * p->alphaScale;
        if (strength < weakest) {
            found = i;
            weakest = strength;
        }
        i = next;
    }
    return found;
}

// FUNC 0x001A4A98 RaceSmoke::update
//   One step of dt for every active particle: the angle turns by the
//   type's spin (sign: flags bit 5) x g_00330750[variant] / max(size, 1),
//   wrapped into [0, 360]; the size grows, alpha fades (the type's rates x
//   the particle's growRate / 128, fadeRate / 256), vy falls by gravity x
//   9.81 and moves y.  A particle with alpha <= 0, or whose top (y + size
//   x 1.2071068, (1 + sqrt 2) / 2) is under its ground, goes back to the
//   free list.  Caller: RacePS2Base::virtual_28.
void RaceSmoke::update(f32 dt)
{
    int prev = -1;
    int i = activeHead;
    SmokeParticle *prefetch;

    /* (96%: retail does not reuse the next field loaded at the top for the
     * unlinking: one lhu per branch; here GCSE keeps it in a register) */
    while (i >= 0) {
        SmokeParticle *p = &particles[i];
        int next = p->next;
        f32 spin, size;
        int dead;

        if (next >= 0) {
            prefetch = &particles[next];
            U058_PREF(prefetch);
        }
        spin = env->smoke[p->type].spin;
        if (!(p->flags & 0x20))
            spin = -spin;
        size = p->size;
        if (size < 1.0f)
            size = 1.0f;
        spin *= g_00330750[p->flags & 7] / size;
        p->angle += spin * dt;
        p->size += env->smoke[p->type].grow * dt * ((f32)p->growRate * (1.0f / 128));
        p->alpha -= env->smoke[p->type].fade * dt * ((f32)p->fadeRate * (1.0f / 256));
        p->vy -= dt * 9.81f * env->smoke[p->type].gravity;
        p->pos.y += p->vy * dt;
        if (p->angle > 360.0f)
            p->angle -= 360.0f;
        if (p->angle < 0.0f)
            p->angle += 360.0f;
        dead = p->alpha <= 0.0f;
        if (!dead && p->pos.y + p->size * 1.2071068f + 0.0f < p->ground)
            dead = 1;
        if (dead) {
            if (prev >= 0)
                particles[prev].next = p->next;
            else
                activeHead = p->next;
            p->next = freeHead;
            freeHead = i;
        } else {
            prev = i;
        }
        i = next;
    }
}

// FUNC 0x001A4CC8 RaceSmoke::render
//   The course's divisors (CourseEnv::smokeDivX / Y) out of 1..64 count as
//   1.  Both 1: draw(mode).  Else into two offscreen buffers of (depth's
//   width rounded up to 64) / divX x depth's height / divY
//   (GSBuffer::GSBuffer): a colour buffer and a PSMZ24 depth buffer that
//   depth is stretched into (blit); the GS state saved (pgl_push_attrib),
//   the colour buffer drawn into and cleared to 0 (gs_clear_buffers),
//   draw(1) (no ground clipping), the state restored (pgl_pop_attrib); then
//   the colour buffer as the texture is added over (0, 0) .. (depth's width,
//   displayHeight) (gs_draw_textured_sprite: no depth test, no fog, blend
//   Cs x 1 + Cd, a white colour, no depth writes).  Both buffers are
//   destroyed (~GSBuffer, egcs flags 2).  Callers: RacePS2Base::drawView
//   / 47 (T131).
void RaceSmoke::render(GSBuffer *depth, int mode)
{
    int divX = env->smokeDivX;
    int divY = env->smokeDivY;
    int w, h;

    if (divY > SMOKE_DIV_MAX)
        divY = 1;
    if (divX > SMOKE_DIV_MAX)
        divX = 1;
    if (divX <= 0)
        divX = 1;
    if (divY <= 0)
        divY = 1;
    if (divX == 1 && divY == divX) {
        draw(mode);
        return;
    }
    w = ((depth->width + 0x3F) & ~0x3F) / divX;
    h = depth->height / divY;
    {
        GSBuffer colour(SCE_GS_PSMCT32, w, h);  /* sp+0x00 */
        GSBuffer zbuf(SCE_GS_PSMZ24, w, h);     /* sp+0x40 */
        int width;

        zbuf.blit(depth, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
        pgl_push_attrib(SMOKE_RENDER_ATTRIBS);
        colour.setDrawBuffer();
        zbuf.setZBuffer();
        gs_set_clear_color(0);
        gs_clear_buffers(PGL_CLEAR_COLOR);
        draw(1);
        pgl_pop_attrib();
        colour.setTexture();
        gfx_disable(PGL_CAP_DEPTH_TEST);
        gfx_disable(PGL_CAP_FOG);
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(SMOKE_BLEND_COMPOSE, 1.0f);
        gfx_depth_mask(0);
        pgl_color4f(1.0f, 1.0f, 1.0f, 1.0f);
        width = depth->width;
        /* (sic: the u range is cut by one texel when divY >= 2, the v range
         * when divX >= 2) */
        gs_draw_textured_sprite(0, 0, width, depth->displayHeight(), 0, 0,
                                zbuf.width - (divY >= 2), zbuf.height - (divX >= 2));
        gfx_depth_mask(1);
    }
}

/* The blend that needs the particles sorted (alpha, not additive).  An
 * inline returning the test: retail branches on its store-flag (xori;
 * beqz), not on a compare with a register holding 0x48. */
static inline int u058_smoke_sorted(const CourseEnv *e)
{
    return e->smokeBlend != SMOKE_BLEND_ADD;
}

/* A GS packet quadword written as two doublewords. */
union U058Qword {
    u128 q;
    u64  d[2];
};

// FUNC 0x001A4F28 RaceSmoke::draw
//   Nothing without a texture.  The fog constants (pgl_get_fog_constants) and the
//   projection matrix (pgl_matrix_mode, pgl_get_matrix) give the depth
//   range zMin = m32 / (m22 - 1) < -z < zMax = m32 / (m22 + 1); a particle
//   is visible inside it with a screen radius (size x 1.2071068 / -z x m00)
//   under 2.4142137 (1 + sqrt 2) and its box on the screen ([-1, 1]).  The
//   visible ones are sorted (sortByDepth, unless the blend is additive),
//   the GS state set (gfx_disable / gfx_enable / gfx_depth_mask /
//   gfx_blend_func; the texture made resident, VramAllocator::loadTex1, and
//   selected, gfx_set_texture_regs; gfx_tex_param: modulated with its
//   alpha, clamped; gs_flush_dirty_regs), the model view pushed
//   (pgl_push_matrix), the inverse transpose of the view matrix
//   (vu0_invert_rigid, vu0_transpose) kept for the ground planes
//   (planeMatrix, a 16-byte aligned stack buffer), the VU0 matrix reset
//   (vu0_load_identity_regs), and one GIF packet written (gfx_open_direct):
//   an A+D PRIM (the triangle strip, gs_prim_attr_with) then the quads.
//   Mode 0: size > 3 plain quads (drawQuad), the others clipped at their
//   ground (drawClippedQuad); mode 1: plain quads only; other modes: no
//   quads.  Then pgl_pop_matrix.  Callers: RacePS2Base::drawMirror, render.
void RaceSmoke::draw(int mode)
{
    Matrix44 proj;                          /* sp+0x0000 */
    SmokeVis vis[SMOKE_PARTICLES];          /* sp+0x0040 */
    /* sp+0x1040 the prefetch pointer: in memory at its declaration (retail's
     * slot is below planeBuf's); volatile is the form that places it so */
    SmokeParticle * volatile prefetch;
    u8 planeBuf[0x40 + 0xF];                /* sp+0x1050 */
    SmokeVis *out;
    f32 zMax, zMin;
    int i;
    int visible;
    u64 *ad;
    u64 *pk;
    u32 prim;

    if (texture == NULL)
        return;
    pgl_get_fog_constants(fog);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_get_matrix(proj.m[0]);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    zMin = proj.m[3][2] / (proj.m[2][2] - 1.0f);
    zMax = proj.m[3][2] / (proj.m[2][2] + 1.0f);
    out = vis;
    i = activeHead;
    while (i >= 0) {
        f32 z, dist;

        if (particles[i].next >= 0) {
            prefetch = &particles[particles[i].next];
            U058_PREF(prefetch);
        }
        u058_xform_point(&out->pos, &particles[i].pos);
        z = out->pos.z;
        dist = -z;
        if (zMin < dist && dist < zMax) {
            f32 invZ = -1.0f / z;
            f32 radius = particles[i].size * 1.2071068f * invZ;
            f32 rx = proj.m[0][0] * radius;
            f32 ry = proj.m[1][1] * radius;
            f32 sx = (out->pos.x * proj.m[0][0] + z * proj.m[2][0]) * invZ;
            f32 sy = (out->pos.y * proj.m[1][1] + z * proj.m[2][1]) * invZ;
            if (rx < 2.4142137f
                && sx - rx < 1.0f && sx + rx > -1.0f
                && sy - ry < 1.0f && sy + ry > -1.0f) {
                out->index = i;
                out++;
            }
        }
        i = particles[i].next;
    }
    visible = (int)(out - vis);
    if (u058_smoke_sorted(env))
        sortByDepth(vis, 0, visible - 1);

    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_enable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_TEXTURE);
    gfx_depth_mask(0);
    gfx_disable(PGL_CAP_FOG);
    if (env->smokeBlend == SMOKE_BLEND_ADD)
        gfx_blend_func(SMOKE_BLEND_ADD, 1.0f);
    else
        gfx_blend_func(SMOKE_BLEND_ALPHA, 1.0f);
    g_002F4250.loadTex1(texture);
    gfx_set_texture_regs(texture->tex);
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_CLAMP);
    gs_flush_dirty_regs();
    pgl_push_matrix();
    planeMatrix = (Matrix44 *)((u32)(planeBuf + 0xF) & 0xFFFFFFF0);
    vu0_invert_rigid();
    vu0_transpose();
    pgl_get_matrix(planeMatrix->m[0]);
    vu0_load_identity_regs();
    gs_flush_dirty_regs();

    pk = (u64 *)gfx_open_direct(0);
    pkStart = pk;
    pkCur = (u32 *)(pk + 2);
    /* through a union (alias set 0): retail reloads pkCur after these */
    ((U058Qword *)pk)->d[0] = GIF_TAG_NREG(1) | GIF_TAG_EOP | 1;    /* NLOOP 1, PACKED: one A+D */
    ((U058Qword *)pk)->d[1] = GIF_REG_AD;
    ad = (u64 *)pkCur;
    pkCur = (u32 *)(ad + 2);
    prim = gs_prim_attr_with(GS_PRIM_TRISTRIP);
    ad[1] = GS_REG_PRIM;
    ad[0] = (u64)prim;

    if (visible != 0) {
        prefetch = &particles[vis[0].index];
        U058_PREF(prefetch);
    }
    switch (mode) {
    case 0:
        for (i = 0; i < visible; i++) {
            if (i + 1 < visible) {
                prefetch = &particles[vis[i + 1].index];
                U058_PREF(prefetch);
            }
            if (particles[vis[i].index].size > 3.0f)
                drawQuad(&vis[i]);
            else
                drawClippedQuad(&vis[i]);
        }
        break;
    case 1:
        for (i = 0; i < visible; i++) {
            if (i + 1 < visible) {
                prefetch = &particles[vis[i + 1].index];
                U058_PREF(prefetch);
            }
            drawQuad(&vis[i]);
        }
        break;
    }
    gfx_open_direct((int)((u32)((u8 *)pkCur - (u8 *)pkStart) >> 4));
    pgl_pop_matrix();
    gfx_disable(PGL_CAP_BLEND);
    gfx_depth_mask(1);
}

// FUNC 0x001A5438 RaceSmoke::emitStrip
//   The screen registers into vf20..vf23 (vu0_load_screen_regs), then a GIF
//   tag (REGLIST, NLOOP 1, EOP, NREG 2n + 2 (mod 16), the registers
//   SMOKE_STRIP_REGS), RGBAQ = rgba0 / Q 1.0, then per vertex its ST (the
//   uv words copied) and its XYZF (u058_project_vertex: x, y 12.4, z >> 4
//   (24 bits), fog = w >> 4); rgba1 (Q 1.0) before the third vertex.  The
//   packet pointer moves.
//   egcs: the original's VU0 macro-mode asm (the vertex in through the GPRs:
//   ldl / ldr + pcpyld + qmtc2.ni; out: qmfc2.ni, pcpyud / ppach), split
//   where retail schedules the C stores between them.  (81%: retail keeps
//   the packet cursor in a stack word, reloaded after each join but not
//   volatile: its load fills the delay slot of the i == 2 test; a volatile
//   cursor is the closest form found (an addressed local, a one-element
//   array, a struct or a pointer to it are kept in a register).  The
//   prefetch of sp+0x40, a saved register slot, is the original's.)
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
void RaceSmoke::emitStrip(const Vec3 *pos, const f32 (*uv)[2], u32 rgba0, u32 rgba1, int n)
{
    u64 *tag;
    u32 * volatile word;
    int i;

    vu0_load_screen_regs();
    tag = (u64 *)pkCur;
    tag[1] = SMOKE_STRIP_REGS;
    tag[0] = GIF_TAG_NREG(n * 2 + 2) | GIF_TAG_REGLIST | GIF_TAG_EOP | 1;
    ((u32 *)tag)[4] = rgba0;
    ((u32 *)tag)[5] = F32_ONE_BITS;         /* Q = 1.0f */
    word = (u32 *)tag + 6;
    for (i = 0; i < n; i++, pos++, uv++) {
        u128 lo, hi, xyzw;
        u64 zw, xy;
        u32 *w;

        __asm__ volatile("pref         0, 0x40($sp)");
        __asm__ volatile(
            "ldl          %0, 7+%2\n\t"
            "ldr          %0, %2\n\t"
            "ldl          %1, 15+%2\n\t"
            "ldr          %1, 8+%2\n\t"
            "pcpyld       %1, %1, %0\n\t"
            "qmtc2.ni     %1, $vf1\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
            "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
            "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
            "vdiv         " VU0_Q ", $vf0w, $vf1w"
            : "=&r"(lo), "=&r"(hi) : "m"(*pos));
        if (i == 2) {
            word[0] = rgba1;
            word[1] = F32_ONE_BITS;         /* Q = 1.0f */
            word += 2;
        }
        __asm__ volatile("vmini.xyzw   $vf1, $vf1, $vf21");
        w = word;
        w[0] = ((const u32 *)*uv)[0];       /* ST */
        w[1] = ((const u32 *)*uv)[1];
        __asm__ volatile(
            "vwaitq\n\t"
            "vmulq.xyzw   $vf1, $vf1, " VU0_Q "\n\t"
            "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
            "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
            "vftoi4.xyzw  $vf1, $vf1\n\t"
            "qmfc2.ni     %0, $vf1"
            : "=r"(xyzw));
        /* (z, w) words; the low halfwords of x, y, z, w */
        __asm__("pcpyud       %0, %1, %0" : "=r"(zw) : "r"(xyzw));
        __asm__("ppach        %0, $0, %1" : "=r"(xy) : "r"(xyzw));
        *(u64 *)(w + 2) = ((zw >> 36) << 56) | (((zw >> 4) & 0xFFFFFF) << 32)
                          | (xy & 0xFFFFFFFF);  /* XYZF */
        word = w + 4;
    }
    pkCur = word;
}
#else
void RaceSmoke::emitStrip(const Vec3 *pos, const f32 (*uv)[2], u32 rgba0, u32 rgba1, int n)
{
    u64 *tag;
    u32 *word;
    const void *stackWord;                  /* (an unset stack word: see below) */
    int i;

    vu0_load_screen_regs();
    tag = (u64 *)pkCur;
    tag[1] = SMOKE_STRIP_REGS;
    tag[0] = GIF_TAG_NREG(n * 2 + 2) | GIF_TAG_REGLIST | GIF_TAG_EOP | 1;
    ((u32 *)tag)[4] = rgba0;
    ((u32 *)tag)[5] = F32_ONE_BITS;         /* Q = 1.0f */
    word = (u32 *)tag + 6;
    for (i = 0; i < n; i++, pos++, uv++) {
        s32 v[4];
        u64 x, y, z, f;

        /* the original prefetches the stack word sp+0x40 here (a saved
         * register slot): no effect on the data */
        U058_PREF(stackWord);
        u058_project_vertex(v, pos);
        if (i == 2) {
            word[0] = rgba1;
            word[1] = F32_ONE_BITS;         /* Q = 1.0f */
            word += 2;
        }
        memcpy(word, *uv, 8);               /* ST */
        x = (u32)v[0] & 0xFFFF;
        y = (u32)v[1] & 0xFFFF;
        z = ((u32)v[2] >> 4) & 0xFFFFFF;
        f = ((u32)v[3] >> 4) & 0xFF;
        *(u64 *)(word + 2) = (f << 56) | (z << 32) | (y << 16) | x;     /* XYZF */
        word += 4;
    }
    pkCur = word;
}
#endif

// FUNC 0x001A55E0 RaceSmoke::drawQuad
//   The quad of half-diagonals a = (s + c) x size, b = (c - s) x size
//   (pgl_sincos_deg of the angle) around the view position, in strip order,
//   the colour with alpha (color) at all four corners, the uv set index & 3
//   (emitStrip).
void RaceSmoke::drawQuad(const SmokeVis *v)
{
    int index = v->index;
    SmokeParticle *p = &particles[index];
    u32 rgba = color(v);
    f32 s, c;
    f32 size, a, b, x, y, z;
    Vec3 q[4];                              /* sp+0x00 */

    pgl_sincos_deg(p->angle, &s, &c);
    size = p->size;
    x = v->pos.x;
    y = v->pos.y;
    z = v->pos.z;
    b = (c - s) * size;
    a = (s + c) * size;
    q[0].x = x + b; q[0].y = y + a; q[0].z = z;
    q[1].x = x - a; q[1].y = y + b; q[1].z = z;
    q[2].x = x + a; q[2].y = y - b; q[2].z = z;
    q[3].x = x - b; q[3].y = y - a; q[3].z = z;
    emitStrip(q, g_00330770[index & 3], rgba, rgba, 4);
}

// FUNC 0x001A56F0 RaceSmoke::drawLoopQuad
//   A quad in loop order (drawClippedQuad's): corners 2 and 3 swapped into
//   strip order, then the plain strip (emitStrip) with the uv set uvSet.
void RaceSmoke::drawLoopQuad(Vec3 *q, u32 rgba, int uvSet)
{
    /* field by field (retail: six lwc1, six swc1; a Vec3 copy gives
     * ldl / ldr) */
    f32 x = q[2].x;
    f32 y = q[2].y;
    f32 z = q[2].z;

    q[2].x = q[3].x;
    q[2].y = q[3].y;
    q[2].z = q[3].z;
    q[3].x = x;
    q[3].y = y;
    q[3].z = z;
    emitStrip(q, g_00330770[uvSet], rgba, rgba, 4);
}

// FUNC 0x001A5760 RaceSmoke::drawClippedQuad
//   The quad of drawQuad in loop order (pgl_sincos_deg); its ground plane to
//   view space (planeMatrix, vf20..vf23).  The plane behind the camera (w <
//   0), or all corners on one side: drawLoopQuad.  Else the corners under
//   the plane are projected onto it (projectToPlane), the two crossing
//   points of the edges found (edgeCross; `down' where the quad goes under,
//   `up' where it comes back), the projected corners next to them pulled to
//   within 2 x size x the edge fraction of the crossing (clampLength); the
//   part under the plane (crossings + projected corners) is drawn fading to
//   the particle's alpha-0 colour from its third vertex, the part above
//   (crossings + corners) with the colour (buildPolygon, toStrip,
//   emitStrip).  The uv set is g_003307F0's.
void RaceSmoke::drawClippedQuad(const SmokeVis *v)
{
    Vec3 q[4];                              /* sp+0x000 */
    Vec4 plane;                             /* sp+0x030 */
    Vec3 onPlane[4];                        /* sp+0x040 the corners projected onto the plane */
    SmokeEdge down;                         /* sp+0x070 */
    SmokeEdge up;                           /* sp+0x090 */
    Vec3 outPos[5];                         /* sp+0x0B0 */
    f32 outUV[6][2];                        /* sp+0x0F0 */
    f32 s, c;                               /* sp+0x120 */
    int index = v->index;
    SmokeParticle *p = &particles[index];   /* sp+0x128 */
    u32 rgba = color(v);                    /* sp+0x12C */
    f32 maxLen;
    f32 size, a, b, x, y, z;
    int uvSet, j, bit, under, pair;
    int iDown, iUp;                         /* sp+0x130, sp+0x134 */
    int n;

    maxLen = p->size + p->size;
    pgl_sincos_deg(p->angle, &s, &c);
    s *= p->size;
    size = p->size;
    c *= size;
    x = v->pos.x;
    y = v->pos.y;
    z = v->pos.z;
    b = c - s;
    a = c + s;
    q[0].x = x + b; q[0].y = y + a; q[0].z = z;
    q[1].x = x - a; q[1].y = y + b; q[1].z = z;
    q[2].x = x - b; q[2].y = y - a; q[2].z = z;
    q[3].x = x + a; q[3].y = y - b; q[3].z = z;
    u058_xform_plane(&plane, &p->plane, &planeMatrix);
    uvSet = index & 3;
    clipUV = g_003307F0[uvSet];
    clipPlane = &plane;
    clipQuad = q;
    if (plane.w < 0.0f) {
        drawLoopQuad(q, rgba, uvSet);
        return;
    }
    under = 0;
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* the corners as a float array, j their first component (retail: the
     * index steps by 3 and is scaled by 4 at each use) */
    for (j = 0, bit = 1; j < 12; j += 3, bit <<= 1) {
        f32 *corner = &q[0].x + j;

        if (corner[0] * plane.x + corner[1] * plane.y + corner[2] * plane.z + plane.w < 0.0f) {
            under |= bit;
            projectToPlane((Vec3 *)(&onPlane[0].x + j), (Vec3 *)corner);
        }
    }
#else
    for (j = 0, bit = 1; j < 4; j++, bit <<= 1) {
        if (q[j].x * plane.x + q[j].y * plane.y + q[j].z * plane.z + plane.w < 0.0f) {
            under |= bit;
            projectToPlane(&onPlane[j], &q[j]);
        }
    }
#endif
    if (under == 0 || under == 0xF) {
        drawLoopQuad(q, rgba, uvSet);
        return;
    }
    iDown = -1;
    iUp = -1;
    under <<= 4;
    for (j = 0, pair = 0x33, bit = 0x10; j < 4; j++, pair <<= 1, bit <<= 1) {
        int t = under & pair;               /* corners j, j + 1 (mod 4) under the plane */
        f32 u;

        if (t == 0 || t == (pair & 0xF0))
            continue;
        if (t == bit) {
            /* j under, j + 1 above */
            iUp = j;
            u = edgeCross(&up, j);
            clampLength(&up, &onPlane[j], maxLen * u);
        } else {
            /* j above, j + 1 under.  (sic: for j = 3 the original passes
             * &onPlane[4], which is `down' itself on its stack: no clamp.
             * GCC 14 passes &down.pos explicitly.) */
            iDown = j;
            u = edgeCross(&down, j);
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
            clampLength(&down, &onPlane[j + 1], maxLen * (1.0f - u));
#else
            clampLength(&down, (j + 1 < 4) ? &onPlane[j + 1] : &down.pos, maxLen * (1.0f - u));
#endif
        }
    }
    n = buildPolygon(iDown, iUp, outPos, outUV, &down, &up, onPlane);
    toStrip(n, outPos, outUV);
    emitStrip(outPos, outUV, rgba, p->color, n);
    n = buildPolygon(iUp, iDown, outPos, outUV, &up, &down, q);
    toStrip(n, outPos, outUV);
    emitStrip(outPos, outUV, rgba, rgba, n);
}

/* min.s: egcs's <? operator (T173 has no -ffast-math, under which a
 * ternary would give it); GCC 14 has none. */
#ifdef BOOT_OLD_GCC
#define MINF(a, b) ((a) <? (b))
#else
#define MINF(a, b) ((a) < (b) ? (a) : (b))
#endif

// FUNC 0x001A5B50 RaceSmoke::color
//   The particle's colour with alpha = alpha x alphaScale x the type's
//   density x (with CourseEnv::flags ENV_SMOKE_FOG: min(fogStart / dist, 1)
//   x fogScale + fogOffset, the fog factor of the VU1 code; else 255),
//   truncated, at most 0x80.
u32 RaceSmoke::color(const SmokeVis *v)
{
    SmokeParticle *p = &particles[v->index];
    CourseEnv *e = env;
    f32 dist = -v->pos.z;
    f32 alpha = p->alpha * p->alphaScale * e->smoke[p->type].density;
    f32 ratio = fog[2] / dist;
    int byte;

    /* (77%: retail computes the fog factor into another register than the
     * 255 of the other arm, so jump2 does not merge the arms' multiplies;
     * here it merges them, the fog arm ending in the shared mul.s) */
    if (e->flags & ENV_SMOKE_FOG)
        byte = (int)(alpha * (MINF(ratio, 1.0f) * fog[1] + fog[0]));
    else
        byte = (int)(alpha * 255.0f);
    if (!(byte < 0x81))
        byte = 0x80;
    return p->color | ((u32)byte << 24);
}

// FUNC 0x001A5C08 RaceSmoke::projectToPlane
//   in moved along the view ray onto clipPlane: in x -w / (in . n).
void RaceSmoke::projectToPlane(Vec3 *out, const Vec3 *in)
{
    Vec4 *plane = clipPlane;
    f32 d = in->x * plane->x + in->y * plane->y + in->z * plane->z;
    f32 scale = -plane->w / d;

    out->x = scale * in->x;
    out->y = scale * in->y;
    out->z = scale * in->z;
}

// FUNC 0x001A5C70 RaceSmoke::edgeCross
//   Where the edge clipQuad[i] -> clipQuad[(i + 1) & 3] (z constant) meets
//   clipPlane: the point, its uv interpolated in clipUV; returns the edge
//   fraction.
f32 RaceSmoke::edgeCross(SmokeEdge *out, int i)
{
    int next = (i + 1) & 3;
    Vec3 *a = &clipQuad[i];
    Vec3 *b = &clipQuad[next];
    Vec4 *plane = clipPlane;
    const f32 *uv0 = clipUV[i];
    const f32 *uv1 = clipUV[next];
    f32 dx = b->x - a->x;
    f32 dy = b->y - a->y;
    f32 du = uv1[0] - uv0[0];
    f32 dv = uv1[1] - uv0[1];
    f32 frac = -(a->x * plane->x + a->y * plane->y + a->z * plane->z + plane->w)
               / (dx * plane->x + dy * plane->y);

    out->pos.x = a->x + frac * dx;
    out->pos.y = a->y + frac * dy;
    out->pos.z = a->z;
    out->uv[0] = uv0[0] + frac * du;
    out->uv[1] = uv0[1] + frac * dv;
    return frac;
}

// FUNC 0x001A5D70 RaceSmoke::clampLength
//   v pulled towards e->pos to a distance of max (rsqrt.s) if it is
//   farther.
void RaceSmoke::clampLength(const SmokeEdge *e, Vec3 *v, f32 max)
{
    f32 dx = v->x - e->pos.x;
    f32 dy = v->y - e->pos.y;
    f32 dz = v->z - e->pos.z;
    f32 d2 = dx * dx + dy * dy + dz * dz;

    if (max * max < d2) {
        f32 scale = u058_rsqrt(max, d2);    /* max / sqrt(d2) */
        v->x = e->pos.x + dx * scale;
        v->y = e->pos.y + dy * scale;
        v->z = e->pos.z + dz * scale;
    }
}

// FUNC 0x001A5DF0 RaceSmoke::buildPolygon
//   A polygon in loop order: first, second (the crossing points), then the
//   corners verts[(from + 1) & 3 ..] up to (to + 1) & 3 (excluded) with
//   their clipUV.  Returns its vertex count.
int RaceSmoke::buildPolygon(int from, int to, Vec3 *pos, f32 (*uv)[2],
                            const SmokeEdge *second, const SmokeEdge *first, const Vec3 *verts)
{
    /* the positions and uvs written through float cursors (retail: one
     * lwc1 / swc1 / addiu per component) */
    f32 *p = &pos->x;
    f32 *t = uv[0];
    int i = (from + 1) & 3;
    int end = (to + 1) & 3;
    int n = 2;

    *p++ = first->pos.x;
    *p++ = first->pos.y;
    *p++ = first->pos.z;
    *t++ = first->uv[0];
    *t++ = first->uv[1];
    *p++ = second->pos.x;
    *p++ = second->pos.y;
    *p++ = second->pos.z;
    *t++ = second->uv[0];
    *t++ = second->uv[1];
    while (i != end) {
        *p++ = verts[i].x;
        *p++ = verts[i].y;
        *p++ = verts[i].z;
        *t++ = clipUV[i][0];
        *t++ = clipUV[i][1];
        n++;
        i = (i + 1) & 3;
    }
    return n;
}

// FUNC 0x001A5EF0 RaceSmoke::toStrip
//   n >= 4: the loop order 0, 1, 2, .., n - 1 into strip order 0, 1,
//   n - 1, 2, .., n - 2 (the last vertex moved to 2, positions and uvs).
void RaceSmoke::toStrip(int n, Vec3 *pos, f32 (*uv)[2])
{
    /* float cursors moving down (retail: one lwc1 / swc1 per component);
     * (68.9%: retail keeps both cursors of the uv loop and loads the last
     * vertex relative to the destination cursor) */
    f32 *src, *dst;
    f32 x, y, z, u, v;
    int i;

    if (n < 4)
        return;
    dst = &pos[n].x;
    src = dst - 3;
    x = src[0];
    y = src[1];
    z = src[2];
    for (i = 0; i < n * 3 - 9; i++)
        *--dst = *--src;
    pos[2].x = x;
    pos[2].y = y;
    pos[2].z = z;
    dst = uv[n];
    src = dst - 2;
    u = src[0];
    v = src[1];
    for (i = 0; i < n * 2 - 6; i++)
        *--dst = *--src;
    uv[2][0] = u;
    uv[2][1] = v;
}

// FUNC 0x001A5FB8 RaceSmoke::sortByDepth
//   Quicksort of v[lo..hi] by the view z, ascending (the farthest first):
//   the pivot is v[(lo + hi) >> 1].z; after the partition (i, j) the
//   ranges [lo, i - 1] and [j + 1, hi] are sorted.  hi < 0: nothing.
void RaceSmoke::sortByDepth(SmokeVis *v, int lo, int hi)
{
    f32 pivot;
    int i, j;

    if (hi < 0)
        return;
    pivot = v[(lo + hi) >> 1].pos.z;
    i = lo;
    j = hi;
    /* indices only: loop.c turns v[i] / v[j] into the cursors of the swap
     * and of the forward scan (retail keeps the backward scan's address
     * computed from j) */
    for (;;) {
        SmokeVis swap;

        while (v[i].pos.z < pivot)
            i++;
        while (pivot < v[j].pos.z)
            j--;
        if (!(i < j))
            break;
        swap = v[i];
        v[i] = v[j];
        v[j] = swap;
        i++;
        j--;
    }
    if (lo < i - 1)
        sortByDepth(v, lo, i - 1);
    if (j + 1 < hi)
        sortByDepth(v, j + 1, hi);
}
