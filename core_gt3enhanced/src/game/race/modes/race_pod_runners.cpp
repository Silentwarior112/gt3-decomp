/* race_pod_runners.cpp - the runners of the Toyota Pod race (the enhanced
 * engine; new code, no GT3 original: Lupo 2's app code L2 0x0012E018 /
 * 0x00130CB8, menus_a.md 9).  Declared in include/game/race/modes/
 * race_pod.h.
 *
 * Each is GT3's runner of the same shape around the Pod race class: the
 * arcade single race (run_arcade_race, T052) with RacePOD, the split battle
 * (run_split_battle, T054) with RacePOD2P.  Lupo 2's own runners follow GT
 * Concept's arcade flow (its running car, the race behind its short menu,
 * the setting menu, the rewards, the kiosk idle time limit); GT3's flow is
 * kept, as for every GT3 arcade race.  Nothing of GT3 calls them: the arcade
 * menu's Pod entry (menus area, menus_a.md 5 / 14) does, with the Pod
 * race's record in args->race and its course in args->course. */
#include <new>                              /* (the race classes' headers) */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"
#include "game/race/display/T154_race_display.h"
#include "game/race/pause/T169_race_pause.h"
#include "game/race/pause/T170_replay_pause.h"
#include "game/race/modes/race_pod.h"       /* RacePOD, RacePOD2P, run_pod_race, run_pod_2p_race */
#include "game/app/T000_main.h"                 /* MenuArgs, RaceEntry */
#include "game/app/T007_game_init.h"            /* race_begin, race_end */
#include "game/menu/gtmode/T036_garage_car.h"           /* GarageCar */
#include "game/menu/gtmode/T056_trophy_prize_entry.h"   /* trophy_prize_screen */
#include "game/menu/options/T062_option_control.h"       /* F_0013FE58 (the options menu) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* gt2k_menu::MEMCARD_REPLAY_SAVE, F_0014AE00 */
#include "game/app/T052_arcade_race_modes.h"    /* ShortMenuRankEntry */
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* ShortMenuArgs / Params, QM_*, run_short_menu,
                                               race_entry_rank, result_list_by_rank, race_memcard,
                                               select_race, MemCardRaceInfo, BlockPool */
#include "game/menu/race_menu/T115_race_option.h"          /* race_option_from_game_options */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/race/core/T123_race_setup.h"           /* race_organization_create */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/race/core/T129_race_base.h"            /* RACE_LIVE, RACE_REPLAY */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base::virtual_41 */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (finished) */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/save/T428_game_options.h"         /* GameOptions */
#include "engine/pdistd/T471_unit_arena.h"           /* _UnitArenaBase (BlockPool) */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T483_car_information.h"      /* CarInformation */
#include "game/data/T484_race_database.h"        /* RaceDataBase, RaceEvent */
#include "game/data/T489_car_database.h"         /* race_kind_text, RACE_KIND_2P_BATTLE */
#include "game/app/arcade_race_modes_internal.h"   /* CONTROL_RECORD_SIZE / CONTROL_RECORDS, RACE_CAR_BY_CODE */

/* The result menus (as the arcade race's and the split battle's: Lupo 2's
 * Pod runners show GT Concept's). */
static ShortMenuParams podResultMenu = { "menu/qm_arc_result.mbl", "root", 0x16 };
static ShortMenuParams pod2PResultMenu = { "menu/qm_arc_2p_result.mbl", "root", 0x16 };

/* A player's own assist level from the car select (MenuArgs::assist1 / 2,
 * menus_a.md 17) to the entry's ASM and TCS (GameOptions::getAssistPreset;
 * Lupo 2's L2 0x00161770); MENU_ASSIST_OPTIONS leaves the options' flags
 * set before it.  As the split battle's (T054's set_entry_assist). */
static void pod_entry_assist(RaceOrganization *org, int level, int entry)
{
    int tcs = 1;
    int asmOn = 1;

    if (level == MENU_ASSIST_OPTIONS)
        return;
    ::GameOptions::getAssistPreset(level, &tcs, &asmOn);
    org->setEntryAsm(asmOn, entry);
    org->setEntryTcs(tcs, entry);
}

