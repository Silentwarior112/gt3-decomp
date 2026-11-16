/* T160_race_meters.cpp - the meters of the race display (0x0019C368..
 * 0x0019D278): RaceMeterBase's constructor, RaceRoundMeterBase and
 * RaceRoundMeter (round dials), RaceBarMeter, their typeinfo functions and
 * T160's copies of RaceMeterBase's inline methods.
 *
 * A round dial is drawn in two parts: drawMeter the panel image (in the
 * panel colour), render_2nd the scale and the needle, in the panel's
 * coordinates (translated to the dial centre, scaled by the radius).  The
 * scale marks are prebuilt into a TStripBuilder packet (buildScale) and,
 * like the needle, drawn RACE_METER_PASSES (8) times at 1/8 alpha with the
 * sub-pixel offsets of g_0032F568 (an anti-aliasing pass).  See
 * game/race/display/T160_race_meters.h.
 *
 * Data (u055's data pass): retail's .rodata 0x0032F240..0x0032F5A8 is
 * "bad_alloc" (<new>), the needle strip g_0032F250 (external linkage:
 * written where it is defined), the vtables of RaceBarMeter, RaceRoundMeter,
 * RaceRoundMeterBase and RaceMeterBase, the type names RaceMiniMap ..
 * RaceDisplayBase (the race display header's classes in reverse definition
 * order, T160's own among them: the display header first, as the old unit
 * header had it), then the sub-pixel offsets g_0032F568 (a static const
 * table: deferred to the end).  Its .sdata (0x003520E8) starts with the ""
 * of T157's inline RaceMessageDisplay::clear (-G8), then the two FLT_MAX
 * constants.  Neither "bad_alloc" nor the "" is referenced: anchors.  The
 * frame info's function order needs RaceRoundMeterBase::draw_scale inline
 * here (GAME_T160_INLINE_DRAW_SCALE, T160_race_meters.h). */
// DATA_START 0x0032F240
// DATA_START 0x003520E8
#define GAME_T160_INLINE_DRAW_SCALE         /* RaceRoundMeterBase::draw_scale, inline in T160 only */
#include "types.h"
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (egcs's <new> and cxxrt.h conflict) */
#endif
/* The race display's header first (it includes T160's after the parts):
 * retail's T160 type names are the display header's in its order
 * (RaceDisplayBase, ValueDisplayBase, UnitArena<64>, RaceCameraBase,
 * RaceMeterBase and T160's own, the tachometers, the speedometer, MiniMap:
 * reversed; the T154 merge). */
#include "game/race/display/T154_race_display.h"         /* RaceDisplayVertex; T160 RaceMeterBase */
#include "game/race/display/T158_race_display_draw.h"    /* race_draw_image_rect, race_align_pos */
#include "game/race/display/T160_race_meters.h"          /* own: RaceMeterBase (its constructor), RaceRoundMeterBase,
                                               RaceRoundMeter, RaceBarMeter, T160_fill_words, the __tf / __ti */
#include "game/race/display/T156_race_display_object.h"  /* RaceDisplayObjectBase, __tf21RaceDisplayObjectBase */
#include "game/car/model/T448_strip_builder.h"        /* TStripBuilder (the dial's scale packet) */
#include "engine/pgl/G001_flare.h"                /* the pgl library, Tex1Image, GsTexRegs, VifModel, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* sincosf */
#include "engine/text/T496_font.h"                 /* Font::flush, the system font g_002F1968 */
#include "engine/io/T506_image_pack.h"           /* image_pack_find */
#include "game/race/display/race_display_draw_internal.h"                           /* RACE_COLOR_WHITE, RACE_METER_PASSES */

#define DEG2RAD 0.017453292f                /* 0x3C8EFA35, gp-0x7BFC / gp-0x7BF8 */
#define FLT_MAX_F 3.40282347e+38f           /* 0x7F7FFFFF, 0x003520EC / 0x003520F0 */

/* The blend modes of the dials (gfx_blend_func selectors). */
#define BLEND_ALPHA   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)    /* 0x44 (Cs - Cd) As + Cd */
#define BLEND_FIX     PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD)   /* 0x64 (Cs - Cd) FIX + Cd */
#define BLEND_ADD_FIX PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD) /* 0x68 Cs FIX + Cd */
#define PASS_ALPHA    0.125f                /* 1 / RACE_METER_PASSES */

/* The GS PRIM of the scale strips: TRISTRIP (4) | ABE (0x40). */
#define SCALE_PRIM 0x44

/* The needle's colour (ABGR: red 0xC0, alpha 0xFF). */
#define NEEDLE_COLOR 0xFF0000C0

/* x clamped to lo..hi: max.s then min.s, volatile asm statements as every
 * max.s / min.s of the image behaves (none is in a delay slot, the code
 * around them stays in source order; u018, u032, u044).  GCC 14: the C
 * comparisons (the same but for -0.0 against 0.0). */
static inline f32 meter_clampf(f32 x, f32 lo, f32 hi)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 t;

    __asm__ __volatile__("max.s %0, %1, %2" : "=f"(t) : "f"(x), "f"(lo));
    __asm__ __volatile__("min.s %0, %1, %2" : "=f"(x) : "f"(t), "f"(hi));
    return x;
#else
    x = x > lo ? x : lo;
    return x < hi ? x : hi;
#endif
}

