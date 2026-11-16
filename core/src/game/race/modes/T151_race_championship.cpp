/* T151_race_championship.cpp - RaceChampionshipInformation and
 * RaceChampionship (0x0018DD80..0x0018F3E0); the classes are declared in
 * include/game/race/modes/T151_race_championship.h.
 *
 * RaceChampionshipInformation is the GT mode championship: a series of
 * sessions (races) whose codes end in the session number, the player's car
 * and the opponents' cars (entries[1..7]) with their points.  At the first
 * session the opponents are drawn at random (pickEnemies, kept in
 * entries[]); at the next ones they are put back into the race cars.  The
 * grid of the first race puts the player last and the opponents by
 * weight / power; orderByTime orders the cars by their best laps.  After
 * each race the points of the finishing positions (RaceDataBase::
 * getChampionshipPoint) are added.  suspend / resume save the championship
 * in progress into the save data (GameStatus::championship, T422).
 * run_championship (T053) drives it.
 *
 * RaceChampionship is the race mode of a championship race: RaceArcade's
 * members (T134) with the championship points board (ResultChampionship)
 * instead of the arcade result; a registered race class.
 *
 * Includes (u050's data pass): the classes retail's T151 saw, in its order.
 * Its .rodata (0x0032D4F0..0x0032DAB6) is "bad_alloc" (<new>), virtual_24's
 * "RaceChampionship", the two vtables, then the type names in reverse
 * definition order: its own two, the result bars (T200), ReplayPauseMenu
 * (T170), PauseMenu (T169), the windows (T199), RaceMenuButton (T198), the
 * menu operator classes (T216), the race display's (T154, which reads T153
 * with UnitArena<64>'s name: between RaceCameraBase and
 * RaceValueDisplayBase), the GameStatus's garage lists (T429, first) and
 * Operator (a non-polymorphic base: last).  Its .sdata (0x00351F80..
 * 0x00351F90) is T157's "" (read with the race display), then the registry
 * node g_00351F88; its .data the registry record g_002BAD10, then the frame
 * info.
 */
// DATA_START 0x0032D4F0
// DATA_START 0x00351F80
#include "types.h"
#include <stddef.h>                         /* size_t, NULL */
#include "libc.h"                           /* strcpy, strlen */
#include <new>                              /* placement new (RaceChampionship_construct); "bad_alloc" */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (GCC 14's __tf functions) */
#endif
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus::championship / menu; first:
                                               the garage names after the race display's */
#include "game/race/display/T154_race_display.h"         /* RaceDisplay::setCarIcons; reads T153 with UnitArena<64>'s name */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the menu operator classes: retail's type-name order) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton: retail's type-name order) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (the windows: retail's type-name order) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (F_001A3248) */
#include "game/menu/records/T200_result_board.h"         /* (the result bars: retail's type-name order) */
#include "game/race/modes/T151_race_championship.h"    /* own; + T152, T133 RaceSinglePlayer, T184 CarIcon, T118
                                               ResultChampionship, T316 DynamicsConductorSinglePlayer,
                                               T482 CarEquipments */
#include "game/menu/records/T117_result_arcade.h"        /* ResultArcade::setDisplays */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar, RaceEntryInformation */
#include "game/race/core/T123_race_setup.h"           /* g_002B3BF0, the race object pool */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, the entry order) */
#include "game/race/core/T129_race_base.h"            /* RaceBase (org, dc), RaceClassInfo / Node, race_class_register */
#include "game/car/model/T184_car_icon_maker.h"       /* car_icons_make */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (the performance record of a car) */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_best_lap */
#include "game/save/T422_championship_save.h"    /* ChampionshipSave */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getChampionshipPoint */
#include "game/race/core/race_championship_internal.h"                           /* the unit's constants */

/* ======================================================================== */
/* RaceChampionshipInformation                                              */
/* ======================================================================== */

