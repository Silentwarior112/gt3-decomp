/* T302 - RaceCarState, the car's state: 0x001EAA88..0x001EBB60.
 *
 * The car's init, lap and finish queries, the finish action, the per-frame
 * step (car_step), and the display side: car_save_prev keeps the previous
 * frame's values, car_view_update computes this frame's view position, and
 * the accessors interpolate between the two with
 *     t = frac + interpBias;  value = prev * t + cur * (1 - t). */
#include "game/car/physics/car_motion_internal.h"
#include "game/car/physics/T302_car_state.h"

/* From DynamicsConductor::initialize. */
// FUNC 0x001EAA88 car_init
void car_init(RaceCarState *car, DynamicsConductor *dc, RaceOrganization *org)
{
    memset((void *)car, 0, 0x16DC);             /* sizeof(RaceCarState) */
    car->org = org;
    car->dc = dc;
    ai_construct(&car->ai, dc, car);
}

/* The conductor's countdown is still running. */
// FUNC 0x001EAAE8 car_is_held
int car_is_held(RaceCarState *car)
{
    return car->dc->getCountDown() > 0;
}

/* The lap to display: 1 before the first, the last one done once
 * finished.  The conductor's lap count (virtual_29) is fetched and
 * ignored. */
// FUNC 0x001EAB08 car_lap_display
int car_lap_display(RaceCarState *car)
{
    DynCarState *st = &car->st;
    int lap = st->lap;

    car->dc->virtual_29();
    if (lap <= 0)
        lap = 1;
    else if (st->finished != 0)
        lap = lap - 1;
    return lap;
}

// FUNC 0x001EAB70 car_no_controller
int car_no_controller(RaceCarState *car)
{
    return car->st.noController != 0;
}

/* From car_update_pit (T350).  1 when the race's time limit (minutes) is
 * over or the car has done the conductor's number of laps (one more when it
 * is less than half a course length into the lap); never with lapAdjust. */
// FUNC 0x001EAB80 car_race_done
int car_race_done(RaceCarState *car)
{
    DynCarState *st = &car->st;
    DynamicsConductor *dc = car->dc;

    if (st->lapAdjust != 0)
        return 0;
    if (dc->isTimeOver(dc->getClockMs()))
        return 1;
    int laps = dc->virtual_29();
    if (laps == 0)
        return 0;
    if (st->lapDistance < dc->getLapLength() * 0.5f)
        laps++;
    return st->lap >= laps;
}

// FUNC 0x001EAC60 car_set_wheel_brakes
void car_set_wheel_brakes(RaceCarState *car, f32 brake)
{
    for (int i = 3; i >= 0; i--)
        car->st.wheel[i].brake = brake;
}

// FUNC 0x001EAC90 car_set_brake
void car_set_brake(RaceCarState *car, f32 brake)
{
    car->st.brake = brake;
    car_set_wheel_brakes(car, brake);
}

/* The car crosses the finish line (10 callers: the conductors' gate
 * handlers, the licence ends, the free run).  CAR_FINISH_AUTO: STOP for a
 * car with DYN_FLAG_COUNTED_GROUND, else DRIVE_ON.  The first time it sets
 * finished, and stops there in the pit modes 2..4; else (and on every later
 * call) the computer driver gets the rules of the car's drive class for a
 * pad-driven car, and the control mode is set (T341).
 * Matching (88.1%): the switch is exact (case RACE_END first, an explicit
 * case DRIVE_ON with default: retail's tree tests 1, < 2, 2, 3).  Open: the
 * AUTO choice is retail `li 2; andi 0x10; movz mode, $0` (mode then gets
 * $s1, car $s2); every ?: / if / else / inline-helper / separate-variable
 * form gives jump.c's cmove with the 2 in a fresh pseudo, which combine
 * folds to `dsrl 3; andi 2` (the reference compiler too).  Only a test
 * combine cannot see through (`~flags & 0x10`, movn) keeps a cmove: not
 * the original (ref/matching/u082/notes.md). */
