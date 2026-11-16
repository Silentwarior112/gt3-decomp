/* T089_records_menu_args.h - gt2k_menu::RecordsMenuArg and its subclasses, the
 * record sources of the records screen; SplitBattleOption (translation unit
 * T089, unit u040), 0x00171CA8..0x00172D58.
 *
 * run_records_menu (T090) gets one of these (a stack object of the race
 * modes, T052 / T053) and asks it, for each place i of a record table of the
 * current GameStatus (getGameStatus, read afresh every time): the number of
 * places, whether place i holds a record, its time (or speed) as text, the
 * name, the packed date, the car name, the 30-character ranking code,
 * whether it may be deleted, delete it.  One subclass per table: the time
 * trial ("compulsory") tests, the free run courses, the licence tests and
 * the three machine tests (0-400 m, 0-1000 m, max speed).
 *
 * The constructors but FreeTT's are inline in their users (they store the
 * fields and the vptr); their out-of-line copies (no callers) are in T089,
 * after the vtables' functions.  Compulsory's is defined inline here (T052
 * expands it; egcs still emits T089's copy, the unit of the class's first
 * virtual function); the others are declared only. */
#ifndef GAME_T089_RECORDS_MENU_ARGS_H
#define GAME_T089_RECORDS_MENU_ARGS_H

#include "game/game.h"
#include "game/menu/records/T089_split_battle_option.h"  /* SplitBattleOption (its own header: GameOptions and
                                               RaceOption hold one, see there) */

namespace gt2k_menu {

/* The abstract source.  typeinfo __tfQ29gt2k_menu14RecordsMenuArg
 * 0x00172D18 (__rtti_user; __user_type_info 0x00353308 gp-0x46E8).  No vtable
 * of its own in the image, no virtual destructor; vptr +0x00, no data.  Size
 * 4.
 *
 * Every virtual function is pure or inline (deleteEnable; the subclasses'
 * isDrawCarName too): the class has no key function, so egcs writes no
 * vtable for it and puts its type name into every unit that sees it, as
 * retail has it (T052, T053, T089..T092 and more).  T089's out-of-line
 * copies of the inline ones (for the vtables) come with the constructors and
 * typeinfo functions egcs emits at the end of the unit, in retail's order
 * (MaxSpeed's constructor and __tf, ..., Machinetest's constructor,
 * isDrawCarName and __tf, License's isDrawCarName, constructor and __tf,
 * ..., deleteEnable, __tf of this class).  The slots: */
class RecordsMenuArg {
public:
    virtual int   isDrawCarName() = 0;      /* [0] +0x0C 1: the car columns are shown */
    virtual int   getCount() = 0;           /* [1] +0x14 the number of places */
    virtual int   isValid(int i) = 0;       /* [2] +0x1C place i holds a record */
    virtual char *getTime(int i, char *buf) = 0;    /* [3] +0x24 its time (speed) as text into buf; buf */
    virtual const char *getName(int i) = 0; /* [4] +0x2C */
    virtual u32   getDate(int i) = 0;       /* [5] +0x34 the packed date (T519), 0: none */
    virtual char *getCarName(int i, char *buf) = 0; /* [6] +0x3C "" without a car column; buf */
    virtual const char *getCode(int i) = 0; /* [7] +0x44 the ranking code: three 10-character parts */
    /* Slot 8 of every vtable but Compulsory's: any place may be deleted. */
    virtual int   deleteEnable(int i) { return 1; }    /* [8] +0x4C 0x00172D10 */
    virtual void  deleteRecord(int i) = 0;  /* [9] +0x54 */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArg, 0x4);

/* vtable 0x00324F38 (10 slots).  typeinfo
 * __tfQ29gt2k_menu24RecordsMenuArgCompulsory 0x00172CC0 (si, base
 * RecordsMenuArg; __si_type_info 0x004B1D20).  The time trial
 * ("compulsory") test `index' (GameStatus timeTrial, T417), 12 places. */
class RecordsMenuArgCompulsory : public RecordsMenuArg {
public:
    /* Inline (u016: T052 expands it, index then the vptr); T089 defines the
     * class's first virtual function, so egcs emits its copy 0x00172CA8 there. */
    RecordsMenuArgCompulsory(int i) : index(i) {}
    /* No car column.  Inline: copy 0x00172CA0 in T089. */
    virtual int   isDrawCarName() { return 0; }     /* [0] +0x0C 0x00172CA0 */
    virtual int   getCount();               /* [1] +0x14 0x00171D48 12 */
    virtual int   isValid(int i);           /* [2] +0x1C 0x00171D50 the time is not 0x157529FF */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00171DA8 */
    virtual const char *getName(int i);     /* [4] +0x2C 0x00171E08 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x00171E50 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00171E98 "" */
    virtual const char *getCode(int i);     /* [7] +0x44 0x00171EA8 */
    virtual int   deleteEnable(int i);      /* [8] +0x4C 0x00171F38 not a preset record */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x00171EF0 the places after it move up */

