/* T356_brake_assist.h - CarWheelState and TcsParams (translation unit T356,
 * unit u079), 0x001E2CC8..0x001E32D8.
 *
 * T356 is the brake assist of the car dynamics: off the brake pedal it brakes
 * the inner wheels against an under- / oversteer slip, on the pedal it
 * releases a side's brakes as the slip grows (ABS-like).  Its functions are
 * used only inside u079 (wheel_add_brake, tcs_brake_or_abs, tcs_brakes,
 * car_brake_assist); the plan puts here the two records it works on:
 *
 * CarWheelState: one wheel of the dynamic state (CarDynState +0x60, 4 of
 * them: FL FR RL RR), u079 / u080 / u081 / u086's name for DynWheel (T335,
 * the same 0xA4 bytes).  It is a typedef here, with no second definition:
 * other headers must not forward-declare `class CarWheelState'.
 *
 * TcsParams: the driving aid block, 0x34 bytes: CarDynamics.tcs (+0xE7C,
 * RaceCarState +0xE8C; u077 AidParam) and the computer driver's copy
 * (AIDriver.params.assist, +0x30, T361 / T362; u079 Transmission tcs2).
 * The code also reads
 * the four bytes at 0x0C and at 0x1C as one word (u077 T284, u086 T362:
 * low half brake, high half ABS) and brakeOn / absOn as one halfword
 * (tcs_brake_or_abs).  egcs takes no anonymous struct in a union, so they
 * are plain bytes here: such reads go through a cast. */
#ifndef GAME_T356_BRAKE_ASSIST_H
#define GAME_T356_BRAKE_ASSIST_H

#include "game/game.h"
#include "game/car/physics/T335_pad_vibration.h"        /* DynWheel (CarWheelState) */

typedef DynWheel CarWheelState;            /* u079, u080, u081, u086 */
EGCS_SIZE(CarWheelState, 0xA4);

/* The driving aids (? TCS / ASM): a brake assist against over- / understeer,
 * an ABS-like release, a throttle cut.  Built by CarDynamics_init (T284)
 * from the spec bytes; u086 reads the same block from the AI parameter record
 * rec[] (the rec[n] notes below).  Not polymorphic.  Size 0x34 (u086's
 * AIParams +0x24 .. +0x58). */
class TcsParams {
public:
    f32 brakeGain;                          /* 0x00 rec[9] / 100 */
    f32 brakeSlip;                          /* 0x04 rec[10] / 100 */
    f32 absGain;                            /* 0x08 rec[20] / 100 */
    u8  brakeFront;                         /* 0x0C rec[19] bit 0: the brake assist works on the front wheel */
    u8  brakeRear;                          /* 0x0D bit 1: on the rear wheel */
    u8  absFront;                           /* 0x0E bit 2 */
    u8  absRear;                            /* 0x0F bit 3 */
    f32 brakeGain2;                         /* 0x10 rec[21] / 100 */
    f32 brakeSlip2;                         /* 0x14 rec[22] / 100 */
    f32 absGain2;                           /* 0x18 rec[23] / 100 */
    u8  brakeFront2;                        /* 0x1C rec[19] bit 4 */
    u8  brakeRear2;                         /* 0x1D bit 5 */
    u8  absFront2;                          /* 0x1E bit 6 */
    u8  absRear2;                           /* 0x1F bit 7 */
    f32 throttleGain;                       /* 0x20 rec[13] / 10; 0: no throttle cut */
    f32 fadeGain;                           /* 0x24 rec[11] / 10 */
    f32 fadeStart;                          /* 0x28 rec[12] / 100 */
    f32 slipTarget;                         /* 0x2C rec[24] / 100 (0 or 100: 1) */
    u8  brakeOn;                            /* 0x30 brakeFront | brakeRear, and brakeGain != 0 */
    u8  absOn;                              /* 0x31 absFront | absRear, and absGain != 0 */
    u8  brakeOn2;                           /* 0x32 */
    u8  absOn2;                             /* 0x33 */
};
EGCS_SIZE(TcsParams, 0x34);

#endif /* GAME_T356_BRAKE_ASSIST_H */
