/* T291 - the engine: 0x001E0BC0..0x001E1768.
 *
 * engine_setup builds the CarEngine (CarDynamics +0x138, RaceCarState
 * +0x148) from the engine bytes of the car's CarSpec: the torque curve (rpm
 * x 100 in rad/s, torque x 9.8 x a gain and a factor interpolated over the
 * rpm range), two turbo stages (turbo_setup) and a power limit.  The torque
 * at a speed is the curve x the condition's power factor
 * (engine_torque_at), x the turbo factor 1 + the boosts (capped at 1 +
 * turboMax), then limited (engine_power_limit).  engine_spec_sheet fills the
 * power / torque table of the tuning screens and of
 * ValuateAutomobilePerformance (T279). */
#include "game/car/physics/car_engine_internal.h"
#include "game/car/physics/T291_engine.h"

/* The torque at `speed' (rad/s) x ConditionModel::powerFactor of the
 * condition (a fresh CarCondition when NULL).  Callers:
 * CarDynamics_calcTopSpeed (T284), engine_spec_sheet, car_pre_step (T300). */
// FUNC 0x001E0BC0 engine_torque_at
f32 engine_torque_at(CarEngine *e, CarCondition *cond, f32 speed)
{
    CarCondition fresh;                     /* sp+0x00 */
    if (cond == NULL)
        cond = &fresh;
    f32 torque = curveLookup(&e->torqueCurve, speed);
    return torque * g_002D2B00.powerFactor(cond);
}

/* Callers: car_pre_step (T300).  cond is not read. */
// FUNC 0x001E0C28 engine_scale
f32 engine_scale(CarEngine *e, CarCondition *cond, f32 x)
{
    return e->torqueScale * x;
}

/* With the limiter on: when torque x speed passes limitPower x (1 -
 * softness), the power is cut to limitPower x (1 - softness / 2) - (the
 * rest of the way to limitPower)^2 / (2 softness limitPower) (limitPower x
 * (1 - softness / 2) from limitPower on), over speed (never above the
 * input).  Callers: CarDynamics_calcTopSpeed (T284), engine_spec_sheet,
 * engine_output. */
// FUNC 0x001E0C38 engine_power_limit
f32 engine_power_limit(CarEngine *e, f32 torque, f32 speed)
{
    if (e->limiter == 0)
        return torque;
    f32 soft = e->limitSoftness;
    f32 power = torque * speed;
    f32 limit = e->limitPower;
    if (power <= limit * (1.0f - soft))
        return torque;
    f32 cut = limit * (1.0f - soft * 0.5f);
    if (!(limit <= power)) {
        f32 rest = limit - power;
        cut = cut - (rest * rest) / ((soft + soft) * limit);
    }
    cut = cut / speed;
    if (cut < torque)
        return cut;
    return torque;
}

/* One stage's boost at rpm: below turboRpm by its curve (TURBO_CURVE_*;
 * another type: 0); from turboRpm to 3 x turboRpm boost - K d^2 / 4; 0
 * beyond. */
// FUNC 0x001E0CD8 turbo_boost_stage
f32 turbo_boost_stage(CarEngine *e, s16 rpm, int stage)
{
    f32 boost = 0.0f;
    int full = e->turboRpm[stage];
    if (rpm < full) {
        switch (e->turboType[stage]) {
        case TURBO_CURVE_FALLING: {
            f32 d = (f32)(rpm - full);
            boost = e->turboBoost[stage] - e->turboK[stage] * d * d;
            break;
        }
        case TURBO_CURVE_SQUARE: {
            f32 r = (f32)rpm;
            boost = e->turboK[stage] * r * r;
            break;
        }
        case TURBO_CURVE_LINEAR:
            boost = (e->turboBoost[stage] * (f32)rpm) / (f32)full;
            break;
        }
    } else if (rpm < full * 3) {
        f32 d = (f32)(rpm - full);
        boost = e->turboBoost[stage] - e->turboK[stage] * d * d * 0.25f;
    }
    return boost;
}

/* 1 + the boosts of the stages that have one (each also to boost[]), stage
 * i at rpm i.  Callers: turbo_boost_factor, engine_setup. */
// FUNC 0x001E0E08 turbo_boost_sum
f32 turbo_boost_sum(CarEngine *e, s16 rpm0, s16 rpm1, f32 *boost)
{
    s16 rpm[2];                             /* sp+0x00 */
    f32 sum = 0.0f;

    rpm[0] = rpm0;
    rpm[1] = rpm1;
    boost[0] = 0.0f;
    boost[1] = 0.0f;
    for (int i = 0; i < 2; i++) {
        if (e->turboBoost[i] != 0.0f) {
            f32 b = turbo_boost_stage(e, rpm[i], i);
            sum += b;
            boost[i] = b;
        }
    }
    return sum + 1.0f;
}

