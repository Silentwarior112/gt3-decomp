/* T281_battle_rule_presets.h - presets of two RaceRuleOptions groups for the
 * battle modes, and the default rules g_002D2988 (translation unit T281,
 * unit u076), 0x001DA1B8..0x001DA308.
 *
 * DynamicsConductorBattle2P / BattleMP (slots 9 and 10, u083) call them with
 * mode 1, 2 or 0 (off).  T281's .data is g_002D2988 (the bytes 0x002D2988..
 * 0x002D29C8 before its .eh_frame; u076 wrote it as g_002D28B0 + 0xD8, but
 * g_002D28B0 is T325's). */
#ifndef GAME_T281_BATTLE_RULE_PRESETS_H
#define GAME_T281_BATTLE_RULE_PRESETS_H

#include "game/game.h"

class RaceRuleOptions;                     /* T326_race_rule_options.h (pointers, and the type of g_002D2988) */

/* The slow-car boost group: mode 1 slowEnemyBoostMax 15, Threshold1 20,
 * Threshold2 100; mode 2 20, 10, 20; enemyBoostType 0 for both; otherwise
 * all 0 and enemyBoostType 0xFF.  Returns 1 for mode 1 or 2. */
int rules_boost_preset(RaceRuleOptions *o, int mode);       /* 0x001DA1B8 (F_001DA1B8) */
/* The tyre wear group (tw*): two presets; otherwise all 0 but
 * twGoodTireGripDown.  Returns mode != 0. */
int rules_tire_wear_preset(RaceRuleOptions *o, int mode);   /* 0x001DA238 (F_001DA238) */

/* The default rules: the SinglePlayer conductor's (u084), and the rules of
 * CarDynamics_init outside a race (ValuateAutomobilePerformance,
 * SelectMeterScale, ShowRoomCar).  launchSpeed 80, enemy volumes 100 %. */
extern RaceRuleOptions g_002D2988;          /* 0x002D2988 */

#endif /* GAME_T281_BATTLE_RULE_PRESETS_H */
