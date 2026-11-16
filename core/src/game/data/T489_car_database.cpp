/* T489 - queries of the car database g_002EBD88 (cars 0x1E, entry / preset
 * cars 0x1F / 0x23, races 0x20, car lists 0x21, courses 0x22), the
 * regulation check of a car against a race (RegurationResult), race-name
 * parsing, the region lock of a car.  0x00239ED0..0x0023AE98. */
/* T429 (GarageCar, GarageWheel and their lists) before T045's
 * RegurationResult: egcs writes the type names of the keyless classes in
 * reverse definition order, and retail T489's .rodata has
 * "16RegurationResult" first, right after the vtable (0x00342718), then the
 * garage names (__tf16RegurationResult's data reference).  The other headers
 * write no type name or string. */
#include "game/save/T429_game_status.h"              /* getGameStatus, GameStatus::getRunningCar /
                                                   getLicenseRecords (LicenseRecords::isPassed) */
#include "game/menu/gtmode/T045_reguration_result.h"        /* RegurationResult */
#include "libc.h"                               /* strlen */
#include "game/data/T481_car_data_base.h"            /* CarDataBase g_002EBD88, DataBlock, StringsDataBase,
                                                   nameToCode */
#include "game/data/T482_car_equipments.h"           /* CarRecord, PresetCarRecord, LAYOUT_*, PART_NONE */
#include "game/data/T483_car_information.h"          /* CarInformation */
#include "game/data/T484_race_database.h"            /* RaceEvent (RaceRecord), CourseRecord, NEED_* */
#include "game/data/T516_country.h"                  /* Country g_00352FD8, COUNTRY_* */
#include "game/car/model/T278_show_room_car.h"            /* ShowRoomCar (check_regulation's stack record) */
#include "game/data/messages/T493_msg_database_race_category.h" /* g_002F12A0 (race_kind_text) */
#include "game/data/car_database_internal.h"
#include "game/data/T489_car_database.h"

/* 1 if every flag but [2] is 0. */
// FUNC 0x00239ED0 RegurationResult::isOk
int RegurationResult::isOk()
{
    for (int i = 0; i < 7; i++) {
        if (i != 2 && flags[i] != 0)
            return 0;
    }
    return 1;
}

/* Index of a race, -1 if absent.  No callers. */
// FUNC 0x00239F08 db_race_index
int db_race_index(s64 race)
{
    return g_002EBD88.getIndex(DB_TABLE_RACE, race);
}

/* Index of a car, -1 if absent. */
// FUNC 0x00239F30 db_car_index
int db_car_index(s64 car)
{
    return g_002EBD88.getIndex(DB_TABLE_CAR, car);
}

/* Index of a course, -1 if absent. */
// FUNC 0x00239F58 db_course_index
int db_course_index(s64 course)
{
    return g_002EBD88.getIndex(DB_TABLE_COURSE, course);
}

/* Number of courses.  No callers. */
// FUNC 0x00239F80 db_course_count
int db_course_count(void)
{
    return g_002EBD88.getDataBlock(DB_TABLE_COURSE)->numElements;
}

/* Number of cars.  No callers. */
// FUNC 0x00239FA8 db_car_count
int db_car_count(void)
{
    return g_002EBD88.getDataBlock(DB_TABLE_CAR)->numElements;
}

/* Code of course i (the record's leading u64).  No callers. */
// FUNC 0x00239FD0 db_course_code
s64 db_course_code(int i)
{
    DataBlock *table = g_002EBD88.getDataBlock(DB_TABLE_COURSE);

    return *(s64 *)(table->elements + table->elementSize * i);
}

/* Code of car i.  No callers. */
// FUNC 0x0023A010 db_car_code
s64 db_car_code(int i)
{
    DataBlock *table = g_002EBD88.getDataBlock(DB_TABLE_CAR);

    return *(s64 *)(table->elements + table->elementSize * i);
}

/* Name (first part, uniStrings) of a car in the country's column; NULL if
 * the car is absent.  Jump table 0x003425D0 over the countries 0..6. */
