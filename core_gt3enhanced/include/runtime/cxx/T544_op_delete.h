/* T544_op_delete.h - operator delete(void *) (translation unit T544, unit u136).
 * egcs libgcc2 new2.cc (L_op_delete), 0x0027C0C0..0x0027C137: `if (ptr)
 * free(ptr);`.  Called by every deleting destructor of the game (the egcs
 * destructor's hidden flags bit 0).  C++ only: a C file sees nothing here.
 *
 * The throw() specification is the original's: it is what generated the
 * landing pads 0x0027C0D8.. (__start_cp_handler, __check_eh_spec(0, ...),
 * __cp_pop_exception, __throw, std::terminate). */
#ifndef GAME_T544_OP_DELETE_H
#define GAME_T544_OP_DELETE_H

#include "game/game.h"

#ifdef __cplusplus

void operator delete(void *ptr) throw();                /* 0x0027C0C0 */

#endif /* __cplusplus */

#endif /* GAME_T544_OP_DELETE_H */
