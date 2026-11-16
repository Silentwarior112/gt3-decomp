/* T287_surface_grip.h - the tyre sound of a wheel by surface class
 * (translation unit T287, unit u077), 0x001DE3B8..0x001DE5E0.
 *
 * Seven evaluators, all called in turn by RaceEntryCar::updateSound
 * (0x00177A50, u042): when the wheel's surface type (DynWheel +0x24) is in
 * its class, *volume = volumeScale x the wheel's squeal volume (DynWheel
 * +0x30) and *pitch = pitchScale x its squeal pitch (+0x34);
 * tyre_sound_roll_0789 uses the rolling sound of car_sound_tyre_roll (T288)
 * instead.  Otherwise they leave both alone.  (u077 read +0x30 / +0x34 as
 * surface grip factors.)  The names are u077's (tentative: the surface
 * classes in the name; first pass F_<address>).  The scales come before the
 * two pointers: the order of the parameter copies decides the evaluators'
 * register allocation (tyre_sound_roll_0789 is exact only so; the call is
 * the same, $f12 / $f13 and $a2 / $a3 either way). */
#ifndef GAME_T287_SURFACE_GRIP_H
#define GAME_T287_SURFACE_GRIP_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only; u077 DynCar) */

void tyre_sound_squeal_0789(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);    /* 0x001DE3B8 surfaces 0, 7, 8, 9 */
void tyre_sound_roll_0789(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);      /* 0x001DE410 surfaces 0, 7, 8, 9: car_sound_tyre_roll */
void tyre_sound_squeal_6_12_14(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch); /* 0x001DE4A0 surfaces 6, 12, 14 */
void tyre_sound_squeal_5(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);       /* 0x001DE508 surface 5 */
void tyre_sound_none(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);           /* 0x001DE548 empty */
void tyre_sound_squeal_1_13(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);    /* 0x001DE550 surfaces 1, 13 */
void tyre_sound_squeal_2_4(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale, f32 *volume, f32 *pitch);     /* 0x001DE598 surfaces 2, 3, 4 */

#endif /* GAME_T287_SURFACE_GRIP_H */
