/* T344 - the drivetrain of a frame (car_drivetrain_step, T296, calls
 * car_wheel_torques, car_active_yaw_control, car_drive_torque and
 * car_axle_differentials in this order): the wheels' torques, the clutch and
 * the engine speed, the torque split between the axles (GearCalc.driveType),
 * the axle speeds (rad/s), and the differential of each axle
 * (GearCalc.diffType), which sets the wheels' spin (m/s: tyreRadius x the
 * axle speed).  DRIVE_LOCKED (98000) is the code's "locked" marker. */
#include "game/car/physics/T344_drivetrain.h"
#include "game/car/physics/drivetrain_internal.h"
#include "game/enhanced.h"                          /* ENH_PHYSICS_* */

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

    if (car->hybrid)
        hybrid_front_motors(car);           /* Lupo 2: the Dualnote's front motors (L2 0x001FB4C4) */

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
 * Quirk kept: frontWheelTq is not used.
 * The enhanced engine, ENH_PHYSICS_ATTESA (physics.md 12; Lupo 2 L2
 * 0x001FB6E8): k falls to 0.15 (not 0.25); in the reverse the drive's sign
 * is flipped, and on the overrun (drive < 0) the coupling lets go
 * (DRIVE_FREE: the front axle decoupled); the lock test counts the front
 * wheels' torque too (drive_layout sets awdTorque 0 for this type then, so
 * the lock tests do not run). */
