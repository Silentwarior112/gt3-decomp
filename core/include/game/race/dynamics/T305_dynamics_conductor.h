/* T305_dynamics_conductor.h - DynamicsConductor, the dynamics and timing
 * conductor of a race (translation unit T305, unit u083),
 * 0x001ED000..0x001ED9F0; CarPairStates (T360).
 *
 * The conductor holds the eight cars (RaceCarState) and runs their frames
 * (update, T317), the car-to-car passes (T349), the slow car boost (T304),
 * the lap counter (T307), the race clock, the lap / checkpoint events and
 * records (T308, T303), the data logger's sector records (T306), the end of a
 * license test (T352), the rule queries (T324) and the per-mode behaviour of
 * its subclasses (Battle2P T309, BattleMP T310, MachineTest T311, License
 * T312, FreeRun T313, TimeTrial T314, FreePractice T315, SinglePlayer T316).
 * The class declares the members of all those translation units; each is
 * marked with its unit.
 *
 * Slot names: the Lupo 2 vtable of DynamicsConductor (vtables_L2.csv, 44
 * slots) is this one with two more slots, GetLapTimeCorrection /
 * SetLapTimeCorrection, after slot 13: GT3 slot k is Lupo 2 slot k (k < 14)
 * or k + 2.  The overrides of every subclass agree with that shift, so the
 * slots Lupo 2 names keep its names; slots 27, 32 and 33 are named after
 * their code (u077: getLapDistance, getGateCount, getGatePosition); the
 * others stay virtual_<slot>.  u078 named slots 14..16, 21, 30 and 33
 * without the shift.
 *
 * Inline in the original: Lupo 2 has the accessors 0x001ED6A8..0x001ED990 as
 * class-inline functions of DynamicsConductor (the same order reversed, the
 * same sizes); T305 emits out-of-line copies of all of them, with the
 * vtable, before the typeinfo function (none has a direct caller).  They are
 * defined inline after the class, in reverse address order (egcs emits the
 * #pragma implementation copies in reverse definition order); their Lupo 2
 * names are noted where the owner's name is kept.
 *
 * Times: the race clock (clock) counts 1/3 ms, 50 per frame (60 Hz); "ms"
 * values are clock / 3.  A crossing inside a frame is interpolated: frac50 =
 * (int)(fraction x 50), added to the clock.  0x157529FF (359,999,999 ms) is
 * the "no time" value of every record. */
#ifndef GAME_T305_DYNAMICS_CONDUCTOR_H
#define GAME_T305_DYNAMICS_CONDUCTOR_H

#include "game/game.h"
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve (held by value) */
#include "game/race/dynamics/T304_slow_car_boost.h"       /* SlowCarBoost (held by value) */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (held by value) */
#include "game/race/ai/T368_enemy_lines.h"          /* CourseLines (held by value) */
#include "game/car/physics/T167_rigid_body.h"           /* CarBoxList (held by value) */
#include "game/race/dynamics/T326_race_rule_options.h"    /* RaceRuleOptions (held by value) */
#include "game/race/ai/T372_pit_lane.h"             /* pit_lane_contains (IsInsidePit*Area, inline) */

#ifdef BOOT_OLD_GCC
/* #pragma interface here, #pragma implementation in T305's source: egcs
 * emits the out-of-line copies of the inline accessors and the vtable only
 * there, before __tf17DynamicsConductor, as the binary has them. */
#pragma interface
#endif

class RaceOrganization;                     /* T125_race_organization.h (pointers only) */
class RaceCourse;                           /* T153_race_course.h */
class RaceInput;                            /* T413_race_input.h */
class CarInput;                             /* T342_car_input.h */
class DynCarState;                          /* T288_car_sound.h */
class OneLapGhostInfo;                         /* T313_dc_free_run.h */
class DynamicsConductor;
class type_info;

/* The typeinfo function (vtable entry 0); it builds the __user_type_info
 * __ti17DynamicsConductor (0x00353410) on the first call.  GCC 14 build
 * only: egcs generates it (rule 8), and an extern "C" declaration before the
 * class would be the one egcs uses, so it would not emit it. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf17DynamicsConductor(void);    /* 0x001ED9B0 */
}
#endif

/* T360 (0x001F5918..0x001F5EF0, u085; no header of its own): for every
 * ordered pair of cars (i, j) a frame counter (0..300, up while the
 * conductor's slip[i][j] is positive, down otherwise) and a state 0..3 that
 * follows the pair's proximity code near[i][j] & 0xF (kinds 1..6); a state
 * of the mirrored pair (j, i) blocks or is set by some transitions.
 * DynamicsConductor +0xBA34 (Lupo 2: GetRaceWatcher returns it).  u093
 * called it CarPairInfo, u083 bA34.  Not polymorphic.  Size 0xC4. */
