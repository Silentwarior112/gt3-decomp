/* T460 - PrizeBase, PrizeModelSetContext and the prize camera.
 *
 * PrizeBase holds a loaded .prm model (T459) and its pose (the context):
 * load / setModel, update (one animation step when updateFlag is 1), draw
 * (the root matrix, the lights, a flattened shadow pass per light 0.1
 * lower each, then the model; an optional callback object before each
 * pass).  Users: MiniPrize / LicensePrize (T145 / T146), the trophy items
 * of the menus (T058).
 */
/* .rodata (0x00341408..0x00341486): the two vtables, then the type names
 * PrizeBase, CallBackWhenPrizeRendering, PrizeModelSetContext (reverse
 * definition order).  The function order at the end of the unit (frame
 * info): PrizeBase's inline members, __tf9PrizeBase, PrizeModelSetContext's
 * destructor and __tf, prize_matrix_set_diagonal last (T459_prize_model.h). */
#include "game/menu/gtmode/prize_model_internal.h"
#include "cxxrt.h"                          /* __rtti_user (GCC 14's typeinfo functions) */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, vu0_scale */
#include "engine/media/G002_mpeg_frameq.h"          /* tanf */
#include "engine/gfx/T451_model_set.h"            /* modelset_reset_anims, modelset_draw_part */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230 */
#include "engine/pdistd/T532_heap.h"                 /* free */
#include "game/menu/gtmode/T460_prize_base.h"           /* own (T459_prize_model.h) */

/* PrizeBase::draw: the shadow of light i lies on the plane y = -0.1 * (i + 1)
 * (each one a little lower, ? so that they do not fight). */
#define PRIZE_SHADOW_STEP (-0.1f)

// FUNC 0x00229C80 PrizeBase::PrizeBase
//   (Callers license_get_screen, trophy_get, MiniPrize::MiniPrize.)  The
//   context's inline constructor: both matrices diagonal 1
//   (prize_matrix_set_diagonal), time 0.
PrizeBase::PrizeBase() : model(NULL)
{
    /* implicit: the context's inline constructor: prize_matrix_set_diagonal
     * twice (PrizeMatrix's inline constructor), time = 0 */
    dispFlag = 0;
    updateFlag = 0;
    keep = 0;
}

// FUNC 0x00229D00 PrizeBase::~PrizeBase
//   Calls unload directly (not through the vtable), then the context's
//   inline destructor (its vptr store only).
PrizeBase::~PrizeBase()
{
    PrizeBase::unload();
}

// FUNC 0x00229D58 PrizeBase::setModel
//   ? name (no caller).  Takes a model already in memory (kept: not
//   freed by unload), relocates it and resets the pose.
void PrizeBase::setModel(PrizeModel *m)
{
    keep = 1;
    model = m;
    prize_model_relocate(m);
    modelset_reset_anims(model->model);
    prize_model_reset(model, &context);
}

// FUNC 0x00229DA8 PrizeBase::load
//   Unloads (virtual), reads the file into buf (size bytes) or into a new
//   buffer when buf is NULL, relocates it and resets the pose.  keep = 1:
//   unload will not free the model.  keep is an int (u048: retail stores
//   the incoming register without masking it).
void PrizeBase::load(const char *path, void *buf, int size, int keep)
{
    unload();
    this->keep = keep;
    if (buf != NULL) {
        file_read_blocking(path, buf, size, 1, &vol_device_object);
        model = (PrizeModel *)buf;
    } else {
        model = (PrizeModel *)file_read_blocking(path, NULL, 0, 1, &vol_device_object);
    }
    prize_model_relocate(model);
    modelset_reset_anims(model->model);
    prize_model_reset(model, &context);
}

// FUNC 0x00229E78 PrizeBase::unload
//   Frees the model unless kept.
void PrizeBase::unload()
{
    if (keep != 1) {
        free(model);
        model = NULL;
    }
}

// FUNC 0x00229EB8 PrizeBase::update
//   step() (virtual) when there is a model and updateFlag is 1.
void PrizeBase::update()
{
    if (model != NULL) {
        if (updateFlag == 1)
            step();
    }
}

// FUNC 0x00229F00 PrizeBase::step
void PrizeBase::step()
{
    prize_model_step(model, &context);
}

// FUNC 0x00229F28 PrizeBase::restart
//   ? name.  The pose and the embedded model's animations back to the
//   start.
void PrizeBase::restart()
{
    if (model != NULL) {
        prize_model_reset(model, &context);
        modelset_reset_anims(model->model);
    }
}

// FUNC 0x00229F68 PrizeBase::show
void PrizeBase::show()
{
    dispFlag = 1;
    updateFlag = 1;
    restart();
}

// FUNC 0x00229F90 PrizeBase::hide
void PrizeBase::hide()
{
    dispFlag = 0;
    updateFlag = 0;
}

