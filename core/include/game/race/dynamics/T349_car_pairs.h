/* T349_car_pairs.h - the car-to-car pass of the DynamicsConductor
 * (translation unit T349, unit u082), 0x001EBB60..0x001EC4B0.
 *
 * dc_car_pairs (from DynamicsConductor::update, T317), for every pair of cars
 * that may meet (car_pair_info: both with a controller, the same flag bit 4,
 * within 100 m and a car height + 5 m of each other): each car's position in
 * the other's frame goes to the computer driver (ai_note_car, T365), a
 * proximity code per ordered pair (DynamicsConductor::near, which
 * CarPairStates::update reads), a push when one car runs into the back of the
 * other within 20 m (ai_note_closing), and a slipstream factor per ordered
 * pair (DynamicsConductor::slip); a car's slipstream is the sum over the
 * others, at most 1.
 *
 * Not this class: u093's CarPairInfo (state[8][8] at +0x84) is the
 * conductor's CarPairStates at +0xBA34 (T305_dynamics_conductor.h). */
#ifndef GAME_T349_CAR_PAIRS_H
#define GAME_T349_CAR_PAIRS_H

#include "game/game.h"

class DynamicsConductor;                    /* T305_dynamics_conductor.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */

/* The relative position of two cars, on the stack of dc_car_pairs.  Not
 * polymorphic.  Size 0x14. */
class CarPairInfo {
public:
    f32 aFwd;                               /* 0x00 b relative to a, along a's forward row (st.rot[0]) */
    f32 aSide;                              /* 0x04 along a's lateral row */
    f32 bFwd;                               /* 0x08 a relative to b, along b's forward row */
    f32 bSide;                              /* 0x0C */
    f32 dist;                               /* 0x10 */
};
EGCS_SIZE(CarPairInfo, 0x14);

void dc_set_near(DynamicsConductor *dc, int car, int other, int code); /* 0x001EBB60 near[car][other] = code (an int: dc_car_pairs passes car_near_code's result unmasked) */
u8   dc_get_near(DynamicsConductor *dc, int car, int other);           /* 0x001EBB80 */
void dc_clear_near(DynamicsConductor *dc);                             /* 0x001EBBA0 (virtual_21) */
/* 0 when the pair cannot meet; else the distance and each car's position in
 * the other's frame. */
int  car_pair_info(CarPairInfo *out, RaceCarState *a, RaceCarState *b); /* 0x001EBBC8 */
/* b at (fwd, side) in a's frame.  For a car not driven by the pad or with a
 * gear mode: 1 when b is ahead with a gap, within the mean width sideways,
 * and a closes on it (each velocity x its car's time scale); *gap, *closing. */
int  car_pair_contact(RaceCarState *a, RaceCarState *b, f32 *gap, f32 *closing, f32 fwd, f32 side); /* 0x001EBDA0 */
void dc_set_slip(DynamicsConductor *dc, int car, int other, f32 v);    /* 0x001EBE98 slip[car][other] = v */
f32  dc_get_slip(DynamicsConductor *dc, int car, int other);           /* 0x001EBEB8 (also CarPairStates::update) */
void dc_clear_slip(DynamicsConductor *dc);                             /* 0x001EBED8 (virtual_21) */
/* a at (fwd, side) in b's frame.  When b runs faster than 100 km/h and a is
 * behind it by less than b's speed in metres and within b's width sideways:
 * (speed + fwd) / speed, 1 right behind b.  a is not read. */
f32  car_slipstream(RaceCarState *a, RaceCarState *b, f32 fwd, f32 side); /* 0x001EBF00 */
/* 0 beyond 50 m; else a longitudinal zone in bits 0..3 (6 far ahead .. 1 far
 * behind) and a lateral zone in bits 4..6 (1 far to one side .. 7 far to the
 * other side), from the gaps and the width. */
int  car_near_code(f32 dist, f32 fwd, f32 side, f32 frontGap, f32 rearGap, f32 width); /* 0x001EBF68 */
void dc_car_pairs(DynamicsConductor *dc);                              /* 0x001EC098 */

#endif /* GAME_T349_CAR_PAIRS_H */