class CarPairStates {
public:
    void setOwner(DynamicsConductor *d);    /* 0x001F5918 (DynamicsConductor::initialize) */
    void reset();                           /* 0x001F5920 (virtual_21) */
    void update();                          /* 0x001F59B0 once a frame (DynamicsConductor::update) */
    /* No callers.  The strongest relation of car with another car (5 / 4 /
     * 3 on the same lap, 1 / 2 laps behind / ahead, 0 none), that car in
     * *other (-1: none). */
    int  strongest(int car, int *other);    /* 0x001F5C58 */
    /* No callers.  Bit j for every car j in a state with car; flags bit 1:
     * state 1 counts too, bit 2: also cars on another lap. */
    int  activeMask(int car, u16 flags);    /* 0x001F5DB8 */

    DynamicsConductor *dc;                  /* 0x00 */
    s16                timer[8][8];         /* 0x04 [i][j] frames, 0..300 */
    u8                 state[8][8];         /* 0x84 [i][j] 0..3 */
};
EGCS_SIZE(CarPairStates, 0xC4);

/* vtable _vt$17DynamicsConductor 0x0033CA20 (42 slots), vptr +0xE3A8 (egcs:
 * after all of its data).  typeinfo __tf17DynamicsConductor 0x001ED9B0
 * (__user_type_info 0x00353410).  No virtual destructor (slot 0 is empty and
 * not a destructor).  Size 0xE3AC (RaceArcade +0x261D4..+0x34580, T134; the
 * vptr store of the constructor at +0xE3A8). */
class DynamicsConductor {
public:
    /* The base constructor of every conductor (the subclass constructors call
     * it): the eight cars (RaceCarState()), lines.data = NULL, the eight
     * CarBoxes of boxes (their constructor 0x001F42E0 is emitted in T317),
     * then rules cleared and replaySpeed = 1. */
    DynamicsConductor();                    /* 0x001F2A10 (T317) */

