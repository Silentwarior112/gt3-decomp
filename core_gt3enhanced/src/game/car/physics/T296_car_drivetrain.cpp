/* T296 - the drivetrain step of a car (car_drivetrain_step, 0x001E8598):
 * brakes, clutch state, slip ratios, tyre forces, the longitudinal and
 * lateral accelerations, the engine speed, the gear, the velocity; the tyre
 * wear and the tyre check; the rolling resistance. */
/* Retail T296's .rodata (0x0033C7C0): "bad_alloc" (nothing references it:
 * DATA_START), car_driven_axle_speed's jump table (0x0033C7D0, 16-aligned),
 * then the type names "14RaceCameraBase", "t9UnitArena1Ui64": the original
 * saw the race headers (u077's notes: every retail file with UnitArena<64>'s
 * name has a "bad_alloc" of its own) and RaceCameraBase after them. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/T383_race_camera_base.h"     /* (seen: RaceCameraBase) */
#include "game/car/physics/T296_car_drivetrain.h"
#include "game/car/physics/suspension_internal.h"
#include "game/enhanced.h"                          /* ENH_PHYSICS_*, ENH_AI_SKILL_AS_GRIP */
// DATA_START 0x0033C7C0

// FUNC 0x001E6C28 car_driven_axle_speed
/* The angular speed of the driven axle(s): the rear, the front, the mean or
 * a blend by the drive type. */
f32 car_driven_axle_speed(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    GearCalc *gear = &car->params.gear;
    f32 speed = 0.0f;

    switch (gear->driveType) {
    case DRIVE_REAR:
    case DRIVE_AWD_SPLIT:
    case DRIVE_AWD_SPEED:
    case DRIVE_AWD_STEER:
        speed = st->axleSpeed[1];
        break;
    case DRIVE_FRONT:
    case DRIVE_AWD_FRONT:
        speed = st->axleSpeed[0];
        break;
    case DRIVE_AWD_CENTRE:
        speed = (st->axleSpeed[0] + st->axleSpeed[1]) * 0.5f;
        break;
    case DRIVE_AWD_FIXED:
        speed = st->axleSpeed[1] + gear->awdSplit * (st->axleSpeed[0] - st->axleSpeed[1]);
        break;
    case DRIVE_AWD_FIXED_FRONT:             /* Lupo 2: its driven front axle (L2 table 0x00348ED4) */
        speed = st->axleSpeed[0];
        break;
    }
    return speed;
}

// FUNC 0x001E6CA8 car_engine_speed_from_wheels
/* The engine speed the driven wheels give in the current gear (>= 0).  The
 * enhanced engine: a clutchless gearbox's gears 2 and up through slot 1
 * (Lupo 2 L2 0x001E4604). */
f32 car_engine_speed_from_wheels(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    f32 speed = car_driven_axle_speed(car)
              * car->params.gear.ratio[gearbox_ratio_slot(&car->params.gear, st->gear)];
    if (speed <= 0.0f)
        speed = 0.0f;
    return speed;
}

// FUNC 0x001E6D00 car_update_rpm
/* rpm = engine speed (rad/s) x 60 / 2 pi, at least the engine's idle - 1. */
void car_update_rpm(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int floorRpm = car->params.engine.idle - 1;
    int rpm = (int)((st->engineSpeed * 60.0f) / U081_TWO_PI);
    if (rpm < floorRpm)
        rpm = floorRpm;
    st->rpm = (s16)rpm;
}

// FUNC 0x001E6D48 car_clutch_state
/* The clutch state machine before the engine step.  A free engine starts
 * slipping when the car moves (faster than 10 km/h) in the gear's direction
 * with throttle > 0.25, or, not moving so, when the engine is above the idle
 * rpm; else the clutch input is cleared.  A locked clutch frees the engine
 * when the clutch input is 0.  While slipping the wheels' engine speed is
 * recorded and, after a shift and in transmission modes other than 3 / 6,
 * the throttle becomes 0 / 1 (the engine must come down / up to the
 * wheels). */
void car_clutch_state(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    GearCalc *gear = &car->params.gear;

    switch (st->clutchState) {
    case CLUTCH_FREE:
        if (st->clutch != 0.0f) {
            f32 speed = st->speedX;
            if ((speed > 10.0f * U081_KMH && st->gear != 0)
                || (speed < -10.0f * U081_KMH && st->gear == 0)) {
                if (st->throttleOut > 0.25f)
                    st->clutchState = CLUTCH_SLIP;
            } else if ((int)gear->idleRpm < st->rpm) {
                st->clutchState = CLUTCH_SLIP;
            } else {
                st->clutch = 0.0f;
            }
        }
        break;
    case CLUTCH_LOCKED:
        if (st->clutch == 0.0f)
            st->clutchState = CLUTCH_FREE;
        else if (gearbox_is_ratio_glide(gear))
            /* Lupo 2: a CVT / clutchless box keeps the wheels' engine speed
             * (L2 0x001E4880: GT3Hooks GB_EngineStateWrap) */
            st->wheelEngineW = car_engine_speed_from_wheels(car);
        break;
    }

    if (st->clutchState == CLUTCH_SLIP || st->clutchState == CLUTCH_SHIFT) {
        u8 transMode = 0;
        if (st->driver != 1 && st->ctrlMode == 0)
            transMode = st->transMode;
        st->engineWPrev = st->engineSpeed;
        f32 wheelSpeed = car_engine_speed_from_wheels(car);
        st->wheelEngineW = wheelSpeed;
        if (transMode == 3)
            return;
        if (transMode == 6)
            return;
        if (st->clutchState == CLUTCH_SHIFT) {
            if (wheelSpeed < st->engineWPrev) {
                st->throttleOut = 0.0f;
                return;
            }
            st->throttleOut = 1.0f;
        }
    }
}

// FUNC 0x001E6EC8 car_brake_torques
/* Clears the engine speed records, runs the clutch state machine, then the
 * brake torque of each wheel (+ the handbrake on the rear) and the wheel
 * flags (braked, in the air). */
