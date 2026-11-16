/* T484 - RaceDataBase: a CarDataBase with the selected race (event) record
 * of table 0x20: its entry cars (table 0x1F preset cars), the regulated cars
 * (a table 0x21 car list), prize money, target times and the enemy / tyre
 * wear parameters; the course records of table 0x22.
 * 0x00238390..0x00239008.
 *
 * Names: the getters 0x00238B80..0x00238F68 are Lupo 2's; the others
 * describe what the listing does (no symbols). */
#include "game/data/race_database_internal.h"
#include "game/data/T484_race_database.h"

/* 0x00352D78 (.sdata, gp-0x4C78): the championship points of positions
 * 1..8 (getChampionshipPoint), defined here: T484's .sdata is this table,
 * then the strings "-", "-----" and "%s %s", as in retail. */
const s8 g_00352D78[8] = {10, 6, 4, 3, 2, 1, 0, 0};

/* Emitted: CarDataBase::CarDataBase, the vptr (0x003423D8), clear(), then
 * the car database g_002EBD88 (copy); on an exception ~CarDataBase.  40
 * callers. */
// FUNC 0x00238390 RaceDataBase::RaceDataBase
RaceDataBase::RaceDataBase()
{
    /* implicit: CarDataBase, ~CarDataBase (compiler-generated: bases, members, deleting form) */
    /* CarDataBase(), the vptr */
    clear();
    copy(g_002EBD88);
}

/* (index.txt: RaceDataBase::virtual_0.)  The vptr, then
 * CarDataBase::~CarDataBase with the caller's flags (the deleting form is
 * there). */
// FUNC 0x00238408 RaceDataBase::~RaceDataBase
RaceDataBase::~RaceDataBase()
{
    /* implicit: ~CarDataBase (compiler-generated: bases, members, deleting form) */
    /* ~CarDataBase (egcs: called with the flags) */
}

/* No race selected.  Caller: the constructor. */
// FUNC 0x00238430 RaceDataBase::clear
void RaceDataBase::clear()
{
    code   = 0;
    course = 0;
    event  = NULL;
}

/* Selects race `code' (table 0x20) and its course; 0 if there is none
 * (nothing changed then).  34 callers. */
// FUNC 0x00238440 RaceDataBase::select
int RaceDataBase::select(u64 code)
{
    RaceEvent *ev;

    ev = (RaceEvent *)getRecord(CARDB_RACE, code);
    if (ev == NULL)
        return 0;
    this->code = code;
    event      = ev;
    course     = ev->course;
    return 1;
}

/* Callers: RaceInformation::virtual_3, RaceOrganization::setCourseCode,
 * RaceSinglePlayerInformation::virtual_3. */
// FUNC 0x002384A0 RaceDataBase::setCourse
void RaceDataBase::setCourse(u64 course)
{
    this->course = course;
}

/* The course set, else the event's. */
// FUNC 0x002384A8 RaceDataBase::getCourse
u64 RaceDataBase::getCourse()
{
    if (course == 0)
        return event->course;
    return course;
}

/* Entry car i (a table 0x1F code). */
// FUNC 0x002384C8 RaceDataBase::getEntryCar
u64 RaceDataBase::getEntryCar(int i)
{
    return event->entries[i];
}

/* The entry cars before the first "-" (at most 32). */
// FUNC 0x002384E0 RaceDataBase::getNumEntryCars
int RaceDataBase::getNumEntryCars()
{
    u64 *entries = event->entries;
    u64  none = nameToCode("-");
    int  n;

    for (n = 0; n < 0x20; n++) {
        if (entries[n] == none)
            break;
    }
    return n;
}

/* Sets `car' up as entry car i: its table 0x1F preset (setPreset), the
 * entry's colour, the preset's settings (loadPresetSettings), and marks it a
 * preset car; 0 if there is no record or no such car.  Callers:
 * make_license_car_name, run_license_test, RaceInformation::virtual_12. */
// FUNC 0x00238550 RaceDataBase::setupEntryCar
int RaceDataBase::setupEntryCar(int i, CarEquipments *car)
{
    PresetCarRecord *preset;

    preset = (PresetCarRecord *)getRecord(CARDB_ENTRY_CAR, event->entries[i]);
    if (preset == NULL || car->setPreset(preset) == 0)
        return 0;
    car->setColorCode(event->entryColor[i]);
    car->loadPresetSettings(preset);
    car->presetCar = 1;         /* a store, not setPresetCar */
    return 1;
}

