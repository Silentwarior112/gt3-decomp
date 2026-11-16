/* T125_race_organization.cpp - RaceOrganization (0x0017A038..0x0017B458 and
 * its typeinfo function) and RaceResult::reset.
 *
 * 0x0017B050..0x0017B450 have no callers: they are the out-of-line copies
 * of RaceOrganization's inline members (Lupo 2 has them all as linkonce
 * copies, in the reverse order; its names: setPosition, eventCount,
 * eventPut, eventGet, eventCheck, ..., getEntry, the arcade / split battle
 * option getters).  They are written here as ordinary members in address
 * order, which egcs compiles to the same code.  (The Lupo 2 map's
 * RaceEntryBase::getRankEnd / getRankTop / getCount at 0x0017B3B0 /
 * 0x0017B3C0 / 0x0017B3F0 are code matches: these read RaceOrganization's
 * entries and option words.)
 *
 * RaceOrganization is the race-wide state object race_organization_create
 * (T123) builds from a RaceInformation: the race option block, the entry
 * list (RaceEntryBase), the race database and its event record, the course,
 * the start countdown and the time limit, the event queue and the race-wide
 * lap record.  RaceBase (T129) points at it (RaceBase::org) and it points
 * back (race).  Almost every function here is reached from other units.
 *
 * Data: retail T125's .rodata (0x00327630..0x003279A0) is "bad_alloc", the
 * jump table of updateStartAndTimer, the vtable, then the type names of the
 * classes the original saw, in the reverse of their definition order:
 * ResultTermBar, ResultTitleBar, ReplayPauseMenu, PauseMenu,
 * RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton, OperatorList,
 * List / OperatorHolder / Cell<OperatorHolder>, RaceMiniMap, the three
 * meters, RaceMeterBase, RaceCameraBase, RaceOrganization, UnitArena<64>,
 * RaceValueDisplayBase, RaceDisplayBase, the garage lists, Operator.  So:
 * <new>, the GameStatus (the garage), RaceDisplay (T154 reads T157,
 * T153 with UnitArena<64>'s name, this file's header, T383, the meters,
 * then defines RaceMiniMap), then the menu classes in retail's order (T216,
 * T198, T199, T169, T170, T200: the race units' order, u001's notes), and
 * the rest.  The "" of T157's inline RaceMessageDisplay::clear is retail
 * T125's .sdata (0x00351DE8).  Both are unreferenced: anchors.
 */
// DATA_START 0x00327630
// DATA_START 0x00351DE8
#include <new>                              /* "bad_alloc" (0x00327630) */
/* The dummy BGM (T175), declared while RaceBGMBase is still incomplete
 * (before T175_race_bgm_base.h): egcs then leaves the 8-byte object out of
 * small data, and getBGM (-G8) builds its address with lui / addiu as
 * retail does, not $gp-relative.  T175_race_bgm_base.h declares it again. */
class RaceBGMBase;
extern RaceBGMBase g_003521D0;              /* 0x003521D0 */
#include "game/save/T429_game_status.h"          /* (seen: the garage) GameStatus::getGameOptions */
#include "game/race/display/T154_race_display.h"         /* (seen) T125_race_organization.h is read there */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen) */
#include "game/race/pause/T169_race_pause.h"           /* (seen) */
#include "game/race/pause/T170_replay_pause.h"         /* (seen) */
#include "game/menu/records/T200_result_board.h"         /* (seen) */
#include "game/race/modes/T134_race_arcade.h"          /* RaceArcadeSingle (isArcadeSingle) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, RaceResult */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime */
#include "game/race/core/T129_race_base.h"            /* RaceBase */
#include "game/race/core/T153_race_course.h"          /* RaceCourse, g_002BB5B8 (the course arena) */
#include "game/sound/T175_race_bgm_base.h"        /* RaceBGMBase */
#include "game/course/T179_course_data.h"          /* F_001A96B8 */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord */
#include "game/race/core/race_organization_internal.h"                           /* START_ACTION_*, BGM_*, ControlRecordSlot */
#include "game/enhanced.h"                  /* ENH_RACE_TIME_LIMIT_GRACE */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_user */
#endif

/* ======================================================================== */
/* allocation, construction, destruction                                    */
/* ======================================================================== */

// FUNC 0x0017A038 RaceOrganization::operator new
//   Class-specific allocation from the race pool g_002B3BF0, 16-byte aligned.
void *RaceOrganization::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x0017A060 RaceOrganization::operator delete
//   The matching free; the size (0x8C8 from the destructor) is ignored.
void RaceOrganization::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x0017A088 RaceOrganization::RaceOrganization
//   The vptr, then the members eventQueue (RaceEventQueue::RaceEventQueue;
//   ~RaceEventQueue in the cleanup), speedRecord0 / speedRecord
//   (SpeedRecord::SpeedRecord) and lapRecord (LapRecord::LapRecord); the body
//   clears the pointers and flags.  Not cleared: the option block, raceCode,
//   courseCode, result, startFlag, startAction and loggerSector.
RaceOrganization::RaceOrganization()
{
    entries = 0;
    timerActive = 1;
    raceDB = 0;
    course = 0;
    race = 0;
    startingCount = 0;
    countDown = 0;
    timeLimit = 0;
    timeRemain = 0;
    backgroundMode = 0;
    loggerMode = 0;
    loggerCreating = 0;
    racePreviewMode = 0;
    demonstration = 0;

    /* implicit: RaceEventQueue, SpeedRecord, LapRecord (the members'
     * constructors), ~RaceEventQueue (the cleanup on a throw) */
}

