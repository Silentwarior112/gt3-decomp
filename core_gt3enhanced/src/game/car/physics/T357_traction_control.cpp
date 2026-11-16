/* T357 (0x001E32D8..0x001E35D8): the traction control (the throttle cut).
 * (T357 has no canonical header: its functions are used inside u079 only,
 * by car_assists_own / car_assists_ai and car_assists, T340.) */
#include "game/car/physics/car_input_internal.h"

/* 1 up to |x| = start, then falling linearly (slope `gain') to 0. */
// FUNC 0x001E32D8 fade_linear
f32 fade_linear(f32 x, f32 gain, f32 start)
{
    if (x < 0.0f)
        x = -x;
    x -= start;
    if (x < 0.0f)
        return 1.0f;
    x *= gain;
    if (x > 1.0f)
        return 0.0f;
    return 1.0f - x;
}

/* The block has a throttle cut (throttleGain != 0). */
// FUNC 0x001E3338 tcs_throttle_on
int tcs_throttle_on(TcsParams *t)
{
    int on = 1;

    if (t->throttleGain == 0.0f)
        on = 0;
    return on;
}

/* Cuts DynCarState.throttleOut by the slip of the driven wheels: of those
 * on the ground, the one with the lowest slip ratio (? the one spinning
 * most: the drive slip is negative, and it is held against its tyre's
 * negative-side peak, peakSlipNeg x slipTarget), with the target faded out
 * by DynCarState.latGripUse (fadeGain / fadeStart).  Nothing while the engine
 * gives no torque (engineTorque <= 0) or without throttle.
 * DynCarState.tractionAssist: 1 armed, 2 cutting. */
// FUNC 0x001E3360 car_traction_control
void car_traction_control(RaceCarDynamics *car, TcsParams *t)
{
    CarDynState *st = &car->st;
    u8 *driveType = &car->params.gear.driveType;     /* sp+0x00 */
    f32 fade;
    f32 minSlip;
    f32 peakSlip;
    f32 slipGrip;
    f32 cut;
    int axle;
    int i;

    st->tractionAssist = (u8)tcs_throttle_on(t);
    if (st->tractionAssist == 0)
        return;
    fade = fade_linear(st->latGripUse, t->fadeGain, t->fadeStart);
    if (st->engineTorque <= 0.0f)
        return;
    if (st->throttleOut == 0.0f)
        return;

    minSlip = 1.0f;
    peakSlip = 1.0f;
    slipGrip = 1.0f;
    for (axle = 0; axle < 2; axle++) {
        /* only the driven axles: the rear for driveType 0 / 5 / 6, the front
         * for 1, both for the other 4WD kinds (as the binary has it: the 4WD
         * kinds 5 and 6 count as rear-driven here) */
        switch (*driveType) {
        case DRIVE_REAR: case DRIVE_AWD_SPEED: case DRIVE_AWD_STEER:
            if (axle == 0)
                continue;
            break;
        case DRIVE_FRONT:
            if (axle == 1)
                continue;
            break;
        }
        for (i = 0; i < 2; i++) {
            int n = axle * 2 + i;
            f32 slip = st->wheel[n].slip;
            if (st->wheel[n].load != 0.0f && slip < minSlip) {
                minSlip = slip;
                peakSlip = CarDynamics_getTyre(&car->params, n, &st->wheel[n])->peakSlipNeg;
                slipGrip = st->wheel[n].slipGrip;
            }
        }
    }

    peakSlip *= fade;                       /* the target slip */
    peakSlip *= t->slipTarget;
    cut = (minSlip - peakSlip) * (t->throttleGain * slipGrip) + 1.0f;
    if (cut < 0.0f)
        cut = 0.0f;
    else if (cut > 1.0f)
        cut = 1.0f;
    if (cut < 1.0f)
        st->tractionAssist = 2;
    st->throttleOut *= cut;
}
