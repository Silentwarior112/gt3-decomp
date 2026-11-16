/* T317 - DynamicsConductor (0x001F2A10..0x001F4330): the base constructor,
 * the per-race set-up of the cars (initialize = slot 22, which names itself
 * "DynamicsConductor::initialize()"), the start rules (slots 23..25: the
 * start type, argument and speed of each car), the frame step of all the
 * cars (update) and small accessors of the course (the checkpoints, the
 * finish line), the rules, the enemy lines (+0xBBA8) and the collision
 * boxes (+0xBBB4).  CarBox's constructor (T376) is emitted here too.  The
 * class is declared in T305_dynamics_conductor.h. */
/* Retail T317's .rodata (0x0033DB28): <new>'s "bad_alloc" (nothing
 * references it: DATA_START), initialize's literal, then the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (reverse definition order: the
 * original saw the race course, then RaceCameraBase). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/T383_race_camera_base.h"     /* (seen: RaceCameraBase) */
#include "game/race/dynamics/control_record_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033DB28

// FUNC 0x001F2A10 DynamicsConductor::DynamicsConductor
/* The base constructor of every conductor (the eight subclass constructors
 * call it).  egcs stores the vptr first, then builds the members: the eight
 * cars (RaceCarState(): the CarCondition of st, 0x001D9B10, then
 * gearWhineOn / unk_16D1 = 0), lines (CourseLines(): data = NULL), the eight
 * boxes of boxes (CarBox::CarBox, 0x001F42E0), then the body. */
DynamicsConductor::DynamicsConductor()
{
    /* implicit: CarCondition, CarBox (the members' constructors) */
    memset(&rules, 0, sizeof(RaceRuleOptions));     /* 0x40 */
    replaySpeed = 1.0f;
}

// FUNC 0x001F2AE8 DynamicsConductor::setFinishTimer
/* Only in the demo (mode 2): the timer runs `seconds' (60 frames each)
 * plus one frame.  From the end of a lap (slot 17) of car 0. */
void DynamicsConductor::setFinishTimer(int seconds)
{
    if (mode == DC_MODE_DEMO)
        finishTimer = seconds * 60 + 1;
}

// FUNC 0x001F2B18 DynamicsConductor::tickFinishTimer
/* Counts the timer down (once a frame, update); when it reaches 0, posts
 * event 1 to the race. */
void DynamicsConductor::tickFinishTimer()
{
    if (finishTimer != 0) {
        finishTimer = finishTimer - 1;
        if (finishTimer == 0)
            org->eventQueue.post(RACE_EVENT_END, 0, 0);
    }
}

// FUNC 0x001F2B68 DynamicsConductor::getCar
RaceCarState *DynamicsConductor::getCar(int i)
{
    return &cars[i];
}

// FUNC 0x001F2B80 DynamicsConductor::getStartingFormat
/* Slot 8: 1 when the rules give no launch speed (a standing start).  The
 * rules are read through RaceRuleOptions' inline accessors here and in
 * getRuleStartArg / getRuleStartSpeed: retail keeps &rules (this + 0xE340)
 * in a register and reads +0 / +0x3A from it. */
int DynamicsConductor::getStartingFormat()
{
    return rules.getLaunchSpeed() == 0;
}

// FUNC 0x001F2B98 DynamicsConductor::getCountDown
int DynamicsConductor::getCountDown()
{
    return org->countDown;
}

// FUNC 0x001F2BA8 DynamicsConductor::getLapLength
/* The course length: the runway's lapLength (RaceCourse::getLapLength). */
f32 DynamicsConductor::getLapLength()
{
    return course->getLapLength();
}

// FUNC 0x001F2BD0 DynamicsConductor::getGateCount
/* Slot 32, the checkpoints of a lap: the runway's markers of key 0 plus
 * one (the line), at most 4. */
int DynamicsConductor::getGateCount()
{
    int count = runway_marker_count(course->data->runway, 0) + 1;

    return (count < 5) ? count : 4;
}

