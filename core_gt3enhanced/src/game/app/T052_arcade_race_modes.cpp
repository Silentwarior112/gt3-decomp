/* T052 - the arcade race sessions.  0x0012F840..0x00131410.
 *
 * Called from T000: main_arcade (the arcade menu loop: state 2 single race,
 * state 3 time trial, state 4 free run) and main_free_run (mode 8, a free
 * run from GT mode).  Every session:
 *   - select_race selects the race once; the GarageCar GameStatus::menu
 *     .raceCar[0] is the car;
 *   - an arcade car (raceCarKind[0] RACE_CAR_BY_CODE, always in the time
 *     trial) gets the menu's colour (and tyres) and has no car setting
 *     (ShortMenuArgs::noSetting); a garage car takes its condition into the
 *     race and back, and the odometer gain goes to GameStatus::distance;
 *   - the race information gets the car (a CarEquipments copy), the race
 *     and the course, builds the RaceOrganization, and the race object runs
 *     against it;
 *   - a loop runs the short menu (run_short_menu) over the race and acts on
 *     the QM_* code it returns, until QM_EXIT.
 * Stack layout (all three): ShortMenuArgs sp+0x000, the information
 * sp+0x190, a CarEquipments temporary / ShortMenuParams sp+0x3F0, a
 * RaceResult copy sp+0x400, the menu lines / name / records sp+0x410, the
 * race sp+0x5E0, the spilled locals after the race.
 *
 * ShortMenuArgs' inline constructor (expanded in all three) constructs the
 * 6 result lines and the 12 rank lines: their inline constructors call the
 * inline clear() (ShortMenuResultEntry::clear, T102's copy 0x0012F820;
 * ShortMenuRankEntry::clear, this unit's copy 0x001313D0), then it sets the
 * members.
 *
 * Matching (u016): the `car' references are declared where they are used
 * (in each arm of the car test, before the time trial's colour): declared
 * earlier, PRE hoists `gcar + 0x10' into the preheader of the vector loops.
 * The CarEquipments copy is setEquipments(*gcar) (the loop end comes from
 * gcar).  The options are read through getGameOptions() before the entry's
 * vibration, tyre, transmission.  ShortMenuArgs is set and read through its
 * inline accessors (T102), as in T053: with a direct read of noSetting,
 * jump.c's thread_jumps finds the same load before both `noSetting' tests
 * of a race start and threads the first into the second's body. */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T052's .rodata (0x003158D0..0x00315E06): "bad_alloc" (<new>, read first),
 * the three menu files (arcadeMenus), the jump tables and "ghost_dlg", then
 * the type names ShortmenuControl, SmartIndexT<int>, ExaminationDisplayer,
 * CallBackWhenPrizeRendering, PrizeModelSetContext, PauseMenuWithRetry,
 * ResultTermBar, ResultTitleBar, ReplayPauseMenu, PauseMenu, the race menu
 * windows, RaceMenuButton, the OperatorHolder group, the race displays,
 * RaceCameraBase, UnitArena<64>, RegurationResult, RecordsMenuArg, the garage
 * lists, Operator.  Its .sdata (0x003514F8): the "" of T157's inline
 * RaceMessageDisplay::clear, then "root".  The screens T052 calls (the
 * trophy and prize car screens, the memory card, the setting menu, the
 * records, the name entry) only through their entry points: retail has
 * none of their controllers' names.  T102's header before T052's (the
 * clear() copies' order, below).  ref/data/u016/notes.md. */
