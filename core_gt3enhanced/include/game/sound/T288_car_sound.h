/* T288_car_sound.h - the car sound parameters and evaluators (translation
 * unit T288, unit u078), 0x001DF4C0..0x001DFF40; DynCarState, the dynamic
 * state of a car; PadVibration.
 *
 * car_sound_params_init turns the car sound block of the table g_002D35B8
 * (T355) into g_0049CFD8 (x 10 km/h to m/s, percentages ...).  The
 * evaluators are called by RaceEntryCar::updateSound (T121; the tyre roll
 * through tyre_sound_roll_0789, T287) and return a volume with a pitch through the
 * pointer; the roles (wind, road, rolling tyre, gear whine, engine levels)
 * are guesses from the formulas.  The crash sound keeps the loudest crash
 * of the frame (cleared by DynamicsConductor::update every frame).
 *
 * DynCarState is RaceCarState +0xF84.  The same 0x538 bytes are u077 /
 * u085's DynState (T330: a typedef of this), u079 / u081's CarDynState
 * (T296) and u076 / u080 / u083 / u084 / u086's CarRunState (T366); the
 * names are merged from all of them (the unit in parentheses where they
 * differ).  Offsets relative to RaceCarState are + 0xF84 (u018 / u060 / u015:
 * lapDistance 0x1368, condition 0x137C, lap 0x13B8, rank 0x1465 ...).
 *
 * Not this class: u042 / u059's CarSoundParams (0x37C bytes: onboard, pos,
 * scale, engine voices, CarSoundVoice[32]) is the sound block of T121's
 * RaceEntryCar, not g_0049CFD8. */
#ifndef GAME_T288_CAR_SOUND_H
#define GAME_T288_CAR_SOUND_H

#include "game/game.h"
#include "game/car/physics/T280_car_condition.h"        /* CarCondition (held by value) */
#include "game/car/physics/T335_pad_vibration.h"        /* DynWheel (held by value) */
#include "game/car/physics/T342_car_input.h"            /* CarCtrlOut (held by value) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class TyreSoundTable;                       /* T355_setting_check.h */

/* The per-wheel force terms of the tyre, DynCarState +0x2F0 + wheel x 0x24
 * (u081 TyreForce, u080 CarWheelTorque; u078's DynSlip +0x310 is slipAngle
 * read 0x20 early).  Not polymorphic.  Size 0x24 (the array stride). */
class TyreForce {
public:
    f32 brakeTorque;                        /* 0x00 (u080 brake: opposes the spin) */
    f32 driveOut;                           /* 0x04 (F_001E7450) */
    f32 brakeOut;                           /* 0x08 */
    f32 torque;                             /* 0x0C the net torque of the frame (u080) */
    s8  slipMode;                           /* 0x10 1 the wheel pulls, -1 it skids, 0 none */
    u8  unk_11;                             /* 0x11 */
    s8  slipSign;                           /* 0x12 */
    u8  unk_13;                             /* 0x13 */
    f32 longForce;                          /* 0x14 (F_001D9098) */
    f32 sinSteer;                           /* 0x18 */
    f32 cosSteer;                           /* 0x1C */
    f32 slipAngle;                          /* 0x20 */
};
EGCS_SIZE(TyreForce, 0x24);

/* Not polymorphic.  Size 0x538 (RaceCarState +0xF84, then the RunwayContact
 * array at +0x14BC). */
