/* T343 - the gearbox: the gear a speed calls for, and the shift logic of the
 * transmission modes (DynCarState.transMode).  The functions return the gear
 * to shift to, or GEAR_KEEP (-1) for no change; car_gear_select (T296)
 * applies it with car_set_gear.  Gear 0 is the reverse, 1..GearCalc.gearCount
 * the forward gears.  The shift buttons are CarCtrlOut's reverse / shift
 * (GearInput, s8: read with lb). */
#include "game/car/physics/T343_gear_select.h"
#include "game/car/physics/drivetrain_internal.h"

// FUNC 0x001E4E78 car_set_gear
void car_set_gear(CarRunState *run, int gear)
{
    run->gear = gear;
}

/* The lowest forward gear whose speed at (revLimit - 500) rpm is above
 * `speed': the top gear if none, 0 (the reverse) for a negative speed.
 * Callers: car_shift_auto, car_shift_semi_auto and two outside the unit. */
// FUNC 0x001E4E80 car_gear_for_speed
int car_gear_for_speed(RaceCarState *car, f32 speed)
{
    GearCalc *gc = &car->params.gear;
    int top;
    int g;

    if (speed < 0.0f)
        return 0;
    top = gc->gearCount;
    for (g = 1; g < top; g++) {
        if (speed < (f32)(gc->revLimit - 500) * RPM_TO_RADS / gc->ratioPerRadius[g])
            return g;
    }
    return top;
}

/* transMode 2's gear offset: the throttle asks for (1 - throttle) x 3 gears
 * above the speed's gear (0 at full brake, no request with the throttle
 * closed); the offset follows after 13 frames of a lower request or 61 of a
 * higher one, one gear at a time.  offsetCooldown is set to 60 and counted
 * down but not read. */
// FUNC 0x001E4F10 car_update_gear_offset
void car_update_gear_offset(CarRunState *st)
{
    int want = -1;                          /* -1: no request */
    int cur;

    if (!(st->throttle <= 0.0f))
        want = (int)((1.0f - st->throttle) * 3.0f);
    if (st->brake == 1.0f)
        want = 0;
    if (st->offsetCooldown != 0)
        st->offsetCooldown--;
    cur = st->gearOffset;
    if (want == cur || want == -1) {
        st->offsetCount = 0;
        return;
    }
    if (want < cur) {
        if (++st->offsetCount >= 13) {
            st->offsetCooldown = 60;
            st->offsetCount = 0;
            st->gearOffset = st->gearOffset - 1;
        }
    } else {
        if (++st->offsetCount >= 61) {
            st->offsetCooldown = 60;
            st->offsetCount = 0;
            st->gearOffset = st->gearOffset + 1;
        }
    }
}

/* The automatic (transMode 0: by the rpm; 2, bySpeed: the speed's gear plus
 * gearOffset).  The reverse on its button; 1st when rolling forward in the
 * reverse or starting off on the throttle; the reverse when rolling
 * backwards off it; an upshift above (redline + 500) rpm.  Then the shift
 * buttons by g_002D4628.manualMode (1: a pressed button keeps the gear; 2:
 * the buttons shift, then the automatic waits shiftHold = 240 frames), and
 * from here on only with the clutch locked.  By the rpm: down below the rpm
 * an upshift into this gear gives (shiftRpm, moved by the pedals), up above
 * the rev limit (revLimit), unless a driven wheel is in the air or spins
 * past its tyre's peak: slip < peakSlipNeg x slipGrip (a negative slip is a
 * tread faster than the ground, T296 tyre_slip_ratio), so that a wheelspin's
 * rpm does not shift up. */