// DATA_START 0x003158D0
// DATA_START 0x003514F8
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* gt2k_menu::RecordsMenuArgFreeTT, ...Compulsory */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase) */
#include "game/race/display/T154_race_display.h"         /* (the race displays; T157's "" in the .sdata) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (the OperatorHolder group) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (RaceMenuButton) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (RaceMenuWindow, RaceMenuSelectWindow) */
#include "game/race/pause/T169_race_pause.h"           /* (PauseMenu) */
#include "game/race/pause/T170_replay_pause.h"         /* (ReplayPauseMenu) */
#include "game/menu/records/T200_result_board.h"         /* (ResultTitleBar, ResultTermBar) */
#include "game/race/pause/T171_race_pause_with_retry.h" /* (PauseMenuWithRetry) */
#include "game/menu/gtmode/T460_prize_base.h"           /* (no use: PrizeModelSetContext, CallBackWhenPrizeRendering) */
#include "game/race/display/T141_race_time_trial_display.h" /* (ExaminationDisplayer) */
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* (no use: SmartIndexT<int>, ShortmenuControl) */
/* The rest adds no type name. */
#include "game/app/T000_main.h"                 /* MenuArgs, RaceEntry */
#include "game/app/T002_render_thread.h"        /* GameObject (base of the races) */
#include "game/app/T007_game_init.h"            /* race_begin, race_end */
#include "game/menu/gtmode/T036_garage_car.h"           /* GarageCar */
#include "game/debug/T055_logger.h"               /* logger_run */
#include "game/menu/gtmode/T056_trophy_prize_entry.h"   /* trophy_prize_screen */
#include "game/menu/gtmode/T061_car_get_entry.h"        /* F_0013F508 (the prize cars, the endings) */
#include "game/menu/options/T062_option_control.h"       /* F_0013FE58 (the options menu) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* gt2k_menu::MEMCARD_* (race_memcard's modes) */
#include "game/menu/car_settings/T080_setting_entry.h"        /* setting_menu */
#include "game/menu/records/T090_records_entry.h"        /* run_records_menu */
#include "game/menu/framework/T097_name_entry_screen.h"    /* name_entry */
/* T102's header first: ShortMenuResultEntry is then defined before
 * ShortMenuRankEntry, and egcs writes their clear() copies in retail's
 * order, 0x001313D0 RankEntry's, 0x001313F0 ResultEntry's. */
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* ShortMenuArgs / Params / ResultEntry, QM_*, run_short_menu,
                                               race_entry_rank, race_set / get_entry_condition,
                                               race_memcard, result_list_by_rank, make_ranking_code,
                                               load_demo_setting, select_race */
#include "game/app/T052_arcade_race_modes.h"    /* ShortMenuRankEntry, run_arcade_race, ... */
#include "game/menu/race_menu/T115_race_option.h"          /* race_option_from_game_options */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T123_race_setup.h"           /* race_organization_create */
#include "game/race/core/T124_race_information.h"     /* RaceInformation */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, RaceResult, RaceDBRecord */
#include "game/race/core/T129_race_base.h"            /* RaceBase */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base */
#include "game/race/modes/T133_race_single_player.h"   /* RaceSinglePlayerInformation */
#include "game/race/modes/T134_race_arcade.h"          /* RaceArcade, RaceArcadeInformation */
#include "game/race/modes/T139_race_solitaire.h"       /* RaceSolitaire, RaceSolitaireInformation */
#include "game/race/modes/T140_race_time_trial.h"      /* RaceTimeTrial */
#include "game/race/modes/T142_race_free_run.h"        /* RaceFreeRun, RaceFreeRunInformation */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState */
#include "game/race/dynamics/T303_race_time_events.h"     /* car_best_lap */
#include "game/save/T415_machine_test_records.h" /* RankingCode */
#include "game/save/T416_free_run_records.h"     /* FreeRunRecords */
#include "game/save/T417_time_trial_records.h"   /* TimeTrialRecords */
#include "game/save/T431_ranking_code.h"         /* RankingKind (make_ranking_code's kind) */
#include "game/save/T424_arcade_records.h"       /* ArcadeRecords */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/save/T428_game_options.h"         /* GameOptions */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T483_car_information.h"      /* CarInformation */
#include "game/data/T484_race_database.h"        /* RaceDataBase, RaceEvent */
#include "game/data/T489_car_database.h"         /* race_name_split */
#include "game/app/arcade_race_modes_internal.h"
#include "game/enhanced.h"                  /* ENH_MENU_FREE_RUN_ASSISTS */

/* The short menus of the three sessions: {file, first page, BGM group}.
 * One array in T052's .data (12-byte elements: separate objects would be
 * 8-aligned), copied to the session's ShortMenuParams (sp+0x3F0). */