// FUNC 0x001F2C10 DynamicsConductor::getGatePosition
/* Slot 33: the course position of checkpoint `gate' (the runway's marker
 * value of key 0); -1 past getGateCount() - 2 (the last one is the line). */
f32 DynamicsConductor::getGatePosition(int gate)
{
    if (getGateCount() - 2 < gate)
        return -1.0f;
    return runway_marker_value(course->data->runway, gate, 0);
}

// FUNC 0x001F2C90 DynamicsConductor::virtual_30
/* Slot 30: the finish line's course position (the runway's unk_1C; 0:
 * none). */
f32 DynamicsConductor::virtual_30()
{
    return course->data->runway->unk_1C;
}

// FUNC 0x001F2CB0 DynamicsConductor::hasFinishLine
/* 1 when the course has a separate finish line (slot 30 != 0); the lap
 * counter (countLap, T307) asks. */
int DynamicsConductor::hasFinishLine()
{
    return virtual_30() != 0.0f;
}

// FUNC 0x001F2CF8 DynamicsConductor::linePointWorld
/* No callers.  The point of enemy line `kind' (lines_select) at course
 * distance dist as a world position: pos = (x, height, -y), the course axes
 * being (x, z, -y); the pit lane (line 4) through line_point_at_linear.
 * Returns what the line query returns, -1 without the line: retail keeps
 * $v0 live at the epilogue (the -1 of the test is the result there; a void
 * function would let reorg fill the first branch's slot from below). */
int DynamicsConductor::linePointWorld(int kind, int hint, f32 *pos, f32 dist)
{
    CourseLines *cl = &lines;
    int line = lines_select(cl, kind);
    int ok;

    if (line == -1)
        return -1;
    if (line != LINE_PIT_LANE)
        ok = line_point_at(cl, line, hint, dist, &pos[0], &pos[2], &pos[1]);
    else
        ok = line_point_at_linear(cl, LINE_PIT_LANE, hint, &pos[0], &pos[2], &pos[1], dist);
    pos[2] = -pos[2];
    return ok;
}

// FUNC 0x001F2DB0 DynamicsConductor::racingLinePoint
/* The point `dist' m along the racing line (clamped to +-(lap length -
 * 20)): its sector, x, y, height and heading.  Returns 0 when there is no
 * racing line, else 1 (dist >= 0) or -1.  DynCar_initialize's start types
 * 1..5 (dist = startArg). */
int DynamicsConductor::racingLinePoint(int *sector, f32 *x, f32 *y, f32 *height, f32 *angle, int dist)
{
    f32 limit = getLapLength() - 20.0f;
    f32 d = (f32)dist;

    if (d < -limit)
        d = -limit;
    if (limit < d)
        d = limit;
    if (lines_get_point(&lines, LINE_RACING, sector, x, y, height, angle, 0, d) != 0)
        return (dist < 0) ? -1 : 1;
    return 0;
}

// FUNC 0x001F2E98 DynamicsConductor::hasPitLane
/* The course has a pit lane (lines 3 and 4). */
int DynamicsConductor::hasPitLane()
{
    return lines.hasPitLane();
}

// FUNC 0x001F2EA8 DynamicsConductor::pitSlotNode
/* The pit lane node of pit slot `slot' (DynCar_initialize). */
int DynamicsConductor::pitSlotNode(int slot)
{
    return pit_slot_node(&lines, slot);
}

// FUNC 0x001F2EC8 DynamicsConductor::pitGateDistance
/* The course distance of pit lane node i (i < 0 from the end): a pit
 * gate's position. */
f32 DynamicsConductor::pitGateDistance(int i)
{
    return pit_node_dist_wrapped(&lines, i);
}

// FUNC 0x001F2EE8 DynamicsConductor::pitBoxDistance
/* The course distance of pit lane node i: a pit box's position. */
f32 DynamicsConductor::pitBoxDistance(int i)
{
    return pit_node_dist(&lines, i);
}

// FUNC 0x001F2F08 DynamicsConductor::pitNodeAngle
/* The heading of pit lane node i, rad. */
f32 DynamicsConductor::pitNodeAngle(int i)
{
    return pit_node_angle(&lines, i);
}