// FUNC 0x001E5998 car_centre_coupling
f32 car_centre_coupling(RaceCarState *car, f32 drive, f32 frontWheelTq, f32 rearWheelTq)
{
    CarRunState *st = &car->st;
    CarDynamics *p = &car->params;
    f32 slipEst, k, limit, share;
#if ENH_PHYSICS_ATTESA
    const f32 kLow = 0.14999999f;           /* Lupo 2: its byte 15 / 100 at run time (L2 gp-0x4B84) */
#else
    const f32 kLow = 0.25f;
#endif

    slipEst = st->latGripUse;                  /* (u081: ? a slip estimate) */
    if (slipEst < 0.0f)
        slipEst = -slipEst;
    if (slipEst <= 0.25f)
        k = 1.0f;
    else if (1.0f <= slipEst)
        k = kLow;
    else
        k = 1.0f - ((slipEst - 0.25f) * (1.0f - kLow)) / 0.75f;
#if ENH_PHYSICS_ATTESA
    /* Lupo 2: the overrun decouples the front (L2 0x001FB778..0x001FB79C) */
    if (st->gear == 0)
        drive = -drive;
    if (drive < 0.0f)
        return DRIVE_FREE;
#endif
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
#if ENH_PHYSICS_ATTESA
            /* Lupo 2: the front wheels' torque counts too (L2 0x001FB814) */
            rearWheelTq = frontWheelTq + rearWheelTq;
#endif
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
 * difference (h = s0 - s1; ...; h *= 0.5), not kept (97.4%, unnatural).
 * The enhanced engine (Lupo 2 L2 0x001FB960; the matching notes above are
 * GT3's): the gearbox types' torques (the electric motor law, the CVT /
 * clutchless controller) and the Dualnote's (the mapped throttle and the
 * crank motor), in Lupo 2's order; a clutchless box's ratio slot and the
 * ratio factor; DRIVE_AWD_FIXED_FRONT (physics.md 11); ENH_PHYSICS_ATTESA's
 * free coupling (12), ENH_PHYSICS_ATTESA_RADII's shared radius (24),
 * ENH_PHYSICS_AWD_COUPLING (30).  With GT3's cars and the switches at 0
 * this is GT3's code path. */
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
    f32 inertia[2];                         /* sp+0x10 per axle (not set for driveType 3 / >= 9: never read then) */
    f32 frontTq;                            /* $f20 the torque to the undriven axle, driveType 4..8 */
    f32 *ratio;
    f32 engineInertia;
    f32 speedDiff;
    int centre;
    int axle;
    int electric = gearbox_is_electric(gc);

    if (clutch != CLUTCH_FREE) {
        switch (gc->driveType) {
        case DRIVE_REAR:
        case DRIVE_AWD_SPLIT:
        case DRIVE_AWD_SPEED:
        case DRIVE_AWD_STEER:
            st->wheel[2].flags |= WHEEL_DRIVEN;
            st->wheel[3].flags |= WHEEL_DRIVEN;
            if (car->hybrid) {
                /* Lupo 2: the Dualnote's front wheels go with its motors
                 * (its type 7's drive bits, L2 table 0x003491EC) */
                st->wheel[0].flags |= WHEEL_COUPLED;
                st->wheel[1].flags |= WHEEL_COUPLED;
            }
            break;
        case DRIVE_FRONT:
        case DRIVE_AWD_FIXED_FRONT:         /* Lupo 2 (L2 table 0x003491F4) */
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
    if (electric) {
        /* Lupo 2: the electric box's motor law runs in every clutch state;
         * its torque drives only through a closed clutch (L2 0x001FBAE8..
         * 0x001FBB14) */
        f32 motorTq = gearbox_electric_torque(car);

        if (clutch != CLUTCH_FREE)
            drive = motorTq;
    } else if (clutch != CLUTCH_FREE) {
        f32 engineTq;
        u8 state;

        if (gearbox_is_ratio_glide(gc))
            engineTq = gearbox_cvt_torque(car);         /* Lupo 2 (L2 0x001FBB30) */
        else if (car->hybrid)
            /* Lupo 2: the Dualnote, the pedal mapped to the system's power,
             * plus the crank motor (L2 0x001FBB4C..0x001FBB70) */
            engineTq = engine_update(car, hybrid_throttle(car)) + hybrid_crank_torque(car);
        else
            engineTq = engine_update(car, st->throttleOut);
        state = st->clutchState;
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
    if (!car->hybrid) {
        /* (Lupo 2: a Dualnote keeps its front motors' load here, L2
         * 0x001FBC10; the whine reads it) */
        st->clutchTorqueAbs = drive;
        if (!(0.0f <= drive))
            st->clutchTorqueAbs = -drive;
    }

    frontTq = 0.0f;
    ratio = gc->ratio;
    drive = drive * gc->ratio[gearbox_ratio_slot(gc, st->gear)];
    wheelTq[0] = st->tyreForce[0].torque + st->tyreForce[1].torque;
    wheelTq[1] = st->tyreForce[2].torque + st->tyreForce[3].torque;
#if ENH_PHYSICS_ATTESA_RADII
    if (gc->sharedRadius) {
        /* Lupo 2: the undriven axle's torque on the driven axle's radius
         * (physics.md 24; L2 0x001FBCDC..0x001FBD1C) */
        int d = gc->drivenAxle;

        wheelTq[1 - d] = wheelTq[1 - d] * (gc->tyreRadius[1 - d] * gc->tyreRadiusInv[d]);
    }
#endif
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
    case DRIVE_AWD_FIXED:
    case DRIVE_AWD_FIXED_FRONT: {
        f32 want = 0.0f;                    /* $f3 the front torque asked for, or DRIVE_LOCKED */
        u8 locked;
        int driven = gc->drivenAxle;        /* 1; 0 for DRIVE_AWD_FIXED_FRONT */

        frontTq = 0.0f;
        switch (gc->driveType) {
        case DRIVE_AWD_FIXED:
        case DRIVE_AWD_FIXED_FRONT:
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
            if (want != DRIVE_LOCKED && want != DRIVE_FREE)
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
            } else if (want == DRIVE_FREE) {
                st->centreLocked = 0;       /* Lupo 2: the coupling lets go (L2 0x001FBEEC) */
                locked = 0;
            } else {
                f32 strain = (wheelTq[1 - driven] - wheelTq[driven]) - drive;
                f32 hold;

                if (strain < 0.0f)
                    strain = -strain;
#if ENH_PHYSICS_AWD_COUPLING
                /* Lupo 2: the lock holds while the strain is within the
                 * coupling torque asked for (L2 0x001FBF2C..0x001FBF40) */
                hold = want;
                if (hold < 0.0f)
                    hold = -hold;
#else
                hold = gc->awdTorque;
#endif
                if (hold < strain) {
                    st->centreLocked = 0;   /* the lock slips */
                    locked = 0;
                } else {
                    centre = CENTRE_LOCKED;
                }
            }
        }
        if (locked == 0) {
            if (want == DRIVE_FREE) {
                frontTq = 0.0f;             /* Lupo 2: the front decoupled (L2 0x001FBF60) */
            } else if (want != DRIVE_LOCKED) {
                frontTq = want;
#if ENH_PHYSICS_AWD_COUPLING
                if (frontTq < 0.0f)         /* Lupo 2: its size only; the axle speeds give the way (L2 0x001FBF80) */
                    frontTq = -frontTq;
#endif
            } else {
                f32 limit = gc->awdTorque;

#if !ENH_PHYSICS_AWD_COUPLING
                if (st->axleSpeed[driven] - st->axleSpeed[1 - driven] < 0.0f)
                    limit = -limit;
#endif
                frontTq = limit;
            }
            if (frontTq != 0.0f)
                centre = CENTRE_COUPLING;
        }
        if (centre != CENTRE_NONE) {
            /* the undriven axle's wheels go with the coupling */
            st->wheel[(1 - driven) * 2].flags |= WHEEL_COUPLED;
            st->wheel[(1 - driven) * 2 + 1].flags |= WHEEL_COUPLED;
        }
        break;
    }
    }

    /* the engine's inertia through the gearbox, with the clutch locked (none
     * for the electric box; a clutchless box's slot and the CVT / clutchless
     * ratio factor: Lupo 2 L2 0x001FBFF8..0x001FC068) */
    engineInertia = 0.0f;
    if (!electric && st->clutchState == CLUTCH_LOCKED) {
        u8 g = st->gear;

        engineInertia = gc->inertia * gc->ratio[gearbox_ratio_slot(gc, g)];
        if (g == 0)
            engineInertia = -engineInertia;
        if (gearbox_is_ratio_glide(gc))
            engineInertia = engineInertia * car->cvtRatio;
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
#if ENH_PHYSICS_AWD_COUPLING
        /* Lupo 2: the coupling torque from the faster axle to the slower
         * (L2 0x001FC2A8..0x001FC2F4) */
        if (st->axleSpeed[1] < st->axleSpeed[0]) {
            st->axleTorque[1] = drive + frontTq;
            st->axleTorque[0] = -frontTq;
            break;
        }
#endif
        st->axleTorque[1] = drive - frontTq;
        st->axleTorque[0] = frontTq;
        break;
    case DRIVE_AWD_FIXED_FRONT:
        /* Lupo 2: the front driven, the rear through the coupling (L2
         * 0x001FC2F8..0x001FC33C) */
        inertia[1] = gc->axleInertia[1];
        inertia[0] = gc->axleInertia[0] + engineInertia;
#if ENH_PHYSICS_AWD_COUPLING
        if (!(st->axleSpeed[1] < st->axleSpeed[0])) {
            st->axleTorque[0] = drive + frontTq;
            st->axleTorque[1] = -frontTq;
            break;
        }
#endif
        st->axleTorque[1] = frontTq;
        st->axleTorque[0] = drive - frontTq;
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
            if ((u32)(gc->driveType - DRIVE_AWD_SPLIT) < 3u        /* 4..6 */
                || gc->driveType == DRIVE_AWD_FIXED_FRONT)          /* Lupo 2 (L2 0x001FC504) */
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
 * that +- half the spin difference.  (The enhanced engine: the radius is the
 * driven axle's with GearCalc.sharedRadius, ENH_PHYSICS_ATTESA_RADII; Lupo 2
 * L2 0x001FC8B8, 0x001FC98C.)
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
            f32 v = axle_tread_radius(gc, axle) * st->axleSpeed[axle];

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
            v = axle_tread_radius(gc, axle) * st->axleSpeed[axle];
            st->wheel[a].spin = v + h;
            st->wheel[b].spin = v - h;
            break;
        }
        }
    }
}

