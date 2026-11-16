/* T260_motion_value.h - MotionValue (translation unit T260, unit u072),
 * 0x001D1A38..0x001D1AE0.
 *
 * A spring-driven coordinate stepped at 60 Hz: the force accumulated during
 * a frame becomes velocity (and is cleared), the velocity is damped and
 * integrated into the value.  The centre of every gt2k_menu::Drawable (T259)
 * is two of them; MenuCarCamera (T411) is built from sixteen.  Every method
 * returns this. */
#ifndef GAME_T260_MOTION_VALUE_H
#define GAME_T260_MOTION_VALUE_H

#include "game/game.h"

/* Not polymorphic.  Size 0xC (Drawable::x / y at +0x20 / +0x2C).
 *
 * The constructor is inline (no out-of-line copy) and zeroes the three
 * values (u020 / u064 / u094's): KeyConfigItem's constructor (T101,
 * 0x0012ACC8) has the egcs vector-construction loop of its scroll[10]
 * (stride 0xC, three zero stores), and Drawable() (T259) zeroes x and y as
 * members, before its Color4f array is constructed; egcs puts this
 * constructor's stores in the listings' order (+0, +8, +4), set(0) does
 * not.  An array of MotionValues is therefore built by a loop. */
class MotionValue {
public:
    MotionValue() : force(0), velocity(0), value(0) {}
    /* (u063) At rest at v from the start: T207's MapButtonList() builds its
     * view and zoom so (mem-initializers; retail has one store per field,
     * no zeroing first). */
    MotionValue(f32 v) : force(0), velocity(0), value(v) {}
    /* Inline in the original (no out-of-line copies).  set: at rest at v
     * (the stores in this order: T212's Car::setColor / virtual_1). */
    void set(f32 v) { force = 0; velocity = 0; value = v; }
    /* (? names) T212's Car::virtual_1 / virtual_2 (the members read through
     * `this', a pointer to the MotionValue): a spring force towards goal, the
     * value clamped from above (at rest there), the velocity, the value. */
    void pull(f32 goal, f32 k) { force += (goal - value) * k; }
    void limit(f32 max)
    {
        if (value > max) {
            value = max;
            velocity = 0;
        }
    }
    void setVelocity(f32 v) { velocity = v; }
    /* (? names, u063) The value clamped from above / below, at rest at the
     * bound: T207's MapButtonList::virtual_1 keeps &zoom in $s0 for them;
     * retail stores the velocity first above, the value first below. */
    void clampMax(f32 max)
    {
        if (value > max) {
            velocity = 0;
            value = max;
        }
    }
    void clampMin(f32 min)
    {
        if (value < min) {
            value = min;
            velocity = 0;
        }
    }
    /* (? name) The value alone (force and velocity kept): T098's
     * HandicapItem() keeps &barX / &barW in $s0 / $s1 across calcBar for it. */
    void setValue(f32 v) { value = v; }
    f32  get() { return value; }

    MotionValue *update(f32 damping);                   /* 0x001D1A38 velocity += force / 60; force = 0;
                                                           velocity -= velocity * damping / 60;
                                                           value += velocity / 60 */
    MotionValue *update(f32 damping, f32 forceScale);   /* 0x001D1A78 the same with force * forceScale
                                                           (no callers) */
    MotionValue *approach(f32 target, f32 rate);        /* 0x001D1AC0 value += (target - value) * rate */

    f32 force;                              /* 0x0 accumulated for the frame, cleared by update */
    f32 velocity;                           /* 0x4 */
    f32 value;                              /* 0x8 */
};
EGCS_SIZE(MotionValue, 0xC);

#endif /* GAME_T260_MOTION_VALUE_H */
