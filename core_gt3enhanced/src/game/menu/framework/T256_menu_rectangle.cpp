/* T256 - gt2k_menu::Rectangle (unit u072, 0x001CE5D8..0x001CF178).
 *
 * A Transitive drawn as one flat quad in its vertex colours (or
 * Drawable::drawBlur when a blur offset is set), then an optional frame of
 * frameSize pixels: bevels (four edge strips in grey, subtracted from or
 * added to the frame buffer) or a drop shadow (an 8-vertex strip below and
 * right of the rectangle).
 *
 * The primitive buffers come from gfx_alloc_tail in quadwords: 4 vertices
 * (12 bytes each) take 3, 8 take 6; 4 colours 1, 8 colours 2; 4 UVs 2.
 */
#include "game/menu/framework/menu_drawables_internal.h"
#include "game/menu/framework/T256_menu_rectangle.h"

using namespace gt2k_menu;

// FUNC 0x001CE5D8 gt2k_menu::Rectangle::shiftByAlign
/* (? name; no callers) the position given was the edge named by align
 * (G001 font_set_align's bits): moves the centre by half the size, right
 * for left alignment, down for top alignment (y grows downwards).
 * Matching: one variable for both masks (retail keeps both in $v1, align
 * stays in $a1) and MotionValue's get / setValue (&x, &y in a register). */
void Rectangle::shiftByAlign(int align)
{
    int a = align & ALIGN_H_MASK;
    switch (a) {
    case ALIGN_LEFT:
        x.setValue(x.get() + w * 0.5f);
        break;
    case ALIGN_RIGHT:
        x.setValue(x.get() - w * 0.5f);
        break;
    }
    a = align & ALIGN_V_MASK;
    switch (a) {
    case ALIGN_TOP:
        y.setValue(y.get() + h * 0.5f);
        break;
    case ALIGN_BOTTOM:
        y.setValue(y.get() - h * 0.5f);
        break;
    }
}

// FUNC 0x001CE6A0 gt2k_menu::Rectangle::drawLeftEdge
/* A 4-vertex strip along the left edge, frameSize wide (outer, inner,
 * outer, inner), in one colour.
 * Matching (the four edges and drawShadow): each vertex's x and y are
 * computed into locals before its stores (retail reads w and h before
 * storing x); stored x, y, z. */
void Rectangle::drawLeftEdge(u32 rgba)
{
    f32 px, py;
    getPosition(&px, &py);
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
    f32 x0 = px - w * 0.5f;
    f32 y0 = py - h * 0.5f;
    v[0].x = x0;
    v[0].y = y0;
    v[0].z = 0;
    f32 x1 = px - w * 0.5f + frameSize;
    f32 y1 = py - h * 0.5f + frameSize;
    v[1].x = x1;
    v[1].y = y1;
    v[1].z = 0;
    f32 x2 = px - w * 0.5f;
    f32 y2 = py + h * 0.5f;
    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    f32 x3 = px - w * 0.5f + frameSize;
    f32 y3 = py + h * 0.5f - frameSize;
    v[3].x = x3;
    v[3].y = y3;
    v[3].z = 0;
    pgl_color(rgba);
    pgl_draw_strip(v, NULL, NULL, NULL, 4);
}

// FUNC 0x001CE7E0 gt2k_menu::Rectangle::drawRightEdge
void Rectangle::drawRightEdge(u32 rgba)
{
    f32 px, py;
    getPosition(&px, &py);
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
    f32 x0 = px + w * 0.5f;
    f32 y0 = py - h * 0.5f;
    v[0].x = x0;
    v[0].y = y0;
    v[0].z = 0;
    f32 x1 = px + w * 0.5f - frameSize;
    f32 y1 = py - h * 0.5f + frameSize;
    v[1].x = x1;
    v[1].y = y1;
    v[1].z = 0;
    f32 x2 = px + w * 0.5f;
    f32 y2 = py + h * 0.5f;
    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    f32 x3 = px + w * 0.5f - frameSize;
    f32 y3 = py + h * 0.5f - frameSize;
    v[3].x = x3;
    v[3].y = y3;
    v[3].z = 0;
    pgl_color(rgba);
    pgl_draw_strip(v, NULL, NULL, NULL, 4);
}

