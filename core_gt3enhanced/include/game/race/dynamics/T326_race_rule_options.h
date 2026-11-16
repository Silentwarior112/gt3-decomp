/* T326_race_rule_options.h - RaceRuleOptions, the rules of a race
 * (translation unit T326, unit u076), 0x001DA708..0x001DA968.
 *
 * DynamicsConductor +0xE340 (setFromRace, from F_001F35B8) and the default
 * g_002D2988 (T281).  setFromRace copies the rule bytes of the race record
 * (RaceEvent +0x198..+0x1C3, T484) and its limitTime / launchPoint; the
 * names are RaceEvent's.  T281 has presets of the boost and tyre wear groups
 * for the battle modes; WearCurve (T327) is built from the tyre wear bytes;
 * CarDynamics_init (T284) reads powerLimit and the enemy grip volumes. */
#ifndef GAME_T326_RACE_RULE_OPTIONS_H
#define GAME_T326_RACE_RULE_OPTIONS_H

#include "game/game.h"

class RaceDataBase;                         /* T484_race_database.h (pointers only) */

/* Not polymorphic.  Size 0x40 (DynamicsConductor +0xE340, then +0xE380;
 * T281's .data g_002D2988..0x002D29C8).  The enemy arrays are indexed by
 * CarDynamics.driveClass.  (Lupo 2: _GtbRaceParameterSource, the type of
 * DynamicsConductor::GetRunTimeRaceParameterSource and of the slots 9 / 10
 * SetTireWearParameters / SetSlowCarBoostParameters.) */
class RaceRuleOptions {
public:
    /* memset 0, then the rule bytes of db->event (u076: a RaceSetup with the
     * record at +0x18, u085: RaceDataBase); twGood* = 4 / 0 when
     * twMaxTireWear is set. */
    void setFromRace(RaceDataBase *db);     /* 0x001DA708 */
    /* Inline (u085 matching: DynamicsConductor's getStartingFormat,
     * getRuleStartSpeed and getRuleStartArg keep &rules in a register). */
    int  getLaunchSpeed() { return launchSpeed; }
    int  getLaunchPoint() { return launchPoint; }

    u8  launchSpeed;                        /* 0x00 RaceEvent +0x198 (u085 startSpeed: ? km/h of a rolling */
                                            /* start, 0 standing; u083 / u084 b00, DynamicsConductor::getStartingFormat) */
    u8  laps;                               /* 0x01 +0x199 numberOfLaps */
    u8  licenseType;                        /* 0x02 +0x19A (u083 / u084 kind: 1 a stop-in-the-box licence */
                                            /* test, 2 ends at the first line, 3 multi-lap) */
    u8  powerLimit;                         /* 0x03 x 10 ps -> engine_setup (setFromRace: 0) */
    u8  enemyMuVolumeF[4];                  /* 0x04 +0x19C % -> CarDynamics.axleGrip[0] of computer cars */
    u8  enemyMuVolumeR[4];                  /* 0x08 +0x1A0 % -> axleGrip[1] */
    u8  enemySkillA[4];                     /* 0x0C +0x1A4 */
    u8  enemySkillB[4];                     /* 0x10 +0x1A8 */
    u8  enemySkillC[4];                     /* 0x14 +0x1AC */
    s8  enemySkillC2[4];                    /* 0x18 +0x1B0 a signed km/h offset (lb: ai_apply_rules, T362) */
    u8  unk_1C[4];                          /* 0x1C */
    u8  enemyDtMasterVolume;                /* 0x20 +0x1B4 */
    u8  unk_21;                             /* 0x21 (RaceEvent +0x1B5 entryCount is not copied) */
    u8  enemyBoostType;                     /* 0x22 +0x1B6 (u083 / u084 b22: Battle / SinglePlayer slot 31; */
                                            /* 0xFF: no boost, rules_boost_preset) */
    u8  unk_23;                             /* 0x23 */
    u8  fastEnemyRestrainMax;               /* 0x24 +0x1B7 */
    u8  fastEnemyRestrainThreshold1;        /* 0x25 +0x1B8 */
    u8  fastEnemyRestrainThreshold2;        /* 0x26 +0x1B9 */
    u8  unk_27;                             /* 0x27 */
    u8  slowEnemyBoostMax;                  /* 0x28 +0x1BC */
    u8  slowEnemyBoostThreshold1;           /* 0x29 +0x1BA */
    u8  slowEnemyBoostThreshold2;           /* 0x2A +0x1BB */
    u8  unk_2B;                             /* 0x2B */
    u8  twMaxTireWear;                      /* 0x2C +0x1BD: WearCurve x3 / 10000 (0: no wear) */
    u8  twMaxTireGripDown;                  /* 0x2D +0x1BE: y3 x 100 */
    u8  twPitTireGripDown;                  /* 0x2E +0x1BF: WearCurve.pitGrip = (100 - it) / 100 */
    u8  twInitialTireWear;                  /* 0x2F +0x1C0: -x0 / 10000 */
    u8  twInitialTireGripDown;              /* 0x30 +0x1C1: y0 x 100 */
    u8  twGoodTireWear;                     /* 0x31 x1 / 10000 (? the record's +0x1E0 is not copied: 4) */
    u8  twGoodTireGripDown;                 /* 0x32 y1 x 100 (? +0x1E1: 0) */
    u8  twTerminalTireWear;                 /* 0x33 +0x1C2: x2 / 10000 */
    u8  twTerminalTireGripDown;             /* 0x34 +0x1C3: y2 x 100 */
    u8  unk_35[3];                          /* 0x35 */
    u16 timeLimit;                          /* 0x38 +0x1E4 limitTime, minutes (0: none; lhu x 60000) */
    s16 launchPoint;                        /* 0x3A +0x1E6 (lh in u085) */
    u8  unk_3C[4];                          /* 0x3C */
};
EGCS_SIZE(RaceRuleOptions, 0x40);

#endif /* GAME_T326_RACE_RULE_OPTIONS_H */