/* ======================================================================== */
/* The enhanced engine: Lupo 2's gearbox types and the Honda Dualnote       */
/* (docs/gearbox_dualnote.md; ref/enhanced/ported/physics.md 23).  Lupo 2's */
/* own code (L2 0x001DF910..0x001E1238), operation for operation, its       */
/* constants as Lupo 2's bits (its compiler rounded them toward zero: the   */
/* literals below are those floats); the GT3Hooks ports (Gearbox.c,         */
/* Hybrid.c), checked bit for bit against Lupo 2 in an EE emulator, are the */
/* reference transcription.  Every function is reached only for a car with  */
/* a gearbox type (GearCalc.gearboxType != 0) or a Dualnote                 */
/* (RaceCarState.hybrid): never for a GT3 car.                              */
/* ======================================================================== */

/* The CVT's rpm limiter (Lupo 2 L2 0x001DF910): a type-1 box holds the
 * engine at its target rpm (GearCalc.shiftRpm[0], CarDynamics_initTopSpeed)
 * by the ratio factor, at most down to the top gear's ratio; *engineSpeed
 * becomes the held speed.  Other types: nothing. */
void gearbox_cvt_limit(GearCalc *g, f32 *engineSpeed, f32 *factor)
{
    f32 limit, w, top;

    if (!gearbox_is_cvt(g))
        return;
    limit = (f32)g->shiftRpm[0] * 0.10471974f;
    w = *engineSpeed;
    if (w < limit) {
        *factor = 1.0f;
        return;
    }
    *factor = limit / w;
    top = g->ratio[g->gearCount];
    if (*factor < top)
        *factor = top;
    *engineSpeed = *factor * w;
}

/* The CVT controller's rates, Lupo 2's bytes x 0.01 computed at run time as
 * Lupo 2 does once (L2 0x001DF9C8; at run time the EE rounds toward zero:
 * 40 x 0.01 = 0x3ECCCCCC, 150 x 0.01 = 0x3FBFFFFF). */
static u8  g_cvtRateBytes[4] = { 40, 15, 150, 150 };   /* L2 0x0035DF98.. */
static int g_cvtRatesReady = 0;
static f32 g_cvtRate40, g_cvtRate15, g_cvtRate150a, g_cvtRate150b;

static void gearbox_cvt_rates()
{
    if (g_cvtRatesReady)
        return;
    g_cvtRatesReady = 1;
    g_cvtRate40 = (f32)g_cvtRateBytes[0] * 0.01f;
    g_cvtRate15 = (f32)g_cvtRateBytes[1] * 0.01f;
    g_cvtRate150a = (f32)g_cvtRateBytes[2] * 0.01f;
    g_cvtRate150b = (f32)g_cvtRateBytes[3] * 0.01f;
}

