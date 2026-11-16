/* T259 - gt2k_menu::Drawable (unit u072, 0x001CFE70..0x001D1A38).
 *
 * The visual state shared by every menu object: centre (two MotionValues),
 * size, four vertex colours, blur offsets, F0..F3 (per-class parameters),
 * an anchor object the position is relative to, alpha ratio, draw flags and
 * visibility, with their accessors; the vertex / colour helpers the draw
 * methods use; and virtual_2, which applies the render state (a custom
 * function, the default state, or one of four blends).
 *
 * Nothing calls 0x001D05E0..0x001D19A0 (the accessors, the setters, the
 * screen-space helpers): like Drawable() and setColor(u32) there, which are
 * inline, they are probably the out-of-line copies of members defined in the
 * class (? the original header; T259_menu_drawable.h keeps them declared,
 * as the i3 note decided for plain field accessors).
 *
 * Data (u072's data pass): the original's .rodata is the tap table, the
 * vtable and Drawable's type name only (0x00337830..0x003378F6; the garage
 * names after it are u073's T262's): T259 saw Drawable's own header, not
 * Transitive's (no menu operator class).
 */
#include "game/menu/framework/menu_drawables_internal.h"
#include "game/menu/framework/T259_menu_drawable.h"

using namespace gt2k_menu;

/* drawBlur's tap offsets (x, y), in units of blur2DX / blur2DY: a 4 x 4
 * grid, x -1.5, -0.5, 0.5, 1.5, y staggered per column.  External (the
 * header's extern): written here, first in .rodata, before the vtable. */
const f32 g_00337830[16][2] = {             /* 0x00337830 */
    { -1.5f, -1.75f }, { -1.5f, -0.75f }, { -1.5f, 0.25f }, { -1.5f, 1.25f },
    { -0.5f, -1.25f }, { -0.5f, -0.25f }, { -0.5f, 0.75f }, { -0.5f, 1.75f },
    {  0.5f, -1.75f }, {  0.5f, -0.75f }, {  0.5f, 0.25f }, {  0.5f, 1.25f },
    {  1.5f, -1.25f }, {  1.5f, -0.25f }, {  1.5f, 0.75f }, {  1.5f, 1.75f },
};

// FUNC 0x001CFE70 gt2k_menu::Drawable::drawBlur
/* (? name) BLUR_TAPS copies of the quad, each offset by a tap of
 * g_00337830 scaled by (blur2DX, blur2DY), each blended over the last with
 * the constant F0 (blend (Cs - Cd) * FIX + Cd), pixels of alpha 0 dropped
 * (alpha test GREATER 0).  Callers: the Rectangle and Image draws.  Uses
 * x / y directly (no anchor).
 * Matching: the loop counts up (check_dbra_loop turns it into retail's
 * count-down with a tap pointer), the taps are read from the const table at
 * each use (not killed by the vertex stores: one load per pass), and the
 * value through MotionValue::get() (&x / &y kept in $s2 / $s1). */
void Drawable::drawBlur(u32 *colors, PrimUV *uv)
{
    gfx_blend_func(MENU_BLEND_FIX, f[0]);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_alpha_func(PGL_ATEST_GREATER, 0.0f);
    for (int i = 0; i < BLUR_TAPS; i++) {
        PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
        v[0].x = x.get() - w * 0.5f + g_00337830[i][0] * blur2DX;
        v[0].y = y.get() - h * 0.5f + g_00337830[i][1] * blur2DY;
        v[0].z = 0;
        v[1].x = x.get() + w * 0.5f + g_00337830[i][0] * blur2DX;
        v[1].y = y.get() - h * 0.5f + g_00337830[i][1] * blur2DY;
        v[1].z = 0;
        v[2].x = x.get() - w * 0.5f + g_00337830[i][0] * blur2DX;
        v[2].y = y.get() + h * 0.5f + g_00337830[i][1] * blur2DY;
        v[2].z = 0;
        v[3].x = x.get() + w * 0.5f + g_00337830[i][0] * blur2DX;
        v[3].y = y.get() + h * 0.5f + g_00337830[i][1] * blur2DY;
        v[3].z = 0;
        pgl_draw_strip(v, NULL, colors, uv, 4);
    }
}

// FUNC 0x001D0058 gt2k_menu::Drawable::setRectVertices
/* The strip of the quad (cx, cy) +- (qw, qh) / 2 (top left, top right,
 * bottom left, bottom right) plus the anchor's offset (anchorType: ANCHOR_X
 * its screen x, ANCHOR_Y its screen y, ANCHOR_XY both; none once the anchor
 * is dead); the right / bottom edges get +0.5. */