// FUNC 0x001EACB8 car_finish
void car_finish(RaceCarState *car, int mode)
{
    DynCarState *st = &car->st;

    if (mode == CAR_FINISH_AUTO)
        mode = (st->flags & DYN_FLAG_COUNTED_GROUND) ? CAR_FINISH_STOP : CAR_FINISH_DRIVE_ON;
    if (st->finished == 0) {
        st->finished = 1;
        if (st->ctrlMode >= CTRL_PIT_IN && st->ctrlMode <= CTRL_PIT_OUT)
            return;                             /* (u32)(ctrlMode - 2) < 3 */
    }
    if (st->driver == DRIVER_PAD) {
        F_001F6548(&car->ai);
        ai_apply_rules(&car->ai, car->params.driveClass);
    }
    switch (mode) {
    case CAR_FINISH_RACE_END:
        ai_reduce_pace(&car->ai);
        car_mode_computer(car);
        break;
    case CAR_FINISH_PULL_OVER:
        car_mode_stop_brake(car);
        break;
    case CAR_FINISH_STOP:
        car_mode_stop(car);
        break;
    case CAR_FINISH_DRIVE_ON:
    default:
        car_mode_computer(car);
        break;
    }
}

/* From DynamicsConductor::initialize and DynamicsConductor::update: the
 * values of the last frame, for the interpolating accessors.
 * Matching (94.8%, 97.1% with the reference compiler): the contact fields
 * read as car->contact[i].f (retail has one pointer giv per field).  Left:
 * GCSE hoists the 18 address constants car + k (PRE reaching registers);
 * their pseudo order follows the hash buckets, so two register pairs, the
 * order of two hoisted adds and four spill slots differ. */
// FUNC 0x001EADB8 car_save_prev
void car_save_prev(RaceCarState *car)
{
    DynCarState *st = &car->st;
    int i;

    const f32 *angle = &st->pitch;
    for (i = 0; i < 3; i++)
        car->anglePrev[i] = angle[i];
    const f32 *groundAngle = &st->groundPitch;
    for (i = 0; i < 2; i++)
        car->groundAnglePrev[i] = groundAngle[i];
    for (i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        car->wheelStrokePrev[i] = w->stroke;
        car->wheelSteerPrev[i] = w->steer;
        car->wheelCamberPrev[i] = w->camber;
        car->wheelLateralPosPrev[i] = w->lateralPos;
        car->wheelRotPrev[i] = w->rotation;
        car->wheelBrakeHeatPrev[i] = w->brakeHeat;
        car->contactUPrev[i] = car->contact[i].u;
        car->contactVPrev[i] = car->contact[i].v;
        car->wheelPosPrev[i][0] = car->contact[i].x;
        car->wheelPosPrev[i][1] = w->groundZ2;
        car->wheelPosPrev[i][2] = car->contact[i].z;
        car->wheelShakePrev[i] = w->shake;
    }
    car->lagYawDiffPrev = st->lagYawDiff;
    car->lagPitchDiffPrev = st->lagPitchDiff;
    car->lapDistancePrev = st->lapDistance;
}

/* Keeps the yaw and the wheel rotations within +-2 pi (moving the previous
 * values by the same turn, so the interpolation stays smooth), turns the
 * wheels by tread speed / radius x dt (not while stopped), and computes
 * this frame's view positions (x, z, -y, less originOffset along the
 * forward row) and the 0..1 steering display. */