// FUNC 0x00229FA0 PrizeBase::draw
//   The model must be there (not checked).  root, cb->afterRoot(); the
//   lighting on and the lights; per light: its shadow matrix at y =
//   -0.1 * (i + 1), body, cb->beforePass(), the shadow part; then body,
//   cb->beforePass(), the model part; the lighting off.
void PrizeBase::draw(CallBackWhenPrizeRendering *cb)
{
    int light;

    context.applyRoot();
    if (cb != NULL)
        cb->afterRoot();
    gfx_enable(PGL_CAP_CPU_LIGHTING);
    prize_model_lights(model);
    for (light = 0; light < model->lightCount; light++) {
        f32 y = (f32)(light + 1) * PRIZE_SHADOW_STEP;

        pgl_push_matrix();
        if (prize_model_shadow(model, light, 0, y) != 0) {
            context.applyBody();
            if (cb != NULL)
                cb->beforePass();
            modelset_draw_part(model->model, PRIZE_PART_SHADOW);
        }
        pgl_pop_matrix();
    }
    pgl_push_matrix();
    context.applyBody();
    if (cb != NULL)
        cb->beforePass();
    modelset_draw_part(model->model, PRIZE_PART_MODEL);
    pgl_pop_matrix();
    gfx_disable(PGL_CAP_CPU_LIGHTING);
}

// FUNC 0x0022A110 F_0022A110
//   ? (no caller): pi / 3, 60 degrees (? the prize camera's field of view;
//   prize_projection uses tan(30 degrees)).
f32 F_0022A110(void)
{
    return 1.0471976f;
}

// FUNC 0x0022A120 prize_projection
//   ? name.  The prize camera: F_00257EF0(1.098) (? the pixel aspect), x
//   scaled by 0.75 in the 16:9 mode, a frustum of half width tan(30 deg)
//   and half height 0.7 tan(30 deg) at near 0.1 (far 300, focal 0.1),
//   shifted by (dx, dy).
void prize_projection(f32 dx, f32 dy)
{
    f32 right = tanf(0.5235988f);               /* tan(30 deg) */
    f32 left = -right;
    f32 aspect = 0.7f;
    f32 near = 0.1f;
    RenderContext *rc = &g_002F4230;
    f32 bottom, top;

    F_00257EF0(1.098f);
    bottom = left * aspect;
    top = -bottom;
    if (rc->wide != 0)
        vu0_scale(0.75f, 1.0f, 1.0f);
    pgl_frustum(left * near + dx, right * near + dx, bottom * near + dy, top * near + dy,
                near, 300.0f, near);
}

// FUNC 0x0022A200 prize_projection_eye
//   ? name.  The frustum shifted for an eye moved by (dx, dy) at the focus
//   distance (the parallax of T058's trophy ring).
void prize_projection_eye(f32 dx, f32 dy, f32 focus)
{
    f32 shift = -0.1f / focus;                  /* near / focus */

    prize_projection(shift * dx, shift * dy);
}

// FUNC 0x0022A230 prize_eye_translate
//   ? name.  The matching eye translation.
void prize_eye_translate(f32 dx, f32 dy)
{
    pgl_translate(-dx, -dy, 0.0f);
}

// FUNC 0x0022A258 PrizeBase::getEyeDelta
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A270 PrizeBase::getFocus
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A288 PrizeBase::getSize
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A2A0 PrizeBase::setUpdateFlag
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A2A8 PrizeBase::setDispFlag
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A2B0 PrizeBase::getUpdateFlag
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A2B8 PrizeBase::getDispFlag
/* inline in game/menu/gtmode/T460_prize_base.h; egcs emits this copy */

// FUNC 0x0022A2C0 PrizeBase::applyBody
/* implicit: inline in game/menu/gtmode/T460_prize_base.h (? name, no caller): context.applyBody();
 * egcs emits this copy */

// FUNC 0x0022A2E0 PrizeBase::applyRoot
/* implicit: inline in game/menu/gtmode/T460_prize_base.h (? name, no caller): context.applyRoot();
 * egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x0022A300 __tf9PrizeBase
//   Builds the __user_type_info at gp-0x4590 on the first call (caller:
//   __tf9MiniPrize).
extern "C" void *__tf9PrizeBase(void)
{
    if (__ti9PrizeBase[0] == 0)
        __rtti_user(__ti9PrizeBase, "9PrizeBase");
    return __ti9PrizeBase;
}
#endif

// FUNC 0x0022A340 PrizeModelSetContext::~PrizeModelSetContext
/* inline in game/menu/gtmode/T459_prize_model.h; egcs emits this copy (the vtable's) */
/* implicit: operator delete (the deleting destructor, flag bit 0) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0022A370 __tf20PrizeModelSetContext
//   (no caller: only in the vtable)
extern "C" void *__tf20PrizeModelSetContext(void)
{
    if (__ti20PrizeModelSetContext[0] == 0)
        __rtti_user(__ti20PrizeModelSetContext, "20PrizeModelSetContext");
    return __ti20PrizeModelSetContext;
}
#endif

// FUNC 0x0022A3B0 prize_matrix_set_diagonal
/* inline in game/menu/gtmode/T459_prize_model.h; egcs emits this copy (called by
 * PrizeMatrix's inline constructor, twice in PrizeBase::PrizeBase), last:
 * its body is saved before PrizeModelSetContext is complete (the matrix
 * class comes first), so it is written after the context's destructor and
 * typeinfo function (u103's data pass) */
