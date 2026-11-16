/* T314_dc_time_trial.h - DynamicsConductorTimeTrial, the conductor of a time
 * trial run (translation unit T314, unit u084), 0x001F1AD8..0x001F2010.
 *
 * One lap (slot 29 = 1): the run ends at the first lap's end (slot 17).  A
 * collision (fault bit 0) or a course out (bit 1) of car 0 disqualifies the
 * run (slot 12 -> disqualify: event 0xA, car_finish(car, 2)). */
#ifndef GAME_T314_DC_TIME_TRIAL_H
#define GAME_T314_DC_TIME_TRIAL_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2880 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf26DynamicsConductorTimeTrial(void);   /* 0x001F1FC0 */
}
#endif

/* vtable 0x0033D548 (42 slots: [4] [8] [12] [13] [15] [16] [17] [20] [21]
 * [23] [24] [26] [28] [29] [34]..[38] [40] its own).  typeinfo
 * __tf26DynamicsConductorTimeTrial 0x001F1FC0 (si, base DynamicsConductor).
 * vptr +0xE3A8.  Size 0xE3AC: no data of its own. */
class DynamicsConductorTimeTrial : public DynamicsConductor {
public:
    DynamicsConductorTimeTrial();           /* 0x001F1AD8 (F_00188088) */
    /* Unless a menu runs over the race or the car has finished: event 0xA
     * for car index 0 (RaceTimeTrialDisplay, T141, takes it and plays the
     * disqualification: renderWhenDisqualifying), then car_finish(car, 2)
     * (licenseFailed, T305 / T352, is the same with event 9).  reason (0 a
     * course out, 1 a collision) is not read.  Caller: slot 12. */
    void disqualify(RaceCarState *car, int reason); /* 0x001F1BB8 */

    virtual int  isDataLoggerSupported();   /* [ 4] +0x02C 0x001F1FB8 1 */
    virtual int  getStartingFormat();       /* [ 8] +0x04C 0x001F1FB0 2 */
    virtual void virtual_12();              /* [12] +0x06C 0x001F1E50 car 0's fault disqualifies the run */
    virtual void virtual_13();              /* [13] +0x074 0x001F1EB0 the base's */
    /* dc_post_section_diff, then the base's section time events. */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001F1C20 */
    /* The start line: a car driven by the pad gets it back (car_mode_driver) and
     * event 4; the lap starts at ms. */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t); /* [16] +0x08C 0x001F1CC0 */
    /* Unless a menu runs over the race: the run ends (car_finish(3)); with a
     * controller setFinishTimer(3) (car 0), event 5 and checkLapRecord (mode
     * 0); the lap difference and lap time events; the lap starts at ms. */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F1D40 */
    virtual void virtual_20(int a);         /* [20] +0x0AC 0x001F1B18 the base's */
    virtual void virtual_21(int a);         /* [21] +0x0B4 0x001F1B38 the base's */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F1B58 getEntrantInfoTrial */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F1F80 slot 25 */
    virtual void virtual_26();              /* [26] +0x0DC 0x001F1ED0 the base, then checkRunResult */
    /* The best lap once finished, else the time since the line. */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001F1B80 */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F1B78 1 */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F1F00 3 unless isDirtCourse */
    virtual int  virtual_35();              /* [35] +0x124 0x001F1F28 */
    virtual int  virtual_36();              /* [36] +0x12C 0x001F1F30 the course-out level on counted ground */
    virtual int  virtual_37();              /* [37] +0x134 0x001F1F38 off the course: 1 unless isDirtCourse */
    virtual int  isPitEnabled();            /* [38] +0x13C 0x001F1F58 */
    virtual void issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force); /* [40] +0x14C 0x001F1F60 F_001DA130 */
};
/* The enhanced engine: DynamicsConductor's (GT3 0xE3AC). */
EGCS_SIZE(DynamicsConductorTimeTrial, 0xE58C);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti26DynamicsConductorTimeTrial[3]; /* 0x004B2880 the __si_type_info (u084 g_004B2880) */
#endif

#endif /* GAME_T314_DC_TIME_TRIAL_H */