// FUNC 0x0018DD80 RaceChampionshipInformation::RaceChampionshipInformation
//   RaceGTmodeInformation::RaceGTmodeInformation, the vptr, the 8 entries
//   (CarEquipments::CarEquipments each; the landing pads destroy the ones
//   built with CarEquipments::~CarEquipments, then RaceGTmodeInformation::
//   ~RaceGTmodeInformation); then no points and no order, the counters
//   cleared.  Caller: run_championship (T053).
RaceChampionshipInformation::RaceChampionshipInformation()
{
    /* implicit: RaceGTmodeInformation (base constructor), CarEquipments (constructor of each entry's car), ~CarEquipments, ~RaceGTmodeInformation (exception cleanup) */
    for (int i = 0; i < CHAMPIONSHIP_ENTRIES; i++) {
        entries[i].points = 0;
        entries[i].order = -1;
    }
    session = 0;
    sessions = 0;
    selectEnemies = 0;
}

// FUNC 0x0018DEB0 RaceChampionshipInformation::~RaceChampionshipInformation
//   (index: virtual_0.)  The vptr, the entries destroyed from the last
//   (CarEquipments::~CarEquipments, flags 2), then RaceGTmodeInformation::
//   ~RaceGTmodeInformation with the caller's flags (it does the operator
//   delete).
RaceChampionshipInformation::~RaceChampionshipInformation()
{
    /* implicit: ~CarEquipments (destructor of each entry's car), ~RaceGTmodeInformation (base destructor) */
}

// FUNC 0x0018DF38 RaceChampionshipInformation::setupOrganization
//   (index: virtual_11.)  The first session draws or puts back the
//   opponents (setupFirstSession), the next ones take the last order
//   (setupNextSession).
void RaceChampionshipInformation::setupOrganization(RaceOrganization *org)
{
    if (session == 0)
        setupFirstSession(org);
    else
        setupNextSession(org);
}

// FUNC 0x0018DF70 RaceChampionshipInformation::applyEntryOrder
//   The order of entries 0..5 into the race cars (RaceEntryCar::order).
//   Callers: orderByTime, orderByPowerWeight.
void RaceChampionshipInformation::applyEntryOrder(RaceOrganization *org)
{
    RaceEntryCar **cars = org->entries->cars;
    for (int i = 0; i < GTMODE_RACE_CARS; i++)
        cars[i]->order = entries[i].order;
}

// FUNC 0x0018DFA8 RaceChampionshipInformation::orderByTime
//   (u017: "after a free run with a time".)  The 6 cars ordered by their
//   best laps (car_best_lap of their states; unsigned, ascending): place[i]
//   = the number of cars with a better time; equal times share a place and
//   the next free position is taken.  The three arrays are new int[6]
//   (operator new[]) and freed with operator delete, not delete[], as in
//   the original.  Caller: run_championship (T053).
//   Matching (83.7%, open): GCSE's busy PRE hoists the second loop's
//   org->entries load above the clearing loop (and onto its back edge),
//   so loop.c skips the loop and does not strength-reduce bestLap[i] as
//   retail does.  Retail behaves as if the clearing loop killed the load:
//   with an asm memory barrier between the loops (or the bestLap stores
//   through an unknown-base union) the code is retail's.  Tried: loop
//   forms, counters, statement orders, accessors / inline helpers for the
//   cars, operator new[] calls, chained stores, laundered pointers, an EH
//   region, -fno-strict-aliasing, the reference compiler.
void RaceChampionshipInformation::orderByTime(RaceOrganization *org)
{
    int i;                                  /* one counter for all loops (retail's $s0) */
    int *place = new int[GTMODE_RACE_CARS]; /* implicit: operator new[] (new-expression) */
    int *taken = new int[GTMODE_RACE_CARS];
    u32 *bestLap = new u32[GTMODE_RACE_CARS];
    for (i = 0; i < GTMODE_RACE_CARS; i++) {
        taken[i] = 0;
        place[i] = 0;
        bestLap[i] = 0;
    }
    for (i = 0; i < GTMODE_RACE_CARS; i++)
        bestLap[i] = car_best_lap(org->entries->cars[i]->state);
    for (i = 0; i < GTMODE_RACE_CARS; i++) {
        for (int j = i + 1; j < GTMODE_RACE_CARS; j++) {
            if (bestLap[i] != bestLap[j]) {
                if (bestLap[j] < bestLap[i])
                    place[i]++;
                else
                    place[j]++;
            }
        }
    }
    for (i = 0; i < GTMODE_RACE_CARS; i++) {
        int pos = place[i];
        while (taken[pos] == 1)
            pos++;
        taken[pos] = 1;
        entries[i].order = pos;
    }
    operator delete(place);                 /* quirk: not operator delete[] */
    operator delete(taken);
    operator delete(bestLap);
    applyEntryOrder(org);
}

