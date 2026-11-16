/* T115_race_option.h - RaceOption, the option block of a race, and
 * race_option_from_game_options (translation unit T115, unit u040),
 * 0x00174AE8..0x00174B48.
 *
 * RaceInformation +0x28 (T124), RaceOrganization +0x0 (T125, copied from
 * the information) and RaceBase +0x4C (T129) hold one.  Its words are the
 * race options of the game options (GameOptions, T428): raceLaps,
 * tireDamage, then the split screen battle block (+0x2C..+0x3B; u083's
 * battle conductors read it through RaceOrganization +0x8..+0x14). */
#ifndef GAME_T115_RACE_OPTION_H
#define GAME_T115_RACE_OPTION_H

#include "game/game.h"
#include "game/menu/records/T089_split_battle_option.h"      /* SplitBattleOption (held by value) */

/* (u043 name.)  Not polymorphic; 0x18 bytes. */
class RaceOption {
public:
    int               raceLaps;             /* 0x00 GameOptions::raceLaps (u040 / u043: field_00) */
    int               tireDamage;           /* 0x04 GameOptions::tireDamage (u040 / u043: field_04) */
    SplitBattleOption splitBattle;          /* 0x08 GameOptions +0x2C..+0x3B */
};
EGCS_SIZE(RaceOption, 0x18);

/* opt = the race options of the current GameStatus's game options
 * (GameStatus +0x2FF38: +0x1C, +0x20 and the split battle block).  Not a
 * constructor (u042 called it RaceOption()): it does not return `this'.
 * Callers: the race setups of u016 / u017 / u042 (u017: RaceInformation's
 * constructor, on its +0x28). */
void race_option_from_game_options(RaceOption *opt);   /* 0x00174AE8 */

#endif /* GAME_T115_RACE_OPTION_H */
