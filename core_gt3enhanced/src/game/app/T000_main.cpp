/* T000 - main and the top-level game modes.  0x00100140..0x00101290. */
/* The includes, in the original's class order (egcs writes the type names
 * of the classes it sees in the reverse order of their definitions; retail
 * T000's .rodata 0x00308378..0x00308780): first the headers without such
 * classes, then the GameStatus's garage, RecordsMenuArg, RegurationResult,
 * UnitArena<64>, the race camera and displays, the menu classes, the race
 * menus, the split battles, SmartIndexT<int> and ShortmenuControl last.  The
 * race headers after them add nothing new.  <new> first: its inline what()
 * writes "bad_alloc", the first bytes of the original's .rodata
 * (0x00308280, right before T000's first string; it was taken for another
 * file's while the tools' .rodata started at 0x00308100). */
// DATA_START 0x00308280
#include <new>
#include "game/app/app_internal.h"
#include "libc.h"                           /* memalign, free, printf */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "sdk/libcdvd/G004_libcdvd.h"              /* __main (GCC 14) */
#include "game/app/T000_main.h"
#include "game/app/T007_game_init.h"            /* game_boot, race_begin, race_end */
#include "game/menu/arcade/T015_arcade_menu.h"          /* arcade_menu */
#include "game/menu/gtmode/T038_gtmode_main.h"          /* gtmode_main */
#include "game/data/T050_country.h"              /* set_country */
#include "game/menu/framework/T051_movie_entry.h"          /* play_movie */
#include "game/app/T052_arcade_race_modes.h"    /* run_arcade_race, run_free_run, run_time_trial */
#include "game/app/T053_gtmode_races.h"         /* run_machine_test, run_license_test, run_gtmode_race,
                                               run_championship */
#include "game/app/T054_split_battle.h"         /* run_split_battle */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00 (the memory card screens), its modes */
#include "game/menu/memorycard/T078_memorycard_warning_entry.h" /* memorycard_warning */
#include "game/race/core/T123_race_setup.h"           /* race_organization_create */
#include "game/race/core/T124_race_information.h"     /* RaceInformation */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Manager::hasSlots */
#include "game/data/T435_product_config.h"       /* ProductConfig, product_config */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "engine/system/T473_link_session.h"         /* F_0022D7A0, F_0022D830, F_0022D850 */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T484_race_database.h"        /* RaceDataBase */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult) */
/* UnitArena<64> instantiated by T180's g_002C2F50, with its type name
 * (T153_race_course.h leaves it out unless asked). */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"
#include "game/car/model/T180_race_car_model.h"
#include "game/camera/T383_race_camera_base.h"     /* RaceCameraBase */
#include "game/race/display/T154_race_display.h"         /* the race displays (their T157's "" in the .sdata) */
#include "game/menu/framework/T216_menu_transitive.h"      /* the menu classes */
#include "game/menu/race_menu/T198_race_menu_button.h"
#include "game/menu/race_menu/T199_race_menu_layer.h"
#include "game/race/pause/T169_race_pause.h"
#include "game/race/pause/T170_replay_pause.h"
#include "game/menu/records/T200_result_board.h"
#include "game/race/modes/T137_race_split_battle.h"    /* RaceBattle<2> */
#include "game/race/modes/T138_race_link_battle.h"     /* RaceLinkBattle, RaceLinkBattleInformation */
#include "game/race/core/T129_race_base.h"            /* RaceBase, RaceInfoBlock, g_00351DF0 */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (virtual_41) */
#include "game/race/modes/T135_race_arcade_demo.h"     /* RaceArcadeDemo, RaceArcadeDemoInformation */
#include "game/race/modes/T139_race_solitaire.h"       /* RaceSolitaire (virtual_59) */
#include "game/race/core/T149_race_replay_information.h"  /* RaceReplayInformation */
#include "game/menu/arcade/T010_arcade_control.h"       /* arcade_class_name (SmartIndexT<int>) */
#include "game/debug/T055_logger.h"               /* logger_main */
#include "game/menu/options/T062_option_control.h"       /* F_0013FE58 (the options) */
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* (no use: ShortmenuControl) */
#include "game/enhanced.h"                  /* ENH_MENU_ATTRACT_REPLAYS */
#include "game/race/modes/race_pod.h"       /* the enhanced engine: run_pod_race, run_pod_2p_race (the race area's) */
#include "engine/io/T502_file_resolve.h"         /* the enhanced engine: file_resolve */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/io/T509_serialize.h"            /* serialize_create / destroy, Serialize::Base::restore */
#include "game/race/core/T150_serialize_replay_data.h"    /* g_00351F40 "ReplayData" */