// FUNC 0x0018E190 RaceChampionshipInformation::orderByPowerWeight
//   (u017: "sets the race up for the session".)  The player starts 6th
//   (order 5); the opponents 1..5 take the orders 0..4 by weight / power
//   (ShowRoomCar::SetupShowRoomCar of each car with a new CarCondition: the
//   spec's weight over ShowRoomCar::power's computed figure, an integer
//   division, then float; ascending, equal values share a place).  The
//   value of the player's car is computed but not used.  The arrays as in
//   orderByTime (operator new[], operator delete).  Callers:
//   run_championship (T053), setupFirstSession.
//   Matching (81.2%, open): the same GCSE hoisting as orderByTime, here of
//   &condition / &rpm (sp+0x1350 / 0x1370) above the clearing loop; retail
//   computes them in the second loop's preheader (loop.c) and does not
//   strength-reduce weightPerPower[i] (one i*4 for both arrays).
void RaceChampionshipInformation::orderByPowerWeight(RaceOrganization *org)
{
    int i;                                  /* one counter for all loops (retail's $s2) */
    entries[0].order = GTMODE_RACE_CARS - 1;
    int *place = new int[GTMODE_RACE_CARS]; /* implicit: operator new[] (new-expression) */
    int *taken = new int[GTMODE_RACE_CARS];
    f32 *weightPerPower = new f32[GTMODE_RACE_CARS];
    for (i = 0; i < GTMODE_RACE_CARS; i++) {
        taken[i] = 0;
        place[i] = 0;
        weightPerPower[i] = 0;
    }
    for (i = 0; i < GTMODE_RACE_CARS; i++) {
        ShowRoomCar car;                    /* sp+0x0000 */
        CarCondition condition;             /* sp+0x1350 */
        int rpm;                            /* sp+0x1370 */
        car.SetupShowRoomCar(&org->entries->cars[i]->info, &condition);
        int weight = car.spec.weight;
        weightPerPower[i] = (f32)(weight / car.power(&rpm, UNIT_TAG_PS, 0));
    }
    for (i = 1; i < GTMODE_RACE_CARS; i++) {
        for (int j = i + 1; j < GTMODE_RACE_CARS; j++) {
            if (weightPerPower[i] != weightPerPower[j]) {
                if (weightPerPower[j] < weightPerPower[i])
                    place[i]++;
                else
                    place[j]++;
            }
        }
    }
    for (i = 1; i < GTMODE_RACE_CARS; i++) {
        int pos = place[i];
        while (taken[pos] == 1)
            pos++;
        taken[pos] = 1;
        entries[i].order = pos;
    }
    operator delete(place);                 /* quirk: not operator delete[] */
    operator delete(taken);
    operator delete(weightPerPower);
    applyEntryOrder(org);
}

// FUNC 0x0018E3F0 RaceChampionshipInformation::getEntryByOrder
//   A copy (returned by value; the implicit copy constructor: the
//   CarEquipments with the CarDataBase / CarEquipments vptrs, then points,
//   order, highlight) of the entry of 0..5 whose order is `order'; the
//   entry's highlight flag is set first for the player's entry, cleared for
//   the others.  When there is none the original copies from address 0 (no
//   check; retail's addresses are constants, lw 0x1E0($zero)).  GCC 14 would
//   turn the null dereference into a trap: there a pointer it cannot see is
//   NULL reads the same bytes.  Caller: result_list_from_entries (T102, the
//   standings).
ChampionshipEntry RaceChampionshipInformation::getEntryByOrder(int order)
{
    for (int i = 0; i < GTMODE_RACE_CARS; i++) {
        if (entries[i].order == order) {
            if (i == 0)
                entries[i].highlight = true;
            else
                entries[i].highlight = false;
            return entries[i];
        }
    }
#ifdef BOOT_OLD_GCC
    return *(ChampionshipEntry *)0;         /* quirk: no entry of that order */
#else
    ChampionshipEntry *volatile none = 0;   /* quirk: *(ChampionshipEntry *)0 */
    return *none;
#endif
}