// FUNC 0x001CE920 gt2k_menu::Rectangle::drawTopEdge
/* Along the edge at y - h/2. */
void Rectangle::drawTopEdge(u32 rgba)
{
    f32 px, py;
    getPosition(&px, &py);
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
    f32 x0 = px + w * 0.5f;
    f32 y0 = py - h * 0.5f;
    v[0].x = x0;
    v[0].y = y0;
    v[0].z = 0;
    f32 x1 = px + w * 0.5f - frameSize;
    f32 y1 = py - h * 0.5f + frameSize;
    v[1].x = x1;
    v[1].y = y1;
    v[1].z = 0;
    f32 x2 = px - w * 0.5f;
    f32 y2 = py - h * 0.5f;
    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    f32 x3 = px - w * 0.5f + frameSize;
    f32 y3 = py - h * 0.5f + frameSize;
    v[3].x = x3;
    v[3].y = y3;
    v[3].z = 0;
    pgl_color(rgba);
    pgl_draw_strip(v, NULL, NULL, NULL, 4);
}

// FUNC 0x001CEA60 gt2k_menu::Rectangle::drawBottomEdge
/* Along the edge at y + h/2. */
void Rectangle::drawBottomEdge(u32 rgba)
{
    f32 px, py;
    getPosition(&px, &py);
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
    f32 x0 = px + w * 0.5f;
    f32 y0 = py + h * 0.5f;
    v[0].x = x0;
    v[0].y = y0;
    v[0].z = 0;
    f32 x1 = px + w * 0.5f - frameSize;
    f32 y1 = py + h * 0.5f - frameSize;
    v[1].x = x1;
    v[1].y = y1;
    v[1].z = 0;
    f32 x2 = px - w * 0.5f;
    f32 y2 = py + h * 0.5f;
    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    f32 x3 = px - w * 0.5f + frameSize;
    f32 y3 = py + h * 0.5f - frameSize;
    v[3].x = x3;
    v[3].y = y3;
    v[3].z = 0;
    pgl_color(rgba);
    pgl_draw_strip(v, NULL, NULL, NULL, 4);
}

// FUNC 0x001CEBA0 gt2k_menu::Rectangle::drawRaised
/* Frame type 1: the left and top edges subtracted (darker), the right and
 * bottom ones added (lighter), then the alpha blend again.  With y growing
 * downwards that is a sunken look (? the names of types 1 / 2 may be
 * swapped; see T256_menu_rectangle.h). */
void Rectangle::drawRaised()
{
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(MENU_BLEND_SUB, 1.0f);
    drawLeftEdge(FRAME_GREY);
    drawTopEdge(FRAME_GREY);
    gfx_blend_func(MENU_BLEND_ADD, 1.0f);
    drawRightEdge(FRAME_GREY);
    drawBottomEdge(FRAME_GREY);
    gfx_blend_func(MENU_BLEND_ALPHA, 1.0f);
}

// FUNC 0x001CEC40 gt2k_menu::Rectangle::drawSunken
/* Frame type 2: the same with the two blends swapped (left / top lighter). */
void Rectangle::drawSunken()
{
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(MENU_BLEND_ADD, 1.0f);
    drawLeftEdge(FRAME_GREY);
    drawTopEdge(FRAME_GREY);
    gfx_blend_func(MENU_BLEND_SUB, 1.0f);
    drawRightEdge(FRAME_GREY);
    drawBottomEdge(FRAME_GREY);
    gfx_blend_func(MENU_BLEND_ALPHA, 1.0f);
}

// FUNC 0x001CECE0 gt2k_menu::Rectangle::drawShadow
/* Frame type 3: an 8-vertex strip, frameSize wide, along the edges at
 * y + h/2 and x + w/2; the vertices on the edge (1, 3, 5) are grey with
 * alpha alphaRatio * 128, the outer ones 0 (and the end vertex 7), so the
 * shadow fades outwards.  Subtracted; the subtractive blend is left set. */
