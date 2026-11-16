/* T089 - the record sources of the records screen (gt2k_menu::RecordsMenuArg
 * and its subclasses, game/menu/records/T089_records_menu_args.h).  0x00171CA8..0x00172D58.
 *
 * run_records_menu (T090) gets one of these and asks it, for each place i
 * of the table: is there a record (isValid), its time (or speed) as text, the
 * name, the packed date, the car name, the 30-character ranking code; may it
 * be deleted, delete it.  Every method reads the current GameStatus
 * (getGameStatus) afresh. */
#include "libc.h"                           /* strcpy, strcat */
#include "game/common/T494_text_utils.h"           /* format_time, format_speed */
#include "game/data/T489_car_database.h"         /* db_car_name, db_car_name2 */
#include "game/race/core/T128_race_lap_record.h"      /* RACE_TIME_INVALID */
/* Retail's type names (0x00324F90..0x003251C8) in the reverse order of the
 * classes' definitions: the records menu args, RegurationResult, the
 * GameStatus's garage lists; no other class with inline virtual functions
 * only (no menu classes, no <new>). */
#include "game/save/T429_game_status.h"          /* getGameStatus; timeTrial, freeRun, licences, machineTest */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used) */
#include "game/menu/records/T089_records_menu_args.h"
#include "game/menu/records/records_internal.h"                           /* RECORDS_PLACES, RECORDS_LICENSE_PLACES */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user */
#endif

/* A time as text (format_time) into buf; returns buf.  Callers: the five
 * getTime of the time tables. */
// FUNC 0x00171CA8 records_format_time
char *records_format_time(char *buf, u32 time)
{
    format_time(buf, time);
    return buf;
}

/* "<name> <second name>" of a car, "" for car 0; returns buf. */
// FUNC 0x00171CD0 records_format_car
char *records_format_car(char *buf, u64 car)
{
    if (car == 0) {
        buf[0] = 0;
        return buf;
    }
    strcpy(buf, db_car_name(car));
    strcat(buf, " ");                       /* 0x00351C78 */
    strcat(buf, db_car_name2(car));
    return buf;
}

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* RecordsMenuArgCompulsory: GameStatus::timeTrial, test `index'             */
/* ------------------------------------------------------------------------ */

// FUNC 0x00171D48 gt2k_menu::RecordsMenuArgCompulsory::getCount
int RecordsMenuArgCompulsory::getCount()
{
    return RECORDS_PLACES;
}

// FUNC 0x00171D50 gt2k_menu::RecordsMenuArgCompulsory::isValid
int RecordsMenuArgCompulsory::isValid(int i)
{
    return getGameStatus()->timeTrial.getTime(index, i) != RACE_TIME_INVALID;
}

// FUNC 0x00171DA8 gt2k_menu::RecordsMenuArgCompulsory::getTime
char *RecordsMenuArgCompulsory::getTime(int i, char *buf)
{
    return records_format_time(buf, getGameStatus()->timeTrial.getTime(index, i));
}

// FUNC 0x00171E08 gt2k_menu::RecordsMenuArgCompulsory::getName
const char *RecordsMenuArgCompulsory::getName(int i)
{
    return getGameStatus()->timeTrial.getName(index, i);
}

// FUNC 0x00171E50 gt2k_menu::RecordsMenuArgCompulsory::getDate
u32 RecordsMenuArgCompulsory::getDate(int i)
{
    return getGameStatus()->timeTrial.getDate(index, i);
}

/* No car column: the empty string. */
// FUNC 0x00171E98 gt2k_menu::RecordsMenuArgCompulsory::getCarName
char *RecordsMenuArgCompulsory::getCarName(int i, char *buf)
{
    buf[0] = 0;
    return buf;
}

// FUNC 0x00171EA8 gt2k_menu::RecordsMenuArgCompulsory::getCode
const char *RecordsMenuArgCompulsory::getCode(int i)
{
    return getGameStatus()->timeTrial.getCode(index, i)->s;
}

// FUNC 0x00171EF0 gt2k_menu::RecordsMenuArgCompulsory::deleteRecord
void RecordsMenuArgCompulsory::deleteRecord(int i)
{
    getGameStatus()->timeTrial.remove(index, i);
}

/* index.txt: virtual_8 (slot 8).  The preset records cannot be deleted
 * (isPlayerRecord 0). */
// FUNC 0x00171F38 gt2k_menu::RecordsMenuArgCompulsory::deleteEnable
int RecordsMenuArgCompulsory::deleteEnable(int i)
{
    return getGameStatus()->timeTrial.isPlayerRecord(index, i);
}

/* ------------------------------------------------------------------------ */
/* RecordsMenuArgFreeTT: GameStatus::freeRun, course `course'                */
/* ------------------------------------------------------------------------ */

