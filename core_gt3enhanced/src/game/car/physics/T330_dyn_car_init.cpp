/* T330 (0x001DD0C8..0x001DDFF0): the set-up of one car (RaceCarState, T302;
 * u077's DynCar) of the DynamicsConductor: DynCar_initialize (from
 * DynamicsConductor::initialize, "DynamicsConductor::initialize()") builds
 * its CarDynamics from the spec record, places it on the course and resets
 * its dynamic state (DynCarState, T288). */
/* Retail T330's .rodata (0x0033C3C0): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START); the original saw the
 * race headers (RaceOrganization, RaceCourse). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getContact (seen: UnitArena<64>) */
#include "game/car/physics/T330_dyn_car_init.h"
#include "game/car/physics/car_param_internal.h"
// DATA_START 0x0033C3C0

// FUNC 0x001DD0C8 DynCarState_setDriver
void DynCarState_setDriver(DynCarState *st, int driver)
{
    st->driver = driver;
    st->ctrlMode = 0;
}

// FUNC 0x001DD0D8 DynCarState_setTransMode
/* transMode for driver 0 (the pad), 0 for drivers 1 and 2, unchanged for
 * the others. */
void DynCarState_setTransMode(DynCarState *st, int transMode)
{
    switch (st->driver) {
    case 0:
        st->transMode = transMode;
        break;
    case 1:
    case 2:
        st->transMode = 0;
        break;
    }
}

// FUNC 0x001DD110 DynCar_initTyreWear
/* Per wheel: squeal, wearLevel, wear and heat cleared, wearGrip = 1; when the
 * conductor's WearCurve is active (x3 != 0) the tyres start worn: wear = x0
 * (wearLevel -0x3F if it is negative), wearGrip = 1 - y0. */
void DynCar_initTyreWear(RaceCarState *car)
{
    DynCarState *st = &car->st;
    int i;

    for (i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        DynamicsConductor *dc;
        WearCurve *wear;
        int active;

        w->wearLevel = 0;
        w->squeal = 0;
        w->wear = 0.0f;
        dc = car->dc;
        w->wearGrip = 1.0f;
        w->brakeHeat = 0.0f;
        wear = &dc->wear;
        active = 1;
        if (wear->x3 == 0.0f)
            active = 0;
        if (active) {
            f32 initialWear = dc->wear.x0;

            w->wear = initialWear;
            if (initialWear < 0.0f)
                w->wearLevel = -0x3F;
            else
                w->wearLevel = 0;
            w->wearGrip = 1.0f - wear->y0;
        }
    }
}

// FUNC 0x001DD1A8 DynCarState_resetWheels
void DynCarState_resetWheels(DynCarState *st)
{
    DynWheel *w = st->wheel;
    int i;

    for (i = 0; i < 4; i++, w++) {
        w->smoke = 0.0f;
        w->b29 = 0xFF;
        w->squealVolume = 0.0f;
        w->squealPitch = 1.0f;
        w->rotation = 0.0f;
        w->speedFactor = 0.0f;
        w->slip = 0.0f;
        w->forceMax = 0.0f;
        w->bump = 0.0f;
        w->strokeSpeed = 0.0f;
        w->springForce = 0.0f;
        w->damperForce = 0.0f;
        w->shake = 0.0f;
        w->shakeVel = 0.0f;
    }
}

// FUNC 0x001DD210 DynCar_place
/* Puts the car at (x, y, z up) with heading yaw on runway sector `sector':
 * the road height under the body (the runway axes are x, z, -y), then under
 * each wheel (its surface type; off the road the body's ground plus the
 * wheel's offset), the static wheel loads and angles, the pitch from the
 * axle heights, lapDistance.  Returns 0.  st->sector is the sector the body
 * query returns (retail reads body.sector back after getContact). */