// FUNC 0x0018E580 RaceChampionshipInformation::pickEnemies
//   Draws random opponents for the cars 1..count-1 (RaceInformation slot
//   13, assignCarRange) and keeps them in entries[1..count-1] (only the
//   CarEquipments part: CarEquipments::operator=); the opponents are then
//   chosen (selectEnemies = 1).  Caller: setupFirstSession.
void RaceChampionshipInformation::pickEnemies(RaceOrganization *org)
{
    int count = org->entries->count;
    assignCarRange(org, 1, count);
    for (int i = 1; i < count; i++)
        entries[i].car = org->entries->cars[i]->info; /* implicit: CarEquipments::operator= (assignment) */
    selectEnemies = 1;
}

// FUNC 0x0018E638 RaceChampionshipInformation::setupFirstSession
//   Opponents chosen (selectEnemies): they go back into the race cars (only
//   the CarEquipments part: CarEquipments::operator=), the entry order is
//   reset and the grid ordered by weight / power.  Otherwise they are drawn
//   (pickEnemies) and the player moved from the front to the back
//   (moveEntryOrder(0, count - 1); count as read on entry).  The loop
//   after orderByPowerWeight is empty in the original (count iterations of
//   nothing).  Caller: setupOrganization.
void RaceChampionshipInformation::setupFirstSession(RaceOrganization *org)
{
    int i;                                  /* one counter for both loops (retail's $s0) */
    int count = org->entries->count;
    if (selectEnemies != 0) {
        for (i = 1; i < count; i++)
            static_cast<CarEquipments &>(org->entries->cars[i]->info) = entries[i].car;   /* implicit: CarEquipments::operator= */
        org->resetEntryOrder();
        orderByPowerWeight(org);
        for (i = 0; i < count; i++) {
            /* empty (? debug output compiled out; egcs counts it down) */
        }
    } else {
        pickEnemies(org);
        org->resetEntryOrder();
        org->moveEntryOrder(0, count - 1);
    }
}

// FUNC 0x0018E738 RaceChampionshipInformation::setupNextSession
//   The opponents back into the race cars (CarEquipments::operator=), the
//   entry order reset, and the order of the last race given to every car.
//   Ends with an empty loop of count iterations, as setupFirstSession.
//   Caller: setupOrganization.
void RaceChampionshipInformation::setupNextSession(RaceOrganization *org)
{
    int i;                                  /* one counter for all loops (retail's $s0) */
    int count = org->entries->count;
    for (i = 1; i < count; i++)
        static_cast<CarEquipments &>(org->entries->cars[i]->info) = entries[i].car;   /* implicit: CarEquipments::operator= */
    org->resetEntryOrder();
    for (i = 0; i < count; i++)
        org->entries->cars[i]->order = entries[i].order;
    for (i = 0; i < count; i++) {
        /* empty (? debug output compiled out; egcs counts it down) */
    }
}

// FUNC 0x0018E838 RaceChampionshipInformation::addPoints
//   (u017: "records a finished session".)  Down the race order (rankTop /
//   rankNext), the car at position i gets the points of position i
//   (RaceDataBase::getChampionshipPoint of the information's database).
//   Caller: run_championship (T053).
void RaceChampionshipInformation::addPoints(RaceOrganization *org)
{
    RaceEntryBase *list = org->entries;
    int count = list->count;
    RaceEntryCar *car = list->rankTop;
    for (int pos = 0; pos < count; pos++) {
        int number = car->number;
        int points = db.getChampionshipPoint(pos);
        car = car->rankNext;
        entries[number].points += points;
    }
}

