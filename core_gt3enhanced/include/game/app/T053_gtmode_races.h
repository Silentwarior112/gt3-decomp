/* T053_gtmode_races.h - the race modes of the GT mode (translation unit
 * T053, unit u017), 0x00131410..0x00133F48.
 *
 * main() enters them through GameStatus::menu.nextMode (T000): the machine
 * test (mode 7), the licence tests (mode 6), one GT mode race (modes 3 / 4)
 * and a championship (mode 2).  Each one runs between race_begin /
 * race_end (T007 F_00102B58(0x20000, 2) / F_00102BB8), builds the race of
 * GameStatus::menu.event for the player's car and loops over a short menu
 * (T102 run_short_menu) drawn over the race.  The four entry points take no
 * mode: main (T000) stores the runMode into GameStatus::menu.runMode before
 * the call, and the constant it leaves in $a0 / $a1 (the store's register,
 * main_championship's second) is not an argument (the callees overwrite
 * $a0 at once).  Each returns 0.
 *
 * T053 also holds unreferenced out-of-line copies of inline functions of
 * other classes (ShortMenuRankEntry / ShortMenuResultEntry::clear,
 * Cell<GarageCar>'s and GarageCar's destructors and their typeinfo
 * functions, 0x00133DC8..0x00133F48): they are declared with their owners.
 *
 * gt2k_game::GameOptions, the class the pass-2 plan gave this header, is
 * ::GameOptions of T428 (the 0x888-byte options at GameStatus+0x2FF38; the
 * 13 units that declared gt2k_game::GameOptions held partial views of it):
 * not a second class, and not a typedef either (see below; groups h07,
 * h08). */
#ifndef GAME_T053_GTMODE_RACES_H
#define GAME_T053_GTMODE_RACES_H

#include "game/game.h"
#include "game/save/T428_game_options.h"         /* ::GameOptions (gt2k_game::GameOptions) */

class GarageCar;                            /* T036 (pointers only) */

/* No gt2k_game::GameOptions typedef: under `using namespace gt2k_game` egcs
 * calls the name ambiguous with ::GameOptions (tools/hdrall.py).  Code inside
 * the namespace finds ::GameOptions anyway. */

/* Enters a machine test result into the records (GameStatus machineTest)
 * when it makes the table: kinds 0 / 1 (0-400 m / 0-1000 m) rank `time' (ms,
 * at most 0x3FFFFF), kind 2 (max speed) ranks `speed' (its x1000 value goes
 * into the ranking code).  The player types a name, then the name, the car
 * code and the ranking code (kind 3 / 4 / 5) are stored.  Caller:
 * run_machine_test. */
void machine_test_new_record(int kind, u32 time, GarageCar *car, u64 course, f32 speed); /* 0x00131410 */
/* Mode 7 (runMode 5): the machine test of menu.event ("M400", "M1000",
 * else the max speed test) under qm_gt_mt_400 / _1000 / _max.mbl. */
int  run_machine_test(void);               /* 0x00131690 */
/* Mode 6 (runMode 4): the licence test menu.event under the licence short
 * menu; afterwards the "licence acquired" screen (T059 license_get_screen)
 * and the licence's prize car (T061). */
int  run_license_test(void);               /* 0x00132030 */
/* Modes 3 / 4 (runMode 2): one GT mode race under qm_gt_event.mbl; the
 * prize money of the place (T056 trophy_prize_screen), a win gives the
 * race's prize car. */
int  run_gtmode_race(void);                /* 0x00132BA0 */
/* Mode 2 (runMode 2): a championship under qm_gt_event_champ.mbl; cont !=
 * 0 continues the saved one (main passes 1 after an autoload).  A won
 * championship: its bonus, the cup (T060 trophy_get), the prize car. */
int  run_championship(int cont);           /* 0x00133388 */

#endif /* GAME_T053_GTMODE_RACES_H */