// FUNC 0x0023A050 db_car_name
const char *db_car_name(s64 car)
{
    CarRecord *rec = (CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car);
    int text;

    if (rec == 0)
        return 0;
    switch (g_00352FD8.value) {
    default:
    case COUNTRY_JP:
        text = rec->name1R0;
        break;
    case COUNTRY_BR:
        text = rec->name1R2;
        break;
    case COUNTRY_US:
    case COUNTRY_FR:
    case COUNTRY_GR:
    case COUNTRY_IT:
    case COUNTRY_SP:
        text = rec->name1;
        break;
    }
    return g_002EBD88.uniStrings->getString(text);
}

/* The same for the second part of the name (jump table 0x003425F0). */
// FUNC 0x0023A0D8 db_car_name2
const char *db_car_name2(s64 car)
{
    CarRecord *rec = (CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car);
    int text;

    if (rec == 0)
        return 0;
    switch (g_00352FD8.value) {
    default:
    case COUNTRY_JP:
        text = rec->name2R0;
        break;
    case COUNTRY_BR:
        text = rec->name2R2;
        break;
    case COUNTRY_US:
    case COUNTRY_FR:
    case COUNTRY_GR:
    case COUNTRY_IT:
    case COUNTRY_SP:
        text = rec->name2;
        break;
    }
    return g_002EBD88.uniStrings->getString(text);
}

/* Short name (CarRecord::shortName, strings) of a car; `code` may also be a
 * preset car (table 0x23) or an entry car (0x1F), which name their car.  No
 * callers; the car record is not NULL-checked. */
// FUNC 0x0023A160 db_car_short_name
const char *db_car_short_name(s64 code)
{
    PresetCarRecord *preset = (PresetCarRecord *)g_002EBD88.getRecord(DB_TABLE_PRESET_CAR, code);
    s64 car;

    if (preset != 0)
        car = preset->car;
    else if ((preset = (PresetCarRecord *)g_002EBD88.getRecord(DB_TABLE_ENTRY_CAR, code)) != 0)
        car = preset->car;
    else
        car = code;
    return g_002EBD88.strings->getString(
        ((CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car))->shortName);
}

/* Name of a course (CourseRecord::name0A, strings; not NULL-checked). */
// FUNC 0x0023A1F0 db_course_name
const char *db_course_name(s64 course)
{
    return g_002EBD88.strings->getString(
        ((CourseRecord *)g_002EBD88.getRecord(DB_TABLE_COURSE, course))->name0A);
}

/* Name of a race (RaceEvent::name, strings; not NULL-checked). */
// FUNC 0x0023A230 db_race_name
const char *db_race_name(s64 race)
{
    return g_002EBD88.strings->getString(
        ((RaceRecord *)g_002EBD88.getRecord(DB_TABLE_RACE, race))->name);
}

/* check_regulation() for a copy of the CarInformation of the car being
 * ridden (getRunningCar() + 0x10, not NULL-checked).  The copy is the inline
 * implicit copy constructor: the 0x1D0 bytes of CarSaveEquipments, the
 * CarDataBase words, presetCar, CarSaveInformation (+0x1E8..+0x213), the vptr
 * 0x00342388 (through 0x00341CC0, 0x003422C0); then ~CarInformation(copy, 2)
 * (CarInformation::~CarInformation 0x00237958, also on the landing pad).  No
 * callers. */
// FUNC 0x0023A270 check_regulation_current_car
RegurationResult check_regulation_current_car(s64 race)
{
    CarInformation car(*getGameStatus()->getRunningCar());

    return check_regulation(race, &car);
}

/* The front tyre part ids (PART_FRONT_TIRE + kind) the list knows, the
 * control tyres first; PART_NONE ends it.  Not const: retail's is in .data
 * (0x002F0670, in front of T489's frame info). */
int g_002F0670[11] = {                          /* 0x002F0670 */
    PART_TIRE_CONTROL, PART_FRONT_TIRE, PART_TIRE_SPORTS, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, PART_TIRE_SUPER_SOFT, PART_NONE
};

