/* T410_camera_base.h - CameraBase (translation unit T410, unit u094),
 * 0x0020EE70..0x0020EFE0.
 *
 * The abstract camera.  Slot 1 sets the projection and slot 2 the view (both
 * empty here), slots 3..5 give the eye position (0 here), slots 6 / 7 return
 * the projection / the view as a matrix: computed into the identity on a
 * pushed matrix stack level (pgl_push_matrix), stored, popped.
 *
 * Derived: VariableCameraBase (T397, the race cameras), CameraManager's base
 * RaceCameraBase (T383), MenuCarCamera (T411), SimpleCameraBase (T412),
 * DynamicEnvmapCamera (T277).
 */
#ifndef GAME_T410_CAMERA_BASE_H
#define GAME_T410_CAMERA_BASE_H

#include "game/game.h"

/* vtable 0x0033F5F0 (8 slots).  typeinfo __tf10CameraBase 0x0020EFA0
 * (__rtti_user: no bases; the __user_type_info node __ti10CameraBase
 * 0x00353438 below: the plan had it in T264_raw_window.h).
 * No data: the vptr is at +0x00 with both compilers, and the derived
 * classes' data start at +0x04 (MenuCarCamera::mode).  The constructor is
 * implicit (inline: it only sets the vptr). */
class CameraBase {
public:
    /* The destructor of every derived class ends here (13 callers); egcs adds
     * the flags argument and the operator delete. */
    virtual ~CameraBase();                  /* [0] +0x0C 0x0020EE70 */
    virtual void applyProjection();               /* [1] +0x14 0x0020EEA0 empty (the derived cameras: set the projection) */
    virtual void applyView();               /* [2] +0x1C 0x0020EEA8 empty (the derived cameras: apply the view) */
    /* The eye position.  The names are VariableCameraBase's overrides' (Lupo 2
     * vslot names of 0x001FEBF8 / F0 / E8); an override keeps the name. */
    virtual f32  getPositionX();            /* [3] +0x24 0x0020EEB0 0.0f */
    virtual f32  getPositionY();            /* [4] +0x2C 0x0020EEC0 0.0f */
    virtual f32  getPositionZ();            /* [5] +0x34 0x0020EED0 0.0f */
    virtual void virtual_6(f32 (*m)[4]);    /* [6] +0x3C 0x0020EEE0 m = applyProjection (the projection) as a matrix */
    virtual void virtual_7(f32 (*m)[4]);    /* [7] +0x44 0x0020EF40 m = applyView (the view) as a matrix */

    /* The enhanced engine (Lupo 2, cameras_sound.md C2): the constant the
     * race cameras' texture LOD bias adds (VariableCameraBase::setLodBias,
     * T397, which defines it): 0.0 (Lupo 2's default, L2 gp-0x4A0C) or GT3's
     * -ln 2 (ENH_CAMERA_LOD_BIAS 0).  Lupo 2's setter (linkonce L2 0x002D1038)
     * has no callers; no data, so CameraBase keeps its size. */
    static void setTextureLODBias(f32 bias) { textureLODBias = bias; }
    static f32  getTextureLODBias() { return textureLODBias; }
    static f32  textureLODBias;

    /*  vptr                                   0x00 */
};
EGCS_SIZE(CameraBase, 0x4);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects a hand declaration) */
extern "C" void *__tf10CameraBase(void);    /* 0x0020EFA0 */
/* Its __user_type_info (2 words; __ti13TireModelBase follows at 0x00353440);
 * T277's __tf19DynamicEnvmapCamera and u088 / u092 / u094 use it too. */
extern "C" u32 __ti10CameraBase[2];         /* 0x00353438 gp-0x45B8 */
#endif

#endif /* GAME_T410_CAMERA_BASE_H */
