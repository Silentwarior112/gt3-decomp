/* T000_main.h - main, the top-level game modes, and the argument records they
 * pass around: MenuArgs, MovieParam, RaceEntry, LinkPlayerInfo (translation
 * unit T000, unit u001), 0x00100140..0x00101290.
 *
 * main boots the game (game_boot, T007), plays "stream/opening.pss", tries
 * the memory-card autoload, then loops over the menus (gtmode_main, T038):
 * when they return 0, up to five attract demos and the opening movie again,
 * otherwise GameStatus::menu.nextMode (jump table 0x003082B0, 27 entries):
 *   1 main_arcade         2 main_championship   3 / 4 main_gtmode_race(2)
 *   5 run_menu_demo_race  6 main_license_test   7 main_machine_test
 *   8 main_free_run       9 the options (F_0013FE58, T062)
 *  11 / 12 play_saved_replay(0 / 1)            13 the logger (T055)
 *  24 / 25 memory-card modes 0x18 / 0x19 with a 0xD00000-byte buffer
 *  26 the memory-card load (mode 0x1A), as the autoload.
 * The mode functions set GameStatus::menu.runMode first.
 *
 * File-local data: g_00350B0C (gp-0x6EE4, the last attract demo, -1).
 *
 * Globals the plan put here that belong elsewhere: g_00351DF0 (the largest
 * race class size) is declared in T129_race_base.h, __ti17IOPLoadModuleBase
 * in T524, __ti10GameObject in T002, the SmartIndexT<int> typeinfo storage
 * in T017, __tiQ29gt2k_menu4Item in T033, _gp in G000, g_00350B68 in T016
 * (see ref/pass2/groups/h31/notes.md).
 */
#ifndef GAME_T000_MAIN_H
#define GAME_T000_MAIN_H

#include "game/game.h"
#include "game/data/T482_car_equipments.h"       /* CarEquipments (RaceEntry::equip, by value) */

class RaceLinkBattleInformation;            /* T138 (pointers only) */

/* What an i.LINK node sent in the race-selection exchange (F_0022D3C0, T473:
 * the five bytes of every node in g_002E98E8[8]; ArcadeControl's i.LINK
 * page sends car1, class1, opt1a..opt1c).  main_arcade's 8-car battle turns
 * each into a RaceEntry.  5 bytes, byte-aligned. */
struct LinkPlayerInfo {
    u8 car;                                 /* 0x0 car index in its class (ProductConfig::carCode) */
    u8 carClass;                            /* 0x1 the class (arcade_class_name, T010) */
    u8 color;                               /* 0x2 -> RaceEntry::color */
    u8 tyre;                                /* 0x3 != 0 -> RaceEntry::tyre */
    u8 transmission;                        /* 0x4 != 0 -> RaceEntry::transmission */
};
EGCS_SIZE(LinkPlayerInfo, 5);

/* The results of a menu, handed to the race modes (the arcade menu fills it
 * in ArcadeControl::finish, T010; main_free_run from the GameStatus menu
 * state).  Player 1's choices, then player 2's (TWO_PLAYER).  Not
 * polymorphic.  Size 0x3C (main_arcade's stack frame). */
struct MenuArgs {
    /* Inline in the original (every user zeroes the 15 words in order). */
    MenuArgs()
    {
        car1 = 0; course = 0; race = 0; unk_0C = 0; color1 = 0;
        tyre1 = 0; transmission1 = 0; opt1d = 0; unk_20 = 0; selection = 0;
        car2 = 0; color2 = 0; tyre2 = 0; transmission2 = 0; opt2d = 0;
    }

    const char *car1;                       /* 0x00 player 1's car (code name; ProductConfig::carCode) */
    const char *course;                     /* 0x04 the course code (NULL in COMPULSORY; main_free_run:
                                               GameStatus menu.str84) */
    const char *race;                       /* 0x08 the race / event code (F_0010F398; GameStatus
                                               menu.event) */
    int         unk_0C;                     /* 0x0C */
    int         color1;                     /* 0x10 player 1's colour (ArcadeControl::color1) */
    int         tyre1;                      /* 0x14 1: F_00236758 tyres (kind 0xB), else F_00236718
                                               (kind 0xA); -> RaceEntry::tyre (ArcadeControl racing1 == 1) */
    int         transmission1;              /* 0x18 1 manual (-> RaceEntry::transmission; main_free_run:
                                               options gearAuto ^ 1) */
    int         opt1d;                      /* 0x1C ArcadeControl opt1d == 0 */
    int         unk_20;                     /* 0x20 */
    int         selection;                  /* 0x24 1 single race, 2 compulsory (main_arcade:
                                               run_time_trial), 3 time attack (run_free_run), 4 two
                                               players, 5 i.LINK (ArcadeControl::getSelection) */
    const char *car2;                       /* 0x28 player 2's car (TWO_PLAYER) */
    int         color2;                     /* 0x2C */
    int         tyre2;                      /* 0x30 */
    int         transmission2;              /* 0x34 */
    int         opt2d;                      /* 0x38 */
};
EGCS_SIZE(MenuArgs, 0x3C);

