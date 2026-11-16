/* T383_camera_projection.h - CameraProjection (translation unit T383, unit
 * u092): the projection window of the race cameras.
 *
 * Its own header, included by T383_camera_manager.h where it defined it (its
 * includers see the same declarations in the same order).  VariableCameraBase
 * (T397, u088) needs only this: without the camera manager's header egcs
 * writes no other type name into T397's .rodata ("14RaceCameraBase" came
 * first), and retail T397 has "18VariableCameraBase" right after its vtable
 * (0x0033DFD8, __tf18VariableCameraBase's data reference).
 */
#ifndef GAME_T383_CAMERA_PROJECTION_H
#define GAME_T383_CAMERA_PROJECTION_H

#include "game/game.h"

/* The projection window CameraManager hands to the cameras' slot 11
 * (CameraManager +0x04).  Not polymorphic.  Size 0x1C (the next member is
 * at +0x20).  VariableCameraBase::setProjection calls pgl_frustum(left, right,
 * bottom, top, nearClip, farClip, focal) (u090: f32 v[7]). */
class CameraProjection {
public:
    f32 left;                               /* 0x00 -320 (updateWindow: the screen in the wall) */
    f32 right;                              /* 0x04 320 */
    f32 top;                                /* 0x08 224 */
    f32 bottom;                             /* 0x0C -224 */
    f32 nearClip;                           /* 0x10 0.1 (slot 8) */
    f32 farClip;                            /* 0x14 750 (slot 9) */
    f32 focal;                              /* 0x18 320: pgl_frustum's focal (the distance of the window) */
};
EGCS_SIZE(CameraProjection, 0x1C);

#endif /* GAME_T383_CAMERA_PROJECTION_H */