/* Index of a tyre part id in g_002F0670 (PART_NONE ends it); the length when
 * absent.  No callers. */
// FUNC 0x0023A3F0 tire_part_index
int tire_part_index(int part)
{
    int i;

    for (i = 0; g_002F0670[i] != PART_NONE; i++)
        if (g_002F0670[i] == part)
            break;
    return i;
}

/* Checks `car` against the rules of race `race` (returned by value in the
 * caller's slot, $a0; the race record is not NULL-checked).  The showroom
 * record is built and never read.  Each broken rule sets its flag:
 * [1] licence, [3] drive train, [4] the race's car list, [5] aspiration,
 * [6] tuning, [0] tyres. */
// FUNC 0x0023A438 check_regulation
RegurationResult check_regulation(s64 race, CarInformation *car)
{
    RegurationResult result;
    RaceRecord *rec = (RaceRecord *)g_002EBD88.getRecord(DB_TABLE_RACE, race);
    ShowRoomCar showroom;

    showroom.SetupShowRoomCar(car, 0);  /* dead: the record is not read here */

    /* the licence: the 8 tests of licence needLicense - 1 passed (jump table
     * 0x00342610) */
    switch (rec->needLicense) {
    case NEED_LICENSE_B:
        if (getGameStatus()->getLicenseRecords()->isPassed(0, 0) == 0)
            result.flags[1] = 1;
        break;
    case NEED_LICENSE_A:
        if (getGameStatus()->getLicenseRecords()->isPassed(1, 0) == 0)
            result.flags[1] = 1;
        break;
    case NEED_LICENSE_IB:
        if (getGameStatus()->getLicenseRecords()->isPassed(2, 0) == 0)
            result.flags[1] = 1;
        break;
    case NEED_LICENSE_IA:
        if (getGameStatus()->getLicenseRecords()->isPassed(3, 0) == 0)
            result.flags[1] = 1;
        break;
    case NEED_LICENSE_S:
        if (getGameStatus()->getLicenseRecords()->isPassed(4, 0) == 0)
            result.flags[1] = 1;
        break;
    case NEED_LICENSE_R:
        if (getGameStatus()->getLicenseRecords()->isPassed(5, 0) == 0)
            result.flags[1] = 1;
        break;
    }

    /* the drive train (jump table 0x00342630) */
    switch (rec->needDrivetrain) {
    case NEED_DRIVETRAIN_FF:
        if (car->getDriveTrain() != LAYOUT_FF)
            result.flags[3] = 1;
        break;
    case NEED_DRIVETRAIN_FR:
        if (car->getDriveTrain() != LAYOUT_FR)
            result.flags[3] = 1;
        break;
    case NEED_DRIVETRAIN_4WD:
        if (car->getDriveTrain() != LAYOUT_4WD)
            result.flags[3] = 1;
        break;
    case NEED_DRIVETRAIN_MR:
        if (car->getDriveTrain() != LAYOUT_MR)
            result.flags[3] = 1;
        break;
    case NEED_DRIVETRAIN_RR:
        if (car->getDriveTrain() != LAYOUT_RR)
            result.flags[3] = 1;
        break;
    }

    /* the cars the race allows ("-" = any; the list record is not
     * NULL-checked; the code of "-----" is recomputed per entry) */
    if (rec->carList != nameToCode("-")) {
        CarListRecord *list = (CarListRecord *)g_002EBD88.getRecord(DB_TABLE_CAR_LIST, rec->carList);
        u64 *entry = list->cars;
        int found = 0;
        int i;

        for (i = 0; i < 0x20; i++) {
            /* the entry is read after the call (retail's ld follows the jal)
             * and once for both tests */
            if (*entry == nameToCode("-----"))
                break;
            if (*entry++ == car->carCode) {
                found = 1;
                break;
            }
        }
        if (found == 0)
            result.flags[4] = 1;
    }

    /* the aspiration */
    switch (rec->needAspiration) {
    case NEED_ASPIRATION_NA:
        if (car->isTurbo() != 0)
            result.flags[5] = 1;
        break;
    case NEED_ASPIRATION_TURBO:
        if (car->isTurbo() == 0)
            result.flags[5] = 1;
        break;
    }

    /* the tuning: a racing car (carType != 0), a tuned one, or a normal one
     * (neither); retail's case bodies come in this order */
    switch (rec->needTuning) {
    case NEED_TUNING_TUNED:
        if (car->isTuned() == 0)
            result.flags[6] = 1;
        break;
    case NEED_TUNING_RACING:
        if (car->carType == 0)
            result.flags[6] = 1;
        break;
    case NEED_TUNING_NORMAL:
        if (car->carType != 0)
            result.flags[6] = 1;
        if (car->isTuned() != 0)
            result.flags[6] = 1;
        break;
    }

    /* the tyres: a dirt race needs the dirt tyres owned */
    switch (rec->needTire) {
    case NEED_TIRE_ANY:
        break;
    case NEED_TIRE_DIRT:
        if (car->hasPart(PART_TIRE_DIRT) == 0)
            result.flags[0] = 1;
        break;
    }

    return result;  /* copied into the caller's slot (vptr, 7 words), then ~result */
}