// FUNC 0x001F2F28 DynamicsConductor::updateCourseKind
void DynamicsConductor::updateCourseKind()
{
    RaceOrganization *o = org;

    courseKind = o->raceDB->getCourseKind(o->courseCode);
}

// FUNC 0x001F2F68 DynamicsConductor::getCourseKind
int DynamicsConductor::getCourseKind()
{
    if (courseKind == COURSE_KIND_NONE)
        return 0;
    return courseKind;
}

// FUNC 0x001F2F90 DynamicsConductor::isDirtCourse
/* A dirt course. */
int DynamicsConductor::isDirtCourse()
{
    return getCourseKind() == COURSE_KIND_DIRT;
}

// FUNC 0x001F2FB8 DynamicsConductor::isCourseKind1
int DynamicsConductor::isCourseKind1()
{
    return getCourseKind() == COURSE_KIND_1;
}

// FUNC 0x001F2FE0 DynamicsConductor::virtual_20
/* Slot 20, the race-wide set-up (initialize calls it with 1 first): the
 * surface and sound tables, the slow car boost (slot 10) and the tyre wear
 * (slot 9, not on a dirt course) from the rules, the input tuning, the
 * clock and the checkpoint table, replaySpeed = 1. */
void DynamicsConductor::virtual_20(int a)
{
    int boost, tyreWear;

    surface_tables_init(0);
    CourseEnv_initSurfaceSmoke(course);
    tyre_sound_init();
    brake_heat_setup();
    boost = SetSlowCarBoostParameters(&rules);
    tyreWear = SetTireWearParameters(&rules);
    dc_setup_slow_car_boost(this, boost);
    if (isDirtCourse() != 0)
        tyreWear = 0;
    wear.setup(&rules, tyreWear, a);                            /* a: not read */
    input_tuning_init(a);                                       /* not read */
    clearGateTimes();
    replaySpeed = 1.0f;
}

// FUNC 0x001F30D8 DynamicsConductor::virtual_21
/* Slot 21, the second half of the set-up (initialize calls it with 1
 * last; a is not read): the order and the ranks (slots 1, 2), the pair
 * tables, the gaps, the pair states, the collision boxes, the race result. */
void DynamicsConductor::virtual_21(int a)
{
    virtual_1();
    virtual_2();
    dc_clear_slip(this);
    dc_clear_near(this);
    clearGapTable();
    updateGapTable();
    pairs.reset();
    conductor_store_car_boxes(this);
    boxes_init(&boxes, this, course);
    org->result.reset();
}

// FUNC 0x001F3188 DynamicsConductor::getEntrantInfoRace
/* Slot 23 of Battle2P, BattleMP, FreePractice, SinglePlayer.  A car with an
 * enabled input is the pad's (DRIVER_AUTO in mode 3, the computer's in the
 * demo); a car without one the computer's.  Only the pad's car gets the
 * entry's tyre flag and transmission (both read before the first store:
 * retail loads transmission ahead of the *tyreFlag store, which int fields
 * and int pointers keep in order otherwise; RaceEntryCar's transmission may
 * have had another type, see getEntrantInfoTrial). */
void DynamicsConductor::getEntrantInfoRace(int car, int *tyreFlag, int *transMode, int *driver,
                                   int *noController)
{
    RaceEntryCar *e = org->entries->cars[car];

    if (e->input->enabled != 0) {
        *driver = DRIVER_PAD;
        if (mode == DC_MODE_3)
            *driver = DRIVER_AUTO;
        else if (mode == DC_MODE_DEMO)
            *driver = DRIVER_COMPUTER;
    } else {
        *driver = DRIVER_COMPUTER;
    }
    if (*driver == DRIVER_PAD) {
        int tyre = e->tyre;
        int trans = e->transmission;
        *tyreFlag = tyre;
        *transMode = trans;
    } else {
        *tyreFlag = 0;
        *transMode = 0;
    }
    *noController = 0;
}

