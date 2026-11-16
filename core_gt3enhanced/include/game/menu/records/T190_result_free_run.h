/* T190_result_free_run.h - ResultFreeRun, the result screen of a free run
 * (translation unit T190, unit u041), 0x001762E8..0x00176478.
 *
 * Looks up the player's best lap in the free run records of the GameStatus
 * (+0x26160, T416) under the course name and shows the place.  Held by
 * value by RaceFreeRun (+0x21800, T142). */
#ifndef GAME_T190_RESULT_FREE_RUN_H
#define GAME_T190_RESULT_FREE_RUN_H

#include "game/game.h"
#include "game/menu/records/T119_result_implement_records.h" /* base ResultImplementRecords */

class type_info;
class RaceOrganization;                     /* T125 (references only) */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E40 (name 0x00326CC8, base __ti22ResultImplementRecords
 * 0x004B1E10). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf13ResultFreeRun(void);         /* 0x001763F8 */
}
#endif

/* vtable 0x00326C60 (12 slots).  Size 0x60 (RaceFreeRun: the next member is
 * at +0x21860).  The constructor and the destructor are implicit. */
class ResultFreeRun : public ResultImplementRecords {
public:
    /* [0] +0x0C 0x00176448 ~ResultFreeRun: implicit (the copy for the
     * vtable follows the typeinfo function). */
    /* ResultImplementRecords' start, then a 0.1 s delay and a 15 s timer. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x001762E8 */
    /*           [2] +0x1C .. [7] +0x44 as ResultImplementRecords */
    /* The place the best lap (getRecord) takes in the course's records. */
    virtual int  checkRecords(RaceOrganization &org);           /* [8] +0x4C 0x00176350 */
    /*           [9] +0x54 ResultImplementRecords::drawRecord */
    /* The player's best lap (RaceLapTime::getBest(-1)). */
    virtual int  getRecord(RaceOrganization &org);              /* [10] +0x5C 0x001763C0 */
    /*           [11] +0x64 ResultImplementRecords::getRecordTitle */
    /* strcpy (no length check); NULL clears the name.  Caller: RaceFreeRun::RaceFreeRun. */
    void setCourse(const char *name);       /* 0x00176320 */

    char course[0x40];                      /* 0x20 the course name (the record table key) */
};
EGCS_SIZE(ResultFreeRun, 0x60);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti13ResultFreeRun[3];          /* 0x004B1E40 (__si_type_info; u041: g_004B1E40) */
}
#endif

#endif /* GAME_T190_RESULT_FREE_RUN_H */