/* The Pod race (Lupo 2 L2 0x0012E018): run_arcade_race's session with
 * RacePOD - the first race at once, then its replay and the result menu;
 * a win shows the trophy screen.  No arcade record and no prize-car check
 * (Lupo 2 has neither for the Pod race).  An arcade car takes the player's
 * assist level, a CVT runs automatic (as run_arcade_race / run_split_battle
 * in the enhanced engine); the short menu hides "load_ghost" and
 * "demonstration" (Lupo 2's arcade shows them for no race).  Returns 0. */
int run_pod_race(MenuArgs *args)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    gt2k_game::MenuStatus *menu = getGameStatus()->getMenuStatus();
    const char *courseName = args->course;
    const char *raceName = args->race;
    GarageCar *gcar = menu->raceCar[0];
    select_race(raceName);

    ShortMenuArgs res;
    {
        RaceArcadeInformation info;

        if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
            /* an arcade car: the menu's colour and tyres, no car setting */
            CarInformation &car = *gcar;
            car.presetCar = 1;
            car.setColorIndex(args->color1);
            if (args->tyre1 != 0)
                car.F_00236758();
            else
                car.F_00236718();
            res.setNoSetting(1);
        } else {
            CarInformation &car = *gcar;
            car.presetCar = 0;
            res.setNoSetting(0);
        }
        res.setNoLoadGhost(1);
        res.setNoDemo(1);
        info.setEquipments(*gcar);

        int action = QM_GO_RACE;                        /* the first race without the menu */
        ::GameOptions *opt = getGameStatus()->getGameOptions();
        info.entry.vibration = opt->vibration1p;
        info.entry.tyre = args->tyre1;
        info.entry.transmission = args->transmission1;
        info.setRaceCode(nameToCode(raceName));
        info.setCourseCode(nameToCode(courseName));
        RaceOrganization *org = race_organization_create(&info);
        u64 carCode = gcar->carCode;
        u64 courseCode = info.getCourseCode();
        res.setRaceInfo(&info);
        {
            RacePOD race;
            race.org = org;

            ShortMenuParams params;
            int distance = 0;                           /* driven in the garage car */
            int loop = 1;
            do {
                if (action != QM_GO_RACE)
                    action = run_short_menu(&params, &res, NULL, NULL, NULL);
                switch (action) {
                case QM_TRY_AGAIN:
                case QM_GO_RACE:
                    race_option_from_game_options(&org->option);
                    if (res.getNoSetting() == 0)
                        race_set_entry_condition(gcar, org, 0);
                    if (res.getNoSetting() != 0) {
                        org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                        org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
                        pod_entry_assist(org, args->assist1, 0);
                    }
                    org->setEntryTransmission(args->transmission1, 0);
                    if (gcar->isCVT() != 0)                 /* a CVT runs automatic (menus_a.md 6 b) */
                        org->setEntryTransmission(0, 0);
                    org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
                    org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
                    race.virtual_41(RACE_LIVE, 1);
                    if (res.getNoSetting() == 0)
                        distance += race_get_entry_condition(gcar, org, 0);
                    res.setCanReplay(1);
                    if (org->entries->cars[0]->state->st.finished != 0) {
                        org->entries->sortByRank();
                        int rank = race_entry_rank(org, 0);
                        int trophy = org->raceDB->event->trophyType;
                        if (rank == 0)
                            trophy_prize_screen(0, 0, trophy, 0);
                        result_list_by_rank(org, &res, 1);
                    } else {
                        for (int i = 0; i < 6; i++)
                            res.list[i].clear();
                    }
                    race.virtual_41(RACE_REPLAY, 1);
                    params = podResultMenu;
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
                default:                                /* QM_EXIT and the others */
                    loop = 0;
                    break;
                }
            } while (loop != 0);

            delete org;
            getGameStatus()->incDistance(distance);
        }                                               /* ~RacePOD */
    }                                                   /* ~RaceArcadeInformation */
    race_end();
    return 0;
}

/* The 2-player Pod race (Lupo 2 L2 0x00130CB8): run_split_battle's session
 * with RacePOD2P on the Pod race's record args->race (GT3's split battle has
 * the fixed "yotsuya001") and course args->course; each player's own assist
 * level (Lupo 2), a CVT runs automatic, no "load_ghost" / "demonstration".
 * Returns 0. */