// FUNC 0x001F3218 DynamicsConductor::getEntrantInfoTrial
/* Slot 23 of MachineTest, License, FreeRun, TimeTrial: the entry's tyre flag
 * and transmission; a car without an enabled input, and every car in modes
 * 2 / 3, is DRIVER_AUTO; car 1 (the ghost) has no controller.
 * 95.3%: enabled is read before the *transMode store (retail stores it in
 * the branch's delay slot).  Left: retail also loads transmission ahead of
 * the *tyreFlag store, which an int field cannot pass (a bool
 * transmission in a shadow T121 made getEntrantInfoRace exact without its
 * locals but did not move this load; locals and store orders: 75.8..89.1%),
 * and enabled gets $v0 instead of $v1. */
void DynamicsConductor::getEntrantInfoTrial(int car, int *tyreFlag, int *transMode, int *driver,
                                   int *noController)
{
    RaceEntryCar *e = org->entries->cars[car];
    int enabled;

    *tyreFlag = e->tyre;
    enabled = e->input->enabled;
    *transMode = e->transmission;
    if (enabled == 0) {
        *driver = DRIVER_AUTO;
    } else {
        *driver = DRIVER_PAD;
        if (mode == DC_MODE_3)
            *driver = DRIVER_AUTO;
        else if (mode == DC_MODE_DEMO)
            *driver = DRIVER_AUTO;
    }
    *noController = 0;
    if (car == 1)
        *noController = car;
}

// FUNC 0x001F3298 DynamicsConductor::getEachEntrantInfo_
/* Slot 23, the set-up of car `car' (initialize): *tyreFlag goes to
 * SetupCarSpec, the others to DynCar_initialize.  The base: a computer car
 * without a controller, with the entry's tyre flag. */
void DynamicsConductor::getEachEntrantInfo_(int car, int *tyreFlag, int *transMode, int *driver,
                                            int *noController)
{
    *tyreFlag = org->entries->cars[car]->tyre;
    *transMode = 0;
    *driver = DRIVER_COMPUTER;
    *noController = 1;
}

// FUNC 0x001F32D0 DynamicsConductor::getRuleStartArg
int DynamicsConductor::getRuleStartArg()
{
    return rules.getLaunchPoint();
}

// FUNC 0x001F32E0 DynamicsConductor::getRuleStartSpeed
int DynamicsConductor::getRuleStartSpeed()
{
    return rules.getLaunchSpeed();
}

// FUNC 0x001F32F0 DynamicsConductor::startAtLaunchPoint
/* Start type 1 at the rules' launch point, if it is not 0 (License slot
 * 24; speedKmh is not read). */
int DynamicsConductor::startAtLaunchPoint(int car, int *startType, int *startArg, int *speedKmh)
{
    int point = getRuleStartArg();

    if (point != 0) {
        *startType = START_AT_POINT;
        *startArg = point;
        return 1;
    }
    return 0;
}

// FUNC 0x001F3340 DynamicsConductor::startWithLaunchSpeed
/* The rules' launch speed, if any (the start type is left). */
int DynamicsConductor::startWithLaunchSpeed(int car, int *startType, int *startArg, int *speedKmh)
{
    int speed = getRuleStartSpeed();

    if (speed > 0) {
        *speedKmh = speed;
        return 1;
    }
    return 0;
}

// FUNC 0x001F3378 DynamicsConductor::getSpecialStart
/* Slot 24, the start of car `car' (initialize: startType / startArg /
 * speedKmh of DynCar_initialize, all 0 before the call). */
int DynamicsConductor::getSpecialStart(int car, int *startType, int *startArg, int *speedKmh)
{
    return startWithLaunchSpeed(car, startType, startArg, speedKmh);
}

// FUNC 0x001F3398 DynamicsConductor::rollingStart
/* Start type 2: a negative launch point together with a launch speed. */
int DynamicsConductor::rollingStart(int car, int *startType, int *startArg, int *speedKmh)
{
    if (getRuleStartArg() < 0 && getRuleStartSpeed() > 0) {
        *startType = START_ROLLING;
        *startArg = getRuleStartArg();
        *speedKmh = getRuleStartSpeed();
        return 1;
    }
    return 0;
}