/* The vptr, then strcpy(course, name).  Caller: run_free_run (T052). */
// FUNC 0x00171F80 gt2k_menu::RecordsMenuArgFreeTT::RecordsMenuArgFreeTT
RecordsMenuArgFreeTT::RecordsMenuArgFreeTT(const char *name)
{
    strcpy(course, name);
}

// FUNC 0x00171FB8 gt2k_menu::RecordsMenuArgFreeTT::getCount
int RecordsMenuArgFreeTT::getCount()
{
    return RECORDS_PLACES;
}

// FUNC 0x00171FC0 gt2k_menu::RecordsMenuArgFreeTT::isValid
int RecordsMenuArgFreeTT::isValid(int i)
{
    return getGameStatus()->freeRun.getTime(course, i) != RACE_TIME_INVALID;
}

// FUNC 0x00172018 gt2k_menu::RecordsMenuArgFreeTT::getTime
char *RecordsMenuArgFreeTT::getTime(int i, char *buf)
{
    return records_format_time(buf, getGameStatus()->freeRun.getTime(course, i));
}

// FUNC 0x00172078 gt2k_menu::RecordsMenuArgFreeTT::getName
const char *RecordsMenuArgFreeTT::getName(int i)
{
    return getGameStatus()->freeRun.getName(course, i);
}

// FUNC 0x001720C0 gt2k_menu::RecordsMenuArgFreeTT::getDate
u32 RecordsMenuArgFreeTT::getDate(int i)
{
    return getGameStatus()->freeRun.getDate(course, i);
}

// FUNC 0x00172108 gt2k_menu::RecordsMenuArgFreeTT::getCarName
char *RecordsMenuArgFreeTT::getCarName(int i, char *buf)
{
    return records_format_car(buf, getGameStatus()->freeRun.getCar(course, i));
}

// FUNC 0x00172168 gt2k_menu::RecordsMenuArgFreeTT::getCode
const char *RecordsMenuArgFreeTT::getCode(int i)
{
    return getGameStatus()->freeRun.getCode(course, i);
}

// FUNC 0x001721B0 gt2k_menu::RecordsMenuArgFreeTT::deleteRecord
void RecordsMenuArgFreeTT::deleteRecord(int i)
{
    getGameStatus()->freeRun.remove(course, i);
}

/* ------------------------------------------------------------------------ */
/* RecordsMenuArgLicense: GameStatus::licences, test `test' of `licence'     */
/* ------------------------------------------------------------------------ */

// FUNC 0x001721F8 gt2k_menu::RecordsMenuArgLicense::getCount
int RecordsMenuArgLicense::getCount()
{
    return RECORDS_LICENSE_PLACES;
}

// FUNC 0x00172200 gt2k_menu::RecordsMenuArgLicense::isValid
int RecordsMenuArgLicense::isValid(int i)
{
    return getGameStatus()->licences.getTime(licence, test, i) != RACE_TIME_INVALID;
}

// FUNC 0x00172260 gt2k_menu::RecordsMenuArgLicense::getTime
char *RecordsMenuArgLicense::getTime(int i, char *buf)
{
    return records_format_time(buf, getGameStatus()->licences.getTime(licence, test, i));
}

// FUNC 0x001722C8 gt2k_menu::RecordsMenuArgLicense::getName
const char *RecordsMenuArgLicense::getName(int i)
{
    return getGameStatus()->licences.getName(licence, test, i);
}

// FUNC 0x00172318 gt2k_menu::RecordsMenuArgLicense::getDate
u32 RecordsMenuArgLicense::getDate(int i)
{
    return getGameStatus()->licences.getDate(licence, test, i);
}