// FUNC 0x0017A150 RaceOrganization::~RaceOrganization
//   Slot 0 of the vtable.  Deletes the database, the entry list and the
//   course; the event queue member is destroyed afterwards (flags 2).  When
//   flags & 1 the object is freed with operator delete(this, 0x8C8).
RaceOrganization::~RaceOrganization()
{
    /* implicit: ~RaceEventQueue (the base / member destructors) */
    releaseRaceDB();
    releaseEntries();
    releaseCourse();
}

// FUNC 0x0017A1C0 RaceOrganization::setLoggerMode
//   The entry list's slot 4 (RaceSolitaireEntry, T139: the one-car view of
//   the list on / off) with the mode, then loggerMode.  Callers: logger_run,
//   logger_main.
void RaceOrganization::setLoggerMode(int mode)
{
    entries->virtual_4(mode);
    loggerMode = mode;
}

// FUNC 0x0017A208 RaceOrganization::isArcadeSingle
//   dynamic_cast<RaceArcadeSingle *>(race) != 0 (null-safe).  Unreferenced.
bool RaceOrganization::isArcadeSingle()
{
    return dynamic_cast<RaceArcadeSingle *>(race) != 0;
}

// FUNC 0x0017A260 RaceOrganization::setRaceCode
//   The 64-bit race code, also selected in the database (RaceDataBase::
//   select).  Caller: race_organization_create.
void RaceOrganization::setRaceCode(u64 code)
{
    raceCode = code;
    raceDB->select(code);
}

// FUNC 0x0017A288 RaceOrganization::releaseRaceDB
//   delete raceDB (virtual destructor, flags 3), then the pointer cleared.
void RaceOrganization::releaseRaceDB()
{
    delete raceDB;
    raceDB = 0;
}

// FUNC 0x0017A2D0 RaceOrganization::setRaceDB
void RaceOrganization::setRaceDB(RaceDataBase *db)
{
    releaseRaceDB();
    raceDB = db;
}

// FUNC 0x0017A308 RaceOrganization::releaseEntries
void RaceOrganization::releaseEntries()
{
    delete entries;
    entries = 0;
}

// FUNC 0x0017A350 RaceOrganization::unloadModels
//   RaceEntryCar::unloadModel on car `car', or on every car when `car' is
//   negative; nothing when it is past the end (signed compares).  Callers:
//   run_free_run, run_machine_test, run_gtmode_race, run_championship.
void RaceOrganization::unloadModels(int car)
{
    RaceEntryBase *list = entries;
    int count = list->count;
    if (car < count) {
        if (car >= 0) {
            list->cars[car]->unloadModel();
        } else {
            for (int i = 0; i < count; i++)
                entries->cars[i]->unloadModel();
        }
    }
}

// FUNC 0x0017A3F0 RaceOrganization::setEntries
void RaceOrganization::setEntries(RaceEntryBase *e)
{
    releaseEntries();
    entries = e;
}

// FUNC 0x0017A428 RaceOrganization::releaseCourse
void RaceOrganization::releaseCourse()
{
    delete course;
    course = 0;
}

// FUNC 0x0017A470 RaceOrganization::setCourse
void RaceOrganization::setCourse(RaceCourse *c)
{
    releaseCourse();
    course = c;
}

// FUNC 0x0017A4A8 RaceOrganization::unloadCourseData
//   Unloads the course data (RaceCourse::unload, when there is a course) and
//   returns the course arena they come from (g_002BB5B8).  Callers:
//   race_memcard, run_split_battle, setting_menu.
_UnitArenaBase *RaceOrganization::unloadCourseData()
{
    if (course)
        course->unload();
    return &g_002BB5B8;
}

// FUNC 0x0017A4D8 RaceOrganization::F_0017A4D8
//   The number of the course runway's markers of key 0.  Unreferenced.
int RaceOrganization::F_0017A4D8()
{
    return runway_marker_count(course->data->runway, 0);
}

// FUNC 0x0017A500 RaceOrganization::F_0017A500
//   The idx-th of them.  Unreferenced.
f32 RaceOrganization::F_0017A500(int idx)
{
    return runway_marker_value(course->data->runway, idx, 0);
}

// FUNC 0x0017A528 RaceOrganization::getLoggerSectorCount
//   The same for key 1 (the data logger's sectors).  Callers:
//   RaceSolitaire::virtual_9, dc_section_count.
int RaceOrganization::getLoggerSectorCount()
{
    return runway_marker_count(course->data->runway, 1);
}

