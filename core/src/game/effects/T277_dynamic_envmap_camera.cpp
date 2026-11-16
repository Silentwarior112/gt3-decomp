#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/effects/T277_dynamic_envmap_camera.cpp"
#pragma implementation "src/game/effects/T277_dynamic_envmap_camera.cpp"
#pragma implementation "T277_dynamic_envmap_camera.h"
#endif
/* T277 - DynamicEnvmapCamera (0x001D8750..0x001D8A80).
 *
 * The camera of the environment-map render (T276): put at a car's place
 * (setPosition builds `view': turned down by `pitch' degrees and about y
 * by 180, translated to minus the position and `height' down).
 * virtual_2 (the CameraBase "apply the view" slot) multiplies by it.
 */
/* Retail's .rodata (0x0033C230..0x0033C2D0): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the vtable, then the type names
 * in the reverse order of the classes' definitions: DynamicEnvmapCamera,
 * RaceCameraBase, UnitArena<64>. */
#include <new>
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager (views, current, applyView); RaceCameraBase */
#include "game/effects/T277_dynamic_envmap_camera.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "engine/pgl/G001_flare.h"                /* pgl_* / vu0_* */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x / y / z */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (state) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si */
#endif

/* .rodata start ("bad_alloc"): no code references it. */
// DATA_START 0x0033C230

/* the CameraBase part is inline (vptr only) */
// FUNC 0x001D8750 DynamicEnvmapCamera::DynamicEnvmapCamera
DynamicEnvmapCamera::DynamicEnvmapCamera()
{
    pitch = 15.0f;
    height = 0.8f;
}

// FUNC 0x001D8778 DynamicEnvmapCamera::applyView
void DynamicEnvmapCamera::applyView()
{
    pgl_mult_matrix(&view[0][0]);
}

/* the view is computed on a pushed matrix level (the current one is
 * restored) */
// FUNC 0x001D8798 DynamicEnvmapCamera::setPosition
void DynamicEnvmapCamera::setPosition(f32 x, f32 y, f32 z)
{
    worldX = x;
    worldY = y;
    worldZ = z;
    pgl_push_matrix();
    pgl_get_matrix(&view[0][0]);
    vu0_load_identity_regs();
    vu0_rotate_x(-pitch);
    vu0_rotate_y(180.0f);
    pgl_mult_matrix(&view[0][0]);
    vu0_reset_translation();
    pgl_translate(-worldX, -worldY, -worldZ);
    pgl_translate(0.0f, -height, 0.0f);
    pgl_get_matrix(&view[0][0]);
    pgl_pop_matrix();
}

/* at the car's place at time t */
// FUNC 0x001D8850 DynamicEnvmapCamera::followCar
void DynamicEnvmapCamera::followCar(RaceCarState *s, f32 t)
{
    f32 carX = car_view_x(s, t);
    f32 carY = car_view_y(s, t);
    f32 carZ = car_view_z(s, t);

    setPosition(carX, carY, carZ);
}

/* no callers */
// FUNC 0x001D88D8 DynamicEnvmapCamera::setFromManager
void DynamicEnvmapCamera::setFromManager(CameraManager *cm, f32 x, f32 y, f32 z)
{
    pgl_push_matrix();
    vu0_load_identity_regs();
    cm->applyView();
    setPosition(x, y, z);
    pgl_pop_matrix();
}

/* at the car the current view of cm follows */
// FUNC 0x001D8968 DynamicEnvmapCamera::followView
void DynamicEnvmapCamera::followView(CameraManager *cm, f32 t)
{
    pgl_push_matrix();
    vu0_load_identity_regs();
    cm->applyView();
    followCar(cm->views[cm->current].targetInfo.car[0]->state, t);
    pgl_pop_matrix();
}

// FUNC 0x001D89F8 DynamicEnvmapCamera::getWorldZ(void) const
/* inline in game/effects/T277_dynamic_envmap_camera.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8A00 DynamicEnvmapCamera::getWorldY(void) const
/* inline in game/effects/T277_dynamic_envmap_camera.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001D8A08 DynamicEnvmapCamera::getWorldX(void) const
/* inline in game/effects/T277_dynamic_envmap_camera.h; egcs emits this copy (#pragma implementation) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D8A10 __tf19DynamicEnvmapCamera
extern "C" void *__tf19DynamicEnvmapCamera(void)
{
    if (__ti19DynamicEnvmapCamera[0] == 0) {
        __tf10CameraBase();
        __rtti_si(__ti19DynamicEnvmapCamera, "19DynamicEnvmapCamera",
                  (const type_info *)__ti10CameraBase);
    }
    return __ti19DynamicEnvmapCamera;
}
#endif

// FUNC 0x001D8A60 DynamicEnvmapCamera::~DynamicEnvmapCamera(void)
/* implicit: ~CameraBase; the implicit destructor (game/effects/T277_dynamic_envmap_camera.h): egcs emits
 * this copy, which only passes its flags on to CameraBase::~CameraBase (the
 * operator delete is there) and stores no vptr */