// FUNC 0x001F3420 DynamicsConductor::courseStart
/* Start type 3 from the course drive parameters (record 0 of the course
 * kind): startArg x -10 m and startSpeed x 5 km/h, when both are set. */
int DynamicsConductor::courseStart(int car, int *startType, int *startArg, int *speedKmh)
{
    CourseDriveParam *p = getDriveParam(this, getCourseKind(), 0);
    int point = p->startArg * -10;
    int speed = p->startSpeed * 5;

    if (point < 0 && speed > 0) {
        *startType = START_COURSE;
        *startArg = point;
        *speedKmh = speed;
        return 1;
    }
    return 0;
}

// FUNC 0x001F34B8 DynamicsConductor::virtual_25
/* Slot 25: start type 2 or 3 if they apply, else type 4: 5 m before the
 * last tight corner entry (radius <= 350 m) of the racing line within 400 m
 * before the line (200 m / 300 m on a dirt course). */
void DynamicsConductor::virtual_25(int car, int *startType, int *startArg, int *speedKmh)
{
    f32 before, maxRadius;

    if (rollingStart(car, startType, startArg, speedKmh) != 0)
        return;
    if (courseStart(car, startType, startArg, speedKmh) != 0)
        return;
    *startType = START_CORNER;
    before = 400.0f;
    maxRadius = 350.0f;
    switch (getCourseKind()) {              /* kinds 0 and 1 named: retail's case tree */
    case 0:                                 /* tests 1, < 2, 2 (as getDriveParam's) */
    case COURSE_KIND_1:
        break;
    case COURSE_KIND_DIRT:
        before = 300.0f;
        maxRadius = 200.0f;
        break;
    }
    *startArg = (int)lines_corner_entry_before(&lines, -before, maxRadius) - 5;
}

// FUNC 0x001F35B8 DynamicsConductor::attach
/* RaceBase::virtual_8: the organization and the rules of its race; in a
 * race preview or the background race there is no time limit and no lap
 * count. */
void DynamicsConductor::attach(RaceOrganization *o)
{
    org = o;
    rules.setFromRace(o->raceDB);
    if (o->racePreviewMode != 0 || o->backgroundMode != 0) {
        rules.laps = 0;
        rules.timeLimit = 0;
    }
}

// FUNC 0x001F3620 DynamicsConductor::SetTireWearParameters
/* Slot 9: 0, no tyre wear. */
int DynamicsConductor::SetTireWearParameters(RaceRuleOptions *o)
{
    return 0;
}

// FUNC 0x001F3628 DynamicsConductor::initialize
/* Slot 22 (RaceBase::virtual_9, with the race's replayMode).  The course,
 * the course kind, the pair states, the enemy lines, the race (slot 20),
 * then every car: car_init, slot 23, its CarSpec, its grid record (by the
 * entry's order; the heading is always record 0's), slot 24,
 * DynCar_initialize, the gearbox flag, the trip, the display values; then
 * slot 21.
 * 80.1%: retail keeps the grid quadword in $v0 and takes its lanes from
 * the register (mtc1; pcpyud + mtc1; dsra32 + mtc1) into pos at sp+0x20,
 * so its frame has no slot for it and every local below spec sits 0x10
 * lower.  egcs puts this union (and a union with a 16-aligned struct, or
 * {u128, f32}) in memory; *(f32 *)&q, which would take the ADDRESSOF path,
 * is an internal compiler error in both 991111b and 991111-01.  pos / rot
 * are arrays in memory as in retail (x, y, z at sp+0x20, the heading at
 * +0x38; with scalars egcs used $f20..$f23). */