    /* ---- the virtual functions, slot order ---- */
    /* Empty; the battles' overrides report the checkpoint time differences
     * (reportGateTimes). */
    virtual void ReportTimeDifference();    /* [ 0] +0x00C 0x001ED988 T305 */
    virtual void virtual_1();               /* [ 1] +0x014 0x001ED358 T305 the order back to the car indices */
    /* Sorts order[] by car_compare (insertion sort, leader first) and gives
     * every car its 1-based running position (st.rank). */
    virtual void virtual_2();               /* [ 2] +0x01C 0x001ED1F0 T305 */
    /* The slow car boost of st against the reference car ref: 1 for a
     * finished car, else boost_factor when ref is ahead (laps, then distance)
     * and the boost is on, slow_factor when st is ahead or level and the
     * brake is on.  Returns factor x scale; st.timeScale = that x
     * boost.scale. */
    virtual f32  virtual_3(DynCarState *ref, DynCarState *st, f32 scale); /* [ 3] +0x024 0x001ECEB8 T304 */
    virtual int  isDataLoggerSupported();   /* [ 4] +0x02C 0x001ED970 T305 0 */
    virtual int  isDataLoggerStandingStart(); /* [ 5] +0x034 0x001ED968 T305 0 */
    virtual int  isDataLoggerMultiLap();    /* [ 6] +0x03C 0x001ED960 T305 0 */
    /* The car's logger section, st.section (flag is not read). */
    virtual int  virtual_7(RaceCarState *car, int flag); /* [ 7] +0x044 0x001E0938 T290 */
    /* 1 when the rules give no launch speed (a standing start). */
    virtual int  getStartingFormat();       /* [ 8] +0x04C 0x001F2B80 T317 */
    /* Non-zero: virtual_20 enables the race's tyre wear (WearCurve::setup,
     * not on course kind 2). */
    virtual int  SetTireWearParameters(RaceRuleOptions *o);     /* [ 9] +0x054 0x001F3620 T317 0 */
    /* virtual_20 passes it to dc_setup_slow_car_boost (non-zero: the boost
     * of the rules). */
    virtual int  SetSlowCarBoostParameters(RaceRuleOptions *o); /* [10] +0x05C 0x001ED938 T305 0 */
    virtual void processCollision_();       /* [11] +0x064 0x001ED8B8 T305 collideWalls(0) */
    virtual void virtual_12();              /* [12] +0x06C 0x001EFCD0 T308 empty (once a frame) */
    /* Per frame: at the last countdown frame the standing-start logger
     * sections begin; after the countdown the clock runs (+50, held at
     * 0x405F7DFD = 99:59:59.999). */
    virtual void virtual_13();              /* [13] +0x074 0x001EFCD8 T308 */
    /* Empty; the battles record the checkpoint time (recordGate). */
    virtual void IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split); /* [14] +0x07C 0x001ED8B0 T305 */
    /* A checkpoint of the lap: dc_post_section_time with the car's fault byte
     * (raw is not read). */
    virtual void virtual_15(int car, int lap, int gate, u32 raw, u32 ms, u32 split); /* [15] +0x084 0x001EFAE8 T308 */
    virtual void virtual_16(int car, u32 ms, int frac50, f32 t);    /* [16] +0x08C 0x001EFB68 T308 empty: the start line */
    /* The end of a lap: the car's race ends (event 5, car_finish(3), for car 0
     * setFinishTimer(3)) when the leader has finished, the lap count is
     * reached or the time limit is over; the next lap starts at ms. */
    virtual void virtual_17(int car, int lap, u32 raw, u32 ms, u32 split, int frac50, f32 t); /* [17] +0x094 0x001EFB70 T308 */
    /* The slot 30 line (the finish); non-zero: close the logger section. */
    virtual int  virtual_18(int car, int lap, u32 raw, u32 ms, u32 split); /* [18] +0x09C 0x001EFCC8 T308 0 */
    /* Empty: the car's input of the frame (car_input_update, T342); FreeRun
     * records it for the ghost. */
    virtual void recordOneLapGhostControl(RaceInput *in, CarInput *packet); /* [19] +0x0A4 0x001ED860 T305 */
    /* The race-wide setup (initialize calls it with 1 first): the surface and
     * sound tables, slots 9 / 10 on the rules, the wear curve (enabled by
     * slot 9 unless the course kind is 2), the input tuning, the clock,
     * replaySpeed = 1. */
    virtual void virtual_20(int a);         /* [20] +0x0AC 0x001F2FE0 T317 */
    /* The second half (initialize calls it with 1 last; a is not read). */
    virtual void virtual_21(int a);         /* [21] +0x0B4 0x001F30D8 T317 */
    /* "DynamicsConductor::initialize()" (SetupCarSpec's caller string): the
     * course, the course kind, the race (slot 20), then every car: car_init,
     * slot 23, its CarSpec, its grid record, slot 24, DynCar_initialize; then
     * slot 21.  mode is kept in mode (u043: the replay mode). */
    virtual void initialize(int mode);      /* [22] +0x0BC 0x001F3628 T317 */
    /* The set-up of car `car': *tyreFlag goes to SetupCarSpec, the others
     * to DynCar_initialize. */
    virtual void getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* [23] +0x0C4 0x001F3298 T317 */
    /* The start of car `car' (startType / startArg / speedKmh of
     * DynCar_initialize, all 0 before the call; the result is not used). */
    virtual int  getSpecialStart(int car, int *startType, int *startArg, int *speedKmh); /* [24] +0x0CC 0x001F3378 T317 */
    /* Start type 2 or 3 if they apply, else type 4 with a grid argument. */
    virtual void virtual_25(int car, int *startType, int *startArg, int *speedKmh); /* [25] +0x0D4 0x001F34B8 T317 */
    virtual void virtual_26();              /* [26] +0x0DC 0x001F42D8 T317 empty */
    /* The car's lap distance (st.lapDistance, updateCar / DynCar_place):
     * courseDistance(car); MachineTest's is noseDistance(car). */
    virtual f32  getLapDistance(RaceCarState *car); /* [27] +0x0E4 0x001EE958 T307 */
    virtual u32  CurrentLapTime(RaceCarState *car); /* [28] +0x0EC 0x001ED750 T305 lapTime(car) */
    virtual int  virtual_29();              /* [29] +0x0F4 0x001EF8B8 T308 the lap count (rules.laps) */
    /* The finish line's course position (the course's runway +0x1C; 0:
     * none). */
    virtual f32  virtual_30();              /* [30] +0x0FC 0x001F2C90 T317 */
    /* -1; 0 / 1 / 2: how update pairs the cars for slot 3. */
    virtual int  getSlowCarBoostType_();    /* [31] +0x104 0x001ED700 T305 -1 */
    virtual int  getGateCount();            /* [32] +0x10C 0x001F2BD0 T317 checkpoints per lap (at most 4) */
    virtual f32  getGatePosition(int gate); /* [33] +0x114 0x001F2C10 T317 a checkpoint's course position (-1: none) */
    virtual int  virtual_34();              /* [34] +0x11C 0x001D9FB8 T324 0 */
    virtual int  virtual_35();              /* [35] +0x124 0x001D9FC0 T324 0 */
    virtual int  virtual_36();              /* [36] +0x12C 0x001D9FC8 T324 0: the course-out level on counted ground */
    virtual int  virtual_37();              /* [37] +0x134 0x001D9FD0 T324 0: off the course */
    /* The race has pit stops: 1; 0 in License / FreeRun / TimeTrial: no
     * DYN_FLAG_COUNTED_GROUND (the pit area's ground), no car_tyre_check
     * pit calls (T296). */
    virtual int  isPitEnabled();            /* [38] +0x13C 0x001D9FD8 T324 1 */
    /* The course-out rule: all four wheels on counted ground (inside the pit
     * lane) / off the course count at the level of slot 36 / 37
     * (st.courseOut, st.fault = FAULT_COURSE_OUT); the counts come from
     * car_ground_contact (T298).  Skipped when fault is set or the driver is
     * not the pad. */
    virtual void chargePenaltyBySurface(RaceCarState *car, int countedWheels, int offCourseWheels); /* [39] +0x144 0x001DA020 T324 */
    /* Empty: the collision rule (car_report_crash, T288, calls it with its four
     * arguments). */
    virtual void issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force); /* [40] +0x14C 0x001DA128 T324 */
    virtual int  EveryTimePitStop();        /* [41] +0x154 0x001ED6D8 T305 0 (non-zero: a pit stop is due) */

    /* ---- T305 ---- */
    int   getOrder(int i);                  /* 0x001ED000 order[i]: the car at running position i */
    /* How far a is ahead of b: whole laps and the course distance, the
     * distance kept with the sign of the laps. */
    void  getGap(DynCarState *a, DynCarState *b, int *laps, f32 *dist); /* 0x001ED028 */
    void  clearGapTable();                  /* 0x001ED410 */
    void  updateGapTable();                 /* 0x001ED458 gapLap / gapDist of every pair */
    int   gapLaps(int i, int j);            /* 0x001ED5A0 gapLap[i][j] */
    f32   gapDistance(int i, int j);        /* 0x001ED5C0 gapDist[i][j] */
    /* The gap with the distance brought into -length/2..length/2. */
    int   gapNearest(int i, int j, f32 *dist); /* 0x001ED5E0 */
    int   GetOptionTT_IsGhostMapped();      /* 0x001ED6A8 1 (u083 F_001ED6A8) */
    int   GetOptionArcadeBattle_CrashDamage(); /* 0x001ED6B0 0 (F_001ED6B0) */
    int   GetOptionArcadeRace_CrashDamage(); /* 0x001ED6B8 0 (F_001ED6B8) */
    int   IsOneWayCourse();                 /* 0x001ED6C0 0 (F_001ED6C0) */
    int   getMode();                        /* 0x001ED6C8 mode (Lupo 2 IsReplay_) */
    int   isCountingDown();                 /* 0x001ED6E0 getCountDown() > 0 (Lupo 2 IsBeforeRace) */
    int   isTimeOver(u32 ms);               /* 0x001ED708 ms at or past the time limit (Lupo 2 isTimeUp) */
    u32   getTimeLimit();                   /* 0x001ED738 rules.timeLimit x 60000 ms (Lupo 2 getLimitTime) */
    u32   clockMs(int frac50);              /* 0x001ED770 (clock + frac50) / 3 (Lupo 2 CorrectedTotalTime) */
    u32   clockRem(int frac50);             /* 0x001ED7A0 (clock + frac50) % 3 (Lupo 2 GetResidualTime3000) */
    u32   getClockMs();                     /* 0x001ED7D8 clock / 3 (Lupo 2 CurrentTotalTime) */
    RaceRuleOptions *getRules();            /* 0x001ED800 &rules (Lupo 2 GetRunTimeRaceParameterSource) */
    CarPairStates *GetRaceWatcher();        /* 0x001ED810 &pairs (u083 getBA34) */
    f32   getGapDistance(int i, int j);     /* 0x001ED820 (Lupo 2 GetCourseVdifferenceForSlowCarBoost) */
    int   getGapLaps(int i, int j);         /* 0x001ED840 (Lupo 2 GetLapDifferenceForSlowCarBoost) */
    WearCurve *getWearCurve();              /* 0x001ED868 &wear (Lupo 2 GetTireWearParameters) */
    CarBoxList *GetCollisionProcessor();    /* 0x001ED870 &boxes (u083 getCarAux) */
    RaceCourse *getCourse();                /* 0x001ED880 */
    int   getCarCount();                    /* 0x001ED890 (Lupo 2 GetNumberOfAutomobiles) */
    void  setCarCount(int n);               /* 0x001ED8A0 (Lupo 2 SetNumberOfAutomobiles) */
    int   IsInsidePitRedArea(f32 pos);      /* 0x001ED8D8 pit_lane_contains(&lines, pos, 1), T372 (u083 F_001ED8D8) */
    int   IsInsidePitYellowArea(f32 pos);   /* 0x001ED900 pit_lane_contains(&lines, pos, 0) (F_001ED900) */
    CourseLines *GetEnemyLineProcessorOld(); /* 0x001ED928 &lines (u083 getGates) */
    f32   getReplaySpeed();                 /* 0x001ED940 (u083 getB7B0) */
    void  setReplaySpeed(f32 v);            /* 0x001ED950 (setB7B0) */
    RaceOrganization *getOrganization();    /* 0x001ED978 org (Lupo 2 GetRaceOrganization) */
    SlowCarBoost *getSlowCarBoost();        /* 0x001ED980 &boost (Lupo 2 getBoostParameters) */
    DynCarState *getRunState(int car);      /* 0x001ED990 &getCar(car)->st (Lupo 2 GetCarStatus) */

    /* ---- T306: the data logger's sector records ---- */
    /* The car (car_copy) and its CarBox record, when that fits 0x400 bytes. */
    void  saveSector(OneLapGhostInfo *rec, int car);    /* 0x001EE638 */
    /* The car back as saved (keeping its noController / carIndex / driver);
     * its lap restarts at the sector's start time. */
    void  restoreSector(int car, OneLapGhostInfo *rec); /* 0x001EE6D8 */

    /* ---- T307: the lap counter ---- */
    void  clearGateTimes();                 /* 0x001EE7E8 the clock and the battles' checkpoint table */
    /* The course distance of the car's nose: st.pos + st.rot[0] (the forward
     * row) x params.lenFront (MachineTest slot 27; not a prediction, h20). */
    f32   noseDistance(RaceCarState *car); /* 0x001EE878 */
    f32   courseDistance(RaceCarState *car);    /* 0x001EE900 the course axes are (x, z, -y) */
    /* prev: the course position of the last frame.  The checkpoints, the
     * logger sections, the slot 30 line, the start / finish line.  1 when the
     * car went backwards (or backed over the line). */
    int   countLap(int car, f32 prev);      /* 0x001EE978 */
    /* Lap counting once the countdown is over, the "wrong way" message, the
     * pit messages, the pit lane (car_update_pit, T350). */
    void  updateCar(int car);               /* 0x001EF060 */
    /* The battles' slot 0: the checkpoint times recorded since the last
     * report as display events (T303). */
    void  reportGateTimes();                /* 0x001EF268 */

    /* ---- T352: the end of a license test ---- */
    void  licenseFailed(RaceCarState *car, int reason); /* 0x001EF520 (reason 3 course out, 4 collision; not read) */
    /* The run ends with time: rank i + 1 of the first of the five
     * RaceOrganization::getTargetTime(i) it does not exceed (2..4 for gold /
     * silver / bronze: i 0 and 4 give 0); none fails the test. */
    void  licenseFinished(RaceCarState *car, u32 time); /* 0x001EF5C0 */
    /* License slot 13: a course out or a collision fails the test; in a stop
     * test (rules.licenseType 1) a stop in the box ends it. */
    void  licenseCheckStop(RaceCarState *car); /* 0x001EF780 */

    /* ---- T308: the race progress ---- */
    u32   lapTime(RaceCarState *car);       /* 0x001EF8C8 ms since the car's last lap end */
    u32   lapTimeIfStarted(RaceCarState *car); /* 0x001EF8F8 */
    /* The battles' slot 14: the time sorted into gate's list of lap (a car a
     * lap or more behind gets its lap gap shown instead). */
    void  recordGate(int car, int lap, int gate, u32 time); /* 0x001EF920 */
    /* Car 0's time at or under the race's best lap: result.lapRecord = 1 and,
     * with a rank, event 0x13.  1 if so. */
    int   checkLapRecord(int car, u32 time); /* 0x001EFDE8 */
    /* Slot 26 of MachineTest / License / TimeTrial: 1 once car 0 has a best
     * lap that beats car 1's (result.beatGhost = 1); a full or finished logger
     * record of the player's input clears result.lapRecord instead. */
    int   checkRunResult();                 /* 0x001EFE80 */

    /* ---- T324 ---- */
    /* The collision rule of License / TimeTrial slot 40: kind 0 is a fault
     * when force x invMass > 10, kind 2 always, kind 1 never (st.fault |= 1);
     * only for carIndex 0 before the finish.  arg is not read. */
    void  F_001DA130(RaceCarState *car, int kind, int arg, f32 force); /* 0x001DA130 */

    /* ---- T317 ---- */
    void  setFinishTimer(int seconds);      /* 0x001F2AE8 only in mode 2: seconds x 60 + 1 frames */
    void  tickFinishTimer();                /* 0x001F2B18 at 0: event 1 to the race */
    RaceCarState *getCar(int i);            /* 0x001F2B68 &cars[i] (50 callers) */
    int   getCountDown();                   /* 0x001F2B98 org's countdown frames */
    f32   getLapLength();                   /* 0x001F2BA8 the course length (RaceCourse::getLapLength) */
    /* The names of the T317 helpers below are u085's (first pass F_<address>). */
    int   hasFinishLine();                  /* 0x001F2CB0 virtual_30() != 0 (countLap, T307) */
    /* No callers.  The point of enemy line `kind' (lines_select) at course
     * distance dist as a world position pos = (x, height, -y). */
    int   linePointWorld(int kind, int hint, f32 *pos, f32 dist); /* 0x001F2CF8 the line query's result, -1 without the line */
    /* The point dist m along the racing line (clamped to +-(lap length -
     * 20)): its sector, x, y, height, heading; 0 without a racing line, else
     * 1 (dist >= 0) or -1 (DynCar_initialize's start types 1..5). */
    int   racingLinePoint(int *sector, f32 *x, f32 *y, f32 *height, f32 *angle, int dist); /* 0x001F2DB0 */
    int   hasPitLane();                     /* 0x001F2E98 lines.hasPit (lines 3 and 4) */
    int   pitSlotNode(int slot);            /* 0x001F2EA8 pit_slot_node: the pit lane node of pit slot `slot' */
    /* The course distance of pit lane node i (i < 0 from the end:
     * pit_node_dist_wrapped): a pit gate's position. */
    f32   pitGateDistance(int i);           /* 0x001F2EC8 */
    f32   pitBoxDistance(int i);            /* 0x001F2EE8 pit_node_dist: a pit box's position */
    /* pit_node_angle: the heading of pit lane node i, rad (car_pit_jack
     * turns the car to its pit box's). */
    f32   pitNodeAngle(int i);              /* 0x001F2F08 */
    void  updateCourseKind();               /* 0x001F2F28 courseKind from the race database */
    int   getCourseKind();                  /* 0x001F2F68 courseKind, 0 for 0xFF */
    int   isDirtCourse();                   /* 0x001F2F90 course kind 2 (the rally courses) */
    int   isCourseKind1();                  /* 0x001F2FB8 course kind 1 (? kind) */
    /* Slot 23 of Battle2P, BattleMP, FreePractice, SinglePlayer / of
     * MachineTest, License, FreeRun, TimeTrial: *tyreFlag goes to
     * SetupCarSpec, the others to DynCar_initialize. */
    void  getEntrantInfoRace(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* 0x001F3188 */
    void  getEntrantInfoTrial(int car, int *tyreFlag, int *transMode, int *driver, int *noController); /* 0x001F3218 */
    int   getRuleStartArg();                /* 0x001F32D0 rules.launchPoint */
    int   getRuleStartSpeed();              /* 0x001F32E0 rules.launchSpeed */
    /* Start type 1 at the rules' launch point, if not 0 (License slot 24,
     * which passes speedKmh in $a4 as for the other start helpers; not
     * read). */
    int   startAtLaunchPoint(int car, int *startType, int *startArg, int *speedKmh); /* 0x001F32F0 */
    int   startWithLaunchSpeed(int car, int *startType, int *startArg, int *speedKmh); /* 0x001F3340 the rules' start speed */
    int   rollingStart(int car, int *startType, int *startArg, int *speedKmh); /* 0x001F3398 start type 2 */
    int   courseStart(int car, int *startType, int *startArg, int *speedKmh);  /* 0x001F3420 start type 3 */
    /* RaceBase::virtual_8: org, the rules from the race (two switched off in
     * a race preview or the background race). */
    void  attach(RaceOrganization *org);    /* 0x001F35B8 */
    /* Slot 11 of Battle2P, BattleMP, FreePractice, SinglePlayer: every car
     * against the walls and the other cars (boxes_step). */
    void  collideAllCars();                 /* 0x001F3918 */
    /* Slot 11 of the base (first 0) and update in a replay (first 1): each
     * car from `first' on alone against the walls (boxes_step_car). */
    void  collideWalls(int first);          /* 0x001F3950 */
    /* One frame of the dynamics (RaceBase::virtual_29).  With mode != 0 the
     * cars step when their step counter (interpBias) runs out (slow motion
     * by replaySpeed).  Then slot 3 on pairs of cars as slot 31 chooses, the
     * finish timer and the pair states. */
    void  update(int replay);               /* 0x001F39D8 */

    /* ---- data ---- */
    RaceOrganization *org;                  /* 0x0000 (u085 owner, attach) */
    u8              unk_0004[0x80];         /* 0x0004 */
    WearCurve       wear;                   /* 0x0084 (u077 DcSub84, u081 DcTyreSettings; u051 / u086 x3 +0x9C) */
    SlowCarBoost    boost;                  /* 0x00AC */
    RaceCarState    cars[8];                /* 0x00C8 (u085 DynCar) */
    RaceCourse     *course;                 /* 0xB7A8 org's course */
    u8              mode;                   /* 0xB7AC initialize(mode): 2 a finish timer, 3 (? a replay); non-zero:
                                               no time records or difference events (u082 noRecord) */
    u8              courseKind;             /* 0xB7AD the race database's course kind (0xFF: none; 2 dirt: isDirtCourse) */
    u16             finishTimer;            /* 0xB7AE frames, event 1 at 0 (setFinishTimer) */
    f32             replaySpeed;            /* 0xB7B0 1.0 (the logger's replay: 1, 0.5, 0.25, 0); the fraction
                                               of the car_view_*_now accessors (u082 interp, u085 timeStep) */
    f32             slip[8][8];             /* 0xB7B4 [car][other]: other's slipstream on car (T349; u085
                                               pairValue) */
    u8              near[8][8];             /* 0xB8B4 [car][other]: the proximity code (car_near_code; u085
                                               pairKind) */
    s8              gapLap[8][8];           /* 0xB8F4 [i][j]: laps of j ahead of i (updateGapTable) */
    f32             gapDist[8][8];          /* 0xB934 [i][j]: course distance of j ahead of i */
    CarPairStates   pairs;                  /* 0xBA34 (u093 CarPairInfo pairs) */
    u32             gateTime[4][8];         /* 0xBAF8 [checkpoint][order] ms (the battles, recordGate) */
    s16             gateLap[4];             /* 0xBB78 the lap the times are of */
    s8              gateCount[4];           /* 0xBB80 times recorded */
    s8              gateDone[4];            /* 0xBB84 times reported */
    s8              gateCar[4][8];          /* 0xBB88 the car of each time */
    CourseLines     lines;                  /* 0xBBA8 the computer drivers' lines (u083 CourseGates, u085
                                               CourseGrid, u086 CourseLines) */
    CarBoxList      boxes;                  /* 0xBBB4 the collision boxes, T373 (u083 CarAuxSet, u089 CarBoxList;
                                               u081 CarBox[8] +0xBBC0) */
    RaceRuleOptions rules;                  /* 0xE340 (u082 RaceRules, u077 tbl_E340) */
    u32             clock;                  /* 0xE380 1/3 ms, +50 a frame (u078 frame, u086 frames) */
    int             carCount;               /* 0xE384 */
    int             order[8];               /* 0xE388 car indices by running position */
    /*  vptr                                   0xE3A8 */
};
EGCS_SIZE(DynamicsConductor, 0xE3AC);