/* ======================================================================== */
/* RaceMeterBase                                                            */
/* ======================================================================== */

// FUNC 0x0019C368 RaceMeterBase::RaceMeterBase
//   Callers: the RaceRoundMeterBase / RaceBarMeter constructors,
//   RaceRoundTachometerBase::RaceRoundTachometerBase,
//   RaceBattleTachometer::RaceBattleTachometer.  The base first
//   (RaceDisplayObjectBase::RaceDisplayObjectBase), then the vptr 0x0032E368
//   (T154's vtable); enabled by default.
RaceMeterBase::RaceMeterBase()
{
    /* implicit: RaceDisplayObjectBase (base constructor) */
    scale = 1.0f;
    enabled = 1;
    value = 0.0f;
}

/* ======================================================================== */
/* RaceRoundMeterBase                                                       */
/* ======================================================================== */

// FUNC 0x0019C3B8 RaceRoundMeterBase::RaceRoundMeterBase
//   Callers: RaceRoundMeter::RaceRoundMeter, RaceBoostmeter::RaceBoostmeter.
//   The dial defaults: centre (0.5, 0.5), radius 150 (aspect 1), 65..295
//   degrees, needle -0.1..0.95, panel colour 0x80FFFFFF; no needle drawn
//   yet.  The EH landing pad restores RaceMeterBase's vptr and destroys the
//   base (~RaceDisplayObjectBase).
RaceRoundMeterBase::RaceRoundMeterBase()
{
    /* implicit: RaceMeterBase (base constructor), ~RaceDisplayObjectBase (exception cleanup, inline ~RaceMeterBase) */
    lastAngle = FLT_MAX_F;
    setCenter(0.5f, 0.5f);
    setRadius(150.0f, 1.0f);
    scale = 1.0f;
    setAngles(65.0f, 295.0f);
    setNeedle(-0.1f, 0.95f);
    setPanelColor(RACE_COLOR_WHITE);
}

// FUNC 0x0019C4A0 RaceRoundMeterBase::~RaceRoundMeterBase
//   Index: RaceRoundMeterBase::virtual_0.  Stores RaceMeterBase's vptr
//   (its own store is dead) and calls ~RaceDisplayObjectBase with the
//   flags unchanged (that one deletes).  Callers: RaceRoundMeter (its
//   constructor's EH, its destructor), RaceBoostmeter (the same).
RaceRoundMeterBase::~RaceRoundMeterBase()
{
    /* implicit: ~RaceDisplayObjectBase (base destructor, inline ~RaceMeterBase) */
}

// FUNC 0x0019C4C8 RaceRoundMeterBase::drawMeter
//   Index: RaceRoundMeter::drawMeter; it is in RaceRoundMeterBase's own
//   vtable (0x0032F370 slot 5), so RaceRoundMeterBase's.  Draws the panel
//   image over the object's rectangle in the panel colour (the pending text
//   flushed first).  The buffer is not used.  Also called directly by
//   RaceRoundTachometerBase::drawMeter and RaceBoostmeter::drawMeter.
void RaceRoundMeterBase::drawMeter(GSBuffer &buf)
{
    g_002F1968.flush();
    pgl_color(panelColor);
    race_draw_image_rect(image, (f32)x, (f32)y, (f32)w, (f32)h, alignX, alignY);
}

