/* T313_dc_free_run.h - DynamicsConductorFreeRun, the conductor of the time
 * trial's free run with the ghost (translation unit T313, unit u084),
 * 0x001F1190..0x001F1AD8; OneLapGhostInfo.
 *
 * The player's input (RaceBase::virtual_26(0x100)) records each lap: at the
 * line its lap record (RaceInput::lap) gets a snapshot of the car
 * (saveSector) and the start time (ghost_record_start); at the lap's end
 * the lap time goes into the record and a lap better than the best one
 * (RaceInput::bestLap) becomes the best (RaceInput::keepLap), offering the
 * ghost save (result +0x0C) and, when it also beats the ghost car's lap, the
 * ghost (result +0x08, RaceOrganization setupGhost).  With two cars, car 1 is
 * the ghost: its input replays its record, and at the line the ghost car is
 * put back to the record's snapshot (ghost_restart).  There are no laps to
 * finish (slot 29 = 0).
 *
 * The plan's class Manager of this unit is AutomobileControlRecord::Manager
 * (u047 / u048 / u049 / u084 nest it in a class AutomobileControlRecord):
 * T318_control_record.h defines it, and this header includes it. */
#ifndef GAME_T313_DC_FREE_RUN_H
#define GAME_T313_DC_FREE_RUN_H

#include "game/game.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* base DynamicsConductor */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (held by value) */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Manager (the plan's Manager) */

class RaceInput;                            /* T413_race_input.h (pointers only) */
class type_info;

/* The typeinfo function: it builds the __si_type_info at 0x004B2870 (base
 * __ti17DynamicsConductor).  GCC 14 build only (see T305): egcs generates
 * it, and an extern "C" declaration before the class would be the one
 * egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf24DynamicsConductorFreeRun(void);     /* 0x001F1A88 */
}
#endif

/* The data logger's record of one sector of a car, and the head of a lap of
 * a RaceInput's control record (the 0x1B00 bytes before the record in a
 * 0x8000-byte slot, AutomobileControlRecord::Manager::getSlot): a snapshot
 * of the car and of its CarBox (saveSector / restoreSector, T306), with the
 * sector's logger span and time (car_sector_begin / car_sector_end, T290).
 * The RaceInput's snapshots (+0x12C) are an array of them.  The name is
 * Lupo 2's (RaceInput::getGhostInfo / setLoggerSector(OneLapGhostInfo *);
 * the first pass called it SectorRecord).  Not polymorphic.  Size 0x1B00. */
class OneLapGhostInfo {
public:
    u8           unk_00;                    /* 0x0000 */
    u8           unk_01;                    /* 0x0001 */
    u8           frac50;                    /* 0x0002 the clock offset of the start inside its frame (u078 kind:
                                               car_sector_begin's argument) */
    u8           clockRem;                  /* 0x0003 (clock + 50) % 3 at the start (u078 variant) */
    int          start;                     /* 0x0004 the logger position at the start (u078) */
    int          length;                    /* 0x0008 the logger positions the sector spans (u078) */
    f32          t;                         /* 0x000C the frame fraction of the start (u078 f0C) */
    u32          time;                      /* 0x0010 ms, 0 / 0x157529FF: none (u084 lapTime: a lap's time in
                                               the free run) */
    u8           unk_14[0x10];              /* 0x0014 */
    /* The snapshot, raw bytes (u084 u8[0x16DC]): car_copy / car_restore
     * (T351) copy it as bytes (lwl / lwr for the last word: alignment 1).
     * GT3's RaceCarState: the enhanced engine's fields after it are not
     * snapshotted, so the 0x1B00-byte slot head keeps its layout. */
    u8           car[RACE_CAR_STATE_GT3_SIZE]; /* 0x0024 */
    u8           aux[0x400];                /* 0x1700 the saved CarBox record (when it fits; else 0) */
};
EGCS_SIZE(OneLapGhostInfo, 0x1B00);

