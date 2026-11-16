/* T054 - the 2-player split screen battle, 0x00133F48..0x00134630.
 *
 * Reached from the arcade menu (main_arcade, selection 4 "two players",
 * runMode 1) with its MenuArgs: the course, and per player the car colour,
 * the tyre choice and the transmission.  The cars are GameStatus::menu
 * .raceCar[0] / [1].  T054 also holds unreferenced copies of the two
 * ShortMenuArgs entry lines' inline clear().
 *
 * The includes, in the original's class order (reverse definition order in
 * .rodata; retail T054's 0x003164F8..0x003168C6): "bad_alloc" first (<new>,
 * retail 0x00316490, although T054 has no new), then the GameStatus's
 * garage, RecordsMenuArg, RegurationResult, UnitArena<64>, the race camera
 * and displays, the menu classes, the race menus, RaceBattleInformation<2>
 * / RaceBattle<2>, SmartIndexT<int> and ShortmenuControl last (T000's
 * order).  The race headers after them add nothing new; the memory card
 * screen through its entry header.  T102_race_menu_helpers.h defines
 * ShortMenuResultEntry before ShortMenuRankEntry (T052's header), so the
 * clear() copies come out RankEntry's first (retail 0x001345F0 /
 * 0x00134610). */
#include "game/app/gtmode_races_internal.h"
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus; GarageCar */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
/* UnitArena<64> instantiated by T180's g_002C2F50, with its type name
 * (T153_race_course.h leaves it out unless asked). */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"
#include "game/car/model/T180_race_car_model.h"
#include "game/camera/T383_race_camera_base.h"     /* RaceCameraBase */
#include "game/race/display/T154_race_display.h"         /* the race displays (their T157's "" opens the .sdata) */
#include "game/menu/framework/T216_menu_transitive.h"      /* the menu classes */
#include "game/menu/race_menu/T198_race_menu_button.h"
#include "game/menu/race_menu/T199_race_menu_layer.h"
#include "game/race/pause/T169_race_pause.h"
#include "game/race/pause/T170_replay_pause.h"
#include "game/menu/records/T200_result_board.h"
#include "game/race/modes/T137_race_split_battle.h"    /* RaceSplitBattle, RaceSplitBattleInformation */
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* (no use: SmartIndexT<int>, ShortmenuControl) */
#include "game/app/T054_split_battle.h"
#include "game/app/T000_main.h"                 /* MenuArgs, RaceEntry */
#include "game/app/T007_game_init.h"            /* race_begin, race_end */
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* ShortMenuArgs / Params, the T052 rank lines,
                                               MemCardRaceInfo, BlockPool, QM_*, run_short_menu,
                                               result_list_by_rank */
#include "game/menu/options/T062_option_control.h"       /* F_0013FE58 (the options menu) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, gt2k_menu::MEMCARD_REPLAY_SAVE */
#include "game/menu/race_menu/T115_race_option.h"          /* race_option_from_game_options */
#include "game/race/core/T123_race_setup.h"           /* race_organization_create */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/race/core/T129_race_base.h"            /* RACE_LIVE, RACE_REPLAY */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (virtual_41) */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus */
#include "game/save/T428_game_options.h"         /* ::GameOptions */
#include "engine/pdistd/T471_unit_arena.h"           /* _UnitArenaBase (BlockPool) */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments */
#include "game/data/T483_car_information.h"      /* CarInformation */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourse */
#include "game/data/T489_car_database.h"         /* race_kind_text, RACE_KIND_2P_BATTLE */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* GCC 14's typeinfo functions (egcs's <new> and cxxrt.h
                                               conflict) */
#endif

/* T054's data starts unreferenced: its .rodata with <new>'s "bad_alloc" and
 * splitResultMenu's file name, its .sdata with T157's "" and
 * splitResultMenu's "root" (the code references only "yotsuya001" and the
 * jump table). */