// FUNC 0x0018E8D0 RaceChampionshipInformation::nextSession
//   The next session: its number (session + 1, two digits) becomes the end
//   of the current race code, which is then the race code (setRaceCode of
//   nameToCode).  Caller: run_championship (T053).
void RaceChampionshipInformation::nextSession()
{
    session++;
    int len = strlen(currentRaceCode);
    currentRaceCode[len - 1] = (session + 1) % 10 + '0';
    currentRaceCode[len - 2] = (session + 1) / 10 + '0';
    setRaceCode(nameToCode(currentRaceCode));
}

// FUNC 0x0018E970 RaceChampionshipInformation::getPlayerRank
//   No callers.  The position (0-based) of the first car number 0 (the
//   player) down the race order; count if it is not found.
int RaceChampionshipInformation::getPlayerRank(RaceOrganization *org)
{
    RaceEntryBase *list = org->entries;
    int pos = 0;
    int count = list->count;
    RaceEntryCar *car = list->rankTop;
    for (; pos < count; pos++) {
        if (car->number == 0)
            break;
        car = car->rankNext;
    }
    return pos;
}

// FUNC 0x0018E9A8 RaceChampionshipInformation::setChampionship
//   (u017: "selects the championship".)  The race code (setRaceCode of
//   nameToCode); unless continued (cont != 0): both codes = code, session
//   0, the global code's last character decremented, `sessions' from the
//   two characters 4 and 3 before the end (signed chars: tens, units) minus
//   one, and the points of the 8 entries cleared (from the last).  Caller:
//   run_championship (T053).
void RaceChampionshipInformation::setChampionship(const char *code, int cont)
{
    setRaceCode(nameToCode(code));
    if (cont == 0) {
        strcpy(globalRaceCode, code);
        strcpy(currentRaceCode, code);
        session = 0;
        int len = strlen(currentRaceCode);
        globalRaceCode[len - 1]--;
        sessions = currentRaceCode[len - 3] - '0' - 1;
        sessions += (currentRaceCode[len - 4] - '0') * 10;
        for (int i = CHAMPIONSHIP_ENTRIES - 1; i >= 0; i--)
            entries[i].points = 0;
    }
}

// FUNC 0x0018EA98 RaceChampionshipInformation::F_0018EA98
//   An empty function, no callers (index: RaceLinkBattle::updatePause, a
//   name matched by identical code; it lies in T151).
void RaceChampionshipInformation::F_0018EA98()
{
}

// FUNC 0x0018EAA0 RaceChampionshipInformation::virtual_9
//   (slot 9.)  The points of entry i.  Caller: run_championship (T053,
//   directly).
int RaceChampionshipInformation::virtual_9(int i)
{
    return entries[i].points;
}

// FUNC 0x0018EAB8 RaceChampionshipInformation::addEnemyPoints
//   (u017: "a session without a time".)  addPoints for every car but number
//   0 (the player).  Caller: run_championship (T053).
void RaceChampionshipInformation::addEnemyPoints(RaceOrganization *org)
{
    RaceEntryBase *list = org->entries;
    int count = list->count;
    RaceEntryCar *car = list->rankTop;
    for (int pos = 0; pos < count; pos++) {
        int number = car->number;
        if (number != 0)
            entries[number].points += db.getChampionshipPoint(pos);
        car = car->rankNext;
    }
}

// FUNC 0x0018EB58 RaceChampionshipInformation::suspend
//   (u017: "before saving the game".)  The championship in progress into
//   the save data (GameStatus::championship): active = 1, the current menu
//   page (MenuStatus::getPage), both codes, the counters, the 8 cars and
//   their points.  Caller: run_championship (T053).
void RaceChampionshipInformation::suspend()
{
    char page[0x40];                        /* sp+0x00 */
    ChampionshipSave *save = getGameStatus()->getChampionshipSave();
    save->active = 1;
    getGameStatus()->getMenuStatus()->getPage(page);
    save->setPage(page);
    save->setGlobalRaceCode(globalRaceCode);
    save->setCurrentRaceCode(currentRaceCode);
    save->setSession(session);
    save->setSessions(sessions);
    for (int i = 0; i < CHAMPIONSHIP_ENTRIES; i++) {
        save->setCar(&entries[i].car, i);
        save->setPoints(entries[i].points, i);
    }
}

