/* T054_split_battle.h - the 2-player split screen battle (translation unit
 * T054, unit u017), 0x00133F48..0x00134630.
 *
 * Reached from the arcade menu (main's F_00100DB8, runMode 1) with its
 * MenuArgs: the course, and per player the car colour, the tyres and a
 * third setting; the cars are GameStatus::menu.raceCar[0] / [1].  The race
 * is a RaceSplitBattle (race code "yotsuya001") under the short menu, the
 * results under qm_arc_2p_result.mbl; "save replay" goes to the memory card
 * screens.  T054 also holds unreferenced copies of the ShortMenuRankEntry /
 * ShortMenuResultEntry inline clear() (0x001345F0 / 0x00134610),
 * declared with their owners. */
#ifndef GAME_T054_SPLIT_BATTLE_H
#define GAME_T054_SPLIT_BATTLE_H

#include "game/game.h"

struct MenuArgs;                            /* T000 (pointers only) */

/* args: course, color1 / 2, tyre1 / 2, transmission1 / 2; the cars are
 * menu.raceCar[0] / [1], an arcade car (raceCarKind RACE_CAR_BY_CODE) gets
 * the menu's colour and tyres.  The first race starts at once.  Returns 0.
 * No second parameter: the 1 main_arcade (T000) leaves in $a1 is the
 * register of its runMode store (RUN_MODE_SPLIT_BATTLE), and the code
 * overwrites $a1 at once (u001's matching pass). */
int run_split_battle(struct MenuArgs *args);   /* 0x00133F48 */

#endif /* GAME_T054_SPLIT_BATTLE_H */
