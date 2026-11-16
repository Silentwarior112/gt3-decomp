/* T535_vsprintf.h - vsprintf (vsprintf.cxx of Polyphony's pdistd; translation unit T535, unit u125).
 *
 * vsprintf formats through pdistd::PrintFormat (T539) into a stack
 * PrintTarget that writes straight into the caller's buffer (no bounds
 * check), then stores the NUL with a direct putc(0).
 *
 * PrintTarget is a class of the file's anonymous namespace
 * ("_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r"), so it is defined in vsprintf.cxx
 * and not here.  Base pdistd::PrintFormatTargetBase (T539), size 0x8:
 *   +0x00 vptr = _vt$Q235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget 0x00347730
 *   +0x04 char *cur (next byte to write)
 *   PrintTarget(char *buffer)             0x0026C750
 *   [0] +0x0C  ~PrintTarget()             0x0026C798
 *   [1] +0x14  int putc(int c)            0x0026C7C0
 * RTTI name 0x00347748, __si_type_info node 0x004B2B20.
 */
#ifndef GAME_T535_VSPRINTF_H
#define GAME_T535_VSPRINTF_H

#include "game/game.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns PrintFormat's count (without the NUL). */
int vsprintf(char *buf, const char *format, va_list args);   /* 0x0026C7D8 */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)
/* __tfQ235_GLOBAL_$N$.._.._vsprintf.cxxUhxD9r11PrintTarget: the symbol has
 * '$' and '.', so the modern build spells it this way; egcs generates it. */
void *tf_vsprintf_PrintTarget(void);                         /* 0x0026C868 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* GAME_T535_VSPRINTF_H */
