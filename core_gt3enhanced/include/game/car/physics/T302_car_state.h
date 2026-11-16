/* T302_car_state.h - RaceCarState, one car of the DynamicsConductor
 * (translation unit T302, unit u082), 0x001EAA88..0x001EBB60.
 *
 * The conductor holds eight (DynamicsConductor +0xC8, stride 0x16DC,
 * DynamicsConductor::getCar 0x001F2B68); a data logger OneLapGhostInfo (T313)
 * holds a snapshot of one at +0x24.  The car is its parameter block
 * (CarDynamics, +0x10), its computer driver (+0xEC8), its run state
 * (DynCarState, +0xF84), the ground contacts of the four wheels (+0x14BC) and
 * the display values of the last two dynamics frames (+0x158C..).
 *
 * T302: the car's init, lap / finish queries, the finish action, the per-frame
 * step (car_step) and the display side: car_save_prev keeps the previous
 * frame's values, car_view_update computes this frame's view position, and
 * the accessors interpolate between the two with
 *     t = frac + interpBias;  value = prev * t + cur * (1 - t).
 *
 * Other names of this class: u077 / u085 DynCar (T330: a typedef of it),
 * u051 / u052 / u079 / u081 / u087 RaceCarDynamics (T154), Lupo 2 Automobile
 * (DynamicsConductor::CurrentLapTime(Automobile &)).  Offsets other units
 * documented inside the members: params (CarDynamics) 0x148 engine, 0x244
 * gear ratios; st (DynCarState, + 0xF84) 0x1368 lapDistance, 0x1370 sector,
 * 0x137C condition, 0x13AC throttle, 0x13B0 brake, 0x13B8 lap, 0x13BA gear,
 * 0x13D0 vel, 0x1414 rpm, 0x1464 finished (u015 / u016 / u017 / u049
 * hasTime), 0x1465 rank, 0x1466 turboSpoolMax, 0x1468 engineSnd, 0x14B4
 * lapStartTime; contact[0] + 0x2C (u060 lightIndex 0x14E8).
 *
 * Names: u082 (the owner), with the DynWheel / CarCondition names of T335 /
 * T280 for the values the accessors interpolate. */
#ifndef GAME_T302_CAR_STATE_H
#define GAME_T302_CAR_STATE_H

#include "game/game.h"
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics (held by value) */
#include "game/race/dynamics/T361_drive_params.h"         /* AIDriver, the computer driver (held by value) */
#include "game/sound/T288_car_sound.h"            /* DynCarState, PadVibration (held by value) */
#include "game/race/core/T153_race_course.h"          /* RunwayContact (held by value) */

class RaceOrganization;                     /* T125_race_organization.h (pointers only) */
class DynamicsConductor;                    /* T305_dynamics_conductor.h */
class RaceCourse;                           /* T153_race_course.h */
class RaceInput;                            /* T413_race_input.h */

/* Not polymorphic.  Size 0x16DC in GT3 (the stride of DynamicsConductor::
 * cars, the memset of car_init, OneLapGhostInfo +0x24..+0x1700); 0x1718 in
 * the enhanced engine (the fields at the end). */
class RaceCarState {
public:
    /* Inline: the conductor's constructor (0x001F2A10, T317) builds its eight
     * cars with it: the DynCarState (its CarCondition, 0x001D9B10), then the
     * two bytes.  DynamicsConductor::initialize sets them again. */
    RaceCarState() { gearWhineOn = 0; unk_16D1 = 0; }