/* The CVT / clutchless controller (Lupo 2 L2 0x001DFA38; GT3Hooks
 * GB_CvtTorque), with the clutch locked and a forward gear: the ratio
 * factor (RaceCarState.cvtRatio, the engine speed over the wheels' engine
 * speed, between the top ratio and 1) moves toward its target, a CVT's from
 * the target rpm at 60 % .. 100 % with the pedal at 15 % .. 40 % of itself
 * per second, a clutchless box's to the gear's step at 150 % per second (the
 * automatic's shift timer is held meanwhile).  Otherwise the factor is 1.
 * Returns the engine torque (engine_update) x the factor. */
f32 gearbox_cvt_torque(RaceCarState *car)
{
    CarRunState *st = &car->st;
    GearCalc *g = &car->params.gear;
    f32 dt = car->dt;
    f32 pedal = st->throttle;
    f32 throttle = st->throttleOut;
    f32 f = 1.0f;

    gearbox_cvt_rates();
    if (st->clutchState == CLUTCH_LOCKED && st->gear != 0) {
        f32 target = 1.0f;
        f32 top;
        f32 rate;
        int mark = 0;

        f = st->engineSpeed / st->wheelEngineW;
        if (1.0f < f) {
            f = 1.0f;
        } else {
            top = g->ratio[g->gearCount];
            if (f < top)
                f = top;
        }
        if (gearbox_is_cvt(g)) {
            f32 e = ((f32)g->shiftRpm[0] * 0.10471974f) * (pedal * 0.39999998f + 0.59999996f);

            if (e < st->wheelEngineW) {
                target = e / st->wheelEngineW;
                top = g->ratio[g->gearCount];
                if (target < top)
                    target = top;
                else if (1.0f < target)
                    target = 1.0f;
            }
            rate = g_cvtRate15 + pedal * (g_cvtRate40 - g_cvtRate15);
        } else {
            if (st->gear >= 2)
                target = g->ratio[st->gear];    /* a step relative to 1st (gear_calc) */
            rate = (target < f) ? g_cvtRate150a : g_cvtRate150b;
            /* the automatic is choosing (driver 1, a game-driven car, AT) */
            mark = (st->driver == DRIVER_AUTO || st->ctrlMode != CTRL_DRIVER || st->transMode == TRANS_AUTO);
        }
        if (target != f) {
            f32 step = (rate * dt) * car->cvtRatio;
            int done;

            if (f < target) {
                f = f + step;
                done = (target <= f);
            } else {
                f = f - step;
                done = (f <= target);
            }
            if (done)
                f = target;
            else if (mark)
                st->shiftTimer = 1;     /* no new gear mid-glide */
        }
    }
    car->cvtRatio = f;
    return f * engine_update(car, throttle);
}

/* The electric single speed's motor law (Lupo 2 L2 0x001DFCA0; GT3Hooks
 * GB_EvMotor): 294 x the throttle less a loss with the motor speed (196 at
 * 5000 rpm), plus a correction above 1000 rpm; the engine curve only drives
 * a servo of the motor's speed toward a target (none below 3 km/h, 1000 rpm
 * up to 10 km/h, then up to the shift limit at 130 km/h; x the throttle in
 * D).  Updates the engine (motor) speed; returns the drive torque. */
f32 gearbox_electric_torque(RaceCarState *car)
{
    CarRunState *st = &car->st;
    GearCalc *g = &car->params.gear;
    int gear = st->gear;
    f32 throttle = st->throttleOut;
    f32 w = st->axleSpeed[g->drivenAxle] * g->ratio[gear];
    f32 drive = throttle * 293.99997f;
    f32 loss = (w * 195.99998f) / 523.5987f;
    f32 a = (gear == 1) ? throttle : 0.0f;
    f32 v = st->speedX;
    f32 target, x, tq, om;

    if (v < 0.83333325f)
        target = 0.0f;
    else if (v < 2.7777774f)
        target = 104.71974f;
    else if (36.111107f < v)
        target = (f32)g->revLimit * 0.10471974f;
    else
        target = ((v - 2.7777774f) * ((f32)((int)g->revLimit - 1000) * 0.10471974f)) / 33.33333f + 104.71974f;
    target = target * a;
    if (target <= 104.61502f)
        target = 0.0f;

    x = (target - st->engineSpeed) * 0.009549297f;
    if (1.0f < x)
        x = 1.0f;
    else if (x < 0.0f)
        x = 0.0f;
    tq = engine_update(car, x);

    om = st->engineSpeed + (tq * g->inertiaInv) * car->dt;
    st->engineSpeed = om;
    if (target == 0.0f && om < 52.35987f)
        st->engineSpeed = om * 0.5f;
    om = st->engineSpeed;
    if (104.71974f < om && 52.35987f < w)
        drive = drive + (((st->engineTorque - tq) * a) * om) / w;
    return drive - loss;
}

/* ---- the Honda Dualnote --------------------------------------------------- */

/* Lupo 2's 27 bytes of the hybrid (L2 0x0035DFB1), expanded at run time by
 * hybrid_params_init as Lupo 2's L2 0x001DFFF0 does (not const: the EE's
 * rounding, not the compiler's). */