// DATA_START 0x00316490
// DATA_START 0x00351528

/* The result menu of the battle: {file, first page, menu music}, T054's
 * .data (writable, as retail), copied to the session's ShortMenuParams
 * after each race. */
static ShortMenuParams splitResultMenu = { "menu/qm_arc_2p_result.mbl", "root", 0x16 };   /* 0x0029D9B0 */

/* The split screen battle: a RaceSplitBattleInformation for the race code
 * "yotsuya001" (the battle's race record) on the course args->course with
 * the two cars (an arcade car, raceCarKind RACE_CAR_BY_CODE, gets the menu's
 * colour and tyres first; a garage car is not a preset car) and their
 * transmission, tyre and vibration settings; a RaceSplitBattle under the
 * short menu.  The first pass of the loop races at once (the action starts
 * as QM_GO_RACE); after each race the replay plays, then the menu is
 * qm_arc_2p_result.mbl with the results (no race behind it).  "save replay"
 * saves through F_0014AE00 with the course arena's unit as the work buffer
 * (race_memcard's code, expanded with the race kind of the battle).  No
 * second argument (main_arcade's $a1 is its runMode store's register).
 * Stack: information sp+0x10, menu arguments sp+0x480, the race sp+0x610,
 * the spilled &info / &menuArgs (GCSE copies of the addresses) sp+0x3F2B4 /
 * sp+0x3F2B8.  Returns 0. */