// FUNC 0x0017A550 RaceOrganization::getLoggerSectorPosition
//   The idx-th marker of key 1.  Callers: LoggerRaceGraph::setup,
//   dc_section_value.
f32 RaceOrganization::getLoggerSectorPosition(int idx)
{
    return runway_marker_value(course->data->runway, idx, 1);
}

/* ======================================================================== */
/* the race state                                                           */
/* ======================================================================== */

// FUNC 0x0017A578 RaceOrganization::initRaceState
//   The start of a race (RaceBase::virtual_8): result.reset() (RaceResult::
//   reset), eventQueue.clear(), the start action and the countdown from the
//   race record, a start action pending, the time limit armed (in frames).
//   Matching: the stores in this order, the limit through timeRemain (retail
//   stores startFlag after the countDown byte is read and timeRemain last).
//   The enhanced engine (race_logic.md C2, ENH_RACE_TIME_LIMIT_GRACE): a set
//   limit gets 59 frames more, so the display shows the full limit for its
//   first second (Lupo 2 L2 0x001784EC).
void RaceOrganization::initRaceState()
{
    result.reset();
    eventQueue.clear();
    startAction = raceDB->event->startAction;
    startingCount = raceDB->event->countDown;
    startFlag = 1;
    timeRemain = timeLimit * 60;
#if ENH_RACE_TIME_LIMIT_GRACE
    if (timeLimit != 0)
        timeRemain = timeLimit * 60 + 59;   /* Lupo 2 (L2 0x00178504) */
#endif
    timerActive = (timeRemain != 0);
    loggerSector = 0;
}

// FUNC 0x0017A5E8 RaceOrganization::updateStartAndTimer
//   Per frame (RaceBase::virtual_29).  A pending start action posts its
//   events (RACE_EVENT_COUNTDOWN, RACE_EVENT_BGM with a code), sets the
//   countdown to startingCount seconds and tells the course data
//   (F_001A96B8) the countdown in seconds.  Then the countdown runs
//   (RACE_EVENT_START at 0) or, once it is over, the time limit
//   (RACE_EVENT_TIME_UP on expiry).  Jump table 0x00327640: -1 posts
//   BGM_RACE and falls into 3 (no countdown), 0 keeps the record's count, 1
//   / 2 set 4 / 10 seconds with their music.  The case bodies in retail's
//   block order (0, 1, 2, -1 / 3); the countdown is read back from
//   startingCount after the switch (CSE knows the 0 of the -1 / 3 path).
void RaceOrganization::updateStartAndTimer()
{
    if (startFlag != 0) {
        startFlag = 0;
        switch (startAction) {
        case START_ACTION_DEFAULT:
            if (startingCount != 0)
                eventQueue.post(RACE_EVENT_COUNTDOWN, 0, 0);
            break;
        case START_ACTION_COUNT4:
            startingCount = 4;
            eventQueue.post(RACE_EVENT_COUNTDOWN, 0, 0);
            eventQueue.post(RACE_EVENT_BGM, 0x1B, 0);
            break;
        case START_ACTION_COUNT10:
            startingCount = 10;
            eventQueue.post(RACE_EVENT_COUNTDOWN, 0, 0);
            eventQueue.post(RACE_EVENT_BGM, 0x12, 0);
            break;
        case START_ACTION_NONE:
            eventQueue.post(RACE_EVENT_BGM, BGM_RACE, 0);
            /* fall through */
        case START_ACTION_NO_COUNTDOWN:
            startingCount = 0;
            break;
        }
        RaceCourse *raceCourse = race->org->course;
        countDown = startingCount * 60;
        F_001A96B8(raceCourse->data, raceCourse, (f32)startingCount);
    }

    if (countDown != 0) {
        countDown--;
        if (countDown == 0)
            eventQueue.post(RACE_EVENT_START, 0, 0);
    } else if (timerActive != 0) {
        timeRemain--;
        if (timeRemain == 0) {
            timerActive = 0;
            eventQueue.post(RACE_EVENT_TIME_UP, 0, 0);
        }
    }
}

// FUNC 0x0017A780 RaceOrganization::updateRanksAndEvents
//   entries->sortByRank(); RACE_EVENT_STOP_TIMER (taken) stops the time
//   limit; loggerSector is set while a RACE_EVENT_LOGGER_SECTOR with
//   argument 0 was posted (all of them are taken).  Callers: RaceBase::
//   virtual_9 and virtual_29.
void RaceOrganization::updateRanksAndEvents()
{
    int arg;

    entries->sortByRank();
    if (eventQueue.find(RACE_EVENT_STOP_TIMER, 0, 1))
        timerActive = 0;
    loggerSector = 0;
    while (eventQueue.find(RACE_EVENT_LOGGER_SECTOR, &arg, 1)) {
        if (arg == 0)
            loggerSector = 1;
    }
}

// FUNC 0x0017A810 RaceOrganization::F_0017A810
//   Empty.  Unreferenced.
void RaceOrganization::F_0017A810()
{
}