/* .sdata: the attract demo played last (-1 at start), after T157's "". */
// DATA_START 0x00350B08
int g_00350B0C = -1;                        /* 0x00350B0C gp-0x6EE4 */

/* The opening movie (640 x 448, fields, with sound, once).  Inline in the
 * original: retail builds both copies through a pointer to the one
 * MovieParam (sp+0x10) that it keeps in $s1 for the whole loop (only the
 * first store of each copy is folded onto $sp). */
static inline void play_opening(MovieParam *movie)
{
    movie->file      = "stream/opening.pss";
    movie->width     = SCREEN_W;
    movie->height    = SCREEN_H;
    movie->field     = 1;
    movie->withSound = 1;
    movie->oneShot   = 1;
    play_movie(movie);
}

#if ENH_MENU_ATTRACT_REPLAYS
static void run_attract(void);              /* the enhanced engine (below run_attract_demo) */
#endif

/* main: boots the game (game_boot), shows the memory card warning, plays the
 * opening movie, tries the autoload, then loops forever.  While the menus
 * (gtmode_main) return 0, up to five attract demos run (one that does not
 * run to its end restarts the loop), then the opening movie again.
 * Otherwise it waits two vsyncs and dispatches on GameStatus::menu.nextMode
 * (jump table 0x003082B0, 27 entries from 0: 0, 10, 14..23 and out-of-range
 * values do nothing).  The cases are in retail's block order (... 13, 26,
 * 25, 24, 9: 25 jumps into 24's shared call). */
// FUNC 0x00100140 main
int main(int argc, char **argv)
{
    MovieParam movie;                       /* sp+0x10 */
    /* implicit: play_movie (play_opening, inline) */

#ifndef BOOT_OLD_GCC
    __main();                               /* egcs inserts this call itself */
#endif
    game_boot(argc, argv);
    memorycard_warning();
    play_opening(&movie);

    /* A loaded game: its country; a championship it left running goes on. */
    if (F_0014AE00(gt2k_menu::MEMCARD_AUTOLOAD, 0, 0, NULL, NULL, 0, 0, 0, NULL) != 0) {
        set_country(getGameStatus()->language);
        if (getGameStatus()->getChampionshipSave()->active != 0)
            main_championship(1);
    }

    for (;;) {
        if (gtmode_main() == 0) {
#if ENH_MENU_ATTRACT_REPLAYS
            /* Lupo 2 (L2 main 0x001000D0 -> 0x00100998; menus_a.md 4): one
             * attract demo each time the menus time out, then the menus
             * again (no five-demo loop, no opening movie in between) */
            run_attract();
            continue;
#endif
            if (run_attract_demo() != 0) continue;
            if (run_attract_demo() != 0) continue;
            if (run_attract_demo() != 0) continue;
            if (run_attract_demo() != 0) continue;
            if (run_attract_demo() != 0) continue;
            play_opening(&movie);
            continue;
        }

        gfx_vsync(2);
        switch (getGameStatus()->getMenuStatus()->nextMode) {
        case MAIN_MODE_NONE:
            break;
        case MAIN_MODE_ARCADE:
            main_arcade();
            break;
        case MAIN_MODE_CHAMPIONSHIP:
            main_championship(0);
            break;
        case MAIN_MODE_GTMODE_RACE:
            main_gtmode_race();
            break;
        case MAIN_MODE_GTMODE_RACE2:
            main_gtmode_race2();
            break;
        case MAIN_MODE_MENU_DEMO:
            run_menu_demo_race();
            break;
        case MAIN_MODE_LICENSE_TEST:
            main_license_test();
            break;
        case MAIN_MODE_MACHINE_TEST:
            main_machine_test();
            break;
        case MAIN_MODE_FREE_RUN:
            main_free_run();
            break;
        case MAIN_MODE_CARD_REPLAY:
            play_saved_replay(0);
            break;
        case MAIN_MODE_DEMO_REPLAY:
            play_saved_replay(1);
            break;
        case MAIN_MODE_LOGGER:
            logger_main();
            break;
        case MAIN_MODE_GAME_LOAD:
            if (F_0014AE00(gt2k_menu::MEMCARD_GAME_LOAD, 0, 0, NULL, NULL, 0, 0, 0, NULL) != 0) {
                set_country(getGameStatus()->language);
                if (getGameStatus()->getChampionshipSave()->active != 0)
                    main_championship(1);
            }
            break;
        case MAIN_MODE_GAME_SAVE: {
            void *work = memalign(MC_WORK_ALIGN, MC_WORK_SIZE);
            F_0014AE00(gt2k_menu::MEMCARD_GAME_SAVE, 0, 0, NULL, work, MC_WORK_SIZE, 0, 0, NULL);
            free(work);
            break;
        }
        case MAIN_MODE_FILE_UTILITY: {
            void *work = memalign(MC_WORK_ALIGN, MC_WORK_SIZE);
            F_0014AE00(gt2k_menu::MEMCARD_FILE_UTILITY, 0, 0, NULL, work, MC_WORK_SIZE, 0, 0, NULL);
            free(work);
            break;
        }
        case MAIN_MODE_OPTIONS:
            F_0013FE58();
            break;
        }
    }
}