/* vtable 0x0033D390 (42 slots: [4] [8] [12] [13] [15] [16] [17] [19] [20]
 * [21] [23] [24] [28] [29] [34]..[38] its own).  typeinfo
 * __tf24DynamicsConductorFreeRun 0x001F1A88 (si, base DynamicsConductor).
 * vptr +0xE3A8.  Size 0xE3AC: no data of its own. */
class DynamicsConductorFreeRun : public DynamicsConductor {
public:
    DynamicsConductorFreeRun();             /* 0x001F1190 (F_00189138) */

    virtual int  isDataLoggerSupported();   /* [ 4] +0x02C 0x001F1A80 1 */
    virtual int  getStartingFormat();       /* [ 8] +0x04C 0x001F1A78 2 */
    /* Car 0's countdown; when it runs out the driver gets the car back
     * (car_mode_driver) and event 4. */
    virtual void virtual_12();              /* [12] +0x06C 0x001F1968 */
    virtual void virtual_13();              /* [13] +0x074 0x001F19E0 the base's */
    /* dc_post_section_diff, then the base's section time events. */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001F12A8 */
    /* The start line: car 0 gets the car back, the ghost car is put back
     * (two cars, outside the logger), a new lap record starts (mode 0);
     * every car: no controller or a logger run drives itself (car_mode_computer);
     * the lap starts at ms. */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t); /* [16] +0x08C 0x001F1540 */
    /* A lap's end: the lap difference and lap time events; car 0's lap goes
     * into its record (the best one and the ghost offered as above); then
     * as slot 16. */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001F16B8 */
    /* The packet of a recorded input (in->enabled) goes to the player's
     * input (RaceInput::recordLap). */
    virtual void recordOneLapGhostControl(RaceInput *in, CarInput *packet); /* [19] +0x0A4 0x001F14E8 */
    virtual void virtual_20(int a);         /* [20] +0x0AC 0x001F11D0 the base's */
    /* The base's; then (mode 0) a best lap record without a time gets
     * 0x157529FF. */
    virtual void virtual_21(int a);         /* [21] +0x0B4 0x001F11F0 */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F1268 getEntrantInfoTrial,
                                               then a non-zero *d becomes 2 */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F1A48 slot 25 */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001F1A28 lapTimeIfStarted(car) */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001F12A0 0: no laps */
    virtual int  virtual_34();              /* [34] +0x11C 0x001F1A00 */
    virtual int  virtual_35();              /* [35] +0x124 0x001F1A08 */
    virtual int  virtual_36();              /* [36] +0x12C 0x001F1A10 */
    virtual int  virtual_37();              /* [37] +0x134 0x001F1A18 */
    virtual int  isPitEnabled();            /* [38] +0x13C 0x001F1A20 */
};
/* The enhanced engine: DynamicsConductor's (GT3 0xE3AC). */
EGCS_SIZE(DynamicsConductorFreeRun, 0xE58C);

/* The start of a recorded lap of input in: the head of its lap record
 * (RaceInput::lap's slot) gets the car's snapshot (saveSector), frac50, t
 * and the remainder (clock + 50) % 3; the lap record rewinds
 * (RaceInput::resetLap, given the start time (clock + 50) / 3 ms, which it
 * does not read).  Callers: slots 16 and 17 (t before frac50: the callers
 * load the argument registers in that order). */
void ghost_record_start(RaceInput *in, RaceCarState *car, f32 t, int frac50); /* 0x001F1348 */
/* For an input that is not held (RaceInput::hold == 0: the ghost runs):
 * the car back to the snapshot at the head of the record it plays (rec's
 * slot, restoreSector), on lap lap; the record rewinds; interpBias = t -
 * the record's t (+1 if negative; then, outside the logger, the car of
 * index 1 runs an extra DynamicsConductor::update(1)).  Callers:
 * dc_logger_start (T290), slots 16 and 17. */
void ghost_restart(RaceInput *in, RaceCarState *car, int lap, f32 t);       /* 0x001F1400 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti24DynamicsConductorFreeRun[3]; /* 0x004B2870 the __si_type_info (u084 g_004B2870) */
#endif

#endif /* GAME_T313_DC_FREE_RUN_H */