void car_brake_torques(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    u32 i;

    st->engineWPrev = st->wheelEngineW = 0.0f;
    if (gearbox_is_electric(&p->gear))
        car_clutch_state_electric(car);     /* Lupo 2 (L2 0x001E49AC) */
    else
        car_clutch_state(car);

    for (i = 0; i < 4; i++)
        st->tyreForce[i].brakeTorque = st->wheel[i].brake * p->brakeTorque[i >> 1];

    f32 handbrake = st->handbrake * p->handbrakeTorque;
    for (i = 2; i < 4; i++)
        st->tyreForce[i].brakeTorque += handbrake;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        u8 flags = 0;
        if (st->tyreForce[i].brakeTorque != 0.0f)
            flags = WHEEL_BRAKED;
        if (w->load == 0.0f)
            flags |= WHEEL_AIRBORNE;
        w->flags = flags;
    }
}

// FUNC 0x001E6FE0 slip_grip_factor
/* The grip factor of a slipping wheel at low speed, kind 0 (skidding) or 1
 * (pulling): 1 when the kind's force mode is off; below gripSpeed (km/h) it
 * rises linearly from gripMin (%) to 1. */
f32 slip_grip_factor(int kind, f32 speed)
{
    f32 factor = 1.0f;

    if (g_002D28B0.forceMode[kind] != 0) {
        f32 fullSpeed = (f32)g_002D28B0.gripSpeed[kind] * U081_KMH;
        f32 minFactor = (f32)g_002D28B0.gripMin[kind] * 0.01f;
        f32 range = 1.0f - minFactor;
        if (speed < 0.0f)
            speed = -speed;
        if (speed < fullSpeed)
            factor = minFactor + (range * speed) / fullSpeed;
    }
    return factor;
}

// FUNC 0x001E7068 tyre_slip_ratio
/* The slip ratio of a wheel from its tread speed (spin) and the ground speed
 * along it: slipMode 1 = the tread is faster (pulling), -1 = slower
 * (skidding); slipSign the sign of the force; w->slipGrip gets the grip
 * factor; w->slip the ratio in [-1, 1], faded out below the slip fade speed
 * of the kind.  (Matching, 91.2%: the code is retail's but for the three
 * callee-saved FPU registers: retail slip $f20, groundSpeed $f21, spin $f22;
 * here global-alloc ranks slip last (its live length counts 408 against
 * about 107 for the others) and gives $f22, $f20, $f21.  Tried: declaration
 * orders, the paths' slip forms in place or in one expression, the clamp and
 * fade forms; the reference compiler 991111-01 gives the same.) */
void tyre_slip_ratio(TyreForce *tf, CarWheelState *w, f32 groundSpeed, TyreData *tyre)
{
    const f32 still = 0.0625f * U081_KMH;   /* 0.0173611 m/s */
    f32 spin = w->spin;
    f32 grip;
    f32 slip;

    tf->slipMode = 0;
    if (tyre->slipVelocityModel != 0) {
        /* Lupo 2: compound flag A, the slip as a slip velocity over 50 km/h,
         * no grip factor and no low-speed fade (physics.md 8; L2 0x001E4BDC
         * ..0x001E4C88).  0 for every GT3 compound. */
        slip = groundSpeed - spin;
        if (0.0f <= spin) {
            s8 mode = (groundSpeed <= spin) ? 1 : -1;

            tf->slipMode = mode;
            tf->slipSign = mode;
        } else {
            slip = -slip;
            if (spin <= groundSpeed) {
                tf->slipMode = 1;
                tf->slipSign = -1;
            } else {
                tf->slipSign = 1;
                tf->slipMode = -1;
            }
        }
        slip = slip / 13.888887f;           /* 50 km/h, Lupo 2's 0x415E38E2 */
        w->slipGrip = 1.0f;
        if (1.0f < slip)
            slip = 1.0f;
        else if (slip < -1.0f)
            slip = -1.0f;
        w->slip = slip;
        return;
    }
    if (spin > -still && spin < still) {
        /* the wheel stands still: a full skid */
        grip = slip_grip_factor(0, w->velX);
        if (spin < groundSpeed) {
            slip = 1.0f;
            tf->slipSign = -1;
        } else if (groundSpeed < spin) {
            slip = 1.0f;
            tf->slipSign = 1;
        } else {
            slip = 0.0f;
            tf->slipSign = 0;
        }
        tf->slipMode = -1;
    } else {
        slip = 0.0f;
        if (spin > 0.0f) {
            if (groundSpeed <= spin) {
                slip = groundSpeed - spin;
                grip = slip_grip_factor(1, w->velX);
                tf->slipSign = 1;
                slip = (slip * grip) / spin;
                tf->slipMode = 1;
            } else {
                grip = slip_grip_factor(0, w->velX);
                if (!(groundSpeed < still) || !(groundSpeed > -still))
                    slip = ((groundSpeed - spin) * grip) / groundSpeed;
                tf->slipSign = -1;
                tf->slipMode = -1;
            }
        } else {
            if (spin <= groundSpeed) {
                slip = groundSpeed - spin;
                grip = slip_grip_factor(1, w->velX);
                tf->slipSign = -1;
                slip *= grip;
                slip /= spin;
                tf->slipMode = 1;
            } else {
                grip = slip_grip_factor(0, w->velX);
                if (!(groundSpeed < still) || !(groundSpeed > -still))
                    slip = ((groundSpeed - spin) * grip) / groundSpeed;
                tf->slipSign = 1;
                tf->slipMode = -1;
            }
        }
    }

    w->slipGrip = grip;
    if (slip > 1.0f)
        slip = 1.0f;
    else if (slip < -1.0f)
        slip = -1.0f;

    s8 mode = tf->slipMode;
    if (mode == 0) {
        /* never: every path above sets slipMode to +-1 */
        slip = 0.0f;
    } else {
        f32 speed;
        u8 fadeKmh;
        if (mode < 0) {
            speed = groundSpeed;
            fadeKmh = g_002D28B0.slipFadeSpeed[0];
        } else {
            speed = spin;
            fadeKmh = g_002D28B0.slipFadeSpeed[1];
        }
        if (fadeKmh != 0) {
            f32 fadeSpeed = (f32)fadeKmh * U081_KMH;
            if (speed < 0.0f)
                speed = -speed;
            if (speed < fadeSpeed)
                slip *= speed / fadeSpeed;
        }
    }
    w->slip = slip;
}

