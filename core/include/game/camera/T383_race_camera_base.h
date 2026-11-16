/* T383_race_camera_base.h - RaceCameraBase, the interface of the race camera
 * (the base of CameraManager, T383_camera_manager.h; translation unit T383,
 * unit u092: its typeinfo function 0x0020A290).
 *
 * Its own header (T383_camera_manager.h includes it where it defined the
 * class): the race display's header includes it too.  RaceCameraBase has
 * only pure virtual functions, so no key function: egcs writes its type name
 * "14RaceCameraBase" into every object that sees it, and retail has it in
 * the objects of T154, T155, T157 and T160..T164, right after the display
 * parts' names and UnitArena<64>'s (T154_race_display.h, the T154 merge). */
#ifndef GAME_T383_RACE_CAMERA_BASE_H
#define GAME_T383_RACE_CAMERA_BASE_H

#include "game/game.h"
#include "game/camera/T410_camera_base.h"          /* base CameraBase */

class RaceOrganization;                     /* T125 (pointers only) */

/* typeinfo __tf14RaceCameraBase 0x0020A290 (T383; __si_type_info 0x004B1200,
 * base CameraBase).  No vtable of its own in the binary: CameraManager's
 * constructor stores only its own vptr and its destructor goes on to
 * CameraBase::~CameraBase (implicit constructor and destructor).  It adds the
 * slots 8..14 (declared pure: the owner's reading; the names are
 * CameraManager's). */
class RaceCameraBase : public CameraBase {
public:
    virtual void setNearClip(f32 z) = 0;    /* [8] +0x4C */
    virtual void setFarClip(f32 z) = 0;     /* [9] +0x54 */
    virtual f32  virtual_10() = 0;          /* [10] +0x5C */
    virtual int  virtual_11() = 0;          /* [11] +0x64 */
    virtual int  virtual_12(RaceOrganization *org, int car) = 0;   /* [12] +0x6C */
    virtual int  isDrawBackMirror() = 0;    /* [13] +0x74 */
    virtual int  getCameraKind() = 0;       /* [14] +0x7C */
};
EGCS_SIZE(RaceCameraBase, 0x4);

#endif /* GAME_T383_RACE_CAMERA_BASE_H */