void Rectangle::drawShadow()
{
    f32 px, py;

    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(MENU_BLEND_SUB, 1.0f);
    getPosition(&px, &py);
    PrimVertex *v = (PrimVertex *)gfx_alloc_tail(6);
    f32 x0 = px - w * 0.5f;
    f32 y0 = py + h * 0.5f;
    v[0].x = x0;
    v[0].y = y0;
    v[0].z = 0;
    f32 x1 = px - w * 0.5f + frameSize;
    f32 y1 = py + h * 0.5f;
    v[1].x = x1;
    v[1].y = y1;
    v[1].z = 0;
    f32 x2 = px - w * 0.5f;
    f32 y2 = py + h * 0.5f + frameSize;
    v[2].x = x2;
    v[2].y = y2;
    v[2].z = 0;
    f32 x3 = px + w * 0.5f;
    f32 y3 = py + h * 0.5f;
    v[3].x = x3;
    v[3].y = y3;
    v[3].z = 0;
    f32 x4 = px + w * 0.5f + frameSize;
    f32 y4 = py + h * 0.5f + frameSize;
    v[4].x = x4;
    v[4].y = y4;
    v[4].z = 0;
    f32 x5 = px + w * 0.5f;
    f32 y5 = py - h * 0.5f + frameSize;
    v[5].x = x5;
    v[5].y = y5;
    v[5].z = 0;
    f32 x6 = px + w * 0.5f + frameSize;
    f32 y6 = py - h * 0.5f;
    v[6].x = x6;
    v[6].y = y6;
    v[6].z = 0;
    f32 x7 = px + w * 0.5f;
    f32 y7 = py - h * 0.5f;
    v[7].x = x7;
    v[7].y = y7;
    v[7].z = 0;

    u32 *colors = (u32 *)gfx_alloc_tail(2);
    /* the float -> unsigned conversion is libgcc's fptoui (__fixunssfsi) */
    u32 grey = ((u32)(alphaRatio * 128.0f) << 24) | 0x808080; /* implicit: fptoui (float to unsigned) */
    colors[0] = 0;
    colors[1] = grey;
    colors[2] = 0;
    colors[3] = grey;
    colors[4] = 0;
    colors[5] = grey;
    colors[6] = 0;
    colors[7] = 0;
    pgl_draw_strip(v, NULL, colors, NULL, 8);
}

// FUNC 0x001CEF40 gt2k_menu::Rectangle::virtual_2
/* draw: the draw settings, the colours, then one quad (or the blur) and the
 * frame. */
void Rectangle::virtual_2()
{
    if (visible) {
        Drawable::virtual_2();
        u32 *colors = (u32 *)gfx_alloc_tail(1);
        getColors(colors);
        if (blur2DX > 0.0f || blur2DY > 0.0f) {
            drawBlur(colors, NULL);
        } else {
            PrimVertex *v = (PrimVertex *)gfx_alloc_tail(3);
            setRectVertices(v, x.value, y.value, w, h);
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
        drawFrame();
    }
}

// FUNC 0x001CF020 gt2k_menu::Rectangle::drawFrame
/* The empty FRAME_NONE case is in the original: it makes the case tree
 * test 1 first (then < 2, 2, 3). */
void Rectangle::drawFrame()
{
    switch (frameType) {
    case FRAME_NONE:
        break;
    case FRAME_RAISED:
        drawRaised();
        break;
    case FRAME_SUNKEN:
        drawSunken();
        break;
    case FRAME_SHADOW:
        drawShadow();
        break;
    }
}

// FUNC 0x001CF090 gt2k_menu::Rectangle::setFrame
void Rectangle::setFrame(int type, f32 size)
{
    frameType = type;
    frameSize = size;
}

// FUNC 0x001CF0A0 gt2k_menu::Rectangle::Rectangle
/* inline in game/menu/framework/T256_menu_rectangle.h; egcs emits this copy (no callers):
 * Transitive(), the vptr, frameType = frameSize = 0 */
/* implicit: Transitive (base constructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Transitive). */
// FUNC 0x001CF0E0 __tfQ29gt2k_menu9Rectangle
extern "C" void *__tfQ29gt2k_menu9Rectangle(void)
{
    if (__tiQ29gt2k_menu9Rectangle[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu9Rectangle, "Q29gt2k_menu9Rectangle",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu9Rectangle;
}
#endif

// FUNC 0x001CF130 gt2k_menu::Rectangle::~Rectangle
/* the implicit destructor (game/menu/framework/T256_menu_rectangle.h): ~Transitive(0)
 * without a vptr store, Object::operator delete if flags & 1 */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