// FUNC 0x0019C540 RaceRoundMeterBase::render_2nd
//   Index: RaceRoundMeter::virtual_10 (slot 10, RaceMeterBase's
//   render_2nd; in RaceRoundMeterBase's vtable).  When enabled: the
//   identity VU0 matrix, translated to the dial centre (the rectangle's
//   aligned centre plus centre * the panel's scale; the panel image's size
//   read from its CLAMP register and converted from u64: __floatdisf with
//   the unsigned fix-up), scaled by the radius; then draw_scale and
//   draw_needle (virtual calls).  Callers: RaceDisplay::virtual_3,
//   RaceRoundTachometerBase::render_2nd, RaceBoostmeter::render_2nd.
void RaceRoundMeterBase::render_2nd()
{
    f32 scaleX, scaleY, centreX, centreY;

    if (!enabled)
        return;
    g_002F1968.flush();
    pgl_push_matrix();
    vu0_load_identity_regs();
    scaleX = (f32)w / (f32)(((image->tex->clamp >> 14) & 0x3FF) + 1); /* implicit: __floatdisf (u64 to float) */
    scaleY = (f32)h / (f32)(((image->tex->clamp >> 34) & 0x3FF) + 1);
    centreX = (f32)(race_align_pos(x, w, alignX) + w / 2) + centerX * scaleX;
    centreY = (f32)(race_align_pos(y, h, alignY) + h / 2) + centerY * scaleY;
    pgl_translate(centreX, centreY, 0.0f);
    vu0_scale(radiusX * scaleX, radiusY * scaleY, 1.0f);
    draw_scale();
    draw_needle();
    pgl_pop_matrix();
}

/* The needle strip: (1, +-0.01) .. (0, +-0.02), in units of the needle's
 * length from needleInner.  .rodata, external linkage (the extern first:
 * egcs writes it here, before the vtables). */
extern const RaceDisplayVertex g_0032F250[4];   /* 0x0032F250 */
const RaceDisplayVertex g_0032F250[4] = {   /* 0x0032F250 */
    { 1.0f,  0.01f, 0 },
    { 0.0f,  0.02f, 0 },
    { 1.0f, -0.01f, 0 },
    { 0.0f, -0.02f, 0 },
};

/* The 8 sub-pixel offsets of the anti-aliasing passes (relative steps for
 * gfx_offset_xy, summing to 0).  Internal linkage: egcs writes it at the
 * end of .rodata, after the type names (retail 0x0032F568). */
static const f32 g_0032F568[RACE_METER_PASSES][2] = {  /* 0x0032F568 */
    {  0.5f,  0.0f  },
    { -0.5f,  0.5f  },
    {  0.5f,  0.0f  },
    { -0.5f, -0.25f },
    {  0.5f,  0.0f  },
    { -0.5f,  0.5f  },
    {  0.5f,  0.0f  },
    { -0.5f, -0.75f },
};

// FUNC 0x0019C738 RaceRoundMeterBase::draw_needle
//   Index: RaceRoundMeter::virtual_14 (slot 14, new in RaceRoundMeterBase;
//   also RaceBoostmeter's).  The angle of value / scale (value clamped to
//   0..scale + 8) between the start and end angles; the needle (g_0032F250,
//   from needleInner, length needleOuter - needleInner) drawn 8 times at
//   alpha 1/8 (BLEND_FIX), rotated by angle + step * (i + 1) with step =
//   (angle - last angle) / 8 (0 the first time) - beyond the current angle,
//   not between the two (as in the code) - each followed by the next
//   sub-pixel offset.  Colour NEEDLE_COLOR; blending on, BLEND_ALPHA back
//   at the end.
void RaceRoundMeterBase::draw_needle()
{
    f32 shown, angle, step, length;
    int i;

    pgl_color(NEEDLE_COLOR);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(BLEND_FIX, PASS_ALPHA);
    shown = meter_clampf(value, 0.0f, scale + 8.0f);
    angle = shown / scale * (endAngle - startAngle) + startAngle;
    if (lastAngle != FLT_MAX_F)
        step = (angle - lastAngle) * PASS_ALPHA;
    else
        step = 0.0f;
    lastAngle = angle;
    length = needleOuter - needleInner;
    for (i = 0; i < RACE_METER_PASSES; i++) {
        pgl_push_matrix();
        vu0_rotate_z(angle + step * (f32)(i + 1));
        pgl_translate(needleInner, 0.0f, 0.0f);
        vu0_scale(length, length, length);
        pgl_draw_strip(g_0032F250, NULL, NULL, NULL, 4);
        gfx_offset_xy(g_0032F568[i][0], g_0032F568[i][1]);
        pgl_pop_matrix();
    }
    gfx_blend_func(BLEND_ALPHA, 1.0f);
}

