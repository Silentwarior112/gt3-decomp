/* T005 - screen transitions.  0x00102088..0x00102960.
 *
 * Four global transition objects; start_transition(type, flag) initialises
 * one and makes it the render thread's object.  Each draws over the last
 * frame after TransitionObject::render's 640 x 448 set-up (an orthographic
 * projection, pgl_ortho(0, 640, 448, 0, 0, 100)).  After `delay' frames
 * (180) the fade and comb transitions also draw a small square at
 * (20, 20)-(36, 36) blinking every 32 frames (drawBusyMark).
 *
 * The darkening transitions blend PGL_BLEND_SEL(ZERO, CS, AS, CD): the
 * frame buffer minus the colour times its alpha. */
#include "game/app/app_internal.h"
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's typeinfo functions) */
#include "game/app/T005_transition.h"
#include "game/menu/framework/T246_render_base.h"          /* TransitionType (start_transition's types) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */

/* TransitionObject::init's delay of the busy mark: 3 s. */
enum { BUSY_MARK_DELAY = 180 };

/* index.txt: FadeoutTransitionObject::virtual_4; it is TransitionObject's
 * slot 4 (FadeoutTransitionObject does not override it). */
// FUNC 0x00102088 TransitionObject::init
void TransitionObject::init()
{
    counter = 0;
    delay   = BUSY_MARK_DELAY;
}

// FUNC 0x00102098 TransitionObject::render
void TransitionObject::render(GSBuffer &buf)
{
    gfx_set_field_mode(PGL_FIELD_HALF_CUR);
    buf.setDrawBuffer();
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    pgl_ortho(0.0f, (float)SCREEN_W, (float)SCREEN_H, 0.0f, 0.0f, 100.0f);
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
}

/* After `delay' frames: a 16 x 16 square at (20, 20), light grey (0.9,
 * gp-0x7FF0 0x0034FA00) or black, switching every 32 frames. */
// FUNC 0x00102170 TransitionObject::drawBusyMark
void TransitionObject::drawBusyMark(GSBuffer &buf)
{
    if (delay != 0) {
        delay--;
        return;
    }
    /* The parity is taken after setDrawBuffer: retail computes
     * ++counter >> 5 before the call (counter stored in its delay slot) and
     * the & 1 after it. */
    int dark = ++counter >> 5;
    buf.setDrawBuffer();
    dark &= 1;
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_BLEND);
    if (dark == 0)
        pgl_color3f(0.9f, 0.9f, 0.9f);
    else
        pgl_color3f(0.0f, 0.0f, 0.0f);
    gs_draw_flat_sprite(20, 20, 36, 36);
}

/* index.txt: ComboutTransitionObject::virtual_5; TransitionObject's slot 5
 * (in all five transition vtables). */
// FUNC 0x00102220 TransitionObject::virtual_5
int TransitionObject::virtual_5()
{
    return 1;
}

/* The four objects: file-local (.bss 0x0049B700..0x0049BB50), defined in
 * this order (egcs constructs them in it and lays .bss out in the reverse
 * one); their implicit constructors and destructors are the static
 * initialiser and destructor below. */
static FadeoutTransitionObject      g_0049BB40;    /* 0x0049BB40 */
static DelayFadeoutTransitionObject g_0049BB28;    /* 0x0049BB28 */
static LineoutTransitionObject      g_0049BB10;    /* 0x0049BB10 */
static ComboutTransitionObject      g_0049B700;    /* 0x0049B700 */
/* type: TransitionType (T246): 0 fade-out, 1 delayed fade-out, 2 line-out,
 * 3 comb-out; others do nothing. */
// FUNC 0x00102228 start_transition
void start_transition(int type, int flag)
{
    TransitionObject *t = NULL;
    switch (type) {
    case TRANSITION_FADEOUT:
        t = &g_0049BB40;
        break;
    case TRANSITION_DELAY_FADEOUT:
        t = &g_0049BB28;
        break;
    case TRANSITION_LINEOUT:
        t = &g_0049BB10;
        break;
    case TRANSITION_COMBOUT:
        t = &g_0049B700;
        break;
    }
    if (t != NULL) {
        t->flag = flag;
        t->init();
        render_set_object(t);
    }
}

/* Compiler-generated: egcs's static destructor and initialiser of the four
 * objects above (named after the unit's first function), emitted here,
 * before the inline functions of the classes. */
// FUNC 0x001022D0 T005_static_destroy (_GLOBAL_$D$init__16TransitionObject)
//   (.dtors entry 0x0028F1E4) the implicit destructors: GameObject::~GameObject
//   (obj, 2) of g_0049B700, g_0049BB10, g_0049BB28, g_0049BB40.
/* implicit: ~GameObject */
// FUNC 0x00102328 T005_static_init (_GLOBAL_$I$init__16TransitionObject)
//   (.ctors entry 0x0028F02C) the implicit constructors: GameObject() and the
//   vptr of g_0049BB40, g_0049BB28, g_0049BB10, g_0049B700.
/* implicit: GameObject */