void DynamicsConductor::initialize(int newMode)
{
    RaceOrganization *o = org;
    int i;

    mode = newMode;
    course = o->course;
    updateCourseKind();
    finishTimer = 0;
    pairs.setOwner(this);
    lines_init(&lines, course);
    virtual_20(1);
    for (i = 0; i < carCount; i++) {
        RaceEntryCar *e = o->entries->cars[i];
        CarEquipments *equip = &e->info;
        f32 pos[4];                         /* the start position */
        f32 rot[4];                         /* [2]: the heading */
        CarSpec spec;
        int tyreFlag, transMode, driver, noController;
        int startType, startArg, speedKmh;
        union {
            u128 q;
            f32  f[4];
        } grid;                             /* the grid record: x, y (up), z (lq) */
        RaceCarState *car;

        car_init(getCar(i), this, o);
        getEachEntrantInfo_(i, &tyreFlag, &transMode, &driver, &noController);
        SetupCarSpec(&spec, equip, tyreFlag, getCourseKind(), 0,
                     "DynamicsConductor::initialize()");
        grid.q = o->course->getGridPosition(e->order);
        pos[0] = grid.f[0];
        pos[1] = -grid.f[2];
        pos[2] = grid.f[1];
        rot[2] = o->course->getGridAngle(0);   /* record 0, not e->order (as in the code) */
        startType = START_GRID;
        startArg = 0;
        speedKmh = 0;
        getSpecialStart(i, &startType, &startArg, &speedKmh);
        DynCar_initialize(getCar(i), &spec, -1, driver, transMode, noController, i, startType,
                          pos[0], pos[1], pos[2], rot[2], startArg, speedKmh, &e->condition);
        car = getCar(i);
        car->gearWhineOn = (equip->getGearPart() != 0x13);  /* not the normal gearbox */
        car->unk_16D1 = 1;
        getCar(i)->st.trip = 0;
        car_save_prev(getCar(i));
        car_view_update(getCar(i));
    }
    virtual_21(1);
}

// FUNC 0x001F3918 DynamicsConductor::collideAllCars
/* Slot 11 (processCollision_) of Battle2P, BattleMP, FreePractice,
 * SinglePlayer: every car against the walls and the other cars. */
void DynamicsConductor::collideAllCars()
{
    conductor_store_car_boxes(this);
    boxes_step(&boxes);
    conductor_load_car_boxes(this);
}

// FUNC 0x001F3950 DynamicsConductor::collideWalls
/* Slot 11 of the base (first 0) and update in a replay (first 1): each car
 * from `first' on alone against the walls. */
void DynamicsConductor::collideWalls(int first)
{
    int i;

    conductor_store_car_boxes(this);
    for (i = first; i < carCount; i++)
        boxes_step_car(&boxes, i);
    conductor_load_car_boxes(this);
}

// FUNC 0x001F39D8 DynamicsConductor::update
/* One frame of the dynamics (RaceBase::virtual_29, ghost_restart).  In a
 * replay (replay != 0) car 0 is not stepped and the slots of the live race
 * are skipped.  With mode != 0 the cars step only when one of their step
 * counters (interpBias) runs out: each loses replaySpeed a frame and gains
 * 1 when it goes below 0 (slow motion).  After the per-car passes, slot 3
 * (the slow car boost) is applied to pairs of cars as slot 31 chooses,
 * around the leading car that is not a computer car (car 0 if there is
 * none): 0 outwards from it along the race order, each result the scale of
 * the next pair; 1 it with every other car; 2 each car with its neighbour
 * towards it.  The finish timer and the pair states run every frame.  The
 * outer loop runs once.
 * 96.9%: the step test sits after the slow-motion loop only, the mode-0
 * path goes straight on (retail emits no step = 1: a step flag set in
 * both arms gave 89.7..93.7%, an inline helper `mode == 0 || step()' the
 * right blocks but other registers); first is set in two statements (as
 * one store-flag loop.c hoists it to the prologue); t is loaded, then
 * reduced (retail loads interpBias into $f20); the case-2 / case-0 ranks
 * are block locals.  Left: retail keeps lastRef in $fp, this + 0x8000 in
 * $s2 and DRIVER_COMPUTER in $s5, and tests lastRef == -1 with beql where
 * egcs if-converts (nor / movz); tried lastRef < 0 (97.3%, not retail's
 * compare), ref == -1, if / continue / order[i] forms of the search. */
