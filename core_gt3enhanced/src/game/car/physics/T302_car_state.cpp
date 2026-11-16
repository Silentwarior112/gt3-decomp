/* T302 - RaceCarState, the car's state: 0x001EAA88..0x001EBB60.
 *
 * The car's init, lap and finish queries, the finish action, the per-frame
 * step (car_step), and the display side: car_save_prev keeps the previous
 * frame's values, car_view_update computes this frame's view position, and
 * the accessors interpolate between the two with
 *     t = frac + interpBias;  value = prev * t + cur * (1 - t). */
#include "game/car/physics/car_motion_internal.h"
#include "game/car/physics/T302_car_state.h"
#include "game/car/physics/T347_ground_surface.h"   /* ground_is_off_course (the enhanced engine's Pod) */

static void car_anim_view_channels(RaceCarState *car);     /* the enhanced engine (below) */

/* From DynamicsConductor::initialize. */
// FUNC 0x001EAA88 car_init
void car_init(RaceCarState *car, DynamicsConductor *dc, RaceOrganization *org)
{
    /* The enhanced engine: the whole car, the fields after GT3's 0x16DC
     * bytes included (ref/enhanced/INTERFACES.md). */
    memset((void *)car, 0, sizeof(RaceCarState));
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
    car_anim_view_channels(car);            /* Lupo 2: the model animation channels 2 / 3 (L2 0x001EAA98) */
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

/* ======================================================================== */
/* The enhanced engine: Lupo 2's per-car model animation inputs, the Toyota */
/* Pod's emotions and parking, the concept cars' moving wing (Lupo 2 L2     */
/* 0x001E1288..0x001E2AA0; ref/enhanced/ported/physics.md).  They act only  */
/* on a car with a special kind (CarDynamics.specialKind, from the car code */
/* in SetupCarSpec): none of GT3's cars.                                    */
/* ======================================================================== */

/* ---- the model animation channels and events (Lupo 2 car+0x80C / +0x808) */

/* Channel `channel' (0..3) = value (Lupo 2 L2 0x001E2AA0); others ignored. */
void car_anim_channel_set(RaceCarState *car, int channel, f32 value)
{
    if (channel < 4)
        car->animChannel[channel] = value;
}

/* An event byte (Lupo 2 L2 0x001E2A80): [0] the gear shift, +1 up, -1 down,
 * 0 again when the shift is over (car_gear_select, car_engine_speed). */
void car_anim_event(RaceCarState *car, int index, int value)
{
    if (index < 4)
        car->animEvent[index] = (s8)value;
}

/* The event byte `index' (Lupo 2 L2 0x001E28C0), 0 past the four. */
int car_anim_event_get(RaceCarState *car, int index)
{
    if (index < 4)
        return car->animEvent[index];
    return 0;
}

/* An axle's suspension load share for channels 4 / 5: its wheels' damper
 * force over the axle's sprung weight (Lupo 2 L2 0x001E28E0). */
static f32 car_anim_axle_load(RaceCarState *car, int axle)
{
    f32 force = car->st.wheel[axle * 2].damperForce + car->st.wheel[axle * 2 + 1].damperForce;

    return force / car->params.sprungWeight[axle];
}

/* Lupo 2's steering spring of channel 8 (L2 0x001E2A10): two floats at its
 * block+0x3B0 / +0x3B4, which are GT3's unused DynCarState.unk_3A8 bytes 4..11
 * here (ref/enhanced/INTERFACES.md: no GT3 code touches them). */
static f32 *car_anim_steer_spring(RaceCarState *car)
{
    return (f32 *)&car->st.unk_3A8[4];
}

/* Model animation channel `channel' (Lupo 2 L2 0x001E2988; rendering and
 * Lupo 2's per-car animation object read it): 0..3 as set (2 the smoothed
 * throttle, 3 the smoothed brake: car_view_update), 4 / 5 the front / rear
 * suspension load share, 6 none (0), 7 the longitudinal acceleration in g,
 * 8 a spring after the steering (stepped at 1/60 s by each read, as in Lupo
 * 2).  0 past 8. */
f32 car_anim_channel(RaceCarState *car, int channel)
{
    if (channel < 4)
        return car->animChannel[channel];
    switch (channel - 4) {
    case 0:
        return car_anim_axle_load(car, 0);
    case 1:
        return car_anim_axle_load(car, 1);
    case 3:
        return car->st.accelX / 9.799999f;
    case 4: {
        f32 *spring = car_anim_steer_spring(car);
        f32 target = car->st.steer * 11.25f;

        spring[1] = spring[1] + ((target - spring[0]) * 200.0f - spring[1] * 20.0f) * 0.016666666f;
        spring[0] = spring[0] + spring[1] * 0.016666666f;
        return spring[0];
    }
    }
    return 0.0f;
}

/* The smoothing of channels 2 and 3 (Lupo 2 L2 0x001EAA98..0x001EAAF0, at
 * the end of car_view_update), Lupo 2's .sdata (L2 gp-0x4B30, gp-0x4B2C;
 * not const: 1 - k is computed at run time there). */
static f32 g_animThrottleBlend = 0.45f;
static f32 g_animBrakeBlend = 0.29999998f;

static void car_anim_view_channels(RaceCarState *car)
{
    f32 k = g_animThrottleBlend;
    f32 old = car_anim_channel(car, 2);

    car_anim_channel_set(car, 2, car->st.throttleOut * k + old * (1.0f - k));
    k = g_animBrakeBlend;
    old = car_anim_channel(car, 3);
    car_anim_channel_set(car, 3, car->st.brake * k + old * (1.0f - k));
}

/* ---- the Toyota Pod: emotions (Lupo 2 L2 0x001E12A0..0x001E1BC8) -------- */

/* Lupo 2's three tables (L2 0x002DE920 / 0x002DE930 / 0x002DE940) by state
 * (0 neutral, 1 wink, 2 genki, 3 kyoro, 4 okoru, 5 odoroku, 6 iya, 7
 * sabishii, 8 kowai, 9 naku, 10 nemui): the default hold in steps (3 s),
 * the attribute (+1 / -1 the lean drifts up / down, 2 the bob runs), the
 * face category (rendering's SWITCH 8). */
static const u8 g_podEmotionTime[11] = { 0, 180, 0, 180, 180, 180, 180, 180, 180, 180, 0 };
static const s8 g_podEmotionAttr[11] = { 0, 2, 1, 2, 1, 1, -1, -1, -1, -1, -1 };
const u8 g_podEmotionCategory[11] = { 0, 4, 4, 4, 1, 1, 1, 2, 2, 2, 3 };

/* The car is the Pod (Lupo 2 L2 0x001E1288: params+0x32 == 1). */
int car_is_pod(RaceCarState *car)
{
    return car->params.specialKind == CAR_KIND_POD;
}

/* The Pod's current emotion (Lupo 2 L2 0x001E12E8: the low nibble). */
int pod_emotion(RaceCarState *car)
{
    return car->podEmotion & 0xF;
}

/* The Pod's emotion state at a race's start (Lupo 2 L2 0x001E12A0, from
 * DynCar_initialize): neutral, a one-second grace in which a weak request
 * is refused. */
void pod_emotion_reset(RaceCarState *car)
{
    if (!car_is_pod(car))
        return;
    car->podSound = 0;
    car->podEmotionTimer = 60;
    car->podEmotion = 0;
    car->podEmotionHold = 0;
    car->podAnimFrame = 0;
    car->podLean = 0;
}

/* Ask for emotion `state' (Lupo 2 L2 0x001E1308): `time' -1 the state's
 * default, 0 weak (only while nothing else runs), 255 locked (only a reset
 * clears it; the only request honoured during a cool-down), else that many
 * steps.  A change of face category starts a cool-down (24 steps, 36
 * leaving a category) that refuses other requests. */
void pod_emotion_set(RaceCarState *car, int state, int time)
{
    int cur;

    if (!car_is_pod(car))
        return;
    if (car->podEmotionHold > 0 && time != 0xFF)
        return;
    if (car->podEmotionTimer == 0xFF)
        return;
    if (time == 0 && car->podEmotionTimer != 0)
        return;
    cur = pod_emotion(car);
    if (cur != state) {
        car->podSound = (u8)state;
        car->podEmotion = (u8)((cur << 4) | state);
    }
    if (time == -1)
        car->podEmotionTimer = g_podEmotionTime[state];
    else
        car->podEmotionTimer = (u8)time;
    if (g_podEmotionCategory[cur] != g_podEmotionCategory[state])
        car->podEmotionHold = (g_podEmotionCategory[cur] != 0) ? 36 : 24;
}

/* The lean / bob oscillator's triangle (Lupo 2 L2 0x001E1470): 0..47 ->
 * -12..12, 0 at 0 and 24. */
int pod_triangle(int n)
{
    if (n < 37) {
        if (n < 13)
            return n;
        return 24 - n;
    }
    return n - 48;
}

/* Every wheel on off-course ground (Lupo 2 L2 0x001E14A0). */
static int pod_all_wheels_off(RaceCarState *car)
{
    int i;

    for (i = 0; i < 4; i++)
        if (!ground_is_off_course(&car->contact[i]))
            return 0;
    return 1;
}

/* The Pod's emotion step (Lupo 2 L2 0x001E1520, from DynamicsConductor::
 * update before the cars move): the first that applies of finished (1st:
 * genki, else sabishii), WRONG WAY (iya), in the pit lane (kyoro), side by
 * side with a car (wink, 2 s), in a full slipstream (genki, 2 s), every
 * wheel off the course (nemui), else back to neutral; then the timer, the
 * cool-down, the lean (to +-30 while the state's attribute is +-1, back to 0
 * otherwise, at the oscillator's rest points) and the bob (the oscillator
 * runs while the lean is 0 and the attribute is 2 or a cycle is under way).
 * The voice's one-shot (podSound) is cleared first. */
void pod_emotion_step(RaceCarState *car)
{
    int other, level, attr, cur;

    if (!car_is_pod(car))
        return;
    car->podSound = 0;
    level = car->dc->pairs.strongest(car->st.carIndex, &other);
    if (car->st.finished)
        pod_emotion_set(car, (car->st.rank == 1) ? 2 : 7, 0);
    else if (car->st.msgKind == 1)
        pod_emotion_set(car, 6, 0);
    else if (car->st.flags & DYN_FLAG_COUNTED_GROUND)
        pod_emotion_set(car, 3, 0);
    else if (level == 5)
        pod_emotion_set(car, 1, 120);
    else if (car->st.slipBlend == 1.0f)
        pod_emotion_set(car, 2, 120);
    else if (pod_all_wheels_off(car))
        pod_emotion_set(car, 10, 0);
    else if (car->podEmotionTimer == 0)
        pod_emotion_set(car, 0, -1);

    if (car->podEmotionTimer != 0 && car->podEmotionTimer != 0xFF) {
        car->podEmotionTimer = car->podEmotionTimer - 1;
        if (car->podEmotionTimer == 0)
            pod_emotion_set(car, 0, -1);
    }
    if (car->podEmotionHold > 0) {
        car->podEmotionHold = car->podEmotionHold - 1;
        if (g_podEmotionCategory[pod_emotion(car)] == 0 && car->podEmotionHold < 25)
            car->podEmotionHold = 0;
    }

    cur = pod_emotion(car);
    attr = g_podEmotionAttr[cur];
    if (attr == 1 || attr == -1) {
        if (pod_triangle(car->podAnimFrame) == 0) {
            int lean = car->podLean;

            if (attr == 1) {
                lean = lean + 1;
                if (!(lean < 31))
                    lean = 30;
            } else {
                lean = lean - 1;
                if (lean < -30)
                    lean = -30;
            }
            car->podLean = (s8)lean;
        }
    } else if (pod_triangle(car->podAnimFrame) == 0) {
        if (car->podLean > 0)
            car->podLean = car->podLean - 1;
        else if (car->podLean < 0)
            car->podLean = car->podLean + 1;
    }
    if (car->podLean == 0
        && (g_podEmotionAttr[pod_emotion(car)] == 2 || pod_triangle(car->podAnimFrame) != 0)) {
        int frame = car->podAnimFrame + 1;

        if (!((u8)frame < 0x30))
            frame = 0;
        car->podAnimFrame = (u8)frame;
    }
}

/* The Pod's reaction to a crash (Lupo 2 L2 0x001E1A00 / 0x001E1A48 /
 * 0x001E1B80, at the head of its car_report_crash; here from the five call
 * sites of car_report_crash, T167 / T375 / T376): a wall naku; a car by the
 * part of the Pod that was hit (corners 1, 2 odoroku, 5, 6 kowai, the others
 * okoru; its edges 8..10 odoroku, 11 / 15 okoru, 12..14 kowai, held 3.5 s);
 * a loose course object odoroku. */
void pod_crash_reaction(RaceCarState *car, int kind, int arg)
{
    if (!car_is_pod(car))
        return;
    if (kind == CRASH_KIND_WALL) {
        pod_emotion_set(car, 9, -1);
    } else if (kind == CRASH_KIND_CAR) {
        if ((u32)(arg - 1) < 2u)
            pod_emotion_set(car, 5, -1);
        else if ((u32)(arg - 5) < 2u)
            pod_emotion_set(car, 8, -1);
        else if (arg < 8)
            pod_emotion_set(car, 4, -1);
        else if (arg < 11)
            pod_emotion_set(car, 5, 210);
        else if (arg == 11 || arg == 15)
            pod_emotion_set(car, 4, 210);
        else
            pod_emotion_set(car, 8, 210);
    } else if (kind == CRASH_KIND_GADGET) {
        pod_emotion_set(car, 5, -1);
    }
}

/* The emotion the Pod's voice plays this frame, 0 none or not a Pod (Lupo
 * 2 L2 0x001E19D0, from RaceEntryCar::updateSound: sound S1).  Set when the
 * Pod enters a state, cleared at the start of the next emotion step, so the
 * sound frame after DynamicsConductor::update still sees it. */
int pod_sound(RaceCarState *car)
{
    if (!car_is_pod(car))
        return 0;
    return car->podSound;
}

/* ---- the Toyota Pod: parking (Lupo 2 L2 0x001E1BC8..0x001E1CF0) --------- */

/* At the car's set-up (Lupo 2 L2 0x001E1BC8): seated (34) or not. */
void pod_parking_init(RaceCarState *car, int seated)
{
    if (!car_is_pod(car))
        return;
    car->podParking = seated ? POD_PARKING_SEATED : 0;
}

/* It wants to sit: the side brake full on below 1 km/h (Lupo 2 L2
 * 0x001E1C18). */
static int pod_wants_to_sit(RaceCarState *car)
{
    f32 vx, vy;

    if (!car_is_pod(car))
        return 0;
    if (car->st.handbrake < 1.0f)
        return 0;
    vx = car->st.speedX;
    vy = car->st.speedY;
    if (0.27777776f < boot_sqrtf_v(vx * vx + vy * vy))
        return 0;
    return 1;
}

/* The parking counter, a step after the input (Lupo 2 L2 0x001E1C88, from
 * DynamicsConductor::update): up to 34 while it wants to sit, down to 0
 * otherwise.  The squat (pod_squat_spring_rate, T298) lets the springs go
 * by it. */
void pod_parking_step(RaceCarState *car)
{
    if (!car_is_pod(car))
        return;
    if (pod_wants_to_sit(car)) {
        int count = car->podParking + 1;

        if (!(count < POD_PARKING_SEATED + 1))
            count = POD_PARKING_SEATED;
        car->podParking = (u8)count;
    } else if (car->podParking != 0) {
        car->podParking = car->podParking - 1;
    }
}

/* It has sat down (Lupo 2 L2 0x001E1CF0; race_logic.md's stop-area rule). */
int pod_is_seated(RaceCarState *car)
{
    return car->podParking == POD_PARKING_SEATED;
}

/* ---- the concept cars' moving wing (Lupo 2 L2 0x001E1E00..0x001E2140) --- */

/* The wing's mode (Lupo 2 L2 0x001E1E00): 1 a step up / down with the speed
 * (kinds 2, 8, 10), 2 a slide with the speed (kind 9), 0 none. */
static int car_wing_mode(RaceCarState *car)
{
    switch (car->params.specialKind) {
    case CAR_KIND_WING_GTR:
    case CAR_KIND_WING_XF:
    case CAR_KIND_WING_W12:
        return 1;
    case CAR_KIND_WING_CIEN:
        return 2;
    }
    return 0;
}

/* Mode 1's speeds (Lupo 2 L2 0x001E1E58): up from *deploy, down below
 * *retract, m/s: 80 / 60 km/h; kind 8 70 / 65 mph; kind 10 120 / 80 km/h. */
static void car_wing_speeds(RaceCarState *car, f32 *deploy, f32 *retract)
{
    switch (car->params.specialKind) {
    case CAR_KIND_WING_XF:
        *retract = 29.056803f;
        *deploy = 31.29194f;
        break;
    case CAR_KIND_WING_W12:
        *retract = 22.22222f;
        *deploy = 33.33333f;
        break;
    default:
        *retract = 16.666664f;
        *deploy = 22.22222f;
        break;
    }
}

/* Mode 2's position (Lupo 2 L2 0x001E1EB0): 0 to 35 mph, up to 1 at 50 mph. */
static f32 car_wing_slide(f32 speed)
{
    if (speed <= 15.64597f)
        return 0.0f;
    if (22.351387f <= speed)
        return 1.0f;
    return (speed - 15.64597f) / 6.7054167f;
}

/* The wing's travel rate per second (Lupo 2 L2 0x001E1EF8): 0.5, kind 9 2. */
static f32 car_wing_rate(RaceCarState *car)
{
    if (car->params.specialKind == CAR_KIND_WING_CIEN)
        return 2.0f;
    return 0.5f;
}

/* At the car's set-up (Lupo 2 L2 0x001E1F30, from DynCar_initialize): down,
 * or already up at a rolling start's speed. */
void car_wing_init(RaceCarState *car)
{
    int mode = car_wing_mode(car);

    if (mode == 0)
        return;
    car->wingPosition = 0.0f;
    car->wingUp = 0;
    if (mode == 1) {
        f32 deploy, retract;

        car_wing_speeds(car, &deploy, &retract);
        if (deploy <= car->st.speedX) {
            car->wingPosition = 1.0f;
            car->wingUp = 1;
        }
    } else {
        car->wingPosition = car_wing_slide(car->st.speedX);
    }
}

/* Each race step after the collisions (Lupo 2 L2 0x001E1FD0, from
 * DynamicsConductor::update): the wing's target by its mode, the position
 * (RaceCarState.wingPosition, 0..1; rendering's SWITCH 6 morph) moving
 * toward it at the rate. */
void car_wing_step(RaceCarState *car)
{
    int mode = car_wing_mode(car);
    f32 target = 0.0f;
    f32 rate, pos;

    if (mode == 0)
        return;
    if (mode == 1) {
        f32 deploy, retract;

        car_wing_speeds(car, &deploy, &retract);
        if (car->wingUp == 0) {
            if (deploy <= car->st.speedX)
                car->wingUp = 1;
        } else if (car->st.speedX < retract) {
            car->wingUp = 0;
        }
        target = (car->wingUp == 1) ? 1.0f : 0.0f;
    } else if (mode == 2) {
        target = car_wing_slide(car->st.speedX);
    }
    rate = car_wing_rate(car);
    pos = car->wingPosition;
    if (pos < target) {
        pos = pos + rate * car->dt;
        car->wingPosition = pos;
        if (target < pos)
            car->wingPosition = target;
    } else if (target < pos) {
        pos = pos - rate * car->dt;
        car->wingPosition = pos;
        if (pos < target)
            car->wingPosition = target;
    }
}
