/* T340 (0x001E3708..0x001E3BF0): the slip estimate and the driving aid
 * dispatch. */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T340_assist_control.h"

/* DynCarState.understeer: how far the yaw rate (with half the yaw
 * acceleration added while it grows) stays below the one the steering asks
 * for (speed x sinf(steer) / wheelbase), when the lateral acceleration is
 * more than 0.2 of the tyres' grip at rest.  DynCarState.oversteer: for the
 * pad driver in CTRL_DRIVER, the larger rear slip angle beyond a
 * speed-dependent angle (g_002D28B0 oversteerAngleLo / Hi and GainLo / Hi
 * over 0..55.6 m/s, the angle up to 90 degrees mapped to 0..1); for the
 * others, the excess of speed x yaw rate over the lateral acceleration
 * against the turn, x 2 / the grip.  The enhanced engine,
 * ENH_PHYSICS_AI_OVERSTEER (physics.md 16; Lupo 2 L2 0x001F8954..
 * 0x001F8960): the others' oversteer stays 0 below 25 km/h. */
// FUNC 0x001E3708 car_slip_estimate
void car_slip_estimate(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams   *p = &car->params;
    f32 grip;
    int i;

    st->understeer = 0.0f;
    st->oversteer = 0.0f;
    if (st->speedX <= st->understeer)       /* (reloads the 0 just stored) */
        return;

    grip = st->understeer;                  /* 0 */
    for (i = 0; i < 4; i++)
        grip += CarDynamics_getTyre(p, i, &st->wheel[i])->grip;

    {
        f32 steer = st->steer;
        f32 yawRate = st->yawRate;
        f32 yawAccel = st->yawAccel;

        if (steer * yawRate >= 0.0f) {
            f32 wanted;

            if (steer < 0.0f) {
                steer = -steer;
                yawRate = -yawRate;
                yawAccel = -yawAccel;
            }
            wanted = st->speedX * sinf(steer) / p->wheelbase;
            if (wanted > 0.0f) {
                f32 lat = st->accelY;
                if (lat < 0.0f)
                    lat = -lat;
                lat /= grip;
                if (lat > 0.2f) {
                    if (yawAccel * yawRate > 0.0f)
                        yawRate += yawAccel * 0.5f;
                    if (yawRate < wanted)
                        st->understeer = 1.0f - yawRate / wanted;
                }
            }
        }
    }

    if (st->driver != DRIVER_AUTO && st->driver != DRIVER_COMPUTER && st->ctrlMode == CTRL_DRIVER) {
        f32 rearSlip = 0.0f;
        f32 speedPos;
        f32 lo, hi, start;

        /* the binary walks a pointer from &tyreForce[2].slipAngle (+0x358)
         * in steps of 0x24 while it is below &oversteer (+0x3A0) */
        for (i = 2; i < 4; i++) {
            f32 a = st->tyreForce[i].slipAngle;
            if (a < 0.0f)
                a = -a;
            if (rearSlip < a)
                rearSlip = a;
        }
        /* 1 at 55.6 m/s (200 km/h); the speed read again here (retail
         * squares it after the loop: no local copy that GCSE could hoist) */
        speedPos = boot_sqrtf_v(st->speedX * st->speedX + st->speedY * st->speedY) * 0.018f;
        if (speedPos > 1.0f)
            speedPos = 1.0f;
        lo = (f32)g_002D28B0.oversteerAngleLo * DEG2RAD;
        hi = (f32)g_002D28B0.oversteerAngleHi * DEG2RAD;
        start = speedPos * (hi - lo) + lo;
        if (start <= rearSlip) {
            f32 gainLo = (f32)g_002D28B0.oversteerGainLo * 0.1f;
            f32 gainHi = (f32)g_002D28B0.oversteerGainHi * 0.1f;
            /* (rearSlip takes the result: one variable in retail, $f4) */
            rearSlip = (speedPos * (gainHi - gainLo) + gainLo) * (rearSlip - start) / (1.5707964f - start);
            if (rearSlip > 1.0f)
                rearSlip = 1.0f;
            st->oversteer = rearSlip;
        }
    } else {
#if ENH_PHYSICS_AI_OVERSTEER
        f32 speed = boot_sqrtf_v(st->speedX * st->speedX + st->speedY * st->speedY);
        f32 yawAcc;
        f32 lat;

        if (speed < 6.9444437f)
            return;
        yawAcc = speed * st->yawRate;
        lat = st->accelY;
#else
        f32 yawAcc = boot_sqrtf_v(st->speedX * st->speedX + st->speedY * st->speedY) * st->yawRate;
        f32 lat = st->accelY;
#endif

        if (yawAcc * lat >= 0.0f)
            return;
        if (yawAcc < 0.0f)
            yawAcc = -yawAcc;
        if (lat < 0.0f)
            lat = -lat;
        yawAcc -= lat;
        if (yawAcc < 0.0f)
            yawAcc = 0.0f;
        st->oversteer = (yawAcc + yawAcc) / grip;
    }
}