// FUNC 0x001E4FF8 car_shift_auto
int car_shift_auto(RaceCarState *car, GearInput *in, int bySpeed)
{
    CarDynamics *p = &car->params;
    GearCalc *gc = &car->params.gear;
    CarRunState *st = &car->st;
    int gear = st->gear;
    f32 speed;
    int downRpm, upRpm;
    int checkFront, checkRear;              /* the driven axles' wheels */
    int next;
    int i;

    if (st->shiftHold != 0)
        st->shiftHold--;
    if (bySpeed != 0)
        car_update_gear_offset(st);
    if (st->shiftTimer != 0) {
        if (--st->shiftTimer == 0 && in->shift != 0)
            st->shiftTimer = 1;             /* a held button waits for its release */
        return GEAR_KEEP;
    }
    if (in->reverse != 0)
        return gear != 0 ? 0 : GEAR_KEEP;

    speed = st->speedX;
    if (KMH_TO_MS < speed && gear == 0)     /* rolling forward in the reverse */
        return 1;
    if (st->throttleOut != 0.0f) {
        if (speed < KMH_TO_MS)              /* starting off */
            return gear != 1 ? 1 : GEAR_KEEP;
    } else if (speed < -KMH_TO_MS && gear != 0) {   /* rolling backwards off the throttle */
        return 0;
    }
    /* over the redline + 500 rpm in this gear (never in the reverse) */
    if (gear > 0 && gear < gc->gearCount &&
        (f32)(p->engine.redline + 500) * RPM_TO_RADS / gc->ratioPerRadius[gear] < speed)
        return gear + 1;

    switch (g_002D4628.manualMode) {
    case 0:
        break;
    case 1:
        if (in->shift != 0)
            return GEAR_KEEP;
        break;
    case 2:
        if (in->shift > 0 && gear < p->gear.gearCount) {
            st->shiftTimer = 1;
            st->shiftHold = 240;
            return gear + 1;
        }
        if (in->shift < 0 && !(gear < 2)) {
            st->shiftTimer = 1;
            st->shiftHold = 240;
            return gear - 1;
        }
        if (st->shiftHold != 0)
            return GEAR_KEEP;
        break;
    }
    if (st->clutchState != CLUTCH_LOCKED)
        return GEAR_KEEP;

    if (bySpeed != 0) {
        int top, want;

        if (gear == 0)
            return GEAR_KEEP;
        want = car_gear_for_speed(car, st->speedX) + st->gearOffset;
        top = p->gear.gearCount;
        if (top < want)
            want = top;
        return gear == want ? GEAR_KEEP : want;
    }

    if (gear == 0)
        return GEAR_KEEP;
    if (gear == 1) {                        /* (the code compares with clutchState, known to be 1) */
        if (gc->gearCount == gear)
            return GEAR_KEEP;
        downRpm = -20000;
        upRpm = gc->revLimit;
    } else {
        upRpm = 20000;
        if (gear < gc->gearCount)
            upRpm = gc->revLimit;
        downRpm = gc->shiftRpm[gear - 2];   /* the rpm after the shift gear - 1 -> gear */
        if (st->throttle < 0.05f)
            downRpm += g_002D4628.liftDown * 100;
        if (0.8f < st->brake)
            downRpm += g_002D4628.brakeDown * 100;
        if (0.25f < st->throttle)
            downRpm -= g_002D4628.accelDown * 100;
    }

    /* the wheels an upshift checks: the front (0, 1) and / or the rear (2, 3) */
    checkRear = 0;
    checkFront = 0;
    switch (gc->driveType) {
    case DRIVE_FRONT:
    case DRIVE_AWD_FRONT:
        checkFront = 1;
        checkRear = 0;
        break;
    case DRIVE_REAR:
    case DRIVE_AWD_SPLIT:
    case DRIVE_AWD_SPEED:
    case DRIVE_AWD_STEER:
        checkFront = 0;
        checkRear = 1;
        break;
    case DRIVE_AWD_CENTRE:
    case DRIVE_AWD_FIXED:
        checkFront = 1;
        checkRear = 1;
        break;
    }

    if (st->rpm < downRpm)
        return st->gear - 1;
    if (upRpm < st->rpm) {
        next = st->gear + 1;
        for (i = 0; i < 4; i++) {
            CarWheelState *w = &st->wheel[i];

            if (checkFront == 0 && i < 2)
                continue;
            if (checkRear == 0 && !(i < 2))
                break;
            if (w->load == 0.0f)                /* in the air */
                return GEAR_KEEP;
            if (w->slip < CarDynamics_getTyre(p, i, w->surface)->peakSlipNeg * w->slipGrip)
                return GEAR_KEEP;               /* it spins */
        }
        return next;
    }
    return GEAR_KEEP;
}

