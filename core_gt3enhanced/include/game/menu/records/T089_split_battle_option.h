/* T089_split_battle_option.h - SplitBattleOption, the split screen battle
 * options of the GameOptions (a plain record, no code; the plan put it with
 * T089's record sources, unit u040).
 *
 * In its own header since u040's data pass (2026-10-17): GameOptions (T428)
 * and RaceOption (T115) hold one by value, and through them almost every
 * unit saw T089_records_menu_args.h.  Retail writes RecordsMenuArg's type
 * name (a class with inline virtual functions only) into the units that see
 * it (T052, T053, T089..T092, ...), not into every unit that sees the
 * GameStatus or a RaceOption: the original's GameOptions header did not
 * define the record sources.  T089_records_menu_args.h includes this header,
 * so its includers still see SplitBattleOption. */
#ifndef GAME_T089_SPLIT_BATTLE_OPTION_H
#define GAME_T089_SPLIT_BATTLE_OPTION_H

#include "game/game.h"

/* The split screen battle options of the GameOptions (+0x2C..+0x3B, T428's
 * splitBattle), copied field by field into RaceOption +0x8 (T115, T138)
 * and returned by RaceOrganization::getSplitBattleOption (T125).  Not
 * polymorphic, 0x10 bytes.  tireDamage is a bool (4 bytes under egcs; the
 * GameOptions' too, RaceOption's an int): its own alias set lets retail's
 * copies (race_option_from_game_options 0x00174AE8, RaceLinkBattleInformation's
 * constructor 0x001848A8) load it ahead of the previous field's store, as
 * the option menus set it from a comparison (u022's T066). */
class SplitBattleOption {
public:
    int  raceLaps;                          /* 0x0 1..100 */
    bool tireDamage;                        /* 0x4 */
    int handicap;                           /* 0x8 -150..151 by 5 m */
    int boost;                              /* 0xC 0 none, 1 low, 2 high */
};
EGCS_SIZE(SplitBattleOption, 0x10);

#endif /* GAME_T089_SPLIT_BATTLE_OPTION_H */
