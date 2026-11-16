/* T396 - CameraRequest (? name), 0x0020EDE8..0x0020EE70: what changed in a
 * camera view this frame (CameraView +0x000).  CameraManager::init and the
 * mode handlers (T383, T387..T391) set the flags; CameraManager::update
 * clears them at the start of its frame (F_0020EE10).
 *
 * The stores are in the source order sched1 turns into retail's: it
 * emits the source's last store first when nothing orders them (the -1
 * stores: the last one, the register's last use, goes first).
 *
 * Retail T396 has no .rodata (no "bad_alloc", no type name): it sees its own
 * record only. */
#include "game/camera/camera_base_internal.h"
#include "game/camera/T396_camera_request.h"

/* Caller: CameraManager::init.  No flag, the arguments -1. */
// FUNC 0x0020EDE8 F_0020EDE8
void F_0020EDE8(CameraRequest *r)
{
    r->targetChanged = 0;
    r->subChanged = 0;
    r->cameraChanged = 0;
    r->modeChanged = 0;
    r->target = -1;
    r->sub = -1;
    r->camera = -1;
    r->mode = -1;
}

/* Caller: CameraManager::update.  Clears the flags (the arguments stay). */
// FUNC 0x0020EE10 F_0020EE10
void F_0020EE10(CameraRequest *r)
{
    r->targetChanged = 0;
    r->subChanged = 0;
    r->cameraChanged = 0;
    r->modeChanged = 0;
}

/* The target car and the second car. */
// FUNC 0x0020EE28 F_0020EE28
void F_0020EE28(CameraRequest *r, int target, int sub)
{
    r->sub = sub;
    r->target = target;
    r->targetChanged = 1;
}

/* The second car alone (the same word as F_0020EE28's sub). */
// FUNC 0x0020EE40 F_0020EE40
void F_0020EE40(CameraRequest *r, int sub)
{
    r->sub = sub;
    r->subChanged = 1;
}

/* The camera's type (VariableCameraBase::type). */
// FUNC 0x0020EE50 F_0020EE50
void F_0020EE50(CameraRequest *r, int camera)
{
    r->camera = camera;
    r->cameraChanged = 1;
}

/* The view mode (CameraViewKind::kind). */
// FUNC 0x0020EE60 F_0020EE60
void F_0020EE60(CameraRequest *r, int mode)
{
    r->mode = mode;
    r->modeChanged = 1;
}