// FUNC 0x0017A818 RaceOrganization::resetEntryOrder
//   Car i gets order i (an unsigned index).  Callers: the RaceInformation::
//   virtual_11 family.
void RaceOrganization::resetEntryOrder()
{
    RaceEntryBase *list = entries;
    for (u32 i = 0; i < (u32)list->count; i++)
        list->cars[i]->order = i;
}

// FUNC 0x0017A858 RaceOrganization::moveEntryOrder
//   The car of order `from' gets order `to' (clamped to 0..count - 1) and the
//   orders in between move by one, like moving an element of a list (the
//   direction is taken from the unclamped `to').  Callers:
//   RaceSinglePlayerInformation::virtual_11 and friends.  Retail computes
//   `from < to' first, before the from == to test (slt in the entry block),
//   and clamps `to' in place (movz), comparing it unsigned with the count.
void RaceOrganization::moveEntryOrder(int from, int to)
{
    RaceEntryBase *list = entries;
    int forward = from < to;
    if (from == to)
        return;
    if (to < 0)
        to = 0;
    else if ((u32)to >= (u32)list->count)
        to = list->count - 1;
    for (u32 i = 0; i < (u32)list->count; i++) {
        RaceEntryCar *car = list->cars[i];
        int order = car->order;
        if (order == from) {
            order = to;
        } else if (!forward) {
            if (order >= to)
                order += (order < from);
        } else if (from < order) {
            if (!(to < order))
                order = order - 1;
        }
        car->order = order;
    }
}

// FUNC 0x0017A900 RaceOrganization::updateColorIndices
//   RaceEntryCar::updateColorIndex on every car (the count read once).
//   Caller: race_organization_create.
void RaceOrganization::updateColorIndices()
{
    RaceEntryBase *list = entries;
    u32 count = list->count;
    for (u32 i = 0; i < count; i++)
        entries->cars[i]->updateColorIndex();
}

// FUNC 0x0017A970 RaceOrganization::attachInputs
//   For every car: car->input = race->virtual_26(info.inputId); an enabled
//   input gets the car's vibration setting unless the race is a replay, and
//   its record rewinds (Manager::reset).  Caller: RaceBase::virtual_9.
void RaceOrganization::attachInputs(RaceBase *r)
{
    RaceEntryBase *list = entries;
    u32 i = 0;
    if (list->count != 0) {
        do {
            RaceEntryCar *car = entries->cars[i];
            RaceInput *input = r->virtual_26(car->info.inputId);
            car->input = input;
            if (input->enabled != 0) {
                if (r->replayMode == 0)
                    input->vibration = car->vibration;
                input->rec.reset();
            }
            i++;
        } while (i < (u32)list->count);
    }
}

// FUNC 0x0017AA48 RaceOrganization::F_0017AA48
//   Every car's enabled input closes its record (Recorder::stop on
//   input->rec.rec).  Caller: RaceBase::endRace.
//   Not exact (90.7%): retail reloads list->count at the bottom on both
//   paths (the load in the beql slot of the not-enabled path); egcs's PRE
//   keeps the count of the loop entry for the path without the call.  Tried:
//   for / while / do-while, list->getCount(), car / input locals, a Recorder
//   pointer, `if (input->enabled)`; a volatile count stops the PRE but keeps
//   the load out of the delay slot (94.3%, not the original).  The reference
//   compiler gives the same.
void RaceOrganization::F_0017AA48()
{
    RaceEntryBase *list = entries;
    u32 i = 0;
    if (list->count != 0) {
        do {
            RaceInput *input = entries->cars[i]->input;
            if (input->enabled != 0)
                input->rec.rec.stop();
            i++;
        } while (i < (u32)list->count);
    }
}

// FUNC 0x0017AAD8 RaceOrganization::setupGhost
//   Sets up car 1, the ghost (? name: setupGhost).  With flag != 0 nothing
//   happens in a replay or when the ghost option (GameOptions::ghost) is
//   off.  A list of one car first gets a second (createEntry, addEntry); an
//   empty list ends it (a signed switch on the count).  result.beatGhost is
//   cleared.  Unless mode == 2 car 1 takes over car 0 (copyFrom, deferModel
//   = flag).  The ghost's record buffer: mode 0 copies buffer 0 into buffer
//   1 (AutomobileControlRecord::copyBuffer), mode 1 copies slot 2 of buffer
//   1 into its slot 0 (getBufferSlot; a struct copy), mode 2 (a record
//   loaded from an info block) keeps it.  Then the input of INPUT_GHOST is
//   released (hold = 0) and, with flag != 0, car 1 is wired up as in
//   RaceBase::virtual_8 (no model drawn, its state, frozen lap times, the
//   input, the record rewound).  Callers: run_free_run, run_time_trial,
//   run_machine_test, run_license_test, RaceBase::virtual_19 (2, 0),
//   DynamicsConductorFreeRun::virtual_17.  Matching: the options through
//   GameStatus::getGameOptions() (retail adds 0x2FF38 first, then reads
//   +0x28); `case 2: break;' (the decision tree tests 1 first); the new
//   entry made before `entries' is read again for addEntry (in the
//   original probably RaceOrganization's inline createEntry / addEntry);
//   the ghost's state stored before model.state.
void RaceOrganization::setupGhost(int mode, int flag)
{
    if (flag != 0) {
        if (race->replayMode != 0)
            return;
        if (getGameStatus()->getGameOptions()->ghost == 0)
            return;
    }

    switch (entries->count) {
    case 0:
        return;
    case 1: {
        RaceEntryCar *car = entries->createEntry();
        entries->addEntry(car);
        break;
    }
    case 2:
        break;
    }
    result.beatGhost = 0;

    if (mode != 2)
        entries->cars[1]->copyFrom(entries->cars[0], flag);

    switch (mode) {
    case 0:
        AutomobileControlRecord::copyBuffer(1, 0);
        break;
    case 1: {
        ControlRecordSlot *dst = (ControlRecordSlot *)AutomobileControlRecord::getBufferSlot(1, 0);
        ControlRecordSlot *src = (ControlRecordSlot *)AutomobileControlRecord::getBufferSlot(1, 2);
        *dst = *src;
        break;
    }
    default:
        break;
    }

    RaceBase *r = race;
    RaceInput *input = r->virtual_26(INPUT_GHOST);
    input->hold = 0;
    if (flag != 0) {
        RaceEntryCar *ghost = entries->cars[1];
        ghost->modelEnabled = 0;
        RaceCarState *state = r->dc->getCar(1);
        ghost->state = state;
        ghost->model.state = state;
        ghost->lapTime.reset(1);
        ghost->input = input;
        input->rec.reset();
    }
}

