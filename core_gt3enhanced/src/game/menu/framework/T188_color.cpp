/* T188 - Color8 (? name), an 8-bit RGBA colour (game/menu/framework/T188_color.h):
 * components 0..255, alpha 0..128 (GS alpha, 0x80 = opaque).
 * 0x00174788..0x00174AE8.
 *
 * setClamped and the accessors are also called by T146 (draw_shadow_text,
 * ExamMessageLine::draw: the licence examination messages); scale / add have
 * no callers. */
/* Retail's T188 has no .rodata: it sees no class with inline virtual
 * functions only (its float constants are lui / mtc1). */
#include "game/menu/framework/T188_color.h"

/* Component i = v clamped to [0, max], converted to unsigned (fptoui,
 * 0x0027AEA0: libgcc's __fixunssfsi).  Three cases (retail computes &c[i]
 * before the max test, in the branch's delay slot). */
// FUNC 0x00174788 Color8::setClamped
void Color8::setClamped(int i, f32 v, f32 max)
{
    if (v < 0.0f)
        c[i] = 0;
    else if (max < v)
        c[i] = (u32)max;
    else
        c[i] = (u32)v;
    /* implicit: fptoui (the float to unsigned conversion) */
}

/* The colour with each component times its factor, clamped.  A factor of
 * exactly 1.0 skips the component, and then the temporary's component is
 * never written: the result gets whatever was on the stack (a bug of the
 * original - the temporary is not initialised from *this).  `return tmp':
 * Color8's inline copy constructor (game/menu/framework/T188_color.h) copies it into the
 * caller's buffer ($a0) through the accessors. */
// FUNC 0x001747E0 Color8::scale
Color8 Color8::scale(f32 r, f32 g, f32 b, f32 a) const
{
    Color8 tmp;                             /* sp+0x0, uninitialised */

    if (r != 1.0f)
        tmp.setClamped(0, (f32)(int)c[0] * r, 255.0f);
    if (g != 1.0f)
        tmp.setClamped(1, (f32)(int)c[1] * g, 255.0f);
    if (b != 1.0f)
        tmp.setClamped(2, (f32)(int)c[2] * b, 255.0f);
    if (a != 1.0f)
        tmp.setClamped(3, (f32)(int)c[3] * a, 128.0f);
    return tmp;
    /* implicit: getR, setR, getG, setG, getB, setB, getA, setA (Color8's inline
     * copy constructor, into the caller's buffer) */
}

/* The colour with the amounts added, clamped.  An amount of 0 skips the
 * component and leaves the temporary's component unwritten, as in scale. */
// FUNC 0x00174958 Color8::add
Color8 Color8::add(u8 r, u8 g, u8 b, u8 a) const
{
    Color8 tmp;                             /* sp+0x0, uninitialised */

    if (r != 0)
        tmp.setClamped(0, (f32)(c[0] + r), 255.0f);
    if (g != 0)
        tmp.setClamped(1, (f32)(c[1] + g), 255.0f);
    if (b != 0)
        tmp.setClamped(2, (f32)(c[2] + b), 255.0f);
    if (a != 0)
        tmp.setClamped(3, (f32)(c[3] + a), 128.0f);
    return tmp;
    /* implicit: getR, setR, getG, setG, getB, setB, getA, setA (Color8's inline
     * copy constructor, into the caller's buffer) */
}

// FUNC 0x00174AA8 Color8::getA
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[3] */

// FUNC 0x00174AB0 Color8::getB
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[2] */

// FUNC 0x00174AB8 Color8::getG
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[1] */

// FUNC 0x00174AC0 Color8::getR
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[0] */

// FUNC 0x00174AC8 Color8::setA
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[3] = v */

// FUNC 0x00174AD0 Color8::setB
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[2] = v */

// FUNC 0x00174AD8 Color8::setG
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[1] = v */

// FUNC 0x00174AE0 Color8::setR
/* inline in game/menu/framework/T188_color.h; egcs emits this copy (the copy constructor
 * calls it out of line): c[0] = v */
