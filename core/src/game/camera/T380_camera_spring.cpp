/* T380: CameraSpring - a damped spring driven by the change of its input, 60
 * steps a second (OnBoardCamera T378, ZoomupCamera T406). */
#include "game/camera/T380_camera_spring.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00206A80 CameraSpring::update
f32 CameraSpring::update(f32 in)
{
    vel += (in - prev + vel * -12.0f) / U091_FPS;
    pos += vel / U091_FPS;
    prev = in;
    return pos;
}

// FUNC 0x00206AE0 CameraSpring::reset
void CameraSpring::reset(f32 pos, f32 prev)
{
    this->prev = prev;
    this->pos = pos;
    vel = 0.0f;
}

// FUNC 0x00206AF0 CameraSpring::clampedPos
f32 CameraSpring::clampedPos()
{
    f32 p = pos;
    if (p < -1.0f)
        p = -1.0f;
    if (1.0f < p)
        p = 1.0f;
    return p;
}
