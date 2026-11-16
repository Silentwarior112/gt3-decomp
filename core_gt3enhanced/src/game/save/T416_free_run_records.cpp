/* T416_free_run_records.cpp - FreeRunRecords (0x002127D0..0x00213210).
 *
 * GameStatus+0x26160: 44 ranking tables of 12 places, one per course of the
 * product database's "TIME_ATTACK" list (table(course)).  The table code is
 * T416's own copy of T415's (times only, no speed functions; deleted places
 * are always cleared as empty times).
 *
 * Retail T416's .rodata is its two literals, the empty code's dashes and
 * "TIME_ATTACK" (0x0033F988..0x0033F9B3): no type names.  The staff names
 * after them (0x0033F9B8..) are T417's (u096: its preset-name table's).
 */
#include "game/race/core/race_input_internal.h"
#include "game/save/T416_free_run_records.h"         /* FreeRunRecords, FreeRunTable */
#include "game/data/T435_product_config.h"           /* ProductConfig, product_config */
#include "game/sound/T465_clock.h"                    /* clock_seconds */

// FUNC 0x002127D0 FreeRunTable::clearEntry
void FreeRunTable::clearEntry(int i)
{
    time[i] = RACE_TIME_INVALID;
    name[i][0] = '\0';
    car[i] = 0;
    date[i] = 0;
    strcpy(code[i].s, RANKING_CODE_EMPTY);
}

// FUNC 0x00212878 FreeRunTable::clear
void FreeRunTable::clear()
{
    int i;

    for (i = 0; i < RANKING_PLACES; i++)
        clearEntry(i);
}

// FUNC 0x002128C8 FreeRunTable::findTime
//   The place a new time would take, -1 if none.
int FreeRunTable::findTime(u32 t)
{
    int place;

    for (place = 0; place < RANKING_PLACES; place++) {
        if (time[place] == RACE_TIME_INVALID || !(time[place] < t))
            break;
    }
    return (place > RANKING_PLACES - 1) ? -1 : place;
}

// FUNC 0x00212928 FreeRunTable::insertTime
int FreeRunTable::insertTime(u32 t)
{
    int place = findTime(t);
    int j;

    if (place >= 0) {
        for (j = RANKING_PLACES - 1; j > place; j--)
            time[j] = time[j - 1];
        time[place] = t;
        return place;
    }
    return -1;
}

// FUNC 0x002129C0 FreeRunTable::getTime
u32 FreeRunTable::getTime(int i)
{
    return time[i];
}

// FUNC 0x002129D0 FreeRunTable::setName
//   As MachineTestTable::setName (the same strlen-sized alloca, one byte
//   short of the terminator).
void FreeRunTable::setName(const char *newName, int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        strcpy(name[j], name[j - 1]);
    int len = strlen(newName);
#ifdef BOOT_OLD_GCC
    char *buf = (char *)__builtin_alloca(len);  /* retail: one byte short */
#else
    char *buf = (char *)__builtin_alloca(len + 1);  /* GCC 14: room for the terminator */
#endif

    strcpy(buf, newName);
    if (len >= 0x10)
        buf[15] = '\0';
    strcpy(name[i], buf);
    insertDate(i);
}

// FUNC 0x00212AC0 FreeRunTable::getName
char *FreeRunTable::getName(int i)
{
    return name[i];
}

// FUNC 0x00212AD0 FreeRunTable::setCar
void FreeRunTable::setCar(u64 newCar, int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        car[j] = car[j - 1];
    car[i] = newCar;
}

// FUNC 0x00212B30 FreeRunTable::getCar
u64 FreeRunTable::getCar(int i)
{
    return car[i];
}

// FUNC 0x00212B40 FreeRunTable::setCode
void FreeRunTable::setCode(const char *newCode, int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        code[j] = code[j - 1];
    code[i] = *(const RankingCode *)newCode;
}

// FUNC 0x00212C00 FreeRunTable::getCode
char *FreeRunTable::getCode(int i)
{
    return code[i].s;
}

// FUNC 0x00212C10 FreeRunTable::insertDate
void FreeRunTable::insertDate(int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        date[j] = date[j - 1];
    date[i] = clock_seconds();                  /* 0x0022BB08 (T465) */
}

// FUNC 0x00212C98 FreeRunTable::getDate
u32 FreeRunTable::getDate(int i)
{
    return date[i];
}