/* No car column: the empty string. */
// FUNC 0x00172368 gt2k_menu::RecordsMenuArgLicense::getCarName
char *RecordsMenuArgLicense::getCarName(int i, char *buf)
{
    buf[0] = 0;
    return buf;
}

// FUNC 0x00172378 gt2k_menu::RecordsMenuArgLicense::getCode
const char *RecordsMenuArgLicense::getCode(int i)
{
    return getGameStatus()->licences.getCode(licence, test, i)->s;
}

// FUNC 0x001723C8 gt2k_menu::RecordsMenuArgLicense::deleteRecord
void RecordsMenuArgLicense::deleteRecord(int i)
{
    getGameStatus()->licences.remove(licence, test, i);
}

/* ------------------------------------------------------------------------ */
/* The machine tests: GameStatus::machineTest, kinds 0 / 1 / 2               */
/* ------------------------------------------------------------------------ */

/* index.txt: RecordsMenuArgMaxSpeed::virtual_1; slot 1 of all four machine
 * test vtables (RecordsMenuArgMachinetest's own). */
// FUNC 0x00172418 gt2k_menu::RecordsMenuArgMachinetest::getCount
int RecordsMenuArgMachinetest::getCount()
{
    return RECORDS_PLACES;
}

// FUNC 0x00172420 gt2k_menu::RecordsMenuArg_0_400::isValid
int RecordsMenuArg_0_400::isValid(int i)
{
    return getGameStatus()->machineTest.getTime(MACHINE_TEST_400M, i) != RACE_TIME_INVALID;
}

// FUNC 0x00172470 gt2k_menu::RecordsMenuArg_0_400::getTime
char *RecordsMenuArg_0_400::getTime(int i, char *buf)
{
    return records_format_time(buf, getGameStatus()->machineTest.getTime(MACHINE_TEST_400M, i));
}

// FUNC 0x001724C8 gt2k_menu::RecordsMenuArg_0_400::getName
const char *RecordsMenuArg_0_400::getName(int i)
{
    return getGameStatus()->machineTest.getName(MACHINE_TEST_400M, i);
}

// FUNC 0x00172508 gt2k_menu::RecordsMenuArg_0_400::getDate
u32 RecordsMenuArg_0_400::getDate(int i)
{
    return getGameStatus()->machineTest.getDate(MACHINE_TEST_400M, i);
}

// FUNC 0x00172548 gt2k_menu::RecordsMenuArg_0_400::getCarName
char *RecordsMenuArg_0_400::getCarName(int i, char *buf)
{
    return records_format_car(buf, getGameStatus()->machineTest.getCar(MACHINE_TEST_400M, i));
}

// FUNC 0x001725A0 gt2k_menu::RecordsMenuArg_0_400::getCode
const char *RecordsMenuArg_0_400::getCode(int i)
{
    return getGameStatus()->machineTest.getCode(MACHINE_TEST_400M, i);
}

// FUNC 0x001725E0 gt2k_menu::RecordsMenuArg_0_400::deleteRecord
void RecordsMenuArg_0_400::deleteRecord(int i)
{
    getGameStatus()->machineTest.remove(MACHINE_TEST_400M, i);
}

// FUNC 0x00172620 gt2k_menu::RecordsMenuArg_0_1000::isValid
int RecordsMenuArg_0_1000::isValid(int i)
{
    return getGameStatus()->machineTest.getTime(MACHINE_TEST_1000M, i) != RACE_TIME_INVALID;
}

// FUNC 0x00172670 gt2k_menu::RecordsMenuArg_0_1000::getTime
char *RecordsMenuArg_0_1000::getTime(int i, char *buf)
{
    return records_format_time(buf, getGameStatus()->machineTest.getTime(MACHINE_TEST_1000M, i));
}

// FUNC 0x001726C8 gt2k_menu::RecordsMenuArg_0_1000::getName
const char *RecordsMenuArg_0_1000::getName(int i)
{
    return getGameStatus()->machineTest.getName(MACHINE_TEST_1000M, i);
}

