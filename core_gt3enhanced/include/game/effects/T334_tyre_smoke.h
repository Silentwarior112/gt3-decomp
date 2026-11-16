/* T334_tyre_smoke.h - SmokeSource and the tyre smoke (translation unit T334,
 * unit u078), 0x001DF148..0x001DF2B0.
 *
 * tyre_smoke_emit gives the course one SmokeSource per wheel
 * (RaceCourse::emitSmoke, T153, to its smoke manager): the wheel's
 * position, yaw + steer, the ground contact's intensity, the ground plane
 * and the wheel's smoke.  Both functions are called by the per-car step
 * car_step (T302); u050's smoke manager reads the records. */
#ifndef GAME_T334_TYRE_SMOKE_H
#define GAME_T334_TYRE_SMOKE_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* One wheel's smoke, built on the stack of tyre_smoke_emit (sp+0x00).  Not
 * polymorphic.  Size 0x3C (u058 reads up to +0x2C).  u058 types pos / plane
 * as Vec3 / Vec4 (T159); the owner's float arrays are kept. */
class SmokeSource {
public:
    u32 flags;                              /* 0x00 the wheel index: bit 0 (the right wheels) spins the other way */
    int kind;                               /* 0x04 the wheel's surface (u058: index of g_003306D0) */
    f32 pos[3];                             /* 0x08 course axes: pos[0] + offX, the wheel's groundZ, -(pos[1] + offY) */
    f32 angle;                              /* 0x14 yaw + the wheel's steer */
    f32 intensity;                          /* 0x18 0..1 the ground contact's u (+0x14) */
    f32 plane[4];                           /* 0x1C the ground plane (normal, d), d raised by 0.05 */
    f32 rate;                               /* 0x2C the wheel's smoke 0..1 */
    f32 f30;                                /* 0x30 the wheel's slip (+0x68) */
    f32 f34;                                /* 0x34 the wheel's velX (+0x4C) */
    f32 f38;                                /* 0x38 the wheel's spin (+0x20) */
};
EGCS_SIZE(SmokeSource, 0x3C);

void F_001DF148(RaceCarState *car);         /* 0x001DF148 empty */
void tyre_smoke_emit(RaceCarState *car);    /* 0x001DF150 */

#endif /* GAME_T334_TYRE_SMOKE_H */
