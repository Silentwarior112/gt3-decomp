/* T342 (0x001E3EA0..0x001E4E78): the driver input processing of a car. */
/* Retail T342's .rodata (0x0033C6B0): "bad_alloc" (unreferenced:
 * DATA_START), car_process_input's jump table, UnitArena<64>'s type name;
 * the original saw the race headers (the conductor, RaceInput, the computer
 * driver's functions). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T342_car_input.h"
// DATA_START 0x0033C6B0

/* .data: the pedal ramps per frame (input_tuning_init sets them from
 * InputTuning.rampTime: 100 / (10 x 60) and 100 / (1 x 60) capped at 1, the
 * values they start with), then the steering class of every surface type
 * (DynWheel.surface; 1: loose ground). */
f32 g_002D4930[6] = {                       /* 0x002D4930 RAMP_* */
    0.16666667f, 0.16666667f, 0.16666667f, 0.16666667f, 1.0f, 1.0f
};
u8 g_002D4948[16] = {                       /* 0x002D4948 */
    0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2, 2, 2
};

/* .sdata (-G8): the steering constants, with the values input_tuning_init
 * gives them from g_002D4628 (InputTuning, T339). */
f32 g_00352970 = 100.0f;                    /* 0x00352970 steerSpring */
f32 g_00352974 = 5.0f;                      /* 0x00352974 steerDamping x 0.1 */
f32 g_00352978 = 50.0f;                     /* 0x00352978 centreSpring */
f32 g_0035297C = 1.5707964f;                /* 0x0035297C 90 deg/s: wheelLockSpeed x 10 deg */
f32 g_00352980 = 0.13962634f;               /* 0x00352980 8 deg: minLock1[0] */
f32 g_00352984 = 0.2617994f;                /* 0x00352984 15 deg: minLock1[1] */
f32 g_00352988 = 0.20943952f;               /* 0x00352988 12 deg (minLock2[0]; input_tuning_init does not set it) */
f32 g_0035298C = 0.34906584f;               /* 0x0035298C 20 deg (minLock2[1]; not set either) */
f32 g_00352990 = 0.1f;                      /* 0x00352990 analogPedalMin x 0.01 */

/* .bss: file statics (only this file uses them). */
static ResponseCurve g_0049D060;            /* 0x0049D060 the wheel controller's throttle curve */
static ResponseCurve g_0049D070;            /* 0x0049D070 the wheel controller's brake curve */
static f32 g_0049D080[3];                   /* 0x0049D080 the digital steering speed, rad/s, per steering class */
static f32 g_0049D090[3];                   /* 0x0049D090 the digital steering speed back to the centre */
static f32 g_0049D0A0[3];                   /* 0x0049D0A0 the analog sticks' steering speed */

/* The input constants from the tuning bytes g_002D4628 (InputTuning, T339):
 * the pedal ramps (per frame: 100 / (rampTime x 60), at most 1), the
 * steering speeds (x 10 degrees per second), the steering springs, the
 * wheel's lock speed and minimum locks, the wheel controller's throttle /
 * brake curves; then steer_force_init (T336).  From the conductor's race
 * setup (DynamicsConductor::virtual_20, which passes its argument; not
 * read). */