// FUNC 0x0017AD28 RaceOrganization::F_0017AD28
//   race->virtual_23() (an empty hook), then car 0's updateGhostLapTime().
//   Callers: run_free_run, run_time_trial, run_machine_test,
//   run_license_test.
void RaceOrganization::F_0017AD28()
{
    race->virtual_23();
    entries->cars[0]->updateGhostLapTime();
}

// FUNC 0x0017AD70 RaceOrganization::getTargetCount
//   5: the number of ranks DynamicsConductor::licenseFinished walks with
//   getTargetTime (u083: ? getTargetCount).
int RaceOrganization::getTargetCount()
{
    return 5;
}

// FUNC 0x0017AD78 RaceOrganization::getTargetTime
//   The race record's time[rank - 1] in ms (minutes, seconds, ms) for rank 1
//   (gold) .. 3 (bronze); 0 for any other rank.  Caller: DynamicsConductor::
//   licenseFinished (it compares the result unsigned).  Each case reads the
//   bytes off the record pointer (retail: 0x1C4 / 0x1C5 / 0x1C6 from it, not
//   from a RaceTime pointer).
int RaceOrganization::getTargetTime(int rank)
{
    switch (rank) {
    case 1:
        return raceDB->event->time[0].min * 60000 + raceDB->event->time[0].sec * 1000 + raceDB->event->time[0].ms;
    case 2:
        return raceDB->event->time[1].min * 60000 + raceDB->event->time[1].sec * 1000 + raceDB->event->time[1].ms;
    case 3:
        return raceDB->event->time[2].min * 60000 + raceDB->event->time[2].sec * 1000 + raceDB->event->time[2].ms;
    default:
        return 0;
    }
}

// FUNC 0x0017AE20 RaceOrganization::getBGM
//   The running race's BGM, or the dummy g_003521D0 without a race.
//   Callers: CameraManager::updateMarkerTime / updateMarkers.
RaceBGMBase *RaceOrganization::getBGM()
{
    if (race == 0)
        return &g_003521D0;
    return race->bgm;
}

// FUNC 0x0017AE40 RaceOrganization::setCourseCode
//   The 64-bit course code, also given to the database (RaceDataBase::
//   setCourse).  Callers: main_arcade, race_organization_create.
void RaceOrganization::setCourseCode(u64 code)
{
    courseCode = code;
    raceDB->setCourse(code);
}

// FUNC 0x0017AE68 RaceOrganization::setEntryVibration
void RaceOrganization::setEntryVibration(int value, int entry)
{
    entries->cars[entry]->vibration = value;
}

// FUNC 0x0017AE88 RaceOrganization::setEntryTransmission
void RaceOrganization::setEntryTransmission(int value, int entry)
{
    entries->cars[entry]->transmission = value;
}

// FUNC 0x0017AEA8 RaceOrganization::recordSectionTime
//   The car's section time (RaceEntryCar::recordSectionTime, which compares
//   it with the car's session best); returns the difference of the car's
//   lap record to the race-wide one.  Caller: dc_post_section_time.
int RaceOrganization::recordSectionTime(int entry, int section, u32 time)
{
    RaceEntryCar *car = entries->cars[entry];
    car->recordSectionTime(section, time);
    return car->lapRecord.sectionDiff(lapRecord, section);
}

// FUNC 0x0017AF10 RaceOrganization::recordLapTime
//   The car's lap total (RaceEntryCar::recordLapTime, its session best
//   updated); the difference to the race-wide record, which takes the car's
//   lap when `update' and it is better.  Caller: dc_post_lap_time.
int RaceOrganization::recordLapTime(int entry, u32 total, int update)
{
    RaceEntryCar *car = entries->cars[entry];
    car->recordLapTime(total, 1);
    int diff = car->lapRecord.totalDiff(lapRecord);
    if (update)
        lapRecord.update(car->lapRecord);
    return diff;
}

