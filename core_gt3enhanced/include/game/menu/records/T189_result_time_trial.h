/* T189_result_time_trial.h - ResultTimeTrial, the result screen of a time
 * trial (translation unit T189, unit u041), 0x00176200..0x001762E8.
 *
 * Looks up the player's race time in the time trial records of the
 * GameStatus (+0x246F8, T417, table `index') and shows the place.  Held by
 * value by RaceTimeTrial (+0x2F9E0, T140). */
#ifndef GAME_T189_RESULT_TIME_TRIAL_H
#define GAME_T189_RESULT_TIME_TRIAL_H

#include "game/game.h"
#include "game/menu/records/T119_result_implement_records.h" /* base ResultImplementRecords */

class type_info;
class RaceOrganization;                     /* T125 (references only) */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E30 (name 0x003269D8, base __ti22ResultImplementRecords
 * 0x004B1E10). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf15ResultTimeTrial(void);       /* 0x00176268 */
}
#endif

/* vtable 0x00326970 (12 slots).  Size 0x24 (RaceTimeTrial: the next member
 * is at +0x2FA04).  The constructor and the destructor are implicit. */
class ResultTimeTrial : public ResultImplementRecords {
public:
    /* [0] +0x0C 0x001762B8 ~ResultTimeTrial: implicit (the copy for the
     * vtable follows the typeinfo function). */
    /*           [1] +0x14 .. [7] +0x44 as ResultImplementRecords */
    /* The place the player's race time takes in time trial table `index'. */
    virtual int  checkRecords(RaceOrganization &org);           /* [8] +0x4C 0x00176208 */
    /*           [9] +0x54 .. [11] +0x64 as ResultImplementRecords */
    void setIndex(int i);                   /* 0x00176200 caller: RaceTimeTrial::virtual_8 */

    int index;                              /* 0x20 the time trial (record table index) */
};
EGCS_SIZE(ResultTimeTrial, 0x24);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti15ResultTimeTrial[3];        /* 0x004B1E30 (__si_type_info; u041: g_004B1E30) */
}
#endif

#endif /* GAME_T189_RESULT_TIME_TRIAL_H */
