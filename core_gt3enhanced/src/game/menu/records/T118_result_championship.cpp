/* T118: ResultChampionship - the result board of a championship race.
 *
 * After ResultArcade's race result (CHAMPIONSHIP_RESULT_RACE) the board shows
 * the points every place earns (OPEN_POINTS -> POINTS), then the standings:
 * the points so far plus the points of this race, sorted (OPEN_STANDINGS ->
 * STANDINGS); then the pad's decide button or 15 s posts RACE_EVENT_END
 * (the result screen is over).
 */
/* Retail's .rodata (0x00326170..0x0032643E): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), update's jump table, the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: ResultChampionship, ResultTermBar, ResultTitleBar,
 * ReplayPauseMenu, PauseMenu, RaceMenuSelectWindow, RaceMenuWindow,
 * RaceMenuButton, OperatorList, List / Cell<OperatorHolder>, OperatorHolder,
 * RaceMiniMap, RaceOnboardSpeedmeter, RaceRemoteTachometer,
 * RaceOnboardTachometer, RaceMeterBase, RaceValueDisplayBase,
 * RaceDisplayBase, RaceCameraBase, UnitArena<64>, Operator.  Its .sdata:
 * the "" of T157's inline RaceMessageDisplay::clear (the race displays'
 * header), one byte, 8-aligned at 0x00351DB0 (T095's ends at 0x00351DAD,
 * T119's starts at 0x00351DB8; retail's bytes there are zero).  Neither
 * "bad_alloc" nor the "" is referenced: the anchors. */
// DATA_START 0x00326170
// DATA_START 0x00351DB0
#include <new>
#include "game/menu/framework/T217_operator.h"             /* (seen) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/race/display/T154_race_display.h"         /* (seen, not used: the race displays) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen) */
#include "game/race/pause/T169_race_pause.h"           /* (seen, not used) */
#include "game/race/pause/T170_replay_pause.h"         /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* ResultBoard */
#include "game/menu/records/T117_result_arcade.h"        /* base ResultArcade */
#include "game/menu/records/T118_result_championship.h"
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (info, number, points, rankNext) */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, raceDB, eventQueue) */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_DECIDE */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getName1 / 2 */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getChampionshipPoint */
#include "game/menu/records/results_internal.h"                           /* RESULT_FRAME_TIME, CHAMPIONSHIP_RESULT_* */

// FUNC 0x001753C0 ResultChampionship::start
//   ResultArcade's start, then the race result with a 10 s timer.
void ResultChampionship::start(RaceOrganization &org, int replay)
{
    ResultArcade::start(org, replay);
    state = CHAMPIONSHIP_RESULT_RACE;
    timer = 10.0f;
}

// FUNC 0x001753F8 ResultChampionship::update
//   Per frame.  Calls RaceResultBase::update directly (not ResultArcade's:
//   no waitEnd here, the states below replace it).  Once the board is in
//   place (isDone) the state machine runs; the board's frame runs whenever
//   the board is active.
void ResultChampionship::update(RaceOrganization &org, Control *pad)
{
    RaceResultBase::update(org, pad);
    if (active == 0)
        return;

    updateBoard(org);
    ResultBoard *b = &board;
    if (b->isDone()) {
        int decide = 0;
        if (pad->getButtonsOn() & CONTROL_DECIDE)
            decide = 1;                     /* andi; sltu (a `!= 0' gives srl; andi) */
        u32 st = state;
        switch (st) {
        case CHAMPIONSHIP_RESULT_OPEN_POINTS: {
            /* the points of each place in this race */
            state = st + 1;                 /* -> CHAMPIONSHIP_RESULT_POINTS */
            int count = org.entries->count;
            timer = 10.0f;
            b->setup(count, RESULT_BOARD_POINTS, 0.0f);
            listRaceResult(org);
            b->start();
            RaceDataBase *db = org.raceDB;
            for (int rank = 0; rank < count; rank++)
                b->setPoints(rank, db->getChampionshipPoint(rank));
            b->highlight(getPlayerRank(org));
            break;
        }
        case CHAMPIONSHIP_RESULT_RACE:
        case CHAMPIONSHIP_RESULT_POINTS:
            if (allFinished != 0)
                timer -= RESULT_FRAME_TIME;
            if (decide || timer < 0.0f) {
                state = st + 1;             /* -> OPEN_POINTS / OPEN_STANDINGS */
                b->close(0.5f);
            }
            break;
        case CHAMPIONSHIP_RESULT_OPEN_STANDINGS: {
            /* the standings */
            state = st + 1;                 /* -> CHAMPIONSHIP_RESULT_STANDINGS */
            int count = org.entries->count; /* read before the allFinished store */
            allFinished = 1;
            timer = 15.0f;
            b->setup(count, RESULT_BOARD_TOTAL_POINTS, 0.0f);
            b->start();
            sortStandings(org);
            int playerRow = 0;
            int row = 0;
            /* board. here (not b->): loop.c hoists this + 0xC, retail's copy
             * of the board pointer in $s4 */
            for (ChampionshipStanding *s = head; s != NULL; row++) {
                const char *name = s->car->info.getName1();
                board.setName(row, name, s->car->info.getName2());
                board.setCar(row, s->car->number);
                board.setPoints(row, s->points);
                int car = s->car->number;
                s = s->next;
                if (car == player)
                    playerRow = row;        /* movz */
            }
            b->highlight(playerRow);
            break;
        }
        case CHAMPIONSHIP_RESULT_STANDINGS:
            if (decide || countDown())
                org.eventQueue.post(RACE_EVENT_END, 0, 0);
            break;
        }
    }
    b->update();
}