class DynCarState {
public:
    f32          pos[3];                    /* 0x000 x, y, z (z up) */
    f32          rot[3][3];                 /* 0x00C rows: forward, lateral, up (F_001D8A80) */
    f32          groundPos[3];              /* 0x030 x, y and the mean height of the wheels (u081 ground[4][3]) */
    f32          groundRot[3][3];           /* 0x03C the same from the wheels' plane */
    DynWheel     wheel[4];                  /* 0x060 FL FR RL RR (u079 CarWheelState) */
    TyreForce    tyreForce[4];              /* 0x2F0 (u081 tf) */
    CarCtrlOut   ctrl;                      /* 0x380 the output of the input processing (u080 GearInput) */
    f32          axleTorque[2];             /* 0x384 (u080) */
    f32          engineWPrev;               /* 0x38C the engine speed before the clutch step */
    f32          wheelEngineW;              /* 0x390 the engine speed the wheels give */
    f32          massInvEff;                /* 0x394 1 / (mass + the free wheels' inertia) */
    f32          rollResist;                /* 0x398 */
    f32          understeer;                /* 0x39C 0..1 (u079) */
    f32          oversteer;                 /* 0x3A0 0..1 (u079) */
    f32          groundCos;                 /* 0x3A4 the ground normal z / the body's up z */
    u8           unk_3A8[0xC];              /* 0x3A8 */
    f32          wheelGap[4];               /* 0x3B4 the body above the ground at each wheel */
    f32          wheelForce[4];             /* 0x3C4 the vertical force of each corner */
    u8           unk_3D4[0x10];             /* 0x3D4 */
    f32          lapDistance;               /* 0x3E4 the course distance (DynamicsConductor::getLapDistance; u086 pos) */
    u8           carIndex;                  /* 0x3E8 (only car 0's collisions are judged, T324) */
    s8           driver;                    /* 0x3E9 DRIVER_* (below; u077 mode) */
    u8           noController;              /* 0x3EA non-zero: the conductor is not asked */
    s8           steerAssist;               /* 0x3EB (F_001E4468 / F_001E4470) */
    int          sector;                    /* 0x3EC the runway sector (RunwayContact +0x0C) */
    f32          trip;                      /* 0x3F0 the distance driven, modulo 8000 km */
    f32          tripStep;                  /* 0x3F4 the part not yet in the condition */
    CarCondition condition;                 /* 0x3F8 (u077 setting) */
    f32          clutch;                    /* 0x418 1: engaged (u079) */
    f32          steer;                     /* 0x41C rad */
    f32          unk_420;                   /* 0x420 1.0 after a reset (T336: x it with useX420) */
    f32          steerSpeed;                /* 0x424 */
    f32          throttle;                  /* 0x428 0..1 */
    f32          brake;                     /* 0x42C 0..1 */
    f32          handbrake;                 /* 0x430 0..1 */
    s16          lap;                       /* 0x434 */
    u8           gear;                      /* 0x436 0 reverse */
    u8           clutchState;               /* 0x437 CLUTCH_* (below; u079 autoClutch) */
    u8           lsdLocked[2];              /* 0x438 per axle (diff types 3 / 4) */
    u8           shiftTimer;                /* 0x43A 12 when the clutch locks; AIDriver.shiftTimer (T366) */
    u8           limiterOn;                 /* 0x43B the rev limiter cuts */
    f32          throttlePrev;              /* 0x43C */
    f32          turboThrottle;             /* 0x440 the throttle x the turbo (u078 throttleOut) */
    s16          turboSpool[2];             /* 0x444 per stage, rpm */
    f32          engineSpeed;               /* 0x448 rad/s (u077 / u081 engineW) */
    f32          vel[3];                    /* 0x44C the world velocity */
    f32          axleSpeed[2];              /* 0x458 rad/s per axle */
    u8           centreLocked;              /* 0x460 (gear layouts 4..6) */
    u8           transMode;                 /* 0x461 TRANS_* (below; u079 / u081 pedalMode) */
    u16          shiftHold;                 /* 0x462 */
    u8           gearOffset;                /* 0x464 */
    u8           offsetCooldown;            /* 0x465 */
    u8           offsetCount;               /* 0x466 */
    u8           lapAdjust;                 /* 0x467 1: a grid start past half the course (u082: ? not racing) */
    f32          pitch;                     /* 0x468 */
    f32          roll;                      /* 0x46C */
    f32          yaw;                       /* 0x470 */
    f32          latGripUse;                /* 0x474 accelY x 4 / the four tyres' surfaceGrip x grip (u081) */
                                            /* the lateral acceleration over the mean grip at rest; it fades the
                                               traction control (T357) and the AWD coupling (T344) */
    f32          yawAccel;                  /* 0x478 */
    f32          yawRate;                   /* 0x47C */
    f32          pitchRate;                 /* 0x480 */
    f32          rollRate;                  /* 0x484 */
    f32          speedX;                    /* 0x488 the forward speed, m/s (u080 / u086 speed) */
    f32          speedY;                    /* 0x48C lateral */
    s16          rpm;                       /* 0x490 */
    u8           gate;                      /* 0x492 the next check gate */
    u8           section;                   /* 0x493 the logger section (DynamicsConductor::virtual_7) */
    f32          groundPitch;               /* 0x494 */
    f32          groundRoll;                /* 0x498 */
    f32          steerOver;                 /* 0x49C after car_process_input: steer / the front wheels' velAngle */
                                            /* beyond the lock, 0..1 (the lock follows a front end sliding out:
                                               a countersteer aid; u079) */
    f32          invDt;                     /* 0x4A0 (u082 fps: 60 / timeScale) */
    f32          throttleOut;               /* 0x4A4 the throttle after the traction control */
    f32          steerMin;                  /* 0x4A8 -CarDynamics.steerLock after a reset */
    f32          steerMax;                  /* 0x4AC +steerLock */
    f32          engineTorque;              /* 0x4B0 engine_torque_at(engine, condition, engineSpeed), every frame */
                                            /* (T300); <= 0 turns the traction control off (T357) */
    f32          frictionTorque;            /* 0x4B4 engine_scale: engine.torqueScale x engineSpeed (T300) */
                                            /* engine_output: load x (engineTorque + it) - it (? the friction) */
    f32          drag;                      /* 0x4B8 the air drag of the frame (T329; u081 a longitudinal term) */
    f32          lift[2];                   /* 0x4BC front, rear (T329; u081 vertical terms) */
    f32          accelX;                    /* 0x4C4 */
    f32          accelY;                    /* 0x4C8 (u079 lateralForce) */
    f32          heaveAccel;                /* 0x4CC */
    f32          unk_4D0;                   /* 0x4D0 a longitudinal force term */
    f32          unk_4D4;                   /* 0x4D4 a lateral force term */
    f32          longForce;                 /* 0x4D8 the sum of the longitudinal forces */
    u8           stopped;                   /* 0x4DC non-zero: the car is held (u082 held) */
    u8           groundBits;                /* 0x4DD low 4 bits: the wheels below the ground; high 4: the last */
    s8           pitBox;                    /* 0x4DE pitSlotNode (pitNodeAngle) */
    u8           courseOut;                 /* 0x4DF the course-out counter (0xFF: at once) */
    u8           finished;                  /* 0x4E0 */
    s8           rank;                      /* 0x4E1 1-based (carIndex + 1 at the start; u086 carNo; lb) */
    u16          turboSpoolMax;             /* 0x4E2 the larger of turboSpool[] */
    u8           engineSnd[3];              /* 0x4E4 three engine sound levels, 0..127 (car_sound_engine_levels) */
                                            /* [0], [1] the throttle's, [2] the turbo's (engine_update; 0
                                               without a turbo) */
    u8           blowOffState;              /* 0x4E7 the blow-off (?): 1 after a lift below 0.75 throttle */
                                            /* 0 again above 0.9 (engine_update) */
    f32          blowOffLevel;              /* 0x4E8 the blow-off's level (?): (turbo throttle - 0.8) / 256 */
                                            /* set at the lift, x engine70 a frame (car_sound_engine_levels) */
    f32          turboFactor;               /* 0x4EC turbo_boost_factor at a rolling start */
                                            /* engine_update averages in the turbo throttle + (1 - throttle) x a
                                               factor of the rpm */
    f32          impact;                    /* 0x4F0 the impact of a collision, held 24 frames at its peak */
                                            /* car_box_impact_feedback (T374); shakes the pad (T335) */
    f32          clutchTorqueAbs;           /* 0x4F4 (u080; T288's gear whine, at most whineTorqueMax) */
    f32          lagYawDiff;                /* 0x4F8 yaw - lagYaw (T337) */
    f32          lagPitchDiff;              /* 0x4FC pitch - lagPitch */
    f32          lagYaw;                    /* 0x500 */
    f32          lagYawVel;                 /* 0x504 */
    f32          lagYawAcc;                 /* 0x508 */
    f32          lagPitch;                  /* 0x50C */
    f32          lagPitchVel;               /* 0x510 */
    f32          steerForce;                /* 0x514 the force feedback filter's state (T336) */
    int          steerHold;                 /* 0x518 frames (0..60) */
    u8           msgKind;                   /* 0x51C the message (T331) */
    u8           msgTime;                   /* 0x51D frames left (0xFF: no time-out) */
    u8           fault;                     /* 0x51E FAULT_* (below; T324; u082: no time display events) */
    u8           revving;                   /* 0x51F the start revving: 1 rising, 0 falling */
    u8           brakeAssist;               /* 0x520 1 armed, 2 working */
    u8           tractionAssist;            /* 0x521 1 armed, 2 working */
    u8           fullThrottle;              /* 0x522 the throttle at full: cleared unless the analog throttle is */
                                            /* 0x1000 or the key is held (car_steering_display, T342; u079;
                                               first pass throttleOk) */
    s8           steering;                  /* 0x523 -100..100 */
    f32          timeScale;                 /* 0x524 1: normal; the slow car boost scales it (u083 boost) */
    f32          slipTarget;                /* 0x528 the slipstream strength (T329; u082 slipstream 0..1) */
    f32          slipBlend;                 /* 0x52C 0..1 */
    u32          lapStartTime;              /* 0x530 */
    u8           ctrlMode;                  /* 0x534 who drives the car: CTRL_* (below; u079 / u081 gearMode) */
                                            /* CTRL_DRIVER after DynCarState_setDriver (u083 pitState, u086
                                               mode) */
    u8           flags;                     /* 0x535 DYN_FLAG_* (below) */
    u16          pitTimer;                  /* 0x536 the pit stop's frames left (u083): car_pit_jack (T298) lifts */
                                            /* the car on it, car_integrate_velocity (T296) clears the planar
                                               velocity while it runs (u081) */
                                            /* 300 at the box (T350), 0x78 for a start in the box
                                               (DynCar_initialize); counts down in car_pre_step; while it runs
                                               the car asks for 1st gear (u079 / u082 forcedGear, u081
                                               resetTimer) */
};
EGCS_SIZE(DynCarState, 0x538);