// FUNC 0x001E7328 car_tyre_slip_ratios
/* The ground speed along each wheel (its velocity turned by the steering
 * angle) -> tyre_slip_ratio.  The enhanced engine: with the wheel's tyre
 * (Lupo 2 L2 0x001E4F60: the compound flags). */
void car_tyre_slip_ratios(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    u32 i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        TyreData *tyre = CarDynamics_getTyre(&car->params, i, w->surface);
        f32 steer = w->steer;
        f32 along = cosf(steer) * w->velX;
        f32 across = sinf(steer) * w->velY;
        tyre_slip_ratio(&st->tyreForce[i], w, along - across, tyre);
    }
}

// FUNC 0x001E73C0 car_tyre_force_limits
/* The longitudinal force and the force limit of each tyre from its slip
 * curves on the surface at its slip ratio.  (Matching: the slip read as
 * st->wheel[i].slip, its own address in retail's loop, not through w.) */
void car_tyre_force_limits(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    u32 i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        TyreData *tyre = CarDynamics_getTyre(&car->params, i, w->surface);
        curveLookupPair(&tyre->slipCurve, &tyre->slipCurve2, &st->tyreForce[i].longForce,
                        &w->forceMax, st->wheel[i].slip);
    }
}

// FUNC 0x001E7450 car_tyre_long_forces
/* Splits each tyre's longitudinal force (x the grip, signed by the slip)
 * into the drive term (slipMode > 0) or the brake term, each also x the
 * slip grip factor (driveOut / brakeOut); force mode 2 of the brake / drive
 * kind puts the term with the factor into the wheel's force.  (Matching: the
 * grip read as st->wheel[i].grip, its own address in retail's loop.) */
void car_tyre_long_forces(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    u32 i;

    for (i = 0; i < 4; i++) {
        TyreForce *tf = &st->tyreForce[i];
        CarWheelState *w = &st->wheel[i];
        f32 force = tf->longForce * st->wheel[i].grip;
        if (tf->slipSign < 0)
            force = -force;
        f32 forceSlip = w->slipGrip * force;
        f32 drive, brake, driveSlip, brakeSlip;
        if (tf->slipMode > 0) {
            brake = 0.0f;
            drive = force;
            driveSlip = forceSlip;
            brakeSlip = 0.0f;
        } else {
            drive = 0.0f;
            brake = force;
            brakeSlip = forceSlip;
            driveSlip = 0.0f;
        }
        tf->driveOut = driveSlip;
        tf->brakeOut = brakeSlip;
        if (g_002D28B0.forceMode[0] == 2)
            w->brakeForce = brakeSlip;
        else
            w->brakeForce = brake;
        if (g_002D28B0.forceMode[1] == 2)
            w->driveForce = driveSlip;
        else
            w->driveForce = drive;
    }
}

// FUNC 0x001E7500 car_engine_speed
/* The engine speed step.  A free engine runs up with its torque
 * (engine_update; at least the idle speed).  A locked clutch: it follows the
 * wheels.  Slipping: the clutch locks (shiftTimer = 12) when the engine and
 * the wheels meet; transmission modes 3 / 6 with a full clutch lock it at
 * once and spin the axles to the engine speed; after a shift it frees the
 * engine when the wheels are at or below the idle speed, else when braking
 * hard below idle + 500 rpm or near standstill.  Then the engine speed is
 * held within 0 .. redline + 10 rpm and the rpm set.  (Matching, 95.1%: the
 * clutch state is read from st at each test, as retail's GCSE carries it in a
 * register set again after each store.  Left: that register ($a0 in retail,
 * $v1 here, the constant 1 in $v1 / $a1) and the order of the lock block's
 * two byte stores; all 24 orders of the lock block and the state's types
 * tried.) */