    RaceOrganization  *org;                 /* 0x0000 */
    DynamicsConductor *dc;                  /* 0x0004 (u076 / u089 conductor) */
    RaceCourse        *course;              /* 0x0008 (u060 / u078; u082 / u084 void *) */
    RaceInput         *input;               /* 0x000C RaceBase::virtual_26's input of the car (u084; u078 /
                                               u083 CarDynamics *cd: its +0xCC recorder, +0x12C snapshots) */
    CarDynamics        params;              /* 0x0010 (u076 / u080 / u086 dyn; u082 CarParams); its tcs is
                                               car +0xE8C */
    f32                dt;                  /* 0x0EC0 the frame time: timeScale / 60 (car_time_scale, T304) */
    f32                interpBias;          /* 0x0EC4 added to every interpolation fraction (u085 stepAcc: the
                                               step counter of DynamicsConductor::update in slow motion) */
    AIDriver           ai;                  /* 0x0EC8 the computer driver, T362..T367 (u082 / u079 Transmission
                                               trans and u077 / u085 DynSub_EC8: typedefs of AIDriver, T361;
                                               u083 sub_EC8) */
    DynCarState        st;                  /* 0x0F84 (u076 / u080 / u086 run; u083 / u084 CarRunState) */
    RunwayContact      contact[4];          /* 0x14BC per wheel, FL FR RL RR (u077 GroundQuery; u083 CarContact) */
    /* The display values of this frame and of the previous one ("Prev"). */
    f32                viewPos[3];          /* 0x158C (x, z, -y) of st.pos, less originOffset along rot[0] */
    f32                viewGroundPos[3];    /* 0x1598 the same of st.groundPos / groundRot[0] */
    f32                steerView;           /* 0x15A4 0..1, 0.5 straight (st.steer / params.steerLock) */
    f32                viewPosPrev[3];      /* 0x15A8 */
    f32                viewGroundPosPrev[3]; /* 0x15B4 */
    f32                anglePrev[3];        /* 0x15C0 st.pitch, roll, yaw */
    f32                groundAnglePrev[2];  /* 0x15CC st.groundPitch, groundRoll */
    f32                wheelStrokePrev[4];  /* 0x15D4 DynWheel.stroke (u082 wheelF18Prev) */
    f32                wheelSteerPrev[4];   /* 0x15E4 DynWheel.steer */
    f32                wheelCamberPrev[4];  /* 0x15F4 DynWheel.camber (u082 wheelF10Prev) */
    f32                wheelLateralPosPrev[4]; /* 0x1604 DynWheel.lateralPos (u082 wheelF14Prev) */
    f32                wheelRotPrev[4];     /* 0x1614 DynWheel.rotation */
    f32                wheelBrakeHeatPrev[4]; /* 0x1624 DynWheel.brakeHeat (u082 wheelF94Prev, u080 wheelHeatPrev) */
    f32                contactUPrev[4];     /* 0x1634 RunwayContact.u, the road's light factor (u082 contactF14Prev) */
    f32                contactVPrev[4];     /* 0x1644 RunwayContact.v (u082 contactF18Prev) */
    f32                wheelPosPrev[4][3];  /* 0x1654 contact pos x, DynWheel.groundZ2, contact pos z */
    f32                wheelShakePrev[4];   /* 0x1684 DynWheel.shake (u082 wheelF98Prev) */
    f32                steerViewPrev;       /* 0x1694 */
    f32                lagYawDiffPrev;      /* 0x1698 st.lagYawDiff */
    f32                lagPitchDiffPrev;    /* 0x169C st.lagPitchDiff */
    f32                lapDistancePrev;     /* 0x16A0 st.lapDistance; the lap counter (T307) moves it by a course
                                               length when the car crosses the line (u083 distance) */
    s16                steerIn;             /* 0x16A4 (u078 / u079; u077 h16A4) */
    s16                steerInPrev;         /* 0x16A6 */
    f32                speed;               /* 0x16A8 |st.speedX|, m/s */
    f32                topSpeed;            /* 0x16AC the highest speed (u077 maxSpeed) */
    u8                 unk_16B0[0x14];      /* 0x16B0 */
    u32                bestLap;             /* 0x16C4 ms, 0x157529FF: none (car_best_lap; u015 / u016 / u083
                                               totalTime) */
    f32                crashLevel;          /* 0x16C8 the loudest crash of the frame (T288) */
    s8                 crashKind;           /* 0x16CC its kind */
    s8                 crashArg;            /* 0x16CD its argument */
    s16                countdown;           /* 0x16CE frames until the driver gets the car back (u084; u077 h16CE) */
    u8                 gearWhineOn;         /* 0x16D0 the car's gearbox is not the normal one (CarEquipments
                                               F_00234EC0 != 0x13; T288's gear whine; u085 b16D0) */
    u8                 unk_16D1;            /* 0x16D1 0 at construction, 1 from DynamicsConductor::initialize */
    u8                 unk_16D2[2];         /* 0x16D2 */
    PadVibration       vib;                 /* 0x16D4 the pad actuators (u077 b16D4..b16D6) */
    f32                speedView;           /* 0x16D8 the speed, followed at 400 km/h per second (u077 f16D8) */

