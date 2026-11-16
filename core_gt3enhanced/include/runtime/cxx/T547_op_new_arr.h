/* T547_op_new_arr.h - operator new[](size_t) (translation unit T547, unit
 * u136).  egcs libgcc2 new1.cc (L_op_vnew), 0x0027C2D8..0x0027C34F: forwards
 * to ::operator new (0x0027C138, T545).  C++ only: a C file sees nothing here.
 *
 * throw(bad_alloc): the original specification (landing pads call
 * __check_eh_spec(1, {&__tf9bad_alloc()})).  GCC 14 only deprecates it; the
 * warning is kept out of the includers. */
#ifndef GAME_T547_OP_NEW_ARR_H
#define GAME_T547_OP_NEW_ARR_H

#include "game/game.h"

#ifdef __cplusplus

#include <stddef.h>
#include "runtime/cxx/T550_new.h"                      /* bad_alloc (exception specification) */

#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

void *operator new[](size_t sz) throw(bad_alloc);       /* 0x0027C2D8 */

#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic pop
#endif

#endif /* __cplusplus */

#endif /* GAME_T547_OP_NEW_ARR_H */