enum {
    ARCADE_MENU_RESULT,                     /* run_arcade_race */
    ARCADE_MENU_FREE_RUN,                   /* run_free_run */
    ARCADE_MENU_TIME_TRIAL                  /* run_time_trial */
};
static ShortMenuParams arcadeMenus[] = {    /* 0x0029D4F0 */
    { "menu/qm_arc_result.mbl", "root", 0x16 },
    { "menu/qm_arc_tt_free.mbl", "root", 0x15 },
    { "menu/qm_arc_tt_ltd.mbl", "root", 0x15 }
};

/* Arcade single race (arcade menu state 2, runMode 6): RaceArcade with the
 * result menu "menu/qm_arc_result.mbl".  The first race starts at once
 * (QM_GO_RACE); after each race the replay plays, then the menu.  A win
 * (place 0) shows the trophy screen of the race's trophy type; the place is
 * recorded per race and course (ArcadeRecords::updatePlace).  At the end,
 * if the last race had a result, the prize-car check F_0013F508 runs.
 * Frame 0x34C20: courseName sp+0x34B60, rank sp+0x34B64, carCode sp+0x34B68,
 * distance sp+0x34B70, loop sp+0x34B74. */
// FUNC 0x0012F840 run_arcade_race
int run_arcade_race(MenuArgs *args)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    const char *courseName = args->course;
    const char *raceName = args->race;
    GarageCar *gcar = menu->raceCar[0];
    select_race(raceName);

    ShortMenuArgs res;                                  /* sp+0x000 */
    /* implicit: ShortMenuResultEntry::clear, ShortMenuRankEntry::clear (the array loops of ShortMenuArgs()) */
    int rank = -1;                                      /* the last race's place; < 0: no result */
    {
        RaceArcadeInformation info;                     /* sp+0x190 */

        if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
            /* an arcade car: the menu's colour and tyres, no car setting */
            CarInformation &car = *gcar;                /* GarageCar +0x10 (no NULL test: a reference) */
            car.presetCar = 1;
            car.setColorIndex(args->color1);
            if (args->tyre1 != 0)
                car.F_00236758();                       /* TIRE_KIND_ARCADE_RACING tyres */
            else
                car.F_00236718();                       /* TIRE_KIND_ARCADE tyres */
            res.setNoSetting(1);
        } else {
            CarInformation &car = *gcar;
            car.presetCar = 0;
            res.setNoSetting(0);
        }
        info.setEquipments(*gcar);                      /* by value: a CarEquipments copy at sp+0x3F0 */

        int action = QM_GO_RACE;                        /* the first race without the menu */
        ::GameOptions *opt = getGameStatus()->getGameOptions();
        info.entry.vibration = opt->vibration1p;
        info.entry.tyre = args->tyre1;
        info.entry.transmission = args->transmission1;
        info.setRaceCode(nameToCode(raceName));
        info.setCourseCode(nameToCode(courseName));     /* direct: RaceSinglePlayerInformation's */
        RaceOrganization *org = race_organization_create(&info);
        u64 carCode = gcar->carCode;
        u64 courseCode = info.getCourseCode();          /* direct: RaceSinglePlayerInformation's */
        res.setRaceInfo(&info);
        {
            RaceArcade race;                            /* sp+0x5E0 */
            race.org = org;

            ShortMenuParams params;                     /* sp+0x3F0, set after each race */
            int distance = 0;                           /* driven in the garage car */
            int loop = 1;
            do {
                if (action != QM_GO_RACE)
                    action = run_short_menu(&params, &res, NULL, NULL, NULL);
                switch (action) {
                case QM_TRY_AGAIN:
                case QM_GO_RACE:
                    rank = -1;
                    race_option_from_game_options(&org->option);
                    if (res.getNoSetting() == 0)
                        race_set_entry_condition(gcar, org, 0);
                    if (res.getNoSetting() != 0) {
                        org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                        org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
                    }
                    org->setEntryTransmission(args->transmission1, 0);
                    /* Lupo 2 (L2 0x00130190; menus_a.md 6 b): a CVT runs
                     * automatic (the car select offers only that) */
                    if (gcar->isCVT() != 0)
                        org->setEntryTransmission(0, 0);
                    org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
                    org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
                    race.virtual_41(RACE_LIVE, 1);      /* the race (direct: RacePS2Base's) */
                    if (res.getNoSetting() == 0)
                        distance += race_get_entry_condition(gcar, org, 0);
                    res.setCanReplay(1);
                    if (org->entries->cars[0]->state->st.finished != 0) {
                        org->entries->sortByRank();
                        rank = race_entry_rank(org, 0);
                        int trophy = org->raceDB->event->trophyType;    /* read before the test */
                        if (rank == 0)
                            trophy_prize_screen(0, 0 /* first */, trophy, 0);
                        result_list_by_rank(org, &res, 1);
                        getGameStatus()->getArcadeRecords()->updatePlace(raceName, courseCode, rank);
                    } else {
                        /* no result: every line empty (pad14 is left) */
                        for (int i = 0; i < 6; i++)
                            res.list[i].clear();
                    }
                    race.virtual_41(RACE_REPLAY, 1);
                    params = arcadeMenus[ARCADE_MENU_RESULT];
                    action = 0;                         /* the menu next */
                    break;
                case QM_REPLAY:
                    race.virtual_41(RACE_REPLAY, 1);
                    break;
                case QM_OPTION:
                    F_0013FE58();
                    break;
                case QM_SAVE_REPLAY:
                    race_memcard(gt2k_menu::MEMCARD_REPLAY_SAVE, courseCode, carCode, raceName, org);
                    break;
                case QM_EXIT:
                    loop = 0;
                    break;
                default:                                /* QM_SETTING, QM_FREE_RUN, QM_NEXT_SESSION, ... */
                    loop = 0;
                    break;
                }
            } while (loop != 0);

            delete org;                                 /* virtual destructor, slot 0 (vptr +0x8C0) */
            getGameStatus()->incDistance(distance);
        }                                               /* ~RaceArcade */
    }                                                   /* ~RaceArcadeInformation */
    race_end();
    if (rank >= 0)
        F_0013F508(raceName, -1);
    return 0;
}