/* Mode 2, and after a load that left a championship running (cont 1).
 * Here and in the next four modes the T053 entry point takes no mode: the
 * runMode constant retail leaves in $a0 ($a1 here) is the register of the
 * store (the first free one after $v0 / $v1); with a mode argument egcs
 * loads the constant twice. */
// FUNC 0x00100488 main_championship
void main_championship(int cont)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_GTMODE;
    run_championship(cont);
}

/* Mode 3. */
// FUNC 0x001004C8 main_gtmode_race
void main_gtmode_race(void)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_GTMODE;
    run_gtmode_race();
}

/* Mode 4: the same code as mode 3. */
// FUNC 0x00100500 main_gtmode_race2
void main_gtmode_race2(void)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_GTMODE;
    run_gtmode_race();
}

/* Mode 5: an arcade demo race of the race GameStatus::menu.event, no time
 * limit, the organization's preview mode on.  The race database only lives
 * for the select (frame: RaceDataBase sp+0x00, RaceArcadeDemoInformation
 * sp+0x30, RaceArcadeDemo sp+0x2A0). */
// FUNC 0x00100538 run_menu_demo_race
void run_menu_demo_race(void)
{
    const char *event = getGameStatus()->menu.event;

    {
        RaceDataBase db;
        db.select(nameToCode(event));
    }
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);

    RaceArcadeDemoInformation info;
    info.setRaceCode(nameToCode(event));
    info.timeLimit = 0;                     /* the default */
    RaceOrganization *org = race_organization_create(&info);
    org->racePreviewMode = 1;

    RaceArcadeDemo race;
    race.org = org;
    race.virtual_41(RACE_DEMO, 1);          /* direct call (complete object) */
    if (org != NULL)
        delete org;                         /* virtual destructor, flags 3 (vptr +0x8C0) */
    race_end();
}

/* Mode 6. */
// FUNC 0x001006C8 main_license_test
void main_license_test(void)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_LICENSE_TEST;
    run_license_test();
}

/* Mode 7. */
// FUNC 0x00100700 main_machine_test
void main_machine_test(void)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_MACHINE_TEST;
    run_machine_test();
}

/* Mode 8: the free run of the GT mode.  The running car is set again (its
 * own value), then run_free_run with the course menu.str84, the race
 * menu.event, the gearbox of the options (gearAuto ^ 1: 1 manual) and
 * selection 3.  The four zero stores after the MenuArgs zeroing are
 * repeated in the original (dead stores). */