/* The colour of entry car i.  Caller: run_license_test. */
// FUNC 0x002385F8 RaceDataBase::getEntryColor
int RaceDataBase::getEntryColor(int i)
{
    return event->entryColor[i];
}

/* The model of entry car i: the `model' of its preset's racing modification
 * record (the car whose model and colours are used); no NULL checks.  No
 * caller. */
// FUNC 0x00238610 RaceDataBase::F_00238610
u64 RaceDataBase::F_00238610(int i)
{
    PresetCarRecord *preset;

    preset = (PresetCarRecord *)getRecord(CARDB_ENTRY_CAR, event->entries[i]);
    return ((RacingModifyRecord *)getRecord(CARDB_RACING_MODIFY, preset->parts.racingModify))->model;
}

/* The regulated cars of the race (its table 0x21 car list in the car
 * database g_002EBD88) before the first "-----" (at most 32); 0 if any car
 * may enter ("-").  The code of "-----" is computed for each car.  Callers:
 * regulated_car_name, create_menu_regulated_car, make_regulation_car. */
// FUNC 0x00238660 RaceDataBase::getNumRegulatedCars
int RaceDataBase::getNumRegulatedCars()
{
    RaceEvent     *ev;
    CarListRecord *list;
    const u64     *car;
    int            n;

    ev = event;
    if (ev->carList == nameToCode("-"))
        return 0;
    list = (CarListRecord *)g_002EBD88.getRecord(CARDB_CAR_LIST, event->carList);
    car  = list->cars;
    for (n = 0; n < 0x20; n++) {
        u64 none = nameToCode("-----");
        if (*car++ == none)
            break;
    }
    return n;
}

/* The name (first part, getName1) of regulated car i, from a CarEquipments
 * built on the stack; NULL if any car may enter.  The car's pointer in two
 * steps (`car = list->cars; car += i;', as getRegulatedCarFullName /
 * getRegulatedCar): retail adds the scaled index to the list's start in its
 * own register.  No caller. */
// FUNC 0x00238700 RaceDataBase::getRegulatedCarName
const char *RaceDataBase::getRegulatedCarName(int i)
{
    RaceEvent     *ev;
    CarListRecord *list;
    const u64     *car;

    ev = event;
    if (ev->carList == nameToCode("-"))
        return NULL;
    list = (CarListRecord *)g_002EBD88.getRecord(CARDB_CAR_LIST, event->carList);
    car  = list->cars;
    car += i;
    {
        CarEquipments equip;        /* sp+0x000 */
        const char   *name;

        equip.setCar(*car);
        name = equip.getName1();
        return name;
    }
}

/* "%s %s" of the two name parts of regulated car i into buf; an empty
 * string if any car may enter.  Callers: regulated_car_name,
 * create_menu_regulated_car. */
// FUNC 0x002387D0 RaceDataBase::getRegulatedCarFullName
void RaceDataBase::getRegulatedCarFullName(char *buf, int i)
{
    RaceEvent     *ev;
    CarListRecord *list;
    const u64     *car;

    ev = event;
    if (ev->carList == nameToCode("-")) {
        buf[0] = 0;
        return;
    }
    list = (CarListRecord *)g_002EBD88.getRecord(CARDB_CAR_LIST, event->carList);
    car  = list->cars;
    car += i;
    {
        CarEquipments equip;        /* sp+0x000 */

        equip.setCar(*car);
        sprintf(buf, "%s %s", equip.getName1(), equip.getName2());
    }
}

/* The code of regulated car i, 0 if any car may enter.  Caller:
 * make_regulation_car. */
// FUNC 0x002388D8 RaceDataBase::getRegulatedCar
u64 RaceDataBase::getRegulatedCar(int i)
{
    RaceEvent     *ev;
    CarListRecord *list;
    const u64     *car;

    ev = event;
    if (ev->carList == nameToCode("-"))
        return 0;
    list = (CarListRecord *)g_002EBD88.getRecord(CARDB_CAR_LIST, event->carList);
    car  = list->cars;
    car += i;
    return *car;
}