// FUNC 0x00212CA8 FreeRunTable::remove
void FreeRunTable::remove(int i)
{
    int j;

    for (j = i; j < RANKING_PLACES - 1; j++) {
        time[j] = time[j + 1];
        strcpy(name[j], name[j + 1]);
        car[j] = car[j + 1];
        date[j] = date[j + 1];
        code[j] = code[j + 1];
    }
    clearEntry(RANKING_PLACES - 1);
}

// FUNC 0x00212DD0 FreeRunTable::size
int FreeRunTable::size()
{
    return 0x300;                               /* sizeof(FreeRunTable) */
}

// FUNC 0x00212DD8 FreeRunTable::load
//   `version' is not used (? name; the caller passes 0).
const u8 *FreeRunTable::load(const u8 *src, int version)
{
    u32 n = 0x300;                              /* sizeof(FreeRunTable); in a variable, egcs calls memcpy */

    memcpy(this, src, n);
    return src + n;
}

// FUNC 0x00212E08 FreeRunTable::save
u8 *FreeRunTable::save(u8 *dst)
{
    u32 n = 0x300;                              /* sizeof(FreeRunTable) */

    memcpy(dst, this, n);
    return dst + n;
}

// FUNC 0x00212E40 FreeRunRecords::FreeRunRecords
FreeRunRecords::FreeRunRecords()
{
    init();
}

// FUNC 0x00212E68 FreeRunRecords::init
void FreeRunRecords::init()
{
    int i;

    for (i = 0; i < FREE_RUN_COURSES; i++)
        tables[i].clear();
}

// FUNC 0x00212EB0 FreeRunRecords::table
//   The course's index in the TIME_ATTACK list (not range-checked: an
//   unknown course, -1, gives the table before tables[0]).
FreeRunTable *FreeRunRecords::table(const char *course)
{
    return &tables[product_config()->courseIndex(course, "TIME_ATTACK")];
}

// FUNC 0x00212F00 FreeRunRecords::findTime
int FreeRunRecords::findTime(const char *course, u32 time)
{
    return table(course)->findTime(time);
}

// FUNC 0x00212F30 FreeRunRecords::insertTime
int FreeRunRecords::insertTime(const char *course, u32 time)
{
    return table(course)->insertTime(time);
}

// FUNC 0x00212F60 FreeRunRecords::getTime
u32 FreeRunRecords::getTime(const char *course, int i)
{
    return table(course)->getTime(i);
}

// FUNC 0x00212F90 FreeRunRecords::setName
void FreeRunRecords::setName(const char *course, const char *name, int i)
{
    table(course)->setName(name, i);
}

// FUNC 0x00212FD0 FreeRunRecords::getName
char *FreeRunRecords::getName(const char *course, int i)
{
    return table(course)->getName(i);
}

// FUNC 0x00213000 FreeRunRecords::setCar
void FreeRunRecords::setCar(const char *course, u64 car, int i)
{
    table(course)->setCar(car, i);
}

// FUNC 0x00213040 FreeRunRecords::getCar
u64 FreeRunRecords::getCar(const char *course, int i)
{
    return table(course)->getCar(i);
}

// FUNC 0x00213070 FreeRunRecords::setCode
void FreeRunRecords::setCode(const char *course, const char *code, int i)
{
    table(course)->setCode(code, i);
}

// FUNC 0x002130B0 FreeRunRecords::getCode
char *FreeRunRecords::getCode(const char *course, int i)
{
    return table(course)->getCode(i);
}

// FUNC 0x002130E0 FreeRunRecords::getDate
u32 FreeRunRecords::getDate(const char *course, int i)
{
    return table(course)->getDate(i);
}

// FUNC 0x00213110 FreeRunRecords::remove
void FreeRunRecords::remove(const char *course, int i)
{
    table(course)->remove(i);
}

// FUNC 0x00213140 FreeRunRecords::size
int FreeRunRecords::size()
{
    return FreeRunTable::size() * FREE_RUN_COURSES;
}

// FUNC 0x00213168 FreeRunRecords::load
//   `arg' is not used (GameStatus passes 0); each table gets 0.
const u8 *FreeRunRecords::load(const u8 *src, int arg)
{
    int i;

    for (i = 0; i < FREE_RUN_COURSES; i++)
        src = tables[i].load(src, 0);
    return src;
}

// FUNC 0x002131C0 FreeRunRecords::save
u8 *FreeRunRecords::save(u8 *dst)
{
    int i;

    for (i = 0; i < FREE_RUN_COURSES; i++)
        dst = tables[i].save(dst);
    return dst;
}
