/* T148_race_machine_test.h - RaceMachineTest (the machine test: 0-400 m,
 * 0-1000 m, the max speed challenge) and RaceMachineTestInformation
 * (translation unit T148, unit u049), 0x0018C7E8..0x0018D1D8.
 *
 * RaceMachineTest is a RaceSolitaire registered in the race class registry
 * as "RaceMachineTest" (RaceClassInfo 0x002BA298 {"RaceMachineTest",
 * 0x48A80, race_machine_test_create, race_machine_test_construct}, node
 * 0x00351F30 gp-0x5AC0; race_machine_test_static_init, .ctors 0x0028F120).
 * The car settings menus run it as the races "M400" / "M1000" / "MMAX"
 * (run_machine_test, kind 0 / 1 / 2).  Its members: the kind, seven car icons
 * (T184), the ResultMachineTest, a RacePauseWithRetry menu (a race), a
 * ReplayPause menu (a replay), a plain RaceDisplay and the
 * DynamicsConductorMachineTest (RaceBase::dc).  The race mode name is
 * "RACE_MODE_MAX_SPEED_CHALLENGE" for the race code "MMAX"
 * (RaceOrganization::raceCode), else "RACE_MODE_0_400".
 *
 * RaceMachineTestInformation is the race information the menu builds on
 * the stack: two entries, the test car and the two option words of a GT
 * mode race (its accessors are the same code, in the same order, as
 * RaceGTmodeInformation's, T152, whose names are known).
 *
 * The virtual methods keep the slot names of the bases (RaceInformation
 * T124, RaceBase T129, RacePS2Base T130, RaceSolitaire T139). */
#ifndef GAME_T148_RACE_MACHINE_TEST_H
#define GAME_T148_RACE_MACHINE_TEST_H

#include "game/game.h"
#include <stddef.h>                             /* size_t */
#include "game/race/core/T124_race_information.h"         /* base RaceInformation */
#include "game/data/T482_car_equipments.h"           /* CarEquipments (held by value) */
#include "game/race/modes/T139_race_solitaire.h"           /* base RaceSolitaire */
#include "game/car/model/T184_car_icon_maker.h"           /* CarIcon (held by value) */
/* The pause headers before the result header: every race original that
 * sees both defines PauseMenu / ReplayPauseMenu before ResultTitleBar /
 * ResultTermBar (type names Term, Title, ReplayPauseMenu, PauseMenu ..),
 * u042 data pass. */
#include "game/race/pause/T171_race_pause_with_retry.h"    /* RacePauseWithRetry (held by value) */
#include "game/race/pause/T170_replay_pause.h"             /* ReplayPause (held by value) */
#include "game/menu/records/T120_result_machine_test.h"      /* ResultMachineTest (held by value) */
#include "game/race/display/T154_race_display.h"             /* RaceDisplay (held by value) */
#include "game/race/dynamics/T311_dc_machine_test.h"          /* DynamicsConductorMachineTest (held by value) */

class RaceBase;                                 /* T129 */
class RaceEntryBase;                            /* T121 (pointers only) */
class RaceEntryCar;                             /* T121 (pointers only) */
class RaceOrganization;                         /* T125 (pointers only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B20C0, base RaceSolitaire 0x004B1FE0) */
const type_info &__tf15RaceMachineTest(void);              /* 0x0018D118 */
/* (__si_type_info 0x004B20B0, base RaceInformation 0x00353378) */
const type_info &__tf26RaceMachineTestInformation(void);   /* 0x0018D188 */
}
#endif

/* vtable 0x0032CE10 (15 slots; 0, 4, 10 and 11 its own), typeinfo
 * __tf26RaceMachineTestInformation 0x0018D188.  vptr +0x54
 * (RaceInformation's); 0x248 bytes (a stack object of run_machine_test /
 * run_license_test). */
class RaceMachineTestInformation : public RaceInformation {
public:
    /* transmission = 0, two entries (vibration is left as it is). */
    RaceMachineTestInformation();               /* 0x0018C7E8 */
    virtual ~RaceMachineTestInformation();      /* [ 0] +0x0C 0x0018C860 */
    /* new RaceEntryPS2 */
    virtual RaceEntryBase *createEntries();     /* [ 4] +0x2C 0x0018C8B0 */
    /* Every entry gets the test car, input id 0x100 + its number and
     * order 0. */
    virtual void setupEntry(RaceEntryCar *entry);           /* [10] +0x5C 0x0018C908 */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x0018C958 nothing */

    /* car = c (u049: F_0018C960; the same code as
     * RaceGTmodeInformation::setCar).  Callers: run_machine_test, run_license_test. */
    void setCar(const CarEquipments &c);        /* 0x0018C960 */
    /* (no callers) loads the car `code', then its colour of colour id 0. */
    void setCar(u64 code);                      /* 0x0018C980 */
    /* (? names: RaceGTmodeInformation's identical accessors, T152 symbols;
     * u049: getField5C .. setField58; no callers.)  Inline: T148's
     * out-of-line copies sit after __tf15RaceMachineTest, before
     * __tf26RaceMachineTestInformation (egcs emits a class's inline members
     * with its typeinfo function, in reverse declaration order). */
    void setVibration(bool v) { vibration = v; }        /* 0x0018D180 */
    int  getVibrartion() { return vibration; }          /* 0x0018D178 (sic, as T152's) */
    void setTransmission(bool v) { transmission = v; }  /* 0x0018D170 */
    int  getTransmission() { return transmission; }     /* 0x0018D168 */