    /* ---- The enhanced engine: Lupo 2's per-car state (ref/enhanced/
     * INTERFACES.md), after GT3's 0x16DC bytes.  Lupo 2 keeps it in its car
     * object (its physics block at car+0xD8 is GT3's st): the Dualnote, the
     * wing and the Pod share the word car+0x5F0 and the bytes +0x5F4 / +0x5F5
     * there (one feature per car); here each has its own field.  car_init
     * clears them; a snapshot (car_copy / car_restore, OneLapGhostInfo)
     * holds GT3's 0x16DC bytes only (RACE_CAR_STATE_GT3_SIZE), so the replay
     * and ghost slots keep their layout and a restore leaves these as they
     * are.  The writers and readers are the wave-1 areas' ports. */
    f32                batteryCharge;       /* 0x16DC the Dualnote's battery, kJ (physics: full at the car's set-up,
                                               the regeneration / assist; Lupo 2 car+0x5F0) */
    f32                batteryCapacity;     /* 0x16E0 its full charge, kJ (physics at the set-up, Lupo 2's constant
                                               CAP; hud C1 shows batteryCharge / batteryCapacity, as Lupo 2's
                                               L2 0x001E02C0) */
    f32                wingPosition;        /* 0x16E4 the moving part (CAR_KIND_WING_*): 0 down .. 1 up (physics's
                                               updater L2 0x001E1FD0; rendering feeds CarModel.wing; Lupo 2
                                               car+0x5F0) */
    f32                displayRpm;          /* 0x16E8 the tachometer's rpm, a spring after st.rpm (physics: reset at
                                               the set-up, stepped in the drivetrain step; hud C2; Lupo 2
                                               0x004A9520[carIndex], L2 0x001E4658 / 0x001E4698) */
    f32                displayRpmSpeed;     /* 0x16EC its rate (Lupo 2 0x004A9540[carIndex]) */
    f32                frontSlip;           /* 0x16F0 the front wheels' mean folded slip (physics: car_tyre_lat_forces;
                                               race: the AI's steering gain, race_logic.md A7; Lupo 2 block+0x424) */
    f32                cvtRatio;            /* 0x16F4 the CVT's ratio factor (GEARBOX_CVT; physics only; Lupo 2
                                               block+0x3A4) */
    f32                animChannel[4];      /* 0x16F8 the model animation channels, smoothed inputs (physics:
                                               car_view_update / car_input_update, L2 0x001E2AA0; rendering and the
                                               per-car animation object read them, L2 0x001E2988; Lupo 2
                                               car+0x80C) */
    s8                 animEvent[4];        /* 0x1708 the model animation event bytes (physics: the shift callbacks
                                               L2 0x001E2A80; rendering, L2 0x001E28C0; Lupo 2 car+0x808) */
    u8                 hybrid;              /* 0x170C 1: a Dualnote, DRIVETRAIN layout 2 / awdType 5 (physics at the
                                               set-up; hud C1 draws the gauge only then, sound S6's whine; Lupo 2:
                                               GearCalc.driveType == 7, L2 0x001E0288) */
    s8                 batteryFlow;         /* 0x170D the battery's energy flow of the step, -100..100, < 0: the
                                               motors draw (physics, cleared each race step; hud C1: blue when
                                               >= 0, else magenta; Lupo 2 car+0x5F5) */
    u8                 wingUp;              /* 0x170E the moving part is deployed (physics's hysteresis; Lupo 2
                                               car+0x5F4) */
    u8                 podParking;          /* 0x170F the Pod's parking counter 0..0x22, 0x22: it sits (physics,
                                               L2 0x001E1C88 / 0x001E1BC8; race: the stop-area rule, L2 0x001E1CF0;
                                               Lupo 2 car+0x5F0, a byte) */
    u8                 podEmotion;          /* 0x1710 the Pod's emotion: low nibble the current, high the previous
                                               (physics's emotion machine L2 0x001E1308 / 0x001E1520; rendering:
                                               the face select; Lupo 2 car+0x5F1) */
    u8                 podEmotionTimer;     /* 0x1711 frames left of the emotion, 0xFF: held (Lupo 2 car+0x5F2) */
    s8                 podEmotionHold;      /* 0x1712 (Lupo 2 car+0x5F3) */
    u8                 podAnimFrame;        /* 0x1713 the Pod's animation frame 0..0x2F (physics; rendering: SWITCH 9,
                                               CarModel.podState; Lupo 2 car+0x5F4) */
    s8                 podLean;             /* 0x1714 the Pod's tail / look lean -30..30 (physics; rendering: SWITCH 7,
                                               CarModel.podLean; Lupo 2 car+0x5F5) */
    u8                 podSound;            /* 0x1715 the emotion the Pod's voice plays (physics; sound S1, L2
                                               0x001E19D0; Lupo 2 car+0x5F6) */
};
EGCS_SIZE(RaceCarState, 0x1718);