/* The aids of the car's own block (CarDynamics.tcs, from the spec's aid
 * settings): the traction control, then the brake aids (the brake assist
 * only below 0.1 handbrake). */
// FUNC 0x001E3A68 car_assists_own
void car_assists_own(RaceCarDynamics *car)
{
    TcsParams *t = &car->params.tcs;
    CarDynState *st = &car->st;
    int enable;

    car_traction_control(car, t);
    enable = 1;
    if (!(st->handbrake < 0.1f))
        enable = 0;
    car_brake_aids(car, t, enable);
}

/* The same with the computer driver's block (AIDriver.params.assist, from
 * the course's drive parameters, T362). */
// FUNC 0x001E3AD0 car_assists_ai
void car_assists_ai(RaceCarDynamics *car)
{
    TcsParams *t = &car->ai.params.assist;

    car_traction_control(car, t);
    car_brake_aids(car, t, 1);
}

/* The enhanced engine (Lupo 2 L2 0x001F7F60): the brake aid's flag - the
 * block has the brake or the ABS part, or with
 * ENH_PHYSICS_UNDERSTEER_BRAKE (physics.md 15) the understeer brake. */
static int tcs_brake_shown(TcsParams *t)
{
    if (tcs_brake_or_abs(t) != 0)
        return 1;
#if ENH_PHYSICS_UNDERSTEER_BRAKE
    if (t->brakeOn2 != 0)
        return 1;
#endif
    return 0;
}

/* From the conductor's car step (F_001F39D8, T317).  The enhanced engine
 * (Lupo 2 L2 0x001F8AB0): with ENH_PHYSICS_DISPLAY_FLAGS (physics.md 22 a)
 * a computer car (driver 2) also sets its aid flags from its own block, as
 * the pad car does while the computer has it; the brake flag counts the
 * understeer brake with ENH_PHYSICS_UNDERSTEER_BRAKE. */
// FUNC 0x001E3B18 car_assists
void car_assists(RaceCarDynamics *car)
{
    CarDynState *st;

    car_slip_estimate(car);
    st = &car->st;
    st->tractionAssist = 0;
    st->brakeAssist = 0;
    switch (st->driver) {
    case DRIVER_PAD:
        if (st->ctrlMode == CTRL_DRIVER || st->pitTimer != 0) {
            car_assists_own(car);
        } else {
            /* the computer has the pad car: its block works, then the flags
             * say whether the car's own aids are on (? for the display,
             * which reads them through car_traction_assist /
             * car_brake_assist, T302) */
            car_assists_ai(car);
            st->tractionAssist = (u8)tcs_throttle_on(&car->params.tcs);
            st->brakeAssist = (u8)tcs_brake_shown(&car->params.tcs);
        }
        break;
    case DRIVER_AUTO:
        car_assists_ai(car);
        break;
    case DRIVER_COMPUTER:
        car_assists_ai(car);
#if ENH_PHYSICS_DISPLAY_FLAGS
        st->tractionAssist = (u8)tcs_throttle_on(&car->params.tcs);
        st->brakeAssist = (u8)tcs_brake_shown(&car->params.tcs);
#endif
        break;
    }
}
