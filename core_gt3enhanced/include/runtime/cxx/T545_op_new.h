/* T545_op_new.h - operator new(size_t) (translation unit T545, unit u136).
 * egcs libgcc2 new1.cc (L_op_new), 0x0027C138..0x0027C25F: malloc(sz ? sz : 1);
 * on failure call __new_handler (T550) and retry, or throw bad_alloc when no
 * handler is installed (nothing in the game installs one).  The size passed
 * by a new-expression is the object size (sizeof the class).  C++ only: a C
 * file sees nothing here.
 *
 * throw(bad_alloc) is the original specification (egcs declares operator new
 * so implicitly too): its landing pads compute {&__tf9bad_alloc()} and call
 * __check_eh_spec(1, spec).  GCC 14 only deprecates it; the warning is kept
 * out of the includers. */
#ifndef GAME_T545_OP_NEW_H
#define GAME_T545_OP_NEW_H

#include "game/game.h"

#ifdef __cplusplus

#include <stddef.h>
#include "runtime/cxx/T550_new.h"                      /* bad_alloc (exception specification) */

#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

void *operator new(size_t sz) throw(bad_alloc);         /* 0x0027C138 */

#ifndef BOOT_OLD_GCC
#pragma GCC diagnostic pop
#endif

#endif /* __cplusplus */

#endif /* GAME_T545_OP_NEW_H */