/* The classes' functions egcs emits with their vtables, in this order: the
 * inline ones are defined in game/app/T005_transition.h. */
// FUNC 0x001023C0 ComboutTransitionObject::render
/* implicit: render, drawBusyMark, gfx_enable, gfx_blend_func, gs_draw_flat_sprite, pgl_color */
// FUNC 0x001024B0 ComboutTransitionObject::init
/* implicit: init, range */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * TransitionObject). */
// FUNC 0x00102510 __tf23ComboutTransitionObject
extern "C" void *__tf23ComboutTransitionObject(void)
{
    if (__ti23ComboutTransitionObject[0] == 0) {
        __tf16TransitionObject();
        __rtti_si(__ti23ComboutTransitionObject, "23ComboutTransitionObject",
                  (const type_info *)__ti16TransitionObject);
    }
    return __ti23ComboutTransitionObject;
}
#endif

// FUNC 0x00102560 ComboutTransitionObject::~ComboutTransitionObject
/* implicit in game/app/T005_transition.h; egcs emits this copy (slot [0]):
 * GameObject::~GameObject with the flags (which does the delete); no vptr
 * store. */
/* implicit: ~GameObject (base destructor) */

// FUNC 0x00102580 LineoutTransitionObject::render
/* implicit: render, gfx_disable, gs_draw_flat_sprite, pgl_color */
// FUNC 0x00102628 LineoutTransitionObject::init
/* implicit: init */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * TransitionObject). */
// FUNC 0x00102650 __tf23LineoutTransitionObject
extern "C" void *__tf23LineoutTransitionObject(void)
{
    if (__ti23LineoutTransitionObject[0] == 0) {
        __tf16TransitionObject();
        __rtti_si(__ti23LineoutTransitionObject, "23LineoutTransitionObject",
                  (const type_info *)__ti16TransitionObject);
    }
    return __ti23LineoutTransitionObject;
}
#endif

// FUNC 0x001026A0 LineoutTransitionObject::~LineoutTransitionObject
/* implicit in game/app/T005_transition.h; egcs emits this copy (slot [0]):
 * GameObject::~GameObject with the flags; no vptr store. */
/* implicit: ~GameObject (base destructor) */

// FUNC 0x001026C0 DelayFadeoutTransitionObject::render
/* implicit: render, drawBusyMark, gfx_enable, gfx_blend_func, gs_draw_flat_sprite, pgl_color4f */
// FUNC 0x00102760 DelayFadeoutTransitionObject::init
/* implicit: init */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * TransitionObject). */
// FUNC 0x00102790 __tf28DelayFadeoutTransitionObject
extern "C" void *__tf28DelayFadeoutTransitionObject(void)
{
    if (__ti28DelayFadeoutTransitionObject[0] == 0) {
        __tf16TransitionObject();
        __rtti_si(__ti28DelayFadeoutTransitionObject, "28DelayFadeoutTransitionObject",
                  (const type_info *)__ti16TransitionObject);
    }
    return __ti28DelayFadeoutTransitionObject;
}
#endif

// FUNC 0x001027E0 DelayFadeoutTransitionObject::~DelayFadeoutTransitionObject
/* implicit in game/app/T005_transition.h; egcs emits this copy (slot [0]):
 * GameObject::~GameObject with the flags; no vptr store. */
/* implicit: ~GameObject (base destructor) */

// FUNC 0x00102800 FadeoutTransitionObject::render
/* implicit: render, drawBusyMark, gfx_enable, gfx_blend_func, gs_draw_flat_sprite, pgl_color */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base
 * TransitionObject). */
// FUNC 0x00102878 __tf23FadeoutTransitionObject
extern "C" void *__tf23FadeoutTransitionObject(void)
{
    if (__ti23FadeoutTransitionObject[0] == 0) {
        __tf16TransitionObject();
        __rtti_si(__ti23FadeoutTransitionObject, "23FadeoutTransitionObject",
                  (const type_info *)__ti16TransitionObject);
    }
    return __ti23FadeoutTransitionObject;
}
#endif

// FUNC 0x001028C8 FadeoutTransitionObject::~FadeoutTransitionObject
/* implicit in game/app/T005_transition.h; egcs emits this copy (slot [0]):
 * GameObject::~GameObject with the flags; no vptr store. */
/* implicit: ~GameObject (base destructor) */

// FUNC 0x001028E8 TransitionObject::setFlag
//   inline in the class: the out-of-line copy egcs emits with the vtable.

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base GameObject). */
// FUNC 0x001028F0 __tf16TransitionObject
extern "C" void *__tf16TransitionObject(void)
{
    if (__ti16TransitionObject[0] == 0) {
        __tf10GameObject();
        __rtti_si(__ti16TransitionObject, "16TransitionObject",
                  (const type_info *)__ti10GameObject);
    }
    return __ti16TransitionObject;
}
#endif

// FUNC 0x00102940 TransitionObject::~TransitionObject
/* implicit in game/app/T005_transition.h; egcs emits this copy (slot [0]):
 * GameObject::~GameObject with the flags; no vptr store. */
/* implicit: ~GameObject (base destructor) */
