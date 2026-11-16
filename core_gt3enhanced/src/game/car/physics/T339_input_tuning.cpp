/* T339 - g_002D4628, the input tuning bytes (InputTuning, T339_misc.h).  A
 * translation unit without code: retail's object is its .data
 * (0x002D4628..0x002D4670, 0x48 bytes) and its frame info (collect2's table
 * entry 0x002D4670: the CIE and the 0 terminator, no FDE), linked between
 * T357 and T358.  No .rodata: the bytes hold no pointer, and the
 * "bad_alloc" / UnitArena<64> pairs between T294's and T342's .rodata are
 * T356's, T358's and T341's (the files whose code uses the conductor or the
 * computer driver's functions).  Its users: input_tuning_init (T342: the
 * pedal ramps, the steering speeds and springs, the wheel's locks and
 * curves), steer_force_init / steer_force_update (T336, u078: +0x34..+0x3E),
 * the gearbox (T343, u080: +0x14..+0x17). */
// DATA_TU frame 0x002D4670
#include "game/car/physics/T339_misc.h"                 /* InputTuning, g_002D4628 */

InputTuning g_002D4628 = {                  /* 0x002D4628 */
    { 10, 10, 10, 10, 1, 1 },               /* 0x00 rampTime, 0.01 s */
    10,                                     /* 0x06 analogPedalMin, % */
    { 0, 1, 80, 13, 0 },                    /* 0x07 unk_07 */
    100,                                    /* 0x0C steerSpring */
    50,                                     /* 0x0D steerDamping, x 0.1 */
    50,                                     /* 0x0E centreSpring */
    3,                                      /* 0x0F steerSpeed, x 10 deg/s */
    12,                                     /* 0x10 steerSpeedBack */
    12,                                     /* 0x11 steerSpeedClass1 */
    12,                                     /* 0x12 analogSteerSpeed */
    0,                                      /* 0x13 unk_13 */
    1,                                      /* 0x14 manualMode */
    5,                                      /* 0x15 accelDown, x 100 rpm */
    0,                                      /* 0x16 liftDown */
    0,                                      /* 0x17 brakeDown */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 },          /* 0x18 unk_18 */
    1,                                      /* 0x21 unk_21 */
    50,                                     /* 0x22 unk_22 */
    9,                                      /* 0x23 wheelLockSpeed, x 10 deg/s */
    { 8, 15 },                              /* 0x24 minLock1, deg */
    { 12, 20 },                             /* 0x26 minLock2, deg */
    { 0, 0, 0, 0 },                         /* 0x28 unk_28 */
    1,                                      /* 0x2C throttleCurveMode */
    50,                                     /* 0x2D throttleCurveN */
    { 0, 0 },                               /* 0x2E unk_2E */
    1,                                      /* 0x30 brakeCurveMode */
    50,                                     /* 0x31 brakeCurveN */
    { 0, 0 },                               /* 0x32 unk_32 */
    200,                                    /* 0x34 forceGain, x 0.01 */
    0,                                      /* 0x35 forceCurveMode (linear) */
    100,                                    /* 0x36 forceCurveN */
    1,                                      /* 0x37 useX420 */
    200,                                    /* 0x38 lateralGain, x 0.1 */
    1,                                      /* 0x39 filter (the average) */
    10,                                     /* 0x3A rateGain */
    0,                                      /* 0x3B steerAngleGain */
    50,                                     /* 0x3C steerAngleLimit, 0.1 deg */
    0,                                      /* 0x3D slipGain */
    40,                                     /* 0x3E slipLimit, 0.1 deg */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }           /* 0x3F unk_3F */
};