// FUNC 0x00100738 main_free_run
void main_free_run(void)
{
    getGameStatus()->getMenuStatus()->runMode = RUN_MODE_FREE_RUN;
    GarageCar *car = getGameStatus()->getRunningCar();
    getGameStatus()->setRunningCar(car);

    MenuArgs args;                          /* sp+0x00 */
    int         manual = getGameStatus()->getGameOptions()->gearAuto ^ 1;
    const char *course = getGameStatus()->getMenuStatus()->str84;
    const char *race   = getGameStatus()->getMenuStatus()->event;
    args.transmission1 = manual;
    args.course        = course;
    args.race          = race;
    args.selection     = 3;                 /* time attack (MenuArgs::selection) */
    args.car1          = NULL;
    args.color1        = 0;
    args.tyre1         = 0;
    args.opt1d         = 0;
    run_free_run(&args, 1);
}

/* Modes 11 / 12: a replay from the memory card (which 0, F_0014AE00 mode
 * 0x11) or a demo replay of the disc (else, mode 0xC), loaded into a 64-byte
 * aligned stack buffer that starts with its RaceInfoBlock.  The race object
 * is built by the replay information (its class by name) in an alloca'd
 * area of g_00351DF0 bytes rounded up to 64.  When the control records
 * after the block (+0x191C) have slots and the race is a RaceSolitaire, it
 * reads the block through virtual_59, any other race through virtual_13.
 * $fp is the frame pointer (alloca). */
/* p rounded up to 64 bytes, in int arithmetic (retail's mask is the signed
 * -64).  Inline in the original: its argument's copy keeps &raw (fp+0x10) in
 * $s5, which the later out-of-range frame address of `info' reuses. */
static inline void *align64(void *p)
{
    return (void *)(((int)p + 0x3F) & ~0x3F);
}

// FUNC 0x00100838 play_saved_replay
void play_saved_replay(int which)
{
    u8 raw[0x41980];                        /* fp+0x10 */

    bgm_stop();
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    u8 *data = (u8 *)align64(raw);
    RaceInfoBlock *blk = (RaceInfoBlock *)data;
    int mode = (which != 0) ? gt2k_menu::MEMCARD_DEMO_REPLAY_LOAD
                            : gt2k_menu::MEMCARD_REPLAY_LOAD;
    if (F_0014AE00(mode, 0, 0, data, NULL, 0, 0, 0, NULL) != 0) {
        int workSize = (g_00351DF0 + 0x3F) & ~0x3F;
        void *work = __builtin_alloca(workSize);

        RaceReplayInformation info(blk);    /* fp+0x41990 */
        RaceOrganization *org = race_organization_create(&info);
        /* The race's static type is RacePS2Base (__dynamic_cast's source
         * type __tf11RacePS2Base); T149 returns a RaceBase *. */
        RacePS2Base *race = static_cast<RacePS2Base *>(info.createRace(work, workSize));
        const u32 *records = (const u32 *)(data + 0x191C);   /* after the block (RaceInfoBlock::size) */
        /* The dynamic_cast tests race for NULL itself. */
        if (AutomobileControlRecord::Manager::hasSlots(records) == 1 &&
            dynamic_cast<RaceSolitaire *>(race) != NULL)
            static_cast<RaceSolitaire *>(race)->virtual_59(0, blk);
        else
            race->virtual_13(blk);
        race->org = org;
        race->virtual_41(RACE_REPLAY, 1);
        info.destroyRace(race, 0);           /* destroyed in place (not on the heap) */
        if (org != NULL)
            delete org;
    }
    race_end();
}

/* Attract mode: advances g_00350B0C to the next demo whose requirement
 * (ProductConfig F_0021D758) GameStatus::isLicenseSPassed() meets.  From
 * there it looks (at most `count' demos) for one whose race course has the
 * kind (RaceDataBase::getCourseKind) of that demo's course, and runs that
 * one as a RaceArcadeDemo: its race, its shop car at its grid position, the
 * attract time limit, and the first demo's course (the last demo of the list
 * keeps its race's own course).  0 when the demo ran to its time limit, else
 * 1 (main then goes back to the menus).  Frame: RaceDataBase sp+0x00,
 * information sp+0x30, race sp+0x2A0. */
/* The race's end code, read through the race's address: retail adds the
 * 0x8000 part of endCode's offset (0x9794) to the register holding &race
 * (an inline accessor's argument), not to $sp. */