// FUNC 0x00133F48 run_split_battle
int run_split_battle(MenuArgs *args)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    GarageCar *gcar1 = getGameStatus()->getMenuStatus()->raceCar[0];   /* s3 */
    GarageCar *gcar2 = getGameStatus()->getMenuStatus()->raceCar[1];   /* s4 */
    getGameStatus();                                        /* a third call, its result unused */
    const char *courseName = args->course;                  /* s2 */
    const char *raceCode = "yotsuya001";                    /* s0, loaded before info's constructor */

    RaceSplitBattleInformation info;                        /* sp+0x00010 */
    info.setRaceCode(nameToCode(raceCode));
    info.setCourseCode(nameToCode(courseName));             /* direct: RaceInformation's */

    /* Each arm converts the garage car to its CarInformation (+0x10) itself:
     * GCSE hoists the conversion to the end of the test's block (the bnez
     * delay slot), and the operator= below reuses it.  presetCar is a bool
     * (T482): the load of args->color1 is scheduled ahead of its store. */
    if (getGameStatus()->getMenuStatus()->raceCarKind[0] == RACE_CAR_BY_CODE) {
        CarInformation &car = *gcar1;                       /* no NULL test: a reference */
        car.presetCar = 1;
        car.setColorIndex(args->color1);
        if (args->tyre1 != 0)
            car.F_00236758();                               /* tyres of kind 0xB (? arcade racing) */
        else
            car.F_00236718();                               /* tyres of kind 0xA (? arcade) */
    } else {
        CarInformation &car = *gcar1;
        car.presetCar = 0;
    }
    RaceEntry *entry1 = info.getEntry(0);                   /* s0 */
    entry1->equip = *gcar1;                                 /* CarEquipments::operator= */
    /* implicit: operator= (CarEquipments::operator=, both entries) */
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
    RaceEntry *entry2 = info.getEntry(1);                   /* s0 (&info stays in a register, spilled
                                                               to sp+0x3F2B4) */
    entry2->equip = *gcar2;                                 /* CarEquipments::operator= */
    entry2->transmission = args->transmission2;
    entry2->tyre = args->tyre2;
    entry2->vibration = getGameStatus()->getGameOptions()->vibration2p;

    RaceOrganization *org = race_organization_create(&info);    /* s3 */
    ShortMenuArgs menuArgs;                                 /* sp+0x00480 (fp): ShortMenuResultEntry() x 6,
                                                               ShortMenuRankEntry() x 12 */
    /* implicit: ShortMenuResultEntry::clear, ShortMenuRankEntry::clear (the entry
       constructors of ShortMenuArgs' arrays call them) */
    RaceSplitBattle race;                                   /* sp+0x00610 (s7) */
    race.org = org;
    int action = QM_GO_RACE;                                /* s5: the first race without the menu */
    int loop = 1;                                           /* sp+0x3F2B0 */
    ShortMenuParams params;                                 /* sp+0x3F290 (set after the first race) */
    MemCardRaceInfo memInfo;                                /* sp+0x3F2A0 */

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
                /* an arcade car: the assists of the options */
                org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 0);
                org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 0);
            }
            org->setEntryTransmission(args->transmission1, 0);
            org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist1p, 0);
            org->setEntryVibration(getGameStatus()->getGameOptions()->vibration1p, 0);
            if (getGameStatus()->getMenuStatus()->raceCarKind[1] == RACE_CAR_BY_CODE) {
                /* (player 2 gets the same 1P assist options) */
                org->setEntryAsm(getGameStatus()->getGameOptions()->assistAsm, 1);
                org->setEntryTcs(getGameStatus()->getGameOptions()->assistTcs, 1);
            }
            org->setEntryTransmission(args->transmission2, 1);
            action = 0;                                     /* the menu next */
            org->setEntrySteeringAssist(getGameStatus()->getGameOptions()->steeringAssist2p, 1);
            org->setEntryVibration(getGameStatus()->getGameOptions()->vibration2p, 1);
            race.virtual_41(RACE_LIVE, 1);                  /* the race (direct: RacePS2Base's) */
            result_list_by_rank(org, &menuArgs, 0);
            menuArgs.setCanReplay(1);                       /* (&menuArgs in a GCSE register, sp+0x3F2B8) */
            race.virtual_41(RACE_REPLAY, 1);
            params = splitResultMenu;
            break;

        case QM_OPTION:
            F_0013FE58();
            break;

        case QM_SAVE_REPLAY: {
            /* race_memcard's work for the battle's race kind (no index) */
            memInfo.licenceName = race_kind_text(RACE_KIND_2P_BATTLE);  /* "2P Battle" */
            memInfo.test = -1;
            memInfo.licence = RACE_KIND_2P_BATTLE;
            BlockPool *arena = org->unloadCourseData();     /* s1, the course data's arena (0x002BB5B8) */
            void *work = arena->alloc(0);                   /* s4 */
            int workSize = arena->unitSize;                 /* s2 (the field, not getUnitSize()) */
            F_0014AE00(gt2k_menu::MEMCARD_REPLAY_SAVE, org->raceDB->getCourse(),
                       0xFFFFFFFFu /* no car code: two cars (zero-extended to 64 bits) */,
                       &memInfo, work, workSize, memInfo.licence, memInfo.test, NULL);
            arena->release(work);
            break;
        }
        }
    } while (loop != 0);

    delete org;                                             /* virtual destructor, slot 0 (vptr +0x8C0) */
    race_end();
    return 0;
}                                                           /* ~RaceSplitBattle, ~RaceSplitBattleInformation
                                                               (after race_end) */

/* ======================================================================== */
/* T054's out-of-line copies of inline functions (unreferenced)             */
/* ======================================================================== */

// FUNC 0x001345F0 ShortMenuRankEntry::clear
/* ShortMenuRankEntry's inline clear() (its constructor calls it), game/
 * T052_arcade_race_modes.h; egcs emits this copy (T054's):
 * the empty line of the rank list (the calls go to T052's copy
 * 0x001313D0). */

// FUNC 0x00134610 ShortMenuResultEntry::clear
/* ShortMenuResultEntry's inline clear() (its constructor calls it), game/
 * T102_race_menu_helpers.h; egcs emits this copy (T054's):
 * the empty line of the result list (the calls go to T102's copy
 * 0x0012F820; pad14 is not written). */
