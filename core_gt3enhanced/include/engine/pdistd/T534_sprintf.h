/* T534_sprintf.h - sprintf (translation unit T534, unit u125; no EH regions). */
#ifndef GAME_T534_SPRINTF_H
#define GAME_T534_SPRINTF_H

#include "game/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Varargs shim over vsprintf (T535).  No bounds check. */
int sprintf(char *buf, const char *format, ...);    /* 0x0026C718 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_T534_SPRINTF_H */