// FUNC 0x0019C8E8 RaceRoundMeterBase::setCenter
//   6 callers (the dial constructors).  In panel image pixels from
//   the rectangle's centre.
void RaceRoundMeterBase::setCenter(f32 cx, f32 cy)
{
    centerX = cx;
    centerY = cy;
}

// FUNC 0x0019C8F8 RaceRoundMeterBase::setRadius
//   radiusX = r * aspect, radiusY = r.
void RaceRoundMeterBase::setRadius(f32 r, f32 aspect)
{
    radiusY = r;
    radiusX = r * aspect;
}

// FUNC 0x0019C908 RaceRoundMeterBase::setAngles
//   Stored + 90 degrees.
void RaceRoundMeterBase::setAngles(f32 start, f32 end)
{
    startAngle = start + 90.0f;
    endAngle = end + 90.0f;
}

/* ======================================================================== */
/* RaceRoundMeter                                                           */
/* ======================================================================== */

// FUNC 0x0019C928 RaceRoundMeter::RaceRoundMeter
//   Callers: RaceRoundTachometerBase::RaceRoundTachometerBase,
//   RaceOnboardSpeedmeter::RaceOnboardSpeedmeter.  No scale packet yet; the
//   two packet buffers 16-byte aligned in dialData ((this + 0x8B) & ~0xF,
//   (this + 0x188B) & ~0xF); the dial defaults again, the scale defaults
//   (radius 0.95, marks 0.03 long, 3/4 filled, 0.1 apart, red from 0.75:
//   0x80D0D0D0 / 0x800000FF).  The EH landing pad calls
//   ~RaceRoundMeterBase.
RaceRoundMeter::RaceRoundMeter()
{
    /* implicit: RaceRoundMeterBase (base constructor), ~RaceRoundMeterBase (exception cleanup) */
    dialPacket = NULL;
    dialIndex = -1;
    dialBuf[0] = (void *)(((int)dialData + 0xF) & ~0xF);
    dialBuf[1] = (void *)(((int)(dialData + 0x1800) + 0xF) & ~0xF);
    setCenter(0.5f, 0.5f);
    setRadius(150.0f, 1.0f);
    scale = 1.0f;
    setAngles(65.0f, 295.0f);
    setNeedle(-0.1f, 0.95f);
    setScaleRadius(0.95f);
    setScaleLine(0.03f, 0.75f, 0.1f);
    setRedZone(0.75f);
    setScaleColor(0x80D0D0D0, 0x800000FF);
    setPanelColor(RACE_COLOR_WHITE);
}

// FUNC 0x0019CA58 RaceRoundMeter::~RaceRoundMeter
//   Index: RaceRoundMeter::virtual_0.  vptr 0x0032F2F0, then
//   ~RaceRoundMeterBase with the flags unchanged.  11 callers (RaceDisplay,
//   the tachometers, the speedometer).
RaceRoundMeter::~RaceRoundMeter()
{
    /* implicit: ~RaceRoundMeterBase (base destructor) */
}

// FUNC 0x0019CA80 RaceRoundMeter::updateScale
//   Callers: RaceDisplay::setupCarMeters,
//   RaceRoundTachometerBase::updateScale.  buildScale().
void RaceRoundMeter::updateScale()
{
    buildScale();
}

// FUNC 0x0019CAA0 T160_fill_words
//   ? name (a T160 helper or an inline library one).  No callers.  Fills
//   count words at dst with value: first ((dst >> 2) & 3) single words -
//   the word index in the quadword, not the number missing to the next
//   one, and not taken off count (as in the code) - then 4 words at a time
//   with sq (pextlw / pcpyld make the quadword; sq ignores the low 4
//   address bits, hence the rounding down here), then the rest one by one.
void T160_fill_words(u32 *dst, int count, u32 value)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    /* The quadword of four values: pextlw / pcpyld (MMI, which egcs never
     * emits: asm in the original, two volatile statements: they stay first),
     * stored with sq (egcs's TImode move; sq ignores the low 4 address
     * bits).  GCC 14: the four word stores of the C below. */
    u128 quad;

    __asm__ __volatile__("pextlw %0, %1, %1" : "=r"(quad) : "r"(value));
    __asm__ __volatile__("pcpyld %0, %1, %1" : "=r"(quad) : "r"(quad));
