/* T295 - the brake heat of each wheel in 0..1 (DynWheel.brakeHeat): it
 * rises with the axle's brakeTorque x tyreRadius x the wheel's brake x
 * |spin| (the brake's work) and falls with (|spin| + coolSpeed) x (heat +
 * coolBias) (the air of the motion); the constants BrakeHeatParams
 * g_0049D0B0 come from the dynamics tuning bytes g_002D28B0 +0x7C..+0x7F.
 * RaceCarModel::drawTires (u060) draws it as the brake disc's glow. */
#include "game/car/physics/T295_tyre_heat.h"
#include "game/car/physics/drivetrain_internal.h"

static BrakeHeatParams g_0049D0B0;          /* 0x0049D0B0 (.bss) */

/* Called by DynamicsConductor::virtual_20 (0x001F2FE0). */
// FUNC 0x001E6878 brake_heat_setup
void brake_heat_setup(void)
{
    DynParams *dp = &g_002D28B0;
    int n;

    n = dp->heatScale;
    if (n == 0)
        n = 1;
    g_0049D0B0.heatGain = 1.0f / ((f32)n * 9800.0f);
    n = dp->coolTime;
    if (n == 0)
        n = 1;
    g_0049D0B0.coolRate = 1.0f / ((f32)n * 0.1f * 100.0f);
    g_0049D0B0.coolSpeed = (f32)dp->coolSpeed * KMH_TO_MS;
    g_0049D0B0.coolBias = (f32)dp->coolBias * 0.01f;
}

/* Called by DynamicsConductor::update (0x001F39D8) every dynamics frame. */
// FUNC 0x001E6928 car_brake_heat
void car_brake_heat(RaceCarState *car)
{
    f32 dt = car->dt;
    BrakeHeatParams *bp = &g_0049D0B0;
    int i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &car->st.wheel[i];
        f32 spin = w->spin;
        f32 brakeK = car->params.brakeTorque[i >> 1] * car->params.gear.tyreRadius[i >> 1];
        f32 heat;

        if (spin < 0.0f)
            spin = -spin;
        heat = w->brakeHeat;
        heat = heat + ((brakeK * w->brake * spin * bp->heatGain) -
                       ((spin + bp->coolSpeed) * bp->coolRate *
                        (heat + bp->coolBias))) * dt;
        if (1.0f < heat)
            heat = 1.0f;
        else if (heat < 0.0f)
            heat = 0.0f;
        w->brakeHeat = heat;
    }
}

/* The heat at fraction t between this frame (t = 0) and the previous one
 * (t = 1, RaceCarState.wheelBrakeHeatPrev), squared.  Caller: RaceCarModel::
 * drawTires (u060), for the tyre model's F_002222B8 (the glow). */
// FUNC 0x001E6A08 car_brake_heat_sq
f32 car_brake_heat_sq(RaceCarState *car, int wheel, f32 t)
{
    f32 v = (car->wheelBrakeHeatPrev[wheel] - car->st.wheel[wheel].brakeHeat) * t + car->st.wheel[wheel].brakeHeat;

    return v * v;
}