/* DynCarState.driver (u078, u079, u082, u085). */
enum {
    DRIVER_PAD      = 0,                    /* the player's input */
    DRIVER_AUTO     = 1,                    /* (? the computer with the pad's steering display) the ghost car of
                                               the time trial modes, the players' cars in conductor mode 3 */
    DRIVER_COMPUTER = 2                     /* a computer car */
};

/* DynCarState.ctrlMode (u078, u079, u082, u083, u086): set by T341's
 * requests, ai_drive (T366) switches on it. */
enum {
    CTRL_DRIVER     = 0,                    /* the car's driver */
    CTRL_COMPUTER   = 1,                    /* the computer, after the finish */
    CTRL_PIT_IN     = 2,                    /* in the pit lane, towards the box */
    CTRL_PIT_BOX    = 3,                    /* at the box (the stop runs pitTimer frames; a 5 m look-ahead) */
    CTRL_PIT_OUT    = 4,                    /* leaving the pit lane (20 m in the pit lane) */
    CTRL_STOP_ROLL  = 5,                    /* stopping on the line without the brake (ai_stop, T366) */
    CTRL_STOP_BRAKE = 6,                    /* stopping on the line with full brake (CAR_FINISH_PULL_OVER) */
    CTRL_STOP       = 7,                    /* ai_stop straight ahead */
    CTRL_STILL      = 8                     /* still: no clutch, the handbrake on */
};

