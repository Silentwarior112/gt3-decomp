/* T291_engine.h - CarEngine, the engine of the car's parameter block, and the
 * engine's spec sheet (translation unit T291, unit u078), 0x001E0BC0..0x001E1768.
 *
 * engine_setup builds the CarEngine (CarDynamics +0x138, RaceCarState
 * +0x148) from the engine bytes of the car's CarSpec (u078 EngineSpec:
 * +0x6C..+0xAF): the torque curve (rpm x 100 in rad/s, torque x 9.8 x a gain
 * and a factor interpolated over the rpm range), two turbo stages
 * (turbo_setup) and a power limit.  The torque at a speed is the curve x the
 * condition's power factor (engine_torque_at, ConditionModel::powerFactor),
 * x the turbo factor 1 + the boosts (capped at 1 + turboMax), then limited
 * (engine_power_limit).  engine_spec_sheet fills the power / torque table of
 * the tuning screens and of ValuateAutomobilePerformance (T279). */
#ifndef GAME_T291_ENGINE_H
#define GAME_T291_ENGINE_H

#include "game/game.h"
#include "game/car/physics/T284_car_param.h"            /* Curve (held by value) */

class CarCondition;                         /* T280_car_condition.h (pointers only) */
struct CarSpec;                             /* T482_car_equipments.h */

/* The engine (u077 EngineParam, u079 / u081 EngineCurve: the same 0xC0
 * bytes at CarDynamics +0x138).  Not polymorphic.  Size 0xC0 (GearCalc
 * follows at CarDynamics +0x1F8). */
class CarEngine {
public:
    Curve torqueCurve;                      /* 0x00 over rpm[] / torque[] */
    f32   rpm[16];                          /* 0x0C rad/s */
    f32   torque[16];                       /* 0x4C */
    u16   redline;                          /* 0x8C rpm (u076 / u080 / u086 CarDynamics.revLimit; u079 rpmMax) */
    u16   idle;                             /* 0x8E rpm, spec +0x6E x 10 (u077 idleRpm, u079 rpmMin; lhu); also the */
                                            /* clutch's stall limit (car_clutch_from_input, car_input_ai, u079) */
    f32   torqueScale;                      /* 0x90 */
    s16   turboRpm[2];                      /* 0x94 per stage: the rpm of full boost */
    u8    turboType[2];                     /* 0x98 per stage: 0, 1, 2 (the boost curve) */
    u8    unk_9A;                           /* 0x9A */
    u8    limiter;                          /* 0x9B the power limit is on */
    f32   limitPower;                       /* 0x9C */
    f32   limitSoftness;                    /* 0xA0 g_002D28B0.powerLimitSoftness / 100 */
    f32   turboBoost[2];                    /* 0xA4 per stage (u077: [1] > 0, a second stage) */
    f32   turboRate[2];                     /* 0xAC per stage: the spool rate */
    f32   turboK[2];                        /* 0xB4 per stage: boost / turboRpm^2 */
    f32   turboMax;                         /* 0xBC the total boost cap (0: no turbo) */
};
EGCS_SIZE(CarEngine, 0xC0);

/* The engine's spec sheet (u076 EngineOutput: power, torque, powerRpm,
 * torqueRpm).  Not polymorphic.  Size 0xB0 (ValuateAutomobilePerformance's
 * stack: +0x12A0, then the CarCondition at +0x1350). */
class EngineSpecSheet {
public:
    f32 maxPower;                           /* 0x00 PS */
    f32 maxTorque;                          /* 0x04 kgm */
    s16 maxPowerRpm;                        /* 0x08 */
    s16 maxTorqueRpm;                       /* 0x0A */
    u16 redline;                            /* 0x0C */
    s16 count;                              /* 0x0E points of rpm[] */
    s16 rpm[16];                            /* 0x10 the curve's rpm points, rounded to 100 */
    f32 power[16];                          /* 0x30 PS at rpm[i] */
    f32 torque[16];                         /* 0x70 kgm at rpm[i] */
};
EGCS_SIZE(EngineSpecSheet, 0xB0);

/* The torque at `speed' (rad/s) x ConditionModel::powerFactor of the
 * condition (a fresh CarCondition when NULL). */
f32  engine_torque_at(CarEngine *e, CarCondition *cond, f32 speed);         /* 0x001E0BC0 */
/* torqueScale x x; car_pre_step passes the condition too, as for
 * engine_torque_at (not read). */
f32  engine_scale(CarEngine *e, CarCondition *cond, f32 x);                 /* 0x001E0C28 */
/* With the limiter on: torque x speed above limitPower x (1 - softness) is
 * cut smoothly to at most limitPower (never above the input torque). */
f32  engine_power_limit(CarEngine *e, f32 torque, f32 speed);               /* 0x001E0C38 */
f32  turbo_boost_stage(CarEngine *e, s16 rpm, int stage);                   /* 0x001E0CD8 */
/* 1 + the boosts of the stages that have one (each also to boost[i]),
 * stage i at rpm i. */
f32  turbo_boost_sum(CarEngine *e, s16 rpm0, s16 rpm1, f32 *boost);         /* 0x001E0E08 */
f32  turbo_boost_factor(CarEngine *e, s16 rpm0, s16 rpm1, f32 *boost);      /* 0x001E0ED8 at most 1 + turboMax */
/* The third argument (engine_setup's arg3) is not read. */
void turbo_setup(const CarSpec *spec, CarEngine *e, int unused);           /* 0x001E0F38 */
/* powerLimit > 0 (ps) turns the limiter on.  arg3: CarDynamics_init passes
 * its log argument (a pointer, not used). */
void engine_setup(CarEngine *e, CarSpec *spec, int powerLimit, int arg3);   /* 0x001E10E8 */
void engine_spec_sheet(CarEngine *e, EngineSpecSheet *out, CarCondition *cond); /* 0x001E14E8 */

#endif /* GAME_T291_ENGINE_H */
