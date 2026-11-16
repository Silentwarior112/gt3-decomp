/* T218_color.h - Color4f (translation unit T218, unit u066), 0x001BD250..0x001BD710.
 *
 * The float RGBA colour of the menu: the four vertex colours of
 * gt2k_menu::Drawable (T259), the colours of the operator scripts (T217).
 * Packed colours are RGBA8 with r in the low byte.  T218 also orders colours
 * by hue / saturation / value for the garage's colour sort (T219 cmp_color).
 */
#ifndef GAME_T218_COLOR_H
#define GAME_T218_COLOR_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

/* (? name) v clamped to [lo, hi]: the clamps of the inline channel / alpha /
 * blur increments (Color4f::incR .. incA, Drawable::incAlphaRatio,
 * incBlur2DX / Y).  Its bounds are arguments: the expansions load 0.0 and 1.0
 * before the first test (T217's handlers, T259's out-of-line copies). */
static inline f32 menu_clampf(f32 v, f32 lo, f32 hi)
{
    if (v < lo)
        v = lo;
    if (v > hi)
        v = hi;
    return v;
}

/* Not polymorphic; 0x10 bytes (the Drawable colour array stride).  The
 * constructors are inline in the original: Color4f(u32) is expanded wherever a
 * packed colour is unpacked (fromRGBA8 0x001BD2F0 is the same code out of
 * line, with no callers).  The default one sets 0.502 (0x3F008312, a .sdata
 * literal per translation unit): Drawable() / Transitive() construct
 * color[4] with it (the egcs array loop), the T069 constructors store it
 * into baseColor before the layout's colour (0x0014CEF4, 0x0014D7A0), and
 * the default-constructed locals of OilChangeCylinder::virtual_2
 * (0x00117174), MiniCursor::virtual_2, Item::drawImage, ... get it as dead
 * stores. */
struct Color4f {
    f32 r;                                  /* 0x0 */
    f32 g;                                  /* 0x4 */
    f32 b;                                  /* 0x8 */
    f32 a;                                  /* 0xC */

    Color4f() : r(0.502f), g(0.502f), b(0.502f), a(0.502f) {}
    Color4f(f32 r_, f32 g_, f32 b_, f32 a_) : r(r_), g(g_), b(b_), a(a_) {}
    /* Bytes / 255, r in the low byte, each converted as unsigned (the bltz /
     * srl / add.s sequence of the u32 -> float conversion). */
    Color4f(u32 rgba)
    {
        r = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
        g = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
        b = (f32)(rgba & 0xFF) / 255.0f; rgba >>= 8;
        a = (f32)(rgba & 0xFF) / 255.0f;
    }
    /* (? name) Inline in the original: MemorycardIconSprites::draw (u028,
     * 0x00150700) sets the four corners through `this' (&color[i] in a
     * register, the first store folded onto the base, the four arguments
     * loaded before the stores). */
    void set(f32 r_, f32 g_, f32 b_, f32 a_)
    {
        r = r_;
        g = g_;
        b = b_;
        a = a_;
    }
    /* Inline in the original, channel by channel: retail copies a colour
     * with four lwc1 / swc1 pairs through `this' (&color[i] in a register,
     * the first store folded onto the base: Drawable::setColors 0x001D0F20,
     * the inline Drawable::setColor(u32) in MiniCursor::virtual_2 and
     * others), where the implicit assignment is an ldl / ldr block copy. */
    Color4f &operator=(const Color4f &c)
    {
        r = c.r;
        g = c.g;
        b = c.b;
        a = c.a;
        return *this;
    }

    /* (? names) Per-channel accessors, inline in the original: Drawable's
     * getColorR .. A, setColorR .. A and incColorR .. A (T259) reach the
     * channel through them (&color[i] built as index + this, then +0x40:
     * the out-of-line copies 0x001D0630.. / 0x001D07A0.. / 0x001D1630..),
     * and so do T217's colour handlers.  inc*: += d, clamped to [0, 1]. */
    f32  getR() const { return r; }
    f32  getG() const { return g; }
    f32  getB() const { return b; }
    f32  getA() const { return a; }
    void setR(f32 v) { r = v; }
    void setG(f32 v) { g = v; }
    void setB(f32 v) { b = v; }
    void setA(f32 v) { a = v; }
    void incR(f32 d) { r = menu_clampf(r + d, 0.0f, 1.0f); }
    void incG(f32 d) { g = menu_clampf(g + d, 0.0f, 1.0f); }
    void incB(f32 d) { b = menu_clampf(b + d, 0.0f, 1.0f); }
    void incA(f32 d) { a = menu_clampf(a + d, 0.0f, 1.0f); }

    /* RGBA8, each channel clamped to [0, 1] (max.s / min.s), alpha multiplied
     * by alphaScale before its clamp. */
    u32  toRGBA8(f32 alphaScale) const;     /* 0x001BD250 */
    void fromRGBA8(u32 rgba);               /* 0x001BD2F0 = Color4f(u32), no callers */
};
EGCS_SIZE(Color4f, 0x10);

/* -1 / 0 / 1.  Two saturated colours (saturation > 0.1) by 30-degree hue
 * sector, one saturated colour by saturation, otherwise by value, then by the
 * hue difference wrapped to [-180, 180].  (rgb_to_hsv 0x001BD5F8 is
 * file-local.) */
int color_compare_hsv(const Color4f *a, const Color4f *b);  /* 0x001BD420 */

#endif /* __cplusplus */

#endif /* GAME_T218_COLOR_H */