/* turbo_boost_sum, at most 1 + turboMax.  Callers: DynCar_setInitialSpeed
 * (T330), engine_setup, engine_update. */
// FUNC 0x001E0ED8 turbo_boost_factor
f32 turbo_boost_factor(CarEngine *e, s16 rpm0, s16 rpm1, f32 *boost)
{
    f32 factor = turbo_boost_sum(e, rpm0, rpm1, boost);
    f32 cap = e->turboMax + 1.0f;
    if (cap < factor)
        factor = cap;
    return factor;
}

/* No turbo unless turboMax and stage 0's rpm, boost and rate are all
 * non-zero; stage 1 only when its three are non-zero.  Types of 3 and
 * above become 0.  The third argument (engine_setup's last) is not read. */
// FUNC 0x001E0F38 turbo_setup
void turbo_setup(const CarSpec *spec, CarEngine *e, int unused)
{
    if (spec->turboMax == 0)
        e->turboMax = 0.0f;
    else {
        e->turboMax = (f32)spec->turboMax * 0.1f;
        if (spec->turboRpm[0] == 0 || spec->turboBoost[0] == 0 || spec->turboRate[0] == 0)
            e->turboMax = 0.0f;
    }

    if (e->turboMax == 0.0f) {
        e->turboType[0] = 0;
        e->turboBoost[0] = 0.0f;
        e->turboRate[0] = 0.0f;
        e->turboK[0] = 0.0f;
        e->turboRpm[0] = 0;
        e->turboType[1] = 0;
        e->turboBoost[1] = 0.0f;
        e->turboRate[1] = 0.0f;
        e->turboK[1] = 0.0f;
        e->turboRpm[1] = 0;
        return;
    }

    e->turboType[0] = spec->turboType[0];
    if (e->turboType[0] >= 3)
        e->turboType[0] = TURBO_CURVE_FALLING;
    e->turboBoost[0] = (f32)spec->turboBoost[0] * 0.1f;
    e->turboRate[0] = (f32)spec->turboRate[0] * 0.1f;
    int rpm = spec->turboRpm[0] * 100;
    e->turboRpm[0] = rpm;
    e->turboK[0] = e->turboBoost[0] / ((f32)rpm * (f32)rpm);
    e->turboType[1] = 0;
    e->turboBoost[1] = 0.0f;
    e->turboRate[1] = 0.0f;
    e->turboK[1] = 0.0f;
    e->turboRpm[1] = 0;

    u8 boost1 = spec->turboBoost[1];
    u8 rate1 = spec->turboRate[1];
    u8 rpm1 = spec->turboRpm[1];
    if (boost1 + rate1 + rpm1 != 0 && boost1 != 0 && rpm1 != 0 && rate1 != 0) {
        e->turboType[1] = spec->turboType[1];
        if (e->turboType[1] >= 3)
            e->turboType[1] = TURBO_CURVE_FALLING;
        e->turboBoost[1] = (f32)spec->turboBoost[1] * 0.1f;
        e->turboRate[1] = (f32)spec->turboRate[1] * 0.1f;
        rpm = spec->turboRpm[1] * 100;
        e->turboRpm[1] = rpm;
        e->turboK[1] = e->turboBoost[1] / ((f32)rpm * (f32)rpm);
    }
}

/* Callers: CarDynamics_init (T284), gear_chart_of_spec / gear_chart_with_sheet (T292).
 * powerLimit > 0 (PS) turns the limiter on (limitPower = powerLimit x 9.8 x
 * 716.2 x 2 pi / 60 / (1 - softness / 2), softness = g_002D28B0.
 * powerLimitSoftness %).  Quirks kept: a zero last torque point is copied
 * from the one before into the spec; the torques are read signed (lh) and
 * those below -1 put back to unsigned.
 * Matching (u078): 99.2%, exact with the reference compiler 991111-01.  Left
 * under 991111b: torqueTop and n - 1, both hoisted in front of the first
 * loop (GCSE), take $t0 / $a3 the other way round (the order of GCSE's new
 * registers, see the bootstrap's MATCHING.md); all 120 orders of the five
 * curve factor statements tried. */
