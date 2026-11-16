/* T191_result_link_battle.h - ResultLinkBattle, the result board of an
 * i.LINK battle (translation unit T191, unit u041), 0x00176478..0x00176690.
 *
 * When the first car reaches the goal the others get 20 s (waitTimer);
 * then the board finishes by itself and leaves 20 s later (the pad is
 * ignored).  Held by value by RaceLinkBattle (+0x299F8, T138). */
#ifndef GAME_T191_RESULT_LINK_BATTLE_H
#define GAME_T191_RESULT_LINK_BATTLE_H

#include "game/game.h"
#include "game/menu/records/T117_result_arcade.h"        /* base ResultArcade */

class type_info;
class RaceOrganization;                     /* T125 (references only) */
class Control;                              /* T474 */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E50 (name 0x00326FA0, base __ti12ResultArcade 0x004B1DF0). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf16ResultLinkBattle(void);      /* 0x001765E8 */
}
#endif

/* vtable 0x00326F48 (10 slots).  Size 0x954 (RaceLinkBattle: the next
 * member is at +0x2A34C).  The constructor and the destructor are
 * implicit. */
class ResultLinkBattle : public ResultArcade {
public:
    /* [0] +0x0C 0x00176638 ~ResultLinkBattle: implicit (the copy for the
     * vtable follows the typeinfo function). */
    /* ResultArcade's start; started = 0, waitTimer and timer 20 s. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x00176478 */
    /*           [2] +0x1C ResultArcade::cleanup */
    /* ResultArcade's update; after the first goal waitTimer runs down, and
     * once it is below 0 finish() unless done. */
    virtual void update(RaceOrganization &org, Control *pad);  /* [3] +0x24 0x001764B0 */
    /*           [4] +0x2C ResultArcade::render */
    virtual void goal(RaceOrganization &org, int car);          /* [5] +0x34 0x001765C8 started = 1,
                                                                   ResultArcade's goal */
    /*           [6] +0x3C ResultArcade::finish, [7] +0x44 RaceResultBase::isPlayer */
    /* Once the board is in place: the timer runs when every car is in or
     * the wait is over; below 0 posts event 1.  `skip' is ignored. */
    virtual void waitEnd(RaceOrganization &org, int skip);      /* [8] +0x4C 0x00176530 */
    /*           [9] +0x54 ResultArcade::setRowResult */

    f32 waitTimer;                          /* 0x94C after the first goal: the time left for the others */
    int started;                            /* 0x950 a car reached the goal */
};
EGCS_SIZE(ResultLinkBattle, 0x954);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti16ResultLinkBattle[3];       /* 0x004B1E50 (__si_type_info; u041: g_004B1E50) */
}
#endif

#endif /* GAME_T191_RESULT_LINK_BATTLE_H */
