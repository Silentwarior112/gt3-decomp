/* T277_dynamic_envmap_camera.h - DynamicEnvmapCamera, the camera of the
 * dynamic environment map (translation unit T277, unit u075).
 *
 * T276's envmapUpdate (and RacePS2Base::drawCars, u045) put it at a car's
 * place: setPosition builds `view' (turned down by `pitch' degrees and by 180
 * about y, translated to minus the position and `height' down); slot 2 (the
 * CameraBase "apply the view" slot) multiplies the current matrix by it. */
#ifndef GAME_T277_DYNAMIC_ENVMAP_CAMERA_H
#define GAME_T277_DYNAMIC_ENVMAP_CAMERA_H

#include "game/game.h"
#include "game/camera/T410_camera_base.h"          /* base CameraBase */

class RaceCarState;                         /* T302 (pointers only) */
class CameraManager;                        /* T383 */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T277_dynamic_envmap_camera.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x0033C240 (8 slots: [0] and [2] its own, the rest CameraBase's).
 * typeinfo __tf19DynamicEnvmapCamera 0x001D8A10 (si, base CameraBase;
 * __si_type_info 0x004B2820).  The vptr is CameraBase's (+0x00).  Size 0x70:
 * a local of envmapUpdate (sp+0x00, the next save slot sp+0x70), 16-aligned
 * for the VU0 matrix `view' (whatever CameraBase holds before +0x10 is its
 * data and the alignment padding).  The CameraBase constructor is inline (it
 * only sets the vptr). */
class DynamicEnvmapCamera : public CameraBase {
public:
    DynamicEnvmapCamera();                  /* 0x001D8750 pitch 15, height 0.8 */
    /*  [0] +0x0C 0x001D8A60 the implicit destructor: only CameraBase::~CameraBase, the flags passed on, no vptr store */
    virtual void applyView();               /* [2] +0x1C 0x001D8778 multiplies the current matrix by view */

    void setPosition(f32 x, f32 y, f32 z);  /* 0x001D8798 world = (x, y, z); builds view */
    void followCar(RaceCarState *s, f32 t); /* 0x001D8850 setPosition(the car's place at time t) */
    void setFromManager(CameraManager *cm, f32 x, f32 y, f32 z);  /* 0x001D88D8 cm's view slot 2, then
                                               setPosition (no callers) */
    void followView(CameraManager *cm, f32 t);     /* 0x001D8968 followCar(the car cm's current view
                                               follows) */
    f32  getWorldZ() const;                 /* 0x001D89F8 */
    f32  getWorldY() const;                 /* 0x001D8A00 */
    f32  getWorldX() const;                 /* 0x001D8A08 */

    f32 view[4][4] __attribute__((aligned(16)));   /* 0x10 the view matrix (setPosition) */
    f32 pitch;                              /* 0x50 15.0: degrees down */
    f32 height;                             /* 0x54 0.8: above the car's point */
    f32 worldX;                             /* 0x58 the position (the VisionList cell envmapRender draws) */
    f32 worldY;                             /* 0x5C */
    f32 worldZ;                             /* 0x60 */
};
EGCS_SIZE(DynamicEnvmapCamera, 0x70);

/* The inline accessors 0x001D89F8..0x001D8A08: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline f32 DynamicEnvmapCamera::getWorldX() const { return worldX; }  /* 0x001D8A08 */
inline f32 DynamicEnvmapCamera::getWorldY() const { return worldY; }  /* 0x001D8A00 */
inline f32 DynamicEnvmapCamera::getWorldZ() const { return worldZ; }  /* 0x001D89F8 */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tf19DynamicEnvmapCamera(void);  /* 0x001D8A10 */
#endif

#endif /* GAME_T277_DYNAMIC_ENVMAP_CAMERA_H */
