/* T004 - GameStatusHolder, the holder of the guest gt2k_game::GameStatus
 * (g_00350B30, gp-0x6EC0).  0x00101F78..0x00102088. */
#include "game/app/T004_game_status_holder.h"
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus (new, init, delete) */

/* .rodata: only the type names of the garage classes the GameStatus holds
 * (egcs writes them for every class it sees whose virtual functions are all
 * inline), 0x00308858..0x00308940 after T003's; no code references them. */
// DATA_START 0x00308858

// FUNC 0x00101F78 GameStatusHolder::GameStatusHolder
GameStatusHolder::GameStatusHolder()
{
    loaded = 0;
    status = NULL;
}

/* delete = GameStatus's virtual destructor (slot 1) with flags 3. */
// FUNC 0x00101F88 GameStatusHolder::release
void GameStatusHolder::release()
{
    loaded = 0;
    if (status != NULL) {
        delete status;
        status = NULL;
    }
}

// FUNC 0x00101FD0 GameStatusHolder::create
void GameStatusHolder::create()
{
    if (status == NULL) {
        /* operator new(0x35210) (0x0027C138), GameStatus() (0x00218668;
         * operator delete 0x0027C0C0 if it throws), then init(). */
        /* implicit: operator new, operator delete (new-expression) */
        status = new gt2k_game::GameStatus;
        status->init();
    }
}

// FUNC 0x00102050 GameStatusHolder::get
gt2k_game::GameStatus *GameStatusHolder::get()
{
    return status;
}

/* The one holder (.sdata 0x00350B30, gp-0x6EC0: a public object with a
 * constructor goes to .data / .sdata, zero-filled). */
GameStatusHolder g_00350B30;                /* 0x00350B30 gp-0x6EC0 */

// FUNC 0x00102058 getGameStatusHolder
GameStatusHolder *getGameStatusHolder(void)
{
    return &g_00350B30;
}

/* Compiler-generated: egcs's static initialiser of g_00350B30 (.ctors entry
 * 0x0028F028), GameStatusHolder() on it, emitted at the end of the unit. */
// FUNC 0x00102068 T004_static_init (_GLOBAL_$I$__16GameStatusHolder)
/* implicit: GameStatusHolder (the constructor) */
