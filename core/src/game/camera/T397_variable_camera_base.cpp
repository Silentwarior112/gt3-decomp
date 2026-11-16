#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/camera/T397_variable_camera_base.cpp"
#pragma implementation "src/game/camera/T397_variable_camera_base.cpp"
#pragma implementation "T397_variable_camera_base.h"
#endif
/* T397 - VariableCameraBase, the base class of the race cameras.
 *
 * The camera values (gamma, zoom, position, rotate, target length) keep
 * HISTORY = 2 entries: [0] this frame, [1] the previous frame.  updateFrame
 * shifts them, runs the camera's update (slot 12), and takes the eye
 * position from the inverse of the view matrix.
 *
 * #pragma implementation (egcs only, first lines): the pair of the header's
 * #pragma interface, so the copies of the inline accessors and the vtable
 * are emitted here (the #line names this file, see T530).
 */
/* Retail T397's .rodata (0x0033DEE0): <new>'s "bad_alloc" (nothing references
 * it: DATA_START), the literal, the vtable, then the type names
 * VariableCameraBase and UnitArena<64> (reverse definition order: T153
 * before T397's header). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_box_internal.h"
#include "game/camera/T397_variable_camera_base.h"
// DATA_START 0x0033DEE0

// FUNC 0x001FE768 VariableCameraBase::VariableCameraBase
//   (CameraBase's inline constructor only sets the vptr.)  trueType and
//   subCounter are not initialised.  Callers: the derived constructors.
VariableCameraBase::VariableCameraBase()
{
    for (int i = 0; i < HISTORY; i++) {
        position[i][0] = 0.0f;
        position[i][1] = 0.0f;
        position[i][2] = 0.0f;
        position[i][3] = 1.0f;
        gamma[i] = 1.0f;
        zoom[i] = 1.0f;
        rotate[i][0] = 0.0f;
        rotate[i][1] = 0.0f;
        rotate[i][2] = 0.0f;
        rotate[i][3] = 0.0f;
        targetLength[i] = 1.0f;
    }
    doppler = 0;
    type = 0;
    effect = 0;
    counter = 0;
    bankForcibly = 0.0f;
}

// FUNC 0x001FE800 VariableCameraBase::~VariableCameraBase
//   slot 0: the vptr, then CameraBase::~CameraBase with the same flags
//   (it does the operator delete).
VariableCameraBase::~VariableCameraBase()
{
    /* implicit: ~CameraBase (the base destructor) */
}

// FUNC 0x001FE828 VariableCameraBase::update
//   slot 12, the camera update (the derived cameras override it; the
//   arguments are updateFrame's).
int VariableCameraBase::update(RaceOrganization *org, RaceInput *input,
                                   CameraTargetInfo *target, CarProgress *progress)
{
    printf("---- not defined  camera update\n");
    return 1;
}

// FUNC 0x001FE850 VariableCameraBase::updateFrame
//   ? name (the CameraManager's per-frame call).  The values move to the
//   history, doppler is set, the giveup counters count down, the update
//   (slot 12, with the arguments passed straight through) runs, then, with
//   interrupts off, the eye position = inverse(matrix) * (0, 0, 0, 1) on a
//   pushed matrix stack level.  When the update returns 0 the history is
//   overwritten with the new values (no interpolation from the previous
//   frame).  Callers: CameraManager::update, ::updateDemoCameras,
//   ::updateDesigner (T383).
//   One `i' for both loops: its register ($t4) is the first loop's in the
//   second one too (a variable per loop: $t0 there).
void VariableCameraBase::updateFrame(RaceOrganization *org, RaceInput *input,
                                     CameraTargetInfo *target, CarProgress *progress)
{
    int i;

    for (i = HISTORY - 1; i > 0; i--) {
        position[i] = position[i - 1];          /* 2 x 64-bit copies */
        gamma[i] = gamma[i - 1];
        zoom[i] = zoom[i - 1];
        rotate[i] = rotate[i - 1];
        targetLength[i] = targetLength[i - 1];
    }
    doppler = 1;
    if (giveupMode > 0)
        giveupMode = giveupMode - 1;
    if (giveupTime > 0)
        giveupTime = giveupTime - 1;

    int keepHistory = update(org, input, target, progress);

    int intrWasOn = intr_suspend();
    pgl_push_matrix();
    pgl_load_matrix(&matrix[0][0]);
    vu0_invert_rigid();
    position[0].x = 0.0f;
    position[0].y = 0.0f;
    position[0].z = 0.0f;
    position[0].w = 1.0f;
    vu0_apply_matrix(position[0], position[0]);
    pgl_pop_matrix();
    if (intrWasOn != 0)
        __asm__ volatile("ei");

    if (keepHistory == 0) {
        for (i = HISTORY - 1; i > 0; i--) {
            position[i] = position[0];
            gamma[i] = gamma[0];
            zoom[i] = zoom[0];
            rotate[i] = rotate[0];
            targetLength[i] = targetLength[0];
        }
    }
}

// FUNC 0x001FEA28 VariableCameraBase::virtual_9
int VariableCameraBase::virtual_9()
{
    return 3;
}

// FUNC 0x001FEA30 VariableCameraBase::getEffect
//   the effect (sign-extended)
int VariableCameraBase::getEffect()
{
    return effect;
}