/* transMode 1: the reverse on its button, from the reverse on the throttle
 * the speed's gear (at least 1st), else the shift buttons. */
// FUNC 0x001E5458 car_shift_semi_auto
int car_shift_semi_auto(RaceCarState *car, GearInput *in)
{
    CarRunState *st = &car->st;
    CarDynamics *p = &car->params;
    int gear = st->gear;

    if (in->reverse != 0)
        return gear != 0 ? 0 : GEAR_KEEP;
    if (gear == 0 && st->throttle != 0.0f) {
        int g = car_gear_for_speed(car, st->speedX);

        return g == 0 ? 1 : g;
    }
    if (st->shiftTimer != 0) {
        if (--st->shiftTimer == 0 && in->shift != 0)
            st->shiftTimer = 1;
        return GEAR_KEEP;
    }
    if (in->shift > 0 && gear < p->gear.gearCount) {
        st->shiftTimer = 1;
        return gear + 1;
    }
    if (in->shift < 0 && !(gear < 2)) {
        st->shiftTimer = 1;
        return gear - 1;
    }
    return GEAR_KEEP;
}

/* transMode 3, 4, 6: the shift buttons, the reverse on its own button. */
// FUNC 0x001E5540 car_shift_manual
int car_shift_manual(RaceCarState *car, GearInput *in)
{
    CarRunState *st = &car->st;
    CarDynamics *p = &car->params;
    int gear = st->gear;

    if (st->shiftTimer != 0) {
        if (--st->shiftTimer == 0 && (in->shift != 0 || in->reverse != 0))
            st->shiftTimer = 1;
    } else {
        if (in->reverse != 0) {
            if (gear == 0)
                return GEAR_KEEP;
            st->shiftTimer = 1;
            return 0;
        }
        if (in->shift > 0 && gear < p->gear.gearCount) {
            st->shiftTimer = 1;
            return gear + 1;
        }
        if (in->shift < 0 && !(gear < 2)) {
            st->shiftTimer = 1;
            return gear - 1;
        }
    }
    return GEAR_KEEP;
}

/* transMode 5: the shift buttons, the reverse below 1st. */
// FUNC 0x001E5600 car_shift_sequential
int car_shift_sequential(RaceCarState *car, GearInput *in)
{
    CarRunState *st = &car->st;
    CarDynamics *p = &car->params;
    int gear = st->gear;

    if (st->shiftTimer != 0) {
        if (--st->shiftTimer == 0 && in->shift != 0)
            st->shiftTimer = 1;
    } else {
        if (in->shift > 0 && gear < p->gear.gearCount) {
            st->shiftTimer = 1;
            return gear + 1;
        }
        if (in->shift < 0 && gear > 0) {
            st->shiftTimer = 1;
            return gear - 1;
        }
    }
    return GEAR_KEEP;
}

/* The gear request of a frame (car_gear_select, T296, passes &st.ctrl). */
// FUNC 0x001E5690 car_select_gear
int car_select_gear(RaceCarState *car, GearInput *in)
{
    CarRunState *st = &car->st;
    int mode;

    if (st->pitTimer != 0)                  /* the pit stop: 1st gear */
        return st->gear != 1 ? 1 : GEAR_KEEP;
    if (st->driver == 1 || st->ctrlMode != 0)
        mode = 0;                           /* the automatic for driver 1 and a control mode */
    else
        mode = st->transMode;
    switch (mode) {
    case 0:
        return car_shift_auto(car, in, 0);
    case 2:
        return car_shift_auto(car, in, 1);
    case 1:
        return car_shift_semi_auto(car, in);
    case 3:
    case 4:
    case 6:
        return car_shift_manual(car, in);
    case 5:
        return car_shift_sequential(car, in);
    default:
        return GEAR_KEEP;
    }
}