// FUNC 0x0018EC48 RaceChampionshipInformation::clearSuspend
//   (u017: "after saving the game".)  The save is no longer active; `this'
//   is not used.  Caller: run_championship (T053).
void RaceChampionshipInformation::clearSuspend()
{
    getGameStatus()->getChampionshipSave()->active = 0;
}

// FUNC 0x0018EC78 RaceChampionshipInformation::resume
//   (u017: "restores a saved championship".)  If one is suspended: the menu
//   goes back to the saved page (MenuStatus::setPage, arg -1) with the
//   event code = the saved global code with its last character incremented
//   (MenuStatus::setEvent) and raceMode = 2 (the next race of a series);
//   then the codes, counters, cars (CarEquipments::operator=) and points
//   back, and the save is no longer active.  Caller: run_championship
//   (T053).
void RaceChampionshipInformation::resume()
{
    char code[0x40];                        /* sp+0x00 */
    ChampionshipSave *save = getGameStatus()->getChampionshipSave();
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    if (save->active != 0) {
        menu->setPage(save->getPage(), -1);
        strcpy(code, save->getGlobalRaceCode());
        code[strlen(code) - 1]++;
        menu->setEvent(code);
        menu->raceMode = MENU_RACE_MODE_SERIES;
        strcpy(globalRaceCode, save->getGlobalRaceCode());
        strcpy(currentRaceCode, save->getCurrentRaceCode());
        session = save->getSession();
        sessions = save->getSessions();
        for (int i = 0; i < CHAMPIONSHIP_ENTRIES; i++) {
            entries[i].car = *save->getCar(i); /* implicit: CarEquipments::operator= (assignment) */
            entries[i].points = save->getPoints(i);
        }
        save->active = 0;
    }
}

// FUNC 0x0018EDF0 RaceChampionshipInformation::isPlayerLeading
//   (u017: "1 if the championship was won".)  1 when the player's points
//   are above those of every other entry (1..7, signed).  Caller:
//   run_championship (T053).
int RaceChampionshipInformation::isPlayerLeading()
{
    int playerPoints = entries[0].points;
    for (int i = 1; i < CHAMPIONSHIP_ENTRIES; i++) {
        if (entries[i].points >= playerPoints)
            return 0;
    }
    return 1;
}

// FUNC 0x0018EE30 RaceChampionshipInformation::setPlayerCar
//   (u017: "the player's car = c".)  RaceGTmodeInformation::setCar and
//   entry 0 (CarEquipments::operator=).  Caller: run_championship (T053).
void RaceChampionshipInformation::setPlayerCar(const CarEquipments &c)
{
    RaceGTmodeInformation::setCar(c);
    entries[0].car = c;                         /* implicit: CarEquipments::operator= */
}

// FUNC 0x0018EE70 RaceChampionshipInformation::setPlayerCar
//   No callers.  The player's car loaded by code (RaceGTmodeInformation::
//   setCar(u64): `code' in $a1 passed through), copied into entry 0
//   (CarEquipments::operator=).
void RaceChampionshipInformation::setPlayerCar(u64 code)
{
    RaceGTmodeInformation::setCar(code);
    entries[0].car = car; /* implicit: CarEquipments::operator= (assignment) */
}

/* ======================================================================== */
/* RaceChampionship                                                         */
/* ======================================================================== */

// FUNC 0x0018EEA0 RaceChampionship::operator new
//   The race pool, 16-byte aligned (pdistd::Allocator::allocate).
void *RaceChampionship::operator new(size_t size)
{
    return g_002B3BF0.allocate(size, 0x10);
}

// FUNC 0x0018EEC8 RaceChampionship::operator delete
//   The size (0x34600) is ignored (pdistd::Allocator::free).
void RaceChampionship::operator delete(void *p, size_t size)
{
    g_002B3BF0.free(p);
}

