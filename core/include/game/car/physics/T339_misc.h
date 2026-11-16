/* T339_misc.h - InputTuning g_002D4628, the input tuning bytes (translation
 * unit T339, no code: its .data only, 0x002D4628..0x002D4670, then its
 * .eh_frame).
 *
 * input_tuning_init (T342) turns +0x00..+0x31 into the pedal ramps, the
 * steering speeds and spring, the wheel controller's lock limits and its
 * throttle / brake curves; steer_force_init (T336) turns +0x34..+0x3E into
 * the steering force's gains and curve; the gearbox (T343) reads the shift
 * parameters +0x14..+0x17 directly.  All bytes, read with lbu.
 *
 * The units' views: u078 SteerForceSource (+0x34..+0x3E), u080 ShiftParams
 * (+0x14..+0x17), both typedefs here; u079 indexes it as u8[] ("the input
 * tuning bytes": c[0x0F] is steerSpeed, ...). */
#ifndef GAME_T339_MISC_H
#define GAME_T339_MISC_H

#include "game/game.h"

/* The comments give the file's values.  Not polymorphic.  Size 0x48: T339's
 * .data up to its .eh_frame (at least 0x45 bytes are data, as the .eh_frame
 * is 4-aligned); +0x3F.. is 0 and unread. */
class InputTuning {
public:
    u8 rampTime[6];                         /* 0x00 {10, 10, 10, 10, 1, 1} 0.01 s: throttle up / down, brake up / down,
                                               handbrake up / down (g_002D4930[i] = 100 / (x 60) per frame, <= 1) */
    u8 analogPedalMin;                      /* 0x06 10 %: x 0.01 = g_00352990; a pressed analog pedal x gives
                                               x + it (1 - x) (u079: the dead zone of the analog pedals) */
    u8 unk_07[5];                           /* 0x07 {0, 1, 80, 13, 0} unread */
    u8 steerSpring;                         /* 0x0C 100: g_00352970, the digital steering's pull to its target */
    u8 steerDamping;                        /* 0x0D 50: x 0.1 = g_00352974 */
    u8 centreSpring;                        /* 0x0E 50: g_00352978, the pull to the centre when steering back */
    u8 steerSpeed;                          /* 0x0F 3: x 10 deg/s = g_0049D080[0] / [2], the digital steering speed */
    u8 steerSpeedBack;                      /* 0x10 12: x 10 deg/s = g_0049D090[0] / [2], the speed back to the centre */
    u8 steerSpeedClass1;                    /* 0x11 12: x 10 deg/s = g_0049D080[1] and g_0049D090[1] (assist class 1) */
    u8 analogSteerSpeed;                    /* 0x12 12: x 10 deg/s = g_0049D0A0[0..2], the analog pads' */
    u8 unk_13;                              /* 0x13 0 */
    u8 manualMode;                          /* 0x14 1: a shift button blocks the automatic; 2: the buttons
                                               shift, then the automatic waits 240 frames (T343) */
    u8 accelDown;                           /* 0x15 5: x 100 rpm off the downshift point while accel > 0.25 */
    u8 liftDown;                            /* 0x16 0: x 100 rpm onto it while accel < 0.05 */
    u8 brakeDown;                           /* 0x17 0: x 100 rpm onto it while brake > 0.8 */
    u8 unk_18[9];                           /* 0x18 0 */
    u8 unk_21;                              /* 0x21 1 (? with unk_22 50 the mode / n of a curve, as +0x2C; unread) */
    u8 unk_22;                              /* 0x22 50 */
    u8 wheelLockSpeed;                      /* 0x23 9: x 10 deg/s = g_0035297C, how fast the wheel's lock follows */
    u8 minLock1[2];                         /* 0x24 {8, 15} deg: the wheel's minimum lock with steering assist 1 or 3,
                                               [1] in assist class 1 (g_00352980 / g_00352984) */
    u8 minLock2[2];                         /* 0x26 {12, 20} (? assist 2's: unread; g_00352988 / g_0035298C hold
                                               12 / 20 deg in .sdata) */
    u8 unk_28[4];                           /* 0x28 0 */
    u8 throttleCurveMode;                   /* 0x2C 1: the wheel controller's throttle curve g_0049D060 (responseInit mode) */
    u8 throttleCurveN;                      /* 0x2D 50: its shape n */
    u8 unk_2E[2];                           /* 0x2E 0 */
    u8 brakeCurveMode;                      /* 0x30 1: the brake curve g_0049D070 */
    u8 brakeCurveN;                         /* 0x31 50 */
    u8 unk_32[2];                           /* 0x32 0 */
    u8 forceGain;                           /* 0x34 200: x 0.01 = g_00352938, the steering force's (u078 gain) */
    u8 forceCurveMode;                      /* 0x35 0 (linear): its curve g_0049D050 (u078 curveA) */
    u8 forceCurveN;                         /* 0x36 100 (u078 curveB) */
    u8 useX420;                             /* 0x37 1: x DynCarState.unk_420 */
    u8 lateralGain;                         /* 0x38 200: x 0.1 = g_00352934, the front tyres' lateral term */
    u8 filter;                              /* 0x39 1: 0 none, 1 the average, 2 the smaller, 3 a peak hold */
    u8 rateGain;                            /* 0x3A 10: x 0.01 / 4096 = g_0035293C, the steering rate's */
    u8 steerAngleGain;                      /* 0x3B 0: x 0.01 / steerAngleLimit = g_00352940 (u078 yawGain) */
    u8 steerAngleLimit;                     /* 0x3C 50: 0.1 deg = g_00352944 (DynCarState.steer's clamp) */
    u8 slipGain;                            /* 0x3D 0: x 0.01 / slipLimit = g_00352948, the front slip angles' */
    u8 slipLimit;                           /* 0x3E 40: 0.1 deg = g_0035294C */
    u8 unk_3F[9];                           /* 0x3F 0 */
};
EGCS_SIZE(InputTuning, 0x48);

typedef InputTuning SteerForceSource;       /* u078 */
typedef InputTuning ShiftParams;            /* u080 */

extern InputTuning g_002D4628;              /* 0x002D4628 */

#endif /* GAME_T339_MISC_H */
