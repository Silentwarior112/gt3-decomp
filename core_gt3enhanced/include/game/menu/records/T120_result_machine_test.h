/* T120_result_machine_test.h - ResultMachineTest, the result screen of a
 * machine test (0-400 m, 0-1000 m, max speed; translation unit T120, unit
 * u041), 0x00175F48..0x00176200.
 *
 * Looks up the player's race time, or in speed mode (the max speed test) the
 * car's top speed, in the machine test records of the GameStatus (T415,
 * table `kind') and shows the place.  Held by value by RaceMachineTest
 * (+0x2F9E0, T148). */
#ifndef GAME_T120_RESULT_MACHINE_TEST_H
#define GAME_T120_RESULT_MACHINE_TEST_H

#include "game/game.h"
#include "game/menu/records/T119_result_implement_records.h" /* base ResultImplementRecords */

class type_info;
class RaceOrganization;                     /* T125 (references only) */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E20 (name 0x003266E8, base __ti22ResultImplementRecords
 * 0x004B1E10). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf17ResultMachineTest(void);     /* 0x00176180 */
}
#endif

/* vtable 0x00326680 (12 slots).  Size 0x28 (RaceMachineTest: the next
 * member is at +0x2FA08).  The destructor is implicit. */
class ResultMachineTest : public ResultImplementRecords {
public:
    /* Inline (Lupo 2: linkonce); 0x00176160 is T120's copy (no callers). */
    ResultMachineTest() : speedMode(0) {}
    /* [0] +0x0C 0x001761D0 ~ResultMachineTest: implicit (the copy for the
     * vtable follows the typeinfo function). */
    /*           [1] +0x14 .. [7] +0x44 as ResultImplementRecords */
    /* The place the speed (speed mode) or the race time would take. */
    virtual int  checkRecords(RaceOrganization &org);           /* [8] +0x4C 0x00175F78 */
    /* Speed mode: "Max Speed" and the speed; else the base's. */
    virtual void drawRecord(RaceOrganization &org);             /* [9] +0x54 0x00176060 */
    /*           [10] +0x5C ResultImplementRecords::getRecord */
    /* Speed mode: "NEW RECORD" / "RECORD SPEED"; else the base's. */
    virtual const char *getRecordTitle();   /* [11] +0x64 0x00176018 */
    /* kind = k; kind 2 (the max speed test) switches to speed mode (never
     * back).  Caller: RaceMachineTest::virtual_8. */
    void setKind(int k);                    /* 0x00175F48 */
    /* The screen reacts after 0.1 s instead of 5 s.  Caller: RaceMachineTest::onExit. */
    void shortenDelay();                    /* 0x00175F68 */

    int speedMode;                          /* 0x20 kind 2: the record is a speed */
    int kind;                               /* 0x24 the test (the machine test record table) */
};
EGCS_SIZE(ResultMachineTest, 0x28);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti17ResultMachineTest[3];      /* 0x004B1E20 (__si_type_info; u041: g_004B1E20) */
}
#endif

#endif /* GAME_T120_RESULT_MACHINE_TEST_H */
