/* T344 - the drivetrain of a frame (car_drivetrain_step, T296, calls
 * car_wheel_torques, car_active_yaw_control, car_drive_torque and
 * car_axle_differentials in this order): the wheels' torques, the clutch and
 * the engine speed, the torque split between the axles (GearCalc.driveType),
 * the axle speeds (rad/s), and the differential of each axle
 * (GearCalc.diffType), which sets the wheels' spin (m/s: tyreRadius x the
 * axle speed).  DRIVE_LOCKED (98000) is the code's "locked" marker. */
#include "game/car/physics/T344_drivetrain.h"
#include "game/car/physics/drivetrain_internal.h"

/* Each wheel's torque: -tyreRadius[axle] x (driveOut + brakeOut), less the
 * brake torque against the spin. */
// FUNC 0x001E5768 car_wheel_torques
void car_wheel_torques(RaceCarState *car)
{
    CarRunState *st = &car->st;
    int i;

    for (i = 0; i < 4; i++) {
        TyreForce *tf = &st->tyreForce[i];
        f32 torque = -car->params.gear.tyreRadius[i >> 1] * (tf->driveOut + tf->brakeOut);
        f32 spin;

        tf->torque = torque;
        spin = st->wheel[i].spin;
        if (spin != 0.0f) {
            if (0.0f < spin)
                tf->torque = torque - tf->brakeTorque;
            else
                tf->torque = torque + tf->brakeTorque;
        }
    }
}

/* diffType 6 and 7 (? an active yaw control): between 10 and 180 km/h, the
 * steering (in full at 15 degrees: x 12 / pi) times diffA[axle] moves torque
 * between the axle's wheels (9/16 off one, 7/16 onto the other) towards a
 * target of their mean spin +- 1/8 of it by the steering's side.
 * Matching (88.0%): the switch's 0..5 cases give retail's range tests (< 0,
 * < 6, < 8); tl / tr are user pointers (retail's daddu copies of the givs);
 * small before big orders the hoisted constants.  Left: retail keeps a copy
 * of the right spin (mov.s f4,f2: compare 1 reads the copy, compare 2 the
 * load) and strength-reduces the wheels from &wheel[right].spin (left at
 * -0xA4); every FP register is one higher because of the copy.  Tried: the
 * spin read through a pointer / car->st / st, in each compare and in the sum
 * (108 combinations), an inline helper for the update (its argument copies
 * coalesce), w[-1] for the left wheel. */
// FUNC 0x001E57F8 car_active_yaw_control
void car_active_yaw_control(RaceCarState *car)
{
    CarRunState *st = &car->st;
    GearCalc *gc = &car->params.gear;
    int axle;

    for (axle = 0; axle < 2; axle++) {
        int left = axle * 2;
        int right = axle * 2 + 1;
        f32 steer, amount, rightSpin, target, bias, big, small;
        TyreForce *tl, *tr;

        switch (gc->diffType[axle]) {
        case DIFF_NONE:
        case DIFF_LOCKED:
        case DIFF_OPEN:
        case DIFF_LSD:
        case DIFF_LSD2:
        case DIFF_VISCOUS:
            continue;
        case DIFF_AYC:
        case DIFF_AYC2:
            break;
        default:
            continue;
        }
        if (st->speedX < 2.777778f)
            continue;
        if (50.000004f < st->speedX)
            continue;
        steer = st->steer;
        if (steer < 0.0f)
            steer = -steer;
        steer = steer * 3.8197186f;
        if (1.0f < steer)
            steer = 1.0f;
        amount = steer * gc->diffA[axle];
        if (amount == 0.0f)
            continue;
        tl = &st->tyreForce[left];
        tr = &st->tyreForce[right];
        rightSpin = st->wheel[right].spin;
        target = (st->wheel[left].spin + rightSpin) * 0.5f;
        bias = target * 0.125f;
        if (0.0f < st->steer)
            target = target + bias;
        else if (st->steer < 0.0f)
            target = target - bias;
        small = amount * 0.4375f;
        big = amount * 0.5625f;
        if (rightSpin < target) {
            tr->torque = tr->torque + small;
            tl->torque = tl->torque - big;
        } else if (target < rightSpin) {
            tr->torque = tr->torque - big;
            tl->torque = tl->torque + small;
        }
    }
}