/* A movie to play: play_movie (T051) and MenuBgMovie::start (T253); the
 * arcade menu's table g_00292930 (T029).  Size 0x18. */
struct MovieParam {
    const char *file;                       /* 0x00 "stream/....pss" */
    int         width;                      /* 0x04 */
    int         height;                     /* 0x08 */
    int         field;                      /* 0x0C 1: interlaced fields (-> MenuBgMovie::field) */
    int         withSound;                  /* 0x10 != 0 -> mpegif::start's withSound */
    int         oneShot;                    /* 0x14 != 0 -> mpegif::start's oneShot */
};
EGCS_SIZE(MovieParam, 0x18);

/* One car of a race information (RaceInformation +0x58 + i * 0x200: T133's
 * entry, T137 / T139's entries).  The race classes copy the words to the
 * RaceEntryCar (+0x0C -> +0x20E4 transmission, +0x10 -> +0x20E8, +0x14 ->
 * +0x20F8 vibration, T121).  An empty entry: car 0, colour -1.  Size 0x200
 * (8-aligned by CarEquipments' u64). */
struct RaceEntry {
    u64           car;                      /* 0x000 the car code (0: none) */
    int           color;                    /* 0x008 the colour index (-1: none; CarEquipments::setColorIndex,
                                               CarColorDataBase::getColor: u046 / u047) */
    int           transmission;             /* 0x00C 1 manual (MenuArgs transmission1) */
    int           tyre;                     /* 0x010 (MenuArgs tyre1) */
    int           vibration;                /* 0x014 (GameStatus options vibration1p for the local player) */
    CarEquipments equip;                    /* 0x018 setCar(car) (set_entry_car: presetCar = 1 at +0x1F8) */

    /* (? name) equip = eq.  Inline in the original: RaceBattleInformationT<N>::
     * setEquipments (u046, 0x001847E0) computes the entry's address as an
     * inline argument (sll; addu idx, this; addiu +0x70), which a direct
     * entries[i].equip = eq does not give. */
    void setEquipments(const CarEquipments &eq) { equip = eq; }
};
EGCS_SIZE(RaceEntry, 0x200);

int  main(int argc, char **argv);           /* 0x00100140 (crt0, G000) */
/* Mode 2 (runMode 2): run_championship(cont) (T053); cont = 1 after a
 * memory-card load that left a championship running. */
void main_championship(int cont);           /* 0x00100488 */
/* Modes 3 and 4 (the same code): runMode 2, run_gtmode_race() (T053). */
void main_gtmode_race(void);                /* 0x001004C8 */
void main_gtmode_race2(void);               /* 0x00100500 */
/* Mode 5: an arcade demo race (RaceArcadeDemo) of the race GameStatus
 * menu.event, with its race database. */
void run_menu_demo_race(void);              /* 0x00100538 */
/* Mode 6: runMode 4, run_license_test() (T053). */
void main_license_test(void);               /* 0x001006C8 */
/* Mode 7: runMode 5, run_machine_test() (T053). */
void main_machine_test(void);               /* 0x00100700 */
/* Mode 8: runMode 7, the running car, then run_free_run(args, 1) (T052)
 * with course GameStatus menu.str84, race menu.event, selection 3. */
void main_free_run(void);                   /* 0x00100738 */
/* Modes 11 / 12: a replay from the memory card (F_0014AE00 mode 0x11 for
 * which == 0, else 0xC), its race built in an alloca'd area of g_00351DF0
 * bytes; a RaceSolitaire replay goes through virtual_59, any other race
 * through virtual_13. */
void play_saved_replay(int which);          /* 0x00100838 */
/* The next attract demo whose progress requirement the GameStatus meets
 * (ProductConfig demo list), as a RaceArcadeDemo; 0 if its endCode was 5
 * (it ran to its time limit: RaceArcadeDemo::virtual_27, event 7), else 1
 * (main goes back to the menus). */
int  run_attract_demo(void);                /* 0x00100A78 */
/* Mode 1: the arcade menu (arcade_menu, T015) until it is left; its
 * selection runs run_arcade_race (runMode 6), run_time_trial (0),
 * run_free_run (7), run_split_battle (1, T054) or the 8-car i.LINK battle
 * (runMode 3: RaceLinkBattle on "AEAT0001" with the nodes' LinkPlayerInfo). */
void main_arcade(void);                     /* 0x00100DB8 */
/* Entry `slot' of the battle information: car, equip.setCar(car),
 * equip.presetCar = 1. */
void set_entry_car(RaceLinkBattleInformation *info, u64 car, int slot);   /* 0x00101240 */

#endif /* GAME_T000_MAIN_H */