/* GT3's RaceCarState: the bytes a snapshot holds (OneLapGhostInfo::car,
 * car_copy / car_restore, T351). */
#define RACE_CAR_STATE_GT3_SIZE 0x16DC

/* memset 0, then org, dc and the computer driver (ai_construct, T367).  From
 * DynamicsConductor::initialize. */
void car_init(RaceCarState *car, DynamicsConductor *dc, RaceOrganization *org); /* 0x001EAA88 */
int  car_is_held(RaceCarState *car);                                   /* 0x001EAAE8 the countdown runs */
/* The lap to display: 1 before the first, the last one done once finished
 * (the conductor's lap count is fetched and not used). */
int  car_lap_display(RaceCarState *car);                               /* 0x001EAB08 */
int  car_no_controller(RaceCarState *car);                             /* 0x001EAB70 st.noController != 0 */
/* 1 when the race's time limit is over or the car has done the conductor's
 * laps (one more when it is less than half a course length into the lap). */
int  car_race_done(RaceCarState *car);                                 /* 0x001EAB80 */
void car_set_wheel_brakes(RaceCarState *car, f32 brake);               /* 0x001EAC60 the four DynWheel.brake */
void car_set_brake(RaceCarState *car, f32 brake);                      /* 0x001EAC90 st.brake and the wheels */
/* car_finish's mode (the callers pass the numbers: 3 the conductors at the
 * end of a car's race, 2 a failed licence test / a disqualification, 4 the
 * free run).  The control modes are T341's (DynCarState.ctrlMode). */
enum {
    CAR_FINISH_DRIVE_ON  = 0,               /* car_mode_computer: the computer drives on (CTRL_COMPUTER) */
    CAR_FINISH_PULL_OVER = 1,               /* car_mode_stop_brake: CTRL_STOP_BRAKE (line 6; ? pulls over) */
    CAR_FINISH_STOP      = 2,               /* car_mode_stop: CTRL_STOP, the car stops */
    CAR_FINISH_RACE_END  = 3,               /* ai_reduce_pace, then as CAR_FINISH_DRIVE_ON */
    CAR_FINISH_AUTO      = 4                /* CAR_FINISH_STOP with DYN_FLAG_COUNTED_GROUND, else DRIVE_ON */
};

/* The car crosses the finish line (the conductors' gate handlers).  mode 4:
 * 2 for a car with DYN_FLAG_COUNTED_GROUND, else 0.  The first time it sets
 * finished (and stops there in st.ctrlMode 2..4); then, for a pad-driven
 * car, F_001F6548 and ai_apply_rules(ai, params.driveClass) (T362), and the
 * driving mode (T341): 1 / 2 car_mode_stop_brake / car_mode_stop, 3
 * ai_reduce_pace then car_mode_computer, others car_mode_computer. */
void car_finish(RaceCarState *car, int mode);                          /* 0x001EACB8 */
void car_save_prev(RaceCarState *car);                                 /* 0x001EADB8 the "Prev" values */
/* The yaw and the wheel rotations kept within +-2 pi (the Prev values moved
 * by the same turn), the wheels turned by spin / radius x dt (not while
 * held), this frame's view positions and steerView. */