// FUNC 0x001EAFC0 car_view_update
void car_view_update(RaceCarState *car)
{
    DynCarState *st = &car->st;
    f32 dt = car->dt;
    f32 yaw = st->yaw;

    if (TWO_PI <= yaw) {
        st->yaw = yaw - TWO_PI;
        car->anglePrev[2] = car->anglePrev[2] - TWO_PI;
    } else if (yaw <= -TWO_PI) {
        st->yaw = yaw + TWO_PI;
        car->anglePrev[2] = car->anglePrev[2] + TWO_PI;
    }

    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        f32 turn = w->spin * car->params.gear.tyreRadiusInv[i >> 1] * dt;
        if (st->stopped != 0)
            turn = 0.0f;
        f32 rot = w->rotation + turn;
        while (TWO_PI <= rot) {
            rot -= TWO_PI;
            car->wheelRotPrev[i] -= TWO_PI;
        }
        while (rot <= -TWO_PI) {
            rot += TWO_PI;
            car->wheelRotPrev[i] += TWO_PI;
        }
        w->rotation = rot;
    }

    CarParams *p = &car->params;
    f32 origin = p->originOffset;
    car->viewPosPrev[0] = car->viewPos[0];
    car->viewPosPrev[1] = car->viewPos[1];
    car->viewPosPrev[2] = car->viewPos[2];
    car->viewPos[0] = st->pos[0] - origin * st->rot[0][0];
    car->viewPos[1] = st->pos[2] - origin * st->rot[0][2];
    car->viewPos[2] = -(st->pos[1] - origin * st->rot[0][1]);
    car->viewGroundPosPrev[0] = car->viewGroundPos[0];
    car->viewGroundPosPrev[1] = car->viewGroundPos[1];
    car->viewGroundPosPrev[2] = car->viewGroundPos[2];
    car->viewGroundPos[0] = st->groundPos[0] - origin * st->groundRot[0][0];
    car->viewGroundPos[1] = st->groundPos[2] - origin * st->groundRot[0][2];
    car->viewGroundPos[2] = -(st->groundPos[1] - origin * st->groundRot[0][1]);
    car->steerViewPrev = car->steerView;

    f32 steer = 0.5f - (st->steer / p->steerLock) * 0.5f;
    if (steer < 0.0f)
        steer = 0.0f;
    else if (1.0f < steer)
        steer = 1.0f;
    car->steerView = steer;
}

/* From DynamicsConductor::update (T317): the dynamics frame of one car. */
// FUNC 0x001EB218 car_step
void car_step(RaceCarState *car)
{
    DynCarState *st = &car->st;

    car_wheel_planar_offsets(car);
    car_wheel_speed_factors(st);                /* T348 (CarDynState *) */
    car_ground_contact(car);
    car_susp_prepare(car);
    car_suspension_forces(car);
    car_body_motion(car);
    car_ground_penetration(car);
    pad_vibration_update(car);
    steer_force_update(car);
    tyre_sound_update(car);
    F_001DF148(car);
    car_sound_engine_levels(car);
    car_silence_hidden(car);
    car_attitude_lag_update(st);
    car_wheel_offsets(car);
    eulerToAxes(st->rot[0], st->rot[1], st->rot[2], st->pitch, st->roll, st->yaw);
    car_ground_attitude(car);
    car_speed_step(car);
    car_view_update(car);
    tyre_smoke_emit(car);
}

/* From RaceDisplay::updateLapTimes (T154). */
// FUNC 0x001EB2F8 car_lap_time
int car_lap_time(RaceCarState *car)
{
    return car->dc->CurrentLapTime(car);
}

// FUNC 0x001EB330 car_brake_assist
u8 car_brake_assist(RaceCarState *car)
{
    return car->st.brakeAssist;
}

// FUNC 0x001EB338 car_traction_assist
u8 car_traction_assist(RaceCarState *car)
{
    return car->st.tractionAssist;
}

// FUNC 0x001EB340 car_oil_needs_change
int car_oil_needs_change(RaceCarState *car)
{
    return car->st.condition.oilNeedsChange();
}

// FUNC 0x001EB360 car_handbrake_on
int car_handbrake_on(RaceCarState *car)
{
    int on = 1;
    if (!(car->st.handbrake > 0.25f))
        on = 0;
    return on;
}

/* At the conductor's current display fraction (replaySpeed). */
// FUNC 0x001EB388 car_view_x_now
f32 car_view_x_now(RaceCarState *car)
{
    return car_view_x(car, car->dc->replaySpeed);
}

