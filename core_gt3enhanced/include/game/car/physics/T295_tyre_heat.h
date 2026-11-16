/* T295_tyre_heat.h - the brake heat (translation unit T295, unit u080),
 * 0x001E6878..0x001E6A38.
 *
 * A per-wheel value in 0..1 (DynWheel.brakeHeat): every frame it rises with
 * the axle's brakeTorque x tyreRadius x the wheel's brake x |spin| (the
 * brake's work: only braking heats it) and falls with (|spin| + a speed) x
 * (heat + an offset) (the air of the car's motion); the four constants
 * (BrakeHeatParams g_0049D0B0) come from the dynamics tuning bytes
 * 0x002D28B0 +0x7C..+0x7F (DynParams heatScale .. coolBias; brake_heat_setup,
 * from DynamicsConductor slot 20).  car_brake_heat_sq is the value between
 * the previous frame's and this one's (RaceCarState.wheelBrakeHeatPrev,
 * +0x1624), squared: RaceCarModel::drawTires (u060) passes it, x CourseEnv
 * +0x4C, to the tyre model's F_002222B8 as the additive glow of a hot brake
 * disc.  (The first pass called it the tyre heat: tyre_heat_setup,
 * car_tyre_heat, car_tyre_heat_sq; u080 / u060 found the brake.)
 *
 * TyreGrip is u080 / u086's name for TyreData (T296), the tyre parameter
 * set CarDynamics_getTyre returns (u080 grip +0x140 = TyreData.peakSlipNeg,
 * u086 grip +0x000 = TyreData.grip).  It is a typedef here, with no second
 * definition: other headers must not forward-declare `class TyreGrip'. */
#ifndef GAME_T295_TYRE_HEAT_H
#define GAME_T295_TYRE_HEAT_H

#include "game/game.h"
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData (TyreGrip) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */

typedef TyreData TyreGrip;                  /* u080, u086 */
EGCS_SIZE(TyreGrip, 0x1B4);

/* The constants of car_brake_heat, set by brake_heat_setup from g_002D28B0
 * (a byte of 0 counts as 1 for the two divisors).  Per frame:
 *   heat += (brakeTorque x tyreRadius x brake x |spin| x heatGain
 *            - (|spin| + coolSpeed) x coolRate x (heat + coolBias)) x dt.
 * Not polymorphic.  Size 0x10 (four floats in .bss, only T295 uses them;
 * u080 TyreHeatParams). */
class BrakeHeatParams {
public:
    f32 heatGain;                           /* 0x0 1 / (heatScale x 9800) */
    f32 coolRate;                           /* 0x4 1 / (coolTime x 0.1 x 100) */
    f32 coolSpeed;                          /* 0x8 coolSpeed km/h in m/s: the cooling air at rest */
    f32 coolBias;                           /* 0xC coolBias / 100: cools even a cold wheel (clamped at 0) */
};
EGCS_SIZE(BrakeHeatParams, 0x10);

/* The parameters themselves: a file static of T295 (.bss 0x0049D0B0). */

void brake_heat_setup(void);                                    /* 0x001E6878 */
void car_brake_heat(RaceCarState *car);                         /* 0x001E6928 from DynamicsConductor::update (T317) */
/* (wheelBrakeHeatPrev x t + brakeHeat x (1 - t))^2; unlike T302's
 * accessors, t gets no interpBias. */
f32  car_brake_heat_sq(RaceCarState *car, int wheel, f32 t);    /* 0x001E6A08 */

#endif /* GAME_T295_TYRE_HEAT_H */
