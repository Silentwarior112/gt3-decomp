/* T396_camera_request.h - CameraRequest (translation unit T396, unit u094),
 * 0x0020EDE8..0x0020EE70.
 *
 * What changed in a camera view this frame (CameraView +0x000, T383): four
 * flags, each with its argument(s).  CameraManager::update (T383) clears the
 * flags of every view at the start of its frame (F_0020EE10); init and the
 * mode handlers (T387..T391) set them when they change the target car, the
 * second car, the camera or the view mode.  ? name (u093: CameraChangeNotice;
 * the field names are u093's, the owner had has[4] / arg0..arg3).
 */
#ifndef GAME_T396_CAMERA_REQUEST_H
#define GAME_T396_CAMERA_REQUEST_H

#include "game/game.h"

/* Not polymorphic.  0x14 bytes of data (CameraView's next known member is at
 * +0x20; +0x14..+0x1F is CameraView's unk_014). */
class CameraRequest {
public:
    u8  targetChanged;                      /* 0x00 F_0020EE28 */
    u8  subChanged;                         /* 0x01 F_0020EE40 */
    u8  cameraChanged;                      /* 0x02 F_0020EE50 */
    u8  modeChanged;                        /* 0x03 F_0020EE60 */
    int target;                             /* 0x04 the car followed */
    int sub;                                /* 0x08 the second car (-1: none) */
    int camera;                             /* 0x0C the camera's type (VariableCameraBase::type) */
    int mode;                               /* 0x10 the view mode (CameraViewKind::kind) */
};

/* The owner's free functions (u093 declared them as CameraChangeNotice
 * methods; the same code). */
void F_0020EDE8(CameraRequest *r);                      /* 0x0020EDE8 no flag, the arguments -1 */
void F_0020EE10(CameraRequest *r);                      /* 0x0020EE10 clears the flags (the arguments stay) */
void F_0020EE28(CameraRequest *r, int target, int sub); /* 0x0020EE28 target, sub; targetChanged = 1 */
void F_0020EE40(CameraRequest *r, int sub);             /* 0x0020EE40 sub; subChanged = 1 */
void F_0020EE50(CameraRequest *r, int camera);          /* 0x0020EE50 camera; cameraChanged = 1 */
void F_0020EE60(CameraRequest *r, int mode);            /* 0x0020EE60 mode; modeChanged = 1 */

#endif /* GAME_T396_CAMERA_REQUEST_H */