void Drawable::setRectVertices(PrimVertex *v, f32 cx, f32 cy, f32 qw, f32 qh)
{
    f32 anchorX, anchorY;
    const Drawable *a = anchor;

    anchorX = 0.0f;
    anchorY = 0.0f;
    if (a != NULL && a->condition != CONDITION_DEAD) {
        switch (anchorType & ANCHOR_XY) {
        case ANCHOR_X:
            anchorX = a->getScreenX();
            break;
        case ANCHOR_Y:
            anchorY = a->getScreenY();
            break;
        case ANCHOR_XY:
            a->getScreenXY(&anchorX, &anchorY);
            break;
        }
    }
    f32 halfW = qw * 0.5f;
    f32 halfH = qh * 0.5f;
    f32 left = cx - halfW + anchorX;
    f32 top = cy - halfH + anchorY;
    f32 right = cx + halfW + anchorX + 0.5f;
    f32 bottom = cy + halfH + anchorY + 0.5f;
    v[0].x = left;
    v[0].y = top;
    v[0].z = 0;
    v[1].x = right;
    v[1].y = top;
    v[1].z = 0;
    v[2].x = left;
    v[2].y = bottom;
    v[2].z = 0;
    v[3].x = right;
    v[3].y = bottom;
    v[3].z = 0;
}

// FUNC 0x001D01A0 gt2k_menu::Drawable::F_001D01A0
/* The same arguments to setRectVertices (callers MpegImage::virtual_2,
 * F_001D18F8, F_001D1920). */
void Drawable::F_001D01A0(PrimVertex *v, f32 cx, f32 cy, f32 qw, f32 qh)
{
    setRectVertices(v, cx, cy, qw, qh);
}

// FUNC 0x001D01C0 gt2k_menu::Drawable::getColors
/* The four vertex colours packed, alpha times alphaRatio. */
void Drawable::getColors(u32 *out)
{
    for (int i = 0; i < 4; i++)
        out[i] = color[i].toRGBA8(alphaRatio);
}

// FUNC 0x001D0228 gt2k_menu::Drawable::getPosition
/* (callers: GearChart::virtual_2 and the Rectangle frames) the centre plus
 * the anchor's screen position, by bits (anchorType & ANCHOR_X x, &
 * ANCHOR_Y y); nothing from a dead anchor. */
void Drawable::getPosition(f32 *px, f32 *py)
{
    f32 anchorY = 0.0f;
    f32 anchorX = anchorY;
    const Drawable *a = anchor;

    if (a != NULL && a->condition != CONDITION_DEAD) {
        if (anchorType & ANCHOR_X)
            anchorX = a->getScreenX();
        if (anchorType & ANCHOR_Y)
            anchorY = anchor->getScreenY();     /* anchor loaded again */
    }
    *px = x.value + anchorX;
    *py = y.value + anchorY;
}

// FUNC 0x001D02E0 gt2k_menu::Drawable::getScreenX
/* x.value plus the anchors' (the whole chain, whatever anchorType, dead or
 * not). */
f32 Drawable::getScreenX() const
{
    if (anchor != NULL)
        return anchor->getScreenX() + x.value;
    return x.value;
}

// FUNC 0x001D0320 gt2k_menu::Drawable::getScreenY
f32 Drawable::getScreenY() const
{
    if (anchor != NULL)
        return anchor->getScreenY() + y.value;
    return y.value;
}

// FUNC 0x001D0360 gt2k_menu::Drawable::getScreenXY
void Drawable::getScreenXY(f32 *px, f32 *py) const
{
    *px = *py = 0.0f;
    if (anchor != NULL)
        anchor->getScreenXY(px, py);
    *px = *px + x.value;
    *py = *py + y.value;
}

// FUNC 0x001D03D8 gt2k_menu::Drawable::setDefaultDrawSetting
/* The render state of an object without DRAW_OWN_SETTINGS: no alpha test
 * (its function GREATER 1.0), alpha blend, no depth test (its function
 * LEQUAL), every colour channel written, the texture alpha used (TCC 1),
 * modulated, filtered linearly. */
void Drawable::setDefaultDrawSetting()
{
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_alpha_func(PGL_ATEST_GREATER, 1.0f);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(MENU_BLEND_ALPHA, 1.0f);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_color_mask(1, 1, 1, 1);
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
}

