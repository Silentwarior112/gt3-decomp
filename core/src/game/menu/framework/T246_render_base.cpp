/* T246 - gt2k_menu::RenderBase.  0x001CB3F0..0x001CB9E8.
 *
 * The GameObject a menu controller gives the render thread (attach()): it
 * owns the RootObject of the menu's object tree, updates it (unless paused)
 * and draws it between beginRender (projection, the magnify spring) and
 * endRender (the optional full-screen mask colour).  sleep() parks the
 * calling thread on a Timer of the tree.
 *
 * Data (retail .rodata 0x00336E18..0x00336F48): RenderBase's vtable, then the
 * type names RenderBase, GarageWheelList, List<GarageWheel>, GarageWheel,
 * Cell<GarageWheel>, GarageCarList, List<GarageCar>, GarageCar,
 * Cell<GarageCar>: the original saw the GameStatus's garage before
 * RenderBase (no menu operator classes, no transition objects).  .sdata
 * 0x00352700..0x00352708: g_00352700, g_00352704. */
#include "game/save/T429_game_status.h"          /* (data layout only) the garage classes */
#include "game/app/T002_render_thread.h"        /* GameObject, render_set_object */
#include "game/app/T005_transition_entry.h"     /* start_transition */
#include "game/menu/framework/T246_render_base.h"
#include "game/menu/framework/T252_timer.h"                /* gt2k_menu::Timer */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/menu/framework/T259_menu_drawable.h"        /* MENU_SCREEN_W / MENU_SCREEN_H */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_set_draw_buffer, menu_clip_reset,
                                               menu_apply_transform, menu_draw_message, its .sdata */
#include "engine/pgl/G001_flare.h"                /* gfx_* / gs_* / pgl_* (the GS driver) */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend / intr_resume */
#include "eekernel.h"                       /* GetThreadId, SleepThread, WakeupThread */
#include "game/menu/framework/render_base_internal.h"

/* .sdata (-G8), in this order: the thread sleeping in RenderBase::sleep (-1:
 * none), then a float no function of this unit reads (40.0: T020's
 * F_001051B0 adds it to 448, the row pitch of the arcade car selects; it
 * lies between g_00352700 and T258's "sw_16", T247 .. T255 have no .sdata
 * before T258's: T246's). */
int g_00352700 = -1;                        /* 0x00352700 gp-0x52F0 */
f32 g_00352704 = 40.0f;                     /* 0x00352704 gp-0x52EC */