// FUNC 0x0018EEF0 RaceChampionship::virtual_24
//   The class name.
const char *RaceChampionship::virtual_24()
{
    return "RaceChampionship";
}

// FUNC 0x0018EF00 RaceChampionship_create
//   RaceClassInfo::create (g_002BAD10 +8): new RaceChampionship (0x34600;
//   RaceChampionship::operator new, the constructor, operator delete(p,
//   0x34600) if it throws).
RaceBase *RaceChampionship_create(void)
{
    return new RaceChampionship;                /* implicit: RaceChampionship::operator delete (if the constructor throws) */
}

// FUNC 0x0018EF60 RaceChampionship_construct
//   RaceClassInfo::construct (g_002BAD10 +0xC): a RaceChampionship in buf
//   (NULL: nothing).  arg is not used.  The global placement new of <new>:
//   egcs's is inline and throw(), so egcs tests its result for NULL itself
//   (retail's daddu v0,a0 / beqz v0 around the constructor; u045's
//   race_basic_construct).  GCC 14 does not test the standard placement
//   form: the explicit test keeps the behaviour.
RaceBase *RaceChampionship_construct(void *buf, int arg)
{
#ifndef BOOT_OLD_GCC
    if (buf == NULL)
        return NULL;
#endif
    return ::new (buf) RaceChampionship;
}

/* The registry record of "RaceChampionship" (.data, before the frame info;
 * its "RaceChampionship" is virtual_24's literal, emitted once).
 * sizeof(RaceChampionship) is 0x34600 under egcs (EGCS_SIZE) and GCC 14's
 * own size there, which is what create / construct build. */
RaceClassInfo g_002BAD10 = {                /* 0x002BAD10 */
    "RaceChampionship", sizeof(RaceChampionship), RaceChampionship_create, RaceChampionship_construct
};
/* Its node in the registry (zeros, linked in at start-up by
 * race_championship_static_ctor): .sdata after T157's "" (0x00351F80).
 * Explicitly initialised, so egcs writes it here (u047 / u049: an
 * uninitialised global would come after every literal; T151 has no later
 * .sdata literal, so either form gives retail's bytes). */
RaceClassNode g_00351F88 = { NULL, NULL };  /* 0x00351F88 gp-0x5A68 */

// FUNC 0x0018EF88 RaceChampionship::RaceChampionship
//   RaceSinglePlayer::RaceSinglePlayer, the vptr, then the members: the 7
//   icons (CarIcon::CarIcon each), the result (ResultChampionship's inline
//   constructor: done = player = 0, the ResultArcade vptr,
//   ResultBoard::ResultBoard, state = head = 0, the ResultChampionship
//   vptr 0x00326198), the pause menu (RacePause::RacePause), the replay
//   pause menu (ReplayPause::ReplayPause, mode 0) and the dynamics
//   (DynamicsConductorSinglePlayer::DynamicsConductorSinglePlayer, never
//   destroyed).  The body installs the dynamics; unlike RaceArcade it does
//   not name the race mode.  The landing pads destroy what was built
//   (ReplayPause::~ReplayPause, RacePause::~RacePause, the result's
//   ResultBoard::~ResultBoard with flags 2 and the RaceResultBase vptr
//   back, RaceSinglePlayer::~RaceSinglePlayer).  Callers: run_championship
//   (T053), create / construct.
RaceChampionship::RaceChampionship()
    : replayPause(0)
{
    /* implicit: RaceSinglePlayer (base constructor), CarIcon, ResultBoard (in ResultChampionship's inline constructor), RacePause, ReplayPause, DynamicsConductorSinglePlayer (member constructors), ~ReplayPause, ~RacePause, ~ResultBoard, ~RaceSinglePlayer (exception cleanup) */
    dc = &dcSingle;
}