int DynCar_place(RaceCarState *car, int sector, f32 x, f32 y, f32 z, f32 yaw)
{
    DynCarState *st = &car->st;
    CarDynamics *p = &car->params;
    GroundQuery body;
    f32 pos[3];                 /* written, never read (dead stores in the original) */
    f32 ground;
    int counted = 0;            /* wheels on counted ground (attr2 bit 0) */
    int i;
    DynWheel *w = st->wheel;
    GroundQuery *c = car->contact;

    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    body.x = x;
    body.y = z;
    body.z = -y;
    body.sector = sector;
    car->course->getContact(&body);
    ground = body.height;
    st->pos[0] = x;
    st->pos[1] = y;
    st->sector = body.sector;
    st->pos[2] = ground + p->cgHeight;
    st->pitch = 0.0f;
    st->roll = 0.0f;
    st->yaw = yaw;
    eulerToAxes(st->rot[0], st->rot[1], st->rot[2], st->pitch, st->roll, st->yaw);
    pos[2] = ground;
    car_wheel_offsets(car);

    for (i = 0; i < 4; i++, w++, c++) {
        f32 wy = st->pos[1] + w->offY;
        f32 wx = st->pos[0] + w->offX;
        f32 wz = st->pos[2] + w->offZ;

        pos[0] = wx;
        pos[1] = wy;
        c->x = wx;
        c->z = -wy;
        pos[2] = wz;
        c->y = wz;
        c->sector = st->sector;
        car->course->getContact(c);
        if (c->offRoad == 0) {
            f32 h = c->height;

            w->groundZ = h;
            w->groundZ2 = h;
            w->surface = ground_surface_type(c);
            w->b25 = c->unk_32;
            w->b26 = c->attr2;
            counted += c->attr2 & 1;
        } else {
            f32 h;

            w->surface = 0;
            h = ground + w->offZ;
            w->b25 = 0;
            w->b26 = 0;
            w->groundZ = h;
            w->groundZ2 = h;
        }
    }
    st->flags &= (u8)~DYN_FLAG_COUNTED_GROUND;
    if (counted != 0) {
        if (car->dc->isPitEnabled() != 0)
            st->flags |= DYN_FLAG_COUNTED_GROUND;
    }

    for (i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        int axle = i >> 1;
        f32 load;

        load = p->sprungWeight[axle] * 0.5f + p->susp[axle].unsprungWeight;
        w->contactTimer = 0;
        w->load = load;
        w->grip = load;
        w->stroke = p->susp[axle].staticStroke;
        w->steer = wheel_steer_angle(p, i, 0.0f, 0.0f);
        w->camber = wheel_camber(p, i, 0.0f, 0.0f);
        w->lateralPos = wheel_lateral_pos(p, i, 0.0f, 0.0f);
    }
    car_ground_attitude(car);
    st->pitch = st->groundPitch;
    st->roll = st->groundRoll;
    {
        f32 pitch = st->pitch + atan2f(p->axleHeight[0] - p->axleHeight[1], p->wheelbase);

        st->pitch = pitch;
        eulerToAxes(st->rot[0], st->rot[1], st->rot[2], pitch, st->roll, st->yaw);
    }
    st->pos[2] = st->groundPos[2] + st->rot[2][2] * p->cgHeight;
    car_wheel_offsets(car);
    st->lapDistance = car->dc->getLapDistance(car);
    return 0;
}

// FUNC 0x001DD5B8 DynCar_clearFaults
void DynCar_clearFaults(RaceCarState *car)
{
    DynCarState *st = &car->st;

    st->fault = 0;
    st->courseOut = 0;
}

// FUNC 0x001DD5C8 DynCar_setSpeed
void DynCar_setSpeed(RaceCarState *car, f32 v)
{
    car->speed = v;
    car->speedView = v;
    if (car->topSpeed < v)
        car->topSpeed = v;
}

// FUNC 0x001DD5F0 DynCar_resetState
/* The order of the independent stores is retail's schedule: sched1 issues
 * a register's last use first among stores that are ready together (the 1.0
 * of timeScale / unk_420, the 0x7F of engineSnd, `zero'), the others in
 * source order. */
