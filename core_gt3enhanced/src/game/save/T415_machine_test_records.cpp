/* T415_machine_test_records.cpp - MachineTestRecords (0x00211B58..0x002127D0).
 *
 * GameStatus+0x2E560: three ranking tables of 12 places (kind 0: 0-400 m
 * times, 1: 0-1000 m times, ascending, empty places RACE_TIME_INVALID; 2:
 * max speeds * 100000, descending, empty places 0).  Each place has a time,
 * a name (15 characters), a car (u64), the date it was set and a 32-byte
 * ranking code.  The set* functions insert at a place (the places after it
 * move down); the time / speed is inserted first by insertTime /
 * insertSpeed, which returns the place.
 *
 * Retail T415's .rodata is the empty code's dashes alone (0x0033F968): no
 * type names, so no header of classes with inline virtuals.
 */
#include "game/race/core/race_input_internal.h"
#include "game/save/T415_machine_test_records.h"     /* MachineTestRecords, MachineTestTable, RankingCode */
#include "game/sound/T465_clock.h"                    /* clock_seconds */

// FUNC 0x00211B58 MachineTestTable::clearEntry
//   The strcpy of the empty code is inlined (31-byte unaligned copy).
void MachineTestTable::clearEntry(int i)
{
    time[i] = RACE_TIME_INVALID;
    name[i][0] = '\0';
    car[i] = 0;
    date[i] = 0;
    strcpy(code[i].s, RANKING_CODE_EMPTY);
}

// FUNC 0x00211C00 MachineTestTable::clear
void MachineTestTable::clear()
{
    int i;

    for (i = 0; i < RANKING_PLACES; i++)
        clearEntry(i);
}

// FUNC 0x00211C50 MachineTestTable::clearEntryZero
//   The speed table's empty place (time 0, the code is left alone).
void MachineTestTable::clearEntryZero(int i)
{
    time[i] = 0;
    name[i][0] = '\0';
    car[i] = 0;
    date[i] = 0;
}

// FUNC 0x00211C80 MachineTestTable::clearZero
void MachineTestTable::clearZero()
{
    int i;

    for (i = 0; i < RANKING_PLACES; i++)
        clearEntryZero(i);
}

// FUNC 0x00211CD0 MachineTestTable::findTime
//   The place a new time would take (before the first empty or not faster
//   place; unsigned compare), -1 if none.
int MachineTestTable::findTime(u32 t)
{
    int place;

    for (place = 0; place < RANKING_PLACES; place++) {
        if (time[place] == RACE_TIME_INVALID || !(time[place] < t))
            break;
    }
    return (place > RANKING_PLACES - 1) ? -1 : place;
}

// FUNC 0x00211D30 MachineTestTable::insertTime
int MachineTestTable::insertTime(u32 t)
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

// FUNC 0x00211DC8 MachineTestTable::getTime
u32 MachineTestTable::getTime(int i)
{
    return time[i];
}

// FUNC 0x00211DD8 MachineTestTable::findSpeed
//   The place a new speed would take (before the first place whose speed,
//   time * 1e-5 from the unsigned word, is not above it), -1 if none.
int MachineTestTable::findSpeed(f32 speed)
{
    int place;

    for (place = 0; place < RANKING_PLACES; place++) {
        if ((f32)time[place] * 0.00001f <= speed)
            break;
    }
    return (place > RANKING_PLACES - 1) ? -1 : place;
}

// FUNC 0x00211E50 MachineTestTable::insertSpeed
int MachineTestTable::insertSpeed(f32 speed)
{
    int place = findSpeed(speed);
    int j;

    if (place >= 0) {
        for (j = RANKING_PLACES - 1; j > place; j--)
            time[j] = time[j - 1];
        time[place] = (int)(speed * 100000.0f); /* cvt.w.s */
        return place;
    }
    return -1;
}

// FUNC 0x00211EF0 MachineTestTable::getSpeed
f32 MachineTestTable::getSpeed(int i)
{
    return (f32)time[i] * 0.00001f;
}

// FUNC 0x00211F38 MachineTestTable::setName
//   Inserts name (cut to 15 characters) at place i, then the date.  The
//   copy buffer is an alloca of strlen(name) bytes - one short of the
//   terminator (the stack rounding to 16 bytes covers it unless the length
//   is a multiple of 16; then it lands at the frame's unused bottom, fp+0,
//   below the saved registers at +0x20).  The GCC 14 build allocates one
//   more byte (its frame differs).  (A VLA gives (len * 8) / 8 and a
//   block-scoped stack restore; retail has neither.)
void MachineTestTable::setName(const char *newName, int i)
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

// FUNC 0x00212028 MachineTestTable::getName
char *MachineTestTable::getName(int i)
{
    return name[i];
}

// FUNC 0x00212038 MachineTestTable::setCar
void MachineTestTable::setCar(u64 newCar, int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        car[j] = car[j - 1];
    car[i] = newCar;
}

// FUNC 0x00212098 MachineTestTable::getCar
u64 MachineTestTable::getCar(int i)
{
    return car[i];
}

// FUNC 0x002120A8 MachineTestTable::setCode
//   32-byte RankingCode copies (unaligned).
void MachineTestTable::setCode(const char *newCode, int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        code[j] = code[j - 1];
    code[i] = *(const RankingCode *)newCode;
}

// FUNC 0x00212168 MachineTestTable::getCode
char *MachineTestTable::getCode(int i)
{
    return code[i].s;
}

// FUNC 0x00212178 MachineTestTable::insertDate
//   Inserts the current date (clock_seconds) at place i (from setName).
void MachineTestTable::insertDate(int i)
{
    int j;

    for (j = RANKING_PLACES - 1; j > i; j--)
        date[j] = date[j - 1];
    date[i] = clock_seconds();                  /* 0x0022BB08 (T465) */
}