// FUNC 0x001E10E8 engine_setup
void engine_setup(CarEngine *e, CarSpec *spec, int powerLimit, int arg3)
{
    int i;

    e->idle = spec->idle * 10;
    if (spec->autoGearRpm != 0)
        e->redline = spec->autoGearRpm * 100;
    else
        e->redline = spec->curveRpm[spec->curveCount - 1] * 100;
    if (e->redline < spec->revLimit * 100 + 200)
        e->redline = spec->revLimit * 100 + 200;

    if (spec->curveTorque[spec->curveCount - 1] == 0)
        spec->curveTorque[spec->curveCount - 1] = spec->curveTorque[spec->curveCount - 2];
    int n = spec->curveCount;
    for (i = 0; i < n; i++)
        e->rpm[i] = (f32)(spec->curveRpm[i] * 100) * 0.10471976f;    /* rpm -> rad/s */

    f32 bottom, top, gain;
    if (spec->torqueBottom == 0)
        bottom = 1.0f;
    else
        bottom = (f32)spec->torqueBottom * 0.01f;
    if (spec->torqueTop == 0)
        top = 1.0f;
    else
        top = (f32)spec->torqueTop * 0.001f;
    f32 rpmFirst = e->rpm[0];
    f32 rpmLast = e->rpm[n - 1];
    if (spec->torqueGain == 0)
        gain = 1.0f;
    else
        gain = (f32)spec->torqueGain * 0.001f;
    gain = gain * 0.01f;
    for (i = 0; i < n; i++) {
        f32 factor = gain * (bottom + ((top - bottom) * (e->rpm[i] - rpmFirst)) / (rpmLast - rpmFirst));
        int torque = spec->curveTorque[i];
        if (torque < -1)
            torque += 0x10000;
        e->torque[i] = (f32)torque * 9.8f * factor;
    }
    curveSet(&e->torqueCurve, spec->curveCount, e->rpm, e->torque, 0, 0);
    e->torqueScale = ((f32)spec->torqueUnit * 0.1f * 9.8f) / e->rpm[spec->curveCount - 1];
    turbo_setup(spec, e, arg3);

    /* the curve without the uncapped part of the boost */
    for (i = 0; i < e->torqueCurve.count; i++) {
        f32 boost[2];                       /* sp+0x00 */
        s16 rpm = spec->curveRpm[i] * 100;
        f32 factor = turbo_boost_factor(e, rpm, rpm, boost);
        factor = factor / turbo_boost_sum(e, rpm, rpm, boost);
        e->torque[i] *= factor;
    }

    e->limiter = 0;
    e->limitPower = 0.0f;
    if (powerLimit > 0) {
        e->limiter = 1;
        f32 power = (f32)powerLimit * 9.8f * 716.2f * 6.2831855f / 60.0f;
        f32 soft = (f32)g_002D28B8 * 0.01f;
        e->limitSoftness = soft;
        e->limitPower = power / (1.0f - soft * 0.5f);
    }
}

/* The spec sheet: the curve's rpm points rounded to 100 (ascending, no
 * repeats), then every 100 rpm from the first point to max(redline, the
 * last point): torque (kgm) and power (PS = kgm x rpm / 716.2) through
 * engine_torque_at and engine_power_limit, stored at the points, the maxima
 * up to the redline.  Quirks kept: no points reads rpm[-1]; the NULL test
 * comes after the writes.  Callers: ShowRoomCar::SetupShowRoomCar (T278),
 * ValuateAutomobilePerformance (T279), gear_chart_with_sheet (T292). */
// FUNC 0x001E14E8 engine_spec_sheet
void engine_spec_sheet(CarEngine *e, EngineSpecSheet *out, CarCondition *cond)
{
    f32 maxPower = 0.0f;
    int maxPowerRpm = 0;
    f32 maxTorque = 0.0f;
    int maxTorqueRpm = 0;
    int count = 0;
    int rpm;

    for (int i = 0; i < e->torqueCurve.count; i++) {
        rpm = (int)((e->rpm[i] * 60.0f) / 6.2831855f);           /* rad/s -> rpm */
        rpm = ((rpm + 50) / 100) * 100;
        if (count <= 0 || out->rpm[count - 1] < rpm) {
            out->rpm[count] = rpm;
            count++;
        }
    }
    out->count = count;

    rpm = out->rpm[0];
    u16 redline = e->redline;
    out->redline = redline;
    int last = redline;
    if (last < out->rpm[count - 1])
        last = out->rpm[count - 1];

    if (!(last < rpm)) {
        int n = 0;
        do {
            f32 rpmF = (f32)rpm;
            f32 speed = rpmF * 0.10471976f;
            f32 kgm = engine_power_limit(e, engine_torque_at(e, cond, speed), speed) / 9.8f;
            f32 ps = (kgm * rpmF) / 716.2f;
            if (out->rpm[n] == rpm) {
                out->power[n] = ps;
                out->torque[n] = kgm;
                n++;
            }
            if (!(out->redline < rpm)) {
                if (maxTorque < kgm) {
                    maxTorque = kgm;
                    maxTorqueRpm = rpm;
                }
                if (maxPower < ps) {
                    maxPower = ps;
                    maxPowerRpm = rpm;
                }
            }
            rpm += 100;
        } while (!(last < rpm));
    }
    if (out != NULL) {
        out->maxPower = maxPower;
        out->maxPowerRpm = maxPowerRpm;
        out->maxTorque = maxTorque;
        out->maxTorqueRpm = maxTorqueRpm;
    }
}