static inline int race_end_code(RaceBase &race)
{
    return race.endCode;
}

// FUNC 0x00100A78 run_attract_demo
int run_attract_demo(void)
{
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    int count = product_config()->demoCount();
    int need;
    do {
        int next = g_00350B0C + 1;
        g_00350B0C = next;
        if (next >= count)
            g_00350B0C = next - count;
        need = product_config()->F_0021D758(g_00350B0C);
    } while (getGameStatus()->isLicenseSPassed() < need);

    int demo = g_00350B0C;
    u64 course = nameToCode(product_config()->F_0021D6F8(demo));
    {
        RaceDataBase db;
        int kind = db.getCourseKind(course);
        for (int tries = 0; tries < count; tries++) {
            db.select(nameToCode(product_config()->demoCar(demo)));
            if (kind == db.getCourseKind(db.course))
                break;
            demo++;
            if (demo >= count)
                demo = 0;
        }
    }

    u64 raceCode = nameToCode(product_config()->demoCar(demo));
    RaceArcadeDemoInformation info;
    const char *shopCar = product_config()->F_0021D718(demo);
    if (shopCar != NULL)                    /* (? never NULL: the file gives "" for none, u099) */
        info.shopCar = nameToCode(shopCar);
    info.shopCarPos = product_config()->F_0021D738(demo);
    info.timeLimit  = product_config()->F_0021D2B0();
    info.setRaceCode(raceCode);
    if (demo == count - 1)
        course = info.getCourseCode();      /* direct: RaceSinglePlayerInformation's */
    info.setCourseCode(course);             /* direct: RaceSinglePlayerInformation's */
    info.setRaceCode(raceCode);
    RaceOrganization *org = race_organization_create(&info);

    RaceArcadeDemo race;
    race.org = org;
    race.virtual_41(RACE_DEMO, 1);
    int endCode = race_end_code(race);
    if (org != NULL)
        delete org;
    race_end();
    if (endCode == RACE_END_DEMO_TIME_UP)
        return 0;
    return 1;
}

#if ENH_MENU_ATTRACT_REPLAYS
/* The enhanced engine: Lupo 2's attract (menus_a.md 4). */

/* A GCF attract demo whose requirement word is this is a replay file of the
 * disc (Lupo 2 reads the word as the demo's kind, L2 0x0022C428; its race
 * word is then the file).  GT3 shows a demo once GameStatus::
 * isLicenseSPassed() reaches the word, which it never does for 1000: GT3's
 * own list has no such entry, and GT3's run_attract_demo skips one. */
enum { ATTRACT_REPLAY_KIND = 1000 };

/* Lupo 2 L2 0x00101240: a replay file of the disc as the attract demo.  The
 * file is read whole (file_read_blocking, inflated when packed) and restored
 * as a "ReplayData" (the demo replays of GT3's memory card screens are read
 * the same way: MemorycardSlotSet, T071), then played as play_saved_replay
 * plays one.  A file that is not on the volume, or does not restore, plays
 * nothing (Lupo 2 does not test it).  Frame as play_saved_replay's. */
static void run_replay_demo(const char *file)
{
    u8 raw[0x41980];
    FileLoc loc;

    if (file == NULL || file_resolve(file, &loc, &vol_device_object) != 0 || !(loc.flags & 1)) {
        printf("menus: attract replay %s: not on the volume\n", file != NULL ? file : "(none)");
        return;
    }
    printf("menus: attract replay %s\n", file);
    race_begin(CONTROL_RECORD_SIZE, CONTROL_RECORDS);
    u8 *data = (u8 *)align64(raw);
    RaceInfoBlock *blk = (RaceInfoBlock *)data;
    void *packed = file_read_blocking(file, NULL, 0, 1, &vol_device_object);
    int ok = 0;
    if (packed != NULL) {
        Serialize::Base *replay = serialize_create(g_00351F40);    /* "ReplayData" */
        if (replay != NULL) {
            ok = replay->restore(packed, data) != NULL;
            serialize_destroy(replay);
        }
        free(packed);
    }
    if (ok == 0)
        printf("menus: attract replay %s: not a replay\n", file);
    if (ok != 0) {
        int workSize = (g_00351DF0 + 0x3F) & ~0x3F;
        void *work = __builtin_alloca(workSize);

        RaceReplayInformation info(blk);
        RaceOrganization *org = race_organization_create(&info);
        RacePS2Base *race = static_cast<RacePS2Base *>(info.createRace(work, workSize));
        const u32 *records = (const u32 *)(data + 0x191C);
        if (AutomobileControlRecord::Manager::hasSlots(records) == 1 &&
            dynamic_cast<RaceSolitaire *>(race) != NULL)
            static_cast<RaceSolitaire *>(race)->virtual_59(0, blk);
        else
            race->virtual_13(blk);
        race->org = org;
        race->virtual_41(RACE_REPLAY, 1);
        info.destroyRace(race, 0);
        if (org != NULL)
            delete org;
    }
    race_end();
}