// FUNC 0x001756C0 ResultChampionship::countDown
//   One frame off the timer; 1 once it is below zero.
int ResultChampionship::countDown()
{
    int expired = 0;
    timer = timer - RESULT_FRAME_TIME;
    if (timer < 0.0f)
        expired = 1;
    return expired;
}

// FUNC 0x001756F0 ResultChampionship::listRaceResult
//   The board's rows: the cars in finishing (rank list) order, their names
//   and icons.
void ResultChampionship::listRaceResult(RaceOrganization &org)
{
    int row = 0;
    for (RaceEntryCar *car = org.entries->rankTop; car != NULL; car = car->rankNext) {
        const char *name = car->info.getName1();
        board.setName(row, name, car->info.getName2());
        board.setCar(row++, car->number);
    }
}

// FUNC 0x00175790 ResultChampionship::sortStandings
//   For the car of each place: points = its points so far + the points of
//   the place (RaceDataBase::getChampionshipPoint), inserted into the list
//   `head' in descending order (a car goes after the cars with as many
//   points: signed compare).
void ResultChampionship::sortStandings(RaceOrganization &org)
{
    int rank = 0;
    head = NULL;
    RaceEntryCar *car = org.entries->rankTop;
    RaceDataBase *db = org.raceDB;
    while (car != NULL) {
        int before = car->points;
        ChampionshipStanding *s = &standings[rank];
        int points = before + db->getChampionshipPoint(rank);
        s->next = NULL;
        s->points = points;
        ChampionshipStanding *prev = NULL;
        ChampionshipStanding *first = head;
        s->car = car;
        ChampionshipStanding *cur = first;
        while (cur != NULL && !(cur->points < points)) {
            prev = cur;
            cur = cur->next;
        }
        if (prev != NULL) {
            prev->next = s;
            s->next = cur;
        } else {
            s->next = first;
            head = s;
        }
        rank++;
        car = car->rankNext;
    }
}

// FUNC 0x001758B0 ResultChampionship::ResultChampionship
/* inline in game/menu/records/T118_result_championship.h; egcs emits this copy (no
 * callers): the inline RaceResultBase / ResultArcade constructors (done =
 * player = 0, vptr _vt$12ResultArcade, the board's ResultBoard()), then head
 * = NULL, vptr _vt$18ResultChampionship, state = 0.  Its EH landing pad
 * (ResultBoard() throws) puts back _vt$14RaceResultBase and rethrows
 * (__throw). */
/* implicit: ResultBoard (the inline constructor's copy: the board's
 * constructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00175920 __tf18ResultChampionship
//   Builds the __si_type_info __ti18ResultChampionship (name
//   "18ResultChampionship" 0x003261F0, base __ti12ResultArcade) on the first
//   call.
extern "C" const type_info &__tf18ResultChampionship(void)
{
    if (__ti18ResultChampionship[0] == 0) {
        __tf12ResultArcade();
        __rtti_si(__ti18ResultChampionship, "18ResultChampionship",
                  (const type_info *)__ti12ResultArcade);
    }
    return *(const type_info *)__ti18ResultChampionship;
}
#endif

// FUNC 0x00175970 ResultChampionship::~ResultChampionship
/* implicit (game/menu/records/T118_result_championship.h declares none); egcs emits this
 * copy for the vtable: ~ResultArcade inlined (board.~ResultBoard(), flags 2;
 * vptr _vt$14RaceResultBase), operator delete(this) when flags & 1 */
/* implicit: ~ResultBoard, operator delete (the base / member destructors,
 * flags & 1) */