// FUNC 0x001E3EA0 input_tuning_init
void input_tuning_init(int unused)
{
    const InputTuning *c = &g_002D4628;

    g_002D4930[RAMP_THROTTLE_UP]    = 100.0f / ((f32)c->rampTime[0] * 60.0f);
    g_002D4930[RAMP_THROTTLE_DOWN]  = 100.0f / ((f32)c->rampTime[1] * 60.0f);
    g_002D4930[RAMP_BRAKE_UP]       = 100.0f / ((f32)c->rampTime[2] * 60.0f);
    g_002D4930[RAMP_BRAKE_DOWN]     = 100.0f / ((f32)c->rampTime[3] * 60.0f);
    g_002D4930[RAMP_HANDBRAKE_UP]   = 100.0f / ((f32)c->rampTime[4] * 60.0f);
    g_002D4930[RAMP_HANDBRAKE_DOWN] = 100.0f / ((f32)c->rampTime[5] * 60.0f);
    if (g_002D4930[0] > 1.0f)
        g_002D4930[0] = 1.0f;
    if (g_002D4930[1] > 1.0f)
        g_002D4930[1] = 1.0f;
    if (g_002D4930[2] > 1.0f)
        g_002D4930[2] = 1.0f;
    if (g_002D4930[3] > 1.0f)
        g_002D4930[3] = 1.0f;
    if (g_002D4930[4] > 1.0f)
        g_002D4930[4] = 1.0f;
    if (g_002D4930[5] > 1.0f)
        g_002D4930[5] = 1.0f;

    /* (72.4%: retail stores these in exactly this order; here sched1 puts
     * the last use of each shared value first.  Tried: the speeds
     * recomputed per statement, read back from [2], stored through
     * pointers, other array declarations; ref/matching/u079/notes.md) */
    {
        f32 analog = (f32)(c->analogSteerSpeed * 10) * DEG2RAD;
        f32 speed = (f32)(c->steerSpeed * 10) * DEG2RAD;
        f32 back = (f32)(c->steerSpeedBack * 10) * DEG2RAD;

        g_00352990 = (f32)c->analogPedalMin * 0.01f;
        g_0049D0A0[2] = analog;
        g_0049D080[2] = speed;
        g_0049D090[2] = back;
        g_00352970 = (f32)c->steerSpring;
        g_00352974 = (f32)c->steerDamping * 0.1f;
        g_00352978 = (f32)c->centreSpring;
        g_0049D080[0] = speed;
        g_0049D080[1] = (f32)(c->steerSpeedClass1 * 10) * DEG2RAD;
        g_0049D090[0] = back;
        g_0049D090[1] = (f32)(c->steerSpeedClass1 * 10) * DEG2RAD;
        g_0049D0A0[0] = analog;
        g_0035297C = (f32)(c->wheelLockSpeed * 10) * DEG2RAD;
        g_00352980 = (f32)c->minLock1[0] * DEG2RAD;
        g_00352984 = (f32)c->minLock1[1] * DEG2RAD;
        g_0049D0A0[1] = analog;
    }
    responseInit(&g_0049D060, c->throttleCurveMode, c->throttleCurveN);
    responseInit(&g_0049D070, c->brakeCurveMode, c->brakeCurveN);
    steer_force_init();
}

/* DynCarState.clutch (1 engaged) from the inputs: 0 while the car is held,
 * in the pit box or still; with TRANS_MANUAL_CLUTCH the brake pedal is the
 * clutch (the brake is then 0, and a pressed clutch also clears the shift
 * and the reverse); with TRANS_MANUAL_6 a shift or the second throttle
 * opens it; a rear-driven car opens it with the handbrake above 0.8; with
 * the clutch free it engages below 0.25 brake, else it stays engaged unless
 * the rpm falls below the idle. */
// FUNC 0x001E4180 car_clutch_from_input
void car_clutch_from_input(RaceCarDynamics *car, CarInput *in, CarCtrlOut *out, int transMode)
{
    CarDynState *st = &car->st;
    CarParams   *p = &car->params;

    if (st->stopped != 0 || st->pitTimer != 0 || st->ctrlMode == CTRL_STILL) {
        st->clutch = 0.0f;
        return;
    }
    if (transMode == TRANS_MANUAL_CLUTCH) {
        f32 pedal = st->brake;
        st->brake = 0.0f;
        st->clutch = 1.0f - pedal;
        if (st->clutch == st->brake)        /* (reloads the 0 just stored) */
            return;
        out->shift = 0;
        out->reverse = 0;
        return;
    }
    if (transMode == TRANS_MANUAL_6) {
        if (in->shift != 0 || in->throttle2 != 0) {
            st->clutch = 0.0f;
            return;
        }
    }
    if (p->gear.drivenAxle == 1 && st->handbrake > 0.8f) {
        st->clutch = 0.0f;
        return;
    }
    if (st->clutchState == CLUTCH_FREE) {
        st->clutch = 0.0f;
        if (st->brake < 0.25f)
            st->clutch = 1.0f;
        return;
    }
    st->clutch = 1.0f;
    if (st->rpm < p->engine.idle)
        st->clutch = 0.0f;
}