    /* run_machine_test fills both from GameStatus options, as for
     * RaceGTmodeInformation (u017: GameOptions::unk14 / unk24 ^ 1). */
    int           vibration;                    /* 0x58 (u049: field58) */
    int           transmission;                 /* 0x5C (u049: field5C) */
    CarEquipments car;                          /* 0x60 the test car */
};
EGCS_SIZE(RaceMachineTestInformation, 0x248);

/* vtable 0x0032CC28 (60 slots; 0, 8, 24, 30, 35 and 43 its own), typeinfo
 * __tf15RaceMachineTest 0x0018D118.  vptr +0x00 (GameObject's); 0x48A80
 * bytes (the race classes are 64-byte aligned: T132). */
class RaceMachineTest : public RaceSolitaire {
public:
    /* T148's copies of RaceBase's race-pool allocation (0x0017BA18 /
     * 0x0017BA40): g_002B3BF0, 16-byte aligned; the size is ignored. */
    static void *operator new(size_t size);     /* 0x0018C9D0 */
    static void operator delete(void *p, size_t size);  /* 0x0018C9F8 */
    /* (race_machine_test_construct uses the global placement new of <new>:
     * a class-member one gets an out-of-line copy retail does not have.) */

    /* kind: 0 0-400 m, 1 0-1000 m, 2 the max speed challenge.  Also called
     * directly by run_machine_test (built in place). */
    RaceMachineTest(int kind);                  /* 0x0018CAC0 */
    /* (deleting form: operator delete(this, 0x48A80)) */
    virtual ~RaceMachineTest();                 /* [ 0] +0x00C 0x0018CC80 */
    /* Race start (exited = 0): the mode name from the race code, the
     * result gets the kind; a race gets the retry pause menu and the result
     * sequence, a replay the replay pause menu. */
    virtual void virtual_8(int replay);         /* [ 8] +0x04C 0x0018CD30 */
    virtual const char *virtual_24();           /* [24] +0x0CC 0x0018CA20 "RaceMachineTest" */
    /* The update while paused (as RaceFreeRun's): the pause menu's result
     * (T168) 2 (timeout) virtual_34(3), 4 (exit) onExit then resumes, 3
     * (continue) resumes, 5 (retry) virtual_34(4) (the race again). */
    virtual void virtual_30();                  /* [30] +0x0FC 0x0018CE90 */
    virtual void virtual_35();                  /* [35] +0x124 0x0018CE48 RaceSolitaire's only */
    /* (RacePS2Base's is not called) builds the icons of the race's cars. */
    virtual void virtual_43();                  /* [43] +0x164 0x0018CE68 */

    /* The pause menu's exit (result 4; u049 onExit, as RaceFreeRun's
     * 0x001897D8).  In the max speed challenge, once per race,
     * outside replays, when car 0 has gone faster than 1.0 and not finished:
     * the run ends here with its top speed (event 5, car_finish(3), the
     * main input's record and lap stopped, g_00351FB8 = 3, the display's
     * finishDisplay off, the result's delay shortened); otherwise
     * virtual_34(2), the race is left. */
    void onExit();                              /* 0x0018CFB0 */

    int                          kind;          /* 0x21800 */
    CarIcon                      icons[7];      /* 0x21804 (u049: T184Object) */
    ResultMachineTest            result;        /* 0x2F9E0 */
    RacePauseWithRetry           retryPause;    /* 0x2FA08 the pause menu of a race */
    ReplayPause                  replayPause;   /* 0x2FCD8 (mode 0) */
    RaceDisplay                  display;       /* 0x30218 RaceBasic's race display */
    DynamicsConductorMachineTest dcMachineTest; /* 0x3A694 RaceBase::dc */
    int                          exited;        /* 0x48A40 the max speed run was ended from the pause menu (onExit; u049 exited) */
    /*  0x48A44 .. 0x48A7F: alignment */
};
EGCS_SIZE(RaceMachineTest, 0x48A80);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info nodes {vptr, name, base} the typeinfo functions fill
 * (u049: g_004B20B0 / g_004B20C0). */
extern "C" u32 __ti26RaceMachineTestInformation[3]; /* 0x004B20B0 */
extern "C" u32 __ti15RaceMachineTest[3];            /* 0x004B20C0 */
#endif

/* RaceClassInfo::create of "RaceMachineTest": new RaceMachineTest(0).
 * Also called by u017. */
RaceBase *race_machine_test_create(void);                   /* 0x0018CA30 */
/* RaceClassInfo::construct: a RaceMachineTest(0) in buf (NULL if buf is
 * NULL); arg is not used. */
RaceBase *race_machine_test_construct(void *buf, int arg);  /* 0x0018CA98 */
/* .ctors entry 0x0028F120: registers "RaceMachineTest". */
void      race_machine_test_static_init(void);              /* 0x0018D0F0 */

#endif /* GAME_T148_RACE_MACHINE_TEST_H */
