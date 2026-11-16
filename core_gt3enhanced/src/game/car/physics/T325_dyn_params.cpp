/* T325 - g_002D28B0, the dynamics tuning bytes (DynParams, declared in
 * T283_condition_model.h; the global in T325_misc.h).  A translation unit
 * without code: retail's object is its .data (0x002D28B0..0x002D296C, 0xBC
 * bytes) and its frame info (collect2's table entry 0x002D296C: the CIE and
 * the 0 terminator, no FDE), linked between T324 and T281.  No .rodata: the
 * bytes hold no pointer, and the type names between T279's and T282's
 * .rodata are T324's.  Its users: the slipstream (T329), the car parameter
 * builder (T284), the engine's power limiter (T291), the tyre heat (T295),
 * the drivetrain (T296), the wear curve (T327), the surface tables (T328),
 * ConditionModel (T283), the assists (T340, T356). */
// DATA_TU frame 0x002D296C
#include "game/car/physics/T325_misc.h"                 /* DynParams (T283_condition_model.h), g_002D28B0 */
#include "game/enhanced.h"                              /* ENH_PHYSICS_DYNPARAMS */

/* The enhanced engine: ENH_PHYSICS_DYNPARAMS (physics.md 5) gives Lupo 2's
 * values of this table (L2 0x002DEB60, the same 0xBC bytes; only these six
 * values differ):
 *   +0x04 fuelMass 55 -> 5: every car 50 kg lighter (weight + 65 + 5 kg);
 *   +0x07 unk_07 50 -> 100: the drive / brake pitch moment doubled
 *         (CarDynamics.unk_130);
 *   +0x2C rollingResistance[0] 10 -> 30: rolling resistance x3 on surface 0;
 *   +0x8F..0x91 compoundLoad 125 -> 110: the wear grip floor lower (a worn
 *         tyre can lose more grip);
 *   +0xAD oversteerAngleLo 20 -> 7 degrees, +0xB0 oversteerGainHi 140 -> 60:
 *         the stability aid sees oversteer earlier at low speed, acts less at
 *         high speed (car_slip_estimate, T340). */
#if ENH_PHYSICS_DYNPARAMS
#define DYNP(gt3, lupo2) lupo2
#else
#define DYNP(gt3, lupo2) gt3
#endif

DynParams g_002D28B0 = {                    /* 0x002D28B0 */
    12,                                     /* 0x00 airDensity */
    6,                                      /* 0x01 slipDrag */
    12,                                     /* 0x02 slipLift */
    65,                                     /* 0x03 driverMass */
    DYNP(55, 5),                            /* 0x04 fuelMass (Lupo 2: 5) */
    5,                                      /* 0x05 slipRise */
    20,                                     /* 0x06 slipFall */
    DYNP(50, 100),                          /* 0x07 unk_07 (Lupo 2: 100) */
    30,                                     /* 0x08 powerLimitSoftness */
    { 0, 0 },                               /* 0x09 unk_09 */
    8,                                      /* 0x0B speedCount */
    { 0, 2, 10, 29, 58, 100, 154, 200 },    /* 0x0C speed, km/h / 2 */
    { 0, 0, 10, 15, 32, 72, 144, 255 },     /* 0x14 speedValue, x 1000 */
    { 0, 10, 10, 24, 20, 10, 0, 0,          /* 0x1C bumpAmplitude, x 1000 */
      0, 0, 0, 0, 0, 0, 0, 0 },
    { DYNP(10, 30), 10, 20, 2550, 30, 20, 10, 10, /* 0x2C rollingResistance, x 100 (Lupo 2: [0] 30) */
      10, 10, 10, 10, 10, 10, 10, 10 },
    { 5, 5, 25, 50, 50, 5, 10, 10,          /* 0x4C bumpPeriod, 0.1 s */
      10, 10, 10, 10, 10, 10, 10, 10 },
    { 0, 0, 1, 1, 1, 1, 1, 0,               /* 0x5C bumpSpeed */
      0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1,               /* 0x6C bumpKind */
      1, 1, 1, 1, 1, 1, 1, 1 },
    7,                                      /* 0x7C heatScale */
    20,                                     /* 0x7D coolTime */
    100,                                    /* 0x7E coolSpeed, km/h */
    5,                                      /* 0x7F coolBias, % */
    { 0, 0, 0, 0 },                         /* 0x80 unk_80 */
    { 20, 20 },                             /* 0x84 slipFadeSpeed, km/h */
    { 1, 1 },                               /* 0x86 forceMode */
    { 50, 50 },                             /* 0x88 gripMin, % */
    { 100, 100 },                           /* 0x8A gripSpeed, km/h */
    20,                                     /* 0x8C wearLo */
    60,                                     /* 0x8D wearHi */
    100,                                    /* 0x8E wearSlipCap */
    { DYNP(125, 110), DYNP(125, 110), DYNP(125, 110) }, /* 0x8F compoundLoad, % (Lupo 2: 110) */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       /* 0x92 unk_92 */
    0,                                      /* 0x9C conditionReady (ConditionModel::init sets it) */
    { 0, 0, 0 },                            /* 0x9D unk_9D */
    { 30, 30, 50, 10, 20, 20, 50, 10, 30 }, /* 0xA0 cond (ConditionModel) */
    { 0, 0, 0, 0 },                         /* 0xA9 unk_A9 */
    DYNP(20, 7),                            /* 0xAD oversteerAngleLo, degrees (Lupo 2: 7) */
    5,                                      /* 0xAE oversteerAngleHi, degrees */
    30,                                     /* 0xAF oversteerGainLo, x 0.1 */
    DYNP(140, 60),                          /* 0xB0 oversteerGainHi, x 0.1 (Lupo 2: 60) */
    { 0, 0, 0, 0 },                         /* 0xB1 unk_B1 */
    15,                                     /* 0xB5 brakeAssistAngle, degrees */
    100,                                    /* 0xB6 brakeAssistFront, % */
    0,                                      /* 0xB7 brakeAssistRear, % */
    { 0, 0, 0, 0 }                          /* 0xB8 unk_B8 (not read) */
};
