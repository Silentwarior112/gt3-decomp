/* T115 - the race options from the game options (game/menu/race_menu/T115_race_option.h).
 * 0x00174AE8..0x00174B48.
 *
 * Callers (9): run_arcade_race, run_free_run, run_time_trial,
 * run_machine_test, run_license_test, run_gtmode_race, run_championship,
 * run_split_battle, ... (the race setups; u017: also RaceInformation's
 * constructor, on its +0x28). */
/* Retail's .rodata (0x00325DF0..0x00325ED8) is the GameStatus's garage
 * lists' type names only (nothing references them: the anchor says where
 * they are; the same 231 bytes occur 111 times in the image). */
// DATA_START 0x00325DF0
#include "game/save/T429_game_status.h"          /* getGameStatus; GameStatus::options (T428 GameOptions) */
#include "game/menu/race_menu/T115_race_option.h"

/* option = the race options of the current GameStatus's game options:
 * raceLaps, tireDamage and the split screen battle block (+0x2C..+0x3B),
 * field by field (as u047's RaceLinkBattleInformation).  The tireDamage
 * fields are bools (T089 / T428): retail loads each one ahead of the
 * previous int field's store. */
// FUNC 0x00174AE8 race_option_from_game_options
void race_option_from_game_options(RaceOption *option)
{
    ::GameOptions *options = &getGameStatus()->options;

    option->raceLaps   = options->raceLaps;
    option->tireDamage = options->tireDamage;
    option->splitBattle.raceLaps   = options->splitBattle.raceLaps;
    option->splitBattle.tireDamage = options->splitBattle.tireDamage;
    option->splitBattle.handicap   = options->splitBattle.handicap;
    option->splitBattle.boost      = options->splitBattle.boost;
}