static u8 g_hybridBytes[27] = {
    20, 35, 6, 15, 6, 15, 50, 15, 20, 35, 20, 50, 75, 70,
    1, 50, 10, 1, 7, 255, 20, 50, 50, 10, 50, 50, 10
};

/* What L2 0x001DFFF0 makes of them (Lupo 2 gp-0x4094..gp-0x4034). */
static struct {
    f32 motorFront, motorCrank;             /* MF, MR: the front motor, the IMA, kW */
    f32 frontDriveTq, crankAssistTq;        /* the torque caps: front drive, IMA assist */
    f32 frontBrakeTq, crankGenTq;           /* front brake, IMA generate */
    f32 frontRate;                          /* the front force per unit */
    f32 capacity;                           /* the battery, kJ (1125) */
    f32 frontRegen, rearRegen;              /* the regeneration caps: each front wheel, the rear pair */
    f32 idleRpm, idleGain;                  /* the clutch-open idle-up: rpm, gain */
    f32 genTorque, genPower;                /* the clutch-open generator: torque share, kW */
    f32 socLow, socHigh;                    /* the charge of battery factor 0 / 1 */
    f32 frontWeak;                          /* the front limit on a weak battery */
    f32 frontShare;                         /* the front share of the power */
    f32 yawAcc, yawTarget, deadband;        /* the torque vectoring under drive */
    f32 yawAcc2, yawTarget2, deadband2;     /*   and under braking */
    f32 flowScale;                          /* the flow meter's scale */
    int gearLow, gearHigh, gearMin;         /* assist in gears 1..7; the gear floor (-1) */
} g_hybrid;

/* Lupo 2 L2 0x001DFFF0 (from drivetrain_layout for awdType 5, and the spec
 * sheet's bonus), in its order. */
void hybrid_params_init()
{
    u8 *b = g_hybridBytes;
    f32 s = ((f32)b[0] + (f32)b[0]) + (f32)b[1];

    g_hybrid.motorFront = (f32)b[0];
    g_hybrid.motorCrank = (f32)b[1];
    g_hybrid.frontDriveTq = (f32)(b[2] * 10);
    g_hybrid.crankAssistTq = (f32)(b[3] * 10);
    g_hybrid.frontBrakeTq = (f32)(b[4] * 10);
    g_hybrid.crankGenTq = (f32)(b[5] * 10);
    g_hybrid.frontRate = (f32)b[6] * 0.099999994f;
    g_hybrid.capacity = (f32)b[7] * s;
    g_hybrid.frontRegen = (f32)(b[8] * 5);
    g_hybrid.rearRegen = (f32)(b[9] * 5);
    g_hybrid.idleRpm = (f32)b[10] * 100.0f;
    g_hybrid.idleGain = (f32)b[11] / 100000.0f;
    g_hybrid.genTorque = (f32)b[12] / 100.0f;
    g_hybrid.genPower = (f32)b[13];
    g_hybrid.socLow = (g_hybrid.capacity * (f32)b[14]) / 100.0f;
    g_hybrid.socHigh = (g_hybrid.capacity * (f32)b[15]) / 100.0f;
    g_hybrid.frontWeak = (f32)b[16] / (f32)b[0];
    g_hybrid.gearLow = (s8)b[17];
    g_hybrid.gearHigh = (s8)b[18];
    g_hybrid.gearMin = (s8)b[19];
    g_hybrid.frontShare = (f32)b[20] / 100.0f;
    g_hybrid.yawAcc = (f32)b[21] / 979.9999f;
    g_hybrid.yawTarget = (f32)b[22] / 979.9999f;
    g_hybrid.deadband = (f32)b[23] / 100.0f;
    g_hybrid.yawAcc2 = (f32)b[24] / 979.9999f;
    g_hybrid.yawTarget2 = (f32)b[25] / 979.9999f;
    g_hybrid.deadband2 = (f32)b[26] / 100.0f;
    g_hybrid.flowScale = 6000.0f / s;
}

/* The battery's flow meter (Lupo 2 L2 0x001E0378): -100..100, this step's
 * energy (RaceCarState.batteryFlow; the HUD gauge's colour). */
static void hybrid_flow(RaceCarState *car, f32 energy)
{
    int flow = car->batteryFlow + (int)(energy * g_hybrid.flowScale);

    if (flow < -100)
        flow = -100;
    if (!(flow < 101))
        flow = 100;
    car->batteryFlow = (s8)flow;
}

/* Store `energy' kJ; returns what fitted (Lupo 2 L2 0x001E03B8). */
static f32 hybrid_regen(RaceCarState *car, f32 energy)
{
    f32 charge = car->batteryCharge;
    f32 total, stored;

    hybrid_flow(car, energy);
    total = charge + energy;
    if (g_hybrid.capacity < total) {
        stored = g_hybrid.capacity;
        energy = energy - (total - g_hybrid.capacity);
    } else {
        stored = total;
    }
    car->batteryCharge = stored;
    return energy;
}

