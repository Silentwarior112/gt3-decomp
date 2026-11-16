/* T417 (0x00213210..0x00213E90): the arcade time trial records of the save
 * data (GameStatus+0x246F8): 10 courses of 12 places.
 *
 * Data: .data the preset names g_002E1080 and the course codes g_002E10F8
 * (0x002E1080..0x002E1120), then the frame info.  .rodata
 * 0x0033F9B8..0x0033FC11: the preset names longer than 8 bytes (only the
 * table points at them, so DATA_START), the empty ranking code (clear), the
 * course codes; no type names (the file saw no polymorphic class).  .sdata
 * 0x00352A10..0x00352A30: the preset names of 8 bytes or less ("T.Izumi",
 * "M.Goto", "S.Ikiuo", "Y.Aoki"; -G8, ref/compile_flags.json; unreferenced
 * by code, so DATA_START).  The course table is defined after clear(): its
 * strings come after clear's dashes. */
// DATA_START 0x0033F9B8
// DATA_START 0x00352A10
#include "game/save/prize_records_internal.h"
#include "game/save/T417_time_trial_records.h"

#define TIME_TRIAL_PLACES 12                    /* places of a course */
#define TIME_TRIAL_COURSES 10                   /* courses AT010001 .. AT100001 */
#define TIME_TRIAL_PRESETS 3                    /* preset records per course */

/* The size of a course's saved block before its BestPlace (times .. codes). */
#define TIME_TRIAL_COURSE_HEAD 0x2A0

/* The 30 preset record names, 3 per course (TimeTrialRecords::reset gives
 * the records the names ~0, ~1, ... in course order). */
const char *g_002E1080[30] = {                  /* 0x002E1080 */
    "T.Izumi", "N.Ishibashi", "S.Ebihara",
    "J.Kobayashi", "Y.Matsumoto", "T.Yokouchi",
    "M.Hasegawa", "K.Miyata", "S.Sawamura",
    "T.Murase", "T.Ikegami", "M.Suzaki",
    "M.Goto", "S.Ikiuo", "T.Tejima",
    "Y.Yasuhara", "M.Kitagawa", "Y.Aoki",
    "Y.Kurata", "M.Fujita", "S.Takano",
    "K.Takayama", "D.Takeuchi", "M.Sakakibara",
    "T.Kagaya", "N.Takama", "Y.Takahashi",
    "K.Yamauchi", "S.Takasugi", "K.Minemura",
};

// FUNC 0x00213210 time_trial_preset_name
const char *time_trial_preset_name(int i)
{
    return g_002E1080[i];
}

// FUNC 0x00213228 TimeTrialCourse::clear
void TimeTrialCourse::clear(int i)
{
    times[i] = RACE_TIME_INVALID;
    names[i][0] = 0;
    dates[i] = 0;
    strcpy(codes[i].s, RANKING_CODE_EMPTY);     /* expanded to a 31-byte copy */
}

/* The race codes of the 10 time trial courses (reset's race data). */
const char *g_002E10F8[10] = {                  /* 0x002E10F8 */
    "AT010001", "AT020001", "AT030001", "AT040001", "AT050001",
    "AT060001", "AT070001", "AT080001", "AT090001", "AT100001",
};

// FUNC 0x002132C0 TimeTrialCourse::clearAll
void TimeTrialCourse::clearAll()
{
    for (int i = 0; i < TIME_TRIAL_PLACES; i++)
        clear(i);
}

// FUNC 0x00213310 TimeTrialCourse::findPlace
int TimeTrialCourse::findPlace(u32 time)
{
    int place;
    for (place = 0; place < TIME_TRIAL_PLACES; place++) {
        if (times[place] == RACE_TIME_INVALID || !(times[place] < time))
            break;
    }
    return (place <= TIME_TRIAL_PLACES - 1) ? place : -1;
}

// FUNC 0x00213370 TimeTrialCourse::enterTime
int TimeTrialCourse::enterTime(u32 time)
{
    int place = findPlace(time);
    if (place >= 0) {
        for (int i = TIME_TRIAL_PLACES - 1; place < i; i--)
            times[i] = times[i - 1];
        times[place] = time;
        best.update(place);
        return place;
    }
    return -1;
}

// FUNC 0x00213420 TimeTrialCourse::getTime
u32 TimeTrialCourse::getTime(int i)
{
    return times[i];
}

/* findPlace inline, and no best place update (reset's preset records). */
// FUNC 0x00213430 TimeTrialCourse::insertTime
int TimeTrialCourse::insertTime(u32 time)
{
    int place;
    for (place = 0; place < TIME_TRIAL_PLACES; place++) {
        if (times[place] == RACE_TIME_INVALID || !(times[place] < time))
            break;
    }
    if (place < TIME_TRIAL_PLACES) {
        for (int i = TIME_TRIAL_PLACES - 1; place < i; i--)
            times[i] = times[i - 1];
        times[place] = time;
        return place;
    }
    return -1;
}