namespace gt2k_menu {

// FUNC 0x001CB3F0 gt2k_menu::RenderBase::RenderBase
RenderBase::RenderBase()
    : pause(0), mask(0), screenSaver(0), screenSaverFlag(1), renderEnable(1),
      transitionOn(0),
      transitionType(TRANSITION_FADEOUT),   /* 0 (unused while transitionOn is 0) */
      maskColor(0),
      magnifySpring(1.0f),                  /* MotionValue(v): at rest at 1, one store per field */
      magnify(1.0f)
{
    /* implicit: GameObject (base constructor), RootObject (member constructor), ~GameObject (exception cleanup) */
    /* GameObject() and root's RootObject() first (the landing pad runs
     * GameObject::~GameObject).  Mem-initializers in declaration order:
     * exact (the magnifySpring.set(1.0f) body form zeroed the spring twice,
     * MotionValue() first, 78.3%). */
}

/* Makes this the object the render thread draws. */
// FUNC 0x001CB498 gt2k_menu::RenderBase::attach
void RenderBase::attach()
{
    render_set_object(this);
}

/* Leaves the render thread (through the screen transition if one is set),
 * then deletes the object tree. */
// FUNC 0x001CB4B8 gt2k_menu::RenderBase::detach
void RenderBase::detach()
{
    if (transitionOn != 0)
        start_transition(transitionType, 1);
    else
        render_set_object(NULL);
    root.deleteChildren();
}

/* The Timer callback of sleep(): wakes the sleeping thread.  (Interrupts
 * are re-enabled before the wakeup.) */
// FUNC 0x001CB508 render_sleep_done
static void render_sleep_done(Timer *timer, void *render)   /* render: the RenderBase (unused) */
{
    int wasEnabled = intr_suspend();
    int sleeper = g_00352700;
    g_00352700 = -1;
    intr_resume(wasEnabled);
    if (sleeper != -1)
        WakeupThread(sleeper);
}

/* Sleeps the calling thread for `seconds' (60 frames each): a Timer in the
 * object tree wakes it up. */
// FUNC 0x001CB548 gt2k_menu::RenderBase::sleep
void RenderBase::sleep(f32 seconds)
{
    /* Object::operator new, then the inline Timer constructor (Object(), the
     * members; the landing pad calls Object::operator delete). */
    Timer *timer = new Timer(render_sleep_done, this, /* implicit: operator new, operator delete (new-expression), Object (Timer's inline constructor) */
                             (int)(seconds * (f32)MENU_FRAMES_PER_SECOND));
    timer->attachTo(&root);
    g_00352700 = GetThreadId();
    SleepThread();
}

/* Sets up the frame: the drawing buffer, the clear, the projection, the
 * magnify spring (clamped to 1 .. 2; its value and the centre offsets go to
 * T261's .sdata), then the 2D drawing state. */
// FUNC 0x001CB618 gt2k_menu::RenderBase::beginRender
void RenderBase::beginRender(GSBuffer &buf)
{
    gfx_set_field_mode(PGL_FIELD_HALF_CUR);
    buf.setDrawBuffer();
    menu_set_draw_buffer(&buf, &buf);       /* buf2 too (g_00352754 is never read) */
    if (g_00352750 != 0) {
        gs_set_clear_color(g_0035274C);
        gs_clear_buffers(PGL_CLEAR_COLOR);
    }
    menu_clip_reset();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    pgl_ortho(0.0f, MENU_SCREEN_W, MENU_SCREEN_H, 0.0f, 0.0f, 100.0f);

    /* MotionValue's inline members (retail keeps &magnifySpring in $s0) */
    magnifySpring.pull(magnify, 80.0f);
    magnifySpring.update(10.0f);
    magnifySpring.clampMax(2.0f);
    magnifySpring.clampMin(1.0f);
    f32 scale = magnifySpring.get();
    g_00352734 = scale;
    g_00352730 = 320.0f / scale + -320.0f;  /* the drawing offset: the screen centre stays put */
    g_0035272C = 224.0f / scale + -224.0f;
    menu_apply_transform();

    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_color_mask(1, 1, 1, 1);
    gfx_tex_param(PGL_TEXPARAM_TCC, 1);
    gfx_tex_param(PGL_TEXPARAM_TFX, PGL_TFX_MODULATE);
    /* (Cs - Cd) * As + Cd: alpha blending */
    gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
    menu_draw_message();
}

/* With the mask on, covers the screen with maskColor. */
// FUNC 0x001CB818 gt2k_menu::RenderBase::endRender
void RenderBase::endRender(GSBuffer &buf)
{
    if (mask != 0) {
        gfx_disable(PGL_CAP_BLEND);
        gfx_disable(PGL_CAP_ALPHA_TEST);
        pgl_color(maskColor);
        gs_draw_flat_sprite(0, 0, 640, 448);
    }
}

/* GameObject slot 1 (index.txt: gt2k_menu::RenderBase::virtual_1). */
// FUNC 0x001CB870 gt2k_menu::RenderBase::update
void RenderBase::update()
{
    if (pause == 0)
        root.virtual_1();                   /* RootObject::virtual_1, a direct call */
}

/* GameObject slot 2 (index.txt: gt2k_menu::RenderBase::virtual_2). */
// FUNC 0x001CB898 gt2k_menu::RenderBase::render
void RenderBase::render(GSBuffer &buf)
{
    if (g_00352748 == 0 && renderEnable != 0) {
        beginRender(buf);
        root.drawSorted();
        endRender(buf);
    }
}

// FUNC 0x001CB8F8 gt2k_menu::RenderBase::getRootObject
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB900 gt2k_menu::RenderBase::setRenderEnable
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB908 gt2k_menu::RenderBase::setTransition
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB918 gt2k_menu::RenderBase::setScreenSaver
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB928 gt2k_menu::RenderBase::setMagnify
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB930 gt2k_menu::RenderBase::setMaskColor
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB938 gt2k_menu::RenderBase::setMask
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB940 gt2k_menu::RenderBase::getPause
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

// FUNC 0x001CB948 gt2k_menu::RenderBase::setPause
/* inline in game/menu/framework/T246_render_base.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base GameObject). */
// FUNC 0x001CB950 __tfQ29gt2k_menu10RenderBase
extern "C" void *__tfQ29gt2k_menu10RenderBase(void)
{
    if (__tiQ29gt2k_menu10RenderBase[0] == 0) {
        __tf10GameObject();
        __rtti_si(__tiQ29gt2k_menu10RenderBase, "Q29gt2k_menu10RenderBase",
                  (const type_info *)__ti10GameObject);
    }
    return __tiQ29gt2k_menu10RenderBase;
}
#endif

// FUNC 0x001CB9A0 gt2k_menu::RenderBase::~RenderBase
/* implicit in game/menu/framework/T246_render_base.h; egcs emits this copy (slot [0]):
 * root's ~RootObject (flags 2), then GameObject::~GameObject with the flags
 * (which does the delete); no vptr store. */
/* implicit: ~RootObject (member destructor), ~GameObject (base destructor) */