// FUNC 0x001EB3B0 car_view_y_now
f32 car_view_y_now(RaceCarState *car)
{
    return car_view_y(car, car->dc->replaySpeed);
}

// FUNC 0x001EB3D8 car_view_z_now
f32 car_view_z_now(RaceCarState *car)
{
    return car_view_z(car, car->dc->replaySpeed);
}

// FUNC 0x001EB400 car_view_x
f32 car_view_x(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewPosPrev[0] * t + car->viewPos[0] * (1.0f - t);
}

// FUNC 0x001EB430 car_lap_distance
f32 car_lap_distance(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->lapDistancePrev * t + car->st.lapDistance * (1.0f - t);
}

// FUNC 0x001EB460 car_view_y
f32 car_view_y(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewPosPrev[1] * t + car->viewPos[1] * (1.0f - t);
}

// FUNC 0x001EB490 car_view_z
f32 car_view_z(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewPosPrev[2] * t + car->viewPos[2] * (1.0f - t);
}

// FUNC 0x001EB4C0 car_pitch
f32 car_pitch(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->anglePrev[0] * t + car->st.pitch * (1.0f - t);
}

// FUNC 0x001EB4F0 car_roll
f32 car_roll(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->anglePrev[1] * t + car->st.roll * (1.0f - t);
}

// FUNC 0x001EB520 car_yaw
f32 car_yaw(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->anglePrev[2] * t + car->st.yaw * (1.0f - t);
}

// FUNC 0x001EB550 car_ground_view_x
f32 car_ground_view_x(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewGroundPosPrev[0] * t + car->viewGroundPos[0] * (1.0f - t);
}

// FUNC 0x001EB580 car_ground_view_y
f32 car_ground_view_y(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewGroundPosPrev[1] * t + car->viewGroundPos[1] * (1.0f - t);
}

// FUNC 0x001EB5B0 car_ground_view_z
f32 car_ground_view_z(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->viewGroundPosPrev[2] * t + car->viewGroundPos[2] * (1.0f - t);
}

// FUNC 0x001EB5E0 car_ground_pitch
f32 car_ground_pitch(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->groundAnglePrev[0] * t + car->st.groundPitch * (1.0f - t);
}

// FUNC 0x001EB610 car_ground_roll
f32 car_ground_roll(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->groundAnglePrev[1] * t + car->st.groundRoll * (1.0f - t);
}

/* The same code as car_yaw (the yaw for the ground frame users). */
// FUNC 0x001EB640 car_yaw2
f32 car_yaw2(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->anglePrev[2] * t + car->st.yaw * (1.0f - t);
}

/* No callers.  view y - ground view y, at least 0.  The fraction passed on
 * already has interpBias added, so the callees add it a second time
 * (kept). */
// FUNC 0x001EB670 car_height_above_ground
f32 car_height_above_ground(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    f32 viewY = car_view_y(car, t);
    f32 height = viewY - car_ground_view_y(car, t);
    if (height < 0.0f)
        height = 0.0f;
    return height;
}

/* The condition's dirt, 0 on course kind 1 (isCourseKind1). */
// FUNC 0x001EB6D8 car_dirt
f32 car_dirt(RaceCarState *car)
{
    return car->dc->isCourseKind1() ? 0.0f : car->st.condition.dirt;
}

// FUNC 0x001EB710 car_wheel_lateral_pos
f32 car_wheel_lateral_pos(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->wheelLateralPosPrev[wheel] * t + car->st.wheel[wheel].lateralPos * (1.0f - t);
}

/* The axle's gapOffset less the interpolated stroke. */
// FUNC 0x001EB750 car_wheel_spring
f32 car_wheel_spring(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    f32 stroke = car->wheelStrokePrev[wheel] * t + car->st.wheel[wheel].stroke * (1.0f - t);
    return car->params.susp[wheel >> 1].gapOffset - stroke;
}