void DynamicsConductor::update(int replay)
{
    int pass;

    for (pass = 0; pass < 1; pass++) {
        int step;
        int i, n, first;
        int lastRef, ref;
        f32 scale;

        if (mode != DC_MODE_RACE) {
            step = 0;
            for (i = 0; i < carCount; i++) {
                f32 t = getCar(i)->interpBias;

                t -= replaySpeed;
                if (t < 0.0f) {
                    step = 1;
                    t += 1.0f;
                }
                getCar(i)->interpBias = t;
            }
            if (step == 0)
                goto done;
        }
        n = carCount;
        first = 0;
        if (replay != 0)
            first = 1;
        for (i = first; i < n; i++)
            car_save_prev(getCar(i));
        for (i = first; i < n; i++)
            car_pre_step(getCar(i));
        for (i = first; i < n; i++)
            car_input_update(getCar(i));
        for (i = first; i < n; i++)
            car_wheel_steer_angles(getCar(i));
        for (i = first; i < n; i++)
            car_tyre_slip_ratios(getCar(i));
        for (i = first; i < n; i++)
            car_assists(getCar(i));
        for (i = first; i < n; i++)
            car_drivetrain_step(getCar(i));
        for (i = first; i < n; i++)
            car_brake_heat(getCar(i));
        for (i = first; i < n; i++)
            car_crash_sound_clear(getCar(i));
        if (replay == 0)
            processCollision_();
        else
            collideWalls(1);
        if (replay == 0)
            dc_car_pairs(this);
        for (i = first; i < n; i++)
            car_step(getCar(i));
        for (i = first; i < n; i++)
            car_tyre_wear(getCar(i));
        for (i = first; i < n; i++)
            car_tyre_check(getCar(i));
        for (i = first; i < n; i++)
            car_wheel_shake(getCar(i));
        if (replay == 0)
            virtual_12();
        for (i = first; i < n; i++)
            updateCar(getOrder(i));
        if (replay == 0) {
            ReportTimeDifference();
            virtual_13();
            updateGapTable();
            virtual_2();
        }

        /* the first and the last car in race order that is not a
         * computer car */
        lastRef = -1;
        ref = -1;
        for (i = 0; i < carCount; i++) {
            int c = order[i];
            if (getCar(c)->st.driver != DRIVER_COMPUTER) {
                lastRef = c;
                if (ref == -1)
                    ref = c;
            }
        }
        if (lastRef == -1)
            ref = 0;

        switch (getSlowCarBoostType_()) {
        case 1:
            for (i = 0; i < carCount; i++) {
                if (i == ref)
                    continue;
                virtual_3(&getCar(ref)->st, &getCar(i)->st, 1.0f);
            }
            break;
        case 2: {
            int rank = getCar(ref)->st.rank;

            for (i = 0; i < rank - 1; i++)
                virtual_3(&getCar(order[i + 1])->st, &getCar(order[i])->st, 1.0f);
            for (i = rank; i < carCount; i++)
                virtual_3(&getCar(order[i - 1])->st, &getCar(order[i])->st, 1.0f);
            break;
        }
        case 0: {
            int pos;                        /* the reference car's place, 0-based */

            scale = 1.0f;
            pos = getCar(ref)->st.rank - 1;
            for (i = pos; i > 0; i--)
                scale = virtual_3(&getCar(order[i])->st, &getCar(order[i - 1])->st, scale);
            scale = 1.0f;
            for (i = pos; i < carCount - 1; i++)
                scale = virtual_3(&getCar(order[i])->st, &getCar(order[i + 1])->st, scale);
            break;
        }
        }

        for (i = first; i < n; i++)
            pad_vibration_send(getCar(i));
    done:
        tickFinishTimer();
        pairs.update();
    }
}

// FUNC 0x001F42D8 DynamicsConductor::virtual_26
void DynamicsConductor::virtual_26()
{
}

// FUNC 0x001F42E0 CarBox::CarBox
/* Declared in T376: the members v[8] (BoxVertex) and e[8] (BoxEdge) have
 * empty inline constructors, which egcs builds in two element loops (7 down
 * to -1) with nothing in them.  Defined after DynamicsConductor's
 * constructor, so that it is not inlined there (i4). */
CarBox::CarBox()
{
}