/* Free run (arcade menu state 4 with gtMode 0, runMode 7; main_free_run's
 * mode 8 with gtMode 1): RaceFreeRun with "menu/qm_arc_tt_free.mbl".  Before
 * each menu the cars and the times of the course's three best places go to
 * the result and rank lists.  A race ends with the replay; a best lap that
 * fits the ranking code and makes the course's table asks for the player's
 * name and stores name, car and ranking code.  gtMode != 0: the
 * transmission from the options (gearAuto ^ 1), re-read at each start,
 * instead of args->transmission1.  The ghost question is asked only while
 * options.ghost is 0; when it is set, the session does not take the new
 * ghost (run_time_trial takes it without asking).  Frame 0x3AF40: args
 * sp+0x3AE60, gtMode sp+0x3AE64, gcar sp+0x3AE68, raceName sp+0x3AE6C,
 * carCode sp+0x3AE70, courseCode sp+0x3AE78, distance sp+0x3AE80, loop
 * sp+0x3AE84, startPage sp+0x3AE88, &params sp+0x3AE8C, &info sp+0x3AE90.
 * Not exact (99.0%): run_short_menu's &race is rematerialised (addiu
 * sp+0x5E0; retail passes the race's register).  CSE1 gives that argument
 * the pseudo of virtual_8's `this' just before; after PRE it is one copy
 * away from the PRE register, which GCSE's single copy-propagation pass
 * does not reach (retail: a fresh computation, propagated to the race's
 * register).  With one use less, &race loses $s6 to courseName ($s6 / $s7
 * swapped).  Tried: a cast, a RacePS2Base pointer, do { } while (0) around
 * the menu call or around virtual_8 (no change or worse). */
