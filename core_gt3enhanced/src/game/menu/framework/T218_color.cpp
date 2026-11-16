/* T218 - Color4f.  0x001BD250..0x001BD710.
 *
 * RGBA8 (r in the low byte) <-> four floats 0..1, and the colour order of
 * the garage's colour sort (T219 cmp_color): hue sectors of 30 degrees for
 * saturated colours, then saturation, then value, then hue. */
/* The original object has no .rodata (no type names, no string): it saw
 * Color4f only.  Its data: .lit4 (0x00350064) and the frame info. */
#include "game/menu/framework/T218_color.h"

/* HSV of a colour (rgb_to_hsv's output). */
struct ColorHSV {
    f32 hue;                                /* 0x0 degrees [0, 360) */
    f32 saturation;                         /* 0x4 (max - min) / max, 0 for black */
    f32 value;                              /* 0x8 the largest channel */
    f32 alpha;                              /* 0xC */
};

/* A colour counts as saturated (ordered by hue) above this. */
static const f32 SATURATED = 0.1f;

/* max.s / min.s as volatile asm statements, as every max.s / min.s of the
 * image behaves (u055's meter_clampf; u018, u032, u044): toRGBA8 converts its
 * channels strictly one after the other, and T218 is compiled without
 * -ffast-math (fromRGBA8 keeps its div.s), so neither the egcs operators
 * >? / <? (max.s / min.s, but the four channels interleave) nor C comparisons
 * (branches) reproduce it.  GCC 14: the C comparisons (the same but for -0.0
 * against 0.0). */
static inline f32 color_maxf(f32 a, f32 b)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;

    __asm__ __volatile__("max.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
#else
    return a > b ? a : b;
#endif
}

static inline f32 color_minf(f32 a, f32 b)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;

    __asm__ __volatile__("min.s %0, %1, %2" : "=f"(r) : "f"(a), "f"(b));
    return r;
#else
    return a < b ? a : b;
#endif
}

/* x clamped to lo..hi. */
static inline f32 color_clampf(f32 x, f32 lo, f32 hi)
{
    return color_minf(color_maxf(x, lo), hi);
}

/* Packs to RGBA8; each channel clamped to [0, 1] (max.s / min.s), alpha
 * multiplied by alphaScale before its clamp.  Alpha is not masked (its top
 * bits are shifted out). */
// FUNC 0x001BD250 Color4f::toRGBA8
u32 Color4f::toRGBA8(f32 alphaScale) const
{
    int red = (int)(color_clampf(r, 0.0f, 1.0f) * 255.0f);
    int green = (int)(color_clampf(g, 0.0f, 1.0f) * 255.0f);
    int blue = (int)(color_clampf(b, 0.0f, 1.0f) * 255.0f);
    int alpha = (int)(color_clampf(a * alphaScale, 0.0f, 1.0f) * 255.0f);

    return (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16) | (alpha << 24);
}

/* Unpacks RGBA8, byte / 255 (unsigned conversions).  No callers: the same
 * code is expanded inline where it is used (Color4f(u32)). */
// FUNC 0x001BD2F0 Color4f::fromRGBA8
void Color4f::fromRGBA8(u32 rgba)
{
    r = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
    g = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
    b = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
    a = (f32)(rgba & 0xFF) / 255.0f;
}

static ColorHSV *rgb_to_hsv(ColorHSV *hsv, const Color4f *c);   /* 0x001BD5F8, below */

/* -1 / 0 / 1.  Two saturated colours by hue sector; one saturated colour by
 * saturation; otherwise by value, then by the hue difference wrapped to
 * [-180, 180]. */
// FUNC 0x001BD420 color_compare_hsv
int color_compare_hsv(const Color4f *a, const Color4f *b)
{
    ColorHSV ha;
    ColorHSV hb;

    rgb_to_hsv(&ha, a);
    rgb_to_hsv(&hb, b);
    if (SATURATED < ha.saturation && SATURATED < hb.saturation) {
        /* the 30-degree hue sectors, 0..11 (the sector around 360 counts as
         * 0); both computed side by side (retail's div / div1 pair) */
        int sa = (int)ha.hue;
        int sb = (int)hb.hue;
        sa = (sa + 15) / 30;
        sb = (sb + 15) / 30;
        if (sa >= 12)
            sa -= 12;
        if (sb >= 12)
            sb -= 12;
        f32 sectorA = sa;
        f32 sectorB = sb;
        if (sectorA != sectorB) {
            if (sectorA < sectorB)
                return -1;
            return 1;
        }
    } else if (SATURATED < ha.saturation || SATURATED < hb.saturation) {
        if (ha.saturation < hb.saturation)
            return -1;
        if (hb.saturation < ha.saturation)
            return 1;
        return 0;
    }
    if (ha.value < hb.value)
        return -1;
    if (hb.value < ha.value)
        return 1;
    {
        f32 dh = ha.hue - hb.hue;
        if (180.0f < dh)
            dh -= 360.0f;
        if (dh < -180.0f)
            dh += 360.0f;
        if (dh < 0.0f)
            return -1;
        if (0.0f < dh)
            return 1;
        return 0;
    }
}

/* RGB -> HSV, the hue in degrees; returns hsv ($v0 = $a0 in retail; the
 * caller ignores it).  The hue would stay unset if no channel equalled the
 * maximum exactly (cannot happen). */
// FUNC 0x001BD5F8 rgb_to_hsv
static ColorHSV *rgb_to_hsv(ColorHSV *hsv, const Color4f *c)
{
    f32 red = c->r;
    f32 green = c->g;
    f32 blue = c->b;
    f32 alpha = c->a;
    f32 max = color_maxf(color_maxf(red, green), blue);
    f32 min = color_minf(color_minf(red, green), blue);
    f32 delta = max - min;

    hsv->value = max;
    hsv->saturation = (max == 0.0f) ? 0.0f : delta / max;
    if (hsv->saturation == 0.0f) {
        hsv->hue = 0.0f;
    } else {
        if (red == max)
            hsv->hue = (green - blue) / delta;
        else if (green == max)
            hsv->hue = (blue - red) / delta + 2.0f;
        else if (blue == max)
            hsv->hue = (red - green) / delta + 4.0f;
        hsv->hue = hsv->hue * 60.0f;
        if (hsv->hue < 0.0f)
            hsv->hue = hsv->hue + 360.0f;
    }
    hsv->alpha = alpha;
    return hsv;
}