void car_engine_speed(RaceCarDynamics *car)
{
    CarParams *p = &car->params;
    GearCalc *gear = &p->gear;
    CarDynState *st = &car->st;
    CarEngine *engine = &p->engine;
    f32 dt = car->dt;

    if (st->clutchState == CLUTCH_FREE) {
        f32 idleSpeed = (f32)engine->idle * U081_RPM;
        f32 torque;
        f32 speed;

        if (car->hybrid)
            torque = hybrid_free_rev_torque(car);   /* Lupo 2: the Dualnote's generator (L2 0x001E51D4) */
        else
            torque = engine_update(car, st->throttleOut);
        speed = st->engineSpeed + torque * gear->inertiaInv * dt;
        st->engineSpeed = speed;
        if (speed < idleSpeed)
            st->engineSpeed = idleSpeed;
    } else {
        f32 wheelSpeed = car_engine_speed_from_wheels(car);
        f32 speed;

        if (st->clutchState == CLUTCH_LOCKED) {
            if (gearbox_is_ratio_glide(gear))
                /* Lupo 2: through the CVT / clutchless ratio factor (L2
                 * 0x001E5238..0x001E5258) */
                wheelSpeed = car->cvtRatio * wheelSpeed;
            st->engineSpeed = wheelSpeed;
            speed = wheelSpeed;
        } else if (st->clutchState == CLUTCH_SLIP || st->clutchState == CLUTCH_SHIFT) {
            u8 transMode = 0;
            if (st->driver != 1 && st->ctrlMode == 0)
                transMode = st->transMode;
            if ((transMode == 3 || transMode == 6) && st->clutch == 1.0f) {
                f32 axleSpeed = st->engineSpeed / gear->ratio[gearbox_ratio_slot(gear, st->gear)];
                int axle;
                for (axle = 0; axle < 2; axle++) {
                    st->axleSpeed[axle] = axleSpeed;
                    f32 spin = gear->tyreRadius[axle] * axleSpeed;
                    st->wheel[axle * 2 + 1].spin = spin;
                    st->wheel[axle * 2].spin = spin;
                }
                st->clutchState = CLUTCH_LOCKED;
                speed = st->engineSpeed;
            } else {
                f32 wheelsBefore = st->wheelEngineW;
                f32 engineBefore = st->engineWPrev;
                speed = st->engineSpeed;
                /* the engine crossed the wheels' speed (either way) */
                if ((engineBefore <= wheelsBefore && wheelSpeed <= speed)
                    || (wheelsBefore <= engineBefore && speed <= wheelSpeed)) {
                    st->clutchState = CLUTCH_LOCKED;
                    st->shiftTimer = 12;
                    st->engineSpeed = wheelSpeed;
                    speed = wheelSpeed;
                }
            }
            if (st->clutchState == CLUTCH_SHIFT) {
                if (st->wheelEngineW <= (f32)p->engine.idle * U081_RPM) {
                    st->clutchState = CLUTCH_FREE;
                }
            }
            if (st->clutchState == CLUTCH_SLIP) {
                if (st->brake > 0.75f) {
                    if (speed < (f32)(p->engine.idle + 500) * U081_RPM)
                        st->clutchState = CLUTCH_FREE;
                    else if (st->speedX < 10.0f * U081_KMH && st->speedX > -10.0f * U081_KMH)
                        st->clutchState = CLUTCH_FREE;
                }
            }
        } else {
            speed = st->engineSpeed;
        }

        if (speed < 0.0f) {
            st->engineSpeed = 0.0f;
        } else {
            f32 maxSpeed = (f32)(p->engine.redline + 10) * U081_RPM;
            if (maxSpeed <= speed)
                st->engineSpeed = maxSpeed;
        }
    }
    /* Lupo 2: the shift event of the model animation ends with the shift
     * (L2 0x001E5510..0x001E5528) */
    if (st->clutchState != CLUTCH_SHIFT)
        car_anim_event(car, 0, 0);
    car_update_rpm(car);
}

// FUNC 0x001E77E0 car_gear_select
/* Applies the gear car_select_gear asks for (-1: none); a slipping or
 * locked clutch becomes CLUTCH_SHIFT for a forward -> forward change, else
 * CLUTCH_SLIP. */
void car_gear_select(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int gear = car_select_gear(car, &st->ctrl);

    if (gear == -1)
        return;
    if (st->clutchState != CLUTCH_FREE) {
        if (gear != 0 && st->gear != 0) {
            /* Lupo 2: a clutchless box changes forward gears without the
             * clutch phase (L2 0x001E55B8) */
            if (!gearbox_is_clutchless(&car->params.gear))
                st->clutchState = CLUTCH_SHIFT;
            /* Lupo 2: the shift event of the model animation, +1 up, -1
             * down (L2 0x001E55D0..0x001E560C) */
            if (st->gear < gear)
                car_anim_event(car, 0, 1);
            else if (gear < st->gear)
                car_anim_event(car, 0, -1);
        } else {
            st->clutchState = CLUTCH_SLIP;
        }
    }
    car_set_gear(st, gear);
}

/* The enhanced engine (new, Lupo 2 L2 0x001E5648): the tachometer of an
 * electric single-speed box (GEARBOX_ELECTRIC): the motor's rpm, no idle
 * floor.  From car_drivetrain_step. */
void car_rpm_electric(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    st->rpm = (s16)(int)((st->engineSpeed * 60.0f) / 6.283185f);
}

/* The enhanced engine (new, Lupo 2 L2 0x001E5678): the R / D selector of
 * the CVT and the electric box (types 1 / 3): the reverse request selects R
 * with no speed check; else D once rolling forward faster than 1 km/h or on
 * the throttle, R once rolling back faster than 1 km/h.  No neutral.  A CVT
 * re-engages its clutch on every change (L2 tests types 1 or 2; type 2 never
 * comes here).  From car_drivetrain_step. */
void car_gear_selector_rd(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int gear = st->gear;
    int request = -1;

    if (st->ctrl.reverse != 0) {
        if (gear != 0)
            request = 0;
    } else {
        f32 v = st->speedX;

        if (0.27777776f < v || 0.0f < st->throttleOut) {
            if (gear == 0)
                request = 1;
        } else if (v < -0.27777776f) {
            if (gear != 0)
                request = 0;
        }
    }
    if (request == -1)
        return;
    car_set_gear(st, request);
    if (gearbox_is_ratio_glide(&car->params.gear))
        st->clutchState = CLUTCH_SLIP;
}

/* The enhanced engine (new, Lupo 2 L2 0x001E4948): the clutch state of an
 * electric box: engaged exactly when its clutch is.  From car_brake_torques. */
void car_clutch_state_electric(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;

    if (st->clutch == 0.0f)
        st->clutchState = CLUTCH_FREE;
    else
        st->clutchState = CLUTCH_LOCKED;
}

/* The enhanced engine (new, Lupo 2 L2 0x001E4658 / 0x001E4698): the
 * tachometer's needle, a spring after the engine's rpm (k 1000, damping 50),
 * one per car: RaceCarState.displayRpm (hud.md C2).  Reset at the car's set-up
 * (DynCar_initialize), stepped in car_drivetrain_step. */
void car_display_rpm_reset(RaceCarDynamics *car)
{
    car->displayRpm = (f32)car->st.rpm;
    car->displayRpmSpeed = 0.0f;
}