// FUNC 0x0017AF98 RaceOrganization::setEntrySteeringAssist
void RaceOrganization::setEntrySteeringAssist(int value, int entry)
{
    entries->cars[entry]->steeringAssist = value;
}

// FUNC 0x0017AFB8 RaceOrganization::setEntryAsm
void RaceOrganization::setEntryAsm(int value, int entry)
{
    entries->cars[entry]->assistAsm = value;
}

// FUNC 0x0017AFD8 RaceOrganization::setEntryTcs
void RaceOrganization::setEntryTcs(int value, int entry)
{
    entries->cars[entry]->assistTcs = (value != 0);
}

// FUNC 0x0017AFF8 RaceOrganization::setEntryInfo
//   The car data of entry `entry' = *src: CarEquipments::operator= on the
//   entry's info (the RaceEntryInformation part, inputId, is not copied).
//   Callers: run_free_run, run_machine_test, run_gtmode_race,
//   run_championship.
void RaceOrganization::setEntryInfo(int entry, const CarEquipments *src)
{
    static_cast<CarEquipments &>(entries->cars[entry]->info) = *src; /* implicit: CarEquipments::operator= */
}

// FUNC 0x0017B030 RaceResult::reset
//   rank -2, time RACE_TIME_INVALID, the flags 0.  Callers:
//   RaceOrganization::initRaceState, DynamicsConductor::
//   recordOneLapGhostControl.
void RaceResult::reset()
{
    rank = -2;
    time = RACE_TIME_INVALID;
    beatGhost = 0;
    lapRecord = 0;
}

// FUNC 0x0017B050 RaceOrganization::setEntryOrder
void RaceOrganization::setEntryOrder(int entry, int order)
{
    entries->cars[entry]->order = order;
}

/* ======================================================================== */
/* forwarders to the event queue (pairs of identical inline functions)      */
/* ======================================================================== */

// FUNC 0x0017B070 RaceOrganization::getEventCount
//   eventQueue.counts[type].
u8 RaceOrganization::getEventCount(int type)
{
    return eventQueue.counts[type];
}

// FUNC 0x0017B080 RaceOrganization::postEvent
//   eventQueue.post (RaceEventQueue::post).
void RaceOrganization::postEvent(int type, int arg, int delay)
{
    eventQueue.post(type, arg, delay);
}

// FUNC 0x0017B0A0 RaceOrganization::postEvent2
//   The same code as postEvent (eventQueue.post).
void RaceOrganization::postEvent2(int type, int arg, int delay)
{
    eventQueue.post(type, arg, delay);
}

// FUNC 0x0017B0C0 RaceOrganization::takeEvent
//   Finds and removes the first event of the type (eventQueue.find(type,
//   arg, 1)).
int RaceOrganization::takeEvent(int type, int *arg)
{
    return eventQueue.find(type, arg, 1);
}

// FUNC 0x0017B0E0 RaceOrganization::takeEvent2
//   The same code as takeEvent (eventQueue.find).
int RaceOrganization::takeEvent2(int type, int *arg)
{
    return eventQueue.find(type, arg, 1);
}

// FUNC 0x0017B100 RaceOrganization::findNthEvent
//   eventQueue.findNth.
int RaceOrganization::findNthEvent(int type, int *arg, int nth)
{
    return eventQueue.findNth(type, arg, nth);
}

// FUNC 0x0017B120 RaceOrganization::findNthEvent2
//   The same code as findNthEvent (eventQueue.findNth).
int RaceOrganization::findNthEvent2(int type, int *arg, int nth)
{
    return eventQueue.findNth(type, arg, nth);
}

// FUNC 0x0017B140 RaceOrganization::findEvent
//   The first event of the type, left in the queue (eventQueue.find(type,
//   arg, 0)).
int RaceOrganization::findEvent(int type, int *arg)
{
    return eventQueue.find(type, arg, 0);
}

// FUNC 0x0017B160 RaceOrganization::findEvent2
//   The same code as findEvent (eventQueue.find).
int RaceOrganization::findEvent2(int type, int *arg)
{
    return eventQueue.find(type, arg, 0);
}

/* ======================================================================== */
/* accessors (Lupo 2 names)                                                 */
/* ======================================================================== */

// FUNC 0x0017B180 RaceOrganization::getEventQueue
RaceEventQueue *RaceOrganization::getEventQueue()
{
    return &eventQueue;
}

// FUNC 0x0017B188 RaceOrganization::getCourseCode
u64 RaceOrganization::getCourseCode()
{
    return courseCode;
}

// FUNC 0x0017B190 RaceOrganization::clearCourseModel
//   RaceCourse::unload, without the NULL test of unloadCourseData.
void RaceOrganization::clearCourseModel()
{
    course->unload();
}

