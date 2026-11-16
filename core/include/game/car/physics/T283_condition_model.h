/* T283_condition_model.h - ConditionModel g_002D2B00, the CarCondition
 * thresholds and the power factor of a car's condition (translation unit
 * T283, unit u076), 0x001DAB28..0x001DAE70; DynParams, the dynamics tuning
 * bytes g_002D28B0.
 *
 * ConditionModel is built once from g_002D28B0.cond[] (every getter calls
 * init(), which runs once: conditionReady):
 *   engine: new at engineNew (power x (1 - runInLoss)), run in at 0, worn
 *           from wearStart to engineMax (down to 1 - wearLoss);
 *   oil:    new at -oilLife, + oilBonus up to oilBonusEnd, fading to + 0 at
 *           0; needs a change from oilChangeAt.
 * u078 / u082 call the class ConditionTable.
 *
 * DynParams is the .data of T325 (0x002D28B0..0x002D296C, a translation unit
 * without code); g_002D28B0 itself is declared in T325_misc.h. */
#ifndef GAME_T283_CONDITION_MODEL_H
#define GAME_T283_CONDITION_MODEL_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class CarCondition;                         /* T280_car_condition.h */

/* The dynamics tuning bytes (u077 DynConstTable; u079 / u081 index them as
 * u8[]).  The comments give the file's values.  Not polymorphic.  Size 0xBC:
 * T325's .data runs to its .eh_frame at 0x002D296C, and the last 4 bytes
 * (0 in the file) are read by no code (as SurfaceTable / ConditionModel /
 * InputTuning: up to the .eh_frame). */
class DynParams {
public:
    u8  airDensity;                         /* 0x00 12: the air density x 10 (g_00352920 = / 10, T329; CarDynamics_init) */
    u8  slipDrag;                           /* 0x01 6: / airDensity = the drag factor in a full slipstream */
    u8  slipLift;                           /* 0x02 12: / airDensity = the lift factor */
    u8  driverMass;                         /* 0x03 65 kg added to the mass (? the driver) */
    u8  fuelMass;                           /* 0x04 55 kg added to the mass (? the fuel) */
    u8  slipRise;                           /* 0x05 5: 10 / it = the blend's rise rate */
    u8  slipFall;                           /* 0x06 20: 10 / it = the blend's fall rate */
    u8  unk_07;                             /* 0x07 50 % (CarDynamics.unk_130) */
    u8  powerLimitSoftness;                 /* 0x08 30 % (u078 g_002D28B8; CarEngine.limitSoftness) */
    u8  unk_09[2];                          /* 0x09 */
    u8  speedCount;                         /* 0x0B 8: points of the rolling resistance curve g_002D2CE0 */
    u8  speed[8];                           /* 0x0C km/h / 2 */
    u8  speedValue[8];                      /* 0x14 x 1000 */
    u8  bumpAmplitude[16];                  /* 0x1C per surface, x 1000 (SurfaceTable) */
    s16 rollingResistance[16];              /* 0x2C per surface, x 100 */
    u8  bumpPeriod[16];                     /* 0x4C per surface, 0.1 s (SurfaceTable.bumpWaveNumber) */
    u8  bumpSpeed[16];                      /* 0x5C per surface */
    u8  bumpKind[16];                       /* 0x6C per surface */
    u8  heatScale;                          /* 0x7C 7: the brake heat's gain = 1 / (x 9800) (T295) */
    u8  coolTime;                           /* 0x7D 20: its cooling rate = 1 / (x 0.1 x 100) */
    u8  coolSpeed;                          /* 0x7E 100 km/h: the cooling air at rest */
    u8  coolBias;                           /* 0x7F 5 %: cools even a cold wheel */
    u8  unk_80[4];                          /* 0x80 */
    u8  slipFadeSpeed[2];                   /* 0x84 20 km/h: the slip fade of skidding / pulling wheels (T296) */
    u8  forceMode[2];                       /* 0x86 1: the brake / drive term (2: with the grip curve, T296) */
    u8  gripMin[2];                         /* 0x88 50 %: the grip curve's minimum (T296) */
    u8  gripSpeed[2];                       /* 0x8A 100 km/h: the grip curve's speed (T296) */
    u8  wearLo;                             /* 0x8C 20 (x 98: CarDynamics.wearBase / wearLoad) */
    u8  wearHi;                             /* 0x8D 60 (x 98) */
    u8  wearSlipCap;                        /* 0x8E 100: / 100 = WearCurve.slipCap (T327) */
    u8  compoundLoad[3];                    /* 0x8F 125 % per compound (TyreData_initLoadCurve's scale) */
    u8  unk_92[0xA];                        /* 0x92 */
    u8  conditionReady;                     /* 0x9C set once ConditionModel::init ran */
    u8  unk_9D[3];                          /* 0x9D */
    u8  cond[9];                            /* 0xA0 {30, 30, 50, 10, 20, 20, 50, 10, 30}: ConditionModel's */
    u8  unk_A9[4];                          /* 0xA9 */
    u8  oversteerAngleLo;                   /* 0xAD 20 degrees at speed 0 (T340: where the oversteer measure starts) */
    u8  oversteerAngleHi;                   /* 0xAE 5 degrees at 200 km/h and above */
    u8  oversteerGainLo;                    /* 0xAF 30: x 0.1, at speed 0 */
    u8  oversteerGainHi;                    /* 0xB0 140: x 0.1, at full speed */
    u8  unk_B1[4];                          /* 0xB1 */
    u8  brakeAssistAngle;                   /* 0xB5 15 degrees (T356: the steering of full brake balancing) */
    u8  brakeAssistFront;                   /* 0xB6 100 % */
    u8  brakeAssistRear;                    /* 0xB7 0 % */
    u8  unk_B8[4];                          /* 0xB8 0 (not read) */
};
EGCS_SIZE(DynParams, 0xBC);

