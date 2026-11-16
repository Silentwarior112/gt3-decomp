/* T388_camera_mode0.h - CameraManager's handler of view modes 0 and 3
 * (translation unit T388, unit u093), 0x0020B8E8..0x0020BE58.
 *
 * The one function is a CameraManager method, declared in
 * T383_camera_manager.h: updateMode0 (0x0020B8E8).
 *
 * CameraSet (u093, u045) is CameraView (T383): the same 0x24C0-byte record
 * of a view (CameraManager +0x300 + 0x24C0 i, built by CameraView's
 * constructor 0x00207768) - u093's notice / selector / targetInfo / planner /
 * current / onboard .. heli are CameraView's request / selector /
 * targetInfo / chooser / camera / onboard .. heli.  One class, so this is a
 * typedef.
 */
#ifndef GAME_T388_CAMERA_MODE0_H
#define GAME_T388_CAMERA_MODE0_H

#include "game/game.h"

class CameraView;                           /* T383 (the definition) */

typedef CameraView CameraSet;

#endif /* GAME_T388_CAMERA_MODE0_H */