// FUNC 0x0017B1B0 RaceOrganization::getCourse
RaceCourse *RaceOrganization::getCourse()
{
    return course;
}

// FUNC 0x0017B1B8 RaceOrganization::getResult
RaceResult *RaceOrganization::getResult()
{
    return &result;
}

// FUNC 0x0017B1C0 RaceOrganization::getTimeRemain
int RaceOrganization::getTimeRemain()
{
    return timeRemain;
}

// FUNC 0x0017B1C8 RaceOrganization::setTimeLimit
void RaceOrganization::setTimeLimit(int v)
{
    timeLimit = v;
}

// FUNC 0x0017B1D0 RaceOrganization::isTimeLimit
bool RaceOrganization::isTimeLimit()
{
    return timeLimit != 0;
}

// FUNC 0x0017B1E0 RaceOrganization::getCountDown
int RaceOrganization::getCountDown()
{
    return countDown;
}

// FUNC 0x0017B1E8 RaceOrganization::getStartingCount
int RaceOrganization::getStartingCount()
{
    return startingCount;
}

// FUNC 0x0017B1F0 RaceOrganization::setCountDown
//   Sets the starting count (seconds), not the running countdown.
void RaceOrganization::setCountDown(int v)
{
    startingCount = v;
}

// FUNC 0x0017B1F8 RaceOrganization::isCountDown
bool RaceOrganization::isCountDown()
{
    return countDown != 0;
}

// FUNC 0x0017B208 RaceOrganization::clearCountDown
//   startAction = START_ACTION_NONE.
void RaceOrganization::clearCountDown()
{
    startAction = START_ACTION_NONE;
}

// FUNC 0x0017B218 RaceOrganization::clearStartAction
//   No start action pending (startFlag).
void RaceOrganization::clearStartAction()
{
    startFlag = 0;
}

// FUNC 0x0017B220 RaceOrganization::getStartAction
int RaceOrganization::getStartAction()
{
    return startAction;
}

// FUNC 0x0017B228 RaceOrganization::setStartAction
void RaceOrganization::setStartAction(int v)
{
    startAction = v;
}

// FUNC 0x0017B230 RaceOrganization::getRaceCode
u64 RaceOrganization::getRaceCode()
{
    return raceCode;
}

// FUNC 0x0017B238 RaceOrganization::getRace
RaceBase *RaceOrganization::getRace()
{
    return race;
}

// FUNC 0x0017B240 RaceOrganization::setRace
void RaceOrganization::setRace(RaceBase *r)
{
    race = r;
}

// FUNC 0x0017B248 RaceOrganization::getSpeedRecord
SpeedRecord *RaceOrganization::getSpeedRecord()
{
    return &speedRecord;
}

// FUNC 0x0017B250 RaceOrganization::getSpeedRecord0
//   The record at +0x8A4 (no symbol).
SpeedRecord *RaceOrganization::getSpeedRecord0()
{
    return &speedRecord0;
}

// FUNC 0x0017B258 RaceOrganization::setLapRecordTotal
void RaceOrganization::setLapRecordTotal(u32 t)
{
    lapRecord.total = t;
}

// FUNC 0x0017B260 RaceOrganization::setLapRecordSection
void RaceOrganization::setLapRecordSection(int i, u32 t)
{
    lapRecord.section[i] = t;
}

// FUNC 0x0017B270 RaceOrganization::getLapRecordTotal
u32 RaceOrganization::getLapRecordTotal()
{
    return lapRecord.total;
}

// FUNC 0x0017B278 RaceOrganization::getLapRecordSection
u32 RaceOrganization::getLapRecordSection(int i)
{
    return lapRecord.section[i];
}

// FUNC 0x0017B288 RaceOrganization::getLapRecordTotal2
//   The same code as getLapRecordTotal.
u32 RaceOrganization::getLapRecordTotal2()
{
    return lapRecord.total;
}

// FUNC 0x0017B290 RaceOrganization::updateLapRecordTotal
bool RaceOrganization::updateLapRecordTotal(u32 t)
{
    return lapRecord.updateTotal(t);
}

// FUNC 0x0017B2B0 RaceOrganization::setLapRecordTotal2
//   The same code as setLapRecordTotal.
void RaceOrganization::setLapRecordTotal2(u32 t)
{
    lapRecord.total = t;
}

// FUNC 0x0017B2B8 RaceOrganization::getEntryInput
RaceInput *RaceOrganization::getEntryInput(u32 idx)
{
    return entries->cars[idx]->input;
}

// FUNC 0x0017B2D8 RaceOrganization::getLapTime
RaceLapTime *RaceOrganization::getLapTime(u32 idx)
{
    return &entries->cars[idx]->lapTime;
}

// FUNC 0x0017B2F8 RaceOrganization::getEntryCar
RaceEntryCar *RaceOrganization::getEntryCar(u32 idx)
{
    return entries->cars[idx];
}

// FUNC 0x0017B310 RaceOrganization::getEntryCount
int RaceOrganization::getEntryCount()
{
    return entries->count;
}

