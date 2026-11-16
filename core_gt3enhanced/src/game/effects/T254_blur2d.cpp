/* T254 - gt2k_menu::Blur2D.  0x001CD6A0..0x001CDA48.
 *
 * A Transitive whose w / h are a blur radius: its draw renders the screen
 * buffer 8 times, offset by the sample pattern g_003372B0 scaled by (w, h),
 * at 1/8 each (additive) into a copy of the buffer, then draws that copy
 * back over the screen.
 *
 * Data (retail .rodata 0x00337178..0x003372F0): Blur2D's vtable, the type
 * names Blur2D, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, Operator (Transitive's header, then Blur2D's), then
 * the sample table g_003372B0: after the type names, so a const of internal
 * linkage (egcs defers those to the end of the file's .rodata). */
#include "game/effects/T254_blur2d.h"               /* (Transitive's operator classes first) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352758 (the screen buffer being drawn) */
#include "engine/pgl/G001_flare.h"                /* gfx_* / gs_* / pgl_* (the GS driver) */
#include "game/menu/framework/render_base_internal.h"

/* The 8 sample offsets of the blur, {x, y} times (w, h). */
static const f32 g_003372B0[8][2] = {       /* 0x003372B0 */
    { -0.875f,  0.875f }, { -0.625f, -0.375f }, { -0.375f,  0.375f }, { -0.125f, -0.875f },
    {  0.125f, -0.125f }, {  0.375f,  0.625f }, {  0.625f, -0.625f }, {  0.875f,  0.125f },
};

namespace gt2k_menu {

/* Slot 2, the draw (index.txt: gt2k_menu::Blur2D::virtual_2). */
// FUNC 0x001CD6A0 gt2k_menu::Blur2D::virtual_2
void Blur2D::virtual_2()
{
    if (w == 0.0f && h == 0.0f)
        return;
    GSBuffer *screen = g_00352758;

    GSBuffer blurred(*screen);              /* the inline copy constructor: GSBuffer::copy */  /* implicit: GSBuffer::copy */
    blurred.psm = PSM_CT32;
    blurred.allocVram();
    blurred.setDrawBuffer();
    screen->setTexture();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_BLEND);
    /* Cs * FIX + Cd: each pass adds 1/8 of the screen */
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_FIX, PGL_SEL_CD), 0.125f);
    pgl_color(0x80FFFFFF);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
    gfx_set_texture_region(PGL_REGION_U, 0, screen->width - 1);
    gfx_set_texture_region(PGL_REGION_V, 0, screen->displayHeight() / 2 - 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    gfx_set_field_mode(PGL_FIELD_FRAME);

    const f32 (*tap)[2] = g_003372B0;
    for (int i = 7; i >= 0; i--, tap++) {
        f32 x0 = (*tap)[0] * w;
        f32 y0 = (*tap)[1] * h;
        f32 x1 = x0 + (f32)blurred.width;
        f32 y1 = y0 + (f32)blurred.displayHeight();
        f32 u1 = (f32)screen->width;
        gs_draw_textured_sprite_strips_f(x0, y0, x1, y1, 0.0f, 0.0f, u1,
                                         (f32)screen->displayHeight() - 0.5f);
    }

    blurred.setTexture();
    screen->setDrawBuffer();
    gfx_disable(PGL_CAP_BLEND);
    gfx_set_field_mode(PGL_FIELD_HALF_CUR);
    f32 screenW = (f32)screen->width;
    f32 screenH = (f32)screen->displayHeight() * 2.0f;
    f32 texW = (f32)blurred.width;
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, texW,
                                     (f32)blurred.displayHeight() - 0.5f);
    blurred.freeVram();
    /* ~GSBuffer (flags 2) */
}

// FUNC 0x001CD978 gt2k_menu::Blur2D::Blur2D
/* inline in game/effects/T254_blur2d.h; egcs emits this copy (no callers):
 * Transitive(), then the vptr. */
/* implicit: Transitive (base constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Transitive). */
// FUNC 0x001CD9B0 __tfQ29gt2k_menu6Blur2D
extern "C" void *__tfQ29gt2k_menu6Blur2D(void)
{
    if (__tiQ29gt2k_menu6Blur2D[0] == 0) {
        __tfQ29gt2k_menu10Transitive();
        __rtti_si(__tiQ29gt2k_menu6Blur2D, "Q29gt2k_menu6Blur2D",
                  (const type_info *)__tiQ29gt2k_menu10Transitive);
    }
    return __tiQ29gt2k_menu6Blur2D;
}
#endif

// FUNC 0x001CDA00 gt2k_menu::Blur2D::~Blur2D
/* implicit in game/effects/T254_blur2d.h; egcs emits this copy (slot [0]):
 * Transitive::~Transitive (flags 0), then Object::operator delete with flags
 * & 1; no vptr store. */
/* implicit: ~Transitive (base destructor), operator delete (deleting destructor) */
