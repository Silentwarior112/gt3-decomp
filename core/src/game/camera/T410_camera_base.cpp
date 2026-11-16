/* T410 - CameraBase, 0x0020EE70..0x0020EFE0: the abstract camera.  No data;
 * slot 1 sets the projection and slot 2 the view (both empty here), slots
 * 3..5 give the eye position (0 here), slots 6 / 7 return the projection /
 * the view as a matrix: computed into the identity on a pushed matrix stack
 * level, stored, popped.
 *
 * Retail T410's .rodata (0x0033F5F0) is its vtable and "10CameraBase" only:
 * no "bad_alloc", no other class seen. */
#include "game/camera/camera_base_internal.h"
#include "game/camera/T410_camera_base.h"
#include "engine/pgl/G001_flare.h"                /* pgl_push_matrix / get_matrix / pop_matrix, vu0_load_identity_regs */

/* Callers: the destructors of the derived cameras and of their owners (13). */
// FUNC 0x0020EE70 CameraBase::~CameraBase
CameraBase::~CameraBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* egcs: the vptr store, then operator delete(this) when flags & 1 */
}

// FUNC 0x0020EEA0 CameraBase::applyProjection
void CameraBase::applyProjection()
{
}

// FUNC 0x0020EEA8 CameraBase::applyView
void CameraBase::applyView()
{
}

// FUNC 0x0020EEB0 CameraBase::getPositionX
f32 CameraBase::getPositionX()
{
    return 0.0f;
}

// FUNC 0x0020EEC0 CameraBase::getPositionY
f32 CameraBase::getPositionY()
{
    return 0.0f;
}

// FUNC 0x0020EED0 CameraBase::getPositionZ
f32 CameraBase::getPositionZ()
{
    return 0.0f;
}

// FUNC 0x0020EEE0 CameraBase::virtual_6
void CameraBase::virtual_6(f32 (*m)[4])
{
    pgl_push_matrix();
    vu0_load_identity_regs();
    applyProjection();
    pgl_get_matrix(&m[0][0]);
    pgl_pop_matrix();
}

// FUNC 0x0020EF40 CameraBase::virtual_7
void CameraBase::virtual_7(f32 (*m)[4])
{
    pgl_push_matrix();
    vu0_load_identity_regs();
    applyView();
    pgl_get_matrix(&m[0][0]);
    pgl_pop_matrix();
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (no bases: __rtti_user). */
// FUNC 0x0020EFA0 __tf10CameraBase
extern "C" void *__tf10CameraBase(void)
{
    if (__ti10CameraBase[0] == 0)
        __rtti_user(__ti10CameraBase, "10CameraBase");
    return __ti10CameraBase;
}
#endif
