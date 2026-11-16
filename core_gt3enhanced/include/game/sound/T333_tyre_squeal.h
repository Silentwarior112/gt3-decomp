/* T333_tyre_squeal.h - the tyre squeal and smoke curves (translation unit
 * T333, unit u078), 0x001DE5E0..0x001DF148.
 *
 * tyre_sound_init (DynamicsConductor::virtual_20) validates the table
 * g_002D35B8 (T355), builds for each of the 4 surface classes three curve
 * sets from it (set 0 the squeal volume, 1 its pitch, 2 the smoke: two
 * curves each, the result sqrt(c1^2 + c2^2), plus a term in the wheel's
 * speedFactor (|vel| / 50) above a threshold speed), the car sound
 * parameters (car_sound_params_init, T288) and the fixed squeal of surfaces
 * 1 / 13 (? the kerbs).  tyre_sound_update (the per-car step car_step, T302)
 * evaluates them per wheel into DynWheel smoke / squealVolume / squealPitch;
 * the smoke of the wheels on the surface classes 1 and 2 adds to the car's
 * dirt (CarCondition::addDirt, x 8 when the course kind is 0).
 * The enhanced engine (Lupo 2, cameras_sound.md S4 / S5): Lupo 2's tarmac
 * rows in place of the table's class 0 (ENH_SOUND_TARMAC_SQUEAL), the
 * right-hand wheels' squeal onset detuned (ENH_SOUND_SQUEAL_DETUNE). */
#ifndef GAME_T333_TYRE_SQUEAL_H
#define GAME_T333_TYRE_SQUEAL_H

#include "game/game.h"
#include "game/car/physics/T284_car_param.h"            /* Curve (held by value) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class TyreSoundRow;                         /* T355_setting_check.h */

/* One built curve set.  Not polymorphic.  Size 0x84 (g_0049C990[3][4] is
 * followed by g_0049CFC0). */
class TyreCurveSet {
public:
    f32   x1[4];                            /* 0x00 */
    f32   y1[4];                            /* 0x10 */
    Curve c1;                               /* 0x20 over the slip angle or the lateral slip speed */
    f32   x2[8];                            /* 0x2C */
    f32   y2[8];                            /* 0x4C */
    Curve c2;                               /* 0x6C over the slip ratio (DynWheel.slip) or the longitudinal slip */
    f32   base;                             /* 0x78 added to a non-zero result (row base / 100) */
    f32   threshold;                        /* 0x7C of DynWheel.speedFactor (row threshold km/h in m/s / 50) */
    f32   gain;                             /* 0x80 above it */
};
EGCS_SIZE(TyreCurveSet, 0x84);

/* The built sets, per surface class (squeal volume g_0049C990[4], its pitch
 * g_0049CBA0[4], the smoke g_0049CDB0[4]) and the squeal on surfaces 1 / 13
 * (g_0049CFC0[5]) are T333's file statics (retail .bss 0x0049C990..
 * 0x0049CFD4; u078's data pass), defined in T333_tyre_squeal.cpp. */

/* Curve 1 of a set: n1 points (x1, y1) scaled; curveSet mode 0 or 2. */
void tyre_curve1_build(TyreCurveSet *s, const TyreSoundRow *r, int mode,
                       f32 xScale, f32 yScale);                         /* 0x001DE5E0 */
/* Curve 2: the n2neg negative points (stored outwards: read backwards,
 * negated), then the n2pos positive ones, each side with its y scale. */
void tyre_curve2_build(TyreCurveSet *s, const TyreSoundRow *r, int mode,
                       f32 xScale, f32 yNegScale, f32 yPosScale);       /* 0x001DE680 */
void tyre_curve_extra(TyreCurveSet *s, const TyreSoundRow *r);          /* 0x001DE7C0 base, threshold, gain */
void tyre_sound_init(void);                                             /* 0x001DE830 */
/* Per wheel: contactTimer is 6 after a frame with load and counts down in
 * the air; at 0 the wheel is silent (smoke 0, volume 0, pitch 1). */
void tyre_sound_update(RaceCarState *car);                              /* 0x001DEBB0 */

#endif /* GAME_T333_TYRE_SQUEAL_H */