/* -wheelbase / 2 for the front wheels, +wheelbase / 2 for the rear ones;
 * t (the callers' fraction, as for the other wheel accessors) is not read. */
// FUNC 0x001EB7A8 car_wheel_axle_x
f32 car_wheel_axle_x(RaceCarState *car, int wheel, f32 t)
{
    f32 half = car->params.wheelbase * 0.5f;
    if ((wheel >> 1) != 0)
        half = -half;
    return -half;
}

// FUNC 0x001EB7D0 car_wheel_steer
f32 car_wheel_steer(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->wheelSteerPrev[wheel] * t + car->st.wheel[wheel].steer * (1.0f - t);
}

// FUNC 0x001EB810 car_wheel_camber
f32 car_wheel_camber(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->wheelCamberPrev[wheel] * t + car->st.wheel[wheel].camber * (1.0f - t);
}

// FUNC 0x001EB850 car_wheel_rotation
f32 car_wheel_rotation(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->wheelRotPrev[wheel] * t + car->st.wheel[wheel].rotation * (1.0f - t);
}

/* RunwayContact.u (+0x14). */
// FUNC 0x001EB890 car_contact_u
f32 car_contact_u(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->contactUPrev[wheel] * t + car->contact[wheel].u * (1.0f - t);
}

/* RunwayContact.v (+0x18). */
// FUNC 0x001EB8D0 car_contact_v
f32 car_contact_v(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return car->contactVPrev[wheel] * t + car->contact[wheel].v * (1.0f - t);
}

/* From RaceEntryCar::updateSound (T121): the wheel's position (contact x,
 * wheel height, contact z), interpolated. */
// FUNC 0x001EB910 car_wheel_pos
void car_wheel_pos(RaceCarState *car, f32 *out, int wheel, f32 t)
{
    t = t + car->interpBias;
    f32 cur = 1.0f - t;
    out[0] = car->wheelPosPrev[wheel][0] * t + car->contact[wheel].x * cur;
    out[1] = car->wheelPosPrev[wheel][1] * t + car->st.wheel[wheel].groundZ2 * cur;
    out[2] = car->wheelPosPrev[wheel][2] * t + car->contact[wheel].z * cur;
}

/* (interpolated DynWheel.shake) x 0.5 + 0.5: 0..1. */
// FUNC 0x001EB9A0 car_wheel_shake_at
f32 car_wheel_shake_at(RaceCarState *car, int wheel, f32 t)
{
    t = t + car->interpBias;
    return (car->wheelShakePrev[wheel] * t + car->st.wheel[wheel].shake * (1.0f - t)) * 0.5f + 0.5f;
}

// FUNC 0x001EB9F0 car_steer_view
f32 car_steer_view(RaceCarState *car, f32 t)
{
    t = t + car->interpBias;
    return car->steerViewPrev * t + car->steerView * (1.0f - t);
}

/* From OnBoardCamera::applyMountView.  The second term in double (1.0). */
// FUNC 0x001EBA20 car_lag_yaw
f32 car_lag_yaw(RaceCarState *car, f32 t)
{
    DynCarState *st = &car->st;
    t = t + car->interpBias;
    /* implicit: double: fptodp, dpsub, dpmul, dpadd, dptofp (the soft-float runtime) */
    return car->lagYawDiffPrev * t + st->lagYawDiff * (1.0 - t);
}

// FUNC 0x001EBAB0 car_lag_pitch
f32 car_lag_pitch(RaceCarState *car, f32 t)
{
    DynCarState *st = &car->st;
    t = t + car->interpBias;
    /* implicit: double: fptodp, dpsub, dpmul, dpadd, dptofp */
    return car->lagPitchDiffPrev * t + st->lagPitchDiff * (1.0 - t);
}

/* No callers.  1 for a pad-driven car with a controller. */
// FUNC 0x001EBB40 car_is_player
int car_is_player(RaceCarState *car)
{
    DynCarState *st = &car->st;
    if (st->noController != 0)
        return 0;
    return st->driver == DRIVER_PAD;
}