/* DynCarState.steering (-100..100) and fullThrottle (the throttle at full)
 * from the raw input. */
// FUNC 0x001E4290 car_steering_display
void car_steering_display(CarDynState *st, CarInput *in)
{
    if ((in->flags & INPUT_ANALOG_STEER) == 0) {
        if (in->steer == 0)
            st->steering = 0;
        else if (in->steer > 0)
            st->steering = 100;
        else
            st->steering = -100;
    } else {
        st->steering = (s8)((in->steer * 100) / INPUT_FULL);
    }
    if ((in->flags & INPUT_ANALOG_THROTTLE) == 0) {
        if (in->throttle == 0)
            st->fullThrottle = 0;
    } else {
        if (in->throttle != INPUT_FULL)
            st->fullThrottle = 0;
    }
}

/* *v moves to target by at most step. */
// FUNC 0x001E4320 approach
void approach(f32 *v, f32 target, f32 step)
{
    f32 x = *v;

    if (x < target) {
        x += step;
        if (target < x)
            x = target;
    } else if (target < x) {
        x -= step;
        if (x < target)
            x = target;
    }
    *v = x;
}

/* The steering of the digital pads: a spring (g_00352970) with damping
 * (g_00352974) toward target, and toward the centre (the target nearer the
 * centre) the centring spring (g_00352978) and the speed speedBack; the
 * steering speed is held to +-speed; the target is not overshot. */
// FUNC 0x001E4368 car_steer_toward
void car_steer_toward(RaceCarDynamics *car, f32 target, f32 speed, f32 speedBack)
{
    CarDynState *st = &car->st;
    f32 dt = car->dt;
    f32 vel = st->steerSpeed;
    f32 accel = (target - st->steer) * g_00352970 - vel * g_00352974;
    f32 next;

    if ((st->steer > 0.0f && target < st->steer) || (st->steer < 0.0f && st->steer < target)) {
        speed = speedBack;
        accel -= st->steer * g_00352978;
    }
    vel = vel + accel * dt;
    if (speed < vel) {
        vel = speed;
    } else {
        speed = -speed;
        if (vel < speed)
            vel = speed;
    }
    next = st->steer + vel * dt;
    if ((st->steer <= target && target <= next) || (target <= st->steer && next <= target)) {
        st->steer = target;
        st->steerSpeed = 0.0f;
    } else {
        st->steer = next;
        st->steerSpeed = vel;
    }
}

/* Callers: DynCar_initialize (T330). */
// FUNC 0x001E4468 car_set_steer_assist
void car_set_steer_assist(RaceCarDynamics *car, int v)
{
    car->st.steerAssist = v;
}

// FUNC 0x001E4470 car_get_steer_assist
s8 car_get_steer_assist(RaceCarDynamics *car)
{
    return car->st.steerAssist;
}

/* The inputs of the pad driver.
 *   The steering class: 1 on a dirt course (course kind 2; for the pads only
 *   when both front wheels are on loose ground, g_002D4948), 2 on course
 *   kind 1, else 0.  It picks the lock curve over the speed (steer2 for
 *   class 1, else steer0 for the pads, steer1 for the wheel), the steering
 *   speeds and the wheel's minimum lock (by DynCarState.steerAssist: 1 / 3,
 *   2; 3 and 4 also free the lock from the front wheels' direction).  When
 *   the front wheels move at an angle (velAngle) beyond the lock, the lock
 *   follows it (steerOver: then the steering's share of it, 0..1).  At most
 *   CarDynamics.steerLock.
 *   The steering: digital (and the pressure buttons) through
 *   car_steer_toward, the wheel directly (its lock following at
 *   g_0035297C), the stick through car_steer_toward with its own speed.
 *   The throttle: ramps for a key, the analog value lifted by the pedal
 *   minimum; the second throttle (the reverse) combined by the transmission
 *   mode (the larger of the two in the automatic modes); the wheel's curve
 *   g_0049D060 for an analog throttle.
 *   The brake and the handbrake: ramps or the analog value (the wheel's
 *   brake through g_0049D070); then the clutch. */
