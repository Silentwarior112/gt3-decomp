/* T312_dc_license.h - DynamicsConductorLicense, the conductor of the license
 * tests (translation unit T312, unit u084), 0x001F0AD8..0x001F1190.
 *
 * rules.launchSpeed (non-zero: a rolling start, the clock starts at the
 * line), rules.licenseType 2 (the test ends at the first finish) or 3
 * (multi-lap: a lap end posts as a section time, the last lap ends the
 * test).  A lap of k checkpoints counts as k sections in a row (slots 7, 15,
 * 17).  A course out or a collision fails the test (slots 13, 36..40). */
#ifndef GAME_T312_DC_LICENSE_H
#define GAME_T312_DC_LICENSE_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2860 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf24DynamicsConductorLicense(void);     /* 0x001F1140 */
}
#endif

/* vtable 0x0033D1F0 (42 slots: [4] [5] [6] [7] [12] [13] [15] [16] [17] [18]
 * [22] [23] [24] [26] [28] [29] [34]..[38] [40] its own).  typeinfo
 * __tf24DynamicsConductorLicense 0x001F1140 (si, base DynamicsConductor).
 * vptr +0xE3A8.  Size 0xE3AC: no data of its own. */
class DynamicsConductorLicense : public DynamicsConductor {
public:
    DynamicsConductorLicense();             /* 0x001F0AD8 (F_00189CA0) */

    virtual int  isDataLoggerSupported();   /* [ 4] +0x02C 0x001F1138 1 */
    virtual int  isDataLoggerStandingStart(); /* [ 5] +0x034 0x001F0BB8 rules.launchSpeed == 0 */
    virtual int  isDataLoggerMultiLap();    /* [ 6] +0x03C 0x001F0BD0 a licenseType 3 test of more than one lap */
    /* The base's section unless multi-lap; else the sections of the laps run
     * plus the next one, less one with flag. */
    virtual int  virtual_7(RaceCarState *car, int flag); /* [ 7] +0x044 0x001F0C00 */
    virtual void virtual_12();              /* [12] +0x06C 0x001F1030 empty */
    virtual void virtual_13();              /* [13] +0x074 0x001F1038 licenseCheckStop of every car, then the base */
    /* Posted as section (lap - 1) x checkpoints + gate of lap 1. */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001F0D90 */
    /* The start line of a rolling start: the lap starts at ms, the driver
     * gets the car back (car_mode_driver), for car 0 event 4. */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t); /* [16] +0x08C 0x001F0E40 */
    /* licenseType 3: the last lap ends the test (licenseFinished with
     * split); any other lap end posts as section lap x checkpoints - 1. */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F0ED8 */
    /* licenseType 2: the test ends (licenseFinished with split), 1; else 0. */
    virtual int  virtual_18(int car, int lap, u32 raw, u32 ms, u32 split); /* [18] +0x09C 0x001F0FC8 */
    /* The base, then the race result reset (time 0x157529FF, rank -2). */
    virtual void initialize(int mode);      /* [22] +0x0BC 0x001F0B18 */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F1118 getEntrantInfoTrial */
    /* Unless rollingStart applies: startAtLaunchPoint, then startWithLaunchSpeed. */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F0D00 */
    virtual void virtual_26();              /* [26] +0x0DC 0x001F0B58 the base, then checkRunResult */
    /* The best lap once finished; else the running lap time (standing start)
     * or the time since the line (lapTimeIfStarted). */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001F0C90 */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F0B88 the base's (rules.laps) */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F10B8 */
    virtual int  virtual_35();              /* [35] +0x124 0x001F10C0 */
    virtual int  virtual_36();              /* [36] +0x12C 0x001F10C8 the course-out level on counted ground */
    virtual int  virtual_37();              /* [37] +0x134 0x001F10D0 off the course: 1 unless isDirtCourse */
    virtual int  isPitEnabled();            /* [38] +0x13C 0x001F10F0 */
    virtual void issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force); /* [40] +0x14C 0x001F10F8 F_001DA130 */
};
/* The enhanced engine: DynamicsConductor's (GT3 0xE3AC). */
EGCS_SIZE(DynamicsConductorLicense, 0xE58C);

/* A rolling start: r->launchSpeed != 0 (getStartingFormat and slot 5 test
 * the opposite, a standing start).  Callers: slots 16 and 28. */
int rules_rolling_start(RaceRuleOptions *r);                  /* 0x001F0BA8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti24DynamicsConductorLicense[3]; /* 0x004B2860 the __si_type_info (u084 g_004B2860) */
#endif

#endif /* GAME_T312_DC_LICENSE_H */
