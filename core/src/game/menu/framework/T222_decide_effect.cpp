/* T222 - gt2k_menu::DecideEffect.  0x001C0640..0x001C0A00.
 *
 * The flash over a decided button (Button::virtual_15 / MapButton's): runs
 * the operator script at 0x00334BC0, sets *done = 1 when it ends, and is
 * drawn above everything (z = 100000) as a strip of two quads, colour[0]
 * down the middle and colour[1] at the edges.  With a view set (MapButton),
 * the rectangle is mapped through the zoomed map first. */
/* Retail's .rodata (0x00334A80..0x00334D10): the vtable, the type names
 * DecideEffect, OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * Operator, then the script (0x00334BC0, internal linkage: egcs writes it
 * after the type names). */
#include "engine/pgl/G001_flare.h"                /* gfx_alloc_tail, gfx_enable, gfx_blend_func,
                                               pgl_draw_strip, PrimVertex */
#include "game/menu/framework/T222_decide_effect.h"        /* (T216: Transitive, T218 Color4f, T259 MENU_SCREEN_*) */
#include "game/menu/framework/menu_widgets_internal.h"

namespace gt2k_menu {

/* One instruction of the script (T217's OperatorCode; the unknown bytes 0). */
#define U067_OP(code, arg, arg2, f10, f14, f18, f1C, f20, result) \
    { (code), (arg), (arg2), { 0, 0, 0, 0 }, (f10), (f14), (f18), (f1C), (f20), \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* The decide effect's script: all four vertex colours white, alpha 0; the
 * alpha of colours 0 / 2 (the centre line) up at 8 / s for 0.05 s, of 1 / 3
 * (the edges) up at 4 / s for 0.1 s, then both down at 4 / s for 0.1 s;
 * virtual_6 (*done = 1); the object is killed.  Codes: T217's handler table
 * (18 opSetColors, 22 opFadeColor2: colours arg and arg2 += f10..f1C per
 * second for f20 s, 32 opVirtual6, 6 opRelease).  Static: retail has it
 * after the type names, where egcs writes a const with internal linkage. */
static const OperatorCode g_00334BC0[] = {  /* 0x00334BC0 */
    U067_OP(18, 0, 0, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0),
    U067_OP(22, 0, 2, 0.0f, 0.0f, 0.0f,  8.0f, 0.05f, 1),
    U067_OP(22, 1, 3, 0.0f, 0.0f, 0.0f,  4.0f, 0.1f, 1),
    U067_OP(22, 0, 2, 0.0f, 0.0f, 0.0f, -4.0f, 0.1f, 1),
    U067_OP(22, 1, 3, 0.0f, 0.0f, 0.0f, -4.0f, 0.1f, 1),
    U067_OP(32, 0, 0, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0),
    U067_OP( 6, 0, 0, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1)
};

// FUNC 0x001C0640 gt2k_menu::DecideEffect::DecideEffect
DecideEffect::DecideEffect(int *doneFlag, f32 cx, f32 cy, f32 width, f32 height)
    : Transitive()
{
    x.set(cx);
    y.set(cy);
    w = width;
    h = height;
    done = doneFlag;
    *doneFlag = 0;
    zoom = 0;
    viewX = 0;
    viewY = 0;
    z = 100000.0f;
    start(g_00334BC0);
}

// FUNC 0x001C0730 gt2k_menu::DecideEffect::virtual_1
void DecideEffect::virtual_1()
{
    Transitive::virtual_1();
}

// FUNC 0x001C0750 gt2k_menu::DecideEffect::setView
void DecideEffect::setView(float *viewZoom, float *centreX, float *centreY)
{
    zoom = viewZoom;
    viewX = centreX;
    viewY = centreY;
}

/* The script has ended. */
// FUNC 0x001C0760 gt2k_menu::DecideEffect::virtual_6
void DecideEffect::virtual_6()
{
    *done = 1;
}

/* Draw: six vertices (5 qwords of the primitive buffer) and six packed
 * colours (2 qwords): the left edge, the centre line and the right edge of
 * the rectangle, top and bottom each; then additive blending (Cs * As + Cd)
 * and the strip. */
// FUNC 0x001C0770 gt2k_menu::DecideEffect::virtual_2
void DecideEffect::virtual_2()
{
    PrimVertex *verts = (PrimVertex *)gfx_alloc_tail(5);
    u32 *colors = (u32 *)gfx_alloc_tail(2);
    f32 cx, cy, width, height;

    if (zoom != 0 && viewX != 0 && viewY != 0) {
        f32 scale = *zoom;
        cx = (x.value + (MENU_SCREEN_CX / scale - *viewX)) * scale;
        cy = (y.value + (MENU_SCREEN_CY / scale - *viewY)) * scale;
        width = w * scale;
        height = h * scale;
    } else {
        cx = x.value;
        cy = y.value;
        width = w;
        height = h;
    }

    f32 halfW = width * 0.5f;
    f32 halfH = height * 0.5f;
    verts[0].x = cx - halfW;  verts[0].y = cy - halfH;  verts[0].z = 0;
    verts[1].x = cx - halfW;  verts[1].y = cy + halfH;  verts[1].z = 0;
    verts[2].x = cx;          verts[2].y = cy - halfH;  verts[2].z = 0;
    verts[3].x = cx;          verts[3].y = cy + halfH;  verts[3].z = 0;
    verts[4].x = cx + halfW;  verts[4].y = cy - halfH;  verts[4].z = 0;
    verts[5].x = cx + halfW;  verts[5].y = cy + halfH;  verts[5].z = 0;

    colors[0] = color[1].toRGBA8(1.0f);
    colors[1] = color[1].toRGBA8(1.0f);
    colors[2] = color[0].toRGBA8(1.0f);
    colors[3] = color[0].toRGBA8(1.0f);
    colors[4] = color[1].toRGBA8(1.0f);
    colors[5] = color[1].toRGBA8(1.0f);

    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD),
                   1.0f);           /* 0x48: Cs * As + Cd */
    pgl_draw_strip(verts, 0, colors, 0, 6);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Transitive). */
// FUNC 0x001C0968 __tfQ29gt2k_menu12DecideEffect
extern "C" void *__tfQ29gt2k_menu12DecideEffect(void)
{
    if (__tiQ29gt2k_menu12DecideEffect[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu12DecideEffect, "Q29gt2k_menu12DecideEffect",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu12DecideEffect;
}
#endif

// FUNC 0x001C09B8 gt2k_menu::DecideEffect::~DecideEffect
/* the implicit destructor (game/menu/framework/T222_decide_effect.h): Transitive::~Transitive
 * (flags 0) without a vptr store, then Object::operator delete if flags & 1 */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