// FUNC 0x002134F8 TimeTrialCourse::insertName
void TimeTrialCourse::insertName(const char *name, int place)
{
    for (int i = TIME_TRIAL_PLACES - 1; place < i; i--)
        strcpy(names[i], names[i - 1]);
    int len = strlen(name);
    /* (sic) len bytes: strcpy writes len + 1 (in retail the extra byte
     * lands in the 16-byte rounding or the frame's outgoing-argument
     * area; the GCC 14 build allocates it).  alloca, not char[len]: a
     * VLA's size goes through bits (len * 8 / 8: sll 3; srl 3), retail
     * rounds len itself */
#ifdef BOOT_OLD_GCC
    char *shortName = (char *)__builtin_alloca(len);
#else
    char *shortName = (char *)__builtin_alloca(len + 1);
#endif
    strcpy(shortName, name);
    if (len >= 0x10)
        shortName[0xF] = 0;
    strcpy(names[place], shortName);
    insertDate(place);
}

/* A preset record's name byte is ~(preset index), negative. */
// FUNC 0x002135E8 TimeTrialCourse::getName
const char *TimeTrialCourse::getName(int i)
{
    char *name = names[i];
    if ((s8)name[0] < 0)
        return time_trial_preset_name(~(s8)name[0]);
    return name;
}

// FUNC 0x00213620 TimeTrialCourse::insertCode
void TimeTrialCourse::insertCode(const RankingCode *code, int place)
{
    for (int i = TIME_TRIAL_PLACES - 1; place < i; i--)
        codes[i] = codes[i - 1];
    codes[place] = *code;
}

// FUNC 0x002136E0 TimeTrialCourse::getCode
RankingCode *TimeTrialCourse::getCode(int i)
{
    return &codes[i];
}

// FUNC 0x002136F0 TimeTrialCourse::insertDate
void TimeTrialCourse::insertDate(int place)
{
    for (int i = TIME_TRIAL_PLACES - 1; place < i; i--)
        dates[i] = dates[i - 1];
    dates[place] = clock_seconds();             /* 0x0022BB08 (F_0022BB08) */
}

// FUNC 0x00213778 TimeTrialCourse::getDate
u32 TimeTrialCourse::getDate(int i)
{
    if ((s8)names[i][0] < 0)
        return TIME_TRIAL_PRESET_DATE;          /* a preset record */
    return dates[i];
}

// FUNC 0x002137A8 TimeTrialCourse::remove
void TimeTrialCourse::remove(int i)
{
    for (; i < TIME_TRIAL_PLACES - 1; i++) {
        times[i] = times[i + 1];
        strcpy(names[i], names[i + 1]);
        dates[i] = dates[i + 1];
        codes[i] = codes[i + 1];
    }
    clear(TIME_TRIAL_PLACES - 1);
}

// FUNC 0x002138A8 TimeTrialCourse::isPlayerRecord
int TimeTrialCourse::isPlayerRecord(int i)
{
    /* an if: retail's slt against -1 in a register; the store-flag
     * `name[0] >= 0' becomes nor / srl 31 */
    if ((s8)names[i][0] < 0)
        return 0;
    return 1;
}

// FUNC 0x002138C0 TimeTrialCourse::getSize
int TimeTrialCourse::getSize()
{
    return BestPlace::getSize() + TIME_TRIAL_COURSE_HEAD;
}

// FUNC 0x002138E0 TimeTrialCourse::load
const u8 *TimeTrialCourse::load(const u8 *src, int arg)
{
    u32 size = TIME_TRIAL_COURSE_HEAD;          /* a variable: retail calls memcpy */

    memcpy((void *)this, src, size);
    return best.load(src + size, arg);
}

// FUNC 0x00213930 TimeTrialCourse::save
u8 *TimeTrialCourse::save(u8 *dst)
{
    u32 size = TIME_TRIAL_COURSE_HEAD;

    memcpy(dst, this, size);
    return best.save(dst + size);
}

/* The array of courses is constructed inline (BestPlace(): place 8 each). */
// FUNC 0x00213978 TimeTrialRecords::TimeTrialRecords
TimeTrialRecords::TimeTrialRecords()
{
    reset();
}

/* A preset time: the race data's gold / silver / bronze time, in ms.  A
 * macro: retail reads each field off db.event (an inline function's
 * pointer argument would be a register of its own). */
#define RACE_TIME_MS(t) ((t).min * 60000 + (t).sec * 1000 + (t).ms)

/* Empties every course, then enters the three preset records of each: the
 * times from the course's race data (RaceEvent::time[3]), the names presets
 * 0, 1, 2, ... (a one-byte name ~index: "\xFF" decremented after each record,
 * never reset between courses).  The preset records get no best place. */
