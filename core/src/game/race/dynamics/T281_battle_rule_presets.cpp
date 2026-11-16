/* T281 - presets of two RaceRuleOptions groups for the battle modes
 * (DynamicsConductorBattle2P / BattleMP SetSlowCarBoostParameters and
 * SetTireWearParameters): mode 1, mode 2 or off.  T281's .data is the
 * default rules g_002D2988. */
#include "game/car/physics/car_spec_internal.h"
#include "game/race/dynamics/T281_battle_rule_presets.h"

#define ENEMY_BOOST_NONE 0xFF                   /* RaceRuleOptions.enemyBoostType: no boost */

/* The default rules (T281's .data, 0x40 bytes before its frame info):
 * launchSpeed 80, the enemy volumes and skills 100 %, enemyDtMasterVolume
 * 100, enemyBoostType 2, restrain 10 / 15 / 3, boost 10 / 20 / 4,
 * twPitTireGripDown 100. */
RaceRuleOptions g_002D2988 = {              /* 0x002D2988 */
    80,                                     /* 0x00 launchSpeed */
    0,                                      /* 0x01 laps */
    0,                                      /* 0x02 licenseType */
    0,                                      /* 0x03 powerLimit */
    { 100, 100, 100, 100 },                 /* 0x04 enemyMuVolumeF */
    { 100, 100, 100, 100 },                 /* 0x08 enemyMuVolumeR */
    { 100, 100, 100, 100 },                 /* 0x0C enemySkillA */
    { 100, 100, 100, 100 },                 /* 0x10 enemySkillB */
    { 100, 100, 100, 100 },                 /* 0x14 enemySkillC */
    { 0, 0, 0, 0 },                         /* 0x18 enemySkillC2 */
    { 0, 0, 0, 0 },                         /* 0x1C unk_1C */
    100,                                    /* 0x20 enemyDtMasterVolume */
    0,                                      /* 0x21 unk_21 */
    2,                                      /* 0x22 enemyBoostType */
    0,                                      /* 0x23 unk_23 */
    10,                                     /* 0x24 fastEnemyRestrainMax */
    15,                                     /* 0x25 fastEnemyRestrainThreshold1 */
    3,                                      /* 0x26 fastEnemyRestrainThreshold2 */
    0,                                      /* 0x27 unk_27 */
    10,                                     /* 0x28 slowEnemyBoostMax */
    20,                                     /* 0x29 slowEnemyBoostThreshold1 */
    4,                                      /* 0x2A slowEnemyBoostThreshold2 */
    0,                                      /* 0x2B unk_2B */
    0,                                      /* 0x2C twMaxTireWear */
    0,                                      /* 0x2D twMaxTireGripDown */
    100,                                    /* 0x2E twPitTireGripDown */
    0,                                      /* 0x2F twInitialTireWear */
    0,                                      /* 0x30 twInitialTireGripDown */
    0,                                      /* 0x31 twGoodTireWear */
    0,                                      /* 0x32 twGoodTireGripDown */
    0,                                      /* 0x33 twTerminalTireWear */
    0,                                      /* 0x34 twTerminalTireGripDown */
    { 0, 0, 0 },                            /* 0x35 unk_35 */
    0,                                      /* 0x38 timeLimit */
    0,                                      /* 0x3A launchPoint */
    { 0, 0, 0, 0 }                          /* 0x3C unk_3C */
};

/* The slow-car boost group; 1 for mode 1 or 2.  (A switch: the if / else
 * chain lays the blocks out in another order.) */
// FUNC 0x001DA1B8 rules_boost_preset
int rules_boost_preset(RaceRuleOptions *o, int mode)
{
    int on = 0;

    switch (mode) {
    case 1:
        o->enemyBoostType = 0;
        o->slowEnemyBoostMax = 15;
        o->slowEnemyBoostThreshold1 = 20;
        o->slowEnemyBoostThreshold2 = 100;
        on = 1;
        break;
    case 2:
        o->enemyBoostType = 0;
        o->slowEnemyBoostMax = 20;
        o->slowEnemyBoostThreshold1 = 10;
        o->slowEnemyBoostThreshold2 = 20;
        on = 1;
        break;
    default:
        o->enemyBoostType = ENEMY_BOOST_NONE;
        o->slowEnemyBoostMax = 0;
        o->slowEnemyBoostThreshold1 = 0;
        o->slowEnemyBoostThreshold2 = 0;
        break;
    }
    return on;
}

/* The tyre wear group (tw*; twGoodTireGripDown is left alone when off);
 * returns mode != 0.  The stores in the order initial, good, terminal,
 * max, pit (sched1 puts each block's last store first). */
// FUNC 0x001DA238 rules_tire_wear_preset
int rules_tire_wear_preset(RaceRuleOptions *o, int mode)
{
    switch (mode) {
    case 1:
        o->twInitialTireWear = 2;
        o->twInitialTireGripDown = 6;
        o->twGoodTireWear = 8;
        o->twGoodTireGripDown = 0;
        o->twTerminalTireWear = 30;
        o->twTerminalTireGripDown = 5;
        o->twMaxTireWear = 42;
        o->twMaxTireGripDown = 30;
        o->twPitTireGripDown = 9;
        break;
    case 2:
        o->twInitialTireWear = 2;               /* stored from the mode register (2) */
        o->twInitialTireGripDown = 6;
        o->twGoodTireWear = 4;
        o->twGoodTireGripDown = 0;
        o->twTerminalTireWear = 15;
        o->twTerminalTireGripDown = 5;
        o->twMaxTireWear = 21;
        o->twMaxTireGripDown = 30;
        o->twPitTireGripDown = 9;
        break;
    default:
        o->twInitialTireWear = 0;
        o->twInitialTireGripDown = 0;
        o->twGoodTireWear = 0;
        o->twTerminalTireWear = 0;
        o->twTerminalTireGripDown = 0;
        o->twMaxTireWear = 0;
        o->twMaxTireGripDown = 0;
        o->twPitTireGripDown = 0;
        break;
    }
    return mode != 0;
}