/* The prize money of position `rank': prize[rank] * 100, again x 100 in
 * region 0. */
// FUNC 0x00238950 RaceDataBase::getPrize
int RaceDataBase::getPrize(int rank)
{
    int money;

    money = event->prize[rank] * 100;
    if (isJapan() != 0)
        money *= 100;
    return money;
}

/* The kind of course `course' (table 0x22; 2 = dirt), no NULL check.  11
 * callers (RaceBase::virtual_39, RaceSinglePlayerInformation::virtual_3,
 * ...). */
// FUNC 0x002389A0 RaceDataBase::getCourseKind
u8 RaceDataBase::getCourseKind(u64 course)
{
    return ((CourseRecord *)getRecord(CARDB_COURSE, course))->kind;
}

/* String name0A (of `strings') of course `course' (u109 db_course_name: the
 * course's name), no NULL check. */
// FUNC 0x002389C8 RaceDataBase::F_002389C8
const char *RaceDataBase::F_002389C8(u64 course)
{
    return strings->getString(((CourseRecord *)getRecord(CARDB_COURSE, course))->name0A);
}

/* String name0C (of `uniStrings') of course `course', NULL if there is no
 * such course.  Callers: memcard_text_replay_course, RaceDisplay::virtual_0,
 * ... */
// FUNC 0x00238A00 RaceDataBase::getCourseName
const char *RaceDataBase::getCourseName(u64 course)
{
    CourseRecord *rec;

    rec = (CourseRecord *)getRecord(CARDB_COURSE, course);
    if (rec == NULL)
        return NULL;
    return uniStrings->getString(rec->name0C);
}

/* String name0E (of `strings') of the table 0x22 record `code' (no NULL
 * check). */
// FUNC 0x00238A48 RaceDataBase::F_00238A48
const char *RaceDataBase::F_00238A48(u64 code)
{
    return strings->getString(((CourseRecord *)getRecord(CARDB_COURSE, code))->name0E);
}

/* The championship points of position `rank' (signed bytes {10, 6, 4, 3, 2,
 * 1, 0, 0}; `this' unused).  Callers: ResultChampionship::update,
 * ResultChampionship::sortStandings, RaceChampionshipInformation::addPoints,
 * RaceChampionshipInformation::addEnemyPoints. */
// FUNC 0x00238A80 RaceDataBase::getChampionshipPoint
s8 RaceDataBase::getChampionshipPoint(int rank)
{
    return g_00352D78[rank];
}

/* F_00238A48 of the selected race's code (a race code looked up in the
 * course table).  No caller. */
// FUNC 0x00238A90 RaceDataBase::F_00238A90
const char *RaceDataBase::F_00238A90()
{
    return F_00238A48(code);
}

/* getCourseName of the course member (not getCourse()).  No caller. */
// FUNC 0x00238AB0 RaceDataBase::F_00238AB0
const char *RaceDataBase::F_00238AB0()
{
    return getCourseName(course);
}

/* F_002389C8 of the course member.  No caller. */
// FUNC 0x00238AD0 RaceDataBase::F_00238AD0
const char *RaceDataBase::F_00238AD0()
{
    return F_002389C8(course);
}

/* The bronze (third) time in ms: min * 60000 + sec * 1000 + ms. */
// FUNC 0x00238AF0 RaceDataBase::getBronzeTime
int RaceDataBase::getBronzeTime()
{
    RaceEvent *ev = event;

    return ev->time[2].min * 60000 + ev->time[2].sec * 1000 + ev->time[2].ms;
}

/* The silver (second) time in ms. */
// FUNC 0x00238B20 RaceDataBase::getSilverTime
int RaceDataBase::getSilverTime()
{
    RaceEvent *ev = event;

    return ev->time[1].min * 60000 + ev->time[1].sec * 1000 + ev->time[1].ms;
}

/* The gold (first) time in ms. */
// FUNC 0x00238B50 RaceDataBase::getGoldTime
int RaceDataBase::getGoldTime()
{
    RaceEvent *ev = event;

    return ev->time[0].min * 60000 + ev->time[0].sec * 1000 + ev->time[0].ms;
}