// FUNC 0x0012FF48 run_free_run
int run_free_run(MenuArgs *args, int gtMode)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    GarageCar *gcar = getGameStatus()->getMenuStatus()->raceCar[0];
    const char *raceName = args->race;
    const char *courseName = args->course;
    select_race(raceName);
    {
        ShortMenuArgs res;                              /* sp+0x000 */
        /* implicit: ShortMenuResultEntry::clear, ShortMenuRankEntry::clear (the array loops of ShortMenuArgs()) */
        RaceFreeRunInformation info;                    /* sp+0x190 */

        if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
            CarInformation &car = *gcar;                /* GarageCar +0x10 (no NULL test: a reference) */
            car.presetCar = 1;
            car.setColorIndex(args->color1);
            if (args->tyre1 != 0)
                car.F_00236758();                       /* TIRE_KIND_ARCADE_RACING tyres */
            else
                car.F_00236718();                       /* TIRE_KIND_ARCADE tyres */
            res.setNoSetting(1);
        } else {
            CarInformation &car = *gcar;
            car.presetCar = 0;
            res.setNoSetting(0);
        }
        info.setEquipments(*gcar);                      /* by value: a CarEquipments copy at sp+0x3F0 */

        ::GameOptions *opt = getGameStatus()->getGameOptions();
        info.entry.vibration = opt->vibration1p;
        info.entry.tyre = args->tyre1;
        info.entry.transmission = args->transmission1;
        info.setRaceCode(nameToCode(raceName));
        info.setCourseCode(nameToCode(courseName));     /* direct: RaceInformation's */
        RaceOrganization *org = race_organization_create(&info);
        u64 carCode = gcar->carCode;
        u64 courseCode = info.getCourseCode();          /* direct: RaceInformation's */
        res.setRaceInfo(&info);
        {
            RaceFreeRun race(courseName);               /* sp+0x5E0 */
            race.org = org;

            ShortMenuParams params = arcadeMenus[ARCADE_MENU_FREE_RUN];  /* sp+0x3F0 */
            int distance = 0;                           /* driven in the garage car */
            int loop = 1;
            const char *startPage = NULL;               /* opened over the menu once ("ghost_dlg") */
            do {
                RaceResult result;                      /* sp+0x400 (below the lines) */
                {
                    ShortMenuResultEntry line;          /* sp+0x410 */
                    ShortMenuRankEntry rankLine;        /* sp+0x430 */
                    /* implicit: ShortMenuResultEntry::clear, ShortMenuRankEntry::clear (their constructors) */
                    FreeRunRecords *records = getGameStatus()->getFreeRunRecords();
                    for (int i = 0; i < RECORD_LINES_SHOWN; i++) {
                        line.car = records->getCar(courseName, i);
                        res.list[i] = line;
                    }
                    for (int i = 0; i < RECORD_LINES_SHOWN; i++) {
                        rankLine.time = records->getTime(courseName, i);
                        res.rank[i] = rankLine;
                    }
                }
                org->backgroundMode = 1;
                race.virtual_8(RACE_DEMO);              /* direct: RaceFreeRun's */
                int action = run_short_menu(&params, &res, &race, NULL, startPage);
                startPage = NULL;
                if (res.getGhostAsk() != 0 && res.getYes() != 0) {    /* "yes" to the ghost question */
                    org->setupGhost(1, 0);
                    res.setGhostAsk(0);
                    res.setYes(0);
                }
                race.virtual_35();                      /* direct: RaceSolitaire's */
                org->backgroundMode = 0;

                switch (action) {
                case QM_GO_RACE: {
                    race_option_from_game_options(&org->option);
                    if (res.getNoSetting() == 0)
                        race_set_entry_condition(gcar, org, 0);
                    if (res.getNoSetting() != 0) {
                        org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                        org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
                    }
                    /* 1 manual (the two calls are cross-jumped into one) */
                    if (gtMode != 0)
                        org->setEntryTransmission(getGameStatus()->getGameOptions()->gearAuto ^ 1, 0);
                    else
                        org->setEntryTransmission(args->transmission1, 0);
#if ENH_MENU_FREE_RUN_ASSISTS
                    /* Lupo 2 (L2 0x0012A9AC..0x0012A9E8; menus_a.md 8): the
                     * options' ASM / TCS for every car, a garage car too
                     * (GT3: only for the arcade cars, above) */
                    org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                    org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
#endif
                    /* Lupo 2 (L2 0x0012AA24; menus_a.md 8): a CVT runs
                     * automatic */
                    if (gcar->isCVT() != 0)
                        org->setEntryTransmission(0, 0);
                    org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
                    org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
                    race.virtual_41(RACE_LIVE, 1);      /* the race (direct: RacePS2Base's) */
                    if (res.getNoSetting() == 0)
                        distance += race_get_entry_condition(gcar, org, 0);
                    org->entries->sortByRank();
                    result = org->result;
                    res.setCanReplay(1);
                    if (result.beatGhost != 0 && getGameStatus()->getGameOptions()->ghost == 0) {
                        res.setGhostAsk(1);
                        res.setYes(0);
                        startPage = "ghost_dlg";
                    }
                    if (result.lapRecord != 0) {
                        org->F_0017AD28();
                        res.setCanLogger(1);
                        res.setCanSaveGhost(1);
                    }
                    u32 bestLap = car_best_lap(org->entries->cars[0]->state);
                    if (bestLap <= RECORD_TIME_MAX) {
                        int place = getGameStatus()->getFreeRunRecords()->insertTime(courseName, bestLap);
                        if (place >= 0) {
                            char name[0x40];            /* sp+0x410 (the code follows at sp+0x450) */
                            char code[0x20];            /* sp+0x450 */
                            name_entry(name);
                            getGameStatus()->getFreeRunRecords()->setName(courseName, name, place);
                            getGameStatus()->getFreeRunRecords()->setCar(courseName, carCode, place);
                            make_ranking_code(code, bestLap, gcar, courseCode, RANKING_KIND_FREE_RUN, name);
                            getGameStatus()->getFreeRunRecords()->setCode(courseName, code, place);
                        }
                    }
                }
                    /* fall through: the replay */
                case QM_REPLAY:
                    race.virtual_41(RACE_REPLAY, 1);
                    break;
                case QM_RECORDS: {
                    gt2k_menu::RecordsMenuArgFreeTT records(courseName);   /* sp+0x410 */
                    run_records_menu(&records);
                    break;
                }
                case QM_SETTING:
                    if (setting_menu(courseCode, org) != 0) {
                        /* the setting changed: the car again, no replay, ghost or logger */
                        CarInformation &car = *gcar;    /* retail keeps gcar + 0x10 in $fp from the car test */
                        org->setEntryInfo(0, &car);
                        org->unloadModels(0);
                        res.setCanLogger(0);
                        res.setCanSaveGhost(0);
                        res.setCanReplay(0);
                    }
                    break;
                case QM_LOAD_GHOST:
                    race_memcard(gt2k_menu::MEMCARD_GHOST_LOAD, courseCode, carCode, raceName, org);
                    break;
                case QM_SAVE_GHOST:
                    race_memcard(gt2k_menu::MEMCARD_GHOST_SAVE, courseCode, carCode, raceName, org);
                    break;
                case QM_LOGGER:
                    logger_run(org, 1);
                    break;
                case QM_OPTION:
                    F_0013FE58();
                    break;
                case QM_EXIT:
                    loop = 0;
                    break;
                }
            } while (loop != 0);

            delete org;                                 /* virtual destructor, slot 0 (vptr +0x8C0) */
            getGameStatus()->incDistance(distance);
        }                                               /* ~RaceFreeRun */
    }                                                   /* ~RaceFreeRunInformation */
    race_end();
    return 0;
}

