/* T117 - ResultArcade: the result sequence of the arcade race modes
 * (game/menu/records/T117_result_arcade.h; RaceArcade+0x2509C and the other single /
 * split / link battle modes; base of ResultChampionship, ResultLinkBattle,
 * Result2PBattle).  0x00174D50..0x001753C0.
 *
 * A ResultBoard (T200) lists the cars in rank order; a car's row gets its
 * time (or, in a time-limit race, its laps) when it reaches the goal.  Once
 * every car is in, a 15 s timer runs (1/60 s per frame); when it expires or
 * the pad's decide button is pressed, RACE_EVENT_END is posted to the
 * organization's queue (every frame from then on). */
/* Retail's .rodata (0x00325F78..0x00326168): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the vtable, then the type names
 * in the reverse order of the classes' definitions: ResultArcade,
 * ResultTermBar, ResultTitleBar, RaceMenuSelectWindow, RaceMenuWindow,
 * RaceMenuButton, OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * RaceCameraBase, UnitArena<64>, Operator.  (The vtable, 0x00325F88, is
 * referenced by T116's code only: the anchor keeps it in T117's range.) */
// DATA_START 0x00325F78
#include <new>
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* RaceMenuWindow (ResultBoard's title / term bars) */
#include "game/menu/records/T200_result_board.h"         /* ResultBoard */
#include "game/menu/records/T117_result_arcade.h"
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (sortByRank, rankTop), RaceEntryCar */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_* */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, race, eventQueue) */
#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID */
#include "game/race/core/T129_race_base.h"            /* RaceBase::dc */
#include "game/car/physics/T302_car_state.h"            /* car_lap_display */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor::rules (T326 timeLimit) */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_DECIDE */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getName1 / getName2 */
#include "game/menu/records/records_internal.h"                           /* RESULT_BOARD_OPEN_TIME, RESULT_END_DELAY, RESULT_FRAME_TIME */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* index.txt: ResultArcade::virtual_1 (slot 1).  The base's start, the board
 * for the entries' count (opening at 5 s), active = !replay, lapsOnly = the
 * race has a time limit (minutes * 60000 != 0), no car finished, the 15 s
 * timer.  Callers: ResultChampionship / ResultLinkBattle / Result2PBattle::
 * start. */
// FUNC 0x00174D50 ResultArcade::start
void ResultArcade::start(RaceOrganization &org, int replay)
{
    RaceResultBase::start(org, replay);
    int cars = org.entries->count;
    board.setup(cars, RESULT_BOARD_TIMES, RESULT_BOARD_OPEN_TIME);
    active = (replay == 0);
    lapsOnly = (org.race->dc->rules.timeLimit * 60000) != 0;
    for (int i = 0; i < cars; i++)
        finished[i] = 0;
    allFinished = 0;
    timer = RESULT_END_DELAY;
}

/* index.txt: ResultArcade::virtual_2 (slot 2). */
// FUNC 0x00174E18 ResultArcade::cleanup
void ResultArcade::cleanup()
{
    if (active != 0)
        board.stop();
    board.clearRows();
}

/* The cars' icons of the race mode (e.g. RaceArcade+0x16EC0) for the board.
 * Callers: RaceArcadeSingle / RaceSplitBattle / RaceLinkBattle /
 * RaceChampionship::virtual_43. */
// FUNC 0x00174E50 ResultArcade::setDisplays
void ResultArcade::setDisplays(CarIcon *displays)
{
    board.displays = displays;
}

/* index.txt: ResultArcade::virtual_3 (slot 3).  The base's update (the goal
 * events); once the player's car is in: the board refreshed and animated,
 * then waitEnd (virtual) with the pad's decide button pressed this frame
 * (Control::getButtonsOn, slot 5).  Caller: ResultLinkBattle::update. */
// FUNC 0x00174E58 ResultArcade::update
void ResultArcade::update(RaceOrganization &org, Control *pad)
{
    RaceResultBase::update(org, pad);
    if (active != 0 && done != 0) {
        updateBoard(org);
        board.update();
        /* the pad first: retail loads waitEnd's vtable slot after the
         * getButtonsOn call and tests the masked bits with sltu */
        u32 decide = pad->getButtonsOn() & CONTROL_DECIDE;
        waitEnd(org, decide != 0);
    }
}

/* index.txt: ResultArcade::virtual_8 (slot 8).  Once the board is in place
 * (isDone): with every car in, the timer runs down 1/60 s (gp-0x7D58 =
 * 0.0166667); then on `skip' or an expired timer RACE_EVENT_END is posted
 * (every frame from then on). */