/* Draw `demand' kJ; returns what was there (Lupo 2 L2 0x001E0438). */
static f32 hybrid_assist(RaceCarState *car, f32 demand)
{
    f32 charge = car->batteryCharge;
    f32 rest;

    hybrid_flow(car, -demand);
    if (charge < demand) {
        f32 x = demand - charge;

        rest = 0.0f;
        demand = demand - x;
    } else {
        rest = charge - demand;
    }
    car->batteryCharge = rest;
    return demand;
}

/* The battery factor: 0 at 1 % charge and below, 1 from 50 %. */
static f32 hybrid_soc(RaceCarState *car)
{
    f32 c = car->batteryCharge;

    if (g_hybrid.socHigh <= c)
        return 1.0f;
    if (c <= g_hybrid.socLow)
        return 0.0f;
    return (c - g_hybrid.socLow) / (g_hybrid.socHigh - g_hybrid.socLow);
}

/* The engine's share of the system: its full-throttle power over itself +
 * twice the IMA's 35 kW. */
static f32 hybrid_share(CarRunState *st)
{
    f32 p = ((st->engineTorque + st->frictionTorque) * (f32)st->rpm) * 0.000104790604f;

    return p / ((p + g_hybrid.motorCrank) + g_hybrid.motorCrank);
}

/* The engine throttle for a pedal that asks for the system's power (Lupo 2
 * L2 0x001E0600). */
f32 hybrid_throttle(RaceCarState *car)
{
    f32 t = car->st.throttleOut / hybrid_share(&car->st);

    return (1.0f < t) ? 1.0f : t;
}

/* The IMA's demand, -1 the full generator .. +1 full assist (Lupo 2 L2
 * 0x001E06D0): no assist outside gears 1..7. */
static f32 hybrid_demand(RaceCarState *car)
{
    CarRunState *st = &car->st;
    f32 share = hybrid_share(st);
    f32 pedal = (st->clutchState == CLUTCH_SLIP) ? st->throttle : st->throttleOut;
    f32 x;
    int gear;

    if (pedal <= share)
        return -1.0f;
    x = ((pedal - share) + (pedal - share)) / (1.0f - share);
    x = x * hybrid_soc(car);
    gear = st->gear;
    if (!(g_hybrid.gearMin < gear))
        x = 0.0f;
    else if (1.0f < x && (gear < g_hybrid.gearLow || g_hybrid.gearHigh < gear))
        x = 1.0f;
    return x - 1.0f;
}

/* The lateral acceleration the steering asks for, at most twice the front
 * tyre's grip (Lupo 2 L2 0x001E07E8). */
static f32 hybrid_target_lateral(RaceCarState *car)
{
    CarRunState *st = &car->st;
    f32 steer = st->steer;
    f32 speed = st->speedX;
    f32 a = speed * ((speed * tanf(steer)) / car->params.wheelbase);
    f32 x, grip;
    int i;
    TyreData *tyre;

    if (a < 0.0f)
        a = -a;
    i = (0.0f < steer);
    tyre = CarDynamics_getTyre(&car->params, i, st->wheel[i].surface);
    x = steer * 1.5f;
    if (x < 0.0f)
        x = -x;
    grip = curveLookupLinear(&tyre->latCurve, x);
    grip = grip * tyre->grip;
    grip = grip + grip;
    if (grip < 0.0f)
        grip = -grip;
    if (grip < a)
        a = grip;
    if (steer < 0.0f)
        a = -a;
    return a;
}

/* The front motors (Lupo 2 L2 0x001E0900; GT3Hooks Hyb_Front): each front
 * wheel's motor torque added to its drive torque (TyreForce.torque), the
 * regeneration and the draw on the battery; the torque vectoring between the
 * front wheels past a deadband of about 0.2 g; full force at no cost to
 * launch; a drag on the overrun.  The motors' load goes to
 * DynCarState.clutchTorqueAbs (the whine).  From car_active_yaw_control. */