void DynCar_resetState(RaceCarState *car)
{
    CarDynamics *p = &car->params;
    DynCarState *st = &car->st;
    DynWheel *w;
    u32 i;
    f32 zero;

    for (i = 0; i < 4; i++) {
        w = &st->wheel[i];
        w->velAngle = 0.0f;
        w->lowSpeedFactor = 0.0f;
        w->dragForce = 0.0f;
        w->latForce = 0.0f;
        w->driveForce = 0.0f;
        w->brakeForce = 0.0f;
        w->velX = 0.0f;
        w->velY = 0.0f;
        w->spin = 0.0f;
        w->flags = 0;
        w->slipGrip = 1.0f;
    }
    for (i = 0; i < 2; i++) {
        st->lsdLocked[i] = 0;
        if ((u32)(p->gear.diffType[i] - 3) < 2)     /* an LSD, types 3 / 4 */
            st->lsdLocked[i] = 1;
        st->axleSpeed[i] = 0.0f;
    }
    st->centreLocked = 0;
    if ((u32)(p->gear.driveType - 4) < 3)           /* 4WD kinds 4..6 */
        st->centreLocked = 1;
    st->clutchTorqueAbs = 0.0f;
    st->timeScale = 1.0f;
    st->unk_420 = 1.0f;
    st->steer = 0.0f;
    st->steerMin = -p->steerLock;
    st->steerMax = p->steerLock;
    st->yawRate = 0.0f;
    st->steerSpeed = 0.0f;
    st->slipTarget = 0.0f;
    st->slipBlend = 0.0f;
    st->vel[0] = 0.0f;
    st->vel[1] = 0.0f;
    st->vel[2] = 0.0f;
    st->steering = 0;
    st->throttle = 0.0f;
    st->throttleOut = 0.0f;
    st->brake = 0.0f;
    for (i = 0; i < 4; i++)
        st->wheel[i].brake = 0.0f;
    st->fullThrottle = 0;
    st->revving = 0;
    st->turboFactor = 0.0f;
    st->throttlePrev = 0.0f;
    st->turboThrottle = 0.0f;
    st->turboSpool[1] = 0;
    st->turboSpool[0] = 0;
    st->turboSpoolMax = 0;
    st->blowOffLevel = 0.0f;
    st->blowOffState = 0;
    st->engineTorque = 0.0f;
    st->frictionTorque = 0.0f;
    st->limiterOn = 0;
    DynCar_clearFaults(car);
    car->topSpeed = 0.0f;
    zero = car->topSpeed;       /* re-read; the stores below use that register */
    DynCar_setSpeed(car, zero);
    car_set_gear(st, 1);
    st->rpm = 0;
    st->brakeAssist = 0;
    st->tractionAssist = 0;
    st->impact = zero;
    {
        PadVibration *vib = &car->vib;

        vib->big = 0;
        vib->small = 0;
        vib->steer = 0x80;      /* no wheel force */
    }
    car->steerIn = 0;
    car->steerInPrev = 0;
    st->steerForce = zero;
    st->steerHold = 0;
    st->engineSnd[2] = 0;
    st->engineSnd[0] = 0x7F;
    st->engineSnd[1] = 0x7F;
    DynCarState_resetWheels(st);
    st->speedX = zero;
    st->speedY = zero;
    st->clutchState = 0;
    st->clutch = zero;
    st->engineSpeed = zero;
    st->pitchRate = zero;
    st->rollRate = zero;
    st->groundBits = 0;
    st->shiftTimer = 0;
    if (st->driver == 1 || st->ctrlMode != 0 || st->transMode == 0 || st->transMode == 2)
        st->shiftTimer = 12;
    st->shiftHold = 0;
    st->gearOffset = 0;
    st->offsetCooldown = 0;
    st->offsetCount = 0;
    st->lift[0] = 0.0f;
    st->lift[1] = 0.0f;
    st->accelX = 0.0f;
    st->accelY = 0.0f;
    st->heaveAccel = 0.0f;
    st->latGripUse = 0.0f;
    st->yawAccel = 0.0f;
    st->steerOver = 0.0f;
    st->unk_4D0 = 0.0f;
    st->unk_4D4 = 0.0f;
    st->longForce = 0.0f;
}

// FUNC 0x001DD8A8 DynCar_setInitialSpeed
/* A rolling start at v (m/s): the velocity along the heading, the gear for
 * the speed, the engine speed (between idle and the redline), the turbo
 * spooled up, the wheel and axle speeds. */