// FUNC 0x001FEA38 VariableCameraBase::setProjection
//   slot 11: the projection (pgl_frustum: left, right, bottom, top, near,
//   far, focal), then the texture LOD bias for the default zoom 1.428148.
void VariableCameraBase::setProjection(const CameraProjection *proj)
{
    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top, proj->nearClip,
                proj->farClip, proj->focal);
    setLodBias(CAMERA_DEFAULT_ZOOM);        /* 0x3FB6CD8E */
}

// FUNC 0x001FEA88 VariableCameraBase::applyView
//   slot 2: the view matrix onto the current matrix.
void VariableCameraBase::applyView()
{
    pgl_mult_matrix(&matrix[0][0]);
}

// FUNC 0x001FEAA8 VariableCameraBase::setLodBias
//   The texture LOD bias for a zoom (1 / tan(half the field of view)):
//   gfx_tex_param(PGL_TEXPARAM_LOD_BIAS, (int)((-log2(zoom) - ln 2) * 256)),
//   the GsContext's lodBias that gfx_set_texture_regs adds to TEX1.K: a
//   narrower view (a larger zoom) selects the finer mip levels.  Callers:
//   every camera's setProjection (9).
void VariableCameraBase::setLodBias(f32 zoom)
{
    gfx_tex_param(PGL_TEXPARAM_LOD_BIAS, (int)((-logf(zoom) * INV_LN2 + NEG_LN2) * 256.0f));
}

// FUNC 0x001FEAF8 VariableCameraBase::disableCrossOver(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB00 VariableCameraBase::isRangeOver(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB08 VariableCameraBase::getBankForcibly(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB10 VariableCameraBase::setBankForcibly(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB18 VariableCameraBase::getGiveupTime(void) const
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB20 VariableCameraBase::setGiveupTime
//   ? name: no symbol; the setter of getGiveupTime's field.
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB28 VariableCameraBase::getGiveupMode(void) const
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB30 VariableCameraBase::getGiveupState
//   giveupState (no callers).
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB38 VariableCameraBase::setGiveup
//   giveupState = state, giveupMode = mode (no callers).
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB48 VariableCameraBase::getKeepTime(void) const
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB50 VariableCameraBase::getTargetLength(int) (getTargetLength__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB60 VariableCameraBase::getTargetLength(void) (getTargetLength__18VariableCameraBase)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB68 VariableCameraBase::getMatrix(void) const
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB70 VariableCameraBase::getRotateZ(int) (getRotateZ__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB80 VariableCameraBase::getRotateY(int) (getRotateY__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEB90 VariableCameraBase::getRotateX(int) (getRotateX__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBA0 VariableCameraBase::getRotateZ(void) (getRotateZ__18VariableCameraBase)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBA8 VariableCameraBase::getRotateY(void) (getRotateY__18VariableCameraBase)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBB0 VariableCameraBase::getRotateX(void) (getRotateX__18VariableCameraBase)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBB8 VariableCameraBase::getPositionZ(int) (getPositionZ__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBC8 VariableCameraBase::getPositionY(int) (getPositionY__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBD8 VariableCameraBase::getPositionX(int) (getPositionX__18VariableCameraBasei)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBE8 VariableCameraBase::getPositionZ(void) (getPositionZ__18VariableCameraBase)
//   slot 5
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBF0 VariableCameraBase::getPositionY(void) (getPositionY__18VariableCameraBase)
//   slot 4
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEBF8 VariableCameraBase::getPositionX(void) (getPositionX__18VariableCameraBase)
//   slot 3
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC00 VariableCameraBase::getGamma(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC10 VariableCameraBase::setGamma(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC18 VariableCameraBase::getDoppler(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC20 VariableCameraBase::setDoppler(bool)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC28 VariableCameraBase::setEffect(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC30 VariableCameraBase::getTrueType(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC38 VariableCameraBase::getType(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC40 VariableCameraBase::setTrueType(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC48 VariableCameraBase::setType(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy */

// FUNC 0x001FEC58 VariableCameraBase::setTargetLength(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC60 VariableCameraBase::setPositionZ(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC68 VariableCameraBase::setPositionY(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC70 VariableCameraBase::setPositionX(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC78 VariableCameraBase::setRotateZ(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC80 VariableCameraBase::setRotateY(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC88 VariableCameraBase::setRotateX(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC90 VariableCameraBase::setZoom(float)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FEC98 VariableCameraBase::getZoom(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECA8 VariableCameraBase::updateSubCounter(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECB8 VariableCameraBase::updateCounter(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECC8 VariableCameraBase::getSubCounter(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECD0 VariableCameraBase::getCounter(void)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECD8 VariableCameraBase::setSubCounter(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001FECE0 VariableCameraBase::setCounter(int)
/* inline in game/camera/T397_variable_camera_base.h; egcs emits this copy (#pragma implementation) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001FECE8 __tf18VariableCameraBase
extern "C" void *__tf18VariableCameraBase(void)
{
    if (__ti18VariableCameraBase[0] == 0) {
        __tf10CameraBase();
        __rtti_si(__ti18VariableCameraBase, "18VariableCameraBase",
                  (const type_info *)__ti10CameraBase);
    }
    return __ti18VariableCameraBase;
}
#endif