// FUNC 0x00238B80 RaceDataBase::getLicenseType
u8 RaceDataBase::getLicenseType()
{
    return event->licenseType;
}

// FUNC 0x00238B90 RaceDataBase::getLaunchPoint
u16 RaceDataBase::getLaunchPoint()
{
    return event->launchPoint;
}

// FUNC 0x00238BA0 RaceDataBase::getLimitTime
u8 RaceDataBase::getLimitTime()
{
    return event->limitTime;
}

// FUNC 0x00238BB0 RaceDataBase::getTrophyType
u8 RaceDataBase::getTrophyType()
{
    return event->trophyType;
}

/* The event's bonus * 100, again x 100 in region 0, as getPrize does (?
 * what the bonus is: its only caller F_00238F98 has none). */
// FUNC 0x00238BC0 RaceDataBase::F_00238BC0
int RaceDataBase::F_00238BC0()
{
    int money;

    money = event->bonus * 100;
    if (isJapan() != 0)
        money *= 100;
    return money;
}

// FUNC 0x00238C08 RaceDataBase::getNeedTire
u8 RaceDataBase::getNeedTire()
{
    return event->needTire;
}

// FUNC 0x00238C18 RaceDataBase::getNeedAspiration
u8 RaceDataBase::getNeedAspiration()
{
    return event->needAspiration;
}

// FUNC 0x00238C28 RaceDataBase::getNeedTuning
u8 RaceDataBase::getNeedTuning()
{
    return event->needTuning;
}

/* Always 0. */
// FUNC 0x00238C38 RaceDataBase::getRestrictor
int RaceDataBase::getRestrictor()
{
    return 0;
}

// FUNC 0x00238C40 RaceDataBase::getNeedDrivetrain
u8 RaceDataBase::getNeedDrivetrain()
{
    return event->needDrivetrain;
}

/* The low word of the car list's code (sign-extended).  No caller. */
// FUNC 0x00238C50 RaceDataBase::F_00238C50
int RaceDataBase::F_00238C50()
{
    return (int)event->carList;
}

// FUNC 0x00238C68 RaceDataBase::getTwGoodTireGripDown
u8 RaceDataBase::getTwGoodTireGripDown()
{
    return event->twGoodTireGripDown;
}

// FUNC 0x00238C78 RaceDataBase::getTwGoodTireWear
u8 RaceDataBase::getTwGoodTireWear()
{
    return event->twGoodTireWear;
}

// FUNC 0x00238C88 RaceDataBase::getTwTerminalTireGripDown
u8 RaceDataBase::getTwTerminalTireGripDown()
{
    return event->twTerminalTireGripDown;
}

// FUNC 0x00238C98 RaceDataBase::getTwTerminalTireWear
u8 RaceDataBase::getTwTerminalTireWear()
{
    return event->twTerminalTireWear;
}

// FUNC 0x00238CA8 RaceDataBase::getTwInitialTireGripDown
u8 RaceDataBase::getTwInitialTireGripDown()
{
    return event->twInitialTireGripDown;
}

// FUNC 0x00238CB8 RaceDataBase::getTwInitialTireWear
u8 RaceDataBase::getTwInitialTireWear()
{
    return event->twInitialTireWear;
}

// FUNC 0x00238CC8 RaceDataBase::getTwPitTireGripDown
u8 RaceDataBase::getTwPitTireGripDown()
{
    return event->twPitTireGripDown;
}

// FUNC 0x00238CD8 RaceDataBase::getTwMaxTireGripDown
u8 RaceDataBase::getTwMaxTireGripDown()
{
    return event->twMaxTireGripDown;
}

// FUNC 0x00238CE8 RaceDataBase::getTwMaxTireWear
u8 RaceDataBase::getTwMaxTireWear()
{
    return event->twMaxTireWear;
}

// FUNC 0x00238CF8 RaceDataBase::getSlowEnemyBoostMax
u8 RaceDataBase::getSlowEnemyBoostMax()
{
    return event->slowEnemyBoostMax;
}

// FUNC 0x00238D08 RaceDataBase::getSlowEnemyBoostThreshold2
u8 RaceDataBase::getSlowEnemyBoostThreshold2()
{
    return event->slowEnemyBoostThreshold2;
}