/* Time trial (arcade menu state 3, runMode 0): RaceTimeTrial with
 * "menu/qm_arc_tt_ltd.mbl".  The race name gives a test index
 * (race_name_split) whose table of GameStatus::timeTrial feeds the rank
 * list and takes new records; the car is always an arcade car (colour only,
 * the tyres are left) and the course is the race's own (no setCourseCode).
 * The ghost question is answered "yes" at once when options.ghost is set.
 * QM_DEMO plays "demo/<race>" as a replay.  A new best time (place 0) runs
 * the prize-car check F_0013F508 after the session.  Frame 0x49140: kind
 * sp+0x49060, index sp+0x49064, args sp+0x49068, gcar sp+0x4906C, topRecord
 * sp+0x49070, carCode sp+0x49078, courseCode sp+0x49080, loop sp+0x49088,
 * startPage sp+0x4908C, &params sp+0x49090, &info sp+0x49094 (gcar is
 * declared before topRecord: the spill slots follow the pseudo numbers).
 * Not exact (99.3%): &info / &params take the two spill slots the other
 * way round: GCSE creates their PRE registers in hash-bucket order, (plus
 * fp 400) bucket 25 before (plus fp 1008) bucket 203 in a 215-bucket table;
 * a table of 213 or 217 (2 to 4 insns more or fewer at GCSE time) gives
 * retail's order.  Tried: the `&&' form of the two bestLap tests, kind = 0,
 * the records through getGameStatus() each time, no options pointer (other
 * code or no change).  And run_short_menu's &race is rematerialised (as in
 * run_free_run; QM_DEMO's virtual_41 `this' is rematerialised in retail
 * too). */