/* The inline accessors (0x001ED6A8..0x001ED990, see the top). */
inline DynCarState *DynamicsConductor::getRunState(int car) { return &getCar(car)->st; }  /* 0x001ED990 */
inline void DynamicsConductor::ReportTimeDifference() { }  /* 0x001ED988 */
inline SlowCarBoost *DynamicsConductor::getSlowCarBoost() { return &boost; }  /* 0x001ED980 */
inline RaceOrganization *DynamicsConductor::getOrganization() { return org; }  /* 0x001ED978 */
inline int DynamicsConductor::isDataLoggerSupported() { return 0; }  /* 0x001ED970 */
inline int DynamicsConductor::isDataLoggerStandingStart() { return 0; }  /* 0x001ED968 */
inline int DynamicsConductor::isDataLoggerMultiLap() { return 0; }  /* 0x001ED960 */
inline void DynamicsConductor::setReplaySpeed(f32 v) { replaySpeed = v; }  /* 0x001ED950 */
inline f32 DynamicsConductor::getReplaySpeed() { return replaySpeed; }  /* 0x001ED940 */
inline int DynamicsConductor::SetSlowCarBoostParameters(RaceRuleOptions *o) { return 0; }  /* 0x001ED938 */
inline CourseLines *DynamicsConductor::GetEnemyLineProcessorOld() { return &lines; }  /* 0x001ED928 */
inline int DynamicsConductor::IsInsidePitYellowArea(f32 pos) { return pit_lane_contains(&lines, pos, 0); }  /* 0x001ED900 */
inline int DynamicsConductor::IsInsidePitRedArea(f32 pos) { return pit_lane_contains(&lines, pos, 1); }  /* 0x001ED8D8 */
inline void DynamicsConductor::processCollision_() { collideWalls(0); }  /* 0x001ED8B8 */
inline void DynamicsConductor::IssuePassingGate(int car, int lap, int gate, u32 ms, u32 split) { }  /* 0x001ED8B0 */
inline void DynamicsConductor::setCarCount(int n) { carCount = n; }  /* 0x001ED8A0 */
inline int DynamicsConductor::getCarCount() { return carCount; }  /* 0x001ED890 */
inline RaceCourse *DynamicsConductor::getCourse() { return course; }  /* 0x001ED880 */
inline CarBoxList *DynamicsConductor::GetCollisionProcessor() { return &boxes; }  /* 0x001ED870 */
inline WearCurve *DynamicsConductor::getWearCurve() { return &wear; }  /* 0x001ED868 */
inline void DynamicsConductor::recordOneLapGhostControl(RaceInput *in, CarInput *packet) { }  /* 0x001ED860 */
inline int DynamicsConductor::getGapLaps(int i, int j) { return gapLaps(i, j); }  /* 0x001ED840 */
inline f32 DynamicsConductor::getGapDistance(int i, int j) { return gapDistance(i, j); }  /* 0x001ED820 */
inline CarPairStates *DynamicsConductor::GetRaceWatcher() { return &pairs; }  /* 0x001ED810 */
inline RaceRuleOptions *DynamicsConductor::getRules() { return &rules; }  /* 0x001ED800 */
inline u32 DynamicsConductor::getClockMs() { return clock / 3; }  /* 0x001ED7D8 */
/* t - (t / 3) x 3, not t % 3: retail divides and multiplies back (divu,
 * mflo, x 3, subu; `%' gives mfhi).  Exact so here (0x001ED7A0) and in
 * car_sector_begin 0x001E0940 (u078). */