// FUNC 0x0018F130 RaceChampionship::~RaceChampionship
//   (index: virtual_0.)  The vptr, then the members in reverse order (flags
//   2): ReplayPause::~ReplayPause, RacePause::~RacePause, the result
//   (implicit: ResultBoard::~ResultBoard, the vptr = RaceResultBase's);
//   RaceSinglePlayer::~RaceSinglePlayer (flags 0); deleting form: operator
//   delete(this, 0x34600).
RaceChampionship::~RaceChampionship()
{
    /* implicit: ~ReplayPause, ~RacePause, ~ResultBoard (member destructors), ~RaceSinglePlayer (base destructor) */
}

// FUNC 0x0018F1E0 RaceChampionship::virtual_8
//   Race start: RaceSinglePlayer's, then the menus: in a replay (mode 1)
//   the replay pause menu (given the organization first, F_001A3248) and
//   no result, otherwise the race pause menu and the championship result
//   (RaceBasic::setPauseMenu / setSubDisplay).
void RaceChampionship::virtual_8(int replay)
{
    RaceSinglePlayer::virtual_8(replay);
    if (replay != RACE_REPLAY) {
        setPauseMenu(&pause);
        setSubDisplay(&result);
    } else {
        replayPause.F_001A3248(org);
        setPauseMenu(&replayPause);
        setSubDisplay(NULL);
    }
}

// FUNC 0x0018F278 RaceChampionship::virtual_43
//   Before the race runs: the car icons of this organization
//   (car_icons_make), given to the result board (ResultArcade::setDisplays)
//   and to the race display (the inline RaceDisplay::setCarIcons: retail
//   stores through the display's address, its `this'; as
//   RaceArcade::virtual_43).
void RaceChampionship::virtual_43()
{
    car_icons_make(icons, org);
    result.setDisplays(icons);
    display.setCarIcons(icons);
}

// FUNC 0x0018F2D8 race_championship_static_ctor
//   .ctors entry (0x0028F128): registers "RaceChampionship" (g_002BAD10,
//   node g_00351F88 gp-0x5A68).
void race_championship_static_ctor(void)
{
    race_class_register(&g_00351F88, &g_002BAD10);
}

#ifndef BOOT_OLD_GCC
// FUNC 0x0018F300 __tf16RaceChampionship
//   The __si_type_info __ti16RaceChampionship, base __ti16RaceSinglePlayer.
extern "C" const type_info &__tf16RaceChampionship(void)
{
    if (__ti16RaceChampionship[0] == 0) {
        __tf16RaceSinglePlayer();
        __rtti_si(__ti16RaceChampionship, "16RaceChampionship",
                  (const type_info *)__ti16RaceSinglePlayer);
    }
    return *(const type_info *)__ti16RaceChampionship;
}
#endif

/* ======================================================================== */
/* RaceChampionshipInformation: accessors                                   */
/* ======================================================================== */

/* Inline in game/race/modes/T151_race_championship.h (declared in the reverse of this
 * order); egcs emits these copies at the end of the unit, after
 * __tf16RaceChampionship and before __tf27RaceChampionshipInformation (this
 * unit defines the class's first virtual function). */
// FUNC 0x0018F350 RaceChampionshipInformation::getSessionNumber
// FUNC 0x0018F358 RaceChampionshipInformation::isLastSession
//   No callers (T053 expands it).
// FUNC 0x0018F370 RaceChampionshipInformation::getCurrentRaceCode
// FUNC 0x0018F378 RaceChampionshipInformation::getGlobalRaceCode
// FUNC 0x0018F380 RaceChampionshipInformation::setSelectEnemies
// FUNC 0x0018F388 RaceChampionshipInformation::IsSelectEnemies

#ifndef BOOT_OLD_GCC
// FUNC 0x0018F390 __tf27RaceChampionshipInformation
//   The __si_type_info __ti27RaceChampionshipInformation, base
//   __ti21RaceGTmodeInformation.
extern "C" const type_info &__tf27RaceChampionshipInformation(void)
{
    if (__ti27RaceChampionshipInformation[0] == 0) {
        __tf21RaceGTmodeInformation();
        __rtti_si(__ti27RaceChampionshipInformation, "27RaceChampionshipInformation",
                  (const type_info *)__ti21RaceGTmodeInformation);
    }
    return *(const type_info *)__ti27RaceChampionshipInformation;
}
#endif