/* Not polymorphic.  Size 0x24 (g_002D2B00..T283's .eh_frame at 0x002D2B24). */
class ConditionModel {
public:
    ConditionModel();                       /* 0x001DAB28 init() */
    void init();                            /* 0x001DAB50 once (g_002D28B0.conditionReady) */
    f32  newEngineMileage();                /* 0x001DAC48 engineNew */
    f32  maxEngineMileage();                /* 0x001DAC70 engineMax */
    f32  newOilMileage();                   /* 0x001DAC98 -oilLife */
    f32  maxOilMileage();                   /* 0x001DACC8 oilLife */
    f32  oilChangeMileage();                /* 0x001DACF0 oilChangeAt */
    f32  distance(RaceCarState *car);       /* 0x001DAD18 the car's speed x dt: this frame's distance */
    f32  powerFactor(CarCondition *c);      /* 0x001DAD28 the engine factor x the oil factor */

    f32 engineNew;                          /* 0x00 -cond[0] x 10000 */
    f32 runInLoss;                          /* 0x04 cond[1] / 1000: the power a new engine lacks */
    f32 wearStart;                          /* 0x08 cond[2] x 10000 */
    f32 engineMax;                          /* 0x0C wearStart + cond[3] x 10000 */
    f32 wearLoss;                           /* 0x10 cond[4] / 1000: the power lost at engineMax */
    f32 oilBonusEnd;                        /* 0x14 -cond[7] x 10000 */
    f32 oilBonus;                           /* 0x18 cond[6] / 1000: the power new oil adds */
    f32 oilLife;                            /* 0x1C (cond[5] + cond[7]) x 10000 */
    f32 oilChangeAt;                        /* 0x20 cond[8] x 10000 */
};
EGCS_SIZE(ConditionModel, 0x24);

extern ConditionModel g_002D2B00;           /* 0x002D2B00 the model (u078 / u082: ConditionTable) */

/* Its static constructor 0x001DAE50 (.ctors entry 0x0028F164) is egcs's
 * _GLOBAL_$I$__14ConditionModel: g_002D2B00 is defined in T283's source
 * (T283_static_init is no longer declared). */

#endif /* GAME_T283_CONDITION_MODEL_H */
