/* T192_result_2p_battle.h - Result2PBattle, the result board of a split
 * screen battle (translation unit T192, unit u041), 0x00176690..0x001767E0.
 *
 * No player: start sets player = -1 and every entry counts as one
 * (isPlayer); it leaves when ResultArcade's timer runs out (the pad is
 * ignored).  Held by value by RaceSplitBattle (+0x29220, T137). */
#ifndef GAME_T192_RESULT_2P_BATTLE_H
#define GAME_T192_RESULT_2P_BATTLE_H

#include "game/game.h"
#include "game/menu/records/T117_result_arcade.h"        /* base ResultArcade */

class type_info;
class RaceOrganization;                     /* T125 (references only) */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E60 (name 0x00327198, base __ti12ResultArcade 0x004B1DF0). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf14Result2PBattle(void);        /* 0x00176738 */
}
#endif

/* vtable 0x00327140 (10 slots).  No data of its own: size 0x94C
 * (RaceSplitBattle: the next member is at +0x29B6C).  The constructor and
 * the destructor are implicit. */
class Result2PBattle : public ResultArcade {
public:
    /* [0] +0x0C 0x00176788 ~Result2PBattle: implicit (the copy for the
     * vtable follows the typeinfo function). */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x00176690 ResultArcade's
                                                                   start, player = -1 */
    /*           [2] +0x1C .. [6] +0x3C as ResultArcade */
    virtual int  isPlayer(int car);         /* [7] +0x44 0x001766C0 1: every entry is a player */
    /* Once the board is in place the timer runs down; below 0 posts event
     * 1.  `skip' is ignored. */
    virtual void waitEnd(RaceOrganization &org, int skip);      /* [8] +0x4C 0x001766C8 */
    /*           [9] +0x54 ResultArcade::setRowResult */
};
EGCS_SIZE(Result2PBattle, 0x94C);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti14Result2PBattle[3];         /* 0x004B1E60 (__si_type_info; u041: g_004B1E60) */
}
#endif

#endif /* GAME_T192_RESULT_2P_BATTLE_H */