void car_display_rpm_step(RaceCarDynamics *car)
{
    f32 dt = car->dt;
    f32 x = (f32)car->st.rpm - car->displayRpm;

    car->displayRpmSpeed = car->displayRpmSpeed + (x * 1000.0f - car->displayRpmSpeed * 50.0f) * dt;
    car->displayRpm = car->displayRpm + car->displayRpmSpeed * dt;
}

/* The tachometer's rpm for the HUD (Lupo 2 L2 0x001E4728, from
 * RaceDisplay::virtual_2): the spring's value as an int. */
int car_display_rpm(RaceCarDynamics *car)
{
    return (int)car->displayRpm;
}

// FUNC 0x001E7848 car_long_accel
/* The longitudinal force sum (the wheels' drive forces along the car + the
 * rolling resistance), the effective inverse mass (with the inertia of the
 * wheels that are neither braked nor in the air), the longitudinal
 * acceleration and speed; then, while moving, the tyre drag terms - a speed
 * that would change sign stops instead.  (Matching: the drag loop has its own
 * counter, which retail runs down to 0.)  The enhanced engine: compound flag
 * C (Lupo 2 L2 0x001E5738). */
void car_long_accel(RaceCarDynamics *car)
{
    CarParams *p = &car->params;
    CarDynState *st = &car->st;
    f32 dt = car->dt;
    f32 rollResist = st->rollResist;
    f32 term4D0 = st->unk_4D0;
    f32 airDrag = st->drag;
    f32 accel = st->accelX;
    f32 speed = st->speedX;
    f32 driveSum = 0.0f;
    int i;

    for (i = 0; i < 4; i++)
        driveSum += st->tyreForce[i].cosSteer * st->wheel[i].driveForce;

    st->longForce = st->longForce + driveSum + rollResist;

    f32 wheelInertia = 0.0f;
    for (i = 0; i < 4; i++) {
        if (st->wheel[i].flags == 0)
            wheelInertia += p->gear.axleMass[i >> 1];
    }
    if (wheelInertia > 0.0f)
        st->massInvEff = 1.0f / (wheelInertia + p->mass);
    else
        st->massInvEff = p->invMass;

    f32 newAccel = accel + (st->longForce + term4D0 + airDrag) * st->massInvEff;
    st->accelX = newAccel;
    f32 newSpeed = speed + newAccel * dt;
    st->speedX = newSpeed;

    if (newSpeed != 0.0f) {
        f32 resist = 0.0f;
        int n;
        for (n = 0; n < 4; n++) {
            CarWheelState *w = &st->wheel[n];
            TyreForce *tf = &st->tyreForce[n];
            TyreData *tyre = CarDynamics_getTyre(p, n, w->surface);
            f32 angle = w->velAngle;
            resist += tf->cosSteer * w->brakeForce;
            if (tyre->noFrictionCircle == 1) {
                /* Lupo 2: compound flag C, the lateral force along the
                 * steering, no drag (physics.md 8; L2 0x001E58EC..0x001E5908) */
                resist -= sinf(w->steer) * w->latForce;
                continue;
            }
            resist -= sinf(angle) * w->latForce;
            resist -= cosf(angle) * w->dragForce;
        }
        f32 before = st->speedX;
        f32 resistAccel = resist * st->massInvEff;
        f32 after = before + resistAccel * dt;
        if (before * after > 0.0f) {
            st->longForce = st->longForce + resist;
            st->accelX = st->accelX + resistAccel;
        } else {
            after = 0.0f;
            st->accelX = st->accelX + st->invDt * -before;
        }
        st->speedX = after;
    }
}

// FUNC 0x001E7A68 slip_angle_fold
/* Folds an angle in (-pi, pi] onto [0, pi/2] for the force curves; *sign
 * is the sign the force takes.  (Below -pi/2 the result is angle + pi, sign
 * +1.) */
f32 slip_angle_fold(f32 *sign, f32 angle)
{
    f32 s = 1.0f;
    f32 folded;

    if (angle < -U081_HALF_PI) {
        folded = angle + U081_PI;
    } else if (angle < 0.0f) {
        s = -1.0f;
        folded = -angle;
    } else if (angle < U081_HALF_PI) {
        folded = angle;
    } else {
        s = -1.0f;
        folded = U081_PI - angle;
    }
    *sign = s;
    return folded;
}

// FUNC 0x001E7AD8 wheel_low_speed_side_force
/* The side force of a wheel at walking pace, which holds a nearly stopped
 * car against sliding sideways (the slip angle forces need a direction of
 * travel; lowSpeedFactor fades them out below 10 km/h): 0 unless the spin is
 * within +-5 km/h, the longitudinal slip within +-2 km/h and the wheel's
 * lateral speed within +-4 km/h; then grip x sin(pi x lateral / 4 km/h) x
 * cos(pi / 2 x spin / 5 km/h).  Its sign is the lateral speed's, and
 * car_yaw_lateral subtracts it along the wheel's lateral axis. */
f32 wheel_low_speed_side_force(CarWheelState *w)
{
    if (w->grip == 0.0f)
        return 0.0f;

    f32 spin = w->spin;
    f32 slip = w->velX - spin;
    if (spin < -5.0f * U081_KMH)
        return 0.0f;
    if (spin > 5.0f * U081_KMH)
        return 0.0f;
    if (slip < -2.0f * U081_KMH)
        return 0.0f;
    if (slip > 2.0f * U081_KMH)
        return 0.0f;

    f32 steer = w->steer;
    f32 lateral = cosf(steer) * w->velY + sinf(steer) * w->velX;
    if (lateral > 4.0f * U081_KMH)
        return 0.0f;
    if (lateral < -4.0f * U081_KMH)
        return 0.0f;

    lateral *= 2.8274333f;                  /* 0.9 pi = pi / (4 km/h) */
    spin *= 1.1309733f;                     /* 0.36 pi = (pi / 2) / (5 km/h) */
    return sinf(lateral) * cosf(spin) * w->grip;
}

