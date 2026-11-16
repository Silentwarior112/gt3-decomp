/* T311_dc_machine_test.h - DynamicsConductorMachineTest, the conductor of the
 * machine test (translation unit T311, unit u084), 0x001F05F8..0x001F0AD8.
 *
 * The car's acceleration / top speed runs.  rules.laps picks the test
 * (testKind): 1 one line at 400 m, 2 two lines (400 m, 1000 m), 0 / 3 three
 * (400 m, 1000 m, 6845 m; g_002D75F0).  Kinds 1 / 2 post line times, kinds
 * 0 / 3 the top speed.  There are no laps (slot 29 = 0) and the lap end is
 * ignored. */
#ifndef GAME_T311_DC_MACHINE_TEST_H
#define GAME_T311_DC_MACHINE_TEST_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */

class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2850 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf28DynamicsConductorMachineTest(void); /* 0x001F0A88 */
}
#endif

/* vtable 0x0033D050 (42 slots: [4] [5] [15] [17] [18] [23] [26] [27] [28]
 * [29] [30] [32] [33] its own).  typeinfo __tf28DynamicsConductorMachineTest
 * 0x001F0A88 (si, base DynamicsConductor).  vptr +0xE3A8.  Size 0xE3AC: no
 * data of its own (the constructor stores only the vtable). */
class DynamicsConductorMachineTest : public DynamicsConductor {
public:
    DynamicsConductorMachineTest();         /* 0x001F05F8 (F_0018CAC0) */
    /* The base's lap count (rules.laps, a direct call of
     * DynamicsConductor::virtual_29): 0..3, 0 when >= 4. */
    int testKind();                         /* 0x001F0678 */

    virtual int  isDataLoggerSupported();   /* [ 4] +0x02C 0x001F0A80 1 */
    virtual int  isDataLoggerStandingStart(); /* [ 5] +0x034 0x001F0A78 1 */
    /* A checkpoint: its section time events (the lap not decremented),
     * unless a menu runs over the race. */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001F07E0 */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F0880 empty */
    /* The finish: the top speed kept, the car's run ended (car_finish(3)),
     * the record of the line tests checked, the line time (kinds 1, 2) or
     * the top speed (dc_post_speed, kinds 0, 3) posted, for car 0 event 5.
     * 0 while a menu runs over the race, else 1. */
    virtual int  virtual_18(int car, int lap, u32 raw, u32 ms, u32 split); /* [18] +0x09C 0x001F0888 */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F0A58 getEntrantInfoTrial */
    virtual void virtual_26();              /* [26] +0x0DC 0x001F0A08 the base, then checkRunResult (kinds 1, 2) */
    virtual f32  getLapDistance(RaceCarState *car); /* [27] +0x0E4 0x001F06A0 noseDistance(car) */
    /* The car's best lap once it has finished, else the running lap time. */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001F0640 */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F0638 0: no laps */
    virtual f32  virtual_30();              /* [30] +0x0FC 0x001F0730 the last line of the test (slot 32) */
    virtual int  getGateCount();              /* [32] +0x10C 0x001F06C0 the lines: kind 1 -> 1, 2 -> 2, 0 / 3 -> 3 */
    virtual f32  getGatePosition(int gate);      /* [33] +0x114 0x001F0780 the lines before the finish, else -1 */
};
EGCS_SIZE(DynamicsConductorMachineTest, 0xE3AC);

/* The lines of the test, m: {400, 1000, 6845, -1}.  Defined in T311
 * (retail's .data: not const). */
extern f32 g_002D75F0[4];                   /* 0x002D75F0 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti28DynamicsConductorMachineTest[3]; /* 0x004B2850 the __si_type_info (u084 g_004B2850) */
#endif

#endif /* GAME_T311_DC_MACHINE_TEST_H */