/* Lupo 2 L2 0x00100998: the next attract demo of the GCF list.  An entry of
 * ATTRACT_REPLAY_KIND plays its replay file; any other runs GT3's demo race
 * (run_attract_demo, which picks the next demo the player's licences
 * allow, as in GT3). */
static void run_attract(void)
{
    int count = product_config()->demoCount();
    if (count <= 0)
        return;
    int next = g_00350B0C + 1;
    if (next >= count)
        next = 0;
    if (product_config()->F_0021D758(next) == ATTRACT_REPLAY_KIND) {
        g_00350B0C = next;
        run_replay_demo(product_config()->demoCar(next));
        return;
    }
    run_attract_demo();
}
#endif

/* Mode 1: a state machine over the arcade menu's selection (jump tables
 * 0x00308340 / 0x00308360).  The race states enter their modes with the
 * menu's MenuArgs; the i.LINK state runs an 8-car battle
 * (RaceLinkBattleInformation sp+0x40, RaceLinkBattle sp+0x10D0) of the race
 * "AEAT0001" on the menu's course, one entry per linked node (its car of
 * its class, colour, gearbox and tyres; the vibration of the options for
 * this console's own node), printing "%d:%d:%s\n" (car, class, code) for
 * each.  `done' lives on the stack at sp+0x360D0.  The free-run state's
 * case comes before the time trial's (retail's block order); each race
 * state sets the menu state after its race (retail fills the getGameStatus
 * jal's delay slot from after the store: the store's constant is not
 * taken from the state register).
 * Not exact (91.8%): only the i.LINK loop differs.  Retail stores colour
 * and gearbox through a strength-reduced pointer (sp+0x98 + slot * 0x200)
 * but tyre and vibration through slot * 0x200 + a copy of &info ($fp,
 * entry = +0x58, not reduced), and set_entry_car gets the original &info
 * ($s5); here loop.c reduces one pointer for all four and the call gets
 * the copy.  Tried: entry pointer / direct accesses per field, a pointer to
 * info before or inside the loop, an inline entry accessor on the battle
 * information, the order of the stores (all 91.8% or lower). */
