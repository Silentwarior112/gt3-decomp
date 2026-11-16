/* T381: CameraSway (? name; no header of its own: game/camera/T379_camera_rot_damper.h)
 * - every 3rd frame, with a probability that grows with `amount', a new random
 * target in -102.4..76.8 (128 x -0.8..0.6) is chosen; the value chases it with
 * a spring.  Read as value / 128.  CameraRotDamper's random gain, and
 * DesignerCamera's. */
#include "game/camera/T379_camera_rot_damper.h"
#include "game/camera/camera_motion_internal.h"

// FUNC 0x00206CC0 CameraSway::init
void CameraSway::init()
{
    u32 seed = (s32)gfx_perf_counter0();
    rng.seed = seed;
    gain = 0;
    tick = 0;
    target = 0.0f;
    value = 0.0f;
    vel = 0.0f;
}

// FUNC 0x00206D08 CameraSway::pickTarget
void CameraSway::pickTarget(f32 amount)
{
    f32 r;

    rng.next();
    r = rng.next01();
    if (tick < 2) {
        tick = tick + 1;
        return;
    }
    tick = 0;
    if (amount < 0.0f)
        amount = -amount;
    if (1.0f < amount)
        amount = 1.0f;
    /* a new target with probability 0.0067 + 0.1575 amount */
    amount = amount * -0.15749991f + 0.9933333f;
    if (amount < r) {
        rng.next();
        /* computed and compared but never used: the inner if's store is
         * dead, so only its compare is left in the code (no branch); the
         * target below takes a fresh number without the next() step */
        r = ((rng.next01() * 0.8f + 0.2f) * 256.0f - 128.0f) * 0.8f;
        if (120.32f <= target) {
            if (0.0f < r)
                r = 0.0f;
        }
        r = rng.next01();
        gain = 0;
        target = (r * 0.8f + 0.2f) * 256.0f - 128.0f;
    }
}

// FUNC 0x00206E50 CameraSway::get
f32 CameraSway::get()
{
    return value * 0.0078125f;              /* 1 / 128 */
}

// FUNC 0x00206E68 CameraSway::update
//   The spring's stiffness is 0 the frame a target is picked, then 60.
void CameraSway::update(f32 amount)
{
    pickTarget(amount);
    if (gain <= 0)
        gain++;
    vel += ((f32)gain * U091_FPS * (target - value) + vel * -10.0f) / U091_FPS;
    value += vel / U091_FPS;
}