// FUNC 0x00172708 gt2k_menu::RecordsMenuArg_0_1000::getDate
u32 RecordsMenuArg_0_1000::getDate(int i)
{
    return getGameStatus()->machineTest.getDate(MACHINE_TEST_1000M, i);
}

// FUNC 0x00172748 gt2k_menu::RecordsMenuArg_0_1000::getCarName
char *RecordsMenuArg_0_1000::getCarName(int i, char *buf)
{
    return records_format_car(buf, getGameStatus()->machineTest.getCar(MACHINE_TEST_1000M, i));
}

// FUNC 0x001727A0 gt2k_menu::RecordsMenuArg_0_1000::getCode
const char *RecordsMenuArg_0_1000::getCode(int i)
{
    return getGameStatus()->machineTest.getCode(MACHINE_TEST_1000M, i);
}

// FUNC 0x001727E0 gt2k_menu::RecordsMenuArg_0_1000::deleteRecord
void RecordsMenuArg_0_1000::deleteRecord(int i)
{
    getGameStatus()->machineTest.remove(MACHINE_TEST_1000M, i);
}

/* The max speed table holds speeds: a place is empty when its speed is 0. */
// FUNC 0x00172820 gt2k_menu::RecordsMenuArgMaxSpeed::isValid
int RecordsMenuArgMaxSpeed::isValid(int i)
{
    return getGameStatus()->machineTest.getSpeed(MACHINE_TEST_MAX_SPEED, i) != 0.0f;
}

/* The speed in the user's unit (format_speed) instead of a time. */
// FUNC 0x00172878 gt2k_menu::RecordsMenuArgMaxSpeed::getTime
char *RecordsMenuArgMaxSpeed::getTime(int i, char *buf)
{
    format_speed(buf, getGameStatus()->machineTest.getSpeed(MACHINE_TEST_MAX_SPEED, i));
    return buf;
}

// FUNC 0x001728D0 gt2k_menu::RecordsMenuArgMaxSpeed::getName
const char *RecordsMenuArgMaxSpeed::getName(int i)
{
    return getGameStatus()->machineTest.getName(MACHINE_TEST_MAX_SPEED, i);
}

// FUNC 0x00172910 gt2k_menu::RecordsMenuArgMaxSpeed::getDate
u32 RecordsMenuArgMaxSpeed::getDate(int i)
{
    return getGameStatus()->machineTest.getDate(MACHINE_TEST_MAX_SPEED, i);
}

// FUNC 0x00172950 gt2k_menu::RecordsMenuArgMaxSpeed::getCarName
char *RecordsMenuArgMaxSpeed::getCarName(int i, char *buf)
{
    return records_format_car(buf, getGameStatus()->machineTest.getCar(MACHINE_TEST_MAX_SPEED, i));
}

// FUNC 0x001729A8 gt2k_menu::RecordsMenuArgMaxSpeed::getCode
const char *RecordsMenuArgMaxSpeed::getCode(int i)
{
    return getGameStatus()->machineTest.getCode(MACHINE_TEST_MAX_SPEED, i);
}

// FUNC 0x001729E8 gt2k_menu::RecordsMenuArgMaxSpeed::deleteRecord
void RecordsMenuArgMaxSpeed::deleteRecord(int i)
{
    getGameStatus()->machineTest.remove(MACHINE_TEST_MAX_SPEED, i);
}

/* ------------------------------------------------------------------------ */
/* Out-of-line copies of the inline constructors (no callers), the typeinfo */
/* functions and the trivial virtuals, in the order egcs emitted them       */
/* ------------------------------------------------------------------------ */