/* DRIVE_AWD_SPEED's front share.  DRIVE_LOCKED when awdTorque != 0 and the
 * car is in the reverse with the clutch locked, the clutch is free, or, with
 * |latGripUse| <= 0.25, the clutch slips (state 2), the brake is over 0.75 or
 * drive and drive + rearWheelTq have one sign; else |the axle speed
 * difference| x awdSplit x k (k: 1 down to 0.25 as |latGripUse| goes
 * 0.25..1), at most 1 (DRIVE_LOCKED if awdTorque != 0).
 * Quirk kept: frontWheelTq is not used. */
// FUNC 0x001E5998 car_centre_coupling
f32 car_centre_coupling(RaceCarState *car, f32 drive, f32 frontWheelTq, f32 rearWheelTq)
{
    CarRunState *st = &car->st;
    CarDynamics *p = &car->params;
    f32 slipEst, k, limit, share;

    slipEst = st->latGripUse;                  /* (u081: ? a slip estimate) */
    if (slipEst < 0.0f)
        slipEst = -slipEst;
    if (slipEst <= 0.25f)
        k = 1.0f;
    else if (1.0f <= slipEst)
        k = 0.25f;
    else
        k = 1.0f - ((slipEst - 0.25f) * 0.75f) / 0.75f;
    limit = p->gear.awdTorque;
    if (limit != 0.0f) {
        u8 clutch;

        /* (egcs tests gear and clutchState with one masked word load at +0x434) */
        if (st->gear == 0 && st->clutchState == CLUTCH_LOCKED)
            return DRIVE_LOCKED;
        clutch = st->clutchState;
        if (clutch == CLUTCH_FREE)
            return DRIVE_LOCKED;
        if (k == 1.0f) {
            if (clutch == CLUTCH_SLIP)
                return DRIVE_LOCKED;
            if (0.75f < st->brake)
                return DRIVE_LOCKED;
            if (0.0f < drive && 0.0f < drive + rearWheelTq)
                return DRIVE_LOCKED;
            if (drive < 0.0f && drive + rearWheelTq < 0.0f)
                return DRIVE_LOCKED;
        }
    }
    {
        f32 front = st->axleSpeed[0];
        f32 rear = st->axleSpeed[1];

        share = rear - front;               /* (front read first: retail's load order) */
    }
    if (share < 0.0f)
        share = -share;
    share *= p->gear.awdSplit;
    share *= k;
    if (1.0f <= share) {
        share = 1.0f;
        if (limit != 0.0f)
            share = DRIVE_LOCKED;
    }
    return share;
}

/* The clutch torque's factor for a slip (rad/s): 1 up to 500 rpm, then
 * + 1 per 8000 rpm, at most 1.125.  (52.35988 = 500 rpm, 837.75806 = 8000
 * rpm in rad/s.) */
// FUNC 0x001E5B30 clutch_slip_factor
f32 clutch_slip_factor(f32 slip)
{
    if (slip < 0.0f)
        slip = -slip;
    slip = slip - 52.35988f;
    if (slip < 0.0f)
        slip = 0.0f;
    slip = slip / 837.75806f + 1.0f;
    if (1.125f <= slip)
        slip = 1.125f;
    return slip;
}

/* The drive torque of a frame.  The clutch: locked passes the engine torque
 * (engine_update); slipping passes the clutch torque (CLUTCH_SHIFT:
 * GearCalc.clutchTorque, else engineTorque / 20 plus the engine torque x the slip
 * factor) with the slip's sign, and the difference drives the engine speed.
 * Times ratio[gear], it is split between the axles by driveType: a centre
 * mode (1 a coupling that locks when the axle speeds cross, 2 the centre
 * differential, 3 both axles locked together), then the axle speeds are
 * integrated; a brake never turns an axle backwards.
 * Matching (97.1%): retail's jump tables need the explicit REAR / FRONT case
 * (the centre switch) and FIXED first in the inner one; SPLIT's two LOCKED
 * paths share one literal (the ?: form); the stores of each case in the
 * order that puts the dying value first (retail's order reversed).  Left:
 * (1) retail re-masks the clutch byte before the second test (andi
 * v0,a2,0xFF; tried int / s8 / u16 / cast / st->clutchState / an inline with
 * a u8 parameter); (2) STEER's two lui constants in swapped registers (the
 * word test as ||, &&, swapped operands); (3) the axle loop: retail computes
 * the tyreForce addresses with mult by 0x24 every pass (not strength-reduced:
 * the multiplier was a register, so not a giv), ours reduces them; index
 * variables, an inline helper, << forms did not change it; (4) CENTRE_DIFF's
 * difference register (f4): exact-looking only with h reused for the
 * difference (h = s0 - s1; ...; h *= 0.5), not kept (97.4%, unnatural). */