#endif
    int lead = ((int)dst >> 2) & 3;

    while (lead > 0) {
        *dst++ = value;
        lead--;
    }
    while (count >= 4) {
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
        *(u128 *)dst = quad;
#else
        u32 *q = (u32 *)((u32)dst & ~0xF);
        q[0] = value;
        q[1] = value;
        q[2] = value;
        q[3] = value;
#endif
        count -= 4;
        dst += 4;
    }
    while (count > 0) {
        *dst++ = value;
        count--;
    }
}

// FUNC 0x0019CB30 RaceRoundMeter::draw_scale
//   Index: RaceRoundMeter::virtual_13 (slot 13).  Once a scale packet
//   exists: drawn 8 times at alpha 1/8 (BLEND_ADD_FIX), each pass a GS
//   register flush, the packet (model_draw: a VifModel, TStripBuilder::finish's)
//   and the next sub-pixel offset; BLEND_ALPHA back at the end.
void RaceRoundMeter::draw_scale()
{
    int i;

    if (dialIndex < 0 || dialPacket == NULL)
        return;
    gfx_blend_func(BLEND_ADD_FIX, PASS_ALPHA);
    for (i = 0; i < RACE_METER_PASSES; i++) {
        gs_flush_dirty_regs();
        model_draw(dialPacket);
        gfx_offset_xy(g_0032F568[i][0], g_0032F568[i][1]);
    }
    gfx_blend_func(BLEND_ALPHA, 1.0f);
}

// FUNC 0x0019CBD0 RaceRoundMeter::buildScale
//   Builds the scale into the other packet buffer: n = scale /
//   scaleStep marks over the angle range, each a strip of segs + 1 pairs
//   of vertices (scaleRadius .. + scaleLength) over scaleFill of its angle
//   (centred: half the gap on each side), segs = 32 / n * |range| / 360
//   clamped to 2..31; the marks from redZone / scaleStep on (when that is
//   not 0) in the red colour.  The TStripBuilder lives on the stack
//   (0x990 bytes; its destructor ~TStripBuilder runs with flags 2, also on
//   the EH path).  Matching: the builder declared before the vertex (frame
//   order sb, vtx, sinA / cosA), the products of sin / cos read once
//   (retail keeps the outer pair across the first addVertex), the abs a
//   volatile asm after (f32)marks, the colour test written firstRed == 0 ||
//   i < firstRed (not if-converted), and no -ffast-math (/ 360 stays a
//   div.s).
void RaceRoundMeter::buildScale()
{
    f32 a = startAngle * DEG2RAD;
    f32 inner = scaleRadius;
    f32 outer = inner + scaleLength;
    int marks = (int)(scale / scaleStep);
    f32 fMarks = (f32)marks;
    f32 range = boot_fabsf(endAngle - startAngle);
    int segs = (int)(32.0f / fMarks * range / 360.0f);
    f32 markAngle, step, halfGap;
    int firstRed, i, j;

    if (segs < 2)
        segs = 2;
    else if (segs >= 32)
        segs = 31;
    markAngle = (endAngle - startAngle) * DEG2RAD / fMarks;
    firstRed = (int)(redZone / scaleStep);
    step = markAngle * scaleFill / (f32)segs;
    halfGap = markAngle * (1.0f - scaleFill) * 0.5f;
    if (dialIndex < 0)
        dialIndex = 0;
    else
        dialIndex = dialIndex ^ 1;

    TStripBuilder sb(dialBuf[dialIndex]);   /* sp+0x000 */
    RaceDisplayVertex vtx;                  /* sp+0x990 */

    sb.setPrim(SCALE_PRIM);
    sb.setShader(0);
    vtx.z = 0;
    for (i = 0; i < marks; i++) {
        const u32 *color;

        a += halfGap;
        if (firstRed == 0 || i < firstRed)
            color = &scaleColor[0];
        else
            color = &scaleColor[1];
        sb.endStrip();
        for (j = segs + 1; j > 0; j--) {
            f32 sinA, cosA;                 /* sp+0x9A0, sp+0x9A4 */
            f32 innerX, innerY, outerX, outerY;

            sincosf(a, &sinA, &cosA);
            innerX = cosA * inner;
            innerY = sinA * inner;
            outerX = cosA * outer;
            outerY = sinA * outer;
            vtx.x = innerX;
            vtx.y = innerY;
            sb.addVertex(&vtx.x, NULL, NULL, color);
            vtx.x = outerX;
            vtx.y = outerY;
            sb.addVertex(&vtx.x, NULL, NULL, color);
            a += step;
        }
        sb.endStrip();
        a += halfGap - step;
    }
    dialPacket = sb.finish(0, 0);
}