/* No callers: the vptr only. */
// FUNC 0x00172A28 gt2k_menu::RecordsMenuArgMaxSpeed::RecordsMenuArgMaxSpeed
/* inline in game/menu/records/T089_records_menu_args.h (T053 expands it); egcs emits this
 * copy (this unit defines the class's first virtual function) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172A40 __tfQ29gt2k_menu22RecordsMenuArgMaxSpeed
extern "C" void *__tfQ29gt2k_menu22RecordsMenuArgMaxSpeed(void)
{
    if (__tiQ29gt2k_menu22RecordsMenuArgMaxSpeed[0] == 0) {
        __tfQ29gt2k_menu25RecordsMenuArgMachinetest();
        __rtti_si(__tiQ29gt2k_menu22RecordsMenuArgMaxSpeed, "Q29gt2k_menu22RecordsMenuArgMaxSpeed",
                  (const type_info *)__tiQ29gt2k_menu25RecordsMenuArgMachinetest);
    }
    return __tiQ29gt2k_menu22RecordsMenuArgMaxSpeed;
}
#endif

namespace gt2k_menu {
/* No callers: the vptr only. */
// FUNC 0x00172A90 gt2k_menu::RecordsMenuArg_0_1000::RecordsMenuArg_0_1000
/* inline in game/menu/records/T089_records_menu_args.h (T053 expands it); egcs emits this
 * copy (this unit defines the class's first virtual function) */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172AA8 __tfQ29gt2k_menu21RecordsMenuArg_0_1000
extern "C" void *__tfQ29gt2k_menu21RecordsMenuArg_0_1000(void)
{
    if (__tiQ29gt2k_menu21RecordsMenuArg_0_1000[0] == 0) {
        __tfQ29gt2k_menu25RecordsMenuArgMachinetest();
        __rtti_si(__tiQ29gt2k_menu21RecordsMenuArg_0_1000, "Q29gt2k_menu21RecordsMenuArg_0_1000",
                  (const type_info *)__tiQ29gt2k_menu25RecordsMenuArgMachinetest);
    }
    return __tiQ29gt2k_menu21RecordsMenuArg_0_1000;
}
#endif