/* 1 if race `race` (by name) is a dirt-tyre race (needTire 9; u013 shows it
 * as "dirt_tire_only").  The record is not NULL-checked. */
// FUNC 0x0023A818 race_dirt_tire_only
int race_dirt_tire_only(const char *race)
{
    s64 code = nameToCode(race);

    return ((RaceRecord *)g_002EBD88.getRecord(DB_TABLE_RACE, code))->needTire == NEED_TIRE_DIRT;
}

/* Car record +0xF8 (not NULL-checked).  No callers. */
// FUNC 0x0023A858 F_0023A858
int F_0023A858(s64 car)
{
    return ((CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car))->unk_0F8;
}

/* Car record +0xFC (not NULL-checked).  No callers. */
// FUNC 0x0023A888 F_0023A888
int F_0023A888(s64 car)
{
    return ((CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car))->unk_0FC;
}

/* Splits a race code into its kind (RaceKind, the text of race_kind_text)
 * and an index:
 *   "A2.." 2P battle; "AT<dd>" time trial dd - 1; "ATA" free run; other
 *   "A.." arcade; "E.." endurance; "D.." dirt; "G..00d" single, other
 *   "G..<dd>" championship dd - 1 (the last two digits); "L.." licence
 *   tests (race_licence_split); "M4" / "M1" / "MM" machine tests.
 * kind RACE_KIND_NONE, index -1 otherwise.  Jump table 0x00342650 on
 * race[0] - 'A' (signed char). */
// FUNC 0x0023A8B8 race_name_split
void race_name_split(const char *race, int *kindOut, int *index)
{
    /* The kinds are stored as RaceKind: in retail the stores to *kind and to
     * *index are in different alias sets (sched1 moves the copy of `index'
     * ahead of `kind''s), so the original parameter was most likely a
     * RaceKind *.  The callers pass int locals, hence this view; it stores
     * the same values (an exact form, not the original signature). */
    RaceKind *kind = (RaceKind *)kindOut;

    *kind = RACE_KIND_NONE;
    *index = -1;
    switch ((s8)(race[0] - 'A')) {
    case 'A' - 'A':
        switch (race[1]) {
        case 'T':
            if (race[2] == 'A') {
                *kind = RACE_KIND_FREE_RUN;
            } else {
                *kind = RACE_KIND_TIME_TRIAL;
                *index = (race[2] - '0') * 10;
                *index += race[3] - '1';
            }
            break;
        case '2':
            *kind = RACE_KIND_2P_BATTLE;
            break;
        default:
            *kind = RACE_KIND_ARCADE;
            break;
        }
        break;
    case 'L' - 'A':
        race_licence_split(race, kindOut, index);
        break;
    case 'E' - 'A':
        *kind = RACE_KIND_GT_ENDURANCE;
        break;
    case 'D' - 'A':
        *kind = RACE_KIND_GT_DIRT;
        break;
    case 'M' - 'A':
        switch (race[1]) {
        case '1':
            *kind = RACE_KIND_MACHINE_1000M;
            break;
        case '4':
            *kind = RACE_KIND_MACHINE_400M;
            break;
        case 'M':
            *kind = RACE_KIND_MACHINE_VMAX;
            break;
        }
        break;
    case 'G' - 'A': {
        int len = strlen(race);

        if (race[len - 3] != '0' || race[len - 4] != race[len - 3]) {
            *kind = RACE_KIND_GT_CHAMPIONSHIP;
            *index = race[len - 1] - '0';
            *index = (race[len - 2] - '0') * 10 + *index - 1;
        } else {
            *kind = RACE_KIND_GT_SINGLE;
        }
        break;
    }
    }
}

