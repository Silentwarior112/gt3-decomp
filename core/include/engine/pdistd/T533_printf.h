/* T533_printf.h - printf (printf.cxx of Polyphony's pdistd; translation unit T533, unit u125).
 *
 * Same machine code as the bootstrap's src/pdistd/printf.cpp.  Output path:
 * printf -> vprintf (0x0026C5F0) -> pdistd::PrintFormat (T539) into a stack
 * PrintTarget -> PrintTarget::flush -> printf_to_deci2 (G003).
 *
 * PrintTarget is a class of the file's anonymous namespace
 * ("_GLOBAL_$N$.._.._printf.cxxQakP7f"), so it is defined in printf.cxx and
 * not here.  Base pdistd::PrintFormatTargetBase (T539), size 0x10:
 *   +0x00 vptr = _vt$Q233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget 0x003476E0
 *   +0x04 char *buffer, +0x08 int capacity (size - 1), +0x0C int length
 *   PrintTarget(char *buffer, int size)  0x0026C4C0
 *   [0] +0x0C  ~PrintTarget()             0x0026C520
 *   [1] +0x14  int putc(int c)            0x0026C548
 *   void flush()                          0x0026C5A8
 * RTTI name 0x003476F8, __si_type_info node 0x004B2B10.
 */
#ifndef GAME_T533_PRINTF_H
#define GAME_T533_PRINTF_H

#include "game/game.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The C-library printf of this binary (game debug output, Allocator::debug,
 * Sony libpad / libmc / libkernl). */
int printf(const char *format, ...);            /* 0x0026C690 */
/* Formats through a stack PrintTarget over a 256-byte buffer, flushes the
 * tail; returns PrintFormat's count (-1 on sink failure).  Caller: printf.
 * (Also in libc.h, with the bootstrap address.) */
int vprintf(const char *format, va_list args);  /* 0x0026C5F0 */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)
/* __tfQ233_GLOBAL_$N$.._.._printf.cxxQakP7f11PrintTarget: the symbol has '$'
 * and '.', so the modern build spells it this way; egcs generates it. */
void *tf_printf_PrintTarget(void);              /* 0x0026C6C8 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* GAME_T533_PRINTF_H */