/* DynCarState.transMode (u079; car_select_gear, T343). */
enum {
    TRANS_AUTO          = 0,
    TRANS_SEMI_AUTO     = 1,
    TRANS_AUTO_SPEED    = 2,                /* automatic by the speed */
    TRANS_MANUAL_CLUTCH = 3,                /* manual: the brake pedal is the clutch */
    TRANS_MANUAL        = 4,
    TRANS_SEQUENTIAL    = 5,                /* the second throttle is not read */
    TRANS_MANUAL_6      = 6                 /* manual: a shift or the second throttle opens the clutch */
};

/* DynCarState.clutchState (u079, u080, u081; car_drive_torque, T344, and
 * car_clutch_state / car_set_gear, T296). */
enum {
    CLUTCH_FREE   = 0,                      /* the engine runs free: no drive (engine_update not called) */
    CLUTCH_LOCKED = 1,                      /* engaged: the engine torque goes through */
    CLUTCH_SLIP   = 2,                      /* slipping (starting off, to / from the reverse): engineTorque / 20 +
                                               the slip factor x the engine torque */
    CLUTCH_SHIFT  = 3                       /* slipping after a forward -> forward shift: GearCalc.clutchTorque */
};

/* DynCarState.fault (u076, u084): the collision rule F_001DA130 and the
 * course-out rule chargePenaltyBySurface (T324). */