// FUNC 0x001E5B98 car_drive_torque
void car_drive_torque(RaceCarState *car)
{
    enum { CENTRE_NONE, CENTRE_COUPLING, CENTRE_DIFF, CENTRE_LOCKED };
    GearCalc *gc = &car->params.gear;
    CarRunState *st = &car->st;
    f32 drive = 0.0f;                       /* $f21 the clutch torque, then x the gear ratio */
    u8 clutch = st->clutchState;
    f32 dt = car->dt;
    f32 wheelTq[2];                         /* sp+0x00 the wheels' torques per axle */
    f32 inertia[2];                         /* sp+0x10 per axle (not set for driveType 3 / >= 8: never read then) */
    f32 frontTq;                            /* $f20 the torque to the front, driveType 4..7 */
    f32 *ratio;
    f32 engineInertia;
    f32 speedDiff;
    int centre;
    int axle;

    if (clutch != CLUTCH_FREE) {
        switch (gc->driveType) {
        case DRIVE_REAR:
        case DRIVE_AWD_SPLIT:
        case DRIVE_AWD_SPEED:
        case DRIVE_AWD_STEER:
            st->wheel[2].flags |= WHEEL_DRIVEN;
            st->wheel[3].flags |= WHEEL_DRIVEN;
            break;
        case DRIVE_FRONT:
            st->wheel[0].flags |= WHEEL_DRIVEN;
            st->wheel[1].flags |= WHEEL_DRIVEN;
            break;
        case DRIVE_AWD_FRONT:
            st->wheel[0].flags |= WHEEL_DRIVEN;
            st->wheel[1].flags |= WHEEL_DRIVEN;
            st->wheel[2].flags |= WHEEL_COUPLED;
            st->wheel[3].flags |= WHEEL_COUPLED;
            break;
        case DRIVE_AWD_CENTRE:
        case DRIVE_AWD_FIXED:
            st->wheel[0].flags |= WHEEL_COUPLED;
            st->wheel[1].flags |= WHEEL_COUPLED;
            st->wheel[2].flags |= WHEEL_DRIVEN;
            st->wheel[3].flags |= WHEEL_DRIVEN;
            break;
        }
    }
    if (clutch != CLUTCH_FREE) {
        f32 engineTq = engine_update(car, st->throttleOut);
        u8 state = st->clutchState;

        if (state == CLUTCH_LOCKED) {
            drive = engineTq;
        } else {
            if (state == CLUTCH_SHIFT) {
                drive = gc->clutchTorque;
            } else {
                drive = st->engineTorque * 0.05f;
                if (0.0f < engineTq)
                    drive = drive + clutch_slip_factor(st->engineWPrev - st->wheelEngineW) * engineTq;
            }
            if (st->engineWPrev < st->wheelEngineW)
                drive = -drive;
            st->engineSpeed = st->engineSpeed + (engineTq - drive) * gc->inertiaInv * dt;
        }
    }
    st->clutchTorqueAbs = drive;
    if (!(0.0f <= drive))
        st->clutchTorqueAbs = -drive;

    frontTq = 0.0f;
    ratio = gc->ratio;
    drive = drive * gc->ratio[st->gear];
    wheelTq[0] = st->tyreForce[0].torque + st->tyreForce[1].torque;
    wheelTq[1] = st->tyreForce[2].torque + st->tyreForce[3].torque;
    centre = CENTRE_NONE;
    switch (gc->driveType) {
    case DRIVE_REAR:
    case DRIVE_FRONT:
        break;
    case DRIVE_AWD_FRONT:
        centre = CENTRE_COUPLING;
        break;
    case DRIVE_AWD_CENTRE:
        centre = CENTRE_DIFF;
        break;
    case DRIVE_AWD_SPLIT:
    case DRIVE_AWD_SPEED:
    case DRIVE_AWD_STEER:
    case DRIVE_AWD_FIXED: {
        f32 want = 0.0f;                    /* $f3 the front torque asked for, or DRIVE_LOCKED */
        u8 locked;

        frontTq = 0.0f;
        switch (gc->driveType) {
        case DRIVE_AWD_FIXED:
            want = gc->awdSplit * drive;
            break;
        case DRIVE_AWD_SPLIT:
            if (gc->awdSplit == 1.0f) {
                want = DRIVE_LOCKED;
            } else if (!(0.2f < st->handbrake)) {
                if (drive <= 0.0f) {
                    want = DRIVE_LOCKED;
                } else {
                    want = (st->gear == 0 && st->clutchState == CLUTCH_LOCKED) ? DRIVE_LOCKED : gc->awdSplit * drive;
                }
            }
            break;
        case DRIVE_AWD_SPEED:
            want = car_centre_coupling(car, drive, wheelTq[0], wheelTq[1]);
            if (want != DRIVE_LOCKED)
                want = want * drive * 0.5f;
            break;
        case DRIVE_AWD_STEER:
            if (!(0.2f < st->handbrake)) {
                want = DRIVE_LOCKED;
                if (!(st->gear == 0 && st->clutchState == CLUTCH_LOCKED)) {   /* (one word load) */
                    f32 s = st->steer;

                    if (s < 0.0f)
                        s = -s;
                    s = 1.0f - s * gc->awdSplit;    /* awdSplit: 1 / the steering angle of no share */
                    if (s < 0.0f)
                        s = 0.0f;
                    if (s < 0.99f)
                        want = s;
                }
            }
            if (want != DRIVE_LOCKED)
                want = want * drive * 0.5f;
            break;
        }

        locked = st->centreLocked;
        if (locked != 0) {
            if (want == DRIVE_LOCKED) {
                centre = CENTRE_LOCKED;
            } else {
                f32 strain = (wheelTq[0] - wheelTq[1]) - drive;

                if (strain < 0.0f)
                    strain = -strain;
                if (gc->awdTorque < strain) {
                    st->centreLocked = 0;   /* the lock slips */
                    locked = 0;
                } else {
                    centre = CENTRE_LOCKED;
                }
            }
        }
        if (locked == 0) {
            if (want != DRIVE_LOCKED) {
                frontTq = want;
            } else {
                f32 limit = gc->awdTorque;

                if (st->axleSpeed[1] - st->axleSpeed[0] < 0.0f)
                    limit = -limit;
                frontTq = limit;
            }
            if (frontTq != 0.0f)
                centre = CENTRE_COUPLING;
        }
        if (centre != CENTRE_NONE) {
            st->wheel[0].flags |= WHEEL_COUPLED;
            st->wheel[1].flags |= WHEEL_COUPLED;
        }
        break;
    }
    }

    /* the engine's inertia through the gearbox, with the clutch locked */
    engineInertia = 0.0f;
    if (st->clutchState == CLUTCH_LOCKED) {
        u8 g = st->gear;

        engineInertia = gc->inertia * gc->ratio[g];
        if (g == 0)
            engineInertia = -engineInertia;
    }

    if (centre == CENTRE_LOCKED) {          /* both axles at one speed */
        f32 old = st->axleSpeed[0];

        st->axleSpeed[0] = old + ((drive + wheelTq[0] + wheelTq[1]) /
                                  (gc->axleInertia[0] + gc->axleInertia[1] + engineInertia)) * dt;
        if (st->tyreForce[0].brakeTorque + st->tyreForce[1].brakeTorque +
            st->tyreForce[2].brakeTorque + st->tyreForce[3].brakeTorque != 0.0f &&
            (st->axleSpeed[0] * old < 0.0f || old == 0.0f))
            st->axleSpeed[0] = 0.0f;
        st->axleSpeed[1] = st->axleSpeed[0];
        st->axleTorque[0] = drive * 0.5f;
        st->axleTorque[1] = drive * 0.5f;
        return;
    }
    if (centre == CENTRE_DIFF) {
        f32 speed0 = st->axleSpeed[0];
        f32 speed1 = st->axleSpeed[1];
        f32 v = (speed0 + speed1) * 0.5f;
        f32 mean = v;
        f32 diff, half, h;

        v = v + ((drive + wheelTq[0] + wheelTq[1]) /
                 (gc->axleInertia[0] + gc->axleInertia[1] + engineInertia)) * dt;
        if (st->tyreForce[0].brakeTorque + st->tyreForce[1].brakeTorque +
            st->tyreForce[2].brakeTorque + st->tyreForce[3].brakeTorque != 0.0f &&
            (v * mean < 0.0f || mean == 0.0f))
            v = 0.0f;
        diff = speed0 - speed1;
        h = diff * 0.5f;
        half = h;
        h = h + ((wheelTq[0] - wheelTq[1]) - gc->awdSplit * diff) * gc->invAxleInertia * dt;
        if (half * h < 0.0f)
            h = 0.0f;
        st->axleTorque[0] = drive * 0.5f;
        st->axleSpeed[0] = v + h;
        st->axleSpeed[1] = v - h;
        st->axleTorque[1] = drive * 0.5f;
        return;
    }

    speedDiff = 0.0f;
    switch (gc->driveType) {
    case DRIVE_FRONT:
        inertia[1] = 0.0f;
        inertia[0] = gc->axleInertia[0] + engineInertia;
        st->axleTorque[1] = 0.0f;
        st->axleTorque[0] = drive;
        break;
    case DRIVE_AWD_FRONT: {
        f32 rearTq = gc->awdSplit * (st->axleSpeed[0] - st->axleSpeed[1]);

        inertia[1] = gc->axleInertia[1];
        inertia[0] = gc->axleInertia[0] + engineInertia;
        st->axleTorque[1] = rearTq;
        st->axleTorque[0] = drive - rearTq;
        break;
    }
    case DRIVE_REAR:
        inertia[1] = gc->axleInertia[1] + engineInertia;
        inertia[0] = 0.0f;
        st->axleTorque[1] = drive;
        st->axleTorque[0] = 0.0f;
        break;
    case DRIVE_AWD_CENTRE:                  /* (CENTRE_DIFF: not reached) */
        break;
    case DRIVE_AWD_SPLIT:
    case DRIVE_AWD_SPEED:
    case DRIVE_AWD_STEER:
    case DRIVE_AWD_FIXED:
        inertia[1] = gc->axleInertia[1] + engineInertia;
        inertia[0] = gc->axleInertia[0];
        st->axleTorque[1] = drive - frontTq;
        st->axleTorque[0] = frontTq;
        break;
    }
    if (centre == CENTRE_COUPLING)
        speedDiff = st->axleSpeed[0] - st->axleSpeed[1];
    for (axle = 0; axle < 2; axle++) {
        if (gc->diffType[axle] != DIFF_NONE) {
            f32 dv = 0.0f;
            f32 in = inertia[axle];
            f32 old = st->axleSpeed[axle];
            f32 now;

            if (in != 0.0f)
                dv = ((st->axleTorque[axle] + wheelTq[axle]) / in) * dt;
            now = old + dv;
            st->axleSpeed[axle] = now;
            if (st->tyreForce[axle * 2].brakeTorque + st->tyreForce[axle * 2 + 1].brakeTorque != 0.0f &&
                (now * old < 0.0f || old == 0.0f))
                st->axleSpeed[axle] = 0.0f;
        }
    }
    if (centre == CENTRE_COUPLING) {
        f32 d = st->axleSpeed[0] - st->axleSpeed[1];

        if ((0.0f < speedDiff && d <= 0.0f) || (speedDiff < 0.0f && 0.0f <= d)) {
            /* the speeds crossed: the other axle takes the driven one's */
            u8 m = gc->drivenAxle;

            st->axleSpeed[1 - m] = st->axleSpeed[m];
            if ((u32)(gc->driveType - DRIVE_AWD_SPLIT) < 3u)    /* 4..6 */
                st->centreLocked = 1;
        }
    }
}

