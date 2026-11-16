/* T546_op_delete_arr.h - operator delete[](void *) (translation unit T546,
 * unit u136).  egcs libgcc2 new2.cc (L_op_vdel), 0x0027C260..0x0027C2D7: the
 * same `if (ptr) free(ptr);` as operator delete (it calls free directly, not
 * ::operator delete).  C++ only: a C file sees nothing here.
 *
 * throw(): the original specification (same landing-pad shape as T544). */
#ifndef GAME_T546_OP_DELETE_ARR_H
#define GAME_T546_OP_DELETE_ARR_H

#include "game/game.h"

#ifdef __cplusplus

void operator delete[](void *ptr) throw();              /* 0x0027C260 */

#endif /* __cplusplus */

#endif /* GAME_T546_OP_DELETE_ARR_H */