// FUNC 0x001E7C08 car_tyre_lat_forces
/* The slip angle of each wheel (its heading - the direction of travel) and
 * the lateral / drag forces from the tyre's curves x lowSpeedFactor, limited
 * to the friction circle (forceMax), x the grip.  (Matching, 96.7%: left is
 * the slip angle's register: retail gives it $f12, slip_angle_fold's
 * argument, and stores it from there in the call's delay slot; here it stays
 * in $f0.  Tried: the store before / after the call, as an assignment in the
 * argument, re-read from tf, inline helpers.)
 * The enhanced engine (Lupo 2 L2 0x001E5B78, physics.md 8): the compound
 * flags B (the curve over the lateral velocity, no low-speed factor) and C
 * (no friction circle, the sign by the travel); every wheel's lateral force
 * use x 128 (DynWheel.latUse); the front wheels' mean folded slip
 * (RaceCarState.frontSlip, the AI's steering gain: race_logic.md A7).  With
 * GT3's compounds (flags 0) the forces are GT3's. */
void car_tyre_lat_forces(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    f32 frontSum = 0.0f;                    /* Lupo 2: the front wheels' folded slip (L2 $f24) */
    int frontCount = 0;
    u32 i;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        TyreForce *tf = &st->tyreForce[i];
        TyreData *tyre = CarDynamics_getTyre(&car->params, i, w->surface);

        tf->slipAngle = 0.0f;
        if (w->lowSpeedFactor == 0.0f) {
            w->latForce = 0.0f;
            w->dragForce = 0.0f;
            continue;
        }
        f32 travel = w->velAngle;
        f32 heading = w->steer;
        if (w->velX < 0.0f)
            heading += U081_PI;
        f32 slipAngle = angleDiff(heading, travel);
        tf->slipAngle = slipAngle;
        f32 sign;
        f32 folded;
        if (tyre->latVelocityModel != 0) {
            /* Lupo 2: flag B, the curve read at the lateral velocity x
             * 0.1131 (L2 0x001E5C88..0x001E5CD4) */
            f32 steer = w->steer;

            folded = (cosf(steer) * w->velY + sinf(steer) * w->velX) * 0.11309733f;
            sign = 1.0f;
            if (folded < 0.0f) {
                folded = -folded;
                sign = -1.0f;
            }
            if (tyre->noFrictionCircle == 0 && w->spin < 0.0f)
                sign = -sign;
        } else {
            folded = slip_angle_fold(&sign, slipAngle);
            if (tyre->noFrictionCircle == 1 && w->spin < 0.0f)
                sign = -sign;               /* Lupo 2: flag C, reversing (L2 0x001E5CE8) */
        }
        if (i < 2) {                        /* Lupo 2: the front wheels (L2 0x001E5D14) */
            frontSum += folded;
            frontCount++;
        }
        f32 latUse = curveLookupLinear(&tyre->latCurve, folded);
        w->latUse = (u8)(int)(latUse * 128.0f);   /* Lupo 2 (L2 0x001E5D4C..0x001E5D74) */
        f32 lat = sign * latUse;
        if (tyre->latVelocityModel == 0)
            lat *= w->lowSpeedFactor;
        if (tyre->noFrictionCircle == 1) {
            /* Lupo 2: flag C, the lateral force on its own, at most the
             * friction limit, and no drag (L2 0x001E5E20..0x001E5E6C) */
            if (w->forceMax < lat)
                lat = w->forceMax;
            else if (lat < -w->forceMax)
                lat = -w->forceMax;
            w->latForce = lat * w->grip;
            w->dragForce = 0.0f;
            continue;
        }
        f32 drag = curveLookupLinear(&tyre->dragCurve, folded);
        if (tyre->latVelocityModel == 0)
            drag *= w->lowSpeedFactor;
        f32 magnitude = boot_sqrtf_v(lat * lat + drag * drag);
        if (w->forceMax < magnitude) {
            f32 k = w->forceMax / magnitude;
            lat *= k;
            drag *= k;
        }
        w->latForce = lat * w->grip;
        w->dragForce = drag * w->grip;
    }
    /* Lupo 2: the front wheels' mean (L2 0x001E5F00..0x001E5F24) */
    if (frontCount >= 2)
        frontSum = frontSum / (f32)frontCount;
    car->frontSlip = frontSum;
}

// FUNC 0x001E7D90 car_yaw_lateral
/* The tyre forces in body axes, the yaw moment -> yaw acceleration and the
 * yaw rate (within +-2 pi), the lateral acceleration and latGripUse (it over
 * the mean surface grip).  In the air both are 0.  (Matching, 98.4%: left
 * are lat / longForce in $f23 / $f22 in retail (local-alloc ranks lat first
 * here) and the grip loop's address: retail keeps &st->wheel[i] and loads
 * the surface at +0x24, loop.c here reduces the surface's own address.
 * Tried: declaration orders, longForce in two steps, the grip loop through
 * a pointer, a wheel local or its own counter.) */