// FUNC 0x001D0478 gt2k_menu::Drawable::virtual_2
/* The draw settings: drawSettingFunc if set; else without DRAW_OWN_SETTINGS
 * the default state; else the texture filter from DRAW_LINEAR, blending on
 * and the blend of bits 4..7 (the alpha test off, except for
 * DRAW_BLEND_TESTED: on, GEQUAL 0.5).  Other blend values set nothing more. */
void Drawable::virtual_2()
{
    if (drawSettingFunc != NULL) {
        drawSettingFunc();
    } else if (drawFlags & DRAW_OWN_SETTINGS) {
        if (drawFlags & DRAW_LINEAR) {
            gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_LINEAR);
            gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_LINEAR);
        } else {
            gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
            gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
        }
        gfx_enable(PGL_CAP_BLEND);
        switch (drawFlags & DRAW_BLEND_MASK) {
        case DRAW_BLEND_ALPHA:
            gfx_disable(PGL_CAP_ALPHA_TEST);
            gfx_blend_func(MENU_BLEND_ALPHA, 1.0f);
            break;
        case DRAW_BLEND_ADD:
            gfx_disable(PGL_CAP_ALPHA_TEST);
            gfx_blend_func(MENU_BLEND_ADD, 1.0f);
            break;
        case DRAW_BLEND_SUB:
            gfx_disable(PGL_CAP_ALPHA_TEST);
            gfx_blend_func(MENU_BLEND_SUB, 1.0f);
            break;
        case DRAW_BLEND_TESTED:
            gfx_enable(PGL_CAP_ALPHA_TEST);
            gfx_alpha_func(PGL_ATEST_GEQUAL, 0.5f);
            gfx_blend_func(MENU_BLEND_NO_DEST, 1.0f);
            break;
        }
    } else {
        setDefaultDrawSetting();
    }
}

// FUNC 0x001D05E0 gt2k_menu::Drawable::getAlphaRatio
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D05E8 gt2k_menu::Drawable::getAnchor
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D05F0 gt2k_menu::Drawable::getF3
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D05F8 gt2k_menu::Drawable::getF2
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0600 gt2k_menu::Drawable::getF1
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0608 gt2k_menu::Drawable::getF0
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0610 gt2k_menu::Drawable::getBlur2DY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0618 gt2k_menu::Drawable::getBlur2DX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0620 gt2k_menu::Drawable::getColor
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0630 gt2k_menu::Drawable::getColorA
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0640 gt2k_menu::Drawable::getColorB
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0650 gt2k_menu::Drawable::getColorG
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0660 gt2k_menu::Drawable::getColorR
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0670 gt2k_menu::Drawable::getH
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0678 gt2k_menu::Drawable::getW
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0680 gt2k_menu::Drawable::getY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0688 gt2k_menu::Drawable::getX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0690 gt2k_menu::Drawable::getDY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0698 gt2k_menu::Drawable::getDX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D06A0 gt2k_menu::Drawable::incAlphaRatio
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D06E0 gt2k_menu::Drawable::incF3
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D06F0 gt2k_menu::Drawable::incF2
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0700 gt2k_menu::Drawable::incF1
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0710 gt2k_menu::Drawable::incF0
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0720 gt2k_menu::Drawable::incBlur2DY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0760 gt2k_menu::Drawable::incBlur2DX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D07A0 gt2k_menu::Drawable::incColorA
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D07E8 gt2k_menu::Drawable::incColorB
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0830 gt2k_menu::Drawable::incColorG
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0878 gt2k_menu::Drawable::incColorR
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D08C0 gt2k_menu::Drawable::incH
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D08D0 gt2k_menu::Drawable::incW
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D08E0 gt2k_menu::Drawable::incY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D08F8 gt2k_menu::Drawable::incX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0910 gt2k_menu::Drawable::setDrawMode
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0928 gt2k_menu::Drawable::setVisible
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0930 gt2k_menu::Drawable::setAlphaRatio
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0938 gt2k_menu::Drawable::setAnchor
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0948 gt2k_menu::Drawable::setF0123
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0960 gt2k_menu::Drawable::setF3
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0968 gt2k_menu::Drawable::setF2
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0970 gt2k_menu::Drawable::setF1
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0978 gt2k_menu::Drawable::setF0
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0980 gt2k_menu::Drawable::setDrawSettingFunc
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0988 gt2k_menu::Drawable::setBlur2DY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0990 gt2k_menu::Drawable::setBlur2DX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

