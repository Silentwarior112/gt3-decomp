/* T045_reguration_result.h - RegurationResult, the result of a car's
 * regulation check (translation unit T045, unit u009; check_regulation and
 * isOk are T489's).
 *
 * In its own header since u040's data pass (2026-10-17): a class with an
 * inline virtual destructor only, so egcs writes its type name into every
 * unit that sees it, and retail units see it without the GT-mode page
 * checks (T089 0x003250C8, T090 0x00325250: RegurationResult's name among
 * the GameStatus's and the record sources').  T045_gtmode_page_checks.h
 * includes this header where it defined the class. */
#ifndef GAME_T045_REGURATION_RESULT_H
#define GAME_T045_REGURATION_RESULT_H

#include "game/game.h"

/* The result of check_regulation (T489 0x0023A438, by value): a flag per
 * rule the car breaks, [0] tyres (dirt tyres, part 0x42, for a needTire 9
 * race), [1] licence, [2] (? not set by check_regulation; isOk ignores it),
 * [3] drive train, [4] the race's car list, [5] aspiration, [6] tuning.
 *
 * vtable 0x00310310 (1 slot); more copies 0x00311C10 (T036), 0x00322CD8
 * (T084), 0x00342708 (T489), all with T045's destructor.  typeinfo
 * __tf16RegurationResult 0x0011C988 (__user_type_info 0x00353310 gp-0x46E0,
 * declared below; weak copies 0x001259D0 T036, 0x0023AE60
 * T489).  Data, then the vptr +0x1C; size 0x20 (the callers' locals). */
class RegurationResult {
public:
    /* Inline (check_regulation expands it): the vptr, then flags[6] ..
     * flags[0] = 0. */
    RegurationResult()
    {
        for (int i = 6; i >= 0; i--)
            flags[i] = 0;
    }
    /* Inline: the vptr, then the delete.  Out-of-line copies 0x0011C958
     * (T045, the one in the vtables), 0x001259A0 (T036), 0x00169710 (T084),
     * 0x0023AE30 (T489). */
    virtual ~RegurationResult() {}          /* [0] +0x0C 0x0011C958 */
    int isOk();                             /* 0x00239ED0 (T489) 1 if every flag but [2] is 0 */

    int flags[7];                           /* 0x00 */
    /*  vptr                                   0x1C */
};
EGCS_SIZE(RegurationResult, 0x20);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf16RegurationResult(void);         /* 0x0011C988 */
/* Its __user_type_info (2 words; T036's, T084's and T489's copies of the
 * __tf function fill it too).  The plan put it in T043. */
extern u32 __ti16RegurationResult[2];       /* 0x00353310 gp-0x46E0 */
}
#endif

#endif /* GAME_T045_REGURATION_RESULT_H */