// FUNC 0x00238D18 RaceDataBase::getSlowEnemyBoostThreshold1
u8 RaceDataBase::getSlowEnemyBoostThreshold1()
{
    return event->slowEnemyBoostThreshold1;
}

// FUNC 0x00238D28 RaceDataBase::getFastEnemyRestrainThreshold2
u8 RaceDataBase::getFastEnemyRestrainThreshold2()
{
    return event->fastEnemyRestrainThreshold2;
}

// FUNC 0x00238D38 RaceDataBase::getFastEnemyRestrainThreshold1
u8 RaceDataBase::getFastEnemyRestrainThreshold1()
{
    return event->fastEnemyRestrainThreshold1;
}

// FUNC 0x00238D48 RaceDataBase::getFastEnemyRestrainMax
u8 RaceDataBase::getFastEnemyRestrainMax()
{
    return event->fastEnemyRestrainMax;
}

// FUNC 0x00238D58 RaceDataBase::getEnemyBoostType
u8 RaceDataBase::getEnemyBoostType()
{
    return event->enemyBoostType;
}

// FUNC 0x00238D68 RaceDataBase::getEnemyDtMasterVolume
u8 RaceDataBase::getEnemyDtMasterVolume()
{
    return event->enemyDtMasterVolume;
}

// FUNC 0x00238D78 RaceDataBase::getEnemySkillA_GTR
u8 RaceDataBase::getEnemySkillA_GTR()
{
    return event->enemySkillA[3];
}

// FUNC 0x00238D88 RaceDataBase::getEnemySkillA_4WD
u8 RaceDataBase::getEnemySkillA_4WD()
{
    return event->enemySkillA[2];
}

// FUNC 0x00238D98 RaceDataBase::getEnemySkillA_FWD
u8 RaceDataBase::getEnemySkillA_FWD()
{
    return event->enemySkillA[1];
}

// FUNC 0x00238DA8 RaceDataBase::getEnemySkillA_RWD
u8 RaceDataBase::getEnemySkillA_RWD()
{
    return event->enemySkillA[0];
}

// FUNC 0x00238DB8 RaceDataBase::getEnemySkillB_GTR
u8 RaceDataBase::getEnemySkillB_GTR()
{
    return event->enemySkillB[3];
}

// FUNC 0x00238DC8 RaceDataBase::getEnemySkillB_4WD
u8 RaceDataBase::getEnemySkillB_4WD()
{
    return event->enemySkillB[2];
}

// FUNC 0x00238DD8 RaceDataBase::getEnemySkillB_FWD
u8 RaceDataBase::getEnemySkillB_FWD()
{
    return event->enemySkillB[1];
}

// FUNC 0x00238DE8 RaceDataBase::getEnemySkillB_RWD
u8 RaceDataBase::getEnemySkillB_RWD()
{
    return event->enemySkillB[0];
}

// FUNC 0x00238DF8 RaceDataBase::getEnemySkillC2_GTR
u8 RaceDataBase::getEnemySkillC2_GTR()
{
    return event->enemySkillC2[3];
}

// FUNC 0x00238E08 RaceDataBase::getEnemySkillC2_4WD
u8 RaceDataBase::getEnemySkillC2_4WD()
{
    return event->enemySkillC2[2];
}

// FUNC 0x00238E18 RaceDataBase::getEnemySkillC2_FWD
u8 RaceDataBase::getEnemySkillC2_FWD()
{
    return event->enemySkillC2[1];
}

// FUNC 0x00238E28 RaceDataBase::getEnemySkillC2_RWD
u8 RaceDataBase::getEnemySkillC2_RWD()
{
    return event->enemySkillC2[0];
}

// FUNC 0x00238E38 RaceDataBase::getEnemySkillC_GTR
u8 RaceDataBase::getEnemySkillC_GTR()
{
    return event->enemySkillC[3];
}

// FUNC 0x00238E48 RaceDataBase::getEnemySkillC_4WD
u8 RaceDataBase::getEnemySkillC_4WD()
{
    return event->enemySkillC[2];
}

// FUNC 0x00238E58 RaceDataBase::getEnemySkillC_FWD
u8 RaceDataBase::getEnemySkillC_FWD()
{
    return event->enemySkillC[1];
}