// FUNC 0x001E4478 car_process_input
void car_process_input(RaceCarDynamics *car, CarInput *in, CarCtrlOut *out)
{
    CarParams   *p = &car->params;
    CarDynState *st = &car->st;
    int steerClass = 0;
    f32 speed = st->speedX;
    u8  frontClass0 = g_002D4948[st->wheel[0].surface];
    u8  frontClass1 = g_002D4948[st->wheel[1].surface];
    f32 dt = car->dt;
    f32 frontVelAngle = (st->wheel[0].velAngle + st->wheel[1].velAngle) * 0.5f;
    u8  surfaceClass = ((frontClass0 ^ frontClass1) == 0) ? frontClass0 : 0;
    f32 lockMax;
    f32 lockMin;
    int lockFollows;
    int transMode;
    int analog;
    int analog2;
    u16 throttle;
    u16 throttle2;
    u8  key;
    f32 v;

    switch (car->dc->getCourseKind()) {
    case 1:
        steerClass = 2;
        break;
    case 2:                         /* dirt */
        steerClass = 1;
        break;
    case 0:
        steerClass = 0;
        break;
    }
    if (speed < 0.0f)
        speed = 0.0f;
    if (in->device != INPUT_DEV_WHEEL) {
        if (steerClass == 1)
            steerClass = surfaceClass;
        if (steerClass != 1)
            lockMax = curveLookupLinear(&p->steer0, speed);
        else
            lockMax = curveLookupLinear(&p->steer2, speed);
    } else {
        if (steerClass != 1)
            lockMax = curveLookupLinear(&p->steer1, speed);
        else
            lockMax = curveLookupLinear(&p->steer2, speed);
    }
    lockFollows = 1;

    if (in->device == INPUT_DEV_WHEEL) {
        f32 minLock;

        switch (car_get_steer_assist(car)) {
        case 3:
            lockFollows = 0;
            /* fall through */
        case 1:
            minLock = (steerClass == 1) ? g_00352984 : g_00352980;
            if (lockMax < minLock)
                lockMax = minLock;
            break;
        case 2:
            minLock = (steerClass == 1) ? g_0035298C : g_00352988;
            if (lockMax < minLock)
                lockMax = minLock;
            break;
        case 4:                     /* (compared with in->device's register, 4 here) */
            lockFollows = 0;
            break;
        }
    }
    lockMin = -lockMax;

    if (lockFollows != 0) {
        if (lockMin <= frontVelAngle && frontVelAngle <= lockMax) {
            st->steerOver = 0.0f;
        } else if (frontVelAngle > 0.0f) {
            lockMax = frontVelAngle;
            st->steerOver = lockMax;
        } else if (frontVelAngle < 0.0f) {
            lockMin = frontVelAngle;
            st->steerOver = frontVelAngle;
        }
    }
    if (p->steerLock < lockMax)
        lockMax = p->steerLock;
    if (lockMin < -p->steerLock)
        lockMin = -p->steerLock;
    st->unk_420 = 1.0f;

    /* ---- the steering */
    if ((in->flags & INPUT_ANALOG_STEER) == 0) {
        f32 target;

        out->steerKind = STEER_KIND_DIGITAL;
        st->steerMax = lockMax;
        st->steerMin = lockMin;
        target = lockMax;
        if (in->steer <= 0) {
            target = 0.0f;
            if (in->steer < 0)
                target = lockMin;
        }
        car_steer_toward(car, target, g_0049D080[steerClass], g_0049D090[steerClass]);
    } else if (in->device == INPUT_DEV_WHEEL) {
        s16 wheel;
        u16 prevWheel;
        f32 x;
        f32 steer;

        out->steerKind = STEER_KIND_WHEEL;
        wheel = in->steer;
        prevWheel = car->steerIn;
        car->steerIn = wheel;
        car->steerInPrev = prevWheel;
        x = (f32)wheel * INPUT_ONE;
        switch (car_get_steer_assist(car)) {
        case 0: case 1: case 2: case 3:
            approach(&st->steerMax, lockMax, g_0035297C * dt);
            approach(&st->steerMin, lockMin, g_0035297C * dt);
            if (x >= 0.0f)
                steer = st->steerMax * x;
            else
                steer = -st->steerMin * x;
            break;
        case 4:                     /* (compared with in->device's register, 4 here) */
            steer = p->steerLock * x;
            break;
        default:
            steer = 0.0f;
            break;
        }
        st->steer = steer;
    } else if ((u8)(in->device - INPUT_DEV_STICK) < 2) {   /* the stick, the pressure buttons */
        s16 raw;
        f32 x;
        f32 target;

        st->steerMax = lockMax;
        st->steerMin = lockMin;
        raw = in->steer;
        x = (f32)raw * INPUT_ONE;
        if (x >= 0.0f)
            target = lockMax * x;
        else
            target = -lockMin * x;
        if (raw != 0) {
            if (in->device == INPUT_DEV_PRESSURE)
                out->steerKind = STEER_KIND_DIGITAL;
            else
                out->steerKind = STEER_KIND_ANALOG;
        }
        if (out->steerKind == STEER_KIND_DIGITAL)
            car_steer_toward(car, target, g_0049D080[steerClass], g_0049D090[steerClass]);
        else
            car_steer_toward(car, target, g_0049D0A0[steerClass], g_0049D0A0[steerClass]);
    }

    if (st->steerOver != 0.0f) {
        f32 share = st->steer / st->steerOver;
        st->steerOver = share;
        if (share < 0.0f)
            st->steerOver = 0.0f;
        else if (share > 1.0f)
            st->steerOver = 1.0f;
    }

    /* ---- the throttle */
    transMode = 0;
    if (st->driver != DRIVER_AUTO && st->ctrlMode == CTRL_DRIVER)
        transMode = st->transMode;
    analog = 1;
    throttle = (u16)in->throttle;
    if ((in->flags & INPUT_ANALOG_THROTTLE) == 0) {
        analog = 0;
        throttle = (u16)(throttle << 12);       /* a key: 1 -> INPUT_FULL */
    }
    throttle2 = (u16)in->throttle2;
    analog2 = 1;
    if ((in->flags & INPUT_ANALOG_THROTTLE2) == 0) {
        analog2 = 0;
        throttle2 = (u16)(throttle2 << 12);
    }
    if (st->pitTimer != 0)
        throttle2 = 0;
    switch (transMode) {
    case TRANS_AUTO: case TRANS_SEMI_AUTO: case TRANS_AUTO_SPEED:
        /* the larger of the two throttles; the second one means the reverse */
        if (throttle2 < throttle) {
            throttle2 = 0;
        } else {
            analog = analog2;
            throttle = throttle2;
        }
        break;
    case TRANS_SEQUENTIAL:
        throttle2 = 0;
        break;
    case TRANS_MANUAL_CLUTCH: case TRANS_MANUAL: case TRANS_MANUAL_6:
        break;
    }
    out->reverse = (throttle2 != 0);

    v = 0.0f;
    if (analog == 0) {
        v = st->throttle;
        if (throttle != 0) {
            v += g_002D4930[RAMP_THROTTLE_UP];
            if (v > 1.0f)
                v = 1.0f;
            if (throttle2 == 0) {
                out->throttleKey = THROTTLE_KEY_ONE;
                key = THROTTLE_KEY_ONE;
            } else {
                out->throttleKey = THROTTLE_KEY_BOTH;
                key = THROTTLE_KEY_BOTH;
            }
        } else {
            v -= g_002D4930[RAMP_THROTTLE_DOWN];
            if (v < 0.0f) {
                out->throttleKey = THROTTLE_KEY_NONE;
                v = 0.0f;
            }
            key = out->throttleKey;
            if (key == THROTTLE_KEY_BOTH)
                out->reverse = 1;
        }
    } else if (throttle != 0) {
        v = (f32)throttle * INPUT_ONE;
        v += g_00352990 * (1.0f - v);
        key = THROTTLE_KEY_NONE;
        out->throttleKey = THROTTLE_KEY_NONE;
    } else {
        /* released: a key's throttle still running down */
        key = out->throttleKey;
        if (key != THROTTLE_KEY_NONE) {
            v = st->throttle - g_002D4930[RAMP_THROTTLE_DOWN];
            if (v <= 0.0f) {
                out->throttleKey = THROTTLE_KEY_NONE;
                v = 0.0f;
                key = THROTTLE_KEY_NONE;
            }
            if (key == THROTTLE_KEY_BOTH)
                out->reverse = 1;
        }
    }
    if (key == THROTTLE_KEY_NONE && in->device == INPUT_DEV_WHEEL)
        v = responseEval(&g_0049D060, v, NULL);
    st->throttle = v;

    /* ---- the brake */
    if ((in->flags & INPUT_ANALOG_BRAKE) == 0) {
        if (in->brake != 0) {
            f32 b = st->brake + g_002D4930[RAMP_BRAKE_UP];
            st->brake = b;
            if (b > 1.0f)
                st->brake = 1.0f;
        } else {
            f32 b = st->brake - g_002D4930[RAMP_BRAKE_DOWN];
            st->brake = b;
            if (b < 0.0f)
                st->brake = 0.0f;
        }
    } else {
        f32 x = (f32)in->brake * INPUT_ONE;
        if (x != 0.0f)
            x += g_00352990 * (1.0f - x);
        if (in->device == INPUT_DEV_WHEEL)
            x = responseEval(&g_0049D070, x, NULL);
        st->brake = x;
    }

    if (st->pitTimer == 0)
        out->shift = in->shift;
    else
        out->shift = 0;

    /* ---- the handbrake */
    if ((in->flags & INPUT_ANALOG_HANDBRAKE) == 0) {
        if (in->handbrake != 0) {
            f32 h = st->handbrake + g_002D4930[RAMP_HANDBRAKE_UP];
            st->handbrake = h;
            if (h > 1.0f)
                st->handbrake = 1.0f;
        } else {
            f32 h = st->handbrake - g_002D4930[RAMP_HANDBRAKE_DOWN];
            st->handbrake = h;
            if (h < 0.0f)
                st->handbrake = 0.0f;
        }
    } else {
        f32 x = (f32)in->handbrake * INPUT_ONE;
        if (x != 0.0f)
            x += g_00352990 * (1.0f - x);
        st->handbrake = x;
    }

    car_clutch_from_input(car, in, out, transMode);
}