    int index;                              /* 0x04 the test */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArgCompulsory, 0x8);

/* vtable 0x00324EE0 (10 slots: [8] RecordsMenuArg's).  typeinfo
 * __tfQ29gt2k_menu20RecordsMenuArgFreeTT 0x00172C50 (si, base
 * RecordsMenuArg; __si_type_info 0x004B1D30).  The free run records of a
 * course (GameStatus freeRun, T416), 12 places. */
class RecordsMenuArgFreeTT : public RecordsMenuArg {
public:
    RecordsMenuArgFreeTT(const char *name); /* 0x00171F80 the vptr, strcpy(course, name) (T052) */
    /* Inline: copy 0x00172C48 in T089. */
    virtual int   isDrawCarName() { return 1; }     /* [0] +0x0C 0x00172C48 */
    virtual int   getCount();               /* [1] +0x14 0x00171FB8 12 */
    virtual int   isValid(int i);           /* [2] +0x1C 0x00171FC0 */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00172018 */
    virtual const char *getName(int i);     /* [4] +0x2C 0x00172078 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x001720C0 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00172108 */
    virtual const char *getCode(int i);     /* [7] +0x44 0x00172168 */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x001721B0 */

    char course[0x40];                      /* 0x04 (? size) the course name */
};

/* vtable 0x00324E88 (10 slots: [8] RecordsMenuArg's).  typeinfo
 * __tfQ29gt2k_menu21RecordsMenuArgLicense 0x00172BF8 (si, base
 * RecordsMenuArg; __si_type_info 0x004B1D60).  The results of a licence
 * test (GameStatus licenses, T420), 6 places. */
class RecordsMenuArgLicense : public RecordsMenuArg {
public:
    /* Inline (u017: T053 expands it, licence, test, then the vptr); T089
     * defines the class's first virtual function, so egcs emits its copy
     * 0x00172BD8 there. */
    RecordsMenuArgLicense(int l, int t) : licence(l), test(t) {}
    /* No car column.  Inline: copy 0x00172BD0 in T089. */
    virtual int   isDrawCarName() { return 0; }     /* [0] +0x0C 0x00172BD0 */
    virtual int   getCount();               /* [1] +0x14 0x001721F8 6 */
    virtual int   isValid(int i);           /* [2] +0x1C 0x00172200 */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00172260 */
    virtual const char *getName(int i);     /* [4] +0x2C 0x001722C8 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x00172318 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00172368 "" */
    virtual const char *getCode(int i);     /* [7] +0x44 0x00172378 */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x001723C8 */