// FUNC 0x00174EF8 ResultArcade::waitEnd
void ResultArcade::waitEnd(RaceOrganization &org, int skip)
{
    if (board.isDone() != 0) {
        if (allFinished != 0)
            timer -= RESULT_FRAME_TIME;
        if (skip != 0 || timer < 0.0f)
            org.eventQueue.post(RACE_EVENT_END, 0, 0);
    }
}

/* The rows of the cars still running, in the current rank order (the list
 * rebuilt first): name, icon and the player's mark.  Nothing once every car
 * is in or before the player's car is.  Callers: update,
 * ResultChampionship::update. */
// FUNC 0x00174F90 ResultArcade::updateBoard
void ResultArcade::updateBoard(RaceOrganization &org)
{
    if (allFinished != 0)
        return;
    if (done == 0)
        return;
    int row = 0;
    org.entries->sortByRank();
    for (RaceEntryCar *car = org.entries->rankTop; car != NULL; car = car->rankNext, row++) {
        int number = car->number;
        if (finished[number] != 0)
            continue;
        board.setName(row, car->info.getName1(), car->info.getName2());
        board.setCar(row, number);
        if (car->number == player)
            board.highlight(row);
    }
}

/* index.txt: ResultArcade::virtual_4 (slot 4). */
// FUNC 0x001750A0 ResultArcade::render
void ResultArcade::render(GSBuffer &buf, RaceOrganization &org)
{
    if (active != 0)
        board.draw(&buf);
}

/* index.txt: ResultArcade::virtual_5 (slot 5).  Car `car' reached the
 * goal: its row in the (rebuilt) rank order is filled for good - name,
 * icon, result (setRowResult, virtual), the player's mark - and allFinished
 * is set when no entry is left running.  Nothing for a car already in, or
 * once every car is.  Caller: ResultLinkBattle::goal. */
// FUNC 0x001750C8 ResultArcade::goal
void ResultArcade::goal(RaceOrganization &org, int car)
{
    if (active == 0)
        return;
    if (allFinished != 0)
        return;
    int row = 0;
    org.entries->sortByRank();
    RaceEntryBase *entries = org.entries;
    for (RaceEntryCar *entry = entries->rankTop; entry != NULL;
         entry = entry->rankNext, row++) {
        int number = entry->number;
        if (finished[number] != 0)
            continue;
        if (car != number)
            continue;
        int count = entries->count;
        finished[number] = 1;
        allFinished = 1;
        for (int i = 0; i < count; i++) {
            if (finished[i] == 0) {
                allFinished = 0;
                break;
            }
        }
        board.setName(row, entry->info.getName1(), entry->info.getName2());
        board.setCar(row, number);
        setRowResult(row, entry);
        if (car == player)
            board.highlight(row);
        return;
    }
}

/* index.txt: ResultArcade::virtual_6 (slot 6): the base's finish (called
 * directly), then the board started. */
// FUNC 0x00175258 ResultArcade::finish
void ResultArcade::finish(RaceOrganization &org)
{
    if (active != 0) {
        RaceResultBase::finish(org);
        board.start();
    }
}

/* index.txt: ResultArcade::virtual_9 (slot 9).  The result of a row: the
 * race time (the car's lap time total) and its laps (car_lap_display of its
 * state), or the laps only in a time-limit race.  Caller: goal (virtual). */
// FUNC 0x00175298 ResultArcade::setRowResult
void ResultArcade::setRowResult(int row, RaceEntryCar *car)
{
    if (lapsOnly == 0) {
        RaceLapTime *lap = &car->lapTime;   /* retail: $s0 = car + 0x210 before the call */
        int laps = car_lap_display(car->state);
        board.setTime(row, lap->total, laps);
    } else {
        int laps = car_lap_display(car->state);   /* &board after the call */
        board.setLaps(row, laps);
    }
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function.  Callers: the typeinfo functions
 * of ResultChampionship, ResultLinkBattle, Result2PBattle. */
// FUNC 0x00175318 __tf12ResultArcade
extern "C" void *__tf12ResultArcade(void)
{
    if (__ti12ResultArcade[0] == 0) {
        __tf14RaceResultBase();
        __rtti_si(__ti12ResultArcade, "12ResultArcade",
                  (const type_info *)__ti14RaceResultBase);
    }
    return __ti12ResultArcade;
}
#endif

// FUNC 0x00175368 ResultArcade::~ResultArcade
/* implicit in game/menu/records/T117_result_arcade.h; egcs emits this copy (slot 0):
 * ~board (ResultBoard::~ResultBoard, flags 2), the base's vptr, then
 * operator delete (0x0027C0C0) when flags & 1 (the base's destructor is
 * inlined: it has nothing more to do) */
/* implicit: ~ResultBoard, operator delete (the base / member destructors,
 * flags & 1) */