/* A licence test code "L<I or 0><B, A, S or R>...<n>": *index = race[6] -
 * '1'; race[2] 'B' -> B (IB if race[1] == 'I'), 'A' -> A (IA), 'S' -> S,
 * 'R' -> R; *kind is left alone otherwise. */
// FUNC 0x0023AA70 race_licence_split
void race_licence_split(const char *race, int *kind, int *index)
{
    *index = race[6] - '1';
    switch (race[2]) {
    case 'A':
        if (race[1] == 'I')
            *kind = RACE_KIND_LICENSE_IA;
        else
            *kind = RACE_KIND_LICENSE_A;
        break;
    case 'B':
        if (race[1] == 'I')
            *kind = RACE_KIND_LICENSE_IB;
        else
            *kind = RACE_KIND_LICENSE_B;
        break;
    case 'S':
        *kind = RACE_KIND_LICENSE_S;
        break;
    case 'R':
        *kind = RACE_KIND_LICENSE_R;
        break;
    }
}

/* The DATABASERACECATEGORY text of a race kind (jump table 0x00342690); the
 * four GT kinds share "Championship"; NULL for RACE_KIND_NONE and others. */
// FUNC 0x0023AB18 race_kind_text
const char *race_kind_text(int kind)
{
    const char *text = 0;

    switch (kind) {
    case RACE_KIND_ARCADE:
        text = g_002F12A0[0];
        break;
    case RACE_KIND_2P_BATTLE:
        text = g_002F12A0[1];
        break;
    case RACE_KIND_TIME_TRIAL:
        text = g_002F12A0[2];
        break;
    case RACE_KIND_FREE_RUN:
        text = g_002F12A0[3];
        break;
    case RACE_KIND_GT_SINGLE:
    case RACE_KIND_GT_CHAMPIONSHIP:
    case RACE_KIND_GT_ENDURANCE:
    case RACE_KIND_GT_DIRT:
        text = g_002F12A0[4];
        break;
    case RACE_KIND_LICENSE_B:
        text = g_002F12A0[5];
        break;
    case RACE_KIND_LICENSE_A:
        text = g_002F12A0[6];
        break;
    case RACE_KIND_LICENSE_IB:
        text = g_002F12A0[7];
        break;
    case RACE_KIND_LICENSE_IA:
        text = g_002F12A0[8];
        break;
    case RACE_KIND_LICENSE_S:
        text = g_002F12A0[9];
        break;
    case RACE_KIND_LICENSE_R:
        text = g_002F12A0[10];
        break;
    case RACE_KIND_MACHINE_400M:
        text = g_002F12A0[11];
        break;
    case RACE_KIND_MACHINE_1000M:
        text = g_002F12A0[12];
        break;
    case RACE_KIND_MACHINE_VMAX:
        text = g_002F12A0[13];
        break;
    }
    return text;
}

/* 1 if the car is unknown or locked out of this region (CarRecord::
 * regionLock); `code` may be a preset car (0x23) or an entry car (0x1F). */