int run_pod_2p_race(MenuArgs *args)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    GarageCar *gcar1 = getGameStatus()->getMenuStatus()->raceCar[0];
    GarageCar *gcar2 = getGameStatus()->getMenuStatus()->raceCar[1];
    const char *courseName = args->course;
    const char *raceCode = args->race;

    RaceSplitBattleInformation info;
    info.setRaceCode(nameToCode(raceCode));
    info.setCourseCode(nameToCode(courseName));

    if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
        CarInformation &car = *gcar1;
        car.presetCar = 1;
        car.setColorIndex(args->color1);
        if (args->tyre1 != 0)
            car.F_00236758();
        else
            car.F_00236718();
    } else {
        CarInformation &car = *gcar1;
        car.presetCar = 0;
    }
    RaceEntry *entry1 = info.getEntry(0);
    entry1->equip = *gcar1;
    entry1->transmission = args->transmission1;
    entry1->tyre = args->tyre1;
    entry1->vibration = getGameStatus()->getGameOptions()->vibration1p;

    if (getGameStatus()->getMenuStatus()->raceCarKind[1] == RACE_CAR_BY_CODE) {
        CarInformation &car = *gcar2;
        car.presetCar = 1;
        car.setColorIndex(args->color2);
        if (args->tyre2 != 0)
            car.F_00236758();
        else
            car.F_00236718();
    } else {
        CarInformation &car = *gcar2;
        car.presetCar = 0;
    }
    RaceEntry *entry2 = info.getEntry(1);
    entry2->equip = *gcar2;
    entry2->transmission = args->transmission2;
    entry2->tyre = args->tyre2;
    entry2->vibration = getGameStatus()->getGameOptions()->vibration2p;

    RaceOrganization *org = race_organization_create(&info);
    ShortMenuArgs menuArgs;
    menuArgs.setNoLoadGhost(1);
    menuArgs.setNoDemo(1);
    RacePOD2P race;
    race.org = org;
    int action = QM_GO_RACE;                            /* the first race without the menu */
    int loop = 1;
    ShortMenuParams params;
    MemCardRaceInfo memInfo;

    do {
        if (action != QM_GO_RACE)
            action = run_short_menu(&params, &menuArgs, NULL, NULL, NULL);

        switch (action) {
        case QM_EXIT:
            loop = 0;
            break;

        case QM_REPLAY:
            race.virtual_41(RACE_REPLAY, 1);
            break;

        case QM_TRY_AGAIN:
        case QM_GO_RACE:
            race_option_from_game_options(&org->option);
            if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
                org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
                pod_entry_assist(org, args->assist1, 0);
            }
            org->setEntryTransmission(args->transmission1, 0);
            if (gcar1->isCVT() != 0)
                org->setEntryTransmission(0, 0);
            org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
            org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
            if (getGameStatus()->getMenuStatus()->raceCarKind[1] == RACE_CAR_BY_CODE) {
                org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 1);
                org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 1);
                pod_entry_assist(org, args->assist2, 1);
            }
            org->setEntryTransmission(args->transmission2, 1);
            if (gcar2->isCVT() != 0)
                org->setEntryTransmission(0, 1);
            action = 0;                                 /* the menu next */
            org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist2p, 1);
            org->setEntryVibration(getGameStatus()->getGameOptions()->vibration2p, 1);
            race.virtual_41(RACE_LIVE, 1);
            result_list_by_rank(org, &menuArgs, 0);
            menuArgs.setCanReplay(1);
            race.virtual_41(RACE_REPLAY, 1);
            params = pod2PResultMenu;
            break;

        case QM_OPTION:
            F_0013FE58();
            break;

        case QM_SAVE_REPLAY: {
            /* as the split battle's (race_memcard's work for its race kind) */
            memInfo.licenceName = race_kind_text(RACE_KIND_2P_BATTLE);
            memInfo.test = -1;
            memInfo.licence = RACE_KIND_2P_BATTLE;
            BlockPool *arena = org->unloadCourseData();
            void *work = arena->alloc(0);
            int workSize = arena->unitSize;
            F_0014AE00(gt2k_menu::MEMCARD_REPLAY_SAVE, org->raceDB->getCourse(),
                       0xFFFFFFFFu /* no car code: two cars */,
                       &memInfo, work, workSize, memInfo.licence, memInfo.test, NULL);
            arena->release(work);
            break;
        }
        }
    } while (loop != 0);

    delete org;
    race_end();
    return 0;
}
