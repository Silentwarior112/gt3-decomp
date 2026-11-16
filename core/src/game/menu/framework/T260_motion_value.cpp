/* T260 - MotionValue (unit u072, 0x001D1A38..0x001D1AE0).
 *
 * A spring-driven coordinate stepped at 60 Hz: the force accumulated for
 * the frame is turned into velocity (and cleared), the velocity is damped
 * and integrated into the value.  approach() eases the value towards a
 * target.  Every method returns this.
 *
 * Data: the original T260 has no .rodata (no type name: it saw no class
 * with virtual functions), so only its own header is included (menu_drawables_internal.h
 * brought the menu operator classes' names, unplaceable).
 */
#include "game/menu/framework/T260_motion_value.h"

// FUNC 0x001D1A38 MotionValue::update
/* velocity += force / 60; force = 0; velocity -= velocity * damping / 60;
 * value += velocity / 60 (1/60 = 0.016666668f, gp-0x794C). */
MotionValue *MotionValue::update(f32 damping)
{
    const f32 dt = 0.016666668f;            /* 1 / 60 s */
    f32 vel = velocity + force * dt;
    force = 0.0f;
    vel = vel - vel * damping * dt;
    velocity = vel;
    value = value + vel * dt;
    return this;
}

// FUNC 0x001D1A78 MotionValue::update
/* (no callers) the same with the force scaled by forceScale (1/60 from
 * gp-0x7948, T260's own copy of the literal). */
MotionValue *MotionValue::update(f32 damping, f32 forceScale)
{
    const f32 dt = 0.016666668f;            /* 1 / 60 s */
    f32 pending = force;
    force = 0.0f;
    f32 vel = velocity + pending * (forceScale * dt);
    vel = vel - vel * (damping * dt);
    velocity = vel;
    value = value + vel * dt;
    return this;
}

// FUNC 0x001D1AC0 MotionValue::approach
/* value += (target - value) * rate */
MotionValue *MotionValue::approach(f32 target, f32 rate)
{
    f32 current = value;
    value = current + (target - current) * rate;
    return this;
}