/* Each axle's differential.  DIFF_LSD (3 / 4): the lock torque is
 * axleTorque x diffB (or -axleTorque x diffC), at least diffA; once the wheel
 * spins meet (lsdLocked) they stay together while the wheels' torque
 * difference is within twice it.  DIFF_VISCOUS: diffB x min(dv^2, diffA).
 * Then: DIFF_NONE each wheel on its own torque (a brake stops it, never
 * turns it back), DIFF_LOCKED (or held) both at tyreRadius x axleSpeed, else
 * that +- half the spin difference.
 * Matching (99.7%): explicit cases for the jump table, tl / tr in registers
 * besides the tf[] array (the else branch and the DIFF_NONE loop read the
 * array), mode by `if (hold)` (movn) and a switch.  Left: LSD's diffA load
 * lands in $f0 (retail $f1, the dead t's register); tried k * t, a local
 * for diffA, declaration orders, `lockTq = t; lockTq *= k`. */
// FUNC 0x001E6488 car_axle_differentials
void car_axle_differentials(RaceCarState *car)
{
    CarRunState *st = &car->st;
    GearCalc *gc = &car->params.gear;
    f32 dt = car->dt;
    int axle;

    for (axle = 0; axle < 2; axle++) {
        int a = axle * 2;
        int b = axle * 2 + 1;
        int hold = 0;
        TyreForce *tf[2];
        TyreForce *tl, *tr;
        f32 dv;
        f32 lockTq;
        int mode;

        tl = &st->tyreForce[a];
        tr = &st->tyreForce[a + 1];
        tf[0] = tl;
        tf[1] = tr;
        dv = st->wheel[a].spin - st->wheel[b].spin;
        switch (gc->diffType[axle]) {
        case DIFF_LOCKED:
            hold = 1;
            break;
        case DIFF_LSD:
        case DIFF_LSD2: {
            f32 t = st->axleTorque[axle];
            f32 k;

            if (0.0f <= t) {
                k = gc->diffB[axle];
            } else {
                k = -t;
                t = gc->diffC[axle];
            }
            lockTq = t * k;
            if (lockTq < gc->diffA[axle])
                lockTq = gc->diffA[axle];
            if (st->lsdLocked[axle] != 0) {
                f32 d = tl->torque - tr->torque;
                f32 twice = lockTq + lockTq;

                if (twice < d || d < -twice) {
                    st->lsdLocked[axle] = 0;
                } else {
                    hold = 1;
                    break;
                }
            }
            if (0.0f < dv) {
                tl->torque = tl->torque - lockTq;
                tr->torque = tr->torque + lockTq;
            } else if (dv < 0.0f) {
                tl->torque = tl->torque + lockTq;
                tr->torque = tr->torque - lockTq;
            }
            break;
        }
        case DIFF_VISCOUS: {
            f32 s = dv * dv;

            if (gc->diffA[axle] < s)
                s = gc->diffA[axle];
            lockTq = gc->diffB[axle] * s;
            if (0.0f < dv) {
                tl->torque = tl->torque - lockTq;
                tr->torque = tr->torque + lockTq;
            } else if (dv < 0.0f) {
                tl->torque = tl->torque + lockTq;
                tr->torque = tr->torque - lockTq;
            }
            break;
        }
        case DIFF_NONE:
        case DIFF_OPEN:
        case DIFF_AYC:
        case DIFF_AYC2:
            break;
        }

        mode = gc->diffType[axle];
        if (hold != 0)
            mode = DIFF_LOCKED;
        switch (mode) {
        case DIFF_NONE: {
            int j;

            for (j = 0; j < 2; j++) {
                TyreForce *t = tf[j];
                CarWheelState *w = &st->wheel[a + j];
                f32 old = w->spin;
                f32 now = old + t->torque * gc->invInertia[axle] * dt;

                if (t->brakeTorque != 0.0f) {
                    if (0.0f < old) {
                        if (now < 0.0f)
                            now = 0.0f;
                    } else if (old < 0.0f) {
                        if (0.0f < now)
                            now = 0.0f;
                    } else {
                        now = 0.0f;
                    }
                }
                w->spin = now;
            }
            break;
        }
        case DIFF_LOCKED: {
            f32 v = gc->tyreRadius[axle] * st->axleSpeed[axle];

            st->wheel[b].spin = v;
            st->wheel[a].spin = v;
            break;
        }
        default: {
            f32 h = dv * 0.5f;
            h = h + (tf[0]->torque - tf[1]->torque) * gc->invInertia[axle] * 0.5f * dt;
            f32 v;

            if (dv * h < 0.0f) {
                h = 0.0f;
                if ((u32)(mode - DIFF_LSD) < 2u)
                    st->lsdLocked[axle] = 1;
            }
            v = gc->tyreRadius[axle] * st->axleSpeed[axle];
            st->wheel[a].spin = v + h;
            st->wheel[b].spin = v - h;
            break;
        }
        }
    }
}
