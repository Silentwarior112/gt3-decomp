/* T412 - SimpleCameraBase, 0x00210008..0x002104E8: a camera with a
 * translation, a rotation (degrees) and a frustum window.  virtual_8 can
 * split the window into n x n tiles (a picture drawn in tiles); virtual_9
 * keeps the angles in [-180, 180].  Nothing in this binary constructs one or
 * calls its non-virtual methods.
 *
 * Retail T412's .rodata (0x0033F6C8) is print's string (written where the
 * header is read), the vtable and "16SimpleCameraBase" only; the
 * "bad_alloc" after it (0x0033F750) is T413's (u095). */
#include "game/camera/camera_base_internal.h"
#include "game/camera/T412_simple_camera_base.h"
#include "engine/pgl/G001_flare.h"                /* pgl_frustum */

/* Caller: setFrustum(l, r, b, t, n, f). */
// FUNC 0x00210008 SimpleCameraBase::setFrustum (setFrustum__16SimpleCameraBasefffffff)
void SimpleCameraBase::setFrustum(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f, f32 d)
{
    left = l;
    right = r;
    bottom = b;
    top = t;
    nearZ = n;
    farZ = f;
    dist = d;
}

// FUNC 0x00210028 SimpleCameraBase::applyProjection
void SimpleCameraBase::applyProjection()
{
    pgl_frustum(left, right, bottom, top, nearZ, farZ, dist);
}

/* tile 0: the whole window (applyProjection, through the vtable); tile k (1..n*n):
 * column (k-1) % n, row (k-1) / n counted from the top. */
// FUNC 0x00210060 SimpleCameraBase::virtual_8
void SimpleCameraBase::virtual_8(int tile, int n)
{
    if (tile == 0) {
        applyProjection();
        return;
    }
    int k = tile - 1;
    int row = k / n;
    int col = k % n;
    f32 l = left;
    f32 t = top;
    f32 tileW = (right - l) / (f32)n;
    f32 tileH = (t - bottom) / (f32)n;
    pgl_frustum(l + tileW * (f32)col, l + tileW * (f32)(col + 1),
                t - tileH * (f32)(row + 1), t - tileH * (f32)row,
                nearZ, farZ, dist);
}

// FUNC 0x00210140 SimpleCameraBase::virtual_9
void SimpleCameraBase::virtual_9()
{
    while (rotation[0] > 180.0f)
        rotation[0] = rotation[0] - 360.0f;
    while (rotation[0] < -180.0f)
        rotation[0] = rotation[0] + 360.0f;
    while (rotation[1] > 180.0f)
        rotation[1] = rotation[1] - 360.0f;
    while (rotation[1] < -180.0f)
        rotation[1] = rotation[1] + 360.0f;
    while (rotation[2] > 180.0f)
        rotation[2] = rotation[2] - 360.0f;
    while (rotation[2] < -180.0f)
        rotation[2] = rotation[2] + 360.0f;
}

/* ---- The constructor and the accessors: inline in
 * game/camera/T412_simple_camera_base.h, emitted after T412's own functions; no
 * callers (Lupo 2 has the same methods as linkonce copies). ---- */

// FUNC 0x002102E0 SimpleCameraBase::print
/* implicit: inline in game/camera/T412_simple_camera_base.h; egcs emits this copy: printf
 * of the translation and the rotation, each float made a double by fptodp
 * (soft-float __extendsfdf2) */

// FUNC 0x002103A0 SimpleCameraBase::getFar
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy: farZ */

// FUNC 0x002103A8 SimpleCameraBase::getNear
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy: nearZ */

// FUNC 0x002103B0 SimpleCameraBase::getTranslationZ
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x002103B8 SimpleCameraBase::getTranslationY
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x002103C0 SimpleCameraBase::getTranslationX
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x002103C8 SimpleCameraBase::setFar
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x002103D0 SimpleCameraBase::setNear
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x002103D8 SimpleCameraBase::setFrustum (setFrustum__16SimpleCameraBaseffffff)
/* implicit: inline in game/camera/T412_simple_camera_base.h; egcs emits this copy: the
 * window at near, setFrustum(l, r, b, t, n, f, n) (0x00210008) */

// FUNC 0x002103F8 SimpleCameraBase::setSpeed
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x00210408 SimpleCameraBase::setRotation
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x00210418 SimpleCameraBase::setTranslation
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy */

// FUNC 0x00210428 SimpleCameraBase::SimpleCameraBase
/* inline in game/camera/T412_simple_camera_base.h; egcs emits this copy: every
 * member 0 (the vptr store after speed[0] is the scheduler's order) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00210478 __tf16SimpleCameraBase
extern "C" void *__tf16SimpleCameraBase(void)
{
    if (__ti16SimpleCameraBase[0] == 0) {
        __tf10CameraBase();
        __rtti_si(__ti16SimpleCameraBase, "16SimpleCameraBase",
                  (const type_info *)__ti10CameraBase);
    }
    return __ti16SimpleCameraBase;
}
#endif

// FUNC 0x002104C8 SimpleCameraBase::~SimpleCameraBase
/* implicit: ~CameraBase; the implicit destructor (game/camera/T412_simple_camera_base.h), the vtable's
 * slot 0: CameraBase::~CameraBase with the same flags, without a vptr
 * store. */