inline u32 DynamicsConductor::clockRem(int frac50) { u32 t = clock + frac50; return t - (t / 3) * 3; }  /* 0x001ED7A0 */
inline u32 DynamicsConductor::clockMs(int frac50) { return (clock + frac50) / 3; }  /* 0x001ED770 */
inline u32 DynamicsConductor::CurrentLapTime(RaceCarState *car) { return lapTime(car); }  /* 0x001ED750 */
inline u32 DynamicsConductor::getTimeLimit() { return rules.timeLimit * 60000; }  /* 0x001ED738 */
/* 0x001ED708 */
inline int DynamicsConductor::isTimeOver(u32 ms)
{
    u32 limit = rules.timeLimit * 60000;
    int over = 0;

    if (limit != 0)
        over = !(ms < limit);
    return over;
}
inline int DynamicsConductor::getSlowCarBoostType_() { return -1; }  /* 0x001ED700 */
inline int DynamicsConductor::isCountingDown() { return getCountDown() > 0; }  /* 0x001ED6E0 */
inline int DynamicsConductor::EveryTimePitStop() { return 0; }  /* 0x001ED6D8 */
inline int DynamicsConductor::getMode() { return mode; }  /* 0x001ED6C8 */
inline int DynamicsConductor::IsOneWayCourse() { return 0; }  /* 0x001ED6C0 */
inline int DynamicsConductor::GetOptionArcadeRace_CrashDamage() { return 0; }  /* 0x001ED6B8 */
inline int DynamicsConductor::GetOptionArcadeBattle_CrashDamage() { return 0; }  /* 0x001ED6B0 */
inline int DynamicsConductor::GetOptionTT_IsGhostMapped() { return 1; }  /* 0x001ED6A8 */

/* The lap count less the times the car backed over the line. */
int  car_effective_lap(DynCarState *st);                       /* 0x001ED018 */
/* > 0: a is ahead of b, < 0: b is, 0 level.  More (effective) laps first;
 * on the same lap the course position while neither has finished; a
 * finished car behind one that has not; two finished cars by their last lap
 * end, the earlier first. */
int  car_compare(DynCarState *a, DynCarState *b);              /* 0x001ED108 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __user_type_info of DynamicsConductor (2 words; u083 / u084
 * g_00353410). */
extern "C" u32 __ti17DynamicsConductor[2];  /* 0x00353410 gp-0x45E0 */
#endif

#endif /* GAME_T305_DYNAMICS_CONDUCTOR_H */
