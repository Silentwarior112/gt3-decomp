/* T326 - RaceRuleOptions from the race record (DynamicsConductor +0xE340,
 * from DynamicsConductor::attach). */
#include "game/car/physics/car_spec_internal.h"
#include "game/race/dynamics/T326_race_rule_options.h"

/* The record is read through db->event again for every byte (the stores
 * may alias it). */
// FUNC 0x001DA708 RaceRuleOptions::setFromRace
void RaceRuleOptions::setFromRace(RaceDataBase *db)
{
    int restrictor = 0;

    memset(this, 0, sizeof(RaceRuleOptions));
    launchSpeed = db->event->launchSpeed;
    laps = db->event->numberOfLaps;
    licenseType = db->event->licenseType;
    /* A value / 10 the compiler reduced to 0: only the divide-by-zero check
     * of the 10 is left in the code.  (? db->getRestrictor() / 10, with an
     * inline getRestrictor returning 0: T484's out-of-line copy does.) */
    powerLimit = restrictor / 10;
    timeLimit = db->event->limitTime;
    launchPoint = db->event->launchPoint;
    enemyMuVolumeF[0] = db->event->enemyMuVolumeF[0];
    enemyMuVolumeF[1] = db->event->enemyMuVolumeF[1];
    enemyMuVolumeF[2] = db->event->enemyMuVolumeF[2];
    enemyMuVolumeF[3] = db->event->enemyMuVolumeF[3];
    enemyMuVolumeR[0] = db->event->enemyMuVolumeR[0];
    enemyMuVolumeR[1] = db->event->enemyMuVolumeR[1];
    enemyMuVolumeR[2] = db->event->enemyMuVolumeR[2];
    enemyMuVolumeR[3] = db->event->enemyMuVolumeR[3];
    enemySkillA[0] = db->event->enemySkillA[0];
    enemySkillA[1] = db->event->enemySkillA[1];
    enemySkillA[2] = db->event->enemySkillA[2];
    enemySkillA[3] = db->event->enemySkillA[3];
    enemySkillB[0] = db->event->enemySkillB[0];
    enemySkillB[1] = db->event->enemySkillB[1];
    enemySkillB[2] = db->event->enemySkillB[2];
    enemySkillB[3] = db->event->enemySkillB[3];
    enemySkillC[0] = db->event->enemySkillC[0];
    enemySkillC[1] = db->event->enemySkillC[1];
    enemySkillC[2] = db->event->enemySkillC[2];
    enemySkillC[3] = db->event->enemySkillC[3];
    enemySkillC2[0] = db->event->enemySkillC2[0];
    enemySkillC2[1] = db->event->enemySkillC2[1];
    enemySkillC2[2] = db->event->enemySkillC2[2];
    enemySkillC2[3] = db->event->enemySkillC2[3];
    enemyDtMasterVolume = db->event->enemyDtMasterVolume;
    enemyBoostType = db->event->enemyBoostType;
    fastEnemyRestrainMax = db->event->fastEnemyRestrainMax;
    fastEnemyRestrainThreshold1 = db->event->fastEnemyRestrainThreshold1;
    fastEnemyRestrainThreshold2 = db->event->fastEnemyRestrainThreshold2;
    slowEnemyBoostMax = db->event->slowEnemyBoostMax;
    slowEnemyBoostThreshold1 = db->event->slowEnemyBoostThreshold1;
    slowEnemyBoostThreshold2 = db->event->slowEnemyBoostThreshold2;
    twMaxTireWear = db->event->twMaxTireWear;
    twMaxTireGripDown = db->event->twMaxTireGripDown;
    twPitTireGripDown = db->event->twPitTireGripDown;
    twInitialTireWear = db->event->twInitialTireWear;
    twInitialTireGripDown = db->event->twInitialTireGripDown;
    twTerminalTireWear = db->event->twTerminalTireWear;
    twTerminalTireGripDown = db->event->twTerminalTireGripDown;
    /* The record's own twGoodTireWear / twGoodTireGripDown (+0x1E0 / +0x1E1)
     * are not used: fixed at 4 and 0 when the tyre wear is on. */
    if (twMaxTireWear != 0) {
        twGoodTireGripDown = 0;
        twGoodTireWear = 4;
    }
}