void car_view_update(RaceCarState *car);                               /* 0x001EAFC0 */
void car_step(RaceCarState *car);                                      /* 0x001EB218 the dynamics frame of one car */
int  car_lap_time(RaceCarState *car);                                  /* 0x001EB2F8 dc->CurrentLapTime(car) */
u8   car_brake_assist(RaceCarState *car);                              /* 0x001EB330 st.brakeAssist */
u8   car_traction_assist(RaceCarState *car);                           /* 0x001EB338 st.tractionAssist */
int  car_oil_needs_change(RaceCarState *car);                          /* 0x001EB340 st.condition.oilNeedsChange()
                                                                          (u082 car_condition_flag) */
int  car_handbrake_on(RaceCarState *car);                              /* 0x001EB360 st.handbrake > 0.25 */
/* The interpolated values at the conductor's current fraction
 * (replaySpeed, DynamicsConductor +0xB7B0). */
f32  car_view_x_now(RaceCarState *car);                                /* 0x001EB388 */
f32  car_view_y_now(RaceCarState *car);                                /* 0x001EB3B0 */
f32  car_view_z_now(RaceCarState *car);                                /* 0x001EB3D8 */
/* Interpolated at fraction t (+ interpBias) between Prev and this frame. */
f32  car_view_x(RaceCarState *car, f32 t);                             /* 0x001EB400 */
f32  car_lap_distance(RaceCarState *car, f32 t);                       /* 0x001EB430 */
f32  car_view_y(RaceCarState *car, f32 t);                             /* 0x001EB460 */
f32  car_view_z(RaceCarState *car, f32 t);                             /* 0x001EB490 */
f32  car_pitch(RaceCarState *car, f32 t);                              /* 0x001EB4C0 */
f32  car_roll(RaceCarState *car, f32 t);                               /* 0x001EB4F0 */
f32  car_yaw(RaceCarState *car, f32 t);                                /* 0x001EB520 */
f32  car_ground_view_x(RaceCarState *car, f32 t);                      /* 0x001EB550 */
f32  car_ground_view_y(RaceCarState *car, f32 t);                      /* 0x001EB580 */
f32  car_ground_view_z(RaceCarState *car, f32 t);                      /* 0x001EB5B0 */
f32  car_ground_pitch(RaceCarState *car, f32 t);                       /* 0x001EB5E0 */
f32  car_ground_roll(RaceCarState *car, f32 t);                        /* 0x001EB610 */
f32  car_yaw2(RaceCarState *car, f32 t);                               /* 0x001EB640 the same code as car_yaw */
/* No callers.  view y - ground view y, at least 0 (interpBias is added
 * twice). */
f32  car_height_above_ground(RaceCarState *car, f32 t);                /* 0x001EB670 */
f32  car_dirt(RaceCarState *car);                                      /* 0x001EB6D8 st.condition.dirt, 0 on course
                                                                          kind 1 (u082 car_condition_f0C) */
f32  car_wheel_lateral_pos(RaceCarState *car, int wheel, f32 t);       /* 0x001EB710 (u082 car_wheel_f14) */
/* The axle's params.susp[axle].gapOffset (params +0x2C4 + axle x 0x44) less
 * the interpolated stroke. */
f32  car_wheel_spring(RaceCarState *car, int wheel, f32 t);            /* 0x001EB750 */
/* -/+ wheelbase / 2; t (the callers set $f12 as for the other accessors) is
 * not read. */
f32  car_wheel_axle_x(RaceCarState *car, int wheel, f32 t);            /* 0x001EB7A8 */
f32  car_wheel_steer(RaceCarState *car, int wheel, f32 t);             /* 0x001EB7D0 */
f32  car_wheel_camber(RaceCarState *car, int wheel, f32 t);            /* 0x001EB810 (u082 car_wheel_f10) */
f32  car_wheel_rotation(RaceCarState *car, int wheel, f32 t);          /* 0x001EB850 */
/* RunwayContact u / v, the road's light factors (T181; u082 car_contact_f14
 * / f18). */