enum {
    FAULT_COLLISION  = 0x01,
    FAULT_COURSE_OUT = 0x02
};

/* DynCarState.flags (u081 CARF_*, u082 DYN_FLAG_*, u083 RUN_*, u086
 * ST_FLAG_BIT4). */
enum {
    /* countLap saw the car go backwards this frame (updateCar, T307); T350
     * then watches only the pit exit. */
    DYN_FLAG_BACKWARDS      = 0x01,
    /* A tyre is worn below the pit grip or a wheel needs service
     * (car_tyre_check, T296); cleared at the pit box (T350). */
    DYN_FLAG_TYRE_DUE       = 0x04,
    /* The computer takes the car into the pits at the next gate-0 entry
     * (car_tyre_check, for driver 1 / 2 cars; once); cleared at the pit
     * exit (T350). */
    DYN_FLAG_PIT_CALL       = 0x08,
    /* A wheel on counted ground (RunwayContact.attr2 bit 0) while the
     * conductor's isPitEnabled allows it (DynCar_place, car_ground_contact): no
     * tyre wear or wear grip (T300, T364), no slow car boost (T304), the
     * corner bonus off (T366), meets only cars with the bit (T349),
     * CAR_FINISH_AUTO stops the car (T302); a driver's car enters the pit
     * lane only with it (T350).  (? the pit area's ground.) */
    DYN_FLAG_COUNTED_GROUND = 0x10
};

/* The pad actuators (RaceCarState +0x16D4), sent by RaceInput::sendVibration
 * (T413).  Not polymorphic.  Size 4. */
class PadVibration {
public:
    u8 small;                               /* 0x0 actuator 0 */
    u8 big;                                 /* 0x1 actuator 1, 0..127 (T335) */
    u8 steer;                               /* 0x2 actuator 2: 0x80 - the wheel force (T336) */
    u8 unk_3;                               /* 0x3 */
};
EGCS_SIZE(PadVibration, 4);

/* g_0049CFD8, set by car_sound_params_init (? names: the roles are guesses
 * from the formulas).  The speeds are m/s; rollLo / rollHi are the volume
 * factors over the tyre width 0.135..0.355 m (GearCalc.tyreWidth);
 * whineTorqueMax is a torque (x 9.8), compared with clutchTorqueAbs.  Not
 * polymorphic.  Size 0x78 (g_0049D050 follows). */
class CarSoundParams {
public:
    f32 windGain;                           /* 0x00 */
    f32 windSpeedLo;                        /* 0x04 m/s */
    f32 windSpeedHi;                        /* 0x08 */
    f32 windPitchLo;                        /* 0x0C */
    f32 windPitchHi;                        /* 0x10 */
    f32 windMax;                            /* 0x14 */
    f32 roadGain;                           /* 0x18 */
    f32 roadSpeedLo;                        /* 0x1C */
    f32 roadSpeedHi;                        /* 0x20 */
    f32 roadPitchLo;                        /* 0x24 */
    f32 roadPitchHi;                        /* 0x28 */
    f32 roadMax;                            /* 0x2C */
    f32 rollLo;                             /* 0x30 */
    f32 rollHi;                             /* 0x34 */
    f32 rollGain;                           /* 0x38 */
    f32 rollSpeedLo;                        /* 0x3C */
    f32 rollSpeedHi;                        /* 0x40 */
    f32 rollPitchLo;                        /* 0x44 */
    f32 rollPitchHi;                        /* 0x48 */
    f32 rollMax;                            /* 0x4C */
    f32 whineLo;                            /* 0x50 */
    f32 whineHi;                            /* 0x54 */
    f32 whineTorqueMax;                      /* 0x58 */
    f32 whinePitchSlope;                    /* 0x5C per rad/s */
    f32 whinePitchBase;                     /* 0x60 */
    f32 engine64;                           /* 0x64 */
    f32 engine68;                           /* 0x68 */
    f32 engine6C;                           /* 0x6C */
    f32 engine70;                           /* 0x70 */
    f32 crashGain;                          /* 0x74 (0x0049D04C) */
};
EGCS_SIZE(CarSoundParams, 0x78);