void hybrid_front_motors(RaceCarState *car)
{
    CarRunState *st = &car->st;
    f32 dt = car->dt;
    f32 throttle, net, power, demand, f, soc, limit, r, cap, e, energy, sum;
    f32 force[2], out[2];
    int slow[2];
    int i;

    st->clutchTorqueAbs = 0.0f;
    throttle = (st->clutchState == CLUTCH_SHIFT) ? st->throttle : st->throttleOut;
    net = engine_output(car, throttle);
    power = (net * (f32)st->rpm) * 0.000104790604f;
    demand = hybrid_demand(car);

    f = (((power + g_hybrid.motorCrank * demand) * g_hybrid.frontShare) / (1.0f - g_hybrid.frontShare))
        / (g_hybrid.motorFront + g_hybrid.motorFront);
    if (1.0f < f)
        f = 1.0f;
    else if (f < -1.0f)
        f = -1.0f;
    soc = hybrid_soc(car);
    limit = soc + g_hybrid.frontWeak * (1.0f - soc);
    if (limit < f)
        f = limit;

    for (i = 0; i < 2; i++) {               /* the front regeneration */
        f32 w = st->wheel[i].spin;
        f32 x = st->tyreForce[i].brakeOut;
        f32 p;

        if (w < 0.0f)
            w = -w;
        slow[i] = (w < 0.27777776f);
        if (x < 0.0f)
            x = -x;
        p = (x * w) * 0.0010006767f;
        if (g_hybrid.frontRegen < p)
            p = g_hybrid.frontRegen;
        hybrid_regen(car, p * dt);
    }

    r = (((car->params.gear.tyreRadius[0] * 6.283185f) * 9.799999f) * 716.19995f) / 44.129997f;
    force[0] = force[1] = f;

    if (0.0f < f) {                         /* drive */
        cap = g_hybrid.frontRate * g_hybrid.frontDriveTq;
        e = hybrid_target_lateral(car) * g_hybrid.yawTarget - st->accelY * g_hybrid.yawAcc;
        if (0.0f < e) {
            e = e - g_hybrid.deadband;
            if (!(e < 0.0f)) {
                f32 a, c;

                e = e * 0.5f;
                a = f * (e + 1.0f);
                c = f * (1.0f - e);
                force[1] = a;
                force[0] = c;
                if (1.0f < a) {
                    force[1] = 1.0f;
                    force[0] = c - (a - 1.0f);
                }
                if (force[0] < -1.0f)
                    force[0] = -1.0f;
            }
        } else if (e < 0.0f) {
            e = e + g_hybrid.deadband;
            if (!(0.0f < e)) {
                f32 a, c;

                e = e * 0.5f;
                a = f * (1.0f - e);
                c = f * (e + 1.0f);
                force[0] = a;
                force[1] = c;
                if (1.0f < a) {
                    force[0] = 1.0f;
                    force[1] = c - (a - 1.0f);
                }
                if (force[1] < -1.0f)
                    force[1] = -1.0f;
            }
        }
        for (i = 0; i < 2; i++) {
            out[i] = 0.0f;
            if (!slow[i]) {
                f32 w = st->wheel[i].spin;

                force[i] = force[i] * ((g_hybrid.motorFront * r) / w);
                if (w < 0.0f)
                    force[i] = -force[i];
                if (cap < force[i])
                    force[i] = cap;
                else if (force[i] < -cap)
                    force[i] = -cap;
                out[i] = (force[i] * w) / r;
                if (w < 0.0f)
                    out[i] = -out[i];
            } else if (st->brake == 0.0f) {
                force[i] = cap;             /* the launch: full force, free */
            }
        }
        energy = (out[0] + out[1]) * dt;
        if (0.0f < energy) {
            f32 q = hybrid_assist(car, energy) / energy;

            force[1] = force[1] * q;
            out[0] = out[0] * q;
            force[0] = force[0] * q;
            out[1] = out[1] * q;
        }
        sum = 0.0f;
        if (0.0f < out[0])
            sum = out[0] + sum;
        if (0.0f < out[1])
            sum = sum + out[1];
        st->clutchTorqueAbs = sum / g_hybrid.motorFront;
    } else {                                /* lift-off: a drag */
        cap = g_hybrid.frontRate * g_hybrid.frontBrakeTq;
        e = hybrid_target_lateral(car) * g_hybrid.yawTarget2 - st->accelY * g_hybrid.yawAcc2;
        if (0.0f < e) {
            e = e - g_hybrid.deadband2;
            if (!(e < 0.0f)) {
                e = e * 0.5f;
                force[0] = force[0] * (e + 1.0f);
                force[1] = force[1] * (1.0f - e);
                if (force[0] < -1.0f) {
                    f32 x = force[0] + 1.0f;

                    force[0] = -1.0f;
                    force[1] = force[1] - x;
                }
                if (0.0f < force[1])
                    force[1] = 0.0f;
            }
        } else if (e < 0.0f) {
            e = e + g_hybrid.deadband2;
            if (!(0.0f < e)) {
                e = e * 0.5f;
                force[1] = force[1] * (1.0f - e);
                force[0] = force[0] * (e + 1.0f);
                if (force[1] < -1.0f) {
                    f32 x = force[1] + 1.0f;

                    force[1] = -1.0f;
                    force[0] = force[0] - x;
                }
                if (0.0f < force[0])
                    force[0] = 0.0f;
            }
        }
        for (i = 0; i < 2; i++) {
            if (!slow[i]) {
                f32 w = st->wheel[i].spin;

                force[i] = force[i] * ((g_hybrid.motorFront * r) / w);
                if (w < 0.0f)
                    force[i] = -force[i];
                if (force[i] < -cap)
                    force[i] = -cap;
                else if (cap < force[i])
                    force[i] = cap;
            }
        }
    }

    if (st->gear == 0) {
        force[0] = -force[0];
        force[1] = -force[1];
    }
    st->tyreForce[0].torque = st->tyreForce[0].torque + force[0];
    st->tyreForce[1].torque = st->tyreForce[1].torque + force[1];
}