/* The four setColor overloads: the markers name their symbols (the copies
 * are compiled from the header, and the name alone is ambiguous). */
// FUNC 0x001D0998 gt2k_menu::Drawable::setColor(u32) (setColor__Q29gt2k_menu8DrawableUi)
/* setColor(u32): inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy
 * (no callers): the four corners, each from a fresh Color4f(rgba) (the
 * conversion four times, through a stack temporary) */

// FUNC 0x001D0EF0 gt2k_menu::Drawable::setColor (setColor__Q29gt2k_menu8DrawablePC7Color4fi)
/* setColor(const Color4f *, int): inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D0F20 gt2k_menu::Drawable::setColors
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1490 gt2k_menu::Drawable::setColor(u32, int) (setColor__Q29gt2k_menu8DrawableUii)
/* setColor(u32, int): inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1608 gt2k_menu::Drawable::setColor(int, f32, f32, f32, f32) (setColor__Q29gt2k_menu8Drawableiffff)
/* setColor(int, f32, f32, f32, f32): inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1630 gt2k_menu::Drawable::setColorA
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1640 gt2k_menu::Drawable::setColorB
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1650 gt2k_menu::Drawable::setColorG
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1660 gt2k_menu::Drawable::setColorR
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1670 gt2k_menu::Drawable::setRect
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D16A0 gt2k_menu::Drawable::setH
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D16A8 gt2k_menu::Drawable::setW
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D16B0 gt2k_menu::Drawable::setY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D16C8 gt2k_menu::Drawable::setX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D16E0 gt2k_menu::Drawable::Drawable
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (no callers:
 * Transitive() and the ButtonList constructors expand it): Object(), x / y
 * zeroed, the vptr, w = h = 100, color[4] = 0.502 (the Color4f() array
 * loop), then visible 1, alphaRatio 1, the rest 0; anchorType unset */
/* implicit: Object (base constructor) */

// FUNC 0x001D17B0 gt2k_menu::Drawable::setVertex
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D17D0 gt2k_menu::Drawable::getScreenH
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (GSBuffer::displayHeight) */
/* implicit: displayHeight (the inline body) */

// FUNC 0x001D1818 gt2k_menu::Drawable::getScreenW
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D1848 gt2k_menu::Drawable::getScreenPosY
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (GSBuffer::displayHeight) */
/* implicit: displayHeight (the inline body) */

// FUNC 0x001D1898 gt2k_menu::Drawable::getScreenPosX
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy */

// FUNC 0x001D18D0 gt2k_menu::Drawable::getPackedColor
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (Color4f::toRGBA8) */
/* implicit: toRGBA8 (the inline body) */

// FUNC 0x001D18F8 gt2k_menu::Drawable::F_001D18F8
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (F_001D01A0) */
/* implicit: F_001D01A0 (the inline body) */

// FUNC 0x001D1920 gt2k_menu::Drawable::F_001D1920
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (F_001D01A0) */
/* implicit: F_001D01A0 (the inline body) */

// FUNC 0x001D1940 gt2k_menu::Drawable::F_001D1940
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (setRectVertices) */
/* implicit: setRectVertices (the inline body) */

// FUNC 0x001D1968 gt2k_menu::Drawable::F_001D1968
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy (setRectVertices) */
/* implicit: setRectVertices (the inline body) */

// FUNC 0x001D1988 gt2k_menu::Drawable::F_001D1988
/* inline in game/menu/framework/T259_menu_drawable.h; egcs emits this copy: a static
 * member (no `this', the index in $a0), the last inline copy before the
 * typeinfo function, so the first inline member declared (u072's data
 * pass; it was a free function after virtual_2 before). */

/* F_001D1988's pair ({1000.0, 0.0}; the meaning is unknown): .sdata (-G8,
 * 8 bytes), T259's only .sdata item. */
const f32 g_00352718[2] = { 1000.0f, 0.0f }; /* 0x00352718 */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Object). */
// FUNC 0x001D19A0 __tfQ29gt2k_menu8Drawable
extern "C" void *__tfQ29gt2k_menu8Drawable(void)
{
    if (__tiQ29gt2k_menu8Drawable[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu8Drawable, "Q29gt2k_menu8Drawable",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu8Drawable;
}
#endif

// FUNC 0x001D19F0 gt2k_menu::Drawable::~Drawable
/* the implicit destructor (game/menu/framework/T259_menu_drawable.h): ~Object(0) without
 * a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Object (base destructor), operator delete (deleting destructor) */
