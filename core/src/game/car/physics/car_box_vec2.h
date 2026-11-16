/* car_box_vec2.h - the 2D vector helpers of the car boxes (T375, T376;
 * unit u088 only).
 *
 * Inline functions that take their vectors by pointer.  Each call takes its
 * arguments into fresh registers: retail reads a field again through each
 * call's own pointer (a box edge's p0 through two or three registers in one
 * function), and a vector's y read through such a pointer stays apart from
 * the same field read directly.  rot: a row-major 2x2 rotation {cos, -sin,
 * sin, cos} of a box's yaw (world = rot * local).
 */
#ifndef U088_CAR_BOX_VEC2_H
#define U088_CAR_BOX_VEC2_H

#include "game/car/physics/car_box_internal.h"

static inline void vec2_set(Vec2 *v, f32 x, f32 y)
{
    v->x = x;
    v->y = y;
}

static inline void rot_perp(Vec2 *out, const f32 *rot, const Vec2 *p)
{
    out->x = rot[0] * p->y + rot[1] * -p->x;
    out->y = rot[2] * p->y + rot[3] * -p->x;
}

static inline void rot_apply_t(Vec2 *out, const f32 *rot, const Vec2 *p)
{
    out->x = rot[0] * p->x + rot[2] * p->y;
    out->y = rot[1] * p->x + rot[3] * p->y;
}

static inline f32 vec2_cross(const Vec2 *a, const Vec2 *b)
{
    return a->x * b->y - a->y * b->x;
}

static inline f32 vec2_dot(const Vec2 *a, const Vec2 *b)
{
    return a->x * b->x + a->y * b->y;
}

static inline void vec2_add(Vec2 *out, const Vec2 *a, const Vec2 *b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
}

static inline void vec2_sub(Vec2 *out, const Vec2 *a, const Vec2 *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
}

static inline void rot_apply(Vec2 *out, const f32 *rot, const Vec2 *p)
{
    out->x = rot[0] * p->x + rot[1] * p->y;
    out->y = rot[2] * p->x + rot[3] * p->y;
}

#endif /* U088_CAR_BOX_VEC2_H */