/* The rear regeneration (only while braking), then the IMA: its crank
 * torque, assisting from the battery or generating into it (Lupo 2 L2
 * 0x001E1018; GT3Hooks Hyb_Rear).  From car_drive_torque (gearbox type 0). */
f32 hybrid_crank_torque(RaceCarState *car)
{
    CarRunState *st = &car->st;
    f32 demand = hybrid_demand(car);
    f32 dt = car->dt;
    f32 p = 0.0f;
    f32 rpm, tq, energy;
    int j;

    for (j = 2; j < 4; j++) {
        f32 x;

        if (st->wheel[j].brake == 0.0f)
            continue;
        x = st->tyreForce[j].brakeOut * st->wheel[j].spin;
        if (x < 0.0f)
            x = -x;
        p = p + x * 0.0010006767f;
    }
    if (g_hybrid.rearRegen < p)
        p = g_hybrid.rearRegen;
    hybrid_regen(car, p * dt);

    rpm = (f32)st->rpm;
    if (0.0f < demand) {                    /* assist */
        tq = (g_hybrid.motorCrank * 9542.84f) / rpm;
        if (g_hybrid.crankAssistTq < tq)
            tq = g_hybrid.crankAssistTq;
        energy = (((tq * rpm) / 9542.84f) * demand) * dt;
        tq = tq * demand;
        tq = tq * (hybrid_assist(car, energy) / energy);
    } else {                                /* generate */
        tq = (g_hybrid.motorCrank * 9542.84f) / rpm;
        if (g_hybrid.crankGenTq < tq)
            tq = g_hybrid.crankGenTq;
        energy = (-(((tq * rpm) / 9542.84f) * demand)) * dt;
        tq = tq * demand;
        hybrid_regen(car, energy);
    }
    return tq;
}

/* The clutch-open torque (Lupo 2 L2 0x001E04B0; GT3Hooks Hyb_FreeRev): with
 * the battery not full, the engine is loaded as a generator (at most 75 % of
 * its torque, 70 kW) and its throttle held up to an idle-up level of about
 * 2000 rpm plus that load.  From car_engine_speed (clutch free). */
f32 hybrid_free_rev_torque(RaceCarState *car)
{
    CarRunState *st = &car->st;
    f32 rpm = (f32)st->rpm;
    f32 throttle = st->throttleOut;
    f32 limit, gen, a, tq;

    if (1.0f <= car->batteryCharge / g_hybrid.capacity)
        return engine_update(car, throttle);
    limit = (g_hybrid.genPower * 9542.84f) / rpm;
    gen = st->engineTorque * g_hybrid.genTorque;
    if (limit < gen)
        gen = limit;
    a = (g_hybrid.idleRpm - rpm) * g_hybrid.idleGain;
    a = a + (gen + st->frictionTorque) / (st->engineTorque + st->frictionTorque);
    if (a < 0.0f)
        a = 0.0f;
    if (throttle < a)
        throttle = a;
    if (1.0f < throttle)
        throttle = 1.0f;
    st->throttleOut = throttle;
    tq = engine_update(car, throttle);
    hybrid_regen(car, ((gen * rpm) * 0.000104790604f) * car->dt);
    return tq - gen;
}

/* The car's set-up (Lupo 2 L2 0x001E0300, from DynCar_initialize; GT3Hooks
 * Hyb_Setup): a Dualnote (RaceCarState.hybrid, from the spec) starts with a
 * full battery; the flow meter starts at 0. */
void hybrid_setup(RaceCarState *car)
{
    if (car->hybrid) {
        car->batteryCapacity = g_hybrid.capacity;
        car->batteryCharge = 1.0f * g_hybrid.capacity;
    } else {
        car->batteryCapacity = 0.0f;
        car->batteryCharge = 0.0f;
    }
    car->batteryFlow = 0;
}

/* Each race step, before the cars move (Lupo 2 L2 0x001E0348, from
 * DynamicsConductor::update): the flow meter shows one step's energy. */
void hybrid_flow_reset(RaceCarState *car)
{
    if (car->hybrid)
        car->batteryFlow = 0;
}

/* The battery's charge for the HUD gauge, 0..1 (Lupo 2 L2 0x001E02C0); 0
 * for a car that is not a Dualnote. */
f32 hybrid_battery_level(RaceCarState *car)
{
    if (!car->hybrid || car->batteryCapacity == 0.0f)
        return 0.0f;
    return car->batteryCharge / car->batteryCapacity;
}

/* The spec sheet's maximum power + the Dualnote's three motors (2 x 20 +
 * 35 kW, in PS: / 0.7355; Lupo 2 L2 0x001E1238, from its gear_chart_with_
 * sheet and ShowRoomCar).  The caller tests the hybrid. */
void hybrid_power_bonus(EngineSpecSheet *sheet)
{
    hybrid_params_init();
    sheet->maxPower = sheet->maxPower
        + ((g_hybrid.motorFront + g_hybrid.motorFront) + g_hybrid.motorCrank) / 0.7355f;
}