f32  car_contact_u(RaceCarState *car, int wheel, f32 t);               /* 0x001EB890 */
f32  car_contact_v(RaceCarState *car, int wheel, f32 t);               /* 0x001EB8D0 */
/* out: contact x, wheel height, contact z, interpolated. */
void car_wheel_pos(RaceCarState *car, f32 *out, int wheel, f32 t);     /* 0x001EB910 */
/* shake x 0.5 + 0.5 (u082 car_wheel_f98; car_wheel_shake until the sweep,
 * an overload of T345's step car_wheel_shake). */
f32  car_wheel_shake_at(RaceCarState *car, int wheel, f32 t);          /* 0x001EB9A0 */
f32  car_steer_view(RaceCarState *car, f32 t);                         /* 0x001EB9F0 */
f32  car_lag_yaw(RaceCarState *car, f32 t);                            /* 0x001EBA20 (1.0 - t in double) */
f32  car_lag_pitch(RaceCarState *car, f32 t);                          /* 0x001EBAB0 */
int  car_is_player(RaceCarState *car);                                 /* 0x001EBB40 no callers: driver 0 with a
                                                                          controller */

/* ---- The enhanced engine (Lupo 2; ref/enhanced/ported/physics.md) ------- */

/* The model animation channels and events (RaceCarState.animChannel /
 * animEvent; Lupo 2 car+0x80C / +0x808).  car_anim_channel also derives
 * channels 4..8 (L2 0x001E2988): rendering reads them. */
void car_anim_channel_set(RaceCarState *car, int channel, f32 value);   /* L2 0x001E2AA0 */
f32  car_anim_channel(RaceCarState *car, int channel);                  /* L2 0x001E2988 */
void car_anim_event(RaceCarState *car, int index, int value);           /* L2 0x001E2A80 */
int  car_anim_event_get(RaceCarState *car, int index);                  /* L2 0x001E28C0 */

/* The Toyota Pod (CAR_KIND_POD): its emotion machine (RaceCarState.
 * podEmotion, podEmotionTimer, podEmotionHold (the cool-down), podAnimFrame
 * (the bob oscillator 0..47), podLean (-30..30), podSound (the state entered
 * this step, 0 none: the voice's one-shot, cleared at the next step); Lupo 2
 * L2 0x001E12A0..0x001E1BC8) and its parking counter (podParking, L2
 * 0x001E1BC8..0x001E1CF0). */
enum { POD_PARKING_SEATED = 0x22 };         /* the counter when it sits (34 steps) */
extern const u8 g_podEmotionCategory[11];   /* the face category per state (rendering's SWITCH 8) */
int  car_is_pod(RaceCarState *car);                                     /* L2 0x001E1288 */
int  pod_emotion(RaceCarState *car);                                    /* L2 0x001E12E8 the current state */
void pod_emotion_reset(RaceCarState *car);                              /* L2 0x001E12A0 (DynCar_initialize) */
void pod_emotion_set(RaceCarState *car, int state, int time);           /* L2 0x001E1308 */
int  pod_triangle(int n);                                               /* L2 0x001E1470 */
void pod_emotion_step(RaceCarState *car);                               /* L2 0x001E1520 (DynamicsConductor::update) */
void pod_crash_reaction(RaceCarState *car, int kind, int arg);          /* L2 0x001E1A00 / 0x001E1A48 / 0x001E1B80 */
int  pod_sound(RaceCarState *car);                                      /* L2 0x001E19D0 (sound: the voice) */
void pod_parking_init(RaceCarState *car, int seated);                   /* L2 0x001E1BC8 (DynCar_initialize) */
void pod_parking_step(RaceCarState *car);                               /* L2 0x001E1C88 (DynamicsConductor::update) */
int  pod_is_seated(RaceCarState *car);                                  /* L2 0x001E1CF0 (race: the stop-area rule) */

/* The concept cars' moving wing (CAR_KIND_WING_*; RaceCarState.wingPosition
 * / wingUp; Lupo 2 L2 0x001E1F30 / 0x001E1FD0). */
void car_wing_init(RaceCarState *car);                                  /* DynCar_initialize */
void car_wing_step(RaceCarState *car);                                  /* DynamicsConductor::update */

#endif /* GAME_T302_CAR_STATE_H */