/* ======================================================================== */
/* RaceBarMeter                                                             */
/* ======================================================================== */

// FUNC 0x0019CEE0 RaceBarMeter::RaceBarMeter
//   Caller: RaceBattleTachometer::RaceBattleTachometer.
//   RaceMeterBase::RaceMeterBase, then the vptr 0x0032F280.
RaceBarMeter::RaceBarMeter()
{
    /* implicit: RaceMeterBase (base constructor) */
}

// FUNC 0x0019CF18 RaceBarMeter::drawMeter
//   Index: RaceBarMeter::drawMeter.  The bar image over the rectangle with
//   the alpha test PGL_ATEST_LESS at the reference value * 56 / scale
//   clamped to 4..64 (blending off while it is drawn): the image's alpha
//   gradient shows the bar up to the value.  The buffer is not used.
//   Caller: RaceBattleTachometer::drawMeter.
void RaceBarMeter::drawMeter(GSBuffer &buf)
{
    f32 ref;

    g_002F1968.flush();
    pgl_push_matrix();
    ref = meter_clampf(value * 56.0f / scale, 4.0f, 64.0f);
    gfx_alpha_func_raw(PGL_ATEST_LESS, (int)ref & 0xFF);
    gfx_disable(PGL_CAP_BLEND);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    pgl_color(RACE_COLOR_WHITE);
    race_draw_image_rect(image, (f32)x, (f32)y, (f32)w, (f32)h, alignX, alignY);
    gfx_enable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    pgl_pop_matrix();
}

// FUNC 0x0019D010 RaceBarMeter::setImage
//   (Tex1Image *): image = img and its TEX1 MMAG / MMIN bits
//   (5..8) cleared: point sampling.  Callers: setImage(const char *),
//   RaceBattleTachometer::setImage(Tex1Image *) (its bar at +0x20).
void RaceBarMeter::setImage(Tex1Image *img)
{
    GsTexRegs *regs = img->tex;

    image = img;
    regs->tex1 &= ~(u64)0x1E0;
}

// FUNC 0x0019D030 RaceBarMeter::setImage
//   (const char *): the named image of the pack (image_pack_find).
//   Callers: RaceBattleTachometer::setScale ("meter_mini"),
//   RaceBattleTachometer::setImage(const char *).
void RaceBarMeter::setImage(const char *name)
{
    setImage(image_pack_find(imagePack, name));
}

// FUNC 0x0019D068 RaceBarMeter::~RaceBarMeter
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy (RaceMeterBase's
 * vptr, its own store dead, then ~RaceDisplayObjectBase with the flags) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (egcs emits it with the vtable). */
// FUNC 0x0019D090 __tf12RaceBarMeter
//   __si_type_info __ti12RaceBarMeter (0x004B2320), base __ti13RaceMeterBase
//   (0x004B11D0, built by T154's __tf13RaceMeterBase, 0x00195AF8).
extern "C" const type_info &__tf12RaceBarMeter(void)
{
    if (__ti12RaceBarMeter[0] == 0) {
        __tf13RaceMeterBase();
        __rtti_si(__ti12RaceBarMeter, "12RaceBarMeter", (const type_info *)__ti13RaceMeterBase);
    }
    return *(const type_info *)__ti12RaceBarMeter;
}
#endif

/* ======================================================================== */
/* RaceRoundMeter / RaceRoundMeterBase accessors (no callers: inlined)      */
/* ======================================================================== */