// FUNC 0x00100DB8 main_arcade
void main_arcade(void)
{
    MenuArgs args;                          /* sp+0x00 */
    int state = ARCADE_STATE_MENU;
    int done  = 0;

    do {
        switch (state) {
        case ARCADE_STATE_EXIT:
            done = 1;
            break;

        case ARCADE_STATE_MENU:
            new (&args) MenuArgs;           /* cleared in place (retail: no temporary) */
            if (arcade_menu(&args) == 0) {
                state = ARCADE_STATE_EXIT;
                break;
            }
            switch (args.selection) {       /* jump table from 0 (0x00308360) */
            case 0: break;
            case 1: state = ARCADE_STATE_SINGLE_RACE;  break;
            case 2: state = ARCADE_STATE_TIME_TRIAL;   break;
            case 3: state = ARCADE_STATE_FREE_RUN;     break;
            case 4: state = ARCADE_STATE_SPLIT_BATTLE; break;
            case 5: state = ARCADE_STATE_LINK_BATTLE;  break;
            /* the enhanced engine: the Pod races (ArcadeControl::getSelection,
             * only when the Pod's data is there: podRaceAvailable) */
            case 6: state = ARCADE_STATE_POD_RACE;     break;
            case 7: state = ARCADE_STATE_POD_2P_RACE;  break;
            }
            break;

        case ARCADE_STATE_SINGLE_RACE:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_ARCADE_RACE;
            run_arcade_race(&args);
            state = ARCADE_STATE_MENU;
            break;

        case ARCADE_STATE_FREE_RUN:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_FREE_RUN;
            run_free_run(&args, 0);
            state = ARCADE_STATE_MENU;
            break;

        case ARCADE_STATE_TIME_TRIAL:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_TIME_TRIAL;
            run_time_trial(&args);
            state = ARCADE_STATE_MENU;
            break;

        case ARCADE_STATE_SPLIT_BATTLE:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_SPLIT_BATTLE;
            run_split_battle(&args);
            state = ARCADE_STATE_MENU;
            break;

        /* The enhanced engine (Lupo 2 main_arcade L2 0x001014A8, its states
         * for selections 7 / 8; menus_a.md 5 / 9): the Pod race as an arcade
         * single race, the 2-player one as a split battle. */
        case ARCADE_STATE_POD_RACE:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_ARCADE_RACE;
            run_pod_race(&args);
            state = ARCADE_STATE_MENU;
            break;

        case ARCADE_STATE_POD_2P_RACE:
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_SPLIT_BATTLE;
            run_pod_2p_race(&args);
            state = ARCADE_STATE_MENU;
            break;

        case ARCADE_STATE_LINK_BATTLE: {
            getGameStatus()->getMenuStatus()->runMode = RUN_MODE_LINK_BATTLE;
            race_begin(CONTROL_RECORD_SIZE, LINK_BATTLE_SLOTS);

            RaceLinkBattleInformation info;
            info.setEntryCount(F_0022D850());   /* direct: RaceInformation's */
            const char *course = args.course;
            info.setRaceCode(nameToCode("AEAT0001"));
            for (int slot = 0; slot < LINK_BATTLE_SLOTS; slot++) {
                LinkPlayerInfo *node = F_0022D7A0(slot);
                if (node == NULL)
                    continue;
                const char *cls  = arcade_class_name(node->carClass);
                const char *code = product_config()->carCode(node->car, cls);
                printf("%d:%d:%s\n", node->car, node->carClass, code);
                set_entry_car(&info, nameToCode(code), slot);
                RaceEntry *entry = &info.entries[slot];
                entry->color        = node->color;
                entry->transmission = (node->transmission != 0);
                entry->tyre         = (node->tyre != 0);
                if (slot == F_0022D830())   /* this console's own node */
                    entry->vibration = getGameStatus()->getGameOptions()->vibration1p;
                else
                    entry->vibration = 0;
            }
            RaceOrganization *org = race_organization_create(&info);
            org->setCourseCode(nameToCode(course));
            org->startingCount = LINK_BATTLE_START_COUNT;
            {
                RaceLinkBattle race;
                race.org = org;
                race.virtual_41(RACE_LIVE, 1);
                if (org != NULL)
                    delete org;
            }
            race_end();
            state = ARCADE_STATE_MENU;
            break;
        }
        }
    } while (done == 0);
    gfx_vsync(2);
}

/* Entry `slot' of the battle information: the car code (a 64-bit store),
 * equip.setCar(car) (its result is not used), equip.presetCar = 1.
 * Not exact (75%): retail computes slot * 0x200 + info once for the car
 * store (+0x58) and setCar's `this' (a copy, + 0x70), and again after the
 * call for presetCar (+0x250).  egcs folds &entries[slot].equip to
 * info + (slot * 0x200 + 0x58) + 0x18 whatever the spelling (member call,
 * pointer arithmetic, reference, inline helpers, slot types); writing the
 * last store as (&info->entries[slot])->equip.presetCar gives 87.5% (the
 * recomputation and registers) but is not natural source. */
// FUNC 0x00101240 set_entry_car
void set_entry_car(RaceLinkBattleInformation *info, u64 car, int slot)
{
    info->entries[slot].car = car;
    info->entries[slot].equip.setCar(car);
    info->entries[slot].equip.presetCar = 1;
}