// FUNC 0x0017B320 RaceOrganization::createEntry
//   entries->createEntry() (RaceEntryBase slot 1).
RaceEntryCar *RaceOrganization::createEntry()
{
    return entries->createEntry();
}

// FUNC 0x0017B350 RaceOrganization::canAddEntry
//   entries->canAddEntry(car) (slot 2).  Lupo 2 calls it inquiry(RaceEntryCar
//   &): retail passes $a1 through untouched (entries in $a2).
int RaceOrganization::canAddEntry(RaceEntryCar *car)
{
    return entries->canAddEntry(car);
}

// FUNC 0x0017B380 RaceOrganization::addEntry
//   entries->addEntry(car) (slot 3).
int RaceOrganization::addEntry(RaceEntryCar *car)
{
    return entries->addEntry(car);
}

// FUNC 0x0017B3B0 RaceOrganization::getEntry
RaceEntryBase *RaceOrganization::getEntry()
{
    return entries;
}

// FUNC 0x0017B3B8 RaceOrganization::getRaceDB
RaceDataBase *RaceOrganization::getRaceDB()
{
    return raceDB;
}

// FUNC 0x0017B3C0 RaceOrganization::getSplitBattleOptionSlowCarBoost
int RaceOrganization::getSplitBattleOptionSlowCarBoost()
{
    return option.splitBattle.boost;
}

// FUNC 0x0017B3C8 RaceOrganization::getSplitBattleOptionHandicap
int RaceOrganization::getSplitBattleOptionHandicap()
{
    return option.splitBattle.handicap;
}

// FUNC 0x0017B3D0 RaceOrganization::getSplitBattleOptionTireDamage
bool RaceOrganization::getSplitBattleOptionTireDamage()
{
    return option.splitBattle.tireDamage;
}

// FUNC 0x0017B3D8 RaceOrganization::getSplitBattleOptionLaps
int RaceOrganization::getSplitBattleOptionLaps()
{
    return option.splitBattle.raceLaps;
}

// FUNC 0x0017B3E0 RaceOrganization::getSplitBattleOption
SplitBattleOption *RaceOrganization::getSplitBattleOption()
{
    return &option.splitBattle;
}

// FUNC 0x0017B3E8 RaceOrganization::getArcadeOptionTireDamage
int RaceOrganization::getArcadeOptionTireDamage()
{
    return option.tireDamage;
}

// FUNC 0x0017B3F0 RaceOrganization::getArcadeOptionLaps
int RaceOrganization::getArcadeOptionLaps()
{
    return option.raceLaps;
}

// FUNC 0x0017B3F8 RaceOrganization::getArcadeOption
//   The option block is the first member: the object's own address.
RaceOption *RaceOrganization::getArcadeOption()
{
    return &option;
}

// FUNC 0x0017B400 RaceOrganization::getRaceOption
RaceOption *RaceOrganization::getRaceOption()
{
    return &option;
}

// FUNC 0x0017B408 RaceOrganization::setDemonstration
void RaceOrganization::setDemonstration(bool v)
{
    demonstration = v;
}

// FUNC 0x0017B410 RaceOrganization::getDemonstration
int RaceOrganization::getDemonstration() const
{
    return demonstration;
}

// FUNC 0x0017B418 RaceOrganization::setRacePreviewMode
void RaceOrganization::setRacePreviewMode(bool v)
{
    racePreviewMode = v;
}

// FUNC 0x0017B420 RaceOrganization::getRacePreviewMode
int RaceOrganization::getRacePreviewMode()
{
    return racePreviewMode;
}

// FUNC 0x0017B428 RaceOrganization::checkLoggerSector
//   Set by updateRanksAndEvents when a RACE_EVENT_LOGGER_SECTOR with
//   argument 0 was posted this frame.
int RaceOrganization::checkLoggerSector()
{
    return loggerSector;
}

// FUNC 0x0017B430 RaceOrganization::setLoggerCreating
void RaceOrganization::setLoggerCreating(bool v)
{
    loggerCreating = v;
}

// FUNC 0x0017B438 RaceOrganization::getLoggerCreating
int RaceOrganization::getLoggerCreating()
{
    return loggerCreating;
}

// FUNC 0x0017B440 RaceOrganization::getLoggerMode
int RaceOrganization::getLoggerMode()
{
    return loggerMode;
}

// FUNC 0x0017B448 RaceOrganization::setBackgroundMode
void RaceOrganization::setBackgroundMode(bool v)
{
    backgroundMode = v;
}

// FUNC 0x0017B450 RaceOrganization::getBackgroundMode
int RaceOrganization::getBackgroundMode()
{
    return backgroundMode;
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0017B458 __tf16RaceOrganization
//   Typeinfo function (vtable entry 0 of RaceOrganization): builds the
//   __user_type_info __ti16RaceOrganization (0x00353380) on the first call
//   (name "16RaceOrganization" at 0x00327840) and returns it.
extern "C" const type_info &__tf16RaceOrganization(void)
{
    if (__ti16RaceOrganization[0] == 0)
        __rtti_user(__ti16RaceOrganization, "16RaceOrganization");
    return *(const type_info *)__ti16RaceOrganization;
}
#endif