void car_yaw_lateral(RaceCarDynamics *car, int airborne)
{
    CarParams *p = &car->params;
    CarDynState *st = &car->st;
    f32 dt = car->dt;

    if (airborne) {
        st->accelY = 0.0f;
        st->latGripUse = 0.0f;
        return;
    }

    f32 wheelY[4];                          /* sp+0x00 the wheels' y */
    f32 wheelX[4];                          /* sp+0x10 the wheels' x */
    wheelY[0] = -p->halfTrack[0];
    wheelX[0] = p->frontDist;
    wheelY[1] = p->halfTrack[0];
    wheelX[1] = p->frontDist;
    wheelY[2] = -p->halfTrack[1];
    wheelX[2] = -p->rearDist;
    wheelY[3] = p->halfTrack[1];
    wheelX[3] = -p->rearDist;

    f32 sumY = 0.0f;
    f32 moment = 0.0f;
    u32 i;
    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        TyreForce *tf = &st->tyreForce[i];
        f32 angle = w->velAngle;
        f32 lat = w->latForce;
        f32 longForce = w->driveForce + w->brakeForce;
        f32 c = cosf(angle);
        f32 s = sinf(angle);
        f32 drag = w->dragForce;
        f32 fy = -c * lat + s * drag;
        f32 fx = -s * lat + -c * drag;
        f32 side = wheel_low_speed_side_force(w);
        fy = fy - tf->cosSteer * side;
        fy = fy - tf->sinSteer * longForce;
        fx = fx - tf->sinSteer * side;
        fx = fx + tf->cosSteer * longForce;
        sumY += fy;
        moment += wheelY[i] * fx - wheelX[i] * fy;
    }

    f32 yawAccel = moment * p->yawInertiaInv;
    st->yawAccel = yawAccel;
    f32 yawRate = st->yawRate + yawAccel * dt;
    st->yawRate = yawRate;
    if (yawRate > U081_TWO_PI)
        st->yawRate = U081_TWO_PI;
    else if (yawRate < -U081_TWO_PI)
        st->yawRate = -U081_TWO_PI;

    f32 accelY = (sumY - st->unk_4D4) * p->invMass;
    st->accelY = accelY;
    f32 grip = 0.0f;
    for (i = 0; i < 4; i++) {
        u8 surface = st->wheel[i].surface;
        TyreData *tyre = CarDynamics_getTyre(p, i, surface);
        grip += tyre->surfaceGrip[surface] * tyre->grip;
    }
    st->latGripUse = (accelY * 4.0f) / grip;
}

// FUNC 0x001E8008 car_integrate_velocity
/* During a pit stop (pitTimer running): clears the planar velocity, the yaw
 * rate and the force sum.  On the ground: a car that is held (stopped, or
 * ctrlMode 8 "still") or stands still (< 1 km/h, every lowSpeedFactor 0, a
 * wheel braked) is stopped; else the body accelerations are added to the
 * world velocity (x, y). */
void car_integrate_velocity(RaceCarDynamics *car, int airborne)
{
    CarDynState *st = &car->st;
    f32 dt = car->dt;

    if (st->pitTimer != 0) {
        st->longForce = 0.0f;
        st->yawRate = 0.0f;
        st->vel[0] = 0.0f;
        st->vel[1] = 0.0f;
        return;
    }
    if (airborne)
        return;

    int stop = 0;
    if (st->stopped != 0 || st->ctrlMode == 8)
        stop = 1;
    f32 speed = st->speedX;
    if (speed > -U081_KMH && speed < U081_KMH
        && st->wheel[0].lowSpeedFactor == 0.0f && st->wheel[1].lowSpeedFactor == 0.0f
        && st->wheel[2].lowSpeedFactor == 0.0f && st->wheel[3].lowSpeedFactor == 0.0f) {
        if ((st->wheel[0].flags & WHEEL_BRAKED) || (st->wheel[1].flags & WHEEL_BRAKED)
            || (st->wheel[2].flags & WHEEL_BRAKED) || (st->wheel[3].flags & WHEEL_BRAKED))
            stop = 1;
    }
    if (stop) {
        st->yawRate = 0.0f;
        st->vel[0] = 0.0f;
        st->vel[1] = 0.0f;
        st->longForce = 0.0f;
        st->accelX = 0.0f;
        st->accelY = 0.0f;
        st->latGripUse = 0.0f;
        st->speedX = 0.0f;
        st->speedY = 0.0f;
        return;
    }

    f32 dvForward = st->accelX * dt;
    f32 dvLateral = st->accelY * dt;
    int axis;
    for (axis = 0; axis < 2; axis++)
        st->vel[axis] += dvForward * st->rot[0][axis] + dvLateral * st->rot[1][axis];
}

// FUNC 0x001E8190 car_tyre_wear
/* With the race's tyre wear on (WearCurve.x3 != 0): clears the squeal
 * levels; unless a wheel is on counted ground (flag 0x10), each loaded
 * wheel off surface 1 wears by min(squealVolume, the cap slipCap) x the
 * tyre's wearRate x (wearBase + load x wearLoad, >= 0) x dt, and its squeal
 * level is min(squealVolume, cap) x wearRate x 255 (at most 255). */
void car_tyre_wear(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    CarParams *p = &car->params;
    WearCurve *wear = &car->dc->wear;
    f32 dt = car->dt;
    int wearOn = 1;
    int i;

    if (wear->x3 == 0.0f)
        wearOn = 0;
    if (!wearOn)
        return;

    for (i = 3; i >= 0; i--)
        st->wheel[i].squeal = 0;
    if (st->flags & DYN_FLAG_COUNTED_GROUND)
        return;

    for (i = 0; i < 4; i++) {
        CarWheelState *w = &st->wheel[i];
        int axle = i >> 1;
        u8 surface = w->surface;
        if (surface == 1)
            continue;
        f32 loadRate = w->load;
        if (loadRate == 0.0f)
            continue;
        loadRate = p->wearBase[axle] + loadRate * p->wearLoad[axle];
        if (loadRate < 0.0f)
            loadRate = 0.0f;
        TyreData *tyre = CarDynamics_getTyre(p, i, surface);
        f32 rate = w->squealVolume;
        if (wear->slipCap < rate)
            rate = wear->slipCap;
        rate = rate * tyre->wearRate;
        w->wear = w->wear + rate * loadRate * dt;
        rate = rate * 255.0f;
        if (rate > 255.0f)
            rate = 255.0f;
        w->squeal = (u8)(int)rate;
    }
}

// FUNC 0x001E8340 car_tyre_check
/* If the conductor allows it (isPitEnabled) and flag 8 is clear: bit 0 when
 * tyre wear is on and a wheel (with wear >= 0) has its wear grip below the
 * pit grip; bit 1 when F_001D9FE0() == 4 and a wheel has needsService.  Either sets
 * flag 4, and flag 8 for driver 2, or for driver 1 unless the conductor's
 * mode is 3 (then only for bit 1). */