// FUNC 0x0023ABF0 db_car_is_locked
int db_car_is_locked(s64 code)
{
    PresetCarRecord *preset = (PresetCarRecord *)g_002EBD88.getRecord(DB_TABLE_PRESET_CAR, code);
    CarRecord *rec;
    s64 car;

    if (preset != 0)
        car = preset->car;
    else if ((preset = (PresetCarRecord *)g_002EBD88.getRecord(DB_TABLE_ENTRY_CAR, code)) != 0)
        car = preset->car;
    else
        car = code;
    rec = (CarRecord *)g_002EBD88.getRecord(DB_TABLE_CAR, car);
    if (rec == 0)
        return 1;
    return region_lock_check(rec->regionLock);
}

/* 1 if a car with region lock `lock` is not available in this country (jump
 * table 0x003426E0); REGION_LOCK_NONE and unknown values: available. */
// FUNC 0x0023AC80 region_lock_check
int region_lock_check(int lock)
{
    int locked = 0;

    switch (lock) {
    case REGION_LOCK_NONE:
        locked = 0;
        break;
    case REGION_LOCK_ONLY_BR:
        locked = g_002EBD88.isUK() == 0;
        break;
    case REGION_LOCK_NOT_BR:
        locked = g_002EBD88.isUK() != 0;
        break;
    case REGION_LOCK_ONLY_US:
        locked = g_002EBD88.isUS() == 0;
        break;
    case REGION_LOCK_NOT_US:
        locked = g_002EBD88.isUS() != 0;
        break;
    case REGION_LOCK_NOT_JP_US:
        locked = g_002EBD88.isEurope() == 0;
        break;
    case REGION_LOCK_ONLY_JP_US:
        locked = g_002EBD88.isEurope() != 0;
        break;
    case REGION_LOCK_ONLY_JP:
        locked = g_002EBD88.isJapan() == 0;
        break;
    case REGION_LOCK_NOT_JP:
        locked = g_002EBD88.isJapan() != 0;
        break;
    }
    return locked;
}

/* The short names of the cars (db_car_short_names), NULL until built.  No
 * initialiser: egcs writes it as zeros at the end of T489's .sdata, after
 * check_regulation's "-" / "-----" (retail 0x00352F00). */
const char **g_00352F00;                        /* 0x00352F00 */

/* The short names (CarRecord::shortName) of all cars, built once into
 * g_00352F00 (operator new[] of the count); returns the number of cars.  No
 * callers. */
// FUNC 0x0023AD68 db_car_short_names
int db_car_short_names(const char ***out)
{
    u16 count = g_002EBD88.getDataBlock(DB_TABLE_CAR)->numElements;

    if (g_00352F00 == 0) {
        const char **names = new const char *[count];   /* implicit: operator new[] */
        int i;

        /* 99.0%: retail computes the record into $v0 (tied to the block
         * pointer), ours into $v1 (tied to the elementSize load).  Tried:
         * getElement (both inlines), i * size, a size / offset / start
         * local, &elements[], (u8 *)table + 0x10, rec inline in the call,
         * names[i] / g_00352F00[i] (worse), do-while, and 991111-01. */
        g_00352F00 = names;
        for (i = 0; i < count; i++, names++) {
            DataBlock *table = g_002EBD88.getDataBlock(DB_TABLE_CAR);
            CarRecord *rec = (CarRecord *)(table->elements + table->elementSize * i);

            *names = g_002EBD88.strings->getString(rec->shortName);
        }
    }
    *out = g_00352F00;
    return count;
}

// FUNC 0x0023AE30 RegurationResult::~RegurationResult
/* implicit: operator delete; inline in game/menu/gtmode/T045_gtmode_page_checks.h; egcs emits this copy (the vptr
 * +0x1C = 0x00310310, then operator delete if flags & 1; unreferenced: the
 * vtables use T045's 0x0011C958) */

#ifndef BOOT_OLD_GCC
/* T489's copy of the typeinfo function (unreferenced; T045's is 0x0011C988).
 * Weak: T045's definition wins at link time. */
// FUNC 0x0023AE60 __tf16RegurationResult
extern "C" __attribute__((weak)) void *__tf16RegurationResult(void)
{
    if (__ti16RegurationResult[0] == 0)
        __rtti_user(__ti16RegurationResult, "16RegurationResult");
    return __ti16RegurationResult;
}
#endif