// FUNC 0x0019D0E0 RaceRoundMeter::setScaleColor
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D0F0 RaceRoundMeter::getRedZone
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D0F8 RaceRoundMeter::setRedZone
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D100 RaceRoundMeter::setScaleLine
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D110 RaceRoundMeter::setScaleRadius
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019D118 __tf14RaceRoundMeter
//   __si_type_info __ti14RaceRoundMeter (0x004B2310), base
//   __ti18RaceRoundMeterBase.  Callers: __tf21RaceOnboardSpeedmeter (T154
//   0x00195A68, T162 0x0019E028).
extern "C" const type_info &__tf14RaceRoundMeter(void)
{
    if (__ti14RaceRoundMeter[0] == 0) {
        __tf18RaceRoundMeterBase();
        __rtti_si(__ti14RaceRoundMeter, "14RaceRoundMeter",
                  (const type_info *)__ti18RaceRoundMeterBase);
    }
    return *(const type_info *)__ti14RaceRoundMeter;
}
#endif

// FUNC 0x0019D168 RaceRoundMeterBase::draw_scale
//   Slot 13; empty (RaceBoostmeter keeps it).
/* inline in game/race/display/T160_race_meters.h (for T160 only: GAME_T160_INLINE_DRAW_SCALE);
 * egcs emits this copy with RaceRoundMeterBase's inline members */

// FUNC 0x0019D170 RaceRoundMeterBase::getCenterY
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D178 RaceRoundMeterBase::getCenterX
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D180 RaceRoundMeterBase::setPanelColor
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

// FUNC 0x0019D188 RaceRoundMeterBase::setNeedle
/* inline in game/race/display/T160_race_meters.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019D198 __tf18RaceRoundMeterBase
//   __si_type_info __ti18RaceRoundMeterBase (0x004B2300), base
//   __ti13RaceMeterBase (T154's __tf13RaceMeterBase).  Callers:
//   __tf14RaceRoundMeter, __tf14RaceBoostmeter.
extern "C" const type_info &__tf18RaceRoundMeterBase(void)
{
    if (__ti18RaceRoundMeterBase[0] == 0) {
        __tf13RaceMeterBase();
        __rtti_si(__ti18RaceRoundMeterBase, "18RaceRoundMeterBase",
                  (const type_info *)__ti13RaceMeterBase);
    }
    return *(const type_info *)__ti18RaceRoundMeterBase;
}
#endif

/* ======================================================================== */
/* T160's copies of RaceMeterBase's inline methods (unreferenced: the       */
/* vtables 0x0032E368 / 0x0032F3F0 point at T154's 0x00195AB8..0x00195AF8,  */
/* in the same order setScale / set / render_2nd / dtor / __tf).            */
/* ======================================================================== */

// FUNC 0x0019D1E8 RaceMeterBase::setScale
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (T154 has the
 * one the vtables use, 0x00195AB8).  index.txt: RaceCapacityMonitor::
 * setCapacity(float) [lupo2m], a match of the 2-instruction code only; its
 * place, the first of the five copies, makes it setScale. */

// FUNC 0x0019D1F0 RaceMeterBase::set
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (T154:
 * 0x00195AC0) */

// FUNC 0x0019D1F8 RaceMeterBase::render_2nd
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (T154:
 * 0x00195AC8) */

// FUNC 0x0019D200 RaceMeterBase::~RaceMeterBase
/* inline in game/race/display/T154_race_display.h; egcs emits this copy (T154:
 * 0x00195AD0; the vptr 0x0032E368, then ~RaceDisplayObjectBase with the
 * flags unchanged) */
/* implicit: ~RaceDisplayObjectBase (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0019D228 __tf13RaceMeterBase
//   T160's copy of T154's 0x00195AF8 (egcs emits the typeinfo function of a
//   class whose virtuals are all inline in every unit that needs it): the
//   __si_type_info __ti13RaceMeterBase (0x004B11D0), base
//   __ti21RaceDisplayObjectBase (0x003533A0, gp-0x4650).  Weak: T154's
//   definition (src/game/race/display/T154_race_display.cpp) is the one linked.
extern "C" __attribute__((weak)) const type_info &__tf13RaceMeterBase(void)
{
    if (__ti13RaceMeterBase[0] == 0) {
        __tf21RaceDisplayObjectBase();
        __rtti_si(__ti13RaceMeterBase, "13RaceMeterBase",
                  (const type_info *)__ti21RaceDisplayObjectBase);
    }
    return *(const type_info *)__ti13RaceMeterBase;
}
#endif