void DynCar_setInitialSpeed(RaceCarState *car, f32 v)
{
    CarDynamics *p = &car->params;
    DynCarState *st = &car->st;
    f32 boost[2];               /* turbo_boost_factor's per-stage boosts (not used) */
    f32 idleSpeed, engineSpeed;
    int i;

    st->yawRate = 0.0f;
    for (i = 0; i < 3; i++)
        st->vel[i] = st->rot[0][i] * v;
    DynCar_setSpeed(car, v);
    car_set_gear(st, car_gear_for_speed(car, v));
    idleSpeed = (f32)p->engine.idle * RPM_TO_RADS;
    engineSpeed = v * p->gear.ratioPerRadius[st->gear];
    /* Lupo 2: the ratio factor starts at 1; a CVT holds its target rpm
     * (L2 0x001F4D58..0x001F4D6C; GT3Hooks GB_RollThunk) */
    car->cvtRatio = 1.0f;
    gearbox_cvt_limit(&p->gear, &engineSpeed, &car->cvtRatio);
    st->engineSpeed = engineSpeed;
    if (engineSpeed < idleSpeed) {
        st->engineSpeed = idleSpeed;
    } else {
        f32 redlineSpeed = (f32)p->engine.redline * RPM_TO_RADS;

        if (redlineSpeed < engineSpeed)
            st->engineSpeed = redlineSpeed;
    }
    {
        int rpm = (int)(st->engineSpeed * 60.0f / TWO_PI);

        st->rpm = rpm;
        if (p->engine.turboMax != 0.0f) {
            st->turboSpool[0] = rpm;
            if (0.0f < p->engine.turboBoost[1])     /* a second stage */
                st->turboSpool[1] = rpm;
            st->turboFactor = turbo_boost_factor(&p->engine, st->rpm, st->rpm, boost);
            st->throttlePrev = 1.0f;
            st->turboThrottle = 1.0f;
            st->turboSpoolMax = st->rpm;
            st->blowOffLevel = 0.0f;
            st->blowOffState = 0;
        }
    }
    st->clutchState = 1;        /* locked */
    st->clutch = 1.0f;
    st->speedX = v;
    st->speedY = 0.0f;
    for (i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];

        w->spin = v;
        w->velX = v;
    }
    for (i = 0; i < 2; i++) {
#if ENH_PHYSICS_ATTESA_RADII
        /* Lupo 2: both axles on the driven axle's radius (physics.md 24;
         * L2 0x001F4EC0..0x001F4EDC) */
        if (p->gear.sharedRadius) {
            st->axleSpeed[i] = p->gear.tyreRadiusInv[p->gear.drivenAxle] * v;
            continue;
        }
#endif
        st->axleSpeed[i] = p->gear.tyreRadiusInv[i] * v;
    }
}

/* The enhanced engine: one log line per race car with a Lupo 2 feature (a
 * gearbox type, the Dualnote, a special kind), on the PCSX2 / deci2 console.
 * Nothing for GT3's cars. */
static void enhanced_car_log(RaceCarState *car)
{
    static const char *const gearboxNames[4] = {
        "stock", "CVT", "clutchless stepped", "electric single speed"
    };
    int type = car->params.gear.gearboxType;
    int kind = car->params.specialKind;

    if (type != GEARBOX_STOCK)
        printf("[enhanced] gearbox: car %d has gearbox type %d, %s\n",
               car->st.carIndex, type, gearboxNames[type & 3]);
    if (car->hybrid)
        printf("[enhanced] dualnote: car %d is a Dualnote hybrid, battery %d kJ\n",
               car->st.carIndex, (int)car->batteryCharge);
    if (kind != CAR_KIND_NONE)
        printf("[enhanced] kind: car %d has special kind %d (%s)\n", car->st.carIndex, kind,
               kind == CAR_KIND_POD ? "Pod" : kind == CAR_KIND_SL ? "active suspension" : "moving wing");
    if (car->params.gear.driveType == DRIVE_AWD_FIXED_FRONT)
        printf("[enhanced] drivetrain: car %d is a front-primary fixed-split 4WD\n", car->st.carIndex);
}

// FUNC 0x001DDAD0 DynCar_initialize
/* startType: 0 the given position, 8 the same with a 300-frame countdown,
 * both moved back by lenFront along the heading; 1..5 the grid spot
 * startArg along the course (racingLinePoint; type 0 if there is none; a result
 * of -1 leaves the car at gate 0 of lap 0); 6 / 7 the car's pit box
 * (pit_slot_pose, a 0x78-frame pit stop); 3 a 180-frame countdown.  For
 * types 2, 3 and 8 a pad-driven car is driven by the computer until the
 * countdown ends.  Type 4 takes the start speed from the computer driver's
 * next corner (ai.nextSpeed), the others speedKmh.  cond (may be NULL: a
 * cleared one) is the car's condition.  Returns CarDynamics_init's result. */
