/* T004_game_status_holder.h - GameStatusHolder (translation unit T004, unit
 * u001), 0x00101F78..0x00102088.
 *
 * Holds a second gt2k_game::GameStatus: the guest's, which the arcade menu
 * (T015, nextMode 0x1C) and GT mode (T038, nextMode 0x1D) load from a
 * memory card (F_0014AE00 into get()); its garage is the arcade "guest
 * garage" class (6).  game_init (T007) recreates it (release, create).  The
 * one object is g_00350B30 (gp-0x6EC0, file-local: the others call
 * getGameStatusHolder). */
#ifndef GAME_T004_GAME_STATUS_HOLDER_H
#define GAME_T004_GAME_STATUS_HOLDER_H

#include "game/game.h"

namespace gt2k_game {
class GameStatus;                           /* T429 (pointers only) */
}

/* Not polymorphic.  Size 8. */
class GameStatusHolder {
public:
    GameStatusHolder();                     /* 0x00101F78 */
    void release();                         /* 0x00101F88 loaded 0, deletes status */
    void create();                          /* 0x00101FD0 a new GameStatus (0x35210 bytes) and its init(),
                                               unless there is one */
    gt2k_game::GameStatus *get();           /* 0x00102050 */

    int                    loaded;          /* 0x00 set to 1 by the guest load (T015, T038) */
    gt2k_game::GameStatus *status;          /* 0x04 */
};
EGCS_SIZE(GameStatusHolder, 8);

GameStatusHolder *getGameStatusHolder(void);    /* 0x00102058 &g_00350B30 */
/* 0x00102068 (.ctors 0x0028F028): egcs's static initialiser of g_00350B30
 * (_GLOBAL_$I$__16GameStatusHolder, T004), GameStatusHolder() on it. */

#endif /* GAME_T004_GAME_STATUS_HOLDER_H */
