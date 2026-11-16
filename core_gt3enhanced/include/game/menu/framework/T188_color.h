/* T188_color.h - Color8 (? name), an 8-bit RGBA colour (translation unit
 * T188, unit u040), 0x00174788..0x00174AE8.
 *
 * Components 0..255, alpha 0..128 (GS alpha, 0x80 = opaque).  setClamped
 * and the accessors are also called by T146 (the licence examination
 * messages: draw_shadow_text 0x0018A798, ExamMessageLine::draw 0x0018AD58);
 * scale and add have no callers.  T146 (u049)
 * holds copies of set (0x0018BE00, called) and of the four setters
 * (0x0018BE20 setA, 0x0018BE28 setB, 0x0018BE30 setG, 0x0018BE38 setR: no
 * callers). */
#ifndef GAME_T188_COLOR_H
#define GAME_T188_COLOR_H

#include "game/game.h"

/* Not polymorphic; 4 bytes.  Every member but setClamped / scale / add is
 * inline, defined in the class in this order (u049, from both units'
 * copies): egcs integrates an inline member's call to another only when
 * the callee's body was compiled first, so the constructors, the
 * assignment, set(r, g, b, a) and setAlpha, compiled first, call the
 * accessors and set(u32) out of line.  The units using those emit the
 * copies they need at their end, after their typeinfo functions, in
 * reverse declaration order: T188 getA .. getR, setA .. setR
 * (0x00174AA8..0x00174AE0, its copy constructor's calls), T146 set(u32)
 * and setA .. setR (0x0018BE00..0x0018BE38; its calls reach T188's
 * setters, the copies linked first). */
class Color8 {
public:
    /* Inline constructors (? the original's): the copy goes through the
     * accessors, out of line (T188's scale / add call getR, setR, getG ...
     * on their result: `return tmp;' copy-constructs it into the caller's
     * buffer).  With a constructor egcs returns a Color8 in memory (the
     * hidden buffer in $a0), as retail's scale / add do. */
    Color8() {}
    Color8(const Color8 &o)
    {
        setR(o.getR());
        setG(o.getG());
        setB(o.getB());
        setA(o.getA());
    }
    /* (? the original's) From a 32-bit value through set(u32), out of line
     * (u049's "constructor Color8(u32)": T146's renderWhenPassing /
     * renderWhenDisqualifying call set on their colour and on the
     * temporaries of `color = Color8(...)'). */
    Color8(u32 rgba) { set(rgba); }
    /* (? the original's) Component-wise assignment: T146's renderWhenPassing
     * copies a colour with four lbu / sb where the implicit (trivial)
     * assignment is one unaligned word move (lwl / lwr). */
    Color8 &operator=(const Color8 &o)
    {
        c[0] = o.c[0];
        c[1] = o.c[1];
        c[2] = o.c[2];
        c[3] = o.c[3];
        return *this;
    }
    /* (? the original's) The four components through the setters, out of
     * line: T146's draw_shadow_text and ExamMessageLine::draw compute the
     * colour's frame address afresh for every setter call (an inline
     * member's `this', a constant frame address). */
    void set(u8 r, u8 g, u8 b, u8 a) { setR(r); setG(g); setB(b); setA(a); }
    /* (? name) Alpha a (0..1) * 128 into component 3, clamped to 128
     * (ExamMessageLine::draw, the same evidence). */
    void setAlpha(f32 a) { setClamped(3, a * 128.0f, 128.0f); }
    /* The 32-bit value (u049 builds it inline from the four bytes). */
    u32  value() const { return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24); }
    /* Component i = v clamped to [0, max], converted to unsigned. */
    void setClamped(int i, f32 v, f32 max);     /* 0x00174788 */
    /* The colour with each component times its factor, clamped to 255
     * (alpha 128).  A factor of exactly 1.0 skips the component, and the
     * temporary's component is then never written (the result gets
     * whatever was on the stack: a bug of the original). */
    Color8 scale(f32 r, f32 g, f32 b, f32 a) const;     /* 0x001747E0 */
    /* The same with added amounts (an amount of 0 skips the component). */
    Color8 add(u8 r, u8 g, u8 b, u8 a) const;           /* 0x00174958 */
    void setR(u8 v) { c[0] = v; }               /* 0x00174AE0, T146 0x0018BE38 */
    void setG(u8 v) { c[1] = v; }               /* 0x00174AD8, T146 0x0018BE30 */
    void setB(u8 v) { c[2] = v; }               /* 0x00174AD0, T146 0x0018BE28 */
    void setA(u8 v) { c[3] = v; }               /* 0x00174AC8, T146 0x0018BE20 */
    u8   getR() const { return c[0]; }          /* 0x00174AC0 */
    u8   getG() const { return c[1]; }          /* 0x00174AB8 */
    u8   getB() const { return c[2]; }          /* 0x00174AB0 */
    u8   getA() const { return c[3]; }          /* 0x00174AA8 */
    /* From a 32-bit value: r = bits 0..7, g = 8..15, b = 16..23, a = 24..31.
     * Only T146 has a copy (0x0018BE00). */
    void set(u32 rgba)
    {
        c[0] = rgba;
        c[1] = rgba >> 8;
        c[2] = rgba >> 16;
        c[3] = rgba >> 24;
    }

    u8 c[4];                                    /* 0x0 r, g, b, a */
};
EGCS_SIZE(Color8, 0x4);

#endif /* GAME_T188_COLOR_H */