int DynCar_initialize(RaceCarState *car, CarSpec *spec, int sector, int driver,
                      int transMode, int noController, int carIdx, int startType,
                      f32 x, f32 y, f32 z, f32 yaw, int startArg, int speedKmh,
                      CarCondition *cond)
{
    DynCarState *st = &car->st;
    CarDynamics *p = &car->params;
    int ret;
    DynamicsConductor *dc = car->dc;
    RaceOrganization *org = car->org;
    RaceEntryCar *entry;
    f32 pitPos[3];
    f32 pitRot[3];
    int behind;
    int steerAssist, lastGate, gate;
    f32 startSpeed;
    AIDriver *ai;

    if (cond == NULL)
        st->condition.clear();
    else
        car_set_condition(car, cond);
    car->course = org->course;
    entry = org->entries->cars[carIdx];
    steerAssist = entry->steeringAssist;
    car->input = entry->input;
    if (steerAssist >= 5)
        steerAssist = 0;
    car_set_steer_assist(car, steerAssist);
    if (org->entries->cars[carIdx]->assistAsm == 0)
        spec->ascFlags = 0;                 /* no brake assist / ABS aids */
    if (org->entries->cars[carIdx]->assistTcs == 0)
        spec->tcsThrottle = 0;                  /* no throttle cut (TcsParams.throttleGain) */
    st->noController = noController;
    st->carIndex = carIdx;
    DynCarState_setDriver(st, driver);
    ai = &car->ai;
    ret = CarDynamics_init(p, spec, st->driver, NULL, &dc->rules, cond);
    ai_load_params(ai, p->driveClass, dc->getCourseKind());
    if (st->driver != 0)
        ai_apply_rules(ai, p->driveClass);
    else
        F_001F6540(ai);
    ai_set_downforce(ai, p);
    DynCarState_setTransMode(st, transMode);
    st->flags = 0;
    behind = 0;

    if ((u32)(startType - 1) < 5) {
        int r = dc->racingLinePoint(&sector, &x, &y, &z, &yaw, startArg);

        if (r == 0)
            startType = 0;
        else if (r < 0)
            behind = 1;
    } else if ((u32)(startType - 6) < 2) {
        pit_slot_pose(&car->dc->lines, carIdx, pitPos, pitRot, &sector);
        x = pitPos[0];
        y = pitPos[1];
        z = pitPos[2];
        yaw = pitRot[2];
    }
    if (startType == 0 || startType == 8) {
        f32 len = p->lenFront;

        x += len * sinf(yaw);
        y -= len * cosf(yaw);
    }
    DynCar_place(car, sector, x, y, z, yaw);
    car_attitude_lag_reset(st);
    DynCar_resetState(car);
    DynCar_initTyreWear(car);
    /* Lupo 2: the Dualnote's battery (L2 0x001E0300) and the Pod's parking
     * counter (L2 0x001E1BC8) */
    car->hybrid = (u8)drivetrain_is_hybrid(spec);
    hybrid_setup(car);
    pod_parking_init(car, 0);
    car_time_scale(car);
    st->lapAdjust = 0;
    st->section = 0;
    if (behind) {
        st->gate = 0;
        st->lap = 0;
    } else {
        /* the next checkpoint ahead of the car */
        lastGate = car->dc->getGateCount() - 1;
        st->gate = lastGate;
        for (gate = 0; gate < lastGate; gate++) {
            if (st->lapDistance < car->dc->getGatePosition(gate)) {
                st->gate = gate;
                break;
            }
        }
        if (startType == 6) {
            if (car->dc->getLapLength() * 0.5f < st->lapDistance)
                st->lapAdjust = 1;
            st->gate = 0;
            st->lap = 0;
        } else if (st->gate == lastGate) {  /* u8 against the int */
            st->gate = 0;
            st->lap = 0;
        } else {
            st->lap = 1;
        }
    }
    st->lapStartTime = 0;
    car_reset_best_lap(car);
    st->stopped = 0;
    st->pitTimer = 0;
    st->finished = 0;
    st->msgKind = 0;
    st->msgTime = 0;
    st->pitBox = dc->pitSlotNode(carIdx);
    st->rank = carIdx + 1;
    {
        int line = -1;

        if ((u32)(startType - 2) < 4)
            line = 6;
        else if (startType != 8)
            line = (st->driver == 0) ? 7 : -1;
        ai_reset(ai, line);
    }
    ai_select_gear(ai);
    if ((u32)(startType - 6) < 2) {
        car_mode_pit_box(car);                    /* control mode 3: at the box */
        st->pitTimer = 0x78;
    }
    car->countdown = 0;
    if (driver == 0 && ((u32)(startType - 2) < 3 || startType == 8)) {
        car_mode_computer(car);                    /* control mode 1: the computer drives first */
        st->shiftTimer = 12;
        if (startType == 3)
            car->countdown = 0xB4;
        else if (startType == 8)
            car->countdown = 0x12C;
    }
    if (startType == 4)
        startSpeed = ai->nextSpeed;
    else
        startSpeed = (f32)speedKmh * KMH_TO_MPS;
    if (startSpeed != 0.0f)
        DynCar_setInitialSpeed(car, startSpeed);
    /* Lupo 2: the moving wing, the Pod's emotions, the tachometer's spring
     * (L2 0x001F5530..0x001F5540) */
    car_wing_init(car);
    pod_emotion_reset(car);
    car_display_rpm_reset(car);
    enhanced_car_log(car);
    return ret;
}