// FUNC 0x00238E68 RaceDataBase::getEnemySkillC_RWD
u8 RaceDataBase::getEnemySkillC_RWD()
{
    return event->enemySkillC[0];
}

// FUNC 0x00238E78 RaceDataBase::getEnemyMuVolumeR_GTR
u8 RaceDataBase::getEnemyMuVolumeR_GTR()
{
    return event->enemyMuVolumeR[3];
}

// FUNC 0x00238E88 RaceDataBase::getEnemyMuVolumeR_4WD
u8 RaceDataBase::getEnemyMuVolumeR_4WD()
{
    return event->enemyMuVolumeR[2];
}

// FUNC 0x00238E98 RaceDataBase::getEnemyMuVolumeR_FWD
u8 RaceDataBase::getEnemyMuVolumeR_FWD()
{
    return event->enemyMuVolumeR[1];
}

// FUNC 0x00238EA8 RaceDataBase::getEnemyMuVolumeR_RWD
u8 RaceDataBase::getEnemyMuVolumeR_RWD()
{
    return event->enemyMuVolumeR[0];
}

// FUNC 0x00238EB8 RaceDataBase::getEnemyMuVolumeF_GTR
u8 RaceDataBase::getEnemyMuVolumeF_GTR()
{
    return event->enemyMuVolumeF[3];
}

// FUNC 0x00238EC8 RaceDataBase::getEnemyMuVolumeF_4WD
u8 RaceDataBase::getEnemyMuVolumeF_4WD()
{
    return event->enemyMuVolumeF[2];
}

// FUNC 0x00238ED8 RaceDataBase::getEnemyMuVolumeF_FWD
u8 RaceDataBase::getEnemyMuVolumeF_FWD()
{
    return event->enemyMuVolumeF[1];
}

// FUNC 0x00238EE8 RaceDataBase::getEnemyMuVolumeF_RWD
u8 RaceDataBase::getEnemyMuVolumeF_RWD()
{
    return event->enemyMuVolumeF[0];
}

// FUNC 0x00238EF8 RaceDataBase::getNeedLicense
u8 RaceDataBase::getNeedLicense()
{
    return event->needLicense;
}

// FUNC 0x00238F08 RaceDataBase::getLaunchSpeed
u8 RaceDataBase::getLaunchSpeed()
{
    return event->launchSpeed;
}

// FUNC 0x00238F18 RaceDataBase::getNumberOfLaps
u8 RaceDataBase::getNumberOfLaps()
{
    return event->numberOfLaps;
}

// FUNC 0x00238F28 RaceDataBase::getStartAction
u8 RaceDataBase::getStartAction()
{
    return event->startAction;
}

// FUNC 0x00238F38 RaceDataBase::getCountDown
u8 RaceDataBase::getCountDown()
{
    return event->countDown;
}

// FUNC 0x00238F48 RaceDataBase::getRaceModeCode
u64 RaceDataBase::getRaceModeCode()
{
    return event->raceModeCode;
}

// FUNC 0x00238F58 RaceDataBase::getDifficulty
u8 RaceDataBase::getDifficulty()
{
    return event->difficulty;
}

// FUNC 0x00238F68 RaceDataBase::getEntries
u8 RaceDataBase::getEntries()
{
    return event->entryCount;
}

/* getCourseKind of the course member.  No caller. */
// FUNC 0x00238F78 RaceDataBase::F_00238F78
u8 RaceDataBase::F_00238F78()
{
    return getCourseKind(course);
}

/* 1 if F_00238BC0 is not 0 (the race has a bonus).  No caller. */
// FUNC 0x00238F98 RaceDataBase::F_00238F98
int RaceDataBase::F_00238F98()
{
    return F_00238BC0() != 0;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __si_type_info __ti12RaceDataBase (base
 * CarDataBase, __ti11CarDataBase gp-0x4560) on the first call; vtable
 * 0x003423D8 entry 0. */
// FUNC 0x00238FB8 __tf12RaceDataBase
extern "C" void *__tf12RaceDataBase(void)
{
    if (__ti12RaceDataBase[0] == 0) {
        __tf11CarDataBase();
        __rtti_si(__ti12RaceDataBase, "12RaceDataBase", (const type_info *)__ti11CarDataBase);
    }
    return __ti12RaceDataBase;
}
#endif