/* (g_0049CFD8, retail .bss 0x0049CFD8: T288's file static, defined in
 * T288_car_sound.cpp; u078's data pass.) */

void car_sound_params_init(TyreSoundTable *t);                          /* 0x001DF4C0 from tyre_sound_init */
/* Volume: gain x the drag share (CarDynamics.unk_E5C) x speedX^2 x (1 -
 * slipBlend), at most windMax.  Pitch (an integer): windPitchLo..Hi over
 * windSpeedLo..Hi. */
f32  car_sound_wind(RaceCarState *car, u16 *pitch);                     /* 0x001DF840 (read back with lhu, u042) */
f32  car_sound_road(RaceCarState *car, f32 *pitch);                     /* 0x001DF910 */
/* Surfaces 0, 1, 7, 8, 9 with a recent load: (|spin| / 100)^2 x rollGain x
 * the tyre width's factor. */
f32  car_sound_tyre_roll(RaceCarState *car, int wheel, f32 *pitch);     /* 0x001DF9D8 */
/* With gearWhineOn and the clutch not free: the torque through the gearbox
 * against 1st gear's; the pitch over the driven axle's speed.  The enhanced
 * engine (Lupo 2, cameras_sound.md S6): a Dualnote's or a gearbox type 3's
 * motor whine instead (RaceCarState::hybrid, GearCalc::gearboxType). */
f32  car_sound_gear_whine(RaceCarState *car, f32 *pitch);               /* 0x001DFB58 */
/* The enhanced engine (Lupo 2): type 3 silent below 100 rpm (S6); the levels
 * around the engine's real load share (S2, ENH_SOUND_ENGINE_LOAD). */
void car_sound_engine_levels(RaceCarState *car);                        /* 0x001DFC60 DynCarState.engineSnd[] */
void car_crash_sound_clear(RaceCarState *car);                          /* 0x001DFD90 crash level 0 (every frame) */
/* The kind of a crash (car_report_crash, car_crash_sound_add; u076, u078,
 * u088 CRASH_*, u057).  arg is the part of the car's box: a corner 0..7 or
 * an edge CRASH_PART_EDGE + j (u088); a loose course object reports 9. */
enum {
    CRASH_KIND_NONE   = -1,                 /* nothing (car_crash_sound_add) */
    CRASH_KIND_WALL   = 0,                  /* a wall (car_box_walls, T376) */
    CRASH_KIND_CAR    = 1,                  /* another car (car_box_cars, T375) */
    CRASH_KIND_GADGET = 2,                  /* a loose course object (Gadget::collideCars, T167) */
    CRASH_PART_EDGE   = 8                   /* arg: the first edge */
};

/* kind -1: nothing; 0: force / 50000, 1: force x 2, 2: |force / 50|, else
 * 0; at most 1, below 0.2 is 0.  Kept (with kind, arg) when louder than the
 * frame's level so far.  force is the second parameter (EABI: the same
 * registers $a0, $f12, $a1, $a2 in any order; car_report_crash 0x001DFEB8
 * sets them in this order, u078). */
void car_crash_sound_add(RaceCarState *car, f32 force, int kind, int arg); /* 0x001DFD98 */
/* Pitch 1, kind / arg of the loudest crash; returns its level x crashGain. */
f32  car_crash_sound_get(RaceCarState *car, f32 *pitch, int *kind, int *arg); /* 0x001DFE70 */
/* The conductor's slot 40 (car, kind, arg, force), then car_crash_sound_add. */
void car_report_crash(RaceCarState *car, int kind, int arg, f32 force); /* 0x001DFEB8 */

#endif /* GAME_T288_CAR_SOUND_H */