// FUNC 0x00130A50 run_time_trial
int run_time_trial(MenuArgs *args)
{
    GarageCar *gcar;
    int topRecord = 0;                                  /* a new first place */
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    gcar = getGameStatus()->getMenuStatus()->raceCar[0];
    const char *raceName = args->race;
    select_race(raceName);
    int kind;                                           /* RACE_KIND_TIME_TRIAL (not checked) */
    int index = -1;                                     /* the test: its record table */
    race_name_split(raceName, &kind, &index);
    {
        ShortMenuArgs res;                              /* sp+0x000 */
        /* implicit: ShortMenuResultEntry::clear, ShortMenuRankEntry::clear (the array loops of ShortMenuArgs()) */
        RaceSolitaireInformation info;                  /* sp+0x190 */

        CarInformation &car = *gcar;                    /* GarageCar +0x10 (no NULL test: a reference) */
        car.presetCar = 1;
        car.setColorIndex(args->color1);
        res.setNoSetting(1);
        info.setEquipments(car);                        /* by value: a CarEquipments copy at sp+0x3F0 */

        ::GameOptions *opt = getGameStatus()->getGameOptions();
        info.entry.vibration = opt->vibration1p;
        info.entry.tyre = args->tyre1;
        info.entry.transmission = args->transmission1;
        info.setRaceCode(nameToCode(raceName));
        RaceOrganization *org = race_organization_create(&info);
        {
            RaceTimeTrial race(index);                  /* sp+0x5E0 */
            race.org = org;
            u64 carCode = gcar->carCode;
            u64 courseCode = info.getCourseCode();      /* direct: RaceInformation's */
            res.setRaceInfo(&info);

            ShortMenuParams params = arcadeMenus[ARCADE_MENU_TIME_TRIAL];    /* sp+0x3F0 */
            int loop = 1;
            const char *startPage = NULL;               /* opened over the menu once ("ghost_dlg") */
            do {
                RaceResult result;                      /* sp+0x400 (below the line) */
                {
                    ShortMenuRankEntry rankLine;        /* sp+0x410 */
                    /* implicit: ShortMenuRankEntry::clear (its constructor) */
                    TimeTrialRecords *records = getGameStatus()->getTimeTrialRecords();
                    for (int i = 0; i < RECORD_LINES_SHOWN; i++) {
                        rankLine.time = records->getTime(index, i);
                        rankLine.name = records->getName(index, i);
                        res.rank[i] = rankLine;
                    }
                }
                org->backgroundMode = 1;
                race.virtual_8(RACE_DEMO);              /* direct: RaceTimeTrial's */
                int action = run_short_menu(&params, &res, &race, NULL, startPage);
                startPage = NULL;
                if (res.getGhostAsk() != 0 && res.getYes() != 0) {    /* "yes" to the ghost question */
                    org->setupGhost(0, 0);
                    res.setGhostAsk(0);
                    res.setYes(0);
                }
                race.virtual_35();                      /* direct: RaceSolitaire's */
                org->backgroundMode = 0;

                switch (action) {
                case QM_GO_RACE: {
                    race_option_from_game_options(&org->option);
                    org->setEntryTransmission(args->transmission1, 0);
                    org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
                    org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
                    race.virtual_41(RACE_LIVE, 1);      /* the race (direct: RacePS2Base's) */
                    org->entries->sortByRank();
                    result = org->result;
                    res.setCanReplay(1);
                    if (result.beatGhost != 0) {
                        res.setGhostAsk(1);
                        if (getGameStatus()->getGameOptions()->ghost != 0) {
                            res.setYes(1);                /* taken without asking */
                            startPage = NULL;
                        } else {
                            res.setYes(0);
                            startPage = "ghost_dlg";
                        }
                    }
                    if (result.lapRecord != 0) {
                        org->F_0017AD28();
                        res.setCanLogger(1);
                        res.setCanSaveGhost(1);
                    }
                    u32 bestLap = car_best_lap(org->entries->cars[0]->state);
                    if (bestLap != RACE_TIME_INVALID) { /* redundant with the next test, as in the original */
                        if (bestLap <= RECORD_TIME_MAX) {
                            int place = getGameStatus()->getTimeTrialRecords()->enterTime(index, bestLap);
                            if (place == 0)
                                topRecord = 1;
                            if (place >= 0) {
                                char name[0x40];        /* sp+0x410 (the code follows at sp+0x450) */
                                RankingCode code;       /* sp+0x450 */
                                name_entry(name);
                                getGameStatus()->getTimeTrialRecords()->insertName(index, name, place);
                                make_ranking_code(code.s, bestLap, gcar, courseCode, RANKING_KIND_TIME_TRIAL, name);
                                getGameStatus()->getTimeTrialRecords()->insertCode(index, &code, place);
                            }
                        }
                    }
                    /* the replay: cross-jumped into QM_REPLAY's call (QM_DEMO's
                     * block lies between them) */
                    race.virtual_41(RACE_REPLAY, 1);
                    break;
                }
                case QM_DEMO: {
                    /* "demo/<race>" plays as a replay; the player's transmission is kept */
                    int transmission = org->entries->cars[0]->transmission;
                    org->demonstration = 1;
                    load_demo_setting(&race, raceName); /* the result is not used */
                    race.virtual_41(RACE_REPLAY, 1);
                    org->entries->cars[0]->transmission = transmission;
                    res.setCanReplay(0);
                    res.setGhostAsk(0);
                    res.setYes(0);
                    org->demonstration = 0;
                    break;
                }
                case QM_REPLAY:
                    race.virtual_41(RACE_REPLAY, 1);
                    break;
                case QM_RECORDS: {
                    /* the inline constructor: index, vptr 0x00324F38 */
                    gt2k_menu::RecordsMenuArgCompulsory records(index);    /* sp+0x410 */
                    run_records_menu(&records);
                    break;
                }
                case QM_LOAD_GHOST:
                    race_memcard(gt2k_menu::MEMCARD_GHOST_LOAD, courseCode, carCode, raceName, org);
                    break;
                case QM_SAVE_GHOST:
                    race_memcard(gt2k_menu::MEMCARD_GHOST_SAVE, courseCode, carCode, raceName, org);
                    break;
                case QM_LOGGER:
                    logger_run(org, 1);
                    break;
                case QM_OPTION:
                    F_0013FE58();
                    break;
                case QM_EXIT:
                    loop = 0;
                    break;
                }
            } while (loop != 0);

            delete org;                                 /* virtual destructor, slot 0 (vptr +0x8C0) */
        }                                               /* ~RaceTimeTrial */
    }                                                   /* ~RaceSolitaireInformation */
    race_end();
    if (topRecord != 0)
        F_0013F508(raceName, -1);
    return 0;
}

/* The empty line of the rank list: time RACE_TIME_INVALID, no name, no
 * highlight, speed 0.  Every ShortMenuRankEntry() calls this copy (the
 * first linked: T053's 0x00133DC8 and T054's 0x001345F0 are unreferenced). */
// FUNC 0x001313D0 ShortMenuRankEntry::clear
/* inline in game/app/T052_arcade_race_modes.h; egcs emits this copy */

/* Unreferenced: T052's copy of ShortMenuResultEntry::clear (T102's
 * 0x0012F820 is the one linked): the empty line of the result list (car 0,
 * points 0, time RACE_TIME_INVALID, no highlight; pad14 is not written). */
// FUNC 0x001313F0 ShortMenuResultEntry::clear
/* inline in game/menu/race_menu/T102_race_menu_helpers.h; egcs emits this copy */