namespace gt2k_menu {
/* No callers: the vptr only. */
// FUNC 0x00172AF8 gt2k_menu::RecordsMenuArg_0_400::RecordsMenuArg_0_400
/* inline in game/menu/records/T089_records_menu_args.h (T053 expands it); egcs emits this
 * copy (this unit defines the class's first virtual function) */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172B10 __tfQ29gt2k_menu20RecordsMenuArg_0_400
extern "C" void *__tfQ29gt2k_menu20RecordsMenuArg_0_400(void)
{
    if (__tiQ29gt2k_menu20RecordsMenuArg_0_400[0] == 0) {
        __tfQ29gt2k_menu25RecordsMenuArgMachinetest();
        __rtti_si(__tiQ29gt2k_menu20RecordsMenuArg_0_400, "Q29gt2k_menu20RecordsMenuArg_0_400",
                  (const type_info *)__tiQ29gt2k_menu25RecordsMenuArgMachinetest);
    }
    return __tiQ29gt2k_menu20RecordsMenuArg_0_400;
}
#endif

namespace gt2k_menu {
/* No callers: the vptr only (the abstract base of the machine tests). */
// FUNC 0x00172B60 gt2k_menu::RecordsMenuArgMachinetest::RecordsMenuArgMachinetest
/* inline in game/menu/records/T089_records_menu_args.h (the derived constructors expand
 * it); egcs emits this copy (this unit defines the class's first virtual
 * function) */

// FUNC 0x00172B78 gt2k_menu::RecordsMenuArgMachinetest::isDrawCarName
/* inline in game/menu/records/T089_records_menu_args.h (1); egcs emits this copy for the
 * vtables */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function.  Callers: the three machine tests'. */
// FUNC 0x00172B80 __tfQ29gt2k_menu25RecordsMenuArgMachinetest
extern "C" void *__tfQ29gt2k_menu25RecordsMenuArgMachinetest(void)
{
    if (__tiQ29gt2k_menu25RecordsMenuArgMachinetest[0] == 0) {
        __tfQ29gt2k_menu14RecordsMenuArg();
        __rtti_si(__tiQ29gt2k_menu25RecordsMenuArgMachinetest, "Q29gt2k_menu25RecordsMenuArgMachinetest",
                  (const type_info *)__tiQ29gt2k_menu14RecordsMenuArg);
    }
    return __tiQ29gt2k_menu25RecordsMenuArgMachinetest;
}
#endif

namespace gt2k_menu {
// FUNC 0x00172BD0 gt2k_menu::RecordsMenuArgLicense::isDrawCarName
/* inline in game/menu/records/T089_records_menu_args.h (0); egcs emits this copy for the
 * vtable */

/* No callers: licence, test, then the vptr. */
// FUNC 0x00172BD8 gt2k_menu::RecordsMenuArgLicense::RecordsMenuArgLicense
/* inline in game/menu/records/T089_records_menu_args.h (T053 expands it); egcs emits this
 * copy (this unit defines the class's first virtual function) */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172BF8 __tfQ29gt2k_menu21RecordsMenuArgLicense
extern "C" void *__tfQ29gt2k_menu21RecordsMenuArgLicense(void)
{
    if (__tiQ29gt2k_menu21RecordsMenuArgLicense[0] == 0) {
        __tfQ29gt2k_menu14RecordsMenuArg();
        __rtti_si(__tiQ29gt2k_menu21RecordsMenuArgLicense, "Q29gt2k_menu21RecordsMenuArgLicense",
                  (const type_info *)__tiQ29gt2k_menu14RecordsMenuArg);
    }
    return __tiQ29gt2k_menu21RecordsMenuArgLicense;
}
#endif

namespace gt2k_menu {
// FUNC 0x00172C48 gt2k_menu::RecordsMenuArgFreeTT::isDrawCarName
/* inline in game/menu/records/T089_records_menu_args.h (1); egcs emits this copy for the
 * vtable */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172C50 __tfQ29gt2k_menu20RecordsMenuArgFreeTT
extern "C" void *__tfQ29gt2k_menu20RecordsMenuArgFreeTT(void)
{
    if (__tiQ29gt2k_menu20RecordsMenuArgFreeTT[0] == 0) {
        __tfQ29gt2k_menu14RecordsMenuArg();
        __rtti_si(__tiQ29gt2k_menu20RecordsMenuArgFreeTT, "Q29gt2k_menu20RecordsMenuArgFreeTT",
                  (const type_info *)__tiQ29gt2k_menu14RecordsMenuArg);
    }
    return __tiQ29gt2k_menu20RecordsMenuArgFreeTT;
}
#endif

namespace gt2k_menu {
// FUNC 0x00172CA0 gt2k_menu::RecordsMenuArgCompulsory::isDrawCarName
/* inline in game/menu/records/T089_records_menu_args.h (0); egcs emits this copy for the
 * vtable */

/* No callers: index, then the vptr. */
// FUNC 0x00172CA8 gt2k_menu::RecordsMenuArgCompulsory::RecordsMenuArgCompulsory
/* inline in game/menu/records/T089_records_menu_args.h (T052 expands it); egcs emits this
 * copy (this unit defines the class's first virtual function) */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00172CC0 __tfQ29gt2k_menu24RecordsMenuArgCompulsory
extern "C" void *__tfQ29gt2k_menu24RecordsMenuArgCompulsory(void)
{
    if (__tiQ29gt2k_menu24RecordsMenuArgCompulsory[0] == 0) {
        __tfQ29gt2k_menu14RecordsMenuArg();
        __rtti_si(__tiQ29gt2k_menu24RecordsMenuArgCompulsory, "Q29gt2k_menu24RecordsMenuArgCompulsory",
                  (const type_info *)__tiQ29gt2k_menu14RecordsMenuArg);
    }
    return __tiQ29gt2k_menu24RecordsMenuArgCompulsory;
}
#endif

namespace gt2k_menu {
// FUNC 0x00172D10 gt2k_menu::RecordsMenuArg::deleteEnable
/* inline in game/menu/records/T089_records_menu_args.h (1: slot 8 of every vtable but
 * Compulsory's); egcs emits this copy for the vtables */
} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function of the base (no bases: __rtti_user;
 * the node is in .sbss at gp-0x46E8). */
// FUNC 0x00172D18 __tfQ29gt2k_menu14RecordsMenuArg
extern "C" void *__tfQ29gt2k_menu14RecordsMenuArg(void)
{
    if (__tiQ29gt2k_menu14RecordsMenuArg[0] == 0)
        __rtti_user(__tiQ29gt2k_menu14RecordsMenuArg, "Q29gt2k_menu14RecordsMenuArg");
    return __tiQ29gt2k_menu14RecordsMenuArg;
}
#endif