    int licence;                            /* 0x04 0..5 */
    int test;                               /* 0x08 */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArgLicense, 0xC);

/* vtable 0x00324E30 (10 slots: [2]..[7], [9] __pure_virtual, [8]
 * RecordsMenuArg's).  typeinfo __tfQ29gt2k_menu25RecordsMenuArgMachinetest
 * 0x00172B80 (si, base RecordsMenuArg; __si_type_info 0x004B1D50).  The base
 * of the three machine tests (GameStatus machineTest, T415), 12 places; no
 * data. */
class RecordsMenuArgMachinetest : public RecordsMenuArg {
public:
    /* Slot 0 of all four machine test vtables.  Inline: copy 0x00172B78 in
     * T089. */
    virtual int   isDrawCarName() { return 1; }     /* [0] +0x0C 0x00172B78 */
    virtual int   getCount();               /* [1] +0x14 0x00172418 12 (index.txt:
                                               RecordsMenuArgMaxSpeed::virtual_1) */
    /* Inline (the derived machine tests' inline constructors expand it);
     * egcs emits its copy 0x00172B60 in T089 (the vptr).  Declared after
     * isDrawCarName: egcs emits a class's inline members in the reverse
     * order of their declarations, and T089 has this copy first (the frame
     * info 0x002B1164), License's and Compulsory's after their
     * isDrawCarName. */
    RecordsMenuArgMachinetest() {}
};
EGCS_SIZE(gt2k_menu::RecordsMenuArgMachinetest, 0x4);

/* vtable 0x00324DD8.  typeinfo __tfQ29gt2k_menu20RecordsMenuArg_0_400
 * 0x00172B10 (si, base RecordsMenuArgMachinetest; __si_type_info
 * 0x004B1D40).  The 0-400 m test (kind 0). */
class RecordsMenuArg_0_400 : public RecordsMenuArgMachinetest {
public:
    /* Inline (u017: T053 expands it, the vptr); copy 0x00172AF8 in T089. */
    RecordsMenuArg_0_400() {}
    virtual int   isValid(int i);           /* [2] +0x1C 0x00172420 */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00172470 */
    virtual const char *getName(int i);     /* [4] +0x2C 0x001724C8 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x00172508 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00172548 */
    virtual const char *getCode(int i);     /* [7] +0x44 0x001725A0 */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x001725E0 */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArg_0_400, 0x4);

/* vtable 0x00324D80.  typeinfo __tfQ29gt2k_menu21RecordsMenuArg_0_1000
 * 0x00172AA8 (si, base RecordsMenuArgMachinetest; __si_type_info
 * 0x004B1D70).  The 0-1000 m test (kind 1). */
class RecordsMenuArg_0_1000 : public RecordsMenuArgMachinetest {
public:
    /* Inline (u017: T053 expands it, the vptr); copy 0x00172A90 in T089. */
    RecordsMenuArg_0_1000() {}
    virtual int   isValid(int i);           /* [2] +0x1C 0x00172620 */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00172670 */
    virtual const char *getName(int i);     /* [4] +0x2C 0x001726C8 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x00172708 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00172748 */
    virtual const char *getCode(int i);     /* [7] +0x44 0x001727A0 */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x001727E0 */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArg_0_1000, 0x4);

/* vtable 0x00324D28.  typeinfo __tfQ29gt2k_menu22RecordsMenuArgMaxSpeed
 * 0x00172A40 (si, base RecordsMenuArgMachinetest; __si_type_info
 * 0x004B1D10).  The max speed test (kind 2): speeds instead of times, a
 * place is empty when its speed is 0. */
class RecordsMenuArgMaxSpeed : public RecordsMenuArgMachinetest {
public:
    /* Inline (u017: T053 expands it, the vptr); copy 0x00172A28 in T089. */
    RecordsMenuArgMaxSpeed() {}
    virtual int   isValid(int i);           /* [2] +0x1C 0x00172820 */
    virtual char *getTime(int i, char *buf);/* [3] +0x24 0x00172878 the speed in the user's unit */
    virtual const char *getName(int i);     /* [4] +0x2C 0x001728D0 */
    virtual u32   getDate(int i);           /* [5] +0x34 0x00172910 */
    virtual char *getCarName(int i, char *buf); /* [6] +0x3C 0x00172950 */
    virtual const char *getCode(int i);     /* [7] +0x44 0x001729A8 */
    virtual void  deleteRecord(int i);      /* [9] +0x54 0x001729E8 */
};
EGCS_SIZE(gt2k_menu::RecordsMenuArgMaxSpeed, 0x4);

} /* namespace gt2k_menu */

/* T089's helpers. */
char *records_format_time(char *buf, u32 time);     /* 0x00171CA8 the time (ms) as text (T494); buf */
char *records_format_car(char *buf, u64 car);       /* 0x00171CD0 "<name> <second name>", "" for car 0; buf */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu22RecordsMenuArgMaxSpeed(void);   /* 0x00172A40 */
void *__tfQ29gt2k_menu21RecordsMenuArg_0_1000(void);    /* 0x00172AA8 */
void *__tfQ29gt2k_menu20RecordsMenuArg_0_400(void);     /* 0x00172B10 */
void *__tfQ29gt2k_menu25RecordsMenuArgMachinetest(void);/* 0x00172B80 */
void *__tfQ29gt2k_menu21RecordsMenuArgLicense(void);    /* 0x00172BF8 */
void *__tfQ29gt2k_menu20RecordsMenuArgFreeTT(void);     /* 0x00172C50 */
void *__tfQ29gt2k_menu24RecordsMenuArgCompulsory(void); /* 0x00172CC0 */
void *__tfQ29gt2k_menu14RecordsMenuArg(void);           /* 0x00172D18 */
/* Their type_info storage (.bss / .sbss), filled by them: __si_type_info = 3
 * words, __user_type_info = 2. */
extern u32 __tiQ29gt2k_menu22RecordsMenuArgMaxSpeed[3];     /* 0x004B1D10 */
extern u32 __tiQ29gt2k_menu24RecordsMenuArgCompulsory[3];   /* 0x004B1D20 */
extern u32 __tiQ29gt2k_menu20RecordsMenuArgFreeTT[3];       /* 0x004B1D30 */
extern u32 __tiQ29gt2k_menu20RecordsMenuArg_0_400[3];       /* 0x004B1D40 */
extern u32 __tiQ29gt2k_menu25RecordsMenuArgMachinetest[3];  /* 0x004B1D50 */
extern u32 __tiQ29gt2k_menu21RecordsMenuArgLicense[3];      /* 0x004B1D60 */
extern u32 __tiQ29gt2k_menu21RecordsMenuArg_0_1000[3];      /* 0x004B1D70 */
extern u32 __tiQ29gt2k_menu14RecordsMenuArg[2];             /* 0x00353308 gp-0x46E8 */
/* (__ti14RaceResultBase / __ti12ResultArcade, which the plan put here, are
 * declared with their classes in T116 / T117.) */
}
#endif

#endif /* GAME_T089_RECORDS_MENU_ARGS_H */