// FUNC 0x002139D0 TimeTrialRecords::reset
void TimeTrialRecords::reset()
{
    for (int i = 0; i < TIME_TRIAL_COURSES; i++)
        courses[i].clearAll();

    char presetName[2];                         /* sp+0x00 */
    presetName[1] = 0;
    presetName[0] = -1;                         /* ~0: preset name 0 */
    for (int c = 0; c < TIME_TRIAL_COURSES; c++) {
        RaceDataBase db;                        /* sp+0x10 */
        db.select(nameToCode(g_002E10F8[c]));
        TimeTrialCourse *course = &courses[c];
        int place;

        /* three times the same code (unrolled) */
        place = course->insertTime(RACE_TIME_MS(db.event->time[0]));
        course->insertName(presetName, place);
        presetName[0] = presetName[0] - 1;

        place = course->insertTime(RACE_TIME_MS(db.event->time[1]));
        course->insertName(presetName, place);
        presetName[0] = presetName[0] - 1;

        place = course->insertTime(RACE_TIME_MS(db.event->time[2]));
        course->insertName(presetName, place);
        presetName[0] = presetName[0] - 1;
    }
}

// FUNC 0x00213BA8 TimeTrialRecords::findPlace
int TimeTrialRecords::findPlace(int c, u32 time)
{
    return courses[c].findPlace(time);
}

// FUNC 0x00213BD0 TimeTrialRecords::enterTime
int TimeTrialRecords::enterTime(int c, u32 time)
{
    return courses[c].enterTime(time);
}

// FUNC 0x00213BF8 TimeTrialRecords::getTime
u32 TimeTrialRecords::getTime(int c, int i)
{
    return courses[c].getTime(i);
}

// FUNC 0x00213C20 TimeTrialRecords::insertName
void TimeTrialRecords::insertName(int c, const char *name, int place)
{
    courses[c].insertName(name, place);
}

// FUNC 0x00213C50 TimeTrialRecords::getName
const char *TimeTrialRecords::getName(int c, int i)
{
    return courses[c].getName(i);
}

// FUNC 0x00213C78 TimeTrialRecords::insertCode
void TimeTrialRecords::insertCode(int c, const RankingCode *code, int place)
{
    courses[c].insertCode(code, place);
}

// FUNC 0x00213CA8 TimeTrialRecords::getCode
RankingCode *TimeTrialRecords::getCode(int c, int i)
{
    return courses[c].getCode(i);
}

// FUNC 0x00213CD0 TimeTrialRecords::getDate
u32 TimeTrialRecords::getDate(int c, int i)
{
    return courses[c].getDate(i);
}

// FUNC 0x00213CF8 TimeTrialRecords::remove
void TimeTrialRecords::remove(int c, int i)
{
    courses[c].remove(i);
}

// FUNC 0x00213D20 TimeTrialRecords::isPlayerRecord
int TimeTrialRecords::isPlayerRecord(int c, int i)
{
    return courses[c].isPlayerRecord(i);
}

// FUNC 0x00213D48 TimeTrialRecords::countFirstPlaces
int TimeTrialRecords::countFirstPlaces()
{
    int count = 0;
    for (int c = 0; c < TIME_TRIAL_COURSES; c++) {
        if (courses[c].getBestPlace() == 0)
            count++;
    }
    return count;
}

// FUNC 0x00213D80 TimeTrialRecords::numCourses
int TimeTrialRecords::numCourses()
{
    return TIME_TRIAL_COURSES;
}

// FUNC 0x00213D88 TimeTrialRecords::allFirstPlaces
int TimeTrialRecords::allFirstPlaces()
{
    int c;
    for (c = 0; c < TIME_TRIAL_COURSES; c++) {
        if (courses[c].getBestPlace() != 0)
            break;
    }
    /* an if, not `return c >= 10': retail compares against the 9 in a
     * register (slt), the store-flag form folds to slti / xori */
    if (c < TIME_TRIAL_COURSES)
        return 0;
    return 1;
}

// FUNC 0x00213DC0 TimeTrialRecords::getSize
int TimeTrialRecords::getSize()
{
    return TimeTrialCourse::getSize() * TIME_TRIAL_COURSES;
}

// FUNC 0x00213DE8 TimeTrialRecords::load
const u8 *TimeTrialRecords::load(const u8 *src, int arg)
{
    for (int c = 0; c < TIME_TRIAL_COURSES; c++)
        src = courses[c].load(src, arg);
    return src;
}

// FUNC 0x00213E48 TimeTrialRecords::save
u8 *TimeTrialRecords::save(u8 *dst)
{
    for (int c = 0; c < TIME_TRIAL_COURSES; c++)
        dst = courses[c].save(dst);
    return dst;
}