/* The pad driver's frame: the inputs into the run state's CarCtrlOut, the
 * throttle without traction control so far, the four wheels' brakes. */
// FUNC 0x001E4CB0 car_input_pad
void car_input_pad(RaceCarDynamics *car, CarInput *in)
{
    CarDynState *st;

    car_process_input(car, in, &car->st.ctrl);
    st = &car->st;
    st->throttleOut = st->throttle;
    car_set_wheel_brakes(car, st->brake);
}

/* The computer driver's outputs into the car; in CTRL_STILL the handbrake
 * on. */
// FUNC 0x001E4CF0 car_input_ai_apply
void car_input_ai_apply(RaceCarDynamics *car)
{
    CarDynState *st;

    ai_apply_outputs(&car->ai);
    st = &car->st;
    if (st->ctrlMode == CTRL_STILL)
        st->handbrake = 1.0f;
}

/* Once per frame (from the conductor's car step, F_001F39D8): reads the
 * driver's input (RaceInput::read; the conductor's slot 19 sees it unless
 * the car has no controller), then by DynCarState.driver: the pad (the
 * computer while ctrlMode != CTRL_DRIVER and no pit stop runs), the
 * computer with the steering display (1), the computer (2). */
// FUNC 0x001E4D38 car_input_update
void car_input_update(RaceCarDynamics *car)
{
    CarInput     in;                /* sp+0x00 */
    CarDynState *st = &car->st;

    car->input->read(&in);
    if (st->noController == 0)
        car->dc->recordOneLapGhostControl(car->input, &in);
    st->fullThrottle = 1;
    switch (st->driver) {
    case DRIVER_PAD:
        car_steering_display(st, &in);
        if (st->ctrlMode == CTRL_DRIVER || st->pitTimer != 0) {
            car_input_pad(car, &in);
            ai_update_gear(&car->ai);
        } else {
            car_input_ai(car, &in);
            ai_select_gear(&car->ai);
            car_input_ai_apply(car);
        }
        break;
    case DRIVER_AUTO:
        car_steering_display(st, &in);
        car_input_ai(car, &in);
        car_input_ai_apply(car);
        break;
    case DRIVER_COMPUTER:
        car_input_ai(car, &in);
        car_input_ai_apply(car);
        break;
    }
}