// FUNC 0x00212200 MachineTestTable::getDate
u32 MachineTestTable::getDate(int i)
{
    return date[i];
}

// FUNC 0x00212210 MachineTestTable::remove
//   Deletes place i (the places after it move up); the last place is
//   cleared as a speed table's (zero) or a time table's.
void MachineTestTable::remove(int i, int zero)
{
    int j;

    for (j = i; j < RANKING_PLACES - 1; j++) {
        time[j] = time[j + 1];
        strcpy(name[j], name[j + 1]);
        car[j] = car[j + 1];
        date[j] = date[j + 1];
        code[j] = code[j + 1];
    }
    if (zero)
        clearEntryZero(RANKING_PLACES - 1);
    else
        clearEntry(RANKING_PLACES - 1);
}

// FUNC 0x00212358 MachineTestTable::size
int MachineTestTable::size()
{
    return 0x300;                               /* sizeof(MachineTestTable) */
}

// FUNC 0x00212360 MachineTestTable::load
//   `version' is not used (? name; the caller passes 0).
const u8 *MachineTestTable::load(const u8 *src, int version)
{
    u32 n = 0x300;                              /* sizeof(MachineTestTable); in a variable, egcs calls memcpy */

    memcpy(this, src, n);
    return src + n;
}

// FUNC 0x00212390 MachineTestTable::save
u8 *MachineTestTable::save(u8 *dst)
{
    u32 n = 0x300;                              /* sizeof(MachineTestTable) */

    memcpy(dst, this, n);
    return dst + n;
}

// FUNC 0x002123C8 MachineTestRecords::MachineTestRecords
MachineTestRecords::MachineTestRecords()
{
    init();
}

// FUNC 0x002123F0 MachineTestRecords::init
void MachineTestRecords::init()
{
    tables[0].clear();
    tables[1].clear();
    tables[MACHINE_TEST_MAX_SPEED].clearZero();
}

// FUNC 0x00212428 MachineTestRecords::table
MachineTestTable *MachineTestRecords::table(int kind)
{
    return &tables[kind];
}

// FUNC 0x00212438 MachineTestRecords::findTime
int MachineTestRecords::findTime(int kind, u32 time)
{
    return table(kind)->findTime(time);
}

// FUNC 0x00212468 MachineTestRecords::insertTime
int MachineTestRecords::insertTime(int kind, u32 time)
{
    return table(kind)->insertTime(time);
}

// FUNC 0x00212498 MachineTestRecords::getTime
u32 MachineTestRecords::getTime(int kind, int i)
{
    return table(kind)->getTime(i);
}

// FUNC 0x002124C8 MachineTestRecords::findSpeed
int MachineTestRecords::findSpeed(int kind, f32 speed)
{
    return table(kind)->findSpeed(speed);
}

// FUNC 0x002124F8 MachineTestRecords::insertSpeed
int MachineTestRecords::insertSpeed(int kind, f32 speed)
{
    return table(kind)->insertSpeed(speed);
}

// FUNC 0x00212528 MachineTestRecords::getSpeed
f32 MachineTestRecords::getSpeed(int kind, int i)
{
    return table(kind)->getSpeed(i);
}

// FUNC 0x00212558 MachineTestRecords::setName
void MachineTestRecords::setName(int kind, const char *name, int i)
{
    table(kind)->setName(name, i);
}

// FUNC 0x00212598 MachineTestRecords::getName
char *MachineTestRecords::getName(int kind, int i)
{
    return table(kind)->getName(i);
}

// FUNC 0x002125C8 MachineTestRecords::setCar
void MachineTestRecords::setCar(int kind, u64 car, int i)
{
    table(kind)->setCar(car, i);
}

// FUNC 0x00212608 MachineTestRecords::getCar
u64 MachineTestRecords::getCar(int kind, int i)
{
    return table(kind)->getCar(i);
}

// FUNC 0x00212638 MachineTestRecords::setCode
//   (computes the table inline, not through table()).
void MachineTestRecords::setCode(int kind, const char *code, int i)
{
    tables[kind].setCode(code, i);
}

// FUNC 0x00212668 MachineTestRecords::getCode
//   (computes the table inline, not through table()).
char *MachineTestRecords::getCode(int kind, int i)
{
    return tables[kind].getCode(i);
}

// FUNC 0x00212690 MachineTestRecords::getDate
u32 MachineTestRecords::getDate(int kind, int i)
{
    return table(kind)->getDate(i);
}

// FUNC 0x002126C0 MachineTestRecords::remove
//   Kind 2 (max speed) clears its last place with zeros.
void MachineTestRecords::remove(int kind, int i)
{
    table(kind)->remove(i, kind == MACHINE_TEST_MAX_SPEED);
}

// FUNC 0x00212708 MachineTestRecords::size
int MachineTestRecords::size()
{
    return MachineTestTable::size() * MACHINE_TEST_KINDS;
}

// FUNC 0x00212730 MachineTestRecords::load
//   `arg' is not used (GameStatus passes 0); each table gets 0.
const u8 *MachineTestRecords::load(const u8 *src, int arg)
{
    int i;

    for (i = 0; i < MACHINE_TEST_KINDS; i++)
        src = tables[i].load(src, 0);
    return src;
}

// FUNC 0x00212788 MachineTestRecords::save
u8 *MachineTestRecords::save(u8 *dst)
{
    int i;

    for (i = 0; i < MACHINE_TEST_KINDS; i++)
        dst = tables[i].save(dst);
    return dst;
}