void car_tyre_check(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    WearCurve *wear = &car->dc->wear;
    int reasons = 0;
    int i;

    if (car->dc->isPitEnabled() == 0)
        return;
    if (st->flags & DYN_FLAG_PIT_CALL)
        return;

    int wearOn = 1;
    if (wear->x3 == 0.0f)
        wearOn = 0;
    if (wearOn) {
        for (i = 0; i < 4; i++) {
            CarWheelState *w = &st->wheel[i];
            if (w->wear < 0.0f)
                continue;
            if (w->wearGrip < wear->pitGrip) {
                reasons = 1;
                break;
            }
        }
    }

    if (F_001D9FE0(car) == 4) {
        int j;
        for (j = 0; j < 4; j++) {
            if (st->wheel[j].needsService != 0) {
                reasons |= 2;
                break;
            }
        }
    }

    if (reasons == 0)
        return;
    u8 flags = st->flags | DYN_FLAG_TYRE_DUE;
    st->flags = flags;
    s8 driver = st->driver;
    if (driver == 2) {
        st->flags = flags | DYN_FLAG_PIT_CALL;
    } else if (driver == 1) {
        if (car->dc->mode != 3)
            st->flags = flags | DYN_FLAG_PIT_CALL;
        else if (reasons & 2)
            st->flags = flags | DYN_FLAG_PIT_CALL;
    }
}

// FUNC 0x001E84D8 car_rolling_resistance
/* Clears the force terms; the rolling resistance = - sign(speed) x the
 * speed curve x the sum of load x the surface's coefficient. */
void car_rolling_resistance(RaceCarDynamics *car)
{
    CarDynState *st = &car->st;
    int i;

    st->unk_4D0 = 0.0f;
    f32 speed = st->speedX;
    st->longForce = 0.0f;
    st->unk_4D4 = 0.0f;
    st->accelX = 0.0f;
    st->rollResist = 0.0f;
    f32 sum = st->unk_4D0;
    for (i = 0; i < 4; i++)
        sum += st->wheel[i].load * g_002D2CE0.rollingResistance[st->wheel[i].surface];

    if (speed > 0.0f)
        st->rollResist = -curveLookupLinear(&g_002D2CE0.resistCurve, speed) * sum;
    else
        st->rollResist = curveLookupLinear(&g_002D2CE0.resistCurve, -speed) * sum;
}

// FUNC 0x001E8598 car_drivetrain_step
/* One drivetrain step (called from DynamicsConductor::update).  The enhanced
 * engine (Lupo 2 L2 0x001E68E8): ENH_AI_SKILL_AS_GRIP's grip handicap of the
 * computer cars; the gearbox types' engine and gear steps (L2 0x001E6A0C..
 * 0x001E6A8C: GEARBOX_ELECTRIC its tachometer and the R / D selector,
 * GEARBOX_CVT the engine and the selector, the others the engine and the
 * gear select); the tachometer's spring (hud.md C2). */
void car_drivetrain_step(RaceCarDynamics *car)
{
    int airborne;
    GearCalc *gear = &car->params.gear;

    car_brake_torques(car);
    airborne = 1;
    car_rolling_resistance(car);
    car_tyre_force_limits(car);
#if ENH_AI_SKILL_AS_GRIP
    {
        /* Lupo 2: a computer-driven car's tyres grip by the race rules'
         * enemySkillC2 of its drive class, as a percentage (physics.md 13;
         * L2 0x001E6918..0x001E69D0).  The AI no longer reads the byte as a
         * km/h offset (race_logic.md A8). */
        CarDynState *st = &car->st;

        if (st->driver == DRIVER_AUTO || st->driver == DRIVER_COMPUTER || st->ctrlMode != CTRL_DRIVER) {
            int skill = car->dc->rules.enemySkillC2[car->params.driveClass];

            if (skill != 0) {
                f32 k = (f32)skill * 0.01f + 1.0f;
                int i;

                for (i = 0; i < 4; i++) {
                    st->wheel[i].lowSpeedFactor = st->wheel[i].lowSpeedFactor * k;
                    st->wheel[i].forceMax = st->wheel[i].forceMax * k;
                }
            }
        }
    }
#endif
    car_tyre_long_forces(car);
    car_tyre_lat_forces(car);
    car_long_accel(car);
    car_wheel_torques(car);
    car_active_yaw_control(car);
    car_drive_torque(car);
    car_axle_differentials(car);
    if (gearbox_is_electric(gear)) {
        car_rpm_electric(car);              /* Lupo 2 (L2 0x001E6A20) */
        car_gear_selector_rd(car);
    } else if (gearbox_is_cvt(gear)) {
        car_engine_speed(car);
        car_gear_selector_rd(car);          /* Lupo 2 (L2 0x001E6A48) */
    } else {
        car_engine_speed(car);
        car_gear_select(car);
    }
    car_display_rpm_step(car);              /* Lupo 2 (L2 0x001E6A90) */

    CarDynState *st = &car->st;
    if (st->wheel[0].load + st->wheel[1].load + st->wheel[2].load + st->wheel[3].load != 0.0f)
        airborne = 0;
    car_yaw_lateral(car, airborne);
    car_integrate_velocity(car, airborne);
}

// FUNC 0x001E8670 tyre_peak_slip_angle
/* The slip angle (0..45 degrees, 1 degree steps) of latCurve's maximum; 90
 * degrees when the curve is never above 0. */
f32 tyre_peak_slip_angle(TyreData *t)
{
    f32 best = 0.0f;
    f32 offset = best;                      /* (f21: always 0) */
    int bestDeg = 90;
    int deg;

    for (deg = 0; deg < 46; deg++) {
        f32 sign;
        f32 folded = slip_angle_fold(&sign, (f32)deg * U081_DEG - offset);
        f32 value = curveLookupLinear(&t->latCurve, folded) * sign;
        if (best < value) {
            best = value;
            bestDeg = deg;
        }
    }
    return (f32)bestDeg * U081_DEG;
}
